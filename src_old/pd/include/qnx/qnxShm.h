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
 
#ifndef _QNX_SHM_H
#define _QNX_SHM_H
/*-------------------------------------------------------------------------
 *
 * qnxShm.c
 *	  System V Shared Memory Emulation Definition
 *
 * Copyright (c) 2002, hjohn in Altibase,co
 *
 *
 * IDENTIFICATION
 *	  $Header$
 *
 *-------------------------------------------------------------------------
 */
#include <sys/ipc.h>

#define HMODE	0777
#define SHMMAX	1024
#define SHM_R	0400			/* read permission */
#define SHM_W	0200			/* write permission */

struct shmid_ds
{
	int			dummy;
	int			shm_nattch;
};

struct shm_info
{
	int			shmid;
	key_t		key;
	size_t		size;
	void	   *addr;
};

class qnxShm {
    
private :
    static char *keytoname(key_t key, char *name);
    static int shm_putinfo(struct shm_info * info);
    static int shm_updinfo(int i, struct shm_info * info);
    static int shm_getinfo(int shmid, struct shm_info * info);
    static int shm_getinfobyaddr(const void *addr, struct shm_info * info);
public :
    static void *shmat(int shmid, const void *shmaddr, int shmflg);
    static int shmdt(const void *addr);
    static int shmctl(int shmid, int cmd, struct shmid_ds * buf);
    static int shmget(key_t key, size_t size, int flags);

};
#endif /* _QNX_SHM_H */