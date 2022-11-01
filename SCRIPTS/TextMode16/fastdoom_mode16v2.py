file = open("PLAYPAL.pal", "rb")

byte = file.read(1)

while byte:
    color8bit = int.from_bytes(byte, "little")
    color6bit = int(round((color8bit * 63) / 255, 0))
    print("0x{:02x}".format(color6bit), end=',')
    byte = file.read(1)