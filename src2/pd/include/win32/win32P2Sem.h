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
 
#ifndef __WIN32_PTH_SEM_H
#define __WIN32_PTH_SEM_H

/*-------------------------------------------------------------------------
 *
 * idlwSemImpl.h (no CPP, but this has inline function)
 *	  System V Semaphore Emulation Implementation for win32
 *
 * Copyright (c) 2002, hjohn in Altibase,co
 *
 *
 * IDENTIFICATION
 *	  $Header$
 *
 *-------------------------------------------------------------------------
 */

#include <stdlib.h>		// for atexit
#include <stddef.h>		// for size_t
#include <stdio.h>		// for remove
#include <string.h>
#include <unistd.h>
#if !defined (PDL_HAS_WINCE)
#include <io.h>			// for open, close
#include <sys/stat.h>	// for _fstat
#include <sys/types.h>	// for _fstat
#include <errno.h>
#include <fcntl.h>
#include "ipc4win.h"
#endif /* !PDL_HAS_WINCE */

typedef unsigned short ushort_t;
typedef int key_t;
/*
 * Semaphore operation flags
 */
#define DEF_MAXBACKENDS 16
#define MAXBACKENDS    (DEF_MAXBACKENDS > 1024 ? DEF_MAXBACKENDS : 1024)
#define PROC_NSEMS_PER_SET     16
#define SETMAX  ((MAXBACKENDS + PROC_NSEMS_PER_SET - 1) / PROC_NSEMS_PER_SET)
#define SEMMAX  (PROC_NSEMS_PER_SET+1)

#define OPMAX   8
#define EMODE	0700
#define SHM_R       0400                    /* read permission */
#define SHM_W       0200                    /* write permission */

#define SHM_INFO_NAME "C:/SYSV_SEM_INFO.SHM"

struct pending_ops
{
	int			op[OPMAX];		/* array of pending operations */
	int			idx;			/* index of first free array member */
};

// There is one semaphore structure for each semaphore in the system.
struct sem
{
	ushort_t	semval;			/* semaphore text map address	*/
	pid_t		sempid;			/* pid of last operation	*/
	ushort_t	semncnt;		/* # awaiting semval > cval */
	ushort_t	semzcnt;		/* # awaiting semval = 0	*/
};

struct sem_info
{
	PDL_sema_t		sem;

	struct 
    {
	    key_t		key;
	    int			nsems;
	    PDL_sema_t	sem[SEMMAX];	/* array of POSIX semaphores */
	    struct sem	semV[SEMMAX];	/* array of System V semaphore structures */
	    struct pending_ops pendingOps[SEMMAX];	/* array of pending operations */
    } set[SETMAX];
};

struct ipc_perm
{
    key_t		key;       /* Key.  */
    uid_t		uid;       /* Owner's user ID.  */
    gid_t		gid;       /* Owner's group ID.  */
    uid_t		cuid;      /* Creator's user ID.  */
    gid_t		cgid;      /* Creator's group ID.  */
    mode_t		mode;      /* Read/write permission.  */
    ushort_t	seq;       /* Sequence number.  */
};

struct semid_ds {
    struct ipc_perm     sem_perm;
    struct sem          *sem_base;
    ushort_t            sem_nsems;
    ushort_t            sempadding;
    time_t              sem_otime;
    long                sem_pad1;
    time_t              sem_ctime;
    long                sem_pad2;
    long                sem_pad3[4];
};


class win32P2Sem {

    private : 
        static void on_proc_exit(void (*function) (), ushort_t arg);
        static void semclean(void);
        static void reverse(char s[]);
        static void name_mangle(int n,char s[]);
    
    public :
        static HANDLE shm_open(const char *name, int flags, int mode);
        static HANDLE shm_open_test(const char *name, int flags, int mode);
        static int semctl(int semid, int semnum, int cmd, union semun arg);
        static int semget(key_t key, int nsems, int semflg);
        static int semop(int semid, struct sembuf * sops, size_t nsops);

}; /* class win32P2Sem */


#endif /* __WIN32_PTH_SEM_H */
