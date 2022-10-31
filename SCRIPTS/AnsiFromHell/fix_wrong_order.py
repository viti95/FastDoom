file = open("merged_lut.txt", "r")
data = file.read()
file.close()

items = data.split(",")

output = open("merged_lut_fix.txt", "w")

count = 0

while count < 131071:
    tmp = items[count]
    items[count] = items[count + 1]
    items[count + 1] = tmp
    count = count + 2

data = ','.join(items)
output.write(data)
output.close()
