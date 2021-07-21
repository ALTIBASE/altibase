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
 * $Id: qdtAlter.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef  _O_QDT_ALTER_H_
#define  _O_QDT_ALTER_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

class qdtAlter
{
private:
    
    // Disk Data tablespace, Disk Temp Tablespace용 ALTER구문에 대한
    // SM의 Tablespace Type조회
    static IDE_RC isDiskTBSType(smiTableSpaceType aQueryTBSType,
                                smiTableSpaceType aStorageTBSType );

    // Memory data tablespace용 ALTER구문에 대한
    // SM의 Tablespace Type조회
    static IDE_RC isMemTBSType(smiTableSpaceType aTBSType );
    
public:
    // validation
    static IDE_RC validateAddFile( qcStatement * aStatement );
    static IDE_RC validateRenameOrDropFile( qcStatement * aStatement );
    static IDE_RC validateModifyFile( qcStatement * aStatement );
    static IDE_RC validateAlterMemVolTBSAutoExtend(qcStatement * aStatement);
    static IDE_RC validateTBSOnlineOrOffline(qcStatement * aStatement);
    //  Tablespace의 Attribute Flag 설정에 대한 Validation함수
    static IDE_RC validateAlterTBSAttrFlag(qcStatement * aStatement);

    static IDE_RC validateAlterTBSRename(qcStatement * aStatement);
    
    // execution
 	static IDE_RC executeTBSOnlineOrOffline( qcStatement * aStatement );
 	static IDE_RC executeAddFile( qcStatement * aStatement );
 	static IDE_RC executeRenameFile( qcStatement * aStatement );
 	static IDE_RC executeModifyFileAutoExtend( qcStatement * aStatement );
 	static IDE_RC executeModifyFileSize( qcStatement * aStatement );
 	static IDE_RC executeModifyFileOnOffLine( qcStatement * aStatement );
 	static IDE_RC executeDropFile( qcStatement * aStatement );
    static IDE_RC executeTBSBackup( qcStatement * aStatement );
    static IDE_RC executeAlterTBSDiscard(qcStatement * aStatement);
    static IDE_RC executeAlterMemoryTBSChkptPath(qcStatement * aStatement);
    static IDE_RC executeAlterMemVolTBSAutoExtend(qcStatement * aStatement);

    // Tablespace의 Attribute Flag 설정에 대한 실행함수
    static IDE_RC executeAlterTBSAttrFlag( qcStatement * aStatement );

    static IDE_RC executeAlterTBSRename(qcStatement * aStatement);

private:

};


#endif // _O_QDT_ALTER_H_
