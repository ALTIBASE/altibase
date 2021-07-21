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
 
// Basic_Types.cpp,v 4.6 1999/01/03 15:18:20 levine Exp

#define PDL_BUILD_DLL

#include "OS.h"
#if !defined (__PDL_INLINE__)
# include "Basic_Types.i"
#endif /* ! __PDL_INLINE__ */

PDL_RCSID(pdl, Basic_Types, "Basic_Types.cpp,v 4.6 1999/01/03 15:18:20 levine Exp")

#if defined (PDL_LACKS_LONGLONG_T)

void
PDL_U_LongLong::output (FILE *file) const
{
  if (h_ () > 0)
    PDL_OS::fprintf (file, "0x%lx%0*lx", h_ (), 2 * sizeof l_ (), l_ ());
  else
    PDL_OS::fprintf (file, "0x%lx", l_ ());
}

#endif /* PDL_LACKS_LONGLONG_T */

