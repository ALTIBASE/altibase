/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef _O_ULA_LINKEDLIST_H_
#define _O_ULA_LINKEDLIST_H_ 1

#include <ula.h>

#define ULA_LINKEDLIST_MUTEX_NAME   ((acp_char_t *)"LINKEDLIST_MUTEX")

typedef struct ulaXLogLinkedList
{
    acp_thr_mutex_t   mMutex;
    acp_bool_t        mUseMutex;

    ulaXLog          *mHeadPtr;    // if empty, then NULL
    ulaXLog          *mTailPtr;    // if empty, then NULL
} ulaXLogLinkedList;

/*
 * -----------------------------------------------------------------------------
 *  Initialize / Finalize
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogLinkedListInitialize(ulaXLogLinkedList *aList,
                                   acp_bool_t         aUseMutex,
                                   ulaErrorMgr       *aOutErrorMgr);
ACI_RC ulaXLogLinkedListDestroy(ulaXLogLinkedList *aList,
                                ulaErrorMgr       *aOutErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Insertion
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogLinkedListInsertToHead(ulaXLogLinkedList *aList,
                                     ulaXLog           *aXLog,
                                     ulaErrorMgr       *aOutErrorMgr);
ACI_RC ulaXLogLinkedListInsertToTail(ulaXLogLinkedList *aList,
                                     ulaXLog           *aXLog,
                                     ulaErrorMgr       *aOutErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Deletion
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogLinkedListRemoveFromHead(ulaXLogLinkedList  *aList,
                                       ulaXLog           **aOutXLog,
                                       ulaErrorMgr        *aOutErrorMgr);
ACI_RC ulaXLogLinkedListRemoveFromTail(ulaXLogLinkedList  *aList,
                                       ulaXLog           **aOutXLog,
                                       ulaErrorMgr        *aOutErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Peeping :-)
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogLinkedListPeepHead(ulaXLogLinkedList  *aList,
                                 ulaXLog           **aOutXLog,
                                 ulaErrorMgr        *aOutErrorMgr);
ACI_RC ulaXLogLinkedListPeepTail(ulaXLogLinkedList  *aList,
                                 ulaXLog           **aOutXLog,
                                 ulaErrorMgr        *aOutErrorMgr);

#endif /* _O_ULA_LINKEDLIST_H_ */
