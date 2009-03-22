;*****************************************************************************************
; SPCUNIT
;
; (spc sound server)
;
; by mukunda
;*****************************************************************************************

;*****************************************************************************************
; PROTOCOL
;*****************************************************************************************
; mm = mimic data
; id = message id
; vv = validation data (not previous value)
; v1 = nonzero validation data (not previous value)
;
; NAME	ID	DESC
;-------------------------------------------------------------
; LOAD	00	Upload Sample
; 
; >> id vv LL LL
; << -- mm -- --
; >> -- v1 DD DD [...until all words transferred]
; << -- mm -- --
; >> -- 00 DD DD [final word]
; << -- mm -- --
;
; LLLL = loop point
;-------------------------------------------------------------
; EVOL	01	Set Echo Volume
;
; >> id vv LL RR
; << -- mm -- --
;
; LL = left evol (-128..127)
; RR = right evol (-128..127)
;-------------------------------------------------------------
; COEF	02	Set Echo Coefficient
;
; >> id vv II CC
; << -- mm -- --
;
; II = index
; CC = coefficient (-128..127)
;-------------------------------------------------------------
; EDL	03	Set Echo Delay
;
; >> id vv -- DD
; << -- mm -- --
;
; DD = echo delay (0..15)
; [WARNING: WILL HALT SYSTEM FOR DD*16 MS]
;-------------------------------------------------------------
; ECEN	04	Set Echo Enable
;
; >> id vv -- EE
; << -- mm -- --
;
; EE = 01=enable echo, 00=disable echo
;-------------------------------------------------------------
; MVOL	05	Set Main Volume
;
; >> id vv LL RR
; << -- mm -- --
;
; LL = left level  (-128..127)
; RR = right level (-128..127)
;-------------------------------------------------------------
; RET	06	End of messages (return to processing)
;
; >> id vv -- --
; << -- mm -- --
;-------------------------------------------------------------
; KOF	08	Stop voices
;
; >> id vv -- aa
; << -- -- mm --
;
; aa = voices to stop (voice/bit)
;-------------------------------------------------------------
; PITCH	1x	Set Voice Pitch
; 
; >> 1x vv pp pp
; << -- mm -- --
;
; x = voice index
; pppp = pitch height (14-bit)
;-------------------------------------------------------------
; VOL	1y	Set Voice Volume
;
; >> 1y vv pp ll
; << -- mm -- --
;
; y = voice index (+8)
; pp = panning level (0..127), 128=dont use
; ll = volume level (0..127)
;-------------------------------------------------------------
; KON	2x	Start voice
;
; >> 2x vv oo ss
; << -- mm -- --
;
; oo = sample offset
; ss = sample index
;-------------------------------------------------------------

;*****************************************************************************************

;*****************************************************************************************
; registers
;*****************************************************************************************
SPC_TEST	=0F0h ; undocumented
SPC_CONTROL	=0F1h ; control register
SPC_DSP		=0F2h
SPC_DSPA	=0F2h
SPC_DSPD	=0F3h
SPC_PORT0	=0F4h ; i/o port0
SPC_PORT1	=0F5h ; i/o port1
SPC_PORT2	=0F6h ; i/o port2
SPC_PORT3	=0F7h ; i/o port3
SPC_FLAGS	=0F8h ; custom flags
SPC_TIMER0	=0FAh ; timer0 setting
SPC_TIMER1	=0FBh ; timer1 setting
SPC_TIMER2	=0FCh ; timer2 setting
SPC_COUNTER0	=0FDh ; timer0 counter
SPC_COUNTER1	=0FEh ; timer1 counter
SPC_COUNTER2	=0FFh ; timer2 counter

;*****************************************************************************************
; dsp registers
;*****************************************************************************************
DSPV_VOL	=00h
DSPV_VOLR	=01h
DSPV_PL		=02h
DSPV_PH		=03h
DSPV_SRCN	=04h
DSPV_ADSR1	=05h
DSPV_ADSR2	=06h
DSPV_GAIN	=07h
DSPV_ENVX	=08h
DSPV_OUTX	=09h

