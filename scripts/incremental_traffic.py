import sys
import subprocess
import time

# 100 * t 1400-Byte packets per second, t increments every 30 mins by 1

def main(host: str, port: str, client_bin: str):
    PACKET_SIZE = 1400
    INITIAL_HZ = 100
    RATE_CHANGE = 100
    SLEEP_TIME = 30 * 60 # 30 miunutes

    hz = INITIAL_HZ
    packet_size = PACKET_SIZE

    while True:
        print(f"Starting traffic at {hz} Hz for 1 hour...")

        client_process = subprocess.Popen(
            [client_bin, host, port, str(hz), str(packet_size)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
        )
        time.sleep(SLEEP_TIME)

        client_process.terminate()
        client_process.wait()

        if client_process.returncode not in (0, -15):
            print(f"Client execution had an error:\n{client_process.stderr.read()}")
        else:
            print("Completed 1-hour run")

        hz += RATE_CHANGE

if "__main__" == __name__:
    argv = sys.argv
    if len(argv) != 4:
        print(f"Invalid argument count. Usage: {argv[0]} <IP> <Port> <Executable>")
        sys.exit(1)

    main(argv[1], argv[2], argv[3])
