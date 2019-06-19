import numpy as np
from matplotlib import pyplot as plt
import time

y= np.r_[0:100:1]

plt.ion()
fig, ax = plt.subplots()
ax.set_ylim(bottom = 0, top = 100)
x = np.arange(0, 100)
line, = ax.plot(x, np.zeros(100))


for _ in range(1000):
    y = np.roll(y, 1)
    line.set_ydata(y)
    fig.canvas.draw()
    fig.canvas.flush_events()
    time.sleep(.5)
