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
 * $Id: qcmCache.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QCM_CACHE_H_
#define _O_QCM_CACHE_H_ 1

#include    <qc.h>
#include    <qcmTableInfo.h>

class qcmCache
{
public:
    static IDE_RC getColumn(qcStatement    * aStatement,
                            qcmTableInfo   * aTableInfo,
                            qcNamePosition   aColumnName,
                            qcmColumn     ** aColumn);

    static IDE_RC getIndex(qcmTableInfo    * aTableInfo,
                           qcNamePosition    aIndexName,
                           qcmIndex       ** aIndex);
    
    static qcmUnique * getUniqueByCols( qcmTableInfo * aTableInfo,
                                        UInt           aKeyColCount,
                                        UInt         * aKeyCols,
                                        UInt         * aKeyColsFlag);

    static IDE_RC getIndexByID( qcmTableInfo * aTableInfo,
                                UInt           aIndexID,
                                qcmIndex    ** aIndex );
    
    static IDE_RC getColumnByID( qcmTableInfo  *aTableInfo,
                                 UInt           aColumnID,
                                 qcmColumn    **aColumn);

    static IDE_RC getColumnByName(qcmTableInfo *aTableInfo,
                                  SChar        *aColumnName,
                                  SInt          aColumnNameLen,
                                  qcmColumn   **aColumn);

    static UInt getConstraintIDByName(qcmTableInfo  * aTableInfo,
                                      SChar         * aConstraintName,
                                      qcmIndex     ** aConstraintIndex);
    
    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC getPartKeyColumn(qcStatement    * aStatement,
                                   qcmTableInfo   * aPartTableInfo,
                                   qcNamePosition   aColumnName,
                                   qcmColumn     ** aColumn);
    
    static IDE_RC getPartKeyColumnByName( qcmTableInfo    * aPartTableInfo,
                                          SChar           * aColumnName,
                                          SInt              aColumnNameLen,
                                          qcmColumn      ** aColumn );
};

#endif // _O_QCM_CACHE_H_
