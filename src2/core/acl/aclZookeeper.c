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
 * $Id$
 **********************************************************************/

#include <aclZookeeper.h>
#include <acp.h>

ZKCResult aclZKC_result( acp_sint32_t aResultCode )
{
    ZKCResult sResult;

    switch( aResultCode )
    {
        case ZOK:
            sResult = ZKC_SUCCESS;
            break;
        case ZCONNECTIONLOSS:
            sResult = ZKC_CONNECTION_MISSING;
            break;
        case ZNONODE:
            sResult = ZKC_NO_NODE;
            break;
        case ZNODEEXISTS:
            sResult = ZKC_NODE_EXISTS;
            break;
        case ZOPERATIONTIMEOUT:
            sResult = ZKC_WAITING_TIMEOUT;
            break;
        case ZKC_DATA_TOO_LARGE:
            sResult = ZKC_DATA_TOO_LARGE;
            break;
        default:
            sResult = aResultCode;
            break;
    }

    return sResult;
}

/* 저장된 ZKS의 정보로 ZKS에 접속하는 함수 */
ZKCResult aclZKC_connect( aclZK_Handler ** aZKHandler,   /* zookeeper handler */
                          acp_char_t     * aServerInfo,  /* 주키퍼 서버의 정보 */
                          acp_uint32_t     aTimeout )    /* timeout 제한 시간 */
{
    aclZK_Handler  * zh;
    ZKCResult        sReturn;

    /* connection test */
    acp_sint32_t     sResult;
    acp_char_t       sTestPath[] = "/altibase_shard";
    acp_char_t       sTestBuffer[128];
    acp_sint32_t     sTestBufferLen = 128;
    acp_sint32_t     sRetryCnt = 0;

    /* 불필요한 zookeeper trace log를 제거한다. */
    zoo_set_debug_level( ZOO_LOG_LEVEL_ERROR );

    /* 정상적으로 연결했으나 연결하자마자 connection lost되는 경우가 발견되었다. *
     * 이를 막기 위해 connection lost의 경우 최대 10번 재시도 하도록 한다.       */
    while( sRetryCnt < ZKC_CONNECT_RETRY_MAX_CNT )
    {
        /* 1. connection info가 정상적인지 체크한다. */
        if( aServerInfo == NULL )
        {
            sReturn = ZKC_CONNECTION_INFO_MISSING;

            break;
        }

        /* 2. ZKS에 접속한다. */
        zh = zookeeper_init( aServerInfo,   /* server ip/port */
                             NULL,          /* watch */
                             aTimeout,      /* timeout check */
                             0,             /* client id */
                             0,             /* context */
                             0 );           /* flags */

        if( zh == NULL )
        {
            sReturn = ZKC_CONNECTION_FAIL;

            break;
        }
        else
        {
            /* 정상 작동 중인 ZKS가 2대 이하인 상태에서 ZKS에 접속할 경우 
             * connection 자체에는 성공하나 ZKS를 이용한 연산을 할 수 없다.
             * 이런 경우를 대비해 접속에 성공했는지 테스트 해본다. */
            sResult = zoo_get( zh,
                               sTestPath,
                               NULL,
                               sTestBuffer,
                               &sTestBufferLen,
                               NULL );

            /* 정상적으로 접근시 ZOK를 리턴하고 zookeeper client가 비어있을 경우 NONODE를 리턴한다. *
             * 어느쪽이든 접근 자체는 성공한 것이므로 SUCCESS 처리한다.                             */
            if( ( sResult == ZOK ) || ( sResult == ZNONODE ) )
            {
                *aZKHandler = zh;

                sReturn = ZKC_SUCCESS;

                break;
            }
            else
            {
                aclZKC_disconnect( zh );

                sReturn =  ZKC_CONNECTION_FAIL;

                sRetryCnt++;
            }
        }
    }

    return sReturn;
}

/* ZKS와의 연결을 끊는 함수 */
void aclZKC_disconnect( aclZK_Handler * aZKHandler /* zookeeper handler */ )
{
    if( aZKHandler != NULL )
    {
        zookeeper_close( aZKHandler );
    }
}

