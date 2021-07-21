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
 * $Id: smrCompResList.h 15368 2006-03-23 01:14:43Z kmkim $
 **********************************************************************/

#ifndef _O_SMR_COMP_RES_LIST_H_
#define _O_SMR_COMP_RES_LIST_H_ 1

#include <idl.h>
#include <iduMemPool.h>
#include <iduMutex.h>
#include <smDef.h>
#include <smrDef.h>
#include <smuList.h>
#include <smuSynchroList.h>

/*
   Compression Resource List

   압축 리소스 풀에서 사용할 목적으로 구현된
   동시성 제어된 링크드 리스트

   smuSynchroList는 범용 클래스이기 때문에,
   압축 리소스 관련 코드를 넣을 수 없다.

   별도의 클래스로 서브 클래싱 하여
   압축 리소스 리스트로 사용한다.
*/

class smrCompResList : public smuSynchroList
{
public :
    /*  특정 시간동안 사용되지 않은 압축 리소스를
        Linked List의 Tail로부터 제거 */
    IDE_RC removeGarbageFromTail(
               UInt          aMinimumResourceCount,
               ULong         aGarbageCollectionMicro,
               smrCompRes ** aCompRes );
    
};

#endif /* _O_SMR_COMP_RES_LIST_H_ */
