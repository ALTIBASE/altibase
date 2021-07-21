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

#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmdDef.h>
#include <mmdManager.h>
#include <mmdXa.h>
#include <mmdXid.h>
#include <mmuProperty.h>
#include <mmtSessionManager.h>
#include <dki.h>

#define FLAG_IS_SET(F, S)              (((F) & (S)) == (S))

const SChar *MMDXASTATE[] =
{
    "XA_STATE_IDLE",
    "XA_STATE_ACTIVE",
    "XA_STATE_PREPARED",
    "XA_STATE_HEURISTICALLY_COMMITTED",
    "XA_STATE_HEURISTICALLY_ROLLBACKED",
    "XA_STATE_NON_EXIST_TRANSACTION",
    "XA_STATE_ROLLBACK_ONLY",
    "NONE"
};

/**
 * 임시로 참조만 할 경우,
 * 편의를 위해서 mmdXa::fix 대신 mmdXaSafeFind를 이용한다.
 *
 * mmdXaSafeFind()는 fix count를 증가시키지 않으므로
 * 도중에 예외가 발생하더라도 fix count와 관련된 별도의 처리를 할 필요가 없다.
 *
 * mmdXaSafeFind()로 얻은 mmdXid의 포인터를 이용해서
 * 포인터 값을 비교, 확인하는 것 외에 다른 연산을 해서는 안되며,
 * 가급적 mmdXa::fix()와 mmdXa::unFix() 사이에서 호출할 것을 권한다.
 *
 * @param aXidObjPtr
 *        xid 객체의 포인터를 담을 변수의 포인터
 * @param aXIDPtr
 *        mmdXidManager에서 검색할 xid 값
 * @param aXaLogFlag
 *        xid 객체를 찾았다는 것을 로그에 남길지 여부.
 *        mmdXidManager::find() 할 때, 세번째 인자로 넘길 값.
 *
 * @return mmdXidManager::find()에 실패한 경우, IDE_FAILURE.
 *         mmdXidManager::find() 실패는 smuHash::findNodeLatch()를 사용했을 때
 *         lock 획득, 해제에 실패하면 발생한다.
 *         그 외의 경우에는 IDE_ASSERT()로 검사하므로 실패하면 서버가 다운 될 것.
 */
IDE_RC mmdXaSafeFind(mmdXid **aXidObjPtr, ID_XID *aXIDPtr, mmdXaLogFlag aXaLogFlag)
{
    IDE_DASSERT((aXidObjPtr != NULL) && (aXIDPtr != NULL));

    UInt sStage = 0;
    UInt sBucket = mmdXidManager::getBucketPos(aXIDPtr);
/* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
   that is to say , chanage the granularity from global to bucket level.
*/               
    IDE_ASSERT(mmdXidManager::lockWrite(sBucket) == IDE_SUCCESS);
    sStage = 1;
    IDE_TEST(mmdXidManager::find(aXidObjPtr, aXIDPtr,sBucket,aXaLogFlag) != IDE_SUCCESS);
    sStage = 0;
    IDE_ASSERT(mmdXidManager::unlock(sBucket) == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sStage == 1)
        {
            IDE_ASSERT(mmdXidManager::unlock(sBucket) == IDE_SUCCESS);
        }
    }

    return IDE_FAILURE;
}

/**
 * ID_XID에 해당하는 mmdXid를 찾고
 * fix count를 1 증가시킨다.
 *
 * @param aXidObjPtr
 *        xid 객체의 포인터를 담을 변수의 포인터
 * @param aXIDPtr
 *        mmdXidManager에서 검색할 xid 값
 * @param aXaLogFlag
 *        xid 객체를 찾았다는 것을 로그에 남길지 여부.
 *        mmdXidManager::find() 할 때, 세번째 인자로 넘길 값.
 *
 * @return mmdXidManager::find()에 실패한 경우, IDE_FAILURE.
 */
IDE_RC mmdXa::fix(mmdXid **aXidObjPtr, ID_XID *aXIDPtr, mmdXaLogFlag aXaLogFlag)
{
    IDE_DASSERT((aXidObjPtr != NULL) && (aXIDPtr != NULL));

    mmdXid     *sXidObj  = NULL;
    UInt        sStage    = 0;
    UInt        sBucketPos = mmdXidManager::getBucketPos(aXIDPtr);
/* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
  that is to say , chanage the granularity from global to bucket level.
 */               
    
    /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
     fix(list-s-latch) , fixWithAdd(list-x-latch)로 분리시켜 fix 병목을 분산시킵니다. */
    IDE_ASSERT(mmdXidManager::lockRead(sBucketPos) == IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(mmdXidManager::find(&sXidObj, aXIDPtr,sBucketPos, aXaLogFlag) != IDE_SUCCESS);

    if (sXidObj != NULL)
    {
        /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
           list-s-latch로 잡았기때문에  xid fix-count정합성을 위하여
          xid latch를 잡고 fix count를 증가하는 fixWithLatch함수를 사용한다.*/
        sXidObj->fixWithLatch();
    }
    sStage = 0;
    IDE_ASSERT(mmdXidManager::unlock(sBucketPos) == IDE_SUCCESS);
    
    *aXidObjPtr = sXidObj;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch (sStage)
        {
            case 1:
                IDE_ASSERT(mmdXidManager::unlock(sBucketPos) == IDE_SUCCESS);
            default:
                break;
        }
    }

    return IDE_FAILURE;
}


/**
 * Caller가 미리 생성해둔 xid object를  xid list에 추가시도를 한다.
 * 이미 list에 있는 경우에는 기등록된 xid object를 활용하도록 한다.
 *
 * @param aXidObj2Add
 *         Caller가 latch duration을 줄이기 위하여 미리 생성해둔 xid object
 * @param aFixedXidObjPtr
 *        xid list에서 aXidPtr value로 검색하여 없으면 aXidObj2Add pointer가 되고,
 *        검색하여 있으면  검색된 xid object pointer가 된다.                               
 * @param aXaLogFlag
 *        xid 객체를 찾았다는 것을 로그에 남길지 여부.
 *        mmdXidManager::find() 할 때, 세번째 인자로 넘길 값.
 *
 * @return mmdXidManager::find()에 실패한 경우, IDE_FAILURE.
 */

IDE_RC mmdXa::fixWithAdd(mmdXaContext */*aXaContext*/,
                         mmdXid       *aXidObj2Add,
                         mmdXid      **aFixedXidObjPtr,
                         ID_XID       *aXIDPtr,
                         mmdXaLogFlag  aXaLogFlag)
{
    IDE_DASSERT(aXidObj2Add != NULL);
    IDE_DASSERT(aXIDPtr != NULL);
    IDE_DASSERT(aFixedXidObjPtr != NULL);

    mmdXid     *sXidObj  = NULL;
    UInt        sStage    = 0;
    /* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
       that is to say , chanage the granularity from global to bucket level. */           
    UInt sBucket = mmdXidManager::getBucketPos(aXIDPtr);

    /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
     fix(list-s-latch) , fixWithAdd(list-x-latch)로 분리시켜 fix 병목을 분산시킵니다. */
    IDE_ASSERT(mmdXidManager::lockWrite(sBucket) == IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(mmdXidManager::find(&sXidObj, aXIDPtr,sBucket,aXaLogFlag) != IDE_SUCCESS);
    if (sXidObj == NULL)
    {
        IDE_TEST(mmdXidManager::add(aXidObj2Add,sBucket) != IDE_SUCCESS);
        sXidObj = aXidObj2Add;
    }
    else
    {
        //nothing to do
    }

    sXidObj->fix();
    sStage = 0;
    IDE_ASSERT(mmdXidManager::unlock(sBucket) == IDE_SUCCESS);

    *aFixedXidObjPtr = sXidObj;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch (sStage)
        {
            case 1:
                IDE_ASSERT(mmdXidManager::unlock(sBucket) == IDE_SUCCESS);
            default:
                break;
        }
    }

    return IDE_FAILURE;
}

/**
 * ID_XID에 해당하는 mmdXid를 찾고
 * fix count를 1 감소시킨다.
 *
 * 만약, fix count가 0이 됐으면
 * 메모리를 반환한다.
 *
 * aXidObj가 NULL이면 아무것도 안한다.
 *
 * @param aXidObj
 *        xid 객체의 포인터
 * @param aXIDPtr
 *        aXidObj로 넘기는 객체의 ID_XID 값
 * @param aOptFlag
 *        unfix 할 때, 추가로 수행할 동작 지정.
 *        MMD_XA_REMOVE_FROM_XIDLIST만 허용되며 그 외의 옵션은 무시된다.
 *        추가로 수행할 동작이 없을 경우에는 명시적으로 MMD_XA_NONE를 넘길 것을 권장.
 *
 * @return mmdXidManager::find()에 실패한 경우, IDE_FAILURE.
 *         mmdXidManager::find() 실패는 smuHash::findNodeLatch()를 사용했을 때
 *         lock 획득, 해제에 실패하면 발생하며,
 *         aXidObj가 NULL이 아닐 경우에만 수행한다.
 *         MMD_XA_REMOVE_FROM_XIDLIST 옵션을 사용한 경우에만 해당하며,
 *         그 외의 경우에는 IDE_ASSERT()로 검사하므로 실패하면 서버가 다운 될 것.
 */
