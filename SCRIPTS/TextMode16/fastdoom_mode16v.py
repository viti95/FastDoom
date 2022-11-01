from PIL import Image
import sys

for i in range(1, 15):
    img = Image.open('pal' + str(i) + '.png')
    colors = img.getpalette()
    colors16 = colors[:48]

    for color8bit in colors16:
        color6bit = int(round((color8bit * 63) / 255, 0))
        print("0x{:02x}".format(color6bit), end=',')

    img.close()
