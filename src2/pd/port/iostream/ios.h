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
*ios.h - definitions/declarations for the ios class.
*
*       Copyright (c) 1990-1997, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file defines the classes, values, macros, and functions
*       used by the ios class.
*       [AT&T C++]
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifdef  __cplusplus

#ifndef _INC_IOS
#define _INC_IOS

#define __cdecl
//#if     !defined(_WIN32) && !defined(_MAC)
//#error ERROR: Only Mac or Win32 targets supported!
//#endif


#ifdef  _MSC_VER
// Currently, all MS C compilers for Win32 platforms default to 8 byte
// alignment.
#pragma pack(push,8)

//#include <useoldio.h>

#endif  // _MSC_VER

/* Define _CRTIMP */

#ifndef NULL
#define NULL    0
#endif

#ifndef EOF
#define EOF     (-1)
#endif

#ifdef  _MSC_VER
// C4514: "unreferenced inline function has been removed"
#pragma warning(disable:4514) // disable C4514 warning
// #pragma warning(default:4514)        // use this to reenable, if desired
#endif  // _MSC_VER

class ostream;
class streambuf;
//class _CRTIMP ostream;

class ios {

public:
    enum io_state {  goodbit = 0x00,
                     eofbit  = 0x01,
                     failbit = 0x02,
                     badbit  = 0x04 };

    enum open_mode { in        = 0x01,
                     out       = 0x02,
                     ate       = 0x04,
                     app       = 0x08,
                     trunc     = 0x10,
                     nocreate  = 0x20,
                     noreplace = 0x40,
                     binary    = 0x80 };

    enum seek_dir { beg=0, cur=1, end=2 };

    enum {  skipws     = 0x0001,
            left       = 0x0002,
            right      = 0x0004,
            internal   = 0x0008,
            dec        = 0x0010,
            oct        = 0x0020,
            hex        = 0x0040,
            showbase   = 0x0080,
            showpoint  = 0x0100,
            uppercase  = 0x0200,
            showpos    = 0x0400,
            scientific = 0x0800,
            fixed      = 0x1000,
            unitbuf    = 0x2000,
            stdio      = 0x4000
                                 };

    static const long basefield;        // dec | oct | hex
    static const long adjustfield;      // left | right | internal
    static const long floatfield;       // scientific | fixed

    ios(streambuf*){};                    // differs from ANSI
    virtual ~ios(){};

    inline long flags()const{return 0;} ;
    inline long flags(long _l){return 0;};

    inline long setf(long _f,long _m){return 0;};
    inline long setf(long _l){return 0;};
    inline long unsetf(long _l){return 0;};

    inline int width() {return 0;};
    inline int width(int _i){return 0;};

    inline ostream* tie(ostream* _os){return _os;};
    inline ostream* tie() {return NULL;}; 

    inline char fill(){return 0;};
    inline char fill(char _c){return 0;};

    inline int precision(int _i){return 0;};
    inline int precision(){return 0;};

    inline int rdstate(){return 0;};
    inline void clear(int _i = 0){};

//  inline operator void*() const;
    operator void *() const { return NULL; }
    inline int operator!(){return 0;};

    inline int  good() {return 0;};
    inline int  eof() {return 0;};
    inline int  fail() {return 0;};
    inline int  bad() {return 0;};

    inline streambuf* rdbuf() {return NULL;};
/*
    inline long & iword(int) {return NULL;};
    inline void * & pword(int) {return 0;};
*/
    static long bitalloc(){return 0;};
    static int xalloc(){return 0;};
    static void sync_with_stdio(){};

    void __cdecl lock() {}
    void __cdecl unlock() {}
    void __cdecl lockbuf() {}
    void __cdecl unlockbuf() {}

protected:
    ios(){};
    ios(const ios&){};                    // treat as private
    ios& operator=(const ios&){return *this;};
    void init(streambuf*){};

    enum { skipping, tied };
    streambuf*  bp;

    int     state;
    int     ispecial;                   // not used
    int     ospecial;                   // not used
    int     isfx_special;               // not used
    int     osfx_special;               // not used
    int     x_delbuf;                   // if set, rdbuf() deleted by ~ios

    ostream* x_tie;
    long    x_flags;
    int     x_precision;
    char    x_fill;
    int     x_width;

    static void lockc() { }
    static void unlockc() { }

public:
    int delbuf() const { return 0; }
    void    delbuf(int _i) {}

private:
    static long x_maxbit;
    static int x_curindex;
    static int sunk_with_stdio;         // make sure sync_with done only once
    static long * x_statebuf;  // used by xalloc()
};

#include "streamb.h"

inline ios& __cdecl dec(ios& _strm) {return _strm; }
inline ios& __cdecl hex(ios& _strm) {return _strm; }
inline ios& __cdecl oct(ios& _strm) {return _strm; }

#ifdef  _MSC_VER
// Restore default packing
#pragma pack(pop)
#endif  // _MSC_VER

#endif  // _INC_IOS

#endif  /* __cplusplus */
