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

#include <idtContainer.h>
#include <mtc.h>
#include <sdiZookeeper.h>
#include <sdiFOThread.h>
#include <smi.h>
#include <qdsd.h>

idBool          sdiZookeeper::mIsConnect = ID_FALSE;
idBool          sdiZookeeper::mIsGetShardLock = ID_FALSE;
idBool          sdiZookeeper::mIsGetZookeeperLock = ID_FALSE;
aclZK_Handler * sdiZookeeper::mZKHandler = NULL ;
SChar           sdiZookeeper::mMyNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
iduMutex        sdiZookeeper::mConnectMutex;
SChar           sdiZookeeper::mTargetNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
SChar           sdiZookeeper::mFailoverNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
ZKJOB           sdiZookeeper::mRunningJobType = ZK_JOB_NONE;
qciUserInfo     sdiZookeeper::mUserInfo;
UInt            sdiZookeeper::mTickTime = 0;
UInt            sdiZookeeper::mSyncLimit = 0;
UInt            sdiZookeeper::mServerCnt = 0;
iduList         sdiZookeeper::mJobAfterTransEnd;
iduList         sdiZookeeper::mRevertJobList;

SChar           sdiZookeeper::mFirstStopNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
SChar           sdiZookeeper::mSecondStopNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
SChar           sdiZookeeper::mFirstStopReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
SChar           sdiZookeeper::mSecondStopReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::logAndSetErrMsg              *
 * ------------------------------------------------------------------*
 * ZKC 함수가 실패했을때 그 결과를 TRC 로그에 남긴다.
 *
 * aErrCode - [IN] 에러 코드
 *********************************************************************/
void sdiZookeeper::logAndSetErrMsg( ZKCResult aErrCode, SChar * aInfo )
{
    SChar sEmptyStr[] = "";
    SChar * sInfo = sEmptyStr;
    if ( aInfo != NULL )
    {
        sInfo = aInfo;
    }
    switch( aErrCode )
    {
        case ZKC_CONNECTION_FAIL:
            ideSetErrorCode( sdERR_ABORT_SDI_ZKC_CONNECTION_FAIL );
            ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] Failed to connect ZKS." );
            break;
        case ZKC_CONNECTION_MISSING:
            /* ZKS와 연결이 끊긴 상태 */
            /* 에러코드를 세팅하고 trc로그를 남긴 후 assert를 호출한다. */
            mIsConnect = ID_FALSE;
            ideSetErrorCode( sdERR_FATAL_SDI_ZKC_CONNECTION_MISSING );
            ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] Connection with ZKS has been lost." );
            IDE_CALLBACK_FATAL( "Connection with ZKS has been lost." );
            break;
        case ZKC_NODE_EXISTS:
            ideSetErrorCode( sdERR_ABORT_SDI_ZKC_NODE_EXISTS );
            ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] Node already exist." );
            break;
        case ZKC_NO_NODE:
            ideSetErrorCode( sdERR_ABORT_SDI_ZKC_NO_NODE );
            ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] Node does not exist." );
            break;
        case ZKC_WAITING_TIMEOUT:
            ideSetErrorCode( sdERR_ABORT_SDI_ZKC_WAITING_TIMEOUT );
            ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] Waiting timeout." );
            break;
        case ZKC_DB_VALIDATE_FAIL:  
            ideSetErrorCode( sdERR_ABORT_SDI_ZKC_DB_VALIDATE_FAIL );
            break;
        case ZKC_VALIDATE_FAIL:
            ideSetErrorCode( sdERR_ABORT_SDI_ZKC_VALIDATE_FAIL, sInfo );
            break;
        case ZKC_STATE_VALIDATE_FAIL:
            ideSetErrorCode( sdERR_ABORT_SDI_ZKC_STATE_VALIDATE_FAIL );
            break;
        case ZKC_META_CORRUPTED:
            ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] Metadata is Corrupted." );
            IDE_CALLBACK_FATAL( "Zookeeper metadata is corrupted." );
            break;
        case ZKC_LOCK_ERROR:
            ideSetErrorCode( sdERR_FATAL_SDI_ZKC_LOCK_ERROR );
            ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] Lock is Currupted." );
            break;
        case ZKC_ETC_ERROR:
        default:
            ideSetErrorCode( sdERR_ABORT_SDI_ZKC_ETC_ERROR , aErrCode);
            ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] ETC Error %"ID_INT32_FMT, aErrCode );
            break;
    } 
}
/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::initializeStatic             *
 * ------------------------------------------------------------------*
 * sdiZookeeper class staticaly initialize
 *
 *********************************************************************/
