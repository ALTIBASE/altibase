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

#include <cmAll.h>

typedef struct cmnLinkPeerDUMMY
{
    cmnLinkPeer mLinkPeer;
} cmnLinkPeerDUMMY;

IDE_RC cmnLinkPeerInitializeServerDUMMY( cmnLink * /*aLink*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFinalizeServerDUMMY( cmnLink * /*aLink*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerCloseServerDUMMY( cmnLink * /*aLink*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetHandleDUMMY( cmnLink * /*aLink*/,
                                  void    * /*aHandle*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetDispatchInfoDUMMY( cmnLink * /*aLink*/,
                                        void    * /*aDispatchInfo*/ )
{
   return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetDispatchInfoDUMMY( cmnLink * /*aLink*/,
                                        void    * /*aDispatchInfo*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetBlockingModeDUMMY( cmnLinkPeer * /*aLink*/,
                                        idBool        /*aBlockingMode*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetInfoDUMMY( cmnLinkPeer    * /*aLink*/,
                                SChar          * aBuf,
                                UInt             aBufLen,
                                cmiLinkInfoKey   aKey )
{
    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
        case CMN_LINK_INFO_IMPL:
            idlOS::snprintf(aBuf, aBufLen, "DUMMY");
            break;

        default:
            IDE_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(UnsupportedLinkInfoKey);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetDescDUMMY( cmnLinkPeer * /*aLink*/,
                                void        * /*aDesc*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerConnectDUMMY( cmnLinkPeer       * /*aLink*/,
                                cmnLinkConnectArg * /*aConnectArg*/,
                                PDL_Time_Value    * /*aTimeout*/,
                                SInt                /*aOption*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetOptionsDUMMY( cmnLinkPeer * /*aLink*/,
                                   SInt          /*aOption*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerAllocChannelServerDUMMY( cmnLinkPeer * /*aLink*/,
                                           SInt        * /*aChannelID*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerHandshakeServerDUMMY( cmnLinkPeer * /*aLink*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerRecvServerDUMMY( cmnLinkPeer     * /*aLink*/,
                                   cmbBlock       ** /*aBlock*/,
                                   cmpHeader       * /*aHeader*/,
                                   PDL_Time_Value  * /*aTimeout*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerShutdownServerDUMMY( cmnLinkPeer     * /*aLink*/,
                                       cmnDirection      /*aDirection*/,
                                       cmnShutdownMode   /*aMode*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSendServerDUMMY( cmnLinkPeer * /*aLink*/,
                                   cmbBlock    * /*aBlock*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerReqCompleteDUMMY( cmnLinkPeer * /*aLink*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerResCompleteDUMMY( cmnLinkPeer * /*aLink*/ )
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerCheckServerDUMMY( cmnLinkPeer * /*aLink*/,
                                    idBool      * aIsClosed )
{
    *aIsClosed = ID_FALSE;

    return IDE_SUCCESS;
}

idBool cmnLinkPeerHasPendingRequestDUMMY( cmnLinkPeer * /*aLink*/ )
{
    return ID_FALSE;
}

IDE_RC cmnLinkPeerAllocBlockServerDUMMY( cmnLinkPeer  *aLink,
                                         cmbBlock    **aBlock )
{
    cmbBlock *sBlock = NULL;

    IDE_TEST(aLink->mPool->mOp->mAllocBlock(aLink->mPool, &sBlock) != IDE_SUCCESS);

    sBlock->mDataSize = CMP_HEADER_SIZE;
    sBlock->mCursor   = CMP_HEADER_SIZE;

    *aBlock = sBlock;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerFreeBlockServerDUMMY( cmnLinkPeer *aLink,
                                        cmbBlock    *aBlock )
{
    IDE_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

struct cmnLinkOP gCmnLinkPeerServerOpDUMMY =
{
    "DUMMY-PEER-SERVER",

    cmnLinkPeerInitializeServerDUMMY,
    cmnLinkPeerFinalizeServerDUMMY,

    cmnLinkPeerCloseServerDUMMY,

    cmnLinkPeerGetHandleDUMMY,

    cmnLinkPeerGetDispatchInfoDUMMY,
    cmnLinkPeerSetDispatchInfoDUMMY
};

struct cmnLinkPeerOP gCmnLinkPeerPeerServerOpDUMMY =
{
    cmnLinkPeerSetBlockingModeDUMMY,

    cmnLinkPeerGetInfoDUMMY,
    cmnLinkPeerGetDescDUMMY,

    cmnLinkPeerConnectDUMMY,
    cmnLinkPeerSetOptionsDUMMY,

    cmnLinkPeerAllocChannelServerDUMMY,
    cmnLinkPeerHandshakeServerDUMMY,

    cmnLinkPeerShutdownServerDUMMY,

    cmnLinkPeerRecvServerDUMMY,
    cmnLinkPeerSendServerDUMMY,

    cmnLinkPeerReqCompleteDUMMY,
    cmnLinkPeerResCompleteDUMMY,

    cmnLinkPeerCheckServerDUMMY,
    cmnLinkPeerHasPendingRequestDUMMY,

    cmnLinkPeerAllocBlockServerDUMMY,
    cmnLinkPeerFreeBlockServerDUMMY,

    /* TASK-5894 Permit sysdba via IPC */
    NULL,

    /* PROJ-2474 SSL/TSL */
    NULL,
    NULL,
    NULL,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    NULL,
    NULL,
    NULL,
    NULL
};

IDE_RC cmnLinkPeerServerMapDUMMY( cmnLink *aLink )
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    IDE_ASSERT( aLink->mType == CMN_LINK_TYPE_PEER_SERVER );
    IDE_ASSERT( aLink->mImpl == CMN_LINK_IMPL_DUMMY );

    IDE_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_LOCAL) != IDE_SUCCESS);

    aLink->mOp         = &gCmnLinkPeerServerOpDUMMY;
    sLink->mPeerOp     = &gCmnLinkPeerPeerServerOpDUMMY;
    sLink->mStatistics = NULL;
    sLink->mUserPtr    = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt cmnLinkPeerServerSizeDUMMY()
{
    return ID_SIZEOF(cmnLinkPeerDUMMY);
}
