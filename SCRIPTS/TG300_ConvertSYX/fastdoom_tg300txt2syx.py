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

text = "    FastDoom     Version  1.0.7 "
output_path = "output.bin"

try:
    initial_byte_data = [0x43, 0x10, 0x2B, 0x07, 0x00, 0x00]
    byte_data = list(text.encode('ascii', 'replace'))

    byte_data = initial_byte_data + byte_data

    checksum = checksum_roland(byte_data[3:])

    byte_data.append(checksum)

    # Save output file
    with open(output_path, "wb") as f:
        f.write(bytes(byte_data))

    print(f"Conversion complete: {output_path}")
except Exception as e:
    print(f"Error: {e}")