IDE_RC sdiZookeeper::initializeStatic()
{
    mIsConnect = ID_FALSE;
    mIsGetShardLock = ID_FALSE;
    mIsGetZookeeperLock = ID_FALSE;

    idlOS::memset( &mUserInfo, 0, ID_SIZEOF( qciUserInfo ) );
    
    IDE_TEST( mConnectMutex.initialize( "ZOOKEEPER_CLIENT_CONNECTION_MUTEX",
                                        IDU_MUTEX_KIND_POSIX,
                                        IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    mMyNodeName[0] = '\0';

    idlOS::strncpy( mFailoverNodeName,
                    SDM_NA_STR,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    idlOS::strncpy( mFirstStopNodeName,
                    SDM_NA_STR,
                    SDI_NODE_NAME_MAX_SIZE + 1 );
    idlOS::strncpy( mSecondStopNodeName,
                    SDM_NA_STR,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    idlOS::strncpy( mFirstStopReplName,
                    SDM_NA_STR,
                    SDI_REPLICATION_NAME_MAX_SIZE + 1 );
    idlOS::strncpy( mSecondStopReplName,
                    SDM_NA_STR,
                    SDI_REPLICATION_NAME_MAX_SIZE + 1 );

    IDU_LIST_INIT(&mJobAfterTransEnd);

    IDU_LIST_INIT(&mRevertJobList);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::initialize                   *
 * ------------------------------------------------------------------*
 * sdiZookeeper nodename 초기화 한다.
 *
 * aNodeName - [IN] 해당 노드에서 사용할 노드의 이름
 *********************************************************************/
void sdiZookeeper::initialize( SChar       * aNodeName,
                               qciUserInfo * aUserInfo )
{
    qciUserInfo * sUserInfo = &mUserInfo;
    idlOS::strncpy( mMyNodeName,
                    aNodeName,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    QCI_COPY_USER_INFO( sUserInfo, aUserInfo );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::finalizeStatic               *
 * ------------------------------------------------------------------*
 * sdiZookeeper class staticaly finalize
 *********************************************************************/
IDE_RC sdiZookeeper::finalizeStatic()
{
    IDE_TEST( mConnectMutex.destroy() != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::finalize                     *
 * ------------------------------------------------------------------*
 * sdiZookeeper 클래스 nodename 정리 한다.
 *********************************************************************/
void sdiZookeeper::finalize()
{
    idlOS::memset( mMyNodeName, 0, SDI_NODE_NAME_MAX_SIZE + 1 );

    return;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::connect                      *
 * ------------------------------------------------------------------*
 * zookeeper server에 접속한다.
 * zoo.cfg에 접근해 연결 정보를 가져온 후 해당 정보를 통해 ZKS에 접속한다.
 * 
 * aIsTempConnect - [IN]  임시 접속인지 여부.
 *                        해당 인자는 create database 구문에서 호출될 경우에만
 *                        TRUE로 세팅되고 나머지 경우에는 FALSE로 세팅된다.
 *********************************************************************/
IDE_RC sdiZookeeper::connect( idBool aIsTempConnect )
{
    ZKCResult sResult;
    idBool    sIsLock = ID_FALSE;

    /* zoo.cfg open */
    SChar   * sAltiHomePath;
    SChar     sFilePath[256] = {0,};
    FILE    * sFP;

    /* file read */
    SInt      sLength;
    SChar     sLineBuffer[IDP_MAX_PROP_LINE_SIZE];
    idBool    sIsFileOpen = ID_FALSE;

    /* zookeeper info */
    UInt      sServerCnt = 0;
    SChar     sServerInfo[256] = {0,};
    UInt      sServerInfoLength = 0;
    SChar   * sStringPtr;
    SChar     sPortNum[6] = {0,};
    UInt      sTickTime = 0;
    UInt      sSyncLimit = 0;

    if( aIsTempConnect == ID_FALSE )
    {
        IDE_ASSERT( mConnectMutex.lock( NULL ) == IDE_SUCCESS );
        sIsLock = ID_TRUE;
    }
    else
    {
        /* 임시 접속의 경우 동시 접근이 불가능하므로 mutex를 잡을 필요가 없다. */
    }

    if( mIsConnect == ID_TRUE )
    {
        /* 이미 연결되어 있다면 재연결 할 필요가 없다. */
        IDE_CONT( normal_exit );
    }

    /* 1. zoo.cfg에 접근해 연결 정보를 가져온다. */
    sAltiHomePath = idlOS::getenv( IDP_HOME_ENV );

    idlOS::strncpy( sFilePath,
                    sAltiHomePath,
                    ID_SIZEOF( sFilePath ) );
    idlOS::strncat( sFilePath,
                    IDL_FILE_SEPARATORS,
                    ID_SIZEOF( sFilePath ) - idlOS::strlen( sFilePath ) - 1 );
    idlOS::strncat( sFilePath,
                    "ZookeeperServer",
                    ID_SIZEOF( sFilePath ) - idlOS::strlen( sFilePath ) - 1 );
    idlOS::strncat( sFilePath,
                    IDL_FILE_SEPARATORS,
                    ID_SIZEOF( sFilePath ) - idlOS::strlen( sFilePath ) - 1 );
    idlOS::strncat( sFilePath, 
                    "conf",
                    ID_SIZEOF( sFilePath ) - idlOS::strlen( sFilePath ) - 1 );
    idlOS::strncat( sFilePath,
                    IDL_FILE_SEPARATORS,
                    ID_SIZEOF( sFilePath ) - idlOS::strlen( sFilePath ) - 1 );
    idlOS::strncat( sFilePath, 
                    "zoo.cfg",
                    ID_SIZEOF( sFilePath ) - idlOS::strlen( sFilePath ) - 1 );
    sFP = idlOS::fopen( sFilePath, "r" );

    if( sFP == NULL )
    {
        /* 파일 open에 실패했다면 create database 구문에서 호출된게 아니라면 오류 상황이다. */
        IDE_TEST_RAISE( aIsTempConnect == ID_FALSE , err_no_file );

        /* create database 구문에서 호출된 경우에는 통과 한다. */
        IDE_CONT( normal_exit );
    }

    sIsFileOpen = ID_TRUE;

    while( idlOS::idlOS_feof( sFP ) == 0 )
    {
        idlOS::memset( sLineBuffer, 0, IDP_MAX_PROP_LINE_SIZE );

        if( idlOS::fgets( sLineBuffer, IDP_MAX_PROP_LINE_SIZE, sFP ) == NULL )
        {
            /* 데이터가 없음 */
            break;        
        }

        /* white space 제거 */
        idp::eraseWhiteSpace( sLineBuffer );
        sLength = idlOS::strlen( sLineBuffer );

        /* 빈 줄이나 주석은 제외 */
        if( ( sLength > 0 ) && ( sLineBuffer[0] != '#' ) )
        {
            /* tickTime 정보를 가져온다. */
            if( idlOS::strncmp( sLineBuffer, "tickTime", 8 ) == 0 )
            {
                sStringPtr = idlOS::strtok( sLineBuffer, "=" );
                sStringPtr = idlOS::strtok( NULL, "\0" );

                /* 사용자 실수로 숫자가 아닌 문자가 들어있을 경우 atoi 함수를 사용하면 0으로 세팅한다. */
                sTickTime = idlOS::atoi( sStringPtr );
            }

            /* synclimit 정보를 가져온다. */
            else if( idlOS::strncmp( sLineBuffer, "syncLimit", 9 ) == 0 )
            {
                sStringPtr = idlOS::strtok( sLineBuffer, "=" );
                sStringPtr = idlOS::strtok( NULL, "\0" );

                /* 사용자 실수로  숫자가 아닌 문자가 들어있을 경우 atoi 함수를 사용하면 0으로 세팅한다. */
                sSyncLimit = idlOS::atoi( sStringPtr );
            }

            /* clientPort 정보를 가져온다. */
            else if( idlOS::strncmp( sLineBuffer, "clientPort", 10 ) == 0 )
            {
                sStringPtr = idlOS::strtok( sLineBuffer, "=" );
                sStringPtr = idlOS::strtok( NULL, "\0" );

                /* clientPort는 5자리+/0 까지 최대 6자리이며 그 이상일 경우 오류인 상황이다. */
                IDE_TEST_RAISE( idlOS::strlen( sStringPtr ) > 6, err_no_serverInfo );

                idlOS::strncpy( sPortNum,
                                sStringPtr,
                                ID_SIZEOF( sPortNum ) );
            }

            /* server 정보를 가져온다. */
            /* 참고 : 서버 정보를 보관하는 server string에 port 정보가 함께 들어가므로
               zoo.cfg에 client port 정보가 server 정보보다 먼저 나와야 한다. */
            else if( idlOS::strncmp( sLineBuffer, "server.", 7 ) == 0 )
            {
                sStringPtr = idlOS::strtok( sLineBuffer, "=" );
                sStringPtr = idlOS::strtok( NULL, ":" );

                if( sServerCnt == 0 )
                {
                    sServerInfoLength = idlOS::strlen( sStringPtr ) + 1 + idlOS::strlen( sPortNum );

                    idlOS::strncat( sServerInfo,
                                    sStringPtr,
                                    ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                    idlOS::strncat( sServerInfo, 
                                    ":",
                                    ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                    idlOS::strncat( sServerInfo,
                                    sPortNum,
                                    ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                }
                else
                {

                    sServerInfoLength += idlOS::strlen( sStringPtr ) + 2 + idlOS::strlen( sPortNum );

                    /* 버퍼크기(256)보다 길면 안된다. */
                    IDE_TEST_RAISE( sServerInfoLength >= 256, err_too_much_serverInfo );

                    idlOS::strncat( sServerInfo,
                                    ",",
                                    ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                    idlOS::strncat( sServerInfo,
                                    sStringPtr,
                                    ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                    idlOS::strncat( sServerInfo,
                                    ":",
                                    ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                    idlOS::strncat( sServerInfo,
                                    sPortNum,
                                    ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                }

                sServerCnt++;
            }
        }
    }

    sIsFileOpen = ID_FALSE;
    (void)idlOS::fclose( sFP );

    if( SDU_SHARD_ZOOKEEPER_TEST == 1 )
    {
            if( sServerCnt == 0 )
            {
                sServerInfoLength = idlOS::strlen( "localhost" ) + 1 + idlOS::strlen( sPortNum );
                /* 버퍼크기(256)보다 길면 안된다. */
                IDE_TEST_RAISE( sServerInfoLength >= 256, err_too_much_serverInfo );

                idlOS::strncat( sServerInfo,
                                "localhost" ,
                                ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                idlOS::strncat( sServerInfo, 
                                ":",
                                ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                idlOS::strncat( sServerInfo,
                                sPortNum,
                                ID_SIZEOF( sServerInfo ) - idlOS::strlen( sServerInfo ) - 1 );
                sServerCnt++;
            }
    }
    else
    {
        /* 서버는 최소 3개 이상 있어야 한다. */
        IDE_TEST_RAISE( sServerCnt < SDI_ZKC_ZKS_MIN_CNT, err_no_serverInfo );
     }
    /* 서버의 숫자는 최대 7개까지만 허용된다. */
    IDE_TEST_RAISE( sServerCnt > SDI_ZKC_ZKS_MAX_CNT, err_too_much_serverInfo );

    /* ticktime과 synclimit가 둘다 존재해야 한다. */
    IDE_TEST_RAISE( sTickTime * sSyncLimit == 0, err_no_serverInfo );

    /* 2. 가져온 연결 정보로 ZKS에 접속한다. */
    sResult = aclZKC_connect( &mZKHandler,
                              sServerInfo,
                              sTickTime * sSyncLimit );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_connectionFail );

    mIsConnect = ID_TRUE;

    mTickTime  = sTickTime;
    mSyncLimit = sSyncLimit;
    mServerCnt = sServerCnt;

    IDE_EXCEPTION_CONT( normal_exit );

    if( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        IDE_ASSERT( mConnectMutex.unlock() == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_no_file )
    {
        /* zoo.cfg가 없음 */
        ideSetErrorCode( sdERR_ABORT_SDI_ZKC_ZOOCFG_NO_EXIST );
    }
    IDE_EXCEPTION( err_no_serverInfo )
    {
        /* zoo.cfg는 있으나 server의 정보가 없음 */
        ideSetErrorCode( sdERR_ABORT_SDI_ZKC_CONNECTION_INFO_MISSING );
    }
    IDE_EXCEPTION( err_too_much_serverInfo )
    {
        /* 서버 정보가 너무 많음 */
       ideSetErrorCode( sdERR_ABORT_SDI_ZKC_TOO_MUCH_SERVER_INFO ); 
    }
    IDE_EXCEPTION( err_connectionFail )
    {
        /* 연결을 시도했으나 실패 */
        if( aIsTempConnect == ID_FALSE )
        {
            logAndSetErrMsg( sResult );
        }
        else
        {
            /* create databse 구문 도중 ZKS와 연결이 안될 경우에는 trc log를 남기지 않는다. */
        }
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    mIsConnect = ID_FALSE;

    if( sIsFileOpen == ID_TRUE )
    {
        (void)idlOS::fclose( sFP );
    }

    if( sIsLock == ID_TRUE )
    {
        IDE_ASSERT( mConnectMutex.unlock() == IDE_SUCCESS );
    }
    IDE_POP();
    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::disconnect                   *
 * ------------------------------------------------------------------*
 * ZKS와의 연결을 종료한다.
 * 연결 종료시 connection_info가 제거되면서 다른 노드에게 알람이 간다.
 *********************************************************************/
void sdiZookeeper::disconnect()
{
    if ( mIsConnect == ID_TRUE )
    {
        aclZKC_disconnect( mZKHandler );
    }

    mIsConnect = ID_FALSE;

    mTickTime  = 0;
    mSyncLimit = 0;
    mServerCnt = 0;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getZookeeperMetaLock         *
 * ------------------------------------------------------------------*
 * ZKS의 클러스터 메타를 수정하기 전에 메타의 동시 수정을 막기 위해 일종의 Lock을 잡는다.
 * 해당함수는 ZKS의 클러스터 메타를 수정할때 호출된다.
 *********************************************************************/
IDE_RC sdiZookeeper::getZookeeperMetaLock(UInt aSessionID)
{

    IDE_TEST_RAISE( isConnect() != ID_TRUE, NOT_CONNECTED );

    IDE_TEST( getMetaLock( SDI_ZKS_META_LOCK, aSessionID ) != IDE_SUCCESS );

    idCore::acpAtomicSet32( &mIsGetZookeeperLock,
                            ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( NOT_CONNECTED )
    {
        ideSetErrorCode( sdERR_ABORT_SDI_SHARD_NOT_JOIN );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getShardMetaLock             *
 * ------------------------------------------------------------------*
 * 노드 내부에서 메타를 변경할때 타 노드가 동시에 변경하는 일을 막기 위해 사용하는 lock
 * 해당 함수는 인터페이스 함수로 ZKS 모듈 상위에서 호출된다.
 *
 * aTID     - [IN] 해당 lock을 잡는 transaction의 ID
 *********************************************************************/
IDE_RC sdiZookeeper::getShardMetaLock( UInt aTID )
{
    idBool sIsAlreadyConnected = ID_FALSE;

    /* R2HA to be remove */
    if ( SDU_SHARD_ZOOKEEPER_TEST == 1 )
    {
        if ( sdiZookeeper::isConnect() != ID_TRUE )
        {
            IDE_TEST( sdiZookeeper::connect(ID_FALSE) != IDE_SUCCESS );
        }
        else
        {
            sIsAlreadyConnected = ID_TRUE;
        }
        IDE_TEST_RAISE( mMyNodeName[0] == '\0', ERR_NOT_EXIST_MYNODENAME );
    
        IDE_TEST( getMetaLock( SDI_ZKS_SHARD_LOCK, aTID ) != IDE_SUCCESS );

         idCore::acpAtomicSet32( & mIsGetShardLock,
                            ID_TRUE );
    }
    else
    {
        sIsAlreadyConnected = ID_TRUE;
    }


    return IDE_SUCCESS;
    IDE_EXCEPTION( ERR_NOT_EXIST_MYNODENAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdiZookeeper::getShardMetaLock",
                                  "NOT EXIST MY NODE NAME " ) );
    }
    IDE_EXCEPTION_END;
    if ( sIsAlreadyConnected != ID_TRUE )
    {
        sdiZookeeper::disconnect();
    }
    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getMetaLock                  *
 * ------------------------------------------------------------------*
 * 해당 함수의 수행 방식은 다음과 같다.
 * 1. zookeeper_meta_lock의 자식 노드가 있는지 확인한다.
 * 2. 있을 경우 일정시간 대기 후 1번부터 재수행한다.
 * 3. 없을 경우 자식 노드를 임시 노드로 생성한다.
 * 4. 자식 노드 생성에 성공했을 경우 lock을 완전히 잡은 것으로 판단하고 함수를 종료한다.
 * 5. 자식 노드 생성에 실패했을 경우 다른 client가 먼저 lock을 잡은 것으로 판단하고
 *    일정시간 대기 후 1번부터 재수행한다.
 *
 * aLockType    - [IN]  잡을 lock의 종류(Zookeeper meta lock / Shard meta lock 중 하나)
 * aID         - [IN]  meta lock을 사용할 경우 저장할 ID 
 *********************************************************************/
IDE_RC sdiZookeeper::getMetaLock( ZookeeperLockType aLockType,
                                  UInt              aID )
{
    ZKCResult       sResult;

    String_vector   sChildren;
    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sLockPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar           sBuffer2[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar         * sStrCheck = NULL;
    SInt            sDataLength = SDI_ZKC_BUFFER_SIZE;
    UInt            sWaitingTime = 0;
    idBool          sIsGetLock = ID_FALSE;

    IDE_DASSERT( ( aLockType == SDI_ZKS_META_LOCK ) || ( aLockType == SDI_ZKS_SHARD_LOCK ) );

    if( aLockType == SDI_ZKS_META_LOCK )
    {
        idlOS::strncpy( sLockPath, 
                        SDI_ZKC_PATH_META_LOCK, 
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sNodePath,
                        SDI_ZKC_PATH_META_LOCK, 
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sNodePath,
                        SDI_ZKC_PATH_DISPLAY_LOCK,
                        ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
        /* sLockPath는 '/altibase_shard/cluster_meta/zookeeper_meta_lock' 이고
         * sNodePath는 '/altibase_shard/cluster_meta/zookeeper_meta_lock/locked' 이다. */
        /* ID of zookeeper meta lock may be session id */
    }
    else
    {
        /* ID of shard meta lock is transaction id */
        IDE_DASSERT( aID != SM_NULL_TID );

        idlOS::strncpy( sLockPath,
                        SDI_ZKC_PATH_SHARD_META_LOCK,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sNodePath,
                        SDI_ZKC_PATH_SHARD_META_LOCK, 
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sNodePath,
                        SDI_ZKC_PATH_DISPLAY_LOCK,
                        ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
        /* sLockPath는 '/altibase_shard/cluster_meta/shard_meta_lock' 이고
         * sNodePath는 '/altibase_shard/cluster_meta/shard_meta_lock/locked' 이다. */
    }

    idlOS::snprintf( sBuffer2, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT32_FMT, aID );

    while( sIsGetLock == ID_FALSE )
    {
        do{
            /* 1. meta lock의 자식 노드가 있는지 확인한다. */
            sResult = aclZKC_getChildren( mZKHandler,
                                          sLockPath,
                                          &sChildren );

            IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

            /* 2. 자식 노드가 있을 경우 일정시간 대기 후 1번 부터 재수행한다. */
            if( sChildren.count > 0 )
            {
                idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
                sDataLength = SDI_ZKC_BUFFER_SIZE;
                sResult = aclZKC_getInfo( mZKHandler,
                                            sNodePath,
                                            sBuffer,
                                            &sDataLength );

                /* 자식을 체크하려고 읽는 사이 lock이 풀릴 경우 */
                if( sResult == ZKC_NO_NODE )
                {
                    continue;
                }
                else
                {
                    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
                }

                /* 가져온 data는 [노드이름]/[ID]의 형태이므로 분리하여 체크한다. */
                sStrCheck = idlOS::strtok( sBuffer, "/" );

                /* 노드 이름이 같은지 체크 */
                if( idlOS::strncmp( sStrCheck,
                                    mMyNodeName, 
                                    SDI_NODE_NAME_MAX_SIZE ) == 0 )
                {
                    sStrCheck = idlOS::strtok( NULL, "\0" );

                    /* ID가 동일한지 체크 */
                    if( idlOS::strncmp( sStrCheck,
                                        sBuffer2,
                                        idlOS::strlen( sBuffer2 ) ) == 0 )
                    {
                        /* 둘다 동일하다면 이미 lock을 잡은것으로 취급한다. */
                        IDE_CONT( normal_exit );
                    }
                } 
                idlOS::sleep( 1 );
                sWaitingTime++;

                IDE_TEST_RAISE( sWaitingTime > ZOOKEEPER_LOCK_WAIT_TIMEOUT, err_waitingTime );
            }

        }while( sChildren.count > 0 );

        zookeeperLockDump((SChar*)"get meta lock begin", aLockType, ID_FALSE);
        /* 3. 자식 노드가 없을 경우 자식 노드를 임시 노드로 생성한다. */
        /* lock의 경우 [노드이름]/[ID]의 형태로 저장한다. */
        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        idlOS::strncpy( sBuffer,
                        mMyNodeName,
                        SDI_ZKC_BUFFER_SIZE );
        idlOS::strncat( sBuffer,
                        "/",
                        ID_SIZEOF( sBuffer ) - idlOS::strlen( sBuffer ) - 1 );
        idlOS::strncat( sBuffer,
                        sBuffer2,
                        ID_SIZEOF( sBuffer ) - idlOS::strlen( sBuffer ) - 1 );


        sResult = aclZKC_insertNode( mZKHandler,
                                     sNodePath,
                                     sBuffer,
                                     idlOS::strlen( sBuffer ),
                                     ACP_TRUE );
        if( sResult == ZKC_SUCCESS )
        {
            /* 4.1 성공 했다면 정상 성공인지 다시 체크한다. *
                  ( 성능을 위해서라면 없에도 되는 부분 )    */

            /* 자식 수가 1개인지 체크 */
            sResult = aclZKC_getChildren( mZKHandler,
                                          sLockPath,
                                          &sChildren );

            IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

            IDE_TEST_RAISE( sChildren.count != 1, err_lockCurrupted );

            /* 자식을 생성한게 나인지 다시 체크 */
            sDataLength = SDI_ZKC_BUFFER_SIZE;
            sResult = aclZKC_getInfo( mZKHandler,
                                      sNodePath,
                                      sBuffer2,
                                      &sDataLength );

            IDE_TEST_RAISE( idlOS::strncmp( sBuffer2, sBuffer, sDataLength ) != 0, err_lockCurrupted )

            sIsGetLock = ID_TRUE;
            break;
        }
        else
        {
            /* 4.2 실패할 경우 다른 노드가 먼저 자식 노드를 생성해 lock을 잡은 것이므로
             *    일정시간 대기 후 1번부터 재수행한다.*/
            idlOS::sleep( 1 );
            sWaitingTime++;

            IDE_TEST_RAISE( sWaitingTime > ZOOKEEPER_LOCK_WAIT_TIMEOUT, err_waitingTime );
        }
    }
    zookeeperLockDump((SChar*)"get meta lock end success", aLockType, ID_FALSE);
    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION( err_waitingTime )
    {
        logAndSetErrMsg( ZKC_WAITING_TIMEOUT );        
    }
    IDE_EXCEPTION( err_lockCurrupted )
    {
        logAndSetErrMsg( ZKC_LOCK_ERROR );
    }
    IDE_EXCEPTION_END;

    zookeeperLockDump((SChar*)"get meta lock end failure", aLockType, ID_FALSE);
    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::releaseZookeeperMetaLock     *
 * ------------------------------------------------------------------*
 * getZookeeperMetaLock() 함수에서 잡은 lock을 해제한다.
 *********************************************************************/
void sdiZookeeper::releaseZookeeperMetaLock()
{
    idBool sIsGetZookeeperLock = ID_FALSE;

    sIsGetZookeeperLock = (idBool)idCore::acpAtomicGet32( & mIsGetZookeeperLock );
    if( ( sIsGetZookeeperLock == ID_TRUE ) && ( mIsConnect == ID_TRUE ) )
    {
        idCore::acpAtomicSet( &mIsGetZookeeperLock,
                              ID_FALSE );

        (void)releaseMetaLock( SDI_ZKS_META_LOCK );
    }

    /* 정상적으로 commit후 작업을 완료한 후 뿐 아니라 예외처리로 취소한 경우에도 *
     * RunningJob을 초기화시켜 다음 작업과 충돌하지 않도록 한다.                 */
    mRunningJobType = ZK_JOB_NONE;
    idlOS::memset( mTargetNodeName, 0, SDI_NODE_NAME_MAX_SIZE + 1 );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::releaseShardMetaLock         *
 * ------------------------------------------------------------------*
 * getShardMetaLock() 함수에서 잡은 lock을 해제한다.
 *********************************************************************/
void sdiZookeeper::releaseShardMetaLock()
{
    idBool sIsGetShardLock = ID_TRUE;
    /* R2HA to be remove */
    if ( SDU_SHARD_ZOOKEEPER_TEST == 1 )
    {
        sIsGetShardLock = (idBool)idCore::acpAtomicGet32( &mIsGetShardLock );

        if ( SDU_SHARD_LOCAL_FORCE != 1)
        {
            if( ( sIsGetShardLock == ID_TRUE ) && ( mIsConnect == ID_TRUE ) )
            {
                idCore::acpAtomicSet32( &mIsGetShardLock,
                                        ID_FALSE );

                (void)releaseMetaLock( SDI_ZKS_SHARD_LOCK );
            }
        }
    }
    
    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::releaseMetaLock              *
 * ------------------------------------------------------------------*
 * getMetaLock 함수에서 잡은 lock을 해제한다.
 * 해당 함수로 인해 임시 노드가 제거되나
 * watch 대상이 아니므로 알람이 가지 않는다.
 *********************************************************************/
IDE_RC sdiZookeeper::releaseMetaLock( ZookeeperLockType aLockType )
{
    ZKCResult   sResult;
    SChar       sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt        sDataLength = SDI_ZKC_BUFFER_SIZE;
    SChar     * sOwnerCheck = NULL;

    zookeeperLockDump((SChar*)"release lock begin", aLockType, ID_FALSE);

    IDE_DASSERT( ( aLockType == SDI_ZKS_META_LOCK ) || ( aLockType == SDI_ZKS_SHARD_LOCK ) );

    if( aLockType == SDI_ZKS_META_LOCK )
    {
        idlOS::strncpy( sPath, 
                        SDI_ZKC_PATH_META_LOCK,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sPath, 
                        SDI_ZKC_PATH_DISPLAY_LOCK,
                        ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
        /* sLockPath는 '/altibase_shard/cluster_meta/zookeeper_meta_lock' 이고
         * sNodePath는 '/altibase_shard/cluster_meta/zookeeper_meta_lock/locked' 이다. */
    }
    else
    {
        idlOS::strncpy( sPath,
                        SDI_ZKC_PATH_SHARD_META_LOCK,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sPath,
                        SDI_ZKC_PATH_DISPLAY_LOCK,
                        ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
        /* 'sLockPath는 /altibase_shard/cluster_meta/shard_meta_lock' 이고
         * 'sNodePath는 /altibase_shard/cluster_meta/shard_meta_lock/locked' 이다. */
    }
    /* 해제할 lock이 자신이 잡은 것이 맞는지 확인한다. */
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              &sDataLength );
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    sOwnerCheck = idlOS::strtok( sBuffer, "/" );
    IDE_TEST_RAISE( sOwnerCheck == NULL , err_NotMyLock );
    IDE_TEST_RAISE( idlOS::strncmp( sOwnerCheck, mMyNodeName, sDataLength ) != 0, err_NotMyLock );

    /* 임시 노드를 제거하여 lock을 해제한다. */
    sResult = aclZKC_deleteNode( mZKHandler, sPath );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    zookeeperLockDump((SChar*)"release lock end success", aLockType, ID_FALSE);
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION( err_NotMyLock )
    {
        logAndSetErrMsg( ZKC_LOCK_ERROR );
    }
    IDE_EXCEPTION_END;
    zookeeperLockDump((SChar*)"release lock end failure", aLockType, ID_FALSE);
    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::validateDB                   *
 * ------------------------------------------------------------------*
 * 현재 생성하려고하는 DB와 char set이 ZKS에 저장된 것과 동일한지 확인한다.
 * 하나라도 일치하지 않을 경우 aIsValid가 ID_FALSE를 리턴한다.
 * 해당 함수는 DB 생성시에도 호출되므로 ZKS에 연결되지 않은 상황에서도 호출될 수 있다.
 *
 * aDBName               - [IN]  새로 생성할 DB의 이름
 * aCharacterSet         - [IN]  새로 생성할 DB의 character set
 * aNationalCharacterSet - [IN] 새로 생성할 DB의 national character set
 *********************************************************************/
IDE_RC sdiZookeeper::validateDB( SChar  * aDBName,
                                 SChar  * aCharacterSet,
                                 SChar  * aNationalCharacterSet )
{
    ZKCResult sResult;

    SChar     sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar     sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt      sDataLength = SDI_ZKC_BUFFER_SIZE;
    idBool    sIsConnect = ID_FALSE;

    /* 해당함수는 create Database 도중 호출될 수 있으므로 ZKS에 연결되지 않은 상태에서도 호출될 수 있다. *
     * 만약  ZKS에 연결되어 있지 않다면 임시로 연결시켜준다.                                             */
    if( mIsConnect == ID_FALSE )
    {
        if( connect( ID_TRUE ) != IDE_SUCCESS )
        {
            /* 연결이 실패할 경우 샤딩과 관계 없으므로 valid 처리한다. */
            IDE_CONT( normal_exit );
        }
        else
        {
            sIsConnect = ID_TRUE;
        }
    }

    /* 1. DB이름을 비교한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_DB_NAME,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/validation/sharded_database_name'의 값을 가진다. */

    IDE_TEST_CONT( mZKHandler == NULL, normal_exit );

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    /* 결과가 NO_NODE라면 클러스터 메타가 아직 없는 상황이므로 비교할 필요가 없다. */
    IDE_TEST_CONT( sResult == ZKC_NO_NODE, normal_exit );

    /* 이 경우에는 추가적인 에러메시지를 남기고 실패처리한다. */
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 정상적으로 가져왔다면 값을 비교한다. */
    IDE_TEST_RAISE( idlOS::strncmp( sBuffer,
                                    aDBName,
                                    idlOS::strlen( aDBName ) ) != 0,
                    validate_Fail );

    /* 2. character set을 비교한다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath, 
                    SDI_ZKC_PATH_CHAR_SET, 
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/validation/character_set'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 정상적으로 가져왔다면 값을 비교한다. */
    IDE_TEST_RAISE( idlOS::strncmp( sBuffer,
                                    aCharacterSet,
                                    idlOS::strlen( aCharacterSet ) ) != 0, 
                    validate_Fail );

    /* 3. national character set을 비교한다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NAT_CHAR_SET,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/validation/national_character_set'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 정상적으로 가져왔다면 값을 비교한다. */
    IDE_TEST_RAISE( idlOS::strncmp( sBuffer,
                                    aNationalCharacterSet,
                                    idlOS::strlen( aNationalCharacterSet ) ) != 0,
                    validate_Fail );

    /* 임시연결일 경우 연결을 해제한다. */
    if( ( mIsConnect == ID_FALSE ) || ( sIsConnect == ID_TRUE ) )
    {
        disconnect();
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( validate_Fail )
    {
        logAndSetErrMsg( ZKC_DB_VALIDATE_FAIL );
    }
    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    /* 임시연결일 경우 연결을 해제한다. */
    if( mIsConnect == ID_FALSE )
    {
        disconnect();
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::validateNode                 *
 * ------------------------------------------------------------------*
 * 클러스터에 추가되는 노드의 메타정보가 ZKS에 저장된 것과 일치하는지 확인한다.
 * 하나라도 일치하지 않을 경우 예외 처리로 튕겨 나온다.
 * 추가로, 삽입 예정인 노드와 동일한 이름의 노드가 이미 있는지도 체크한다.
 * 해당함수는 ZKS와 연결된 이후에 사용이 가능하다.
 *
 * aDBName                   - [IN]  DB의 이름
 * aCharacterSet             - [IN]  DB의 character set
 * aNationalCharacterSet     - [IN]  DB의 national character set
 * aKSafety                  - [IN]  k-safety 값
 * aReplicationMode          - [IN]  replication mode(10,11,12 중 하나)
 * aParallelCount            - [IN]  이중화 복제를 처리하는 applier의 수
 * aBinaryVersion            - [IN]  binary version(sm version)
 * aShardVersion             - [IN]  shard version
 * aTransTBLSize             - [IN]  트랜잭션 테이블의 크기
 * aNodeName                 - [IN]  삽입 예정인 노드의 이름
 *********************************************************************/
IDE_RC sdiZookeeper::validateNode( SChar  * aDBName,
                                   SChar  * aCharacterSet,
                                   SChar  * aNationalCharacterSet,
                                   SChar  * aKSafety,
                                   SChar  * aReplicationMode,
                                   SChar  * aParallelCount,
                                   SChar  * aBinaryVersion,
                                   SChar  * aShardVersion,
                                   SChar  * aTransTBLSize,
                                   SChar  * aNodeName )
{
    ZKCResult sResult;

    SChar     sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar     sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt      sDataLength;
    const UInt sErrMsgBufferLen = 1024;
    SChar      sErrMsgBuffer[sErrMsgBufferLen] = {0,};

    /* 1. DB이름을 비교한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_DB_NAME,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/validation/sharded_database_name'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    if( sResult != ZKC_SUCCESS )
    {
        if( sResult == ZKC_NO_NODE )
        {
            /* 이 경우에는 클러스터 메타가 아직 없는 상황이므로 비교할 필요가 없다. */
            IDE_CONT( normal_exit );
        }
        else
        {
            IDE_RAISE( err_ZKC );
        }
    }

    /* 정상적으로 가져왔다면 비교한다. */
    if( idlOS::strncmp( sBuffer, aDBName, idlOS::strlen( aDBName ) ) != 0 )
    {
        idlOS::snprintf(sErrMsgBuffer,
                        sErrMsgBufferLen,
                        "local sharded database name[%s] is different with cluster's[%s]",
                        aDBName,
                        sBuffer);
        IDE_RAISE( validate_fail );
    }

    /* 2. character set을 비교한다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_CHAR_SET,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/validation/character_set'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 정상적으로 가져왔다면 비교한다. */
    if( idlOS::strncmp( sBuffer, aCharacterSet, idlOS::strlen( aCharacterSet ) ) != 0 )
    {
        idlOS::snprintf(sErrMsgBuffer,
                        sErrMsgBufferLen,
                        "local database character set[%s] is different with cluster's[%s]",
                        aCharacterSet,
                        sBuffer);
        IDE_RAISE( validate_fail );
    }

    /* 3. national character set을 비교한다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NAT_CHAR_SET,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/validation/national_character_set'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 정상적으로 가져왔다면 비교한다. */
    if( idlOS::strncmp( sBuffer, aNationalCharacterSet, idlOS::strlen( aNationalCharacterSet ) ) != 0 )
    {
        idlOS::snprintf(sErrMsgBuffer,
                        sErrMsgBufferLen,
                        "local database national character set[%s] is different with cluster's[%s]",
                        aNationalCharacterSet,
                        sBuffer);
        IDE_RAISE( validate_fail );
    }

    /* 4. K-safety를 비교한다. */
    if( aKSafety != NULL )
    {
        idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sPath,
                        SDI_ZKC_PATH_K_SAFETY,
                        SDI_ZKC_PATH_LENGTH );
        /* sPath는 '/altibase_shard/cluster_meta/validation/k-safety'의 값을 가진다. */

        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        /* 정상적으로 가져왔다면 비교한다. */
        if( idlOS::strncmp( sBuffer, aKSafety, idlOS::strlen( aKSafety ) ) != 0 )
        {
            idlOS::snprintf(sErrMsgBuffer,
                            sErrMsgBufferLen,
                            "local database k-safety[%s] is different with cluster's[%s]",
                            aKSafety,
                            sBuffer);
            IDE_RAISE( validate_fail );
        }
    }
    else
    {
        /* replication 정보( k-safety, replication mode, parallel count )가 비어있는 경우    *
         * 정상적으로 validation을 성공처리 한 후 값을 받아 내 노드의 메타에 삽입해야 한다.  */
    }

    /* 5. replication_mode를 비교한다. */
    if( aReplicationMode != NULL )
    {
        idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sPath,
                        SDI_ZKC_PATH_REP_MODE,
                        SDI_ZKC_PATH_LENGTH );
        /* sPath는 '/altibase_shard/cluster_meta/validation/replication_mode'의 값을 가진다. */

        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        /* 정상적으로 가져왔다면 비교한다. */
        if( idlOS::strncmp( sBuffer, aReplicationMode, idlOS::strlen( aReplicationMode ) ) != 0 )
        {
            idlOS::snprintf(sErrMsgBuffer,
                            sErrMsgBufferLen,
                            "local database replication mode[%s] is different with cluster's[%s]",
                            aReplicationMode,
                            sBuffer);
            IDE_RAISE( validate_fail );
        }
    }
    else
    {
        /* replication 정보( k-safety, replication mode, parallel count )가 비어있는 경우    *
         * 정상적으로 validation을 성공처리 한 후 값을 받아 내 노드의 메타에 삽입해야 한다.  */
    }

    /* 6. parallel count를 비교한다. */
    if( aParallelCount != NULL )
    {
        idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sPath,
                        SDI_ZKC_PATH_PARALLEL_CNT,
                        SDI_ZKC_PATH_LENGTH );
        /* sPath는 '/altibase_shard/cluster_meta/validation/parallel_count'의 값을 가진다. */

        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler, 
                                  sPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        /* 정상적으로 가져왔다면 비교한다. */
        if( idlOS::strncmp( sBuffer, aParallelCount, idlOS::strlen( aParallelCount ) ) != 0 )
        {
            idlOS::snprintf(sErrMsgBuffer,
                            sErrMsgBufferLen,
                            "local database replication parallel count[%s] is different with cluster's[%s]",
                            aParallelCount,
                            sBuffer);
            IDE_RAISE( validate_fail );
        }
    }
    else
    {
        /* replication 정보( k-safety, replication mode, parallel count )가 비어있는 경우    *
         * 정상적으로 validation을 성공처리 한 후 값을 받아 내 노드의 메타에 삽입해야 한다.  */
    }

    /* 7. binary version을 비교한다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath, 
                    SDI_ZKC_PATH_BINARY_VERSION,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/validation/binary_version'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 정상적으로 가져왔다면 비교한다. */
    if( idlOS::strncmp( sBuffer, aBinaryVersion, idlOS::strlen( aBinaryVersion ) ) != 0 )
    {
        idlOS::snprintf(sErrMsgBuffer,
                        sErrMsgBufferLen,
                        "local database binary version[%s] is different with cluster's[%s]",
                        aBinaryVersion,
                        sBuffer);
        IDE_RAISE( validate_fail );
    }

    /* 8. shard version을 비교한다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_SHARD_VERSION,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/validation/shard_version'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 정상적으로 가져왔다면 비교한다. */
    if( idlOS::strncmp( sBuffer, aShardVersion, idlOS::strlen( aShardVersion ) ) != 0 )
    {
        idlOS::snprintf(sErrMsgBuffer,
                        sErrMsgBufferLen,
                        "local database shard version[%s] is different with cluster's[%s]",
                        aShardVersion,
                        sBuffer);
        IDE_RAISE( validate_fail );
    }

    /* 9. trans TBL size를 비교한다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_TRANS_TBL_SIZE,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/validation/trans_TBL_size'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 정상적으로 가져왔다면 비교한다. */
    if( idlOS::strncmp( sBuffer, aTransTBLSize, idlOS::strlen( aTransTBLSize ) ) != 0 )
    {
        idlOS::snprintf(sErrMsgBuffer,
                        sErrMsgBufferLen,
                        "local database transaction table size[%s] is different with cluster's[%s]",
                        aTransTBLSize,
                        sBuffer);
        IDE_RAISE( validate_fail );
    }

    /* 10. 삽입 예정인 노드와 동일한 이름의 노드가 있는지 체크한다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[aNodeName]/state'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              &sDataLength );

    if( sResult == ZKC_SUCCESS )
    {
        if( ( stateStringConvert( sBuffer ) ) == ZK_ADD )
        {
            /* 이미 동일한 노드가 존재하고 해당 노드의 상태가 ADD라면 이전 ADD 도중 실패한 것이므로 * 
             * 삽입 과정에서 기존 내역을 지우고 진행한다. 일단 여기는 통과처리 한다.                */
        }
        else
        {
            /* 이미 동일한 이름의 노드가 존재하고 정상 적동 중 */
            IDE_RAISE( err_alreadyExist );
        }
    }
    else
    {
        IDE_TEST_RAISE( sResult != ZKC_NO_NODE, err_ZKC );
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( validate_fail )
    {
        logAndSetErrMsg( ZKC_VALIDATE_FAIL, sErrMsgBuffer );
    }
    IDE_EXCEPTION( err_alreadyExist )
    {
        logAndSetErrMsg( ZKC_NODE_EXISTS );
    }
    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getKSafetyInfo( SChar  * aKSafety,
                                     SChar  * aReplicationMode,
                                     SChar  * aParallelCount )
{
    ZKCResult sResult;
    SChar     sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar     sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt      sDataLength;

    IDE_DASSERT( aKSafety != NULL );
    /* sPath는 '/altibase_shard/cluster_meta/validation/k-safety'의 값을 가진다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_K_SAFETY,
                    SDI_ZKC_PATH_LENGTH );

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                                sPath,
                                sBuffer,
                                &sDataLength );
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    idlOS::strncpy( aKSafety, sBuffer, sDataLength );

    IDE_DASSERT( aReplicationMode != NULL );
    /* sPath는 '/altibase_shard/cluster_meta/validation/replication_mode'의 값을 가진다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_REP_MODE,
                    SDI_ZKC_PATH_LENGTH );

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                                sPath,
                                sBuffer,
                                &sDataLength );
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    idlOS::strncpy( aReplicationMode, sBuffer, sDataLength );

    IDE_DASSERT( aParallelCount != NULL );
    /* sPath는 '/altibase_shard/cluster_meta/validation/parallel_count'의 값을 가진다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_PARALLEL_CNT,
                    SDI_ZKC_PATH_LENGTH );

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    idlOS::strncpy( aParallelCount, sBuffer, sDataLength );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::validateState                *
 * ------------------------------------------------------------------*
 * 특정 노드에 대해 상태 변경을 수행하려고 할 때
 * 해당 노드의 현재 상태와 비교하여 가능한 상태변경인지 체크한다.
 * 해당 함수는 ZKS와 연결된 이후에 사용이 가능하다.
 * 해당 함수는 자신 외의 다른 노드에 대해서도 수행할 수 있다.
 *
 * aState    - [IN]  변경하려는 상태
 * aNodeName - [IN]  상태를 변경하려는 노드의 이름
 *********************************************************************/
IDE_RC sdiZookeeper::validateState( ZKState   aState, 
                                    SChar   * aNodeName )
{
    ZKCResult sResult;

    SChar     sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar     sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt      sDataLength;

    ZKState   sOldState;
    idBool    sIsAlive;

    if( ( aState == ZK_NONE ) || ( aState == ZK_ADD ) )
    {
        /* 변경하려는 상태가 ADD와 DROP일 경우에는 체크할 필요가 없다.  */
        IDE_CONT( normal_exit );
    }

    /* 1. 대상 노드의 state를 얻어온다. */
    idlOS::strncpy( sPath, 
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[aNodeName]'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              &sDataLength );
                              
    if( sResult != ZKC_SUCCESS )
    {
        /* 대상 노드가 존재하지 않을 경우 */
        if( sResult == ZKC_NO_NODE )
        {
            IDE_CONT( normal_exit );
        }
        else
        {
            IDE_RAISE( err_ZKC );
        }
    }

    sOldState = stateStringConvert( sBuffer );

    /* 2. 대상 노드의 connection_info를 얻어온다. */
    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              &sDataLength );
    /* sPath는 '/altibase_shard/connection_info/[aNodeName]'의 값을 가진다. */

    switch( sResult )
    {
        case ZKC_SUCCESS:
            sIsAlive = ID_TRUE;
            break;
        case ZKC_NO_NODE:
            sIsAlive = ID_FALSE;
            break;
        default:
            IDE_RAISE( err_ZKC );
            break;
    }
    
    /* 3. 대상 노드의 state와 connection_info를 조합해 정상적인 상태전이인지 체크한다. */
    if( sOldState == ZK_ADD ) 
    {
        /* 기존 상태가 ADD인 경우 DROP외에는 RUN으로의 변경만 가능하다. */
        IDE_TEST_RAISE( aState != ZK_RUN, validate_Fail );
    }
    else if( ( sOldState == ZK_RUN ) && ( sIsAlive == ID_TRUE ) )
    {
        /* 기존 상태가 RUN(alive)일 경우 FAILOVER, SHUTDOWN만 가능하다. */
        IDE_TEST_RAISE( ( aState != ZK_FAILOVER ) && 
                        ( aState != ZK_SHUTDOWN ) &&
                        ( aState != ZK_KILL ) , validate_Fail );
    }
    else if( ( sOldState == ZK_RUN ) && ( sIsAlive == ID_FALSE ) )
    {
        /* 기존 상태가 RUN(die)일 경우에는 RUN/FAILBACK/KILL만 가능하다. */        
        IDE_TEST_RAISE( ( aState != ZK_RUN ) &&
                        ( aState != ZK_FAILBACK ) &&
                        ( aState != ZK_KILL ), validate_Fail );
    }
    else if( sOldState == ZK_FAILBACK )
    {
        /* 기존 상태가 FAILBACK일 경우에는 RUN/FAILOVER/FAILBACK만 가능하다. */
        IDE_TEST_RAISE( ( aState != ZK_RUN ) && ( aState != ZK_FAILOVER ) && ( aState != ZK_FAILBACK ), validate_Fail );
    }
    else if( sOldState == ZK_FAILOVER )
    {
        /* 기존 상태가 FAILOVER일 경우에는 RUN/FAILBACK만 가능하다. */
        IDE_TEST_RAISE( ( aState != ZK_RUN ) && ( aState != ZK_FAILBACK ), validate_Fail );
    }
    else if( sOldState == ZK_SHUTDOWN )
    {
        /* 기존 상태가 SHUTDOWN일 경우에는 JOIN 및 failover만 가능하다. */
        IDE_TEST_RAISE( ( aState != ZK_JOIN ) && ( aState != ZK_FAILOVER ), validate_Fail );
    }
    else if( sOldState == ZK_JOIN )
    {
        /* 기존 상태가 JOIN일 경우에는 RUN만 가능하다. */
        IDE_TEST_RAISE( aState != ZK_RUN, validate_Fail );
    }
    else if( sOldState == ZK_KILL )
    {
        /* 기존 상태가 KILL일 경우에는 RUN/FAILBACK만 가능하다. */
        IDE_TEST_RAISE( ( aState != ZK_RUN ) && ( aState != ZK_FAILBACK ), validate_Fail );
    }
    else
    {
        /* 예외처리 후 assert */
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( validate_Fail )
    {
        logAndSetErrMsg( ZKC_STATE_VALIDATE_FAIL );
    }
    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::initClusterMeta              *
 * ------------------------------------------------------------------*
 * 클러스터 메타가 비어있을때 클러스터 메타를 생성한다.
 * 해당 함수는 공통으로 사용하는 /altibase_shard/cluster_meta 이하의 정보만 생성한다.
 * 해당 함수는 multi op를 이용해 cluster meta를 생성한다.
 *
 * aShardDBInfo     - [IN]  생성할 클러스터 메타의 정보
 * aLocalMetaInfo   - [IN]  생성할 클러스터 메타의 정보 중 replication 관련 정보
 *********************************************************************/
IDE_RC sdiZookeeper::initClusterMeta( sdiDatabaseInfo   aShardDBInfo,
                                      sdiLocalMetaInfo aLocalMetaInfo)
/* [BUGBUG] aLocalMetaInfo에서 replication 정보를 가져와 사용하나 PROJ-2726 진행 당시에는                 *
   aLocalMetaInfo에 아직 replication 정보가 없기 때문에 compiler waring을 막기 위해 현재와 같이 처리한다. */
{
    ZKCResult   sResult;

    SChar       sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt        sDataLength;
    SChar       sShardVersion[10] = {0,};
    SChar       sTransTBLSize[10] = {0,};
    SChar               sKSafetyStr[SDI_KSAFETY_STR_MAX_SIZE + 1] = {0,};
    SChar               sReplicationModeStr[SDI_REPLICATION_MODE_MAX_SIZE + 1] = {0,};
    SChar               sParallelCountStr[SDI_PARALLEL_COUNT_STR_MAX_SIZE + 1] = {0,};

    /* multi op */
    SInt            i = 0;
    zoo_op_t        sOp[19];
    zoo_op_result_t sOpResult[19];

    /* cluster meta를 생성하기 전 다른 client에 의해 생성 되었을 수 있으니 한번 더 확인한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_META_LOCK,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/zookeeper_meta_lock'의 값을 가진다. '*/

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler, 
                              sPath,
                              sBuffer,
                              &sDataLength );

    if( sResult == ZKC_SUCCESS )
    {
        /* 이 경우는 다른 client에 의해 cluster meta가 생성된 상태이므로 생성 할 필요가 없다. */
        IDE_CONT( normal_exit );
    }
    
    /* cluster meta를 한번에 생성하기 위해 multi op로 처리할 operation을 생성한다. */
    zoo_create_op_init( &sOp[0],                      /* operation */
                        SDI_ZKC_PATH_ALTIBASE_SHARD,  /* path */
                        NULL,                         /* value */
                        -1,                           /* value length */
                        &ZOO_OPEN_ACL_UNSAFE,         /* ACL */
                        ZOO_PERSISTENT,               /* flag */
                        NULL,                         /* path_buffer */
                        0 );                          /* path_buffer_len */
    /* sOp[0]은 '/altibase_shard'의 path를 생성한다. */

    zoo_create_op_init( &sOp[1],
                        SDI_ZKC_PATH_CLUSTER_META,
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[1]은 '/altibase_shard/cluster_meta'의 path를 생성한다. */

    zoo_create_op_init( &sOp[2],
                        SDI_ZKC_PATH_META_LOCK,
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[2]은 '/altibase_shard/cluster_meta/zookeeper_meta_lock'의 path를 생성한다. */

    zoo_create_op_init( &sOp[3],
                        SDI_ZKC_PATH_VALIDATION,
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[3]은 '/altibase_shard/cluster_meta/validation'의 path를 생성한다. */

    zoo_create_op_init( &sOp[4],
                        SDI_ZKC_PATH_DB_NAME,
                        aShardDBInfo.mDBName,   /* DB 생성시 입력한 DB의 이름 */
                        idlOS::strlen( aShardDBInfo.mDBName ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[4]은 '/altibase_shard/cluster_meta/validation/sharded_database_name'의 path를 생성한다. */

    IDE_TEST_RAISE( idlOS::uIntToStr( aLocalMetaInfo.mKSafety,
                                      sKSafetyStr,
                                      SDI_KSAFETY_STR_MAX_SIZE + 1 ) != 0, ERR_INVALID_KSAFETY );
    zoo_create_op_init( &sOp[5],
                        SDI_ZKC_PATH_K_SAFETY,
                        sKSafetyStr,   /* 복사본의 수 */
                        idlOS::strlen( sKSafetyStr ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[5]은 '/altibase_shard/cluster_meta/validation/k-safety'의 path를 생성한다. */

    IDE_TEST_RAISE( idlOS::uIntToStr( aLocalMetaInfo.mReplicationMode,
                                      sReplicationModeStr,
                                      SDI_REPLICATION_MODE_MAX_SIZE + 1) != 0, ERR_INVALID_KSAFETY );
    zoo_create_op_init( &sOp[6],
                        SDI_ZKC_PATH_REP_MODE,
                        sReplicationModeStr,/* replication mode */
                        idlOS::strlen( sReplicationModeStr ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[6]은 '/altibase_shard/cluster_meta/validation/replication_mode'의 path를 생성한다. */

    IDE_TEST_RAISE( idlOS::uIntToStr( aLocalMetaInfo.mParallelCount,
                                      sParallelCountStr,
                                      SDI_PARALLEL_COUNT_STR_MAX_SIZE + 1) != 0, ERR_INVALID_KSAFETY );
    zoo_create_op_init( &sOp[7],
                        SDI_ZKC_PATH_PARALLEL_CNT,
                        sParallelCountStr,   /* parallel count */
                        idlOS::strlen( sParallelCountStr ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[7]은 '/altibase_shard/cluster_meta/validation/parallel_count'의 path를 생성한다. */

    zoo_create_op_init( &sOp[8],
                        SDI_ZKC_PATH_CHAR_SET,
                        aShardDBInfo.mDBCharSet,   /* character set */
                        idlOS::strlen( aShardDBInfo.mDBCharSet ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[8]은 '/altibase_shard/cluster_meta/validation/character_set'의 path를 생성한다. */

    zoo_create_op_init( &sOp[9],
                        SDI_ZKC_PATH_NAT_CHAR_SET,
                        aShardDBInfo.mNationalCharSet,   /* national character set */
                        idlOS::strlen( aShardDBInfo.mNationalCharSet ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[9]은 '/altibase_shard/cluster_meta/validation/national_character_set'의 path를 생성한다. */

    zoo_create_op_init( &sOp[10],
                        SDI_ZKC_PATH_BINARY_VERSION,
                        aShardDBInfo.mDBVersion,   /* binary version */
                        idlOS::strlen( aShardDBInfo.mDBVersion ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[10]은 '/altibase_shard/cluster_meta/validation/binary_version'의 path를 생성한다. */

    idlOS::snprintf( sShardVersion, 10, "%"ID_INT32_FMT".%"ID_INT32_FMT".%"ID_INT32_FMT,
                    aShardDBInfo.mMajorVersion, aShardDBInfo.mMinorVersion, aShardDBInfo.mPatchVersion );
    zoo_create_op_init( &sOp[11],
                        SDI_ZKC_PATH_SHARD_VERSION,
                        sShardVersion,   /* shard version */
                        idlOS::strlen( sShardVersion ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[11]은 '/altibase_shard/cluster_meta/validation/shard_version'의 path를 생성한다. */

    idlOS::snprintf( sTransTBLSize, 10, "%"ID_UINT32_FMT, aShardDBInfo.mTransTblSize );
    zoo_create_op_init( &sOp[12],
                        SDI_ZKC_PATH_TRANS_TBL_SIZE,
                        sTransTBLSize,  /* trans TBL size */
                        idlOS::strlen( sTransTBLSize ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[12]은 '/altibase_shard/cluster_meta/validation/trans_TBL_size'의 path를 생성한다. */

    zoo_create_op_init( &sOp[13],
                        SDI_ZKC_PATH_FAILOVER_HISTORY,
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[13]은 '/altibase_shard/cluster_meta/failover_history'의 path를 생성한다. */

    zoo_create_op_init( &sOp[14],
                        SDI_ZKC_PATH_SMN,
                        NULL,   /* 클러스터 메타 생성시에는 SMN도 비어있다. */
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[14]은 '/altibase_shard/cluster_meta/validation/SMN'의 path를 생성한다. */

    zoo_create_op_init( &sOp[15],
                        SDI_ZKC_PATH_FAULT_DETECT_TIME,
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[15]은 '/altibase_shard/cluster_meta/validation/fault_detection_time'의 path를 생성한다. */

    zoo_create_op_init( &sOp[16],
                        SDI_ZKC_PATH_SHARD_META_LOCK,
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[16]은 '/altibase_shard/cluster_meta/validation/shard_meta_lock'의 path를 생성한다. */

    zoo_create_op_init( &sOp[17],
                        SDI_ZKC_PATH_CONNECTION_INFO,
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[17]은 '/altibase_shard/cluster_meta/connection_info'의 path를 생성한다. */

    zoo_create_op_init( &sOp[18],
                        SDI_ZKC_PATH_NODE_META,
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );
    /* sOp[18]은 '/altibase_shard/node_meta'의 path를 생성한다. */

    /* 생성된 operation들을 multi op로 한번에 수행한다. */
    sResult = aclZKC_multiOp( mZKHandler, 19, sOp, sOpResult );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for( i = 0; i <= 18; i++ )
    {
        IDE_TEST_RAISE( sOpResult[i].err != ZOK, err_ZKC );
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_KSAFETY )
    {
        ideSetErrorCode( sdERR_ABORT_SDI_INVALID_KSAFETY );
    }
    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::finalizeClusterMeta()
{
    IDE_TEST( removeRecursivePathAndInfo((SChar*)"/altibase_shard") != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::addNode                      *
 * ------------------------------------------------------------------*
 * 노드가 추가될때 노드 메타에 해당 노드에 대한 정보를 추가한다.
 * 해당 함수는 /altibase_shard/node_meta/노드이름 이하의 정보를 생성한다.
 * 만약 클러스터 메타가 비어있을 경우에는 실패처리한다.
 * 해당 함수를 수행하기 전 validateNode 함수를 먼저 수행해 add가 가능한지 체크해야 한다.
 * 해당 함수에서는 SMN과 state는 변경하지 않으므로 이 둘은
 * 따로 updateSMN과 updateState를 호출하여 처리해야 한다.
 *
 * aShardNodeID         - [IN] 추가할 노드의 식별자 ID
 * aNodeIPnPort         - [IN] 추가할 노드의 외부 IP 및 Port
 * aInternalNodeIPnPort - [IN] 추가할 노드의 내부 IP 및 Port
 * aInternalR...IPnPort - [IN] 추가할 노드의 replication 내부 IP 및 Port
 * aConnType            - [IN] 추가할 노드의 internal 연결 방식
 *********************************************************************/
IDE_RC sdiZookeeper::addNode( SChar * aShardNodeID,
                              SChar * aNodeIPnPort,
                              SChar * aInternalNodeIPnPort,
                              SChar * aInternalReplicationHostIPnPort,
                              SChar * aConnType )
{
    ZKCResult       sResult;
    
    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sPath[8][SDI_ZKC_PATH_LENGTH] = {{0,},};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt            sDataLength;
    ZKState         sState;

    /* multi op */
    SInt            i = 0;
    zoo_op_t        sOp[8];
    zoo_op_result_t sOpResult[8];

    idlOS::strncpy( sNodePath,
                    SDI_ZKC_PATH_NODE_META, 
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sNodePath,
                    "/",
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    idlOS::strncat( sNodePath,
                    mMyNodeName,
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    /* sNodePath는 '/altibase_shard/node_meta/[mMyNodeName]'의 값을 가진다. */

    /* 1. 추가할 노드와 동일한 이름의 노드가 있는지 확인한다. */
    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sNodePath,
                              sBuffer,
                              &sDataLength );

    /* 추가할 노드와 동일한 이름의 노드가 있을 경우 */
    if( sResult == ZKC_SUCCESS )
    {
        /* 해당 노드의 상태를 체크한다. */
        idlOS::strncpy( sPath[0],
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sPath[0],
                        SDI_ZKC_PATH_NODE_STATE,
                        ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
        /* sPath[0]는 '/altibase_shard/node_meta/[mMyNodeName]/state'의 값을 가진다. */       
 
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sPath[0],
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        sState = stateStringConvert( sBuffer );

        if( sState == ZK_ADD )
        {
            /* 해당 노드가 ADD 도중 실패했으나 제거되지 않은 상태이므로 제거하고 진행한다. */
            IDE_TEST( dropNode( mMyNodeName, ID_FALSE ) != IDE_SUCCESS );
        }
        else
        {
            /* 정상적인 노드와 이름이 겹침 */
            IDE_RAISE( err_alreadyExist );
        }
    }
    else
    {
        IDE_TEST_RAISE( sResult != ZKC_NO_NODE, err_ZKC );
    }

    /* 3. 새로 추가될 노드의 노드 메타를 multi op로 생성한다. */
    idlOS::memset( sPath[0], 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath[0],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[0]는 '/altibase_shard/node_meta/[mMyNodeName]'의 값을 가진다. */

    zoo_create_op_init( &sOp[0],                 /* operation */
                        sPath[0],                /* path */
                        NULL,                    /* value */
                        -1,                      /* value length */
                        &ZOO_OPEN_ACL_UNSAFE,    /* ACL */
                        ZOO_PERSISTENT,          /* flag */
                        NULL,                    /* path_buffer */
                        0 );                     /* path_buffer_len */

    idlOS::strncpy( sPath[1],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[1],
                    SDI_ZKC_PATH_NODE_ID,
                    ID_SIZEOF( sPath[1] ) - idlOS::strlen( sPath[1] ) - 1 );
    /* sPath[1]는 '/altibase_shard/node_meta/[mMyNodeName]/shard_node_id'의 값을 가진다. */

    zoo_create_op_init( &sOp[1],
                        sPath[1],
                        aShardNodeID,
                        idlOS::strlen( aShardNodeID ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );

    idlOS::strncpy( sPath[2],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[2],
                    SDI_ZKC_PATH_NODE_IP,
                    ID_SIZEOF( sPath[2] ) - idlOS::strlen( sPath[2] ) - 1 );

    /* sPath[2]는 '/altibase_shard/node_meta/[mMyNodeName]/node_ip:port'의 값을 가진다. */
    zoo_create_op_init( &sOp[2],
                        sPath[2],
                        aNodeIPnPort,
                        idlOS::strlen( aNodeIPnPort ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );

    idlOS::strncpy( sPath[3],
                    sNodePath, 
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[3],
                    SDI_ZKC_PATH_NODE_INTERNAL_IP,
                    ID_SIZEOF( sPath[3] ) - idlOS::strlen( sPath[3] ) - 1 );
    /* sPath[3]는 '/altibase_shard/node_meta/[mMyNodeName]/internal_node_ip:port'의 값을 가진다. */

    zoo_create_op_init( &sOp[3],
                        sPath[3],
                        aInternalNodeIPnPort,
                        idlOS::strlen( aInternalNodeIPnPort ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );

    idlOS::strncpy( sPath[4],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[4],
                    SDI_ZKC_PATH_NODE_REPL_HOST_IP,
                    ID_SIZEOF( sPath[4] ) - idlOS::strlen( sPath[4] ) - 1 );
    /* sPath[4]는 '/altibase_shard/node_meta/[mMyNodeName]/internal_replication_host_ip:port'의 값을 가진다. */

    zoo_create_op_init( &sOp[4],
                        sPath[4],
                        aInternalReplicationHostIPnPort,
                        idlOS::strlen( aInternalReplicationHostIPnPort ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );

    idlOS::strncpy( sPath[5], 
                    sNodePath, 
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[5],
                    SDI_ZKC_PATH_NODE_CONN_TYPE,
                    ID_SIZEOF( sPath[5] ) - idlOS::strlen( sPath[5] ) - 1 );
    /* sPath[5]는 '/altibase_shard/node_meta/[mMyNodeName]/conn_type'의 값을 가진다. */

    zoo_create_op_init( &sOp[5],
                        sPath[5],
                        aConnType,
                        idlOS::strlen( aConnType ),
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );

    idlOS::strncpy( sPath[6],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[6],
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath[6] ) - idlOS::strlen( sPath[6] ) - 1 );
    /* sPath[6]는 '/altibase_shard/node_meta/[mMyNodeName]/state'의 값을 가진다. */

    zoo_create_op_init( &sOp[6],
                        sPath[6],
                        SDI_ZKC_STATE_ADD,
                        3,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );

    idlOS::strncpy( sPath[7],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[7],
                    SDI_ZKC_PATH_NODE_FAILOVER_TO,
                    ID_SIZEOF( sPath[7] ) - idlOS::strlen( sPath[7] ) - 1 );
    /* sPath[7]는 '/altibase_shard/node_meta/[mMyNodeName]/failoverTo'의 값을 가진다. */

    zoo_create_op_init( &sOp[7],
                        sPath[7],
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT,
                        NULL,
                        0 );

    /* 4. multi op를 수행한다. */
    sResult = aclZKC_multiOp( mZKHandler, 8, sOp, sOpResult );

    /* multi op로 수행한 각 operation의 정상 수행 여부를 확인한다. */
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for( i = 0; i <= 7; i++ )
    {
        IDE_TEST_RAISE( sOpResult[i].err != ZOK, err_ZKC );
    }

    /* 5. connection_info를 추가한다. */
    IDE_TEST( addConnectionInfo() != IDE_SUCCESS );

    /* 6. watch를 건다. */
    IDE_TEST( settingAddNodeWatch() != IDE_SUCCESS );
    IDE_TEST( settingAllDeleteNodeWatch() != IDE_SUCCESS );

    /* mm 모듈에서 commit시 addNodeAfterCommit을 호출할 수 있도록 현재 수행 중인 작업이 add임을 표시한다. */
    mRunningJobType = ZK_JOB_ADD;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_alreadyExist )
    {
        logAndSetErrMsg( ZKC_NODE_EXISTS );
    }
    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::addNodeAfterCommit           *
 * ------------------------------------------------------------------*
 * ADD 작업이 완료되어 commit 된 후 호출된다.
 * 내 노드의 상태를 RUN으로 변경하고 클러스터의 SMN 값을 갱신한 후 lock을 해제한다.
 * state와 SMN은 multi op로 한번에 변경한다.
 *
 * aSMN    - [IN] 변경할 클러스터 메타의 SMN 값
 *********************************************************************/
IDE_RC sdiZookeeper::addNodeAfterCommit( ULong aSMN )
{
    ZKCResult   sResult;
    SChar       sPath[2][SDI_ZKC_PATH_LENGTH] = {{0,},};
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    /* multi op */
    SInt            i = 0;
    zoo_op_t        sOp[2];
    zoo_op_result_t sOpResult[2];

    IDE_DASSERT( mRunningJobType == ZK_JOB_ADD );
    IDE_DASSERT( mIsGetZookeeperLock == ID_TRUE );

    /* state를 변경하기 위해 path를 세팅한다. */
    idlOS::strncpy( sPath[0],
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[0],
                    "/",
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    idlOS::strncat( sPath[0],
                    mMyNodeName,
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    idlOS::strncat( sPath[0],
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    /* sPath[0]는 '/altibase_shard/node_meta/[mMyNodeName]/state'의 값을 가진다. */

    /* SMN을 변경하기 위해 path와 값(aSMN)을 세팅한다. */
    idlOS::strncpy( sPath[1],
                    SDI_ZKC_PATH_SMN,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[1]은 '/altibase_shard/cluster_meta/SMN'의 값을 가진다. */
    (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT64_FMT, aSMN );

    /* multi op를 생성한다. */

    /* state */
    zoo_set_op_init( &sOp[0],
                     sPath[0],
                     SDI_ZKC_STATE_RUN,
                     idlOS::strlen( SDI_ZKC_STATE_RUN ),
                     -1,
                     NULL );

    /* SMN */
    zoo_set_op_init( &sOp[1],
                     sPath[1],
                     sBuffer,
                     idlOS::strlen( sBuffer ),
                     -1,
                     NULL );

    /* multi op를 수행해 한번에 변경한다. */
    sResult = aclZKC_multiOp( mZKHandler, 2, sOp, sOpResult );

    /* 정상 수행 여부를 확인한다. */
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for( i = 0; i <= 1; i++ )
    {
        IDE_TEST_RAISE( sOpResult[i].err != ZOK, err_ZKC );
    }

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::dropNode                     *
 * ------------------------------------------------------------------*
 * 클러스터 메타에서 특정 노드의 정보를 제거한다.
 * 해당 함수는 자신 뿐 아니라 타 노드에 대해서도 수행이 가능하다.
 * 해당 함수에서는 대상 노드의 state를 DROP으로 바꾸는 작업만 수행한다.
 * 실제 제거 작업은 commit시 호출되는 dropNodeAfterCommit에서 수행된다.
 *
 * aNodeName    - [IN] 클러스터 메타에서 제거할 노드의 이름
 * aIsDropForce - [IN] DROP FORCE 여부
 *********************************************************************/
IDE_RC sdiZookeeper::dropNode( SChar * aNodeName, idBool aIsDropForce )
{
    ZKCResult       sResult;

    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    idlOS::strncat( sPath,
                    aNodeName,
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1);
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[aNodeName]/state'의 값을 가진다. */

    /* 해당 노드의 상태를 DROP으로 변경한다. */
    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
    idlOS::strncpy( sBuffer,
                    SDI_ZKC_STATE_DROP, 
                    SDI_ZKC_BUFFER_SIZE );

    sResult = aclZKC_setInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              idlOS::strlen( sBuffer ) );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* mm 모듈에서 commit시 dropNodeAfterCommit을 호출할 수 있도록 현재 수행 중인 작업이 drop임을 표시한다. */
    if ( aIsDropForce != ID_TRUE )
    {
        mRunningJobType = ZK_JOB_DROP;
    }
    else
    {
        mRunningJobType = ZK_JOB_DROP_FORCE;
    }
    idlOS::strncpy( mTargetNodeName, 
                    aNodeName, 
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::dropNodeAfterCommit          *
 * ------------------------------------------------------------------*
 * DROP 작업이 완료되어 commit 된 후 호출된다.
 * 해당 노드의 노드 메타를 제거한 후 failover history / 클러스터의 SMN 값을 갱신한다.
 * 만약 DROP 작업의 대상이 자신이라면 lock을 풀고 zookeeper와의 연결을 종료한다. 
 *
 * aSMN      - [IN]  변경할 클러스터 메타의 SMN 값
 *********************************************************************/
IDE_RC sdiZookeeper::dropNodeAfterCommit( ULong     aSMN )
{
    ZKCResult       sResult;
    SChar           sPath[2][SDI_ZKC_PATH_LENGTH] = {{0,},};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar         * sStringPtr = NULL;
    SChar           sOldFailoverHistory[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar           sNewFailoverHistory[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar           sTargetNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SInt            sDataLength;
    idBool          sIsDead = ID_FALSE;
    idBool          sIsAlloc = ID_FALSE;
    iduList       * sNodeInfoList;
    UInt            sNodeCnt ;
    /* multi op */
    SInt            i = 0;
    zoo_op_t        sOp[2];
    zoo_op_result_t sOpResult[2];

    IDE_DASSERT( mIsGetZookeeperLock == ID_TRUE );
    IDE_DASSERT( mRunningJobType == ZK_JOB_DROP );
    IDE_DASSERT( idlOS::strlen( mTargetNodeName ) > 0 );

    idlOS::strncpy( sTargetNodeName,
                    mTargetNodeName,
                    idlOS::strlen( mTargetNodeName ) );

    /* 제거 대상 노드가 살아 있는 상태인지 체크한다. */
    idlOS::strncpy( sPath[0],
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[0],
                    "/",
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    idlOS::strncat( sPath[0],
                    mTargetNodeName,
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    /* sPath[0]는 '/altibase_shard/connection_info/[mTargetNodeName]'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath[0],
                              sBuffer,
                              &sDataLength );

    /* 제거 대상 노드가 죽어있는 상태라면 failover history에서 해당 노드의 정보를 제거해야 할 수 있다. */
    if( sResult == ZKC_NO_NODE )
    {
        sIsDead = ID_TRUE;

        idlOS::memset( sPath[0], 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sPath[0],
                        SDI_ZKC_PATH_FAILOVER_HISTORY,
                        SDI_ZKC_PATH_LENGTH );
        /* sPath[0]는 '/altibase_shard/cluster_meta/failover_history'의 값을 가진다. '*/

        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sPath[0],
                                  sOldFailoverHistory,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        if( sDataLength > 0 )
        {
            sStringPtr = idlOS::strtok( sOldFailoverHistory, "/" );

            /* failover history는 [노드이름]:[SMN]/[노드이름]:[SMN]/...의 형식으로 되어 있으므로  *
             * 앞부분만 비교한다면 노드 이름만 정상적으로 비교할 수 있다.                         */
            if( isNodeMatch( sStringPtr, mTargetNodeName ) == ID_TRUE )
            {
                /* 대상 노드의 failover history 정보를 읽었다면 해당 정보를 무시한다. */
            }
            else
            {
                /* 대상 노드가 아닌 다른 노드의 failover history 정보를 읽었다면 *
                 * 그 정보를 그대로 new Failover histroy에 복사한다.             */
                idlOS::strncpy( sNewFailoverHistory,
                                sStringPtr,
                                SDI_ZKC_BUFFER_SIZE );
            }

            while( sStringPtr != NULL )
            {
                sStringPtr = idlOS::strtok( NULL, "/" );

                if( sStringPtr != NULL )
                {
                    if( isNodeMatch( sStringPtr, mTargetNodeName ) == ID_TRUE )
                    {
                        /* 대상 노드의 failover history 정보를 읽었다면 해당 정보를 무시한다. */
                    }
                    else
                    {
                        /* 대상 노드가 아닌 다른 노드의 failover history 정보를 읽었다면 *
                         * 그 정보를 그대로 new Failover histroy에 복사한다.             */

                        if( idlOS::strlen( sNewFailoverHistory ) > 1 )
                        {
                            idlOS::strncat( sNewFailoverHistory,
                                            "/",
                                            ID_SIZEOF( sNewFailoverHistory ) - idlOS::strlen( sNewFailoverHistory ) - 1 );
                        }

                        idlOS::strncat( sNewFailoverHistory,
                                        sStringPtr,
                                        ID_SIZEOF( sNewFailoverHistory ) - idlOS::strlen( sNewFailoverHistory ) - 1 );
                    }
                }
                else
                {
                    /* failover history의 끝까지 다 옴 */
                }
            }
        }
        else
        {
            /* failover history가 비어있다면 해당 노드는 죽은게 아니라 SHUTDOWN된 상태이다. */
            sIsDead = ID_FALSE;
        }
    }
    else
    {
        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    }

    /* 메타에서 노드 정보를 제거한다. */
    IDE_TEST( removeNode( mTargetNodeName ) != IDE_SUCCESS );

    /* SMN을 변경하기 위해 path와 값(aSMN)을 세팅한다. */
    idlOS::strncpy( sPath[0],
                    SDI_ZKC_PATH_SMN,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[0]은 '/altibase_shard/cluster_meta/SMN'의 값을 가진다. */
    (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT64_FMT, aSMN );

    idlOS::strncpy( sPath[1],
                    SDI_ZKC_PATH_FAILOVER_HISTORY,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[1]는 '/altibase_shard/cluster_meta/failover_history'의 값을 가진다. */

    /* multi op를 생성한다. */

    /* SMN */
    zoo_set_op_init( &sOp[0],
                     sPath[0],
                     sBuffer,
                     idlOS::strlen( sBuffer ),
                     -1,
                     NULL );

    /* failover history */

    if( sNewFailoverHistory != NULL )
    {
        zoo_set_op_init( &sOp[1],
                         sPath[1],
                         sNewFailoverHistory,
                         idlOS::strlen( sNewFailoverHistory ),
                         -1,
                         NULL );
    }
    else
    {
        zoo_set_op_init( &sOp[1],
                         sPath[1],
                         NULL,
                         -1,
                         -1,
                         NULL );       
    }

    /* multi op를 수행해 한번에 변경한다. */
    if( sIsDead == ID_FALSE )
    {
        /* 대상 노드가 살아있다면 state와 SMN만 변경한다. */
        sResult = aclZKC_multiOp( mZKHandler, 1, sOp, sOpResult );

        /* 정상 수행 여부를 확인한다. */
        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
        IDE_TEST_RAISE( sOpResult[0].err != ZOK, err_ZKC );
    }
    else
    {
        /* 대상 노드가 죽어있다면 failover history도 변경해야 한다. */
        sResult = aclZKC_multiOp( mZKHandler, 2, sOp, sOpResult );

        /* 정상 수행 여부를 확인한다. */
        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        for( i = 0; i <= 1; i++ )
        {
            IDE_TEST_RAISE( sOpResult[i].err != ZOK, err_ZKC );
        }
    }

    IDE_TEST( getAllNodeInfoList( &sNodeInfoList,
                                  &sNodeCnt )
              != IDE_SUCCESS );
    sIsAlloc = ID_TRUE;

    if (sNodeCnt == 0)
    {
        IDE_TEST(finalizeClusterMeta() != IDE_SUCCESS);
    }
    else
    {
        releaseShardMetaLock();
        releaseZookeeperMetaLock();
    }
    /* 제거된 노드가 자신이라면 연결을 끊는다. */
    /* metalock을 해제하면서 mTargetNodeName을 초기화하므로 미리 복사해둔 sTargetNodeName과 비교해야 한다. */
    if( isNodeMatch( sTargetNodeName, mMyNodeName ) == ID_TRUE )
    {
        sdiZookeeper::disconnect();
    }

    sIsAlloc = ID_FALSE;
    freeList( sNodeInfoList, SDI_ZKS_LIST_NODEINFO );

    /* shard meta를 초기화한다. */
    IDE_TEST( sdi::resetShardMetaWithDummyStmt() != IDE_SUCCESS );
    /* 차후 local node 정보로 제거해야 한다.(node id 프로젝트시 처리할 예정)*/

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    if (sIsAlloc == ID_TRUE)
    {
        freeList( sNodeInfoList, SDI_ZKS_LIST_NODEINFO );
    }
    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::dropForceAfterCommit          *
 * ------------------------------------------------------------------*
 * DROP Force 작업이 완료되어 commit 된 후 호출된다.
 * 해당 노드의 노드 메타를 제거한 후 failover history / 클러스터의 SMN 값을 갱신한다.
 * 만약 DROP 작업의 대상이 자신이라면 lock을 풀고 zookeeper와의 연결을 종료한다. 
 *
 * aSMN      - [IN]  변경할 클러스터 메타의 SMN 값
 *********************************************************************/
IDE_RC sdiZookeeper::dropForceAfterCommit( ULong     aSMN )
{
    ZKCResult       sResult;
    SChar           sPath[2][SDI_ZKC_PATH_LENGTH] = {{0,},};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar         * sStringPtr = NULL;
    SChar           sOldFailoverHistory[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar           sNewFailoverHistory[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar           sTargetNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SInt            sDataLength;
    idBool          sIsDead = ID_FALSE;
    idBool          sIsAlloc = ID_FALSE;
    iduList       * sNodeInfoList;
    UInt            sNodeCnt ;
    /* multi op */
    SInt            i = 0;
    zoo_op_t        sOp[2];
    zoo_op_result_t sOpResult[2];

    SChar           sFailoverToNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    sdiLocalMetaInfo * sLocalMetaInfo = NULL;
    iduListNode   * sNode = NULL;
    iduListNode   * sDummy = NULL;

    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};

    IDE_DASSERT( mIsGetZookeeperLock == ID_TRUE );
    IDE_DASSERT( mRunningJobType == ZK_JOB_DROP_FORCE );
    IDE_DASSERT( idlOS::strlen( mTargetNodeName ) > 0 );

    idlOS::strncpy( sTargetNodeName,
                    mTargetNodeName,
                    idlOS::strlen( mTargetNodeName ) );

    idlOS::strncpy( sNodePath,
                    SDI_ZKC_PATH_NODE_META, 
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sNodePath,
                    "/",
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    idlOS::strncat( sNodePath,
                    sTargetNodeName,
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    /* sNodePath는 '/altibase_shard/node_meta/[mMyNodeName]'의 값을 가진다. */


    /* TargetNode의 FailoverTo를 확인한다. */
    sDataLength = SDI_ZKC_BUFFER_SIZE;
    IDE_TEST( sdiZookeeper::getNodeInfo( sTargetNodeName,
                                         (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                         sBuffer,
                                         &sDataLength ) != IDE_SUCCESS );

    idlOS::strncpy( sFailoverToNodeName,
                    sBuffer,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] TargetNode's FailoverTo : %s", sFailoverToNodeName);

    /* 제거 대상 노드가 살아 있는 상태인지 체크한다. */
    idlOS::strncpy( sPath[0],
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[0],
                    "/",
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    idlOS::strncat( sPath[0],
                    mTargetNodeName,
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    /* sPath[0]는 '/altibase_shard/connection_info/[mTargetNodeName]'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath[0],
                              sBuffer,
                              &sDataLength );

    ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] TargetNode Connection Info : %d", sResult);

    /* 제거 대상 노드가 죽어있는 상태라면 failover history에서 해당 노드의 정보를 제거해야 할 수 있다. */
    if( sResult == ZKC_NO_NODE )
    {
        sIsDead = ID_TRUE;

        idlOS::memset( sPath[0], 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sPath[0],
                        SDI_ZKC_PATH_FAILOVER_HISTORY,
                        SDI_ZKC_PATH_LENGTH );
        /* sPath[0]는 '/altibase_shard/cluster_meta/failover_history'의 값을 가진다. '*/

        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sPath[0],
                                  sOldFailoverHistory,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] Old FailoverHistory : %s", sOldFailoverHistory);

        if( sDataLength > 0 )
        {
            sStringPtr = idlOS::strtok( sOldFailoverHistory, ":" );

            /* failover history는 [노드이름]:[SMN]/[노드이름]:[SMN]/...의 형식으로 되어 있으므로  *
             * 앞부분만 비교한다면 노드 이름만 정상적으로 비교할 수 있다.                         */
            if( isNodeMatch( sStringPtr, mTargetNodeName ) == ID_TRUE )
            {
                /* 대상 노드의 failover history 정보를 읽었다면 해당 정보를 무시한다. */
            }
            else
            {
                /* 대상 노드가 아닌 다른 노드의 failover history 정보를 읽었다면 *
                 * 그 정보를 그대로 new Failover histroy에 복사한다.             */
                idlOS::strncpy( sNewFailoverHistory,
                                sStringPtr,
                                SDI_ZKC_BUFFER_SIZE );
            }

            ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] before Loop NEw FailoverHistory : %s", sNewFailoverHistory);
            ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] before Loop1 StrPtr : %s", sStringPtr );

            sStringPtr = idlOS::strtok( NULL, "/" );

            ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] before Loop2 StrPtr : %s", sStringPtr );

            while( sStringPtr != NULL )
            {
                sStringPtr = idlOS::strtok( NULL, ":" );

                if( sStringPtr != NULL )
                {
                    if( isNodeMatch( sStringPtr, mTargetNodeName ) == ID_TRUE )
                    {
                        /* 대상 노드의 failover history 정보를 읽었다면 해당 정보를 무시한다. */
                        sStringPtr = idlOS::strtok( NULL, "/" );
                    }
                    else
                    {
                        /* 대상 노드가 아닌 다른 노드의 failover history 정보를 읽었다면 *
                         * 그 정보를 그대로 new Failover histroy에 복사한다.             */

                        if( idlOS::strlen( sNewFailoverHistory ) > 1 )
                        {
                            idlOS::strncat( sNewFailoverHistory,
                                            "/",
                                            ID_SIZEOF( sNewFailoverHistory ) - idlOS::strlen( sNewFailoverHistory ) - 1 );
                        }

                        idlOS::strncat( sNewFailoverHistory,
                                        sStringPtr,
                                        ID_SIZEOF( sNewFailoverHistory ) - idlOS::strlen( sNewFailoverHistory ) - 1 );
                        sStringPtr = idlOS::strtok( NULL, "/" );

                        idlOS::strncat( sNewFailoverHistory,
                                        ":",
                                        ID_SIZEOF( sNewFailoverHistory ) - idlOS::strlen( sNewFailoverHistory ) - 1 );

                        idlOS::strncat( sNewFailoverHistory,
                                        sStringPtr,
                                        ID_SIZEOF( sNewFailoverHistory ) - idlOS::strlen( sNewFailoverHistory ) - 1 );

                    }
                }
                else
                {
                    /* failover history의 끝까지 다 옴 */
                }


                ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] In Loop NEw FailoverHistory : %s", sNewFailoverHistory);
                ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] In Loop StrPtr : %s", sStringPtr );


            }
        }
        else
        {
            /* failover history가 비어있다면 해당 노드는 죽은게 아니라 SHUTDOWN된 상태이다. */
            sIsDead = ID_FALSE;
        }

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] New FailoverHistory : %s", sNewFailoverHistory);

    }
    else
    {
        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    }

    /* 메타에서 노드 정보를 제거한다. */
    IDE_TEST( removeNode( mTargetNodeName ) != IDE_SUCCESS );

    /* SMN을 변경하기 위해 path와 값(aSMN)을 세팅한다. */
    idlOS::strncpy( sPath[0],
                    SDI_ZKC_PATH_SMN,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[0]은 '/altibase_shard/cluster_meta/SMN'의 값을 가진다. */
    (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT64_FMT, aSMN );

    idlOS::strncpy( sPath[1],
                    SDI_ZKC_PATH_FAILOVER_HISTORY,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[1]는 '/altibase_shard/cluster_meta/failover_history'의 값을 가진다. */

    /* multi op를 생성한다. */

    /* SMN */
    zoo_set_op_init( &sOp[0],
                     sPath[0],
                     sBuffer,
                     idlOS::strlen( sBuffer ),
                     -1,
                     NULL );

    /* failover history */

    if( sNewFailoverHistory != NULL )
    {
        zoo_set_op_init( &sOp[1],
                         sPath[1],
                         sNewFailoverHistory,
                         idlOS::strlen( sNewFailoverHistory ),
                         -1,
                         NULL );
    }
    else
    {
        zoo_set_op_init( &sOp[1],
                         sPath[1],
                         NULL,
                         -1,
                         -1,
                         NULL );       
    }

    /* multi op를 수행해 한번에 변경한다. */
    if( sIsDead == ID_FALSE )
    {
        /* 대상 노드가 살아있다면 state와 SMN만 변경한다. */
        sResult = aclZKC_multiOp( mZKHandler, 1, sOp, sOpResult );

        /* 정상 수행 여부를 확인한다. */
        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
        IDE_TEST_RAISE( sOpResult[0].err != ZOK, err_ZKC );
    }
    else
    {
        /* 대상 노드가 죽어있다면 failover history도 변경해야 한다. */
        sResult = aclZKC_multiOp( mZKHandler, 2, sOp, sOpResult );

        /* 정상 수행 여부를 확인한다. */
        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        for( i = 0; i <= 1; i++ )
        {
            IDE_TEST_RAISE( sOpResult[i].err != ZOK, err_ZKC );
        }
    }

    IDE_TEST( getAllNodeInfoList( &sNodeInfoList,
                                  &sNodeCnt )
              != IDE_SUCCESS );
    sIsAlloc = ID_TRUE;

    /* FailoverTo가 TargetNode인 노드 들의 정보를 TargetNode의 FailoverTo 정보로 변경한다. */
    IDU_LIST_ITERATE_SAFE( sNodeInfoList, sNode, sDummy )
    {
        sLocalMetaInfo = (sdiLocalMetaInfo *)sNode->mObj;

        sDataLength = SDI_ZKC_BUFFER_SIZE;
        IDE_TEST( sdiZookeeper::getNodeInfo( sLocalMetaInfo->mNodeName,
                                             (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                             sBuffer,
                                             &sDataLength ) != IDE_SUCCESS );

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] TCTC Node Name : %s", sLocalMetaInfo->mNodeName );
 
        if ( sDataLength > 0 )
        {

            ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] FailoverTo Node Name : %s", sBuffer );
            ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] TargetName & FT Name : %s, %s", sTargetNodeName, sFailoverToNodeName );

            if ( idlOS::strncmp( sBuffer, sTargetNodeName, SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {

                idlOS::strncpy( sNodePath,
                                SDI_ZKC_PATH_NODE_META, 
                                SDI_ZKC_PATH_LENGTH );
                idlOS::strncat( sNodePath,
                                "/",
                                ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
                idlOS::strncat( sNodePath,
                                sLocalMetaInfo->mNodeName,
                                ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );

                idlOS::strncpy( sPath[0],
                                sNodePath,
                                SDI_ZKC_PATH_LENGTH );
                idlOS::strncat( sPath[0],
                                SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );

                sResult = aclZKC_setInfo( mZKHandler,
                                          sPath[0],
                                          sFailoverToNodeName,
                                          idlOS::strlen( sFailoverToNodeName ) );

                IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
            }
        }
    }

    if (sNodeCnt == 0)
    {
        IDE_TEST(finalizeClusterMeta() != IDE_SUCCESS);
    }
    else
    {
        releaseShardMetaLock();
        releaseZookeeperMetaLock();
    }
    /* 제거된 노드가 자신이라면 연결을 끊는다. */
    /* metalock을 해제하면서 mTargetNodeName을 초기화하므로 미리 복사해둔 sTargetNodeName과 비교해야 한다. */
    if( isNodeMatch( sTargetNodeName, mMyNodeName ) == ID_TRUE )
    {
        sdiZookeeper::disconnect();
    }

    sIsAlloc = ID_FALSE;
    freeList( sNodeInfoList, SDI_ZKS_LIST_NODEINFO );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    if (sIsAlloc == ID_TRUE)
    {
        freeList( sNodeInfoList, SDI_ZKS_LIST_NODEINFO );
    }
    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::shutdown                     *
 * ------------------------------------------------------------------*
 * 클러스터 메타에 해당 노드가 SHUTDOWN 상태임을 표기한다.
 *********************************************************************/
IDE_RC sdiZookeeper::shutdown()
{
    ZKCResult   sResult;

    SChar       sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    /* 연결되어 있지 않다면 zookeeper 메타를 변경할 필요가 없다. */
    if( isConnect() != ID_TRUE )
    {
        IDE_CONT( normal_exit );
    }
    /* 1. zookeeper_meta_lock의 lock을 잡는다 */
    IDE_TEST( getZookeeperMetaLock(ID_NULL_SESSION_ID) != IDE_SUCCESS );

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/", 
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    mMyNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[mMyNodeName]/state'의 값을 가진다. */

    /* 2. shutdown이 가능한 상태인지 확인한다. */
    IDE_TEST( validateState( ZK_SHUTDOWN, 
                             mMyNodeName ) != IDE_SUCCESS );

    /* 3. state를 shutdown으로 변경한다. */
    idlOS::strncpy( sBuffer,
                    SDI_ZKC_STATE_SHUTDOWN,
                    SDI_ZKC_BUFFER_SIZE );
    sResult = aclZKC_setInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              idlOS::strlen( sBuffer ) );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 4. zookeeper_meta_lock의 lock을 해제한다. */
    releaseZookeeperMetaLock();

    /* 5. ZKS와 연결을 끊는다. */
    disconnect();

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    releaseZookeeperMetaLock();

    return IDE_FAILURE; 
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::joinNode                     *
 * ------------------------------------------------------------------*
 * 클러스터 메타에 해당 노드가 JOIN 상태임을 표기한 후 watch를 건다.
 *********************************************************************/
IDE_RC sdiZookeeper::joinNode()
{
    ZKCResult   sResult;

    SChar       sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

     /* 1. join이 가능한 상태인지 확인한다. */
    IDE_TEST( validateState( ZK_JOIN, 
                             mMyNodeName ) != IDE_SUCCESS );

    /* 3. 메타 정보를 변경한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    mMyNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sNodePath는 '/altibase_shard/node_meta/[mMyNodeName]/state'의 값을 가진다. */

    idlOS::strncpy( sBuffer,
                    SDI_ZKC_STATE_JOIN,
                    SDI_ZKC_BUFFER_SIZE );

    sResult = aclZKC_setInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              idlOS::strlen( sBuffer ) );
    
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 4. 연결 정보를 생성한다. */
    IDE_TEST( addConnectionInfo() != IDE_SUCCESS );
   
    /* 5. watch를 건다. */
    IDE_TEST( settingAddNodeWatch() != IDE_SUCCESS );
    IDE_TEST( settingAllDeleteNodeWatch() != IDE_SUCCESS );

    /* mm 모듈에서 commit시 joinNodeAfterCommit을 호출할 수 있도록 현재 수행 중인 작업이 join임을 표시한다. */
    mRunningJobType = ZK_JOB_JOIN;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::joinNodeAfterCommit          *
 * ------------------------------------------------------------------*
 * JOIN 작업이 완료되어 commit 된 후 호출된다.
 *********************************************************************/
IDE_RC sdiZookeeper::joinNodeAfterCommit()
{
    SChar sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    IDE_DASSERT( mIsGetZookeeperLock == ID_TRUE );
    IDE_DASSERT( mRunningJobType == ZK_JOB_JOIN );

    idlOS::strncpy( sBuffer,
                    SDI_ZKC_STATE_RUN,
                    SDI_ZKC_BUFFER_SIZE );

    IDE_TEST( updateState( sBuffer,
                           mMyNodeName ) != IDE_SUCCESS );

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::reshardingJob                *
 * ------------------------------------------------------------------*
 * 내 노드에서 현재 작업이 resharding 상태임을 표기
 *********************************************************************/
void sdiZookeeper::reshardingJob()
{
    mRunningJobType = ZK_JOB_RESHARD;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::failoverForWatcher           *
 * ------------------------------------------------------------------*
 * FAILOVER를 수행하고 그에 맞춰 클러스터 메타를 수정한다.
 * 해당 함수는 watcher에 의해 자동적으로 호출된다.
 * failoverForQuery 함수와는 달리 validate 작업을 추가로 해줘야 한다.
 * 주의 : 대상 노드의 앞 노드도 failover가 필요하다면 추가로 failover를 수행한다.
 *
 * aNodeName - [IN] FAILOVER를 수행할 죽은 노드의 이름
 *********************************************************************/
IDE_RC sdiZookeeper::failoverForWatcher( SChar * aNodeName )
{
    ZKCResult       sResult;
    idBool          sIsAlive  = ID_FALSE;
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar           sPrevNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SChar           sCheckNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SInt            sDataLength;
    SInt            sDeadNodeCnt = 0;
    ZKState         sState;
    SInt            i = 0;
    ULong           sWaitTime;

    iduList       * sDeadNodeList = NULL;
    iduListNode   * sDeadNode     = NULL;
    SChar         * sDeadNodeName = NULL;
    SChar           sNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};

    iduListNode   * sIterator = NULL;

    idBool          sIsAlloc = ID_FALSE;
    SInt            sAllocState = 0;
    idBool          sIsLocked = ID_FALSE;

    IDE_DASSERT( idlOS::strlen( aNodeName ) > 0 );

    IDE_TEST( getZookeeperMetaLock( ID_UINT_MAX ) != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    /* 1. failover 대상인지 validate를 수행한다. */
    /* 내 노드의 state를 체크해 failover가 가능한 상태인지 확인한다. */
    IDE_TEST( validateState( ZK_FAILOVER,
                             mMyNodeName ) != IDE_SUCCESS );

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[aNodeName]/state'의 값을 가진다. */

    /* watcher에 의해 호출될 경우 DROP이나 SHUTDOWN으로 인한 연결 정보 제거때도 호출되므로 *
     * DROP이나 SHUTDOWN으로 인한 연결 정보 제거인지 확인해야 한다.                        */
    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              &sDataLength );

    if( sResult == ZKC_NO_NODE )
    {
        /* 해당 노드가 DROP되어 제거되었거나 ADD 구문 수행 중 예외가 발생한 상황에서 *
         * 다른 노드가 먼저 해당 노드를 failover를 수행해 노드 메타가 제거된 상태로  *
         * 이 경우에는 failover를 수행할 필요가 없다. */
        IDE_CONT( normal_exit );
    }
    else
    {
        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    }

    sState = stateStringConvert( sBuffer );

    if ( ( sState == ZK_SHUTDOWN ) || ( sState == ZK_KILL ) )
    {
        /* ZK_SHUTDOWN : 해당 노드가 SHUTDOWN되어 연결이 끊긴 것이므로 failover를 수행할 필요가 없다. */
        /* ZK_KILL     : Failover 구문에 의한 강제 KILL 이므로 FailoverForWatcher 대상이 아니다. */
        IDE_CONT( normal_exit );
    }
    if ( sState == ZK_ADD )
    {
        /* 죽은 노드가 ADD 작업 중 죽었을 경우 zookeeper 메타의 정보를 제거하기만 하면 된다. */
        IDE_TEST( removeNode( aNodeName ) != IDE_SUCCESS );

        IDE_CONT( normal_exit );
    }
    else
    {
        IDE_ERROR( sState != ZK_ERROR );
    }

    /* 해당 노드가 이미 failover 되었는지 확인한다. */
    sDataLength = SDI_ZKC_BUFFER_SIZE;
    IDE_TEST( getNodeInfo( aNodeName,
                           (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                           sBuffer,
                           &sDataLength ) != IDE_SUCCESS );

    if( sDataLength > 0 )
    {
        /* 이미 failover가 되어있는 상태이므로 따로 작업할 필요가 없다. */
        IDE_CONT( normal_exit );
    }

    /* Lock 풀고 기다린다. */
    sIsLocked = ID_FALSE;
    releaseZookeeperMetaLock();

    /* Delete 된 Node가 Re-Connect를 수행하였을 수 있다. 대기한다. */
    sWaitTime = mTickTime * mSyncLimit * mServerCnt; // *2

    /* sWaitTime은 millisecond 이다. */
    idlOS::sleep((UInt) (sWaitTime / 1000) );

    /* 다시 lock 잡고 확인한다. */
    IDE_TEST( getZookeeperMetaLock( ID_UINT_MAX ) != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    /* 잠시 끊겨서 재 접속 했을수 있다. NodeAliveCheck를 수행한다. */
    IDE_TEST( checkNodeAlive( aNodeName,
                              &sIsAlive ) != IDE_SUCCESS );

    IDE_TEST_CONT( sIsAlive == ID_TRUE, normal_exit );

//R2HA Merge를 위해 일단 Failover를 막는다.
//    sIsLocked = ID_FALSE;
//    releaseZookeeperMetaLock();

//    return IDE_SUCCESS;

    /* failover 해야 하는 상황이다. Failover Target List를 생성한다. */

    /* list를 초기화 한다. */
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                 ID_SIZEOF( iduList ),
                                 (void**)&(sDeadNodeList),
                                 SDI_ZKC_ALLOC_WAIT_TIME ) != IDE_SUCCESS );
    IDU_LIST_INIT( sDeadNodeList );

    sIsAlloc = ID_TRUE;

    /* 3. 죽은 노드 외에도 failover를 수행해야 하는 노드가 있는지 체크한다. */

    /* 죽은 노드가 failover를 수행하는 중이었거나 failover 요청은 받았지만  *
     * failover를 수행하지 못하고 죽은 경우를 대비하여 죽은 노드 외에도     *
     * failover를 수행해야 하는 노드가 있는지 체크해야 한다.                */
    idlOS::strncpy( sCheckNodeName,
                    mMyNodeName,
                    SDI_NODE_NAME_MAX_SIZE + 1 );
    // sPrevNodeName 초기화 필요?

    /* 참고 : sCheckNodeName은 이미 체크가 끝난 노드를 의미한다.                                  *
     *        각 loop에서 실질적으로 체크하는 대상은 sChecNodeName의 앞 노드인 sPrevNodeName이다. */
    while( isNodeMatch( mMyNodeName, sPrevNodeName ) != ID_TRUE )  
    {
        /* 대상 노드의 앞 노드가 살아있는지 확인한다. */
        IDE_TEST( getPrevNode( sCheckNodeName, sPrevNodeName, &sState ) != IDE_SUCCESS );

        IDE_TEST( checkNodeAlive( sPrevNodeName,
                                  &sIsAlive ) != IDE_SUCCESS );

        if( sIsAlive == ID_TRUE )
        {
            /* 살아있으면 다음 확인. */
            idlOS::strncpy( sCheckNodeName,
                            sPrevNodeName,
                            SDI_NODE_NAME_MAX_SIZE + 1 );

            continue;
        }
        else
        {
            /* 죽어있을 경우 해당 노드의 상태와 failoverTo를 체크한다. */

            if( sState == ZK_SHUTDOWN )
            {
                /* 죽어있을 경우에도 노드의 상태가 SHUTDOWN 일 경우에는 failover 대상이 아니므로 *
                 * 해당 노드는 무시하고 앞 노드에 대해 다시 수행한다.                            */
                idlOS::memset( sCheckNodeName, 0, SDI_NODE_NAME_MAX_SIZE + 1 );
                idlOS::strncpy( sCheckNodeName,
                                sPrevNodeName,
                                SDI_NODE_NAME_MAX_SIZE + 1 );
            }
            else if( sState == ZK_ADD )
            {
                /* 앞 노드가 ADD 작업 중 죽었을 경우 zookeeper 메타의 정보를 제거하기만 하면 된다. */
                IDE_TEST( removeNode( sPrevNodeName ) != IDE_SUCCESS );

                /* 앞 노드가 제거되었으므로 sCheckNodeName의 앞 노드가 변경되었을 것이다. */
            }
            else
            {
                /* failoverTo를 체크하여 failover가 이미 수행되었는지 확인한다. */
                sDataLength = SDI_ZKC_BUFFER_SIZE;
                IDE_TEST( getNodeInfo( sPrevNodeName,
                                       (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                       sBuffer,
                                       &sDataLength ) != IDE_SUCCESS );

                if( sDataLength > 0 )
                {
                    /* failoverTo가 비어있지 않을 경우 failover가 이미 수행된 것이므로
                     * 해당 노드는 무시하고 앞 노드에 대해 다시 수행한다. */
                    idlOS::memset( sCheckNodeName, 0, SDI_NODE_NAME_MAX_SIZE + 1 );
                    idlOS::strncpy( sCheckNodeName,
                                    sPrevNodeName,
                                    SDI_NODE_NAME_MAX_SIZE + 1 );
                }
                else
                {
                    /* 여기까지 왔다면 이전 노드는 failover의 수행 대상이다. */
                    sDeadNodeCnt++;

                    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                                 ID_SIZEOF( iduListNode ),
                                                 (void**)&(sDeadNode),
                                                 SDI_ZKC_ALLOC_WAIT_TIME ) != IDE_SUCCESS );
                    sAllocState = 1;

                    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                                 SDI_NODE_NAME_MAX_SIZE + 1,
                                                 (void**)&(sDeadNodeName),
                                                 SDI_ZKC_ALLOC_WAIT_TIME ) != IDE_SUCCESS );
                    sAllocState = 2;

                    idlOS::strncpy( sDeadNodeName,
                                    sPrevNodeName,
                                    SDI_NODE_NAME_MAX_SIZE + 1 );

                    IDU_LIST_INIT_OBJ( sDeadNode, (void*)sDeadNodeName );

                    /* List 의 Last에 삽입한다. 순서를 유지하기 위함. */
                    IDU_LIST_ADD_LAST( sDeadNodeList, sDeadNode );

                    sAllocState = 0;

                    /* 이후 앞 노드에 대해서도 다시 수행한다. */
                    idlOS::memset( sCheckNodeName, 0, SDI_NODE_NAME_MAX_SIZE + 1 );
                    idlOS::strncpy( sCheckNodeName,
                                    sPrevNodeName,
                                    SDI_NODE_NAME_MAX_SIZE + 1 );
                }
            }
        }
    }

    /* Failover List 작성이 끝났다. 
     * Failover 구문에서 Meta Lock 잡도록 풀어준다.
     * 동시 수행되는 Failover가 있다면 구문 내부에서 Lock 획득 후 확인한다. */
    sIsLocked = ID_FALSE;
    releaseZookeeperMetaLock();

    /* 4. failover 작업을 수행한다. */
    // for check
    i = 0;
    IDU_LIST_ITERATE( sDeadNodeList, sIterator )
    {
        idlOS::memset( sNodeName, 0, SDI_NODE_NAME_MAX_SIZE + 1 );
        idlOS::strncpy( sNodeName,
                        (SChar *) sIterator->mObj,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        mRunningJobType = ZK_JOB_FAILOVER_FOR_WATCHER;
        idlOS::strncpy( mTargetNodeName,
                        sNodeName,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        /* failover를 수행한다. */
        /* Zookeeper Thread는 idtBaseThread를 가지고 있지 않아서 여러가지 문제가 발생한다.
         * 새로 Thread를 생성해서 수행하도록 한다. */
        IDE_TEST( sdiFOThread::failoverExec( mMyNodeName,
                                             mTargetNodeName,
                                             1 ) != IDE_SUCCESS );

        i++;
    }

    /* for check */
    IDE_DASSERT( i == sDeadNodeCnt );

    if ( sIsAlloc == ID_TRUE )
    {
        sIsAlloc = ID_FALSE;
        freeList( sDeadNodeList, SDI_ZKS_LIST_NODENAME );
    }

    IDE_EXCEPTION_CONT( normal_exit );

    if ( sIsLocked == ID_TRUE )
    {
        releaseZookeeperMetaLock();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    /* list에 아직 추가되지 못한 메모리 할당을 정리한다. */
    switch( sAllocState )
    {
        case 2:
            (void)iduMemMgr::free( sDeadNodeName );
        case 1:
            (void)iduMemMgr::free( sDeadNode );
        case 0:
            break;
        default:
            break;
    }

    if ( sIsAlloc == ID_TRUE )
    {
        freeList( sDeadNodeList, SDI_ZKS_LIST_NODENAME );
    }

    if ( sIsLocked == ID_TRUE ) 
    {
        releaseZookeeperMetaLock();
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::failoverForQuery             *
 * ------------------------------------------------------------------*
 * FAILOVER를 수행하고 그에 맞춰 클러스터 메타를 수정한다.
 * 해당 함수는 ALTER DATABASE SHARD FAILOVER 구문에 의해 호출된다.
 * failoverForWatcher 함수와는 달리 qdsd::validateShardFailover 함수에서
 * validation을 수행했으므로 따로 해줄 필요가 없다.
 * 해당 함수에서는 내 노드의 state를 FAILOVER로 바꾸는 작업만 수행한다.
 * 실제 failover 작업은 commit시 호출되는 failoverAfterCommit에서 수행된다.
 * 주의 : failoverForWatcher와는 달리 대상 노드 하나만 failover가 수행된다.
 *********************************************************************/
IDE_RC sdiZookeeper::failoverForQuery( SChar * aNodeName,
                                       SChar * aFailoverNodeName )
{
    ZKCResult   sResult;
    SChar       sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    /* 내 state를 FAILOVER로 변경한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    mMyNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[mMyNodeName]/state'의 값을 가진다. */

    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
    idlOS::strncpy( sBuffer,
                    SDI_ZKC_STATE_FAILOVER,
                    SDI_ZKC_BUFFER_SIZE );

    sResult = aclZKC_setInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              idlOS::strlen( sBuffer ) );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* mm 모듈에서 commit시 failoverAfterCommit을 호출할 수 있도록 현재 수행 중인 작업이 failover임을 표시한다. */
    mRunningJobType = ZK_JOB_FAILOVER;
    idlOS::strncpy( mTargetNodeName, 
                    aNodeName,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    idlOS::strncpy( mFailoverNodeName, 
                    aFailoverNodeName,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::failoverAfterCommit          *
 * ------------------------------------------------------------------*
 * FAILOVER 구문이 완료되어 commit 된 후 호출된다.
 * watcher에 의해 failover 작업이 수행될 경우에는 호출되지 않는다.a
 * (갱신이 필요할 경우) fault detection time을 갱신 한 후
 * failoverTo / failover history / 클러스터의 SMN을 multi op로 동시에 변경한다.
 *
 * aSMN      - [IN] 변경할 클러스터 메타의 SMN 값
 * aSMN      - [IN] 변경하기 전 클러스터 메타의 SMN 값
 *********************************************************************/
IDE_RC sdiZookeeper::failoverAfterCommit( ULong   aNewSMN, ULong aBeforeSMN )
{
    ZKCResult   sResult;

    SChar       sPath[5][SDI_ZKC_PATH_LENGTH] = {{0,},};
    SChar       sFailoverHistory[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt        sDataLength;
    SLong       sDeadtime;

    SChar       sSqlStr[128];

    /* multi op */
    SInt            i = 0;
    zoo_op_t        sOp[5];
    zoo_op_result_t sOpResult[5];

    IDE_DASSERT( mIsGetZookeeperLock == ID_TRUE );
    IDE_DASSERT( ( mRunningJobType == ZK_JOB_FAILOVER ) || ( mRunningJobType == ZK_JOB_FAILOVER_FOR_WATCHER ) );
    IDE_DASSERT( idlOS::strlen( mTargetNodeName ) > 0 );

    /* 1. fault detection time을 확인하고 비어있다면 추가한다. */
    idlOS::memset( sPath[0], 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath[0],
                    SDI_ZKC_PATH_FAULT_DETECT_TIME,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[0]는 '/altibase_shard/cluster_meta/fault_detection_time'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath[0],
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    if( sDataLength > 0 )
    {
        /* 이미 다른 노드의 값이 들어있다면 세팅할 필요가 없다. */
    }
    else
    {
        /* fault dectection time이 비어있을 경우 현재 시간을 삽입한다. */
        sDeadtime = mtc::getSystemTimezoneSecond();

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_INT32_FMT, sDeadtime );

        sResult = aclZKC_setInfo( mZKHandler,
                                  sPath[0],
                                  sBuffer,
                                  idlOS::strlen( sBuffer ) );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    }

    /* '죽은 노드이름:현재 SMN' 값으로 죽은 노드에 대한 failover_history를 만든다. */
    idlOS::strncpy( sFailoverHistory,
                    mTargetNodeName,
                    SDI_ZKC_BUFFER_SIZE );
    idlOS::strncat( sFailoverHistory,
                    ":",
                    ID_SIZEOF( sFailoverHistory ) - idlOS::strlen( sFailoverHistory ) - 1 );

    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
    (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT64_FMT, aBeforeSMN );

    idlOS::strncat( sFailoverHistory,
                    sBuffer,
                    ID_SIZEOF( sFailoverHistory ) - idlOS::strlen( sFailoverHistory ) - 1 );

    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
    (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT64_FMT, aNewSMN );

    idlOS::strncat( sFailoverHistory,
                    ":",
                    ID_SIZEOF( sFailoverHistory ) - idlOS::strlen( sFailoverHistory ) - 1 );
    idlOS::strncat( sFailoverHistory,
                    sBuffer,
                    ID_SIZEOF( sFailoverHistory ) - idlOS::strlen( sFailoverHistory ) - 1 );

    /* 기존의 failover_history를 가져와 합친다. */
    idlOS::memset( sPath[0], 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath[0],
                    SDI_ZKC_PATH_FAILOVER_HISTORY,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[0]는 '/altibase_shard/cluster_meta/failover_history'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath[0],
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    IDE_TEST_RAISE( sDataLength + idlOS::strlen( sFailoverHistory ) >= SDI_ZKC_BUFFER_SIZE, too_long_info );

    /* failover history에 다른 노드의 정보가 이미 있다면 /로 구분해 줘야 한다. */
    if( sDataLength > 0 )
    {
        idlOS::strncat( sFailoverHistory,
                        "/",
                        ID_SIZEOF( sFailoverHistory ) - idlOS::strlen( sFailoverHistory ) - 1 );
        idlOS::strncat( sFailoverHistory,
                        sBuffer,
                        ID_SIZEOF( sFailoverHistory ) - idlOS::strlen( sFailoverHistory ) - 1 );
    }

    /* 죽은 노드의 failoverTo를 추가한다. */
    idlOS::memset( sPath[1], 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath[1],
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[1],
                    "/",
                    ID_SIZEOF( sPath[1] ) - idlOS::strlen( sPath[1] ) - 1 );
    idlOS::strncat( sPath[1],
                    mTargetNodeName,
                    ID_SIZEOF( sPath[1] ) - idlOS::strlen( sPath[1] ) - 1 );
    idlOS::strncat( sPath[1],
                    SDI_ZKC_PATH_NODE_FAILOVER_TO,
                    ID_SIZEOF( sPath[1] ) - idlOS::strlen( sPath[1] ) - 1 );
    /* sPath는 '/altibase_shard/cluster_meta/[mTargetNodeName]/failoverTo'의 값을 가진다. */

    /* SMN을 변경하기 위해 path와 값(aNewSMN)을 세팅한다. */
    idlOS::strncpy( sPath[2],
                    SDI_ZKC_PATH_SMN,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[2]은 '/altibase_shard/cluster_meta/SMN'의 값을 가진다. */
    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
    (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT64_FMT, aNewSMN );

    /* state를 변경하기 위해 path를 세팅한다. */
    idlOS::strncpy( sPath[3],
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[3],
                    "/",
                    ID_SIZEOF( sPath[3] ) - idlOS::strlen( sPath[3] ) - 1 );
    idlOS::strncat( sPath[3],
                    mMyNodeName,
                    ID_SIZEOF( sPath[3] ) - idlOS::strlen( sPath[3] ) - 1 );
    idlOS::strncat( sPath[3],
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath[3] ) - idlOS::strlen( sPath[3] ) - 1 );
    /* sPath[3]는 '/altibase_shard/node_meta/[mMyNodeName]/state'의 값을 가진다. */

    /* TargetNode의 state를 변경하기 위해 path를 세팅한다. */
    idlOS::strncpy( sPath[4],
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[4],
                    "/",
                    ID_SIZEOF( sPath[4] ) - idlOS::strlen( sPath[4] ) - 1 );
    idlOS::strncat( sPath[4],
                    mTargetNodeName,
                    ID_SIZEOF( sPath[4] ) - idlOS::strlen( sPath[4] ) - 1 );
    idlOS::strncat( sPath[4],
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath[4] ) - idlOS::strlen( sPath[4] ) - 1 );
    /* sPath[4]는 '/altibase_shard/node_meta/[mTargetNodeName]/state'의 값을 가진다. */


    /* multi op를 생성한다. */
    /* failover history */
    zoo_set_op_init( &sOp[0],
                     sPath[0],
                     sFailoverHistory,
                     idlOS::strlen( sFailoverHistory ),
                     -1,
                     NULL );

    /* failoverTo */
    zoo_set_op_init( &sOp[1],
                     sPath[1],
                     mFailoverNodeName, /* Failover 해간 Node의 이름을 기록 */
                     idlOS::strlen( mFailoverNodeName ),
                     -1,
                     NULL );

    /* SMN */
    zoo_set_op_init( &sOp[2],
                     sPath[2],
                     sBuffer,
                     idlOS::strlen( sBuffer ),
                     -1,
                     NULL );

    /* state */
    zoo_set_op_init( &sOp[3],
                     sPath[3],
                     SDI_ZKC_STATE_RUN,
                     idlOS::strlen( SDI_ZKC_STATE_RUN ),
                     -1,
                     NULL );

    /* Target Node state */
    zoo_set_op_init( &sOp[4],
                     sPath[4],
                     SDI_ZKC_STATE_KILL,
                     idlOS::strlen( SDI_ZKC_STATE_KILL ),
                     -1,
                     NULL );

                    
    /* multi op를 수행해 한번에 변경한다. */
    /* state 변경과 lock 해제까지 모두 수행해야 한다. */    
    sResult = aclZKC_multiOp( mZKHandler, 5, sOp, sOpResult );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for( i = 0; i < 5; i++ )
    {
        IDE_TEST_RAISE( sOpResult[i].err != ZOK, err_ZKC );
    }

    /* RP Stop이 필요하다면 Stop 한다. */
    if ( idlOS::strncmp( mFirstStopNodeName,
                         SDM_NA_STR,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
    {
        idlOS::snprintf( sSqlStr, 128,
                         "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP", 
                         mFirstStopReplName );

        IDE_TEST( sdi::shardExecTempSQLWithoutSession( sSqlStr,
                                                       mFirstStopNodeName,
                                                       0,
                                                       QCI_STMT_MASK_MAX )
                  != IDE_SUCCESS );
    }

    if ( idlOS::strncmp( mSecondStopNodeName,
                         SDM_NA_STR,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
    {
        idlOS::snprintf( sSqlStr, 128,
                         "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP", 
                         mSecondStopReplName );

        IDE_TEST( sdi::shardExecTempSQLWithoutSession( sSqlStr,
                                                       mSecondStopNodeName,
                                                       0,
                                                       QCI_STMT_MASK_MAX )
                  != IDE_SUCCESS );
    }

    /* Stop 끝났으면 초기화. */
    idlOS::strncpy( mFirstStopNodeName,
                    SDM_NA_STR,
                    SDI_NODE_NAME_MAX_SIZE + 1 );
    idlOS::strncpy( mSecondStopNodeName,
                    SDM_NA_STR,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    idlOS::strncpy( mFirstStopReplName,
                    SDM_NA_STR,
                    SDI_REPLICATION_NAME_MAX_SIZE + 1 );
    idlOS::strncpy( mSecondStopReplName,
                    SDM_NA_STR,
                    SDI_REPLICATION_NAME_MAX_SIZE + 1 );

    idlOS::strncpy( mFailoverNodeName,
                    SDM_NA_STR,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_info )
    {
        /* failover history가 128자를 넘겼을 경우 */
        IDE_SET( ideSetErrorCode( sdERR_ABORT_ZKC_FAILOVER_HISTORY_TOO_LONG ) );
    }
    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::failoverAfterRollback          *
 * ------------------------------------------------------------------*
 * FAILOVER 구문이 Rollback 된 후 호출된다.
 * FailoverNode의 State 와 mFailoverNodeName을 초기화 해야한다.
 * mRunningJobType/mTargetNodeName 은 releaseZookeeperMetaLock에서 처리한다.
 * Create한 RP가 있으면 DROP 해야한다.
 *
 *********************************************************************/
IDE_RC sdiZookeeper::failoverAfterRollback()
{
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    IDE_DASSERT( mIsGetZookeeperLock == ID_TRUE );
    IDE_DASSERT( ( mRunningJobType == ZK_JOB_FAILOVER ) || ( mRunningJobType == ZK_JOB_FAILOVER_FOR_WATCHER ) );
    IDE_DASSERT( idlOS::strlen( mTargetNodeName ) > 0 );

    idlOS::strncpy( mFailoverNodeName,
                    SDM_NA_STR,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    IDE_TEST( validateState( ZK_RUN,
                             mMyNodeName ) != IDE_SUCCESS );

    idlOS::strncpy( sBuffer,
                    SDI_ZKC_STATE_RUN,
                    SDI_ZKC_BUFFER_SIZE );

    IDE_TEST( updateState( sBuffer,
                           mMyNodeName ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    idlOS::strncpy( mFailoverNodeName,
                    SDM_NA_STR,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::failback                     *
 * ------------------------------------------------------------------*
 * FAILBACK을 수행하고 그에 맞춰 클러스터 메타를 수정한다.
 * 해당 함수는 ALTER DATABASE SHARD FAILBACK 구문에 의해 호출된다.
 * 해당 함수에서는 내 노드의 state를 FAILBACK으로 바꾸는 작업만 수행한다.
 * 나머지 메타를 변경시키는 작업은 commit시 호출되는 failbackAfterCommit에서 수행된다.
 *********************************************************************/
IDE_RC sdiZookeeper::failback()
{
    ZKCResult   sResult;
    SChar       sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    /* 내 state를 FAILBACK으로 변경한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    mMyNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[mMyNodeName]/state'의 값을 가진다. */

    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
    idlOS::strncpy( sBuffer,
                    SDI_ZKC_STATE_FAILBACK,
                    SDI_ZKC_BUFFER_SIZE );

    sResult = aclZKC_setInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              idlOS::strlen( sBuffer ) );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* mm 모듈에서 commit시 failbackAfterCommit을 호출할 수 있도록 현재 수행 중인 작업이 failback임을 표시한다. */
    mRunningJobType = ZK_JOB_FAILBACK;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::failbackAfterCommit          *
 * ------------------------------------------------------------------*
 * FAILBACK 작업이 완료되어 commit 된 후 호출된다.
 * failover history / fault detection time / 클러스터의 SMN을 갱신한다.
 *
 * aSMN    - [IN] 변경할 클러스터 메타의 SMN 값
 *********************************************************************/
IDE_RC sdiZookeeper::failbackAfterCommit( ULong aSMN )
{
    ZKCResult       sResult;

    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sPath[4][SDI_ZKC_PATH_LENGTH] = {{0,}};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar           sFailoverHistory[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar         * sCheckBuffer;
    SChar         * sOtherFailoverHistory;
    SInt            sDataLength;

    /* multi op */
    SInt            i = 0;
    zoo_op_t        sOp[4];
    zoo_op_result_t sOpResult[4];

    IDE_DASSERT( mIsGetZookeeperLock == ID_TRUE );
    IDE_DASSERT( mRunningJobType == ZK_JOB_FAILBACK );

    idlOS::strncpy( sNodePath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sNodePath,
                    "/",
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    idlOS::strncat( sNodePath,
                    mMyNodeName,
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    /* sNodePath는 '/altibase_shard/node_meta/[mMyNodeName]'의 값을 가진다. */

    /* 2. failover history에서 자신에 대한 내역을 제거한다. */
    idlOS::strncpy( sPath[1], 
                    SDI_ZKC_PATH_FAILOVER_HISTORY,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[1]은 '/altibase_shard/cluster_meta/failover_history'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath[1],
                              sFailoverHistory,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 내 node가 Failover 되어 있는지 확인한다. */
    idlOS::strncpy( sPath[0],
                    sNodePath, 
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[0], 
                    SDI_ZKC_PATH_NODE_FAILOVER_TO,
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    /* sPath[0]은 '/altibase_shard/node_meta/[mMyNodeName]/failoverTo'의 값을 가진다. */
    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath[0],
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* 내 Node가 Failover 되어 있으면 Failover History도 변경해야 한다. */
    if ( sDataLength > 0 )
    {
        /* failover history는 노드이름1:SMN/노드이름2:SMN/...의 형태로 이루어져 있으므로 
         * 첫 ':'문자의 위치만 알면 가장 최근에 죽은 노드의 이름을 알 수 있다. */   
        sCheckBuffer = idlOS::strtok( sFailoverHistory, ":" );

        /* 자신이 가장 최근에 죽은 노드가 아니라면 failback을 수행할 수 없다. */
        IDE_TEST( isNodeMatch( mMyNodeName, sCheckBuffer ) != ID_TRUE );

        /* failback 작업이 끝난 후 failover history를 갱신하기 위해 받아온 값을 자른다. */
        sOtherFailoverHistory = idlOS::strtok( NULL, "/" );
        sOtherFailoverHistory = idlOS::strtok( NULL, "\0" );
    }
    else
    {
        /* 내 Node는 Failover 되어 있지 않으니 FailoverHistory는 수정하지 않아도 된다. */
        sOtherFailoverHistory = sFailoverHistory;
    }

    /* 3. 메타 변경을 수행할 multi op를 생성한다. */
    idlOS::strncpy( sPath[0],
                    sNodePath, 
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[0], 
                    SDI_ZKC_PATH_NODE_FAILOVER_TO,
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    /* sPath[0]은 '/altibase_shard/node_meta/[mMyNodeName]/failoverTo'의 값을 가진다. */

    zoo_set_op_init( &sOp[0],
                     sPath[0],
                     NULL,
                     -1,
                     -1,
                     NULL );

    idlOS::strncpy( sPath[1], 
                    SDI_ZKC_PATH_FAILOVER_HISTORY, 
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[1]는 '/altibase_shard/cluster_meta/failover_history'의 값을 가진다. */

    /* 내 노드를 제외한 다른 failover 정보가 있다면 해당 정보를 삽입한다. */
    if( sOtherFailoverHistory != NULL )
    {
        zoo_set_op_init( &sOp[1],
                         sPath[1],
                         sOtherFailoverHistory,
                         idlOS::strlen( sOtherFailoverHistory ),
                         -1,
                         NULL );
    }
    else
    {
        /* 내 노드가 죽어있던 유일한 노드라면 빈 값을 삽입한다. */
        zoo_set_op_init( &sOp[1],
                         sPath[1],
                         NULL,
                         -1,
                         -1,
                         NULL );       
    }

    idlOS::strncpy( sPath[2],
                    SDI_ZKC_PATH_SMN,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath[2]은 '/altibase_shard/cluster_meta/SMN'의 값을 가진다. */

    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
    (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT64_FMT, aSMN );

    zoo_set_op_init( &sOp[2],
                     sPath[2],
                     sBuffer,
                     idlOS::strlen( sBuffer ),
                     -1,
                     NULL );

    idlOS::strncpy( sPath[3],
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[3],
                    "/",
                    ID_SIZEOF( sPath[3] ) - idlOS::strlen( sPath[3] ) - 1 );
    idlOS::strncat( sPath[3],
                    mMyNodeName,
                    ID_SIZEOF( sPath[3] ) - idlOS::strlen( sPath[3] ) - 1 );
    idlOS::strncat( sPath[3],
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath[3] ) - idlOS::strlen( sPath[3] ) - 1 );
    /* sPath[3]은 '/altibase_shard/node_meta/[myNodeName]/state'의 값을 가진다. */

    zoo_set_op_init( &sOp[3],
                     sPath[3],
                     SDI_ZKC_STATE_RUN,
                     idlOS::strlen( SDI_ZKC_STATE_RUN ),
                     -1,
                     NULL );   

    /* 4. multi op를 수행한다. */
    sResult = aclZKC_multiOp( mZKHandler, 4, sOp, sOpResult );

    /* multi op로 수행한 각 operation의 정상 수행 여부를 확인한다. */
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for( i = 0; i <= 3; i++ )
    {
        IDE_TEST_RAISE( sOpResult[i].err != ZOK, err_ZKC );
    }

    /* 5. 자신이 유일하게 죽어있던 노드라면 fault detection time도 정리해야 한다. */
    if( sOtherFailoverHistory == NULL )
    {
        idlOS::memset( sPath[0],
                       0,
                       SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sPath[0], 
                        SDI_ZKC_PATH_FAULT_DETECT_TIME,
                        SDI_ZKC_PATH_LENGTH );
        sResult = aclZKC_deleteInfo( mZKHandler,
                                     sPath[0] );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    } 

    /* 6. 연결 정보를 생성한다. */
    IDE_TEST( addConnectionInfo() != IDE_SUCCESS );

    /* 7. watch를 건다. */
    IDE_TEST( settingAddNodeWatch() != IDE_SUCCESS );
    IDE_TEST( settingAllDeleteNodeWatch() != IDE_SUCCESS );

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FAILBACK After Commit Complete");

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Failed FAILBACK After Commit");

    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::updateSMN                    *
 * ------------------------------------------------------------------*
 * 클러스터 메타에 저장된 SMN 값을 갱신한다.
 * 해당 함수는 메타를 변경하는 작업이 끝난 후 호출되어야 한다.
 *
 * aSMN    [IN] - 갱신할 SMN 값
 *********************************************************************/
IDE_RC sdiZookeeper::updateSMN( ULong aSMN )
{
    ZKCResult       sResult;
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    IDE_TEST_RAISE( mIsConnect == ID_FALSE, err_not_connect );
    /* 1. path를 세팅한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_SMN,
                    SDI_ZKC_PATH_LENGTH );

    (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT64_FMT, aSMN );
    /* sPath는 '/altibase_shard/cluster_meta/SMN'의 값을 가진다. */

    /* 3. zookeeper server의 SMN을 변경한다. */
    sResult = aclZKC_setInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              idlOS::strlen( sBuffer ) );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS ,err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_not_connect )
    {
        logAndSetErrMsg( ZKC_CONNECTION_FAIL );
    }
    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::updateSMN                    *
 * ------------------------------------------------------------------*
 * preparing for update SMN
 * aSMN    [IN] - 갱신할 SMN 값
 *********************************************************************/
IDE_RC sdiZookeeper::prepareForUpdateSMN( ULong aSMN )
{
    ZKCResult       sResult;
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    IDE_TEST_RAISE( mIsConnect == ID_FALSE, err_not_connect );
    IDE_TEST_RAISE(mMyNodeName[0] == '\0', ERR_NOT_EXIST_MYNODENAME);
    
    /* 1. path를 세팅한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_SMN,
                    SDI_ZKC_PATH_LENGTH );
    
    (void)idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%s:%"ID_UINT64_FMT, mMyNodeName, aSMN );
    /* sPath는 '/altibase_shard/cluster_meta/SMN'의 값을 가진다. NODENAME:SMNValue*/
    
    /* 3. zookeeper server의 SMN을 temporary 변경한다. */
    sResult = aclZKC_setInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              idlOS::strlen( sBuffer ) );
    
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS ,err_ZKC );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_not_connect )
    {
        logAndSetErrMsg( ZKC_CONNECTION_FAIL );
    }
    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_MYNODENAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdiZookeeper::updateSMN",
                                  "NOT EXIST MY NODE NAME " ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::updateState                  *
 * ------------------------------------------------------------------*
 * zookeeper server 에 저장된 내 노드의 state를 변경한다.
 * 해당 함수에서는 state의 validate를 수행하지 않으므로 validate가 필요하다면
 * 상위에서 직접 validateState 함수를 호출해서 확인해야 한다.
 *
 * aState    [IN] - 갱신할 state
 *********************************************************************/
IDE_RC sdiZookeeper::updateState( SChar     * aState,
                                  SChar     * aTargetNodeName )
{
    ZKCResult       sResult;
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};

    /* 1. path를 설정한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aTargetNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[aTargetNodeName]/state'의 값을 가진다. */

    /* 3. zookeeper server에 저장된 내 노드의 state를 변경한다. */
    sResult = aclZKC_setInfo( mZKHandler,
                              sPath,
                              aState,
                              idlOS::strlen( aState ) );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getAllNodeNameList           *
 * ------------------------------------------------------------------*
 * 클러스터의 모든 노드의 이름 리스트를 이름순으로 정렬하여 가져온다.
 * 죽어있거나 SHUTDOWN된 노드도 클러스터에 추가되었다면 대상이 된다.
 *
 * aList    [OUT] - 정렬된 노드의 리스트
 *********************************************************************/
IDE_RC sdiZookeeper::getAllNodeNameList( iduList ** aList )
{
    ZKCResult       sResult;
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    String_vector   sChildren;
    SInt            i = 0;
    idBool          sIsAlloced = ID_FALSE;
    SInt            sState = 0;

    iduList       * sList     = NULL;
    iduListNode   * sNode     = NULL;
    SChar         * sNodeName = NULL;

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/node_meta'의 값을 가진다. */

    /* 1. 노드 메타의 각 children을 가져온다. */
    sResult = aclZKC_getChildren( mZKHandler,
                                  sPath,
                                  &sChildren );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    IDE_TEST_RAISE( sChildren.count <= 0, err_no_node );

    /* list를 초기화 한다. */
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                 ID_SIZEOF( iduList ),
                                 (void**)&( sList ),
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
    IDU_LIST_INIT( sList );

    sIsAlloced = ID_TRUE;

    /* 2. 노드를 할당하고 ZKS에서 가져온 데이터를 넣는다. */
    for( i = 0; sChildren.count > i; i++ )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     ID_SIZEOF(iduListNode),
                                     (void**)&(sNode),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     SDI_NODE_NAME_MAX_SIZE + 1,
                                     (void**)&(sNodeName),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 2;

        idlOS::strncpy( sNodeName,
                        sChildren.data[i], 
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        IDU_LIST_INIT_OBJ( sNode, (void*)sNodeName );

        IDU_LIST_ADD_FIRST( sList, sNode );

        sState = 0;
    }

    /* 3. 노드 이름 순으로 정렬한다. */

    /* 노드가 최소 2개 이상이어야지 정렬이 의미가 있다. */
    if( sChildren.count > 1)
    {
        sortNode( sList );
    }

    *aList = sList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION( err_no_node )
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDI_ZKC_NO_NODE));
    }
    IDE_EXCEPTION_END;

    /* list에 아직 추가되지 못한 메모리 할당을 정리한다. */   
    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free( sNodeName );
        case 1:
            (void)iduMemMgr::free( sNode );
        case 0:
            break;
        default:
            break;
    }
 
    if( sIsAlloced == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getAllNodeInfoList           *
 * ------------------------------------------------------------------*
 * 클러스터의 모든 노드의 리스트의 정보를 이름순으로 정렬해서 가져온다.
 * 만약 해당 노드가 첫 노드라 다른 노드가 없을 경우에는 aList에 NULL을 세팅한다.
 * getAllNodeNameList 함수와는 달리 노드의 이름 뿐 아니라 다른 정보도 가져온다.
 * list를 위해 자체적으로 메모리를 할당해 사용한다.
 *
 * aList    [OUT] - 정렬된 노드의 리스트
 * aNodeCnt [OUT] - 현재 존재하는 노드의 숫자
 *********************************************************************/
IDE_RC sdiZookeeper::getAllNodeInfoList( iduList ** aList,
                                         UInt     * aNodeCnt )
{
    ZKCResult       sResult;
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sSubPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    String_vector   sChildren;
    SInt            i = 0;
    SInt            sDataLength = 0;
    SChar         * sStringPtr = NULL;
    idBool          sIsAlloced = ID_FALSE;
    SInt            sState = 0;

    iduList             * sList     = NULL;
    iduListNode         * sNode     = NULL;
    sdiLocalMetaInfo    * sNodeInfo = NULL;

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/node_meta'의 값을 가진다. */

    /* 1. 노드 메타의 각 children을 가져온다. */
    sResult = aclZKC_getChildren( mZKHandler,
                                  sPath,
                                  &sChildren );

    if( sResult == ZKC_NO_NODE )
    {
        /* 클러스터 메타가 존재하지 않는 경우 연결된 노드가 하나도 없다. */
        *aList = NULL;
        *aNodeCnt = 0;
        IDE_CONT( normal_exit );
    }

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    if( sChildren.count < 1 )
    {
        /* 현재 연결된 노드가 하나도 없을 경우 */
        *aList = NULL;
        *aNodeCnt = 0;
        IDE_CONT( normal_exit );
    }

    IDE_DASSERT( sChildren.count > 0 );

    /* list를 초기화 한다. */
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                 ID_SIZEOF(iduList),
                                 (void**)&(sList),
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
    IDU_LIST_INIT( sList );

    sIsAlloced = ID_TRUE;

    /* 2. 각 노드에게서 자식을 가져와 정보를 꺼낸다. */
    for( i = 0; sChildren.count > i; i++ )
    {
        /* 2.1 노드 리스트에 사용할 공간을 할당 받는다. */
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     ID_SIZEOF(iduListNode),
                                     (void**)&(sNode),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     ID_SIZEOF(sdiLocalMetaInfo),
                                     (void**)&(sNodeInfo),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 2;

        /* 2.2  구조체용으로 할당 받은 공간을 초기화 */
        sNodeInfo->mShardNodeId = 0;
        idlOS::memset( sNodeInfo->mShardedDBName, 0, SDI_SHARDED_DB_NAME_MAX_SIZE + 1 );
        idlOS::memset( sNodeInfo->mNodeName, 0, SDI_NODE_NAME_MAX_SIZE + 1 );
        idlOS::memset( sNodeInfo->mHostIP, 0, SDI_SERVER_IP_SIZE + 1 );
        sNodeInfo->mPortNo = 0;
        idlOS::memset( sNodeInfo->mInternalHostIP, 0, SDI_SERVER_IP_SIZE + 1 );
        sNodeInfo->mInternalPortNo = 0;
        idlOS::memset( sNodeInfo->mInternalRPHostIP, 0, SDI_SERVER_IP_SIZE + 1 );
        sNodeInfo->mInternalRPPortNo = 0;
        sNodeInfo->mInternalConnType = 0;

        /* 2.3 노드 이름 삽입 */
        idlOS::strncpy( sNodeInfo->mNodeName,
                        sChildren.data[i],
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        /* sNodePath 세팅 */
        idlOS::memset( sNodePath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sNodePath,
                        SDI_ZKC_PATH_NODE_META,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sNodePath,
                        "/",
                        ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
        idlOS::strncat( sNodePath,
                        sChildren.data[i],
                        ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
        /* sNodePath는 '/altibase_shard/node_meta/[sChildren.data[i]]'의 값을 가진다. */

        /* 2.4 Shard Node ID 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_ID,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/shard_node_id'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        sNodeInfo->mShardNodeId = idlOS::atoi( sBuffer );

        /* mShardedDBName는 삽입하지 않는다. */

        /* 2.5 Node IP 및 Port 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_IP,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/node_ip:port'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        /* IP와 port를 분리한다. */
        sStringPtr = idlOS::strtok( sBuffer, ":" );
        idlOS::strncpy( sNodeInfo->mHostIP,
                        sStringPtr,
                        SDI_SERVER_IP_SIZE + 1 );

        sStringPtr = idlOS::strtok( NULL, "\0" );
        sNodeInfo->mPortNo = idlOS::atoi( sStringPtr );

        /* 2.6 Internal Node IP 및 Port 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_INTERNAL_IP,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/internal_node_ip:port'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        /* IP와 port를 분리한다. */
        sStringPtr = idlOS::strtok( sBuffer, ":" );
        idlOS::strncpy( sNodeInfo->mInternalHostIP,
                        sStringPtr,
                        SDI_SERVER_IP_SIZE );

        sStringPtr = idlOS::strtok( NULL, "\0" );
        sNodeInfo->mInternalPortNo = idlOS::atoi( sStringPtr );

        /* 2.7 Replication 내부 IP 및 Port 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_REPL_HOST_IP,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/internal_replication_host_ip:port'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        /* IP와 port를 분리한다. */
        sStringPtr = idlOS::strtok( sBuffer, ":" );
        idlOS::strncpy( sNodeInfo->mInternalRPHostIP,
                        sStringPtr,
                        SDI_SERVER_IP_SIZE );

        sStringPtr = idlOS::strtok( NULL, "\0" );
        sNodeInfo->mInternalRPPortNo = idlOS::atoi( sStringPtr );

        /* 2.8 연결 방식 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_CONN_TYPE,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/conn_type'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        sNodeInfo->mInternalConnType = idlOS::atoi( sBuffer );

        /* 2.9 노드 리스트에 데이터를 삽입한다. */
        IDU_LIST_INIT_OBJ( sNode, (void*)sNodeInfo );

        IDU_LIST_ADD_FIRST( sList, sNode );

        sState = 0;
    }

    /* 3. 노드 이름 순으로 정렬한다. */
    /* 노드가 최소 2개 이상이어야지 정렬이 의미가 있다. */
    if( sChildren.count > 1 )
    {
        sortNode( sList );
    }

    *aList = sList;
    *aNodeCnt = sChildren.count;

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    /* list에 아직 추가되지 못한 메모리 할당을 정리한다. */
    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free( sNodeInfo );
        case 1:
            (void)iduMemMgr::free( sNode );
        case 0:
            break;
        default:
            break;
    }

    if( sIsAlloced == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODEINFO );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getAllNodeInfoList           *
 * ------------------------------------------------------------------*
 * 클러스터의 모든 노드의 리스트의 정보를 이름순으로 정렬해서 가져온다.
 * 만약 해당 노드가 첫 노드라 다른 노드가 없을 경우에는 aList에 NULL을 세팅한다.
 * getAllNodeNameList 함수와는 달리 노드의 이름 뿐 아니라 다른 정보도 가져온다.
 *
 * aList    [OUT] - 정렬된 노드의 리스트
 * aNodeCnt [OUT] - 현재 존재하는 노드의 숫자
 * aMem     [IN]  - 리스트를 할당받을 메모리
 *********************************************************************/
IDE_RC sdiZookeeper::getAllNodeInfoList( iduList  ** aList,
                                         UInt      * aNodeCnt,
                                         iduMemory * aMem )
{
    ZKCResult       sResult;
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sSubPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    String_vector   sChildren;
    SInt            i = 0;
    SInt            sDataLength = 0;
    SChar         * sStringPtr = NULL;

    iduList             * sList     = NULL;
    iduListNode         * sNode     = NULL;
    sdiLocalMetaInfo    * sNodeInfo = NULL;

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/node_meta'의 값을 가진다. */

    /* 1. 노드 메타의 각 children을 가져온다. */
    sResult = aclZKC_getChildren( mZKHandler,
                                  sPath,
                                  &sChildren );

    if( sResult == ZKC_NO_NODE )
    {
        /* 클러스터 메타가 존재하지 않는 경우 연결된 노드가 하나도 없다. */
        *aList = NULL;
        *aNodeCnt = 0;
        IDE_CONT( normal_exit );
    }

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    if( sChildren.count < 1 )
    {
        /* 현재 연결된 노드가 하나도 없을 경우 */
        *aList = NULL;
        *aNodeCnt = 0;
        IDE_CONT( normal_exit );
    }

    IDE_DASSERT( sChildren.count > 0 );

    /* list를 초기화 한다. */
    IDE_TEST( aMem->alloc( ID_SIZEOF(iduList),
                          (void**)&(sList) )
              != IDE_SUCCESS );
    IDU_LIST_INIT( sList );

    /* 2. 각 노드에게서 자식을 가져와 정보를 꺼낸다. */
    for( i = 0; sChildren.count > i; i++ )
    {
        /* 2.1 노드 리스트에 사용할 공간을 할당 받는다. */
        IDE_TEST( aMem->alloc( ID_SIZEOF(iduListNode),
                               (void**)&(sNode) )
                  != IDE_SUCCESS );

        IDE_TEST( aMem->alloc( ID_SIZEOF(sdiLocalMetaInfo),
                               (void**)&(sNodeInfo) )
                  != IDE_SUCCESS );

        /* 2.2  구조체용으로 할당 받은 공간을 초기화 */
        sNodeInfo->mShardNodeId = 0;
        idlOS::memset( sNodeInfo->mShardedDBName, 0, SDI_SHARDED_DB_NAME_MAX_SIZE + 1 );
        idlOS::memset( sNodeInfo->mNodeName, 0, SDI_NODE_NAME_MAX_SIZE + 1 );
        idlOS::memset( sNodeInfo->mHostIP, 0, SDI_SERVER_IP_SIZE + 1 );
        sNodeInfo->mPortNo = 0;
        idlOS::memset( sNodeInfo->mInternalHostIP, 0, SDI_SERVER_IP_SIZE + 1 );
        sNodeInfo->mInternalPortNo = 0;
        idlOS::memset( sNodeInfo->mInternalRPHostIP, 0, SDI_SERVER_IP_SIZE + 1 );
        sNodeInfo->mInternalRPPortNo = 0;
        sNodeInfo->mInternalConnType = 0;

        /* 2.3 노드 이름 삽입 */
        idlOS::strncpy( sNodeInfo->mNodeName,
                        sChildren.data[i],
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        /* sNodePath 세팅 */
        idlOS::memset( sNodePath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sNodePath,
                        SDI_ZKC_PATH_NODE_META,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sNodePath,
                        "/",
                        ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
        idlOS::strncat( sNodePath,
                        sChildren.data[i],
                        ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
        /* sNodePath는 '/altibase_shard/node_meta/[sChildren.data[i]]'의 값을 가진다. */

        /* 2.4 Shard Node ID 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_ID,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/shard_node_id'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        sNodeInfo->mShardNodeId = idlOS::atoi( sBuffer );

        /* mShardedDBName는 삽입하지 않는다. */

        /* 2.5 Node IP 및 Port 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_IP,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/node_ip:port'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        /* IP와 port를 분리한다. */
        sStringPtr = idlOS::strtok( sBuffer, ":" );
        idlOS::strncpy( sNodeInfo->mHostIP,
                        sStringPtr,
                        SDI_SERVER_IP_SIZE + 1 );

        sStringPtr = idlOS::strtok( NULL, "\0" );
        sNodeInfo->mPortNo = idlOS::atoi( sStringPtr );

        /* 2.6 Internal Node IP 및 Port 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_INTERNAL_IP,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/internal_node_ip:port'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        /* IP와 port를 분리한다. */
        sStringPtr = idlOS::strtok( sBuffer, ":" );
        idlOS::strncpy( sNodeInfo->mInternalHostIP,
                        sStringPtr,
                        SDI_SERVER_IP_SIZE );

        sStringPtr = idlOS::strtok( NULL, "\0" );
        sNodeInfo->mInternalPortNo = idlOS::atoi( sStringPtr );

        /* 2.7 Replication 내부 IP 및 Port 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_REPL_HOST_IP,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/internal_replication_host_ip:port'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        /* IP와 port를 분리한다. */
        sStringPtr = idlOS::strtok( sBuffer, ":" );
        idlOS::strncpy( sNodeInfo->mInternalRPHostIP,
                        sStringPtr,
                        SDI_SERVER_IP_SIZE );

        sStringPtr = idlOS::strtok( NULL, "\0" );
        sNodeInfo->mInternalRPPortNo = idlOS::atoi( sStringPtr );

        /* 2.8 연결 방식 획득 및 복사 */
        idlOS::memset( sSubPath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sSubPath,
                        sNodePath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sSubPath,
                        SDI_ZKC_PATH_NODE_CONN_TYPE,
                        ID_SIZEOF( sSubPath ) - idlOS::strlen( sSubPath ) - 1 );
        /* sSubPath는 '/altibase_shard/node_meta/[sChildren.data[i]]/conn_type'의 값을 가진다. */

        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        sResult = aclZKC_getInfo( mZKHandler,
                                  sSubPath,
                                  sBuffer,
                                  &sDataLength );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        sNodeInfo->mInternalConnType = idlOS::atoi( sBuffer );

        /* 2.9 노드 리스트에 데이터를 삽입한다. */
        IDU_LIST_INIT_OBJ( sNode, (void*)sNodeInfo );

        IDU_LIST_ADD_FIRST( sList, sNode );
    }

    /* 3. 노드 이름 순으로 정렬한다. */
    /* 노드가 최소 2개 이상이어야지 정렬이 의미가 있다. */
    if( sChildren.count > 1 )
    {
        sortNode( sList );
    }

    *aList = sList;
    *aNodeCnt = sChildren.count;

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getAliveNodeNameList         *
 * ------------------------------------------------------------------*
 * 클러스터의 모든 연결중인 노드의 이름 리스트를 이름순으로 정렬하여 가져온다.
 * 죽어있거나 SHUTDOWN된 노드는 클러스터에 추가되어도 대상이 되지 않는다.
 *
 * aList    [OUT] - 정렬된 노드의 리스트
 *********************************************************************/
IDE_RC sdiZookeeper::getAliveNodeNameList( iduList ** aList )
{
    ZKCResult       sResult;
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    String_vector   sChildren;
    SInt            i = 0;
    idBool          sIsAlloced = ID_FALSE;
    SInt            sState = 0;

    iduList       * sList     = NULL;
    iduListNode   * sNode     = NULL;
    SChar         * sNodeName = NULL;

    IDE_TEST_RAISE( sdiZookeeper::isConnect() != ID_TRUE, ERR_NOT_CONNECT );

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_CONNECTION_INFO, 
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/connection_info'의 값을 가진다. */

    /* 1. connection_info에는 살아있는 노드만 있으므로 여기서 children을 가져온다. */
    sResult = aclZKC_getChildren( mZKHandler,
                                  sPath,
                                  &sChildren );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    IDE_TEST_RAISE( sChildren.count <= 0, ERR_NODE_NOT_EXIST );

    /* list를 초기화 한다. */
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                 ID_SIZEOF(iduList),
                                 (void**)&(sList),
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
    IDU_LIST_INIT( sList );

    sIsAlloced = ID_TRUE;

    /* 2. 노드를 할당하고 ZKS에서 가져온 데이터를 넣는다. */
    for( i = 0; sChildren.count > i; i++ )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     ID_SIZEOF(iduListNode),
                                     (void**)&(sNode),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     SDI_NODE_NAME_MAX_SIZE + 1,
                                     (void**)&(sNodeName),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 2;

        idlOS::strncpy( sNodeName, 
                        sChildren.data[i], 
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        /* list에 삽입한다. */
        IDU_LIST_INIT_OBJ( sNode, (void*)sNodeName );

        IDU_LIST_ADD_FIRST( sList, sNode );

        sState = 0;
    }

    /* 3. 노드 이름 순으로 정렬한다. */

    /* 노드가 최소 2개 이상이어야지 정렬이 의미가 있다. */
    if( sChildren.count > 1)
    {
        sortNode( sList );
    }

    *aList = sList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdiZookeeper::getAliveNodeNameList",
                                  "znode not exist" ) );
    }
    IDE_EXCEPTION( ERR_NOT_CONNECT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdiZookeeper::getAliveNodeNameList",
                                  "The shard node has not yet been joined to the sharding cluster." ) );
    }
    IDE_EXCEPTION_END;

    /* list에 아직 추가되지 못한 메모리 할당을 정리한다. */
    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free( sNodeName );
        case 1:
            (void)iduMemMgr::free( sNode );
        case 0:
            break;
        default:
            break;
    }

    if( sIsAlloced == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getAliveNodeNameListIncludeNodeName *
 * ------------------------------------------------------------------*
 * 클러스터의 모든 Alive 중인 노드의 이름 리스트에 특정 Node를 포함시켜서 이름순으로 정렬하여 가져온다.
 * 죽어있거나 SHUTDOWN된 노드는 클러스터에 추가되어도 대상이 되지 않는다.
 *
 * aList    [OUT] - 정렬된 노드의 리스트
 *********************************************************************/
IDE_RC sdiZookeeper::getAliveNodeNameListIncludeNodeName( iduList ** aList,
                                                          SChar    * aNodeName )
{
    ZKCResult       sResult;
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    String_vector   sChildren;
    SInt            i = 0;
    idBool          sIsAlloced = ID_FALSE;
    SInt            sState = 0;
    idBool          sTargetNodeAlived = ID_FALSE;

    iduList       * sList     = NULL;
    iduListNode   * sNode     = NULL;
    SChar         * sNodeName = NULL;

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_CONNECTION_INFO, 
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/connection_info'의 값을 가진다. */

    /* 1. connection_info에는 살아있는 노드만 있으므로 여기서 children을 가져온다. */
    sResult = aclZKC_getChildren( mZKHandler,
                                  sPath,
                                  &sChildren );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    IDE_DASSERT( sChildren.count > 0 );

    /* list를 초기화 한다. */
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                 ID_SIZEOF(iduList),
                                 (void**)&(sList),
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
    IDU_LIST_INIT( sList );

    sIsAlloced = ID_TRUE;

    /* 2. 노드를 할당하고 ZKS에서 가져온 데이터를 넣는다. */
    for( i = 0; sChildren.count > i; i++ )
    {
        if ( idlOS::strncmp( aNodeName,
                             sChildren.data[i],
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            /* Alive List에 지정된 NodeName이 있으면 확인해 둔다. */
            sTargetNodeAlived = ID_TRUE;
        }

        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     ID_SIZEOF(iduListNode),
                                     (void**)&(sNode),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     SDI_NODE_NAME_MAX_SIZE + 1,
                                     (void**)&(sNodeName),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 2;

        idlOS::strncpy( sNodeName, 
                        sChildren.data[i], 
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        /* list에 삽입한다. */
        IDU_LIST_INIT_OBJ( sNode, (void*)sNodeName );

        IDU_LIST_ADD_FIRST( sList, sNode );

        sState = 0;
    }

    if ( sTargetNodeAlived == ID_FALSE )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     ID_SIZEOF(iduListNode),
                                     (void**)&(sNode),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                     SDI_NODE_NAME_MAX_SIZE + 1,
                                     (void**)&(sNodeName),
                                     IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
        sState = 2;

        idlOS::strncpy( sNodeName, 
                        aNodeName, 
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        /* list에 삽입한다. */
        IDU_LIST_INIT_OBJ( sNode, (void*)sNodeName );

        IDU_LIST_ADD_FIRST( sList, sNode );

        sState = 0;
    }

    /* 3. 노드 이름 순으로 정렬한다. */

    /* 노드가 최소 2개 이상이어야지 정렬이 의미가 있다. */
    if( sChildren.count > 1)
    {
        sortNode( sList );
    }

    *aList = sList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    /* list에 아직 추가되지 못한 메모리 할당을 정리한다. */
    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free( sNodeName );
        case 1:
            (void)iduMemMgr::free( sNode );
        case 0:
            break;
        default:
            break;
    }

    if( sIsAlloced == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getNextNode                  *
 * ------------------------------------------------------------------*
 * 특정 노드의 다음 노드(이름순 정렬 기준)의 이름과 해당 노드의 상태를 가져온다.
 * 입력받은 노드가 이름순 정렬의 마지막 노드일 경우
 * 이름순 정렬의 첫 노드가 반환 대상이 된다.
 * 죽어있거나 SHUTDOWN된 노드도 탐색 대상이 된다.
 *
 * aTargetName [IN]  - 다음 노드를 찾을 대상 노드의 이름
 * aReturnName [OUT] - 대상 노드의 다음 노드의 이름
 * aNodeState  [OUT] - 대상 노드의 다음 노드의 상태
 *********************************************************************/
IDE_RC sdiZookeeper::getNextNode( SChar   * aTargetName, 
                                  SChar   * aReturnName, 
                                  ZKState * aNodeState )
{
    ZKCResult         sResult;
    iduList         * sList = NULL;
    iduListNode     * sIterator = NULL;
    SChar             sBuffer[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SChar             sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    idBool            sIsAllocList = ID_FALSE;
    idBool            sIsFound = ID_FALSE;
    SInt              sDataLength;

    IDE_TEST( getAllNodeNameList ( &sList ) != IDE_SUCCESS );
    sIsAllocList = ID_TRUE;

    IDU_LIST_ITERATE( sList, sIterator )
    {
        idlOS::memset( sBuffer, 0, SDI_NODE_NAME_MAX_SIZE + 1 );
        idlOS::strncpy( sBuffer,
                        (SChar*)sIterator->mObj,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        /* 리스트에서 목표 노드를 찾을 경우 해당 노드의 다음 노드의 state를 가져온다. */
        if( isNodeMatch( sBuffer, aTargetName ) == ID_TRUE )
        {
            sIsFound = ID_TRUE;

            idlOS::memset( sBuffer, 0, SDI_NODE_NAME_MAX_SIZE + 1 );

            if( sIterator->mNext != sList )
            {
                /* list의 마지막 노드가 아니라면 다음 노드가 탐색 대상이다. */
                idlOS::strncpy( sBuffer, 
                                (SChar*)sIterator->mNext->mObj, 
                                SDI_NODE_NAME_MAX_SIZE + 1 );
            }
            else
            {
                /* list의 마지막 노드일 경우 첫 노드가 탐색 대상이다. */
                idlOS::strncpy( sBuffer, 
                                (SChar*)sList->mNext->mObj, 
                                SDI_NODE_NAME_MAX_SIZE + 1 ); 
            }

            idlOS::strncpy( aReturnName,
                            sBuffer,
                            SDI_NODE_NAME_MAX_SIZE + 1 );
            idlOS::strncpy( sPath,
                            SDI_ZKC_PATH_NODE_META, 
                            SDI_ZKC_PATH_LENGTH );
            idlOS::strncat( sPath,
                            "/",
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
            idlOS::strncat( sPath,
                            sBuffer,
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
            idlOS::strncat( sPath,
                            SDI_ZKC_PATH_NODE_STATE,
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
            /* sPath는 '/altibase_shard/node_meta/[sBuffer]/state'의 값을 가진다. */

            sDataLength = SDI_NODE_NAME_MAX_SIZE + 1;
            sResult = aclZKC_getInfo( mZKHandler,
                                      sPath,
                                      sBuffer,
                                      &sDataLength );    

            IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

            *aNodeState = stateStringConvert( sBuffer );

            IDE_TEST( *aNodeState == ZK_ERROR );

            break;
        }
    }

    sIsAllocList = ID_FALSE;
    freeList( sList, SDI_ZKS_LIST_NODENAME );

    IDE_TEST_RAISE( sIsFound == ID_FALSE, err_Meta );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION( err_Meta )
    {
        logAndSetErrMsg( ZKC_META_CORRUPTED );
    }
    IDE_EXCEPTION_END;

    if( sIsAllocList == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getPrevNode                  *
 * ------------------------------------------------------------------*
 * 특정 노드의 이전 노드(이름순 정렬 기준)의 이름과 해당 노드의 상태를 가져온다.
 * 입력받은 노드가 이름순 정렬의 첫 노드일 경우
 * 이름순 정렬의 마지막 노드가 반환 대상이 된다.
 * 죽어있거나 SHUTDOWN된 노드도 탐색 대상이 된다.
 *
 * aTargetName [IN]  - 이전 노드를 찾을 대상 노드의 이름
 * aReturnName [OUT] - 대상 노드의 이전 노드의 이름
 * aNodeState  [OUT] - 대상 노드의 이전 노드의 상태
 *********************************************************************/
IDE_RC sdiZookeeper::getPrevNode( SChar   * aTargetName, 
                                  SChar   * aReturnName, 
                                  ZKState * aNodeState )
{
    ZKCResult         sResult;
    iduList         * sList = NULL;
    iduListNode     * sIterator = NULL;
    SChar             sBuffer[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SChar             sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    idBool            sIsAllocList = ID_FALSE;
    idBool            sIsFound = ID_FALSE;
    SInt              sDataLength;

    IDE_TEST( getAllNodeNameList ( &sList ) != IDE_SUCCESS );
    sIsAllocList = ID_TRUE;

    IDU_LIST_ITERATE( sList, sIterator )
    {
        idlOS::memset( sBuffer, 0, SDI_NODE_NAME_MAX_SIZE + 1 );
        idlOS::strncpy( sBuffer,
                        (SChar*)sIterator->mObj,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        if( isNodeMatch( sBuffer, aTargetName ) == ID_TRUE )
        {
            sIsFound = ID_TRUE;

            idlOS::memset( sBuffer, 0, SDI_NODE_NAME_MAX_SIZE + 1 );

            if( sIterator->mPrev != sList )
            {
                /* list의 첫 노드가 아니라면 바로 이전 노드가 탐색 대상이다. */
                idlOS::strncpy( sBuffer,
                                (SChar*)sIterator->mPrev->mObj,
                                SDI_NODE_NAME_MAX_SIZE + 1 );
            }
            else
            {
                /* list의 첫 노드라면 list의 마지막 노드가 탐색 대상이다. */
                idlOS::strncpy( sBuffer,
                                (SChar*)sList->mPrev->mObj,
                                SDI_NODE_NAME_MAX_SIZE + 1 );
            }

            idlOS::strncpy( aReturnName,
                            sBuffer, 
                            SDI_NODE_NAME_MAX_SIZE + 1 );
            idlOS::strncpy( sPath,
                            SDI_ZKC_PATH_NODE_META,
                            SDI_ZKC_PATH_LENGTH );
            idlOS::strncat( sPath,
                            "/",
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
            idlOS::strncat( sPath,
                            sBuffer,
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
            idlOS::strncat( sPath,
                            SDI_ZKC_PATH_NODE_STATE,
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
            /* sPath는 '/altibase_shard/node_meta/[sBuffer]/state'의 값을 가진다. */

            sDataLength = SDI_NODE_NAME_MAX_SIZE + 1;
            sResult = aclZKC_getInfo( mZKHandler,
                                      sPath,
                                      sBuffer,
                                      &sDataLength );    

            IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

            *aNodeState = stateStringConvert( sBuffer );

            IDE_TEST( *aNodeState == ZK_ERROR );

            break;
        }
    }

    sIsAllocList = ID_FALSE;
    freeList( sList, SDI_ZKS_LIST_NODENAME );

    IDE_TEST_RAISE( sIsFound == ID_FALSE, err_Meta );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION( err_Meta )
    {
        logAndSetErrMsg( ZKC_META_CORRUPTED );
    }
    IDE_EXCEPTION_END;

    if( sIsAllocList == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getNextAliveNode             *
 * ------------------------------------------------------------------*
 * 특정 노드의 다음 노드(이름순 정렬 기준)의 이름과 해당 노드의 상태를 가져온다.
 * 입력받은 노드가 이름순 정렬의 마지막 노드일 경우
 * 이름순 정렬의 첫 노드가 반환 대상이 된다.
 * 죽어있거나 SHUTDOWN된 노드는 탐색 대상이 아니다.
 *
 * aTargetName [IN]  - 다음 노드를 찾을 대상 노드의 이름
 * aReturnName [OUT] - 대상 노드의 다음 노드의 이름
 *********************************************************************/
IDE_RC sdiZookeeper::getNextAliveNode( SChar   * aTargetNodeName,
                                       SChar   * aReturnName )
{
    iduList         * sList = NULL;
    iduListNode     * sIterator = NULL;
    iduListNode     * sNode;
    idBool            sIsAllocList = ID_FALSE;
    idBool            sIsFound = ID_FALSE;

    IDE_TEST( getAliveNodeNameList ( &sList ) != IDE_SUCCESS );
    sIsAllocList = ID_TRUE;

    IDU_LIST_ITERATE( sList, sIterator )
    {
        if( idlOS::strncmp( (SChar*)sIterator->mObj, aTargetNodeName, SDI_NODE_NAME_MAX_SIZE ) > 0 )
        {
            /* 탐색 대상보다 큰 노드를 처음으로 찾을 경우 해당 노드가 반환 대상 노드다. */
            sNode = sIterator;
            sIsFound = ID_TRUE;

            idlOS::strncpy( aReturnName,
                            (SChar*)sNode->mObj,
                            SDI_NODE_NAME_MAX_SIZE + 1 );
            break;
        }
    }

    /* 죽은 노드가 이름순으로 가장 큰 노드일 경우 */
    if( sIsFound == ID_FALSE )
    {
        /* 이 경우 리스트의 가장 앞 노드(가장 작은 노드)가 반환 대상 노드가 된다. */
        sNode = IDU_LIST_GET_FIRST(sList);

        idlOS::strncpy( aReturnName,
                        (SChar*)sNode->mObj,
                        SDI_NODE_NAME_MAX_SIZE + 1 );
    }

    sIsAllocList = ID_FALSE;
    freeList( sList, SDI_ZKS_LIST_NODENAME );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsAllocList == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getPrevAliveNode             *
 * ------------------------------------------------------------------*
 * 특정 노드의 이전 노드(이름순 정렬 기준)의 이름과 해당 노드의 상태를 가져온다.
 * 입력받은 노드가 이름순 정렬의 첫 노드일 경우
 * 이름순 정렬의 마지막 노드가 반환 대상이 된다.
 * 죽어있거나 SHUTDOWN된 노드는 탐색 대상이 아니다.
 *
 * aTargetName [IN]  - 이전 노드를 찾을 대상 노드의 이름
 * aReturnName [OUT] - 대상 노드의 이전 노드의 이름
 *********************************************************************/
IDE_RC sdiZookeeper::getPrevAliveNode( SChar   * aTargetNodeName,
                                       SChar   * aReturnName )
{
    iduList         * sList = NULL;
    iduListNode     * sIterator = NULL;
    iduListNode     * sNode;
    idBool            sIsAllocList = ID_FALSE;
    idBool            sIsFound = ID_FALSE;

    IDE_TEST( getAliveNodeNameList ( &sList ) != IDE_SUCCESS );
    sIsAllocList = ID_TRUE;

    IDU_LIST_ITERATE( sList, sIterator )
    {
        if( sIterator->mPrev != sList )
        {
            if( idlOS::strncmp( (SChar*)sIterator->mObj, aTargetNodeName, SDI_NODE_NAME_MAX_SIZE ) > 0 )
            {
                /* 탐색 대상보다 큰 노드를 처음으로 찾을 경우 해당 노드의 이전 노드가 반환 대상 노드다. */
                sNode = sIterator->mPrev;
                sIsFound = ID_TRUE;

                idlOS::strncpy( aReturnName,
                                (SChar*)sNode->mObj,
                                SDI_NODE_NAME_MAX_SIZE + 1 );
                break;
            }
        }
        else
        {
            /* 첫 노드의 경우 header와 비교하려고 할텐데 header는 값이 없으므로 비교해서는 안된다. */
        }
    }

    /* 죽은 노드가 이름순으로 가장 큰 노드일 경우 */
    if( sIsFound == ID_FALSE )
    {
        /* 이 경우 리스트의 마지막 노드가 반환 대상 노드가 된다. */
        sNode = IDU_LIST_GET_LAST( sList );

        idlOS::strncpy( aReturnName,
                        (SChar*)sNode->mObj,
                        SDI_NODE_NAME_MAX_SIZE + 1 );
    }

    sIsAllocList = ID_FALSE;
    freeList( sList, SDI_ZKS_LIST_NODENAME );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsAllocList == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::settingAddNodeWatch          *
 * ------------------------------------------------------------------*
 * ZKS의 연결정보(/altibase_shard/connection_info)에 CHILD_CHANGED watch를 건다.
 * 새 노드가 연결되면 연결정보의 데이터를 증가시킨다.
 * 변경에 대한 알람을 받은 각 노드들은 새 노드가 추가된 것을 알 수 있다.
 *********************************************************************/
IDE_RC sdiZookeeper::settingAddNodeWatch()
{

    ZKCResult sResult;
    SChar     sPath[SDI_ZKC_PATH_LENGTH] = {0,};

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/connection_info'의 값을 가진다. */

    sResult = aclZKC_Watch_Data( mZKHandler,
                                 watch_addNewNode,
                                 sPath );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::settingAllDeleteNodeWatch    *
 * ------------------------------------------------------------------*
 * ZKS의 연결정보(/altibase_shard/connection_info)의 모든 자식들에게 watch를 건다.
 * 연결정보에 변경이 생길 경우 알람이 발생해 watch_deleteNode함수가 수행된다.
 * 해당 함수는 연결정보에 새 노드가 추가되었을때 호출된다.
 *********************************************************************/
IDE_RC sdiZookeeper::settingAllDeleteNodeWatch()
{
    ZKCResult       sResult;
    String_vector   sChildren;
    SChar           sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar           sConnInfoPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};
    SInt            i = 0;

    idlOS::strncpy( sConnInfoPath,
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    /* sConnInfoPath는 '/altibase_shard/connection_info'의 값을 가진다. */

    sResult = aclZKC_getChildren( mZKHandler,
                                  sConnInfoPath,
                                  &sChildren );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for( i = 0; sChildren.count > i; i++ )
    {
        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        idlOS::strncpy( sBuffer,
                        sChildren.data[i],
                        SDI_ZKC_BUFFER_SIZE );

        /* 자기 자신의 delete도 Watch 한다. Failover 구문에 의해 제거될수 있기 때문 */
        idlOS::memset( sNodePath, 0, SDI_ZKC_PATH_LENGTH );
        idlOS::strncpy( sNodePath,
                        sConnInfoPath,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sNodePath, 
                        "/",
                        ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
        idlOS::strncat( sNodePath,
                        sBuffer,
                        ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
        /* sNodePath는 '/altibase_shard/connection_info/[sBuffer]'의 값을 가진다. */

        sResult = aclZKC_Watch_Node( mZKHandler,
                                     watch_deleteNode,
                                     sNodePath );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::settingDeleteNodeWatch       *
 * ------------------------------------------------------------------*
 * ZKS의 연결정보(/altibase_shard/connection_info)의 특정 자식에게 watch를 건다.
 * 연결정보에 변경이 생길 경우 알람이 발생해 watch_deleteNode함수가 수행된다.
 * 해당 함수는 특정 노드 삭제 알람이 온 후 다시 알람을 걸기 위해 호출된다.
 *********************************************************************/
IDE_RC sdiZookeeper::settingDeleteNodeWatch( SChar * aPath )
{
    ZKCResult sResult;

    sResult = aclZKC_Watch_Node( mZKHandler,
                                 watch_deleteNode,
                                 aPath );

    IDE_TEST_RAISE( ( sResult != ZKC_SUCCESS ) && ( sResult != ZKC_NO_NODE ), err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::watch_addNewNode             *
 * ------------------------------------------------------------------*
 * 연결정보의 데이터에 변경이 생길 경우 알람이 발생해 호출된다.
 * 혹은 알람을 받은 직후 재 알람을 위해 호출된다.
 * 이 경우 새 노드가 추가된 것이므로 connection_info의 각 자식노드에 대해
 * watch를 다시 걸수 있도록 watch_deleteNode 함수를 호출한다.
 *********************************************************************/
void sdiZookeeper::watch_addNewNode( aclZK_Handler *,
                                     SInt           aType,
                                     SInt           ,
                                     const SChar   *,
                                     void          * )
{
    SInt                sDataLength = SDI_ZKC_BUFFER_SIZE;
    SChar               sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    initTempContainer( (SChar *)"[ZOOKEEPER] watch_addNewNode", IDE_SD_20 );

    if( aType == ZOO_SESSION_EVENT )
    {
        /* ZKS와 연결이 끊긴 상태 */
        if ( isConnect() == ID_TRUE )
        {
            disconnect();
        }

        if ( connect( ID_FALSE ) == IDE_SUCCESS )
        {
            /* ConnectionInfo를 Add하기 위해 MetaLock을 획득한다. */
            if( getZookeeperMetaLock( ID_UINT_MAX ) == IDE_SUCCESS )
            {

                /* 이미 failover 되었는지 확인 해야 한다. */
                (void) getNodeInfo( mMyNodeName,
                                    (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                    sBuffer,
                                    &sDataLength );

                if ( sDataLength > 0 )
                {
                    /* 이미 failover가 되어있는 상태이므로 따로 작업할 필요가 없다. */
                    releaseZookeeperMetaLock();
                    disconnect();
                }
                else
                {
                    if ( addConnectionInfo() == IDE_SUCCESS )
                    {
                        /* R2HA Watch 설정에 실패했을 경우 그냥 무시하면 안된다.
                         * 추후 확인이 필요함 */
                        /* add watch를 걸어야 한다. */
                        (void)settingAddNodeWatch();

                        /* delete watch를 다시 걸어야 한다. */
                        (void)settingAllDeleteNodeWatch();

                        releaseZookeeperMetaLock();

                    }
                    else
                    {
                        /* 재접속 실패 */
                        releaseZookeeperMetaLock();
                        disconnect();
                    }
                }
            }
            else
            {
                /* 재접속 실패 */
                disconnect();
            }
        }
        else
        {
            /* 재접속 실패 */
        }

        if ( isConnect() != ID_TRUE )
        {
            /* re-connect 에 실패하였다. */
            /* 에러코드를 세팅하고 trc로그를 남긴 후 assert를 호출한다. */
            logAndSetErrMsg( ZKC_CONNECTION_MISSING );
            IDE_CALLBACK_FATAL( "Connection with ZKS has been lost." );
        }

    }
    else if( aType == ZOO_DELETED_EVENT )
    {
        /* 연결은 되어 있으나 connection_info 자체를 누군가 강제로 지운 상태 */
    }
    else
    {
        /* watch 알람을 받은 직후 다시 watch를 걸어야 한다. */
        (void)settingAddNodeWatch();

        /* 새 노드가 추가되었으므로 CHILD에 대한 watch를 다시 걸어야 한다. */
        (void)settingAllDeleteNodeWatch();
    }

    destroyTempContainer();

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::watch_deleteNode             *
 * ------------------------------------------------------------------*
 * 연결정보에 변경이 생길 경우 알람이 발생해 호출된다.
 * 받은 알람을 분석해 자신이 failover를 수행해야 하는 경우 failover를 호출한다.
 *********************************************************************/
void sdiZookeeper::watch_deleteNode( aclZK_Handler *,
                                     SInt            aType,
                                     SInt           ,
                                     const SChar   * aPath,
                                     void          * )
{
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar       sNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SChar       sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar     * sStrPtr;
    ZKState     sNodeState = ZK_NONE;

    initTempContainer( (SChar *)"[ZOOKEEPER] watch_deleteNode", IDE_SD_20 );

    if( aType != ZOO_SESSION_EVENT )
    {
        /* watch 알람을 받은 직후 다시 watch를 걸어야 한다. */
        idlOS::strncpy( sPath,
                        aPath,
                        SDI_ZKC_PATH_LENGTH );
        (void)settingDeleteNodeWatch( sPath );

        if( ( aType == ZOO_DELETED_EVENT ) )
        {
            /* aPath는 '/altibase_shard/connection_info/[deadNodeName]'의 형태이므로 * 
             * [deadNodeName] 부분만 분리하면 바로 죽은 노드의 이름을 알 수 있다. */
            sStrPtr = (SChar*)aPath + idlOS::strlen( SDI_ZKC_PATH_CONNECTION_INFO ) + 1;
            idlOS::strncpy( sNodeName,
                            sStrPtr,
                            SDI_NODE_NAME_MAX_SIZE + 1 );

            /* delete로 인한 알람일 경우 내가 처리해야 하는지 체크 */
            if ( getNextAliveNode( sNodeName, sBuffer ) == IDE_SUCCESS )
            {
                if( isNodeMatch( sBuffer, mMyNodeName ) == ID_TRUE )
                {
                    (void)getNodeState( sNodeName, &sNodeState );

                    // check State ZK_KILL 이면 Failover4Watcher대상이 아님
                    if ( sNodeState != ZK_KILL )
                    {
                        /* 내 이전 노드가 죽었을 경우 내가 failover 해야 한다. */
                        /* FailoverForWatcher가 꺼져있는지 확인. */
                        if ( SDU_DISABLE_FAILOVER_FOR_WATCHER == 0 )
                        {
                            (void)failoverForWatcher( sNodeName );
                        }
                    }
                }
                else if ( isNodeMatch( sNodeName, mMyNodeName ) == ID_TRUE )
                {
                    (void)getNodeState( sNodeName, &sNodeState );

                    // check State ZK_KILL 이면 KILL
                    if ( sNodeState == ZK_KILL )
                    {
                        ideLog::log( IDE_SD_20, "This Node is Failed Over" );
                        IDE_CALLBACK_FATAL("This node is Failed Over" );
                    }
                }
            }
            else
            {
                /* in case of all node delete, the alarm was ignored */
            }
        }
        else
        {
            /* add로 인한 알람일 경우 무시 */
        }
    }
    else
    {
        /* ZKS와 연결이 끊긴 상태 */
        /* 연결이 끊기면 add/delete watch 모두 받으므로 add 에서 처리한다. */
        /* 에러코드를 세팅하고 trc로그를 남긴 후 assert를 호출한다. */
//        logAndSetErrMsg( ZKC_CONNECTION_MISSING );
//        IDE_CALLBACK_FATAL("Connection with ZKS has been lost." );
    }

    destroyTempContainer();

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getNodeInfo                  *
 * ------------------------------------------------------------------*
 * 특정 노드의 메타 정보를 반환한다.
 * aDataLen은 두가지 용도로 사용된다.
 *
 * aTargetName - [IN]  메타 정보를 얻고 싶은 노드의 이름
 * aInfoType   - [IN]  얻고 싶은 메타 정보
 * aValue      - [OUT] 메타 정보를 반환할 버퍼
 * aDataLen    - [IN]  메타 정보를 반환할 버퍼의 길이
 *             - [OUT] 반환된 메타 정보의 길이
 *********************************************************************/
IDE_RC sdiZookeeper::getNodeInfo( SChar * aTargetName,
                                  SChar * aInfoType,
                                  SChar * aValue,
                                  SInt  * aDataLen )
{
    ZKCResult sResult;
    SChar     sPath[SDI_ZKC_PATH_LENGTH] = {0,};

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aTargetName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aInfoType,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[aTargetName][aInfoType]'의 값을 가진다. */

    sResult = aclZKC_getInfo( mZKHandler,
                              sPath,
                              aValue,
                              aDataLen );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getNodeState                 *
 * ------------------------------------------------------------------*
 * 특정 노드의 상태(state)를 반환한다.
 *
 * aTargetName - [IN]  상태 정보를 얻고 싶은 노드의 이름
 * aState      - [OUT] 요청한 노드의 상태
 *********************************************************************/
IDE_RC sdiZookeeper::getNodeState( SChar   * aTargetName,
                                   ZKState * aState )
{
    ZKCResult sResult;
    SChar     sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar     sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt      sDataSize = SDI_ZKC_BUFFER_SIZE;
    ZKState   sState;

    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aTargetName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* sPath는 '/altibase_shard/node_meta/[aTargetName]/state'의 값을 가진다. */

    sResult = aclZKC_getInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              &sDataSize );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    sState = sdiZookeeper::stateStringConvert( sBuffer );

    IDE_TEST_RAISE( sState == ZK_ERROR, err_ZKC );

    *aState = sState;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::insertInfo                   *
 * ------------------------------------------------------------------*
 * 특정 path에 원하는 값을 삽입한다.
 * 해당 함수는 metaLock을 잡지 않고 수행하므로 다수의 노드가 
 * 동시에 변경할 여지가 있는 부분을 변경할 경우에는 따로 metaLock을 잡아야 한다.
 *
 * aPath       - [IN] 데이터를 삽입할 path(full path)
 * aData       - [IN] 삽입할 데이터
 * aDataLength - [IN] 삽입할 데이터의 길이
 *********************************************************************/
IDE_RC sdiZookeeper::insertInfo( SChar * aPath,
                                 SChar * aData )
{
    ZKCResult sResult;

    sResult = aclZKC_setInfo( mZKHandler,
                              aPath,
                              aData,
                              idlOS::strlen( aData ) );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::readInfo                     *
 * ------------------------------------------------------------------*
 * 특정 path의 값을 읽는다.
 *
 * aPath    - [IN]  데이터를 읽을 path(full path)
 * aData    - [OUT] 읽은 데이터를 반환할 버퍼
 * aDataLen - [IN]  읽은 데이터를 반환할 버퍼의 크기
 *          - [OUT] 읽은 데이터의 길이
 *********************************************************************/
IDE_RC sdiZookeeper::readInfo( SChar * aPath,
                               SChar * aData,
                               SInt  * aDataLen )
{
    ZKCResult sResult;

    sResult = aclZKC_getInfo( mZKHandler,
                              aPath,
                              aData,
                              aDataLen );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );


    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::removeInfo                   *
 * ------------------------------------------------------------------*
 * 특정 path의 값을 지운다.(path를 지우지는 않는다.)
 * 해당 함수는 metaLock을 잡지 않고 수행하므로 다수의 노드가 
 * 동시에 변경할 여지가 있는 부분을 변경할 경우에는 따로 metaLock을 잡아야 한다.
 *
 * aPath       - [IN] 데이터를 삭제할 path(full path)
 *********************************************************************/
IDE_RC sdiZookeeper::removeInfo( SChar * aPath )
{
    ZKCResult sResult;

    sResult = aclZKC_deleteInfo( mZKHandler,
                                 aPath );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::createPath                   *
 * ------------------------------------------------------------------*
 * 특정 path를 생성하고 데이터를 넣는다.
 * 해당 함수는 metaLock을 잡지 않고 수행하므로 다수의 노드가 
 * 동시에 변경할 여지가 있는 부분을 변경할 경우에는 따로 metaLock을 잡아야 한다.
 *
 * aPath       - [IN] 새로 생성할 path(full path)
 * aData       - [IN] 생성시 삽입할 데이터
 *********************************************************************/
IDE_RC sdiZookeeper::createPath( SChar * aPath,
                                 SChar * aData )
{
    ZKCResult sResult;

    sResult = aclZKC_insertNode( mZKHandler,
                                 aPath,
                                 aData,
                                 idlOS::strlen( aData ),
                                 ACP_FALSE );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::removePath                   *
 * ------------------------------------------------------------------*
 * 특정 path와 그 데이터를 제거한다.
 * 해당 함수는 metaLock을 잡지 않고 수행하므로 다수의 노드가 
 * 동시에 변경할 여지가 있는 부분을 변경할 경우에는 따로 metaLock을 잡아야 한다.
 *
 * aPath       - [IN] 제거할 path(full path)
 *********************************************************************/
IDE_RC sdiZookeeper::removePath( SChar * aPath )
{
    ZKCResult sResult;

    sResult = aclZKC_deleteNode( mZKHandler,
                                 aPath );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::removeRecursivePathAndInfo( SChar * aPath )
{
    ZKCResult sResult;
    String_vector sChildren;
    SInt i = 0;
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE+1] = {0,};
    SInt        sReturn = 0;
    
    sResult = aclZKC_getChildren( mZKHandler,
                                  aPath,
                                  &sChildren );
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for ( i = 0; i < sChildren.count ; i++ )
    {
        sReturn = idlOS::snprintf(sBuffer,SDI_ZKC_BUFFER_SIZE+1,"%s/%s",aPath, sChildren.data[i]);
        IDE_TEST_RAISE(sReturn >= SDI_ZKC_BUFFER_SIZE+1, err_buffer_size);
        removeRecursivePathAndInfo( sBuffer);
    }

    sResult = aclZKC_deleteNode( mZKHandler,
                                 aPath );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;
    IDE_EXCEPTION( err_buffer_size )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdiZookeeper::removeRecursivePathAndInfo",
                                  "internal module error, buffer size too small" ) );
    }
    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::checkPath                    *
 * ------------------------------------------------------------------*
 * 특정 path가 존재하는지 확인한다.
 *
 * aPath       - [IN]  존재여부를 확인할 path(full path)
 * aIsExist    - [OUT] 해당 path의 존재여부
 *********************************************************************/
IDE_RC sdiZookeeper::checkPath( SChar  * aPath,
                                idBool * aIsExist )
{
    ZKCResult sResult;

    sResult = aclZKC_existNode( mZKHandler,
                                aPath );

    switch( sResult )
    {
        case ZKC_SUCCESS:
            *aIsExist = ID_TRUE;
            break;
        case ZKC_NO_NODE:
            *aIsExist = ID_FALSE;
            break;
        default:
            /* 그 외에 다른 결과라면 예외가 발생한다. */
            IDE_RAISE( err_ZKC );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::addConnectionInfo            *
 * ------------------------------------------------------------------*
 * 내 노드의 연결 정보를 추가한다.
 * /altibase_shard/connection_info에 내 노드의 이름으로 새 임시 노드를 생성한 후
 * connection_info의 값을 변경시켜 다른 노드들에게 새 노드가 추가 되었음을 알린다.
 * 알람을 받은 노드들은 새 임시 노드에 대한 watch를 추가한다.
 *********************************************************************/
IDE_RC sdiZookeeper::addConnectionInfo()
{
    ZKCResult   sResult;
    SChar       sConnPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar       sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};

    idlOS::strncpy( sConnPath,
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    /* sNodePath는 '/altibase_shard/connection_info'의 값을 가진다. */

    idlOS::strncpy( sNodePath,
                    sConnPath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sNodePath,
                    "/",
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    idlOS::strncat( sNodePath,
                    mMyNodeName,
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    /* sNodePath는 '/altibase_shard/connection_info/[mMyNodeName]'의 값을 가진다. */

    /* connection_info에 내 노드의 이름으로 새 임시 노드를 생성한다.  */
    sResult = aclZKC_insertNode( mZKHandler,
                                 sNodePath,
                                 NULL,
                                 -1,
                                 ACP_TRUE );
 
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    /* connection_info의 값을 변경시켜 새 노드가 추가 된 것을 알린다.*/
    sResult = aclZKC_setInfo( mZKHandler,
                              sConnPath,
                              mMyNodeName,
                              idlOS::strlen( mMyNodeName ) );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::addConnectionInfo            *
 * ------------------------------------------------------------------*
 * 연결되어 있는 노드정보를 제거한다.
 * /altibase_shard/connection_info/[TargetNodeName]으로 생성되어 있는
 * 값을 제거하여 다른 노드들에게 노드 정보가 변경되었음을 알린다.
 * 알람을 받은 노드들은 다시 watch를 추가한다.
 *********************************************************************/
IDE_RC sdiZookeeper::removeConnectionInfo( SChar * aTargetNodeName )
{
    ZKCResult   sResult;
    SChar       sConnPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar       sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};

    idlOS::strncpy( sConnPath,
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    /* sNodePath는 '/altibase_shard/connection_info'의 값을 가진다. */

    idlOS::strncpy( sNodePath,
                    sConnPath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sNodePath,
                    "/",
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    idlOS::strncat( sNodePath,
                    aTargetNodeName,
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    /* sNodePath는 '/altibase_shard/connection_info/[aTargetNodeName]'의 값을 가진다. */

    /* connection_info를 제거한다. */
    sResult = aclZKC_deleteNode( mZKHandler, sNodePath );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::checkAllNodeAlive            *
 * ------------------------------------------------------------------*
 * zookeeper에 연결된 모든 노드가 살아있는지 체크한다.
 * 모든 노드가 살아있을 경우 true를 리턴하고 하나라도 죽어있을 경우 false를 리턴한다.
 * 해당 작업은 읽기 작업이라 lock을 잡을 필요는 없으나
 * 노드 숫자 확인 중 노드가 추가/제거될 경우를 대비해 lock을 잡고 수행한다.
 * 죽어있는 노드가 하나라도 있을 경우 add/drop(non-force) 작업을
 * 막아야 하므로 작업 수행전 해당 함수를 호출해 체크해야 한다.
 *
 * aIsAlive     - [OUT] 모든 노드가 살아있는지 여부
 *********************************************************************/
IDE_RC sdiZookeeper::checkAllNodeAlive( idBool   * aIsAlive )
{
    ZKCResult       sResult;
    SChar           sConnPath[] = SDI_ZKC_PATH_CONNECTION_INFO;
    SChar           sNodePath[] = SDI_ZKC_PATH_NODE_META;
    SChar           sCheckPath[] = SDI_ZKC_PATH_META_LOCK;
    SInt            sAllNodeCnt = 0;
    SInt            sAliveNodeCnt = 0;
    String_vector   sChildren;
    idBool          sIsInited = ID_FALSE;

    UInt            sRemoveCnt = 0;
    ZKState         sNodeState = ZK_NONE;

    iduList       * sList = NULL;
    iduListNode   * sIterator = NULL;
    SChar           sNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    idBool          sIsAllocList = ID_FALSE;

    /* R2HA to be remove */
    if ( SDU_SHARD_ZOOKEEPER_TEST == 1 )
    {
        /* 클러스터 메타가 존재하지 않는다면 lock을 잡을수 없으므로 한번 체크한다. */
        IDE_TEST( checkPath( sCheckPath, &sIsInited ) != IDE_SUCCESS );

        if( sIsInited == ID_FALSE )
        {
            *aIsAlive = ID_TRUE;
            IDE_CONT( normal_exit );
        }

        /* 1. 연결된 살아있는 노드의 수를 얻는다. */
        sResult = aclZKC_getChildren( mZKHandler,
                                      sConnPath,
                                      &sChildren );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        sAliveNodeCnt = sChildren.count;

        /* 2. 연결된 모든 노드의 수를 얻는다. */
        sResult = aclZKC_getChildren( mZKHandler,
                                      sNodePath,
                                      &sChildren );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        sAllNodeCnt = sChildren.count;

        /* 둘을 비교한다. */
        if( sAllNodeCnt == sAliveNodeCnt )
        {
            *aIsAlive = ID_TRUE;
        }
        else
        {
            /* Add 중에 실패해서 남아 있는 경우는 제거한다. */
            // getAllNodeInfo
            IDE_TEST( getAllNodeNameList( &sList ) != IDE_SUCCESS );
            sIsAllocList = ID_TRUE;

            IDU_LIST_ITERATE( sList, sIterator )
            {
                idlOS::memset( sNodeName, 0, SDI_NODE_NAME_MAX_SIZE + 1 );
                idlOS::strncpy( sNodeName,
                                (SChar*)sIterator->mObj,
                                SDI_NODE_NAME_MAX_SIZE + 1 );
                // check NodeState
                IDE_TEST( getNodeState( sNodeName, &sNodeState ) != IDE_SUCCESS );
                if( sNodeState == ZK_ADD )
                {
                    IDE_TEST( removeNode( sNodeName ) != IDE_SUCCESS );
                    sRemoveCnt++;
                }
            }

            if ( sAllNodeCnt == ( sAliveNodeCnt + sRemoveCnt ) )
            {
                *aIsAlive = ID_TRUE;
            }
            else
            {
                *aIsAlive = ID_FALSE;
            }
        }

        if ( sIsAllocList == ID_TRUE )
        {
            sIsAllocList = ID_FALSE;
            freeList( sList, SDI_ZKS_LIST_NODENAME );
        }

        IDE_EXCEPTION_CONT( normal_exit );
    }
    else
    {
        *aIsAlive = ID_TRUE;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    if( sIsAllocList == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::checkRecentDeadNode          *
 * ------------------------------------------------------------------*
 * 어떤 노드가 가장 최근에 죽었는지를 반환한다.
 * 가장 최근에 죽은 노드만 failback이 가능하므로 failback시 호출된다.
 * 죽은 순서의 기준은 failover history이다.
 *
 * aNodeName - [OUT] 가장 최근에 죽은 노드의 이름
 *********************************************************************/
IDE_RC sdiZookeeper::checkRecentDeadNode( SChar * aNodeName,
                                          ULong * aFailbackSMN,
                                          ULong * aFailoveredSMN )
{
    ZKCResult   sResult;
    SChar       sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar       sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt        sDataLength;
    SChar     * sCheckBuffer = NULL;
    ULong       sFailbackSMN = 0;
    ULong       sFailoveredSMN = 0;

    idlOS::strncpy( sPath, 
                    SDI_ZKC_PATH_FAILOVER_HISTORY,
                    SDI_ZKC_PATH_LENGTH );
    /* sPath는 '/altibase_shard/cluster_meta/failover_history'의 값을 가진다. */

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    sResult = aclZKC_getInfo( mZKHandler,
                              sPath,
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    if( sDataLength > 0 )
    {
        /* failover history는 노드이름1:SMN/노드이름2:SMN/...의 형태로 이루어져 있으므로 
         * 첫 ':'문자의 위치만 알면 가장 최근에 죽은 노드의 이름을 알 수 있다. */
        /* NodeName */
        sCheckBuffer = idlOS::strtok( sBuffer, ":" );

        idlOS::strncpy( aNodeName,
                        sCheckBuffer,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        /* SMN */
        sCheckBuffer = idlOS::strtok( NULL, ":" );

        sFailbackSMN = idlOS::strToULong( (UChar*)sCheckBuffer, idlOS::strlen( sCheckBuffer ) );

        sCheckBuffer = idlOS::strtok( NULL, "/" );

        sFailoveredSMN = idlOS::strToULong( (UChar*)sCheckBuffer, idlOS::strlen( sCheckBuffer ) );

        ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] failback SMN : %u, failoverd SMN : %u", sFailbackSMN, sFailoveredSMN);
        ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] RecentDeadNode : %s", aNodeName);


        if ( aFailbackSMN != NULL )
        {
            *aFailbackSMN = sFailbackSMN;
        }
        if ( aFailoveredSMN != NULL )
        {
            *aFailoveredSMN = sFailoveredSMN;
        }

    }
    else
    {
        /* 죽은 노드가 없는 경우 */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::removeNode                   *
 * ------------------------------------------------------------------*
 * 특정 노드의 정보를 zookeeper 메타에서 삭제한다.
 * 해당 함수에서 validation은 수행하지 않으므로 해당 함수를 호출하기 전 수행해야 한다.
 * 해당 함수는 lock을 잡지 않으므로 동시성 문제가 걱정된다면 lock을 잡고 수행해야 한다.
 * 삭제 작업은 multi operation으로 이루어지며 삭제된 내용은 돌릴수 없다.
 * 해당 함수는 DROP 구문이나 FAILOVER 과정에서 ADD도중 실패한 내역을 정리할때 사용된다.
 *
 * aNodeName - [OUT] 제거할 노드의 이름
 *********************************************************************/
IDE_RC sdiZookeeper::removeNode( SChar * aNodeName )
{
    ZKCResult       sResult;

    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sPath[7][SDI_ZKC_PATH_LENGTH] = {{0,},};

    SInt            i = 0;
    zoo_op_t        sOp[8];
    zoo_op_result_t sOpResult[8];

    idlOS::strncpy( sNodePath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sNodePath,
                    "/",
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    idlOS::strncat( sNodePath,
                    aNodeName,
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1);
    /* sNodePath는 '/altibase_shard/node_meta/[aNodeName]'의 값을 가진다. */

    /* 메타 변경을 수행할 multi op를 생성한다. */
    idlOS::memset( sPath[0], 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath[0],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[0],
                    SDI_ZKC_PATH_NODE_ID,
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    /* sPath[0]는 '/altibase_shard/node_meta/[aNodeName]/shard_node_id'의 값을 가진다. */

    zoo_delete_op_init( &sOp[0],
                        sPath[0],
                        -1 );

    idlOS::strncpy( sPath[1],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[1],
                    SDI_ZKC_PATH_NODE_IP,
                    ID_SIZEOF( sPath[1] ) - idlOS::strlen( sPath[1] ) - 1 );
    /* sPath[1]는 '/altibase_shard/node_meta/[aNodeName]/node_ip:port'의 값을 가진다. */

    zoo_delete_op_init( &sOp[1],
                        sPath[1],
                        -1 );

    idlOS::strncpy( sPath[2],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[2],
                    SDI_ZKC_PATH_NODE_INTERNAL_IP,
                    ID_SIZEOF( sPath[2] ) - idlOS::strlen( sPath[2] ) - 1 );
    /* sPath[2]는 '/altibase_shard/node_meta/[aNodeName]/internal_node_ip:port'의 값을 가진다. */

    zoo_delete_op_init( &sOp[2],
                        sPath[2],
                        -1 );

    idlOS::strncpy( sPath[3],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[3],
                    SDI_ZKC_PATH_NODE_REPL_HOST_IP,
                    ID_SIZEOF( sPath[3] ) - idlOS::strlen( sPath[3] ) - 1 );
    /* sPath[3]는 '/altibase_shard/node_meta/[aNodeName]/internal_replication_host_ip:port'의 값을 가진다. */

    zoo_delete_op_init( &sOp[3],
                        sPath[3],
                        -1 );

    idlOS::strncpy( sPath[4],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[4],
                    SDI_ZKC_PATH_NODE_CONN_TYPE,
                    ID_SIZEOF( sPath[4] ) - idlOS::strlen( sPath[4] ) - 1 );
    /* sPath[4]는 '/altibase_shard/node_meta/[aNodeName]/conn_type'의 값을 가진다. */

    zoo_delete_op_init( &sOp[4],
                        sPath[4],
                        -1 );

    idlOS::strncpy( sPath[5],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[5],
                    SDI_ZKC_PATH_NODE_FAILOVER_TO,
                    ID_SIZEOF( sPath[5] ) - idlOS::strlen( sPath[5] ) - 1 );
    /* sPath[5]는 '/altibase_shard/node_meta/[aNodeName]/failoverTo'의 값을 가진다. */

    zoo_delete_op_init( &sOp[5],
                        sPath[5],
                        -1 );

    idlOS::strncpy( sPath[6],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[6],
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath[6] ) - idlOS::strlen( sPath[6] ) - 1 );
    /* sPath[6]는 '/altibase_shard/node_meta/[aNodeName]/state'의 값을 가진다. */

    zoo_delete_op_init( &sOp[6],
                        sPath[6],
                        -1 );

    /* 모든 서브 path가 제거된 후에 대상 path도 제거되어야 한다. */
    zoo_delete_op_init( &sOp[7],
                        sNodePath,
                        -1 );

    /* 생성한 multi op를 수행한다. */
    sResult = aclZKC_multiOp( mZKHandler, 8, sOp, sOpResult );

    /* multi op로 수행한 각 operation의 정상 수행 여부를 확인한다. */
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for( i = 0; i <= 7; i++ )
    {
        IDE_TEST_RAISE( sOpResult[i].err != ZOK, err_ZKC );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::killNode                     *
 * ------------------------------------------------------------------*
 * 특정 노드의 State를 KILL로 변경하고 Connection_info를 제거한다.
 * 해당 함수에서 validation은 수행하지 않으므로 해당 함수를 호출하기 전 수행해야 한다.
 * 해당 함수는 lock을 잡지 않으므로 동시성 문제가 걱정된다면 lock을 잡고 수행해야 한다.
 * 삭제 작업은 multi operation으로 이루어지며 변경된 내용은 돌릴수 없다.
 * 해당 함수는 Failover 수행시 대상 Node를 KILL하기 위해 호출된다.
 *
 * aNodeName - [OUT] KILL할 노드의 이름
 *********************************************************************/
IDE_RC sdiZookeeper::killNode( SChar * aNodeName )
{
    ZKCResult       sResult;

    SChar           sNodePath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar           sPath[2][SDI_ZKC_PATH_LENGTH] = {{0,},};

    SInt            i = 0;
    zoo_op_t        sOp[2];
    zoo_op_result_t sOpResult[2];

    IDE_TEST( validateState( ZK_KILL,
                             aNodeName ) != IDE_SUCCESS );

    idlOS::strncpy( sNodePath,
                    SDI_ZKC_PATH_NODE_META,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sNodePath,
                    "/",
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1 );
    idlOS::strncat( sNodePath,
                    aNodeName,
                    ID_SIZEOF( sNodePath ) - idlOS::strlen( sNodePath ) - 1);
    /* sNodePath는 '/altibase_shard/node_meta/[aNodeName]'의 값을 가진다. */

    /* 메타 변경을 수행할 multi op를 생성한다. */
    idlOS::memset( sPath[0], 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath[0],
                    sNodePath,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[0],
                    SDI_ZKC_PATH_NODE_STATE,
                    ID_SIZEOF( sPath[0] ) - idlOS::strlen( sPath[0] ) - 1 );
    /* sPath[0]는 '/altibase_shard/node_meta/[aNodeName]/state'의 값을 가진다. */

    zoo_set_op_init( &sOp[0],
                     sPath[0],
                     SDI_ZKC_STATE_KILL,
                     idlOS::strlen( SDI_ZKC_STATE_KILL ),
                     -1,
                     NULL );   

    idlOS::strncpy( sPath[1],
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath[1],
                    "/",
                    ID_SIZEOF( sPath[1] ) - idlOS::strlen( sPath[1] ) - 1 );
    idlOS::strncat( sPath[1],
                    aNodeName,
                    ID_SIZEOF( sPath[1] ) - idlOS::strlen( sPath[1] ) - 1 );
    /* sPath[1]는 '/altibase_shard/connection_info/[aNodeName]'의 값을 가진다. */

    zoo_delete_op_init( &sOp[1],
                        sPath[1],
                        -1 );

    /* 생성한 multi op를 수행한다. */
    sResult = aclZKC_multiOp( mZKHandler, 2, sOp, sOpResult );

    /* multi op로 수행한 각 operation의 정상 수행 여부를 확인한다. */
    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for( i = 0; i < 2; i++ )
    {
        IDE_TEST_RAISE( sOpResult[i].err != ZOK, err_ZKC );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

sdiLocalMetaInfo * sdiZookeeper::getNodeInfo(iduList * aNodeList, SChar * aNodeName)
{
    sdiLocalMetaInfo * sLocalMetaInfo = NULL;
    iduListNode * sNode = NULL;
    iduListNode * sDummy = NULL;

    if ( IDU_LIST_IS_EMPTY( aNodeList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( aNodeList, sNode, sDummy )
        {
            sLocalMetaInfo = (sdiLocalMetaInfo *)sNode->mObj;
            if( idlOS::strncmp(sLocalMetaInfo->mNodeName, aNodeName, SDI_NODE_NAME_MAX_SIZE) != 0 )
            {
                sLocalMetaInfo = NULL;
            }
            else
            {
                /* return current sLocalMetaInfo */
                break;
            }
        }
    }
    else
    {
        sLocalMetaInfo = NULL;
    }

    return sLocalMetaInfo;
}

sdiLocalMetaInfo * sdiZookeeper::getNodeInfoByID(iduList * aNodeList, UInt aNodeId )
{
    sdiLocalMetaInfo * sLocalMetaInfo = NULL;
    iduListNode * sNode = NULL;
    iduListNode * sDummy = NULL;

    if ( IDU_LIST_IS_EMPTY( aNodeList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( aNodeList, sNode, sDummy )
        {
            sLocalMetaInfo = (sdiLocalMetaInfo *)sNode->mObj;
            if( sLocalMetaInfo->mShardNodeId != aNodeId )
            {
                sLocalMetaInfo = NULL;
            }
            else
            {
                /* return current sLocalMetaInfo */
                break;
            }
        }
    }
    else
    {
        sLocalMetaInfo = NULL;
    }

    return sLocalMetaInfo;
}

IDE_RC sdiZookeeper::checkNodeAlive( SChar   * aNodeName,
                                     idBool  * aIsAlive )
{
    SChar           sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    idBool          sIsAlive  = ID_FALSE;

    idlOS::memset( sPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    /* path는 '/altibase_shard/connection_info/[aNodeName]'의 값을 가진다. */

    IDE_TEST( checkPath( sPath,
                         &sIsAlive ) != IDE_SUCCESS );

    * aIsAlive = sIsAlive;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void sdiZookeeper::setReplInfo4FailoverAfterCommit( SChar * aFirstNodeName,
                                                    SChar * aSecondNodeName,
                                                    SChar * aFirstReplName,
                                                    SChar * aSecondReplName )
{
    idlOS::strncpy( mFirstStopNodeName,
                    aFirstNodeName,
                    SDI_NODE_NAME_MAX_SIZE + 1 );
    idlOS::strncpy( mSecondStopNodeName,
                    aSecondNodeName,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    idlOS::strncpy( mFirstStopReplName,
                    aFirstReplName,
                    SDI_REPLICATION_NAME_MAX_SIZE + 1 );
    idlOS::strncpy( mSecondStopReplName,
                    aSecondReplName,
                    SDI_REPLICATION_NAME_MAX_SIZE + 1 );
}

IDE_RC sdiZookeeper::addPendingJob( SChar            * aSQL,
                                    SChar            * aNodeName,
                                    ZKPendingJobType   aPendingType,
                                    qciStmtType        aSQLStmtType )
{
    sdiZKPendingJob * sJob = NULL;
    UInt sJobSQLLen = 0;
    UInt sJobSize = 0;

    sJobSQLLen = idlOS::strlen(aSQL);
    sJobSize   = idlOS::align8((UInt)(offsetof(sdiZKPendingJob, mSQL) + sJobSQLLen + 1));

    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_SDI,
                                     1,
                                     sJobSize,
                                     (void **)&sJob,
                                     IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );
    sJob->mIsCommitJob = ID_FALSE;
    sJob->mIsRollbackJob = ID_FALSE;
    sJob->mStmtType = QCI_STMT_MASK_MAX;
        
    /* node name copy */
    idlOS::strncpy(sJob->mNodeName, (const SChar*)aNodeName, SDI_NODE_NAME_MAX_SIZE + 1);

    /* job sql copy */
    idlOS::strncpy(sJob->mSQL, (const SChar*)aSQL, sJobSQLLen + 1);

    switch ( aPendingType )
    {
        case ZK_PENDING_JOB_AFTER_COMMIT:
            sJob->mIsCommitJob = ID_TRUE;
            sJob->mStmtType = aSQLStmtType;
            break;
        case ZK_PENDING_JOB_AFTER_ROLLBACK:
            sJob->mIsRollbackJob = ID_TRUE;
            sJob->mStmtType = aSQLStmtType;
            break;
        case ZK_PENDING_JOB_AFTER_END_ALL:
            sJob->mIsRollbackJob = ID_TRUE;
            sJob->mIsCommitJob = ID_TRUE;
            sJob->mStmtType = aSQLStmtType;
            break;
        case ZK_PENDING_JOB_NONE:
            sJob->mIsRollbackJob = ID_FALSE;
            sJob->mIsCommitJob = ID_FALSE;
            sJob->mStmtType = QCI_STMT_MASK_MAX;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }
    
    /* add to list */
    IDU_LIST_INIT_OBJ( &(sJob->mListNode), sJob );

    IDU_LIST_ADD_LAST( &mJobAfterTransEnd, &(sJob->mListNode) );

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool sdiZookeeper::isExistPendingJob()
{
    if ( IDU_LIST_IS_EMPTY( &mJobAfterTransEnd ) != ID_TRUE )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

void sdiZookeeper::executePendingJob( ZKPendingJobType aPendingType )
{
    UInt             sPendingDDLLockTimeout = 3;
    iduListNode     *sIterator = NULL;
    iduListNode     *sNextNode = NULL;
    sdiZKPendingJob * sJob = NULL;
    idBool          sIsExecute = ID_FALSE;

    if ( IDU_LIST_IS_EMPTY( &mJobAfterTransEnd ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &mJobAfterTransEnd, sIterator, sNextNode )
        {
            sJob = (sdiZKPendingJob *)sIterator->mObj;
            IDU_LIST_REMOVE( &(sJob->mListNode) );
            sIsExecute = ID_FALSE;
            switch (aPendingType)
            {
                case ZK_PENDING_JOB_AFTER_COMMIT:
                    if ( sJob->mIsCommitJob == ID_TRUE )
                    {
                        ideLog::logLine(IDE_SD_17,"ZKC commit pending execute: %s", sJob->mSQL);
                        sIsExecute = ID_TRUE;
                    }
                    break;
                case ZK_PENDING_JOB_AFTER_ROLLBACK:
                    if ( sJob->mIsRollbackJob == ID_TRUE )
                    {
                        ideLog::logLine(IDE_SD_17,"ZKC rollback pending execute: %s", sJob->mSQL);
                        sIsExecute = ID_TRUE;
                    }
                    break;
                case ZK_PENDING_JOB_AFTER_END_ALL:
                    if ( ( sJob->mIsRollbackJob == ID_TRUE ) || ( sJob->mIsCommitJob == ID_TRUE ) )
                    {
                        sIsExecute = ID_TRUE;
                    }
                    break;
                case ZK_PENDING_JOB_NONE:
                default:
                    /* only remove */
                    break;
            }
            if ( sIsExecute == ID_TRUE )
            {
                if( sdi::shardExecTempSQLWithoutSession(sJob->mSQL, //idlOS::strlen(sJob->mSQL),
                                                        sJob->mNodeName,
                                                        sPendingDDLLockTimeout,
                                                        sJob->mStmtType ) != IDE_SUCCESS )
                {
                    IDE_ERRLOG(IDE_SD_4);
                    /* ideLog::logErrorMgrStackInternalForDebug(IDE_SD_0); */
                    ideLog::logLine( IDE_SD_4,
                                     "ZKC Pending Job failed: [NODE:%s], [JOB: %s]",
                                     sJob->mNodeName, sJob->mSQL);
                }
                else
                {
                    ideLog::logLine( IDE_SD_17,
                                 "ZKC Pending Job success: [NODE:%s], [JOB: %s]",
                                 sJob->mNodeName, sJob->mSQL);
                }
            }
            (void)iduMemMgr::free(sJob);
        }
    }
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &mJobAfterTransEnd ) == ID_TRUE );

    return;
}

void sdiZookeeper::removePendingJob()
{
    sdiZookeeper::executePendingJob( ZK_PENDING_JOB_NONE );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::callAfterCommitFunc          *
 * ------------------------------------------------------------------*
 * 수행 한 작업에 맞춰 해당하는 AfterCommit 함수를 호출한다.
 * 해당 함수는 mm 모듈에서 commit할때 호출되며 1차 수정에서 하지 않은 작업을 수행한다.
 *
 * aNewSMN         - [IN] 작업 수행후 zookeeper 메타에 저장할 SMN
 * aBeforeSMN      - [IN] 작업 수행전 zookeeper 메타에 저장되어 있는 SMN
 *********************************************************************/
void sdiZookeeper::callAfterCommitFunc( ULong   aNewSMN, ULong aBeforeSMN )
{
    ULong             sNewSMN = SDI_NULL_SMN;
    smiTrans          sTrans;
    smiStatement    * sDummySmiStmt = NULL;
    UInt              sTxState = 0;

    /* 연결 상태가 아니라면 뒷처리 할 작업이 없다. */
    if( isConnect() == ID_FALSE )
    {
        removePendingJob();
        IDE_CONT( normal_exit );
    }
    else
    {
        executePendingJob( ZK_PENDING_JOB_AFTER_COMMIT );
    }

    /* PROJ-2757 Advanced Global DDL
     *   Split Partition이 아닌 Global DDL은 SMN이 변경되지 않는다 */
    if( aNewSMN == SDI_NULL_SMN )
    {
        /* 현재 수행중인 shard ddl 구문으로 일반 tx에서 shard tx로 변환된 경우 */
        IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
        sTxState = 1;

        IDE_TEST( sTrans.begin( &sDummySmiStmt,
                                NULL,
                                ( SMI_ISOLATION_NO_PHANTOM     |
                                  SMI_TRANSACTION_NORMAL       |
                                  SMI_TRANSACTION_REPL_DEFAULT |
                                  SMI_COMMIT_WRITE_NOWAIT ) )
                  != IDE_SUCCESS );
        sTxState = 2;

        IDE_TEST( sdi::getIncreasedSMNForMetaNode( &sTrans, &sNewSMN ) != IDE_SUCCESS );

        IDE_TEST( sTrans.commit() != IDE_SUCCESS );
        sTxState = 1;

        sTxState = 0;
        IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

        if ( sNewSMN > sdi::getSMNForDataNode() )
        {
            sdi::setSMNForDataNode(sNewSMN );
            ideLog::log( IDE_SD_17, "[CHANGE_SHARD_META : RELOAD SMN(%"ID_UINT64_FMT")]", sNewSMN );
        }

    }
    else
    {
        sNewSMN = aNewSMN;
    }

    switch( mRunningJobType )
    {
        case ZK_JOB_ADD:
            IDE_DASSERT( sNewSMN != SDI_NULL_SMN );
            IDE_TEST( addNodeAfterCommit( sNewSMN ) != IDE_SUCCESS );
            sdi::setShardStatus(1); // SHARD_STATUS  0 -> 1
            (void)idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)0, 0, NULL ); //SHARD_ADMIN_MODE 1 -> 0
            break;
        case ZK_JOB_DROP:
            IDE_DASSERT( sNewSMN != SDI_NULL_SMN );
            IDE_TEST( dropNodeAfterCommit( sNewSMN ) != IDE_SUCCESS );
            /* shard status와 SHARD_ADMIN_MODE는 상위 함수(qdsd::executeZookeeperDrop)에서
             * 이미 바꿔주었으므로 여기서는 할 필요가 없다. */
            qcm::unsetShardNodeID();
            break;
        case ZK_JOB_DROP_FORCE:
            IDE_DASSERT(aNewSMN != SDI_NULL_SMN);
            IDE_TEST( dropForceAfterCommit( aNewSMN ) != IDE_SUCCESS );
            break;
        case ZK_JOB_JOIN:
            IDE_TEST( joinNodeAfterCommit() != IDE_SUCCESS );
            sdi::setShardStatus(1); // SHARD_STATUS  0 -> 1
            (void)idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)0, 0, NULL ); //SHARD_ADMIN_MODE 1 -> 0
            break;
        case ZK_JOB_FAILOVER:
        case ZK_JOB_FAILOVER_FOR_WATCHER:
            IDE_DASSERT( sNewSMN != SDI_NULL_SMN );
            IDE_DASSERT( aBeforeSMN != SDI_NULL_SMN );
            IDE_TEST( failoverAfterCommit( sNewSMN, aBeforeSMN ) != IDE_SUCCESS );
            break;
        case ZK_JOB_FAILBACK:
            IDE_DASSERT( sNewSMN != SDI_NULL_SMN );
            IDE_TEST( failbackAfterCommit( sNewSMN ) != IDE_SUCCESS );
            sdi::setShardStatus(1); // SHARD_STATUS  0 -> 1
            (void)idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)0, 0, NULL ); //SHARD_ADMIN_MODE 1 -> 0
            break;
        case ZK_JOB_SHUTDOWN:
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            break;
        case ZK_JOB_RESHARD:
            sdiZookeeper::updateSMN( sNewSMN );
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            break;

        /* PROJ-2757 Advanced Global DDL */
        case ZK_JOB_GLOBAL_DDL:
            if ( aNewSMN != aBeforeSMN )
            {
                sdiZookeeper::updateSMN( sNewSMN );
            }
            releaseShardMetaLock();
            break;

        case ZK_JOB_NONE:
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            break;
        default:
            IDE_CONT( normal_exit );
        case ZK_JOB_ERROR:
            IDE_DASSERT( 0 );
            break;
    }

    IDE_EXCEPTION_CONT( normal_exit );

    ideLog::log( IDE_SD_20, "[ZOOKEEPER] after commit for smn: SMN(%"ID_UINT64_FMT")", sNewSMN );

    return;

    IDE_EXCEPTION_END;

    switch( sTxState )
    {
        case 2:
            ( void )sTrans.rollback();
        case 1:
            ( void )sTrans.destroy( NULL );
        case 0:
        default:
            break;
    }

    IDE_ERRLOG(IDE_SD_0);
    releaseShardMetaLock();
    releaseZookeeperMetaLock();

    ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] after commit for smn: SMN(%"ID_UINT64_FMT")", aNewSMN );

    return;
}

void sdiZookeeper::callAfterRollbackFunc( ULong   aNewSMN, ULong aBeforeSMN )
{
    IDE_DASSERT(aNewSMN == SDI_NULL_SMN);
    PDL_UNUSED_ARG(aNewSMN);
    PDL_UNUSED_ARG(aBeforeSMN);
    sdiZookeeper::executePendingJob( ZK_PENDING_JOB_AFTER_ROLLBACK );

    switch( mRunningJobType )
    {
        case ZK_JOB_ADD:
            sdi::setShardStatus(0);
            (void)idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)1, 0, NULL );
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            sdiZookeeper::disconnect();
            qcm::unsetShardNodeID();
            break;
        case ZK_JOB_DROP:
            sdi::setShardStatus(1);
            (void)idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)0, 0, NULL );
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            break;
        case ZK_JOB_DROP_FORCE:
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            break;
        case ZK_JOB_JOIN:
            sdi::setShardStatus(0);
            (void)idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)1, 0, NULL );
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            sdiZookeeper::disconnect();
            qcm::unsetShardNodeID();
            break;
        case ZK_JOB_FAILOVER:
        case ZK_JOB_FAILOVER_FOR_WATCHER:
            sdiZookeeper::failoverAfterRollback();
            sdiZookeeper::setReplInfo4FailoverAfterCommit( SDM_NA_STR, SDM_NA_STR, SDM_NA_STR, SDM_NA_STR );
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            break;
        case ZK_JOB_FAILBACK:
            sdi::setShardStatus(0);
            (void)idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)1, 0, NULL );
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            sdiZookeeper::disconnect();
            qcm::unsetShardNodeID();
            break;
        case ZK_JOB_SHUTDOWN:
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            break;
        case ZK_JOB_RESHARD:
            sdiZookeeper::releaseShardMetaLock();
            sdiZookeeper::releaseZookeeperMetaLock();
            break;

        /* PROJ-2757 Advanced Global DDL */
        case ZK_JOB_GLOBAL_DDL:
            releaseShardMetaLock();
            break;

        case ZK_JOB_NONE:
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            break;
        default:
        case ZK_JOB_ERROR:
            IDE_DASSERT( 0 );
            releaseShardMetaLock();
            releaseZookeeperMetaLock();
            break;
    }

    return;
}

IDE_RC sdiZookeeper::addRevertJob( SChar           * aSQL,
                                   SChar           * aNodeName,
                                   ZKRevertJobType   aRevertType )
{
    sdiZKRevertJob * sJob = NULL;
    UInt             sJobSQLLen = 0;
    UInt             sJobSize = 0;

    sJobSQLLen = idlOS::strlen(aSQL);
    sJobSize   = idlOS::align8((UInt)(offsetof(sdiZKPendingJob, mSQL) + sJobSQLLen + 1));
    
    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_SDI,
                                     1,
                                     sJobSize,
                                     (void **)&sJob,
                                     IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );
    
    /* node name copy */
    idlOS::strncpy(sJob->mNodeName, (const SChar*)aNodeName, SDI_NODE_NAME_MAX_SIZE + 1);

    /* job sql copy */
    idlOS::strncpy(sJob->mSQL, (const SChar*)aSQL, sJobSQLLen + 1);

    sJob->mRevertJobType = aRevertType;
    
    /* add to list */
    IDU_LIST_INIT_OBJ( &(sJob->mListNode), sJob );

    IDU_LIST_ADD_LAST( &mRevertJobList, &(sJob->mListNode) );

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool sdiZookeeper::isExistRevertJob()
{
    if ( IDU_LIST_IS_EMPTY( &mRevertJobList ) != ID_TRUE )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

void sdiZookeeper::executeRevertJob( ZKRevertJobType aRevertType )
{
    UInt              sPendingDDLLockTimeout = 3;
    iduListNode     * sIterator = NULL;
    iduListNode     * sNextNode = NULL;
    sdiZKRevertJob  * sJob = NULL;
    idBool            sIsExecute = ID_FALSE;

    if ( IDU_LIST_IS_EMPTY( &mRevertJobList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &mRevertJobList, sIterator, sNextNode )
        {
            sJob = (sdiZKRevertJob *)sIterator->mObj;
            sIsExecute = ID_FALSE;
            
            switch ( aRevertType )
            {
                case ZK_REVERT_JOB_REPL_DROP:
                    if ( sJob->mRevertJobType == ZK_REVERT_JOB_REPL_DROP )
                    {
                        ideLog::logLine(IDE_SD_17,"ZKC revert execute: %s", sJob->mSQL);
                        sIsExecute = ID_TRUE;
                    }
                    break;
                case ZK_REVERT_JOB_REPL_ITEM_DROP:
                    if ( sJob->mRevertJobType == ZK_REVERT_JOB_REPL_ITEM_DROP )
                    {
                        ideLog::logLine(IDE_SD_17,"ZKC revert execute: %s", sJob->mSQL);
                        sIsExecute = ID_TRUE;
                    }
                    break;
                case ZK_REVERT_JOB_TABLE_ALTER:
                    /* TASK-7307 DML Data Consistency in Shard */
                    if ( sJob->mRevertJobType == ZK_REVERT_JOB_TABLE_ALTER )
                    {
                        ideLog::logLine(IDE_SD_17,"ZKC revert execute: %s", sJob->mSQL);
                        sIsExecute = ID_TRUE;
                    }
                    break;
                case ZK_REVERT_JOB_NONE:
                    IDU_LIST_REMOVE( &(sJob->mListNode) );
                    (void)iduMemMgr::free(sJob);
                    break;
                default:
                    /* only remove */
                    break;
            }
            
            if ( sIsExecute == ID_TRUE )
            {
                IDU_LIST_REMOVE( &(sJob->mListNode) );

                if ( sdi::shardExecTempSQLWithoutSession( sJob->mSQL,
                                                          sJob->mNodeName,
                                                          sPendingDDLLockTimeout,
                                                          QCI_STMT_MASK_MAX )
                     != IDE_SUCCESS )
                {
                    IDE_ERRLOG(IDE_SD_4);
                    /* ideLog::logErrorMgrStackInternalForDebug(IDE_SD_0); */
                    ideLog::logLine( IDE_SD_4,
                                     "ZKC Revert Job failed: [NODE:%s], [JOB: %s]",
                                     sJob->mNodeName, sJob->mSQL);
                }
                else
                {
                    ideLog::logLine( IDE_SD_17,
                                 "ZKC Revert Job success: [NODE:%s], [JOB: %s]",
                                 sJob->mNodeName, sJob->mSQL);
                }
                
                (void)iduMemMgr::free(sJob);
            }
        }
    }
    
    return;
}

void sdiZookeeper::removeRevertJob()
{
    sdiZookeeper::executeRevertJob( ZK_REVERT_JOB_NONE );
}

IDE_RC sdiZookeeper::checkExistNotFailedoverDeadNode( idBool * aExistNotFailedoverDeadNode )
{
    idBool            sExist = ID_FALSE;
    idBool            sIsAlive = ID_FALSE;
    iduList         * sList = NULL;
    iduListNode     * sIterator = NULL;
    SChar             sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    idBool            sIsAllocList = ID_FALSE;
    SInt              sDataLength;
    SChar             sNodeName[SDI_NODE_NAME_MAX_SIZE + 1 ] = {0,};

    IDE_TEST( getAllNodeNameList( &sList ) != IDE_SUCCESS );
    sIsAllocList = ID_TRUE;

    IDU_LIST_ITERATE( sList, sIterator )
    {
        idlOS::strncpy( sNodeName,
                        (SChar *) sIterator->mObj,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        IDE_TEST( checkNodeAlive( sNodeName,
                                  &sIsAlive ) != IDE_SUCCESS );

        if ( sIsAlive == ID_FALSE )
        {
            sDataLength = SDI_ZKC_BUFFER_SIZE;
            IDE_TEST( getNodeInfo( sNodeName,
                                   (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                   sBuffer,
                                   &sDataLength ) != IDE_SUCCESS );

            if ( sDataLength <= 0 )
            {
                sExist = ID_TRUE;
            }
        }
    }

    *aExistNotFailedoverDeadNode =  sExist;

    if ( sIsAllocList == ID_TRUE )
    {
        sIsAllocList = ID_FALSE;
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsAllocList == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getZookeeperSMN( ULong * aSMN )
{

    SInt  sDataLength = 0;
    SChar sBuffer[SDI_ZKC_BUFFER_SIZE];

    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    
    IDE_TEST( sdiZookeeper::readInfo( (SChar*)SDI_ZKC_PATH_SMN,
                                      sBuffer,
                                      &sDataLength ) != IDE_SUCCESS );

    *aSMN = idlOS::strToULong ( (UChar*)sBuffer, idlOS::strlen( sBuffer) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::getZookeeperInfo4PV          *
 * ------------------------------------------------------------------*
 * X$ZOOKEEPER_DATA_INFO를 출력하기 위해 zookeeper server에게 데이터를 가져온다.
 * 저장된 데이터는 list의 형태를 가지므로 사용이 끝난 후에 freeList 함수를 호출해야 한다.
 *
 * aList - [OUT] zookeeper에게 가져온 데이터가 저장될 list의 header
 *********************************************************************/
IDE_RC sdiZookeeper::getZookeeperInfo4PV( iduList ** aList )
{
    ZKCResult           sResult;

    /* list */
    idBool              sIsAlloced = ID_FALSE;
    iduList           * sList = NULL;

    /* for */
    SInt                i = 0;
    SInt                j = 0;
    SInt                k = 0;

    /* subpath */
    SChar               sPathLevel1[SDI_ZKC_PATH_LENGTH] = {0,};
    String_vector       sChildrenLevel1;
    SChar               sPathLevel2[SDI_ZKC_PATH_LENGTH] = {0,};
    String_vector       sChildrenLevel2;
    SChar               sPathLevel3[SDI_ZKC_PATH_LENGTH] = {0,};
    String_vector       sChildrenLevel3;
    SChar               sPathLevel4[SDI_ZKC_PATH_LENGTH] = {0,};

    /* list 초기화 */
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                  ID_SIZEOF( iduList ),
                                  (void**)&( sList ),
                                  IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
    IDU_LIST_INIT( sList );

    sIsAlloced = ID_TRUE;

    /* zookeeper로부터 데이터를 받아온다. */
    idlOS::snprintf( sPathLevel1, 
                     SDI_ZKC_PATH_LENGTH, 
                     "%s", 
                     SDI_ZKC_PATH_ALTIBASE_SHARD );

    IDE_TEST( insertList4PV( sList, sPathLevel1 ) != IDE_SUCCESS );

    sResult = aclZKC_getChildren( mZKHandler,
                                  sPathLevel1,
                                  &sChildrenLevel1 );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    for( i = 0; i < sChildrenLevel1.count; i++ )
    {
        idlOS::memset( sPathLevel2, 0, SDI_ZKC_BUFFER_SIZE );

        idlOS::snprintf( sPathLevel2, 
                         SDI_ZKC_PATH_LENGTH, 
                         "%s/%s", 
                         sPathLevel1, sChildrenLevel1.data[i] );

        IDE_TEST( insertList4PV( sList, sPathLevel2 ) != IDE_SUCCESS );

        sResult = aclZKC_getChildren( mZKHandler,
                                      sPathLevel2,
                                      &sChildrenLevel2 );

        IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

        for( j = 0; j < sChildrenLevel2.count; j++ )
        {
            idlOS::memset( sPathLevel3, 0, SDI_ZKC_BUFFER_SIZE );

            idlOS::snprintf( sPathLevel3, 
                             SDI_ZKC_PATH_LENGTH, 
                             "%s/%s", 
                             sPathLevel2, sChildrenLevel2.data[j] );

            IDE_TEST( insertList4PV( sList, sPathLevel3 ) != IDE_SUCCESS );

            sResult = aclZKC_getChildren( mZKHandler,
                                          sPathLevel3,
                                          &sChildrenLevel3 );

            IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

            for( k = 0; k < sChildrenLevel3.count; k++ )
            {
                idlOS::memset( sPathLevel4, 0, SDI_ZKC_BUFFER_SIZE );

                idlOS::snprintf( sPathLevel4, 
                                 SDI_ZKC_PATH_LENGTH, 
                                 "%s/%s", 
                                 sPathLevel3, sChildrenLevel3.data[k] );

                IDE_TEST( insertList4PV( sList, sPathLevel4 ) != IDE_SUCCESS );

                /* 현재 구조상으로는 zookeeper data level이 4단계가 한계이므로 여기서 멈춘다.      *
                 * 차후 zookeeper data 구조가 변경되어 level이 변경될 경우 이 이하를 탐색해야 한다.*/
            }
        }
    }

    *aList = sList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    if( sIsAlloced == ID_TRUE )
    {
        freeList( sList, SDI_ZKS_LIST_PRINT );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdiZookeeper::insertList4PV                *
 * ------------------------------------------------------------------*
 * 주어진 zookeeper path에서 값을 가져와 list에 삽입한다.
 * 만약 데이터가 없다면 NULL을 대신 삽입한다.
 *
 * aList          - [IN] zookeeper에게 가져온 데이터가 저장될 list의 header
 * aZookeeperPath - [IN] 데이터를 읽을 zookeeper path 
 *********************************************************************/
IDE_RC sdiZookeeper::insertList4PV( iduList * aList,
                                    SChar   * aZookeeperPath )
{
    ZKCResult               sResult;
    SChar                   sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SInt                    sDataLength = SDI_ZKC_BUFFER_SIZE;
    iduListNode *           sNode = NULL;
    sdiZookeeperInfo4PV *   sInfo4PV = NULL;
    SInt                    sState = 0;

    IDE_DASSERT( aList != NULL );

    sResult = aclZKC_getInfo( mZKHandler,
                              aZookeeperPath,
                              sBuffer,
                              &sDataLength );

    IDE_TEST_RAISE( sResult != ZKC_SUCCESS, err_ZKC );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                 ID_SIZEOF(iduListNode),
                                 (void**)&(sNode),
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SDI,
                                 ID_SIZEOF(sdiZookeeperInfo4PV),
                                 (void**)&(sInfo4PV),
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
    sState = 2;

    idlOS::memset( sInfo4PV->mPath, 0, SDI_ZKC_PATH_LENGTH );
    idlOS::memset( sInfo4PV->mData, 0, SDI_ZKC_BUFFER_SIZE );

    idlOS::strncpy( sInfo4PV->mPath,
                    aZookeeperPath,
                    SDI_ZKC_PATH_LENGTH );

    if( sDataLength > 0 )
    {
        idlOS::strncpy( sInfo4PV->mData,
                        sBuffer,
                        SDI_ZKC_BUFFER_SIZE );
    }
    else
    {
        idlOS::strncpy( sInfo4PV->mData,
                        "NULL",
                        4 );
    }  

    IDU_LIST_INIT_OBJ( sNode, (void*)sInfo4PV );

    IDU_LIST_ADD_LAST( aList, sNode );

    sState = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_ZKC )
    {
        logAndSetErrMsg( sResult );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free( sInfo4PV );
        case 1:
            (void)iduMemMgr::free( sNode );
        case 0:
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

void sdiZookeeper::globalDDLJob()
{
    mRunningJobType = ZK_JOB_GLOBAL_DDL;
}