IDE_RC mmdXa::unFix(mmdXid *aXidObj, ID_XID * /*aXIDPtr*/, UInt aOptFlag)
{
    mmdXid     *sXidObj  = NULL;
    UInt        sFixCount;
    UInt        sStage   = 0;

    ID_XID     *sXIDPtr  = NULL;
    UInt        sBucket;
    
    if (aXidObj != NULL)
    {
        /* bug-35619: valgrind warning: invalid read of size 1
           aXIDPtr 인자를 잘 못 넘겨 사용하는 것을 막기 위해
           mmdXid 내부의 XID를 사용함.
           함수호출하는 곳이 많으므로 인자는 그대로 유지 */
        sXIDPtr = aXidObj->getXid();

        /* fix BUG-35374 To improve scalability about XA, latch
           granularity of XID hash should be more better than now.
           that is to say , chanage the granularity
           from global to bucket level. */
        sBucket = mmdXidManager::getBucketPos(sXIDPtr);

        IDE_ASSERT(mmdXidManager::lockWrite(sBucket) == IDE_SUCCESS);
        sStage = 1;

        aXidObj->unfix(&sFixCount);


        if (FLAG_IS_SET(aOptFlag, MMD_XA_REMOVE_FROM_XIDLIST))
        {
            IDE_TEST(mmdXidManager::find(&sXidObj, sXIDPtr,sBucket) != IDE_SUCCESS);

            if (sXidObj != NULL)
            {
                IDE_ASSERT(mmdXidManager::remove(sXidObj,&sFixCount) == IDE_SUCCESS);
            }
        }
        sStage = 0;
        IDE_ASSERT(mmdXidManager::unlock(sBucket) == IDE_SUCCESS);
       /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
        XA Unfix 시에 latch duaration을 줄이기위하여 xid fix-Count를 xid list latch release하고 나서 
        aXid Object를 해제 한다.*/
        if(sFixCount == 0)
        {    
            IDE_ASSERT(mmdXidManager::free(aXidObj, ID_TRUE)
                    == IDE_SUCCESS);
            aXidObj = NULL;
        }
    }
    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sStage == 1)
        {
            IDE_ASSERT(mmdXidManager::unlock(sBucket) == IDE_SUCCESS);
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmdXa::beginOpen(mmdXaContext *aXaContext)
{
    aXaContext->mReturnValue = XA_OK;

    return IDE_SUCCESS;
}

IDE_RC mmdXa::beginClose(mmdXaContext *aXaContext)
{
    IDE_TEST_RAISE(aXaContext->mSession->isXaSession() != ID_TRUE, InvalidXaSession);

    aXaContext->mReturnValue = XA_OK;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaSession);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_SESSION ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmdXa::beginStart(mmdXaContext     *aXaContext,
                         mmdXid           *aXid,
                         mmdXaAssocState  aXaAssocState,
                         UInt             *aState)
{
    PDL_Time_Value sSleepTime;
    PDL_Time_Value sMaxSleepTime;
    PDL_Time_Value sCurSleepTime;
    /* BUG-22935  같은  XID에 대하여 ,  xa_start with TMJOIN과 xa_commit,xa_rollback이 동시에
       수행될때 비정상 종료*/
    mmdXid        *sXid = NULL;
    ID_XID         sXidValue;

    IDE_TEST_RAISE(aXaContext->mSession->isXaSession() != ID_TRUE, InvalidXaSession);
    IDE_TEST_RAISE(aXaAssocState   == MMD_XA_ASSOC_STATE_ASSOCIATED,
                   NotAssociated);
    IDE_TEST_RAISE( (aXaContext->mFlag & TMRESUME) &&
                    (aXaAssocState  !=  MMD_XA_ASSOC_STATE_SUSPENDED),
                    InvalidXaState);

    if (aXid == NULL)
    {
        IDE_TEST_RAISE(MMD_XA_XID_RESTART_FLAG(aXaContext->mFlag), XidNotExist);
    }
    else
    {
        if (aXaContext->mFlag & TMNOWAIT)
        {
            IDE_TEST_RAISE(aXid->getState() == MMD_XA_STATE_ACTIVE, XidInUse);
        }
        else
        {
            if(aXid->getState() == MMD_XA_STATE_ACTIVE)
            {
                sSleepTime.set(0, MMD_XA_RETRY_SPIN_TIMEOUT);
                sCurSleepTime.set(0,0);
                sMaxSleepTime.set(mmuProperty::getQueryTimeout(),0);
                /* BUG-22935  같은  XID에 대하여 ,  xa_start with TMJOIN과 xa_commit,xa_rollback이 동시에
                   수행될때 비정상 종료*/
                idlOS::memcpy(&sXidValue,aXid->getXid(),ID_SIZEOF(sXidValue));
                *aState =1;
                aXid->unlock();
              retry:
                IDE_TEST(mmdXaSafeFind(&sXid,&sXidValue,MMD_XA_DO_NOT_LOG) != IDE_SUCCESS);
                IDE_TEST_RAISE(((sXid == NULL) ||( sXid != aXid)),XidNotExist);
                //fix BUG-27101, Xa BeginStart시에 lock성공이전에 state를 변경하고 있습니다.
                //그리고 Code-Sonar Null reference waring을 제거하기 위하여
                // sXid대신 aXid를 사용합니다( sXid 와 aXid와 같기때문입니다).
                aXid->lock();
                *aState =2;
                if ( (aXid->getState() == MMD_XA_STATE_ACTIVE ) &&  (sCurSleepTime < sMaxSleepTime  ) )
                {
                    *aState = 1;
                    //Code-Sonar null reference waring을 제거하기 위하여
                    //sXid대신 aXid를 사용합니다( sXid 와 aXid와 같기때문입니다).
                    aXid->unlock();
                    idlOS::sleep(sSleepTime);

                    sCurSleepTime += sSleepTime;
                    goto retry;
                }//if
            }//else
            else
            {
                //nothig to do
            }

        }//else

        /* BUG-22935  같은  XID에 대하여 ,  xa_start with TMJOIN과 xa_commit,xa_rollback이 동시에
         수행될때 비정상 종료*/
        IDE_TEST_RAISE((aXid->getState() != MMD_XA_STATE_IDLE), InvalidXaState);

        IDE_TEST_RAISE(!MMD_XA_XID_RESTART_FLAG(aXaContext->mFlag), XidAlreadyExist);
    }

    aXaContext->mReturnValue = XA_OK;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaSession);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_SESSION ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(NotAssociated);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_ASSOCIATED_BRANCH ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(InvalidXaState);
    {
        // bug-27071: xa: beginStart xid null ref
        if (aXid == NULL)
        {
            IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_STATE,
                                     "UNKNOWN XA_STATE: XID NULL"));
        }
        else
        {
            IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_STATE,
                                     MMDXASTATE[aXid->getState()] ));
        }
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(XidInUse);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_XID_INUSE ));
        aXaContext->mReturnValue = XA_RETRY;
    }
    IDE_EXCEPTION(XidNotExist);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_EXIST_XID ));
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION(XidAlreadyExist);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_ALREADY_EXIST_XID ));
        aXaContext->mReturnValue = XAER_DUPID;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mmdXa::beginEnd(mmdXaContext *aXaContext, mmdXid *aXid, mmdXid *aCurrentXid,
                       mmdXaAssocState  aXaAssocState)
{
    IDE_TEST_RAISE(aXaContext->mSession->isXaSession() != ID_TRUE, InvalidXaSession);
    // bug-26973: codesonar: xid null ref
    // xid null 검사하는 부분을 switch 이후에서 이전으로 옮김.
    IDE_TEST_RAISE(aXid == NULL, XidNotExist);

    switch (aXaContext->mSession->getXaAssocState())
    {
        case MMD_XA_ASSOC_STATE_NOTASSOCIATED:
            IDE_RAISE(NotAssociated);
            break;

        case MMD_XA_ASSOC_STATE_ASSOCIATED:
            break;

        case MMD_XA_ASSOC_STATE_SUSPENDED:
            IDE_TEST_RAISE(aXaContext->mFlag & TMSUSPEND, InvalidXaState);
            break;

        default:
            IDE_RAISE(InvalidXaState);
            break;
    }

    IDE_TEST_RAISE(aXid != aCurrentXid, InvalidXid);
    if(aXid->getState() != MMD_XA_STATE_ACTIVE)
    {

        //fix BUG-22561
        IDE_TEST_RAISE(aXaAssocState != MMD_XA_ASSOC_STATE_SUSPENDED,
                       NotAssociated);
    }
    else
    {
        //nothing to do
    }

    aXaContext->mReturnValue = XA_OK;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaSession);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_SESSION ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(NotAssociated);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_ASSOCIATED_BRANCH ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(XidNotExist);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_EXIST_XID ));
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION(InvalidXaState);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_STATE, MMDXASTATE[aXid->getState()] ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(InvalidXid);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XID ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmdXa::beginPrepare(mmdXaContext *aXaContext, mmdXid *aXid)
{
    IDE_TEST_RAISE(aXaContext->mSession->isXaSession() != ID_TRUE, InvalidXaSession);
    IDE_TEST_RAISE(aXaContext->mSession->getXaAssocState()
                   != MMD_XA_ASSOC_STATE_NOTASSOCIATED, InvalidXaSession);

    IDE_TEST_RAISE(aXid == NULL, XidNotExist);

    switch (aXid->getState())
    {
        case MMD_XA_STATE_IDLE:
            aXaContext->mReturnValue = XA_OK;
            break;

        case MMD_XA_STATE_ACTIVE:
        case MMD_XA_STATE_PREPARED:
        case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
        case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
        case MMD_XA_STATE_NO_TX:
        case MMD_XA_STATE_ROLLBACK_ONLY:
            IDE_RAISE(InvalidXaState);
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaSession);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_SESSION ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(XidNotExist);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_EXIST_XID ));
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION(InvalidXaState);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_STATE, MMDXASTATE[aXid->getState()] ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmdXa::beginCommit(mmdXaContext *aXaContext, mmdXid *aXid)
{
    IDE_TEST_RAISE(aXaContext->mSession->isXaSession() != ID_TRUE, InvalidXaSession);
    //IDE_TEST_RAISE(aXaContext->mSession->isXaAssociated() == ID_TRUE, NotAssociated);
    IDE_TEST_RAISE(aXaContext->mSession->getXaAssocState()
                   != MMD_XA_ASSOC_STATE_NOTASSOCIATED, InvalidXaSession);

    IDE_TEST_RAISE(aXid == NULL, XidNotExist);

    switch (aXid->getState())
    {
        case MMD_XA_STATE_IDLE:
            IDE_TEST_RAISE(!(aXaContext->mFlag & TMONEPHASE), InvalidXaState);

        case MMD_XA_STATE_PREPARED:
            aXaContext->mReturnValue = XA_OK;
            break;

        case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
            aXaContext->mReturnValue = XA_HEURCOM;
            IDE_RAISE(HeuristicState); /* BUG-20728 */
            break;

        case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
            aXaContext->mReturnValue = XA_HEURRB;
            IDE_RAISE(HeuristicState); /* BUG-20728 */
            break;

        case MMD_XA_STATE_ACTIVE:
        case MMD_XA_STATE_NO_TX:
        case MMD_XA_STATE_ROLLBACK_ONLY:
            IDE_RAISE(InvalidXaState);
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaSession);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_SESSION ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(XidNotExist);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_EXIST_XID ));
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION(InvalidXaState);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_STATE, MMDXASTATE[aXid->getState()] ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(HeuristicState);
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmdXa::beginRollback(mmdXaContext *aXaContext, mmdXid *aXid, mmdXaWaitMode  *aWaitMode)
{
    //fix BUG-22033
    *aWaitMode = MMD_XA_NO_WAIT;

    IDE_TEST_RAISE(aXaContext->mSession->isXaSession() != ID_TRUE, InvalidXaSession);
    IDE_TEST_RAISE(aXaContext->mSession->getXaAssocState()
                   == MMD_XA_ASSOC_STATE_ASSOCIATED, NotAssociated);

    // bug-27071: xa: xid null ref
    IDE_TEST_RAISE(aXid == NULL, XidNotExist);

    switch (aXid->getState())
    {
        case MMD_XA_STATE_IDLE:
        case MMD_XA_STATE_PREPARED:
        case MMD_XA_STATE_ROLLBACK_ONLY:
            aXaContext->mReturnValue = XA_OK;
            break;

        case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
            aXaContext->mReturnValue = XA_HEURCOM;
            IDE_RAISE(HeuristicState); /* BUG-20728 */
            break;

        case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
            aXaContext->mReturnValue = XA_HEURRB;
            IDE_RAISE(HeuristicState); /* BUG-20728 */
            break;

        case MMD_XA_STATE_ACTIVE:
            *aWaitMode = MMD_XA_WAIT;
            break;
        case MMD_XA_STATE_NO_TX:
            IDE_RAISE(InvalidXaState);
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaSession);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_SESSION ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(NotAssociated);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_ASSOCIATED_BRANCH ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    // bug-27071: xa: xid null ref
    IDE_EXCEPTION(XidNotExist);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_EXIST_XID ));
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION(InvalidXaState);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_STATE, MMDXASTATE[aXid->getState()] ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(HeuristicState);
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmdXa::beginForget(mmdXaContext *aXaContext, mmdXid *aXid)
{
    IDE_TEST_RAISE(aXaContext->mSession->isXaSession() != ID_TRUE, InvalidXaSession);
    //IDE_TEST_RAISE(aXaContext->mSession->isXaAssociated() == ID_TRUE, NotAssociated);
    IDE_TEST_RAISE(aXaContext->mSession->getXaAssocState()
                   == MMD_XA_ASSOC_STATE_ASSOCIATED, NotAssociated);

    IDE_TEST_RAISE(aXid == NULL, XidNotExist);

    switch (aXid->getState())
    {
        case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
        case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
            aXaContext->mReturnValue = XA_OK;
            break;

        case MMD_XA_STATE_IDLE:
        case MMD_XA_STATE_ACTIVE:
        case MMD_XA_STATE_PREPARED:
        case MMD_XA_STATE_NO_TX:
        case MMD_XA_STATE_ROLLBACK_ONLY:
            IDE_RAISE(InvalidXaState);
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaSession);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_SESSION ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(NotAssociated);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_ASSOCIATED_BRANCH ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(XidNotExist);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_EXIST_XID ));
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION(InvalidXaState);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_STATE, MMDXASTATE[aXid->getState()] ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmdXa::beginRecover(mmdXaContext *aXaContext)
{
    IDE_TEST_RAISE(aXaContext->mSession->isXaSession() != ID_TRUE, InvalidXaSession);
    IDE_TEST_RAISE(aXaContext->mSession->getXaAssocState()
                   != MMD_XA_ASSOC_STATE_NOTASSOCIATED, NotAssociated);

    aXaContext->mReturnValue = XA_OK;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaSession);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_SESSION ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(NotAssociated);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_ASSOCIATED_BRANCH ));
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mmdXa::open(mmdXaContext *aXaContext)
{
    // bug-24840 divide xa log: IDE_SERVER -> IDE_XA
    ideLog::logLine(IDE_XA_1, "OPEN (%d)", aXaContext->mSession->getSessionID());

    IDE_TEST(beginOpen(aXaContext) != IDE_SUCCESS);

    /* BUG-45844 (Server-Side) (Autocommit Mode) Multi-Transaction을 지원해야 합니다. */
    IDE_TEST( aXaContext->mSession->setGlobalTransactionLevel( 2 ) != IDE_SUCCESS );

    aXaContext->mSession->setXaSession(ID_TRUE);
    aXaContext->mSession->setXaAssocState(MMD_XA_ASSOC_STATE_NOTASSOCIATED);
    /* BUG-20850 xa_open 세션도 local transaction 으로 서비스가 가능함 */
    aXaContext->mSession->setSessionState(MMC_SESSION_STATE_SERVICE);

    return;

    IDE_EXCEPTION_END;
    {
        if (aXaContext->mReturnValue == XA_OK)
        {
            aXaContext->mReturnValue = XAER_RMERR;
        }
    }

    ideLog::logLine(IDE_XA_0, "OPEN ERROR -> %d", aXaContext->mReturnValue);

}

