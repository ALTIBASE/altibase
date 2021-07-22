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

#ifndef _O_MMD_XA_H_
#define _O_MMD_XA_H_ 1

#include <idl.h>


class mmcSession;
class mmdXid;

typedef struct mmdXaContext
{
    mmcSession *mSession;
    UChar       mOperation;
    SInt        mReturnValue;
    SInt        mRmID;
    vSLong      mFlag;
} mmdXaContext;

class mmdXa
{
private:
    static IDE_RC beginOpen(mmdXaContext *aXaContext);
    static IDE_RC beginClose(mmdXaContext *aXaContext);
    static IDE_RC beginStart(mmdXaContext *aXaContext, mmdXid *aXid,
                             mmdXaAssocState  aXaAssocState,
                             UInt             *aState);
    static IDE_RC beginEnd(mmdXaContext *aXaContext, mmdXid *aXid, mmdXid *aCurrentXid,
                           mmdXaAssocState  aXaAssocState);
    static IDE_RC beginPrepare(mmdXaContext *aXaContext, mmdXid *aXid);
    static IDE_RC beginCommit(mmdXaContext *aXaContext, mmdXid *aXid);
    //fix BUG-22033
    static IDE_RC beginRollback(mmdXaContext *aXaContext, mmdXid *aXid,mmdXaWaitMode  *aWaitMode);
    static IDE_RC beginForget(mmdXaContext *aXaContext, mmdXid *aXid);
    static IDE_RC beginRecover(mmdXaContext *aXaContext);

public:
    static void open(mmdXaContext *aXaContext);
    static void close(mmdXaContext *aXaContext);
    /* BUG-18981 */
    static void start(mmdXaContext *aXaContext, ID_XID *aXid);
    static void end(mmdXaContext *aXaContext, ID_XID *aXid);
    static void heuristicEnd(mmcSession *aSession, ID_XID *aXid);            //BUG-29078
    static void prepare(mmdXaContext *aXaContext, ID_XID *aXid);
    static void commit(mmdXaContext *aXaContext, ID_XID *aXid);
    static void rollback(mmdXaContext *aXaContext, ID_XID *aXid);
    static void forget(mmdXaContext *aXaContext, ID_XID *aXid);
    static void recover(mmdXaContext  *aXaContext,
                        ID_XID       **aPreparedXids,
                        SInt          *aPreparedXidsCnt,
                        ID_XID       **aHeuristicXids,
                        SInt          *aHeuristicXidsCnt);
    static void heuristicCompleted(mmdXaContext *aXaContext, ID_XID *aXid);

    static IDE_RC commitForce( idvSQL   *aStatistics,   /* PROJ-2446 */
                               SChar    *aXIDStr, 
                               UInt      aXIDStrSize );
    static IDE_RC rollbackForce( idvSQL *aStatistics,   /* PROJ-2446 */ 
                                 SChar * aXIDStr, 
                                 UInt    aXIDStrSize );
                                
    /* BUG-25999 */
    static IDE_RC removeHeuristicXid( idvSQL    *aStatistics,   /* PROJ-2446 */
                                      SChar     *aXIDStr, 
                                      UInt       aXIDStrSize );
    //fix BUG-26844 mmdXa::rollback이 장시간 진행될때 어떠한 XA call도 진행할수 없습니다
    //Bug Fix를  mmdManager::checkXATimeOut에도 적용해야 합니다.
    /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
     fix(list-s-latch) , fixWithAdd(list-x-latch)로 분리시켜 fix 병목을 분산시킵니다. */
    static IDE_RC fix(mmdXid **aXidObjPtr, ID_XID *aXIDPtr, mmdXaLogFlag aXaLogFlag);
    static IDE_RC fixWithAdd(mmdXaContext *aXaContext,mmdXid* aXidObj2Add, mmdXid **aFixedXidObjPtr, ID_XID *aXIDPtr, mmdXaLogFlag aXaLogFlag);
    static IDE_RC unFix(mmdXid *aXidObj, ID_XID *aXIDPtr, UInt aOptFlag);
    
    static IDE_RC convertStringToXid(SChar *aStr, UInt aStrLen, ID_XID *aXID);

public:
    static void terminateSession(mmcSession *aSession);

private:
    static IDE_RC cleanupLocalTrans(mmcSession *aSession);
    static void   prepareLocalTrans(mmcSession *aSession);
    static void   restoreLocalTrans(mmcSession *aSession);
};


#endif
