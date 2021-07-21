/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: WinCE_Assert.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

/***********************************************************************
 * NAME
 *  WinCE_Assert.h
 *
 * DESCRIPTION
 *  This file defines Assert.
 *   
 * **********************************************************************/

#ifndef _WINCE_ASSERT_H_
#define _WINCE_ASSERT_H_ 1

#include <idl.h>

#if defined(PDL_HAS_WINCE)
# ifdef NDEBUG
#   define assert(exp)   ((void)0)
# else
#   define assert(a) if(!(a)){ \
    fprintf(stdout, "ASSERT(%s)[%s:%d] \n", #a, __FILE__, __LINE__); \
    idlOS::exit(-1);}
# endif /* NDEBUG */
#endif /* PDL_HAS_WINCE */

#endif /* _WINCE_ASSERT_H_ */