void mmdXa::close(mmdXaContext *aXaContext)
{
    mmcSession *sSession = aXaContext->mSession;
    //fix BUG-21527
    ID_XID     *sXidValue;
    mmdXid     *sXid;
    mmdXaState  sXaState;
    //fix BUG-22349 XA need to handle state.
    UInt        sState = 0;
    //fix BUG-21794
    iduList         *sXidList;
    iduListNode     *sIterator;
    iduListNode     *sNodeNext;
    mmdIdXidNode    *sXidNode;
    //fix BUG-XA close시 record-lock dead-lock이 발생할수 있음.
    mmcSessID       sAssocSessionID;
    mmcSessID       sSessionID;

    ideLog::logLine(IDE_XA_1, "CLOSE (%d)", aXaContext->mSession->getSessionID());

    IDE_TEST(beginClose(aXaContext) != IDE_SUCCESS);
    if (aXaContext->mSession->getXaAssocState() == MMD_XA_ASSOC_STATE_ASSOCIATED)
    {
        sXidValue = aXaContext->mSession->getLastXid();
        end(aXaContext,sXidValue);
        IDE_TEST(aXaContext->mReturnValue != XA_OK);
    }

    /* BUG-20734 */
    //fix BUG-21527
    sXidList   = aXaContext->mSession->getXidList();

    //fix BUG-XA close시 record-lock dead-lock이 발생할수 있음.
    sSessionID = aXaContext->mSession->getSessionID();

    IDU_LIST_ITERATE_SAFE(sXidList,sIterator,sNodeNext)
    {
        sXidNode = (mmdIdXidNode*) sIterator->mObj;
        sXidValue = &(sXidNode->mXID);
        IDE_TEST(mmdXa::fix(&sXid, sXidValue, MMD_XA_DO_LOG) != IDE_SUCCESS);
        //fix BUG-22349 XA need to handle state.
        sState = 1;

        if( sXid != NULL)
        {
            sXaState = sXid->getState();
            //fix BUG-XA close시 record-lock dead-lock이 발생할수 있음.
            sAssocSessionID = sXid->getAssocSessionID();
            //fix BUG-22349 XA need to handle state.
            sState = 0;
            IDE_ASSERT(mmdXa::unFix(sXid, sXidValue, MMD_XA_NONE) == IDE_SUCCESS);
            switch (sXaState)
            {
                case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
                case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
                case MMD_XA_STATE_PREPARED:
                case MMD_XA_STATE_NO_TX:
                    //fix BUG-21794
                    IDU_LIST_REMOVE(&(sXidNode->mLstNode));
                    IDE_ASSERT(mmdXidManager::free(sXidNode) == IDE_SUCCESS);
                    break;

                case MMD_XA_STATE_IDLE:
                case MMD_XA_STATE_ACTIVE:
                case MMD_XA_STATE_ROLLBACK_ONLY:  /* BUG-44976 */
                     //fix BUG-XA close시 record-lock dead-lock이 발생할수 있음.
                    if( sSessionID  == sAssocSessionID)
                    {
                        rollback(aXaContext,sXidValue);
                        IDE_TEST(aXaContext->mReturnValue != XA_OK);
                    }
                    else
                    {
                        //fix BUG-XA close시 record-lock dead-lock이 발생할수 있음.
                        IDU_LIST_REMOVE(&(sXidNode->mLstNode));
                        IDE_ASSERT(mmdXidManager::free(sXidNode) == IDE_SUCCESS);
                    }
                    break;
                default:
                    IDE_ASSERT(0);
                    break;
            }
        }
        else
        {
            IDU_LIST_REMOVE(&(sXidNode->mLstNode));
            IDE_ASSERT(mmdXidManager::free(sXidNode) == IDE_SUCCESS);
            //fix BUG-22349 XA need to handle state.
            sState = 0;
        }

    }// IDU_LIST_ITERATE_SAFE
    sSession->setSessionState(MMC_SESSION_STATE_SERVICE);
    sSession->setXaSession(ID_FALSE);

    return;
    IDE_EXCEPTION_END;
    {
        if (aXaContext->mReturnValue == XA_OK)
        {
            aXaContext->mReturnValue = XAER_RMERR;
        }
        //fix BUG-22349 XA need to handle state.
        switch(sState)
        {
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXid, sXidValue, MMD_XA_NONE) == IDE_SUCCESS);
            default :
                break;
        }
    }

    ideLog::logLine(IDE_XA_0, "CLOSE ERROR -> %d", aXaContext->mReturnValue);
}

