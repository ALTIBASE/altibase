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
 * $Id: stndrStackMgr.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#include <stndrDef.h>
#include <stndrStackMgr.h>



IDE_RC stndrStackMgr::initialize( stndrStack * aStack,
                                  SInt         aStackSize /* = STNDR_STACK_DEFAULT_SIZE */ )
{
    aStack->mDepth      = STNDR_STACK_DEPTH_EMPTY;
    aStack->mStackSize  = aStackSize;

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ST_STN,
                                 ID_SIZEOF(stndrStackSlot) * aStack->mStackSize,
                                 (void**)&aStack->mStack )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC stndrStackMgr::destroy( stndrStack * aStack )
{
    if( aStack->mStack != NULL )
    {
        IDE_TEST( iduMemMgr::free(aStack->mStack) != IDE_SUCCESS );
        aStack->mStack = NULL;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void stndrStackMgr::clear( stndrStack * aStack )
{
    aStack->mDepth  = STNDR_STACK_DEPTH_EMPTY;
}

IDE_RC stndrStackMgr::push( stndrStack      * aStack,
                            stndrStackSlot  * aItem )
{
    stndrStackSlot  * sTempStack = NULL;
    SInt              sTempStackSize = 0;
    UInt              sState = 0;

    
    aStack->mDepth++;
    
    if( aStack->mDepth >= aStack->mStackSize )
    {
        sTempStackSize = aStack->mStackSize + STNDR_STACK_DEFAULT_SIZE;

        IDE_TEST( iduMemMgr::malloc(
                      IDU_MEM_ST_STN,
                      ID_SIZEOF(stndrStackSlot) * sTempStackSize,
                      (void**)&sTempStack)
                  != IDE_SUCCESS );
        sState = 1;

        idlOS::memcpy( sTempStack, aStack->mStack,
                       sizeof(stndrStackSlot) * aStack->mStackSize );

        IDE_TEST( iduMemMgr::free(aStack->mStack) != IDE_SUCCESS );

        aStack->mStack = sTempStack;
        aStack->mStackSize = sTempStackSize;
        
        sState = 0;
    }
    
    aStack->mStack[aStack->mDepth] = *aItem;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)iduMemMgr::free( sTempStack );
    }
    
    return IDE_FAILURE;
}

IDE_RC stndrStackMgr::push( stndrStack  * aStack,
                            scPageID      aNodePID,
                            ULong         aSmoNo,
                            SShort        aKeySeq )
{
    stndrStackSlot  sSlot;

    sSlot.mNodePID  = aNodePID;
    sSlot.mSmoNo    = aSmoNo;
    sSlot.mKeySeq   = aKeySeq;
    
    return stndrStackMgr::push( aStack, &sSlot );
}

stndrStackSlot stndrStackMgr::pop( stndrStack * aStack )
{
    IDE_ASSERT( aStack->mDepth < aStack->mStackSize );
    IDE_ASSERT( aStack->mDepth >= 0 );
    
    return aStack->mStack[ aStack->mDepth-- ];
}

void stndrStackMgr::copy( stndrStack * aDstStack,
                          stndrStack * aSrcStack )
{
    IDE_ASSERT( aDstStack->mStack != NULL );
    IDE_ASSERT( aDstStack->mStackSize > aSrcStack->mDepth );
    
    aDstStack->mDepth = aSrcStack->mDepth;

    if( aSrcStack->mDepth >= 0 )
    {
        idlOS::memcpy( aDstStack->mStack,
                       aSrcStack->mStack,
                       ID_SIZEOF(stndrStackSlot) * (aSrcStack->mDepth + 1) );
    }
}

SInt stndrStackMgr::getDepth( stndrStack * aStack )
{
    return aStack->mDepth;
}

SInt stndrStackMgr::getStackSize( stndrStack * aStack )
{
    return aStack->mStackSize;
}