DSP_MVOL	=0Ch
DSP_MVOLR	=1Ch
DSP_EVOL	=2Ch
DSP_EVOLR	=3Ch
DSP_KON		=4Ch
DSP_KOF		=5Ch
DSP_FLG		=6Ch
DSP_ENDX	=7Ch

DSP_EFB		=0Dh
DSP_PMON	=2Dh
DSP_NON		=3Dh
DSP_EON		=4Dh
DSP_DIR		=5Dh
DSP_ESA		=6Dh
DSP_EDL		=7Dh

DSP_C0		=0Fh
DSP_C1		=1Fh
DSP_C2		=2Fh
DSP_C3		=3Fh
DSP_C4		=4Fh
DSP_C5		=5Fh
DSP_C6		=6Fh
DSP_C7		=7Fh

FLG_RESET	=80h
FLG_MUTE	=40h
FLG_ECEN	=20h

;*****************************************************************************************
; zero-page memory
;*****************************************************************************************

bits:
	.block 8

transfer_address:
	.block 2
next_sample:
	.block 1
comms_v:
	.block 1
	

channel_rvolume:
	.block 8	; (gain) (actual levels)
channel_volume:
	.block 8	; (gain) (target levels)
channel_panning:
	.block 8	; (vol)
channel_ofs:
	.block 8	; (sample offset)
channel_srcn:
	.block 8	; (srcn/keyon)
channel_pitch_l:
	.block 8	; (PL)
channel_pitch_h:
	.block 8	; (PH)

kof_flags:
	.block 1
panning_flags:
	.block 1
pitch_flags:
	.block 1
update_dsp:
	.block 1	; boolean: true=need dsp update

m0:	.block 1	; scratch variables
m1:	.block 1	;
m2:	.block 1	;
m3:	.block 1	;

;*****************************************************************************************
; sample directory
;*****************************************************************************************
.org	0200h

SampleDirectory:
	.block	64*4

;*****************************************************************************************
; program (load @ 300h)
;*****************************************************************************************
.org	0300h

;-------------------------------------------------------------------------
ProgramEntry:
;-------------------------------------------------------------------------
	mov	SPC_PORT1, #0		; reset port1
	mov	SPC_CONTROL, #0		; reset control
					;---------------------------------
	mov	SPC_DSPA, #DSP_FLG	; disable mute and reset
	mov	SPC_DSPD, #FLG_ECEN	;
	mov	SPC_DSPA, #DSP_PMON	;
	mov	SPC_DSPD, #0		;
					;
	mov	SPC_DSPA, #DSP_MVOL	;
	mov	SPC_DSPD, #80		;
	mov	SPC_DSPA, #DSP_MVOLR	;
	mov	SPC_DSPD, #80		;
					;---------------------------------
	mov	SPC_DSPA, #DSP_DIR	; set source dir
	mov	SPC_DSPD, #SampleDirectory >> 8
					;---------------------------------
	mov	transfer_address, #(DataSector & 0ffh)	; setup transfer addresses
	mov	transfer_address+1, #(DataSector >> 8)	;
	mov	next_sample, #(SampleDirectory & 0ffh)	;
	mov	next_sample+1, #(SampleDirectory >> 8)	;
					;---------------------------------
	mov	a, #1			; generate bit table
	mov	x, #bits		;
_make_bits:				;
	mov	(x)+, a			;
	asl	a			;
	bne	_make_bits		;---------------------------------
	bra	NextCommand		; enter initial command loop
;-------------------------------------------------------------------------
_main_loop:
;-------------------------------------------------------------------------
	cmp	SPC_PORT1, comms_v	; test for message
	bne	ProcessComms		;
ExitComms:
					;---------------------------------
	bra	_main_loop		;
;-------------------------------------------------------------------------

;-------------------------------------------------------------------------
ProcessComms:
;-------------------------------------------------------------------------
	mov	comms_v, SPC_PORT1	; copy V
	mov	a, SPC_PORT0		; jump to message handler
	asl	a			;
	mov	x, a			;
	jmp	[CommandTable+x]	;
;-------------------------------------------------------------------------
	
