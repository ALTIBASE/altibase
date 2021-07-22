/* ****************************************************************** *
 * Copyright 1999-2006, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 * ****************************************************************** */

/* ****************************************************************** *
 * idkAtomic - is atomic operation for spurc v8plus 
 * compile such : as     -P -xarch=v8plus     sparcv9.S -o atomic.o
 * or could be  : CC/c++ -c -Wa,-xarch=v8plus sparcv9.S -o atomic.o
 * $Id: sparcv8p.S,v 1.1 2006/05/22 03:56:08 alex Exp $
 * ****************************************************************** */

#define ENTER(x) 	\
.text;				\
.align  16;			\
.globl  x;			\
.type   x, #function;\
x:			

#define LEAVE(x) 

/* ***********  AS - code here *********** */
	.file	"sparcv8p.S"

ENTER(idkAtomicINC1)
	ba	add_8
	add	%g0, 1, %o1
LEAVE(idkAtomicINC1)
	
ENTER(idkAtomicINC2)
	ba	add_16
	add	%g0, 1, %o1
LEAVE(idkAtomicINC2)

ENTER(idkAtomicINC4)
    ba  add_32
    add %g0, 1, %o1
LEAVE(idkAtomic4)

ENTER(idkAtomicINC8)
    ba  add_64
    add %g0, 1, %o1
LEAVE(idkAtomicINC8)

ENTER(idkAtomicDEC1)
	ba	add_8
	sub	%g0, 1, %o1
LEAVE(idkAtomicDEC1)	

ENTER(idkAtomicDEC2)
    ba  add_16
    sub %g0, 1, %o1
LEAVE(idkAtomicDEC2)	

ENTER(idkAtomicDEC4)
    ba  add_32
    sub %g0, 1, %o1
LEAVE(idkAtomicDEC4)

ENTER(idkAtomicDEC8)
    ba  add_64
    sub %g0, 1, %o1
LEAVE(idkAtomicDEC8)

ENTER(idkAtomicADD1)
add_8:
	and	%o0, 0x3, %o4		! %o4 = byte offset, left-to-right
	xor	%o4, 0x3, %g1		! %g1 = byte offset, right-to-left
	sll	%g1, 3, %g1			! %g1 = bit offset, right-to-left
	set	0xff, %o3			! %o3 = mask
	sll	%o3, %g1, %o3		! %o3 = shifted to bit offset
	sll	%o1, %g1, %o1		! %o1 = shifted to bit offset
	and	%o1, %o3, %o1		! %o1 = single byte value
	andn	%o0, 0x3, %o0	! %o0 = word address
	ld	[%o0], %o2			! read old value
1:
	add	%o2, %o1, %o5		! add value to the old value
	and	%o5, %o3, %o5		! clear other bits
	andn	%o2, %o3, %o4	! clear target bits
	or	%o4, %o5, %o5		! insert the new value
	cas	[%o0], %o2, %o5
	cmp	%o2, %o5
	bne,a,pn %icc, 1b
	mov	%o5, %o2			! %o2 = old value
	add	%o2, %o1, %o5
	and	%o5, %o3, %o5
	retl
	srl	%o5, %g1, %o0		! %o0 = new value
LEAVE(idkAtomicADD1)

ENTER(idkAtomicADD2)
add_16:
	and	%o0, 0x2, %o4		! %o4 = byte offset, left-to-right
	xor	%o4, 0x2, %g1		! %g1 = byte offset, right-to-left
	sll	%o4, 3, %o4			! %o4 = bit offset, left-to-right
	sll	%g1, 3, %g1			! %g1 = bit offset, right-to-left
	sethi	%hi(0xffff0000), %o3	! %o3 = mask
	srl	%o3, %o4, %o3		! %o3 = shifted to bit offset
	sll	%o1, %g1, %o1		! %o1 = shifted to bit offset
	and	%o1, %o3, %o1		! %o1 = single short value
	andn	%o0, 0x2, %o0	! %o0 = word address

	/* if low-order bit is 1, we will properly get an alignment fault here */
	ld	[%o0], %o2			/*read old value*/

1:
	add	%o1, %o2, %o5		/*add value to the old value*/
	and	%o5, %o3, %o5		/*clear other bits*/
	andn	%o2, %o3, %o4	/*clear target bits*/
	or	%o4, %o5, %o5		/*insert the new value */
	cas	[%o0], %o2, %o5
	cmp	%o2, %o5
	bne,a,pn %icc, 1b
  	mov	%o5, %o2			! %o2 = old value */
	add	%o1, %o2, %o5
	and	%o5, %o3, %o5
	retl
	srl	%o5, %g1, %o0		! %o0 = new value
