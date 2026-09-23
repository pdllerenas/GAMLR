import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.lines import lineMarkers

def plot_client(filename):
    df = pd.read_csv(filename)
    df["timestamp"] = pd.to_datetime(df["timestamp"], format="%Y-%m-%d_%H-%M-%S")

    rho_mean = df['rho'].mean()
    beta_mean = df['beta'].mean()
    # remove 'ms' from owd column
    df['owd'] = df['owd'].str.replace('ms', '')
    df['owd'] = df['owd'].astype(float)
    owd_mean = df['owd'].mean()

    print(f"rho mean: {rho_mean}, beta mean: {beta_mean}, owd mean: {owd_mean}")

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 8), sharex=True)

    ax1.plot(df["timestamp"], df["rho"], color="tab:blue", linestyle="-", linewidth=1)
    ax1.set_title("rho value over time")
    ax1.set_ylabel("rho")
    ax1.grid(True, linestyle="--", alpha=0.7)

    ax2.plot(
        df["timestamp"], df["beta"], color="tab:orange", linestyle="-", linewidth=1
    )
    ax2.set_title("beta value over time")
    ax2.set_ylabel("beta")
    ax2.set_xlabel("Timestamp")
    ax2.grid(True, linestyle="--", alpha=0.7)

    ax3.plot(
        df["timestamp"], df["owd"], color="tab:green", linestyle="-", linewidth=1
    )
    ax3.set_title("owd value over time")
    ax3.set_ylabel("owd")
    ax3.set_xlabel("Timestamp")
    ax3.grid(True, linestyle="--", alpha=0.7)

    plt.xticks(rotation=45)
    plt.tight_layout()

    plt.savefig(f"{filename}.png")
    plt.show()


def plot_data(filename):
    df = pd.read_csv(filename)
    df["timestamp"] = pd.to_datetime(df["timestamp"], format="%Y-%m-%d_%H-%M-%S")

    rho_mean = df['rho'].mean()
    beta_mean = df['beta'].mean()

    print(f"rho mean: {rho_mean}, beta mean: {beta_mean}")

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)

    ax1.plot(df["timestamp"], df["rho"], color="tab:blue", linestyle="-", linewidth=1)
    ax1.set_title("rho value over time")
    ax1.set_ylabel("rho")
    ax1.grid(True, linestyle="--", alpha=0.7)

    ax2.plot(
        df["timestamp"], df["beta"], color="tab:orange", linestyle="-", linewidth=1
    )
    ax2.set_title("beta value over time")
    ax2.set_ylabel("beta")
    ax2.set_xlabel("Timestamp")
    ax2.grid(True, linestyle="--", alpha=0.7)

    plt.xticks(rotation=45)
    plt.tight_layout()

    plt.savefig(f"{filename}.png")
    plt.show()


def main():
    plot_data("molote-mty-data.csv")
    plot_client("mty-data.csv")

if __name__ == "__main__":
    main()
