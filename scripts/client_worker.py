import subprocess
import time
import sys
import datetime
import re

def get_tracepath_hops(target_ip: str, port: str = '7500') -> int:
    command = ["tracepath", "-p", port, target_ip]
    
    try:
        # Run the command, capture stdout directly as a string
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        output = result.stdout
        
        # Log to file
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        with open("hops.log", "a") as log_file:
            log_file.write(f"\n--- Trace to {target_ip} at {timestamp} ---\n")
            log_file.write(output)
            
        # Parse output for the maximum hop count
        max_hop = 0
        for line in output.splitlines():
            # Regex to catch lines starting with spaces, numbers, and then a colon or question mark
            # Matches: "  1:  192.168.1.1" or " 1?: [LOCALHOST]"
            match = re.match(r'^\s*(\d+)(?:\?:|:)', line)
            if match:
                current_hop = int(match.group(1))
                if current_hop > max_hop:
                    max_hop = current_hop
    except:
        print("Error running tracepth")
                    
        return max_hop

def main(host: str, port: str, packet_size: str, client_bin: str):
    HOST = host
    PORT = port
    CLIENT_BIN = client_bin

    EXECUTIONS_PER_HOUR = 12

    SLEEP_TIME = 60 * 60 / EXECUTIONS_PER_HOUR

    TOLERANCE = 2.0

    execution_count = 0
    max_count = 12 * 24

    prev_owd = None

    while (execution_count < max_count):
        client_process = subprocess.run(
            [CLIENT_BIN, HOST, PORT, packet_size], capture_output=True, text=True
        )

        output = client_process.stdout

        if client_process.returncode != 0:
            print(f"Client execution failed:\n{client_process.stderr}")
            continue
        else:
            print("Wrote values to experiment.log")

        match = re.search(r"One-Way Delay:\s*([-0-9.]+)", output)

        if match:
            current_owd = float(match.group(1))
            print(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] Current OWD: {current_owd}")

            if prev_owd is not None:
                owd_diff = abs(prev_owd - current_owd)

                if (owd_diff > TOLERANCE):
                    print("Triggering tracepath...")
                    hops = get_tracepath_hops(HOST, PORT)
                    print(f"Tracepath logged. Max hops reached: {hops}")

            prev_owd = current_owd
        else:
            print(f"Could not parse owd")

        execution_count += 1
        time.sleep(SLEEP_TIME)

if "__main__" == __name__:
    argv = sys.argv
    if len(argv) != 5:
        print(f"Invalid argument count. Usage: {argv[0]} <IP> <Port> <Packet Size> <Executable>")
        sys.exit(1)

    main(argv[1], argv[2], argv[3], argv[4])
