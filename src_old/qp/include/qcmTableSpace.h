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
 * $Id: qcmTableSpace.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QCM_TABLESPACE_H_
#define _O_QCM_TABLESPACE_H_ 1

#include <smiDef.h>
#include <qc.h>
#include <qcm.h>
#include <qtc.h>

class qcmTablespace
{
public:

    // 이름을 이용하여 TBS 정보를 획득
    static IDE_RC getTBSAttrByName( qcStatement       * aStatement,
                                    SChar             * aName,
                                    UInt                aNameLength,
                                    smiTableSpaceAttr * aTBSAttr );

    // ID를 이용하여 TBS 정보를 획득
    static IDE_RC getTBSAttrByID( scSpaceID           aTBSID,
                                  smiTableSpaceAttr * aTBSAttr );

    // 해당 TBS내에 Data File이 존재하는 지 검사
    static IDE_RC existDataFileInTBS( scSpaceID           aTBSID,
                                      SChar             * aName,
                                      UInt                aNameLength,
                                      idBool            * aExist );

    // 전체 DB에서 Data File이 존재하는 지 검사
    static IDE_RC existDataFileInDB( SChar             * aName,
                                     UInt                aNameLength,
                                     idBool            * aExist );

    static IDE_RC existObject(qcStatement      *aStatement,
                              scSpaceID         aTBSID,
                              idBool           *aExist);

    static IDE_RC findTableInfoListInTBS(
            qcStatement       *aStatement,
            scSpaceID          aTBSID,
            idBool             aIsForValidation,
            qcmTableInfoList **aTableInfoList);

    static IDE_RC findIndexInfoListInTBS(
            qcStatement       *aStatement,
            scSpaceID          aTBSID,
            idBool             aIsForValidation,
            qcmIndexInfoList **aIndexInfoList);

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC findTablePartInfoListInTBS(
            qcStatement       *aStatement,
            scSpaceID          aTBSID,
            qcmTableInfoList **aTablePartInfoList);

    static IDE_RC findIndexPartInfoListInTBS(
            qcStatement       *aStatement,
            scSpaceID          aTBSID,
            qcmIndexInfoList **aIndexPartInfoList);

    static IDE_RC checkAccessTBS(
            qcStatement       *aStatement,
            UInt               aUserID,
            scSpaceID          aTBSID);

    static IDE_RC existAccessTBS(
            smiStatement      *aSmiStmt,
            scSpaceID          aTBSID,
            UInt               aUserID,
            idBool            *aExist);

    // Alias => Tablespace 이름 으로의 Mapping 을 실시
    static IDE_RC lookupTBSNameAlias( SChar  * aAliasNamePtr,
                                      SChar ** aTBSNamePtr );
private :
};

#endif /* _O_QCM_TABLESPACE_H_ */












