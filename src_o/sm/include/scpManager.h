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
 * $Id: scpManager.h 
 * 
 * Proj-2059 DB Upgrade 기능
 * Server 중심적으로 데이터를 가져오고 넣는 기능

 **********************************************************************/

#ifndef __O_SCP_MANAGER_H__
#define __O_SCP_MANAGER_H__ 1_

#include <smiDef.h>
#include <scpDef.h>
#include <smuList.h>

class scpManager
{
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static IDE_RC findHandle( SChar          * aName,
                              void          ** aHandle );

    static inline smuList *getListHead(){return &mHandleList;}

    static inline IDE_RC lock() {return mMutex.lock( NULL /* idvSQL* */ ); }
    static inline IDE_RC unlock() {return mMutex.unlock(); }

    static IDE_RC      addNode( void          * aNode );
    static IDE_RC      delNode( void          * aNode );

private:
    static iduMutex mMutex;
    static smuList  mHandleList;
    static IDE_RC   destroyList();
};

#endif /* __O_SCP_MANAGER_H__ */