/* BUG-18981 */
void mmdXa::start(mmdXaContext *aXaContext, ID_XID *aXid)
{
    mmcSession       *sSession   = aXaContext->mSession;
    mmdXid           *sXid       = NULL;
    /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
     XaFixWithAdd latch duration을 줄이기 위하여 xid object를 미리 할당하고
     Begin합니다. */
    mmdXid           *sXid2Add   = NULL;
    mmdXaAssocState   sXaAssocState;
    //fix BUG-22349 XA need to handle state.
    UInt              sState = 0;
    //fix BUG-21795
    idBool            sNeed2RestoreLocalTx = ID_FALSE;

    //BUG-25050 Xid 출력
    UChar             sXidString[XID_DATA_MAX_LEN];
    /* PROJ-2436 ADO.NET MSDTC */
    mmcTransEscalation sTransEscalation = sSession->getTransEscalation();
    mmcTransObj       *sTransToEscalate = NULL;

    if (sTransEscalation == MMC_TRANS_DO_NOT_ESCALATE)
    {
        /* nothing to do */
    }
    else
    {
        sTransToEscalate = sSession->getTransPtr();
    }

    (void)idaXaConvertXIDToString(NULL, aXid, sXidString, XID_DATA_MAX_LEN);

    ideLog::logLine(IDE_XA_1, "START (%d)(XID:%s)",
                aXaContext->mSession->getSessionID(),
                sXidString);    //BUG-25050 Xid 출력

    IDE_TEST(mmdXa::fix(&sXid, aXid, MMD_XA_DO_LOG) != IDE_SUCCESS);
    sState = 1;

reCheck:

    if (sXid != NULL)
    {
        sXid->lock();
        //fix BUG-22349 XA need to handle state.
        sState = 2;
    }

    sXaAssocState = sSession->getXaAssocState();
    IDE_TEST(beginStart(aXaContext, sXid, sXaAssocState,&sState) != IDE_SUCCESS);
    
    /* BUG-20850 global transation 시작전에 local transaction 의 상태를 점검한다. */
    if (sNeed2RestoreLocalTx != ID_TRUE)
    {
        if (sTransEscalation == MMC_TRANS_DO_NOT_ESCALATE)
        {
            IDE_TEST_RAISE(cleanupLocalTrans(sSession) != IDE_SUCCESS, LocalTransError);
            sNeed2RestoreLocalTx = ID_TRUE;
        }
        else
        {
            prepareLocalTrans(sSession);
        }
    }

    if (sXid == NULL)
    {
        /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
           XaFixWithAdd latch duration을 줄이기 위하여 xid object를 미리 할당하고
           Begin합니다. */
        IDE_TEST(mmdXidManager::alloc(&sXid2Add, aXid, sTransToEscalate)
                != IDE_SUCCESS);

        if (sTransEscalation == MMC_TRANS_DO_NOT_ESCALATE)
        {
            sXid2Add->beginTrans(aXaContext->mSession);
        }
        else
        {
            /* already began */
        }

        sXid2Add->lock();
        
        IDE_TEST_RAISE(mmdXa::fixWithAdd(aXaContext,
                                         sXid2Add,
                                         &sXid,
                                         aXid,
                                         MMD_XA_DO_LOG) != IDE_SUCCESS,
                errorFixWithAdd);

        if (sXid2Add != sXid)
        {
            /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
              이미 등록된 경우에는, 미리할당된 xid object를 해제한다  */
            sXid2Add->unlock();

            if (sTransEscalation == MMC_TRANS_DO_NOT_ESCALATE)
            {
                sXid2Add->rollbackTrans(NULL);

                IDE_ASSERT(mmdXidManager::free(sXid2Add, ID_TRUE)
                        == IDE_SUCCESS);
            }
            else
            {
                /* not needed rollback */

                IDE_ASSERT(mmdXidManager::free(sXid2Add, ID_FALSE)
                        == IDE_SUCCESS);
            }

            sXid2Add = NULL;

            goto reCheck;
        }
        else
        {
            sState = 2;        //BUG-28994 [CodeSonar]Null Pointer Dereference

            if (sTransEscalation == MMC_TRANS_DO_NOT_ESCALATE)
            {
                sSession->setTrans(sXid->getTransPtr());
                sSession->setSessionBegin( ID_TRUE );
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        sSession->setTrans(sXid->getTransPtr());
        sSession->setSessionBegin( ID_TRUE );
    }

    sXid->setState(MMD_XA_STATE_ACTIVE);
    //fix BUG-22669 XID list에 대한 performance view 필요.
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    // XID와 session을 associate시킴.
    sXid->associate( aXaContext->mSession);
    
    /*
     * BUGBUG: IsolationLevel을 여기서 세팅해도 되나?
     */

    // Session Info 중에서 transaction 시작 시 필요한 정보 설정

    sSession->setSessionInfoFlagForTx(sSession->getIsolationLevel(), /* BUG-39817 */
                                      SMI_TRANSACTION_REPL_DEFAULT,
                                      SMI_TRANSACTION_NORMAL,
                                      (idBool)mmuProperty::getCommitWriteWaitMode());

    sSession->setSessionState(MMC_SESSION_STATE_SERVICE);
    /* BUG-20850 global transaction 은 NONAUTOCOMMIT 모드여야 함 */
    sSession->setGlobalCommitMode(MMC_COMMITMODE_NONAUTOCOMMIT);
    sSession->setXaAssocState(MMD_XA_ASSOC_STATE_ASSOCIATED);
    sSession->setTransEscalation(MMC_TRANS_DO_NOT_ESCALATE);
    //fix BUG-21891,40772
    IDE_TEST( sSession->addXid(aXid) != IDE_SUCCESS );
    //fix BUG-22349 XA need to handle state.
    sState = 1;
    sXid->unlock();

    //fix BUG-22349 XA need to handle state.
    sState = 0;
    IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);

    return;

    IDE_EXCEPTION(LocalTransError);
    {
        aXaContext->mReturnValue = XAER_OUTSIDE;
    }
    IDE_EXCEPTION(errorFixWithAdd);
    {
        /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
           에러가발생한 경우에는,미리할당된 xid object를 해제한다 */
        sState =1;
        sXid2Add->unlock();

        if (sTransEscalation == MMC_TRANS_DO_NOT_ESCALATE)
        {
            sXid2Add->rollbackTrans(NULL);

            IDE_ASSERT(mmdXidManager::free(sXid2Add, ID_TRUE) == IDE_SUCCESS);
        }
        else
        {
            IDE_ASSERT(mmdXidManager::free(sXid2Add, ID_FALSE) == IDE_SUCCESS);
        }
    }
    
    IDE_EXCEPTION_END;
    {
        /* BUG-20850 xa_start 실패시, global transaction 수행 전의
           local transaction 상태로 복구한다. */
        //fix BUG-21795
        if(sNeed2RestoreLocalTx == ID_TRUE)
        {
            restoreLocalTrans(sSession);
        }

        if (aXaContext->mReturnValue == XA_OK)
        {
            aXaContext->mReturnValue = XAER_RMERR;
        }

        //fix BUG-22349 XA need to handle state.
        switch(sState)
        {
            case 2:
                sXid->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
            default :
                break;
        }//switch


    }

    ideLog::logLine(IDE_XA_0, "START ERROR -> %d(XID:%s)",
                aXaContext->mReturnValue,
                sXidString);        //BUG-25050 Xid 출력
}

/* BUG-18981 */
void mmdXa::end(mmdXaContext *aXaContext, ID_XID *aXid)
{
    mmcSession *sSession   = aXaContext->mSession;
    //fix BUG-21527
    ID_XID          *sXidValue;
    mmdXid          *sXid        = NULL;
    mmdXid          *sSessionXid = NULL;
    mmdXaAssocState sXaAssocState;
    UInt            sState = 0;


    //BUG-25050 Xid 출력
    UChar          sXidString[XID_DATA_MAX_LEN];

    (void)idaXaConvertXIDToString(NULL, aXid, sXidString, XID_DATA_MAX_LEN);

    ideLog::logLine(IDE_XA_1, "END (%d)(XID:%s)",
                sSession->getSessionID(),
                sXidString);            //BUG-25050 Xid 출력

    IDE_TEST(mmdXa::fix(&sXid, aXid, MMD_XA_DO_LOG) != IDE_SUCCESS);
    sState = 1;

    //fix BUG-21527
    sXidValue = sSession->getLastXid();
    //fix BUG-22561 xa_end시에 Transaction Branch Assoc현재 state가 suspend일때 상태전이가
    //잘못됨.
    IDE_TEST_RAISE(sXidValue == NULL, PROTO_ERROR);
    IDE_TEST(mmdXaSafeFind(&sSessionXid, sXidValue, MMD_XA_DO_LOG) != IDE_SUCCESS);
    sXaAssocState = sSession->getXaAssocState();
    if (sXid != NULL)
    {
        sXid->lock();
        sState = 2;
    }
    if(sSessionXid != NULL)
    {
        //fix BUG-22561 xa_end시에 Transaction Branch Assoc현재 state가 suspend일때 상태전이가
        //잘못됨.
        IDE_TEST(beginEnd(aXaContext,sXid, sSessionXid,sXaAssocState) != IDE_SUCCESS);
    }
    else
    {
        /* BUG-29078
         * NOTA Error
         */
        IDE_TEST_RAISE(sSessionXid == NULL, XidNotExist)
    }
    //fix BUG-22479 xa_end시에 fetch중에 있는 statement에 대하여
    //statement end를 자동으로처리해야 합니다.
    /* PROJ-1381 FAC : End 시점에는 Non-Holdable Fetch만 정리한다.
     * Holdable Fetch는 Commit, Rollback 시점에 어떻게 처리할지 결정된다. */
    IDE_ASSERT(sSession->closeAllCursor(ID_TRUE, MMC_CLOSEMODE_REMAIN_HOLD) == IDE_SUCCESS);
    IDE_TEST(sXid->addAssocFetchListItem(sSession) != IDE_SUCCESS);

    IDU_FIT_POINT_RAISE( "mmdXa::end::lock::isAllStmtEndExceptHold",  
                         StmtRemainError);
    IDE_TEST_RAISE(sSession->isAllStmtEndExceptHold() != ID_TRUE, StmtRemainError);

    //fix BUG-22561 xa_end시에 Transaction Branch Assoc현재 state가 suspend일때
    //상태전이가 잘못됨.
    if(sState == 2)
    {
        if (aXaContext->mFlag & TMFAIL )
        {
            sXid->setState(MMD_XA_STATE_ROLLBACK_ONLY);
            aXaContext->mReturnValue = XA_RBROLLBACK;
        }
        else
        {
            sXid->setState(MMD_XA_STATE_IDLE);
        }

        sXid->setHeuristicXaEnd(ID_FALSE);        //BUG-29351
        
        sXid->unlock();
        sState = 1;
    }

    IDE_DASSERT( sState == 1);
    sState = 0;

    IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);

    sSession->setSessionState(MMC_SESSION_STATE_READY);


    switch (sXaAssocState)
    {
        case MMD_XA_ASSOC_STATE_ASSOCIATED:
            if (aXaContext->mFlag & TMSUSPEND )
            {
                sSession->setXaAssocState(MMD_XA_ASSOC_STATE_SUSPENDED);
            }
            else
            {
                sSession->setXaAssocState(MMD_XA_ASSOC_STATE_NOTASSOCIATED);
            }
            break;

        case MMD_XA_ASSOC_STATE_SUSPENDED:
            //fix BUG-22561 xa_end시에 Transaction Branch Assoc현재 state가 suspend일때
            //상태전이가 잘못됨.
            sSession->setXaAssocState(MMD_XA_ASSOC_STATE_NOTASSOCIATED);
            break;

        case MMD_XA_ASSOC_STATE_NOTASSOCIATED:
        default:
            break;
    }

    /* BUG-20850 global transaction 종료 후, global transaction 수행 전의
       local transaction 상태로 복구한다. */
    if(sXaAssocState != MMD_XA_ASSOC_STATE_SUSPENDED)
    {

        restoreLocalTrans(sSession);
    }
    else
    {
        // nothing to do.
    }

    return;

    IDE_EXCEPTION(PROTO_ERROR);
    {
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(StmtRemainError);
    {
        aXaContext->mReturnValue = XAER_RMERR;
        ideLog::logLine(IDE_XA_0, "XA-WARNING !! XA END ERROR(%d) Statement Remain !!!! -> %d(XID:%s)",
                    sSession->getSessionID(),
                    aXaContext->mReturnValue,
                    sXidString);        //BUG-25050 Xid 출력
    }
    IDE_EXCEPTION(XidNotExist);
    {
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION_END;
    {
        switch(sState )
        {
            case 2:
                sXid->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
            default:
                break;

        }
        if (aXaContext->mReturnValue == XA_OK)
        {
            aXaContext->mReturnValue = XAER_RMERR;
        }
    }

    ideLog::logLine(IDE_XA_0, "END ERROR(%d) -> %d(XID:%s)",
                sSession->getSessionID(),
                aXaContext->mReturnValue,
                sXidString);        //BUG-25050 Xid 출력

}

/* BUG-29078
 * XA_ROLLBACK을 XA_END전에 수신했을 경우에 수행중인 Statement가 존재할 경우에, 
 * 해당 XID를 Heuristic하게 XA_END를 수행한다.
 */
void mmdXa::heuristicEnd(mmcSession* aSession, ID_XID *aXid)
{
    mmcSession *sSession   = aSession;
    //fix BUG-21527
    ID_XID          *sXidValue;
    mmdXid          *sXid        = NULL;
    mmdXid          *sSessionXid = NULL;
    UInt            sState = 0;

    //BUG-25050 Xid 출력
    UChar          sXidString[XID_DATA_MAX_LEN];

    (void)idaXaConvertXIDToString(NULL, aXid, sXidString, XID_DATA_MAX_LEN);

    IDE_TEST( sSession == NULL );

    IDE_TEST( mmdXa::fix(&sXid, aXid, MMD_XA_DO_NOT_LOG) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sXid == NULL );
    
    IDE_TEST( sXid->getHeuristicXaEnd() != ID_TRUE );
    
    ideLog::logLine(IDE_XA_1, "Heuristic END (%d)(XID:%s)",
                sSession->getSessionID(),
                sXidString);            //BUG-25050 Xid 출력

    //fix BUG-21527
    sXidValue = sSession->getLastXid();

    IDE_TEST_RAISE(sXidValue == NULL, occurErrorWithLog);
    IDE_TEST_RAISE(mmdXaSafeFind(&sSessionXid, sXidValue, MMD_XA_DO_LOG) != IDE_SUCCESS, occurErrorWithLog);
    
    if (sXid != NULL)
    {
        sXid->lock();
        sState = 2;
    }
    
    //fix BUG-22479 xa_end시에 fetch중에 있는 statement에 대하여
    //statement end를 자동으로처리해야 합니다.
    /* PROJ-1381 FAC : Rollback 처리를 하므로 Commit 안된 Fetch를 모두 닫는다. */
    IDE_TEST(sSession->closeAllCursor(ID_TRUE, MMC_CLOSEMODE_NON_COMMITED) != IDE_SUCCESS);

    IDU_FIT_POINT_RAISE( "mmdXa::heuristicEnd::lock::isAllStmtEndExceptHold", 
                         occurErrorWithLog);
    IDE_TEST_RAISE( sSession->isAllStmtEndExceptHold() != ID_TRUE, occurErrorWithLog );

    /* BUG-29078
     * If Heuristic XA_END is done, State is not to be SUSPEND. 
     * Because XA_END flag is must a TMSUCCESS.
     */
    if(sState == 2)
    {
        sXid->setState(MMD_XA_STATE_IDLE);
        sXid->setHeuristicXaEnd(ID_FALSE);
        sXid->unlock();
        sState = 1;
    }

    IDE_DASSERT(sState == 1);
    sState = 0;

    IDE_ASSERT( mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS );

    sSession->setSessionState(MMC_SESSION_STATE_READY);

    sSession->setXaAssocState(MMD_XA_ASSOC_STATE_NOTASSOCIATED);

    /* BUG-20850 global transaction 종료 후, global transaction 수행 전의
       local transaction 상태로 복구한다. */
    restoreLocalTrans(sSession);

    return;
    
    IDE_EXCEPTION(occurErrorWithLog);
    {
        ideLog::logLine(IDE_XA_0, "Heuristic END ERROR(%d) -> (XID:%s)",
                        sSession->getSessionID(),
                        sXidString);        //BUG-25050 Xid 출력
    }
    IDE_EXCEPTION_END;
    {
        switch(sState )
        {
            case 2:
                sXid->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
            default:
                break;

        }
    }

}

/* BUG-18981 */
void mmdXa::prepare(mmdXaContext *aXaContext, ID_XID *aXid)
{
    mmdXid *sXid = NULL;
    //fix BUG-30343 Committing a xid can be failed in replication environment.
    SChar         *sErrorMsg;
    //fix BUG-22349 XA need to handle state.
    UInt    sState = 0;
    idBool  sReadOnly;
    UInt    sUnfixOpt = MMD_XA_NONE;

    //BUG-25050 Xid 출력
    UChar          sXidString[XID_DATA_MAX_LEN];

    (void)idaXaConvertXIDToString(NULL, aXid, sXidString, XID_DATA_MAX_LEN);

    ideLog::logLine(IDE_XA_1, "PREPARE (%d)(XID:%s)",
                aXaContext->mSession->getSessionID(),
                sXidString);            //BUG-25050 Xid 출력

    IDE_TEST(mmdXa::fix(&sXid, aXid, MMD_XA_DO_LOG) != IDE_SUCCESS);
    //fix BUG-22349 XA need to handle state.
    sState = 1;

    if (sXid != NULL)
    {
        sXid->lock();
        //fix BUG-22349 XA need to handle state.
        sState = 2;
    }

    IDE_TEST(beginPrepare(aXaContext, sXid) != IDE_SUCCESS);

    IDE_ASSERT(mmcTrans::isReadOnly(sXid->getTransPtr(), &sReadOnly) == IDE_SUCCESS);

    if (sReadOnly == ID_TRUE)
    {
        //dblink나 shard로 인해 commit이 필요할 수 있다.
        sReadOnly = dkiIsReadOnly( aXaContext->mSession->getDatabaseLinkSession() );
    }

    if (sReadOnly == ID_TRUE)
    {
        ideLog::logLine(IDE_XA_1, "PREPARE READ ONLY (%d) -> 0x%x(XID:%s)",
                    aXaContext->mSession->getSessionID(),
                    sXid,
                    sXidString);        //BUG-25050 Xid 출력


        IDU_FIT_POINT("mmdXa::prepare::lock::prepareTransReadOnly" );
        //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
        IDE_TEST(sXid->prepareTrans(aXaContext->mSession) != IDE_SUCCESS);
        //fix BUG-22349 XA need to handle state.
        sState = 1;
        sXid->unlock();
        //fix BUG-22349 XA need to handle state.
        sState = 0;
        IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);

        IDE_TEST(mmdXa::fix(&sXid, aXid,MMD_XA_DO_LOG) != IDE_SUCCESS);
        //fix BUG-22349 XA need to handle state.
        sState = 1;

        if (sXid != NULL)
        {
            sXid->lock();
            //fix BUG-22349 XA need to handle state.
            sState = 2;
            //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
            //fix BUG-30343 Committing a xid can be failed in replication environment.
            IDE_TEST_RAISE(sXid->commitTrans(aXaContext->mSession) != IDE_SUCCESS, commitError);


            aXaContext->mReturnValue = XA_RDONLY;
            //fix BUG-22349 XA need to handle state.
            sState = 1;
            sXid->unlock();

            sUnfixOpt = (MMD_XA_REMOVE_FROM_XIDLIST | MMD_XA_REMOVE_FROM_SESSION);
        }
        else
        {
            ideLog::logLine(IDE_XA_0, "PREPARE READ ONLY (%d) -> 0x%x(XID:%s)NOT FOUND",
                        aXaContext->mSession->getSessionID(),
                        sXid,
                        sXidString);        //BUG-25050 Xid 출력
        }
    }
    else
    {
        IDU_FIT_POINT("mmdXa::prepare::lock::prepareTrans" );

        //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
        IDE_TEST(sXid->prepareTrans(aXaContext->mSession) != IDE_SUCCESS);
        sState = 1;
        sXid->unlock();
    }
    //fix BUG-22349 XA need to handle state.
    sState = 0;
    IDE_TEST(mmdXa::unFix(sXid, aXid, sUnfixOpt) != IDE_SUCCESS);
    if (FLAG_IS_SET(sUnfixOpt, MMD_XA_REMOVE_FROM_SESSION))
    {
        aXaContext->mSession->removeXid(aXid);
    }

    return;
    
    IDE_EXCEPTION(commitError)
    {
        //fix BUG-30343 Committing a xid can be failed in replication environment.
        sErrorMsg =  ideGetErrorMsg(ideGetErrorCode());
        ideLog::logLine(IDE_XA_0, "Xid  Commit Error [%s], reason [%s]", sXidString,sErrorMsg);

    }

    IDE_EXCEPTION_END;
    {
        if (aXaContext->mReturnValue == XA_OK)
        {
            aXaContext->mReturnValue = XAER_RMERR;
        }

        //fix BUG-22349 XA need to handle state.
        switch(sState)
        {
            case 2:
                sXid->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
            default :
                break;
        }//switch
    }

    ideLog::logLine(IDE_XA_0, "PREPARE ERROR -> %d(XID:%s)",
                aXaContext->mReturnValue,
                sXidString);            //BUG-25050 Xid 출력

}

/* BUG-18981 */
void mmdXa::commit(mmdXaContext *aXaContext, ID_XID *aXid)
{
    mmdXid         *sXid = NULL;
    mmcSession     *sSession = aXaContext->mSession;
    //fix BUG-22479 xa_end시에 fetch중에 있는 statement에 대하여
    //statement end를 자동으로처리해야 합니다.
    UInt           sErrorCode;
    SChar         *sErrorMsg;
    //fix BUG-22349 XA need to handle state.
    UInt            sState = 0;

    //BUG-25050 Xid 출력
    UChar          sXidString[XID_DATA_MAX_LEN];

    (void)idaXaConvertXIDToString(NULL, aXid, sXidString, XID_DATA_MAX_LEN);

    ideLog::logLine(IDE_XA_1, "COMMIT (%d)(XID:%s)",
                aXaContext->mSession->getSessionID(),
                sXidString);            //BUG-25050 Xid 출력

    IDE_TEST(mmdXa::fix(&sXid, aXid, MMD_XA_DO_LOG) != IDE_SUCCESS);
    //fix BUG-22349 XA need to handle state.
    sState = 1;

    if (sXid != NULL)
    {
        sXid->lock();
        //fix BUG-22349 XA need to handle state.
        sState = 2;
    }

    IDE_TEST(beginCommit(aXaContext, sXid) != IDE_SUCCESS);

    IDU_FIT_POINT_RAISE( "mmdXa::commit::lock::commitTrans", 
                         CommitError);
    //fix BUG-22479 xa_end시에 fetch중에 있는 statement에 대하여
    //statement end를 자동으로처리해야 합니다.
    //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
    IDE_TEST_RAISE(sXid->commitTrans(aXaContext->mSession) != IDE_SUCCESS,CommitError);

    //fix BUG-22349 XA need to handle state.
    sState = 1;
    sXid->unlock();

    //fix BUG-22349 XA need to handle state.
    sState = 0;
    IDE_TEST(mmdXa::unFix(sXid, aXid, MMD_XA_REMOVE_FROM_XIDLIST) != IDE_SUCCESS);
    sSession->removeXid(aXid);

    return;

    IDE_EXCEPTION(CommitError);
    {
        //fix BUG-22479 xa_end시에 fetch중에 있는 statement에 대하여
        //statement end를 자동으로처리해야 합니다.
        aXaContext->mReturnValue = XAER_RMERR;
        sErrorCode = ideGetErrorCode();
        sErrorMsg =  ideGetErrorMsg(sErrorCode);
        if(sErrorMsg != NULL)
        {
            ideLog::logLine(IDE_XA_0, "XA-WARNING !! XA COMMIT  ERROR(%d) : Reason %s (XID:%s)",
                        aXaContext->mSession->getSessionID(),
                        sErrorMsg,
                        sXidString);        //BUG-25050 Xid 출력
        }
    }

    IDE_EXCEPTION_END;
    {
        if (aXaContext->mReturnValue == XA_OK)
        {
            aXaContext->mReturnValue = XAER_RMERR;
        }

        //fix BUG-22349 XA need to handle state.
        switch(sState)
        {
            case 2:
                sXid->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
            default :
                break;
        }//switch
    }

    ideLog::logLine(IDE_XA_0, "COMMIT ERROR -> %d(XID:%s)",
                aXaContext->mReturnValue,
                sXidString);        //BUG-25050 Xid 출력
}

/* BUG-18981 */
void mmdXa::rollback(mmdXaContext *aXaContext, ID_XID *aXid)
{
    mmdXid        *sXid = NULL;
    mmcSession    *sSession = aXaContext->mSession;
    UInt           sCurSleepTime = 0;
    //fix BUG-23776, XA ROLLBACK시 XID가 ACTIVE일때 대기시간을
    //QueryTime Out이 아니라,Property를 제공해야 함.
    UInt           sMaxWaitTime =   mmuProperty::getXaRollbackTimeOut() ;

    mmdXaWaitMode  sWaitMode;
    mmdXaLogFlag   sXaLogFlag = MMD_XA_DO_LOG;
    //fix BUG-22349 XA need to handle state.
    UInt            sState = 0;
    UInt            sUnfixOpt = MMD_XA_NONE;

    //BUG-25050 Xid 출력
    UChar          sXidString[XID_DATA_MAX_LEN];
    // BUG-25020
    mmcSession    *sAssocSession;
    idBool         sIsSetTimeout = ID_FALSE;

    (void)idaXaConvertXIDToString(NULL, aXid, sXidString, XID_DATA_MAX_LEN);

    ideLog::logLine(IDE_XA_1, "ROLLBACK (%d)(XID:%s)",
                aXaContext->mSession->getSessionID(),
                sXidString);        //BUG-25050 Xid 출력

  retry:
    IDE_TEST(mmdXa::fix(&sXid, aXid, sXaLogFlag) != IDE_SUCCESS);
    //fix BUG-22349 XA need to handle state.
    sState = 1;

    if (sXid != NULL)
    {
        sXid->lock();
        //fix BUG-22349 XA need to handle state.
        sState = 2;

        IDU_FIT_POINT("mmdXa::rollback::lock::beginRollback");
        IDE_TEST(beginRollback(aXaContext, sXid,&sWaitMode) != IDE_SUCCESS);
        if(sWaitMode == MMD_XA_WAIT)
        {
            if ( sIsSetTimeout == ID_FALSE )
            {
                // Fix BUG-25020 XA가 Active 상태인데, rollback이 오면
                // 곧바로 Query Timeout으로 처리하여 해당 Transaction이 Rollback 되도록 함.
                sAssocSession = sXid->getAssocSession();

                //BUG-25323 Trace Code...
                if (sAssocSession == NULL)
                {
                    ideLog::logLine(IDE_XA_0, "AssocSession is NULL(%d) -> (XID:%s)",
                    aXaContext->mSession->getSessionID(),
                    sXidString);
                }
                else
                {
                    mmtSessionManager::setQueryTimeoutEvent(sAssocSession,
                                                            sAssocSession->getInfo()->mCurrStmtID);
                    sIsSetTimeout = ID_TRUE;
                    /* BUG-29078
                     * 모든 Statement가 종료된 시점이면, XA_ROLLBACK_TIMEOUT시까지 대기하고,
                     * Statement가 남아있는 경우이면, 해당 Statement가 종료된 후에 Heuristic XA_END를 수행한다.
                     */
                    /* PROJ-1381 FAC : Holdable Fetch는 transaction에 영향을 받지 않는다. */
                    if (sAssocSession->isAllStmtEndExceptHold() != ID_TRUE)
                    {
                        sXid->setHeuristicXaEnd(ID_TRUE);
                    }
                }
            }

            if( sXid->getHeuristicXaEnd() != ID_TRUE )
            {
                /* BUG-29078
                 * Statement가 모두 종료된 경우, XA_ROLLBACK_TIMEOUT Property 시간만큼
                 * XA_END가 수신될 때까지 대기한다.
                 */
                if(sCurSleepTime <  sMaxWaitTime)
                {
                    //fix BUG-22349 XA need to handle state.
                    sState = 1;
                    sXid->unlock();
                    sState = 0;
                    IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
                    idlOS::sleep(1);
                    sCurSleepTime += 1;
                    sXaLogFlag = MMD_XA_DO_NOT_LOG;
                    goto retry;
                }
                else
                {
                    //fix BUG-22306 XA ROLLBACK시 해당 XID가 ACTIVE일때
                    //대기 완료후 이에 대한 rollback처리 정책이 잘못됨.
                    // 아직 XID가 DML수행중이다.
                    aXaContext->mReturnValue =  XAER_RMERR;
                }
            }
            else        // if getIsRollback is true..
            {
                /* BUG-29078 
                 * Statement가 남아있는 경우에, Heuristic XA_END가 될때까지 대기한다.
                 */
                sState = 1;
                sXid->unlock();
                sState = 0;
                IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
                idlOS::sleep(1);
                sXaLogFlag = MMD_XA_DO_NOT_LOG;
                goto retry;
            }
        }
        else
        {
            //fix BUG-22033
            //nothing to do
        }

        //fix BUG-22306 XA ROLLBACK시 해당 XID가 ACTIVE일때
        //대기 완료후 이에 대한 rollback처리 정책이 잘못됨.
        // 아직 XID가 DML수행중이다.
        IDE_TEST(sWaitMode == MMD_XA_WAIT);

        //BUG-25020
        if ( sIsSetTimeout == ID_TRUE )
        {
            ideLog::logLine(IDE_XA_0, "ROLLBACK By RM Session (%d)(XID:%s)",
                            aXaContext->mSession->getSessionID(),
                            sXidString);
        }

        //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
        sXid->rollbackTrans(aXaContext->mSession);

        sState = 1;
        sXid->unlock();

        sUnfixOpt = (MMD_XA_REMOVE_FROM_XIDLIST | MMD_XA_REMOVE_FROM_SESSION);
    }
    else
    {
        //fix BUG-22033
        aXaContext->mReturnValue = XA_OK;
    }
    sState = 0;
    IDE_TEST(mmdXa::unFix(sXid, aXid, sUnfixOpt) != IDE_SUCCESS);
    if (FLAG_IS_SET(sUnfixOpt, MMD_XA_REMOVE_FROM_SESSION))
    {
        sSession->removeXid(aXid);
    }


    return;

    IDE_EXCEPTION_END;
    {
        if (aXaContext->mReturnValue == XA_OK)
        {
            aXaContext->mReturnValue = XAER_RMERR;
        }

        switch(sState )
        {
            case 2:
                sXid->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
            default:
                break;

        }//switch.
    }

    ideLog::logLine(IDE_XA_0, "ROLLBACK ERROR(%d) -> %d(XID:%s)",
                aXaContext->mSession->getSessionID(),
                aXaContext->mReturnValue,
                sXidString);            //BUG-25050 Xid 출력

}

/* BUG-18981 */
void mmdXa::forget(mmdXaContext *aXaContext, ID_XID *aXid)
{
    //fix BUG-22349 XA need to handle state.
    UInt    sState = 0;
    mmdXid *sXid = NULL;

    //BUG-25050 Xid 출력
    UChar          sXidString[XID_DATA_MAX_LEN];

    /* PROJ-2446 */ 
    mmcSession    *sSession = aXaContext->mSession;

    (void)idaXaConvertXIDToString(NULL, aXid, sXidString, XID_DATA_MAX_LEN);

    ideLog::logLine(IDE_XA_0, "FORGET (%d)(XID:%s)",
                aXaContext->mSession->getSessionID(),
                sXidString);        //BUG-25050 Xid 출력

    IDE_TEST(mmdXa::fix(&sXid, aXid, MMD_XA_DO_LOG) != IDE_SUCCESS);
    sState = 1;

    if (sXid != NULL)
    {
        sXid->lock();
        sState = 2;
    }

    IDU_FIT_POINT("mmdXa::forget::lock::beginForget");
    IDE_TEST(beginForget(aXaContext, sXid) != IDE_SUCCESS);
    sXid->setState(MMD_XA_STATE_NO_TX);

    IDE_TEST( mmdManager::removeHeuristicTrans( sSession->getStatSQL(), /* PROJ-2446 */
                                                aXid ) 
              != IDE_SUCCESS );

    sState = 1;
    sXid->unlock();

    sState = 0;
    IDE_TEST(mmdXa::unFix(sXid, aXid, MMD_XA_REMOVE_FROM_XIDLIST) != IDE_SUCCESS);

    return;

    IDE_EXCEPTION_END;
    {
        if (aXaContext->mReturnValue == XA_OK)
        {
            aXaContext->mReturnValue = XAER_RMERR;
        }

        //fix BUG-22349 XA need to handle state.
        switch(sState)
        {
            case 2:
                sXid->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
            default :
                break;
        }//switch

    }

    ideLog::logLine(IDE_XA_0, "FORGET ERROR -> %d(XID:%s)",
                aXaContext->mReturnValue,
                sXidString);            //BUG-25050 Xid 출력
}

/* BUG-18981 */
void mmdXa::recover(mmdXaContext  *aXaContext,
                    ID_XID       **aPreparedXids,
                    SInt          *aPreparedXidsCnt,
                    ID_XID       **aHeuristicXids,
                    SInt          *aHeuristicXidsCnt)
{
    ID_XID         sXid;
    timeval        sTime;
    smiCommitState sTxState;
    SInt           sSlotID = -1;
    SInt           i;
    //fix BUG-22383  [XA] xa_recover에서 xid 리스트 저장공간 할당문제.
    SInt           sPreparedMax = smiGetTransTblSize();

    ID_XID        *sXidList = NULL;
    
    /* PROJ-2446 */ 
    mmcSession    *sSession = aXaContext->mSession;

    ideLog::logLine(IDE_XA_0, "RECOVER (%d)", aXaContext->mSession->getSessionID());

    IDE_TEST(beginRecover(aXaContext) != IDE_SUCCESS);

    // BUG-20668
    // select from SYSTEM_.SYS_XA_HEURISTIC_TRANS_
     //fix BUG-27218 XA Load Heurisitc Transaction함수 내용을 명확히 해야 한다.
    IDE_TEST( mmdManager::loadHeuristicTrans( sSession->getStatSQL(),   /* PROJ-2446 */
                                              MMD_LOAD_HEURISTIC_XIDS_AT_XA_RECOVER, 
                                              aHeuristicXids, 
                                              aHeuristicXidsCnt ) 
              != IDE_SUCCESS );
             
    IDU_FIT_POINT_RAISE( "mmdXa::recover::malloc::XidList", 
                          InsufficientMemory );
            
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_MMT,
                                     ID_SIZEOF(ID_XID) * sPreparedMax,
                                     (void **)&sXidList,
                                     IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );
    *aPreparedXids = sXidList;

    for (i=0; i <sPreparedMax; i++ )
    {
        IDE_TEST(smiXaRecover(&sSlotID, &sXid, &sTime, &sTxState) != IDE_SUCCESS);

        if (sSlotID < 0)
        {
            break;
        }

        idlOS::memcpy(&(sXidList[i]), &sXid, ID_SIZEOF(ID_XID));
    }

    if ( i  > 0)
    {
        *aPreparedXidsCnt = i;
    }
    aXaContext->mReturnValue = *aPreparedXidsCnt + *aHeuristicXidsCnt;

    return;

    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    {
        if (aXaContext->mReturnValue == XA_OK)
        {
            aXaContext->mReturnValue = XAER_RMERR;
        }
        *aPreparedXidsCnt = 0;
        *aHeuristicXidsCnt = 0;
    }

    ideLog::logLine(IDE_XA_0, "RECOVER ERROR -> %d", aXaContext->mReturnValue);
}

