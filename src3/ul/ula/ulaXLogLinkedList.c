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
 

/***********************************************************************
 * $Id: ulaXLogLinkedList.c 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <acp.h>
#include <acl.h>
#include <ace.h>

#include <ulaXLogLinkedList.h>

ACI_RC ulaXLogLinkedListInitialize(ulaXLogLinkedList *aList,
                                   acp_bool_t         aUseMutex,
                                   ulaErrorMgr       *aOutErrorMgr)
{
    acp_rc_t sRc;

    aList->mHeadPtr  = NULL;
    aList->mTailPtr  = NULL;
    aList->mUseMutex = aUseMutex;

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexCreate(&aList->mMutex, ACP_THR_MUTEX_DEFAULT);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_INIT);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_INIT)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_INITIALIZE,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaXLogLinkedListDestroy(ulaXLogLinkedList *aList,
                                ulaErrorMgr       *aOutErrorMgr)
{
    acp_rc_t sRc;

    ACI_TEST_RAISE((aList->mHeadPtr != NULL) || (aList->mTailPtr != NULL),
                   ERR_NOT_EMPTY);

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexDestroy(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_DESTROY);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_NOT_EMPTY)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_LINKEDLIST_NOT_EMPTY);
    }
    ACI_EXCEPTION(ERR_MUTEX_DESTROY)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_DESTROY,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  Insertion
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogLinkedListInsertToHead(ulaXLogLinkedList *aList,
                                     ulaXLog           *aXLog,
                                     ulaErrorMgr       *aOutErrorMgr)
{
    acp_rc_t sRc;

    ACE_DASSERT(aXLog != NULL);

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexLock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_LOCK);
    }

    if (aList->mHeadPtr == NULL)                // empty
    {
        ACE_DASSERT(aList->mTailPtr == NULL);

        aList->mHeadPtr = aXLog;
        aList->mTailPtr = aXLog;

        aXLog->mPrev = NULL;
        aXLog->mNext = NULL;
    }
    else                                // >= 1
    {
        aList->mHeadPtr->mPrev = aXLog;

        aXLog->mPrev = NULL;
        aXLog->mNext = aList->mHeadPtr;

        aList->mHeadPtr = aXLog;
    }

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexUnlock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_UNLOCK);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_LOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_UNLOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaXLogLinkedListInsertToTail(ulaXLogLinkedList *aList,
                                     ulaXLog           *aXLog,
                                     ulaErrorMgr       *aOutErrorMgr)
{
    acp_rc_t sRc;

    ACE_DASSERT(aXLog != NULL);

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexLock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_LOCK);
    }

    if (aList->mHeadPtr == NULL)                // empty
    {
        ACE_DASSERT(aList->mTailPtr == NULL);

        aList->mHeadPtr = aXLog;
        aList->mTailPtr = aXLog;

        aXLog->mPrev = NULL;
        aXLog->mNext = NULL;
    }
    else                                // >= 1
    {
        aList->mTailPtr->mNext = aXLog;

        aXLog->mPrev = aList->mTailPtr;
        aXLog->mNext = NULL;

        aList->mTailPtr = aXLog;
    }

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexUnlock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_UNLOCK);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_LOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_UNLOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  Deletion
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogLinkedListRemoveFromHead(ulaXLogLinkedList  *aList,
                                       ulaXLog           **aOutXLog,
                                       ulaErrorMgr        *aOutErrorMgr)
{
    acp_rc_t sRc;

    ACE_DASSERT(aOutXLog != NULL);

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexLock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_LOCK);
    }

    if (aList->mHeadPtr == NULL)                // empty
    {
        ACE_DASSERT(aList->mTailPtr == NULL);

        *aOutXLog = NULL;
    }
    else if (aList->mHeadPtr == aList->mTailPtr)       // 1
    {
        *aOutXLog = aList->mHeadPtr;

        aList->mHeadPtr = NULL;
        aList->mTailPtr = NULL;
    }
    else                                // > 1
    {
        *aOutXLog = aList->mHeadPtr;

        aList->mHeadPtr        = aList->mHeadPtr->mNext;
        aList->mHeadPtr->mPrev = NULL;
    }

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexUnlock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_UNLOCK);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_LOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_UNLOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaXLogLinkedListRemoveFromTail(ulaXLogLinkedList  *aList,
                                       ulaXLog           **aOutXLog,
                                       ulaErrorMgr        *aOutErrorMgr)
{
    acp_rc_t sRc;

    ACE_DASSERT(aOutXLog != NULL);

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexLock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_LOCK);
    }

    if (aList->mHeadPtr == NULL)                // empty
    {
        ACE_DASSERT(aList->mTailPtr == NULL);

        *aOutXLog = NULL;
    }
    else if (aList->mHeadPtr == aList->mTailPtr)       // 1
    {
        *aOutXLog = aList->mHeadPtr;

        aList->mHeadPtr = NULL;
        aList->mTailPtr = NULL;
    }
    else                                // > 1
    {
        *aOutXLog = aList->mTailPtr;

        aList->mTailPtr        = aList->mTailPtr->mPrev;
        aList->mTailPtr->mNext = NULL;
    }

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexUnlock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_UNLOCK);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_LOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_UNLOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  Peeping :-)
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogLinkedListPeepHead(ulaXLogLinkedList  *aList,
                                 ulaXLog           **aOutXLog,
                                 ulaErrorMgr        *aOutErrorMgr)
{
    acp_rc_t sRc;

    ACE_DASSERT(aOutXLog != NULL);

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexLock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_LOCK);
    }

    if (aList->mHeadPtr == NULL)                // empty
    {
        ACE_DASSERT(aList->mTailPtr == NULL);

        *aOutXLog = NULL;
    }
    else                                // >= 1
    {
        *aOutXLog = aList->mHeadPtr;
    }

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexUnlock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_UNLOCK);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_LOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_UNLOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaXLogLinkedListPeepTail(ulaXLogLinkedList  *aList,
                                 ulaXLog           **aOutXLog,
                                 ulaErrorMgr        *aOutErrorMgr)
{
    acp_rc_t sRc;

    ACE_DASSERT(aOutXLog != NULL);

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexLock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_LOCK);
    }

    if (aList->mHeadPtr == NULL)                // empty
    {
        ACE_DASSERT(aList->mTailPtr == NULL);

        *aOutXLog = NULL;
    }
    else                                // >= 1
    {
        *aOutXLog = aList->mTailPtr;
    }

    if (aList->mUseMutex == ACP_TRUE)
    {
        sRc = acpThrMutexUnlock(&aList->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ERR_MUTEX_UNLOCK);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_MUTEX_LOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_LOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION(ERR_MUTEX_UNLOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_FATAL_MUTEX_UNLOCK,
                        ULA_LINKEDLIST_MUTEX_NAME);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