;-------------------------------------------------------------------------
NextCommand_R:
;-------------------------------------------------------------------------
	mov	SPC_PORT1, comms_v	; reply to client
;-------------------------------------------------------------------------
NextCommand:
;-------------------------------------------------------------------------
	cmp	comms_v, SPC_PORT1	; wait for another command
	bne	ProcessComms		;
	bra	NextCommand		;
;-------------------------------------------------------------------------
	
;-------------------------------------------------------------------------
CommandTable:
;-------------------------------------------------------------------------
	.word	CMD_LOAD		; 00
	.word	CMD_EVOL		; 01
	.word	CMD_COEF		; 02
	.word	CMD_EDL			; 03
	.word	CMD_ECEN		; 04
	.word	CMD_MVOL		; 05
	.word	CMD_RET			; 06
	.word	0			; 07
	.word	CMD_KOF			; 08
	.word	0			; 09
	.word	0			; 0A
	.word	0			; 0B
	.word	0			; 0C
	.word	0			; 0D
	.word	0			; 0E
	.word	0			; 0F
	.word	CMD_PITCH		; 10
	.word	CMD_PITCH		; 11
	.word	CMD_PITCH		; 12
	.word	CMD_PITCH		; 13
	.word	CMD_PITCH		; 14
	.word	CMD_PITCH		; 15
	.word	CMD_PITCH		; 16
	.word	CMD_PITCH		; 17
	.word	CMD_VOL			; 18
	.word	CMD_VOL			; 19
	.word	CMD_VOL			; 1A
	.word	CMD_VOL			; 1B
	.word	CMD_VOL			; 1C
	.word	CMD_VOL			; 1D
	.word	CMD_VOL			; 1E
	.word	CMD_VOL			; 1F
	.word	CMD_KON			; 20
	.word	CMD_KON			; 21
	.word	CMD_KON			; 22
	.word	CMD_KON			; 23
	.word	CMD_KON			; 24
	.word	CMD_KON			; 25
	.word	CMD_KON			; 26
	.word	CMD_KON			; 27
	
;-------------------------------------------------------------------------
CMD_LOAD:
;-------------------------------------------------------------------------
	mov	a, transfer_address	; register sample in directory
	mov	y, next_sample		; word:[next_sample]   = start
	mov	!SampleDirectory+y, a	; word:[next_sample+2] = start+loop
	clrc				; next_sample += 4
	adc	a, SPC_PORT2		;
	mov	!SampleDirectory+2+y, a	;
					;
	mov	a, transfer_address+1	;
	mov	!SampleDirectory+1+y, a	;
					;
	adc	a, SPC_PORT3		;
	mov	!SampleDirectory+3+y, a	;
					;
	adc	next_sample, #4		;
					;---------------------------------
	mov	x, comms_v		; reply to client
	mov	y, #0			;
	mov	SPC_PORT1, x		;
;-------------------------------------------------------------------------
transfer_data:
;-------------------------------------------------------------------------
	cmp	x, SPC_PORT1		; wait for data
	beq	transfer_data		;
					;---------------------------------
	mov	a, SPC_PORT2		; copy data
	mov	[transfer_address]+y, a	;
	mov	a, SPC_PORT3		;
	inc	y			;
	mov	[transfer_address]+y, a	;
	inc	y			;
	beq	inc_address		;
					;---------------------------------
cont1:	mov	x, SPC_PORT1		; reply to client
	mov	SPC_PORT1, x		;
	bne	transfer_data		; transfer more if not zero
					;---------------------------------
	mov	m0, y			; add y to transfer_address
	clrc				;
	adc	transfer_address, m0	;
	adc	transfer_address+1, #0	;
					;---------------------------------
	mov	comms_v, x		; save v
	jmp	NextCommand		; -> next command
;-------------------------------------------------------------------------
inc_address:
;-------------------------------------------------------------------------
	inc	transfer_address+1	; transfer_address += 100h
	bra	cont1			;
;-------------------------------------------------------------------------
	
