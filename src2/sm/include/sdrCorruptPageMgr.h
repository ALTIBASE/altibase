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
 * $Id$
 *
 * Description :
 *
 * 본 파일은 DRDB 복구과정에서의 Corrupt page 관리자에 대한 헤더 파일이다.
 *
 * DRDB에 대한 restart recovery 과정중 corrupted page를 발견 하였을 시
 * 그에 대한 처리를 위한 class이다.
 *
 * restart redo 과정 중 corrupt page를 발견하면 corruptedPagesHash에
 * 모아 두었다가 undo과정을 모두 마친 후 hash 에 포함된 page에 대하여
 * page가 속한 extent가 free상태인지 확인한다.
 * free이면 무시하고 alloc된 상태이면 서버를 종료시킨다.
 *
 * 서버 종료 여부는 CORRUPT_PAGE_ERR_POLICY 프로퍼티로 정할수 있다
 *
 * 0 - restart recovery 과정 중에 corrupt page를 발견 했더라도
 *     서버를 종료시키지 않는다. (default)
 * 1 - restart recovery 과정 중에 corrupt page를 발견한 경우
 *     corrupt page를 발견한 경우 서버를 종료한다.
 *
 * restart redo과정 중에 corrupt page를 발견한 경우에만 해당되며
 * restart undo과정 중에 corrupt page를 발견하였거나,
 * GG, LG Hdr page가 corrupt로 발견 된 경우
 * 프로퍼티와 상관 없이 바로 종료된다.
 *
 **********************************************************************/
#ifndef _O_SDR_CORRUPT_PAGE_MGR_H_
#define _O_SDR_CORRUPT_PAGE_MGR_H_ 1

#include <sdrDef.h>

class sdrCorruptPageMgr
{
public:

    static IDE_RC initialize( UInt  aHashTableSize );

    static IDE_RC destroy();

    // Corrupt Page를 mCorruptedPages 에 등록
    static IDE_RC addCorruptPage( scSpaceID aSpaceID,
                                  scPageID  aPageID );

    // Corrupt Page를 mCorruptedPages 에서 삭제
    static IDE_RC delCorruptPage( scSpaceID aSpaceID,
                                  scPageID  aPageID );
    // Corrupted Page 상태 검사
    static IDE_RC checkCorruptedPages();

    static idBool isOverwriteLog( sdrLogType aLogType );

    static void allowReadCorruptPage();

    static void fatalReadCorruptPage();

    /* BUG-45598: CORRUPT_PAGE_ERR_POLICY 프로퍼티에 정의된 값에 따라서 
     * Error처리 정책 셋팅.
     */ 
    static void setPageErrPolicyByProp();

    static inline sdbCorruptPageReadPolicy getCorruptPageReadPolicy();

private:
    // 해쉬 key 생성 함수
    static UInt genHashValueFunc( void* aGRID );

    // 해쉬 비교 함수
    static SInt compareFunc( void* aLhs, void* aRhs );

private:
    // corrupted page가 저장된 Hash
    static smuHashBase   mCorruptedPages;

    // 해시 크기
    static UInt          mHashTableSize;

    // corrupt page를 읽었을 때의 정책 (fatal,abort,pass)
    static sdbCorruptPageReadPolicy mCorruptPageReadPolicy;
};

/****************************************************************
 * Description: corrupt page 를 읽었을 때 대처에 대한 정책을 설정한다.
 ****************************************************************/
inline sdbCorruptPageReadPolicy sdrCorruptPageMgr::getCorruptPageReadPolicy()
{
    return mCorruptPageReadPolicy;
}


#endif // _O_SDR_CORRUPT_PAGE_MGR_H_
