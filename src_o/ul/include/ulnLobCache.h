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

#ifndef _O_ULN_LOB_CACHE_H_
#define _O_ULN_LOB_CACHE_H_ 1

#include <ulnPrivate.h>

typedef enum ulnLobCacheErr {
    LOB_CACHE_NO_ERR        = 0,
    LOB_CACHE_INVALID_RANGE = 1
} ulnLobCacheErr;

/**
 *  ulnLobCacheCreate
 *
 *  @aLobCache          : ulnLobCache의 객체
 *
 *  = LOB을 캐쉬할 수 있도록 구조를 생성한다.
 *  = 성공하면 ACI_SUCCESS, 실패하면 ACI_FAILURE
 */
ACI_RC ulnLobCacheCreate(ulnLobCache **aLobCache);

/**
 *  ulnLobCacheReInitialize
 *
 *  = 기존에 할당된 메모리를 재사용한다.
 */
ACI_RC ulnLobCacheReInitialize(ulnLobCache *aLobCache);

/**
 *  ulnLobCacheDestroy
 *
 *  = ulnLobCache 객체를 소멸한다.
 */
void   ulnLobCacheDestroy(ulnLobCache **aLobCache);

/**
 *  ulnLobCacheGetErr
 *
 *  내부에서 발생한 에러를 가져온다.
 */
ulnLobCacheErr ulnLobCacheGetErr(ulnLobCache  *aLobCache);

/**
 *  ulnLobCacheAdd
 *
 *  @aValue     : Caching할 LOB Data
 *  @aLength    : LOB Data 길이
 *
 *  aValue가 NULL인 경우에는 LOB Data길이만 저장한다.
 *  즉 임계치보다 LOB Data가 크더라도 LOB Data 길이는 저장해
 *  SQLGetLobLength()에서 활용한다.
 */
ACI_RC ulnLobCacheAdd(ulnLobCache  *aLobCache,
                      acp_uint64_t  aLocatorID,
                      acp_uint8_t  *aValue,
                      acp_uint32_t  aLength,
                      acp_bool_t    aIsNull);

/**
 *  ulnLobCacheRemove
 *
 *  aLocatorID에 해당하는 데이터를 제거한다.
 */
ACI_RC ulnLobCacheRemove(ulnLobCache  *aLobCache,
                         acp_uint64_t  aLocatorID);

/**
 *  ulnLobCacheGetLob
 *
 *  @aFromPos   : LOB의 시작 Offset
 *  @aForLength : LOB의 길이
 *  @aContext   : LOB Interface의 포인터가 저장되어 있다.
 *
 *  Cache되어 있는 LOB 데이터를 LOB Buffer(User Buffer)에 반환한다.
 */
ACI_RC ulnLobCacheGetLob(ulnLobCache  *aLobCache,
                         acp_uint64_t  aLocatorID,
                         acp_uint32_t  aFromPos,
                         acp_uint32_t  aForLength,
                         ulnFnContext *aContext);

/**
 *  ulnLobCacheGetLobLength
 *
 *  aLocatorID에 해당하는 LOB Data의 길이정보를 aLength에 반환한다.
 */
ACI_RC ulnLobCacheGetLobLength(ulnLobCache  *aLobCache,
                               acp_uint64_t  aLocatorID,
                               acp_uint32_t *aLength,
                               acp_bool_t   *aIsNull);

#endif /* _O_ULN_LOB_CACHE_H_ */
