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
 * $Id: qmxShard.cpp 88285 2020-08-05 05:49:37Z bethy $
 **********************************************************************/

#include <qmxShard.h>
#include <sdiLob.h>

/*
 * PROJ-2728 Server-side Sharding LOB
 *   qmx의 LobInfo 관련 기능을 Sharding용으로 추가
 */
IDE_RC qmxShard::initializeLobInfo( qcStatement  * aStatement,
                                    qmxLobInfo  ** aLobInfo,
                                    UShort         aSize )
{
    qmxLobInfo  * sLobInfo = NULL;
    UShort        i;

    IDE_TEST( qmx::initializeLobInfo( aStatement,
                                      &sLobInfo,
                                      aSize )
              != IDE_SUCCESS );

    if ( aSize > 0 )
    {
        // BindId for PutLob
        IDE_TEST( aStatement->qmxMem->alloc(
                      ID_SIZEOF(UShort) * aSize,
                      (void**) & sLobInfo->mBindId4PutLob )
                  != IDE_SUCCESS );

        for ( i = 0; i < aSize; i++ )
        {
            sLobInfo->mBindId4PutLob[i] = 0;
        }

        // BindId for OutBind Non-LOB
        IDE_TEST( aStatement->qmxMem->alloc(
                      ID_SIZEOF(UShort) * aSize,
                      (void**) & sLobInfo->mBindId4OutBindNonLob )
                  != IDE_SUCCESS );

        for ( i = 0; i < aSize; i++ )
        {
            sLobInfo->mBindId4OutBindNonLob[i] = 0;
        }
    }
    else
    {
        // Nothing to do.
    }

    *aLobInfo = sLobInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmxShard::initLobInfo( qmxLobInfo  * aLobInfo )
{
    UShort   i;

    (void) qmx::initLobInfo( aLobInfo );

    if ( aLobInfo != NULL )
    {
        aLobInfo->mCount4PutLob = 0;
        aLobInfo->mCount4OutBindNonLob = 0;
        
        for ( i = 0; i < aLobInfo->size; i++ )
        {
            aLobInfo->mBindId4PutLob[i] = 0;
            aLobInfo->mBindId4OutBindNonLob[i] = 0;
        }
    }
    else
    {
        // Nothing to do.
    }
}

void qmxShard::clearLobInfo( qmxLobInfo  * aLobInfo )
{
    UShort   i;

    (void) qmx::clearLobInfo( aLobInfo );

    if ( aLobInfo != NULL )
    {
        aLobInfo->mCount4PutLob = 0;
        aLobInfo->mCount4OutBindNonLob = 0;
        
        for ( i = 0; i < aLobInfo->size; i++ )
        {
            aLobInfo->mBindId4PutLob[i] = 0;
            aLobInfo->mBindId4OutBindNonLob[i] = 0;
        }
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC qmxShard::addLobInfoForCopy( qmxLobInfo   * aLobInfo,
                                    smLobLocator   aLocator,
                                    UShort         aBindId )
{
    // BUG-38188 instead of trigger에서 aLobInfo가 NULL일 수 있다.
    if ( aLobInfo != NULL )
    {
        IDE_ASSERT( aLobInfo->count < aLobInfo->size );

        /* BUG-44022 CREATE AS SELECT로 TABLE 생성할 때에 OUTER JOIN과 LOB를 같이 사용하면 에러가 발생 */
        if ( ( aLocator == MTD_LOCATOR_NULL )
             ||
             ( SMI_IS_NULL_LOCATOR( aLocator ) ) )
        {
            // Nothing To Do
        }
        else
        {
            aLobInfo->column[aLobInfo->count] = NULL;
            aLobInfo->locator[aLobInfo->count] = aLocator;
            aLobInfo->dstBindId[aLobInfo->count] = aBindId;
            aLobInfo->count++;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC qmxShard::addLobInfoForOutBind( qmxLobInfo   * aLobInfo,
                                       UShort         aBindId )
{
    IDE_ASSERT( aLobInfo != NULL );
    IDE_ASSERT( aLobInfo->outCount < aLobInfo->size );

    aLobInfo->outColumn[aLobInfo->outCount] = NULL;
    aLobInfo->outBindId[aLobInfo->outCount] = aBindId;
    aLobInfo->outCount++;

    return IDE_SUCCESS;
}

IDE_RC qmxShard::addLobInfoForPutLob( qmxLobInfo   * aLobInfo,
                                      UShort         aBindId )
{
    IDE_ASSERT( aLobInfo != NULL );
    IDE_ASSERT( aLobInfo->mCount4PutLob < aLobInfo->size );

    aLobInfo->mBindId4PutLob[aLobInfo->mCount4PutLob] = aBindId;
    aLobInfo->mCount4PutLob++;

    return IDE_SUCCESS;
}

IDE_RC qmxShard::addLobInfoForOutBindNonLob( qmxLobInfo   * aLobInfo,
                                             UShort         aBindId )
{
    IDE_ASSERT( aLobInfo != NULL );
    IDE_ASSERT( aLobInfo->mCount4OutBindNonLob < aLobInfo->size );

    aLobInfo->mBindId4OutBindNonLob[aLobInfo->mCount4OutBindNonLob] = aBindId;
    aLobInfo->mCount4OutBindNonLob++;

    return IDE_SUCCESS;
}

IDE_RC qmxShard::copyChar2Lob( mtdCharType    * aCharValue,
                               mtdLobType     * aLobValue,
                               UInt             aDataSize )
{
    /* empty 와 NULL 구분이 안 되므로 무조건 NULL로 */
    if ( aCharValue->length == 0 )
    {
        aLobValue->length = MTD_LOB_NULL_LENGTH;
    }
    else
    {
        aLobValue->length = aCharValue->length;
    }

    if ( aCharValue->length > 0 )
    {
        IDE_TEST( MTD_LOB_TYPE_STRUCT_SIZE(aLobValue->length) > aDataSize );

        idlOS::memcpy( aLobValue->value,
                       aCharValue->value,
                       aCharValue->length );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_TEST( MTD_LOB_TYPE_STRUCT_SIZE(aLobValue->length) > aDataSize );

    return IDE_FAILURE;
}

IDE_RC qmxShard::copyBinary2Lob( mtdBinaryType  * aBinaryValue,
                                 mtdLobType     * aLobValue,
                                 UInt             aDataSize )
{
    /* empty 와 NULL 구분이 안 되므로 무조건 NULL로 */
    if ( aBinaryValue->mLength == 0 )
    {
        aLobValue->length = MTD_LOB_NULL_LENGTH;
    }
    else
    {
        aLobValue->length = aBinaryValue->mLength;
    }

    if ( aBinaryValue->mLength > 0 )
    {
        IDE_TEST( MTD_LOB_TYPE_STRUCT_SIZE(aLobValue->length) > aDataSize );

        idlOS::memcpy( aLobValue->value,
                       aBinaryValue->mValue,
                       aBinaryValue->mLength );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );

    return IDE_FAILURE;
}

/* Copied from qmx::copyAndOutBindLobInfo */
IDE_RC qmxShard::copyAndOutBindLobInfo( qcStatement    * aStatement,
                                        sdiConnectInfo * aConnectInfo,
                                        UShort           aIndex,
                                        qmxLobInfo     * aLobInfo,
                                        sdiDataNode    * aDataNode )
{
    smLobLocator       sLocator = MTD_LOCATOR_NULL;
    smLobLocator       sTempLocator;
    qcTemplate       * sTemplate;
    mtdLobType       * sLobValue;
    void             * sOutDataBuf = NULL;
    UShort             sBindTuple;
    UShort             sBindId;
    UInt               sInfo = 0;
    UInt               sMmSessId;
    UInt               sMmStmtId;
    UInt               i;

    IDE_TEST_CONT( aLobInfo == NULL, NORMAL_EXIT );

    sInfo = MTC_LOB_LOCATOR_CLOSE_TRUE;

    sMmSessId = qci::mSessionCallback.mGetSessionID( aStatement->session->mMmSession );
    sMmStmtId = qci::mSessionCallback.mGetStmtId( QC_MM_STMT( aStatement ) );

    // for copy
    for ( i = 0;
          i < aLobInfo->count;
          i++ )
    {
        sBindId = aLobInfo->dstBindId[i];

        if ( aDataNode->mOutBindParams[sBindId].mShadowData != NULL )
        {
            sOutDataBuf = SDI_GET_SHADOW_DATA( aDataNode, aIndex, sBindId );

            IDE_TEST( smiLob::openShardLobCursor(
                          (QC_SMI_STMT(aStatement))->getTrans(),
                          sMmSessId,
                          sMmStmtId,
                          aDataNode->mRemoteStmt->mRemoteStmtId,
                          aConnectInfo->mNodeId,
                          aDataNode->mBindParams[sBindId].mType,
                          *(smLobLocator *) sOutDataBuf,
                          sInfo,
                          SMI_LOB_TABLE_CURSOR_MODE,
                          & sLocator )
                      != IDE_SUCCESS );

            IDE_TEST( smiLob::copy( aStatement->mStatistics,
                                    sLocator,
                                    aLobInfo->locator[i] )
                      != IDE_SUCCESS );

            sTempLocator = sLocator;
            sLocator = MTD_LOCATOR_NULL;
            IDE_TEST( qmx::closeLobLocator( aStatement->mStatistics,
                                            sTempLocator )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        /* BUG-30351
         * insert into select에서 각 Row Insert 후 해당 Lob Cursor를 바로 해제했으면 합니다.
         * ==>
         * For Sharing,
         *   여러 DataNode들에 대해 수행될 수 있기 때문에,
         *   Src. Lob Locator를 여기서 닫지 않고 모든 대상 노드로의 수행이 완료된 후
         *   qmxShard::closeLobLocatorForCopy에서 수행함.
         */
    }

    sInfo = MTC_LOB_COLUMN_NOTNULL_FALSE;

    // for outbind
    for ( i = 0;
          i < aLobInfo->outCount;
          i++ )
    {
        sBindId = aLobInfo->outBindId[i];

        if ( aDataNode->mOutBindParams[sBindId].mShadowData != NULL )
        {
            sOutDataBuf = SDI_GET_SHADOW_DATA( aDataNode, aIndex, sBindId );

            IDE_TEST( smiLob::openShardLobCursor(
                          (QC_SMI_STMT(aStatement))->getTrans(),
                          sMmSessId,
                          sMmStmtId,
                          aDataNode->mRemoteStmt->mRemoteStmtId,
                          aConnectInfo->mNodeId,
                          aDataNode->mBindParams[sBindId].mType,
                          *(smLobLocator *) sOutDataBuf,
                          sInfo,
                          SMI_LOB_TABLE_CURSOR_MODE,
                          & sLocator )
                      != IDE_SUCCESS );

            // locator가 out-bound 되었을 때
            // getParamData시 첫번째 locator를 넘겨주기 위해
            // 첫번째 locator들을 bind-tuple에 저장한다.
            if ( aLobInfo->outFirst == ID_TRUE )
            {
                sTemplate = QC_PRIVATE_TMPLATE(aStatement);
                sBindTuple = sTemplate->tmplate.variableRow;

                IDE_DASSERT( sBindTuple != ID_USHORT_MAX );

                idlOS::memcpy( (SChar*) sTemplate->tmplate.rows[sBindTuple].row
                               + sTemplate->tmplate.rows[sBindTuple].
                           columns[sBindId].column.offset,
                           & sLocator,
                           ID_SIZEOF(sLocator) );

                // 첫번째 locator들을 따로 저장한다.
                // mm에서 locator-list의 hash function에 사용한다.
                aLobInfo->outFirstLocator[i] = sLocator;
            }
            else
            {
                // Nothing to do.
            }

            aLobInfo->outLocator[i] = sLocator;
            sLocator = MTD_LOCATOR_NULL;
        }
        else
        {
            // Nothing to do.
        }
    }

    aLobInfo->outFirst = ID_FALSE;

    if ( aLobInfo->outCount > 0 )
    {
        IDE_TEST( qci::mOutBindLobCallback(
                      aStatement->session->mMmSession,
                      aLobInfo->outLocator,
                      aLobInfo->outFirstLocator,
                      aLobInfo->outCount )
                  != IDE_SUCCESS );

        // callback을 호출했음을 표시
        aLobInfo->outCallback = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    // for putLob
    for ( i = 0;
          i < aLobInfo->mCount4PutLob;
          i++ )
    {
        sBindId = aLobInfo->mBindId4PutLob[i];

        if ( aDataNode->mOutBindParams[sBindId].mShadowData != NULL )
        {
            sOutDataBuf = SDI_GET_SHADOW_DATA( aDataNode, aIndex, sBindId );

            sLocator = *((smLobLocator *)sOutDataBuf);
            sLobValue = (mtdLobType *)aDataNode->mBindParams[sBindId].mData;

            IDE_TEST( sdiLob::put( aConnectInfo,
                         aDataNode->mRemoteStmt,
                         aDataNode->mBindParams[sBindId].mType,
                         sLocator,
                         sLobValue->value,
                         sLobValue->length ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    // for outBind Non-LOB
    for ( i = 0;
          i < aLobInfo->mCount4OutBindNonLob;
          i++ )
    {
        sBindId = aLobInfo->mBindId4OutBindNonLob[i];

        if ( aDataNode->mOutBindParams[sBindId].mShadowData != NULL )
        {
            sOutDataBuf = SDI_GET_SHADOW_DATA( aDataNode, aIndex, sBindId );

            if ( aDataNode->mBindParams[sBindId].mType == MTD_VARCHAR_ID )
            {
                IDE_TEST( copyChar2Lob(
                            (mtdCharType *)sOutDataBuf,
                            (mtdLobType *)aDataNode->mBindParams[sBindId].mData,
                            aDataNode->mBindParams[sBindId].mDataSize )
                          != IDE_SUCCESS );
            }
            else if ( aDataNode->mBindParams[sBindId].mType == MTD_BINARY_ID )
            {
                IDE_TEST( copyBinary2Lob(
                            (mtdBinaryType *)sOutDataBuf,
                            (mtdLobType *)aDataNode->mBindParams[sBindId].mData,
                            aDataNode->mBindParams[sBindId].mDataSize )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_EXCEPTION_CONT(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void) qmx::closeLobLocator( aStatement->mStatistics, sLocator );

    for ( i = 0;
          i < aLobInfo->outCount;
          i++ )
    {
        (void) qmx::closeLobLocator( aStatement->mStatistics, aLobInfo->outLocator[i] );
        aLobInfo->outLocator[i] = MTD_LOCATOR_NULL;
    }

    return IDE_FAILURE;
}

IDE_RC qmxShard::closeLobLocatorForCopy( idvSQL         * aStatistics,
                                         qmxLobInfo     * aLobInfo )
{
    smLobLocator     sTempLocator;
    idBool           sSuccess = ID_TRUE;
    SInt             i;

    if ( aLobInfo != NULL )
    {
        for ( i = 0;
              i < aLobInfo->count;
              i++ )
        {
            /* BUG-30351
             * insert into select에서 각 Row Insert 후 해당 Lob Cursor를 바로 해제했으면 합니다.
             */
            sTempLocator = aLobInfo->locator[i];
            aLobInfo->locator[i] = MTD_LOCATOR_NULL;

            if( aLobInfo->mImmediateClose == ID_TRUE )
            {
                if( ( sTempLocator == MTD_LOCATOR_NULL )
                    ||
                    ( SMI_IS_NULL_LOCATOR( sTempLocator )) )
                {
                    // nothing todo
                }
                else
                {
                    if ( smiLob::closeLobCursor( aStatistics, sTempLocator )
                         != IDE_SUCCESS )
                    {
                        sSuccess = ID_FALSE;
                    }
                }
            }
            else
            {
                if ( qmx::closeLobLocator( aStatistics, sTempLocator )
                     != IDE_SUCCESS )
                {
                    sSuccess = ID_FALSE;
                }
            }
        }
    }

    IDE_TEST( sSuccess == ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
