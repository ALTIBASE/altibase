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
 
#ifndef _QNX_SEM_H
#define _QNX_SEM_H
/*-------------------------------------------------------------------------
 *
 * qnxSem.h
 *	  System V Semaphore Emulation Class Definition
 *
 * Copyright (c) 2002, hjohn in Altibase,co
 *
 *
 * IDENTIFICATION
 *	  $Header$
 *
 *-------------------------------------------------------------------------
 */
#include <semaphore.h>
#include <sys/ipc.h>
/*
 *	Semctl Command Definitions.
 */

#define GETNCNT 3	/* get semncnt */
#define GETPID	4	/* get sempid */
#define GETVAL	5	/* get semval */
#define GETALL	6	/* get all semval's */
#define GETZCNT 7	/* get semzcnt */
#define SETVAL	8	/* set semval */
#define SETALL	9	/* set all semval's */
/*
 * Semaphore operation flags
 */
#define SEM_UNDO        010000

#define SHM_INFO_NAME	"/dev/shmem/RtSysV_Sem_Info"
#define PROC_NSEMS_PER_SET     16
#define PROC_SEM_MAP_ENTRIES(maxBackends)  ((maxBackends)/PROC_NSEMS_PER_SET+1)
#define SEMMAX	1024 /*(PROC_NSEMS_PER_SET+1)*/
#define OPMAX	8
#define EMODE	0700

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

struct sem_set_info
{
	key_t		key;
	int			nsems;
	sem_t		sem[SEMMAX];	/* array of POSIX semaphores */
	struct sem	semV[SEMMAX];	/* array of System V semaphore structures */
	struct pending_ops pendingOps[SEMMAX];	/* array of pending operations */
};

struct sem_info
{
	sem_t		sem;
	int			nsets;
	/* there are actually nsets of these: */
	struct sem_set_info set[1]; /* VARIABLE LENGTH ARRAY */
};

// User semaphore template for semop system calls.
struct sembuf
{
	ushort_t	sem_num;		/* semaphore #			*/
	short		sem_op;			/* semaphore operation		*/
	short		sem_flg;		/* operation flags		*/
};

struct semid_ds {
    struct ipc_perm     sem_perm;
    struct sem          *sem_base;
    unsigned short int  sem_nsems;
    unsigned short int  sempadding;
    time_t              sem_otime;
    long                sem_pad1;
    time_t              sem_ctime;
    long                sem_pad2;
    long                sem_pad3[4];
};

/*
union semun
{
    int         val;
    struct semid_ds *buf;
    unsigned short *array;
};
*/

class qnxSem {

    private : 
        static void on_proc_exit(void (*function) (), ushort_t arg);
        static void semclean(void);
    
    public :
//        static int MaxBackends;
        static int semctl(int semid, int semnum, int cmd, union semun arg);
        static int semget(key_t key, int nsems, int semflg);
        static int semop(int semid, struct sembuf * sops, size_t nsops);

}; /* class qnxSem */
#endif /* _QNX_SEM_H */
