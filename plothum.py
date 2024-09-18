# time, ip, tag, temp, hum, temp2, pressure, height on sea
#34537940 192.168.100.2 HTPA 17.1 51 17.4 966 409
#34598182 192.168.100.2 HTPA 17.1 51 17.4 966 409
#34658431 192.168.100.2 HTPA 17.1 51 17.4 966 408
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
import pandas as pd

# Read the data
data = pd.read_csv('hum', sep=' ', comment='#', 
                   names=['time', 'ip', 'tag', 'temp', 'hum', 'temp2', 'pressure', 'height'])

# Convert time to datetime (time is in milliseconds)
data['datetime'] = pd.to_datetime(data['time'], unit='ms')

# Calculate hours since start
start_time = data['datetime'].min()
data['hours_since_start'] = (data['datetime'] - start_time).dt.total_seconds() / 3600

# Calculate dewpoint
def calculate_dewpoint(temp, hum):
    a = 17.27
    b = 237.7
    alpha = ((a * temp) / (b + temp)) + np.log(hum/100.0)
    return (b * alpha) / (a - alpha)

data['dewpoint'] = calculate_dewpoint(data['temp'], data['hum'])

# Create a 3x2 subplot figure
fig, axs = plt.subplots(3, 2, figsize=(15, 20))
fig.suptitle('Environmental Measurements Over Time and vs Altitude', fontsize=16)

# Plot vs Time
axs[0, 0].plot(data['hours_since_start'], data['temp'], label='Temperature')
axs[0, 0].set_ylabel('Temperature (°C)')
axs[0, 0].legend()

axs[1, 0].plot(data['hours_since_start'], data['hum'], label='Humidity')
axs[1, 0].set_ylabel('Humidity (%)')
axs[1, 0].legend()

axs[2, 0].plot(data['hours_since_start'], data['dewpoint'], label='Dewpoint')
axs[2, 0].set_ylabel('Dewpoint (°C)')
axs[2, 0].legend()

axs[0, 1].plot(data['hours_since_start'], data['pressure'], label='Pressure')
axs[0, 1].set_ylabel('Pressure (hPa)')
axs[0, 1].legend()

axs[1, 1].plot(data['hours_since_start'], data['height'], label='Altitude')
axs[1, 1].set_ylabel('Altitude (m)')
axs[1, 1].legend()

axs[2, 1].plot(data['hours_since_start'], data['temp'] - data['dewpoint'], label='Temperature - Dewpoint')
axs[2, 1].set_ylabel('Temperature - Dewpoint (°C)')
axs[2, 1].legend()

for ax in axs[:, 0]:
    ax.set_xlabel('Hours since start')
for ax in axs[:, 1]:
    ax.set_xlabel('Hours since start')

# Create a new figure for scatter plots vs Altitude
fig2, axs2 = plt.subplots(2, 3, figsize=(20, 15))
fig2.suptitle('Environmental Measurements vs Altitude', fontsize=16)

axs2[0, 0].scatter(data['height'], data['temp'], alpha=0.5)
axs2[0, 0].set_xlabel('Altitude (m)')
axs2[0, 0].set_ylabel('Temperature (°C)')

axs2[0, 1].scatter(data['height'], data['hum'], alpha=0.5)
axs2[0, 1].set_xlabel('Altitude (m)')
axs2[0, 1].set_ylabel('Humidity (%)')

axs2[0, 2].scatter(data['height'], data['dewpoint'], alpha=0.5)
axs2[0, 2].set_xlabel('Altitude (m)')
axs2[0, 2].set_ylabel('Dewpoint (°C)')

axs2[1, 0].scatter(data['height'], data['pressure'], alpha=0.5)
axs2[1, 0].set_xlabel('Altitude (m)')
axs2[1, 0].set_ylabel('Pressure (hPa)')

# Plot the difference between temperature and dewpoint
axs2[1, 1].scatter(data['height'], data['temp'] - data['dewpoint'], alpha=0.5)
axs2[1, 1].set_xlabel('Altitude (m)')
axs2[1, 1].set_ylabel('Temperature - Dewpoint (°C)')

# Remove the unused subplot
fig2.delaxes(axs2[1, 2])

plt.tight_layout()
plt.show()
