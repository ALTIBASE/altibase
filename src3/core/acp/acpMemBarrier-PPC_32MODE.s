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
	
	.file	 "acpMemBarrier-PPC_32MODE.s"
    .machine "any"

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
    .globl acpMemBarrier;
    .csect .text[PR]
    .toc
    .csect .text[PR]
    .align 2
    .globl acpMemBarrier
    .globl .acpMemBarrier
    .csect acpMemBarrier[DS]
acpMemBarrier:
    .long .acpMemBarrier, TOC[tc0], 0
    .csect .text[PR]
    .acpMemBarrier:
    isync                   # SMP system sync\n"
    eieio                   # SMP system write barrier
    blr                     # Return

