import sys

inputfile = open(sys.argv[1], "rb")
byte = inputfile.read(1)

outputitems = []

while byte:
    color8bit = int.from_bytes(byte, "little")
    color6bit = int(round((color8bit * 63) / 255, 0))
    print("0x{:02x}".format(color6bit), end=',')
    outputitems.append(color6bit)
    byte = inputfile.read(1)

inputfile.close()

outputfile = open(sys.argv[2], "wb")

for byte in outputitems:
    outputfile.write(byte.to_bytes(1, byteorder='little'))

outputfile.close()
