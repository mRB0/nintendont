; SPCUNIT TESTING FACILITY

.include "inc\memmap.inc"		; memory map stuff
.include "inc\cheader.inc"		; snes rom header
.include "inc\InitSNES.asm"		; snes initialization code

.include "inc\snes.inc"			; snes register definitions

.BANK 1

.SECTION "GRAPHICS"			; include graphics (bank1)
.include "data\gfx_font.inc"
.ENDS

.BANK 0					; include test data (bank2)
.section "TEST_DATA"

spcunit_bin:
.incbin "..\spc\SPCUNIT.OBJ"
spcunit_end:


test_sample:
;.incbin "data\piano.brr"
.incbin "data\choir.brr"
test_sample_end:

.equ	SAMPLELOOP, 1287

.ends

.ramsection "globals" BANK 0 SLOT 1

joypad:		dw		; joypad state
joypadc:	dw		; joypad 'clicks'
joypadl:	dw		; last joypad state

svar1:		dw		; general purpose
svar2:		dw

scr_fade:	db
custom_string:	dsb 32


sf_a:		db
sf_b:		db
sf_c:		db
sf_v:		db

.ends

.BANK 0

.SECTION "MAIN"
;----------------------------------------------------------------------
; program entry point
;----------------------------------------------------------------------
Main:
;----------------------------------------------------------------------
	InitSNES		; initialize stuff
				;--------------------------------------
	REP	#$10		; 16bit index, 8bit akku
	SEP	#$20		;
				;--------------------------------------
	lda	#$80		; turn off screen
	sta	REG_INIDISP	;
				;--------------------------------------
	lda	#%00001001	; set display mode (1)
	sta	REG_BGMODE	;
				;--------------------------------------
	jsl	LoadFont	; load font and clear layer
	jsl	ClearAllText	; 
				;--------------------------------------
	ldx	#STR_SPCDATA	; draw "SPC Ports"
	ldy	#8*32+12	;
	jsl	DrawText	;
				;--------------------------------------
	lda	#%0100		; blend the window with the background
	sta	REG_TM		; enable bg2 main screen
	stz	REG_TD		;
				;--------------------------------------
				; Load driver
	jsl	BootSPC		;
;	jsl	UploadTest	;
				;--------------------------------------
	lda	#%10100001	; enable vblank irq, enable joypad
	sta	REG_NMITIMEN	;
	stz	$4016		;--------------------------------------
	cli			; enable interrupts
				;--------------------------------------
	wai			; wait for new frame
				;--------------------------------------
	lda	#255		; reset darkness
	sta	scr_fade	;
	lda	#15		;
	sta	REG_INIDISP	
;----------------------------------------------------------------------
_mainloop:
;----------------------------------------------------------------------

	;---------------------------------------------------------
	lda	scr_fade		; fade in screen
	cmp	#255			;
	beq	+			;
	clc				;
	adc	#20			;
	bcc	++			;
	lda	#255			;
++					;
	sta	scr_fade		;
	lsr				;
	lsr				;
	lsr				;
	lsr				;
	sta	REG_INIDISP		;
+	;---------------------------------------------------------
	jsl DrawSPCData			; Display SPC Port Data
	;---------------------------------------------------------
	; if user presses A, play effect1, full volume, freq:4

	;---------------------------------------------------------
	; Input A
	;---------------------------------------------------------
	lda joypadc
	and #%10000000
	beq +++++

	;------------------
	; Insert Code Here:
	;------------------
	
	jsl	UploadTest
	jsl	DoRet
+++++
	;---------------------------------------------------------
	; Input B
	;---------------------------------------------------------
	lda joypadc+1
	and #%10000000
	beq +++++

	;------------------
	; Insert Code Here:
	;------------------
	
	jsl	PlayTest
	
+++++	;---------------------------------------------------------
	; Input X
	;---------------------------------------------------------
	lda joypadc
	and #%1000000
	beq +++++
	
	;------------------
	; Insert Code Here:
	;------------------
	
	lda	#$19
	sta	REG_APUI00
	lda	#$80
	sta	REG_APUI02
	lda	#$10
	sta	REG_APUI03
	lda	sf_v
	eor	#128
	sta	sf_v
	sta	REG_APUI01
-	cmp	REG_APUI01	
	bne	-
	jsl	DoRet
	
