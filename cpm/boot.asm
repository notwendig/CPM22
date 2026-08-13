

;       CP/M 2.2 boot-loader for Z80-Simulator
;	Copyright (C) 1998-2012 by Juergen Sievers
;
		include cpm.inc

		ORG     0           ; mem base of cp/m
;
;       begin the load operation
;
COLD:	DI
		LD		SP, TBUFF
		LD		HL, BOOTMSG
		LD		A,FF
		JR		MSG

BERR:	LD		HL,HALTMSG
		LD      A,BEEP

MSG:	OUT		(CONDAT), A
		LD		A, (HL)
		INC		HL
		CP		'$'
		JR		NZ, MSG

		LD      BC,BOFFST		; b=track, c=sector
		LD      D, BSECTS       ; d=# sectors to load
		LD      HL,BIOS         ; BIOS start address
		XOR		A               ; select drive A
		OUT		(DPBL),A
		OUT		(DPBH),A		; set default IBM 8" Disk 26 sector/track
		OUT     (FDCD),A
;
;       load the next sector
;
LSECT:	LD      A,B             ; set track
		OUT     (FDCT),A
		LD      A,C             ; set sector
		OUT     (FDCS),A
		LD      A,L             ; set dma address low
		OUT     (DMAL),A
		LD      A,H             ; set dma adress high
		OUT     (DMAH),A
		LD		A,'.'
		OUT		(CONDAT),A
		XOR     A               ; read sector
		OUT     (FDCOP),A
		IN      A,(FDCST)       ; get status of fdc
		OR      A               ; read successful ?
		JR      NZ,BERR         ; no, stop
		DEC     D               ; go to next sector if load is incomplete
		JR      NZ,NEXT         ; head for the bios
		LD		A,'+'
		OUT		(CONDAT),A
		JP		BIOS
;
;       more sectors to load
;
;       we aren't using a stack, so use <sp> as scratch register
;             to hold the load address increment
;
NEXT:	LD		A,128
		ADD		A,L
		LD		L,A
		LD		A,H
		ADC		A,0
		LD		H,A				; <hl> = <hl> + 128

;
		INC     C               ; sector = sector + 1
		LD      A,C
		CP      27              ; last sector of track ?
		JR      C,LSECT         ; no, go read another
;
;       end of track, increment to next track
;
		LD      C,1             ; sector = 1
		INC     B               ; track = track + 1
		JR      LSECT           ; for another group

HALTMSG:DB	CR,LF,"Error",CR,LF
BOOTMSG:DB	"BL V 1.0 boot:$"
		DC  80h-$-2,0ffh
		DW	NOT(MAGICID)
		END	COLD							; of boot loader
