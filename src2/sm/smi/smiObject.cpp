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
 * $Id: smiObject.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smDef.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smr.h>
#include <smc.h>
#include <smn.h>
#include <sma.h>
#include <sml.h>
#include <smiMisc.h>
#include <smiStatement.h>
#include <smiTrans.h>
#include <smiObject.h>

IDE_RC smiObject::createObject( smiStatement    * aStatement,
                                const void      * aInfo,
                                UInt              aInfoSize,
                                const void      * aTempInfo,
                                smiObjectType     aObjectType,
                                const void     ** aTable )
{

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans) != IDE_SUCCESS );
    
    IDE_TEST(smcObject::createObject((smxTrans*)aStatement->mTrans->mTrans,
                                     aInfo,
                                     aInfoSize,
                                     aTempInfo,
                                     aObjectType,
                                     (smcTableHeader**)aTable)
             != IDE_SUCCESS);
    
    *aTable = (const void*)((const UChar*)*aTable-SMP_SLOT_HEADER_SIZE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}

IDE_RC smiObject::dropObject(smiStatement    *aStatement,
                             const void      *aTable)
{

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans ) != IDE_SUCCESS );

    IDE_TEST( smcTable::dropTable(
                            (smxTrans*)aStatement->mTrans->mTrans,
                            (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE),
                            SCT_VAL_DDL_DML,
                            SMC_LOCK_MUTEX)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}

void smiObject::getObjectInfo( const void   * aTable,
                               void        ** aObjectInfo )
{
    smcTableHeader *sTable;
    smVCDesc       *sColumnVCDesc;

    /* object info를 저장하기 위한 aObjectInfo에 대한 메모리 공간은
     * qp 단에서 할당받아야 함 */
    IDE_ASSERT(*aObjectInfo != NULL);
    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sColumnVCDesc = &(sTable->mInfo);

    if ( sColumnVCDesc->length != 0 )
    {
        smcObject::getObjectInfo(sTable, aObjectInfo);
    }
    else
    {
        *aObjectInfo = NULL;
    }
}

void smiObject::getObjectInfoSize( const void* aTable, UInt *aInfoSize )
{
    const smVCDesc       *sColumnVCDesc;
    const smcTableHeader *sTable;

    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sColumnVCDesc = &(sTable->mInfo);

    *aInfoSize = sColumnVCDesc->length;
}

IDE_RC smiObject::setObjectInfo( smiStatement    *aStatement,
                                 const void      *aTable,
                                 void            *aInfo,
                                 UInt             aInfoSize)
{
    if ( aInfo != NULL )
    {
        IDE_TEST( smcTable::setInfoAtTableHeader(
                      (smxTrans*)aStatement->mTrans->mTrans,
                      (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE),
                      aInfo,
                      aInfoSize) 
                != IDE_SUCCESS);
       
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiObject::getObjectTempInfo( const void    * aTable,
                                     void         ** aRuntimeInfo )
{
    *aRuntimeInfo = SMI_MISC_TABLE_HEADER(aTable)->mRuntimeInfo;

    return IDE_SUCCESS;
}

IDE_RC smiObject::setObjectTempInfo( const void  * aTable,
                                     void        * aTempInfo )
{
    ((smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable))->mRuntimeInfo = aTempInfo;

    return IDE_SUCCESS;
}

    

