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
 * $Id: qcmCache.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcm.h>
#include <qcmCache.h>
#include <qcuSqlSourceInfo.h>

IDE_RC qcmCache::getColumn(
    qcStatement     * aStatement,
    qcmTableInfo    * aTableInfo,
    qcNamePosition    aColumnName,
    qcmColumn      ** aColumn)
{
    qcuSqlSourceInfo     sqlInfo;
    
    IDE_TEST(getColumnByName(aTableInfo,
                             aColumnName.stmtText+aColumnName.offset,
                             aColumnName.size,
                             aColumn)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    {
        sqlInfo.setSourceInfo( aStatement,
                               & aColumnName );

        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_COLUMN_NAME,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }

    return IDE_FAILURE;
}

IDE_RC qcmCache::getIndex(
    qcmTableInfo    * aTableInfo,
    qcNamePosition    aIndexName,
    qcmIndex       ** aIndex)
{
    UInt    i;

    *aIndex = NULL;

    for (i = 0; i < aTableInfo->indexCount; i++)
    {
        // fix BUG-33355
        if ( idlOS::strMatch( aTableInfo->indices[i].name,
                              idlOS::strlen( aTableInfo->indices[i].name ),
                              aIndexName.stmtText + aIndexName.offset,
                              (UInt)aIndexName.size ) == 0 )
        {
            *aIndex = &aTableInfo->indices[i];
            break;
        }
    }

    IDE_TEST_RAISE(*aIndex == NULL, ERR_NOT_EXIST_INDEX);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_INDEX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXISTS_INDEX));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


qcmUnique * qcmCache::getUniqueByCols( qcmTableInfo * aTableInfo,
                                       UInt           aKeyColCount,
                                       UInt         * aKeyCols,
                                       UInt         * aKeyColsFlag)
{
/***********************************************************************
 *
 * Description :
 *    프로시져와 관련된 오브젝트에 대한 정보를 삭제한다.
 *
 * Implementation :
 *    aTableInfo 의 uniqueKey 중에서 명시한 aKeyCols, aKeyColsFlag 와
 *    동일한 것을 찾아서 반환, 없으면 NULL 이 반환됨
 *
 ***********************************************************************/

    UInt        i;
    UInt        j;
    idBool      sMatchFound = ID_FALSE;
    qcmIndex  * sIndex;
    qcmUnique * sUnique;

    sUnique     = aTableInfo->uniqueKeys;

    for (i = 0; i < aTableInfo->uniqueKeyCount; i++)
    {
        sIndex = aTableInfo->uniqueKeys[i].constraintIndex;

        if (sIndex->keyColCount == aKeyColCount)
        {
            sMatchFound = ID_TRUE;
            for (j = 0; j < aKeyColCount; j++)
            {
                // To Fix PR-10247
                // Column의 ID와 Column의 Order를 검사해야 함.
                if ( sIndex->keyColumns[j].column.id != aKeyCols[j] ||
                     (sIndex->keyColsFlag[j] & SMI_COLUMN_ORDER_MASK)
                     != (aKeyColsFlag[j] & SMI_COLUMN_ORDER_MASK) )
                {
                    sMatchFound = ID_FALSE;
                }
            }
            if (sMatchFound == ID_TRUE)
            {
                sUnique = &aTableInfo->uniqueKeys[i];
                break;
            }
        }
    }

    if (sMatchFound == ID_FALSE)
    {
        sUnique = NULL;
    }

    return sUnique;
}

