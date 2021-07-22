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
 

#include <dkp.h>

#include <dkErrorCode.h>

#include <dkpBatchStatementMgr.h>

/*
 *
 */
struct dkpBatchStatement 
{
    iduListNode    mNode;

    iduList        mPacketList;
    UInt           mPacketListCount;

    iduListNode  * mIterator;
};

/*
 *
 */
typedef struct dkpBatchStatementBindVariable 
{
    iduListNode mNode;

    UInt        mBindVariableIndex;
    UInt        mBindVariableType;
    UInt        mBindVariableLength;
    SChar     * mBindVariableValue;

} dkpBatchStatementBindVariable;

/*
 *
 */
struct dkpBatchStatementMgr 
{
    iduList mBatchStatementList;
    UInt    mBatchStatementListCount;

    /*
     * Working set.
     */
    iduList mBindVariableList;
    UInt    mBindVariableCount;

    iduListNode * mIterator;
};

/*
 *
 */
IDE_RC dkpBatchStatementMgrCreate( dkpBatchStatementMgr ** aManager )
{
    dkpBatchStatementMgr * sManager = NULL;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dkpBatchStatementMgr ),
                                       (void **)&sManager,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDU_LIST_INIT( &(sManager->mBatchStatementList) );
    sManager->mBatchStatementListCount = 0;

    sManager->mIterator = NULL;

    IDU_LIST_INIT( &(sManager->mBindVariableList) );
    sManager->mBindVariableCount = 0;

    *aManager = sManager;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sManager != NULL )
    {
        (void)iduMemMgr::free( sManager );
        sManager = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_FAILURE;
}

/*
 *
 */
static void freeBindVariableList( iduList * aList )
{
    iduListNode * sIterator = NULL;
    iduListNode * sNodeNext = NULL;

    IDU_LIST_ITERATE_SAFE( aList, sIterator, sNodeNext )
    {
        dkpBatchStatementBindVariable * sVariable = (dkpBatchStatementBindVariable *)sIterator;

        IDU_LIST_REMOVE( sIterator );

        (void)iduMemMgr::free( sVariable );
    }
}

/*
 *
 */
static void freePacketList( iduList * aPacketList )
{
    iduListNode * sIterator = NULL;
    iduListNode * sNodeNext = NULL;

    IDU_LIST_ITERATE_SAFE( aPacketList, sIterator, sNodeNext )
    {
        dknPacket * sPacket = (dknPacket *)sIterator;

        IDU_LIST_REMOVE( sIterator );

        dknPacketDestroy( sPacket );
        sPacket = NULL;
    }
}


/*
 *
 */
static void freeBatchStatement( dkpBatchStatement * aBatchStatement )
{
    IDE_DASSERT( aBatchStatement != NULL );
    
    freePacketList( &(aBatchStatement->mPacketList) );
    aBatchStatement->mPacketListCount = 0;

    (void)iduMemMgr::free( aBatchStatement );
    aBatchStatement = NULL;
}


/*
 *
 */
static void freeBatchStatementList( iduList * aList )
{
    iduListNode * sIterator = NULL;
    iduListNode * sNodeNext = NULL;

    IDE_DASSERT( aList != NULL );
    IDE_DASSERT( aList->mNext != NULL );
    
    IDU_LIST_ITERATE_SAFE( aList, sIterator, sNodeNext )
    {
        dkpBatchStatement * sBatchStatement = (dkpBatchStatement *)sIterator;

        IDU_LIST_REMOVE( sIterator );

        freeBatchStatement( sBatchStatement );
        sBatchStatement = NULL;
    }
}


/*
 *
 */
void dkpBatchStatementMgrDestroy( dkpBatchStatementMgr * aManager )
{
    IDE_DASSERT( aManager != NULL );

    (void)dkpBatchStatementMgrClear( aManager );

    (void)iduMemMgr::free( aManager );
    aManager = NULL;
}

/*
 *
 */
IDE_RC dkpBatchStatementMgrClear( dkpBatchStatementMgr * aManager )
{
    freeBatchStatementList( &(aManager->mBatchStatementList) );
    aManager->mBatchStatementListCount = 0;

    freeBindVariableList( &(aManager->mBindVariableList) );
    aManager->mBindVariableCount = 0;

    return IDE_SUCCESS;
}

