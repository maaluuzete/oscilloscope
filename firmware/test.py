import numpy as np
import matplotlib.pyplot as plt

fs=32000
t = np.arange(0, 0.01, 1/fs)
signal=1.65+np.sin(2*np.pi*1000*t)

vpp = signal.max()-signal.min()
vrms = np.sqrt(np.mean((signal-signal.mean())**2))

print("vpp:", vpp)
print("vrms:", vrms)

plt.plot(t, signal)
plt.show()
