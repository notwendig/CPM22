; **************************************************************
;	CBIOS for Z80-Simulator
;
;	Copyright (C) 1998-2010 by Juergen Sievers
;   Taken from the skeleton implementation.
;
;   Set memory limit here. This is the amount of contigeous
; ram starting from 0000. CP/M will reside at the end of this space.
;
MEM		EQU	64			; for a 64k system
;
;
;	"bias" is address offset from 3400H for memory systems
;	than 16K (referred to as "b" throughout the text).
;
RESET	EQU	0
BIAS	EQU	(MEM-20)*1024
CCP		EQU 3400H+BIAS	; base of ccp
BDOS	EQU	CCP+806H	; base of bdos
BIOS	EQU	CCP+1600H	; base of bios
CDISK	EQU	0004H		; current disk number 0=A,...,15=P
TDRIVE  EQU CDISK
IOBYTE	EQU	0003H		; intel i/o byte

ENTRY	EQU	5			; entry point for the cp/m bdos.
TFCB	EQU	5CH			; default file control block.
TBUFF	EQU	80H			; i/o buffer and command line storage.
TBASE	EQU	100H		; transiant program storage area.

NSECTS	EQU	(BDOS-CCP+127)/128 +1; warm start sector count
CSECTS  EQU (BIOS-CCP)/128 +1; cold start sector count
BSECTS	EQU (0FFFFH-BIOS)/128+1


NOFFST	EQU	0001H		; track,sector of BDOS
COFFST	EQU 0001H
BOFFST	EQU (((CSECTS/26) shl 8) | (CSECTS%26) +1)
MAGICID	EQU 1F2Eh
;
;   Set control character equates.
;
CNTRLC	EQU	3			; control-c
CNTRLE	EQU	05H			; control-e
BS		EQU	08H			; backspace
TAB		EQU	09H			; tab
LF		EQU	0AH			; line feed
FF		EQU	0CH			; form feed
CR		EQU	0DH			; carriage return
SPC		EQU 20H			; space
BEEP	EQU 7H			; beep

CNTRLP	EQU	10H			; control-p
CNTRLR	EQU	12H			; control-r
CNTRLS	EQU	13H			; control-s
CNTRLU	EQU	15H			; control-u
CNTRLX	EQU	18H			; control-x
CNTRLZ	EQU	1AH			; control-z (end-of-file mark)
DEL		EQU	7FH			; rubout

;	I/O ports
;
CONSTA	EQU	0			; console status port
CONDAT	EQU	1			; console data port
PRTSTA	EQU	2			; printer status port
PRTDAT	EQU	3			; printer data port
AUXDAT	EQU	5			; auxiliary data port
FDCD	EQU	10			; fdc-port: # of drive
FDCT	EQU	11			; fdc-port: # of track
FDCS	EQU	12			; fdc-port: # of sector
FDCOP	EQU	13			; fdc-port: command
FDCST	EQU	14			; fdc-port: status
DMAL	EQU	15			; dma-port: dma address low
DMAH	EQU	16			; dma-port: dma address high
DPBL	EQU 17			; dpb low
DPBH	EQU 18			; dpb high
; RESET	EQU	19			; System reset

CTRACE	EQU 80H			; 1=set, 0=unset trace
BREAKL	EQU 81H			; break-point low
BREAKH	EQU 82H			; break-point high
CBREAK	EQU	83H			; 1=set, 0=clr breakpoint

