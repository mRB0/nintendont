#!/usr/bin/env python

import sys
import pyIT
import logging
import collections
import math

logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)-7s %(message)s")

class ColState(object):
    attrs = 'hz vol sample enable vibenabled vibspeed vibpos vibdepth'.split()

    def __init__(self, *vals):
        for attr, val in zip(ColState.attrs, vals):
            setattr(self, attr, val)
            
    def _replace(self, **kwargs):
        for (attr, val) in kwargs.items():
            if attr not in ColState.attrs:
                raise ValueError("tried to set nonexistent attr %s", attr)
            setattr(self, attr, val)
            

outfile_fmt='fancysong.%(extension)s'

def convert(inpath):
    itfile = pyIT.ITfile()
    itfile.open(inpath)

    used_samples = set()

    outraw = []
    
    states = [ColState(1000, 0, 0, False, False, 0, 0, 0) for i in xrange(4)]
    speed = itfile.IS
    
    lastout = None
    
    for ptn_idx in itfile.Orders:
        if ptn_idx == 255:
            break
        if ptn_idx == 254:
            continue

        ptn = itfile.Patterns[ptn_idx]
        for row in ptn.Rows:
            logging.info(' | '.join([str(note) for note in row[:3]]))

            for col in xrange(4):
                note = row[col]
                state = states[col]
                if note.Note >= 253:
                    state._replace(enable=False)
                if note.Note is not None and note.Note <= 119:
                    state._replace(hz=pow(2, float(note.Note - 57) / 12) * 220)
                    state._replace(vol=15)
                    state._replace(enable=True)

                if note.Volume is not None and note.Volume <= 64:
                    state._replace(vol=min(note.Volume / 4, 15))

                if note.Instrument is not None:
                    if (col >= 3 or
                        (col < 3 and note.Instrument >= 8 and note.Instrument <= 15)):
                            state._replace(sample=note.Instrument)
                
                # process commands
                if note.Effect == 1 and note.EffectArg:
                    speed = note.EffectArg
                if note.Effect == 8:
                    vibdepth = (0xff & note.EffectArg) & 7
                    vibspeed = (0xff & note.EffectArg) >> 4

                    if vibdepth:
                        state._replace(vibdepth=vibdepth)
                    if vibspeed:
                        state._replace(vibspeed=vibspeed)
                    if not state.vibenabled:
                        state._replace(vibenabled=True)
                        state._replace(vibpos=0)
                else:
                    state._replace(vibenabled=False)

            # output states for each tick

            for tick in xrange(speed):
                thisout = []
                for col in xrange(3): # todo: enable 4th channel
                    state = states[col]
                    hz = float(state.hz)
                    if state.vibenabled:
                        hz = int(hz / pow(2, state.vibdepth * math.sin(state.vibpos * (2 * math.pi) / 256) / (12 * 6)))
                        state._replace(vibpos=state.vibpos + state.vibspeed * 3)
                    
                    if col < 2:
                        per = int(2048000.0 / ((hz + 1) * 16))
                        smp = ((state.sample - 8) << 4)
                    elif col < 3:
                        per = int(2048000.0 / ((hz + 1) * 14))
                        smp = 0

                    thisout.append(smp | state.vol)
                    thisout.append(per & 0xff)
                    thisout.append((per >> 8) | ((1 if state.enable else 0) << 7))

                if not lastout:
                    # first frame, output everything
                    outraw.append([0xff] + thisout)
                else:
                    diff = 0
                    incl = set()
                    writes = []

                    for i in xrange(len(thisout)):
                        if thisout[i] != lastout[i]:
                            incl.add(i)
                            diff |= 1 << (7 if (i == 8) else i)

                    writes.append(diff)

                    for i in xrange(len(thisout)):
                        if diff & (1 << (7 if (i == 8) else i)):
                            writes.append(thisout[i])
                    outraw.append(writes)

                lastout = thisout
                    
    cfile = open(outfile_fmt %{'extension': 'cpp'}, 'w')

    cfile.write('#include <stdint.h>\n')
    cfile.write('#include <avr/pgmspace.h>\n')
    cfile.write('\n')
    
    fancysong_data = ',\n'.join(['\t' + ', '.join(['0x%02X' %(0xff & i) for i in e]) for e in outraw])
    
    cfile.write('prog_uint8_t fancysong[] PROGMEM = {\n%s\n};\n\n' %(fancysong_data,))
    cfile.write('prog_uint32_t fancysong_len = %d;' %(sum(map(len, outraw)),))

    cfile.close()
    
    hfile = open(outfile_fmt %{'extension': 'h'}, 'w')
    hfile.write('''\
#ifndef FANCYSONG_H_
#define FANCYSONG_H_

#include <stdint.h>
#include <avr/pgmspace.h>

extern prog_uint8_t fancysong[] PROGMEM;
extern prog_uint32_t fancysong_len;

#endif
''')
    
    hfile.close()
                    

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print "need arg"
    else:
        convert(sys.argv[1])
    
