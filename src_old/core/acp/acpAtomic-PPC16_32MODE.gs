#*******************************************************************************
#  Copyright 1999-2009, ALTIBASE Corporation or its subsidiaries.
#  All rights reserved.
#*******************************************************************************

#*******************************************************************************
#  $Id:
#*******************************************************************************


	.csect .text[PR]
	.toc
	.csect .text[PR]
	.align 2
	.globl acpAtomicCas16
	.globl .acpAtomicCas16
	.csect acpAtomicCas16[DS]
acpAtomicCas16:
	.long .acpAtomicCas16, TOC[tc0], 0
	.csect .text[PR]
.acpAtomicCas16:
	stw 31,-4(1)
	stwu 1,-32(1)
	mr 31,1
	stw 3,56(31)
	mr 0,4
	mr 9,5
	sth 0,60(31)
	sth 9,64(31)
	lhz 0,64(31)
	extsh 0,0
	rlwinm 9,0,0,0xffff
	lhz 0,60(31)
	extsh 0,0
	rlwinm 0,0,0,0xffff
	lwz 7,56(31)
	rlwinm 11,7,3,27,27
	xori 11,11,16
	rlwinm 8,9,0,16,31
	slw 8,8,11
	rlwinm 10,0,0,16,31
	slw 10,10,11
	li 0,0
	ori 0,0,65535
	slw 0,0,11
	rlwinm 9,7,0,0,29
	.long 0x7c2004ac
L..3:
	lwarx 7,0,9
	and 6,7,0
	cmpw 0,6,8
	bne 0,L..4
	andc 7,7,0
	or 7,7,10
	stwcx. 7,0,9
	bne 0,L..3
	isync
L..4:
	mr 0,6
	srw 0,0,11
	rlwinm 0,0,0,0xffff
	extsh 0,0
	mr 3,0
	lwz 1,0(1)
	lwz 31,-4(1)
	blr
LT..acpAtomicCas16:
	.long 0
	.byte 0,0,32,96,128,1,3,1
	.long 0
	.long LT..acpAtomicCas16-.acpAtomicCas16
	.short 14
	.byte "acpAtomicCas16"
	.byte 31
	.align 2
	.align 2
	.globl acpAtomicSet16
	.globl .acpAtomicSet16
	.csect acpAtomicSet16[DS]
acpAtomicSet16:
	.long .acpAtomicSet16, TOC[tc0], 0
	.csect .text[PR]
.acpAtomicSet16:
	stw 31,-4(1)
	stwu 1,-64(1)
	mr 31,1
	stw 3,88(31)
	mr 0,4
	sth 0,92(31)
	lhz 0,92(31)
	extsh 0,0
	rlwinm 0,0,0,0xffff
	stw 0,24(31)
	lwz 6,88(31)
	stw 6,28(31)
	lwz 7,28(31)
	lhz 7,0(7)
	sth 7,34(31)
L..6:
	lhz 9,34(31)
	sth 9,32(31)
	lwz 6,28(31)
	rlwinm 10,6,3,27,27
	xori 10,10,16
	lhz 0,32(31)
	rlwinm 8,0,0,16,31
	slw 8,8,10
	lwz 7,24(31)
	rlwinm 11,7,0,16,31
	slw 11,11,10
	li 0,0
	ori 0,0,65535
	slw 0,0,10
	lwz 6,28(31)
	rlwinm 9,6,0,0,29
	.long 0x7c2004ac
L..8:
	lwarx 7,0,9
	and 6,7,0
	cmpw 0,6,8
	bne 0,L..9
	andc 7,7,0
	or 7,7,11
	stwcx. 7,0,9
	bne 0,L..8
	isync
L..9:
	mr 0,6
	srw 0,0,10
	sth 0,34(31)
	lhz 9,34(31)
	lhz 0,32(31)
	cmpw 7,9,0
	bne 7,L..6
	lhz 0,32(31)
	extsh 0,0
	mr 3,0
	lwz 1,0(1)
	lwz 31,-4(1)
	blr
LT..acpAtomicSet16:
	.long 0
	.byte 0,0,32,96,128,1,2,1
	.long 0
	.long LT..acpAtomicSet16-.acpAtomicSet16
	.short 14
	.byte "acpAtomicSet16"
	.byte 31
	.align 2
	.align 2
	.globl acpAtomicAdd16
	.globl .acpAtomicAdd16
	.csect acpAtomicAdd16[DS]
acpAtomicAdd16:
	.long .acpAtomicAdd16, TOC[tc0], 0
	.csect .text[PR]
.acpAtomicAdd16:
	stw 31,-4(1)
	stwu 1,-48(1)
	mr 31,1
	stw 3,72(31)
	mr 0,4
	sth 0,76(31)
	lhz 0,76(31)
	extsh 0,0
	rlwinm 11,0,0,0xffff
	lwz 0,72(31)
	.long 0x7c2004ac
	rlwinm 10,0,3,27,27
	xori 10,10,16
	rlwinm 9,0,0,0,29
	rlwinm 11,11,0,16,31
	slw 11,11,10
	li 0,0
	ori 0,0,65535
	slw 0,0,10
	lwarx 6,0,9
	add 7,6,11
	andc 8,6,0
	and 7,7,0
	or 7,7,8
	stwcx. 7,0,9
	bne- $-24
	mr 0,7
	srw 0,0,10
	isync
	rlwinm 0,0,0,0xffff
	extsh 0,0
	mr 3,0
	lwz 1,0(1)
	lwz 31,-4(1)
	blr
LT..acpAtomicAdd16:
	.long 0
	.byte 0,0,32,96,128,1,2,1
	.long 0
	.long LT..acpAtomicAdd16-.acpAtomicAdd16
	.short 14
	.byte "acpAtomicAdd16"
	.byte 31
	.align 2
_section_.text:
	.csect .data[RW],4
	.long _section_.text
