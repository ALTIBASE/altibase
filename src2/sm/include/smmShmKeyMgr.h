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
 * $Id: smmShmKeyMgr.h 18299 2006-09-26 07:51:56Z xcom73 $
 **********************************************************************/

#ifndef _O_SMM_SHM_KEY_MGR_H_
#define _O_SMM_SHM_KEY_MGR_H_ (1)

#include <idl.h>
#include <idu.h>
#include <smm.h>
#include <smmDef.h>
#include <smmShmKeyList.h>

/*
    smmShmKeyMgr - DB의 공유메모리 Key 후보를 관리한다.

    [PROJ-1548] User Memory Tablespace
    
    - 공유 메모리 키 후보란?
      - 공유메모리 Key가 사용 가능한지 여부를
        직접 공유메모리 영역을  생성해봐야 알 수 있다.
      - 해당 Key를 이용하여 새 공유메모리 영역을 생성할 수 있을 수도
        있고 이미 해당 Key로 공유메모리 영역이 생성되어 있어서
        사용이 불가능한 Key일 수도 있다.

    - 사용가능한 공유메모리키를 찾는 알고리즘 
      - SHM_DB_KEY 프로퍼티를 시작으로 1씩 감소해가며
        사용가능한 Key를 찾는 방식으로
        
    - Tablespace별 공유메모리 키 관리
      - Tablespace별로 공유메모리 Key범위가 따로 분리되어 있지 않다.
      - 즉, SHM_DB_KEY로부터 시작하여 하나씩 감소해가며 여러
        테이블 스페이스가 공유메모리 키를 나누어 가져간다.

    - 동시성 제어
      - 여러 Tablespace에 접근하는 여러 Thread가 동시에
        공유메모리 Key후보를 가져오려 할 수 있다.
      - Mutex를 두어 한번에 하나의 Thread만이 공유 메모리 Key 후보를
        가져갈 수 있도록 구현한다.

    - 공유메모리 Key의 재활용
      - 가능하면 이전에 다른 Tablespace에서 사용되었던 Key를
        재활용하여 사용한다.
        
      -  Tablespace drop시에 Tablespace에서 사용중이던 공유메모리 Key를
         다른 Tablespace에서 재활용할 수 있어야 한다.

         - 문제
           Tablespace 가 drop될때 해당 Tablespace에서 사용중이던
           공유메모리 Key를 재활용하지 않을 경우
           Tablespace create/drop을 반복하면 공유메모리 Key가
           고갈되는 문제가 생김

         - 해결책
           Tablespace가 drop될때 해당 Tablespace에서 사용중이던
           공유메모리 Key를 재활용한다.
       
 */

class smmShmKeyMgr
{
private :
    static iduMutex      mMutex;       // mSeekKey를 보호하는 Mutex
    static smmShmKeyList mFreeKeyList; // 재활용 공유메모리 Key List
    // 재활용 Key가 없을때 1씩 감소해가며 공유메모리 후보키 찾을 기준 키
    static key_t         mSeekKey;     
    
    
    smmShmKeyMgr();
    ~smmShmKeyMgr();

public :

    // 다음 사용할 공유메모리 Key 후보를 찾아 리턴한다.
    static IDE_RC getShmKeyCandidate(key_t * aShmKeyCandidate) ;


    // 현재 사용중인 Key를 알려준다.
    static IDE_RC notifyUsedKey(key_t aUsedKey);

    // 더이상 사용되지 않는 Key를 알려준다.
    static IDE_RC notifyUnusedKey(key_t aUnusedKey);
    
    // shmShmKeyMgr의 static 초기화 수행 
    static IDE_RC initializeStatic();
    

    // shmShmKeyMgr의 static 파괴 수행 
    static IDE_RC destroyStatic();

    static IDE_RC lock() { return mMutex.lock( NULL ); }
    static IDE_RC unlock() { return mMutex.unlock(); }
    
};


#endif // _O_SMM_SHM_KEY_MGR_H_
