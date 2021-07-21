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
	
	.file	 "acpAtomic-PPC64.s"
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
    .globl acpAtomicInc32;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl acpAtomicInc32
    .globl .acpAtomicInc32
    .csect acpAtomicInc32[DS]
acpAtomicInc32:
    .long .acpAtomicInc32, TOC[tc0], 0
    .csect .text[PR]
    .acpAtomicInc32:
    eieio                   # SMP system write barrier
loopInc32:
    lwarx   6,0,3           # Load and reserve
    addi    6,6,1         
    stwcx.  6,0,3           # Store new value if still  reserved
    bne-    loopInc32       # Loop if lost reservation
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return

#   .text;
    .globl acpAtomicDec32;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl acpAtomicDec32
    .globl .acpAtomicDec32
    .csect acpAtomicDec32[DS]
acpAtomicDec32:
    .long .acpAtomicDec32, TOC[tc0], 0
    .csect .text[PR]
    .acpAtomicDec32:
    eieio                   # SMP system write barrier
loopDec32:
    lwarx   6,0,3           # Load and reserve
    subi    6,6,1
    stwcx.  6,0,3           # Store new value if still  reserved
    bne-    loopDec32       # Loop if lost reservation
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return

#   .text;
    .globl acpAtomicAdd32;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl acpAtomicAdd32
    .globl .acpAtomicAdd32
    .csect acpAtomicAdd32[DS]
acpAtomicAdd32:
    .long .acpAtomicAdd32, TOC[tc0], 0
    .csect .text[PR]
    .acpAtomicAdd32:

    eieio                   # SMP system write barrier
loopAdd32:
    lwarx   6,0,3           # Load and reserve
    add     6,6,4
    stwcx.  6,0,3           # Store new value if still  reserved
    bne-    loopAdd32       # Loop if lost reservation
    mr      3,6             # Return value from storage
    isync                   # SMP system sync\n"
    blr                     # Return


######### CPP -> AS  code ######## #
# r3 - contains the address of the word to be tested.
# r4 - contains the value to be compared against the value in memory.
# r5 - contains the new value to be stored after a successful match.

#   .text;
    .globl acpAtomicCas32;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl acpAtomicCas32
    .globl .acpAtomicCas32
    .csect acpAtomicCas32[DS]
acpAtomicCas32:
    .long .acpAtomicCas32, TOC[tc0], 0
    .csect .text[PR]
    .acpAtomicCas32:
	eieio                	# SMP system write barrier
loopCas32:   
	lwarx   6,0,3          	# Load and reserve
	cmpw    5,6            	# Are the first two operands equal?
	bne-    exitCas32       # Skip if not equal
	stwcx.  4,0,3          	# Store new value if still  reserved
	bne-    loopCas32       # Loop if lost reservation
exitCas32:   
	mr      3,6            	# Return value from storage
	isync                   # SMP system sync\n"
	blr 					# Return 

