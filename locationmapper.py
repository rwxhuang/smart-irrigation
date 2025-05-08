import serial
import pandas as pd
import time
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib.ticker as ticker  # add at the top of your file

# df = pd.read_csv("logs/location_data_20250508_135821.csv")

# fig, ax = plt.subplots()
# ax.plot(df["lng"], df['lat'], 'b.-')
# ax.set_title("Irrigation System Map")
# ax.set_xlabel("Longitude")
# ax.set_ylabel("Latitude")

# # Avoid scientific notation
# ax.ticklabel_format(style='plain', axis='both')  # optional fallback
# ax.xaxis.set_major_formatter(ticker.FormatStrFormatter('%.6f'))
# ax.yaxis.set_major_formatter(ticker.FormatStrFormatter('%.6f'))

# plt.tight_layout()  # Adjust layout to prevent clipping
# plt.savefig("test.png")
# plt.show()


save = True

# Set up serial
ser_arduino = serial.Serial('COM3', 115200, timeout=1)
time.sleep(2)

# Initialize DataFrame
df = pd.DataFrame(columns=["Time", "lat", "lng"])
start_time = time.time()

# Set up matplotlib plot
fig, ax = plt.subplots()
scat, = ax.plot([], [], 'bo-', markersize=4)
ax.set_title("Irrigation System Mapping")
ax.set_xlabel("Longitude")
ax.set_ylabel("Latitude")

def update(frame):
    global df

    if ser_arduino.in_waiting > 0:
        try:
            output = ser_arduino.read(ser_arduino.inWaiting()).decode('utf-8')
            if '\n' in output:
                lines = output.strip().split('\n')
                recentdata = lines[-1]
                if not recentdata.startswith("invalid"):
                    lat, lng = map(float, recentdata.split(','))
                    timestamp = time.time() - start_time
                    df.loc[len(df)] = [timestamp, lat, lng]

        except Exception as e:
            print(f"Error: {e}")

    # Update plot
    if not df.empty:
        ax.clear()
        ax.set_title("Irrigation System Map")
        ax.set_xlabel("Longitude")
        ax.set_ylabel("Latitude")

        # Clean up formatting
        ax.ticklabel_format(style='plain', useOffset=False)
        ax.xaxis.set_major_formatter(ticker.FormatStrFormatter('%.6f'))
        ax.yaxis.set_major_formatter(ticker.FormatStrFormatter('%.6f'))

        ax.plot(df["lng"], df["lat"], 'b.-')
        plt.tight_layout()  # Adjust layout to prevent clipping
    return scat,

# Live update interval (milliseconds)
ani = FuncAnimation(fig, update, interval=2000)

try:
    plt.show()
except KeyboardInterrupt:
    print("Stopped by user.")
finally:
    if ser_arduino.is_open:
        ser_arduino.close()
    if save and not df.empty:
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        df.to_csv(f"logs/location_data_{timestamp}.csv", index=False)
        fig.savefig(f"logs/location_plot_{timestamp}.png", dpi=300)
        print(f"Saved data and plot to logs/ as location_data_{timestamp}.csv and .png")
