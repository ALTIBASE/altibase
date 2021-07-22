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
 * $Id: qcmDump.h 13836 2007-02-05 01:52:33Z leekmo $
 *
 * Description :
 *
 *    D$XXXX 계열의 DUMP OBJECT를 위한 정보 획득 등을 관리한다.
 *
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 *
 **********************************************************************/

#ifndef _O_QCM_DUMP_H_
#define _O_QCM_DUMP_H_ 1

#include    <iduFixedTable.h>
#include    <smiDef.h>
#include    <qc.h>
#include    <qtc.h>

class qcmDump
{
public:
    static IDE_RC getDumpObjectInfo( qcStatement * aStatement,
                                     qmsTableRef * aTableRef );
private:

    // 인덱스 이름을 인자로 넣으면, 인덱스 헤더(smnIndexHeader)
    // 를 dumpObj로 넣어준다.
    // for D$*_INDEX_*
    static IDE_RC getIndexInfo( qcStatement    * aStatement,
                                qmsDumpObjList * aDumpObjList,
                                idBool           aEnableDiskIdx,
                                idBool           aEnableMemIdx,
                                idBool           aEnableVolIdx );


    // for D$*_TABLE_*
    // 테이블 이름을 인자로 넣으면, 테이블 헤더(smcTableHeader)
    // 를 dumpObj로 넣어준다.
    static IDE_RC getTableInfo( qcStatement    * aStatement,
                                qmsDumpObjList * aDumpObjList,
                                idBool           aEnableDiskTable,
                                idBool           aEnableMemTable,
                                idBool           aEnableVolTable );
    
    // for D$*_TBS_*
    // 테이블 스페이스 이름을 인자로 넣으면, 테이블스페이스ID
    // 를 dumpObj로 넣어준다.
    static IDE_RC getTBSID( qcStatement    * aStatement,
                            qmsDumpObjList * aDumpObjList,
                            idBool           aEnableDiskTBS,
                            idBool           aEnableMemTBS,
                            idBool           aEnableVolTBS );

    // for D$*_DB_*
    /* TASK-4007 [SM] PBT를 위한 기능 추가
     * Disk, Memory, volatile Tablespace의 페이지를 dump하기 위해
     * SID와 PID를 인자로 받아들임 */
    static IDE_RC getGRID( qcStatement    * aStatement,
                           qmsDumpObjList * aDumpObjList,
                           idBool           aEnableDiskTBS,
                           idBool           aEnableMemTBS,
                           idBool           aEnableVolTBS );
};


#endif /* _O_QCM_DUMP_H_ */
