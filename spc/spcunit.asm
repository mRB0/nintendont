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
; MVOL	01	Set Main Volume
;
; >> id vv LL RR
; << -- mm -- --
;
; LL = left level  (-128..127)
; RR = right level (-128..127)
;-------------------------------------------------------------
; EVOL	02	Set Echo Volume
;
; >> id vv LL RR
; << -- mm -- --
;
; LL = left evol (-128..127)
; RR = right evol (-128..127)
;-------------------------------------------------------------
; EDL	03	Set Echo Delay
;
; >> id vv -- DD
; << -- mm -- --
;
; DD = echo delay (0..15)
; [WARNING: WILL HALT SYSTEM FOR DD*16 MS]
;-------------------------------------------------------------
; EFB	04	Set Echo Feedback
; 
; >> id vv -- ff
; << -- mm -- --
;
; ff = new echo feedback level (-128..127)
;-------------------------------------------------------------
; COEF	05	Set Echo Coefficient
;
; >> id vv II CC
; << -- mm -- --
;
; II = index
; CC = coefficient (-128..127)
;-------------------------------------------------------------
; EON	06	Voice Echo Enable
;
; >> id vv -- ee
;
; ee = echo enable bits (1 per voice)
;-------------------------------------------------------------
; ECEN	07	Set Echo Enable
;
; >> id vv -- EE
; << -- mm -- --
;
; EE = 01=enable echo, 00=disable echo
;-------------------------------------------------------------
; RET	08	End of messages (return to processing)
;
; >> id vv -- --
; << -- mm -- --
;-------------------------------------------------------------
; RESET	09	Reset system
;
; >> id vv -- --
; << -- mm -- --
;-------------------------------------------------------------
; KOF	0A	Stop voices (KeyOFF)
;
; >> id vv -- aa
; << -- -- mm --
;
; aa = voices to stop (voice/bit)
;-------------------------------------------------------------
; OFS	0B	Sample Offset
;
; >> id vv ii oo
;
; ii = voice index
; oo = sample offset (0..255) (*256 sample offset)
;-------------------------------------------------------------
; PITCH	1x	Set Voice Pitch
; 
; >> 1x vv pp pp
; << -- mm -- --
;
; x = voice index
; pppp = pitch height (14-bit) port2 is LSB
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
; KON	2x	Start voice (KeyON)
;
; >> 2x vv VV ss
; << -- mm -- --
;
; VV = volume level
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

bits			=0	; 8 bytes	(index to bit conversion)
transfer_address	=8	; 2 bytes	(address to write next sample)
next_sample		=10	; 1 byte	(index of next sample)
comms_v			=11	; 1 byte	(communication variable)
channel_volume		=12	; 8 bytes	(gain) (target levels)
channel_rvolume		=20	; 8 bytes	(gain) (actual levels)
channel_panning		=28	; 8 bytes	(panning levels) (0..127)
channel_ofs		=36	; 8 bytes	(sample offsets for keyon)
channel_srcn		=44	; 8 bytes	(srcn+1/keyon)
channel_pitch_l		=52	; 8 bytes	(PL)
channel_pitch_h		=60	; 8 bytes	(PH)
kof_flags		=68	; 1 byte	(keyoff flags)
panning_flags		=69	; 1 byte	(update panning flags)
pitch_flags		=70	; 1 byte	(update pitch flags)
update_dsp		=71	; 1 byte	(update dsp boolean)
m0			=72	; 1 byte	(scratch variable 1)
m1			=73	; 1 byte	(scratch variable 2)
m2			=74	; 1 byte	(scratch variable 3)
m3			=75	; 1 byte	(scratch variable 4)

;*****************************************************************************************
; sample directory
;*****************************************************************************************

SampleDirectory		=0200h	; 256 bytes	(64-sample directory)

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
	
	
	call	ResetMemory
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
ExitComms:				;
	call	SlideVolumes		; perform volume sliding
	bra	_main_loop		;
;-------------------------------------------------------------------------

;-------------------------------------------------------------------------
ResetMemory:
;-------------------------------------------------------------------------
	mov	transfer_address, #(DataSector & 0ffh)	; setup transfer addresses
	mov	transfer_address+1, #(DataSector >> 8)	;
	mov	next_sample, #0
	ret

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
	.word	CMD_MVOL		; 01
	.word	CMD_EVOL		; 02
	.word	CMD_EDL			; 03
	.word	CMD_EFB			; 04
	.word	CMD_COEF		; 05
	.word	CMD_EON			; 06
	.word	CMD_ECEN		; 07
	.word	CMD_RET			; 08
	.word	CMD_RESET		; 09
	.word	CMD_KOF			; 0A
	.word	CMD_OFS			; 0B
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
CMD_MVOL:
;-------------------------------------------------------------------------
	mov	SPC_DSPA, #DSP_MVOL	; copy volume 
	mov	SPC_DSPD, SPC_PORT2	;
	mov	SPC_DSPA, #DSP_MVOLR	;
	mov	SPC_DSPD, SPC_PORT3	;
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_EVOL:
;-------------------------------------------------------------------------
	mov	SPC_DSPA, #DSP_EVOL	; copy volume levels into dsp registers
	mov	SPC_DSPD, SPC_PORT2	;
	mov	SPC_DSPA, #DSP_EVOLR	;
	mov	SPC_DSPD, SPC_PORT3	;
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_EDL:
;-------------------------------------------------------------------------
	mov	a, SPC_PORT3		; set ESA to 256 - EDL * 8
	asl	a			;
	asl	a			;
	asl	a			;
	eor	a, #255			;
	inc	a			;
	bne	_l1			;
	dec	a			;
