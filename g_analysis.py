import numpy as np

# open a.txt and b.txt with format
# 23587 192.168.100.2 A 1.00  1.00 20575 0.04 0.10 0.99
# field 6 (counting from 1) is time in ms and the sum in quadrature of fields 7, 8, and 9 is the amplitude
#open both files
a = open('../a83.txt', 'r')
b = open('../b83.txt', 'r')
#a = open('../a.txt', 'r')
#b = open('../b.txt', 'r')
# use numpy to read the data
# a_data = np.loadtxt(a)
# b_data = np.loadtxt(b)
# ignore string columns 2 and 3
a_data = np.loadtxt(a, usecols=(0, 5, 6, 7, 8))
b_data = np.loadtxt(b, usecols=(0, 5, 6, 7, 8))

# define the time and amplitude
a_time = a_data[:,1]/1000
#sqrt of sum of squares of 3rd to 5th columns
a_amplitude = np.sqrt(np.sum(a_data[:,2:5]**2, axis=1))
b_time = b_data[:,1]/1000
b_amplitude = np.sqrt(np.sum(b_data[:,2:5]**2, axis=1))
#plot the data
import matplotlib.pyplot as plt
#time on x axis
#split figure in 6 parts
plt.subplot(231)
#plot in first pad

plt.plot(a_time, a_amplitude)
plt.plot(b_time, b_amplitude)



# non-equispaced fast Fourier transform
#f = nfft(x, f_k)
#make sure the vector have even number of elements
a_time = a_time[0:len(a_time)//2*2]
a_amplitude = a_amplitude[0:len(a_amplitude)//2*2]
b_time = b_time[0:len(b_time)//2*2]
b_amplitude = b_amplitude[0:len(b_amplitude)//2*2]

# make a spectrogram with non uniform time sampling
#interpolate and resample the data
from scipy.interpolate import interp1d
f_a = interp1d(a_time, a_amplitude)
f_b = interp1d(b_time, b_amplitude)
#make linearly spaced time vector
xmax= min(a_time[-2], b_time[-2])
xmin= max(a_time[0], b_time[0])
x = np.linspace(xmin,xmax, int((xmax-xmin)*250))


#compare interpolated data with original data

#plot in position 2

plt.subplot(232)

#scatter plot t, amp
plt.scatter(a_time, a_amplitude)
plt.scatter(x, f_a(x))

plt.subplot(233)
#plt.plot(b_time, b_amplitude)
#plt.plot(x, f_b(x))
plt.scatter(b_time, b_amplitude)
plt.scatter(x, f_b(x))

#spectrogram
from scipy import signal
import matplotlib
#spectrogram with interpolated data

f, t, Sxxa = signal.spectrogram(f_a(x),250, nperseg=256)
#Sxxa=Sxxa*f.reshape(-1,1) 
minlog=0.0001
Sxxa = np.where(Sxxa < minlog, np.log10(minlog), np.log10(Sxxa))
plt.subplot(234)
plt.pcolormesh(t, f, Sxxa)
cb1=plt.colorbar()
plt.ylabel('Frequency [Hz]')
plt.xlabel('Time [sec]')
#show in non blocking mode

#spectrogram with interpolated data b
#new window
#show legend
f, t, Sxxb = signal.spectrogram(f_b(x),100, nperseg=256)
Sxxb = np.where(Sxxb < minlog, np.log10(minlog), np.log10(Sxxb))
plt.subplot(235)
#log z
#Sxx =  np.log10(Sxx)
#reuse colorbar from previous plot
plt.pcolormesh(t, f, Sxxb)
#legend for z axis
cb2=plt.colorbar()
cb2.mappable.set_clim(*cb1.mappable.get_clim())
plt.ylabel('Frequency [Hz]')
plt.xlabel('Time [sec]')

plt.subplot(236)
Sxx=Sxxa-Sxxb
plt.pcolormesh(t, f, Sxx)
plt.colorbar()
plt.ylabel('Frequency [Hz]')
plt.xlabel('Time [sec]')

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