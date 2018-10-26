f = open("padding.bin", "wb")
b = [0]
for i in range(28740):
  f.write(bytearray(b))
f.close()

