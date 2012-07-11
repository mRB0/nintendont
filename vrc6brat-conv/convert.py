#!/usr/bin/env python

import sys
import pyIT
import logging
import collections
import math
import itertools

logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)-7s %(message)s")

class ColState(object):
    attrs = 'hz vol sample enable vibenabled vibspeed vibpos vibdepth trig'.split()

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

    sample_map = {}

    outraw = []
    
    states = [ColState(1000, 0, 0, False, False, 0, 0, 0, False) for i in xrange(4)]
    speed = itfile.IS
    
    lastout = None
    
    for (ptn_idx, order_num) in zip(itfile.Orders, itertools.count()):
        if ptn_idx == 255:
            break
        if ptn_idx == 254:
            continue

        ptn = itfile.Patterns[ptn_idx]
        for (row, row_num) in zip(ptn.Rows, itertools.count()):
            logging.info(' | '.join([str(note) for note in row[:4]]))

            for col in xrange(4):
                note = row[col]
                state = states[col]
                if note.Note >= 253:
                    state.trig = True
                    state.enable = False
                    state.vibenabled = False
                if note.Note is not None and note.Note <= 119:
                    if col < 3:
                        state.hz = pow(2, float(note.Note - 57) / 12) * 220
                    else:
                        c5speed = itfile.Samples[note.Instrument - 1].C5Speed
                        state.hz = pow(2, float(note.Note - 60) / 12) * c5speed
                    state.vol = 15 if col < 3 else 5
                    state.enable = True
                    state.trig = True

                if note.Volume is not None and note.Volume <= 64:
                    if col < 3:
                        state.vol = min(note.Volume / 4, 15)
                    else:
                        state.vol = min(int(note.Volume / 10.6), 5)

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
                for col in xrange(4):
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

                    if col < 3:
                        thisout.append(smp | state.vol)
                        thisout.append(per & 0xff)
                        thisout.append((per >> 8) | ((1 if state.enable else 0) << 7))
                    elif col == 3:
                        trig = False
                        if state.trig:
                            if state.enable:
                                trig = True
                                if state.sample in sample_map:
                                    smp_num = sample_map[state.sample]
                                else:
                                    smp_num = len(sample_map.values())
                                    sample_map[state.sample] = smp_num
                                
                                thisout.append(smp_num)
                            else:
                                thisout.append(0xfe)
                            state.trig = False
                        else:
                            thisout.append(0xff)

                        # calculate optimal interrupt period and sample stride
                        min_intr_period = 0x600 # ticks
                        cpu_speed = 16000000 # Hz
                        
                        
                        for stride in itertools.count(1): # play every (stride)-th sample from data
                            intr_period = int(float(cpu_speed) / (hz / stride))
                            if intr_period >= min_intr_period:
                                break

                        if trig:
                            logging.info("%d:%d sample=%d period=%d stride=%d vol=%d", order_num, row_num, smp_num, intr_period, stride, state.vol)

                        thisout.append(intr_period)
                        thisout.append(stride)
                        thisout.append(state.vol)
                        
                        
                sizes = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1]
                
                if not lastout:
                    # first frame, output everything
                    writes = []
                    for (i, size) in zip(xrange(len(thisout)), sizes):
                        for b in xrange(size):
                            byte = 0xff & (thisout[i] >> ((size - b - 1) * 8))
                            writes.append(byte)
                    
                    outraw.append([0xff, 0xf8] + writes)
                else:
                    diff = 0
                    writes = []
                    
                    for i in xrange(len(thisout)):
                        if thisout[i] != lastout[i]:
                            diff |= 1 << (15 - i)

                    writes.append((diff >> 8) & 0xff)
                    writes.append(diff & 0xff)

                    for (i, size) in zip(xrange(len(thisout)), sizes):
                        if diff & (1 << (15 - i)):
                            for b in xrange(size):
                                byte = 0xff & (thisout[i] >> ((size - b - 1) * 8))
                                writes.append(byte)
                    outraw.append(writes)
                    
                lastout = thisout
                    
    cfile = open(outfile_fmt %{'extension': 'cpp'}, 'w')

    cfile.write('#include <stdint.h>\n')
    cfile.write('#include <avr/pgmspace.h>\n')
    cfile.write('\n')
    
    fancysong_data = ',\n'.join(['\t' + ', '.join(['0x%02X' %(0xff & i) for i in e]) for e in outraw])
    
    cfile.write('prog_uint8_t fancysong[] PROGMEM = {\n%s\n};\n\n' %(fancysong_data,))
    cfile.write('prog_uint32_t fancysong_len = %d;\n\n' %(sum(map(len, outraw)),))

    if sample_map:
        i = 0
        sample_lens = []
        for (it_smp, c_smp) in sample_map.items():
            data = itfile.Samples[it_smp-1].SampleData
            altered_data = ''.join([chr((0xff & (128 + ord(b))) >> 2) for b in data])
            while '!!!' in altered_data: # work around arduino bootloader bug
                altered_data = altered_data.replace('!!!', '!"!')
            sample_lens.append(len(altered_data))
            cdata = 'static prog_uint8_t sample_%d[] PROGMEM = { %s };\n' %(i, ', '.join(['0x%02X' %(0xff & ord(s)) for s in altered_data]),)
            cfile.write(cdata)
            i += 1

        sample_list = ', '.join(['sample_%d' %x for x in range(i)])
        sample_len_list = ', '.join(map(str, sample_lens))
        
        cfile.write('\nprog_uint8_t *samples[] = { %s };\n' %sample_list)
        cfile.write('prog_uint16_t sample_lens[] PROGMEM = { %s };\n' %sample_len_list)
        
    cfile.close()
    
    hfile = open(outfile_fmt %{'extension': 'h'}, 'w')
    hfile.write('''\
#ifndef FANCYSONG_H_
#define FANCYSONG_H_

#include <stdint.h>
#include <avr/pgmspace.h>

extern prog_uint8_t fancysong[] PROGMEM;
extern prog_uint32_t fancysong_len;

extern prog_uint8_t *samples[];
extern prog_uint16_t sample_lens[] PROGMEM;


#endif
''')
    
    hfile.close()
                    

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print "need arg"
    else:
        convert(sys.argv[1])
    
