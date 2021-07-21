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
 * $Id: sdiLob.h 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

/***********************************************************************
 * PROJ-2728 Server-side Sharding LOB
 **********************************************************************/

#ifndef _O_SDI_LOB_H_
#define _O_SDI_LOB_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smc.h>
#include <smp.h>
#include <smm.h>
#include <sdiStatement.h>
#include <sdiStatementManager.h>
#include <sdlStatement.h>

class sdiLob
{
public:
    static IDE_RC open();
    
    static IDE_RC close( idvSQL*       aStatistics,
                         void          * aTrans,
                         smLobViewEnv  * aLobViewEnv );
    
    static IDE_RC read(idvSQL*       aStatistics,
                       void*         aTrans,
                       smLobViewEnv* aLobViewEnv,
                       UInt          aOffset,
                       UInt          aMount,
                       UChar*        aPiece,
                       UInt*         aReadLength);
    
    static IDE_RC write(idvSQL       * /* aStatistics */,
                        void         * aTrans,
                        smLobViewEnv * aLobViewEnv,
                        smLobLocator   aLobLocator,
                        UInt           aOffset,
                        UInt           aPieceLen,
                        UChar        * aPiece,
                        idBool         /* aIsFromAPI */,
                        UInt           /* aContType */);

    static IDE_RC erase( idvSQL       * aStatistics,
                         void         * aTrans,
                         smLobViewEnv * aLobViewEnv,
                         smLobLocator   aLobLocator,
                         UInt           aOffset,
                         UInt           aPieceLen );

    static IDE_RC trim( idvSQL       * aStatistics,
                        void         * aTrans,
                        smLobViewEnv * aLobViewEnv,
                        smLobLocator   aLobLocator,
                        UInt           aOffset );
    
    static IDE_RC prepare4Write(idvSQL*       aStatistics,
                                void*         aTrans,
                                smLobViewEnv* aLobViewEnv,
                                smLobLocator  aLobLocator,
                                UInt          aOffset,
                                UInt          aNewSize);

    static IDE_RC finishWrite( idvSQL       * aStatistics,
                               void         * aTrans,
                               smLobViewEnv * aLobViewEnv,
                               smLobLocator   aLobLocator );

    static IDE_RC getLobInfo(idvSQL*        aStatistics,
                             void*          aTrans,
                             smLobViewEnv*  aLobViewEnv,
                             UInt*          aLobLen,
                             UInt*          aLobMode,
                             idBool*        aIsNullLob);

    // for replication
    static IDE_RC writeLog4CursorOpen( idvSQL       * aStatistics,
                                       void         * aTrans,
                                       smLobLocator   aLobLocator,
                                       smLobViewEnv * aLobViewEnv );

    static IDE_RC put( sdiConnectInfo   *aConnectInfo,
                       sdlRemoteStmt    *aRemoteStmt,
                       UInt              aLocatorType,
                       smLobLocator      aLobLocator,
                       void             *aValue,
                       SLong             aLength );

private:
    static void enter( idvSQL            * aStatistics,
                       void              * aTrans,
                       smShardLobCursor  * aShardLobCursor,
                       sdiConnectInfo   ** aConnectInfo,
                       sdlRemoteStmt    ** aRemoteStmt );

};

inline void sdiLob::enter( idvSQL            * aStatistics,
                           void              * aTrans,
                           smShardLobCursor  * aShardLobCursor,
                           sdiConnectInfo   ** aConnectInfo,
                           sdlRemoteStmt    ** aRemoteStmt )
{
    sdiClientInfo    *sClientInfo  = NULL;
    sdiConnectInfo   *sConnectInfo = NULL;
    sdiStatement     *sSdStmt      = NULL;
    sdlRemoteStmt    *sRemoteStmt  = NULL;
    qcSession        *sQcSession   = NULL;
    void             *sMmSession   = NULL;
    UInt              sMmSessId;

    /*
     * BUG-48034
     *   aStatistics의 sess 를 이용해야 한다.
     *   shard 공유 트랜잭션에서 여러 mmcSession이 하나의 smxTrans를 공유하는 경우,
     *   aTrans->mStatistics의 sess가 현재 이 함수를 실행하는 mmcSession인지 보장할 수 없다.
     */
    if ( aStatistics != NULL )
    {
        sMmSession = aStatistics->mSess->mSession;
    }
    else
    {
        sMmSession = SMI_GET_SESSION_STATISTICS(aTrans)->mSess->mSession;
    }
    sMmSessId = qci::mSessionCallback.mGetSessionID( sMmSession );

    IDE_DASSERT( sMmSessId == aShardLobCursor->mMmSessId );

    if( sMmSessId != aShardLobCursor->mMmSessId )
    {
        ideLog::logLine( IDE_SD_31,
                         "[DEBUG] Failure. sdiLob::enter : Invalid SessionID : expected[%d], but got[%d].",
                         aShardLobCursor->mMmSessId, sMmSessId);
        IDE_CONT( NORMAL_EXIT );
    }

    sQcSession = qci::mSessionCallback.mGetQcSession( sMmSession );
    sClientInfo = sQcSession->mQPSpecific.mClientInfo;

    IDE_TEST_CONT( sClientInfo == NULL, NORMAL_EXIT );

    sConnectInfo = sdi::findConnect( sClientInfo, aShardLobCursor->mNodeId );

    sSdStmt = (sdiStatement *) qci::mSessionCallback.mFindShardStmt(
                                   sMmSession,
                                   aShardLobCursor->mMmStmtId );

    if ( sSdStmt != NULL )
    {
        sdiStatementManager::findRemoteStatement(
                                   sSdStmt,
                                   aShardLobCursor->mNodeId,
                                   aShardLobCursor->mRemoteStmtId,
                                   &sRemoteStmt );

        if ( ( sRemoteStmt != NULL ) &&
             ( ( sRemoteStmt->mFreeFlag & SDL_STMT_FREE_FLAG_ALLOCATED_MASK )
               != SDL_STMT_FREE_FLAG_ALLOCATED_TRUE ) )
        {
            sRemoteStmt = NULL;
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        sRemoteStmt = NULL;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    *aConnectInfo = sConnectInfo;
    *aRemoteStmt  = sRemoteStmt;
}

#endif
