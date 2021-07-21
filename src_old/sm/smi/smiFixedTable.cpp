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
 * $Id: smiFixedTable.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <idl.h>
#include <iduMemMgr.h>
#include <smiFixedTable.h>
#include <smuHash.h>
#include <smnfModule.h>
#include <smErrorCode.h>
#include <smiMisc.h>

smuHashBase        smiFixedTable::mTableHeaderHash;

// PROJ-2083 DUAL Table
static UChar      *gConstRecord      = NULL;

UInt smiFixedTable::genHashValueFunc(void *aKey)
{
    UInt   sValue = 0;
    SChar *sName;
    // 저장된 포인터를 이용하여, 이름 포인터 얻기
    sName = (SChar *)(*((void **)aKey));

    while( (*sName++) != 0)
    {
        sValue ^= (UInt)(*sName);
    }
    return sValue;
}

SInt smiFixedTable::compareFunc(void* aLhs, void* aRhs)
{

    IDE_DASSERT( aLhs != NULL );
    IDE_DASSERT( aRhs != NULL );

    return idlOS::strcmp((SChar *)(*(void **)aLhs), (SChar *)*((void **)(aRhs)) );

}


/*
 * QP에서 표현될 Column의 크기와 Offset을 계산한다.
 * 원래 이 계산은 QP에 의해 수행되어야 하나,
 * A4의 특성상 Meta 없이도 접근해야 하기 때문에 SM에서 계산한다.
 * BUGBUG : 실제 QP가 사용하는 방식과 유사한지 검증해야 한다.
 */
void   smiFixedTable::initColumnInformation(iduFixedTableDesc *aDesc)
{
    UInt                  sOffset;
    UInt                  sColCount;
    iduFixedTableColDesc *sColDesc;

    for (sColDesc = aDesc->mColDesc,
             sOffset = SMP_SLOT_HEADER_SIZE,
             sColCount = 0;
         sColDesc->mName != NULL;
         sColDesc++, sColCount++)
    {
        sColDesc->mTableName = aDesc->mTableName;
        sColDesc->mColOffset = sOffset;
        switch(IDU_FT_GET_TYPE(sColDesc->mDataType))
        {
            case IDU_FT_TYPE_VARCHAR:
            case IDU_FT_TYPE_CHAR:
                sColDesc->mColSize = idlOS::align8(sColDesc->mLength + 2);
                sOffset += sColDesc->mColSize;
                break;
            case IDU_FT_TYPE_UBIGINT:
            case IDU_FT_TYPE_USMALLINT:
            case IDU_FT_TYPE_UINTEGER:
            case IDU_FT_TYPE_BIGINT:
            case IDU_FT_TYPE_SMALLINT:
            case IDU_FT_TYPE_INTEGER:
            case IDU_FT_TYPE_DOUBLE:
                sColDesc->mColSize = idlOS::align8(sColDesc->mLength);
                sOffset += sColDesc->mColSize;
                break;
            default:
                IDE_CALLBACK_FATAL("Unknown Datatype");
        }
    }

    /* ------------------------------------------------
     *  Record의 Set이 연속적이지 않을 수 있기 때문에
     *  각 레코드끼리 포인터로 연결되어 있다.
     *  그래서, 레코드의 처음 8바이트를
     *  next record pointer로 사용한다.
     * ----------------------------------------------*/

    aDesc->mSlotSize = (sOffset + idlOS::align8( ID_SIZEOF(void *)));
    aDesc->mColCount = sColCount;
}


// Fixed Table 헤더를 초기화 한다.
// initializeStatic()을 통해 단 한번 호출됨.
void   smiFixedTable::initFixedTableHeader(smiFixedTableHeader *aHeader,
                                           iduFixedTableDesc   *aDesc)
{
    IDE_ASSERT( aHeader != NULL );
    IDE_ASSERT( aDesc != NULL );
    
    // Slot Header
    SM_INIT_SCN( &(aHeader->mSlotHeader.mCreateSCN) );
    SM_SET_SCN_FREE_ROW( &(aHeader->mSlotHeader.mLimitSCN) );
    SMP_SLOT_SET_OFFSET( &(aHeader->mSlotHeader), 0 );

    // To Fix BUG-18186
    //   Fixed Table의 Cursor open도중 Table Header의
    //   mIndexes필드를 초기화되지 않은 채로 읽음
    idlOS::memset( &aHeader->mHeader, 0, ID_SIZEOF(aHeader->mHeader));

    // Table Header Init
    aHeader->mHeader.mFlag = SMI_TABLE_REPLICATION_DISABLE |
                             SMI_TABLE_LOCK_ESCALATION_DISABLE |
                             SMI_TABLE_FIXED;

    /* PROJ-2162 */
    aHeader->mHeader.mIsConsistent = ID_TRUE;

    // PROJ-2083 Dual Table
    aHeader->mHeader.mFlag &= ~SMI_TABLE_DUAL_MASK;

    // PROJ-1071 Parallel query
    // Default parallel degree is 1
    aHeader->mHeader.mParallelDegree = 1;

    if( idlOS::strcmp( aDesc->mTableName, "X$DUAL" ) == 0 )
    {
        aHeader->mHeader.mFlag |= SMI_TABLE_DUAL_TRUE;
    }
    else
    {
        aHeader->mHeader.mFlag |= SMI_TABLE_DUAL_FALSE;
    }

    aHeader->mHeader.mRuntimeInfo = NULL;

    /* BUG-42928 No Partition Lock */
    aHeader->mHeader.mReplacementLock = NULL;

    // Descriptor Assign
    aHeader->mDesc = aDesc;

}

// 등록된 모든 Fixed Table에 대해 객체를 생성하고, Hash를 구성한다.

