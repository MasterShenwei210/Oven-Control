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
import time
import sys

import xlwt
from xlwt import Workbook

wb = Workbook()
sheet1 = wb.add_sheet("Sheet 1")
row = [0]

B = 4043.185
T = 273.15 + 35
R = 30.326e3

plot_top = 45
plot_bottom = 20
plot_time = 18

ser = serial.Serial(sys.argv[1], baudrate=115200)


def temperature(Rth):
    return 1/(1/T - np.log(R/Rth)/B)

def resistance(Vm):
    return -75e3 / (Vm - 1.25) - 30e3
# initialization function: plot the background of each frame
#
def lsb_to_celsius(lsb):
    return temperature(resistance(lsb*2.5/2**23)) - 273.15

def parse_input(row):
    string = ser.readline().decode()
    if (string == "ADC Readings:\r\n"):
        data = np.zeros(4)
        for i in range(4):
            string = ser.readline()
            start = string.find(b':')+2
            end = string.find(b'\r')
            data[i] = int(string[start:end])

        yval = lsb_to_celsius(np.mean(data))

        sheet1.write(row[0], 0, str(time.time()))
        sheet1.write(row[0], 1, str(yval))
        row[0] += 1


print("Connected to", ser.name)
ser.write(b'set_rate 5\r')
time.sleep(.5)
ser.write(b'stream_adc on\r')
time.sleep(.5)
ser.write(b'set_dac 4095\r')

start_time = time.time()
while(time.time() - start_time < plot_time):
    if ser.inWaiting():
        parse_input(row)

ser.close()
wb.save('temp_data_18sec.xls')

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
