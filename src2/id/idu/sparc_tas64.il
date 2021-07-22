/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: sparc_tas64.il 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

.inline iduCas32, 8 
    cas [%o0], %g0, %o1
    mov %o1, %o0
    tst %o1
.end

.inline iduLockEnter, 0
    membar #LoadLoad | #LoadStore
.end

.inline iduLockExit, 0
    membar #StoreStore
    membar #LoadStore
.end