/* PROJ-2436 ADO.NET MSDTC */
void mmdXa::heuristicCompleted(mmdXaContext *aXaContext, ID_XID *aXid)
{
    mmdXid     *sXid     = NULL;
    UInt        sState   = 0;

    IDE_ASSERT(aXaContext != NULL);
    IDE_ASSERT(aXid != NULL);

    IDE_TEST_RAISE(aXaContext->mSession->isXaSession() != ID_TRUE, InvalidXaSession);

    IDE_TEST(mmdXa::fix(&sXid, aXid, MMD_XA_DO_NOT_LOG) != IDE_SUCCESS);
    sState = 1;

    IDE_TEST_RAISE(sXid == NULL, XidNotExist);
    
    sXid->lock();

    switch (sXid->getState())
    {
        case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
            aXaContext->mReturnValue = XA_HEURCOM;
            break;

        case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
            aXaContext->mReturnValue = XA_HEURRB;
            break;

        case MMD_XA_STATE_IDLE:
        case MMD_XA_STATE_PREPARED:
        case MMD_XA_STATE_ACTIVE:
        case MMD_XA_STATE_ROLLBACK_ONLY:
            aXaContext->mReturnValue = XA_RETRY;
            break;

        case MMD_XA_STATE_NO_TX:
            aXaContext->mReturnValue = XAER_NOTA;
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    sXid->unlock();

    IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
    sState = 0;

    return;
    
    IDE_EXCEPTION(InvalidXaSession);
    {
        aXaContext->mReturnValue = XAER_PROTO;
    }
    IDE_EXCEPTION(XidNotExist);
    {
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION_END;
    {
        switch (sState)
        {
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXid, aXid, MMD_XA_NONE) == IDE_SUCCESS);
            default:
                break;
        }
    }
}