+++++	;---------------------------------------------------------
	; Input Y
	;---------------------------------------------------------

	lda joypadc+1
	and #%1000000
	beq +++++
	
	;------------------
	; Insert Code Here
	;------------------

	lda	#$19
	sta	REG_APUI00
	lda	#$80
	sta	REG_APUI02
	lda	#$7F
	sta	REG_APUI03
	lda	sf_v
	eor	#128
	sta	sf_v
	sta	REG_APUI01
-	cmp	REG_APUI01	
	bne	-
	jsl	DoRet
	
+++++	;---------------------------------------------------------
	
	ldx #0			; clear clicks (stx?)
	stx joypadc		;
	wai			; wait for vblank
	jmp _mainloop		; loop
	
	
	

;###############################################################################
;...............................................................................
;-------------------------------------------------------------------------------
;###############################################################################

.equ REG_APUIO0	$2140
.equ REG_APUIO1	$2141
.equ REG_APUIO2	$2142
.equ REG_APUIO3	$2143

.equ	SPC_ENTRY, $300

;----------------------------------------------------------------------
; Boot SPC with SPCUNIT
;----------------------------------------------------------------------
BootSPC:
;----------------------------------------------------------------------
-	ldx	REG_APUIO0	; wait for 'ready' signal from SPC
	cpx	#0BBAAh		;
	bne	-		;--------------------------------------
	lda	#1		; start transfer:
	sta	REG_APUIO1	; port1   = !0
	inc	a		;
	inc	a
	stz	REG_APUIO2	; port2/3 = transfer address ( 0300h )
	sta	REG_APUIO3	;
	lda	#0CCh		; port0   = 0CCh
	sta	REG_APUIO0	;--------------------------------------
-	cmp	REG_APUIO0	; wait for SPC
	bne	-		;
;----------------------------------------------------------------------
; ready to transfer...
;----------------------------------------------------------------------
	lda.w	spcunit_bin	; read first byte
	xba			; 
	lda	#0		; 
	ldx	#1		; 
	bra	sb_start	; 
;----------------------------------------------------------------------
; transfer data
;----------------------------------------------------------------------
sb_send:
;----------------------------------------------------------------------
	xba			; swap DATA into A
	lda.w	spcunit_bin,x	; read next byte
	inx			;
	xba			; swap DATA into B
-	cmp	REG_APUIO0	; wait for spc
	bne	-		;
	ina			; increment counter (port0 data)
;----------------------------------------------------------------------
sb_start:	
;----------------------------------------------------------------------
	rep	#20h		; write port0+port1 data
	sta	REG_APUIO0	;
	sep	#20h		;--------------------------------------
	cpx	#spcunit_end-spcunit_bin	; all bytes transferred?
	bcc	sb_send		; no: transfer another byte
;----------------------------------------------------------------------
; all bytes transferred
;----------------------------------------------------------------------
-	cmp	REG_APUIO0	; wait for spc
	bne	-		;--------------------------------------
	ina			; add 2 or so...
	ina			;--------------------------------------
	stz	REG_APUIO1	; port1=0
	ldx	#SPC_ENTRY	; write program entry point
	stx	REG_APUIO2	;
	sta	REG_APUIO0	; write validation
				;--------------------------------------
-	cmp	REG_APUIO0	; final sync
	bne	-		;--------------------------------------
	stz	sf_v		; reset V
;----------------------------------------------------------------------
; driver installation successful
;----------------------------------------------------------------------
	rtl			; return
;----------------------------------------------------------------------	

;----------------------------------------------------------------------
UploadTest:
;----------------------------------------------------------------------

	lda	#00		; set message id, zero data, and dispatch
	sta	REG_APUI00	;
	lda	#SAMPLELOOP & $FF
	sta	REG_APUI02	;
	lda	#SAMPLELOOP >> 8
	sta	REG_APUI03	;
	lda	sf_v		;
	eor	#128		;
	ora	#1		;
	sta	sf_v		;
	sta	REG_APUI01	;
				;--------------------------------------
-	cmp	REG_APUI01	; wait for spc
	bne	-		;
				;--------------------------------------
	ldx	#0		; reset iterator

;----------------------------------------------------------------------
_next_word:
;----------------------------------------------------------------------
	lda.w	test_sample,x	; copy 2 bytes
	inx			;
	sta	REG_APUI02	;
	lda.w	test_sample,x	;
	inx			;
	sta	REG_APUI03	;
				;--------------------------------------
	cpx	#test_sample_end-test_sample	; catch end of data
	bcs	_end_of_transfer		;
					;------------------------------
	lda	sf_v			; dispatch packet
	eor	#128			;
	sta	sf_v			;
	sta	REG_APUI01		;------------------------------
