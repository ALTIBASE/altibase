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
 * $Id: qcmDump.cpp 18910 2007-02-05 01:56:34Z leekmo $
 *
 * Description :
 *
 *
 * 용어 설명 :
 *
 * 약어 :

 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <qcm.h>
#include <qcmPartition.h>
#include <qcmTableSpace.h>
#include <qcmDump.h>
#include <qcuSqlSourceInfo.h>
#include <qcuTemporaryObj.h>

/***********************************************************************
 * Description :
 *
 *    DUMP OBJECT의 정보를 획득한다.
 *
 * Implementation :
 *
 *    DUMP TABLE의 종류에 따른 DUMP OBJECT 정보를 설정한다.
 *
 **********************************************************************/

IDE_RC
qcmDump::getDumpObjectInfo( qcStatement * aStatement,
                            qmsTableRef * aTableRef )
{
    SChar    * sName;
    idBool     sFound;

    //---------------------------------------
    // Parameter Validation
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableRef != NULL );
    IDE_DASSERT( aTableRef->tableInfo != NULL );

    //---------------------------------------
    // GET Dump Object Information
    //---------------------------------------

    if ( aTableRef->mDumpObjList == NULL )
    {
        // Nothing To Do
        // Dump Object 를 명시하지 않음
        // ex) desc D$DISK_INDEX_BTREE_STRUCTURE
    }
    else
    {
        sName = aTableRef->tableInfo->name;
        sFound = ID_FALSE;

        // D$DISK_DB_*
        if ( ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_DB_PAGE",
                                14 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_DB_PHYPAGEHDR",
                                20 ) == 0 ) )
        {
            IDE_TEST( getGRID( aStatement,
                               aTableRef->mDumpObjList,
                               ID_TRUE,  // enableDisk
                               ID_FALSE, // enableMem
                               ID_FALSE) // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$DISK_INDEX_*
        if ( ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_INDEX_BTREE_CTS",
                                22 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_INDEX_RTREE_CTS",
                                22 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_INDEX_BTREE_KEY",
                                22 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_INDEX_RTREE_KEY",
                                22 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_INDEX_BTREE_STRUCTURE",
                                28 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_INDEX_RTREE_STRUCTURE",
                                28 ) == 0 ) )
        {
            IDE_TEST( getIndexInfo( aStatement,
                                    aTableRef->mDumpObjList,
                                    ID_TRUE,  // enableDisk
                                    ID_FALSE, // enableMem
                                    ID_FALSE) // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$DISK_TABLE_*
        if ( ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_CTL",
                                16 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_CTS",
                                16 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_LOBINFO",
                                20 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_RECORD",
                                19 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_SLOT",
                                17 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_EXTLIST",
                                20 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_PIDLIST",
                                20 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_LOB_AGINGLIST",
                                26 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_LOB_META",
                                21 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_FMS_SEGHDR",
                                23 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_FMS_FREEPIDLIST",
                                28 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_FMS_PVTPIDLIST",
                                27 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_FMS_UFMTPIDLIST",
                                28 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_SEGHDR",
                                23 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_RTBMPHDR",
                                25 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_RTBMPBODY",
                                26 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_ITBMPHDR",
                                25 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_ITBMPBODY",
                                26 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_LFBMPHDR",
                                25 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_LFBMPRANGESLOT",
                                31 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_LFBMPPBSTBL",
                                28 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_SEGCACHE",
                                25 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$DISK_TABLE_TMS_BMPSTRUCTURE",
                                29 ) == 0 ) )
        {
            IDE_TEST( getTableInfo( aStatement,
                                    aTableRef->mDumpObjList,
                                    ID_TRUE,  // enableDisk
                                    ID_FALSE, // enableMem
                                    ID_FALSE) // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$DISK_TBS_*
        if ( idlOS::strMatch( sName,
                              idlOS::strlen( sName ),
                              "D$DISK_TBS_FREEEXTLIST",
                              22 ) == 0 )
        {
            IDE_TEST( getTBSID( aStatement,
                                aTableRef->mDumpObjList,
                                ID_TRUE,  // enableDisk
                                ID_FALSE, // enableMem
                                ID_FALSE) // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$MEM_DB_*
        if ( ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$MEM_DB_PAGE",
                                13 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$MEM_DB_PERSPAGEHDR",
                                20 ) == 0 ) )
        {
            IDE_TEST( getGRID( aStatement,
                               aTableRef->mDumpObjList,
                               ID_FALSE, // enableDisk
                               ID_TRUE,  // enableMem
                               ID_FALSE) // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$MEM_TBS_*
        if ( idlOS::strMatch( sName,
                              idlOS::strlen( sName ),
                              "D$MEM_TBS_PCH",
                              13 ) == 0 )
        {
            IDE_TEST( getTBSID( aStatement,
                                aTableRef->mDumpObjList,
                                ID_FALSE, // enableDisk
                                ID_TRUE,  // enableMem
                                ID_FALSE) // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$MEM_INDEX_*
        if ( ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$MEM_INDEX_BTREE_STRUCTURE",
                                27 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$MEM_INDEX_BTREE_KEY",
                                21 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$MEM_INDEX_RTREE_STRUCTURE",
                                27 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$MEM_INDEX_RTREE_KEY",
                                21 ) == 0 ) )
        {
            IDE_TEST( getIndexInfo( aStatement,
                                    aTableRef->mDumpObjList,
                                    ID_FALSE, // enableDiskIndex
                                    ID_TRUE,  // enableMemIndex
                                    ID_FALSE) // enableVolIndex
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$VOL_INDEX_*
        if ( ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$VOL_INDEX_BTREE_STRUCTURE",
                                27 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$VOL_INDEX_BTREE_KEY",
                                21 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$VOL_INDEX_RTREE_STRUCTURE",
                                27 ) == 0 ) ||
             ( idlOS::strMatch( sName,
                                idlOS::strlen( sName ),
                                "D$VOL_INDEX_RTREE_KEY",
                                21 ) == 0 ) )
        {
            IDE_TEST( getIndexInfo( aStatement,
                                    aTableRef->mDumpObjList,
                                    ID_FALSE, // enableDiskIndex
                                    ID_FALSE, // enableMemIndex
                                    ID_TRUE)  // enableVolIndex
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$MEM_TABLE_*
        if ( idlOS::strMatch( sName,
                              idlOS::strlen( sName ),
                              "D$MEM_TABLE_RECORD",
                              18 ) == 0 )
        {
            IDE_TEST( getTableInfo( aStatement,
                                    aTableRef->mDumpObjList,
                                    ID_FALSE, // enableDisk
                                    ID_TRUE,  // enableMem
                                    ID_FALSE) // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$VOL_TABLE_*
        if ( idlOS::strMatch( sName,
                              idlOS::strlen( sName ),
                              "D$VOL_TABLE_RECORD",
                              18 ) == 0 )
        {
            IDE_TEST( getTableInfo( aStatement,
                                    aTableRef->mDumpObjList,
                                    ID_FALSE, // enableDisk
                                    ID_FALSE, // enableMem
                                    ID_TRUE)  // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$VOL_DB_*
        if ( idlOS::strMatch( sName,
                              idlOS::strlen( sName ),
                              "D$VOL_DB_PAGE",
                              13 ) == 0 )
        {
            IDE_TEST( getGRID( aStatement,
                               aTableRef->mDumpObjList,
                               ID_FALSE, // enableDisk
                               ID_FALSE, // enableMem
                               ID_TRUE)  // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        // D$VOL_TBS_*
        if ( idlOS::strMatch( sName,
                              idlOS::strlen( sName ),
                              "D$VOL_TBS_PCH",
                              13 ) == 0 )
        {
            IDE_TEST( getTBSID( aStatement,
                                aTableRef->mDumpObjList,
                                ID_FALSE, // enableDisk
                                ID_FALSE, // enableMem
                                ID_TRUE)  // enableVol
                      != IDE_SUCCESS );
            sFound = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }

        IDE_TEST_RAISE( sFound != ID_TRUE , ERR_INVALID_DUMP_TABLE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_INVALID_DUMP_TABLE ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *
 *    Index 정보를 획득한다.
 *
 * Implementation :
 *
 *
 **********************************************************************/

IDE_RC qcmDump::getIndexInfo( qcStatement    * aStatement,
                              qmsDumpObjList * aDumpObjList,
                              idBool           aEnableDiskIdx,
                              idBool           aEnableMemIdx,
                              idBool           aEnableVolIdx )
{
    UInt           i;
    UInt           sUserID;
    UInt           sTableID;
    UInt           sTablePartID;
    UInt           sIndexID;
    qcmTableInfo * sTableInfo;
    void         * sTableHandle;
    smSCN          sSCN;
    qcmIndex     * sIndexInfo = NULL;
    const void   * sTempIndexHandle = NULL;

    //---------------------------------------
    // Parameter Validation
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDumpObjList != NULL );

    IDE_TEST_RAISE( aDumpObjList->mNext != NULL, ERR_TOO_MANY_DUMP_OBJECT );

    //---------------------------------------
    // Get Index Handle
    //---------------------------------------
    IDE_TEST( qcm::checkIndexByUser( aStatement,
                                     aDumpObjList->mUserNamePos,
                                     aDumpObjList->mDumpObjPos,
                                     &sUserID,
                                     &sTableID,
                                     &sIndexID) != IDE_SUCCESS );

    // NON-PARTITIONED TABLE
    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     &sTableInfo,
                                     &sSCN,
                                     &sTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sTableHandle,
                                         sSCN,
                                         SMI_TABLE_LOCK_IS )
              != IDE_SUCCESS );

    if ( aDumpObjList->mDumpPartitionRef != NULL )
    {
        // PROJ-1502 PARTITIONED DISK TABLE
        IDE_TEST(qcmPartition::checkIndexPartitionByUser(
                     aStatement,
                     aDumpObjList->mUserNamePos,
                     aDumpObjList->mDumpPartitionRef->partitionName,
                     sIndexID,
                     &sUserID,
                     &sTablePartID )
                 != IDE_SUCCESS);

        IDE_TEST(qcmPartition::getPartitionInfoByID( aStatement,
                                                     sTablePartID,
                                                     &sTableInfo,
                                                     &sSCN,
                                                     &sTableHandle )
                 != IDE_SUCCESS);

        IDE_TEST( qcmPartition::validateAndLockOnePartition( aStatement,
                                                             sTableHandle,
                                                             sSCN,
                                                             SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                             SMI_TABLE_LOCK_IS,
                                                             ID_ULONG_MAX )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // 원치 않는 Index인가?
    IDE_TEST_RAISE( ( (aEnableDiskIdx == ID_FALSE) &&
                      (smiIsDiskTable( sTableHandle ) == ID_TRUE) ) ||
                    ( (aEnableMemIdx == ID_FALSE) &&
                      (smiIsMemTable( sTableHandle ) == ID_TRUE) ) ||
                    ( (aEnableVolIdx == ID_FALSE) &&
                      (smiIsVolTable( sTableHandle ) == ID_TRUE) ),
                    ERR_INVALID_OBJECT );

    // PROJ-1407 Temporary Table
    if ( qcuTemporaryObj::isTemporaryTable( sTableInfo ) == ID_TRUE )
    {
        sTempIndexHandle = qcuTemporaryObj::getTempIndexHandle(
            aStatement,
            sTableInfo,
            sIndexID );
    }

    if( sTempIndexHandle != NULL )
    {
        /* session temporary table이 존재하는 경우이다.
         * 나 혼자만 볼 수 있는 table이므로
         * 다른 트랜잭션이 session table을 변경하지 않는다.
         * session temp table 을 위한 별도의 추가 table lock을 잡을
         * 필요가 없다.*/
        aDumpObjList->mObjInfo = (void*)sTempIndexHandle;
    }
    else
    {
        // temporary table이 아니거나 temporary table이더라도
        // session temp table이 생성되지 않은 경우

        for ( i = 0; i < sTableInfo->indexCount; i++)
        {
            if (sTableInfo->indices[i].indexId == sIndexID)
            {
                sIndexInfo = &sTableInfo->indices[i];
                break;
            }
        }

        IDE_TEST_RAISE(sIndexInfo == NULL, ERR_NOT_EXIST_INDEX);

        aDumpObjList->mObjInfo = sIndexInfo->indexHandle;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_TOO_MANY_DUMP_OBJECT ));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_INDEX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXISTS_INDEX));
    }
    IDE_EXCEPTION(ERR_INVALID_OBJECT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INVALID_DUMP_OBJECT));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *
 *    TableSpace 이름을 바탕으로 SpaceID를 가져온다.
 *
 * Implementation :
 *
 *
 **********************************************************************/
IDE_RC qcmDump::getTBSID( qcStatement    * aStatement,
                          qmsDumpObjList * aDumpObjList,
                          idBool           aEnableDiskTBS,
                          idBool           aEnableMemTBS,
                          idBool           aEnableVolTBS )
{
    SChar               sBuffer[256];
    scSpaceID         * sSpaceID;
    smiTableSpaceAttr   sTBSAttr;
    qcuSqlSourceInfo    sqlInfo;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDumpObjList != NULL );

    IDE_TEST_RAISE( aDumpObjList->mNext != NULL, ERR_TOO_MANY_DUMP_OBJECT );

    /* BUG-28678  [SM] qmsDumpObjList::mObjInfo에 설정될 메모리 주소는
     * 반드시 공간을 할당해서 설정해야합니다. */
    IDU_LIMITPOINT("qcmDump::getTBSID::malloc");
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(scSpaceID ),
                                             (void**)&( sSpaceID ) )
             != IDE_SUCCESS );


    if( aDumpObjList->mDumpObjPos.size > 255 )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aDumpObjList->mDumpObjPos );

        IDE_RAISE( ERR_MAX_NAME_LENGTH_OVERFLOW );
    }
    else
    {
        // Nothing to do.
    }

    idlOS::strncpy( sBuffer,
                    aDumpObjList->mDumpObjPos.stmtText + aDumpObjList->mDumpObjPos.offset,
                    aDumpObjList->mDumpObjPos.size );

    sBuffer[aDumpObjList->mDumpObjPos.size] = '\0';

    IDE_TEST( qcmTablespace::getTBSAttrByName( aStatement,
                                               sBuffer,
                                               idlOS::strlen( sBuffer ),
                                               &sTBSAttr )
              != IDE_SUCCESS );
    *sSpaceID = sTBSAttr.mID;
    aDumpObjList->mObjInfo = (void*)sSpaceID;

    // 원치 않는 테이블스페이스 인가?
    IDE_TEST_RAISE( ( (aEnableDiskTBS == ID_FALSE) &&
                      ( ( sTBSAttr.mType == SMI_DISK_SYSTEM_DATA ) ||
                        ( sTBSAttr.mType == SMI_DISK_USER_DATA ) ||
                        ( sTBSAttr.mType == SMI_DISK_SYSTEM_TEMP ) ||
                        ( sTBSAttr.mType == SMI_DISK_USER_TEMP ) ||
                        ( sTBSAttr.mType == SMI_DISK_SYSTEM_UNDO ) ) ) ||
                    ( (aEnableMemTBS == ID_FALSE) &&
                      ( ( sTBSAttr.mType == SMI_MEMORY_SYSTEM_DICTIONARY ) ||
                        ( sTBSAttr.mType == SMI_MEMORY_SYSTEM_DATA ) ||
                        ( sTBSAttr.mType == SMI_MEMORY_USER_DATA ) ) ) ||
                    ( (aEnableVolTBS == ID_FALSE) &&
                        ( sTBSAttr.mType == SMI_VOLATILE_USER_DATA ) ),
                    ERR_INVALID_OBJECT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_TOO_MANY_DUMP_OBJECT ));
    }
    IDE_EXCEPTION( ERR_MAX_NAME_LENGTH_OVERFLOW )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_OBJECT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INVALID_DUMP_OBJECT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *
 *    MEMORY/DISK TABLE의 OBJECT 정보를 출력한다.
 *
 * Implementation :
 *
 *
 **********************************************************************/
IDE_RC qcmDump::getTableInfo( qcStatement    * aStatement,
                              qmsDumpObjList * aDumpObjList,
                              idBool           aEnableDiskTable,
                              idBool           aEnableMemTable,
                              idBool           aEnableVolTable )
{
    UInt           sUserID;
    qcmTableInfo * sTableInfo = NULL;
    void         * sTableHandle;
    const void   * sTempTableHandle = NULL;
    smSCN          sSCN;
    smSCN          sBaseTableSCN;

    //---------------------------------------
    // Parameter Validation
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDumpObjList != NULL );

    IDE_TEST_RAISE( aDumpObjList->mNext != NULL, ERR_TOO_MANY_DUMP_OBJECT );

    //---------------------------------------
    // Get Index Handle
    //---------------------------------------
    IDE_TEST( qdbCommon::checkTableInfo( aStatement,
                                         aDumpObjList->mUserNamePos,
                                         aDumpObjList->mDumpObjPos,
                                         &sUserID,
                                         &sTableInfo,
                                         &sTableHandle,
                                         &sSCN )
              != IDE_SUCCESS );

    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sTableHandle,
                                         sSCN,
                                         SMI_TABLE_LOCK_IS )
              != IDE_SUCCESS );

    // PROJ-1407 Temporary Table
    if ( qcuTemporaryObj::isTemporaryTable( sTableInfo ) == ID_TRUE )
    {
        qcuTemporaryObj::getTempTableHandle( aStatement,
                                             sTableInfo,
                                             &sTempTableHandle,
                                             &sBaseTableSCN );
        if( sTempTableHandle != NULL )
        {
            IDE_TEST_RAISE( !SM_SCN_IS_EQ( &sSCN, &sBaseTableSCN ),
                            ERR_TEMPORARY_TABLE_EXIST );

            // session temporary table이 존재하는 경우.
            sTableHandle = (void*)sTempTableHandle;
        }
    }

    if( aDumpObjList->mDumpPartitionRef != NULL )
    {
        IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                                  sTableInfo->tableID,
                                                  aDumpObjList->mDumpPartitionRef->partitionName,
                                                  & sTableInfo,
                                                  & sSCN,
                                                  & sTableHandle )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::validateAndLockOnePartition( aStatement,
                                                             sTableHandle,
                                                             sSCN,
                                                             SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                             SMI_TABLE_LOCK_IS,
                                                             ID_ULONG_MAX )
                  != IDE_SUCCESS );
    }

    // 원치 않는 Table인가?
    IDE_TEST_RAISE( ( (aEnableDiskTable == ID_FALSE) &&
                      (smiIsDiskTable( sTableHandle ) == ID_TRUE) ) ||
                    ( (aEnableMemTable == ID_FALSE) &&
                      (smiIsMemTable( sTableHandle ) == ID_TRUE) ) ||
                    ( (aEnableVolTable == ID_FALSE) &&
                      (smiIsVolTable( sTableHandle ) == ID_TRUE) ),
                    ERR_INVALID_OBJECT );

    aDumpObjList->mObjInfo = sTableHandle;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_TOO_MANY_DUMP_OBJECT ));
    }
    IDE_EXCEPTION(ERR_INVALID_OBJECT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INVALID_DUMP_OBJECT));
    }
    IDE_EXCEPTION( ERR_TEMPORARY_TABLE_EXIST )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMN_INVALID_TEMPORARY_TABLE ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TASK-4007 [SM] PBT를 위한 기능 추가
 * D$BUFFER_FRAME, D$DISK_PAGE, D$MEM_PAGE을 위해
 * SID와 PID를 가져옴*/
