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
 
#ifndef _WIN32_IPC_H
#define _WIN32_IPC_H

/*
 * redefinition (Please ignore warning)
 */
#define IPC_PRIVATE     0
#define IPC_CREAT       00001000   /* create if key is nonexistent */
#define IPC_EXCL        00002000   /* fail if key exists */
#define IPC_NOWAIT      00004000   /* return error on wait */
#define IPC_RMID        0     /* remove resource */
#define IPC_SET         1     /* set ipc_perm options */
#define IPC_STAT        2     /* get ipc_perm options */
#define IPC_INFO        3     /* see ipcs */
#define IPC_OLD         0   /* Old version (no 32-bit UID support on many */

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

#endif /* _WIN32_IPC_H */
