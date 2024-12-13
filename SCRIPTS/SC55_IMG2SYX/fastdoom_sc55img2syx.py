from PIL import Image
import math

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

# Save output file
image_path = "image.png"
output_path = "output.bin"

try:
    byte_data = convert_image_to_bytes(image_path)
    with open(output_path, "wb") as f:
        f.write(bytes(byte_data))
    print(f"Conversion complete: {output_path}")
except Exception as e:
    print(f"Error: {e}")
