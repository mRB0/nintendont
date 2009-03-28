# hello, world of python!

output = open("spc_ftab.c", "w")
output.write("static const rom uint16_t spc_ftab[] = {")

for i in range(0,767):
    if( (i % 64) == 0 ):
        output.write( "\n    " )
    output.write( str( int(round((1070.464*8) * 2**(i/768.0)) )) )
    output.write( ", " )

output.write( "\n};" )

output.close()

