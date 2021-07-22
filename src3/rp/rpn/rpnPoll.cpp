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
 

#include <idl.h>
#include <ide.h>
#include <idu.h>

#include <rpError.h>

#include <rpnSocket.h>
#include <rpnPoll.h>

static acp_sint32_t dispatcherCallback( acp_poll_set_t          * /*aPollSet*/,
                                        const acp_poll_obj_t    * aPollObj,
                                        void                    * aData )
{
    rpnPoll     * sPoll     = (rpnPoll*)aData;

    IDE_TEST( sPoll->mCallback( sPoll,
                                (rpnSocket*)aPollObj->mUserData )
              != IDE_SUCCESS );

    return ACP_RC_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );

    return ACP_RC_ECANCELED;
}

IDE_RC rpnPollInitialize( rpnPoll   * aPoll,
                          SInt        aMaxCount )
{
    IDE_TEST_RAISE( acpPollCreate( &(aPoll->mPollSet),
                                   aMaxCount )
                    != ACP_RC_SUCCESS, ERR_POLL_CREATE );

    aPoll->mCount = 0;
    aPoll->mCallback = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_POLL_CREATE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_POLL_CREATE_ERROR  ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpnPollFinalize(rpnPoll * aPoll )
{
    IDE_DASSERT( aPoll->mCount == 0 );

    (void)acpPollDestroy( &(aPoll->mPollSet) );
    aPoll->mCount = 0;
    aPoll->mCallback = NULL;
}

IDE_RC rpnPollAddSocket( rpnPoll      * aPoll,
                         rpnSocket    * aSocket,
                         void         * aData,
                         SInt           aEvent )
{
    IDU_FIT_POINT( "rpnPoll::rpnPollAddSocket::acpPollAddSock::Event" );
    IDE_TEST( acpPollAddSock( &(aPoll->mPollSet),
                              &(aSocket->mSocket),
                              aEvent,
                              aData )
              != IDE_SUCCESS );

    aPoll->mCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpnPollRemoveSocket( rpnPoll     * aPoll,
                          rpnSocket   * aSocket )
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    SChar    sErrorMessage[128] = { 0, };
    
    sRC = acpPollRemoveSock( &(aPoll->mPollSet), &(aSocket->mSocket) );
    
    if ( sRC != ACP_RC_SUCCESS )
    {
        idlOS::snprintf( sErrorMessage,
                         ID_SIZEOF( sErrorMessage ),
                         "[rpnPollRemoveSocket] acpPollRemoveSock failure ( return : %"ID_INT32_FMT")",
                         sRC );        
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorMessage ) );
        IDE_ERRLOG( IDE_RP_0 );
        
        IDE_DASSERT( 0 );
    }
    
    aPoll->mCount--;
}

static IDE_RC rpnPollDispatchResume( rpnPoll *  aPoll )
{
    acp_rc_t    sRC = ACP_RC_ECANCELED;

    while ( sRC == ACP_RC_ECANCELED )
    {
        sRC = acpPollDispatchResume( &(aPoll->mPollSet),
                                     dispatcherCallback,
                                     aPoll );

        switch ( sRC )
        {
            case ACP_RC_SUCCESS:
            case ACP_RC_EOF:
                /* exit loop */
                break;

            case ACP_RC_ECANCELED:
                /* keep going loop */
                break;

            default:
                break;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC rpnPollDispatch( rpnPoll         * aPoll,
                        SLong             aTimeoutMilliSec,
                        rpnPollCallback * aCallback )
{
    acp_rc_t    sRC = ACP_RC_SUCCESS;

    aPoll->mCallback = aCallback;

    sRC = acpPollDispatch( &(aPoll->mPollSet),
                           acpTimeFromMsec( aTimeoutMilliSec ),
                           dispatcherCallback,
                           aPoll );

    switch ( sRC )
    {
        case ACP_RC_SUCCESS:
            break;

        case ACP_RC_ECANCELED:
            IDE_TEST( rpnPollDispatchResume( aPoll ) != IDE_SUCCESS );
            break;

        case ACP_RC_ETIMEDOUT:
            IDE_RAISE( ERR_TIMED_OUT );
            break;

        default:
            IDE_RAISE( ERR_DISPATCH );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TIMED_OUT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_POLL_DISPATCH_TIMEOUT ) );
    }
    IDE_EXCEPTION( ERR_DISPATCH )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_POLL_DISPATCH_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt rpnPollGetCount( rpnPoll     * aPoll )
{
    return aPoll->mCount;
}
