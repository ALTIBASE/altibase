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
 
#ifndef __WIN32_SHM_H
#define __WIN32_SHM_H

/*-------------------------------------------------------------------------
 *
 * idlwShmImpl.h (no CPP, but this has inline function)
 *	  System V SharedMemory Emulation Implementation for win32
 *
 * Copyright (c) 2002, hjohn in Altibase,co
 *
 *
 * IDENTIFICATION
 *	  $Header$
 *
 *-------------------------------------------------------------------------
 */
#include <windows.h>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "ipc4win.h"

#define SHM_R       0400                    /* read permission */
#define SHM_W       0200                    /* write permission */
#define PATHNAME    "C:/DEV/SHMEM/W32SHM"
#define EXT         ".SHM"
/* "project" arguments for ftok when accessing shared memory segments */
//#define SHM_PROJ    's' --> don't use const variable for variable shared memory key value

typedef HANDLE      SHM;

class win32Shm
{
private :
    static char *keytoname(int key, char *name);
    static int CreateShm  (SHM *shm, char *name, int size);
    static int CreateShmEx(SHM *shm, char *name, int proj, int size);
    static int OpenShm  (SHM *shm, char *name, int size);
    static int OpenShmEx(SHM *shm, char *name, int proj, int size);
    static int DeleteShm(SHM shm);
    static int AttachShm(SHM shm, void **ptr);
    static int DetachShm(void *ptr);

public :
	static SHM   shmget(int key, size_t size, int flags);
	static void *shmat (SHM shmid, void *shmaddr, int shmflg);
	static int   shmdt (void *shmaddr);
	static int	 shmctl(SHM shmid, int cmd, struct shmid_ds *buf);
};


#endif /* __WIN32_SHM_H */
