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
 * $Id: smcSequenceUpdate.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smcDef.h>
#include <smcSequenceUpdate.h>



/* Update type: SMR_SMC_TABLEHEADER_SET_SEQUENCE */
IDE_RC smcSequenceUpdate::redo_undo_SMC_TABLEHEADER_SET_SEQUENCE(
                                                         smTID       /*aTid*/,
                                                         scSpaceID     aSpace,
                                                         scPageID      aPID,
                                                         scOffset      aOffset,
                                                         vULong      /*aData*/, 
                                                         SChar        *aImage,
                                                         SInt          aSize,
                                                         UInt       /*aFlag*/)
{
    
    smcTableHeader *sTableHeader;
    
    IDE_ASSERT( smmManager::getOIDPtr( aSpace,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void **)&sTableHeader )
                == IDE_SUCCESS );

    idlOS::memcpy(&(sTableHeader->mSequence), aImage, aSize);

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpace, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}
