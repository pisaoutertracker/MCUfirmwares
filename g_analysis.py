import numpy as np
from scipy import signal
# open a.txt and b.txt with format
# 23587 192.168.100.2 A 1.00  1.00 20575 0.04 0.10 0.99
# field 6 (counting from 1) is time in ms and the sum in quadrature of fields 7, 8, and 9 is the amplitude
#open both files
a = open('a.txt', 'r')
b = open('b.txt', 'r')
#a = open('../a.txt', 'r')
#b = open('../b.txt', 'r')
# use numpy to read the data
# a_data = np.loadtxt(a)
# b_data = np.loadtxt(b)
# ignore string columns 2 and 3
a_data = np.loadtxt(a, usecols=(0, 5, 6, 7, 8))
b_data = np.loadtxt(b, usecols=(0, 5, 6, 7, 8))

# define the time and amplitude
# Define time and amplitude for both datasets
a_time = a_data[:,1]/1000
b_time = b_data[:,1]/1000

# Calculate the norm of the acceleration vector
a_amplitude = np.sqrt(np.sum(a_data[:,2:5]**2, axis=1))
b_amplitude = np.sqrt(np.sum(b_data[:,2:5]**2, axis=1))

# Extract individual components
a_x, a_y, a_z = a_data[:,2], a_data[:,3], a_data[:,4]
b_x, b_y, b_z = b_data[:,2], b_data[:,3], b_data[:,4]

# Plot the data
import matplotlib.pyplot as plt

# Function to create plots for each component
def create_component_plots(time_a, time_b, data_a, data_b, component_name):
    fig, axs = plt.subplots(2, 3, figsize=(15, 10))
    
    # Time series plot
    axs[0, 0].plot(time_a, data_a)
    axs[0, 0].plot(time_b, data_b)
    axs[0, 0].set_xlabel('Time')
    axs[0, 0].set_ylabel(f'{component_name} Amplitude')
    axs[0, 0].set_title(f'{component_name} Component Time Series')
    
    # Histogram
    bins = np.linspace(min(np.min(data_a), np.min(data_b)), 
                       max(np.max(data_a), np.max(data_b)), 51)
    axs[0, 1].hist(data_a, bins=bins, alpha=0.5, label='A', log=True)
    axs[0, 1].hist(data_b, bins=bins, alpha=0.5, label='B', log=True)
    axs[0, 1].set_xlabel(f'{component_name} Amplitude')
    axs[0, 1].set_ylabel('Count (log scale)')
    axs[0, 1].set_yscale('log')
    axs[0, 1].legend()
    axs[0, 1].set_title(f'{component_name} Component Histogram')
    
    # Calculate and plot max values in 30-second bins for A and B
    bin_size = 30  # seconds
    for time, data, label in [(time_a, data_a, 'A'), (time_b, data_b, 'B')]:
        bins = np.arange(min(time), max(time), bin_size)
        max_values = [np.max(data[(time >= bin_start) & (time < bin_end)]) for bin_start, bin_end in zip(bins[:-1], bins[1:])]
        bin_centers = bins[:-1] + bin_size / 2
        axs[0, 2].plot(bin_centers, max_values, label=f'Signal {label}')
    
    axs[0, 2].set_xlabel('Time (s)')
    axs[0, 2].set_ylabel(f'Max {component_name} Amplitude')
    axs[0, 2].set_title(f'Max {component_name} Component in 30s Bins')
    axs[0, 2].legend()
    
    # Interpolate and resample the data
    from scipy.interpolate import interp1d
    f_a = interp1d(time_a, data_a)
    f_b = interp1d(time_b, data_b)
    # Make linearly spaced time vector
    xmax = min(time_a[-1], time_b[-1])
    xmin = max(time_a[0], time_b[0])
    x = np.linspace(xmin, xmax, int((xmax-xmin)*250))
    
    # Calculate spectrograms
    f, t, Sxxa = signal.spectrogram(f_a(x), fs=250, nperseg=256)
    f, t, Sxxb = signal.spectrogram(f_b(x), fs=250, nperseg=256)
    
    minlog = 0.0001
    Sxxa = np.where(Sxxa < minlog, np.log10(minlog), np.log10(Sxxa))
    Sxxb = np.where(Sxxb < minlog, np.log10(minlog), np.log10(Sxxb))
    
    # Plot spectrogram for a
    im1 = axs[1, 0].pcolormesh(t, f, Sxxa)
    fig.colorbar(im1, ax=axs[1, 0])
    axs[1, 0].set_ylabel('Frequency [Hz]')
    axs[1, 0].set_xlabel('Time [sec]')
    axs[1, 0].set_title(f'{component_name} Component Spectrogram A')
    
    # Plot spectrogram for b
    im2 = axs[1, 1].pcolormesh(t, f, Sxxb)
    fig.colorbar(im2, ax=axs[1, 1])
    axs[1, 1].set_ylabel('Frequency [Hz]')
    axs[1, 1].set_xlabel('Time [sec]')
    axs[1, 1].set_title(f'{component_name} Component Spectrogram B')
    
    # Ensure both colorbars have the same scale
    vmin = min(Sxxa.min(), Sxxb.min())
    vmax = max(Sxxa.max(), Sxxb.max())
    im1.set_clim(vmin, vmax)
    im2.set_clim(vmin, vmax)
    
    # Plot the projection of the two spectrograms on the Y axis
    projection_a = np.sum(Sxxa, axis=1)
    projection_b = np.sum(Sxxb, axis=1)
    axs[1, 2].plot(f, projection_a, label='Signal A')
    axs[1, 2].plot(f, projection_b, label='Signal B')
    axs[1, 2].set_ylabel('Magnitude')
    axs[1, 2].set_xlabel('Frequency [Hz]')
    axs[1, 2].legend()
    axs[1, 2].set_title(f'{component_name} Component Projection of Spectrograms')
    
    # Plot frequency projection in energy units
   





    plt.tight_layout()
    plt.show()