/* ZKS의 데이터를 삽입할 함수 */
ZKCResult aclZKC_setInfo( aclZK_Handler * aZKHandler, /* zookeeper handler */
                          acp_char_t    * aPath,      /* path */
                          acp_char_t    * aData,      /* 삽입할 데이터 */
                          acp_sint32_t    aDataSize ) /* 삽입할 데이터의 길이 */
{
    acp_sint32_t sResult;

    sResult = zoo_set( aZKHandler,  /* zookeeper handler */
                       aPath,       /* path */
                       aData,       /* 삽입할 데이터 */
                       aDataSize,   /* 삽입할 데이터의 길이 */
                       -1 );        /* version */

    return aclZKC_result( sResult );
}

/* ZKS의 디렉토리(노드)를 추가하는 함수 */
ZKCResult aclZKC_insertNode( aclZK_Handler * aZKHandler,    /* zookeeper handler */
                             acp_char_t    * aPath,         /* path */
                             acp_char_t    * aData,         /* 삽입할 데이터 */
                             acp_sint32_t    aDataSize,     /* 삽입할 데이터의 길이 */
                             acp_bool_t      aIsEphemeral ) /* 임시 노드 여부 */
{
    acp_sint32_t sResult;
    acp_sint32_t sNodeflag;

    if( aIsEphemeral == ACP_TRUE )
    {
        sNodeflag = ZOO_EPHEMERAL;
    }
    else
    {
        sNodeflag = ZOO_PERSISTENT;
    }

    sResult = zoo_create( aZKHandler,            /* zookeeper handler */ 
                          aPath,                 /* 추가할 path */
                          aData,                 /* 삽입할 데이터 */
                          aDataSize,             /* 삽입할 데이터의 길이 */
                          &ZOO_OPEN_ACL_UNSAFE,  /* ACL */
                          sNodeflag,             /* flags */
                          NULL,                  /* path_buffer */
                          0 );                   /* path_buffer_len */

    return aclZKC_result( sResult );
}

/* ZKS의 데이터를 가져오는 함수 */ 
ZKCResult aclZKC_getInfo( aclZK_Handler * aZKHandler,   /* zookeeper handler */
                          acp_char_t    * aPath,        /* 데이터를 가져올 path */
                          acp_char_t    * aBuffer,      /* 가져올 데이터를 저장할 버퍼 */
                          acp_sint32_t  * aDataLen )    /* 가져올 데이터의 길이 */

{
    acp_sint32_t sResult;
    acp_sint32_t sBufferSize = *aDataLen;

    acpMemSet( aBuffer, 0, *aDataLen );

    /* 주의점 : zoo_get의 DataLen 변수는 하나의 변수로 2가지 일을 한다.
     *          1. 데이터를 가져올 버퍼의 길이(IN)
     *          2. 실제로 가져온 데이터의 길이(OUT)
     *          그러므로 한번 get 함수를 사용한 후 DataLen 변수를 그대로 다시 사용할 경우 
     *          두번째 get은 첫 get에서 가져온 길이로 제한되는 문제가 발생한다.  */

    sResult = zoo_get( aZKHandler,   /* zookeeper handler */
                       aPath,        /* 데이터를 가져올 path */
                       NULL,         /* watch */
                       aBuffer,      /* 가져올 데이터를 저장할 버퍼 */
                       aDataLen,     /* 가져온 데이터의 길이 */
                       NULL );       /* 통계 */

    /* 정상적으로 받아왔지만 데이터의 길이가 버퍼의 크기와 같다면            *
     * 데이터의 길이가 더 길어 zookeeper server에 의해 잘렸을 가능성이 있다. */
    if( ( sResult == ZOK ) && ( sBufferSize == *aDataLen ) )
    {
        sResult = ZKC_DATA_TOO_LARGE;
    }

    return aclZKC_result( sResult );
}

/* ZKS의 특정 노드의 자식을 가져오는 함수 */
ZKCResult aclZKC_getChildren( aclZK_Handler * aZKHandler, /* zookeeper handler */
                              acp_char_t    * aPath,      /* 자식 노드를 가져올 노드의 path */
                              String_vector * aBuffer )   /* 가져온 자식을 보관할 공간 */
{
    acp_sint32_t sResult;

    sResult = zoo_get_children( aZKHandler, /* zookeeper handler */
                                aPath,      /* 자식 노드를 가져올 노드의 path */
                                NULL,       /* watch */
                                aBuffer );  /* 가져온 자식을 보관할 공간(string vector) */ 

    return aclZKC_result( sResult );
}