IDE_RC smiFixedTable::initializeStatic( SChar * aNameType )
{
    iduFixedTableDesc *sDesc;
    UInt               sColCount;
    SChar             *sTableName; // sys_tables_
    UInt               sTableNameLen;
    UInt               i;

    IDE_ASSERT( aNameType != NULL );
    
    if( idlOS::strncmp( aNameType, "X$", 2 ) == 0 )
    {
        iduFixedTable::setCallback(NULL, buildRecord, checkKeyRange );

        IDE_TEST( smuHash::initialize(&mTableHeaderHash,
                                      1,
                                      16,    /* aBucketCount */
                                      ID_SIZEOF(void *), /* aKeyLength */
                                      ID_FALSE,          /* aUseLatch */
                                      genHashValueFunc,
                                      compareFunc) != IDE_SUCCESS );
    }

    for (sDesc = iduFixedTable::getTableDescHeader(), sColCount = 0;
         sDesc != NULL;
         sDesc = sDesc->mNext, sColCount++)
    {
        /* TC/FIT/Limit/sm/smi/smiFixedTable_initializeStatic_malloc1.sql */
        IDU_FIT_POINT_RAISE( "smiFixedTable::initializeStatic::malloc1",
                              insufficient_memory );

        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMI,
                                   40, //  BUGBUG, DEFINED CONSTANT
                                   (void **)&sTableName) != IDE_SUCCESS,
                       insufficient_memory );

        sTableNameLen = idlOS::strlen( sDesc->mTableName );
        for( i = 0; i < sTableNameLen; i++ )
        {
            // PRJ-1678 : multi-byte character로 된 fixed table은 없겠지...
            sTableName[i] = idlOS::idlOS_toupper( sDesc->mTableName[i] );
        }
        sTableName[i] = '\0';

        if( idlOS::strncmp( aNameType, sTableName, 2 ) == 0 )
        {
            smiFixedTableHeader *sHeader;
            smiFixedTableNode   *sNode;

            /* TC/FIT/Limit/sm/smi/smiFixedTable_initializeStatic_malloc2.sql */
            IDU_FIT_POINT_RAISE( "smiFixedTable::initializeStatic::malloc2",
                                  insufficient_memory );

            // smcTable 객체 할당
            IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMI,
                                       ID_SIZEOF(smiFixedTableHeader),
                                       (void **)&sHeader) != IDE_SUCCESS,
                           insufficient_memory );

            initFixedTableHeader(sHeader, sDesc);

            initColumnInformation(sDesc);

            /* TC/FIT/Limit/sm/smi/smiFixedTable_initializeStatic_malloc3.sql */
            IDU_FIT_POINT_RAISE( "smiFixedTable::initializeStatic::malloc3",
                                  insufficient_memory );

            // Node 객체 할당
            IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMI,
                                       ID_SIZEOF(smiFixedTableNode),
                                       (void **)&sNode) != IDE_SUCCESS,
                           insufficient_memory );

            sDesc->mTableName = sNode->mName   = sTableName;

            for( i = 0; i < sDesc->mColCount; i++ )
            {
                sDesc->mColDesc[i].mTableName = sTableName;
            }

            sNode->mHeader = sHeader;

            IDE_TEST(smuHash::insertNode(&mTableHeaderHash,
                                         (void *)&(sNode->mName),
                                         (void *)sNode) != IDE_SUCCESS);

        }
        else
        {
            IDE_TEST( iduMemMgr::free(sTableName) != IDE_SUCCESS);

        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiFixedTable::destroyStatic()
{
    smiFixedTableNode   *sNode;

    IDE_TEST( smuHash::open(&mTableHeaderHash) != IDE_SUCCESS );

    IDE_TEST(smuHash::cutNode(&mTableHeaderHash,
                              (void **)&sNode) != IDE_SUCCESS);

    while( sNode != NULL)
    {
        // free fixed table name memory
        IDE_TEST( iduMemMgr::free(sNode->mName ) != IDE_SUCCESS);
        // free fixed table header slot
        IDE_TEST(iduMemMgr::free(sNode->mHeader) != IDE_SUCCESS);
        // free hash node
        IDE_TEST(iduMemMgr::free(sNode) != IDE_SUCCESS);
        IDE_TEST(smuHash::cutNode(&mTableHeaderHash,
                                      (void **)&sNode) != IDE_SUCCESS);
    }//while

    IDE_TEST( smuHash::close(&mTableHeaderHash) != IDE_SUCCESS );
    IDE_TEST( smuHash::destroy(&mTableHeaderHash) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*
IDE_RC smiFixedTable::initializeTables(smiStartupPhase aPhase)
{
    iduFixedTableDesc *sDesc;

    for (sDesc = iduFixedTable::getTableDescHeader();
         sDesc != NULL;
         sDesc = sDesc->mNext)
    {
        if (sDesc->mEnablePhase == (iduStartupPhase)aPhase)
        {
            // 초기화를 통해 사용할 수 있도록 한다.
            // mHeader 초기화?..
        }
    }

    return IDE_SUCCESS;
}
*/
IDE_RC smiFixedTable::findTable(SChar *aName, void **aHeader)
{
    smiFixedTableNode *sNode;

    IDE_ASSERT( aName != NULL );
    IDE_ASSERT( aHeader != NULL );
    
    IDE_TEST(smuHash::findNode(&mTableHeaderHash,
                               (void *)(&aName),
                               (void **)&sNode) != IDE_SUCCESS);

    if (sNode == NULL)
    {
        *aHeader = NULL;
    }
    else
    {
        *aHeader = sNode->mHeader;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

extern smiStartupPhase smiGetStartupPhase();

idBool smiFixedTable::canUse(void *aHeader)
{
    smiFixedTableHeader *sHeader;

    IDE_ASSERT( aHeader != NULL );

    sHeader = (smiFixedTableHeader *)aHeader;

    if (smiGetStartupPhase() < (smiStartupPhase)sHeader->mDesc->mEnablePhase)
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}


UInt   smiFixedTable::getSlotSize(void *aHeader)
{
    smiFixedTableHeader *sHeader;

    IDE_ASSERT( aHeader != NULL );

    sHeader = (smiFixedTableHeader *)((SChar *)aHeader -  SMP_SLOT_HEADER_SIZE);

    return sHeader->mDesc->mSlotSize;
}


iduFixedTableBuildFunc smiFixedTable::getBuildFunc(void *aHeader)
{
    smiFixedTableHeader *sHeader;

    IDE_ASSERT( aHeader != NULL );

    sHeader = (smiFixedTableHeader *)((SChar *)aHeader -  SMP_SLOT_HEADER_SIZE);

    return sHeader->mDesc->mBuildFunc;
}

/* ------------------------------------------------
 *  이 함수는 CHAR 타입의 변환시에
 *  입력된 스트링의 길이가 Null-Teminate가
 *  아닐 수 있기 때문에 그 길이의 검사를
 *  대상 CHAR 타입의 길이만큼만 검사하여, 무한루프를
 *  방지하는데 있다.
 * ----------------------------------------------*/
static UInt getCharLength(SChar *aChar, UInt aExpectedLength)
{
    UInt i;

    IDE_ASSERT( aChar != NULL );
    
    for (i = 0; i < aExpectedLength; i++)
    {
        if (aChar[i] == 0)
        {
            return i;
        }
    }
    return aExpectedLength;
}

SShort smiFixedTable::mSizeOfDataType[] =
{
    /* IDU_FT_TYPE_CHAR       (0x0000)*/ 0,
    /* IDU_FT_TYPE_BIGINT     (0x0001)*/  IDU_FT_SIZEOF_BIGINT,
    /* IDU_FT_TYPE_SMALLINT   (0x0002)*/  IDU_FT_SIZEOF_SMALLINT,
    /* IDU_FT_TYPE_INTEGER    (0x0003)*/  IDU_FT_SIZEOF_INTEGER,
    /* IDU_FT_TYPE_DOUBLE     (0x0004)*/  IDU_FT_SIZEOF_DOUBLE,
    /* IDU_FT_TYPE_UBIGINT    (0x0005)*/  IDU_FT_SIZEOF_BIGINT,
    /* IDU_FT_TYPE_USMALLINT  (0x0006)*/  IDU_FT_SIZEOF_SMALLINT,
    /* IDU_FT_TYPE_UINTEGER   (0x0007)*/  IDU_FT_SIZEOF_INTEGER
};

void smiFixedTable::buildOneColumn( iduFixedTableDataType          aDataType,
                                    UChar                        * aObj,
                                    void                         * aSourcePtr,
                                    UInt                           aSourceSize,
                                    void                         * aTargetPtr,
                                    iduFixedTableConvertCallback   aConvCallback )
{
    UInt    sStrLen;
    UChar * sColPos;
    UInt    sTargetSize; /* 해당 데이타 타입에 맞는 대상의 메모리 크기 */

    switch (IDU_FT_GET_TYPE(aDataType))
    {
        case IDU_FT_TYPE_UBIGINT:
        case IDU_FT_TYPE_USMALLINT:
        case IDU_FT_TYPE_UINTEGER:
        case IDU_FT_TYPE_BIGINT:
        case IDU_FT_TYPE_SMALLINT:
        case IDU_FT_TYPE_INTEGER:
        case IDU_FT_TYPE_DOUBLE:
        {
            /*
             *  사용자에 의해 어떠한 타입 예를 들면 BIGINT(8)로 정의되었더라도
             *  실제 값을 변환하는 원래의 타입이 8바이트가 아닐 수 있다.
             *  typedef enum 같은 경우 그 크기를 알 수 없기에 이러한 크기의
             *  mismatch가 발생할 수 있다.
             *
             *  그런 이유로 서로간의 데이타 크기를 비교하여, 적절하게
             *  자동적으로 변환을 해주도록 한다.
             *
             *   sObj                         aRowBuf
             *  +---------+                  +-----------------+
             *  |11223344 |sObject(4)   ===> |1122334455667788 | (8 byte)
             *  |---------|                  |-----------------|
             *  +---------+                  +-----------------+
             */

            sTargetSize = mSizeOfDataType[IDU_FT_GET_TYPE(aDataType)];
            if ( aConvCallback != NULL )
            {
                aConvCallback( aObj,
                               aSourcePtr,
                               (UChar *)aTargetPtr,
                               sTargetSize );
            }
            else
            {
                /*
                 * 4개의 인자를 이용해서 변환 요망.
                 */
                if (sTargetSize == aSourceSize)
                {
                    idlOS::memcpy(aTargetPtr, aSourcePtr, sTargetSize);
                }
                else
                {
                    /*
                     *  Source가 큰 경우는 발생할 수 없음.
                     *  만일 있다면, Fixed Table을 잘못 정의한 것임.
                     *  변환 조건 : 2->4, 2->8, 4->8 , 그외는 없음.
                     */
                    IDE_ASSERT(sTargetSize >= aSourceSize);

                    if (sTargetSize == 4)
                    {
                        /* Short->Integer 2->4*/
                        UInt sIntBuf;

                        sIntBuf = (UInt)(*((UShort *)aSourcePtr));
                        idlOS::memcpy(aTargetPtr, &sIntBuf, sTargetSize);
                    }
                    else
                    {
                        /* sTargetSize == 8 */
                        if (aSourceSize == 2)
                        {
                            /* 2->8 */
                            ULong sLongBuf;

                            sLongBuf = (ULong)(*((UShort *)aSourcePtr));
                            idlOS::memcpy(aTargetPtr, &sLongBuf, sTargetSize);
                        }
                        else
                        {
                            /* 4->8*/
                            /* 2->8 */
                            ULong sLongBuf;

                            sLongBuf = (ULong)(*((UInt *)aSourcePtr));
                            idlOS::memcpy(aTargetPtr, &sLongBuf, sTargetSize);
                        }
                    }
                }
            }
        }
        break;

        case IDU_FT_TYPE_CHAR:
        {
            sColPos = ( UChar * )aTargetPtr;
            *(UShort *)sColPos = aSourceSize;
            sColPos += ID_SIZEOF(UShort);

            if ( aConvCallback != NULL )
            {
                // 해당 Object를 CHAR로 변환하라!
                sStrLen = aConvCallback( aObj,
                                         aSourcePtr,
                                         sColPos,
                                         aSourceSize );
            }
            else
            {

                sStrLen = getCharLength((SChar *)aSourcePtr, aSourceSize );

                idlOS::memcpy(sColPos, aSourcePtr, sStrLen);
            }
            // Fill ' '
            idlOS::memset(sColPos + sStrLen ,
                          ' ',
                          aSourceSize - sStrLen);

            break;
        }

        case IDU_FT_TYPE_VARCHAR:
        {
            sColPos = ( UChar * )aTargetPtr;

            if ( aConvCallback != NULL )
            {
                // 해당 Object를 CHAR로 변환하라!
                sStrLen = aConvCallback( aObj,
                                         aSourcePtr,
                                         (sColPos + ID_SIZEOF(UShort)),
                                         aSourceSize );
                *(UShort *)sColPos = sStrLen;
            }
            else
            {
                sStrLen = getCharLength((SChar *)aSourcePtr, aSourceSize );

                // Set String Size
                *(UShort *)sColPos = sStrLen;
                sColPos += ID_SIZEOF(UShort);

                // Fill Value with 0
                idlOS::memcpy(sColPos, aSourcePtr, sStrLen);
                idlOS::memset(sColPos + sStrLen ,
                              0,
                              aSourceSize - sStrLen);
            }

            break;
        }
        default:
            IDE_CALLBACK_FATAL("Unknown Datatype");
    }
}

IDE_RC smiFixedTable::buildOneRecord(iduFixedTableDesc *aTabDesc,
                                     UChar             *aRowBuf,
                                     UChar             *aObj)
{
    iduFixedTableColDesc *sColDesc;
    UChar                *sObject;

    IDE_ASSERT( aTabDesc != NULL );
    IDE_ASSERT( aRowBuf != NULL );
    IDE_ASSERT( aObj != NULL );
    
    for (sColDesc         = aTabDesc->mColDesc;
         sColDesc->mName != NULL;
         sColDesc++ )
    {
        sObject = (UChar *)aObj + sColDesc->mOffset;

        if (IDU_FT_IS_POINTER(sColDesc->mDataType))
        {
            // de-reference if pointer type..
            sObject = (UChar *)(*((void **)sObject));
        }

        switch(IDU_FT_GET_TYPE(sColDesc->mDataType))
        {
            case IDU_FT_TYPE_UBIGINT:
            case IDU_FT_TYPE_USMALLINT:
            case IDU_FT_TYPE_UINTEGER:
            case IDU_FT_TYPE_BIGINT:
            case IDU_FT_TYPE_SMALLINT:
            case IDU_FT_TYPE_INTEGER:
            case IDU_FT_TYPE_DOUBLE:
            {
                ULong sNullBuf[4] = { 0, 0, 0, 0 };
                UInt  sSourceSize; /* 사용자가 지정한 메모리 크기 */
                void *sTargetPtr;
                void *sSourcePtr;

                /*
                 *  사용자에 의해 어떠한 타입 예를 들면 BIGINT(8)로 정의되었더라도
                 *  실제 값을 변환하는 원래의 타입이 8바이트가 아닐 수 있다.
                 *  typedef enum 같은 경우 그 크기를 알 수 없기에 이러한 크기의
                 *  mismatch가 발생할 수 있다.
                 *
                 *  그런 이유로 서로간의 데이타 크기를 비교하여, 적절하게
                 *  자동적으로 변환을 해주도록 한다.
                 *
                 *   sObj                         aRowBuf
                 *  +---------+                  +-----------------+
                 *  |11223344 |sObject(4)   ===> |1122334455667788 | (8 byte)
                 *  |---------|                  |-----------------|
                 *  +---------+                  +-----------------+
                 */

                sSourceSize = sColDesc->mLength;
                sTargetPtr  = aRowBuf + sColDesc->mColOffset;
                if (sObject == NULL)
                {
                    // 32 byte를 0으로 채우고, memcpy를 위해 사용한다.
                    // 현실적으로 32byte 길이의 데이터 타입이 없기 때문에
                    // 안전하다고 판단된다.
                    IDE_DASSERT(ID_SIZEOF(sNullBuf) >= sColDesc->mLength);
                    sSourcePtr  = sNullBuf;
                }
                else
                {
                    sSourcePtr  = sObject;
                }

                buildOneColumn( sColDesc->mDataType,
                                aObj,
                                sSourcePtr,
                                sSourceSize,
                                sTargetPtr,
                                sColDesc->mConvCallback );
                break;
            }
            case IDU_FT_TYPE_CHAR:
            {
                UChar *sColPos;

                if (sColDesc->mConvCallback != NULL)
                {
                    sColPos = aRowBuf + sColDesc->mColOffset;
                }
                else
                {
                    if (sObject == NULL)
                    {
                        sObject = (UChar *)"";
                    }

                    sColPos = aRowBuf + sColDesc->mColOffset;
                }

                buildOneColumn( sColDesc->mDataType,
                                aObj,
                                sObject,
                                sColDesc->mLength,
                                (void *)sColPos,
                                sColDesc->mConvCallback );
                break;
            }

            case IDU_FT_TYPE_VARCHAR:
            {
                UChar *sColPos;

                if (sColDesc->mConvCallback != NULL)
                {
                    sColPos = aRowBuf + sColDesc->mColOffset;
                }
                else
                {
                    if (sObject == NULL)
                    {
                        sObject = (UChar *)"";
                    }

                    sColPos = aRowBuf + sColDesc->mColOffset;
                }

                buildOneColumn( sColDesc->mDataType,
                                aObj,
                                sObject,
                                sColDesc->mLength,
                                sColPos,
                                sColDesc->mConvCallback );
                break;
            }
            default:
                IDE_CALLBACK_FATAL("Unknown Datatype");
        }
    }

    return IDE_SUCCESS;
}

IDE_RC smiFixedTable::buildRecord( void                * aHeader,
                                   iduFixedTableMemory * aMemory,
                                   void                * aObj )
{
    smiFixedTableHeader  *sHeader;
    iduFixedTableDesc    *sTabDesc;
    UChar                *sRecord;
    idBool                sFilterResult;
    idBool                sLimitResult;
    smiFixedTableRecord  *sCurrRec;
    smiFixedTableRecord  *sBeforeRec;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );
    IDE_ERROR( aObj    != NULL );

    sHeader = (smiFixedTableHeader *)((SChar *)aHeader -  SMP_SLOT_HEADER_SIZE);
    sTabDesc = sHeader->mDesc;
    sFilterResult = ID_FALSE;
    sLimitResult  = ID_FALSE;

    if ( ( (((smcTableHeader*)aHeader)->mFlag & SMI_TABLE_DUAL_MASK)
         == SMI_TABLE_DUAL_TRUE ) &&
         ( aMemory->useExternalMemory() == ID_FALSE ) )
    {
        if( gConstRecord == NULL )
        {
            IDE_TEST(aMemory->allocateRecord(sTabDesc->mSlotSize, (void **)&gConstRecord)
                     != IDE_SUCCESS);

            // 해당 레코드를 메모리에 구축해야 한다.
            (void)buildOneRecord(
                sTabDesc,
                gConstRecord + idlOS::align8(ID_SIZEOF(void *)), // 실제 레코드 위치로
                (UChar *)aObj);
        }
        else
        {
            IDE_TEST( aMemory->initRecord((void **)&gConstRecord)
                      != IDE_SUCCESS );
        }

        sRecord = gConstRecord;
    }
    else
    {
        IDE_TEST(aMemory->allocateRecord(sTabDesc->mSlotSize, (void **)&sRecord)
                 != IDE_SUCCESS);
        
        // 해당 레코드를 메모리에 구축해야 한다.
        (void)buildOneRecord(
            sTabDesc,
            sRecord + idlOS::align8(ID_SIZEOF(void *)), // 실제 레코드 위치로
            (UChar *)aObj);
    }
    
    /*
     * 1. Filter 처리
     */
    IDE_DASSERT( aMemory->getContext() != NULL );

    /* BUG-42639 Monitoring query */
    if ( aMemory->useExternalMemory() == ID_FALSE )
    {
        IDE_TEST( smnfCheckFilter( aMemory->getContext(),
                                   sRecord,
                                   &sFilterResult )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( checkFilter( aMemory->getContext(),
                               sRecord,
                               &sFilterResult )
                  != IDE_SUCCESS );
    }

    if( sFilterResult == ID_TRUE )
    {
        /* BUG-42639 Monitoring query */
        if ( aMemory->useExternalMemory() == ID_FALSE )
        {
            /*
             * 2. Limit 처리
             */
            smnfCheckLimitAndMovePos( aMemory->getContext(),
                                      &sLimitResult,
                                      ID_TRUE ); // aIsMovePos
        }
        else
        {
            checkLimitAndMovePos( aMemory->getContext(),
                                  &sLimitResult,
                                  ID_TRUE ); // aIsMovePos
        }
    }
    
    if( (sFilterResult == ID_TRUE) && (sLimitResult == ID_TRUE) )
    {
        sCurrRec   = (smiFixedTableRecord *)aMemory->getCurrRecord();
        sBeforeRec = (smiFixedTableRecord *)aMemory->getBeforeRecord();

        if (sBeforeRec != NULL)
        {
            sBeforeRec->mNext = sCurrRec;
        }
    }
    else
    {
        /* BUG-42639 Monitoring query */
        if ( aMemory->useExternalMemory() == ID_FALSE )
        {
            if ( (((smcTableHeader*)aHeader)->mFlag & SMI_TABLE_DUAL_MASK)
                == SMI_TABLE_DUAL_FALSE )
            {
                aMemory->freeRecord(); // 결과 셋에 포함되지 않는 레코드는 free한다.
            }
            else
            {
                aMemory->resetRecord();
            }
        }
        else
        {
            aMemory->freeRecord(); // 결과 셋에 포함되지 않는 레코드는 free한다.
        }
    }
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//  IDE_RC smiFixedTable::allocRecordBuffer(void  *aHeader,
//                                          UInt   aRecordCount,
//                                          void **aBuffer)
//  {
//  //      IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMI,
//  //                                 1,
//  //                                 getSlotSize(aHeader) *  aRecordCount,
//  //                                 aBuffer)
//  //               != IDE_SUCCESS);

//      return IDE_SUCCESS;
//      IDE_EXCEPTION_END;
//      return IDE_FAILURE;
//  }


//IDE_RC smiFixedTable::freeRecordBuffer(void */*aBuffer*/)
//{
//      IDE_TEST(iduMemMgr::free(aBuffer) != IDE_SUCCESS);

//    return IDE_SUCCESS;
//}


UInt smiFixedTable::getColumnCount(void *aHeader)
{
    smiFixedTableHeader *sHeader;

    IDE_ASSERT( aHeader != NULL );
    
    sHeader = (smiFixedTableHeader *)aHeader;

    return sHeader->mDesc->mColCount;
}

SChar *smiFixedTable::getColumnName(void *aHeader, UInt aNum)
{
    smiFixedTableHeader *sHeader;

    IDE_ASSERT( aHeader != NULL );
    
    sHeader = (smiFixedTableHeader *)aHeader;

    return sHeader->mDesc->mColDesc[aNum].mName;
}

UInt smiFixedTable::getColumnOffset(void *aHeader, UInt aNum)
{
    smiFixedTableHeader *sHeader;

    IDE_ASSERT( aHeader != NULL );
    
    sHeader = (smiFixedTableHeader *)aHeader;

    return sHeader->mDesc->mColDesc[aNum].mColOffset;
}

UInt smiFixedTable::getColumnLength(void *aHeader, UInt aNum)
{
    smiFixedTableHeader *sHeader;

    IDE_ASSERT( aHeader != NULL );
    
    sHeader = (smiFixedTableHeader *)aHeader;

    return sHeader->mDesc->mColDesc[aNum].mLength;
}

iduFixedTableDataType smiFixedTable::getColumnType(void *aHeader, UInt aNum)
{
    smiFixedTableHeader *sHeader;

    IDE_ASSERT( aHeader != NULL );
    
    sHeader = (smiFixedTableHeader *)aHeader;

    return IDU_FT_GET_TYPE(sHeader->mDesc->mColDesc[aNum].mDataType);
}

void   smiFixedTable::setNullRow(void *aHeader, void *aNullRow)
{
    smiFixedTableHeader *sHeader;

    IDE_ASSERT( aHeader != NULL );
    
    sHeader = (smiFixedTableHeader *)aHeader;

    sHeader->mNullRow = aNullRow;
}

/*
SChar *smiFixedTable::getTableName(void *aHeader)
{
    smiFixedTableHeader *sHeader;

    sHeader = (smiFixedTableHeader *)aHeader;

    return sHeader->mDesc->mTableName;
}
*/

UInt smiFixedTable::convertSCN(void        * /*aBaseObj*/,
                                 void        *aMember,
                                 UChar       *aBuf,
                                 UInt         aBufSize)
{
    SChar  sBuf[30];
    UInt   sSize;
    ULong  sInfiniteMask;
    ULong  sSCNValue;
    smSCN *sOrgSCN;

    sOrgSCN = (smSCN *)aMember;
    sSCNValue = SM_SCN_TO_LONG( *sOrgSCN );

    if (SM_SCN_IS_NOT_INFINITE(*sOrgSCN))
    {
        sSize = idlOS::snprintf(sBuf, ID_SIZEOF(sBuf),
                                "%19"ID_UINT64_FMT,
                                sSCNValue);
    }
    /* BUG-16564:
     * SCN이 INFINITE인 경우 값이 0x8000000000000000일 수 있다.
     * 이 값은 query processor 입장에서는 BIGINT의 NULL 값에 해당한다.
     * 따라서, fixed table에서 SCN 값 출력을 위해 BIGINT 타입을 사용할 경우
     * INFINITE이 NULL로 처리되어버리게 된다.
     * 결과적으로 iSQL에서 INFINITE인 SCN 값을 SELECT해보면,
     * 화면에는 아무런 값도 출력되지 않게 된다.
     * NULL인 SCN을 INFINITE로 이해하라고 사용자에게 강요할 수도 있지만
     * 이는 사용자 친화적이지 않으므로,
     * SCN이 INFINITE인 경우 SCN 컬럼 값을 INFINITE(n) 형태로 출력하도록
     * 코드를 수정한다.
     * 이 때, n은 SCN에서 MSB 1bit(INFINITE bit)을 끈 SCN 값이다.
     *
     * PROJ-1381 Fetch Across Commits
     * SlotHeader Refactoring에 의해서 infinite SCN을
     * [4bytes infinite SCN + 4bytes TID]로 변경했다.
     * 따라서 4Bytes 만큼 shift해서 이전과 동일하게 infinite SCN을 출력한다.*/
    else
    {
        sInfiniteMask = SM_SCN_INFINITE;
        sSize = idlOS::snprintf(sBuf, ID_SIZEOF(sBuf),
                                "INFINITE(%19"ID_UINT64_FMT")",
                                (sSCNValue & (~sInfiniteMask))>>32);
    }

    if (aBufSize < sSize)
    {
        sSize = aBufSize;
    }

    idlOS::memset(aBuf, 0, aBufSize);
    idlOS::memcpy(aBuf, sBuf, sSize);

    return sSize;
}

UInt smiFixedTable::convertTIMESTAMP(void        * /*aBaseObj*/,
                                       void        *aMember,
                                       UChar       *aBuf,
                                       UInt         aBufSize)
{
    struct timeval *sTime;
    UInt            sSize;

    sTime = (struct timeval *)aMember;

    idlOS::memset(aBuf, 0, aBufSize);

    sSize = idlOS::snprintf((SChar *)aBuf, aBufSize,
                            "[%"ID_UINT64_FMT"][%"ID_UINT64_FMT"]",
                           (ULong)sTime->tv_sec,
                           (ULong)sTime->tv_usec);

    return sSize;
}

/* 시간출력시 usec 앞부분을 0으로 채워서 항상 6자리로 찍게 한다. */
UInt smiFixedTable::convertAlignedTIMESTAMP( void       * /*aBaseObj*/,
                                             void       * aMember,
                                             UChar      * aBuf,
                                             UInt         aBufSize )
{
    struct timeval * sTime;
    UInt             sSize;

    sTime = (struct timeval *)aMember;

    idlOS::memset(aBuf, 0, aBufSize);

    sSize = idlOS::snprintf( (SChar *)aBuf, aBufSize,
                             "[%"ID_UINT64_FMT"][%06"ID_UINT64_FMT"]",
                             (ULong)sTime->tv_sec,
                             (ULong)sTime->tv_usec );

    return sSize;
}

UInt smiFixedTable::convertShardPinToString( void       * /*aBaseObj*/,
                                             void       * aMember,
                                             UChar      * aBuf,
                                             UInt         aBufSize )
{
    smiShardPin * sShardPin; 
    UInt          sSize;

    sShardPin = (smiShardPin*)aMember;
    
    sSize = idlOS::snprintf( (SChar *)aBuf, aBufSize,
                             SMI_SHARD_PIN_FORMAT_STR,
                             SMI_SHARD_PIN_FORMAT_ARG( *sShardPin ) );

    return sSize;
}

/* BUG-42639 Monitoring query */
IDE_RC smiFixedTable::build( idvSQL               * aStatistics,
                             void                 * aHeader,
                             void                 * aDumpObject,
                             iduFixedTableMemory  * aFixedMemory,
                             UChar               ** aRecRecord,
                             UChar               ** aTraversePtr )
{
    smcTableHeader         * sTable;
    iduFixedTableBuildFunc   sBuildFunc;
    smiFixedTableRecord    * sEndRecord;

    IDE_ERROR( aHeader      != NULL );
    IDE_ERROR( aFixedMemory != NULL );
    IDE_ERROR( aRecRecord   != NULL );
    IDE_ERROR( aTraversePtr != NULL );

    sTable = (smcTableHeader *)((UChar*)aHeader + SMP_SLOT_HEADER_SIZE);
    sBuildFunc = smiFixedTable::getBuildFunc( sTable );

    // PROJ-1618
    IDE_TEST( sBuildFunc( aStatistics,
                          sTable,
                          aDumpObject,
                          aFixedMemory )
              != IDE_SUCCESS );

    *aRecRecord   = aFixedMemory->getBeginRecord();
    *aTraversePtr = *aRecRecord;

    // Check End of Record
    sEndRecord = (smiFixedTableRecord *)aFixedMemory->getCurrRecord();

    if ( sEndRecord != NULL )
    {
        sEndRecord->mNext = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aStatistics != NULL )
    {
        IDV_SQL_ADD( aStatistics, mMemCursorSeqScan, 1 );
        IDV_SESS_ADD( aStatistics->mSess, IDV_STAT_INDEX_MEM_CURSOR_SEQ_SCAN, 1 );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-42639 Monitoring query */
IDE_RC smiFixedTable::checkFilter( void   * aProperty,
                                   void   * aRecord,
                                   idBool * aResult )
{
    smiFixedTableProperties * sProperty;

    IDE_ERROR( aProperty != NULL );
    IDE_ERROR( aRecord   != NULL );
    IDE_ERROR( aResult   != NULL );

    sProperty = ( smiFixedTableProperties * )aProperty;

    IDE_TEST( sProperty->mFilter->callback( aResult,
                                            getRecordPtr((UChar *)aRecord),
                                            NULL,
                                            0,
                                            SC_NULL_GRID,
                                            sProperty->mFilter->data )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-42639 Monitoring query */
void * smiFixedTable::getRecordPtr( UChar * aCurRec )
{
    return smnfGetRecordPtr( ( void * )aCurRec );
}

/* BUG-42639 Monitoring query */
void smiFixedTable::checkLimitAndMovePos( void   * aProperty,
                                          idBool * aLimitResult,
                                          idBool   aIsMovePos )
{
    smiFixedTableProperties * sProperty;
    idBool                    sFirstLimitResult;
    idBool                    sLastLimitResult;

    sProperty = ( smiFixedTableProperties * )aProperty;

    *aLimitResult = ID_FALSE;
    sFirstLimitResult = ID_FALSE;
    sLastLimitResult = ID_FALSE;

    if ( sProperty->mFirstReadRecordPos <= sProperty->mReadRecordPos )
    {
        sFirstLimitResult = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    if ( ( sProperty->mFirstReadRecordPos + sProperty->mReadRecordCount ) >
         sProperty->mReadRecordPos )
    {
        sLastLimitResult = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sFirstLimitResult == ID_FALSE ) // 읽어야 될 위치보다 앞에 있는 경우.
    {
        /*
         *         first           last
         * ----------*---------------*---------->
         *      ^
         */
    }
    else
    {
        if ( sLastLimitResult == ID_TRUE ) // 읽어야 될 범위 안에 있는 경우.
        {
            /*
             *         first           last
             * ----------*---------------*---------->
             *                 ^
             */
            *aLimitResult = ID_TRUE;
        }
        else // 읽어야 될 위치보다 뒤에 있는 경우.
        {
            /*
             *         first           last
             * ----------*---------------*---------->
             *                                 ^
             */
        }
    }

    if ( aIsMovePos == ID_TRUE )
    {
        sProperty->mReadRecordPos++;
    }
    else
    {
        /* Nothing to do */
    }
}

/* BUG-42639 Monitoring query */
IDE_RC smiFixedTable::checkLastLimit( void   * aProperty,
                                      idBool * aIsLastLimitResult )
{
    smiFixedTableProperties * sProperty;

    IDE_ERROR( aProperty != NULL );
    IDE_ERROR( aIsLastLimitResult   != NULL );

    sProperty = ( smiFixedTableProperties * )aProperty;

    // Session Event를 검사한다.
    IDE_TEST( iduCheckSessionEvent( sProperty->mStatistics ) != IDE_SUCCESS );

    if ( ( sProperty->mFirstReadRecordPos + sProperty->mReadRecordCount ) >
           sProperty->mReadRecordPos )
    {
        *aIsLastLimitResult = ID_FALSE;
    }
    else
    {
        // 읽어야 될 위치보다 뒤에 있는 경우.
        /*
         *         first           last
         * ----------*---------------*---------->
         *                                 ^
         */
        *aIsLastLimitResult = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-42639 Monitoring query */
idBool smiFixedTable::useTrans( void *aHeader )
{
    smiFixedTableHeader * sHeader;
    idBool                sIsUse;

    IDE_ASSERT( aHeader != NULL );

    sHeader = ( smiFixedTableHeader * )aHeader;

    if ( sHeader->mDesc->mUseTrans == IDU_FT_DESC_TRANS_NOT_USE )
    {
        sIsUse = ID_FALSE;
    }
    else
    {
        sIsUse = ID_TRUE;
    }

    return sIsUse;
}

/**
 *  BUG-43006 Fixed Table Indexing Filter
 *
 *  FixedTable에 적용된 Key Range Filter를 검사해서
 *  해당 컬럼이 해당되는지 않되는지 검사한다.
 */
idBool smiFixedTable::checkKeyRange( iduFixedTableMemory   * aMemory,
                                     iduFixedTableColDesc  * aColDesc,
                                     void                 ** aObjects )
{
    idBool                    sResult;
    smiFixedTableProperties * sProperty;
    UChar                     sRowBuf[IDP_MAX_VALUE_LEN];
    UInt                      sOffset1;
    UInt                      sOffset2;
    UChar                   * sObject;
    ULong                     sNullBuf[4];
    void                    * sSourcePtr;
    void                    * sObj;
    iduFixedTableColDesc    * sColDesc;
    iduFixedTableColDesc    * sColIndexDesc;
    UInt                      sCount;
    smiRange                * sRange;

    sResult = ID_TRUE;

    IDE_TEST_CONT( aMemory->useExternalMemory() == ID_FALSE, normal_exit );

    sProperty = ( smiFixedTableProperties * )aMemory->getContext();

    IDE_TEST_CONT( sProperty->mKeyRange == smiGetDefaultKeyRange(), normal_exit );

    if ( ( sProperty->mMaxColumn->offset == 0 ) &&
         ( sProperty->mMaxColumn->size == 0 ) )
    {
        sResult = ID_FALSE;
        IDE_CONT( normal_exit );
    }
    else
    {
        /* Nothing to do */
    }

    for ( sColDesc = aColDesc, sCount = 0, sColIndexDesc = NULL;
          sColDesc->mName != NULL;
          sColDesc++ )
    {
        if ( IDU_FT_IS_COLUMN_INDEX( sColDesc->mDataType ) )
        {
            if ( sColDesc->mColOffset == sProperty->mMaxColumn->offset )
            {
                sColIndexDesc = sColDesc;
                break;
            }
            else
            {
                sCount++;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_TEST_CONT( sColIndexDesc == NULL, normal_exit );

    IDE_TEST_CONT( ( sColIndexDesc->mLength > IDP_MAX_VALUE_LEN ) ||
                   ( sColIndexDesc->mConvCallback != NULL ), normal_exit );

    sObj = aObjects[sCount];

    if ( IDU_FT_IS_POINTER( sColIndexDesc->mDataType ) )
    {
        // de-reference if pointer type..
        sObject = (UChar *)(*((void **)sObj));
    }
    else
    {
        sObject = (UChar *)sObj;
    }

    if ( ( sObject == NULL ) &&
         ( IDU_FT_GET_TYPE( sColIndexDesc->mDataType ) != IDU_FT_TYPE_VARCHAR ) &&
         ( IDU_FT_GET_TYPE( sColIndexDesc->mDataType ) != IDU_FT_TYPE_CHAR ) )
    {
        sNullBuf[0] = 0;
        sNullBuf[1] = 0;
        sNullBuf[2] = 0;
        sNullBuf[3] = 0;

        // 32 byte를 0으로 채우고, memcpy를 위해 사용한다.
        // 현실적으로 32byte 길이의 데이터 타입이 없기 때문에
        // 안전하다고 판단된다.
        IDE_DASSERT(ID_SIZEOF(sNullBuf) >= sColIndexDesc->mLength);
        sSourcePtr  = sNullBuf;
    }
    else
    {
        sSourcePtr = sObject;
    }

    buildOneColumn( sColIndexDesc->mDataType,
                    (UChar *)sObj,
                    sSourcePtr,
                    sColIndexDesc->mLength,
                    sRowBuf,
                    sColIndexDesc->mConvCallback );

    sOffset1 = sProperty->mMinColumn->offset;
    sOffset2 = sProperty->mMaxColumn->offset;
    /* Column 한개만 대상으로 하기때문에 Column offset 을 0 으로
     * 바꿔서 검사해줘야한다
     */
    sProperty->mMinColumn->offset = 0;
    sProperty->mMaxColumn->offset = 0;

    for ( sRange = sProperty->mKeyRange; sRange != NULL; sRange = sRange->next )
    {
        if ( sRange->minimum.callback( &sResult,
                                       sRowBuf,
                                       NULL,
                                       0,
                                       SC_NULL_GRID,
                                       sRange->minimum.data )
             != IDE_SUCCESS )
        {
            sResult = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sResult == ID_TRUE )
        {
            if ( sRange->maximum.callback( &sResult,
                                           sRowBuf,
                                           NULL,
                                           0,
                                           SC_NULL_GRID,
                                           sRange->maximum.data )
                 != IDE_SUCCESS )
            {
                sResult = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }

        if ( sResult == ID_TRUE )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sProperty->mMinColumn->offset = sOffset1;
    sProperty->mMaxColumn->offset = sOffset2;

    IDE_EXCEPTION_CONT( normal_exit );

    return sResult;
}