# Create plots for each component and the norm
create_component_plots(a_time, b_time, a_amplitude, b_amplitude, 'Norm')
create_component_plots(a_time, b_time, a_y, b_z, 'Y(a)Z(b) up/down')
create_component_plots(a_time, b_time, a_z, b_y, 'Z(a)Y(b) fwd/back')
create_component_plots(a_time, b_time, a_x, b_x, 'X left/right')



# Plot the difference
# plt.subplot(236)
# Sxx = Sxxa - Sxxb
# plt.pcolormesh(t, f, Sxx)
# plt.colorbar()
# plt.ylabel('Frequency [Hz]')
# plt.xlabel('Time [sec]')

# Plot the projection of the two spectrograms on the Y axis
plt.subplot(236)
projection_a = np.sum(Sxxa, axis=1)
projection_b = np.sum(Sxxb, axis=1)
plt.plot(f, projection_a, label='Signal A')
plt.plot(f, projection_b, label='Signal B')
plt.ylabel('Magnitude')
plt.xlabel('Frequency [Hz]')
plt.legend()
plt.title('Projection of Spectrograms on Frequency Axis')

plt.show()

from scipy.signal import  ShortTimeFFT
from scipy.signal.windows import gaussian
g_std = 100  # standard deviation for Gaussian window in samples
N=len(x)
print(x,N)
T_x = (x[-1] - x[0]  )/N  # time step of signal
w = gaussian(500, std=g_std, sym=True)  # symmetric Gaussian window
SFT = ShortTimeFFT(w, hop=10, fs=1/T_x, mfft=2000, scale_to='magnitude')
Sxa = SFT.stft(f_a(x))
Sxb = SFT.stft(f_b(x))
Sx = abs(Sxa)-abs(Sxb)
fig1, ax1 = plt.subplots(figsize=(6., 4.))  # enlarge plot a bit
t_lo, t_hi = SFT.extent(N)[:2]  # time range of plot
ax1.set_title(rf"STFT ({SFT.m_num*SFT.T:g}$\,s$ Gaussian window, " +
              rf"$\sigma_t={g_std*SFT.T}\,$s)")
ax1.set(xlabel=f"Time $t$ in seconds ({SFT.p_num(N)} slices, " +
               rf"$\Delta t = {SFT.delta_t:g}\,$s)",
        ylabel=f"Freq. $f$ in Hz ({SFT.f_pts} bins, " +
               rf"$\Delta f = {SFT.delta_f:g}\,$Hz)",
        xlim=(t_lo, t_hi))
#cut frequency below 0.1 Hz
#Sx = Sx[SFT.f_pts//10:,:]
im1 = ax1.imshow(Sx, origin='lower', aspect='auto',
                 extent=SFT.extent(N), cmap='viridis')

fig1.colorbar(im1, label="Magnitude $|S_x(t, f)|$")

fig1.tight_layout()
plt.show()
