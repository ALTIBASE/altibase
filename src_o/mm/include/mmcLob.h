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

#ifndef _O_MMC_LOB_H_
#define _O_MMC_LOB_H_

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smi.h>


typedef struct mmcLobLocator
{
    smLobLocator mLocatorID;
    UInt         mStatementID;
    iduListNode  mListNode;
} mmcLobLocator;


class mmcLob
{
private:
    static iduMemPool mPool;

public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    static IDE_RC alloc(mmcLobLocator **aLobLocator,
                        UInt            aStatementID,
                        smLobLocator    aLocatorID);
    static IDE_RC free(idvSQL        *aStatistics,
                       mmcLobLocator *aLobLocator);

    static IDE_RC addLocator(mmcLobLocator *aLobLocator,
                             UInt           aStatementID,
                             smLobLocator   aLocatorID);

    /* PROJ-2047 Strengthening LOB - Removed aOldSize */
    static IDE_RC beginWrite(idvSQL        *aStatistics, 
                             mmcLobLocator *aLobLocator, 
                             UInt           aOffset, 
                             UInt           aSize);

    /* PROJ-2047 Strengthening LOB - Removed aOffset */
    static IDE_RC write(idvSQL           *aStatistics, 
                        mmcLobLocator    *aLobLocator, 
                        UInt              aSize, 
                        UChar            *aData);
 
    static IDE_RC endWrite(idvSQL        *aStatistics, 
                           mmcLobLocator *aLobLocator);

public:
    static UInt hashFunc(void *aKey);
    static SInt compFunc(void *aLhs, void *aRhs);
};


#endif
