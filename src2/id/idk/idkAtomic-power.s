#* ****************************************************************** *
#* Copyright 1999-2006, ALTIBASE Corporation or its subsidiaries.
#* All rights reserved.
#* ****************************************************************** */

#* ****************************************************************** *
#* VAC :/bin/as -o atomic.o -many -a[32/64] power.s 
#* GNU : gas/as -o atomic.o -m620 -a[32/64] power.s
#* 	  both generate same for AIX 5.3 code of 620 processor and above
#*
#* GNU : c++ -c -maix[32/64] -mcpu=620 -o atomic.o power.S
#* VAC : xlC_r -g -q64  -c -maix       -o atomic.o power.s 
#*
#* $Id: power.s,v 1.1 2006/05/25 08:07:46 alex Exp $
#* ****************************************************************** */
	
	.file	"power.S"
    .machine "any"
#define ACQ_INSTR  isync
#define REL_INSTR  sync

#define SEM  ACQ_INSTR

#* ******************************* *
#* AIX standard register usage
#* ******************************* 
#* r3 - return value from function
#*
#* r3  - parameter 1 
#* r4  - parameter 2 
#* r5  - parameter 3 
#* r6  - parameter 4 
#* r7  - parameter 5 
#* r8  - parameter 6 
#* r9  - parameter 7 
#* r10 - parameter 4 
#* FPx - Floating point register
#* ******************************* */

#    .text;
    .globl idkAtomicINC4;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl idkAtomicINC4
    .globl .idkAtomicINC4
    .csect idkAtomicINC4[DS]
idkAtomicINC4:
    .long .idkAtomicINC4, TOC[tc0], 0
    .csect .text[PR]
    .idkAtomicINC4:
    eieio                   # SMP system write barrier
loopINC4:
    lwarx   6,0,3           # Load and reserve
    addi    6,6,1         
    stwcx.  6,0,3           # Store new value if still  reserved
    bne-    loopINC4        # Loop if lost reservation
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return

#   .text;
    .globl idkAtomicDEC4;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl idkAtomicDEC4
    .globl .idkAtomicDEC4
    .csect idkAtomicDEC4[DS]
idkAtomicDEC4:
    .long .idkAtomicDEC4, TOC[tc0], 0
    .csect .text[PR]
    .idkAtomicDEC4:
    eieio                   # SMP system write barrier
loopDEC4:
    lwarx   6,0,3           # Load and reserve
    subi    6,6,1
    stwcx.  6,0,3           # Store new value if still  reserved
    bne-    loopDEC4        # Loop if lost reservation
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return

#   .text;
    .globl idkAtomicADD4;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl idkAtomicADD4
    .globl .idkAtomicADD4
    .csect idkAtomicADD4[DS]
idkAtomicADD4:
    .long .idkAtomicADD4, TOC[tc0], 0
    .csect .text[PR]
    .idkAtomicADD4:

    eieio                   # SMP system write barrier
loopADD4:
    lwarx   6,0,3           # Load and reserve
    addi    6,6,4
    stwcx.  6,0,3           # Store new value if still  reserved
    bne-    loopADD4        # Loop if lost reservation
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return


######### CPP -> AS  code ######## #
# r3 - contains the address of the word to be tested.
# r4 - contains the value to be compared against the value in memory.
# r5 - contains the new value to be stored after a successful match.

#   .text;
    .globl idkAtomicCAS4;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl idkAtomicCAS4
    .globl .idkAtomicCAS4
    .csect idkAtomicCAS4[DS]
idkAtomicCAS4:
    .long .idkAtomicCAS4, TOC[tc0], 0
    .csect .text[PR]
    .idkAtomicCAS4:
	eieio                	# SMP system write barrier
loopCAS4:   
	lwarx   6,0,3          	# Load and reserve
	cmpw    4,6            	# Are the first two operands equal?
	bne-    exitCAS4        # Skip if not equal
	stwcx.  5,0,3          	# Store new value if still  reserved
	bne-    loopCAS4        # Loop if lost reservation
exitCAS4:   
	mr      3,6            	# Return value from storage
	isync                   # SMP system sync\n"
	blr 					# Return 

#   .text;
    .globl idkAtomicINC8;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl idkAtomicINC8
    .globl .idkAtomicINC8
    .csect idkAtomicINC8[DS]
idkAtomicINC8:
    .long .idkAtomicINC8, TOC[tc0], 0
    .csect .text[PR]
    .idkAtomicINC8:
    eieio                   # SMP system write barrier
loopINC8:
    ldarx   6,0,3           # Load and reserve
    addi    6,6,1         
    stdcx.  6,0,3           # Store new value if still  reserved
    bne-    loopINC8        # Loop if lost reservation
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return

#   .text;
    .globl idkAtomicDEC8;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl idkAtomicDEC8
    .globl .idkAtomicDEC8
    .csect idkAtomicDEC8[DS]
idkAtomicDEC8:
    .long .idkAtomicDEC8, TOC[tc0], 0
    .csect .text[PR]
    .idkAtomicDEC8:
    eieio                   # SMP system write barrier
loopDEC8:
    ldarx   6,0,3           # Load and reserve
    subi    6,6,1
    stdcx.  6,0,3           # Store new value if still  reserved
    bne-    loopDEC8        # Loop if lost reservation
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return

#   .text;
    .globl idkAtomicADD8;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl idkAtomicADD8
    .globl .idkAtomicADD8
    .csect idkAtomicADD8[DS]
idkAtomicADD8:
    .long .idkAtomicADD8, TOC[tc0], 0
    .csect .text[PR]
    .idkAtomicADD8:

    eieio                   # SMP system write barrier
loopADD8:
    ldarx   6,0,3           # Load and reserve
    addi    6,6,4
    stdcx.  6,0,3           # Store new value if still  reserved
    bne-    loopADD8        # Loop if lost reservation
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return


#   .text;
    .globl idkAtomicCAS8;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl idkAtomicCAS8
    .globl .idkAtomicCAS8
    .csect idkAtomicCAS8[DS]
idkAtomicCAS8:
    .long .idkAtomicCAS8, TOC[tc0], 0
    .csect .text[PR]
    .idkAtomicCAS8:
    eieio                   # SMP system write barrier
loopCAS8:
    ldarx   6,0,3           # Load and reserve
    cmpd    4,6             # Are the first two operands equal?
    bne-    exitCAS8        # Skip if not equal
    stdcx.  5,0,3           # Store new value if still  reserved
    bne-    loopCAS8        # Loop if lost reservation
exitCAS8:
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return