LEAVE(idkAtomicADD2)

ENTER(idkAtomicADD4)
add_32:
	ld	[%o0], %o2
1:
	add	%o2, %o1, %o3
	cas	[%o0], %o2, %o3
	cmp	%o2, %o3
	bne,a,pn %icc, 1b
	mov	%o3, %o2
	retl
	add	%o2, %o1, %o0		! return new value
LEAVE(idkAtomicADD4)

ENTER(idkAtomicADD8)
add_64:
	ldx	[%o0], %o2
1:
	add	%o2, %o1, %o3
	casx	[%o0], %o2, %o3
	cmp	%o2, %o3
	bne,a,pn %xcc, 1b
    mov	%o3, %o2
	retl
	add	%o2, %o1, %o0		! return new value
LEAVE(idkAtomicADD)

ENTER(idkAtomicCAS1)
	and	%o0, 0x3, %o4		! %o4 = byte offset, left-to-right
	xor	%o4, 0x3, %g1		! %g1 = byte offset, right-to-left
	sll	%g1, 3, %g1			! %g1 = bit offset, right-to-left
	set	0xff, %o3			! %o3 = mask
	sll	%o3, %g1, %o3		! %o3 = shifted to bit offset
	sll	%o1, %g1, %o1		! %o1 = shifted to bit offset
	and	%o1, %o3, %o1		! %o1 = single byte value
	sll	%o2, %g1, %o2		! %o2 = shifted to bit offset
	and	%o2, %o3, %o2		! %o2 = single byte value
	andn	%o0, 0x3, %o0	! %o0 = word address
	ld	[%o0], %o4			! read old value
1:
	andn	%o4, %o3, %o4	! clear target bits
	or	%o4, %o2, %o5		! insert the new value
	or	%o4, %o1, %o4		! insert the comparison value
	cas	[%o0], %o4, %o5
	cmp	%o4, %o5			! did we succeed?
	be,pt	%icc, 2f
	  and	%o5, %o3, %o4	! isolate the old value
	cmp	%o1, %o4			! should we have succeeded?
	be,a,pt	%icc, 1b		! yes, try again
	mov	%o5, %o4			! %o4 = old value
2:
	retl
	srl	%o4, %g1, %o0		! %o0 = old value
LEAVE(idkAtomicCAS1)

ENTER(idkAtomicCAS2)
	and	%o0, 0x2, %o4				! %o4 = byte offset, left-to-right
	xor	%o4, 0x2, %g1				! %g1 = byte offset, right-to-left
	sll	%o4, 3, %o4					! %o4 = bit offset, left-to-right
	sll	%g1, 3, %g1					! %g1 = bit offset, right-to-left
	sethi	%hi(0xffff0000), %o3 	! %o3 = mask
	srl	%o3, %o4, %o3				! %o3 = shifted to bit offset
	sll	%o1, %g1, %o1				! %o1 = shifted to bit offset
	and	%o1, %o3, %o1				! %o1 = single short value
	sll	%o2, %g1, %o2				! %o2 = shifted to bit offset
	and	%o2, %o3, %o2				! %o2 = single short value
	andn	%o0, 0x2, %o0			! %o0 = word address

	/* if low-order bit is 1, we will properly get an alignment fault here */
	ld	[%o0], %o4					! read old value

1:
	andn	%o4, %o3, %o4			! clear target bits
	or	%o4, %o2, %o5				! insert the new value
	or	%o4, %o1, %o4				! insert the comparison value
	cas	[%o0], %o4, %o5
	cmp	%o4, %o5					! did we succeed?
	be,pt	%icc, 2f
	and	%o5, %o3, %o4				! isolate the old value
	cmp	%o1, %o4					! should we have succeeded?
	be,a,pt	%icc, 1b				! yes, try again
	mov	%o5, %o4					! %o4 = old value
2:
	retl
	srl	%o4, %g1, %o0				! %o0 = old value
LEAVE(idkAtomicCAS2)

ENTER(idkAtomicCAS4)
	cas	[%o0], %o1, %o2
	retl
	mov	%o2, %o0
LEAVE(idkAtomicCAS4)

ENTER(idkAtomicCAS8)
	casx	[%o0], %o1, %o2
	retl
	mov	%o2, %o0
LEAVE(idkAtomicCAS8)

# /* END */
