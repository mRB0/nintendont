# hello, world of python!
# vrc6 frequency table generator

output = open("vrc6_ftab.c", "w")
output.write("static const rom uint16_t vrc6_ftab[] = {")

for i in range(0,767):
    if( (i % 64) == 0 ):
        output.write( "\n    " )
    output.write( str(int(round(6848.347156570608633265574554582 / 2**(i/768.0)))))
    output.write( ", " )

output.write( "\n};" )

output.close()

