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
 
/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _O_IDU_SHM_KEY_MGR_H_
#define _O_IDU_SHM_KEY_MGR_H_ (1)

#include <idl.h>
#include <idu.h>
#include <iduShmDef.h>

#define IDU_MIN_SHM_KEY_CANDIDATE (1024)
#define IDU_MAX_SHM_KEY_CANDIDATE (16777215)

class iduShmKeyMgr
{
private :
    static iduShmSSegment  * mSSegment;

public :
    iduShmKeyMgr();
    ~iduShmKeyMgr();

    static IDE_RC initializeStatic( iduShmSSegment *aSSegment );

    static IDE_RC destroyStatic();

    static IDE_RC getShmKeyCandidate( key_t * aShmKeyCandidate );
};


#endif // _O_IDU_SHM_KEY_MGR_H_
