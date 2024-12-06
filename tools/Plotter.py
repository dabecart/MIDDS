import numpy as np
import matplotlib.pyplot as plt

clkFreq = 160e6

deltaMicrosecond = 1e6/clkFreq
print(f'dt = {deltaMicrosecond} us/step')

fig, ax = plt.subplots()

try:
    dataT1 = np.genfromtxt('InputT1.0.csv', delimiter='\n')*deltaMicrosecond
    print(f'T1. Avg {np.average(dataT1)}. Min {np.min(dataT1)} ({np.argmin(dataT1)}). Max {np.max(dataT1)} ({np.argmax(dataT1)})')
    deltasT1 = dataT1[1:] - dataT1[:-1]
    # X-Axis in seconds.
    dataT1 /= 1e6
    ax.scatter(dataT1[1:], deltasT1, c='#ff0000', alpha=0.5)
except:
    pass

try:
    dataT2 = np.genfromtxt('InputT1.1.csv', delimiter='\n')*deltaMicrosecond
    print(f'T2. Avg {np.average(dataT2)}. Min {np.min(dataT2)} ({np.argmin(dataT2)}). Max {np.max(dataT2)} ({np.argmax(dataT2)})')
    deltasT2 = dataT2[1:] - dataT2[:-1]
    # X-Axis in seconds.
    dataT2 /= 1e6
    ax.scatter(dataT2[1:], deltasT2, c='#00ff00', alpha=0.5)
except:
    pass

# dataT1 = genfromtxt('InputT1.0.csv', delimiter='\n')*deltaMicrosecond
# # X-Axis in seconds.
# dataT2 = genfromtxt('InputT1.1.csv', delimiter='\n')*deltaMicrosecond
# # X-Axis in seconds.
# ax.set(xlim=(min(dataT1), max(dataT1)))
# deltaT2T1 = dataT2 - dataT1
# ax.scatter(dataT1, deltaT2T1, c='#ff0000')
# ax.set(xlim=(min(dataT1), max(dataT1)))
# print(deltaT2T1)

ax.set_xlabel("Time (s)")
ax.set_ylabel("Delta (us)")
plt.show()