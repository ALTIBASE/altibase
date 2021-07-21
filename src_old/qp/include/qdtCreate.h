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
 * $Id: qdtCreate.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef  _O_QDT_CREATE_H_
#define  _O_QDT_CREATE_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

class qdtCreate
{
public:
    // validation
    static IDE_RC validateDiskDataTBS( qcStatement * aStatement );
    static IDE_RC validateDiskTemporaryTBS( qcStatement * aStatement );
    // CREATE MEMORY TABLESPACE에 대한 Validation
    static IDE_RC validateMemoryTBS( qcStatement * aStatement );
    static IDE_RC validateVolatileTBS( qcStatement * aStatement );

    // execution
    static IDE_RC executeDiskDataTBS( qcStatement * aStatement );
    static IDE_RC executeDiskTemporaryTBS( qcStatement * aStatement );
    static IDE_RC executeMemoryTBS(qcStatement * aStatement);
    static IDE_RC executeVolatileTBS( qcStatement * aStatement );

private:
    // 현재 접속한 사용자에게 특정 Tablespace 로의 접근권한을 준다.
    static IDE_RC grantTBSAccess(qcStatement * aStatement,
                                 scSpaceID     aTBSID );

    //  Tablespace의 Attribute Flag List로부터
    static IDE_RC calculateTBSAttrFlag(  qcStatement          * aStatement,
                                         qdCreateTBSParseTree * aCreateTBS );

    
    // Volatile Tablespace생성중 수행하는 에러처리

    static IDE_RC checkError4CreateVolatileTBS(
                      qdCreateTBSParseTree  * aCreateTBS );
    
};


#endif // _O_QDT_CREATE_H_