IDE_RC mmdXa::cleanupLocalTrans(mmcSession *aSession)
{
    idBool       sReadOnly;
    mmcTransObj *sTrans = NULL;

    /* BUG-20850 local transaction 이 readOnly 상태이면 commit 을 해 줌 */
    if ( (aSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT) &&
         (aSession->isActivated() == ID_TRUE) )
    {
        sTrans = aSession->getTransPtr();
        if (sTrans != NULL)
        {
            IDE_ASSERT(mmcTrans::isReadOnly(sTrans, &sReadOnly) == IDE_SUCCESS);
            if (sReadOnly == ID_TRUE)
            {
                /* PROJ-1381 FAC : Commit 하므로 Holdable Fetch는 유지한다. */
                IDE_ASSERT(aSession->closeAllCursor(ID_TRUE, MMC_CLOSEMODE_REMAIN_HOLD) == IDE_SUCCESS);
                aSession->lockForFetchList();
                IDU_LIST_JOIN_LIST(aSession->getCommitedFetchList(), aSession->getFetchList());
                aSession->unlockForFetchList();
                IDE_TEST_RAISE(aSession->isAllStmtEndExceptHold() != ID_TRUE, StmtRemainError);

                IDE_TEST(mmcTrans::commit(sTrans, aSession) != IDE_SUCCESS);
                aSession->setNeedLocalTxBegin(ID_TRUE);
                aSession->setActivated(ID_FALSE);
            }
        }
    }
    /* BUG-20850 local transaction 이 active 상태이면 에러 발생 */
    /* PROJ-1381 FAC */
    IDE_TEST_RAISE(aSession->isAllStmtEndExceptHold() != ID_TRUE, StmtRemainError);
    IDE_TEST_RAISE(aSession->isActivated() != ID_FALSE, AlreadyActiveError);

    /* BUG-20850 global transaction 시작전에 local transaction 의 상태를 저장해 둔다. */
    aSession->saveLocalCommitMode();
    aSession->saveLocalTrans();

    return IDE_SUCCESS;

    IDE_EXCEPTION(AlreadyActiveError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_MMC_TRANSACTION_ALREADY_ACTIVE));
    }
    IDE_EXCEPTION(StmtRemainError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_OTHER_STATEMENT_REMAINS));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* PROJ-2436 ADO.NET MSDTC */
