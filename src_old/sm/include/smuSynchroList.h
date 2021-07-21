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
 * $Id: smuSynchroList.h 15368 2006-03-23 01:14:43Z kmkim $
 **********************************************************************/

#ifndef _O_SMU_SYNCHRO_LIST_H_
#define _O_SMU_SYNCHRO_LIST_H_ 1

#include <idl.h>
#include <iduMemPool.h>
#include <iduMutex.h>
#include <smDef.h>
#include <smuList.h>

/*
   Synchronized List

   동시성 제어된 링크드 리스트의 구현
*/

class smuSynchroList
{
public :
    /* 객체 초기화 */
    IDE_RC initialize( SChar * aListName,
                       iduMemoryClientIndex aMemoryIndex);
    
    /* 객체 파괴 */
    IDE_RC destroy();

    /* Linked List의 Head에 Data를 Add */
    IDE_RC addToHead( void * aData );

    /* Linked List의 Tail에 Data를 Add */
    IDE_RC addToTail( void * aData );
    
    /* Linked List의 Head로부터 Data를 제거 */
    IDE_RC removeFromHead( void ** aData );

    /*  Linked List의 Tail로부터 Data를 제거 */
    IDE_RC removeFromTail( void ** aData );

    /* Linked List의 Tail을 리턴, 리스트에서 제거하지는 않음 */
    IDE_RC peekTail( void ** aData );

    /* Linked List안의 Element갯수를 리턴한다 */
    IDE_RC getElementCount( UInt * aElemCount );
    

protected :
    UInt       mElemCount;    /* Linked List안의 Element 갯수 */
    smuList    mList;         /* Linked List 의 Base Node */
    iduMemPool mListNodePool; /* Linked List Node의 Memory Pool */
    iduMutex   mListMutex;    /* 동시성 제어를 위한 Mutex */
};

#endif /* _O_SMU_SYNCHRO_LIST_H_ */
