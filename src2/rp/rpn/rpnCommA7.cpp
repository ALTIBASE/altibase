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
 * $Id: rpnComm.cpp 16602 2006-06-09 01:39:30Z lswhh $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <rpDef.h>
#include <rpuProperty.h>
#include <rpcHBT.h>
#include <rpnComm.h>
#include <rpdCatalog.h>

static void compactCIDAndColumnValueArray( UInt     * aCIDs,
                                           smiValue * aBCols,
                                           smiValue * aACols,
                                           UInt       aDataCount,
                                           UInt       aColumnCount )
{
    UInt        i = 0;
    UInt        sNextColumnIndex = 0;

    i = 0;
    while ( ( i < aColumnCount ) && ( sNextColumnIndex < aDataCount ) )
    {
        if ( aCIDs[i] != RP_INVALID_COLUMN_ID )
        {
            if ( i == sNextColumnIndex )
            {
                /* do nothing */
            }
            else
            {
                aCIDs[sNextColumnIndex] = aCIDs[i];
                aCIDs[i] =RP_INVALID_COLUMN_ID;

                aBCols[sNextColumnIndex].length = aBCols[i].length;
                aBCols[sNextColumnIndex].value = aBCols[i].value;
                aBCols[i].length = 0;
                aBCols[i].value = NULL;

                aACols[sNextColumnIndex].length = aACols[i].length;
                aACols[sNextColumnIndex].value = aACols[i].value;
                aACols[i].length = 0;
                aACols[i].value = NULL;
            }

            sNextColumnIndex++;
        }
        else
        {
            /* do nothing */
        }

        i++;
    }
}


IDE_RC rpnComm::sendVersionA7( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               rpdVersion         * aReplVersion,
                               UInt                 aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 8, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Version Set and Send */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Version );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR8( aProtocolContext, &(aReplVersion->mVersion) );

    IDU_FIT_POINT( "rpnComm::sendVersion::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvVersionA7( cmiProtocolContext * aProtocolContext,
                               idBool             * /* aExitFlag */,
                               rpdVersion         * aReplVersion )
{
    UChar sOpCode;
    
    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, Version ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Version Number Set */
    CMI_RD8( aProtocolContext, &(aReplVersion->mVersion) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendMetaDictTableCountA7( void               * aHBTResource,
                                          cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          SInt               * aDictTableCount,
                                          UInt                 aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Validate Parameters */
    IDE_DASSERT( aDictTableCount != NULL );

    IDE_TEST( checkAndFlush( NULL, /* aHBTResource */
                             aProtocolContext,
                             aExitFlag,
                             1 + 4,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaDictTableCount );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt *)aDictTableCount );

    IDU_FIT_POINT( "rpnComm::sendMetaDictTableCount::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaPartitionCountA7( void               * aHBTResource,
                                          cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt               * aCount,
                                          UInt                 aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Validate Parameters */
    IDE_DASSERT( aCount != NULL );

    IDE_TEST( checkAndFlush( NULL, /* aHBTResource */
                             aProtocolContext,
                             aExitFlag,
                             1 + 4,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaPartitionCount );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt *)aCount );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaPartitionCountA7( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt               * aCount,
                                          ULong                aTimeoutSec )
{
    UChar   sOpCode;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */, 
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    //CMP_OP_RP_MetaPartitionCount
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaPartitionCount ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, (UInt*)aCount );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}
IDE_RC rpnComm::recvMetaDictTableCountA7( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          SInt               * aDictTableCount,
                                          ULong                aTimeoutSec )
{
    UChar   sOpCode;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */, 
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaDictTableCount ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, (UInt*)aDictTableCount );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}


