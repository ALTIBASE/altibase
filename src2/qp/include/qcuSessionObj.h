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
 * $Id: qcuSessionObj.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_QCU_SESSIONOBJ_H_
#define _O_QCU_SESSIONOBJ_H_  1

#include <qc.h>

class qcuSessionObj
{
public:
    /* BUG-41307 User Lock 지원 */
    static IDE_RC initializeStatic();
    static void   finalizeStatic();

    static IDE_RC initOpenedFileList( qcSessionObjInfo * aSessionObjInfo );
    static IDE_RC initSendSocket( qcSessionObjInfo * aSessionObjInfo );
    static IDE_RC closeSendSocket( qcSessionObjInfo * aSessionObjInfo );
    static IDE_RC getSendSocket( PDL_SOCKET* aSendSocket,
                                 qcSessionObjInfo * aSessionObjInfo,
                                 SInt aInetFamily);
    static IDE_RC closeAllOpenedFile( qcSessionObjInfo * aSessionObjInfo );
    static IDE_RC addOpenedFile( qcSessionObjInfo * aSessionObjInfo,
                                 FILE             * aFp );
    static IDE_RC delOpenedFile( qcSessionObjInfo * aSessionObjInfo,
                                 FILE             * aFp,
                                 idBool           * aExist );
    
    // BUG-40854
    static IDE_RC initConnection( qcSessionObjInfo * aSessionObjInfo );
    static IDE_RC addConnection( qcSessionObjInfo * aSessionObjInfo,
                                 PDL_SOCKET         aSocket,
                                 SLong            * aConnectionNodeKey );

    static IDE_RC closeAllConnection( qcSessionObjInfo * aSessionObjInfo );
    static IDE_RC delConnection( qcSessionObjInfo * aSessionObjInfo,
                                 SLong              aConnectionNodeKey );
    
    static IDE_RC getConnectionSocket( qcSessionObjInfo * aSessionObjInfo,
                                       SLong              aConnectionNodeKey,
                                       PDL_SOCKET       * aSocket );

    /* PROJ-2657 UTL_SMTP 지원 */
    static void setConnectionState( qcSessionObjInfo * aSessionObjInfo,
                                    SLong              aConnectionNodeKey,
                                    SInt               aState );

    static void getConnectionState( qcSessionObjInfo * aSessionObjInfo,
                                    SLong              aConnectionNodeKey,
                                    SInt             * aState );

    /* BUG-41307 User Lock 지원 */
    static void   initializeUserLockList( qcSessionObjInfo * aSessionObjInfo );
    static void   finalizeUserLockList( qcSessionObjInfo * aSessionObjInfo );

    static IDE_RC requestUserLock( qcSessionObjInfo * aSessionObjInfo,
                                   SInt               aUserLockID,
                                   SInt             * aResult );
    static IDE_RC releaseUserLock( qcSessionObjInfo * aSessionObjInfo,
                                   SInt               aUserLockID,
                                   SInt             * aResult );

private:
    /* BUG-41307 User Lock 지원 */
    static void   findUserLockFromSession( qcSessionObjInfo  * aSessionObjInfo,
                                           SInt                aUserLockID,
                                           qcUserLockNode   ** aUserLockNode );
    static IDE_RC checkUserLockRequestLimit( qcSessionObjInfo * aSessionObjInfo );
    static void   addUserLockToSession( qcSessionObjInfo * aSessionObjInfo,
                                        qcUserLockNode   * aUserLockNode );
    static void   removeUserLockFromSession( qcSessionObjInfo * aSessionObjInfo,
                                             SInt               aUserLockID );

    static IDE_RC findUserLockFromHashTable( SInt              aUserLockID,
                                             qcUserLockNode ** aUserLockNode );
    static IDE_RC addUserLockToHashTable( qcUserLockNode * aUserLockNode,
                                          idBool         * aAlreadyExist );

    static IDE_RC makeAndAddUserLockToHashTable( SInt              aUserLockID,
                                                 qcUserLockNode ** aUserLockNode );

    static IDE_RC waitForUserLock( qcUserLockNode * aUserLockNode,
                                   idBool         * aIsTimeout );
};

#endif // _O_QCU_SESSIONOBJ_H_
