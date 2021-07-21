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
 * $Id: sdnDef.h 29386 2008-11-18 06:52:08Z upinel9 $
 **********************************************************************/

#ifndef _O_SDN_DEF_H_
# define _O_SDN_DEF_H_ 1

# include <idl.h>
# include <idpULong.h>
# include <idpUInt.h>
# include <smDef.h>
# include <smnDef.h>

#define SDN_CTS_MAX_KEY_CACHE  (3) /* BUG-48064 : CTS의 KEY CACHE 수를 4 -> 3 로 변경 */
#define SDN_CTS_KEY_CACHE_NULL (0)


#define SDN_RUNTIME_PARAMETERS                                           \
    SMN_RUNTIME_PARAMETERS                                               \
                                                                         \
    iduLatch       mLatch; /* FOR SMO Operation */                       \
                                                                         \
    scSpaceID      mTableTSID;                                           \
    scSpaceID      mIndexTSID;                                           \
    sdRID          mMetaRID;                                             \
    smOID          mTableOID;                                            \
    UInt           mIndexID;                                             \
                                                                         \
    /*  PROJ-1671 Bitmap-based Tablespace And Segment Space Management*/ \
    /* Segment Handle : Segment RID 및 Semgnet Cache */                  \
    sdpSegmentDesc mSegmentDesc;                                         \
                                                                         \
    ULong          mSmoNo; /* 시스템 startup시에 1로 세팅(0 아님)  */    \
                           /* 0값은 이전 startup시에 node에 기록된 */    \
                           /* SmoNo를 reset하는데 사용됨           */    \
                                                                         \
    idBool         mIsUnique;                                            \
    idBool         mIsNotNull; /*PK는 NULL을 가질수 없다(BUG-17762).*/   \
    idBool         mLogging;                                             \
    /* BUG-17957 */                                                      \
    /* index run-time header에 creation option(logging, force) 추가*/    \
    idBool              mIsCreatedWithLogging;                           \
    idBool              mIsCreatedWithForce;                             \
                                                                         \
    smLSN               mCompletionLSN;

 /* BUG-25279     Btree for spatial과 Disk Btree의 자료구조 및 로깅 분리 
  * Btree For Spatial과 일반 Disk Btree의 저장구조가 분리된다. 하지만 Disk Index로써
  * 공통적으로 가져야 하는 런타임 헤더 항목이 존재한다. 이를 다음과 같이 정의한다. */
typedef struct sdnRuntimeHeader
{
    SDN_RUNTIME_PARAMETERS
} sdnRuntimeHeader;

/* BUG-48064
   CTS CHANING 기능제거로 관련변수 삭제하고 CTS KEY CACHE를 4->3개로 줄여서,
   CTS 구조체 크기 작아짐. (40->24bytes) */
typedef struct sdnCTS
{
    smSCN       mCommitSCN;     // Commit SCN or Begin SCN
    scPageID    mTSSlotPID;     // TSS page id
    scSlotNum   mTSSlotNum;     // TSS slotnum 
    UChar       mState;         // CTS State
    UChar       mDummy;
    UShort      mRefKey[ SDN_CTS_MAX_KEY_CACHE ];
    UShort      mRefCnt;        /* 이 CTS를 참조하는 key의 갯수 */
} sdnCTS;

typedef struct sdnCTLayerHdr
{
    UChar       mCount;
    UChar       mUsedCount;
    UChar       mDummy[6];
} sdnCTLayerHdr;

typedef struct sdnCTL
{
    UChar       mCount;
    UChar       mUsedCount;
    UChar       mDummy[6];
    sdnCTS      mArrCTS[1];
} sdnCTL;

#define SDN_CTS_NONE          (0)  // 'N': 초기 상태
#define SDN_CTS_UNCOMMITTED   (1)  // 'U': 스탬핑을 하지 않은 상태(커밋 불확실)
#define SDN_CTS_STAMPED       (2)  // 'S': 스탬핑을한 상태(커밋 보장)
#define SDN_CTS_DEAD          (3)  // 'D': 더이상 사용되지 않는 상태

#define SDN_CTS_INFINITE     ((UChar)0x3F) /* 63 */
#define SDN_CTS_IN_KEY       ((UChar)0x3E) /* 62 */

#define SDN_IS_VALID_CTS( aCTS ) ( (aCTS != SDN_CTS_INFINITE) && (aCTS != SDN_CTS_IN_KEY) )

/* BUG-24091
 * [SD-기능추가] vrow column 만들때 원하는 크기만큼만 복사하는 기능 추가 */
#define SDN_FETCH_SIZE_UNLIMITED    ID_UINT_MAX

typedef IDE_RC (*sdnSoftKeyStamping)( sdrMtx        * aMtx,
                                      sdpPhyPageHdr * aPage,
                                      UChar           aCTSlotNum,
                                      UChar         * aContxt);

typedef IDE_RC (*sdnHardKeyStamping)( idvSQL        * aStatistics,
                                      sdrMtx        * aMtx,
                                      sdpPhyPageHdr * aPage,
                                      UChar           aCTSlotNum,
                                      UChar         * aContxt,
                                      idBool        * aSuccess );

typedef struct sdnCallbackFuncs
{
    sdnSoftKeyStamping          mSoftKeyStamping;
    sdnHardKeyStamping          mHardKeyStamping;
} sdnCallbackFuncs;

#endif /* _O_SDN_DEF_H_ */
