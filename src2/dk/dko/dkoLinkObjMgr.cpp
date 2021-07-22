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
 * $id$
 **********************************************************************/

#include <dkoLinkObjMgr.h>

#include <dkoLinkInfo.h>

#include <dkdResultSetMetaCache.h>

/************************************************************************
 * Description : DK object manager 를 초기화한다.
 *
 ************************************************************************/
IDE_RC  dkoLinkObjMgr::initializeStatic()
{
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
    
    return IDE_SUCCESS;

#else
    IDE_TEST( dkoLinkInfoInitialize() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/************************************************************************
 * Description : DK object manager 를 정리한다.
 *
 ************************************************************************/
IDE_RC  dkoLinkObjMgr::finalizeStatic()
{
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */

    return IDE_SUCCESS;
#else
    IDE_TEST( dkoLinkInfoFinalize() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/************************************************************************
 * Description : SYS_DATABASE_LINKS_ 메타테이블을 읽어 기존에 존재하는 
 *               DB-Link 에 대한 DK link object 를 리스트로 생성한다.
 *
 ************************************************************************/
IDE_RC  dkoLinkObjMgr::createLinkObjectsFromMeta( idvSQL * aStatistics )
{
    qciDatabaseLinksItem    *sDblinkFirstItem = NULL;    
    qciDatabaseLinksItem    *sDblinkItem      = NULL;
    SInt                     sStage = 0;
    
    IDE_TEST( qciMisc::getDatabaseLinkFirstItem( aStatistics, &sDblinkFirstItem ) 
              != IDE_SUCCESS );
    sStage = 1;
    
    sDblinkItem = sDblinkFirstItem;
    
    while ( sDblinkItem != NULL )
    {
#ifdef ALTIBASE_PRODUCT_XDB
        /* nothing to do */
#else
        IDE_TEST( dkoLinkInfoCreate( sDblinkItem->linkID ) != IDE_SUCCESS )
#endif
        IDE_TEST( qciMisc::getDatabaseLinkNextItem( sDblinkItem, 
                                                    &sDblinkItem )
                  != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( qciMisc::freeDatabaseLinkItems( sDblinkFirstItem )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
        
    switch( sStage )
    {
        case 1:
            (void)qciMisc::freeDatabaseLinkItems( sDblinkFirstItem );
            /* keep going */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 새로운 데이터베이스 링크 객체를 하나 생성하여 list 에 
 *               추가해준다.
 *  
 *  aQcStatement    - [IN] SM 함수 호출시 필요한 QcStatement 정보
 *  aUserId         - [IN] 데이터베이스 링크를 생성한 user 의 id
 *  aNewDblinkId    - [IN] 데이터베이스 링크의 id
 *  aLinkType       - [IN] 생성하려는 데이터베이스 링크의 유형
 *
 *      @ Database Link Type
 *          0: Heterogeneous database link
 *          1: Homogeneous database link
 *          
 *  aUserMode       - [IN] 생성하려는 데이터베이스 링크가 public 인지 
 *                         private 인지 여부
 *  aLinkName       - [IN] 데이터베이스 링크의 이름
 *  aTargetName     - [IN] Target remote server 의 이름
 *  aRemoteUserId   - [IN] Target remote server 의 접속 id
 *  aRemoteUserPasswd - [IN] Target remote server 의 접속 password
 *  aLinkObj        - [OUT] 생성한 데이터베이스 링크 객체에 대한 포인터
 *
 ************************************************************************/
IDE_RC  dkoLinkObjMgr::createLinkObject( void       *aQcStatement, 
                                         SInt        aUserId,
                                         UInt        aNewDblinkId,
                                         SInt        aLinkType, 
                                         SInt        aUserMode, 
                                         SChar      *aLinkName, 
                                         SChar      *aTargetName, 
                                         SChar      *aRemoteUserId, 
                                         SChar      *aRemoteUserPasswd, 
                                         idBool      aPublicFlag )
{
    UInt                     sStage     = 0;
    smiStatement            *sStatement = NULL;
    dkaTargetInfo            sTargetInfo;
    const void              *sLinkHandle = NULL;
    qciDatabaseLinksItem     sItem;
    idBool                   sExistFlag = ID_FALSE;
    
    idlOS::memset( &sTargetInfo, 0, ID_SIZEOF( sTargetInfo ) );

    /* 1. Validate target name */
    IDE_TEST( dkaLinkerProcessMgr::getTargetInfo( aTargetName, &sTargetInfo )
              != IDE_SUCCESS );

    /* 2. Check link object already exists */
    IDE_TEST( isExistLinkObject( qciMisc::getStatisticsFromQcStatement( aQcStatement ),
                                 aLinkName,
                                 &sExistFlag )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sExistFlag == ID_TRUE, ERR_DBLINK_ALREADY_EXIST );

    /* 3. Create sm object */
    IDE_TEST( qciMisc::getSmiStatement( aQcStatement, &sStatement ) 
              != IDE_SUCCESS );

    IDE_TEST( smiObject::createObject( sStatement,
                                       NULL,
                                       0,
                                       NULL,
                                       SMI_OBJECT_DATABASE_LINK,
                                       &sLinkHandle )
              != IDE_SUCCESS );
    sStage = 1;

    /* 4. Create DK link object */
    idlOS::memset( &sItem, 0, ID_SIZEOF( sItem ) );

    sItem.userID        = aUserId;
    sItem.linkID        = aNewDblinkId;
    sItem.linkOID       = smiGetTableId( sLinkHandle );
    sItem.userMode      = aUserMode;
    sItem.linkType      = aLinkType;

    idlOS::strncpy( sItem.linkName, aLinkName, ID_SIZEOF( sItem.linkName ) );
    idlOS::strncpy( sItem.targetName, aTargetName, ID_SIZEOF( sItem.targetName ) );

    if ( idlOS::strlen( aRemoteUserId ) != 0 )
    {
        idlOS::strncpy( sItem.remoteUserID,
                        aRemoteUserId,
                        ID_SIZEOF( sItem.remoteUserID ) );
    }
    else
    {
        if ( idlOS::strlen( sTargetInfo.mRemoteServerUserID ) != 0 )
        {
            idlOS::strncpy( sItem.remoteUserID,
                            sTargetInfo.mRemoteServerUserID,
                            ID_SIZEOF( sItem.remoteUserID ) );
        }
        else
        {
            /* nothing to do */
        }
    }

    if ( idlOS:: strlen( aRemoteUserPasswd ) != 0 )
    {
        idlOS::strncpy( (SChar *)sItem.remoteUserPassword,
                        aRemoteUserPasswd,
                        ID_SIZEOF( sItem.remoteUserPassword ) );
    }
    else
    {
        if ( idlOS::strlen( sTargetInfo.mRemoteServerPasswd ) != 0 )
        {
            idlOS::strncpy( (SChar *)sItem.remoteUserPassword,
                            sTargetInfo.mRemoteServerPasswd,
                            ID_SIZEOF( sItem.remoteUserPassword ) );
        }
        else
        {
            /* nothing to do */
        }
    }

    IDE_TEST( qciMisc::insertDatabaseLinkItem( aQcStatement, &sItem, aPublicFlag ) != IDE_SUCCESS );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoCreate( aNewDblinkId ) != IDE_SUCCESS )
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DBLINK_ALREADY_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKO_DBLINK_ALREADY_EXIST ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)smiObject::dropObject( sStatement, sLinkHandle );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : aSrcLinkObj 로 입력받은 데이터베이스 링크 객체를 
 *               aDestLinkObj 가 가리키는 주소에 copy 한다.
 *               
 *  aSrcLinkObj     - [IN] Source database link object
 *  aDestLinkObj    - [IN] Destination database link object
 *
 ************************************************************************/
void    dkoLinkObjMgr::copyLinkObject( dkoLink     *aSrcLinkObj,
                                       dkoLink     *aDestLinkObj )
{
    IDE_ASSERT( aSrcLinkObj != NULL );
    IDE_ASSERT( aDestLinkObj != NULL );

    aDestLinkObj->mId       = aSrcLinkObj->mId;
    aDestLinkObj->mUserId   = aSrcLinkObj->mUserId;
    aDestLinkObj->mUserMode = aSrcLinkObj->mUserMode;
    aDestLinkObj->mLinkType = aSrcLinkObj->mLinkType;
    aDestLinkObj->mLinkOID  = aSrcLinkObj->mLinkOID;

    /* mendatory */
    idlOS::strncpy( aDestLinkObj->mLinkName,
                    aSrcLinkObj->mLinkName,
                    ID_SIZEOF( aDestLinkObj->mLinkName ) );
    idlOS::strncpy( aDestLinkObj->mTargetName,
                    aSrcLinkObj->mTargetName,
                    ID_SIZEOF( aDestLinkObj->mTargetName ) );

    /* optional */
    if ( idlOS::strlen( aSrcLinkObj->mRemoteUserId ) != 0 )
    {
        idlOS::strncpy( aDestLinkObj->mRemoteUserId, 
                        aSrcLinkObj->mRemoteUserId,
                        ID_SIZEOF( aDestLinkObj->mRemoteUserId ) );
    }
    else
    {
        /* do nothing */
    }

    /* optional */
    if ( idlOS::strlen( aSrcLinkObj->mRemoteUserPasswd ) != 0 )
    {
        idlOS::strncpy( aDestLinkObj->mRemoteUserPasswd, 
                        aSrcLinkObj->mRemoteUserPasswd,
                        ID_SIZEOF( aDestLinkObj->mRemoteUserPasswd ) );
    }
    else
    {
        /* do nothing */
    }
}

/************************************************************************
 * Description : 입력받은 데이터베이스 링크 객체를 초기화한다.
 *               이 함수는 dkoLinkObjMgr 에 의해 list 로 관리되는 링크 
 *               객체 이외의 링크객체에 대해 사용될 수 있으며, list 및 
 *               mutex 관련 초기화는 수행하지 않는다.
 *  
 *  aLinkObj     - [IN] 초기화할 데이터베이스 링크 객체를 가리키는 포인터
 *
 ************************************************************************/
void    dkoLinkObjMgr::initCopiedLinkObject( dkoLink   *aLinkObj )
{
    IDE_ASSERT( aLinkObj != NULL );

    idlOS::memset( aLinkObj, 0, ID_SIZEOF( dkoLink ) );

    aLinkObj->mId       = DK_INVALID_LINK_ID;
    aLinkObj->mUserId   = DK_INVALID_USER_ID;
    aLinkObj->mUserMode = DKO_LINK_USER_MODE_PUBLIC;
}

/************************************************************************
 * Description : 데이터베이스 링크 객체를 list 로부터 제거하고 할당받은 
 *               자원을 반환한다.
 *
 *  aQcStatement - [IN] 메타에서 제거하기 위하여 필요한 qp 정보
 *  aLinkObj     - [IN] 제거할 데이터베이스 링크 객체 
 *
 ************************************************************************/
IDE_RC  dkoLinkObjMgr::destroyLinkObject( void      *aQcStatement,
                                          dkoLink   *aLinkObj )
{
    smiStatement   *sStatement = NULL;  
    void           *sLinkHandle;
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;
    UInt            sReferenceCount = 0;
#endif
    
    IDE_ASSERT( aLinkObj != NULL );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoGetStatus( aLinkObj->mId, &sStatus ) != IDE_SUCCESS );
    IDE_TEST( dkoLinkInfoGetReferenceCount( aLinkObj->mId, &sReferenceCount ) != IDE_SUCCESS );
    
    IDE_TEST_RAISE( ( sReferenceCount > 0 ) || ( sStatus == DKO_LINK_OBJ_ACTIVE ),
                    ERR_DBLINK_IS_BEING_USED );
#endif

    /* 1. Remove sm object */
    sLinkHandle = (void *)smiGetTable( aLinkObj->mLinkOID );

    IDE_TEST( qciMisc::getSmiStatement( aQcStatement, &sStatement ) 
              != IDE_SUCCESS );

    IDE_TEST( smiObject::dropObject( sStatement, sLinkHandle ) 
              != IDE_SUCCESS ); 

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    /* 2. Remove link object from list */
    IDE_TEST( dkoLinkInfoDestroy( aLinkObj->mId ) != IDE_SUCCESS );
#endif
    
    return IDE_SUCCESS;

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_EXCEPTION( ERR_DBLINK_IS_BEING_USED );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKO_DBLINK_IS_BEING_USED ) );
    }
#endif
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkoLinkObjMgr::destroyLinkObjectByUserId( idvSQL *aStatistics,
                                                 void   *aQcStatement,
                                                 UInt    aUserId )
{
    qciDatabaseLinksItem    *sDblinkFirstItem = NULL;    
    qciDatabaseLinksItem    *sDblinkItem      = NULL;
    SInt                     sStage = 0;
    smiStatement            *sStatement = NULL;
    void                    *sLinkHandle = NULL;
    
    IDE_TEST( qciMisc::getDatabaseLinkFirstItem( aStatistics, &sDblinkFirstItem ) 
              != IDE_SUCCESS );
    sStage = 1;
    
    sDblinkItem = sDblinkFirstItem;
    
    while ( sDblinkItem != NULL )
    {
        if ( sDblinkItem->userID == aUserId )
        {
            IDE_TEST( dkdResultSetMetaCacheDelete( sDblinkItem->linkName ) != IDE_SUCCESS );

#ifdef ALTIBASE_PRODUCT_XDB
            /* nothing to do */
#else
            IDE_TEST( dkoLinkInfoDestroy( sDblinkItem->linkID ) != IDE_SUCCESS );
#endif

            sLinkHandle = (void *)smiGetTable( sDblinkItem->linkOID );

            IDE_TEST( qciMisc::getSmiStatement( aQcStatement, &sStatement ) != IDE_SUCCESS );

            IDE_TEST( smiObject::dropObject( sStatement, sLinkHandle ) != IDE_SUCCESS );


        }
        else
        {
            /* nothing to do */
        }
        
        IDE_TEST( qciMisc::getDatabaseLinkNextItem( sDblinkItem, 
                                                    &sDblinkItem )
                  != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( qciMisc::freeDatabaseLinkItems( sDblinkFirstItem )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
        
    switch( sStage )
    {
        case 1:
            (void)qciMisc::freeDatabaseLinkItems( sDblinkFirstItem );
            /* keep going */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;        
        
}

/*
 *
 */
void dkoLinkObjMgr::validateLinkObjectInternal( void        *aQcStatement,
                                                dkoLink     *aLinkObj,
                                                idBool      *aIsValid )
{
    if ( isPublicLink( aLinkObj ) != ID_TRUE )
    {
        if ( qciMisc::isSysdba( aQcStatement ) == ID_TRUE )
        {
            *aIsValid = ID_TRUE;
        }
        else
        {
            if ( qciMisc::getSessionUserID( aQcStatement ) 
                 == getLinkUserId( aLinkObj ) )
            {
                *aIsValid = ID_TRUE;
            }
            else
            {
                *aIsValid = ID_FALSE;
            }
        }
    }
    else
    {
        *aIsValid = ID_TRUE;
    }    
}

/************************************************************************
 * Description : 입력받은 dblink 객체가 사용할 수 있는 상태인지 검사한다. 
 *              
 *  aQcStatement - [IN] 세션으로 부터 user id 를 얻어오기 위해 필요한 
 *                      qp 정보
 *  aSession     - [IN] Linker data session
 *  aLinkObj     - [IN] 검사할 데이터베이스 링크 객체 
 *
 ************************************************************************/
IDE_RC  dkoLinkObjMgr::validateLinkObject( void             *aQcStatement,
                                           dksDataSession   *aSession,
                                           dkoLink          *aLinkObj )
{
    idBool  sIsValid = ID_FALSE;
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;
#endif
    
    IDE_ASSERT( aSession != NULL );

    IDE_TEST_RAISE( aLinkObj == NULL, ERR_DBLINK_NOT_EXIST );

#ifdef ALTIBASE_PRODUCT_XDB
    
    validateLinkObjectInternal( aQcStatement, aLinkObj, &sIsValid );
    
#else
    /* Validate link object */
    IDE_TEST( dkoLinkInfoGetStatus( aLinkObj->mId, &sStatus ) != IDE_SUCCESS );
    if ( sStatus > DKO_LINK_OBJ_META )
    {
        validateLinkObjectInternal( aQcStatement, aLinkObj, &sIsValid );
    }
    else
    {
        sIsValid = ID_FALSE;
    }
#endif /* ALTIBASE_PRODUCT_XDB */
    
#ifdef ALTIBASE_PRODUCT_XDB
    if ( sIsValid != ID_TRUE )
    {
        IDE_RAISE( ERR_INVALID_DBLINK );
    }
    else
    {
        /* nothing to do */
    }
#else
     /* Change the status of link object */
    if ( sIsValid != ID_TRUE )
    {
        UInt sReferenceCount = 0;

        IDE_TEST( dkoLinkInfoGetReferenceCount( aLinkObj->mId, &sReferenceCount ) != IDE_SUCCESS );
        if ( sReferenceCount == 0 )
        {
            IDE_TEST( dkoLinkInfoSetStatus( aLinkObj->mId, DKO_LINK_OBJ_READY ) != IDE_SUCCESS );
        }
        else
        {
            /* remain current status */
        }

        IDE_RAISE( ERR_INVALID_DBLINK );
    }
    else
    {
        /* valid link */
    }
#endif
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKO_DBLINK_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 입력받은 dblink 객체가 사용할 수 있는 상태인지 검사한다. 
 *              
 *  aQcStatement - [IN] 세션으로 부터 user id 를 얻어오기 위해 필요한 
 *                      qp 정보
 *  aLinkObj     - [IN] 검사할 데이터베이스 링크 객체 
 *  aIsValid     - [OUT] 이 link object 에 대한 접근권한이 있는지를
 *                       나타내며 접근권한이 있는 경우 ID_TRUE, 아니면
 *                       ID_FALSE 
 *
 ************************************************************************/
IDE_RC   dkoLinkObjMgr::validateLinkObjectUser( void     *aQcStatement, 
                                                dkoLink  *aLinkObj, 
                                                idBool   *aIsValid )
{
    idBool  sIsPublic;

    IDE_TEST_RAISE( aLinkObj == NULL, ERR_DBLINK_NOT_EXIST );

    sIsPublic = isPublicLink( aLinkObj );

    if ( sIsPublic != ID_TRUE )
    {
        if ( qciMisc::isSysdba( aQcStatement ) == ID_TRUE )
        {
            *aIsValid = ID_TRUE;
        }
        else
        {
            if ( qciMisc::getSessionUserID( aQcStatement ) == aLinkObj->mUserId )
            {
                *aIsValid = ID_TRUE;
            }
            else
            {
                *aIsValid = ID_FALSE;
            }
        }
    }
    else
    {
        *aIsValid = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKO_DBLINK_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : aLinkName 에 해당하는 dblink 객체를 찾아 반환한다.
 *              
 *  aLinkName    - [IN] 찾을 데이터베이스 링크 객체의 이름
 *  aLinkObj     - [OUT] 찾은 데이터베이스 링크 객체를 가리키는 포인터
 *
 ************************************************************************/
IDE_RC  dkoLinkObjMgr::findLinkObject( idvSQL *aStatistics, SChar *aLinkName, dkoLink *aLinkObj )
{
    idBool                  sFoundFlag = ID_FALSE;
    qciDatabaseLinksItem   *sDblinkFirstItem = NULL;
    qciDatabaseLinksItem   *sDblinkItem = NULL;
    SInt                    sStage = 0;

    IDE_DASSERT( aLinkObj != NULL );
    
    IDE_TEST( qciMisc::getDatabaseLinkFirstItem( aStatistics, &sDblinkFirstItem ) != IDE_SUCCESS );
    sStage = 1;

    sDblinkItem = sDblinkFirstItem;
    while ( sDblinkItem != NULL )
    {
        if ( idlOS::strcasecmp( sDblinkItem->linkName, aLinkName ) == 0 )
        {
            aLinkObj->mUserId    = sDblinkItem->userID;
            aLinkObj->mId        = sDblinkItem->linkID;
            aLinkObj->mLinkOID   = sDblinkItem->linkOID;
            aLinkObj->mUserMode  = sDblinkItem->userMode;
            aLinkObj->mLinkType  = sDblinkItem->linkType;

            idlOS::strncpy( aLinkObj->mLinkName,
                            sDblinkItem->linkName,
                            ID_SIZEOF( aLinkObj->mLinkName ) );
            idlOS::strncpy( aLinkObj->mTargetName,
                            sDblinkItem->targetName,
                            ID_SIZEOF( aLinkObj->mTargetName ) );
            idlOS::strncpy( aLinkObj->mRemoteUserId,
                            sDblinkItem->remoteUserID,
                            ID_SIZEOF( aLinkObj->mRemoteUserId ) );
            idlOS::strncpy( aLinkObj->mRemoteUserPasswd,
                            (SChar *)sDblinkItem->remoteUserPassword,
                            ID_SIZEOF( aLinkObj->mRemoteUserPasswd ) );

            sFoundFlag = ID_TRUE;
            break;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( qciMisc::getDatabaseLinkNextItem( sDblinkItem, &sDblinkItem ) != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( qciMisc::freeDatabaseLinkItems( sDblinkFirstItem ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sFoundFlag == ID_FALSE, ERR_INVALID_DBLINK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)qciMisc::freeDatabaseLinkItems( sDblinkFirstItem );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 입력받은 DK link object 가 존재하는지 검사한다.
 *
 * Return      : 해당 데이터베이스 링크 객체가 존재하는지 여부를 반환.
 *               존재하는 경우 ID_TRUE, 그렇지 않으면 ID_FALSE.
 *              
 *  aLinkName    - [IN] 찾을 데이터베이스 링크 객체의 이름
 *
 ************************************************************************/
IDE_RC dkoLinkObjMgr::isExistLinkObject( idvSQL *aStatistics, SChar *aLinkName, idBool *aIsExist )
{
    idBool                  sFoundFlag = ID_FALSE;
    qciDatabaseLinksItem   *sDblinkFirstItem = NULL;
    qciDatabaseLinksItem   *sDblinkItem = NULL;
    SInt                    sStage = 0;
    
    IDE_TEST( qciMisc::getDatabaseLinkFirstItem( aStatistics, &sDblinkFirstItem ) != IDE_SUCCESS );
    sStage = 1;

    sDblinkItem = sDblinkFirstItem;
    while ( sDblinkItem != NULL )
    {
        if ( idlOS::strcmp( sDblinkItem->linkName, aLinkName ) == 0 )
        {
            sFoundFlag = ID_TRUE;
            break;
        }
        else
        {
            /* Iterate next */
        }        

        IDE_TEST( qciMisc::getDatabaseLinkNextItem( sDblinkItem, &sDblinkItem ) != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( qciMisc::freeDatabaseLinkItems( sDblinkFirstItem ) != IDE_SUCCESS );

    *aIsExist = sFoundFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)qciMisc::freeDatabaseLinkItems( sDblinkFirstItem );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

