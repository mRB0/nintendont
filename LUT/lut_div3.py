# hello, world of python!

output = open("lut_div3.c", "w")
output.write("static const rom uint8_t lut_div3[] = {\n    ")

for i in range(0,24):
    output.write( str( i / 3 ) )
    output.write( ", " )

output.write( "\n};" )

output.close()

