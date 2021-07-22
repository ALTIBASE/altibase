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
 * $Id: dumploganchor.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <smiDef.h>
#include <smriDef.h>
#include <dumploganchor.h>

void ShowCopyRight(void);
    
/***********************************************************************
 * Description :  loganchor dump
 **********************************************************************/
IDE_RC dumpLoganchor( SChar*   aLogAnchorPath )
{
    UInt               sLoop;
    ULong              sFileSize = 0;
    UInt               sReadOffset;
    smrLogAnchor       sLogAnchor;
    smiTableSpaceAttr  sSpaceAttr;
    smiDataFileAttr    sFileAttr;
    smiChkptPathAttr   sChkptPathAttr;
    smmChkptImageAttr  sChkptImageAttr;
    smiSBufferFileAttr sSBufferFileAttr;
    iduFile            sFile;
    SChar              sObjStatusBuff[500];
    smiNodeAttrType    sAttrType;
    UInt               sTBSState;
    SChar              sMSGBuff[128] = "\0";
    UInt               sDelLogCnt = 0;

    IDE_DASSERT( aLogAnchorPath != NULL );

    IDE_TEST( sFile.initialize( IDU_MEM_SM_SMR,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    
    IDE_TEST( sFile.setFileName( aLogAnchorPath ) != IDE_SUCCESS );

    IDE_TEST( sFile.open() != IDE_SUCCESS );

    IDE_TEST( sFile.read( NULL,
                          0, 
                          (SChar*)&sLogAnchor,
                          (UInt)ID_SIZEOF(smrLogAnchor) )
              != IDE_SUCCESS );
    sReadOffset = (UInt)ID_SIZEOF(smrLogAnchor);

    idlOS::printf( "<< DUMP OF LOGANCHOR FILE - %s >>\n\n",
                   aLogAnchorPath );

    //12345678901234567890123456789012345678901234
    idlOS::printf( "\n------------------------\n" );
    idlOS::printf( "[LOGANCHOR ATTRIBUTE SIZE\n" );
    idlOS::printf( "Loganchor Static Area                [ %"ID_UINT32_FMT" ]\n",
                   ID_SIZEOF(smrLogAnchor) );
    idlOS::printf( "Tablespace Attribute                 [ %"ID_UINT32_FMT" ]\n",
                   ID_SIZEOF(smiTableSpaceAttr) );
    idlOS::printf( "Checkpoint Path Attribute            [ %"ID_UINT32_FMT" ]\n",
                   ID_SIZEOF(smiChkptPathAttr) );
    idlOS::printf( "Checkpoint Image Attribute           [ %"ID_UINT32_FMT" ]\n",
                   ID_SIZEOF(smmChkptImageAttr) );
    idlOS::printf( "Disk Datafile Attribute              [ %"ID_UINT32_FMT" ]\n",
                   ID_SIZEOF(smiDataFileAttr) );
    idlOS::printf( "\n------------------------\n" );
    idlOS::printf( "[LOGANCHOR HEADER]\n" );

    idlOS::memset( sObjStatusBuff, 0x00, 32 );
    idlOS::snprintf( sObjStatusBuff, 
                     32,
                     "%"ID_xINT32_FMT".%"ID_xINT32_FMT".%"ID_xINT32_FMT,
                     ((sLogAnchor.mSmVersionID & SM_MAJOR_VERSION_MASK) >> 24),
                     ((sLogAnchor.mSmVersionID & SM_MINOR_VERSION_MASK) >> 16),
                     (sLogAnchor.mSmVersionID  & SM_PATCH_VERSION_MASK) );

    //12345678901234567890123456789012345678901234
    idlOS::printf( "Binary DB Version                    [ %s ]\n", 
                   sObjStatusBuff );

    idlOS::printf( "Archivelog Mode                      [ %s ]\n",
                   (sLogAnchor.mArchiveMode == SMI_LOG_ARCHIVE ?
                   "Archivelog" : "No-Archivelog") );

    idlOS::printf( "Transaction Segment Entry Count      [ %"ID_UINT32_FMT" ]\n", sLogAnchor.mTXSEGEntryCnt );


    idlOS::printf( "Begin Checkpoint LSN                 [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mBeginChkptLSN.mFileNo,
                   sLogAnchor.mBeginChkptLSN.mOffset );

    idlOS::printf( "End Checkpoint LSN                   [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mEndChkptLSN.mFileNo,
                   sLogAnchor.mEndChkptLSN.mOffset );
    
    idlOS::printf( "Disk Redo LSN                        [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mDiskRedoLSN.mFileNo,
                   sLogAnchor.mDiskRedoLSN.mOffset);

    if ( !SM_IS_LSN_MAX( sLogAnchor.mReplRecoveryLSN ) )
    {
        idlOS::printf( "LSN for Recovery from Replication    [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                       sLogAnchor.mReplRecoveryLSN.mFileNo,
                       sLogAnchor.mReplRecoveryLSN.mOffset );
    }
    else
    {
        idlOS::printf( "LSN for Recovery from Replication     [ NULL ]\n" );
    }

    idlOS::printf( "Server Status                        [ %s ]\n", 
                   ( (sLogAnchor.mServerStatus == SMR_SERVER_SHUTDOWN) ? 
                     (SChar*)"SERVER SHUTDOWN" : (SChar*)"SERVER STARTED" ) );

    //12345678901234567890123456789012345678901234
    idlOS::printf( "End LSN                              [ %"ID_UINT32_FMT",%"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mMemEndLSN.mFileNo,
                   sLogAnchor.mMemEndLSN.mOffset );

    idlOS::printf( "Media Recovery LSN                   [ %"ID_UINT32_FMT",%"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mMediaRecoveryLSN.mFileNo,
                   sLogAnchor.mMediaRecoveryLSN.mOffset );

    idlOS::printf( "ResetLog LSN                         [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mResetLSN.mFileNo,
                   sLogAnchor.mResetLSN.mOffset );

    idlOS::printf( "Last Created Logfile Num             [ %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mLstCreatedLogFileNo );
    
    /* BUG-39289 : FST = LST + deleted log count */
    sDelLogCnt = sLogAnchor.mFstDeleteFileNo - sLogAnchor.mLstDeleteFileNo;

    idlOS::printf( "Delete Logfile(s) Range              [ %"ID_UINT32_FMT" ~ %"ID_UINT32_FMT" ]\n",
                   (sLogAnchor.mFstDeleteFileNo == 0) ? 0 : (sLogAnchor.mLstDeleteFileNo - sDelLogCnt),
                   (sLogAnchor.mLstDeleteFileNo == 0) ? 0 : (sLogAnchor.mLstDeleteFileNo - 1) );
        

    //12345678901234567890123456789012345678901234
    idlOS::printf( "Update And Flush Count               [ %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mUpdateAndFlushCount );

    idlOS::printf( "New Tablespace ID                    [ %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mNewTableSpaceID );

    //PROJ-2133 incremental backup
    idlOS::printf( "\n[ Change Tracking ATTRIBUTE ]\n" );

    idlOS::printf( "Last Flush LSN                       [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mCTFileAttr.mLastFlushLSN.mFileNo,
                   sLogAnchor.mCTFileAttr.mLastFlushLSN.mOffset );

    switch ( sLogAnchor.mCTFileAttr.mCTMgrState )
    {
        case SMRI_CT_MGR_FILE_CREATING:
            {
                idlOS::snprintf( sMSGBuff, 
                                 sizeof( sMSGBuff ), 
                                 "%s", 
                                 "CHANGE TRACKING FILE CREATING" );
                break;
            }
        case SMRI_CT_MGR_ENABLED:
            {
                idlOS::snprintf( sMSGBuff, 
                                 sizeof( sMSGBuff ), 
                                 "%s", 
                                 "CHANGE TRACKING MGR ENABLED" );
                break;
            }
        case SMRI_CT_MGR_DISABLED:
            {
                idlOS::snprintf( sMSGBuff, 
                                 sizeof( sMSGBuff ), 
                                 "%s", 
                                 "CHANGE TRACKING MGR DISABLED" );
                break;
            }
        default:
            {
                IDE_ASSERT( 0 ); 
                break;
            }
    }
    idlOS::printf( "Change Tracking Manager State        [ %s ]\n", sMSGBuff );

    idlOS::printf( "Change Tracking File Name            [ %s ]\n", sLogAnchor.mCTFileAttr.mCTFileName );

    //PROJ-2133 incremental backup
    idlOS::printf( "\n[ Backup Info ATTRIBUTE ]\n" );

    if ( sLogAnchor.mBIFileAttr.mDeleteArchLogFileNo == ID_UINT_MAX )
    {
        idlOS::printf( "Delete Archivelog File Range         [ Not Applicable ]\n" );
    }
    else
    {
        idlOS::printf( "Delete Archivelog File Range         [ %"ID_UINT32_FMT" ~ %"ID_UINT32_FMT" ]\n",
                       0,
                       sLogAnchor.mBIFileAttr.mDeleteArchLogFileNo );

    }

    idlOS::printf( "Last Backup LSN                      [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mBIFileAttr.mLastBackupLSN.mFileNo,
                   sLogAnchor.mBIFileAttr.mLastBackupLSN.mOffset );

    idlOS::printf( "Before Backup LSN                    [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sLogAnchor.mBIFileAttr.mBeforeBackupLSN.mFileNo,
                   sLogAnchor.mBIFileAttr.mBeforeBackupLSN.mOffset );

    switch ( sLogAnchor.mBIFileAttr.mBIMgrState )
    {
        case SMRI_BI_MGR_FILE_CREATED:
            {
                idlOS::snprintf( sMSGBuff, 
                                 sizeof( sMSGBuff ), 
                                 "%s", 
                                 "BACKUP INFO FILE CREATED" );
                break;
            }
        case SMRI_BI_MGR_FILE_REMOVED:
            {
                idlOS::snprintf( sMSGBuff, 
                                 sizeof( sMSGBuff ), 
                                 "%s", 
                                 "BACKUP INFO FILE REMOVED" );
                break;
            }
        case SMRI_BI_MGR_INITIALIZED:
            {
                idlOS::snprintf( sMSGBuff, 
                                 sizeof( sMSGBuff ), 
                                 "%s", 
                                 "BACKUP INFO MGR INITIALIZED" );
                break;
            }
        case SMRI_BI_MGR_DESTROYED:
            {
                idlOS::snprintf( sMSGBuff, 
                                 sizeof( sMSGBuff ), 
                                 "%s", 
                                 "BACKUP INFO MGR DESTROYED" );
                break;
            }
        default:
            {
                IDE_ASSERT( 0 ); 
                break;
            }
    }

    idlOS::printf( "Backup Info Manager State            [ %s ]\n", sMSGBuff );
    idlOS::printf( "Backup Directory Path                [ %s ]\n", sLogAnchor.mBIFileAttr.mBackupDir );
    idlOS::printf( "Backup Info File Name                [ %s ]\n", sLogAnchor.mBIFileAttr.mBIFileName );

    IDE_TEST( sFile.getFileSize(&sFileSize) != IDE_SUCCESS );

    if ( sFileSize > ID_SIZEOF(smrLogAnchor) )
    {
        // 가변영역의 첫번째 Node Attribute Type을 판독한다. 
        IDE_TEST( smrLogAnchorMgr::getFstNodeAttrType( &sFile,
                                                       &sReadOffset,
                                                       &sAttrType ) != IDE_SUCCESS );
        while ( 1 )
        {
            switch ( sAttrType )
            {
                case SMI_TBS_ATTR:
                    {
                        IDE_TEST( smrLogAnchorMgr::readTBSNodeAttr( &sFile,
                                                                    &sReadOffset,
                                                                    &sSpaceAttr ) 
                                  != IDE_SUCCESS );

                        idlOS::printf( "\n-----------------------\n" );
                        idlOS::printf( "[ TABLESPACE ATTRIBUTE ]\n" );
                        //12345678901234567890123456789012345678901234
                        idlOS::printf( "Tablespace ID                        [ %"ID_UINT32_FMT" ]\n",
                                       sSpaceAttr.mID );
                        idlOS::printf( "Tablespace Name                      [ %s ]\n", sSpaceAttr.mName);
                        idlOS::printf( "New Database File ID                 [ %"ID_UINT32_FMT" ]\n",
                                       sSpaceAttr.mDiskAttr.mNewFileID );
                        idlOS::printf( "Extent Management                    [ %s ]\n",
                                       "FREE EXTENT BITMAP TABLESPACE");
                        idlOS::printf( "Tablespace Status                    [ " );

                        sTBSState = sSpaceAttr.mTBSStateOnLA;
                        
                        if ( SMI_TBS_IS_OFFLINE(sTBSState) )
                        {
                            idlOS::printf( "OFFLINE " );
                        }
                        if ( SMI_TBS_IS_ONLINE(sTBSState) )
                        {
                            idlOS::printf( "ONLINE " );
                        }
                        if ( (sTBSState & SMI_TBS_INCONSISTENT) 
                             == SMI_TBS_INCONSISTENT )
                        {
                            idlOS::printf( "INCONSISTENT " );
                        }
                        if ( SMI_TBS_IS_CREATING(sTBSState) )
                        {
                            idlOS::printf( "CREATING " );
                        }
                        if ( SMI_TBS_IS_DROPPING(sTBSState) )
                        {
                            idlOS::printf( "DROPPING " );
                        }
                        if ( SMI_TBS_IS_DROP_PENDING(sTBSState) )
                        {
                            idlOS::printf( "DROP_PENDING " );
                        }
                        if ( SMI_TBS_IS_DROPPED(sTBSState) )
                        {
                            idlOS::printf( "DROPPED " );
                        }
                        if ( SMI_TBS_IS_DISCARDED(sTBSState) )
                        {
                            idlOS::printf( "DISCARDED " );
                        }
                        if ( SMI_TBS_IS_BACKUP(sTBSState) )
                        {
                            idlOS::printf( "BACKUP " );
                        }
                        if ( ( sTBSState & SMI_TBS_SWITCHING_TO_OFFLINE )
                               == SMI_TBS_SWITCHING_TO_OFFLINE )
                        {
                            idlOS::printf( "SWITCHING_TO_OFFLINE " );
                        }
                        if ( ( sTBSState & SMI_TBS_SWITCHING_TO_ONLINE )
                               == SMI_TBS_SWITCHING_TO_ONLINE )
                        {
                            idlOS::printf( "SWITCHING_TO_ONLINE " );
                        }
                        
                        idlOS::printf( "]\n" );

                        //12345678901234567890123456789012345678901234
                        idlOS::printf( "TableSpace Type                      [ %"ID_UINT32_FMT" ]\n",
                                       sSpaceAttr.mType );
                        
                        if ( (sSpaceAttr.mType == SMI_MEMORY_SYSTEM_DICTIONARY) ||
                             (sSpaceAttr.mType == SMI_MEMORY_SYSTEM_DATA) ||
                             (sSpaceAttr.mType == SMI_MEMORY_USER_DATA) )
                        {
                            //12345678901234567890123456789012345678901234
                            idlOS::printf( "Checkpoint Path Count                [ %"ID_UINT32_FMT" ]\n", sSpaceAttr.mMemAttr.mChkptPathCount );
            
                            idlOS::printf( "Autoextend Mode                      [ %s ]\n", ( sSpaceAttr.mMemAttr.mIsAutoExtend == ID_TRUE )
                                           ? (SChar*)"Autoextend":(SChar*)"Non-AutoExtend" );

                            idlOS::printf( "Shared Memory Key                    [ %"ID_UINT32_FMT" ]\n", sSpaceAttr.mMemAttr.mShmKey );

                            idlOS::printf( "Stable Checkpoint Image Num.         [ %"ID_UINT32_FMT" ]\n", sSpaceAttr.mMemAttr.mCurrentDB );
 
                            idlOS::printf( "Init Size                            [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n",
                                           (((UInt)sSpaceAttr.mMemAttr.mInitPageCount) * SM_PAGE_SIZE) / 1024 / 1024,
                                           sSpaceAttr.mMemAttr.mInitPageCount );

                            idlOS::printf( "Next Size                            [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n", 
                                           (((UInt)sSpaceAttr.mMemAttr.mNextPageCount) * SM_PAGE_SIZE) / 1024 / 1024,
                                           sSpaceAttr.mMemAttr.mNextPageCount );

                            idlOS::printf( "Maximum Size                         [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n", 
                                           (((UInt)sSpaceAttr.mMemAttr.mMaxPageCount) * SM_PAGE_SIZE) / 1024 / 1024,
                                           sSpaceAttr.mMemAttr.mMaxPageCount );

                            idlOS::printf( "Split File Size                      [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n", 
                                           (((UInt)sSpaceAttr.mMemAttr.mSplitFilePageCount) * SM_PAGE_SIZE) / 1024 /1024,
                                           sSpaceAttr.mMemAttr.mSplitFilePageCount );
                        }
                        else if ( sSpaceAttr.mType == SMI_VOLATILE_USER_DATA )
                        {
                            idlOS::printf( "Autoextend Mode                      [ %s ]\n", (sSpaceAttr.mVolAttr.mIsAutoExtend == ID_TRUE)
                                           ? (SChar*)"Autoextend":(SChar*)"Non-AutoExtend" );

                            idlOS::printf( "Init Size                            [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n",
                                            (((UInt)sSpaceAttr.mVolAttr.mInitPageCount) * SM_PAGE_SIZE) / 1024 / 1024,
                                            sSpaceAttr.mVolAttr.mInitPageCount );

                            idlOS::printf( "Next Size                            [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n",
                                           (((UInt)sSpaceAttr.mVolAttr.mNextPageCount) * SM_PAGE_SIZE) / 1024 / 1024,
                                           sSpaceAttr.mVolAttr.mNextPageCount );

                            idlOS::printf( "Maximum Size                         [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n",
                                           (((UInt)sSpaceAttr.mVolAttr.mMaxPageCount) * SM_PAGE_SIZE) / 1024 / 1024,
                                           sSpaceAttr.mVolAttr.mMaxPageCount );
                        }
                        else
                        {
                            // nothing to do ..
                        }
                        idlOS::printf( "\n\n" );
                        break;
                    }
                case SMI_DBF_ATTR :
                    {
                        IDE_TEST( smrLogAnchorMgr::readDBFNodeAttr( &sFile,
                                                                    &sReadOffset,
                                                                    &sFileAttr ) != IDE_SUCCESS );

                        //12345678901234567890123456789012345678901234
                        idlOS::printf( "[ DISK DATABASE FILE ATTRIBUTE ]\n" );
                        idlOS::printf( "Tablespace ID                        [ %"ID_UINT32_FMT" ]\n", sFileAttr.mSpaceID );
                        idlOS::printf( "Database File ID                     [ %"ID_UINT32_FMT" ]\n", sFileAttr.mID );
                        idlOS::printf( "Database File Path                   [ %s ]\n", sFileAttr.mName ); 
                        idlOS::printf( "Create LSN                           [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                                       sFileAttr.mCreateLSN.mFileNo,
                                       sFileAttr.mCreateLSN.mOffset );
            
                        idlOS::printf( "Database File Status                 [ " );

                        if ( SMI_FILE_STATE_IS_OFFLINE( sFileAttr.mState ) )
                        {
                            idlOS::printf( "OFFLINE " );
                        }
                        
                        if ( SMI_FILE_STATE_IS_ONLINE( sFileAttr.mState ) )
                        {
                            idlOS::printf( "ONLINE " );
                        }
                        
                        if ( SMI_FILE_STATE_IS_CREATING( sFileAttr.mState ) )
                        {
                            idlOS::printf( "CREATING " );
                        }
                        
                        if ( SMI_FILE_STATE_IS_BACKUP( sFileAttr.mState ) )
                        {
                            idlOS::printf( "BACKUP " );
                        }

                        if ( SMI_FILE_STATE_IS_DROPPING( sFileAttr.mState ) )
                        {
                            idlOS::printf( "DROPPING " );
                        }

                        if ( SMI_FILE_STATE_IS_RESIZING( sFileAttr.mState ) )
                        {
                            idlOS::printf( "RESIZING " );
                        }

                        if ( SMI_FILE_STATE_IS_DROPPED( sFileAttr.mState ) )
                        {
                            idlOS::printf( "DROPPED " );
                        }

                        idlOS::printf( "]\n" );
            
                        //12345678901234567890123456789012345678901234
                        idlOS::printf( "Autoextend Mode                      [ %s ]\n",
                                       (sFileAttr.mIsAutoExtend == ID_TRUE) ?  (SChar*)"Autoextend":(SChar*)"Non-Autoextend" );

                        idlOS::printf( "Create Mode                          [ %"ID_UINT32_FMT" ]\n", sFileAttr.mCreateMode );

                        idlOS::printf( "Initialize Size                      [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n",
                                       (UInt)((sFileAttr.mInitSize * SD_PAGE_SIZE) / 1024 / 1024),
                                       sFileAttr.mInitSize );

                        idlOS::printf( "Current Size                         [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n", 
                                       (UInt)((sFileAttr.mCurrSize * SD_PAGE_SIZE) / 1024 / 1024),
                                       sFileAttr.mCurrSize );

                        idlOS::printf( "Next Size                            [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n", 
                                       (UInt)((sFileAttr.mNextSize * SD_PAGE_SIZE) / 1024 / 1024),
                                       sFileAttr.mNextSize );

                        idlOS::printf( "Maximum Size                         [ %8"ID_UINT32_FMT" MBytes ( %10"ID_UINT32_FMT" Pages ) ]\n", 
                                       (UInt)((sFileAttr.mMaxSize * SD_PAGE_SIZE) / 1024 / 1024),
                                       sFileAttr.mMaxSize );
                        //PROJ-2133 incremental backup
                        idlOS::printf( "ChangeTracking DataFileDescSlot ID   [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n", 
                                       sFileAttr.mDataFileDescSlotID.mBlockID, 
                                       sFileAttr.mDataFileDescSlotID.mSlotIdx );

                        break;
                    }
                case SMI_CHKPTPATH_ATTR :
                    {
                        IDE_TEST( smrLogAnchorMgr::readChkptPathNodeAttr( &sFile,
                                                                          &sReadOffset,
                                                                          &sChkptPathAttr ) != IDE_SUCCESS );

                        //12345678901234567890123456789012345678901234
                        idlOS::printf( "[ MEMORY CHECKPOINT PATH ATTRIBUTE ]\n" );
                        idlOS::printf( "Tablespace ID                        [ %"ID_UINT32_FMT" ]\n", sChkptPathAttr.mSpaceID );
                        idlOS::printf( "Checkpoint Path                      [ %s ]\n", sChkptPathAttr.mChkptPath );
                        break;
                    }
                case SMI_CHKPTIMG_ATTR:
                    {
                        IDE_TEST( smrLogAnchorMgr::readChkptImageAttr( &sFile,
                                                                       &sReadOffset,
                                                                       &sChkptImageAttr ) != IDE_SUCCESS );

                        //12345678901234567890123456789012345678901234
                        idlOS::printf( "[ MEMORY CHECKPOINT IMAGE ATTRIBUTE ]\n" );
                        idlOS::printf( "Tablespace ID                        [ %"ID_UINT32_FMT" ]\n", sChkptImageAttr.mSpaceID );

                        idlOS::printf( "File Number                          [ %"ID_UINT32_FMT" ]\n", sChkptImageAttr.mFileNum );

                        //12345678901234567890123456789012345678901234
                        idlOS::printf( "Create LSN                           [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                                       sChkptImageAttr.mMemCreateLSN.mFileNo,
                                       sChkptImageAttr.mMemCreateLSN.mOffset );

                        for ( sLoop = 0 ; sLoop < SMM_PINGPONG_COUNT ; sLoop ++ )
                        {
                            //12345678901234567890123456789012345678901234
                            idlOS::printf( "Create On Disk (PingPong %"ID_UINT32_FMT")          [ %s ]\n", 
                                           sLoop,
                                           (sChkptImageAttr.mCreateDBFileOnDisk[ sLoop ] == ID_TRUE ) ? (SChar*)"Created":(SChar*)"None" );
                        }
                        //PROJ-2133 incremental backup
                        idlOS::printf( "ChangeTracking DataFileDescSlot ID   [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n", 
                                       sChkptImageAttr.mDataFileDescSlotID.mBlockID, 
                                       sChkptImageAttr.mDataFileDescSlotID.mSlotIdx );
                        break;
                    }
                case SMI_SBUFFER_ATTR:
                    {
                        IDE_TEST( smrLogAnchorMgr::readSBufferFileAttr( &sFile,
                                                                        &sReadOffset,
                                                                        &sSBufferFileAttr ) 
                                  != IDE_SUCCESS );

                        break;
                    }
                default:
                    {
                        IDE_ASSERT( 0 ); 
                        break;
                    }
            }

            // EOF 확인
            if ( sFileSize <= ((ULong)sReadOffset) )
            {
                idlOS::printf( "\nEND OF LOGANCHOR\n" );
                break;  
            }
            
            // attribute type 판독
            IDE_TEST( smrLogAnchorMgr::getNxtNodeAttrType( &sFile,
                                                           sReadOffset,
                                                           &sAttrType ) != IDE_SUCCESS );
        } // while
    }
    else
    {
        // CREATE DATABASE 과정중 Loganchor의 고정영역만 저장된 경우
        // nothing to do ...
    }

    IDE_TEST( sFile.close() != IDE_SUCCESS );
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


int main(SInt argc, SChar *argv[])
{
    IDE_RC rc;

    ShowCopyRight();
    
    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
   
    //fix TASK-3870
    (void)iduLatch::initializeStatic( IDU_CLIENT_TYPE );

    IDE_TEST( iduCond::initializeStatic() != IDE_SUCCESS );
 
    if ( argc == 2 )
    {
        rc = dumpLoganchor( argv[1] );
        IDE_TEST_RAISE( rc != IDE_SUCCESS, err_file_invalid );
    }
    else
    {
        idlOS::printf( "usage: dumpla loganchor_file\n" );
        IDE_RAISE( err_arg_invalid );
    }

    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS );
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );
    
    return 0;

    IDE_EXCEPTION(err_arg_invalid);
    {
    }
    IDE_EXCEPTION(err_file_invalid);
    {
    idlOS::printf( "Check file type\n" );
    }
    IDE_EXCEPTION_END;
    
    return -1;
}

void ShowCopyRight(void)
{
    SChar         sBuf[1024 + 1];
    SChar         sBannerFile[1024];
    SInt          sCount;
    FILE        * sFP;
    SChar       * sAltiHome;
    const SChar * sBanner = "DUMPLA.ban";

    sAltiHome = idlOS::getenv(IDP_HOME_ENV);
    IDE_TEST_RAISE( sAltiHome == NULL, err_altibase_home );

    // make full path banner file name
    idlOS::memset( sBannerFile, 0, ID_SIZEOF(sBannerFile) );
    idlOS::snprintf( sBannerFile, 
                     ID_SIZEOF(sBannerFile), 
                     "%s%c%s%c%s",
                     sAltiHome, 
                     IDL_FILE_SEPARATOR, 
                     "msg", 
                     IDL_FILE_SEPARATOR, 
                     sBanner );

    sFP = idlOS::fopen( sBannerFile, "r" );
    IDE_TEST_RAISE( sFP == NULL, err_file_open );

    sCount = idlOS::fread( (void*) sBuf, 1, 1024, sFP );
    IDE_TEST_RAISE( sCount <= 0, err_file_read );

    sBuf[sCount] = '\0';
    idlOS::printf( "%s", sBuf );
    idlOS::fflush( stdout );

    idlOS::fclose( sFP );

    IDE_EXCEPTION( err_altibase_home );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_open );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_read );
    {
        idlOS::fclose( sFP );
    }
    IDE_EXCEPTION_END;
}
