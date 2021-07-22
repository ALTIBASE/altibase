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
*istream.h - definitions/declarations for the istream class
*
*       Copyright (c) 1990-1997, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file defines the classes, values, macros, and functions
*       used by the istream class.
*       [AT&T C++]
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifdef  __cplusplus

#ifndef _INC_ISTREAM
#define _INC_ISTREAM

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
// C4069: "long double != double"
#pragma warning(disable:4069)   // disable C4069 warning
// #pragma warning(default:4069)    // use this to reenable, if desired

// C4514: "unreferenced inline function has been removed"
#pragma warning(disable:4514) // disable C4514 warning
// #pragma warning(default:4514)    // use this to reenable, if desired
#endif  // _MSC_VER


typedef long streamoff, streampos;

class  istream : virtual public ios {

public:
    istream(streambuf*){};
    virtual ~istream(){};

    int  ipfx(int =0){};
    void isfx() {};

    inline istream& operator>>(istream& (__cdecl * _f)(istream&)){return *this;};
    inline istream& operator>>(ios& (__cdecl * _f)(ios&)){return *this;};
    istream& operator>>(char *){return *this;};
    inline istream& operator>>(unsigned char *){return *this;};
    inline istream& operator>>(signed char *){return *this;};
    istream& operator>>(char &){return *this;};
    inline istream& operator>>(unsigned char &){return *this;};
    inline istream& operator>>(signed char &){return *this;};
    istream& operator>>(short &){return *this;};
    istream& operator>>(unsigned short &){return *this;};
    istream& operator>>(int &){return *this;};
    istream& operator>>(unsigned int &){return *this;};
    istream& operator>>(long &){return *this;};
    istream& operator>>(unsigned long &){return *this;};
    istream& operator>>(float &){return *this;};
    istream& operator>>(double &){return *this;};
    istream& operator>>(long double &){return *this;};
    istream& operator>>(streambuf*){return *this;};

    int get();

    inline istream& get(         char *,int,char ='\n'){return *this;};
    inline istream& get(unsigned char *,int,char ='\n'){return *this;};
    inline istream& get(  signed char *,int,char ='\n'){return *this;};

    istream& get(char &){return *this;};
    inline istream& get(unsigned char &){return *this;};
    inline istream& get(  signed char &){return *this;};

    istream& get(streambuf&,char ='\n'){return *this;};
    inline istream& getline(         char *,int,char ='\n'){return *this;};
    inline istream& getline(unsigned char *,int,char ='\n'){return *this;};
    inline istream& getline(  signed char *,int,char ='\n'){return *this;};

    inline istream& ignore(int =1,int =EOF){return *this;};
    istream& read(char *,int){return *this;};
    inline istream& read(unsigned char *,int){return *this;};
    inline istream& read(signed char *,int){return *this;};

    int gcount() const { return 0; }
    int peek(){return 0;};
    istream& putback(char){return *this;};
    int sync(){return 0;};

    istream& seekg(streampos){return *this;};
    istream& seekg(streamoff,ios::seek_dir){return *this;};
    streampos tellg(){return 0;};

    void eatwhite(){};

protected:
    istream(){};
    istream(const istream&){};    // treat as private
    istream& operator=(streambuf* _isb){return *this;}; // treat as private
    istream& operator=(const istream& _is) { return *this; }
    istream& get(char *, int, int){};
     int do_ipfx(int){};

private:
    istream(ios&){};
    int getint(char *){return 0;};
    int getdouble(char *, int){return 0;};
    int _fGline;
    int x_gcount;
};

class  istream_withassign : public istream {
public:
    istream_withassign(){};
    istream_withassign(streambuf*){};
   ~istream_withassign(){};
    istream& operator=(const istream& _is) { return *this; }
    istream& operator=(streambuf* _isb) { return *this; }
};

extern  istream_withassign cin;

inline  istream& __cdecl ws(istream& _ins) { return _ins; }

// ios&        __cdecl dec(ios&);
// ios&        __cdecl hex(ios&);
// ios&        __cdecl oct(ios&);

#ifdef  _MSC_VER
// Restore default packing
#pragma pack(pop)
#endif  // _MSC_VER

#endif  // _INC_ISTREAM

#endif  /* __cplusplus */
