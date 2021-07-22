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
 * $Id: sdpSegDescMgr.h 27220 2008-07-23 14:56:22Z newdaily $
 **********************************************************************/

#ifndef _O_SDP_SEG_DESC_MGR_H_
#define _O_SDP_SEG_DESC_MGR_H_ 1

# include <sdpDef.h>
# include <sdpTableSpace.h>
# include <sctTableSpaceMgr.h>
# include <sdpModule.h>

class sdpSegDescMgr
{
public:

    static IDE_RC initSegDesc( sdpSegmentDesc  * aSegmentDesc,
                               scSpaceID         aSpaceID,
                               scPageID          aSegPID,
                               sdpSegType        aSegType,
                               smOID             aTableOID,
                               UInt              aIndexID );

    static IDE_RC destSegDesc( sdpSegmentDesc * aSegmentDesc );

    static inline sdpSegMgmtOp * getSegMgmtOp( smiColumn * aLobColumn );
    static inline sdpSegHandle * getSegHandle( smiColumn * aLobColumn );
    static inline sdpSegHandle * getSegHandle( sdpPageListEntry * aPageEntry );
    static inline sdpSegHandle * getSegHandle( sdpSegmentDesc * aSegmentDesc );


    static inline sdpSegMgmtOp * getSegMgmtOp( scSpaceID  aSpaceID );
    static inline sdpSegMgmtOp * getSegMgmtOp( sdpPageListEntry * aPageEntry );
    static inline sdpSegMgmtOp * getSegMgmtOp( sdpSegmentDesc * aSegmentDesc );
    static inline sdpSegMgmtOp * getSegMgmtOp( smiSegMgmtType  aSegMgmtType );

    static inline smiSegStorageAttr * getSegStoAttr( sdpSegmentDesc * aSegmentDesc );
    static inline smiSegAttr        * getSegAttr( sdpSegmentDesc * aSegmentDesc );
    static inline smiSegStorageAttr * getSegStoAttr( sdpSegHandle * aSegHandle );

    static inline scPageID getSegPID( sdpSegmentDesc * aSegmentDesc );
    static inline scPageID getSegPID( sdpPageListEntry * aPageEntry );
    static inline scPageID getSegPID( smiColumn * aLobColumn );

    static void setDefaultSegAttr( smiSegAttr  * aSegAttr,
                                   sdpSegType    aSegType );
    static inline void setSegAttr( sdpSegmentDesc    * aSegmentDesc,
                                   smiSegAttr        * aSegAttr );

    static void setDefaultSegStoAttr( smiSegStorageAttr  * aSegStorageAttr );
    static inline void setSegStoAttr( sdpSegmentDesc    * aSegmentDesc,
                                      smiSegStorageAttr * aSegStoAttr );

    static inline sdpSegCCache* getSegCCache( sdpPageListEntry * aPageEntry );

    static IDE_RC dumpSegDesc( sdpSegmentDesc* aSegDesc );
};

inline scPageID sdpSegDescMgr::getSegPID( sdpPageListEntry * aPageEntry )
{
    return aPageEntry->mSegDesc.mSegHandle.mSegPID;
}

inline scPageID sdpSegDescMgr::getSegPID( sdpSegmentDesc * aSegmentDesc )
{
    return aSegmentDesc->mSegHandle.mSegPID;
}

inline scPageID sdpSegDescMgr::getSegPID( smiColumn * aLobColumn )
{
    return ((sdpSegmentDesc*)aLobColumn->descSeg)->mSegHandle.mSegPID;
}

inline sdpSegMgmtOp * sdpSegDescMgr::getSegMgmtOp( smiColumn * aLobColumn )
{
    return ((sdpSegmentDesc*)aLobColumn->descSeg)->mSegMgmtOp;
}

inline sdpSegHandle * sdpSegDescMgr::getSegHandle( smiColumn * aLobColumn )
{
    return &(((sdpSegmentDesc*)aLobColumn->descSeg)->mSegHandle);
}

inline sdpSegHandle * sdpSegDescMgr::getSegHandle( sdpPageListEntry * aPageEntry )
{
    return &(aPageEntry->mSegDesc.mSegHandle);
}

inline sdpSegHandle * sdpSegDescMgr::getSegHandle( sdpSegmentDesc * aSegmentDesc )
{
    return &(aSegmentDesc->mSegHandle);
}

inline smiSegAttr *
sdpSegDescMgr::getSegAttr( sdpSegmentDesc * aSegmentDesc )
{
    return &(aSegmentDesc->mSegHandle.mSegAttr);
}

inline smiSegStorageAttr *
sdpSegDescMgr::getSegStoAttr( sdpSegmentDesc * aSegmentDesc )
{
    return &(aSegmentDesc->mSegHandle.mSegStoAttr);
}

inline smiSegStorageAttr *
sdpSegDescMgr::getSegStoAttr( sdpSegHandle * aSegHandle )
{
    return &(aSegHandle->mSegStoAttr);
}

inline void sdpSegDescMgr::setSegStoAttr( sdpSegmentDesc    * aSegmentDesc,
                                          smiSegStorageAttr * aSegStoAttr )
{
    aSegmentDesc->mSegHandle.mSegStoAttr = * aSegStoAttr;
}

inline void sdpSegDescMgr::setSegAttr( sdpSegmentDesc    * aSegmentDesc,
                                       smiSegAttr        * aSegAttr )
{
    aSegmentDesc->mSegHandle.mSegAttr = * aSegAttr;
}

inline sdpSegMgmtOp * sdpSegDescMgr::getSegMgmtOp( sdpSegmentDesc * aSegmentDesc  )
{
    return (sdpSegMgmtOp*)(aSegmentDesc->mSegMgmtOp);
}

inline sdpSegMgmtOp * sdpSegDescMgr::getSegMgmtOp( sdpPageListEntry * aPageEntry )
{
    return (sdpSegMgmtOp*)(aPageEntry->mSegDesc.mSegMgmtOp);
}

inline sdpSegMgmtOp * sdpSegDescMgr::getSegMgmtOp( scSpaceID  aSpaceID )
{
    IDE_ASSERT( sctTableSpaceMgr::isDiskTableSpace(aSpaceID) == ID_TRUE );
    return getSegMgmtOp(sdpTableSpace::getSegMgmtType( aSpaceID ));
}

inline sdpSegMgmtOp * sdpSegDescMgr::getSegMgmtOp( 
                                        smiSegMgmtType  aSegMgmtType )
{
    sdpSegMgmtOp  * sSegMgmtOp = NULL;

    switch( aSegMgmtType )
    {
        case SMI_SEGMENT_MGMT_FREELIST_TYPE:
            sSegMgmtOp = &gSdpsfOp;
            break;
        case SMI_SEGMENT_MGMT_TREELIST_TYPE:
            sSegMgmtOp = &gSdpstOp;
            break;
        case SMI_SEGMENT_MGMT_CIRCULARLIST_TYPE:
            sSegMgmtOp = &gSdpscOp;
            break;
        default:
            break;
    }
    return sSegMgmtOp;
}

inline sdpSegCCache* sdpSegDescMgr::getSegCCache( sdpPageListEntry * aPageEntry )
{
    sdpSegHandle * sSegHandle = getSegHandle( aPageEntry );
    return (sdpSegCCache*)sSegHandle->mCache;
}

#endif // _O_SDP_SEG_DESC_MGR_H_
