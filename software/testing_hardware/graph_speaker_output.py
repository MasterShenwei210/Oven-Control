import pyaudio
import struct
import matplotlib.pyplot as plt
import numpy as np

FORMAT = pyaudio.paFloat32
DEVICE = 'Speakers (Realtek High Definition Audio)'
p = pyaudio.PyAudio( )

for i in range(p.get_device_count()):
    info = p.get_device_info_by_index(i)
    if info['name'] == DEVICE:
        device_info = info

FRAME_SIZE = 3 * 1024
SAMPLE_RATE = int(device_info["defaultSampleRate"])

stream = p.open(format = FORMAT,
                channels = 2,
                rate = SAMPLE_RATE,
                input = True,
                frames_per_buffer = FRAME_SIZE,
                input_device_index = device_info['index'],
                as_loopback = True)
plt.ion()

fig, ax = plt.subplots()
ax.set_ylim(bottom = -.25, top = .25)
x = np.arange(0, FRAME_SIZE)
line, = ax.plot(x, np.zeros(FRAME_SIZE))


for _ in range(1000):
    data = stream.read(FRAME_SIZE)
    decoded = struct.unpack(str(FRAME_SIZE * 2) + 'f', data)[::2]
    line.set_ydata(decoded)
    fig.canvas.draw()
    fig.canvas.flush_events()

stream.stop_stream()
stream.close()
