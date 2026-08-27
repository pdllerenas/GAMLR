import subprocess
import time

import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import gamma, linregress

CLIENT_BIN = '/home/hasimoto/Llerenas/GAMLR/build/src/delay_client'
HOST = '148.207.185.30'
PORT = '7500'

client_process = subprocess.run(
    [CLIENT_BIN, HOST, PORT], capture_output=True, text=True
)

output = client_process.stdout

if client_process.returncode != 0:
    print(f"Client execution failed:\n{client_process.stderr}")
    sys.exit(1)

owd_samples = [float(match) for match in re.findall(r"OWD\[\d+\] = ([-0-9.]+)", output)]
rho_match = re.search(r"rho:\s*([-0-9.]+)", output)
beta_match = re.search(r"beta:\s*([-0-9.]+)", output)
offset_match = re.search(r"Local Offset \(gamma\):\s*([-0-9.]+)", output)
symmetrical_match = re.search(r"Symmetrical Offset:\s*([-0-9.]+)", output)

if not owd_samples or not rho_match or not beta_match or not offset_match or not symmetrical_match:
    print("Error: Could not parse all required values from the output.")
    sys.exit(0)

rho = float(rho_match.group(1))
beta = float(beta_match.group(1))
local_offset = float(offset_match.group(1))
symmetrical_offset = float(symmetrical_match.group(1))
adjusted_offset = local_offset + symmetrical_offset
adjusted_owd = np.array(owd_samples) + symmetrical_offset

mean_owd = np.mean(adjusted_owd)
shift = mean_owd - (rho * beta)

# === QQ-Plot ===
sorted_owd = np.sort(adjusted_owd)
p = np.arange(1, len(sorted_owd) + 1) / (len(sorted_owd) + 1)
theoretical_quantiles = gamma.ppf(p, a=rho, scale=1)
slope, intercept, r_value, p_value, std_err = linregress(
    sorted_owd, theoretical_quantiles
)
qq_min_owd = -intercept / slope
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

# === Generate the Gamma Distribution ===
x_range = max(sorted_owd) - min(shift, adjusted_offset)
if x_range <= 0:
    x_range = 10.0
x_min = min(shift, adjusted_offset) - (x_range * 0.1)
x_max = max(adjusted_owd) + (x_range * 0.3)
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
    sorted_owd,
    ymin=0,
    ymax=max(pdf) * 0.1,
    colors="red",
    linestyles="solid",
    label="Observed OWD Packets",
    linewidth=2,
)

ax1.axvline(
    x=adjusted_offset,
    color="green",
    linestyle="--",
    linewidth=2.5,
    label=f"Calculated Offset ({adjusted_offset:.4f} ms)",
)

ax1.set_title("Network Delay Estimation: Shifted Gamma Distribution vs Observations")
ax1.set_xlabel("One-Way Delay (ms)")
ax1.set_ylabel("Probability Density")
ax1.legend()
ax1.grid(True, alpha=0.4)
ax1.set_xlim(min(shift, adjusted_offset) - (beta * 0.5), x_max)

ax2.plot(sorted_owd, theoretical_quantiles, "ro", label="Empirical vs Theoretical")

x_line = np.linspace(qq_min_owd, max(sorted_owd), 100)
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
