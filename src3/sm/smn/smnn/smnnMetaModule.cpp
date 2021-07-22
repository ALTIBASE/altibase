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
 

/*******************************************************************************
 * $Id: smnnMetaModule.cpp 86324 2019-11-01 09:11:29Z et16 $
 ******************************************************************************/

#include <ide.h>
#include <idu.h>
#include <smi.h>
#include <smDef.h>
#include <smnModules.h>
#include <smnnDef.h>
#include <smnnModule.h>
#include <smnManager.h>

static IDE_RC smnnMetaAA( const smnIndexModule* );

smnIndexModule smnnMetaModule = {
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_META,
                              SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID ),
    SMN_RANGE_DISABLE|SMN_DIMENSION_DISABLE,
    ID_SIZEOF( smnnIterator ),

    /* smnnModule의 초기화 및 파괴 작업과 중복으로 초기화 및 파괴를 수행하지
     * 않도록, smnMemoryFunc 함수에 smnnMetaAA 함수 포인터를 달아놓는다. */
    (smnMemoryFunc) smnnMetaAA,
    (smnMemoryFunc) smnnMetaAA,
    (smnMemoryFunc) NULL,
    (smnMemoryFunc) NULL,
    (smnCreateFunc) NULL,
    (smnDropFunc)   NULL,
    (smTableCursorLockRowFunc)  smnManager::lockRow,
    (smnDeleteFunc)       NULL,
    (smnFreeFunc)         NULL,
    (smnInsertRollbackFunc)    NULL,
    (smnDeleteRollbackFunc)    NULL,
    (smnAgingFunc)             NULL,
    (smInitFunc)          smnnSeq::init,
    (smDestFunc)          smnnSeq::dest,
    (smnFreeNodeListFunc) NULL,
    (smnGetPositionFunc)  NULL,
    (smnSetPositionFunc)  NULL,

    (smnAllocIteratorFunc) smnnSeq::allocIterator,
    (smnFreeIteratorFunc)  smnnSeq::freeIterator,
    (smnReInitFunc)        NULL,
    (smnInitMetaFunc)      NULL,
    
    (smnMakeDiskImageFunc)  NULL,

    (smnBuildIndexFunc)     NULL,
    (smnGetSmoNoFunc)       NULL,
    (smnMakeKeyFromRow)     NULL,
    (smnMakeKeyFromSmiValue)NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat)         smnnSeq::gatherStat
};

IDE_RC smnnMetaAA( const smnIndexModule* )
{
    return IDE_SUCCESS;
}

