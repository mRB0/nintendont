# hello, world of python!
# vrc6 frequency table generator

output = open("vrc6_ftab.c", "w")
output.write("#include \"stdint.h\"\n")
output.write("const rom uint16_t vrc6_ftab[] = {")

VRC6_CLK = 2048000.0

for i in range(0,767):
    if( (i % 64) == 0 ):
        output.write( "\n    " )
    output.write( str(int(round( (VRC6_CLK/8363.0) * 2.0**5.0 / 2.0**(i/768.0) ))));
    output.write( ", " )

output.write( "\n};\n" )

output.close()