/* ZKS의 노드를 제거하는 함수 */
ZKCResult aclZKC_deleteNode( aclZK_Handler * aZKHandler, /* zookeeper handler */
                             acp_char_t    * aPath )     /* 제거할 노드의 path */
{
    acp_sint32_t sResult;

    sResult = zoo_delete( aZKHandler, /* zookeeper handler */
                          aPath,      /* 제거할 노드의 path */
                          -1 );       /* version */

    return aclZKC_result( sResult );
}

/* ZKS의 데이터를 제거할 함수
   ZKS에는 명시적인 데이터 제거가 없으므로 빈 데이터로 값을 대체하여 데이터를 지운 것과 동일한 효과를 낸다.  */
ZKCResult aclZKC_deleteInfo( aclZK_Handler * aZKHandler,  /* zookeeper handler */
                             acp_char_t    * aPath )      /* 데이터를 제거할 노드의 path */
{
    acp_sint32_t sResult;

    sResult = zoo_set( aZKHandler,  /* zookeeper handler */
                       aPath,       /* path */
                       NULL,        /* 삽입할 데이터 */
                       0,           /* 삽입할 데이터의 길이 */
                       -1 );        /* version */

    return aclZKC_result( sResult );
}

/* ZKS의 특정 노드가 존재하는지 체크하는 함수 */
ZKCResult aclZKC_existNode( aclZK_Handler * aZKHandler, /* zookeeper handler */
                            acp_char_t    * aPath )      /* 체크할 노드의 path */
{
    acp_sint32_t sResult;

    sResult = zoo_exists( aZKHandler, /* zookeeper handler */
                          aPath,      /* 체크할 노드의 path */
                          0,          /* watch */
                          NULL );     /* 통계정보 */

    return aclZKC_result( sResult );    
}

/* multi operation으로 작업을 수행하는 함수 */
ZKCResult aclZKC_multiOp( aclZK_Handler   * aZKHandler,    /* zookeeper handler */
                          acp_sint32_t      aCount,        /* multi op로 수행할 count */
                          zoo_op_t        * aOps,          /* multi operation의 set  */
                          zoo_op_result_t * aOpResult )    /* 각 op의 결과를 받을 result set */
{
    acp_sint32_t sResult;

    sResult = zoo_multi( aZKHandler, 
                         aCount, 
                         aOps, 
                         aOpResult );

    return aclZKC_result( sResult );
}

/* 특정 노드의 데이터에 watch를 거는 함수 
 * 노드의 데이터가 변경될때 알람이 발생한다.
 * 데이터를 현재와 동일한 값으로 변경해도 알람이 발생한다. */
ZKCResult aclZKC_Watch_Data( aclZK_Handler   * aZKHandler,  /* zookeeper handler */
                             watcher_fn        aWatchFunc,  /* 알람 발생시 호출되는 함수 */
                             acp_char_t      * aPath )      /* 데이터 변경을 확인할 노드의 path */
{
    acp_sint32_t sResult;
    acp_char_t   sBuffer[128];
    acp_sint32_t sBufferLen = 128;
    
    sResult = zoo_wget( aZKHandler,     /* zookeeper handler */
                        aPath,          /* path */
                        aWatchFunc,     /* watcher func */
                        NULL,           /* watcherCtx */
                        sBuffer,        /* buffer */
                        &sBufferLen,    /* buffer_len */
                        NULL );         /* 통계정보 */

    return aclZKC_result( sResult );
} 

/* 특정 노드의 추가/삭제에 watch를 거는 함수
 * 해당 노드가 추가/삭제 될 때 마다 알람이 발생한다. */
ZKCResult aclZKC_Watch_Node( aclZK_Handler   * aZKHandler,  /* zookeeper_handler */
                             watcher_fn        aWatchFunc,  /* 알람 발생시 호출되는 함수 */
                             acp_char_t      * aPath )      /* 추가/삭제를 확인할 노드의 path */
{
    acp_sint32_t          sResult;
     
    sResult = zoo_wexists( aZKHandler,  /* zookeeper_handler */
                           aPath,       /* path */
                           aWatchFunc,  /* watcher func */
                           NULL,        /* watcherCtx */
                           NULL );      /* 통계정보 */
    
    return aclZKC_result( sResult );
}