-	cmp	REG_APUI01		; wait for spc
	bne	-			;
	bra	_next_word		; copy next word
;----------------------------------------------------------------------
_end_of_transfer:
;----------------------------------------------------------------------
	stz	REG_APUI01	; terminate transfer
	stz	sf_v		;
	lda	#0		;--------------------------------------
-	cmp	REG_APUI01	; wait for spc
	bne	-		;
	rtl			;
;----------------------------------------------------------------------

;----------------------------------------------------------------------
DoRet:
;----------------------------------------------------------------------
	lda	#$08		; set message id and dispatch
	sta	REG_APUI00	;
	lda	sf_v		;
	eor	#128		;
	sta	sf_v		;
	sta	REG_APUI01	;--------------------------------------
-	cmp	REG_APUI01	; wait for spc
	bne	-		;
	rtl			;
;----------------------------------------------------------------------

;----------------------------------------------------------------------
PlayTest:
;----------------------------------------------------------------------
	lda	#$11		; set frequency
	sta	REG_APUI00	;
	lda	#$11		;
	sta	REG_APUI02	;
	sta	REG_APUI03	;
	lda	sf_v		;
	eor	#128		;
	sta	REG_APUI01	;
	sta	sf_v		;
-	cmp	REG_APUI01	;
	bne	-		;--------------------------------------
	lda	#$19		; set volume&pan
	sta	REG_APUI00	;
	lda	#$60		;
	sta	REG_APUI02	;
	lda	#$7F		;
	sta	REG_APUI03	;
	lda	sf_v		;
	eor	#128		;
	sta	REG_APUI01	;
	sta	sf_v		;
-	cmp	REG_APUI01	;
	bne	-		;--------------------------------------
	lda	#$21		; keyon (volume:7f, sample:0)
	sta	REG_APUI00	;
	lda	#$7F
	sta	REG_APUI02	;
	stz	REG_APUI03	;
	lda	sf_v		;
	eor	#128		;
	sta	REG_APUI01	;
	sta	sf_v		;
-	cmp	REG_APUI01	;
	bne	-		;

	jsl	DoRet
	rtl

;###############################################################################
;...............................................................................
;-------------------------------------------------------------------------------
;###############################################################################








;-------------------------------------------------------------------------------
;
; TEXT RENDERING
;
;-------------------------------------------------------------------------------
	
DrawText:
	; x = source
	; y = offset
	stz	REG_VMAIN		; setup vram increment stuff
	rep	#$20			; set destination address
	tya				;
	ora	#($9000/2)		;
	sta	REG_VMADDL		;
	tay				;
	sec				;
	sep 	#$20			;
_dt_loop:
	lda 	$0000, x		;
	beq 	_dt_exit		; exit when 0
	inx				; increment counter
	sbc 	#32			; viewable ascii characters start at 32
	bpl 	+			; check for newline
	tya				; newline
	adc 	#32			; edit vram address
	sta 	REG_VMADDL		;
	tay				;
	sec				;
	bra 	_dt_loop		; loop
+
	sta 	REG_VMDATAL		; store value
	bra 	_dt_loop		; loop
_dt_exit:
	rtl				; end

ClearText:
	; x = length
	; y = offset

	stz 	REG_VMAIN		; setup vram pointer
	rep 	#$20			;
	tya				;
	ora 	#($9000/2)		;
	sta 	REG_VMADDL		;
	sep 	#$20
	lda 	#$00			; start clearing memory

_ct_loop:
	sta 	REG_VMDATAL		; store...
	dex					; count...
	bne 	_ct_loop		; loop...
_ct_exit:
	sep 	#$20			; restore 8-bit accu
	rtl				; end

;-------------------------------------------------------------------------------
ClearAllText:
;-------------------------------------------------------------------------------
	lda 	#%10000000		; setup vram pointer
	sta 	REG_VMAIN		;
	ldx 	#($9000/2)		;
	stx 	REG_VMADDL		;
	
	ldx 	#$100|(0<<10)|(1<<13)	; $100 = blank tile
	ldy 	#1024			; 1024 = 32*32 tiles
-
	stx 	REG_VMDATAL		; store value
	dey				; count..
	bne 	-			; loop
	rtl				; end

;-------------------------------------------------------------------------------
DrawCentered:
;-------------------------------------------------------------------------------
	; x = string address
	; y = line
	sty 	svar2
	stx 	svar1
	jsl 	strlen
	rep 	#$20
	txa
	sec
	sbc 	svar1
	lsr
	clc
	eor 	#$FFFF
	inc 	a
	adc 	svar2
	tay
	ldx 	svar1
	sep 	#$20
	jmp 	DrawText

