/* ****************************************************************** *
 * Copyright 1999-2006, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 * ****************************************************************** */

/* ****************************************************************** *
 * Itanium atomic operation 
 * use preprocessor CC/C++ for compile [-milp32,-mlp64] 
 * cc -milp32 
 * $Id: ia64.S,v 1.8 2006/05/24 03:10:46 alex Exp $
 * ****************************************************************** */

    .file   "ia64.S"
/* atomic op semaphore [.rel/..acq] define */
#define SEM acq 

#define ENTER(name)\
	.align  16;     \
	.global name; 	\
    .proc   name; 	\
	name:			

#define END(name) 	.endp name


		.text	
ENTER(idkAtomicADD1)
        .prologue
        .body
        mf
        ld1.SEM r15 = [r32]
        nop 0
        ;;
.L1ADD1:
        mov ar.ccv = r15
        add r8 = r15, r33
        mov r2 = r15
        ;;
        nop 0
        cmpxchg1.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1ADD1 
END(idkAtomicADD1)

ENTER(idkAtomicADD2)
        .prologue
        .body
        mf
        ld2.SEM r15 = [r32]
        nop 0
        ;;
.L1ADD2:
        mov ar.ccv = r15
        add r8 = r15, r33
        mov r2 = r15
        ;;
        nop 0
        cmpxchg2.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br .L1ADD2
END(idkAtomicADD2)

ENTER(idkAtomicADD4)
        .prologue
        .body
        mf
        ld4.SEM r15 = [r32]
        nop 0
        ;;
.L1ADD4:
        mov ar.ccv = r15
        add r8 = r15, r33
        mov r2 = r15
        ;;
        nop 0
        cmpxchg4.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp4.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1ADD4 
END(idkAtomicADD4)

ENTER(idkAtomicADD8)
        .prologue
        .body
        mf
        ld8.SEM r15 = [r32]
        nop 0
        ;;
.L1ADD8:
        mov ar.ccv = r15
        add r8 = r15, r33
        mov r2 = r15
        ;;
        nop 0
        cmpxchg8.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br .L1ADD8
END(idkAtomicADD8)

ENTER(idkAtomicCAS1)
        .prologue
        .body
        mf
        ld1.SEM r15 = [r32]
        nop 0
        ;;
        mov ar.ccv = r33
        ;;
        nop 0
        cmpxchg1.SEM r15 = [r32], r34, ar.ccv
        nop 0
        ;;
        mov ret0 = r15
        br.ret.sptk.many rp
END(idkAtomicCAS1)


ENTER(idkAtomicCAS2)
        .prologue
        .body
        mf
        ld2.SEM r15 = [r32]
        nop 0
        ;;
        mov ar.ccv = r33
        ;;
        nop 0
        cmpxchg2.SEM r15 = [r32], r34, ar.ccv
        nop 0
        ;;
        mov ret0 = r15
        br.ret.sptk.many rp
END(idkAtomicCAS2)


ENTER(idkAtomicCAS4)
        .prologue
        .body
        mf
        ld4.SEM r15 = [r32]
        nop 0
        ;;
        mov ar.ccv = r33
        ;;
        nop 0
        cmpxchg4.SEM r15 = [r32], r34, ar.ccv
        nop 0
        ;;
        mov ret0 = r15
        br.ret.sptk.many rp
END(idkAtomicCAS4)


ENTER(idkAtomicCAS8)
        .prologue
        .body
        mf
        ld8.SEM r15 = [r32]
        nop 0   
        ;;
		mov ar.ccv = r33                                              
        ;;
        nop 0
        cmpxchg8.SEM r15 = [r32], r34, ar.ccv
        nop 0
        ;;
		mov ret0 = r15
    	br.ret.sptk.many rp
END(idkAtomicCAS8)

ENTER(idkAtomicINC1)
        .prologue
        .body
        mf
        ld1.SEM r15 = [r32]
        nop 0
        ;;
.L1INC1:
        mov ar.ccv = r15
		adds r8 = 1, r15
        mov  r2 = r15
        ;;
        nop 0
        cmpxchg1.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1INC1
END(idkAtomicINC1)

ENTER(idkAtomicINC2)
        .prologue
        .body
        mf
        ld2.SEM r15 = [r32]
        nop 0
        ;;
.L1INC2:
        mov ar.ccv = r15
		adds r8 = 1, r15
        mov  r2 = r15
        ;;
        nop 0
        cmpxchg2.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1INC2
END(idkAtomicINC2)


ENTER(idkAtomicINC4)
        .prologue
        .body
        mf
        ld4.SEM r15 = [r32]
        nop 0
        ;;
.L1INC4:
        mov ar.ccv = r15
		adds r8 = 1, r15
        mov  r2 = r15
        ;;
        nop 0
        cmpxchg4.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1INC4
END(idkAtomicINC4)

ENTER(idkAtomicINC8)
        .prologue
        .body
        mf
        ld8.SEM r15 = [r32]
        nop 0
        ;;
.L1INC8:
        mov ar.ccv = r15
		adds r8 = 1, r15
        mov  r2 = r15
        ;;
        nop 0
        cmpxchg8.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1INC8
END(idkAtomicINC8)

ENTER(idkAtomicDEC1)
        .prologue
        .body
        mf
        ld1.SEM r15 = [r32]
        nop 0
        ;;
.L1DEC1:
        mov ar.ccv = r15
        adds r8 = -1, r15
        mov  r2 = r15
        ;;
        nop 0
        cmpxchg1.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1DEC1 
END(idkAtomicDEC1)

ENTER(idkAtomicDEC2)
        .prologue
        .body
        mf
        ld2.SEM r15 = [r32]
        nop 0
        ;;
.L1DEC2:
        mov ar.ccv = r15
        adds r8 = -1, r15
        mov  r2 = r15
        ;;
        nop 0
        cmpxchg2.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1DEC2 
END(idkAtomicDEC2)

ENTER(idkAtomicDEC4)
        .prologue
        .body
        mf
        ld4.SEM r15 = [r32]
        nop 0
        ;;
.L1DEC4:
        mov ar.ccv = r15
        adds r8 = -1, r15
        mov  r2 = r15
        ;;
        nop 0
        cmpxchg4.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1DEC4 
END(idkAtomicDEC4)

ENTER(idkAtomicDEC8)
        .prologue
        .body
        mf
        ld8.SEM r15 = [r32]
        nop 0
        ;;
.L1DEC8:
        mov ar.ccv = r15
        adds r8 = -1, r15
        mov  r2 = r15
        ;;
        nop 0
        cmpxchg8.SEM r15 = [r32], r8, ar.ccv
        nop 0
        ;;
        cmp.ne p8, p9 = r2, r15
        (p9) br.ret.dpnt.many rp
        br	.L1DEC8 
END(idkAtomicDEC8)

