from PIL import Image
import math

def checksum_roland(buf):
    cksum = 0

    # Add up all the bytes
    for b in buf:
        cksum += b

    # Use only the 7 least significant bits
    cksum &= 0x7F

    # Adjust the checksum to ensure it sums to zero with the data
    if cksum != 0:
        cksum = 0x80 - cksum

    return cksum

def convert_image_to_bytes(image_path):
    # Open image
    with Image.open(image_path) as img:

        # Check dimensions
        if img.size != (16, 16):
            raise ValueError("Image doesn't have 16x16 resolution")

        # Get pixel list 
        pixels = list(img.getdata())
        
        # Convert to columns
        columns = [[pixels[y * 16 + x] for y in range(16)] for x in range(16)]

        # Output array
        bytes_output = []

        # Do full 5 bit blocks
        for j in range(0, 3):
            for i in range(0, 16):
                group = [columns[j*5+0][i], columns[j*5+1][i], columns[j*5+2][i], columns[j*5+3][i], columns[j*5+4][i]]
                # Convertir el grupo a un byte (5 bits -> 0 a 31)
                byte = sum(bit << (4 - idx) for idx, bit in enumerate(group))
                bytes_output.append(byte)

        # Do single bit final blocks
        for i in range(0, 16):
            group = [columns[3*5][i], 0, 0, 0, 0]
            # Convertir el grupo a un byte (5 bits -> 0 a 31)
            byte = sum(bit << (4 - idx) for idx, bit in enumerate(group))
            bytes_output.append(byte)

        return bytes_output

image_path = "image.png"
output_path = "output.bin"

try:
    initial_byte_data = [0x41, 0x10, 0x45, 0x12, 0x10, 0x01, 0x00]
    byte_data = convert_image_to_bytes(image_path)

    byte_data = initial_byte_data + byte_data

    checksum = checksum_roland(byte_data[4:])

    byte_data.append(checksum)

    # Save output file
    with open(output_path, "wb") as f:
        f.write(bytes(byte_data))

    print(f"Conversion complete: {output_path}")
except Exception as e:
    print(f"Error: {e}")