;-------------------------------------------------------------------------------
DrawSPCData:
;-------------------------------------------------------------------------------
	lda 	#0
	xba

	ldx 	#0
	stx 	svar1
-
	lda 	REG_APUI00, x
	inx
	stx 	svar2
	pha
	lsr
	lsr
	lsr
	lsr
	clc
	tay
	lda 	HEX2ASCII, y
	ldx 	svar1
	sta 	custom_string,x
	inx
	pla
	and 	#$0F
	tay
	lda 	HEX2ASCII, y
	sta 	custom_string,x
	inx
	lda 	#32
	sta 	custom_string,x
	inx
	stx 	svar1
	ldx 	svar2
	cpx 	#4
	bne -
	ldy 	#11+(10*32)
	ldx 	#custom_string
	jsl 	DrawText
	rtl

;-------------------------------------------------------------------------------
;
; INITIALIZATION / EFFECTS
;
;-------------------------------------------------------------------------------

LoadFont:
	lda	#%10000000		; setup vram pointer
	sta	REG_VMAIN
	ldy	#($7000/2)
	sty	REG_VMADDL
	ldx	#0
	rep	#$20
	
-
	lda.l	gfx_font, x	; load byte
	sta	REG_VMDATAL	; store
	inx			; count
	inx
	cpx	#$600		; transfer $600*2 bytes
	bne	-
	sep	#$20		; setup palette
	lda	#0
	sta	REG_CGADD

	sta	REG_CGDATA	; dark blue
	lda	#$1C		
	sta	REG_CGDATA

	stz	REG_CGDATA
	stz	REG_CGDATA
	
	stz	REG_CGDATA	; black
	stz	REG_CGDATA

	lda	#$FF		; white
	sta	REG_CGDATA
	lda	#$7F
	sta	REG_CGDATA
	
	lda	#-1		; setup bg
	sta	REG_BG2VOFS
	stz	REG_BG2VOFS
	lda	#($12<<2)	; source = $9000
	sta	REG_BG2SC
	
	lda	#$03		; set character offset
	sta	REG_BG23NBA
	
	rtl

;-------------------------------------------------------------------------------
;
; DMA
;
;-------------------------------------------------------------------------------
	
DMA_TRANSFER:
	; x = src
	; y = length
	; a = bank#
	; b = dest
	stz	REG_DMAP0		; set mode
	stx	REG_A1T0L		; set source
	sta	REG_A1B0		; set bank#
	xba					; 
	sta	REG_BBAD0		; set dest
	sty	REG_DAS0L		; set #bytes
	lda	#1			; start transfer
	sta	REG_MDMAEN
	rtl				; end

;-------------------------------------------------------------------------------
;
; INTERRUPTS
;
;-------------------------------------------------------------------------------
.index 16
.accu 8

VBlank:
	sei
	rep	#$20
	pha			; preserve a

	lda	#(32*16)/2
	sta	REG_VMADDL
	lda	#580
	sta	REG_DAS1L
	SEP	#$20
	
	lda	#%10000000
	sta	REG_VMAIN
	lda	#%00001000
	sta	REG_DMAP1
	stz	REG_A1T1L
	stz	REG_A1B1
	lda	#REG_VMDATAL&255
	sta	REG_BBAD1
	lda	#2
	sta	REG_MDMAEN

	REP	#$20
	
	lda	joypad		; update last joypad state
	sta	joypadl		; 

	sep	#$20
	lda	#1
-
	bit	$4212		; check if joypad is ready
	bne	-			; wait...
	rep	#$20
	lda	REG_JOY1L		; load joystate
	sta	joypad		; save to memory
	eor	joypadl		; mask with old state
	and	joypad		; mask some more..
	sta	joypadc		; store button 'clicks'
	
	sep	#$20
	lda	REG_TIMEUP	; do something
	rep	#$20
	pla			; restore a
	rti			; exit

;-------------------------------------------------------------------------------
;
; misc
;
;-------------------------------------------------------------------------------

strlen:
	; x = str address
	; returns x = str address + length
-
	lda	$0000, x
	beq	+
	inx
	bra	-
+
	rtl

;-------------------------------------------------------------------------------
; STRINGS
;-------------------------------------------------------------------------------

STR_SPCDATA:
.db "SPC Ports",0

HEX2ASCII:
.db 48,49,50,51,52,53,54,55,56,57,65,66,67,68,69,70

.ENDS
