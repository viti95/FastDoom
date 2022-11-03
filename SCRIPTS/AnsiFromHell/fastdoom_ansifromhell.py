from PIL import ImageChops, ImageStat, Image
import sys
import math
import numpy

vars = sys.argv[1:]

first = int(vars[0])
last = int(vars[1])
outfile_str = vars[2]

colors = [
    (0x00,0x00,0x00), #0
    (0x00,0x00,0xAA), #1
    (0x00,0xAA,0x00), #2
    (0x00,0xAA,0xAA), #3
    (0xAA,0x00,0x00), #4
    (0xAA,0x00,0xAA), #5
    (0xAA,0x55,0x00), #6
    (0xAA,0xAA,0xAA), #7
    (0x55,0x55,0x55), #8
    (0x55,0x55,0xFF), #9
    (0x55,0xFF,0x55), #10
    (0x55,0xFF,0xFF), #11
    (0xFF,0x55,0x55), #12
    (0xFF,0x55,0xFF), #13
    (0xFF,0xFF,0x55), #14
    (0xFF,0xFF,0xFF)]  #15
    
character_set = [None] * 256

def compare_images_mse(img1, img2):
    errors = numpy.asarray(ImageChops.difference(img1, img2)) / 255
    return math.sqrt(numpy.mean(numpy.square(errors)))

def create_image_lut(lut_item):
    im = Image.new(mode="RGB", size=(8,2))
    im.putpixel((0,0), lut_item[0])
    im.putpixel((1,0), lut_item[0])
    im.putpixel((0,1), lut_item[0])
    im.putpixel((1,1), lut_item[0])
    im.putpixel((2,0), lut_item[1])
    im.putpixel((3,0), lut_item[1])
    im.putpixel((2,1), lut_item[1])
    im.putpixel((3,1), lut_item[1])
    im.putpixel((4,0), lut_item[2])
    im.putpixel((5,0), lut_item[2])
    im.putpixel((4,1), lut_item[2])
    im.putpixel((5,1), lut_item[2])
    im.putpixel((6,0), lut_item[3])
    im.putpixel((7,0), lut_item[3])
    im.putpixel((6,1), lut_item[3])
    im.putpixel((7,1), lut_item[3])
    return im

def is_set(x, n):
    return int.from_bytes(x, "big") & 1 << n != 0

def create_image_character(character_item, color_0, color_1):
    im = Image.new(mode="RGB", size=(8,2))
    im.putpixel((0,0), color_0 if is_set(character_set[character_item][0], 0) == 0 else color_1)
    im.putpixel((1,0), color_0 if is_set(character_set[character_item][0], 1) == 0 else color_1)
    im.putpixel((0,1), color_0 if is_set(character_set[character_item][1], 0) == 0 else color_1)
    im.putpixel((1,1), color_0 if is_set(character_set[character_item][1], 1) == 0 else color_1)
    im.putpixel((2,0), color_0 if is_set(character_set[character_item][0], 2) == 0 else color_1)
    im.putpixel((3,0), color_0 if is_set(character_set[character_item][0], 3) == 0 else color_1)
    im.putpixel((2,1), color_0 if is_set(character_set[character_item][1], 2) == 0 else color_1)
    im.putpixel((3,1), color_0 if is_set(character_set[character_item][1], 3) == 0 else color_1)
    im.putpixel((4,0), color_0 if is_set(character_set[character_item][0], 4) == 0 else color_1)
    im.putpixel((5,0), color_0 if is_set(character_set[character_item][0], 5) == 0 else color_1)
    im.putpixel((4,1), color_0 if is_set(character_set[character_item][1], 4) == 0 else color_1)
    im.putpixel((5,1), color_0 if is_set(character_set[character_item][1], 5) == 0 else color_1)
    im.putpixel((6,0), color_0 if is_set(character_set[character_item][0], 6) == 0 else color_1)
    im.putpixel((7,0), color_0 if is_set(character_set[character_item][0], 7) == 0 else color_1)
    im.putpixel((6,1), color_0 if is_set(character_set[character_item][1], 6) == 0 else color_1)
    im.putpixel((7,1), color_0 if is_set(character_set[character_item][1], 7) == 0 else color_1)
    return im

file = open("mda.rom","rb")
file.seek(6144)

# Read first two rows of each CGA character
for i in range(256):
    partial_character = [None] * 2
    partial_character[0] = file.read(1)
    partial_character[1] = file.read(1)
    file.seek(6, 1)
    character_set[i] = partial_character

file.close()

current_lut_item = [None] * 4

final_lut = [None] * 65536

output_file = open(outfile_str, "a")

character_lut_set = [[[None for k in range(16)] for j in range(16)] for i in range(256)]

# Generate character / color combinations
for x in range(256):
    for y in range(16):
        for z in range(16):
            # Create 8x2 character/color combination
            character_lut_set[x][y][z] = create_image_character(x, colors[y], colors[z])

# Generate all LUT combinatios, and get best result

count = 0

#for a in range(16):
for a in range(first, last):
    current_lut_item[0] = colors[a]
    for b in range(16):
        current_lut_item[1] = colors[b]
        for c in range(16):
            current_lut_item[2] = colors[c]
            for d in range(16):
                current_lut_item[3] = colors[d]
                
                count = count + 1

                # Create 8x2 image from the 4x1 image
                lut_image = create_image_lut(current_lut_item)

                best_ratio = 1000.0
                best_character = 0
                best_color_0 = 0
                best_color_1 = 0

                for x in range(256):

                    # Optimization 1
                    if x == 7 or x == 22 or x == 28 or x == 32 or x == 44 or x == 45 or x == 46 or x == 61 or x == 95 or x == 97 or x == 99 or x == 101 or \
                       x == 103 or x == 109 or x == 110 or x == 111 or x == 112 or x == 113 or x == 114 or x == 115 or x == 117 or x == 118 or x == 119 or \
                       x == 120 or x == 121 or x == 122 or x == 135 or x == 145 or x == 169 or x == 170 or x == 183 or x == 184 or x == 187 or x == 191 or \
                       x == 194 or x == 196 or x == 201 or x == 203 or x == 205 or x == 209 or x == 210 or x == 213 or x == 214 or x == 218 or x == 219 or \
                       x == 222 or x == 223 or x == 224 or x == 229 or x == 236 or x == 249 or x == 250 or x == 254 or x == 255:
                       continue

                    for y in range(16):
                        for z in range(16):
                            character_image = character_lut_set[x][y][z]
                            ratio = compare_images_mse(lut_image, character_image)

                            if ratio < best_ratio:
                                best_ratio = ratio
                                best_character = x
                                best_color_0 = y
                                best_color_1 = z
                                #best_image = character_image

                            if best_ratio == 0.0:
                                break
                        if best_ratio == 0.0:
                            break
                    if best_ratio == 0.0:
                        break

                #final_lut[position] = [best_ratio, best_character, best_color_0, best_color_1]

                #lut_image_filename = "%06dl.png" % position
                #lut_image.save(lut_image_filename)
                #best_image_filename = "%06dm.png" % position
                #best_image.save(best_image_filename)

                print(outfile_str, count, best_ratio, best_character, best_color_0, best_color_1)

                output_file.write(str(best_character))
                output_file.write(",")                
                output_file.write(str(best_color_0 << 4 | best_color_1))
                output_file.write(",")
                output_file.flush()

output_file.close()

print("END!!")