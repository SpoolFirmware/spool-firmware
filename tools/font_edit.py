import numpy as np
import matplotlib.pyplot as plt
import math

packed_data = bytearray([0x80,0x00,0x00,0x00,0x00])

font = np.unpackbits(packed_data)[0:35].reshape((7,-1))
# Create a figure and axis object
fig, ax = plt.subplots()

# Set the aspect ratio to 1
ax.set_aspect('equal')

# Plot the font as an image
img = ax.imshow(font, cmap='binary')

# Hide the ticks on the x and y axis
ax.set_xticks([])
ax.set_yticks([])

def onclick(event):
    if event.xdata != None and event.ydata != None:
        x = int(math.floor(event.xdata + 0.5))
        y = int(math.floor(event.ydata + 0.5))
        font[y,x] = 1 if font[y,x] == 0 else 0 
        img.set_data(font)
        fig.canvas.draw()
cid = fig.canvas.mpl_connect('button_press_event', onclick)

# Show the plot
plt.show()
print(np.packbits(font.reshape((35))))