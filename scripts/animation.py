import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import pandas as pd
from scipy.stats import gamma

def create_gamma_animation(filename, output_filename="gamma_animation.gif", step=10):
    print(f"Loading data from {filename}...")
    df = pd.read_csv(filename)
    df["timestamp"] = pd.to_datetime(df["timestamp"], format="%Y-%m-%d_%H-%M-%S")
    
    # Subsample the data to make the animation render faster and keep file size down.
    # A step of 10 means it uses every 10th row. Adjust this based on your dataset size.
    df = df.iloc[::step].reset_index(drop=True)
    frames = len(df)
    
    print(f"Generating animation with {frames} frames...")

    # Calculate global max x-value to keep the x-axis fixed and stable during the animation
    # We find the 99th percentile of the distribution with the highest mean
    max_x = 0
    for _, row in df.iterrows():
        r, b = row['rho'], row['beta']
        # If your beta is a rate parameter, change `scale=b` to `scale=1/b` here:
        val = gamma.ppf(0.99, a=r, scale=b) 
        if pd.notna(val) and val > max_x:
            max_x = val

    x = np.linspace(0, max_x, 1000)

    fig, ax = plt.subplots(figsize=(10, 6))
    line, = ax.plot([], [], lw=2, color="tab:blue")
    
    # Text annotations for the timestamp and parameters
    time_text = ax.text(0.5, 0.95, '', transform=ax.transAxes, ha='center', fontsize=12)
    param_text = ax.text(0.5, 0.90, '', transform=ax.transAxes, ha='center', fontsize=11)
    
    ax.set_xlim(0, max_x)
    ax.set_title("Gamma Distribution Over Time")
    ax.set_xlabel("Value")
    ax.set_ylabel("Probability Density")
    ax.grid(True, linestyle="--", alpha=0.7)

    def init():
        line.set_data([], [])
        time_text.set_text('')
        param_text.set_text('')
        return line, time_text, param_text

    def update(frame):
        row = df.iloc[frame]
        r = row['rho']
        b = row['beta']
        t = row['timestamp']
        
        # If your beta is a rate parameter, change `scale=b` to `scale=1/b` here:
        y = gamma.pdf(x, a=r, scale=b)
        
        # Fill NaN values with 0 just in case math errors occur with extreme parameters
        y = np.nan_to_num(y) 
        
        line.set_data(x, y)
        
        # Dynamically adjust the y-axis to accommodate peaks
        current_max_y = np.max(y)
        if current_max_y > 0:
            ax.set_ylim(0, current_max_y * 1.1)
            
        time_text.set_text(f"Timestamp: {t}")
        param_text.set_text(f"rho (shape): {r:.4f} | beta (scale): {b:.4f}")
        
        return line, time_text, param_text

    # Create the animation
    ani = animation.FuncAnimation(
        fig, update, frames=frames, init_func=init, blit=False, interval=100
    )

    # Save the animation as a GIF
    writer = animation.PillowWriter(fps=10)
    ani.save(output_filename, writer=writer)
    print(f"Animation saved successfully as {output_filename}")

def main():
    # Replace with your actual filename
    create_gamma_animation("merida-500B-30ms-data.csv", output_filename="merida_gamma.gif", step=5)

if __name__ == "__main__":
    main()
