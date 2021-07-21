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
*streamb.h - definitions/declarations for the streambuf class
*
*       Copyright (c) 1990-1997, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file defines the classes, values, macros, and functions
*       used by the streambuf class.
*       [AT&T C++]
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifdef  __cplusplus

#ifndef _INC_STREAMB
#define _INC_STREAMB

//#if     !defined(_WIN32) && !defined(_MAC)
//#error ERROR: Only Mac or Win32 targets supported!
//#endif


#ifdef  _MSC_VER
// Currently, all MS C compilers for Win32 platforms default to 8 byte
// alignment.
#pragma pack(push,8)

//#include <useoldio.h>

#endif  // _MSC_VER

#include "ios.h"        // need ios::seek_dir definition

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

typedef long streampos, streamoff;

class ios;

class streambuf {
public:

    virtual ~streambuf(){};

    inline int in_avail() const {return 0;};
    inline int out_waiting() const {return 0;};
    int sgetc()  {return 0;};
    int snextc() {return 0;};
    int sbumpc() {return 0;};
    void stossc() {};

    inline int sputbackc(char) {return 0;};

    inline int sputc(int) {return 0;};
    inline int sputn(const char *,int){return 0;};
    inline int sgetn(char *,int){return 0;};

    virtual int sync(){return 0;};

    virtual streambuf* setbuf(char *, int){return this;};
    virtual streampos seekoff(streamoff,ios::seek_dir,int =ios::in|ios::out){return 0;};
    virtual streampos seekpos(streampos,int =ios::in|ios::out){return 0;};

    virtual int xsputn(const char *,int){return 0;};
    virtual int xsgetn(char *,int){return 0;};

    virtual int overflow(int =EOF) = 0; // pure virtual function
    virtual int underflow() = 0;        // pure virtual function

    virtual int pbackfail(int){return 0;};

    void dbp();

    void lock() { }
    void unlock() { }

protected:
    streambuf(){};
    streambuf(char *,int){};

    inline char * base() const {return NULL;};
    inline char * ebuf() const {return NULL;};
    inline char * pbase() const {return NULL;};
    inline char * pptr() const {return NULL;};
    inline char * epptr() const {return NULL;};
    inline char * eback() const {return NULL;};
    inline char * gptr() const {return NULL;};
    inline char * egptr() const {return NULL;};
    inline int blen() const {return 0;};
    inline void setp(char *,char *) {};
    inline void setg(char *,char *,char *){};
    inline void pbump(int){};
    inline void gbump(int){};

    void setb(char *,char *,int =0){};
    inline int unbuffered() const {return 0;};
    inline void unbuffered(int) {};
    int allocate(){return 0;};
    virtual int doallocate() {return 0;};

private:
    int _fAlloc;
    int _fUnbuf;
    int x_lastc;
    char * _base;
    char * _ebuf;
    char * _pbase;
    char * _pptr;
    char * _epptr;
    char * _eback;
    char * _gptr;
    char * _egptr;
};

#ifdef  _MSC_VER
// Restore previous packing
#pragma pack(pop)
#endif  // _MSC_VER

#endif  // _INC_STREAMB

#endif  /* __cplusplus */
