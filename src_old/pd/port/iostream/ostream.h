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
*ostream.h - definitions/declarations for the ostream class
*
*       Copyright (c) 1991-1997, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file defines the classes, values, macros, and functions
*       used by the ostream class.
*       [AT&T C++]
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifdef  __cplusplus

#ifndef _INC_OSTREAM
#define _INC_OSTREAM

//#if     !defined(_WIN32) && !defined(_MAC)
//#error ERROR: Only Mac or Win32 targets supported!
//#endif


#ifdef  _MSC_VER
// Currently, all MS C compilers for Win32 platforms default to 8 byte
// alignment.
#pragma pack(push,8)

//#include <useoldio.h>

#endif  // _MSC_VER

#include "ios.h"

#ifdef  _MSC_VER
// C4514: "unreferenced inline function has been removed"
#pragma warning(disable:4514) // disable C4514 warning
// #pragma warning(default:4514)        // use this to reenable, if desired
#endif  // _MSC_VER

typedef long streamoff, streampos;

class ostream : virtual public ios {

public:
		ostream(streambuf*){};
        virtual ~ostream(){};

        ostream& flush(){return *this;};
        int  opfx(){return 0;};
        void osfx(){};

inline  ostream& operator<<(ostream& (__cdecl * _f)(ostream&)){return *this;};
inline  ostream& operator<<(ios& (__cdecl * _f)(ios&)){return *this;};
        ostream& operator<<(const char *){return *this;};
inline  ostream& operator<<(const unsigned char *){return *this;};
inline  ostream& operator<<(const signed char *){return *this;};
inline  ostream& operator<<(char){return *this;};
        ostream& operator<<(unsigned char){return *this;};
inline  ostream& operator<<(signed char){return *this;};
        ostream& operator<<(short){return *this;};
        ostream& operator<<(unsigned short){return *this;};
        ostream& operator<<(int){return *this;};
        ostream& operator<<(unsigned int){return *this;};
        ostream& operator<<(long){return *this;};
        ostream& operator<<(unsigned long){return *this;};
inline  ostream& operator<<(float){return *this;};
        ostream& operator<<(double){return *this;};
        ostream& operator<<(long double){return *this;};
        ostream& operator<<(const void *){return *this;};
        ostream& operator<<(streambuf*){return *this;};
inline  ostream& put(char){return *this;};
        ostream& put(unsigned char){return *this;};
inline  ostream& put(signed char){return *this;};
        ostream& write(const char *,int){return *this;};
inline  ostream& write(const unsigned char *,int){return *this;};
inline  ostream& write(const signed char *,int){return *this;};
        ostream& seekp(streampos){return *this;};
        ostream& seekp(streamoff,ios::seek_dir){return *this;};
        streampos tellp(){return 0;};

protected:
        ostream(){};
        ostream(const ostream&){};        // treat as private
        ostream& operator=(streambuf*){return *this;}; // treat as private
        ostream& operator=(const ostream& _os) {return *this;};
        int do_opfx(int){return 0;};               // not used
        void do_osfx(){};                 // not used

private:
        ostream(ios&){};
        ostream& writepad(const char *, const char *){return *this;};
        int x_floatused;
};


class  ostream_withassign : public ostream {
        public:
			ostream_withassign(){};
			ostream_withassign(streambuf* _is){};
			~ostream_withassign(){};
    ostream& operator=(const ostream& _os) { return *this; }
    ostream& operator=(streambuf* _sb) { return *this; }
};

extern ostream_withassign  cout;
extern ostream_withassign  cerr;
extern ostream_withassign  clog;

inline  ostream& __cdecl flush(ostream& _outs) { return _outs; }
inline  ostream& __cdecl endl(ostream& _outs) { return _outs; }
inline  ostream& __cdecl ends(ostream& _outs) { return _outs; }

 ios&           __cdecl dec(ios&);
 ios&           __cdecl hex(ios&);
 ios&           __cdecl oct(ios&);

#ifdef  _MSC_VER
// Restore default packing
#pragma pack(pop)
#endif  // _MSC_VER

#endif  // _INC_OSTREAM

#endif  /* __cplusplus */
