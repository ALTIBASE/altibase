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
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#ifndef _O_IDU_SHM_PROC_TYPE_H_ 
#define _O_IDU_SHM_PROC_TYPE_H_ 1

#include <idl.h>

typedef enum iduShmProcType
{
    IDU_PROC_TYPE_DAEMON,
    IDU_PROC_TYPE_WSERVER,
    IDU_PROC_TYPE_RSERVER,
    IDU_PROC_TYPE_USER,
    IDU_SHM_PROC_TYPE_NULL
} iduShmProcType;

static inline iduShmProcType iduGetShmProcType()
{
    extern iduShmProcType gShmProcType;

    return gShmProcType;
}

static inline void iduSetShmProcType( iduShmProcType aShmProcType )
{
    extern iduShmProcType gShmProcType;

    gShmProcType = aShmProcType;
}

static inline SChar const *iduGetShmProcTypeDesc( iduShmProcType aShmProcType )
{
    SChar const *sRetVal = NULL;

    switch ( aShmProcType )
    {
    case IDU_PROC_TYPE_DAEMON:
        sRetVal = "daemon";
        break;
    case IDU_PROC_TYPE_WSERVER:
        sRetVal = "wserver";
        break;
    case IDU_PROC_TYPE_RSERVER:
        sRetVal = "rserver";
        break;
    case IDU_PROC_TYPE_USER:
        sRetVal = "user";
        break;
    default:
        sRetVal = "";
        break;
    }

    return sRetVal;
}

#endif /* _O_IDU_SHM_PROC_TYPE_H_ */
