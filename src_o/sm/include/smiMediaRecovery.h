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
 * $Id: smiMediaRecovery.h 82075 2018-01-17 06:39:52Z jina.kim $
 * Description :
 *
 * 본 파일은 media recovery 처리 class에대한 헤더 파일입니다.
 ***********************************************************************/

#ifndef _O_SMI_MEDIA_RECOVERY_H_
#define _O_SMI_MEDIA_RECOVERY_H_ 1

#include <idl.h>

class smiMediaRecovery
{

public:


    /////////////////////////////////////////////////////
    // 메모리/디스크 공통 복구

    // 미디어 오류가 감지된 메모리/디스크 데이타파일
    static IDE_RC recoverDB( idvSQL*        aStatistics,
                             smiRecoverType aRecoverType,
                             UInt           aUntilTIME,
                             SChar*         aUntilTag = NULL );// TODO 임시용 default인자 삭제 필요

    // Startup Control 단계에서 로그앵커를 참고하여 
    // EMPTY 디스크 데이타파일을 생성한다 
    static IDE_RC createDatafile( SChar* aOldFileSpec,
                                  SChar* aNewFileSpec );

    // Startup Control 단계에서 로그앵커를 참고하여 
    // EMPTY 메모리 데이타파일을 생성한다 
    static IDE_RC createChkptImage( SChar * aOldFileSpec );
                                  

    // Startup Control 단계에서 로그앵커의 
    // 디스크 데이타파일 경로를 변경한다.
    static IDE_RC renameDataFile( SChar* aOldFileName,
                                  SChar* aNewFileName );
    // PROJ-2133 incremental backup
    // incremental bakcup파일을 이용해 데이터베이스를
    // 복원한다.
    static IDE_RC restoreDB( smiRestoreType aRestoreType,   
                             UInt           aUntilTime,     
                             SChar*         aUntilTag );

    // PROJ-2133 incremental backup
    // incremental bakcup파일을 이용해 테이블스페이스를
    // 복원한다.
    static IDE_RC restoreTBS( scSpaceID aSpaceID );
};

#endif