/*
 *
 */
static IDE_RC allocBatchStatement( dkpBatchStatement ** aNewBatchStatement )
{
    dkpBatchStatement * sBatchStatement = NULL;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dkpBatchStatement ),
                                       (void **)&sBatchStatement,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDU_LIST_INIT_OBJ( &(sBatchStatement->mNode), sBatchStatement );

    IDU_LIST_INIT( &(sBatchStatement->mPacketList) );
    sBatchStatement->mPacketListCount = 0;

    sBatchStatement->mIterator = NULL;

    *aNewBatchStatement = sBatchStatement;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sBatchStatement != NULL )
    {
        (void)iduMemMgr::free( sBatchStatement );
        sBatchStatement = NULL;
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkpBatchStatementMgrAddBindVariable( dkpBatchStatementMgr * aManager,
                                            UInt           aBindVarIdx,
                                            UInt           aBindVarType,
                                            UInt           aBindVarLen,
                                            SChar         *aBindVar )
{
    dkpBatchStatementBindVariable *sVariable = NULL;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dkpBatchStatementBindVariable ) + aBindVarLen,
                                       (void **)&sVariable,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDU_LIST_INIT_OBJ( &(sVariable->mNode), sVariable );

    sVariable->mBindVariableIndex = aBindVarIdx;
    sVariable->mBindVariableType = aBindVarType;
    sVariable->mBindVariableLength = aBindVarLen;
    sVariable->mBindVariableValue = (SChar *)sVariable + ID_SIZEOF( dkpBatchStatementBindVariable );

    idlOS::memcpy( sVariable->mBindVariableValue, aBindVar, aBindVarLen );

    IDU_LIST_ADD_LAST( &(aManager->mBindVariableList), &(sVariable->mNode) );
    aManager->mBindVariableCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sVariable != NULL )
    {
        (void)iduMemMgr::free( sVariable );
        sVariable = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_FAILURE;
}

/*
 *
 */
static UInt calculateDataSizeOfBindVariables( iduList * aList )
{
    UInt sSize = 0;
    iduListNode * sIterator = NULL;

    IDU_LIST_ITERATE( aList, sIterator )
    {
        dkpBatchStatementBindVariable * sVariable = (dkpBatchStatementBindVariable *)sIterator;

        sSize += ID_SIZEOF( sVariable->mBindVariableIndex ) + ID_SIZEOF( sVariable->mBindVariableType ) +
            ID_SIZEOF( sVariable->mBindVariableLength ) + sVariable->mBindVariableLength;
    }

    return sSize;
}

/*
 *
 */