_l1:	mov	SPC_DSPA, #DSP_ESA	;
	mov	SPC_DSPD, a		;
					;---------------------------------
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
CMD_EFB:
;-------------------------------------------------------------------------
	mov	y, SPC_PORT3		; copy new EFB value to DSP
	mov	a, #DSP_EFB		;
	movw	SPC_DSP, ya		;
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_COEF:
;-------------------------------------------------------------------------
	mov	a, SPC_PORT2		; compute dsp address (DSP_C0 + p2*16)
	xcn	a			;
	clrc				;
	adc	a, #DSP_C0		;
	mov	SPC_DSPA, a		;---------------------------------
	mov	SPC_DSPD, SPC_PORT3	; copy value into dsp coefficient
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_EON:
;-------------------------------------------------------------------------
	mov	y, SPC_PORT3		; copy new EON value to DSP
	mov	a, #DSP_EON		;
	movw	SPC_DSP, ya		;
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
CMD_RET:
;-------------------------------------------------------------------------
	mov	SPC_PORT1, comms_v	; process data and return to main loop
	call	ProcessKeyOff		;
	call	UpdateDSP		;
	jmp	ExitComms		;
;-------------------------------------------------------------------------
CMD_RESET:
;-------------------------------------------------------------------------
	mov	SPC_DSPA, #DSP_KOF	; turn off channels
	mov	SPC_DSPD, #0FFh		;---------------------------------
	call	ResetMemory		; reset memory
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_KOF:
;-------------------------------------------------------------------------
	or	kof_flags, SPC_PORT3	; add kof flags
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_OFS:
;-------------------------------------------------------------------------
	mov	x, SPC_PORT2		; set sample offset for voice
	mov	a, SPC_PORT3		; (should be followed by KeyON)
	mov	channel_ofs+x, a	;
	ret				;
;-------------------------------------------------------------------------
CMD_PITCH:
;-------------------------------------------------------------------------
	mov	x, SPC_PORT0		; copy pitch values
	mov	a, SPC_PORT2		;
	mov	channel_pitch_l-10h+x, a;
	mov	a, SPC_PORT3		;
	mov	channel_pitch_h-10h+x, a;---------------------------------
	mov	a, bits-10h+x		; set pitch flag
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
	mov	channel_panning-18h+x, a; copy pan and set flag
	mov	a, bits-18h+x		; 
	or	a, panning_flags	;
	mov	panning_flags, a	;
	mov	update_dsp, #1		; set update dsp flag
					;---------------------------------
nopan:	mov	a, SPC_PORT3		; copy volume level
	mov	channel_volume-18h+x, a	;
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------
CMD_KON:
;-------------------------------------------------------------------------
	mov	x, SPC_PORT0		; copy SRCN value
	mov	a, SPC_PORT3		;
	inc	a			; (+1)
	mov	channel_srcn-20h+x, a	;---------------------------------
	mov	a, SPC_PORT2		; copy volume
	mov	channel_volume-20h+x, a	;---------------------------------
	mov	update_dsp, #1		; set update dsp flag
	jmp	NextCommand_R		;
;-------------------------------------------------------------------------

;*************************************************************************
UpdateDSP:
;*************************************************************************
	cmp	update_dsp, #0		; skip update if flag is not set
	bne	_update_dsp_check	;
	ret				;
_update_dsp_check:			;
	mov	update_dsp, #0		;---------------------------------
					; init variables
	mov	x, #0			; x = channel counter
	mov	SPC_DSPA, #00h		; DSPA = first voice
					;
;-------------------------------------------------------------------------
_update_loop:
;-------------------------------------------------------------------------
	mov	y, channel_srcn+x	; test srcn value (skip if 0)
	beq	_no_keyon		;---------------------------------
	mov	a, channel_ofs+x	; test sample offset
	beq	_no_offset		; (skip if 0)
					;---------------------------------
	mov	y, #0			; reset channel offset
	mov	channel_ofs+x, y	;
					;---------------------------------
	mov	y, #144			; m0,m1 = offset * 144 (256/16*9 samples)
	mul	ya			;
	movw	m0, ya			;---------------------------------
	mov	a, channel_srcn+x	; y = sample*4
	asl	a			;
	asl	a			;
	mov	y, a			;---------------------------------
	mov	a, !SampleDirectory+1+y	; m2 = original address hi-byte
	mov	m2, a			;---------------------------------
	mov	a, !SampleDirectory+y	; push original address lo-byte
	push	a			;
					;---------------------------------
	adc	a, m0			; add m0, m1 to sample address
	mov	!SampleDirectory+y, a	;
	mov	a, m2			;
	adc	a, m1			;
	mov	!SampleDirectory+1+y, a	;---------------------------------
	push	y			; save offset for restoring
