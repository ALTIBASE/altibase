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

#ifndef _O_ACL_ZKC_H_
#define _O_ACL_ZKC_H_ 1

#include <acpTypes.h>

#if defined(ALTI_CFG_OS_LINUX)
#include <zookeeper.h>
#include <zookeeper.jute.h>
#include <zk_adaptor.h>
#endif

/* Zookeeper C Client가 리눅스만 지원하므로 리눅스 에서는 정상적으로 aclZookeeper.c를 컴파일 하지만 *
 * 리눅스 외 OS에서는 형태만 있는 비어있는 파일(aclZookeeperstub.cpp)을 대신 compile한다.           */

ACP_EXTERN_C_BEGIN

#define ZKC_CONNECT_RETRY_MAX_CNT (10)

#if defined(ALTI_CFG_OS_LINUX)
typedef zhandle_t   aclZK_Handler;
#else
/* 해당 부분은 LINUX외 장비에서 compile을 수행하기 위해 추가된 부분일뿐 실제 수행과는 관계가 없다. */
typedef acp_sint32_t aclZK_Handler;
typedef acp_sint32_t watcher_fn;
typedef acp_sint32_t String_vector;
typedef acp_sint32_t zoo_op_t;
typedef acp_sint32_t zoo_op_result_t;
#endif

typedef enum
{
    ZKC_SUCCESS = 0,
    ZKC_CONNECTION_INFO_MISSING,
    ZKC_CONNECTION_FAIL,
    ZKC_CONNECTION_MISSING,
    ZKC_ZOOCFG_NO_EXIST,
    ZKC_NODE_EXISTS,
    ZKC_NO_NODE,
    ZKC_WAITING_TIMEOUT,
    ZKC_DB_VALIDATE_FAIL,
    ZKC_VALIDATE_FAIL,
    ZKC_STATE_VALIDATE_FAIL,
    ZKC_LOCK_ERROR,
    ZKC_DATA_TOO_LARGE,
    ZKC_META_CORRUPTED,
    ZKC_ETC_ERROR
} ZKCResult;

/* ZKC 함수의 리턴값을 에러코드로 바꿔주는 함수 */
ZKCResult aclZKC_result( acp_sint32_t );

/* 저장된 ZKS의 정보로 ZKS에 접속하는 함수 */
ZKCResult aclZKC_connect( aclZK_Handler **, acp_char_t *, acp_uint32_t );

/* ZKS와의 연결을 끊는 함수 */
void aclZKC_disconnect( aclZK_Handler * );

/* ZKS의 데이터를 삽입하는 함수 */
ZKCResult aclZKC_setInfo( aclZK_Handler *, acp_char_t *, acp_char_t *, acp_sint32_t );

/* ZKS의 디렉토리(노드)를 추가하는 함수 */
ZKCResult aclZKC_insertNode( aclZK_Handler *, acp_char_t *, acp_char_t *, acp_sint32_t, acp_bool_t );

/* ZKS의 데이터를 가져오는 함수 */
ZKCResult aclZKC_getInfo( aclZK_Handler *, acp_char_t *, acp_char_t *, acp_sint32_t *);

/* ZKS의 특정 노드의 자식을 가져오는 함수 */
ZKCResult aclZKC_getChildren( aclZK_Handler *, acp_char_t *, String_vector * );

/* ZKS의 디렉토리를 제거하는 함수 */
ZKCResult aclZKC_deleteNode( aclZK_Handler *, acp_char_t * );

/* ZKS의 데이터를 제거하는 함수 */
ZKCResult aclZKC_deleteInfo( aclZK_Handler *, acp_char_t * );

/* ZKS내 특정 노드의 존재를 확인하는 함수 */
ZKCResult aclZKC_existNode( aclZK_Handler *, acp_char_t * );

/* multi op를 수행하는 함수 */
ZKCResult aclZKC_multiOp( aclZK_Handler *, acp_sint32_t, zoo_op_t *, zoo_op_result_t * );

/* 특정 노드의 데이터에 watch를 거는 함수 */
ZKCResult aclZKC_Watch_Data( aclZK_Handler *, watcher_fn, acp_char_t * );

/* 특정 노드의 추가/삭제에 watch를 거는 함수 */
ZKCResult aclZKC_Watch_Node( aclZK_Handler *, watcher_fn, acp_char_t * );

ACP_EXTERN_C_END

#endif