void mmdXa::prepareLocalTrans(mmcSession *aSession)
{
    aSession->saveLocalCommitMode();
    aSession->allocLocalTrans();
    aSession->setNeedLocalTxBegin(ID_TRUE);
}

void mmdXa::restoreLocalTrans(mmcSession *aSession)
{
    idBool      sIsDummyBegin = ID_FALSE;

    /* BUG-20850 global transaction 종료 후, global transaction 수행 전의
       local transaction 상태로 복구한다. */
    aSession->setSessionState(MMC_SESSION_STATE_SERVICE);
    aSession->restoreLocalTrans();
    aSession->restoreLocalCommitMode();

    if(aSession->getNeedLocalTxBegin() == ID_TRUE)
    {
        mmcTrans::begin( aSession->getTransPtr(),
                         aSession->getStatSQL(),
                         aSession->getSessionInfoFlagForTx(),
                         aSession,
                         &sIsDummyBegin );
        aSession->setNeedLocalTxBegin(ID_FALSE);
    }
    aSession->setActivated(ID_FALSE);

}

void mmdXa::terminateSession(mmcSession *aSession)
{
    //fix BUG-21527
    ID_XID          *sXidValue;
    mmdXid          *sXid;
    //fix BUG-21794
    iduList         *sXidList;
    iduListNode     *sIterator;
    iduListNode     *sNodeNext;
    mmdIdXidNode    *sXidNode;
    UInt             sUnfixOpt = MMD_XA_NONE;

    //fix BUG-21862
    ideLog::logLine(IDE_XA_1, "terminate session (%d)", aSession->getSessionID());
    sXidList = aSession->getXidList();
    IDU_LIST_ITERATE_SAFE(sXidList,sIterator,sNodeNext)
    {
        sXidNode = (mmdIdXidNode*) sIterator->mObj;
        sXidValue = &(sXidNode->mXID);
        /* ------------------------------------------------
         * BUG-20662
         *
         * mmcSession:finalize 로부터 XaSession 일 경우에만 호출됨.
         * client 비정상 종료시 완료되지 않은(xa_end,xa_prepare 하지 않은)
         * transaction 을 rollback 처리해야 한다.
         * ----------------------------------------------*/
        //fix BUG-21527
        IDE_ASSERT(mmdXa::fix(&sXid, sXidValue,MMD_XA_DO_LOG) == IDE_SUCCESS);
        //fix BUG-21527
        if(sXid != NULL)
        {
            // fix BUG-23306 XA 세션 종료시 자신이 생성한 XID만 rollback 처리 해야합니다.
            if (sXid->getAssocSessionID() == aSession->getSessionID())
            {
                sXid->lock();

                /* BUG-20662 xa_end 를 하지 않았을 경우 */
                if (aSession->getXaAssocState() == MMD_XA_ASSOC_STATE_ASSOCIATED)
                {
                    if (sXid->getState() == MMD_XA_STATE_ACTIVE)
                    {
                        sXid->setState(MMD_XA_STATE_IDLE);
                    }
                }

                /*
                * fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
                * 그들간의 관계를 정확히 유지해야 함.
                * XID와 session을 dis-associate 시킴.
                *
                * fix BUG-26164 Heuristic Commit/Rollback 이후에 서버 비정상종료.
                * Xid_state에 따라서 Transaction 처리가 달라진다.
                */
                sXid->disAssociate(sXid->getState());

                switch (sXid->getState())
                {
                    case MMD_XA_STATE_IDLE:
                    case MMD_XA_STATE_ACTIVE:
                    case MMD_XA_STATE_ROLLBACK_ONLY:
                        /* BUG-20662 xa_prepare 를 하기 전의 상태이면 */
                        //fix BUG-22479 xa_end시에 fetch중에 있는 statement에 대하여
                        //statement end를 자동으로처리해야 합니다.
                        /* PROJ-1381 FAC : Commit 되지 않은 Fetch를 모두 닫는다. */
                        IDE_ASSERT(aSession->closeAllCursor(ID_TRUE, MMC_CLOSEMODE_NON_COMMITED) == IDE_SUCCESS);
                        IDE_ASSERT(aSession->isAllStmtEndExceptHold() == ID_TRUE);

                        //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
                        sXid->rollbackTrans(aSession);

                        sXid->unlock();

                        sUnfixOpt = MMD_XA_REMOVE_FROM_XIDLIST;
                        break;

                    case MMD_XA_STATE_PREPARED:
                    case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
                    case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
                    case MMD_XA_STATE_NO_TX:
                        sXid->unlock();
                        break;

                    default:
                        IDE_ASSERT(0);
                        break;
                }//switch

            }
        }//sXid
        IDE_ASSERT(mmdXa::unFix(sXid, sXidValue, sUnfixOpt) == IDE_SUCCESS);

        IDU_LIST_REMOVE(&(sXidNode->mLstNode));
        IDE_ASSERT(mmdXidManager::free(sXidNode) == IDE_SUCCESS) ;
    }//IDU_LIST

    if (aSession->getXaAssocState() == MMD_XA_ASSOC_STATE_ASSOCIATED)
    {
        //fix BUG-21862
        ideLog::logLine(IDE_XA_1, "terminate session  DO restoreLocalTrans(%d)", aSession->getSessionID());
        restoreLocalTrans(aSession);
    }
    else
    {
        //fix BUG-21862
        ideLog::logLine(IDE_XA_1, "terminate session  DONT'T  restoreLocalTrans(%d)", aSession->getSessionID());
    }
    //fix BUG-21948
    if(aSession->getSessionState() != MMC_SESSION_STATE_INIT)
    {
        aSession->setSessionState(MMC_SESSION_STATE_SERVICE);
    }
    else
    {
        //nothing to do
        // 이미 disconect protocol에 의하여 endSession이 된경우이다.
    }
    aSession->setXaAssocState(MMD_XA_ASSOC_STATE_NOTASSOCIATED);
}




