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
 
/* -*- C++ -*- */

//=============================================================================
/**
 *  @file    iosfwd.h
 *
 *  iosfwd.h,v 1.11 2000/11/01 22:17:39 coryan Exp
 *
 *  @author Irfan Pyarali
 *
 *  This file contains the portability ugliness for the Standard C++
 *  Library.  As implementations of the "standard" emerge, this file
 *  will need to be updated.
 *
 *  This files deals with forward declaration for the stream
 *  classes.  Remember that since the new Standard C++ Library code
 *  for streams uses templates, simple forward declaration will not
 *  work.
 *
 *
 */
//=============================================================================


#ifndef PDL_IOSFWD_H
#define PDL_IOSFWD_H

#include "config.h"

#if !defined (PDL_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* PDL_LACKS_PRAGMA_ONCE */

#if defined (PDL_HAS_STANDARD_CPP_LIBRARY)  && \
    (PDL_HAS_STANDARD_CPP_LIBRARY != 0)

# if !defined (PDL_USES_OLD_IOSTREAMS)  || \
    defined (PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION)
#   include /**/ <iosfwd>
# endif /* ! PDL_USES_OLD_IOSTREAMS  ||  PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION */

# if defined (PDL_USES_STD_NAMESPPDL_FOR_STDCPP_LIB) && \
             (PDL_USES_STD_NAMESPPDL_FOR_STDCPP_LIB != 0)

#   if !defined (PDL_USES_OLD_IOSTREAMS)
      // Make these available in the global name space
      using std::ios;
      using std::streambuf;
      using std::istream;
      using std::ostream;
      using std::iostream;
      using std::filebuf;
      using std::ifstream;
      using std::ofstream;
      using std::fstream;
#   endif /* ! PDL_USES_OLD_IOSTREAMS */

# endif /* PDL_USES_STD_NAMESPPDL_FOR_STDCPP_LIB */

#else /* ! PDL_HAS_STANDARD_CPP_LIBRARY */

  class ios;
  class streambuf;
  class istream;
  class ostream;
  class iostream;
  class filebuf;
  class ifstream;
  class ofstream;
  class fstream;

# endif /* ! PDL_HAS_STANDARD_CPP_LIBRARY */

#endif /* PDL_IOSFWD_H */
