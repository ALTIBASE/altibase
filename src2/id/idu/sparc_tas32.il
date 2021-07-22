/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: sparc_tas32.il 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

.inline iduCas32, 8 
    ldstub [%o0], %o1
    mov %o1, %o0
    tst %o1
.end

.inline iduLockEnter, 0
    stbar
.end

.inline iduLockExit, 0
    stbar
.end