IDE_RC rpnComm::sendMetaReplA7( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                rpdReplications    * aRepl,
                                UInt                 aTimeoutSec )
{
    UInt sRepNameLen;
    UInt sOSInfoLen;
    UInt sCharSetLen;
    UInt sNationalCharSetLen;
    UInt sServerIDLen;
    UInt sRemoteFaultDetectTimeLen;
	
    ULong sDeprecated = -1;

    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Validate Parameters */
    IDE_DASSERT( aRepl != NULL );

    sRepNameLen = idlOS::strlen( aRepl->mRepName );
    sOSInfoLen = idlOS::strlen( aRepl->mOSInfo );
    sCharSetLen = idlOS::strlen( aRepl->mDBCharSet );
    sNationalCharSetLen = idlOS::strlen( aRepl->mNationalCharSet );
    sServerIDLen = idlOS::strlen( aRepl->mServerID );
    sRemoteFaultDetectTimeLen = idlOS::strlen( aRepl->mRemoteFaultDetectTime );
   
    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 100 + sRepNameLen + sOSInfoLen  + sCharSetLen +
                             sNationalCharSetLen + sServerIDLen + sRemoteFaultDetectTimeLen,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaRepl );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &sRepNameLen );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mRepName, sRepNameLen );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mItemCount) );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mRole) );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mConflictResolution) );
    CMI_WR4( aProtocolContext, &(aRepl->mTransTblSize) );
    CMI_WR4( aProtocolContext, &(aRepl->mFlags) );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mOptions) );
    CMI_WR8( aProtocolContext, &(aRepl->mRPRecoverySN) );
    CMI_WR4( aProtocolContext, &(aRepl->mReplMode) );
    CMI_WR4( aProtocolContext, &(aRepl->mParallelID) );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mIsStarted) );
    /* PROJ-1915 OS, Endian, SMVersion, LFGCount, LogFileSize */
    CMI_WR4( aProtocolContext, &(sOSInfoLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mOSInfo, sOSInfoLen );
    CMI_WR4( aProtocolContext, &(aRepl->mCompileBit) );
    CMI_WR4( aProtocolContext, &(aRepl->mSmVersionID) );
    CMI_WR4( aProtocolContext, &(aRepl->mLFGCount) );
    CMI_WR8( aProtocolContext, &(aRepl->mLogFileSize) );
    CMI_WR8( aProtocolContext, &( sDeprecated ) );
    /* CharSet Set */
    CMI_WR4( aProtocolContext, &(sCharSetLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mDBCharSet, sCharSetLen );
    CMI_WR4( aProtocolContext, &(sNationalCharSetLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mNationalCharSet, sNationalCharSetLen );
    /* Server ID */
    CMI_WR4( aProtocolContext, &(sServerIDLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mServerID, sServerIDLen );
    CMI_WR4( aProtocolContext, &(sRemoteFaultDetectTimeLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mRemoteFaultDetectTime, sRemoteFaultDetectTimeLen );

    if ( rpdMeta::needToProcessProtocolOperation( RP_META_COMPRESSTYPE,
                                                  aRepl->mRemoteVersion )
         == ID_TRUE )
    {
        CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mCompressType) );
    }
    
    IDU_FIT_POINT( "rpnComm::sendMetaRepl::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplA7( cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                rpdReplications    * aRepl,
                                ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt sRepNameLen;
    UInt sOSInfoLen;
    UInt sCharSetLen;
    UInt sNationalCharSetLen;
    UInt sServerIDLen;
    UInt sRemoteFaultDetectTimeLen;

    ULong sDeprecated = 0;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaRepl ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Information */
    CMI_RD4( aProtocolContext, &(sRepNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mRepName, sRepNameLen );
    aRepl->mRepName[sRepNameLen] = '\0';
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mItemCount) );
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mRole) );
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mConflictResolution) );
    CMI_RD4( aProtocolContext, &(aRepl->mTransTblSize) );
    CMI_RD4( aProtocolContext, &(aRepl->mFlags) );
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mOptions) );
    CMI_RD8( aProtocolContext, &(aRepl->mRPRecoverySN) );
    CMI_RD4( aProtocolContext, &(aRepl->mReplMode) );
    CMI_RD4( aProtocolContext, &(aRepl->mParallelID) );
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mIsStarted) );
    /* PROJ-1915 OS, Endian, SMVersion, LFGCount, LogFileSize */
    CMI_RD4( aProtocolContext, &(sOSInfoLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mOSInfo, sOSInfoLen );
    aRepl->mOSInfo[sOSInfoLen] = '\0';
    CMI_RD4( aProtocolContext, &(aRepl->mCompileBit) );
    CMI_RD4( aProtocolContext, &(aRepl->mSmVersionID) );
    CMI_RD4( aProtocolContext, &(aRepl->mLFGCount) );
    CMI_RD8( aProtocolContext, &(aRepl->mLogFileSize) );
    CMI_RD8( aProtocolContext, &( sDeprecated ) );
    /* CharSet Get */
    CMI_RD4( aProtocolContext, &(sCharSetLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mDBCharSet, sCharSetLen );
    aRepl->mDBCharSet[sCharSetLen] = '\0';
    CMI_RD4( aProtocolContext, &(sNationalCharSetLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mNationalCharSet, sNationalCharSetLen );
    aRepl->mNationalCharSet[sNationalCharSetLen] = '\0';
    /* Server ID */
    CMI_RD4( aProtocolContext, &(sServerIDLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mServerID, sServerIDLen );
    aRepl->mServerID[sServerIDLen] = '\0';
    CMI_RD4( aProtocolContext, &(sRemoteFaultDetectTimeLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mRemoteFaultDetectTime, sRemoteFaultDetectTimeLen );
    aRepl->mRemoteFaultDetectTime[sRemoteFaultDetectTimeLen] = '\0';
    
    if ( rpdMeta::needToProcessProtocolOperation( RP_META_COMPRESSTYPE,
                                                  aRepl->mRemoteVersion )
         == ID_TRUE )
    {
        CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mCompressType) );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendTempSyncMetaReplA7( void               * aHBTResource,
                                        cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        rpdReplications    * aRepl,
                                        UInt                 aTimeoutSec )
{
    UInt sRepNameLen;
    UInt sOSInfoLen;
    UInt sCharSetLen;
    UInt sNationalCharSetLen;
    UInt sServerIDLen;
    UInt sRemoteFaultDetectTimeLen;
	
    ULong sDeprecated = -1;

    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Validate Parameters */
    IDE_DASSERT( aRepl != NULL );

    sRepNameLen = idlOS::strlen( aRepl->mRepName );
    sOSInfoLen = idlOS::strlen( aRepl->mOSInfo );
    sCharSetLen = idlOS::strlen( aRepl->mDBCharSet );
    sNationalCharSetLen = idlOS::strlen( aRepl->mNationalCharSet );
    sServerIDLen = idlOS::strlen( aRepl->mServerID );
    sRemoteFaultDetectTimeLen = idlOS::strlen( aRepl->mRemoteFaultDetectTime );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 96 + sRepNameLen + sOSInfoLen  + sCharSetLen +
                             sNationalCharSetLen + sServerIDLen + sRemoteFaultDetectTimeLen, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, TemporarySyncInfo );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &sRepNameLen );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mRepName, sRepNameLen );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mItemCount) );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mRole) );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mConflictResolution) );
    CMI_WR4( aProtocolContext, &(aRepl->mTransTblSize) );
    CMI_WR4( aProtocolContext, &(aRepl->mFlags) );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mOptions) );
    CMI_WR8( aProtocolContext, &(aRepl->mRPRecoverySN) );
    CMI_WR4( aProtocolContext, &(aRepl->mReplMode) );
    CMI_WR4( aProtocolContext, &(aRepl->mParallelID) );
    CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mIsStarted) );
    /* PROJ-1915 OS, Endian, SMVersion, LFGCount, LogFileSize */
    CMI_WR4( aProtocolContext, &(sOSInfoLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mOSInfo, sOSInfoLen );
    CMI_WR4( aProtocolContext, &(aRepl->mCompileBit) );
    CMI_WR4( aProtocolContext, &(aRepl->mSmVersionID) );
    CMI_WR4( aProtocolContext, &(aRepl->mLFGCount) );
    CMI_WR8( aProtocolContext, &(aRepl->mLogFileSize) );
    CMI_WR8( aProtocolContext, &( sDeprecated ) );
    /* CharSet Set */
    CMI_WR4( aProtocolContext, &(sCharSetLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mDBCharSet, sCharSetLen );
    CMI_WR4( aProtocolContext, &(sNationalCharSetLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mNationalCharSet, sNationalCharSetLen );
    /* Server ID */
    CMI_WR4( aProtocolContext, &(sServerIDLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mServerID, sServerIDLen );
    CMI_WR4( aProtocolContext, &(sRemoteFaultDetectTimeLen) );
    CMI_WCP( aProtocolContext, (UChar *)aRepl->mRemoteFaultDetectTime, sRemoteFaultDetectTimeLen );
    
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTempSyncMetaReplA7( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        rpdReplications    * aRepl,
                                        ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt sRepNameLen;
    UInt sOSInfoLen;
    UInt sCharSetLen;
    UInt sNationalCharSetLen;
    UInt sServerIDLen;
    UInt sRemoteFaultDetectTimeLen;

    ULong sDeprecated = 0;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, TemporarySyncInfo ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Information */
    CMI_RD4( aProtocolContext, &(sRepNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mRepName, sRepNameLen );
    aRepl->mRepName[sRepNameLen] = '\0';
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mItemCount) );
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mRole) );
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mConflictResolution) );
    CMI_RD4( aProtocolContext, &(aRepl->mTransTblSize) );
    CMI_RD4( aProtocolContext, &(aRepl->mFlags) );
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mOptions) );
    CMI_RD8( aProtocolContext, &(aRepl->mRPRecoverySN) );
    CMI_RD4( aProtocolContext, &(aRepl->mReplMode) );
    CMI_RD4( aProtocolContext, &(aRepl->mParallelID) );
    CMI_RD4( aProtocolContext, (UInt*)&(aRepl->mIsStarted) );
    /* PROJ-1915 OS, Endian, SMVersion, LFGCount, LogFileSize */
    CMI_RD4( aProtocolContext, &(sOSInfoLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mOSInfo, sOSInfoLen );
    aRepl->mOSInfo[sOSInfoLen] = '\0';
    CMI_RD4( aProtocolContext, &(aRepl->mCompileBit) );
    CMI_RD4( aProtocolContext, &(aRepl->mSmVersionID) );
    CMI_RD4( aProtocolContext, &(aRepl->mLFGCount) );
    CMI_RD8( aProtocolContext, &(aRepl->mLogFileSize) );
    CMI_RD8( aProtocolContext, &( sDeprecated ) );
    /* CharSet Get */
    CMI_RD4( aProtocolContext, &(sCharSetLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mDBCharSet, sCharSetLen );
    aRepl->mDBCharSet[sCharSetLen] = '\0';
    CMI_RD4( aProtocolContext, &(sNationalCharSetLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mNationalCharSet, sNationalCharSetLen );
    aRepl->mNationalCharSet[sNationalCharSetLen] = '\0';
    /* Server ID */
    CMI_RD4( aProtocolContext, &(sServerIDLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mServerID, sServerIDLen );
    aRepl->mServerID[sServerIDLen] = '\0';
    CMI_RD4( aProtocolContext, &(sRemoteFaultDetectTimeLen) );
    CMI_RCP( aProtocolContext, (UChar *)aRepl->mRemoteFaultDetectTime, sRemoteFaultDetectTimeLen );
    aRepl->mRemoteFaultDetectTime[sRemoteFaultDetectTimeLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}
IDE_RC rpnComm::sendTempSyncReplItemA7( void               * aHBTResource,
                                        cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        rpdReplSyncItem    * aTempSyncItem,
                                        UInt                 aTimeoutSec )
{
    UInt sUserNameLen;
    UInt sTableNameLen;
    UInt sPartNameLen;
    UInt sRepUnitLen;

    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Validate Parameters */
    IDE_DASSERT( aTempSyncItem != NULL );

    sUserNameLen = idlOS::strlen( aTempSyncItem->mUserName ); 
    sTableNameLen = idlOS::strlen( aTempSyncItem->mTableName ); 
    sPartNameLen = idlOS::strlen( aTempSyncItem->mPartitionName ); 
    sRepUnitLen = idlOS::strlen( aTempSyncItem->mReplUnit ); 

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4 + sUserNameLen +
                             4 + sTableNameLen +
                             4 + sPartNameLen +
                             4 + sRepUnitLen + 8,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, TemporarySyncItem );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &sUserNameLen );
    CMI_WCP( aProtocolContext, (UChar *)aTempSyncItem->mUserName, sUserNameLen );
    CMI_WR4( aProtocolContext, &sTableNameLen );
    CMI_WCP( aProtocolContext, (UChar *)aTempSyncItem->mTableName, sTableNameLen );
    CMI_WR4( aProtocolContext, &sPartNameLen );
    CMI_WCP( aProtocolContext, (UChar *)aTempSyncItem->mPartitionName, sPartNameLen );
    CMI_WR4( aProtocolContext, &sRepUnitLen );
    CMI_WCP( aProtocolContext, (UChar *)aTempSyncItem->mReplUnit, sRepUnitLen );
    CMI_WR8( aProtocolContext, &(aTempSyncItem->mTableOID) );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTempSyncReplItemA7( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        rpdReplSyncItem    * aTempSyncItem,
                                        ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt sUserNameLen;
    UInt sTableNameLen;
    UInt sPartNameLen;
    UInt sRepUnitLen;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, TemporarySyncItem ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, &(sUserNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aTempSyncItem->mUserName, sUserNameLen );
    aTempSyncItem->mUserName[sUserNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sTableNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aTempSyncItem->mTableName, sTableNameLen );
    aTempSyncItem->mTableName[sTableNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sPartNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aTempSyncItem->mPartitionName, sPartNameLen );
    aTempSyncItem->mPartitionName[sPartNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sRepUnitLen) );
    CMI_RCP( aProtocolContext, (UChar *)aTempSyncItem->mReplUnit, sRepUnitLen );
    aTempSyncItem->mReplUnit[sRepUnitLen] = '\0';
    CMI_RD8( aProtocolContext, &(aTempSyncItem->mTableOID) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendMetaReplTblA7( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdMetaItem        * aItem,
                                   UInt                 aTimeoutSec )
{
    UInt sRepNameLen;
    UInt sLocalUserNameLen;
    UInt sLocalTableNameLen;
    UInt sLocalPartNameLen;
    UInt sRemoteUserNameLen;
    UInt sRemoteTableNameLen;
    UInt sRemotePartNameLen;
    UInt sPartCondMinValuesLen;
    UInt sPartCondMaxValuesLen;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    /* Validate Parameters */
    IDE_DASSERT( aItem != NULL );

    sRepNameLen = idlOS::strlen( aItem->mItem.mRepName );
    sLocalUserNameLen = idlOS::strlen( aItem->mItem.mLocalUsername );
    sLocalTableNameLen = idlOS::strlen( aItem->mItem.mLocalTablename );
    sLocalPartNameLen = idlOS::strlen( aItem->mItem.mLocalPartname );
    sRemoteUserNameLen = idlOS::strlen( aItem->mItem.mRemoteUsername );
    sRemoteTableNameLen = idlOS::strlen( aItem->mItem.mRemoteTablename );
    sRemotePartNameLen = idlOS::strlen( aItem->mItem.mRemotePartname );
    sPartCondMinValuesLen  = idlOS::strlen( aItem->mPartCondMinValues );
    sPartCondMaxValuesLen  = idlOS::strlen( aItem->mPartCondMaxValues );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 80 + sRepNameLen + sLocalUserNameLen + sLocalTableNameLen +
                             sLocalPartNameLen + sRemoteUserNameLen + sRemoteTableNameLen +
                             sRemotePartNameLen + sPartCondMinValuesLen + sPartCondMaxValuesLen,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Item Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaReplTbl );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sRepNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aItem->mItem.mRepName, sRepNameLen );
    CMI_WR4( aProtocolContext, &(sLocalUserNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aItem->mItem.mLocalUsername, sLocalUserNameLen );
    CMI_WR4( aProtocolContext, &(sLocalTableNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aItem->mItem.mLocalTablename, sLocalTableNameLen );
    CMI_WR4( aProtocolContext, &(sLocalPartNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aItem->mItem.mLocalPartname, sLocalPartNameLen );
    CMI_WR4( aProtocolContext, &(sRemoteUserNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aItem->mItem.mRemoteUsername, sRemoteUserNameLen );
    CMI_WR4( aProtocolContext, &(sRemoteTableNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aItem->mItem.mRemoteTablename, sRemoteTableNameLen );
    CMI_WR4( aProtocolContext, &(sRemotePartNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aItem->mItem.mRemotePartname, sRemotePartNameLen );
    CMI_WR4( aProtocolContext, &(sPartCondMinValuesLen) );
    CMI_WCP( aProtocolContext, (UChar *)aItem->mPartCondMinValues, sPartCondMinValuesLen );
    CMI_WR4( aProtocolContext, &(sPartCondMaxValuesLen) );
    CMI_WCP( aProtocolContext, (UChar *)aItem->mPartCondMaxValues, sPartCondMaxValuesLen );
    CMI_WR4( aProtocolContext, &(aItem->mPartitionMethod) );
    CMI_WR4( aProtocolContext, &(aItem->mPartitionOrder) );
    CMI_WR8( aProtocolContext, &(aItem->mItem.mTableOID) );
    CMI_WR4( aProtocolContext, &(aItem->mPKIndex.indexId) );
    CMI_WR4( aProtocolContext, (UInt*)&(aItem->mPKColCount) );
    CMI_WR4( aProtocolContext, (UInt*)&(aItem->mColCount) );
    CMI_WR4( aProtocolContext, (UInt*)&(aItem->mIndexCount) );
    /* PROJ-1915 Invalid Max SN을 전송 한다. */
    CMI_WR8( aProtocolContext, &(aItem->mItem.mInvalidMaxSN) );

    /* BUG-34360 Check Constraint */
    CMI_WR4( aProtocolContext, (UInt*)&(aItem->mCheckCount) );

    IDU_FIT_POINT( "rpnComm::sendMetaReplTbl::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplTblA7( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdMetaItem        * aItem,
                                   ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt sRepNameLen;
    UInt sLocalUserNameLen;
    UInt sLocalTableNameLen;
    UInt sLocalPartNameLen;
    UInt sRemoteUserNameLen;
    UInt sRemoteTableNameLen;
    UInt sRemotePartNameLen;
    UInt sPartCondMinValuesLen;
    UInt sPartCondMaxValuesLen;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */, 
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplTbl ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Item Information */
    CMI_RD4( aProtocolContext, &(sRepNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mRepName, sRepNameLen );
    aItem->mItem.mRepName[sRepNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sLocalUserNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mLocalUsername, sLocalUserNameLen );
    aItem->mItem.mLocalUsername[sLocalUserNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sLocalTableNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mLocalTablename, sLocalTableNameLen );
    aItem->mItem.mLocalTablename[sLocalTableNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sLocalPartNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mLocalPartname, sLocalPartNameLen );
    aItem->mItem.mLocalPartname[sLocalPartNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sRemoteUserNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mRemoteUsername, sRemoteUserNameLen );
    aItem->mItem.mRemoteUsername[sRemoteUserNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sRemoteTableNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mRemoteTablename, sRemoteTableNameLen );
    aItem->mItem.mRemoteTablename[sRemoteTableNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sRemotePartNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mRemotePartname, sRemotePartNameLen );
    aItem->mItem.mRemotePartname[sRemotePartNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sPartCondMinValuesLen) );
    CMI_RCP( aProtocolContext, (UChar *)aItem->mPartCondMinValues, sPartCondMinValuesLen );
    aItem->mPartCondMinValues[sPartCondMinValuesLen] = '\0';
    CMI_RD4( aProtocolContext, &(sPartCondMaxValuesLen) );
    CMI_RCP( aProtocolContext, (UChar *)aItem->mPartCondMaxValues, sPartCondMaxValuesLen );
    aItem->mPartCondMaxValues[sPartCondMaxValuesLen] = '\0';
    CMI_RD4( aProtocolContext, &(aItem->mPartitionMethod) );
    CMI_RD4( aProtocolContext, &(aItem->mPartitionOrder) );
    CMI_RD8( aProtocolContext, &(aItem->mItem.mTableOID) );
    CMI_RD4( aProtocolContext, &(aItem->mPKIndex.indexId) );
    CMI_RD4( aProtocolContext, (UInt*)&(aItem->mPKColCount) );
    CMI_RD4( aProtocolContext, (UInt*)&(aItem->mColCount) );
    CMI_RD4( aProtocolContext, (UInt*)&(aItem->mIndexCount) );
    /* PROJ-1915 Invalid Max SN을 전송 한다. */
    CMI_RD8( aProtocolContext, &(aItem->mItem.mInvalidMaxSN) );
    
    /* BUG-34260 Check Constraint */
    CMI_RD4( aProtocolContext, (UInt*)&(aItem->mCheckCount) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendMetaReplColA7( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdColumn          * aColumn,
                                   rpdVersion           aRemoteVersion, 
                                   UInt                 aTimeoutSec )
{
    UInt sColumnNameLen;
    UInt sPolicyNameLen;
    UInt sPolicyCodeLen;
    UInt sECCPolicyNameLen;
    UInt sECCPolicyCodeLen;
    UChar sOpCode;
    UInt sDefaultExprLen = 0;
    UInt sDummyLen = 0;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    /* Validate Parameters */
    IDE_DASSERT( aColumn != NULL );

    sColumnNameLen = idlOS::strlen( aColumn->mColumnName );
    sPolicyNameLen = idlOS::strlen( aColumn->mColumn.mColumnAttr.mEncAttr.mPolicy );
    sPolicyCodeLen = idlOS::strlen( (SChar *)aColumn->mPolicyCode );
    sECCPolicyNameLen = idlOS::strlen (aColumn->mECCPolicyName );
    sECCPolicyCodeLen = idlOS::strlen( (SChar *)aColumn->mECCPolicyCode );

    if ( aColumn->mFuncBasedIdxStr != NULL )
    {
        sDefaultExprLen = idlOS::strlen( aColumn->mFuncBasedIdxStr );
    }
    else 
    {
        sDefaultExprLen = 0;
    }

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 68 + sColumnNameLen + sPolicyNameLen + sPolicyCodeLen +
                             sECCPolicyNameLen + sECCPolicyCodeLen + sDefaultExprLen,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Item Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaReplCol );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sColumnNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aColumn->mColumnName, sColumnNameLen );
    CMI_WR4( aProtocolContext, &(aColumn->mColumn.column.id) );
    CMI_WR4( aProtocolContext, &(aColumn->mColumn.column.flag) );
    CMI_WR4( aProtocolContext, &(aColumn->mColumn.column.offset) );
    CMI_WR4( aProtocolContext, &(aColumn->mColumn.column.size) );
    CMI_WR4( aProtocolContext, &(aColumn->mColumn.type.dataTypeId) );
    CMI_WR4( aProtocolContext, &(aColumn->mColumn.type.languageId) );
    CMI_WR4( aProtocolContext, &(aColumn->mColumn.flag) );
    CMI_WR4( aProtocolContext, (UInt*)&(aColumn->mColumn.precision) );
    CMI_WR4( aProtocolContext, (UInt*)&(aColumn->mColumn.scale) );
    
    if ( ( aColumn->mColumn.type.dataTypeId == MTD_GEOMETRY_ID ) &&
         ( rpdMeta::needToProcessProtocolOperation( RP_META_SRID, aRemoteVersion ) == ID_TRUE ) )
    {
        CMI_WR4( aProtocolContext, (UInt*)&(aColumn->mColumn.mColumnAttr.mSridAttr.mSrid) );
        CMI_WR4( aProtocolContext, &(sDummyLen) );
        /* sDummyLen은 항상 0이어야 하므로 CMI_WCP를 호출 하지 않는다. */        
    }
    else
    {
        CMI_WR4( aProtocolContext, (UInt*)&(aColumn->mColumn.mColumnAttr.mEncAttr.mEncPrecision) );
        CMI_WR4( aProtocolContext, &(sPolicyNameLen) );
        CMI_WCP( aProtocolContext, (UChar *)aColumn->mColumn.mColumnAttr.mEncAttr.mPolicy, sPolicyNameLen );
    }
    
    CMI_WR4( aProtocolContext, &(sPolicyCodeLen) );    
    CMI_WCP( aProtocolContext, (UChar *)aColumn->mPolicyCode, sPolicyCodeLen );
    CMI_WR4( aProtocolContext, &(sECCPolicyNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aColumn->mECCPolicyName, sECCPolicyNameLen );
    CMI_WR4( aProtocolContext, &(sECCPolicyCodeLen) );
    CMI_WCP( aProtocolContext, (UChar *)aColumn->mECCPolicyCode, sECCPolicyCodeLen );

    /* BUG-35210 Function-based index */
    CMI_WR4( aProtocolContext, &(aColumn->mQPFlag) );
    CMI_WR4( aProtocolContext, &(sDefaultExprLen) );
    if ( sDefaultExprLen != 0 )
    {
        CMI_WCP( aProtocolContext, (UChar *)aColumn->mFuncBasedIdxStr, sDefaultExprLen );
    }
    else
    {
        /* do nothing */
    }

    IDU_FIT_POINT( "rpnComm::sendMetaReplCol::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplColA7( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdColumn          * aColumn,
                                   rpdVersion           aRemoteVersion,
                                   ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt sColumnNameLen;
    UInt sPolicyNameLen;
    UInt sPolicyCodeLen;
    UInt sECCPolicyNameLen;
    UInt sECCPolicyCodeLen;
    UInt sDefaultExprLen = 0;
    UInt sDummyLen = 0;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */, 
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplCol ),
                    ERR_CHECK_OPERATION_TYPE );
    /* Get Replication Item Column Information */
    CMI_RD4( aProtocolContext, &(sColumnNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aColumn->mColumnName, sColumnNameLen );
    aColumn->mColumnName[sColumnNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(aColumn->mColumn.column.id) );
    CMI_RD4( aProtocolContext, &(aColumn->mColumn.column.flag) );
    CMI_RD4( aProtocolContext, &(aColumn->mColumn.column.offset) );
    CMI_RD4( aProtocolContext, &(aColumn->mColumn.column.size) );
    CMI_RD4( aProtocolContext, &(aColumn->mColumn.type.dataTypeId) );
    CMI_RD4( aProtocolContext, &(aColumn->mColumn.type.languageId) );
    CMI_RD4( aProtocolContext, &(aColumn->mColumn.flag) );
    CMI_RD4( aProtocolContext, (UInt*)&(aColumn->mColumn.precision) );
    CMI_RD4( aProtocolContext, (UInt*)&(aColumn->mColumn.scale) );
    
    if ( ( aColumn->mColumn.type.dataTypeId == MTD_GEOMETRY_ID ) &&
         ( rpdMeta::needToProcessProtocolOperation( RP_META_SRID, aRemoteVersion ) == ID_TRUE ) )
    {
        CMI_RD4( aProtocolContext, (UInt*)&(aColumn->mColumn.mColumnAttr.mSridAttr.mSrid) );
        CMI_RD4( aProtocolContext, &(sDummyLen) );
        IDE_DASSERT( sDummyLen == 0 )
        /* sDummyLen은 항상 0이어야 하므로 CMI_RCP를 호출 하지 않는다. */
    }
    else
    {
        CMI_RD4( aProtocolContext, (UInt*)&(aColumn->mColumn.mColumnAttr.mEncAttr.mEncPrecision) );
        CMI_RD4( aProtocolContext, &(sPolicyNameLen) );
        CMI_RCP( aProtocolContext, (UChar *)aColumn->mColumn.mColumnAttr.mEncAttr.mPolicy, sPolicyNameLen );
        aColumn->mColumn.mColumnAttr.mEncAttr.mPolicy[sPolicyNameLen] = '\0';
    }
    
    CMI_RD4( aProtocolContext, &(sPolicyCodeLen) );
    CMI_RCP( aProtocolContext, (UChar *)aColumn->mPolicyCode, sPolicyCodeLen );
    aColumn->mPolicyCode[sPolicyCodeLen] = '\0';
    CMI_RD4( aProtocolContext, &(sECCPolicyNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aColumn->mECCPolicyName, sECCPolicyNameLen );
    aColumn->mECCPolicyName[sECCPolicyNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sECCPolicyCodeLen) );
    CMI_RCP( aProtocolContext, (UChar *)aColumn->mECCPolicyCode, sECCPolicyCodeLen );
    aColumn->mECCPolicyCode[sECCPolicyCodeLen] = '\0';

    /* BUG-35210 Function-based Index */
    CMI_RD4( aProtocolContext, &(aColumn->mQPFlag) );
    CMI_RD4( aProtocolContext, &(sDefaultExprLen) );
    if ( sDefaultExprLen != 0 )
    {
        IDU_FIT_POINT_RAISE( "rpnComm::recvMetaReplColA7::calloc::FuncBasedIndexExpr",
                              ERR_MEMORY_ALLOC );
        IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPN,
                                           sDefaultExprLen + 1,
                                           ID_SIZEOF(SChar),
                                           (void **)&(aColumn->mFuncBasedIdxStr),
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );
        CMI_RCP( aProtocolContext, (UChar *)aColumn->mFuncBasedIdxStr, sDefaultExprLen );
        aColumn->mFuncBasedIdxStr[sDefaultExprLen] = '\0';
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnComm::recvMetaReplColA7",
                                  "aColumn->mFuncBasedIdxStr" ) );
        IDE_ERRLOG( IDE_RP_0 );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplIdxA7( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   qcmIndex           * aIndex,
                                   UInt                 aTimeoutSec )
{
    UInt sNameLen;
    UInt sTempValue;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Validate Parameters */
    IDE_DASSERT( aIndex != NULL );

    sNameLen = idlOS::strlen( aIndex->name );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 24 + sNameLen,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Item Index Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaReplIdx );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aIndex->name, sNameLen );
    CMI_WR4( aProtocolContext, &(aIndex->indexId) );
    CMI_WR4( aProtocolContext, &(aIndex->indexTypeId) );
    CMI_WR4( aProtocolContext, &(aIndex->keyColCount) );

    sTempValue = aIndex->isUnique;
    CMI_WR4( aProtocolContext, &(sTempValue) );

    sTempValue = aIndex->isRange;
    CMI_WR4( aProtocolContext, &(sTempValue) );

    IDU_FIT_POINT( "rpnComm::sendMetaReplIdx::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplIdxA7( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   qcmIndex           * aIndex,
                                   ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt  sNameLen;
    UInt  sIsUnique;
    UInt  sIsRange;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */,
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplIdx ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, &(sNameLen) );
    CMI_RCP( aProtocolContext, (UChar *)aIndex->name, sNameLen );
    aIndex->name[sNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(aIndex->indexId) );
    CMI_RD4( aProtocolContext, &(aIndex->indexTypeId) );
    CMI_RD4( aProtocolContext, &(aIndex->keyColCount) );
    CMI_RD4( aProtocolContext, &(sIsUnique) );
    CMI_RD4( aProtocolContext, &(sIsRange) );

    aIndex->isUnique = ( sIsUnique == 0 ) ? ID_FALSE : ID_TRUE;
    aIndex->isRange = ( sIsRange == 0 ) ? ID_FALSE : ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplIdxColA7( void                * aHBTResource,
                                      cmiProtocolContext  * aProtocolContext,
                                      idBool              * aExitFlag,
                                      UInt                  aColumnID,
                                      UInt                  aKeyColumnFlag,
                                      UInt                  aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 8, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Item Index Column Description Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaReplIdxCol );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(aColumnID) );
    CMI_WR4( aProtocolContext, &(aKeyColumnFlag) );

    IDU_FIT_POINT( "rpnComm::sendMetaReplIdxCol::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplIdxColA7( cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      UInt               * aColumnID,
                                      UInt               * aKeyColumnFlag,
                                      ULong                aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */, 
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplIdxCol ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Item Index Information */
    CMI_RD4( aProtocolContext, aColumnID );
    CMI_RD4( aProtocolContext, aKeyColumnFlag );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplCheckA7( void                 * aHBTResource,
                                     cmiProtocolContext   * aProtocolContext,
                                     idBool               * aExitFlag,
                                     qcmCheck             * aCheck,
                                     UInt                   aTimeoutSec ) 
{
    UInt    sCheckConditionLength = 0;
    UInt    sColumnCount = 0;
    UInt    sCheckNameLength = 0;
    UInt    sColumnIndex = 0;
    UChar   sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Validate Parameters */
    sCheckNameLength = idlOS::strlen( aCheck->name );
    sColumnCount = aCheck->constraintColumnCount;
    sCheckConditionLength = idlOS::strlen( aCheck->checkCondition );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 16 + sCheckNameLength + 
                             sColumnCount * 4 + sCheckConditionLength,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Item Check Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaReplCheck );

    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &sCheckNameLength );
    CMI_WCP( aProtocolContext, (UChar *)aCheck->name, sCheckNameLength );

    CMI_WR4( aProtocolContext, &(aCheck->constraintID) );
    CMI_WR4( aProtocolContext, &(aCheck->constraintColumnCount) );

    for ( sColumnIndex = 0; sColumnIndex < aCheck->constraintColumnCount; sColumnIndex++ )
    {
        CMI_WR4( aProtocolContext, &(aCheck->constraintColumn[sColumnIndex]) );
    }

    CMI_WR4( aProtocolContext, &sCheckConditionLength );
    CMI_WCP( aProtocolContext, (UChar *)aCheck->checkCondition, sCheckConditionLength );

    IDU_FIT_POINT( "rpnComm::sendMetaReplCheck::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplCheckA7( cmiProtocolContext   * aProtocolContext,
                                     idBool               * aExitFlag,
                                     qcmCheck             * aCheck,
                                     const ULong            aTimeoutSec )
{
    UChar       sOpCode;
    UInt        sCheckNameLength = 0;
    UInt        sCheckConditionLength = 0; 
    UInt        sColumnIndex = 0;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */, 
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplCheck ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Item check Information */
    CMI_RD4( aProtocolContext, &sCheckNameLength );
    CMI_RCP( aProtocolContext, (UChar *)aCheck->name, sCheckNameLength );
    aCheck->name[sCheckNameLength] = '\0';

    CMI_RD4( aProtocolContext, &(aCheck->constraintID) );
    CMI_RD4( aProtocolContext, &(aCheck->constraintColumnCount) );

    for ( sColumnIndex = 0; sColumnIndex < aCheck->constraintColumnCount; sColumnIndex++ )
    {
        CMI_RD4( aProtocolContext, &(aCheck->constraintColumn[sColumnIndex]) );
    }

    CMI_RD4( aProtocolContext, &sCheckConditionLength );

    IDU_FIT_POINT_RAISE( "rpnComm::recvMetaReplCheckA7::calloc::CheckCondition", 
                          ERR_MEMORY_ALLOC_CHECK );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPN,
                                       sCheckConditionLength + 1,
                                       ID_SIZEOF(SChar),
                                       (void **)&(aCheck->checkCondition),
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_CHECK );

    CMI_RCP( aProtocolContext, (UChar *)aCheck->checkCondition, sCheckConditionLength );
    aCheck->checkCondition[sCheckConditionLength] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CHECK );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnComm::recvMetaReplCheckA7",
                                  "aChecks" ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTempSyncHandshakeAckA7( cmiProtocolContext  * aProtocolContext,
                                            idBool              * aExitFlag,
                                            UInt                  aResult,
                                            SChar               * aMsg,
                                            UInt                  aTimeoutSec )
{
    UInt sMsgLen = 0;
    UChar sOpCode = 0;
    ULong sVersion = RP_CURRENT_VERSION;

    if ( aResult == RP_MSG_OK )
    {
        // copy protocol version.
        sMsgLen = ID_SIZEOF(ULong);
    }
    else
    {
        sMsgLen = idlOS::strlen( aMsg ) + 1;
    }

    IDE_TEST( checkAndFlush( NULL,  /* aHBTResource */
                             aProtocolContext, 
                             aExitFlag,
                             1 + 
                             4 +
                             4 + sMsgLen,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Item Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, TemporarySyncHandshakeAck );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &aResult );
    CMI_WR4( aProtocolContext, &sMsgLen );

    if ( aResult == RP_MSG_OK )
    {
        CMI_WR8( aProtocolContext, (ULong*)&sVersion );
    }
    else
    {
        CMI_WCP( aProtocolContext, (UChar *)aMsg, sMsgLen );
    }

    IDE_TEST( sendCmBlock( NULL, /* aHBTResource */
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTempSyncHandshakeAckA7( cmiProtocolContext * aProtocolContext,
                                            idBool             * aExitFlag,
                                            UInt               * aResult,
                                            SChar              * aMsg,
                                            UInt               * aMsgLen,
                                            ULong                aTimeOut )
{
    UChar sOpCode;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */,
                           aTimeOut )
              != IDE_SUCCESS );

    /*
     * recvHandshakeAck()를 통해 처음으로 패킷을 받는 경우가 있다.
     * 그래서 readCmBlock() 이후에 검사를 한다.
     */ 
    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, TemporarySyncHandshakeAck ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get argument */
    CMI_RD4( aProtocolContext, aResult );
    CMI_RD4( aProtocolContext, aMsgLen );
    CMI_RCP( aProtocolContext, (UChar *)aMsg, *aMsgLen );
    aMsg[*aMsgLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpnComm::sendHandshakeAckA7( cmiProtocolContext  * aProtocolContext,
                                    idBool              * aExitFlag,
                                    UInt                  aResult,
                                    SInt                  aFailbackStatus,
                                    ULong                 aXSN,
                                    const SChar         * aMsg,
                                    UInt                  aTimeoutSec )
{
    UInt sMsgLen = 0;
    UChar sOpCode = 0;
    ULong sVersion = RP_CURRENT_VERSION;

    if ( aResult == RP_MSG_OK )
    {
        // copy protocol version.
        sMsgLen = ID_SIZEOF(ULong);
    }
    else
    {
        sMsgLen = idlOS::strlen( aMsg ) + 1;
    }

    IDE_TEST( checkAndFlush( NULL,  /* aHBTResource */
                             aProtocolContext, 
                             aExitFlag,
                             1 + 20 + sMsgLen,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Item Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, HandshakeAck );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &aResult );
    CMI_WR4( aProtocolContext, (UInt*)&aFailbackStatus );
    CMI_WR8( aProtocolContext, (ULong*)&aXSN );
    CMI_WR4( aProtocolContext, &sMsgLen );

    if ( aResult == RP_MSG_OK )
    {
        CMI_WR8( aProtocolContext, (ULong*)&sVersion );
    }
    else
    {
        CMI_WCP( aProtocolContext, (UChar *)aMsg, sMsgLen );
    }

    IDU_FIT_POINT( "rpnComm::sendHandshakeAckA7::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( NULL, /* aHBTResource */
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvHandshakeAckA7( void               * aHBTResource,
                                    cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    UInt               * aResult,
                                    SInt               * aFailbackStatus,
                                    ULong              * aXSN,
                                    SChar              * aMsg,
                                    UInt               * aMsgLen,
                                    ULong                aTimeOut )
{
    UChar sOpCode;

    IDE_TEST( readCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */,
                           aTimeOut )
              != IDE_SUCCESS );

    /*
     * recvHandshakeAck()를 통해 처음으로 패킷을 받는 경우가 있다.
     * 그래서 readCmBlock() 이후에 검사를 한다.
     */ 
    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, HandshakeAck ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get argument */
    CMI_RD4( aProtocolContext, aResult );
    CMI_RD4( aProtocolContext, (UInt*)aFailbackStatus );
    CMI_RD8( aProtocolContext, (ULong*)aXSN );
    CMI_RD4( aProtocolContext, aMsgLen );
    CMI_RCP( aProtocolContext, (UChar *)aMsg, *aMsgLen );
    aMsg[*aMsgLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvXLogA7( iduMemAllocator    * aAllocator,
                            rpxReceiverReadContext aReadContext,
                            idBool             * aExitFlag,
                            rpdMeta            * aMeta,
                            rpdXLog            * aXLog,
                            ULong                aTimeOutSec )
{
    UChar sOpCode = CMI_PROTOCOL_OPERATION( RP, MAX );

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        if ( CMI_IS_READ_ALL( aReadContext.mCMContext ) == ID_TRUE )
        {
            IDE_TEST( readCmBlock( NULL, aReadContext.mCMContext, aExitFlag, NULL /* TimeoutFlag */, aTimeOutSec )
                      != IDE_SUCCESS );
        }
        CMI_RD1( aReadContext.mCMContext, sOpCode );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        IDE_TEST( readXLogfile( aReadContext.mXLogfileContext ) != IDE_SUCCESS );
        (void)aReadContext.mXLogfileContext->readXLog(&sOpCode, 1);
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    // BUG-17215
    IDU_FIT_POINT_RAISE( "1.BUG-17215@rpnComm::recvXLogA7",
                          ERR_PROTOCOL_OPID );

    switch ( sOpCode )
    {
        case CMI_PROTOCOL_OPERATION( RP, TrBegin ):
        {
            IDE_TEST( recvTrBeginA7( aReadContext,
                                     aExitFlag, 
                                     aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, TrCommit ):
        {
            IDE_TEST( recvTrCommitA7( aReadContext,
                                      aExitFlag, 
                                      aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, TrAbort ):
        {
            IDE_TEST( recvTrAbortA7( aReadContext,
                                     aExitFlag,
                                     aXLog )
                     != IDE_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SPSet ):
        {
            IDE_TEST( recvSPSetA7( aReadContext,
                                   aExitFlag,
                                   aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SPAbort ):
        {
            IDE_TEST( recvSPAbortA7( aReadContext,
                                     aExitFlag,
                                     aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Insert ):
        {
            IDE_TEST( recvInsertA7( aAllocator,
                                    aExitFlag,
                                    aReadContext,
                                    aMeta,     // BUG-20506
                                    aXLog,
                                    aTimeOutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Update ):
        {
            IDE_TEST( recvUpdateA7( aAllocator,
                                    aExitFlag,
                                    aReadContext,
                                    aMeta,     // BUG-20506
                                    aXLog,
                                    aTimeOutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Delete ):
        {
            IDE_TEST( recvDeleteA7( aAllocator,
                                    aExitFlag,
                                    aReadContext,
                                    aMeta,     // BUG-20506
                                    aXLog,
                                    aTimeOutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Stop ):
        {
            IDE_TEST( recvStopA7( aReadContext,
                                  aExitFlag,
                                  aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, KeepAlive ):
        {
            IDE_TEST( recvKeepAliveA7( aReadContext,
                                       aExitFlag,
                                       aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Flush ):
        {
            IDE_TEST( recvFlushA7( aReadContext,
                                   aExitFlag,
                                   aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobCursorOpen ):
        {
            IDE_TEST( recvLobCursorOpenA7( aAllocator,
                                           aExitFlag,
                                           aReadContext,
                                           aMeta,  // BUG-20506
                                           aXLog,
                                           aTimeOutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobCursorClose ):
        {
            IDE_TEST( recvLobCursorCloseA7( aReadContext,
                                            aExitFlag,
                                            aXLog )
                      != IDE_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobPrepare4Write ):
        {
            IDE_TEST( recvLobPrepare4WriteA7( aReadContext,
                                              aExitFlag,
                                              aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobPartialWrite ):
        {
            IDE_TEST( recvLobPartialWriteA7( aAllocator,
                                             aExitFlag,
                                             aReadContext,
                                             aXLog,
                                             aTimeOutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobFinish2Write ):
        {
            IDE_TEST( recvLobFinish2WriteA7( aReadContext,
                                             aExitFlag,
                                             aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobTrim ):
        {

            IDE_TEST( recvLobTrim( aAllocator,
                                   aExitFlag,
                                   aReadContext,
                                   aXLog,
                                   aTimeOutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Handshake ): // BUG-23195
        {
            IDE_TEST( recvHandshakeA7( aReadContext,
                                       aExitFlag,
                                       aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SyncPKBegin ):
        {
            IDE_TEST( recvSyncPKBeginA7( aReadContext.mCMContext,
                                         aExitFlag,
                                         aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SyncPK ):
        {
            IDE_TEST( recvSyncPKA7( aExitFlag,
                                    aReadContext.mCMContext,
                                    aXLog,
                                    aTimeOutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SyncPKEnd ):
        {
            IDE_TEST( recvSyncPKEndA7( aReadContext.mCMContext,
                                       aExitFlag,
                                       aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION(RP, FailbackEnd):
        {
            IDE_TEST( recvFailbackEndA7( aReadContext.mCMContext,
                                         aExitFlag,
                                         aXLog )
                     != IDE_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SyncStart ):
        {
            IDE_TEST( recvSyncStart( aAllocator,
                                     aExitFlag,
                                     aReadContext.mCMContext,
                                     aXLog,
                                     aTimeOutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SyncEnd ):
        {
            IDE_TEST( recvSyncEnd( aAllocator,
                                        aExitFlag,
                                        aReadContext.mCMContext,
                                        aXLog,
                                        aTimeOutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, AckOnDML ):
        {
            IDE_TEST( recvAckOnDML( aReadContext.mCMContext,
                                    aExitFlag,
                                    aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, DDLReplicateHandshake ):
        {
            IDE_TEST( recvDDLASyncStartA7( aReadContext.mCMContext,
                                           aExitFlag,
                                           aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Truncate ):
        {
            IDE_TEST( recvTruncateA7( aReadContext.mCMContext,
                                      aExitFlag,
                                      aMeta,
                                      aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, XA_START_REQ ) :
        {
            IDE_TEST( recvXaStartReqA7( aReadContext,
                                        aExitFlag,
                                        aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, XA_PREPARE_REQ ) :
        {
            IDE_TEST( recvXaPrepareReqA7( aReadContext,
                                          aExitFlag,
                                          aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, XA_PREPARE ) :
        {
            IDE_TEST( recvXaPrepareA7( aReadContext,
                                       aExitFlag,
                                       aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, XA_COMMIT ):
        {
            IDE_TEST( recvXaCommitA7( aReadContext,
                                      aExitFlag, 
                                      aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, XA_END ) :
        {
            IDE_TEST( recvXaEndA7( aReadContext,
                                   aExitFlag,
                                   aXLog )
                      != IDE_SUCCESS );
            break;
        }
        default:
            // BUG-17215
            IDE_RAISE( ERR_PROTOCOL_OPID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PROTOCOL_OPID );
    {
        IDE_SET( ideSetErrorCode(rpERR_ABORT_RECEIVER_UNEXPECTED_PROTOCOL,
                                 sOpCode ) );
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTrBeginA7( void                * aHBTResource,
                               cmiProtocolContext  * aProtocolContext,
                               idBool              * aExitFlag,
                               smTID                 aTID,
                               smSN                  aSN,
                               smSN                  aSyncSN,
                               UInt                  aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    sType= RP_X_BEGIN;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 24,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, TrBegin );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTrBeginA7( rpxReceiverReadContext aReadContext,
                               idBool              * /* aExitFlag */,
                               rpdXLog             * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN), 8 );
    }
    else
    {
        IDE_RAISE( INVALID_READ_CONTEXT );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( INVALID_READ_CONTEXT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;


}

IDE_RC rpnComm::sendTrCommitA7( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                idBool               aForceFlush,
                                UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType= RP_X_COMMIT;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 24, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, TrCommit );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );

    if( aForceFlush == ID_TRUE )
    {
        IDU_FIT_POINT( "rpnComm::sendTrCommit::cmiSend::ERR_SEND" );
        IDE_TEST( sendCmBlock( aHBTResource,
                               aProtocolContext, 
                               aExitFlag,
                               ID_TRUE,
                               aTimeoutSec ) 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTrCommitA7( rpxReceiverReadContext aReadContext,
                                idBool              * /*aExitFlag*/,
                                rpdXLog             * aXLog )

{
    UInt * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN), 8 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendXaCommitA7( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                smSCN                aGlobalCommitSCN,
                                idBool               aForceFlush,
                                UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_XA_COMMIT;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 24 + 8, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, XA_COMMIT );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    CMI_WR8( aProtocolContext, &(aGlobalCommitSCN) );

    if( aForceFlush == ID_TRUE )
    {
        IDE_TEST( sendCmBlock( aHBTResource,
                               aProtocolContext, 
                               aExitFlag,
                               ID_TRUE,
                               aTimeoutSec ) 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvXaCommitA7( rpxReceiverReadContext aReadContext,
                                idBool              * /*aExitFlag*/,
                                rpdXLog             * aXLog )

{
    UInt * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mGlobalCommitSCN) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN), 8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mGlobalCommitSCN), 8 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC rpnComm::sendTrAbortA7( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    sType= RP_X_ABORT;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 24,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, TrAbort );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTrAbortA7( rpxReceiverReadContext aReadContext,
                               idBool              * /* aExitFlag */,
                               rpdXLog             * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN), 8 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSPSetA7( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             smSN                 aSyncSN,
                             UInt                 aSPNameLen,
                             SChar              * aSPName,
                             UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType= RP_X_SP_SET;
    
    /* Validate Parameters */
    IDE_DASSERT( aSPNameLen != 0 );
    IDE_DASSERT( aSPName != NULL );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 28 + 
                             aSPNameLen,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, SPSet );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    CMI_WR4( aProtocolContext, &(aSPNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aSPName, aSPNameLen );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvSPSetA7( rpxReceiverReadContext aReadContext,
                             idBool              * /*aExitFlag*/,
                             rpdXLog             * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    /* Get Argument XLog Hdr */
    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mSPNameLen) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),      4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),       4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),        8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),    8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSPNameLen), 4 );
    }

    IDE_DASSERT( aXLog->mSPNameLen > 0 );

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mSPNameLen + 1,
                                          (void **)&( aXLog->mSPName ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RCP( aReadContext.mCMContext, (UChar *)aXLog->mSPName, aXLog->mSPNameLen );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( aXLog->mSPName, aXLog->mSPNameLen, ID_FALSE );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    aXLog->mSPName[aXLog->mSPNameLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnCommA7::recvSPSetA7",
                                  "mSPName" ) );
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSPAbortA7( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aSPNameLen,
                               SChar              * aSPName,
                               UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType= RP_X_SP_ABORT;

    /* Validate Parameters */
    IDE_DASSERT( aSPNameLen != 0 );
    IDE_DASSERT( aSPName != NULL );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 28 + 
                             aSPNameLen,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, SPAbort );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    CMI_WR4( aProtocolContext, &(aSPNameLen) );
    CMI_WCP( aProtocolContext, (UChar *)aSPName, aSPNameLen );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvSPAbortA7( rpxReceiverReadContext aReadContext,
                               idBool              * /*aExitFlag*/,
                               rpdXLog             * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    /* Get Argument XLog Hdr */
    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mSPNameLen) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),      4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),       4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),        8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),    8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSPNameLen), 4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    IDE_DASSERT( aXLog->mSPNameLen > 0 );

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mSPNameLen + 1,
                                          (void **)&( aXLog->mSPName ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RCP( aReadContext.mCMContext, (UChar *)aXLog->mSPName, aXLog->mSPNameLen );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( aXLog->mSPName, aXLog->mSPNameLen, ID_FALSE );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    aXLog->mSPName[aXLog->mSPNameLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnCommA7::recvSPAbortA7",
                                  "mSPName" ) );
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendInsertA7( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              UInt                 aImplSPDepth,
                              ULong                aTableOID,
                              UInt                 aColCnt,
                              smiValue           * aACols,
                              rpValueLen         * aALen,
                              UInt                 aTimeoutSec )
{
    UInt i;
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType= RP_X_INSERT;

    /* Validate Parameters */
    IDE_DASSERT(aImplSPDepth <= SMI_STATEMENT_DEPTH_MAX);
    IDE_DASSERT(aColCnt <= QCI_MAX_COLUMN_COUNT);
    IDE_DASSERT(aACols != NULL);

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 40, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Insert );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information     */
    CMI_WR4( aProtocolContext, &(aImplSPDepth) );
    CMI_WR8( aProtocolContext, &(aTableOID) );
    CMI_WR4( aProtocolContext, &(aColCnt) );

    /* Send Value repeatedly */
    for(i = 0; i < aColCnt; i ++)
    {
        IDE_TEST( sendValueForA7( aHBTResource,
                                  aProtocolContext,
                                  aExitFlag,
                                  &aACols[i],
                                  aALen[i],
                                  aTimeoutSec )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvInsertA7( iduMemAllocator    * aAllocator,
                              idBool             * aExitFlag,
                              rpxReceiverReadContext aReadContext,
                              rpdMeta            * aMeta,  // BUG-20506
                              rpdXLog            * aXLog,
                              ULong                aTimeOutSec )
{
    UInt             i;
    rpdMetaItem     *sItem   = NULL;
    rpdColumn       *sColumn = NULL;
    const mtdModule *sMtd    = NULL;
    smiValue         sDummy;
    UInt             sCID;
    UInt           * sType = (UInt*)&(aXLog->mType);
    const void     * sTable = NULL;
    mtcColumn      * sMtcColumn = NULL;

    IDE_ASSERT(aMeta != NULL);

    /* Get Argument XLog Hdr */
    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mImplSPDepth) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mTableOID) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mColCnt) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),        4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),         4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),          8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),      8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mImplSPDepth), 4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTableOID),    8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mColCnt),      4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * XLog 내의 Column 정보를 재배열한다.
     *
     * Column : mColCnt, mCIDs[], mACols[]
     */
    (void)aMeta->searchRemoteTable(&sItem, aXLog->mTableOID);

    if ( sItem != NULL )
    {
        IDE_TEST_RAISE( aXLog->mMemory.alloc( sItem->mColCount * ID_SIZEOF( UInt ),
                                              (void **)&( aXLog->mCIDs ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        IDE_TEST_RAISE( aXLog->mMemory.alloc( sItem->mColCount * ID_SIZEOF( smiValue ),
                                              (void **)&( aXLog->mBCols ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        IDE_TEST_RAISE( aXLog->mMemory.alloc( sItem->mColCount * ID_SIZEOF( smiValue ),
                                              (void **)&( aXLog->mACols ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        sTable = smiGetTable( sItem->mItem.mTableOID );

        /* Recv Value repeatedly */
        // Column ID 순서로 Column 값을 재배열한다.
        for(i = 0; i < aXLog->mColCnt; i++)
        {
            sCID = sItem->mMapColID[i];

            if( sCID != RP_INVALID_COLUMN_ID )
            {
                aXLog->mCIDs[sCID] = sCID;
                sMtcColumn = (mtcColumn*)rpdCatalog::rpdGetTableColumns( sTable, sCID );
                IDE_TEST_RAISE( sMtcColumn == NULL, ERR_NOT_FOUND_COLUMN );

                IDE_TEST(recvValueA7( aAllocator,
                                      aExitFlag,
                                      aReadContext,
                                      &aXLog->mMemory,
                                      &aXLog->mACols[sCID],
                                      aTimeOutSec,
                                      sMtcColumn )
                         != IDE_SUCCESS);
            }
            else
            {
                sDummy.value = NULL;
                IDE_TEST(recvValueA7(aAllocator,
                                     aExitFlag,
                                     aReadContext,
                                     NULL, /* aMemory */
                                     &sDummy,
                                     aTimeOutSec,
                                     NULL)
                         != IDE_SUCCESS);

                if(sDummy.value != NULL)
                {
                    (void)iduMemMgr::free((void *)sDummy.value, aAllocator);
                }
            }
        }

        // Standby Server에서 Replication 대상이 아닌 Column에 NULL을 할당한다.
        if(sItem->mHasOnlyReplCol != ID_TRUE)
        {
            for(i = 0; i < (UInt)sItem->mColCount; i++)
            {
                // 재배열된 Column을 XLog에 복사한다.
                aXLog->mCIDs[i] = i;

                // Replication 대상이 아닌 Column은,
                // DEFAULT 값 대신 mtdModule의 staticNull을 사용한다.
                if(sItem->mIsReplCol[i] != ID_TRUE)
                {
                    sColumn = sItem->getRpdColumn(i);
                    IDE_TEST_RAISE(sColumn == NULL, ERR_NOT_FOUND_COLUMN);

                    IDU_FIT_POINT_RAISE( "rpnComm::recvInsertA7::Erratic::rpERR_ABORT_GET_MODULE",
                                         ERR_GET_MODULE );
                    IDE_TEST_RAISE(mtd::moduleById(&sMtd,
                                                   sColumn->mColumn.type.dataTypeId)
                                   != IDE_SUCCESS, ERR_GET_MODULE);

                    IDE_TEST(allocNullValue(aAllocator,
                                            &aXLog->mMemory,
                                            &aXLog->mACols[i],
                                            (const mtcColumn *)&sColumn->mColumn,
                                            sMtd)
                             != IDE_SUCCESS);
                }
            }
        }
        aXLog->mColCnt = sItem->mColCount;
    }
    else
    {
        /* Recv Value repeatedly */
        // Column ID 순서로 Column 값을 재배열한다.
        for(i = 0; i < aXLog->mColCnt; i++)
        {
            sDummy.value = NULL;
            IDE_TEST(recvValueA7(aAllocator,
                                 aExitFlag,
                                 aReadContext,
                                 NULL, /* aMemory */
                                 &sDummy,
                                 aTimeOutSec,
                                 NULL)
                     != IDE_SUCCESS);

            if(sDummy.value != NULL)
            {
                (void)iduMemMgr::free((void *)sDummy.value, aAllocator);
            }
        }

        IDU_FIT_POINT_RAISE( "rpnComm::recvInsertA7::Erratic::rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE",
                             ERR_NOT_FOUND_TABLE );
        IDE_RAISE( ERR_NOT_FOUND_TABLE );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                aXLog->mSN,
                                aXLog->mTableOID));
    }
    IDE_EXCEPTION(ERR_NOT_FOUND_COLUMN);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_COLUMN,
                                aXLog->mSN,
                                aXLog->mTableOID,
                                i));
    }
    IDE_EXCEPTION(ERR_GET_MODULE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_GET_MODULE));
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvInsertA7",
                                "Column"));        
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendUpdateA7( void            * aHBTResource,
                              cmiProtocolContext *aProtocolContext,
                              idBool          * aExitFlag,
                              smTID             aTID,
                              smSN              aSN,
                              smSN              aSyncSN,
                              UInt              aImplSPDepth,
                              ULong             aTableOID,
                              UInt              aPKColCnt,
                              UInt              aUpdateColCnt,
                              smiValue        * aPKCols,
                              UInt            * aCIDs,
                              smiValue        * aBCols,
                              smiChainedValue * aBChainedCols, // PROJ-1705
                              UInt            * aBChainedColsTotalLen, /* BUG-33722 */
                              smiValue        * aACols,
                              rpValueLen      * aPKLen,
                              rpValueLen      * aALen,
                              rpValueLen      * aBMtdValueLen,
                              UInt              aTimeoutSec )
{
    UInt i;
    smiChainedValue * sChainedValue = NULL;
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType= RP_X_UPDATE;

    /* Validate Parameters */
    IDE_DASSERT( aImplSPDepth <= SMI_STATEMENT_DEPTH_MAX );
    IDE_DASSERT( aPKColCnt <= QCI_MAX_KEY_COLUMN_COUNT );
    IDE_DASSERT( aUpdateColCnt <= QCI_MAX_COLUMN_COUNT );
    IDE_DASSERT( aPKCols != NULL );
    IDE_DASSERT( aCIDs != NULL );
    IDE_DASSERT( aBCols != NULL );
    IDE_DASSERT( aACols != NULL );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 44, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Update );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information     */
    CMI_WR4( aProtocolContext, &(aImplSPDepth) );
    CMI_WR8( aProtocolContext, &(aTableOID) );
    CMI_WR4( aProtocolContext, &(aPKColCnt) );
    CMI_WR4( aProtocolContext, &(aUpdateColCnt) );

    /* Send Primary Key Value */
    for ( i = 0; i < aPKColCnt; i ++ )
    {
        IDE_TEST( sendPKValueForA7( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    &aPKCols[i],
                                    aPKLen[i],
                                    aTimeoutSec )
                  != IDE_SUCCESS );
    }

    /* Send Update Column Value : Before/After */
    for ( i = 0; i < aUpdateColCnt; i ++ )
    {
        IDE_TEST( sendCIDForA7( aHBTResource,
                                aProtocolContext,
                                aExitFlag,
                                aCIDs[i],
                                aTimeoutSec )
                  != IDE_SUCCESS );

        // PROJ-1705
        sChainedValue = &aBChainedCols[aCIDs[i]];

        // 메모리 테이블에 발생한 update의 분석 결과는 BCols에 들어있다.
        if ( (sChainedValue->mAllocMethod == SMI_NON_ALLOCED) && // mAllocMethod의 초기값은 SMI_NON_ALLOCED
             (aBMtdValueLen[aCIDs[i]].lengthSize == 0) )
        {
            IDE_TEST( sendValueForA7( aHBTResource,
                                      aProtocolContext,
                                      aExitFlag,
                                      &aBCols[aCIDs[i]],
                                      aBMtdValueLen[aCIDs[i]],
                                      aTimeoutSec )
                      != IDE_SUCCESS );
        }
        // 디스크 테이블에 발생한 update의 분석 결과는 BChainedCols에 들어있다.
        else
        {
            IDE_TEST( sendChainedValueForA7( aHBTResource,
                                             aProtocolContext,
                                             aExitFlag,
                                             sChainedValue,
                                             aBMtdValueLen[aCIDs[i]],
                                             aBChainedColsTotalLen[aCIDs[i]],
                                             aTimeoutSec )
                      != IDE_SUCCESS );
        }

        IDE_TEST( sendValueForA7( aHBTResource,
                                  aProtocolContext,
                                  aExitFlag,
                                  &aACols[aCIDs[i]],
                                  aALen[aCIDs[i]],
                                  aTimeoutSec )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvUpdateA7( iduMemAllocator    * aAllocator,
                              idBool             * aExitFlag,
                              rpxReceiverReadContext aReadContext,
                              rpdMeta            * aMeta,  // BUG-20506
                              rpdXLog            * aXLog,
                              ULong                aTimeOutSec )
{
    UInt i;
    UInt sRecvIndex = 0;
    UInt sCID = 0;
    rpdMetaItem *sItem = NULL;
    UInt * sType = (UInt*)&(aXLog->mType);
    smiValue     sDummy;
    idBool       sNeedCompact = ID_FALSE;
    UInt         sColumnCount = 0;
    const void * sTable       = NULL;
    mtcColumn  * sMtcColumn   = NULL;

    IDE_ASSERT( aMeta != NULL );

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        /* Get Argument XLog Hdr */
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        /* Get Argument XLog Body */
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mImplSPDepth) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mTableOID) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mPKColCnt) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mColCnt) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),        4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),         4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),          8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),      8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mImplSPDepth), 4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTableOID),    8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mPKColCnt),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mColCnt),      4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    (void)aMeta->searchRemoteTable(&sItem, aXLog->mTableOID);
    
    if ( sItem != NULL )
    {
        sNeedCompact = sItem->needCompact();
        if ( sNeedCompact == ID_TRUE )
        {
            sColumnCount = sItem->mColCount;
        }
        else
        {
            sColumnCount = aXLog->mColCnt;
        }

        IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mPKColCnt * ID_SIZEOF( smiValue ),
                                              (void **)&( aXLog->mPKCols ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        IDE_TEST_RAISE( aXLog->mMemory.alloc( sColumnCount * ID_SIZEOF( UInt ),
                                              (void **)&( aXLog->mCIDs ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        IDE_TEST_RAISE( aXLog->mMemory.alloc( sColumnCount * ID_SIZEOF( smiValue),
                                              (void **)&( aXLog->mBCols ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        IDE_TEST_RAISE( aXLog->mMemory.alloc( sColumnCount * ID_SIZEOF( smiValue ),
                                              (void **)&( aXLog->mACols ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        if ( sNeedCompact == ID_TRUE )
        {
            //for ( i = 0; i < sColumnCount; i++ )
            //{
            //    aXLog->mCIDs[i] = RP_INVALID_COLUMN_ID;
            //}
            IDE_DASSERT( RP_INVALID_COLUMN_ID == 0xFFFFFFFF );
            idlOS::memset( aXLog->mCIDs, 0xFF, sColumnCount * ID_SIZEOF(UInt) );
        }
        else
        {
            /* do nothing */
        }

        /* Recv PK Value repeatedly */
        for ( i = 0; i < aXLog->mPKColCnt; i ++ )
        {
            IDE_TEST( recvValueA7( aAllocator,
                                   aExitFlag,
                                   aReadContext,
                                   &aXLog->mMemory,
                                   &aXLog->mPKCols[i],
                                   aTimeOutSec,
                                   NULL )
                      != IDE_SUCCESS );
        }

        /* PROJ-1442 Replication Online 중 DDL 허용
         * XLog 내의 Column 정보를 재배열한다.
         *   - mPKColCnt는 변하지 않으므로, 제외한다.
         *   - Primary Key의 순서가 같으므로, mPKCIDs[]와 mPKCols[]는 제외한다.
         *
         * Column : mColCnt, mCIDs[], mBCols[], mACols[]
         */

        sTable = smiGetTable( sItem->mItem.mTableOID );

        /* Recv Update Value repeatedly */
        for(i = 0; i < aXLog->mColCnt; i++)
        {
            IDE_TEST( recvCIDA7( aExitFlag,
                                 aReadContext,
                                 &sCID,
                                 aTimeOutSec )
                      != IDE_SUCCESS );

            sRecvIndex = sItem->mMapColID[sCID];

            if( sRecvIndex != RP_INVALID_COLUMN_ID )
            {
                if ( sNeedCompact == ID_FALSE )
                {
                    aXLog->mCIDs[i] = sRecvIndex;
                    sRecvIndex = i;
                }
                else
                {
                    aXLog->mCIDs[sRecvIndex] = sRecvIndex;
                }

                sMtcColumn = (mtcColumn*)rpdCatalog::rpdGetTableColumns( sTable, sCID );
                IDE_TEST_RAISE( sMtcColumn == NULL, ERR_NOT_FOUND_COLUMN );

                IDE_TEST(recvValueA7(aAllocator,
                                     aExitFlag,
                                     aReadContext,
                                     &aXLog->mMemory,
                                     &aXLog->mBCols[sRecvIndex],
                                     aTimeOutSec,
                                     sMtcColumn)
                         != IDE_SUCCESS);
                IDE_TEST(recvValueA7(aAllocator,
                                     aExitFlag,
                                     aReadContext,
                                     &aXLog->mMemory,
                                     &aXLog->mACols[sRecvIndex],
                                     aTimeOutSec,
                                     sMtcColumn)
                         != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST(recvValueA7(aAllocator,
                                     aExitFlag,
                                     aReadContext,
                                     NULL, /* aMemory */
                                     &sDummy,
                                     aTimeOutSec,
                                     NULL)
                         != IDE_SUCCESS);
                if(sDummy.value != NULL)
                {
                    (void)iduMemMgr::free((void *)sDummy.value, aAllocator);
                }

                IDE_TEST(recvValueA7(aAllocator,
                                     aExitFlag,
                                     aReadContext,
                                     NULL, /* aMemory */
                                     &sDummy,
                                     aTimeOutSec,
                                     NULL)
                         != IDE_SUCCESS);
                if(sDummy.value != NULL)
                {
                    (void)iduMemMgr::free((void *)sDummy.value, aAllocator);
                }

                i--;
                aXLog->mColCnt--;
            }
        }

        if ( sNeedCompact == ID_TRUE )
        {
            compactCIDAndColumnValueArray( aXLog->mCIDs,
                                           aXLog->mBCols,
                                           aXLog->mACols,
                                           aXLog->mColCnt,
                                           sColumnCount );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* Recv PK Value repeatedly */
        for ( i = 0; i < aXLog->mPKColCnt; i ++ )
        {
            IDE_TEST(recvValueA7(aAllocator,
                                 aExitFlag,
                                 aReadContext,
                                 NULL, /* aMemory */
                                 &sDummy,
                                 aTimeOutSec,
                                 NULL)
                     != IDE_SUCCESS);
            if(sDummy.value != NULL)
            {
                (void)iduMemMgr::free((void *)sDummy.value, aAllocator);
            }
        }
        
        /* Recv Update Value repeatedly */
        for(i = 0; i < aXLog->mColCnt; i++)
        {
            IDE_TEST( recvCIDA7( aExitFlag,
                                 aReadContext,
                                 &sCID,
                                 aTimeOutSec )
                      != IDE_SUCCESS );

            IDE_TEST(recvValueA7(aAllocator,
                                 aExitFlag,
                                 aReadContext,
                                 NULL, /* aMemory */
                                 &sDummy,
                                 aTimeOutSec,
                                 NULL)
                     != IDE_SUCCESS);
            if(sDummy.value != NULL)
            {
                (void)iduMemMgr::free((void *)sDummy.value, aAllocator);
            }

            IDE_TEST(recvValueA7(aAllocator,
                                 aExitFlag,
                                 aReadContext,
                                 NULL, /* aMemory */
                                 &sDummy,
                                 aTimeOutSec,
                                 NULL)
                     != IDE_SUCCESS);
            if(sDummy.value != NULL)
            {
                (void)iduMemMgr::free((void *)sDummy.value, aAllocator);
            }

            i--;
            aXLog->mColCnt--;
        }
    
        IDU_FIT_POINT_RAISE( "rpnComm::recvUpdateA7::Erratic::rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE",
                             ERR_NOT_FOUND_TABLE );
        IDE_RAISE(ERR_NOT_FOUND_TABLE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                aXLog->mSN,
                                aXLog->mTableOID));
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvUpdateA7",
                                "Column"));
    }
    IDE_EXCEPTION(ERR_NOT_FOUND_COLUMN);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_COLUMN,
                                aXLog->mSN,
                                aXLog->mTableOID,
                                i));
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDeleteA7( void               *aHBTResource,
                              cmiProtocolContext *aProtocolContext,
                              idBool            * aExitFlag,
                              smTID               aTID,
                              smSN                aSN,
                              smSN                aSyncSN,
                              UInt                aImplSPDepth,
                              ULong               aTableOID,
                              UInt                aPKColCnt,
                              smiValue          * aPKCols,
                              rpValueLen        * aPKLen,
                              UInt                aColCnt,
                              UInt              * aCIDs,
                              smiValue          * aBCols,
                              smiChainedValue   * aBChainedCols, // PROJ-1705
                              rpValueLen        * aBMtdValueLen,
                              UInt              * aBChainedColsTotalLen,
                              UInt                aTimeoutSec )
{
    UInt i;
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType= RP_X_DELETE;

    /* Validate Parameters */
    IDE_DASSERT( aImplSPDepth <= SMI_STATEMENT_DEPTH_MAX );
    IDE_DASSERT( aPKColCnt <= QCI_MAX_KEY_COLUMN_COUNT );
    IDE_DASSERT( aPKCols != NULL );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 44, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Delete );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information     */
    CMI_WR4( aProtocolContext, &(aImplSPDepth) );
    CMI_WR8( aProtocolContext, &(aTableOID) );
    CMI_WR4( aProtocolContext, &(aPKColCnt) );
    CMI_WR4( aProtocolContext, &(aColCnt) );

    /* Send Primary Key Value */
    for(i = 0; i < aPKColCnt; i ++)
    {
        IDE_TEST( sendPKValueForA7( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    &aPKCols[i],
                                    aPKLen[i],
                                    aTimeoutSec )
                  != IDE_SUCCESS );
    }

    /* Send old values */
    for ( i = 0; i < aColCnt; i++ )
    {
        IDE_TEST( sendCIDForA7( aHBTResource,
                                aProtocolContext,
                                aExitFlag,
                                aCIDs[i],
                                aTimeoutSec )
                  != IDE_SUCCESS );

        /*
         * 메모리 테이블에 발생한 update의 분석 결과는 BCols에 들어있다.
         * mAllocMethod의 초기값은 SMI_NON_ALLOCED
         */
        if ( ( aBChainedCols[aCIDs[i]].mAllocMethod == SMI_NON_ALLOCED ) &&
             ( aBMtdValueLen[aCIDs[i]].lengthSize == 0 ) )
        {
            IDE_TEST( sendValueForA7( aHBTResource,
                                      aProtocolContext,
                                      aExitFlag,
                                      &aBCols[aCIDs[i]],
                                      aBMtdValueLen[aCIDs[i]],
                                      aTimeoutSec )
                      != IDE_SUCCESS );
        }
        // 디스크 테이블에 발생한 update의 분석 결과는 BChainedCols에 들어있다.
        else
        {
            IDE_TEST( sendChainedValueForA7( aHBTResource,
                                             aProtocolContext,
                                             aExitFlag,
                                             &aBChainedCols[aCIDs[i]],
                                             aBMtdValueLen[aCIDs[i]],
                                             aBChainedColsTotalLen[aCIDs[i]],
                                             aTimeoutSec )
                     != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDeleteA7( iduMemAllocator     * aAllocator,
                              idBool              * aExitFlag,
                              rpxReceiverReadContext aReadContext,
                              rpdMeta             * aMeta,
                              rpdXLog             * aXLog,
                              ULong                 aTimeOutSec )
{
    UInt i;
    UInt * sType = (UInt*)&(aXLog->mType);

    rpdMetaItem    * sItem = NULL;
    smiValue         sDummy;

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        /* Get Argument XLog Hdr */
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        /* Get Argument XLog Body */
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mImplSPDepth) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mTableOID) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mPKColCnt) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mColCnt) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),        4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),         4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),          8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),      8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mImplSPDepth), 4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTableOID),    8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mPKColCnt),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mColCnt),      4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mPKColCnt * ID_SIZEOF( smiValue ),
                                          (void **)&( aXLog->mPKCols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    if ( aXLog->mColCnt > 0 )
    {
        IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mColCnt * ID_SIZEOF( UInt ),
                                              (void **)&( aXLog->mCIDs ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        IDE_TEST_RAISE( aXLog->mMemory.alloc(  aXLog->mColCnt * ID_SIZEOF( smiValue ),
                                               (void **)&( aXLog->mBCols ) )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    }
    else
    {
        /* nothing to do */
    }

    /* Recv PK Value repeatedly */
    for( i = 0; i < aXLog->mPKColCnt; i ++ )
    {
        IDE_TEST(recvValueA7(aAllocator,
                             aExitFlag,
                             aReadContext,
                             &aXLog->mMemory,
                             &aXLog->mPKCols[i],
                             aTimeOutSec,
                             NULL)
                 != IDE_SUCCESS);
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * XLog 내의 Column 정보를 재배열한다.
     *   - mPKColCnt는 변하지 않으므로, 제외한다.
     *   - Primary Key의 순서가 같으므로, mPKCIDs[]와 mPKCols[]는 제외한다.
     *
     * Column : mColCnt, mCIDs[], mBCols[], mACols[]
     */
    (void)aMeta->searchRemoteTable(&sItem, aXLog->mTableOID);

    if ( sItem != NULL )
    {
        /* Receive old values */
        for ( i = 0; i < aXLog->mColCnt; i++ )
        {
            IDE_TEST( recvCIDA7( aExitFlag,
                                 aReadContext,
                                 &aXLog->mCIDs[i],
                                 aTimeOutSec )
                      != IDE_SUCCESS );

            aXLog->mCIDs[i] = sItem->mMapColID[aXLog->mCIDs[i]];

            if ( aXLog->mCIDs[i] != RP_INVALID_COLUMN_ID )
            {
                IDE_TEST( recvValueA7( aAllocator,
                                       aExitFlag,
                                       aReadContext,
                                       &aXLog->mMemory,
                                       &aXLog->mBCols[i],
                                       aTimeOutSec,
                                       NULL )
                          != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST( recvValueA7( aAllocator,
                                       aExitFlag,
                                       aReadContext,
                                       NULL, /* aMemory */
                                       &sDummy,
                                       aTimeOutSec,
                                       NULL )
                          != IDE_SUCCESS);
                if ( sDummy.value != NULL )
                {
                    (void)iduMemMgr::free( (void *)sDummy.value, aAllocator );
                }

                i--;
                aXLog->mColCnt--;
            }
        }
    }
    else
    {
        /* Receive old values */
        for ( i = 0; i < aXLog->mColCnt; i++ )
        {
            IDE_TEST( recvCIDA7( aExitFlag,
                                 aReadContext,
                                 &aXLog->mCIDs[i],
                                 aTimeOutSec )
                      != IDE_SUCCESS );



            IDE_TEST( recvValueA7( aAllocator,
                                   aExitFlag,
                                   aReadContext,
                                   NULL, /* aMemory */
                                   &sDummy,
                                   aTimeOutSec,
                                   NULL )
                      != IDE_SUCCESS);
            if ( sDummy.value != NULL )
            {
                (void)iduMemMgr::free( (void *)sDummy.value, aAllocator );
            }

            i--;
            aXLog->mColCnt--;
        }
        IDU_FIT_POINT_RAISE( "rpnComm::recvDeleteA7::Erratic::rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE",
                             ERR_NOT_FOUND_TABLE );
        IDE_RAISE(ERR_NOT_FOUND_TABLE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                aXLog->mSN,
                                aXLog->mTableOID));
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvDeleteA7",
                                "Column"));
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendCIDForA7( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              UInt                 aCID,
                              UInt                 aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    /* Validate Parameters */
    IDE_DASSERT( aCID < QCI_MAX_COLUMN_COUNT );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 4, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, UIntID );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(aCID) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvCIDA7( idBool              * aExitFlag,
                           rpxReceiverReadContext aReadContext,
                           UInt                * aCID,
                           ULong                 aTimeoutSec)
{
    UChar sOpCode;

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        if ( CMI_IS_READ_ALL( aReadContext.mCMContext ) == ID_TRUE )
        {
            IDE_TEST( readCmBlock( NULL, aReadContext.mCMContext, aExitFlag, NULL /* TimeoutFlag */, aTimeoutSec )
                      != IDE_SUCCESS );
        }
        CMI_RD1( aReadContext.mCMContext, sOpCode );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        IDE_TEST( aReadContext.mXLogfileContext->checkRemainXLog() != IDE_SUCCESS );
        (void)aReadContext.mXLogfileContext->readXLog(&sOpCode, 1);


    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, UIntID ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Argument */
    if( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, aCID );
    }
    else if (aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog(aCID, 4);
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTxAckA7( cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             UInt                 aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( checkAndFlush( NULL,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 12, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, TxAck );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTxAckForA7( idBool             * aExitFlag,
                                cmiProtocolContext * aProtocolContext,
                                smTID              * aTID,
                                smSN               * aSN,
                                ULong                aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, TxAck ),
                    ERR_CHECK_OPERATION_TYPE );
    /* Get Argument */
    CMI_RD4( aProtocolContext, aTID );
    CMI_RD8( aProtocolContext, aSN );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_CHECK_OPERATION_TYPE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                sOpCode));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendValueForA7( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smiValue           * aValue,
                                rpValueLen           aValueLen,
                                UInt                 aTimeoutSec )
{
    UChar sOneByteLenValue;
    UInt sRemainSpaceInCmBlock;
    UInt sRemainDataOfValue;
    UInt sLength;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    /* Validate Parameters */
    IDE_DASSERT( aValue != NULL );

    /* check : OpCode (1) + value length(4)
     * value가 한 블럭에 쓰이는지 나눠지는지 판단하기 위해서는 length정보를 볼 수 있어야하므로
     * 이 정보가 나눠질 것 같으면 그냥 새 cmBlock에 써야한다
     */
    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4 + 2,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Value );
    CMI_WR1( aProtocolContext, sOpCode );

    /* Set Communication Argument : mtdValueLength + value */

    /* Send mtdValueLength : 이 값은 value와 하나의 데이터로 취급하여 전송되어야한다.
     * 따라서 마샬링 처리 없이 CMI_WCP로 그냥 복사하도록 한다.
     * memoryTable, non-divisible value 또는 null value인 경우, rpValueLen정보가 초기값이다.
     */
    if ( aValueLen.lengthSize > 0 )
    {
        /* Value length Set */
        sLength = (UInt)( aValueLen.lengthSize + aValueLen.lengthValue );
        CMI_WR4( aProtocolContext, &sLength );

        /* UShort를 UChar 포인터로 1바이트만 보내는 경우, endian문제가 발생한다.
         * UChar 변수에 담는 과정을 거치도록한다.
         */
        if ( aValueLen.lengthSize == 1 )
        {
            sOneByteLenValue = (UChar)aValueLen.lengthValue;
            CMI_WCP( aProtocolContext, (UChar*)&sOneByteLenValue, 1 );
        }
        else
        {
            // BUGBUG  lengthValue 정보에 1 또는 2 byte 이외의 크기가 존재할까?
            CMI_WCP( aProtocolContext, (UChar*)&aValueLen.lengthValue, aValueLen.lengthSize );
        }
    }
    else
    {
        /* Value length Set */
        sLength = aValue->length;
        CMI_WR4( aProtocolContext, &sLength );
    }

    sRemainDataOfValue = aValue->length;
    sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );

    while ( sRemainDataOfValue > 0 )
    {
        if ( sRemainSpaceInCmBlock == 0 )
        {
            IDU_FIT_POINT( "rpnComm::sendValue::cmiSend::ERR_SEND" );
            IDE_TEST( sendCmBlock( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   ID_FALSE,
                                   aTimeoutSec )
                      != IDE_SUCCESS );
            sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
        }
        else
        {
            if ( sRemainSpaceInCmBlock >= sRemainDataOfValue )
            {
                CMI_WCP( aProtocolContext,
                         (UChar *)aValue->value + ( aValue->length - sRemainDataOfValue ),
                         sRemainDataOfValue );
                sRemainDataOfValue = 0;
            }
            else
            {
                CMI_WCP( aProtocolContext,
                         (UChar *)aValue->value + ( aValue->length - sRemainDataOfValue ),
                         sRemainSpaceInCmBlock );
                sRemainDataOfValue = sRemainDataOfValue - sRemainSpaceInCmBlock;
            }
            sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendPKValueForA7( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  smiValue           * aValue,
                                  rpValueLen           aPKLen,
                                  UInt                 aTimeoutSec )
{
    UChar sOneByteLenValue;
    UInt sRemainSpaceInCmBlock;
    UInt sRemainDataOfValue;
    UInt sLength;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    /* Validate Parameters */
    IDE_DASSERT( aValue != NULL );

    /*
     * PROJ-1705로 인해,
     * divisible data type인 경우, null value이면, 앞에 mtdLength만큼의 길이를 length로 가지나,
     * value에서 그 길이 만큼의 데이터를 저장하고 있지는 않으므로,
     * (rpLenSize 에서 그 값을 가지고 있다.) 다른 함수에서는 동일한 내용의 ASSERT를 해제하였으나,
     * PK Value는 null이 올 수 없으므로 유지시킴.
     */
    IDE_DASSERT( (aValue->value != NULL) || (aValue->length == 0) );

    /* check : OpCode (1) + value length(4)
     * value가 한 블럭에 쓰이는지 나눠지는지 판단하기 위해서는 length정보를 볼 수 있어야하므로
     * 이 정보가 나눠질 것 같으면 그냥 새 cmBlock에 써야한다
     */
    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4 + 2,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Value );
    CMI_WR1( aProtocolContext, sOpCode );

    /* Set Communication Argument */

    /* Send mtdValueLength : 이 값은 value와 하나의 데이터로 취급하여 전송되어야한다.
     * 따라서 마샬링 처리 없이 CMI_WCP로 그냥 복사하도록 한다.
     * memoryTable, non-divisible value 또는 null value인 경우, rpValueLen정보가 초기값이다.
     */
    if( aPKLen.lengthSize > 0 )
    {
        /* Value length Set */
        sLength = (UInt)( aPKLen.lengthSize + aPKLen.lengthValue );
        CMI_WR4( aProtocolContext, &sLength );

        // UShort를 UChar 포인터로 1바이트만 보내는 경우, endian문제가 발생한다.
        // UChar 변수에 담는 과정을 거치도록한다.
        if( aPKLen.lengthSize == 1 )
        {
            sOneByteLenValue = (UChar)aPKLen.lengthValue;
            CMI_WCP( aProtocolContext, (UChar*)&sOneByteLenValue, 1 );
        }
        else
        {
            // BUGBUG  lengthValue 정보에 1 또는 2 byte 이외의 크기가 존재할까?
            CMI_WCP( aProtocolContext, (UChar*)&aPKLen.lengthValue, aPKLen.lengthSize);
        }
    }
    else
    {
        /* Value length Set */
        sLength = aValue->length;
        CMI_WR4( aProtocolContext, &sLength );
    }

    sRemainDataOfValue = aValue->length;
    sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
    while ( sRemainDataOfValue > 0 )
    {
        if ( sRemainSpaceInCmBlock == 0 )
        {
            IDU_FIT_POINT( "rpnComm::sendPKValue::cmiSend::ERR_SEND" );
            IDE_TEST( sendCmBlock( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   ID_FALSE,
                                   aTimeoutSec )
                      != IDE_SUCCESS );
            sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
        }
        else
        {
            if ( sRemainSpaceInCmBlock >= sRemainDataOfValue )
            {
                CMI_WCP( aProtocolContext,
                         (UChar *)aValue->value + ( aValue->length - sRemainDataOfValue ),
                         sRemainDataOfValue );
                sRemainDataOfValue = 0;
            }
            else
            {
                CMI_WCP( aProtocolContext,
                         (UChar *)aValue->value + ( aValue->length - sRemainDataOfValue ),
                         sRemainSpaceInCmBlock );
                sRemainDataOfValue = sRemainDataOfValue - sRemainSpaceInCmBlock;
            }
            sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendChainedValueForA7( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       smiChainedValue    * aChainedValue,
                                       rpValueLen           aBMtdValueLen,
                                       UInt                 aChainedValueLen,
                                       UInt                 aTimeoutSec )
{
    smiChainedValue * sChainedValue = NULL;
    UChar sOneByteLenValue;
    UInt sRemainSpaceInCmBlock;
    UInt sRemainDataOfValue;
    UInt sLength;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    /* Validate Parameters */
    IDE_DASSERT(aChainedValue != NULL);

    sChainedValue = aChainedValue;

    /* check : OpCode (1) + value length(4)
     * value가 한 블럭에 쓰이는지 나눠지는지 판단하기 위해서는 length정보를 볼 수 있어야하므로
     * 이 정보가 나눠질 것 같으면 그냥 새 cmBlock에 써야한다
     */
    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4 + 2,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );
    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Value );
    CMI_WR1( aProtocolContext, sOpCode );

    /* Set Communication Argument : mtdValueLength + value */

    /* Send mtdValueLength : 이 값은 value와 하나의 데이터로 취급하여 전송되어야한다.
     * 따라서 마샬링 처리 없이 CMI_WCP로 그냥 복사하도록 한다.
     * memoryTable, non-divisible value 또는 null value인 경우, rpValueLen정보가 초기값이다.
     */

    if ( aBMtdValueLen.lengthSize > 0 )
    {
        /* Value length Set */
        sLength = (UInt)( aBMtdValueLen.lengthSize + aBMtdValueLen.lengthValue );
        CMI_WR4( aProtocolContext, &sLength );

        /*
         * UShort를 UChar 포인터로 1바이트만 보내는 경우, endian문제가 발생한다.
         * UChar 변수에 담는 과정을 거치도록한다.
         */
        if ( aBMtdValueLen.lengthSize == 1 )
        {
            sOneByteLenValue = (UChar)aBMtdValueLen.lengthValue;
            CMI_WCP( aProtocolContext, (UChar*)&sOneByteLenValue, 1 );
        }
        else
        {
            // BUGBUG  lengthValue 정보에 1 또는 2 byte 이외의 크기가 존재할까?
            CMI_WCP( aProtocolContext, (UChar*)&aBMtdValueLen.lengthValue, aBMtdValueLen.lengthSize );
        }
    }
    else
    {
        /* Value length Set */
        sLength = aChainedValueLen; /* BUG-33722 */
        CMI_WR4( aProtocolContext, &sLength );
    }

    sRemainDataOfValue = sChainedValue->mColumn.length;
    sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
    while ( sRemainDataOfValue > 0 )
    {
        /* mColumn.length가 0보다 큰데, value는 NULL일 수 있나? */
        IDE_ASSERT( sChainedValue->mColumn.value != NULL );

        if ( sRemainSpaceInCmBlock == 0 )
        {
            IDU_FIT_POINT( "rpnComm::sendChainedValue::cmiSend::ERR_SEND" );
            IDE_TEST( sendCmBlock( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   ID_FALSE,
                                   aTimeoutSec )
                      != IDE_SUCCESS );
            sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
        }
        else
        {
            if ( sRemainSpaceInCmBlock >= sRemainDataOfValue )
            {
                CMI_WCP( aProtocolContext,
                         (UChar *)sChainedValue->mColumn.value + ( sChainedValue->mColumn.length - sRemainDataOfValue ),
                         sRemainDataOfValue );
                sChainedValue = sChainedValue->mLink;
                if ( sChainedValue == NULL )
                {
                    break;
                }
                else
                {
                    sRemainDataOfValue = sChainedValue->mColumn.length;
                }
            }
            else
            {
                CMI_WCP( aProtocolContext,
                         (UChar *)sChainedValue->mColumn.value + ( sChainedValue->mColumn.length - sRemainDataOfValue ),
                         sRemainSpaceInCmBlock );
                sRemainDataOfValue = sRemainDataOfValue - sRemainSpaceInCmBlock;
            }
            sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvValueA7( iduMemAllocator     * aAllocator,
                             idBool              * aExitFlag,
                             rpxReceiverReadContext aReadContext,
                             iduMemory           * aMemory,
                             smiValue            * aValue,
                             ULong                 aTimeOutSec,
                             mtcColumn           * aMtcColumn )
{
    UChar   sOpCode               = CMI_PROTOCOL_OPERATION( RP, MAX );
    UInt    sRemainDataInCmBlock  = 0;
    UInt    sRemainDataOfValue    = 0;
    idBool  sIsAllocByiduMemMgr   = ID_FALSE;
    UInt    sStartBufferPoint     = 0;
    UInt    sRecvValueLength      = 0;
    SLong   sLobLength            = 0;

    aValue->value = NULL;

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        if ( CMI_IS_READ_ALL( aReadContext.mCMContext ) == ID_TRUE )
        {
            IDE_TEST( readCmBlock( NULL, aReadContext.mCMContext, aExitFlag, NULL /* TimeoutFlag */, aTimeOutSec )
                      != IDE_SUCCESS );
        }
        CMI_RD1( aReadContext.mCMContext, sOpCode );

    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        IDE_TEST( readXLogfile( aReadContext.mXLogfileContext ) != IDE_SUCCESS );
        (void)aReadContext.mXLogfileContext->readXLog(&sOpCode, 1);
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, Value ),
                    ERR_CHECK_OPERATION_TYPE );

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, &(sRecvValueLength) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(sRecvValueLength), 4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    /* BUG-47458 SQL Apply Lob Support */
    if ( ( aMtcColumn != NULL ) && ( sRecvValueLength != 0 ) )
    {
        if ( ( aMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_LOB )
        {
            aValue->length = sRecvValueLength - SMI_LOB_DUMMY_HEADER_LEN + RP_LOB_MTD_HEADER_SIZE;
            sStartBufferPoint = RP_LOB_MTD_HEADER_SIZE - SMI_LOB_DUMMY_HEADER_LEN;
        }
        else
        {
            aValue->length = sRecvValueLength;
            sStartBufferPoint = 0;
        }
    }
    else
    {
        aValue->length = sRecvValueLength;
        sStartBufferPoint = 0;
    }

    /* NOTICE!!!!!
     * 실제 Value에 대한 값을 저장하는 메모리 공간을 여기서 할당받는다.
     * 따라서, 이 XLog를 사용한 쪽(ReceiverApply::execXLog)에서
     * 현재 XLog를 사용하였으면, 여기서 할당해준 메모리 공간을
     * 해제해 주어야 한다.
     * 이렇게 Value를 위한 메모리를 할당하는 Protocol은
     * Insert, Update, Delete, LOB cursor open이 있다.
     */
    if( sRecvValueLength > 0 )
    {
        /* NULL value가 전송되는 경우에는 length가 0으로 넘어오게
         * 되므로, 이 경우에는 메모리 할당을 하지 않도록 한다. */
        IDU_FIT_POINT_RAISE( "rpnComm::recvValueA7::malloc::Value", 
                              ERR_MEMORY_ALLOC_VALUE );
        if ( aMemory != NULL )
        {
            IDE_TEST_RAISE( aMemory->alloc( aValue->length,
                                            (void **)&aValue->value )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOC_VALUE );
        }
        else
        {
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPN,
                                               aValue->length,
                                               (void **)&aValue->value,
                                               IDU_MEM_IMMEDIATE,
                                               aAllocator )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOC_VALUE );
            sIsAllocByiduMemMgr = ID_TRUE;
        }

        sRemainDataOfValue = sRecvValueLength;
        if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
        {
            sRemainDataInCmBlock = CMI_REMAIN_DATA_IN_READ_BLOCK( aReadContext.mCMContext );
        }
        else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
        {
            IDE_TEST( aReadContext.mXLogfileContext->getRemainSize( &sRemainDataInCmBlock ) != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE( ERR_INTERNAL );
        }

        while ( sRemainDataOfValue > 0 )
        {
            if ( sRemainDataInCmBlock == 0 )
            {
                if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
                {
                    IDE_TEST( readCmBlock( NULL, aReadContext.mCMContext, aExitFlag, NULL /* TimeoutFlag */, aTimeOutSec )
                            != IDE_SUCCESS );
                    sRemainDataInCmBlock = CMI_REMAIN_DATA_IN_READ_BLOCK( aReadContext.mCMContext );
                }
                else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
                {
                    IDE_TEST( aReadContext.mXLogfileContext->getRemainSize( &sRemainDataInCmBlock) != IDE_SUCCESS );
                }
                else
                {
                    IDE_RAISE( ERR_INTERNAL );
                }
            }
            else
            {
                if ( sRemainDataInCmBlock >= sRemainDataOfValue )
                {
                    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
                    {
                        CMI_RCP( aReadContext.mCMContext,
                                (UChar *)aValue->value + ( sRecvValueLength + sStartBufferPoint - sRemainDataOfValue ),
                                sRemainDataOfValue );
                    }
                    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
                    {
                        (void)aReadContext.mXLogfileContext->readXLog((UChar *)aValue->value + ( sRecvValueLength + sStartBufferPoint - sRemainDataOfValue ),
                                                                       sRemainDataOfValue,
                                                                       ID_FALSE );
                    }
                    else
                    {
                        IDE_RAISE( ERR_INTERNAL );
                    }

                    sRemainDataOfValue = 0;
                }
                else
                {
                    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
                    {
                        CMI_RCP( aReadContext.mCMContext,
                                (UChar *)aValue->value + ( sRecvValueLength + sStartBufferPoint - sRemainDataOfValue ),
                                sRemainDataInCmBlock );
                    }
                    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
                    {
                        (void)aReadContext.mXLogfileContext->readXLog((UChar *)aValue->value + ( sRecvValueLength + sStartBufferPoint - sRemainDataOfValue ),
                                                                      sRemainDataInCmBlock,
                                                                      ID_FALSE);
                    }
                    else
                    {
                        IDE_RAISE( ERR_INTERNAL );
                    }

                    sRemainDataOfValue = sRemainDataOfValue - sRemainDataInCmBlock;
                }
                if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
                {
                    sRemainDataInCmBlock = CMI_REMAIN_DATA_IN_READ_BLOCK( aReadContext.mCMContext );
                }
                else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
                {
                    IDE_TEST( aReadContext.mXLogfileContext->getRemainSize( &sRemainDataInCmBlock ) != IDE_SUCCESS );
                }
                else
                {
                    IDE_RAISE( ERR_INTERNAL );
                }
            }
        }

        if ( aMtcColumn != NULL )
        {
            /* BUG-47458 SQL Apply Lob Support */
            if ( ( aMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_LOB )
            {
                sLobLength = aValue->length - RP_LOB_MTD_HEADER_SIZE;
                idlOS::memcpy( (void *)( aValue->value ),
                               (void *)&sLobLength,
                               ID_SIZEOF( SLong ) );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_VALUE);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvValueA7",
                                "aValue->value"));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    if ( sIsAllocByiduMemMgr == ID_TRUE )
    {
        (void)iduMemMgr::free((void *)aValue->value, aAllocator);
        aValue->value = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendStopA7( void               * aHBTResource,
                            cmiProtocolContext * aProtocolContext,
                            idBool             * aExitFlag,
                            smTID                aTID,
                            smSN                 aSN,
                            smSN                 aSyncSN,
                            smSN                 aRestartSN,
                            UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 32, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    sType = RP_X_REPL_STOP;

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Stop );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    CMI_WR8( aProtocolContext, &(aRestartSN) );

    IDU_FIT_POINT( "rpnComm::sendStop::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvStopA7( rpxReceiverReadContext aReadContext,
                            idBool              * /*aExitFlag*/,
                            rpdXLog             * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    /* Get Argument XLog Hdr */
    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mRestartSN) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),      4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),       4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),        8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),    8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mRestartSN), 8 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendKeepAliveA7( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 smSN                 aRestartSN,
                                 UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_KEEP_ALIVE;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 32, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, KeepAlive );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    CMI_WR8( aProtocolContext, &(aRestartSN) );

    IDU_FIT_POINT( "rpnComm::sendKeepAlive::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvKeepAliveA7( rpxReceiverReadContext aReadContext,
                                 idBool              * /* aExitFlag */,
                                 rpdXLog             * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    /* Get Argument XLog Hdr */
    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mRestartSN) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),      4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),       4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),        8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),    8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mRestartSN), 8 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendFlushA7( void               * /*aHBTResource*/,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * /*aExitFlag*/,
                             smTID                aTID,
                             smSN                 aSN,
                             smSN                 aSyncSN,
                             UInt                 aFlushOption,
                             UInt                 /*aTimeoutSec*/ )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_FLUSH;

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Flush );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    CMI_WR4( aProtocolContext, &(aFlushOption) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvFlushA7( rpxReceiverReadContext aReadContext,
                             idBool              * /* aExitFlag */,
                             rpdXLog             * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mFlushOption) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),        4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),         4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),          8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),      8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mFlushOption), 4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendAckA7( cmiProtocolContext * aProtocolContext,
                           idBool             * aExitFlag,
                           rpXLogAck          * aAck,
                           UInt                 aTimeoutSec )
{
    UInt i = 0;
    UChar sOpCode;

    IDE_TEST( checkAndFlush( NULL,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 52, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Ack );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(aAck->mAckType) );
    CMI_WR4( aProtocolContext, &(aAck->mAbortTxCount) );
    CMI_WR4( aProtocolContext, &(aAck->mClearTxCount) );
    CMI_WR8( aProtocolContext, &(aAck->mRestartSN) );
    CMI_WR8( aProtocolContext, &(aAck->mLastCommitSN) );
    CMI_WR8( aProtocolContext, &(aAck->mLastArrivedSN) );
    CMI_WR8( aProtocolContext, &(aAck->mLastProcessedSN) );
    CMI_WR8( aProtocolContext, &(aAck->mFlushSN) );

    IDU_FIT_POINT( "1.TASK-2004@rpnComm::sendAckA7" );

    for(i = 0; i < aAck->mAbortTxCount; i ++)
    {
        IDE_TEST( sendTxAckA7( aProtocolContext,
                               aExitFlag,
                               aAck->mAbortTxList[i].mTID,
                               aAck->mAbortTxList[i].mSN,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }

    for(i = 0; i < aAck->mClearTxCount; i ++)
    {
        IDE_TEST( sendTxAckA7( aProtocolContext,
                               aExitFlag,
                               aAck->mClearTxList[i].mTID,
                               aAck->mClearTxList[i].mSN,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }

    IDU_FIT_POINT( "rpnComm::sendAckA7::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( NULL,    /* aHBTResource */
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvAckA7( iduMemAllocator    * /*aAllocator*/,
                           void               * aHBTResource,
                           cmiProtocolContext * aProtocolContext,
                           idBool             * aExitFlag,
                           rpXLogAck          * aAck,
                           ULong                aTimeoutSec,
                           idBool             * aIsTimeoutSec )
{
    ULong i;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    *aIsTimeoutSec = ID_FALSE;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( aHBTResource,
                               aProtocolContext,
                               aExitFlag,
                               aIsTimeoutSec,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }

    IDU_FIT_POINT_RAISE( "1.TASK-2004@rpnComm::recvAck", 
                          ERR_EXIT );

    if( *aIsTimeoutSec == ID_FALSE )
    {
        /* Check Operation Type */
        CMI_RD1( aProtocolContext, sOpCode );

        IDE_TEST_RAISE( ( sOpCode != CMI_PROTOCOL_OPERATION( RP, Ack ) ) &&
                        ( sOpCode != CMI_PROTOCOL_OPERATION( RP, AckEager ) ),
                        ERR_CHECK_OPERATION_TYPE );
        /* Get argument */
        CMI_RD4( aProtocolContext, &(aAck->mAckType) );
        CMI_RD4( aProtocolContext, &(aAck->mAbortTxCount) );
        CMI_RD4( aProtocolContext, &(aAck->mClearTxCount) );
        CMI_RD8( aProtocolContext, &(aAck->mRestartSN) );
        CMI_RD8( aProtocolContext, &(aAck->mLastCommitSN) );
        CMI_RD8( aProtocolContext, &(aAck->mLastArrivedSN) );
        CMI_RD8( aProtocolContext, &(aAck->mLastProcessedSN) );
        CMI_RD8( aProtocolContext, &(aAck->mFlushSN) );

        if ( sOpCode == CMI_PROTOCOL_OPERATION( RP, AckEager ) )
        {
            CMI_RD4( aProtocolContext, &(aAck->mTID) );
        }
        else
        {
            aAck->mTID = SM_NULL_TID;
        }

        /* Get Abort Tx List */
        for(i = 0; i < aAck->mAbortTxCount; i++)
        {
            IDE_TEST(recvTxAckForA7(aExitFlag,
                                    aProtocolContext,
                                    &(aAck->mAbortTxList[i].mTID),
                                    &(aAck->mAbortTxList[i].mSN),
                                    aTimeoutSec)
                     != IDE_SUCCESS);
        }

        /* Get Clear Tx List */
        for(i = 0; i < aAck->mClearTxCount; i++)
        {
            IDE_TEST(recvTxAckForA7(aExitFlag,
                                    aProtocolContext,
                                    &(aAck->mClearTxList[i].mTID),
                                    &(aAck->mClearTxList[i].mSN),
                                    aTimeoutSec)
                     != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
    }
    IDE_EXCEPTION(ERR_EXIT);
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_IGNORE_EXIT_FLAG_SET));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobCursorOpenA7( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     smTID                aTID,
                                     smSN                 aSN,
                                     smSN                 aSyncSN,
                                     ULong                aTableOID,
                                     ULong                aLobLocator,
                                     UInt                 aLobColumnID,
                                     UInt                 aPKColCnt,
                                     smiValue           * aPKCols,
                                     rpValueLen         * aPKLen,
                                     UInt                 aTimeoutSec )
{
    UInt i;
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_LOB_CURSOR_OPEN;

    /* Validate Parameters */
    IDE_DASSERT(aLobColumnID < QCI_MAX_COLUMN_COUNT);
    IDE_DASSERT(aPKColCnt <= QCI_MAX_KEY_COLUMN_COUNT);
    IDE_DASSERT(aPKCols != NULL);

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 48, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, LobCursorOpen );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information */
    CMI_WR8( aProtocolContext, &(aTableOID) );
    CMI_WR8( aProtocolContext, &(aLobLocator) );
    CMI_WR4( aProtocolContext, &(aLobColumnID) );
    CMI_WR4( aProtocolContext, &(aPKColCnt) );

    /* Send Primary Key Value */
    for(i = 0; i < aPKColCnt; i ++)
    {
        IDE_TEST( sendPKValueForA7( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    &aPKCols[i],
                                    aPKLen[i],
                                    aTimeoutSec )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvLobCursorOpenA7( iduMemAllocator     * aAllocator,
                                     idBool              * aExitFlag,
                                     rpxReceiverReadContext aReadContext,
                                     rpdMeta             * aMeta,  // BUG-20506
                                     rpdXLog             * aXLog,
                                     ULong                 aTimeOutSec )
{
    UInt                    i;
    rpdMetaItem            *sItem = NULL;
    UInt                    sLobColumnID;
    UInt * sType = (UInt*)&(aXLog->mType);

    IDE_ASSERT(aMeta != NULL);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        /* Get Argument XLog Hdr */
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        /* Get Argument XLog Body */
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mTableOID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mLobPtr->mLobLocator) );
        CMI_RD4( aReadContext.mCMContext, &(sLobColumnID) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mPKColCnt) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),                4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),                 4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),                  8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),              8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTableOID),            8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mLobPtr->mLobLocator), 8 );
        aReadContext.mXLogfileContext->readXLog( &(sLobColumnID),                4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mPKColCnt),            4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mPKColCnt * ID_SIZEOF( smiValue ),
                                          (void **)&( aXLog->mPKCols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    /* Recv PK Value repeatedly */
    for ( i = 0; i < aXLog->mPKColCnt; i ++ )
    {
        IDE_TEST( recvValueA7( aAllocator,
                               aExitFlag,
                               aReadContext,
                               &aXLog->mMemory,
                               &aXLog->mPKCols[i],
                               aTimeOutSec,
                               NULL )
                  != IDE_SUCCESS );
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * XLog 내의 Column 정보를 재배열한다.
     *   - mPKColCnt는 변하지 않으므로, 제외한다.
     *   - Primary Key의 순서가 같으므로, mPKCIDs[]와 mPKCols[]는 제외한다.
     *
     * LOB    : mLobColumnID
     */
    (void)aMeta->searchRemoteTable(&sItem, aXLog->mTableOID);

    IDE_TEST_RAISE(sItem == NULL, ERR_NOT_FOUND_TABLE);

    aXLog->mLobPtr->mLobColumnID = sItem->mMapColID[sLobColumnID];

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                aXLog->mSN,
                                aXLog->mTableOID));
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvLobCursorOpenA7",
                                "Column"));
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobCursorCloseA7( void                * aHBTResource,
                                      cmiProtocolContext  * aProtocolContext,
                                      idBool              * aExitFlag,
                                      smTID                 aTID,
                                      smSN                  aSN,
                                      smSN                  aSyncSN,
                                      ULong                 aLobLocator,
                                      UInt                  aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_LOB_CURSOR_CLOSE;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 32, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, LobCursorClose );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information     */
    CMI_WR8( aProtocolContext, &(aLobLocator) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvLobCursorCloseA7( rpxReceiverReadContext aReadContext,
                                      idBool              * /* aExitFlag */,
                                      rpdXLog             * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        /* Get Argument XLog Hdr */
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        /* Get Lob Information */
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mLobPtr->mLobLocator) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),                4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),                 4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),                  8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),              8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mLobPtr->mLobLocator), 8 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobPrepare4WriteA7( void               * aHBTResource,
                                        cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        smTID                aTID,
                                        smSN                 aSN,
                                        smSN                 aSyncSN,
                                        ULong                aLobLocator,
                                        UInt                 aLobOffset,
                                        UInt                 aLobOldSize,
                                        UInt                 aLobNewSize,
                                        UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    sType = RP_X_LOB_PREPARE4WRITE;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 44, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, LobPrepare4Write );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information */
    CMI_WR8( aProtocolContext, &(aLobLocator) );
    CMI_WR4( aProtocolContext, &(aLobOffset) );
    CMI_WR4( aProtocolContext, &(aLobOldSize) );
    CMI_WR4( aProtocolContext, &(aLobNewSize) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvLobPrepare4WriteA7( rpxReceiverReadContext aReadContext,
                                        idBool              * /*aExitFlag*/,
                                        rpdXLog             * aXLog )
{
    UInt   * sType   = (UInt*)&(aXLog->mType);
    rpdLob * sLobPtr = aXLog->mLobPtr;

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        /* Get Argument XLog Hdr */
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        /* Get Lob Information */
        CMI_RD8( aReadContext.mCMContext, &(sLobPtr->mLobLocator) );
        CMI_RD4( aReadContext.mCMContext, &(sLobPtr->mLobOffset) );
        CMI_RD4( aReadContext.mCMContext, &(sLobPtr->mLobOldSize) );
        CMI_RD4( aReadContext.mCMContext, &(sLobPtr->mLobNewSize) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),         4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),          4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),           8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),       8 );
        aReadContext.mXLogfileContext->readXLog( &(sLobPtr->mLobLocator), 8 );
        aReadContext.mXLogfileContext->readXLog( &(sLobPtr->mLobOffset),  4 );
        aReadContext.mXLogfileContext->readXLog( &(sLobPtr->mLobOldSize), 4 );
        aReadContext.mXLogfileContext->readXLog( &(sLobPtr->mLobNewSize), 4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobTrimA7( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               ULong                aLobLocator,
                               UInt                 aLobOffset,
                               UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_LOB_TRIM;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 36, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, LobTrim );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information */
    CMI_WR8( aProtocolContext, &(aLobLocator) );
    CMI_WR4( aProtocolContext, &(aLobOffset) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobPartialWriteA7( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       smTID                aTID,
                                       smSN                 aSN,
                                       smSN                 aSyncSN,
                                       ULong                aLobLocator,
                                       UInt                 aLobOffset,
                                       UInt                 aLobPieceLen,
                                       SChar              * aLobPiece,
                                       UInt                 aTimeoutSec )
{
    UInt  sType;
    UChar sOpCode;
    UInt  sRemainSize = 0;
    UInt  sSendSize = 0;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_LOB_PARTIAL_WRITE;

    /* ASSERT : Size must not be 0 */
    IDE_DASSERT( aLobPieceLen != 0 );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             RP_BLOB_VALUSE_HEADER_SIZE_FOR_CM + aLobPieceLen,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, LobPartialWrite );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information */
    CMI_WR8( aProtocolContext, &(aLobLocator) );
    CMI_WR4( aProtocolContext, &(aLobOffset) );
    CMI_WR4( aProtocolContext, &(aLobPieceLen) );

    sSendSize = aLobPieceLen;
    sRemainSize = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
    while( sSendSize > 0 )
    {
        if ( sRemainSize == 0 )
        {
            IDE_TEST( sendCmBlock( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   ID_FALSE,
                                   aTimeoutSec )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( sRemainSize >= sSendSize )
            {
                CMI_WCP( aProtocolContext, 
                         (UChar *)aLobPiece + ( aLobPieceLen - sSendSize ), 
                         sSendSize );
                sSendSize = 0;
            }
            else
            {
                CMI_WCP( aProtocolContext, 
                         (UChar *)aLobPiece + ( aLobPieceLen - sSendSize ), 
                         sRemainSize );
                sSendSize = sSendSize - sRemainSize;
            }
        }
        sRemainSize = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvLobPartialWriteA7( iduMemAllocator     * aAllocator,
                                       idBool              * aExitFlag,
                                       rpxReceiverReadContext aReadContext,
                                       rpdXLog             * aXLog,
                                       ULong                 aTimeOutSec )
{
    UInt * sType = (UInt*)&(aXLog->mType);
    UInt   sRecvSize = 0;
    UInt   sRemainSize = 0;

    rpdLob * sLobPtr = aXLog->mLobPtr;



    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        /* Get Argument XLog Hdr */
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        /* Get Lob Information */
        CMI_RD8( aReadContext.mCMContext, &(sLobPtr->mLobLocator) );
        CMI_RD4( aReadContext.mCMContext, &(sLobPtr->mLobOffset) );
        CMI_RD4( aReadContext.mCMContext, &(sLobPtr->mLobPieceLen) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),         4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),          4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),           8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),       8 );
        aReadContext.mXLogfileContext->readXLog( &(sLobPtr->mLobLocator), 8 );
        aReadContext.mXLogfileContext->readXLog( &(sLobPtr->mLobOffset),  4 );
        aReadContext.mXLogfileContext->readXLog( &(sLobPtr->mLobPieceLen), 4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }


    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_RP_RPN,
                                     sLobPtr->mLobPieceLen,
                                     (void **)&(sLobPtr->mLobPiece),
                                     IDU_MEM_IMMEDIATE,
                                     aAllocator)
                   != IDE_SUCCESS, ERR_MEMORY_ALLOC_LOB_PIECE);

    sRecvSize = sLobPtr->mLobPieceLen;
    sRemainSize = CMI_REMAIN_DATA_IN_READ_BLOCK( aReadContext.mCMContext );
    while( sRecvSize > 0 )
    {
        if ( sRemainSize == 0 )
        {
            if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
            {
                IDE_TEST( readCmBlock( NULL, aReadContext.mCMContext,
                                    aExitFlag,
                                    NULL,
                                    aTimeOutSec )
                        != IDE_SUCCESS );

            }
            else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
            {
                IDE_TEST( readXLogfile( aReadContext.mXLogfileContext ) != IDE_SUCCESS );
            }
            else
            {
                IDE_RAISE( ERR_INTERNAL );
            }
        }
        else
        {
            if ( sRemainSize >= sRecvSize )
            {
                if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
                {
                    CMI_RCP( aReadContext.mCMContext,
                         (UChar *)sLobPtr->mLobPiece + ( sLobPtr->mLobPieceLen - sRecvSize ), 
                         sRecvSize );
                }
                else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
                {
                    (void)aReadContext.mXLogfileContext->readXLog((UChar *)sLobPtr->mLobPiece + ( sLobPtr->mLobPieceLen - sRecvSize ),
                                                                  sRecvSize,
                                                                  ID_FALSE);
                }
                else
                {
                    IDE_RAISE( ERR_INTERNAL );
                }
                sRecvSize = 0;
            }
            else
            {

                if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
                {
                    CMI_RCP( aReadContext.mCMContext,
                            (UChar *)sLobPtr->mLobPiece + ( sLobPtr->mLobPieceLen - sRecvSize ), 
                            sRemainSize );
                }
                else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
                {
                    (void)aReadContext.mXLogfileContext->readXLog((UChar *)sLobPtr->mLobPiece + ( sLobPtr->mLobPieceLen - sRecvSize ),
                                                                  sRemainSize,
                                                                  ID_FALSE);
                }
                else
                {
                    IDE_RAISE( ERR_INTERNAL );
                }
                sRecvSize = sRecvSize - sRemainSize;
            }
        }
        if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
        {
            sRemainSize = CMI_REMAIN_DATA_IN_READ_BLOCK( aReadContext.mCMContext );
        }
        else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
        {
            IDE_TEST( aReadContext.mXLogfileContext->getRemainSize( &sRemainSize ) != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE( ERR_INTERNAL );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MEMORY_ALLOC_LOB_PIECE);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvLobPartialWrite",
                                "aXLog->mLobPtr->mLobPiece"));
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    if(sLobPtr->mLobPiece != NULL)
    {
        (void)iduMemMgr::free(sLobPtr->mLobPiece, aAllocator);
        sLobPtr->mLobPiece = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobFinish2WriteA7( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       smTID                aTID,
                                       smSN                 aSN,
                                       smSN                 aSyncSN,
                                       ULong                aLobLocator,
                                       UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_LOB_FINISH2WRITE;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 32, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, LobFinish2Write );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information */
    CMI_WR8( aProtocolContext, &(aLobLocator) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvLobFinish2WriteA7( rpxReceiverReadContext aReadContext,
                                       idBool              * /* aExitFlag */,
                                       rpdXLog             * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        /* Get Argument XLog Hdr */
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        /* Get Lob Information */
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mLobPtr->mLobLocator) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN), 8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mLobPtr->mLobLocator), 8 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendHandshakeA7( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_HANDSHAKE;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 24, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Handshake );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );

    IDU_FIT_POINT( "rpnComm::sendHandshake::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvHandshakeA7( rpxReceiverReadContext aReadContext,
                                 idBool             * /*aExitFlag*/,
                                 rpdXLog            * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        /* Get Argument XLog Hdr */
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN), 8 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncPKBeginA7( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_SYNC_PK_BEGIN;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 24, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, SyncPKBegin );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );

    IDU_FIT_POINT( "rpnComm::sendSyncPKBegin::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvSyncPKBeginA7( cmiProtocolContext * aProtocolContext,
                                   idBool             * /* aExitFlag */,
                                   rpdXLog            * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    /* Get Argument XLog Hdr */
    CMI_RD4( aProtocolContext, sType );
    CMI_RD4( aProtocolContext, &(aXLog->mTID) );
    CMI_RD8( aProtocolContext, &(aXLog->mSN) );
    CMI_RD8( aProtocolContext, &(aXLog->mSyncSN) );

    return IDE_SUCCESS;
}

IDE_RC rpnComm::sendSyncPKA7( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              ULong                aTableOID,
                              UInt                 aPKColCnt,
                              smiValue           * aPKCols,
                              rpValueLen         * aPKLen,
                              UInt                 aTimeoutSec )
{
    UInt i;
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_SYNC_PK;

    /* Validate Parameters */
    IDE_DASSERT(aPKColCnt <= QCI_MAX_KEY_COLUMN_COUNT);
    IDE_DASSERT(aPKCols != NULL);

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 36, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, SyncPK );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information */
    CMI_WR8( aProtocolContext, &(aTableOID) );
    CMI_WR4( aProtocolContext, &(aPKColCnt) );

    /* Send Primary Key Value */
    for(i = 0; i < aPKColCnt; i ++)
    {
        IDE_TEST( sendPKValueForA7( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    &aPKCols[i],
                                    aPKLen[i],
                                    aTimeoutSec )
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvSyncPKA7( idBool             * aExitFlag,
                              cmiProtocolContext * aProtocolContext,
                              rpdXLog            * aXLog,
                              ULong                aTimeOutSec )
{
    UInt i;
    UInt * sType = (UInt*)&(aXLog->mType);

    rpxReceiverReadContext sTempReadContext;

    RP_INIT_RECEIVER_TEMP_READCONTEXT( &sTempReadContext, aProtocolContext );

    /* Get Argument XLog Hdr */
    CMI_RD4( aProtocolContext, sType );
    CMI_RD4( aProtocolContext, &(aXLog->mTID) );
    CMI_RD8( aProtocolContext, &(aXLog->mSN) );
    CMI_RD8( aProtocolContext, &(aXLog->mSyncSN) );
    /* Get Argument Savepoint Name */
    CMI_RD8( aProtocolContext, &(aXLog->mTableOID) );
    CMI_RD4( aProtocolContext, &(aXLog->mPKColCnt) );

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mPKColCnt * ID_SIZEOF( smiValue ),
                                          (void **)&( aXLog->mPKCols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    /* Recv PK Value repeatedly */
    for( i = 0; i < aXLog->mPKColCnt; i ++ )
    {
        IDE_TEST( recvValueA7( NULL, /* PK column value 메모리를 iduMemMgr로 할당받아야한다. */
                               aExitFlag,
                               sTempReadContext,
                               NULL, /* aMemory: Sender에 전달할 것이므로, 직접할당 */
                               &aXLog->mPKCols[i],
                               aTimeOutSec,
                               NULL )
                  != IDE_SUCCESS );
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * XLog 내의 Column 정보를 재배열한다.
     *   - mPKColCnt는 변하지 않으므로, 제외한다.
     *   - Primary Key의 순서가 같으므로, mPKCIDs[]와 mPKCols[]는 제외한다.
     */

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvSyncPKA7",
                                "Column"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncPKEndA7( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_SYNC_PK_END;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 24, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, SyncPKEnd );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );

    IDU_FIT_POINT( "rpnComm::sendSyncPKEnd::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvSyncPKEndA7( cmiProtocolContext * aProtocolContext,
                                 idBool             * /* aExitFlag */,
                                 rpdXLog            * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    /* Get Argument XLog Hdr */
    CMI_RD4( aProtocolContext, sType );
    CMI_RD4( aProtocolContext, &(aXLog->mTID) );
    CMI_RD8( aProtocolContext, &(aXLog->mSN) );
    CMI_RD8( aProtocolContext, &(aXLog->mSyncSN) );

    return IDE_SUCCESS;
}

IDE_RC rpnComm::sendFailbackEndA7( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec )
{
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_FAILBACK_END;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 24, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, FailbackEnd );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );

    IDU_FIT_POINT( "rpnComm::sendFailbackEnd::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvFailbackEndA7( cmiProtocolContext * aProtocolContext,
                                   idBool             * /*aExitFlag*/,
                                   rpdXLog            * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    /* Get Argument XLog Hdr */
    CMI_RD4( aProtocolContext, sType );
    CMI_RD4( aProtocolContext, &(aXLog->mTID) );
    CMI_RD8( aProtocolContext, &(aXLog->mSN) );
    CMI_RD8( aProtocolContext, &(aXLog->mSyncSN) );

    return IDE_SUCCESS;
}

IDE_RC rpnComm::recvAckOnDML( cmiProtocolContext * aProtocolContext,
                              idBool             * /* aExitFlag */,
                              rpdXLog            * aXLog )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    /* Get Argument XLog Hdr */
    CMI_RD4( aProtocolContext, sType );

    return IDE_SUCCESS;
}

IDE_RC rpnComm::recvOperationInfoA7( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     UChar              * aOpCode,
                                     ULong                aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */,
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_PEEK1( aProtocolContext, sOpCode, 0 );

    *aOpCode = sOpCode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLASyncStartA7( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     UInt                 aType,
                                     ULong                aSendTimeout )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4,
                             ID_TRUE,
                             aSendTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLReplicateHandshake );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &aType );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aSendTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLASyncStartA7( cmiProtocolContext * aProtocolContext,
                                     idBool             * /*aExitFlag*/,
                                     rpdXLog            * aXLog )

{
    UInt * sType = (UInt*)&(aXLog->mType);

    /* Get Argument XLog Hdr */
    CMI_RD4( aProtocolContext, sType );

    return IDE_SUCCESS;
}

IDE_RC rpnComm::sendDDLASyncStartAckA7( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt                 aType,
                                        ULong                aSendTimeout )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( NULL,
                             aProtocolContext,
                             aExitFlag,
                             1 +
                             4,
                             ID_TRUE,
                             aSendTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLReplicateAck );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &aType );

    IDE_TEST( sendCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aSendTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLASyncStartAckA7( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt               * aType,
                                        ULong                aRecvTimeout )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }

    CMI_RD1( aProtocolContext, sOpCode );     
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLReplicateAck ), 
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, aType );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLASyncExecuteA7( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       UInt                 aType,
                                       SChar              * aUserName,
                                       UInt                 aDDLEnableLevel,
                                       UInt                 aTargetCount,
                                       SChar              * aTargetTableName,
                                       SChar              * aTargetPartNames,
                                       smSN                 aDDLCommitSN,
                                       rpdVersion         * aReplVersion,
                                       SChar              * aDDLStmt,
                                       ULong                aSendTimeout )


{
    UChar sOpCode;
    UInt  i;
    UInt  sOffset      = 0;
    UInt  sDDLStmtLen  = 0;
    UInt  sUserNameLen = 0;
    UInt  sTargetTableNameLen   = 0;
    UInt  sTargetPartNameLen    = 0;
    UInt  sTargetPartNameLenSum = 0;
    SChar * sTargetPartName     = NULL;

    sDDLStmtLen  = idlOS::strlen( aDDLStmt );
    sUserNameLen = idlOS::strlen( aUserName );
    sTargetTableNameLen = idlOS::strlen( aTargetTableName );

    for ( i = 0; i < aTargetCount; i++ )
    {
        sTargetPartName = aTargetPartNames + sOffset;
        sTargetPartNameLenSum += idlOS::strlen( sTargetPartName );

        sOffset += QC_MAX_OBJECT_NAME_LEN + 1;
    }
    sOffset = 0; 

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4 +
                             4 + sUserNameLen +
                             4 +
                             4 + 4 + sTargetTableNameLen + 4 * aTargetCount + sTargetPartNameLenSum +
                             8 +
                             8, 
                             ID_TRUE,
                             aSendTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLReplicateExecute  );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &aType );

    CMI_WR4( aProtocolContext, &sUserNameLen );
    CMI_WCP( aProtocolContext, (UChar *)aUserName, sUserNameLen );

    CMI_WR4( aProtocolContext, &aDDLEnableLevel );

    CMI_WR4( aProtocolContext, &aTargetCount );

    CMI_WR4( aProtocolContext, &sTargetTableNameLen );        
    IDE_DASSERT( sTargetTableNameLen > 0 );
    CMI_WCP( aProtocolContext, (UChar *)aTargetTableName, sTargetTableNameLen );

    for ( i = 0; i < aTargetCount; i++ )
    {
        sTargetPartName = aTargetPartNames + sOffset;

        sTargetPartNameLen = idlOS::strlen( sTargetPartName );

        CMI_WR4( aProtocolContext, &( sTargetPartNameLen ) );
        if ( sTargetPartNameLen > 0 )
        {
            CMI_WCP( aProtocolContext, (UChar *)sTargetPartName, sTargetPartNameLen );

            sOffset += QC_MAX_OBJECT_NAME_LEN + 1;
        }
        else
        {
            /* Only Non Partitioned Table Here */
            IDE_DASSERT( aTargetCount == 1 );
        }
    }

    CMI_WR8( aProtocolContext, &aDDLCommitSN );

    CMI_WR8( aProtocolContext, &(aReplVersion->mVersion) );

    IDE_TEST( sendQueryString( aHBTResource,
                               aProtocolContext,
                               aExitFlag,
                               aType,
                               sDDLStmtLen,
                               aDDLStmt,
                               aSendTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLASyncExecuteA7( cmiProtocolContext  * aProtocolContext,
                                       idBool              * aExitFlag,
                                       UInt                * aType,
                                       SChar               * aUserName,
                                       UInt                * aDDLEnableLevel,
                                       UInt                * aTargetCount,
                                       SChar               * aTargetTableName,
                                       SChar              ** aTargetPartNames,
                                       smSN                * aDDLCommitSN,
                                       rpdVersion          * aReplVersion,
                                       SChar              ** aDDLStmt,
                                       ULong                 aRecvTimeout )
{
    UChar sOpCode;
    UInt  i;    
    UInt  sOffset        = 0;
    UInt  sUserNameLen   = 0;
    UInt  sTargetPartNameLen = 0;
    UInt  sTargetTableNameLen = 0;
    SChar * sTargetPartName   = NULL;
    SChar * sTargetPartNames  = NULL;
   
    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLReplicateExecute ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, aType );

    CMI_RD4( aProtocolContext, &( sUserNameLen ) );
    IDE_DASSERT( sUserNameLen > 0 );

    CMI_RCP( aProtocolContext, (UChar *)aUserName, sUserNameLen );
    aUserName[sUserNameLen] = '\0';

    CMI_RD4( aProtocolContext, aDDLEnableLevel );

    CMI_RD4( aProtocolContext, aTargetCount );
   
    CMI_RD4( aProtocolContext, &sTargetTableNameLen );        
    IDE_DASSERT( sTargetTableNameLen > 0 );

    CMI_RCP( aProtocolContext, (UChar *)( aTargetTableName ), sTargetTableNameLen );
    aTargetTableName[sTargetTableNameLen] = '\0';

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPN,
                                 *aTargetCount * 
                                 ( ID_SIZEOF(SChar) * ( QC_MAX_OBJECT_NAME_LEN + 1 ) ),
                                 (void**)&( sTargetPartNames ),
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    for ( i = 0; i < *aTargetCount; i++ )
    {
        sTargetPartName = sTargetPartNames + sOffset;

        CMI_RD4( aProtocolContext, &sTargetPartNameLen );
        if ( sTargetPartNameLen > 0 )
        {
            IDU_FIT_POINT_RAISE( "rpnComm::recvDDLASyncExecuteA7::ERR_TOO_LONG_PART_NAME", ERR_TOO_LONG_PART_NAME );
            IDE_TEST_RAISE( sTargetPartNameLen > QC_MAX_OBJECT_NAME_LEN , ERR_TOO_LONG_PART_NAME );

            CMI_RCP( aProtocolContext, (UChar *)sTargetPartName, sTargetPartNameLen );

            sOffset += QC_MAX_OBJECT_NAME_LEN + 1;
        }
        else
        {
            /* Only Non Partitioned Table Here */
            IDE_DASSERT( *aTargetCount == 1 );
        }
            
        sTargetPartName[sTargetPartNameLen] = '\0';
    }

    CMI_RD8( aProtocolContext, aDDLCommitSN );

    CMI_RD8( aProtocolContext, &( aReplVersion->mVersion ) );

    IDE_TEST( recvQueryString( aProtocolContext,
                               aExitFlag,
                               aType,
                               aDDLStmt,
                               aRecvTimeout )
              != IDE_SUCCESS );

    *aTargetPartNames = sTargetPartNames;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_PART_NAME );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Too long received target name" ) );
    }
    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
    }
    IDE_EXCEPTION_END;

    if ( sTargetPartNames != NULL )
    {
        (void)iduMemMgr::free( sTargetPartNames );
        sTargetPartNames = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLASyncExecuteAckA7( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt                 aType,
                                          UInt                 aIsSuccess,
                                          UInt                 aErrCode,
                                          SChar              * aErrMsg,
                                          ULong                aSendTimeout )
{
    UChar sOpCode;
    UInt  sErrMsgLen  = 0;
    UInt  sSendMsgLen = 0;

    if ( aErrMsg != NULL )
    {
        sErrMsgLen = idlOS::strlen( aErrMsg );
    }
    else
    {
        sErrMsgLen = 0;
    }

    if ( sErrMsgLen > RP_MAX_MSG_LEN )
    {
        sSendMsgLen = RP_MAX_MSG_LEN;
    }
    else
    {
        sSendMsgLen = sErrMsgLen;
    }

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( NULL,
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4 +
                             4 +
                             4 +
                             4 + sSendMsgLen, 
                             ID_TRUE,
                             aSendTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLReplicateAck );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &aType );

    CMI_WR4( aProtocolContext, &aIsSuccess );

    CMI_WR4( aProtocolContext, &aErrCode );

    CMI_WR4( aProtocolContext, &sSendMsgLen );
    if ( sSendMsgLen > 0 )
    {
        CMI_WCP( aProtocolContext, (UChar *)aErrMsg, sSendMsgLen );
    }

    IDE_TEST( sendCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aSendTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLASyncExecuteAckA7( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt               * aType,
                                          UInt               * aIsSuccess,
                                          UInt               * aErrCode,
                                          SChar              * aErrMsg,
                                          ULong                aRecvTimeout )
{
    UChar sOpCode;
    UInt  sErrMsgLen     = 0;
    UInt  sErrMsgRecvLen = 0;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }

    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLReplicateAck ), 
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext,aType );

    CMI_RD4( aProtocolContext, aIsSuccess );

    CMI_RD4( aProtocolContext,(UInt*)aErrCode );

    CMI_RD4( aProtocolContext, &sErrMsgRecvLen );
    if ( sErrMsgRecvLen > 0 )
    {
        if ( sErrMsgRecvLen > RP_MAX_MSG_LEN )
        {
            sErrMsgLen = RP_MAX_MSG_LEN;
        }
        else
        {
            sErrMsgLen = sErrMsgRecvLen;
        }

        CMI_RCP( aProtocolContext, (UChar *)aErrMsg, sErrMsgLen );
        aErrMsg[sErrMsgLen] = '\0';

        if ( sErrMsgRecvLen > sErrMsgLen )
        {
            /* 나머지는 Skip */
            CMI_SKIP_READ_BLOCK( aProtocolContext, sErrMsgRecvLen - sErrMsgLen );
        }
    }
    else
    {
        aErrMsg[0] = '\0';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendQueryString(  void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  UInt                 aType,
                                  UInt                 aSqlLen,
                                  SChar              * aSql,
                                  ULong                aTimeout )
{
    UChar sOpCode;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4 +
                             4 + aSqlLen,
                             ID_TRUE,
                             aTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLReplicateQueryStatement );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &aType );

    CMI_WR4( aProtocolContext, &aSqlLen );
    if ( aSqlLen > 0 )
    {
        CMI_WCP( aProtocolContext, (UChar *)aSql, aSqlLen );
    }

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvQueryString( cmiProtocolContext  * aProtocolContext,
                                 idBool              * aExitFlag,
                                 UInt                * aType,
                                 SChar              ** aSql,
                                 ULong                 aRecvTimeout )
{
    UChar   sOpCode;
    UInt    sSqlLen  = 0;
    SChar * sSql     = NULL;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }

    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLReplicateQueryStatement ),
                    ERR_CHECK_OPERATION_TYPE );
    
    CMI_RD4( aProtocolContext, aType );

    CMI_RD4( aProtocolContext, &sSqlLen );
    if ( sSqlLen > 0 )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPN,
                                     sSqlLen + 1,
                                     (void**)&( sSql ),
                                     IDU_MEM_IMMEDIATE )
                  != IDE_SUCCESS );

        CMI_RCP( aProtocolContext, (UChar *)sSql, sSqlLen );
        sSql[sSqlLen] = '\0';
    }

    *aSql = sSql;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE, sOpCode ) );
    }

    IDE_EXCEPTION_END;

    if ( sSql != NULL )
    {
        (void)iduMemMgr::free( sSql );
        sSql = NULL;
    }

    return IDE_FAILURE;
}



IDE_RC rpnComm::sendMetaInitFlagA7( void               * aHBTResource,
                                    cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    idBool               aMetaInitFlag,
                                    UInt                 aTimeoutSec )
{
    UChar sOpCode;
    UInt  sMetaInitFlag = 0;
    
    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    IDE_TEST( checkAndFlush( NULL, /* aHBTResource */
                             aProtocolContext,
                             aExitFlag,
                             1 + 4,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaInitialize );
    CMI_WR1( aProtocolContext, sOpCode );
    
    if ( aMetaInitFlag == ID_TRUE )
    {
        sMetaInitFlag = 1;
    }
    else
    {
        sMetaInitFlag = 0;
    }
    CMI_WR4( aProtocolContext, &sMetaInitFlag );
    
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaInitFlagA7( cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    idBool             * aMetaInitFlag,
                                    ULong                aTimeoutSec )
{
    UChar   sOpCode;
    UInt    sMetaInitFlag = 0;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           aExitFlag,
                           NULL /* TimeoutFlag */, 
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaInitialize ), 
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, &sMetaInitFlag );

    if ( sMetaInitFlag == 1 )
    {
        *aMetaInitFlag = ID_TRUE;
    }
    else
    {
        *aMetaInitFlag = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendConditionInfoA7( void                        * aHBTResource,
                                     cmiProtocolContext          * aProtocolContext,
                                     idBool                      * aExitFlag,
                                     rpdConditionItemInfo        * aConditionInfo,
                                     UInt                          aConditionCnt,
                                     UInt                          aTimeoutSec )
{
    UInt  i;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    /* Validate Parameters */
    IDE_DASSERT( aConditionInfo != NULL );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4  + (8+4)*aConditionCnt,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Item Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaReplTblCondition );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&aConditionCnt );
  
    for ( i = 0 ; i < aConditionCnt ; i++ )
    {
        CMI_WR8( aProtocolContext, (ULong*)&(aConditionInfo[i].mTableOID) );
        CMI_WR4( aProtocolContext, (UInt*)&(aConditionInfo[i].mIsEmpty) );
    }
    IDU_FIT_POINT( "rpnComm::sendMetaReplTblCondition::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );
    return IDE_FAILURE;
}

IDE_RC rpnComm::recvConditionInfoA7( cmiProtocolContext    * aProtocolContext,
                                     idBool                * aExitFlag,
                                     rpdConditionItemInfo  * aRecvConditionInfo,
                                     UInt                  * aRecvConditionCnt,
                                     ULong                   aTimeoutSec )
{
    UChar sOpCode;
    UInt  sItemCount;
    UInt  i;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext, 
                           aExitFlag,
                           NULL /* TimeoutFlag */, 
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplTblCondition ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, &(sItemCount) );
    IDE_DASSERT( sItemCount > 0 );
    
    for ( i = 0 ; i < sItemCount ; i++ )
    {
        CMI_RD8( aProtocolContext, &(aRecvConditionInfo[i].mTableOID) );
        CMI_RD4( aProtocolContext, (UInt*)&(aRecvConditionInfo[i].mIsEmpty) );
    }
    *aRecvConditionCnt = sItemCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendConditionInfoResultA7( cmiProtocolContext          * aProtocolContext,
                                           idBool                      * aExitFlag,
                                           rpdConditionActInfo         * aConditionInfo,
                                           UInt                          aConditionCnt,
                                           UInt                          aTimeoutSec )
{
    UInt  i;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    /* Validate Parameters */
    IDE_DASSERT( aConditionInfo != NULL );

    IDE_TEST( checkAndFlush( NULL, /* HBT Resource */
                             aProtocolContext,
                             aExitFlag,
                             1 + 4  + (8+4)*aConditionCnt,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication Item Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaReplTblConditionResult );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&aConditionCnt );
  
    for ( i = 0 ; i < aConditionCnt ; i++ )
    {
        CMI_WR8( aProtocolContext, &(aConditionInfo[i].mTableOID) );
        CMI_WR4( aProtocolContext, (UInt*)&(aConditionInfo[i].mAction) );

    }
    IDU_FIT_POINT( "rpnComm::sendMetaReplTblCondition::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( NULL, /* HBT Resource */
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );
    return IDE_FAILURE;
}

IDE_RC rpnComm::recvConditionInfoResultA7( cmiProtocolContext   * aProtocolContext,
                                           idBool               * aExitFlag,
                                           rpdConditionActInfo  * aConditionActInfo,
                                           ULong                  aTimeoutSec )
{
    UChar sOpCode;
    UInt  sItemCount;
    UInt  i;

    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext, 
                           aExitFlag,
                           NULL /* TimeoutFlag */, 
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplTblConditionResult ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, &(sItemCount) );
    IDE_DASSERT( sItemCount > 0 );
    
    for ( i = 0 ; i < sItemCount ; i++ )
    {
        CMI_RD8( aProtocolContext, &(aConditionActInfo[i].mTableOID) );
        CMI_RD4( aProtocolContext, (UInt*)&(aConditionActInfo[i].mAction) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendTruncateA7( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                ULong                aTableOID,
                                UInt                 aTimeoutSec )
{
    UChar sOpCode;

    UInt sType;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( aHBTResource, 
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4 + 
                             8,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, Truncate );
    sType   = RP_X_TRUNCATE;

    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&sType );
    CMI_WR8( aProtocolContext, &aTableOID );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTruncateA7( cmiProtocolContext * aProtocolContext,
                                idBool             * /*aExitFlag*/,
                                rpdMeta            * aMeta, 
                                rpdXLog            * aXLog )
{
    UInt           * sType = (UInt*)&(aXLog->mType);
    rpdMetaItem    * sItem = NULL;

    IDE_ASSERT(aMeta != NULL);

    /* Get Argument XLog Hdr */
    CMI_RD4( aProtocolContext, sType );
    CMI_RD8( aProtocolContext, &(aXLog->mTableOID) );

    (void)aMeta->searchRemoteTable(&sItem, aXLog->mTableOID);
    IDE_TEST_RAISE(sItem == NULL, ERR_NOT_FOUND_TABLE);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                aXLog->mSN,
                                aXLog->mTableOID));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC rpnComm::sendXaStartReqA7( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  ID_XID             * aXID,
                                  smTID                aTID,
                                  smSN                 aSN,
                                  ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt  sXIDDataSize = ID_XIDDATASIZE;
    UInt  sType;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( aHBTResource, 
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4 +
                             4 +
                             8 +
                             8 + 8 + 8 + 4 + sXIDDataSize, /* ID_XID */
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, XA_START_REQ );
    sType   = RP_X_XA_START_REQ;

    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&sType );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );

    CMI_WR8( aProtocolContext, (ULong*)&(aXID->formatID) );
    CMI_WR8( aProtocolContext, (ULong*)&(aXID->gtrid_length) );
    CMI_WR8( aProtocolContext, (ULong*)&(aXID->bqual_length) );
    CMI_WR4( aProtocolContext, (UInt*)&(sXIDDataSize) );
    CMI_WCP( aProtocolContext, aXID->data, sXIDDataSize );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvXaStartReqA7( rpxReceiverReadContext   aReadContext,
                                  idBool                 * /*aExitFlag*/,
                                  rpdXLog                * aXLog )
{
    UInt             sXIDDataSize = 0;
    UInt           * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );

        CMI_RD8( aReadContext.mCMContext, (ULong*)&(aXLog->mXID.formatID) );
        CMI_RD8( aReadContext.mCMContext, (ULong*)&(aXLog->mXID.gtrid_length) );
        CMI_RD8( aReadContext.mCMContext, (ULong*)&(aXLog->mXID.bqual_length) );
        CMI_RD4( aReadContext.mCMContext, &(sXIDDataSize) );
        CMI_RCP( aReadContext.mCMContext, aXLog->mXID.data,  sXIDDataSize );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
       
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mXID.formatID),     8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mXID.gtrid_length), 8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mXID.bqual_length), 8 );
        aReadContext.mXLogfileContext->readXLog( &(sXIDDataSize),             4 );
        aReadContext.mXLogfileContext->readXLog( aXLog->mXID.data,            sXIDDataSize );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC rpnComm::sendXaPrepareReqA7( void               * aHBTResource,
                                    cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    ID_XID             * aXID,
                                    smTID                aTID,
                                    smSN                 aSN,
                                    ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt  sXIDDataSize = ID_XIDDATASIZE;
    UInt  sType;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( aHBTResource, 
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4 +
                             4 +
                             8 +
                             8 + 8 + 8 + 4 + sXIDDataSize,  /* ID_XID */
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, XA_PREPARE_REQ );
    sType   = RP_X_XA_PREPARE_REQ;

    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&sType );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );

    CMI_WR8( aProtocolContext, (ULong*)&(aXID->formatID) );
    CMI_WR8( aProtocolContext, (ULong*)&(aXID->gtrid_length) );
    CMI_WR8( aProtocolContext, (ULong*)&(aXID->bqual_length) );
    CMI_WR4( aProtocolContext, (UInt*)&(sXIDDataSize) );
    CMI_WCP( aProtocolContext, aXID->data, sXIDDataSize );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvXaPrepareReqA7( rpxReceiverReadContext   aReadContext,
                                    idBool                 * /*aExitFlag*/,
                                    rpdXLog                * aXLog )

{
    UInt             sXIDDataSize = 0;
    UInt           * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );

        CMI_RD8( aReadContext.mCMContext, (ULong*)&(aXLog->mXID.formatID) );
        CMI_RD8( aReadContext.mCMContext, (ULong*)&(aXLog->mXID.gtrid_length) );
        CMI_RD8( aReadContext.mCMContext, (ULong*)&(aXLog->mXID.bqual_length) );
        CMI_RD4( aReadContext.mCMContext, &(sXIDDataSize) );
        CMI_RCP( aReadContext.mCMContext, aXLog->mXID.data,  sXIDDataSize );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
       
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mXID.formatID),     8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mXID.gtrid_length), 8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mXID.bqual_length), 8 );
        aReadContext.mXLogfileContext->readXLog( &(sXIDDataSize),             4 );
        aReadContext.mXLogfileContext->readXLog( aXLog->mXID.data,            sXIDDataSize );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC rpnComm::sendXaPrepareA7( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 ID_XID             * aXID,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt  sXIDDataSize = ID_XIDDATASIZE;
    UInt  sType;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( aHBTResource, 
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4 +
                             4 +
                             8 +
                             8 + 8 + 8 + 4 + sXIDDataSize,  /* ID_XID */
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP,XA_PREPARE );
    sType   = RP_X_XA_PREPARE;

    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&sType );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );

    CMI_WR8( aProtocolContext, (ULong*)&(aXID->formatID) );
    CMI_WR8( aProtocolContext, (ULong*)&(aXID->gtrid_length) );
    CMI_WR8( aProtocolContext, (ULong*)&(aXID->bqual_length) );
    CMI_WR4( aProtocolContext, (UInt*)&(sXIDDataSize) );
    CMI_WCP( aProtocolContext, aXID->data, sXIDDataSize );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_FALSE,
                           aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvXaPrepareA7( rpxReceiverReadContext   aReadContext,
                                 idBool                 * /*aExitFlag*/,
                                 rpdXLog                * aXLog )

{
    UInt             sXIDDataSize = 0;
    UInt           * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );

        CMI_RD8( aReadContext.mCMContext, (ULong*)&(aXLog->mXID.formatID) );
        CMI_RD8( aReadContext.mCMContext, (ULong*)&(aXLog->mXID.gtrid_length) );
        CMI_RD8( aReadContext.mCMContext, (ULong*)&(aXLog->mXID.bqual_length) );
        CMI_RD4( aReadContext.mCMContext, &(sXIDDataSize) );
        CMI_RCP( aReadContext.mCMContext, aXLog->mXID.data,  sXIDDataSize );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
       
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mXID.formatID),     8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mXID.gtrid_length), 8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mXID.bqual_length), 8 );
        aReadContext.mXLogfileContext->readXLog( &(sXIDDataSize),             4 );
        aReadContext.mXLogfileContext->readXLog( aXLog->mXID.data,            sXIDDataSize );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC rpnComm::sendXaEndA7( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             ULong                aTimeoutSec )
{
    UChar sOpCode;
    UInt  sType;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( aHBTResource, 
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4 +
                             4 +
                             8, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, XA_END );
    sType   = RP_X_XA_END;

    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&sType );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_FALSE,
                           aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvXaEndA7( rpxReceiverReadContext   aReadContext,
                             idBool                 * /*aExitFlag*/,
                             rpdXLog                * aXLog )
{
    UInt           * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),   4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),    4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),     8 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}



IDE_RC rpnComm::readXLogfile( rpdXLogfileMgr * aXLogfileManager )
{
    IDE_TEST( aXLogfileManager->checkRemainXLog() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