_no_offset:				;---------------------------------
	mov	y, channel_srcn+x	; set SRCN for voice
	or	SPC_DSPA, #04h		; 
	dec	y			;
 	mov	SPC_DSPD, y		;
					;---------------------------------
	mov	y, bits+x		; set KON bit for voice
	mov	a, #DSP_KON		;
	movw	SPC_DSP, ya		;
	mov	a, x			;
	xcn	a			;
	mov	SPC_DSPA, a		;
					;---------------------------------
	mov	a, channel_volume+x	; set volume level directly
	mov	channel_rvolume+x, a	;
	or	SPC_DSPA, #5		;
	mov	SPC_DSPD, #0		;
	or	SPC_DSPA, #7		;
	mov	SPC_DSPD, a		;
	and	SPC_DSPA, #0F0h		;
					;---------------------------------
	mov	y, #0			; clear data
	mov	channel_srcn+x, y	;
_no_keyon:				;---------------------------------
	lsr	panning_flags		; update panning if flag is set
	bcc	_skip_pan_update	;
	mov	a, channel_panning+x	;
	eor	a, #127			;
	mov	SPC_DSPD, a		;
	inc	SPC_DSPA		;
	eor	a, #127			;
	mov	SPC_DSPD, a		;
	inc	SPC_DSPA		;
_skip_pan_update:			;---------------------------------
	lsr	pitch_flags		; update pitch if flag is set
	bcc	_skip_pitch_update	;
	mov	a, channel_pitch_l+x	;
	or	SPC_DSPA, #02h		;
	mov	SPC_DSPD, a		;
	mov	a, channel_pitch_h+x	;
	inc	SPC_DSPA		;
	mov	SPC_DSPD, a		;
_skip_pitch_update:			;
					;---------------------------------
	mov	a, channel_ofs+x	; test sample offset
	beq	_no_offset2		; (skip if 0)
					;---------------------------------
	pop	y			; restore sample address
	pop	a			;
	mov	!SampleDirectory+y, a	;
	mov	a, m2			;
	mov	!SampleDirectory+1+y, a	;
					;
_no_offset2:				;---------------------------------
	inc	x			; increment iterator
	or	SPC_DSPA, #15		;
	inc	SPC_DSPA		; 
	bmi	_exit_update		; break after 8 channels
	jmp	_update_loop		;
;-------------------------------------------------------------------------
_exit_update:
;-------------------------------------------------------------------------
	ret
;-------------------------------------------------------------------------

;*************************************************************************
ProcessKeyOff:
;*************************************************************************
	mov	x, #channel_volume-1	;
	mov	a, #0			;---------------------------------
_pko_loop:				; loop through kof bits and
	inc	x			; clear channel volumes
	lsr	kof_flags		;
	bcc	_pko_nextbit		;
	mov	(x), a			;
_pko_nextbit:				;
	bne	_pko_loop		;
	ret				;
;-------------------------------------------------------------------------
	
VS_RATE		=2
GAIN_OFF	=10011110b

;*************************************************************************
SlideVolumes:
;*************************************************************************
	mov	SPC_DSPA, #07h
	mov	x, #channel_volume
;-------------------------------------------------------------------------
_sv_loop:
;-------------------------------------------------------------------------
	mov	a, (x)			; special behavior for volume 0
	beq	_sv_off			;---------------------------------
	mov	a, channel_rvolume-channel_volume+x
	cmp	a, (x)			; test slide direction
	beq	_no_slide		;
	bcc	_slide_up		;
;-------------------------------------------------------------------------
_slide_down:				; subtract and clamp to lower boundary
	sbc	a, #VS_RATE		;
	bmi	_c1			;
	cmp	a, (x)			;
	bcs	_writegain		;
_c1:	mov	a, (x)			;
	bra	_writegain		;
;-------------------------------------------------------------------------
_sv_off:				; set gain to decrease mode
	mov	SPC_DSPD, #GAIN_OFF	;
	mov	a, #0			;
	bra	_writevol		;
;-------------------------------------------------------------------------
_slide_up:				; add and clamp to higher boundary
	adc	a, #VS_RATE		;
	cmp	a, (x)			;
	bcc	_writegain		;
	mov	a, (x)			;
;-------------------------------------------------------------------------
_writegain:				; set GAIN value
	mov	SPC_DSPD, a		;
;-------------------------------------------------------------------------
_writevol:				; set volume level
	mov	channel_rvolume-channel_volume+x, a
	setc				;
;-------------------------------------------------------------------------
_no_slide:				; increment iterator
	adc	SPC_DSPA, #0Fh		; [carry is set: add 10h]
	inc	x			;
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
