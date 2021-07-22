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

#ifndef _O_CMN_LINK_CLIENT_H_
#define _O_CMN_LINK_CLIENT_H_ 1


typedef struct cmnLink
{
    cmnLinkType       mType;
    cmnLinkImpl       mImpl;
    acp_uint32_t      mFeature;

    acp_list_node_t   mDispatchListNode;
    acp_list_node_t   mReadyListNode;

    struct cmnLinkOP *mOp;
    // proj_2160 cm_type removal
    cmpPacketType     mPacketType;
} cmnLink;


struct cmnLinkOP
{
    const acp_char_t *mName;

    ACI_RC (*mInitialize)(cmnLink *aLink);
    ACI_RC (*mFinalize)(cmnLink *aLink);

    ACI_RC (*mClose)(cmnLink *aLink);

    ACI_RC (*mGetSocket)(cmnLink *aLink, void **aSock);

    ACI_RC (*mGetDispatchInfo)(cmnLink *aLink, void *aDispatchInfo);
    ACI_RC (*mSetDispatchInfo)(cmnLink *aLink, void *aDispatchInfo);
};


acp_bool_t cmnLinkIsSupportedImpl(cmnLinkImpl aImpl);

ACI_RC cmnLinkAlloc(cmnLink **aLink, cmnLinkType aType, cmnLinkImpl aImpl);
ACI_RC cmnLinkFree(cmnLink *aLink);


#endif
