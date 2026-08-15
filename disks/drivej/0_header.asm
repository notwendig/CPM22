;
;	One record empty header to get boot code into place.
;	The boot code used fits into a single record, but two
;	could be used if needed.
;
;	11/2006, Udo Munk
;
	org	0000h

	rept	80h
	db	0
	endm

	end
