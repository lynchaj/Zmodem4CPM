;
;  Make hi rez screen bios handle 50 lines of 8x8 characters
;
;  Adapted to Manx C use from origional PD asm posting
;  from atari corp.
;
; 		Jwahar Bammi
; 			usenet: cwruecmp!bammi@decvax.UUCP
;			csnet:  bammi@cwru.edu
;			arpa:   bammi@cwru.edu
; 			CompuServe: 71515,155
; 
;
	cseg

	public _hi50

_hi50:				  ; switch to 50 line mode
	link	a6,#0		  ; routine preamble

	dc.w	$A000		  ; get the important pointers (line A init)

	movea.l    4(a1),a1	  ; a1 -> 8x8 font header

	move.l  72(a1),-$0A(a0)   ; v_off_ad <- 8x8 offset table addr
	move.l  76(a1),-$16(a0)   ; v_fnt_ad <- 8x8 font data addr

	move    #8,-$2E(a0)	  ; v_cel_ht <- 8    8x8 cell height
	move    #49,-$2A(a0)	  ; v_cel_my <- 49   maximum cell "Y"
	move    #640,-$28(a0)     ; v_cel_wr <- 640  offset to cell Y+1

	unlk	a6		  ; routine postable
	rts			  ; and return

   
;
; Make hi rez screen bios handle 25 lines of 8x16 characters
;

	public _hi25

_hi25:				  ; Switch to 25 lines display
	link	a6,#0		  ; routine preamble

	dc.w    $A000		  ; get the important pointers
	
	movea.l    8(a1),a1	  ; a1 -> 8x16 font header

	move.l  72(a1),-$0A(a0)   ; v_off_ad <- 8x16 offset table addr
	move.l  76(a1),-$16(a0)   ; v_fnt_ad <- 8x16 font data addr

	move    #16,-$2E(a0)	  ; v_cel_ht <- 16    8x16 cell height
	move    #24,-$2A(a0)	  ; v_cel_my <- 24    maximum cell "Y"
	move    #1280,-$28(a0)	  ;  v_cel_wr <- 1280  vertical byte offset

	unlk	a6		  ; routine postamble
	rts			  ; bye


; return the base address of the line A variables 

	public _aaddress

_aaddress:
	link	a6,#0
	dc.w $A000		; Line A trap - 0000 is init aline
				; d0 and a0 now contain the address
				; so we can just return and the result
				; will be valid
	unlk	a6
	rts

	end
