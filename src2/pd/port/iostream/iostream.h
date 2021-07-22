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
 
/***
*iostream.h - definitions/declarations for iostream classes
*
*       Copyright (c) 1990-1997, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file defines the classes, values, macros, and functions
*       used by the iostream classes.
*       [AT&T C++]
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifdef  __cplusplus

#ifndef _INC_IOSTREAM
#define _INC_IOSTREAM

//#if     !defined(_WIN32) && !defined(_MAC)
//#error ERROR: Only Mac or Win32 targets supported!
//#endif


#ifdef  _MSC_VER
// Currently, all MS C compilers for Win32 platforms default to 8 byte
// alignment.
#pragma pack(push,8)

//#include <useoldio.h>

#endif  // _MSC_VER


typedef long streamoff, streampos;

#include "ios.h"                // Define ios.

#include "streamb.h"            // Define streambuf.

#include "istream.h"            // Define istream.

#include "ostream.h"            // Define ostream.

#ifdef  _MSC_VER
// C4514: "unreferenced inline function has been removed"
#pragma warning(disable:4514) // disable C4514 warning
// #pragma warning(default:4514)        // use this to reenable, if desired
#endif  // _MSC_VER

class iostream : public istream, public ostream {
public:
        iostream(streambuf*){};
        virtual ~iostream(){};
protected:
        iostream();
        iostream(const iostream&){};
inline iostream& operator=(streambuf*){return *this;};
inline iostream& operator=(iostream&){return *this;};
private:
        iostream(ios&){};
        iostream(istream&){};
        iostream(ostream&){};
};

class Iostream_init {
public:
        Iostream_init();
        Iostream_init(ios &, int =0);   // treat as private
        ~Iostream_init();
};

// used internally
// static Iostream_init __iostreaminit; // initializes cin/cout/cerr/clog

#ifdef  _MSC_VER
// Restore previous packing
#pragma pack(pop)
#endif  // _MSC_VER

#endif  // _INC_IOSTREAM

#endif  /* __cplusplus */
