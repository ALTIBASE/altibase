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
 * $Id:$
 ***********************************************************************/

# ifndef _O_SDPSF_FT_H_
# define _O_SDPSF_FT_H_ 1

# include <sdpsfDef.h>
# include <sdr.h>
# include <sdpsfDef.h>
# include <sdpsfUFmtPIDList.h>
# include <sdpsfFreePIDList.h>
# include <sdpsfPvtFreePIDList.h>
# include <sdpPhyPage.h>
# include <sdpSglPIDList.h>

//------------------------------------------------------
// D$DISK_TABLE_FMS_SEGHDR
//------------------------------------------------------

typedef struct sdpsfDumpSegHdr
{
    scPageID           mSegHdrPID;     // Segment Hdr PageID 
    SChar              mSegType;       // 실제 SegmentType
    UShort             mState;         // 할당 상태 
    UInt               mPageCntInExt;  // PageCnt In Ext 
    sdpSglPIDListBase  mPvtFreePIDList;// Private page list
    sdpSglPIDListBase  mUFmtPIDList;   // Unformated page list/
    sdpSglPIDListBase  mFreePIDList;   // Insert할 공간이 있는 PageList 
    ULong              mFmtPageCnt;    // Segment에서 한번이라도 할당된 Page의 갯수 
    scPageID           mHWMPID;        // Segment에서 Format된 마지막 페이지를 가리킨다. 
    sdRID              mAllocExtRID;   // Segment에서 현재 Page Alloc을 위해 사용중인 Ext RID 
} sdpsfDumpSegHdr;

//------------------------------------------------------
// D$DISK_TABLE_FMS_FREEPIDLIST
//------------------------------------------------------
typedef struct sdpsfPageInfoOfFreeLst
{
    SChar              mSegType;    // 실제 SegmentType
    scPageID           mPID;        // Free Page ID
    UInt               mRecCnt;     // Page 내 Record Count
    UInt               mFreeSize;   // Page 의 FreeSize
    UInt               mPageType;   // Page Type
    UInt               mState;      // Page 상태
} sdpsfPageInfoOfFreeLst;

//------------------------------------------------------
// D$DISK_TABLE_FMS_PVTPIDLIST
//------------------------------------------------------
typedef struct sdpsfPageInfoOfPvtFreeLst
{
    SChar              mSegType;    // 실제 SegmentType
    scPageID           mPID;        // Private Page ID
} sdpsfPageInfoOfPvtFreeLst;

//------------------------------------------------------
// D$DISK_TABLE_FMS_UFMTPIDLIST
//------------------------------------------------------
typedef struct sdpsfPageInfoOfUFmtFreeLst
{
    SChar              mSegType;    // 실제 SegmentType
    scPageID           mPID;        // Unformatedd Page ID
} sdpsfPageInfoOfUFmtFreeLst;

class sdpsfFT
{
public:
    //------------------------------------------------------
    // D$DISK_TABLE_FMS_SEGHDR
    //------------------------------------------------------

    static IDE_RC buildRecord4SegHdr( idvSQL              * /*aStatistics*/,
                                      void                * aHeader,
                                      void                * aDumpObj,
                                      iduFixedTableMemory * aMemory );

    static IDE_RC dumpSegHdr( scSpaceID             aSpaceID,
                              scPageID              aPageID,
                              sdpSegType            aSegType,
                              void                * aHeader,
                              iduFixedTableMemory * aMemory );

    //------------------------------------------------------
    // D$DISK_TABLE_FMS_FREEPIDLIST
    //------------------------------------------------------
    static IDE_RC buildRecord4FreePIDList( idvSQL              * /*aStatistics*/,
                                           void                * aHeader,
                                           void                * aDumpObj,
                                           iduFixedTableMemory * aMemory );

    static IDE_RC dumpFreePIDList( scSpaceID             aSpaceID,
                                   scPageID              aPageID,
                                   sdpSegType            aSegType,
                                   void                * aHeader,
                                   iduFixedTableMemory * aMemory );

    //------------------------------------------------------
    // D$DISK_TABLE_FMS_PVTPIDLIST
    //------------------------------------------------------
    static IDE_RC buildRecord4PvtPIDList( idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * aDumpObj,
                                          iduFixedTableMemory * aMemory );

    static IDE_RC dumpPvtPIDList( scSpaceID             aSpaceID,
                                  scPageID              aPageID,
                                  sdpSegType            aSegType,
                                  void                * aHeader,
                                  iduFixedTableMemory * aMemory );


    //------------------------------------------------------
    // D$DISK_TABLE_FMS_UFMTPIDLIST
    //------------------------------------------------------
    static IDE_RC buildRecord4UFmtPIDList( idvSQL              * /*aStatistics*/,
                                           void                * aHeader,
                                           void                * aDumpObj,
                                           iduFixedTableMemory * aMemory );

    static IDE_RC dumpUfmtPIDList( scSpaceID             aSpaceID,
                                   scPageID              aPageID,
                                   sdpSegType            aSegType,
                                   void                * aHeader,
                                   iduFixedTableMemory * aMemory );

};

#endif     // _O_SDPSF_FT_H_

