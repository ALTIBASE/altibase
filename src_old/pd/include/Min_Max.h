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
 
// -*- C++ -*-
// Min_Max.h,v 4.1 2000/01/17 05:09:30 schmidt Exp

#ifndef PDL_MIN_MAX_H
#define PDL_MIN_MAX_H

// ============================================================================
//
// = LIBRARY
//   pdl
//
// = FILENAME
//   Min_Max.h
//
// = DESCRIPTION
//   Define an appropriate set of min()/max() functions using templates. 
//
// = AUTHOR
//   Derek Dominish <Derek.Dominish@Australia.Boeing.com>
//
// ============================================================================

# if !defined (PDL_LACKS_PRAGMA_ONCE)
#   pragma once
# endif /* PDL_LACKS_PRAGMA_ONCE */

# if !defined (PDL_LACKS_MIN_MAX_TEMPLATES)
template <class T>
inline const T &
pdl_min (const T &t1, const T &t2)
{
  return t2 > t1 ? t1 : t2;
}

template <class T>
inline const T & 
pdl_max (const T &t1, const T &t2) 
{
  return t1 > t2 ? t1 : t2;
}

template <class T>
inline const T &
pdl_min (const T &t1, const T &t2, const T &t3) 
{
  return pdl_min (pdl_min (t1, t2), t3);
}

template <class T>
inline const T &
pdl_max (const T &t1, const T &t2, const T &t3) 
{
  return pdl_max (pdl_max (t1, t2), t3);
}

template <class T>
inline const T &
pdl_range (const T &min, const T &max, const T &val)
{
  return pdl_min (pdl_max (min, val), max);
}
# else
// These macros should only be used if a C++ compiler can't grok the
// inline templates 
#  define pdl_min(a,b)      (((b) > (a)) ? (a) : (b))
#  define pdl_max(a,b)      (((a) > (b)) ? (a) : (b))
#  define pdl_range(a,b,c)  (pdl_min(pdl_max((a), (c)), (b))

# endif /* PDL_LACKS_MIN_MAX_TEMPLATES */

# define PDL_MIN(a,b)     pdl_min((a),(b))
# define PDL_MAX(a,b)     pdl_max((a),(b))
# define PDL_RANGE(a,b,c) pdl_range((a),(b),(c))

#endif  /* PDL_MIN_MAX_H */
