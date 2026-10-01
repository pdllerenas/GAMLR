import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.lines import lineMarkers

def plot_client(filename):
    df = pd.read_csv(filename)
    df["timestamp"] = pd.to_datetime(df["timestamp"], format="%Y-%m-%d_%H-%M-%S")

    rho_mean = df['rho'].mean()
    beta_mean = df['beta'].mean()
    min_owd_mean = df['min_owd'].mean()
    owd1_mean = df['owd1'].mean()
    owd2_mean = df['owd2'].mean()
    owd3_mean = df['owd3'].mean()
    owd4_mean = df['owd4'].mean()
    owd5_mean = df['owd5'].mean()

    print(f"rho mean: {rho_mean}, beta mean: {beta_mean}, min_owd mean: {min_owd_mean}, owd1 mean: {owd1_mean}, owd2 mean: {owd2_mean}, owd3 mean: {owd3_mean}, owd4 mean: {owd4_mean}, owd5 mean: {owd5_mean}")

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 8), sharex=True)
    fig.suptitle(f"{filename}")

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

    ax3.plot(df["timestamp"], df["min_owd"], color="tab:green", label="min_owd", linestyle="-", linewidth=1)
    ax3.plot(df["timestamp"], df["owd1"], label="owd1", linestyle="-", linewidth=1, alpha=0.8)
    ax3.plot(df["timestamp"], df["owd2"], label="owd2", linestyle="-", linewidth=1, alpha=0.8)
    ax3.plot(df["timestamp"], df["owd3"], label="owd3", linestyle="-", linewidth=1, alpha=0.8)
    ax3.plot(df["timestamp"], df["owd4"], label="owd4", linestyle="-", linewidth=1, alpha=0.8)
    ax3.plot(df["timestamp"], df["owd5"], label="owd5", linestyle="-", linewidth=1, alpha=0.8)

    ax3.set_title("owd values over time")
    ax3.set_ylabel("owd")
    ax3.set_xlabel("Timestamp")
    ax3.grid(True, linestyle="--", alpha=0.7)
    ax3.legend(loc="upper right", fontsize="small", ncol=2) # Added a legend for the multiple lines

    plt.xticks(rotation=45)
    plt.tight_layout()

    plt.savefig(f"{filename}.png")
    # plt.show()


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
    # plt.show()


def main():
    # plot_data("molote-mty-data.csv")
    plot_client("mty-500B-30ms-data.csv")

if __name__ == "__main__":
    main()