IDE_RC mmdXa::convertStringToXid(SChar *aStr, UInt aStrLen, ID_XID *aXID)
{
    SChar *sDotPos   = NULL;
    SChar *sTmp      = NULL;
    vULong            sFormatID;
    SChar           * sGlobalTxId;
    SChar           * sBranchQualifier;
    mtdIntegerType    sInt;
    SChar             sXidString[XID_DATA_MAX_LEN];

    /* bug-36037: invalid xid
       original string을  보존하기 위해 복사 */
    IDE_TEST_RAISE(aStrLen >= XID_DATA_MAX_LEN, ERR_INVALID_XID);
    idlOS::memcpy(sXidString, aStr, aStrLen);
    sXidString[aStrLen] = '\0';

    sTmp = sXidString;
    sDotPos = idlOS::strchr(sTmp, '.');
    IDE_TEST_RAISE(sDotPos == NULL, ERR_INVALID_XID);
    *sDotPos = 0;
    IDE_TEST(mtc::makeInteger(&sInt, (UChar *)sTmp, sDotPos-sTmp) != IDE_SUCCESS);
    sFormatID = sInt;

    sTmp = sDotPos + 1;
    sDotPos = idlOS::strchr(sTmp, '.');
    IDE_TEST_RAISE(sDotPos == NULL, ERR_INVALID_XID);
    *sDotPos = 0;
    sGlobalTxId = sTmp;
    sBranchQualifier = sDotPos + 1;

    idaXaStringToXid(sFormatID, sGlobalTxId, sBranchQualifier, aXID);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_XID )
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XID ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// fix BUG-23815 XA session level  self record lock dead-lock이 발생한경우에
// TP monitor를 비정상 종료시에 강제로 XID처리할 명령필요.
// 소스 코드 수정하면서   commitForce와 RollbackForce 역할을 명확히 하면서
// 기존 if문을 제거.
IDE_RC mmdXa::commitForce( idvSQL   *aStatistics,
                           SChar    *aXIDStr, 
                           UInt      aXIDStrSize )
{
    ID_XID    sXID;
    mmdXid   *sXidObj = NULL;
    //fix BUG-30343 Committing a xid can be failed in replication environment.
    SChar         *sErrorMsg;
    UInt      sStage  = 0;

    ideLog::logLine(IDE_XA_0, "COMMIT FORCE(Trans_ID:%s)", aXIDStr);

    IDE_TEST(convertStringToXid(aXIDStr, aXIDStrSize, &sXID)
             != IDE_SUCCESS);

    IDE_TEST_RAISE(mmdXa::fix(&sXidObj, &sXID,MMD_XA_DO_LOG) != IDE_SUCCESS, not_exist_XID);
    //fix BUG-22349 XA need to handle state.
    sStage = 1;

    IDE_TEST_RAISE(sXidObj == NULL, not_exist_XID);

    sXidObj->lock();
    //fix BUG-22349 XA need to handle state.
    sStage = 2;

    if ( sXidObj->getState() == MMD_XA_STATE_PREPARED )
    {
        //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
        //fix BUG-30343 Committing a xid can be failed in replication environment.
        IDE_TEST_RAISE(sXidObj->commitTrans(NULL) != IDE_SUCCESS,commitError);

        /* bug-36037: invalid xid
           invalid xid의 경우 insertHeuri 실패를 허용하므로
           수행결과 체크 안함 */
        mmdManager::insertHeuristicTrans( aStatistics,  /* PROJ-2446 */
                                          &sXID, 
                                          QCM_XA_COMMITTED );
        sXidObj->setState(MMD_XA_STATE_HEURISTICALLY_COMMITTED);
    }
    //fix BUG-22349 XA need to handle state.
    sStage = 1;
    sXidObj->unlock();

    //fix BUG-22349 XA need to handle state.
    sStage = 0;
    IDE_ASSERT(mmdXa::unFix(sXidObj, &sXID, MMD_XA_NONE) == IDE_SUCCESS);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(commitError)
    {
        //fix BUG-30343 Committing a xid can be failed in replication environment.
        sErrorMsg =  ideGetErrorMsg(ideGetErrorCode());
        ideLog::logLine(IDE_XA_0, "Xid  Commit Error [%s], reason [%s]",aXIDStr ,sErrorMsg);

    }
    IDE_EXCEPTION(not_exist_XID);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_EXIST_XID ));
    }

    IDE_EXCEPTION_END;
    {
        //fix BUG-22349 XA need to handle state.
        switch(sStage)
        {
            case 2:
                sXidObj->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXidObj, &sXID, MMD_XA_NONE) == IDE_SUCCESS);
            default :
                break;
        }//switch

    }
    ideLog::logLine(IDE_XA_0, "COMMIT FORCE ERROR (Trans_ID:%s)", aXIDStr);

    return IDE_FAILURE;

}
// fix BUG-23815 XA session level  self record lock dead-lock이 발생한경우에
// TP monitor를 비정상 종료시에 강제로 XID처리할 명령필요.
// 소스 코드 수정하면서   commitForce와 RollbackForce 역할을 명확히 하면서
// 기존 if문을 제거.
/*
    여기서 XID는 트랜잭션 branch를 의미한다.
    XA session A에서 XID 100을 start
                      update t1 set C1=C1+1 WHERE C1=1;
                      xID 100 end.

                      XID 200을 start
                      update t1 set C1=C1+1 WHERE C1=1; <--- session level self recrod dead-lock발생.
                      xID 200 end.
   이러한문제를 해결하기 위하여  rollback force 명령어로 XID 100번을 rollback시켜야함.

*/
IDE_RC mmdXa::rollbackForce( idvSQL     *aStatistics,
                             SChar      *aXIDStr, 
                             UInt        aXIDStrSize )
{
    ID_XID         sXID;
    mmdXaState     sXIDState;
    mmdXid        *sXidObj = NULL;
    UInt           sStage  = 0;
    UInt           sUnfixOpt = MMD_XA_NONE;

    ideLog::logLine(IDE_XA_0, "ROLLBACK FORCE(Trans_ID:%s)", aXIDStr);

    IDE_TEST(convertStringToXid(aXIDStr, aXIDStrSize, &sXID)
             != IDE_SUCCESS);

    IDE_TEST_RAISE(mmdXa::fix(&sXidObj, &sXID, MMD_XA_DO_LOG) != IDE_SUCCESS, not_exist_XID);
    //fix BUG-22349 XA need to handle state.
    sStage = 1;

    IDE_TEST_RAISE(sXidObj == NULL, not_exist_XID);
    sXidObj->lock();
    //fix BUG-22349 XA need to handle state.
    sStage = 2;
    sXIDState = sXidObj->getState();

    // fix BUG-23815 XA session level  self record lock dead-lock이 발생한경우에
    // TP monitor를 비정상 종료시에 강제로 XID처리할 명령필요.
    // MMD_XA_STATE_ROLLBACK_ONLY도 rollback가능해야함.
    if ( (sXIDState == MMD_XA_STATE_PREPARED ) || (sXIDState == MMD_XA_STATE_ROLLBACK_ONLY))
    {
        //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
        sXidObj->rollbackTrans(NULL);
        mmdManager::insertHeuristicTrans( aStatistics,  /* PROJ-2446 */
                                          &sXID, 
                                          QCM_XA_ROLLBACKED );
        sXidObj->setState(MMD_XA_STATE_HEURISTICALLY_ROLLBACKED);
        //fix BUG-22349 XA need to handle state.
        sStage = 1;
        sXidObj->unlock();
    }
    else
    {
      // fix BUG-23815 XA session level  self record lock dead-lock이 발생한경우에
      // TP monitor를 비정상 종료시에 강제로 XID처리할 명령필요.
      /*
           여기서 XID는 트랜잭션 branch를 의미한다.
           XA session A에서 XID 100을 start
                      update t1 set C1=C1+1 WHERE C1=1;
                      xID 100 end.

                      XID 200을 start
                      update t1 set C1=C1+1 WHERE C1=1; <--- session level self recrod dead-lock발생.
                      xID 200 end.
          이러한문제를 해결하기 위하여  rollback force 명령어로 XID 100번을 rollback시켜야함.
      */
        if( sXIDState == MMD_XA_STATE_IDLE)
        {
            sXidObj->rollbackTrans(NULL);
            sStage = 1;
            sXidObj->unlock();
            sUnfixOpt = MMD_XA_REMOVE_FROM_XIDLIST;
        }
        else
        {
            //ignore, nothig to do
            sStage = 1;
            sXidObj->unlock();
        }
    }
    //fix BUG-22349 XA need to handle state.
    sStage = 0;
    IDE_TEST(mmdXa::unFix(sXidObj, &sXID, sUnfixOpt) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION(not_exist_XID);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_EXIST_XID ));
    }
    IDE_EXCEPTION_END;
    {
        //fix BUG-22349 XA need to handle state.
        switch(sStage)
        {
            case 2:
                sXidObj->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXidObj, &sXID, MMD_XA_NONE) == IDE_SUCCESS);
            default :
                break;
        }//switch

    }

    ideLog::logLine(IDE_XA_0, "ROLLBACK FORCE ERROR(Trans_ID:%s)", aXIDStr);

    return IDE_FAILURE;
}


/* BUG-25999
   Procedure를 이용하여 Heuristic하게 처리된, 사용자가 입력한 XID를 제거함
 */
IDE_RC mmdXa::removeHeuristicXid( idvSQL    *aStatistics,
                                  SChar     *aXIDStr, 
                                  UInt       aXIDStrSize )
{
    ID_XID    sXID;
    mmdXid   *sXidObj = NULL;
    UInt      sState  = 0;

    ideLog::logLine(IDE_XA_0, "Remove Xid that processed Heuristically(XID:%s)", aXIDStr);

    IDE_TEST(convertStringToXid(aXIDStr, aXIDStrSize, &sXID)
             != IDE_SUCCESS);

    IDE_TEST_RAISE(mmdXa::fix(&sXidObj, &sXID, MMD_XA_DO_LOG) != IDE_SUCCESS, not_exist_XID);
    sState = 1;

    IDE_TEST_RAISE(sXidObj == NULL, not_exist_XID);

    sXidObj->lock();
    sState = 2;

    switch ( sXidObj->getState() )
    {
        case MMD_XA_STATE_HEURISTICALLY_COMMITTED:
        case MMD_XA_STATE_HEURISTICALLY_ROLLBACKED:
            break;
        case MMD_XA_STATE_IDLE:
        case MMD_XA_STATE_ACTIVE:
        case MMD_XA_STATE_PREPARED:
        case MMD_XA_STATE_NO_TX:
        case MMD_XA_STATE_ROLLBACK_ONLY:
            IDE_RAISE(InvalidXaState);
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    IDU_FIT_POINT("mmdXa::removeHeuristicXid::lock::XID");
    IDE_TEST( mmdManager::removeHeuristicTrans( aStatistics, /* PROJ-2446 */
                                                &sXID ) 
              != IDE_SUCCESS );

    sState = 1;

    sXidObj->unlock();
    sState = 0;

    IDE_TEST(mmdXa::unFix(sXidObj, &sXID, MMD_XA_REMOVE_FROM_XIDLIST) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_exist_XID);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_NOT_EXIST_XID ));
    }
    IDE_EXCEPTION(InvalidXaState);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XA_STATE, MMDXASTATE[sXidObj->getState()] ));
    }
    IDE_EXCEPTION_END;
    {
        //fix BUG-22349 XA need to handle state.
        switch(sState)
        {
            case 2:
                sXidObj->unlock();
            case 1:
                IDE_ASSERT(mmdXa::unFix(sXidObj, &sXID, MMD_XA_NONE) == IDE_SUCCESS);
            default :
                break;
        }//switch

    }

    ideLog::logLine(IDE_XA_0, "Remove Xid that processed Heuristically. Error (XID:%s)", aXIDStr);

    return IDE_FAILURE;
}


