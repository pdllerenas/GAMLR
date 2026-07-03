import os
import re
import subprocess
import sys
import time

import matplotlib.pyplot as plt
import numpy as np
import paramiko
from dotenv import load_dotenv
from scipy.stats import gamma, linregress

load_dotenv("../.env")
LOCAL_HOST = "127.0.0.1"
# HOST = "127.0.0.1"  # Localhost
# HOST = "148.207.185.20"  # MOLOTE PUBLIC IP
# HOST = 10.102.1.46 # MOLOTE PRIVATE IP
HOST = os.getenv("HOST")
if not HOST:
    raise ValueError("HOST environment variable not set")

PORT = os.getenv("UDP_PORT")
if not PORT:
    raise ValueError("PORT environment variable not set")

SERVER_BIN = os.getenv("SERVER_BIN")
if not SERVER_BIN:
    raise ValueError("SERVER_BIN environment variable not set")

CLIENT_BIN = os.getenv("CLIENT_BIN")
if not CLIENT_BIN:
    raise ValueError("CLIENT_BIN environment variable not set")

if HOST != LOCAL_HOST:
    SSH_HOST = os.getenv("SSH_HOST")
    if not SSH_HOST:
        raise ValueError("SSH_HOST environment variable not set")
    SSH_USER = os.getenv("SSH_USER")
    if not SSH_USER:
        raise ValueError("SSH_USER environment variable not set")
    SSH_PASSWORD = os.getenv("SSH_PASS")
    if not SSH_PASSWORD:
        raise ValueError("SSH_PASSWORD environment variable not set")
    print(f"Connecting to {SSH_HOST} as {SSH_USER}...")
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_client.connect(SSH_HOST, username=SSH_USER, password=SSH_PASSWORD, port=2235)

    print(f"Starting UDP Server on {HOST}:{PORT}...")
    stdin, stdout, stderr = ssh_client.exec_command(f"{SERVER_BIN} {PORT}")
    print(f"Running UDP Client against {HOST}:{PORT}...")
    # run() executes the client and blocks until it finishes
    client_process = subprocess.run(
        [CLIENT_BIN, HOST, PORT], capture_output=True, text=True
    )
    _ = ssh_client.exec_command("pkill -f delay_server")
    ssh_client.close()
else:
    print(f"Starting UDP Server on {HOST}:{PORT}...")
    server_process = subprocess.Popen(
        [SERVER_BIN, PORT], stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )

    time.sleep(1)

    print(f"Running UDP Client against {HOST}:{PORT}...")
    # run() executes the client and blocks until it finishes
    client_process = subprocess.run(
        [CLIENT_BIN, HOST, PORT], capture_output=True, text=True
    )

    server_process.terminate()
    _ = server_process.wait()

# === Parse Output ===
output = client_process.stdout

print(output.strip())

if client_process.returncode != 0:
    print(f"Client execution failed:\n{client_process.stderr}")
    sys.exit(1)

owd_samples = [float(match) for match in re.findall(r"OWD\[\d+\] = ([-0-9.]+)", output)]
rho_match = re.search(r"rho:\s*([-0-9.]+)", output)
beta_match = re.search(r"beta:\s*([-0-9.]+)", output)
offset_match = re.search(r"Local Offset \(gamma\):\s*([-0-9.]+)", output)


if not owd_samples or not rho_match or not beta_match or not offset_match:
    print("Error: Could not parse all required values from the output.")
    sys.exit(0)

rho = float(rho_match.group(1))
beta = float(beta_match.group(1))
estimated_offset = float(offset_match.group(1))

mean_owd = np.mean(owd_samples)
shift = mean_owd - (rho * beta)

# === QQ-Plot ===
sorted_owd = np.sort(owd_samples)
p = np.arange(1, len(sorted_owd) + 1) / (len(sorted_owd) + 1)
theoretical_quantiles = gamma.ppf(p, a=rho, scale=1)
slope, intercept, r_value, p_value, std_err = linregress(
    sorted_owd, theoretical_quantiles
)
qq_min_owd = -intercept / slope
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

# === Generate the Gamma Distribution ===
x_range = max(owd_samples) - min(shift, estimated_offset)
if x_range <= 0:
    x_range = 10.0
x_min = min(shift, estimated_offset) - (x_range * 0.1)
x_max = max(owd_samples) + (x_range * 0.3)
x = np.linspace(x_min, x_max, 1000)

pdf = gamma.pdf(x, a=rho, loc=shift, scale=beta)

# Plot the theoretical curve
ax1.plot(
    x,
    pdf,
    "b-",
    lw=2,
    label=f"Theoretical Shifted Gamma\n(rho={rho:.4f}, beta={beta:.4f}, shift={shift:.4f})",
)

ax1.vlines(
    owd_samples,
    ymin=0,
    ymax=max(pdf) * 0.1,
    colors="red",
    linestyles="solid",
    label="Observed OWD Packets",
    linewidth=2,
)

ax1.axvline(
    x=estimated_offset,
    color="green",
    linestyle="--",
    linewidth=2.5,
    label=f"Calculated Offset ({estimated_offset:.4f} ms)",
)

ax1.set_title("Network Delay Estimation: Shifted Gamma Distribution vs Observations")
ax1.set_xlabel("One-Way Delay (ms)")
ax1.set_ylabel("Probability Density")
ax1.legend()
ax1.grid(True, alpha=0.4)
ax1.set_xlim(min(shift, estimated_offset) - (beta * 0.5), x_max)

ax2.plot(sorted_owd, theoretical_quantiles, "ro", label="Empirical vs Theoretical")

x_line = np.linspace(qq_min_owd, max(owd_samples), 100)
y_line = slope * x_line + intercept

ax2.plot(x_line, y_line, "b-", label=f"Linear Fit (R²={r_value**2:.4f})")

ax2.plot(
    qq_min_owd,
    0,
    "go",
    markersize=8,
    label=f"X-Intercept (Min OWD = {qq_min_owd:.4f} ms)",
)
ax2.axvline(x=qq_min_owd, color="green", linestyle=":", alpha=0.6)
ax2.axhline(y=0, color="black", linestyle="-", linewidth=1)

ax2.set_title("QQ-Plot: Extrapolating the True Minimum OWD")
ax2.set_xlabel("Empirical Observed OWD (ms)")
ax2.set_ylabel("Theoretical Gamma Quantiles")
ax2.legend()
ax2.grid(True, alpha=0.4)

plt.tight_layout()
plt.savefig("gamma_distribution.png")
