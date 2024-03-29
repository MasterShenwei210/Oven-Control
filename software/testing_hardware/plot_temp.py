"""
Matplotlib Animation Example

author: Jake Vanderplas
email: vanderplas@astro.washington.edu
website: http://jakevdp.github.com
license: BSD
Please feel free to use and modify this, but keep the above information. Thanks!
"""
import serial
import numpy as np
from matplotlib import pyplot as plt
from matplotlib import animation
import time
import sys

import xlwt
from xlwt import Workbook

wb = Workbook()
# First set up the figure, the axis, and the plot element we want to animate
B = 4043.185
T = 273.15 + 35
R = 30.326e3

plot_top = 45
plot_bottom = 20

adc_data = np.zeros(100)
ser = serial.Serial(sys.argv[1], baudrate=115200)


def temperature(Rth):
    return 1/(1/T - np.log(R/Rth)/B)

def resistance(Vm):
    return -75e3 / (Vm - 1.25) - 30e3
# initialization function: plot the background of each frame
#
def lsb_to_celsius(lsb):
    return temperature(resistance(lsb*2.5/2**23)) - 273.15

def parse_input(adc_data):
    string = ser.readline().decode()
    if (string == "ADC Readings:\r\n"):
        data = np.zeros(4)
        for i in range(4):
            string = ser.readline()
            start = string.find(b':')+2
            end = string.find(b'\r')
            data[i] = int(string[start:end])
        adc_data[:] = np.roll(adc_data, 1)
        adc_data[0] = lsb_to_celsius(np.mean(data))


plt.ion()
fig, ax = plt.subplots()
ax.set_ylim(bottom = plot_bottom, top = plot_top)
x = np.arange(0, 100)
line, = ax.plot(x, np.zeros(100))

print("Connected to", ser.name)
ser.write(b'set_rate 5\r')
time.sleep(.5)
ser.write(b'stream_adc on\r')
time.sleep(.5)
ser.write(b'set_dac 4095\r')
for _ in range(10000000):
    #ser.write(b'read_adc\r')
    while(ser.inWaiting()):
        parse_input(adc_data)

    line.set_ydata(adc_data)
    fig.canvas.draw()
    fig.canvas.flush_events()
    time.sleep(.1)

ser.close()

# call the animator.  blit=True means only re-draw the parts that have changed.
#anim = animation.FuncAnimation(fig, animate, init_func=init,
#                               frames=200, interval=1, blit=True)

# save the animation as an mp4.  This requires ffmpeg or mencoder to be
# installed.  The extra_args ensure that the x264 codec is used, so that
# the video can be embedded in html5.  You may need to adjust this for
# your system: for more information, see
# http://matplotlib.sourceforge.net/api/animation_api.html

#plt.show()
#print("hey")
