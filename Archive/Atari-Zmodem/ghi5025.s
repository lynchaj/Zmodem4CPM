|
|  Make hi rez screen bios handle 50 lines of 8x8 characters
|
|  Adapted to Gnu C use from origional asm posting
|
|
	.text
	.globl _hi50

_hi50:				  | switch to 50 line mode
	moveml	a2/d2,sp@-

	.word	0xA000		  | get the important pointers (line A init)

	movl    a1@(4),a1	  | a1 -> 8x8 font header

	movl  a1@(72),a0@(-10)  | v_off_ad <- 8x8 offset table addr
	movl  a1@(76),a0@(-22)  | v_fnt_ad <- 8x8 font data addr

	moveq	#8,d0
	movw    d0,a0@(-0x2E)	| v_cel_ht <- 8    8x8 cell height	
	movw	a0@(-0x04),d1	| vertical pixel resolution
	extl	d1
	divu	d0,d1		| d1 : # rows : 50
	subl	#1,d1
	movw    d1,a0@(-0x2A)	| v_cel_my <- 49   maximum cell "Y"
	movw	a0@(0x02),d1	| ld_vwrap
	mulu	d0,d1		| * v_cel_ht
	movw	d1,a0@(-0x28)	| -> v_cel_my
				| offset to cell Y+1

	moveml	sp@+,a2/d2
	rts			  | and return

   
|
| Make hi rez screen bios handle 25 lines of 8x16 characters
|

	.even
	.globl _hi25

_hi25:				  | Switch to 25 lines display
	moveml	a2/d2,sp@-

	.word   0xA000		  | get the important pointers
	
	movl    a1@(8),a1	  | a1 -> 8x16 font header

	movl  a1@(72),a0@(-10)  | v_off_ad <- 8x16 offset table addr
	movl  a1@(76),a0@(-22)  | v_fnt_ad <- 8x16 font data addr

	moveq	#16,d0
	movw    d0,a0@(-0x2E)	  | v_cel_ht <- 16    8x16 cell height
	movw	a0@(-4),d1	  | v_rez_vt
	extl	d1
	divu	d0,d1		  | d1 = # rows: 25
	subl	#1,d1
	movw    d1,a0@(-0x2A)	  | v_cel_my <- 24    maximum cell "Y"
	movw	a0@(2),d1	  | ld_vwrap
	mulu	d0,d1		  | * v_cel_ht
	movw    d1,a0@(-0x28)	  |  v_cel_wr <- 1280  vertical byte offset

	moveml	sp@+,a2/d2
	rts			  | bye

|
| return lineA base
|
	.even
	.globl _aaddress

_aaddress:
	moveml	a2/d2,sp@-

	.word	0xa000		| a0 and d0 contain base
	moveml	sp@+,a2/d2
	rts
