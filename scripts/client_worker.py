import subprocess
import time
import sys

def main(host, port, client_bin):
    # CLIENT_BIN = '/home/pdllerenas/dev/GAMLR/build/src/delay_client'
    # HOST = '148.207.185.20'
    # PORT = '7500'
    HOST = host
    PORT = port
    CLIENT_BIN = client_bin

    EXECUTIONS_PER_HOUR = 12

    SLEEP_TIME = 60 * 60 / EXECUTIONS_PER_HOUR

    execution_count = 0
    max_count = 12 * 24

    while (execution_count < max_count):
        client_process = subprocess.run(
            [CLIENT_BIN, HOST, PORT], capture_output=True, text=True
        )

        output = client_process.stdout

        if client_process.returncode != 0:
            print(f"Client execution failed:\n{client_process.stderr}")
            continue
        else:
            print("Wrote values to experiment.log")

        execution_count += 1
        time.sleep(SLEEP_TIME)

if "__main__" == __name__:
    argv = sys.argv
    if len(argv) != 4:
        print(f"Invalid argument count. Usage: {argv[0]} <IP> <Port> <Executable>")
        sys.exit(1)

    main(argv[1], argv[2], argv[3])