;-------------------------------------------------------------------------
CMD_EVOL:
;-------------------------------------------------------------------------
	mov	SPC_DSPA, #DSP_EVOL	; copy volume levels into dsp registers
	mov	SPC_DSPD, SPC_PORT2	;
	mov	SPC_DSPA, #DSP_EVOLR	;
	mov	SPC_DSPD, SPC_PORT3	;
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_COEF:
;-------------------------------------------------------------------------
	mov	SPC_DSPA, #DSP_C0	; copy value into dsp coefficient
	clrc				;
	adc	SPC_DSPA, SPC_PORT2	;
	mov	SPC_DSPD, SPC_PORT3	;
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_EDL:
;-------------------------------------------------------------------------
	mov	a, SPC_PORT3		; copy value into EDL
	mov	SPC_DSPA, #DSP_EDL	;
	mov	SPC_DSPD, a		;
					;---------------------------------
	asl	a			; delay a*16ms
	asl	a			;
	mov	m0+1, a			;
	mov	m0, #0			;
_delay_16clks:				;
	cmp	a, [0]+y		; <6
	decw	m0			; <6
	bne	_delay_16clks		; <4
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_ECEN:
;-------------------------------------------------------------------------
	mov	SPC_DSPA, #DSP_FLG	; FLG = port3 ? 0 : FLG_ECEN
	mov	a, SPC_PORT3		; 
	beq	_disable_echo		;
_enable_echo:				;
	mov	SPC_DSPD, #0		;
	jmp	NextCommand_R		;
_disable_echo:				;
	mov	SPC_DSPD, #FLG_ECEN	;
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_MVOL:
;-------------------------------------------------------------------------
	mov	SPC_DSPA, #DSP_MVOL	; copy volume 
	mov	SPC_DSPD, SPC_PORT2	;
	mov	SPC_DSPA, #DSP_MVOLR	;
	mov	SPC_DSPD, SPC_PORT3	;
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_RET:
;-------------------------------------------------------------------------
	mov	SPC_PORT1, comms_v	; return to main loop
	jmp	ExitComms		;
;-------------------------------------------------------------------------
CMD_KOF:
;-------------------------------------------------------------------------
	or	kof_flags, SPC_PORT3	; add kof flags
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_PITCH:
;-------------------------------------------------------------------------
	mov	x, SPC_PORT0		; copy pitch values
	mov	a, SPC_PORT2		;
	mov	channel_pitch_l-10h+x, a;
	mov	a, SPC_PORT3		;
	mov	channel_pitch_h-10h+x, a;---------------------------------
	mov	a, !bits-10h+y		; set pitch flag
	or	a, pitch_flags		;
	mov	pitch_flags, a		;---------------------------------
	mov	update_dsp, #1		; set update dsp flag
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_VOL:
;-------------------------------------------------------------------------
	mov	x, SPC_PORT0		; check for panning change
	mov	a, SPC_PORT2		;
	bmi	nopan			;---------------------------------
	mov	channel_panning-18h+x, a; copy pan
	mov	a, !bits-10h+y		; set pan flag
	or	a, panning_flags	;
	mov	panning_flags, a	;
	mov	update_dsp, #1		; set update dsp flag
					;---------------------------------
nopan:	mov	a, SPC_PORT3		; copy volume level and set bit
	mov	channel_volume-18h+x, a	;
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_KON:
;-------------------------------------------------------------------------
	mov	x, SPC_PORT0		; copy SRCN value
	mov	a, SPC_PORT3		;
	inc	a			; (+1)
	mov	channel_srcn-20h+x, a	;---------------------------------
	mov	a, SPC_PORT2		; copy starting offset
	mov	channel_ofs-20h+x, a	;---------------------------------
	mov	update_dsp, #1		; set update dsp flag
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------

;*************************************************************************
UpdateDSP:
;*************************************************************************
	
	mov	x, #0
	mov	SPC_DSPA, #00h
	
;-------------------------------------------------------------------------
_update_loop:
;-------------------------------------------------------------------------
	mov	y, channel_srcn+x	; test srcn value (skip if 0)
	beq	_no_keyon		;---------------------------------
	mov	a, channel_ofs+x	; test sample offset
	beq	_no_offset		; (skip if 0)
					;---------------------------------
					; TODO: sample offset
					;