IDE_RC dkpBatchStatementMgrAddBatchStatement( dkpBatchStatementMgr * aManager,
                                              SLong               aRemoteStmtId )
{
    dknPacket * sPacket = NULL;
    UInt sDataSize = 0;
    UInt sRemainedDataSize = 0;
    iduListNode * sIterator = NULL;
    dkpBatchStatement * sBatchStatement = NULL;

    /*
     * ADLP Header and ADD Batch Protocol's first part( Remote Statement Id(8), Bind Variable Count(4) )
     */
    sDataSize += DKP_ADLP_HDR_LEN + ID_SIZEOF( ULong ) + ID_SIZEOF( UInt );

    sDataSize += calculateDataSizeOfBindVariables( &(aManager->mBindVariableList) );

    IDE_TEST( allocBatchStatement( &(sBatchStatement) ) != IDE_SUCCESS );

    if ( sDataSize < dknPacketGetMaxDataLength() )
    {
        IDE_TEST( dknPacketCreate( &sPacket, sDataSize ) != IDE_SUCCESS );
        IDE_TEST( dknPacketSkip( sPacket, DKP_ADLP_HDR_LEN ) != IDE_SUCCESS );

        IDE_TEST( dknPacketWriteEightByteNumber( sPacket, &aRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST( dknPacketWriteFourByteNumber( sPacket, &(aManager->mBindVariableCount) ) != IDE_SUCCESS );

        IDU_LIST_ITERATE( &(aManager->mBindVariableList), sIterator )
        {
            dkpBatchStatementBindVariable * sVariable = (dkpBatchStatementBindVariable *)sIterator;

            IDE_TEST( dknPacketWriteFourByteNumber( sPacket, &(sVariable->mBindVariableIndex) ) != IDE_SUCCESS );
            IDE_TEST( dknPacketWriteFourByteNumber( sPacket, &(sVariable->mBindVariableType) ) != IDE_SUCCESS );
            IDE_TEST( dknPacketWriteFourByteNumber( sPacket, &(sVariable->mBindVariableLength) ) != IDE_SUCCESS );
            IDE_TEST( dknPacketWrite( sPacket, sVariable->mBindVariableValue, sVariable->mBindVariableLength )
                      != IDE_SUCCESS );
        }

        IDU_LIST_ADD_LAST( &(sBatchStatement->mPacketList), &(sPacket->mNode) );
        sBatchStatement->mPacketListCount++;
        sPacket = NULL;

    }
    else
    {
        /*
         * 2개 이상의 패킷으로 나누는 지점
         *      Bind Variable Index 시작 부분
         *      Bind Variable Value의 중간 부분
         */
        
        UInt sThreeFieldLength = ID_SIZEOF( UInt ) + ID_SIZEOF( UInt ) + ID_SIZEOF( UInt );

        sRemainedDataSize = sDataSize;

        IDE_TEST( dknPacketCreate( &sPacket, dknPacketGetMaxDataLength() ) != IDE_SUCCESS );

        IDE_TEST( dknPacketSkip( sPacket, DKP_ADLP_HDR_LEN ) != IDE_SUCCESS );
        IDE_TEST( dknPacketWriteEightByteNumber( sPacket, &aRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST( dknPacketWriteFourByteNumber( sPacket, &(aManager->mBindVariableCount) ) != IDE_SUCCESS );
        sRemainedDataSize -= (DKP_ADLP_HDR_LEN + ID_SIZEOF( ULong ) + ID_SIZEOF( UInt ) );

        IDU_LIST_ITERATE( &(aManager->mBindVariableList), sIterator )
        {
            dkpBatchStatementBindVariable * sVariable = (dkpBatchStatementBindVariable *)sIterator;

            if ( dknPacketGetCapacity( sPacket ) < sThreeFieldLength )
            {
                IDU_LIST_ADD_LAST( &(sBatchStatement->mPacketList), &(sPacket->mNode) );
                sBatchStatement->mPacketListCount++;

                if ( sRemainedDataSize > dknPacketGetMaxDataLength() )
                {
                    IDE_TEST( dknPacketCreate( &sPacket, dknPacketGetMaxDataLength() ) != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( dknPacketCreate( &sPacket, sRemainedDataSize ) != IDE_SUCCESS );
                }
            }
            else
            {
                /* nothing to do */
            }

            IDE_TEST( dknPacketWriteFourByteNumber( sPacket, &(sVariable->mBindVariableIndex) ) != IDE_SUCCESS );
            IDE_TEST( dknPacketWriteFourByteNumber( sPacket, &(sVariable->mBindVariableType) ) != IDE_SUCCESS );
            IDE_TEST( dknPacketWriteFourByteNumber( sPacket, &(sVariable->mBindVariableLength) ) != IDE_SUCCESS );
            sRemainedDataSize -= sThreeFieldLength;

            if ( dknPacketGetCapacity( sPacket ) > sVariable->mBindVariableLength )
            {
                IDE_TEST( dknPacketWrite( sPacket, sVariable->mBindVariableValue, sVariable->mBindVariableLength )
                          != IDE_SUCCESS );
                sRemainedDataSize -= sVariable->mBindVariableLength;
            }
            else
            {
                UInt sRemainedVariableLength = sVariable->mBindVariableLength;

                while ( sRemainedVariableLength > 0 )
                {
                    UInt sPacketCapacity = dknPacketGetCapacity( sPacket );

                    if ( sPacketCapacity > sRemainedVariableLength )
                    {
                        IDE_TEST( dknPacketWrite( sPacket, sVariable->mBindVariableValue, sRemainedVariableLength )
                                  != IDE_SUCCESS );
                        sRemainedVariableLength -= sRemainedVariableLength;
                        sRemainedDataSize -= sRemainedVariableLength;
                    }
                    else
                    {
                        IDE_TEST( dknPacketWrite( sPacket, sVariable->mBindVariableValue, sPacketCapacity )
                                  != IDE_SUCCESS );
                        sRemainedVariableLength -= sPacketCapacity;
                        sRemainedDataSize -= sPacketCapacity;
                    }

                    if ( sRemainedVariableLength > 0 )
                    {
                        IDU_LIST_ADD_LAST( &(sBatchStatement->mPacketList), &(sPacket->mNode) );
                        sBatchStatement->mPacketListCount++;

                        if ( sRemainedDataSize > dknPacketGetMaxDataLength() )
                        {
                            IDE_TEST( dknPacketCreate( &sPacket, dknPacketGetMaxDataLength() ) != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( dknPacketCreate( &sPacket, sRemainedDataSize ) != IDE_SUCCESS );
                        }
                    }
                    else
                    {
                        /* nothing to do */
                    }
                }
            }
        }

        IDU_LIST_ADD_LAST( &(sBatchStatement->mPacketList), &(sPacket->mNode) );
        sBatchStatement->mPacketListCount++;
        sPacket = NULL;
    }

    IDU_LIST_ADD_LAST( &aManager->mBatchStatementList, &(sBatchStatement->mNode) );
    aManager->mBatchStatementListCount++;
    sBatchStatement = NULL;

    freeBindVariableList( &(aManager->mBindVariableList) );
    aManager->mBindVariableCount = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sPacket != NULL )
    {
        dknPacketDestroy( sPacket );
        sPacket = NULL;
    }
    else
    {
        /* nothing to do */
    }

    if ( sBatchStatement != NULL )
    {
        freeBatchStatement( sBatchStatement );
        sBatchStatement = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/*
 *
 */
UInt dkpBatchStatementMgrCountBatchStatement( dkpBatchStatementMgr * aManager )
{
    return aManager->mBatchStatementListCount;
}

/*
 *
 */
UInt dkpBatchStatementMgrCountPacket( dkpBatchStatement * aBatchStatement )
{
    return aBatchStatement->mPacketListCount;
}

/*
 *
 */
IDE_RC dkpBatchStatementMgrGetNextBatchStatement( dkpBatchStatementMgr * aManager,
                                                  dkpBatchStatement   ** aBatchStatement )
{
    dkpBatchStatement * sBatchStatement = NULL;

    if ( aManager->mIterator == NULL )
    {
        if ( IDU_LIST_IS_EMPTY( &(aManager->mBatchStatementList) ) == ID_TRUE )
        {
            sBatchStatement = NULL;
        }
        else
        {
            aManager->mIterator = IDU_LIST_GET_FIRST( &(aManager->mBatchStatementList) );
            sBatchStatement = (dkpBatchStatement *)aManager->mIterator->mObj;
        }
    }
    else
    {
        if ( IDU_LIST_IS_EMPTY( &(aManager->mBatchStatementList) ) == ID_TRUE )
        {
            sBatchStatement = NULL;
        }
        else
        {
            aManager->mIterator = IDU_LIST_GET_NEXT( aManager->mIterator );
            sBatchStatement = (dkpBatchStatement *)aManager->mIterator->mObj;
        }
    }

    *aBatchStatement = sBatchStatement;

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkpBatchStatementMgrGetNextPacket( dkpBatchStatement * aBatchStatement,
                                          dknPacket        ** aPacket )
{
    dknPacket * sPacket = NULL;

    if ( aBatchStatement->mIterator == NULL )
    {
        if ( IDU_LIST_IS_EMPTY( &(aBatchStatement->mPacketList) ) == ID_TRUE )
        {
            sPacket = NULL;
        }
        else
        {
            aBatchStatement->mIterator = IDU_LIST_GET_FIRST( &(aBatchStatement->mPacketList) );
            sPacket = (dknPacket *)aBatchStatement->mIterator->mObj;
        }
    }
    else
    {
        if ( IDU_LIST_IS_EMPTY( &(aBatchStatement->mPacketList) ) == ID_TRUE )
        {
            sPacket = NULL;
        }
        else
        {
            aBatchStatement->mIterator = IDU_LIST_GET_NEXT( aBatchStatement->mIterator );
            sPacket = (dknPacket *)aBatchStatement->mIterator->mObj;
        }
    }

    *aPacket = sPacket;

    return IDE_SUCCESS;
}
