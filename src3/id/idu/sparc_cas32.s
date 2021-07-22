/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id:
 **********************************************************************/

    .file "sparc_cas32.s"

    .text
    .align 4
    .globl iduCas32
    .type iduCas32, #function
iduCas32:
    ldstub [%o0], %o1
    mov %o1, %o0
    tst %o1

    .text
    .align 4
    .globl iduLockEnter
    .type iduLockEnter, #function
iduLockEnter:
    stbar

    .text
    .align 4
    .globl iduLockExit
    .type iduLockExit, #function
iduLockExit:
    stbar
