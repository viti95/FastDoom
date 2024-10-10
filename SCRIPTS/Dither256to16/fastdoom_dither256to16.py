from PIL import ImageChops, Image
import sys
import math
import numpy as np
import csv

vars = sys.argv[1:]

outfile_str = 'output.txt'

colors = [
    (0x00,0x00,0x00),  #0
    (0x00,0x00,0xAA),  #1
    (0x00,0xAA,0x00),  #2
    (0x00,0xAA,0xAA),  #3
    (0xAA,0x00,0x00),  #4
    (0xAA,0x00,0xAA),  #5
    (0xAA,0x55,0x00),  #6
    (0xAA,0xAA,0xAA),  #7
    (0x55,0x55,0x55),  #8
    (0x55,0x55,0xFF),  #9
    (0x55,0xFF,0x55),  #10
    (0x55,0xFF,0xFF),  #11
    (0xFF,0x55,0x55),  #12
    (0xFF,0x55,0xFF),  #13
    (0xFF,0xFF,0x55),  #14
    (0xFF,0xFF,0xFF)]  #15

def srgb_to_lineal(color_srgb):
    def convert(c):
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    return tuple(convert(c / 255.0) for c in color_srgb)

def lineal_to_srgb(color_lineal):
    def convert(c):
        return 255.0 * (12.92 * c if c <= 0.0031308 else 1.055 * (c ** (1.0 / 2.4)) - 0.055)
    return tuple(int(round(convert(c))) for c in color_lineal)

def blend_colors(color1, color2, porcentaje=0.5):
    # Convertir los colores a espacio lineal
    color1_lineal = srgb_to_lineal(color1)
    color2_lineal = srgb_to_lineal(color2)
    
    # Interpolar entre los dos colores en espacio lineal
    color_mezclado_lineal = (
        color1_lineal[0] * (1 - porcentaje) + color2_lineal[0] * porcentaje,
        color1_lineal[1] * (1 - porcentaje) + color2_lineal[1] * porcentaje,
        color1_lineal[2] * (1 - porcentaje) + color2_lineal[2] * porcentaje
    )
    
    # Convertir de vuelta a sRGB
    return lineal_to_srgb(color_mezclado_lineal)

def compare_images_mse(img1, img2):
    errors = np.asarray(ImageChops.difference(img1, img2)) / 255
    return math.sqrt(np.mean(np.square(errors)))

def create_image_original(color):
    im = Image.new(mode="RGB", size=(2,1))
    im.putpixel((0,0), color)
    im.putpixel((1,0), color)
    return im

def create_image_dithered(color_0, color_1):

    #mixedcolor = blend_colors(color_0, color_1, 0.5)

    mixedcolor = (
        (color_0[0] + color_1[0]) // 2,
        (color_0[1] + color_1[1]) // 2,
        (color_0[2] + color_1[2]) // 2
    )

    im = Image.new(mode="RGB", size=(2,1))
    im.putpixel((0,0), mixedcolor)
    im.putpixel((1,0), mixedcolor)
    return im

def convert_6bits_to_8bits(value):
    # Multiplicar el valor de 6 bits por 255 / 63 para escalarlo a 8 bits
    return round((value * 255) / 63)

def tuple_from_colormap_array(num):
    newtuple = (colormap[num, 0], colormap[num, 1], colormap[num, 2])
    return newtuple

# Read colormap
colormap_file = []
with open('colormap.txt', newline='') as csvfile:
    reader = csv.reader(csvfile)
    for row in reader:
        colormap_file = [int(x) for x in row]

# Convert colormap to RGB colors
colormap = np.empty((14 * 256, 3), dtype=np.uint8)
for i in range(14 * 256):
    colormap[i,0] = convert_6bits_to_8bits(colormap_file[i * 3])
    colormap[i,1] = convert_6bits_to_8bits(colormap_file[(i * 3) + 1])
    colormap[i,2] = convert_6bits_to_8bits(colormap_file[(i * 3) + 2])

# Generate all LUT combinatios, and get best result
count = 0

output_file = open(outfile_str, "a")

for colormapentry in range(14 * 256):
    original_image = create_image_original(tuple_from_colormap_array(colormapentry))

    best_ratio = 1000.0
    best_color_0 = 0
    best_color_1 = 0

    count = count + 1

    for color0 in range(16):
        for color1 in range(16):
            dither_image = create_image_dithered(colors[color0], colors[color1])

            ratio = compare_images_mse(original_image, dither_image)

            if ratio < best_ratio:
                best_ratio = ratio
                best_color_0 = color0
                best_color_1 = color1

            if best_ratio == 0.0:
                break

        if best_ratio == 0.0:
            break

    print(count, best_ratio, best_color_0, best_color_1)
            
    finalcolor = 0

    finalcolor |= (best_color_0 & 0x08) << 4;
    finalcolor |= (best_color_0 & 0x04) << 3;
    finalcolor |= (best_color_0 & 0x02) << 2;
    finalcolor |= (best_color_0 & 0x01) << 1;

    finalcolor |= (best_color_1 & 0x08) << 3;
    finalcolor |= (best_color_1 & 0x04) << 2;
    finalcolor |= (best_color_1 & 0x02) << 1;
    finalcolor |= (best_color_1 & 0x01);

    output_file.write(str(finalcolor))
    output_file.write(",")
    output_file.flush()

output_file.close()

print(outfile_str)