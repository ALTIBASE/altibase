/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*------------------------------------------------------------------------*//*!
\file  fioLibX.c
\brief Add the snprintf and vsnprintf functions to fioLib

\verbatim
CVS $Id: fioLibX.c,v 1.1 2003/08/27 18:45:03 apw Exp $

\endverbatim
\author A.P.Waite - apw@slac.stanford.edu

The "standard" I/O routines snprintf and vsnprintf are mysteriously missing
from the VxWorks distribution.  They can however be built out of the tools
VxWorks provides.  These are the implementations.
*//*-------------------------------------------------------------------------*/

#include <stdio.h>
#include "fioLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* === Structures ========================================================== */

/*------------------------------------------------------------------------*//*!
\struct  _fioFormatV_Prm
\brief   Structure used to pass context through fioFormatV
*//*-------------------------------------------------------------------------*/
struct _fioFormatV_Prm {
    char                         *buf;/*!< Output buffer location            */
    int                           siz;/*!< Characters left in output buffer  */
};

/* === Function prototypes ==================================================*/

unsigned int
    fioFormatV_cb                     /* Callback for routine fioFormatV()   */
    (
        char                     *buf,/* Output string (converted token)     */
        int                       siz,/* Length of output string             */
        int                       prm /* User's pass-through parameter       */
    );

/* === Function implementations =============================================*/

/*------------------------------------------------------------------------*//*!
\fn      int snprintf(
             char           *buf,
             int             siz,
             const char     *fmt,
             ...
         )
\brief   Implement the standard snprintf() routine with VxWorks tools 

\param   buf  (in) Pointer to current location in output buffer
\param   siz  (in) Size remaining in output buffer
\param   fmt  (in) Formatting string

VxWorks does not provide a snprintf() entry point, but a substitute can be
constructed using VxWorks provided tools.  This is it.
*//*-------------------------------------------------------------------------*/
int snprintf
(
    char        *buf,
    int          siz,
    const char  *fmt,
    ...
)
{

int
    cnt;

va_list
    list;

struct _fioFormatV_Prm
    out;

/*
 | Pass the boundary condition parameters through the fioFormatV parameter.
*/
out.buf = buf;
out.siz = siz;

/*
 | Turn this into a variable argument string list.
*/
va_start( list, fmt );
cnt = fioFormatV( fmt, list, (FUNCPTR) fioFormatV_cb, (int) &out );
va_end( list );

/*
 | Return total length.
*/
return( cnt );
}

/*------------------------------------------------------------------------*//*!
\fn      int vsnprintf(
             char           *buf,
             int             siz,
             const char     *fmt,
             va_list        list
         )
\brief   Implement the standard vsnprintf() routine with VxWorks tools 

\param   buf  (in) Pointer to current location in output buffer
\param   siz  (in) Size remaining in output buffer
\param   fmt  (in) Formatting string
\param   list (in) Varaible argument list description

VxWorks does not provide a vsnprintf() entry point, but a substitute can be
constructed using VxWorks provided tools.  This is it.
*//*-------------------------------------------------------------------------*/
int vsnprintf
(
    char        *buf,
    int          siz,
    const char  *fmt,
    va_list     list
)
{

int
    cnt;

struct _fioFormatV_Prm
    out;

/*
 | Pass the boundary condition parameters through the fioFormatV parameter.
*/
out.buf = buf;
out.siz = siz;

/*
 | This is already a variable argument string list.
*/
cnt = fioFormatV( fmt, list, (FUNCPTR) fioFormatV_cb, (int) &out );

/*
 | Return total length.
*/
return( cnt );
}

/*------------------------------------------------------------------------*//*!
\fn      int fioFormatV_cb(
             char *buf,
             int   siz,
             int   prm
         )
\brief   Format a message into the output buffer.

\param   buf  (in) Text source buffer provided by fioFormatV()
\param   siz  (in) The number of letters in the text source buffer
\param   prm  (in) Pass through parameter from snprintf

Callback routine for the VxWorks fioFormatV routine.  Used to mimic the
operation of the snprintf routine.
*//*-------------------------------------------------------------------------*/
unsigned int fioFormatV_cb
(
    char       *buf,
    int         siz, 
    int         prm
)
{

struct _fioFormatV_Prm
   *out;

int
   min;

char
   *co;

/*
 | For my own convenience.
*/
out = (struct _fioFormatV_Prm *) prm;

/*
 | Copy the minimum of the size of the input string and the available space
 | in the output string (remember to leave space for a string terminator in
 | the output string).
*/
min = ( siz < (out->siz - 1) ) ? siz : (out->siz - 1);
for( co = out->buf; co < (out->buf + min); *co++ = *buf++ ) {;}

/*
 | Always terminate.
*/
*co = '\0';

/*
 | Accounting is always done with the full length of the requested string.
*/
out->buf += siz;
out->siz -= siz;

return( OK );
}


#ifdef __cplusplus
}
#endif

