from numpy import genfromtxt
import matplotlib.pyplot as plt

clkFreq = 80e6

deltaMicrosecond = 1e6/clkFreq

dataT1 = genfromtxt('InputT1.0.csv', delimiter='\n')
dataT2 = genfromtxt('InputT1.1.csv', delimiter='\n')

fig, ax = plt.subplots()

deltasT1 = dataT1[1:] - dataT1[:-1]
deltasT2 = dataT2[1:] - dataT2[:-1]
ax.scatter(dataT1[1:], deltasT1, c='#ff0000')
ax.scatter(dataT2[1:], deltasT2, c='#00ff00')
ax.set(xlim=(min(dataT1), max(dataT1)))

print(dataT1)
print(deltasT1)

# deltaT2T1 = dataT2 - dataT1
# ax.scatter(dataT1, deltaT2T1, c='#ff0000')
# ax.set(xlim=(min(dataT1), max(dataT1)))
# print(deltaT2T1)

ax.set_xlabel("Time (ns)")
ax.set_ylabel("Delta (ns)")
plt.show()