#*******************************************************************************
# * Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# * All rights reserved.
#*******************************************************************************

#*******************************************************************************
#* $Id$
#*******************************************************************************

# acpAtomicCas16 implementation (IA64-32MODE)

        .file	"acpAtomic-IA64_CAS16_32MODE.c"
	.pred.safe_across_calls p1-p5,p16-p63
	.section	.text,	"ax",	"progbits"
	.align 16
	.global acpAtomicCas16#
	.proc acpAtomicCas16#
acpAtomicCas16:
	.prologue 2, 2
	.vframe r2
	mov r2 = r12
	.body
	;;
	st8 [r2] = r32
	adds r14 = 8, r2
	;;
	st2.rel [r14] = r33
	adds r14 = 10, r2
	;;
	st2.rel [r14] = r34
	ld8 r16 = [r2]
	adds r14 = 8, r2
	;;
	ld2.acq r15 = [r14]
	adds r14 = 10, r2
	;;
	ld2.acq r14 = [r14]
	;;
#APP
	addp4 r17=0,r16
	mov ar.ccv=r14;; cmpxchg2.acq r17=[r17],r15,ar.ccv
#NO_APP
	;;
	mov r14 = r17
	adds r15 = 12, r2
	;;
	st2 [r15] = r14
	adds r14 = 12, r2
	;;
	ld2 r14 = [r14]
	;;
	sxt2 r14 = r14
	;;
	mov r8 = r14
	.restore sp
	mov r12 = r2
	br.ret.sptk.many b0
	;;
	.endp acpAtomicCas16#
	.ident	"GCC: (GNU) 3.4.5"
