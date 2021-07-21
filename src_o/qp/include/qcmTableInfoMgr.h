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
 * $Id: qcmTableInfoMgr.h 19310 2006-12-06 01:56:51Z sungminee $
 **********************************************************************/

#ifndef _O_QCM_TABLE_INFO_MGR_H_
#define _O_QCM_TABLE_INFO_MGR_H_ 1

#include <smiDef.h>
#include <qc.h>
#include <qcmTableInfo.h>

class qcmTableInfoMgr
{
public:

    // tableInfo를 새로 생성한다.
    static IDE_RC makeTableInfoFirst( qcStatement   * aStatement,
                                      UInt            aTableID,
                                      smOID           aTableOID,
                                      qcmTableInfo ** aNewTableInfo );

    // 기존의 tableInfo가 있으며 새로운 tableInfo로 변경한다.
    static IDE_RC makeTableInfo( qcStatement   * aStatement,
                                 qcmTableInfo  * aOldTableInfo,
                                 qcmTableInfo ** aNewTableInfo );

    // tableInfo를 삭제한다.
    // 새로운 tableInfo가 생성되어있다면, 새로운 tableInfo를 삭제한다.
    static IDE_RC destroyTableInfo( qcStatement  * aStatement,
                                    qcmTableInfo * aTableInfo );

    // execute중 생성된 old tableInfo를 삭제한다.
    static void   destroyAllOldTableInfo( qcStatement  * aStatement );

    // execute중 생성된 new tableInfo를 삭제하고 원복한다.
    static void   revokeAllNewTableInfo( qcStatement  * aStatement );
};

#endif /* _O_QCM_TABLE_INFO_H_ */
