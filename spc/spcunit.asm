;*****************************************************************************************
; SPCUNIT
;
; (spc sound server)
;
; by mukunda
;*****************************************************************************************

;*****************************************************************************************
; PROTOCOLS
;*****************************************************************************************
; mm = mimic data
; id = message id
; vv = validation data (not previous value)
;
; NAME	ID	DESC
; LOAD	00	Load Sample
; 
; >> id vv LL LL
;
; LLLL = loop point
; ...
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



;*****************************************************************************************
; program (load @ 200h)
;*****************************************************************************************
.org	0200h

;-------------------------------------------------------------------------
__StartProgram__:
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
	
_main_loop:
	
	bra	_main_loop

;----------------------------------------------------------
; disable mute and reset
;----------------------------------------------------------
ResetSound:
	mov	SPC_DSPA, #DSP_FLG
	mov	SPC_DSPD, #FLG_ECEN
	mov	SPC_DSPA, #DSP_PMON
	mov	SPC_DSPD, #0
	
	mov	SPC_DSPA, #DSP_MVOL
	mov	SPC_DSPD, #80
	mov	SPC_DSPA, #DSP_MVOLR
	mov	SPC_DSPD, #80
	ret
	
.end

EOF