UInt qcmCache::getConstraintIDByName( qcmTableInfo  * aTableInfo,
                                      SChar         * aConstraintName,
                                      qcmIndex     ** aConstraintIndex )
{
    UInt i;
    UInt sConstrID = 0;

    // search UNIQUE KEY
    for (i = 0; i < aTableInfo->uniqueKeyCount; i++)
    {
        if ( idlOS::strMatch( aConstraintName,
                              idlOS::strlen( aConstraintName ),
                              aTableInfo->uniqueKeys[i].name,
                              idlOS::strlen( aTableInfo->uniqueKeys[i].name ) ) == 0 )
        {
            if (aConstraintIndex != NULL)
            {
                *aConstraintIndex = aTableInfo->uniqueKeys[i].constraintIndex;
            }

            sConstrID = aTableInfo->uniqueKeys[i].constraintID;
        }
    }

    // search FOREIGN KEY
    if (sConstrID == 0) // not found! we continue.
    {
        for (i = 0; i < aTableInfo->foreignKeyCount; i++)
        {
            if ( idlOS::strMatch( aConstraintName,
                                  idlOS::strlen( aConstraintName ),
                                  aTableInfo->foreignKeys[i].name,
                                  idlOS::strlen( aTableInfo->foreignKeys[i].name ) ) == 0 )
            {
                if (aConstraintIndex != NULL)
                {
                    *aConstraintIndex = NULL;
                }
                sConstrID =aTableInfo->foreignKeys[i].constraintID;
            }
        }
    }

    // search NOT NULL
    if (sConstrID == 0) // not found we still continue.
    {
        for (i = 0; i < aTableInfo->notNullCount; i++)
        {
            if ( idlOS::strMatch( aConstraintName,
                                  idlOS::strlen( aConstraintName ),
                                  aTableInfo->notNulls[i].name,
                                  idlOS::strlen( aTableInfo->notNulls[i].name ) ) == 0 )
            {
                if (aConstraintIndex != NULL)
                {
                    *aConstraintIndex = NULL;
                }
                sConstrID = aTableInfo->notNulls[i].constraintID;
            }
        }
    }

    /* PROJ-1107 Check Constraint 지원
     *  search CHECK
     */
    if ( sConstrID == 0 ) /* not found we still continue. */
    {
        for ( i = 0; i < aTableInfo->checkCount; i++ )
        {
            if ( idlOS::strMatch( aConstraintName,
                                  idlOS::strlen( aConstraintName ),
                                  aTableInfo->checks[i].name,
                                  idlOS::strlen( aTableInfo->checks[i].name ) ) == 0 )
            {
                if ( aConstraintIndex != NULL )
                {
                    *aConstraintIndex = NULL;
                }
                else
                {
                    /* Nothing to do */
                }

                sConstrID = aTableInfo->checks[i].constraintID;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    // search TIMESTAMP
    if (sConstrID == 0) // not found we still continue.
    {
        if (aTableInfo->timestamp != NULL)
        {
            if ( idlOS::strMatch( aConstraintName,
                                  idlOS::strlen( aConstraintName ),
                                  aTableInfo->timestamp->name,
                                  idlOS::strlen( aTableInfo->timestamp->name ) ) == 0 )
            {
                if (aConstraintIndex != NULL)
                {
                    *aConstraintIndex = NULL;
                }
                sConstrID = aTableInfo->timestamp->constraintID;
            }
        }
    }

    return sConstrID;
}

IDE_RC qcmCache::getColumnByID( qcmTableInfo   * aTableInfo,
                                UInt             aColumnID,
                                qcmColumn     ** aColumn )
{
    UInt i;

    *aColumn = NULL;

    for (i = 0; i < aTableInfo->columnCount; i++)
    {
        if ( aTableInfo->columns[i].basicInfo->column.id == aColumnID )
        {
            *aColumn = &aTableInfo->columns[i];
            break;
        }
    }

    IDE_TEST_RAISE(*aColumn == NULL, ERR_NOT_EXIST_COLUMN);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_COLUMN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmCache::getColumnByName(qcmTableInfo    * aTableInfo,
                                 SChar           * aColumnName,
                                 SInt              aColumnNameLen,
                                 qcmColumn      ** aColumn)
{
    UInt i;

    *aColumn = NULL;

    for (i = 0; i < aTableInfo->columnCount; i++)
    {
        if ( idlOS::strMatch( aTableInfo->columns[i].name,
                              idlOS::strlen( aTableInfo->columns[i].name ),
                              aColumnName,
                              (UInt)aColumnNameLen ) == 0 )
        {
            *aColumn = &aTableInfo->columns[i];
            break;
        }
    }

    IDE_TEST_RAISE(*aColumn == NULL, ERR_NOT_EXIST_COLUMN);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_COLUMN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmCache::getIndexByID( qcmTableInfo * aTableInfo,
                               UInt           aIndexID,
                               qcmIndex    ** aIndex)
{
    UInt i;

    *aIndex = NULL;
            
    for (i = 0; i < aTableInfo->indexCount; i ++)
    {
        if (aTableInfo->indices[i].indexId == aIndexID)
        {
            *aIndex = &(aTableInfo->indices[i]);
            break;
        }
    }
    IDE_TEST_RAISE( *aIndex == NULL,
                    ERR_NOT_EXIST_INDEX);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_INDEX);
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXISTS_INDEX ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmCache::getPartKeyColumn( qcStatement     * aStatement,
                                   qcmTableInfo    * aPartTableInfo,
                                   qcNamePosition    aColumnName,
                                   qcmColumn      ** aColumn )
{
    qcuSqlSourceInfo     sqlInfo;

    IDE_TEST(getPartKeyColumnByName(aPartTableInfo,
                                    aColumnName.stmtText+aColumnName.offset,
                                    aColumnName.size,
                                    aColumn)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    {
        sqlInfo.setSourceInfo( aStatement,
                               & aColumnName );

        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_COLUMN_NAME,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }

    return IDE_FAILURE;
}

IDE_RC qcmCache::getPartKeyColumnByName( qcmTableInfo    * aPartTableInfo,
                                         SChar           * aColumnName,
                                         SInt              aColumnNameLen,
                                         qcmColumn      ** aColumn )
{
    UInt i;

    *aColumn = NULL;

    for (i = 0; i < aPartTableInfo->partKeyColCount; i++)
    {
        if ( idlOS::strMatch( aPartTableInfo->partKeyColumns[i].name,
                              idlOS::strlen( aPartTableInfo->partKeyColumns[i].name ),
                              aColumnName,
                              (UInt)aColumnNameLen ) == 0 )
        {
            *aColumn = &aPartTableInfo->partKeyColumns[i];
            break;
        }
    }

    IDE_TEST_RAISE(*aColumn == NULL, ERR_NOT_EXIST_COLUMN);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_COLUMN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
