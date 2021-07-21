/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id:
 **********************************************************************/

    .file "sparc_cas64.s"

    .text
    .align 8
    .globl iduCas32
    .type iduCas32, #function
iduCas32:
    mov 0, %g1
    cas [%o0], %g1, %o1
    retl
    mov %o1, %o0
    
    .text
    .align 8
    .globl iduLockEnter
    .type iduLockEnter, #function
iduLockEnter:
    membar #LoadLoad
    membar #LoadStore
    retl
    nop
    
    .text
    .align 8
    .globl iduLockExit
    .type iduLockExit, #function
iduLockExit:
    membar #LoadLoad
    membar #LoadStore
    retl
    nop
