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
 * $Id: qcmTableInfoMgr.cpp 19891 2007-01-12 01:44:27Z sungminee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcm.h>
#include <qcmTableInfo.h>
#include <qcmTableInfoMgr.h>
#include <qcmPartition.h>

IDE_RC qcmTableInfoMgr::makeTableInfoFirst( qcStatement   * aStatement,
                                            UInt            aTableID,
                                            smOID           aTableOID,
                                            qcmTableInfo ** aNewTableInfo )
{
    qcTableInfoMgr * sTableInfoMgr;
    void           * sTableHandle;
    qcmTableInfo   * sNewTableInfo;

    sTableHandle = (void*) smiGetTable( aTableOID );

    // tableInfoMgr 노드를 미리 생성한다.
    // makeTableInfoFirst가 IDE_FAILURE를 리턴했다면,
    // tableInfo는 생성하지 않았다.
    IDU_LIMITPOINT("qcmTableInfoMgr::makeTableInfoFirst::malloc");
    IDE_TEST( aStatement->qmxMem->alloc( ID_SIZEOF(qcTableInfoMgr),
                                         (void**) &sTableInfoMgr )
              != IDE_SUCCESS);

    // new tableInfo를 생성한다.
    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT(aStatement),
                                           aTableID,
                                           aTableOID )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( sTableHandle,
                                   (void**)&sNewTableInfo )
              != IDE_SUCCESS );

    // tableInfoMgr 노드를 구성한다.
    sTableInfoMgr->tableHandle  = sTableHandle;
    sTableInfoMgr->oldTableInfo = NULL;
    sTableInfoMgr->newTableInfo = (void *) sNewTableInfo;
    sTableInfoMgr->next         = QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr;

    // 등록한다.
    QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr = sTableInfoMgr;

    if ( aNewTableInfo != NULL )
    {
        (*aNewTableInfo) = sNewTableInfo;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTableInfoMgr::makeTableInfo( qcStatement   * aStatement,
                                       qcmTableInfo  * aOldTableInfo,
                                       qcmTableInfo ** aNewTableInfo )
{
    qcTableInfoMgr * sTableInfoMgr;
    void           * sTableHandle;
    qcmTableInfo   * sOldTableInfo;
    qcmTableInfo   * sNewTableInfo;
    idBool           sIsDestroyed = ID_FALSE;
    idBool           sIsPartition = ID_FALSE;
    UInt             sPartitionID = 0;
    UInt             sTableID = 0;
    qcmTableInfo   * sTempTableInfo = NULL;
    smSCN            sSCN;
    void           * sTempTableHandle;

    sOldTableInfo = aOldTableInfo;
    sTableHandle  = sOldTableInfo->tableHandle;

    // PROJ-1502
    if( sOldTableInfo->tablePartitionType == QCM_TABLE_PARTITION )
    {
        sIsPartition = ID_TRUE;
    }
    else
    {
        sIsPartition = ID_FALSE;
    }

    // BUG-16489
    // 가장 최신의 tableInfo를 찾는다.
    for ( sTableInfoMgr = QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr;
          sTableInfoMgr != NULL;
          sTableInfoMgr = sTableInfoMgr->next )
    {
        if ( sTableInfoMgr->tableHandle == sTableHandle )
        {
            if ( sTableInfoMgr->newTableInfo == NULL )
            {
                // 이미 삭제됨
                sIsDestroyed = ID_TRUE;
            }
            else
            {
                sOldTableInfo = (qcmTableInfo *) sTableInfoMgr->newTableInfo;
            }

            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sIsDestroyed == ID_FALSE )
    {
        // tableInfoMgr 노드를 미리 생성한다.
        // makeTableInfo가 IDE_FAILURE를 리턴했다면,
        // tableInfo는 생성하지 않았다.
        IDU_LIMITPOINT("qcmTableInfoMgr::makeTableInfo::malloc");
        IDE_TEST( aStatement->qmxMem->alloc( ID_SIZEOF(qcTableInfoMgr),
                                             (void**) &sTableInfoMgr )
                  != IDE_SUCCESS);

        // PROJ-1502 PARTITIONED DISK TABLE
        // 인자로 넘어온 aOldTableInfo는 partitionInfo임.
        if( sIsPartition == ID_TRUE )
        {
            sPartitionID = sOldTableInfo->partitionID;

            IDE_TEST( qcmPartition::getTableIDByPartitionID(
                          QC_SMI_STMT(aStatement),
                          sPartitionID,
                          & sTableID )
                      != IDE_SUCCESS );

            IDE_TEST(qcm::getTableInfoByID(aStatement,
                                           sTableID,
                                           & sTempTableInfo,
                                           & sSCN,
                                           & sTempTableHandle)
                     != IDE_SUCCESS);

            // new partition Info를 생성한다.
            IDE_TEST( qcmPartition::makeAndSetQcmPartitionInfo(
                          QC_SMI_STMT(aStatement),
                          sPartitionID,
                          smiGetTableId(sTableHandle),
                          sTempTableInfo,
                          NULL )
                      != IDE_SUCCESS );
        }
        else
        {
            // new tableInfo를 생성한다.
            IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT(aStatement),
                                                   sOldTableInfo->tableID,
                                                   smiGetTableId(sTableHandle) )
                      != IDE_SUCCESS );
        }

        IDE_TEST( smiGetTableTempInfo( sTableHandle,
                                       (void**)&sNewTableInfo )
                  != IDE_SUCCESS );

        // tableInfoMgr 노드를 구성한다.
        sTableInfoMgr->tableHandle  = sTableHandle;
        sTableInfoMgr->oldTableInfo = (void *) sOldTableInfo;
        sTableInfoMgr->newTableInfo = (void *) sNewTableInfo;
        sTableInfoMgr->next         = QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr;

        // 등록한다.
        QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr = sTableInfoMgr;

        if ( aNewTableInfo != NULL )
        {
            (*aNewTableInfo) = sNewTableInfo;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        if ( aNewTableInfo != NULL )
        {
            (*aNewTableInfo) = NULL;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTableInfoMgr::destroyTableInfo( qcStatement  * aStatement,
                                          qcmTableInfo * aTableInfo )
{
    qcTableInfoMgr  * sTableInfoMgr;
    void            * sTableHandle;
    qcmTableInfo    * sTableInfo;
    idBool            sIsDestroyed = ID_FALSE;

    sTableInfo   = aTableInfo;
    sTableHandle = aTableInfo->tableHandle;

    // 가장 최신의 tableInfo를 찾는다.
    for ( sTableInfoMgr = QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr;
          sTableInfoMgr != NULL;
          sTableInfoMgr = sTableInfoMgr->next )
    {
        if ( sTableInfoMgr->tableHandle == sTableHandle )
        {
            if ( sTableInfoMgr->newTableInfo == NULL )
            {
                // 이미 삭제됨
                sIsDestroyed = ID_TRUE;
            }
            else
            {
                sTableInfo = (qcmTableInfo *) sTableInfoMgr->newTableInfo;
            }

            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sIsDestroyed == ID_FALSE )
    {
        // tableInfoMgr 노드를 생성한다.
        IDU_LIMITPOINT("qcmTableInfoMgr::destroyTableInfo::malloc");
        IDE_TEST( aStatement->qmxMem->alloc( ID_SIZEOF(qcTableInfoMgr),
                                             (void**) &sTableInfoMgr )
                  != IDE_SUCCESS);

        // tableInfoMgr 노드를 구성한다.
        sTableInfoMgr->tableHandle  = sTableHandle;
        sTableInfoMgr->oldTableInfo = (void *) sTableInfo;
        sTableInfoMgr->newTableInfo = NULL;
        sTableInfoMgr->next         = QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr;

        // 등록한다.
        QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr = sTableInfoMgr;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qcmTableInfoMgr::destroyAllOldTableInfo( qcStatement  * aStatement )
{
    qcTableInfoMgr  * sTableInfoMgr;
    qcmTableInfo    * sOldTableInfo;

    IDE_ASSERT( QC_PRIVATE_TMPLATE(aStatement) != NULL );
    
    for ( sTableInfoMgr = QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr;
          sTableInfoMgr != NULL;
          sTableInfoMgr = sTableInfoMgr->next )
    {
        sOldTableInfo = (qcmTableInfo *) sTableInfoMgr->oldTableInfo;

        if ( sOldTableInfo != NULL )
        {
            // PROJ-1502 PARTITIONED DISK TABLE
            if( sOldTableInfo->tablePartitionType == QCM_TABLE_PARTITION )
            {
                (void)qcmPartition::destroyQcmPartitionInfo( (qcmTableInfo *) sOldTableInfo );
            }
            else
            {
                (void)qcm::destroyQcmTableInfo( (qcmTableInfo *) sOldTableInfo );
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    return;
}

void qcmTableInfoMgr::revokeAllNewTableInfo( qcStatement  * aStatement )
{
    qcTableInfoMgr  * sTableInfoMgr;
    qcmTableInfo    * sNewTableInfo;

    IDE_ASSERT( QC_PRIVATE_TMPLATE(aStatement) != NULL );
    
    for ( sTableInfoMgr = QC_PRIVATE_TMPLATE(aStatement)->tableInfoMgr;
          sTableInfoMgr != NULL;
          sTableInfoMgr = sTableInfoMgr->next )
    {
        sNewTableInfo = (qcmTableInfo *) sTableInfoMgr->newTableInfo;

        // set old tableInfo
        smiSetTableTempInfo( sTableInfoMgr->tableHandle,
                             (void*) sTableInfoMgr->oldTableInfo );

        if ( sNewTableInfo != NULL )
        {
            // PROJ-1502 PARTITIONED DISK TABLE
            if( sNewTableInfo->tablePartitionType == QCM_TABLE_PARTITION )
            {
                (void)qcmPartition::destroyQcmPartitionInfo( (qcmTableInfo *) sNewTableInfo );
            }
            else
            {
                (void)qcm::destroyQcmTableInfo( (qcmTableInfo *) sNewTableInfo );
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    return;
}
