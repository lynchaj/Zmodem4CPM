/
/  Make hi rez screen bios handle 50 lines of 8x8 characters
/
/  Adapted to Mark Williams C use from origional PD asm posting
/  from atari corp.
/
/ 		Jwahar Bammi
/ 			usenet: cwruecmp!bammi@decvax.UUCP
/			csnet:  bammi@cwru.edu
/			arpa:   bammi@cwru.edu
/ 			CompuServe: 71515,155
/ 
/
		.shri

	.globl hi50_

hi50_:				  / switch to 8x8 font
/	link	a6,$0		  / routine preamble

	.word	0xA000		  / get the important pointers (line A init)

	movea.l    4(a1),a1	  / a1 -> 8x8 font header

	move.l  72(a1),-0x0A(a0)  / v_off_ad <- 8x8 offset table addr
	move.l  76(a1),-0x16(a0)  / v_fnt_ad <- 8x8 font data addr

	moveq	$8,d0
	move    d0,  -0x2E(a0)	  / v_cel_ht <- 8    8x8 cell height	
	move	-0x04(a0),d1	/ vertical pixel resolution
	ext.l	d1
	divu	d0,d1		/ d1 : # rows : 50
	subq	$1,d1
	move    d1, -0x2A(a0)	  / v_cel_my <- 49   maximum cell "Y"
	move	0x02(a0),d1	/ ld_vwrap
	mulu	d0,d1		/ * v_cel_ht
	move	d1,-0x28(a0)	/ -> v_cel_my
				/ offset to cell Y+1

/	unlk	a6		  / routine postable
	rts			  / and return

   
/
/ Make hi rez screen bios handle 25 lines of 8x16 characters
/

	.globl hi25_

hi25_:				  / Switch to 8x16 font
/	link	a6,$0		  / routine preamble

	.word   0xA000		  / get the important pointers
	
	movea.l    8(a1),a1	  / a1 -> 8x16 font header

	move.l  72(a1),-0x0A(a0)  / v_off_ad <- 8x16 offset table addr
	move.l  76(a1),-0x16(a0)  / v_fnt_ad <- 8x16 font data addr

	moveq	$16,d0
	move    d0,-0x2E(a0)	  / v_cel_ht <- 16    8x16 cell height
	move	-4(a0),d1	  / v_rez_vt
	ext.l	d1
	divu	d0,d1		  / d1 = # rows: 25
	subq	$1,d1
	move    d1,  -0x2A(a0)	  / v_cel_my <- 24    maximum cell "Y"
	move	2(a0),d1	/ ld_vwrap
	mulu	d0,d1		/ * v_cel_ht

	move    d1,-0x28(a0)	  /  v_cel_wr <- 1280  vertical byte offset

/	unlk	a6		  / routine postamble
	rts			  / bye
