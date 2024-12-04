from numpy import genfromtxt
import matplotlib.pyplot as plt

clkFreq = 160e6

deltaMicrosecond = 1e6/clkFreq
print(f'dt = {deltaMicrosecond} us/step')

fig, ax = plt.subplots()

try:
    dataT1 = genfromtxt('InputT1.0.csv', delimiter='\n')*deltaMicrosecond
    deltasT1 = dataT1[1:] - dataT1[:-1]
    # X-Axis in seconds.
    dataT1 /= 1e6
    ax.scatter(dataT1[1:], deltasT1, c='#ff0000', alpha=0.5)
except:
    pass

try:
    dataT2 = genfromtxt('InputT1.1.csv', delimiter='\n')*deltaMicrosecond
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