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
 * $Id: qdbFlashback.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef  _O_QDB_FLASHBACK_H_
#define  _O_QDB_FLASHBACK_H_  1

#include <qc.h>

#define QDB_RECYCLEBIN_GENERATED_NAME_POSTFIX_LEN   (32)

class qdbFlashback
{
public:
    static IDE_RC dropToRecyclebin( qcStatement   * aStatement,
                                    qcmTableInfo  * aTableInfo );

    static IDE_RC validatePurgeTable( qcStatement * aStatement );
    static IDE_RC executePurgeTable( qcStatement * aStatement );
    static IDE_RC validateFlashbackTable( qcStatement * aStatement );
    static IDE_RC executeFlashbackTable( qcStatement * aStatement );

    static IDE_RC checkRecyclebinSpace( qcStatement  * aStatement,
                                        qcmTableInfo * aTableInfo,
                                        idBool       * aIsMemAvailable,
                                        idBool       * aIsDiskAvailable );
private:
    static IDE_RC getRecyclebinUsage( qcStatement * aStatement,
                                      ULong       * aMemUseage,
                                      ULong       * aDiskUseage );
    static IDE_RC flashbackFromRecyclebin( qcStatement   * aStatement,
                                           qcNamePosition  aTableNamePos,
                                           UInt            aTableID );
    static IDE_RC getTableIdByOriginalName( qcStatement    * aStatement,
                                            qcNamePosition   aOriginalNamePos,
                                            UInt             aUserID,
                                            idBool           aIsOLD,
                                            UInt           * aTableID );
};

#endif
