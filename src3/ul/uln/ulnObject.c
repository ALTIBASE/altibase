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

#include <uln.h>
#include <ulnPrivate.h>
#include <uluLock.h>
#include <ulnObject.h>

/**
 * ulnObjectInitialize
 *
 * @param aObject
 *  초기화하고자 하는 객체를 가리키는 포인터.
 * @param aType
 *  객체의 타입.
 * @param aSubType
 *  객체의 서브타입. DESC 에만 해당한다. 객체의 타입이 DESC 가 아니면
 *  ULN_DESC_TYPE_NODESC 를 넘겨주면 된다.
 * @param aState
 *  초기 상태로 설정하고자 하는 state.
 * @param aPool
 *  객체가 사용할 청크 풀을 가리키는 포인터.
 * @param aMemory
 *  객체가 사용할 메모리 인스턴스를 가리키는 포인터.
 *
 * @note
 *  주의 : 이 함수는 Object 안의 diagnostic header 와 record 는 건드리지 않는다.
 */
ACI_RC ulnObjectInitialize(ulnObject    *aObject,
                           ulnObjType    aType,
                           ulnDescType   aSubType,
                           acp_sint16_t  aState,
                           uluChunkPool *aPool,
                           uluMemory    *aMemory)
{
    ACE_ASSERT(aObject != NULL);
    ACE_ASSERT(aPool != NULL);
    ACE_ASSERT(aMemory != NULL);

    ACE_ASSERT(aType > ULN_OBJ_TYPE_MIN && aType < ULN_OBJ_TYPE_MAX);

    acpListInitObj(&aObject->mList, aObject);

    ULN_OBJ_SET_TYPE(aObject, aType);
    ULN_OBJ_SET_DESC_TYPE(aObject, aSubType);
    ULN_OBJ_SET_STATE(aObject, aState);

    /*
     * 일단 최초에 객체가 만들어졌을 때에는 AllocHandle 일 경우이다.
     * 그러나, AllocHandle 에서 빠져나가고 할 때 다시 ULN_FID_NONE 으로 세팅하기 어려우므로
     * 지금 곧장 FID_NONE 으로 세팅하자.
     * 솔직히 AllocHandle 이 리턴하기도 전에 그 핸들로 함수 호출하거나 하는 경우는 없다고 봐도
     * 무방할 것이다.
     */
    aObject->mExecFuncID = ULN_FID_NONE;

    aObject->mPool       = aPool;
    aObject->mMemory     = aMemory;

    /*
     * SQLError() 함수때문에 존재한다.
     * ulnClearDiagnosticInfoFromObject() 함수에서도 1 로 초기화 한다.
     */
    aObject->mSqlErrorRecordNumber = 1;

    return ACI_SUCCESS;
}

acp_sint32_t ulnObjectGetSqlErrorRecordNumber(ulnObject *aHandle)
{
    return aHandle->mSqlErrorRecordNumber;
}

ACI_RC ulnObjectSetSqlErrorRecordNumber(ulnObject *aHandle, acp_sint32_t aRecNumber)
{
    aHandle->mSqlErrorRecordNumber = aRecNumber;

    return ACI_SUCCESS;
}