IDE_RC qcmDump::getGRID( qcStatement    * aStatement,
                         qmsDumpObjList * aDumpObjList,
                         idBool           aEnableDiskTBS,
                         idBool           aEnableMemTBS,
                         idBool           aEnableVolTBS )
{
    smiTableSpaceAttr   sTBSAttr;
    qmsDumpObjList    * sDumpObjList;
    SChar               sBuffer[256];
    UInt                i;
    scGRID            * sGRID;
    qcuSqlSourceInfo    sqlInfo;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDumpObjList != NULL );

    /* 인자 개수 체크 :
     * 첫번째 인자: Tablespace 이름, 두번째 인자: PID */
    sDumpObjList = aDumpObjList;

    for( i = 0; i < 2 ; i ++ )
    {
        IDE_TEST_RAISE( sDumpObjList == NULL, ERR_TOO_MANY_DUMP_OBJECT );
        sDumpObjList = sDumpObjList->mNext;
    }

    IDE_TEST_RAISE( sDumpObjList != NULL, ERR_TOO_MANY_DUMP_OBJECT );
    sDumpObjList = aDumpObjList;



    // 메모리 할당
    IDU_LIMITPOINT("qcmDump::getGRID::malloc");
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( scGRID ),
                                             (void**)&( sGRID ) )
             != IDE_SUCCESS );



    // TableSpaceID 획득
    if( sDumpObjList->mDumpObjPos.size > 255 )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sDumpObjList->mDumpObjPos );

        IDE_RAISE( ERR_MAX_NAME_LENGTH_OVERFLOW );
    }
    else
    {
        // Nothing to do.
    }

    idlOS::strncpy( sBuffer,
                    sDumpObjList->mDumpObjPos.stmtText + sDumpObjList->mDumpObjPos.offset,
                    sDumpObjList->mDumpObjPos.size );
    sBuffer[sDumpObjList->mDumpObjPos.size] = '\0';

    IDE_TEST_RAISE( qcmTablespace::getTBSAttrByName( aStatement,
                                                     sBuffer,
                                                     idlOS::strlen( sBuffer ),
                                                     &sTBSAttr )
                    != IDE_SUCCESS,
                    ERR_INVALID_ARGUMENT );
    sGRID->mSpaceID = sTBSAttr.mID;
    sDumpObjList = sDumpObjList->mNext;

    // 원치 않는 테이블스페이스 인가?
    IDE_TEST_RAISE( ( (aEnableDiskTBS == ID_FALSE) &&
                      ( ( sTBSAttr.mType == SMI_DISK_SYSTEM_DATA ) ||
                        ( sTBSAttr.mType == SMI_DISK_USER_DATA ) ||
                        ( sTBSAttr.mType == SMI_DISK_SYSTEM_TEMP ) ||
                        ( sTBSAttr.mType == SMI_DISK_USER_TEMP ) ||
                        ( sTBSAttr.mType == SMI_DISK_SYSTEM_UNDO ) ) ) ||
                    ( (aEnableMemTBS == ID_FALSE) &&
                      ( ( sTBSAttr.mType == SMI_MEMORY_SYSTEM_DICTIONARY ) ||
                        ( sTBSAttr.mType == SMI_MEMORY_SYSTEM_DATA ) ||
                        ( sTBSAttr.mType == SMI_MEMORY_USER_DATA ) ) ) ||
                    ( (aEnableVolTBS == ID_FALSE) &&
                        ( sTBSAttr.mType == SMI_VOLATILE_USER_DATA ) ),
                    ERR_INVALID_ARGUMENT );

    // PageID 획득
    if( sDumpObjList->mDumpObjPos.size > 255 )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sDumpObjList->mDumpObjPos );

        IDE_RAISE( ERR_MAX_NAME_LENGTH_OVERFLOW );
    }
    else
    {
        // Nothing to do.
    }

    idlOS::strncpy( sBuffer,
                    sDumpObjList->mDumpObjPos.stmtText + sDumpObjList->mDumpObjPos.offset,
                    sDumpObjList->mDumpObjPos.size );
    sBuffer[sDumpObjList->mDumpObjPos.size] = '\0';
    sGRID->mPageID = idlOS::atoi( sBuffer );

    aDumpObjList->mObjInfo = (void*)sGRID;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_TOO_MANY_DUMP_OBJECT ));
    }
    IDE_EXCEPTION( ERR_INVALID_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INVALID_DUMP_OBJECT));
    }
    IDE_EXCEPTION( ERR_MAX_NAME_LENGTH_OVERFLOW )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

