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
 

#include <ide.h>
#include <idl.h>

#include <qc.h>
#include <qcg.h>

#include <qmsParseTree.h>

#include <qmr.h>

static qcRemoteTableCallback gCallback;

/*
 *
 */ 
IDE_RC qmrSetRemoteTableCallback( qcRemoteTableCallback * aCallback )
{
    gCallback = *aCallback;
    
    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC qmrGetRemoteTableMeta( qcStatement * aStatement,
                              qmsTableRef * aTableRef )
{
    SInt    i = 0;
    SInt    sStage = 0;
    SChar   sDatabaseLinkName[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };
    
    qcmTableInfo             * sTableInfo               = NULL;
    qcmColumn                * sPrevColumn              = NULL;

    void                     * sTableHandle             = NULL;
    SInt                       sColumnCount             = 0;
    qciRemoteTableColumnInfo * sRemoteTableColumnArray  = NULL;
    
    IDE_TEST_RAISE( gCallback.mGetRemoteTableMeta == NULL, NO_CALLBACK );
    IDE_TEST_RAISE( gCallback.mFreeRemoteTableColumnInfo == NULL, NO_CALLBACK );

    IDE_TEST_RAISE( QC_IS_NULL_NAME( aTableRef->remoteTable->linkName ),
                    NO_DATABASE_LINK_NAME );

    IDE_TEST( STRUCT_CRALLOC( QC_QMP_MEM( aStatement ),
                              qcmTableInfo,
                              &sTableInfo )
              != IDE_SUCCESS);

    sTableInfo->tableType = QCM_USER_TABLE;
    sTableInfo->tablePartitionType = QCM_NONE_PARTITIONED_TABLE;
    
    QC_STR_COPY( sDatabaseLinkName, aTableRef->remoteTable->linkName );

    IDE_TEST( gCallback.mGetRemoteTableMeta(
                  (void *)aStatement,
                  QCG_GET_DATABASE_LINK_SESSION( aStatement ),
                  aTableRef->remoteTable->mIsStore,  /* BUG-37077 REMOTE_TABLE_STORE */
                  sDatabaseLinkName,
                  aTableRef->remoteQuery,
                  &sTableHandle,
                  &sColumnCount,
                  &sRemoteTableColumnArray )
              != IDE_SUCCESS );
    sStage = 1;
    
    sTableInfo->columnCount = sColumnCount;
    sTableInfo->tableHandle = sTableHandle;
    
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE(
            QC_QMP_MEM( aStatement ),
            qcmColumn,
            ID_SIZEOF( qcmColumn ) * sColumnCount,
            (void **)&(sTableInfo->columns) )
        != IDE_SUCCESS);

    idlOS::memset( (void *)sTableInfo->columns,
                   0x00,
                   ID_SIZEOF( qcmColumn ) * sColumnCount );

    for ( i = 0; i < sColumnCount; i++ )
    {
        IDE_TEST( STRUCT_ALLOC_WITH_SIZE(
                QC_QMP_MEM( aStatement ),
                mtcColumn,
                ID_SIZEOF( *(sTableInfo->columns[i].basicInfo) ),
                (void **)&(sTableInfo->columns[i].basicInfo) )
            != IDE_SUCCESS);

        idlOS::memcpy(
            (void *)sTableInfo->columns[i].basicInfo,
            &(sRemoteTableColumnArray[i].column),
            ID_SIZEOF( sRemoteTableColumnArray[i].column ) );

        idlOS::memcpy(
            (void *)sTableInfo->columns[i].name,
            sRemoteTableColumnArray[i].columnName,
            ID_SIZEOF( sRemoteTableColumnArray[i].columnName ) );

        SET_EMPTY_POSITION( sTableInfo->columns[i].userNamePos );
        SET_EMPTY_POSITION( sTableInfo->columns[i].tableNamePos );
        SET_EMPTY_POSITION( sTableInfo->columns[i].namePos );

        if ( sPrevColumn != NULL )
        {
            sPrevColumn->next = &(sTableInfo->columns[i]);
        }
        else
        {
            /* nothing to do */
        }
        sPrevColumn = &(sTableInfo->columns[i]);
    }

    sStage = 0;
    IDE_TEST( gCallback.mFreeRemoteTableColumnInfo( sRemoteTableColumnArray )
              != IDE_SUCCESS );

    aTableRef->tableHandle = sTableInfo->tableHandle;
    aTableRef->tableSCN = smiGetRowSCN( sTableInfo->tableHandle );
    
    aTableRef->tableInfo = sTableInfo;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( NO_CALLBACK )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMR_NO_CALLBACK ) );
    }
    IDE_EXCEPTION( NO_DATABASE_LINK_NAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMR_NO_DATABASE_LINK_NAME ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sStage )
    {
        case 1:
            if ( sRemoteTableColumnArray != NULL )
            {
                (void)gCallback.mFreeRemoteTableColumnInfo(
                    sRemoteTableColumnArray );
            }
            else
            {
                /* nothing to do */
            }
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC qmrInvalidRemoteTableMetaCache( qcStatement * aStatement )
{
    qcTableMap  * sTableMap = NULL;
    qmsTableRef * sTableRef = NULL;
    SChar         sDatabaseLinkName[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };
    UShort        i = 0;

    IDE_TEST_RAISE( gCallback.mInvalidResultSetMetaCache == NULL,
                    NO_CALLBACK );
    
    sTableMap = QC_SHARED_TMPLATE( aStatement )->tableMap;

    for ( i = 0; i < QC_SHARED_TMPLATE( aStatement )->tmplate.rowCount; i++ )
    {
        if ( sTableMap[i].from != NULL )
        {
            sTableRef = sTableMap[i].from->tableRef;
            if ( ( sTableRef != NULL ) &&
                 ( sTableRef->remoteTable != NULL ) )
            {
                QC_STR_COPY( sDatabaseLinkName, sTableRef->remoteTable->linkName );

                IDE_TEST( gCallback.mInvalidResultSetMetaCache(
                              sDatabaseLinkName,
                              sTableRef->remoteQuery )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( NO_CALLBACK )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMR_NO_CALLBACK ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
