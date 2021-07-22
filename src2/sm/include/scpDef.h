/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 *
 * $Id: scpDef.h $
 *
 **********************************************************************/

#ifndef _O_SCP_DEF_H_
#define _O_SCP_DEF_H_ 1

#include <smiDef.h>
#include <scpModule.h>

// Export 또는 Import 수행 중 사용되는 Handle.
// scpManager를 통해 관리된다.
typedef struct scpHandle
{
    UChar                mJobName[ SMI_DATAPORT_JOBNAME_SIZE ];
    UInt                 mType;
    smiDataPortHeader  * mHeader;       // 공통 헤더
    scpModule          * mModule;       // 세부 scpModule
    void               * mSelf;         // 삭제를 위해 자신의 Node Ptr를 가짐
} scpHandle;

 
#endif /*_O_SCP_DEF_H_ */
