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

#ifndef _APRE_PREDEFINED_TYPES_H_
#define _APRE_PREDEFINED_TYPES_H_ 1

typedef unsigned int size_t;
typedef int ssize_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef void pthread_mutex_t;
typedef unsigned int pthread_t;
typedef void pthread_attr_t;
typedef long int time_t;
struct timeval
{
    long tv_sec;
    long tv_usec;
};
struct timezone
{
    int tz_minuteswest;
    int tz_dsttime;
};

typedef int FILE;
typedef int DIR;
struct dirent
{
    long d_ino;long d_off;unsigned short d_reclen;char d_name[NAME_MAX+1];
};

struct stat
{
    unsigned long int    st_dev;
    unsigned long int    st_ino;
    unsigned int         st_mode;
    unsigned long int    st_nlink;
    unsigned int         st_uid;
    unsigned int         st_gid;
    unsigned long int    st_rdev;
    long int             st_size;
    long int             st_blksize;
    long int             st_blocks;
    long int             st_atime;
    long int             st_mtime;
    long int             st_ctime;
};

#endif /* _APRE_PREDEFINED_TYPES_H_ */