_no_offset:				;---------------------------------
	or	SPC_DSPA, #04h		; set SRCN for voice
 	mov	SPC_DSPD, y		;
					;---------------------------------
	mov	a, channel_volume+x	; set volume level directly
	mov	channel_rvolume+x, a	;
					;---------------------------------
	mov	y, bits+x		; set KON bit for voice
	mov	a, #DSP_KON		;
	movw	SPC_DSP, ya		;
	mov	a, x			;
	xcn	a			;
	mov	SPC_DSPA, a		;
					;---------------------------------
	mov	y, #0			; clear data
	mov	channel_srcn+x, y	;
_no_keyon:				;---------------------------------
	lsr	panning_flags		; update panning if flag is set
	bcs	_skip_pan_update	;
	mov	a, channel_panning+x	;
	eor	a, #127			;
	mov	SPC_DSPD, a		;
	inc	SPC_DSPA		;
	eor	a, #127			;
	mov	SPC_DSPD, a		;
_skip_pan_update:			;---------------------------------
	mov	a, channel_rvolume+x	; set volume level
	or	SPC_DSPA, #5		;
	mov	SPC_DSPD, #0		;
	or	SPC_DSPA, #7		;
	mov	SPC_DSPD, a		;
					;---------------------------------
	mov	a, channel_ofs+x	; test sample offset
	beq	_no_offset2		; (skip if 0)
					;---------------------------------
					; TODO: restore sample address
					;
_no_offset2:				;---------------------------------
	or	SPC_DSPA, #15		; increment iterator
	inc	SPC_DSPA		;
	inc	x			;---------------------------------
	cmp	x, #8			; loop for 8 channels
	bne	_update_loop		;
;-------------------------------------------------------------------------
	ret
;-------------------------------------------------------------------------

;*************************************************************************
ProcessKeyOff:
;*************************************************************************
	mov	SPC_DSPA, #DSP_KOF	; set KOF
	mov	SPC_DSPD, kof_flags	;
	mov	kof_flags, #0		;
	ret				;
	
VS_RATE =5

;*************************************************************************
SlideVolumes:
;*************************************************************************
	mov	SPC_DSPA, #07h
	mov	x, #channel_volume
;-------------------------------------------------------------------------
_sv_loop:
;-------------------------------------------------------------------------
	mov	a, channel_rvolume-channel_volume+x
	beq	_off1			; AAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHH
	cmp	a, (x)			; slide volume for channel 0, 2, 4, 6
	beq	_sv1			;
	bcc	_up1			;
_dn1:	sbc	a, #VS_RATE		;
	cmp	a, (x)			;
	bcs	_wr1			;
	mov	a, (x)			;
	bra	_wr1			;
_off1:
	
_up1:	adc	a, #VS_RATE		;
	cmp	a, (x)			;
	bcc	_wr1			;
	mov	a, (x)			;
_wr1:	mov	SPC_DSPD, a		;
	setc				;
_sv1:	adc	SPC_DSPA, #0Fh		;
	inc	x			;
;-------------------------------------------------------------------------
	mov	a, channel_rvolume-channel_volume+x		
	cmp	a, (x)			; slide volume for channel 1, 3, 5, 7
	beq	_sv2			;
	bcc	_up2			;
_dn2:	sbc	a, #VS_RATE		;
	cmp	a, (x)			;
	bcs	_wr2			;
	mov	a, (x)			;
	bra	_wr2			;
_up2:	adc	a, #VS_RATE		;
	cmp	a, (x)			;
	bcc	_wr2			;
	mov	a, (x)			;
_wr2:	mov	SPC_DSPD, a		;
	setc				;
_sv2:	adc	SPC_DSPA, #0Fh		;
	inc	x			;
;-------------------------------------------------------------------------
	cmp	x, #channel_volume+8	;
	bcc	_sv_loop		;
;-------------------------------------------------------------------------
	ret
;-------------------------------------------------------------------------

;*****************************************************************************************
; data (remaining memory space minus echo region)
;*****************************************************************************************
	
;-------------------------------------------------------------------------
DataSector:
;-------------------------------------------------------------------------

.end

EOF
