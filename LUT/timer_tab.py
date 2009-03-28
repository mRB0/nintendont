# hello, world of python!
# bpm -> timer table generator

output = open("timer_tab.c", "w")
output.write("#include \"stdint.h\"\n");
output.write("const rom uint16_t timer_tab[] = {\n    ")

bpm = 0
for bpm in range(32,64):
    output.write( str(65536 - (1310720/bpm)) )
    output.write( ", " )
output.write( "\n    " )

for bpm in range(64,96):
    output.write( str(65536 - (1310720/bpm)) )
    output.write( ", " )
output.write( "\n    " )

for bpm in range(96,128):
    output.write( str(65536 - (1310720/bpm)) )
    output.write( ", " )
output.write( "\n    " )

for bpm in range(128,160):
    output.write( str(65536 - (1310720/bpm)) )
    output.write( ", " )
output.write( "\n    " )

for bpm in range(160,192):
    output.write( str(65536 - (1310720/bpm)) )
    output.write( ", " )
output.write( "\n    " )

for bpm in range(192,224):
    output.write( str(65536 - (1310720/bpm)) )
    output.write( ", " )
output.write( "\n    " )

for bpm in range(224,256):
    output.write( str(65536 - (1310720/bpm)) )
    output.write( ", " )
output.write( "\n};\n" )

output.close()

