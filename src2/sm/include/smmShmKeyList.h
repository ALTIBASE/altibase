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

#ifndef _O_SMM_SHM_KEY_LIST_H_
#define _O_SMM_SHM_KEY_LIST_H_ (1)

#include <idl.h>
#include <idu.h>
#include <smm.h>
#include <smmDef.h>


/*
    smmShmKeyList - DB의 공유메모리 Key 리스트를 관리한다.

    - 지원 operation
      - addKey
        - 공유메모리 Key를 Key List에 추가한다.
      - removeKey
        - Key List에서 공유메모리 Key를 제거한다.
*/
    

class smmShmKeyList 
{
private :
    smuList      mKeyList;      // 공유메모리 Key List
    iduMemPool   mListNodePool; // 공유메모리 Key List의 Node Mempool

public :
    smmShmKeyList();
    ~smmShmKeyList();

    // 공유메모리 Key List를 초기화한다.
    IDE_RC initialize();
    
    // 공유메모리 Key List를 파괴한다
    IDE_RC destroy();
    
    // 재활용 Key를 재활용 Key List에 추가한다
    IDE_RC addKey(key_t aKey);

    // 재활용 Key List에서 재활용 Key를 빼낸다.
    IDE_RC removeKey(key_t * aKey);

    // 공유메모리 Key List안의 모든 Key를 제거한다
    IDE_RC removeAll();
    
    // 재활용 Key List가 비어있는지 체크한다.
    idBool isEmpty();

};


#endif /* _O_SMM_SHM_KEY_LIST_H_ */
