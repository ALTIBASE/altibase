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
 * $Id: sdptbDef.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * Bitmap-based tablespace를 위한 자료구조및 매크로를 정의한다.
 ***********************************************************************/

#ifndef _O_SDPTB_DEF_H_
#define _O_SDPTB_DEF_H_ 1

#include <iduLatch.h>
#include <sdpModule.h>
#include <sdpDef.h>
#include <sdpPhyPage.h>
#include <sdpSglPIDList.h>
#include <smDef.h>

#define SDPTB_BITS_PER_BYTE    (8)     //한바이트에 있는 비트수
#define SDPTB_BITS_PER_UINT    (SDPTB_BITS_PER_BYTE * 4) //UInt 에 있는 비트수  // BUG-47666
#define SDPTB_BITS_PER_ULONG   (SDPTB_BITS_PER_BYTE * 8) //ULong에 있는 비트수


#define SDPTB_BIT_OFF          (0)
#define SDPTB_BIT_ON           (1)

/*
 * 파일의 가용여부를 bit로 관리할때 할당해야 하는 배열의 크기정의
 * TBS에는 최대 1024개의 파일이 있을수있다.
 * 일반적으로 SDPTB_GLOBAL_GROUP_ARRAY_SIZE은 16 이다.
 */
#define SDPTB_GLOBAL_GROUP_ARRAY_SIZE  \
            ( SD_MAX_FID_COUNT/(ID_SIZEOF(ULong)* SDPTB_BITS_PER_BYTE) )

//GG hdr를 위한 페이지 갯수
#define SDPTB_GG_HDR_PAGE_CNT  (1)

/*  LG hdr를 위한 페이지 갯수
 * alloc LG header,dealloc LG header를 위해 각각 1개의 페이지가 필요하므로
 * 총 2개의 페이지가 필요하다.
 */
#define SDPTB_LG_HDR_PAGE_CNT  (2)

//하나의 Extent의 크기를 바이트로 계산
#define SDPTB_EXTENT_SIZE_IN_BYTES( pages_per_extent )  \
                                   ((pages_per_extent)*SD_PAGE_SIZE)


#define SDPTB_LGID_MAX          (SDPTB_BITS_PER_ULONG*2)

//LG에서 비트멥관리시 사용하는 매직넘버
#define SDPTB_LG_BITMAP_MAGIC       (ID_ULONG(0xaf))

//하나의  LG에 있는 페이지 갯수( 이값은 헤더까지 포함한크기이다)
#define SDPTB_PAGES_PER_LG(pages_per_extent)  \
    ( ( sdptbGroup::nBitsPerLG() * pages_per_extent ) + SDPTB_LG_HDR_PAGE_CNT )

//FID로 GG hdr의 PID를 얻어낸다.
#define SDPTB_GLOBAL_GROUP_HEADER_PID(file_id)  SD_CREATE_PID(file_id, 0)

/*
 * LG hdr의 PID를 얻어낸다.
 * which가 0이면 하나의 LG group에 있는 LG hdr중 앞으것의 PID
 * which가 1이면 하나의 LG group에 있는 LG hdr중 뒤의것의 PID
 */
#define SDPTB_LG_HDR_PID_FROM_LGID( file_id , lg_id, which, pages_per_extent )        \
    ( SD_CREATE_PID( file_id, SDPTB_GG_HDR_PAGE_CNT +                                 \
                      lg_id * SDPTB_PAGES_PER_LG( pages_per_extent ) + which ) )

//LGID로부터 해당 LG에서 extent가 시작하는곳의 FPID를 얻어낸다.
#define SDPTB_EXTENT_START_FPID_FROM_LGID( lg_id, pages_per_extent )  \
   (( SDPTB_GG_HDR_PAGE_CNT + lg_id * SDPTB_PAGES_PER_LG(pages_per_extent) )  \
     +  SDPTB_LG_HDR_PAGE_CNT)

/* extent의 첫번째 PID 를 받아서 LGID를 리턴하는 매크로 함수이다.*/
#define SDPTB_GET_LGID_BY_PID( aExtPID, aPagesPerExt)           \
                ((SD_MAKE_FPID( aExtPID ) - SDPTB_GG_HDR_PAGE_CNT )      \
                    /  SDPTB_PAGES_PER_LG( aPagesPerExt ))

// PID로부터 해당 LG에서 extent가 시작하는곳의 PID를 얻어낸다.
#define SDPTB_EXTENT_START_PID_FROM_PID( aPID, aPagesPerExt )            \
    ( SD_CREATE_PID( SD_MAKE_FID( aPID ) ,                                \
                     SDPTB_EXTENT_START_FPID_FROM_LGID (                  \
                           SDPTB_GET_LGID_BY_PID( aPID, aPagesPerExt ),   \
                           aPagesPerExt ) ) )

// PID를 받아서 page가 속한 extent의 index를 리턴하는 매크로 함수이다.
#define SDPTB_EXTENT_IDX_AT_LG_BY_PID( aPID, aPagesPerExt )              \
    ( ( aPID - SDPTB_EXTENT_START_PID_FROM_PID( aPID, aPagesPerExt ) )  \
      / aPagesPerExt )

// Page가 속한 extent PID를 리턴하는 매크로 함수이다.
#define SDPTB_GET_EXTENT_PID_BY_PID( aPID, aPagesPerExt )                  \
    ( ( SDPTB_EXTENT_IDX_AT_LG_BY_PID( aPID, aPagesPerExt ) * aPagesPerExt ) \
      + SDPTB_EXTENT_START_PID_FROM_PID( aPID, aPagesPerExt ) )


typedef  UInt sdptbGGID; // Global Group ID
typedef  UInt sdptbLGID; // Local Group ID

// FEBT 테이블스페이스의 Space Cache Runtime 자료구조
typedef struct sdptbSpaceCache
{
    sdpSpaceCacheCommon mCommon;      // Space Cache Header

    /* Free된 Extent Pool을 가지고 있다. */
    iduStackMgr         mFreeExtPool;

    /* Extent를 할당 탐색연산의 시작 GGID */
    volatile sdptbGGID  mGGIDHint;

    /* MaxGGID는 현재 해당TBS에 유용한 GG에서 최대 ID값을 의미한다.
     * free가 있든없든 그것은 고려대상이 아니다.
     * 즉, GG에 free extent가 없는 GGID라 할지라도 Max GGID가 될수 있다.
     */
    volatile sdptbGGID  mMaxGGID;

    /*
     * 할당가능한 Extent의 존재여부를 bit로 표현하며,
     * DROP된 GlobalGroup의 경우 비트를 0으로 설정된다.
     */
    ULong        mFreenessOfGGs[ SDPTB_GLOBAL_GROUP_ARRAY_SIZE ];

    /* TBS에서 파일확장(Autoextend) 을 위한 Mutex */
    iduMutex     mMutexForExtend;
    /* BUG-31608 [sm-disk-page] add datafile during DML
     * AddDataFile을 하기 위한 Mutex */
    iduMutex     mMutexForAddDataFile;
    idBool       mOnExtend;        // 현재 확장이 진행되고 있는가?
    iduCond      mCondVar;
    UInt         mWaitThr4Extend;  // water들의 수

    // 할당가능한 ExtDir 여부
    idBool     mArrIsFreeExtDir[ SDP_MAX_FREE_EXTDIR_LIST ];
} sdptbSpaceCache;

/*
 * Extent Slot 정의
 *
 * Extent를 TBS에 해제할때 사용되는 임시 구조체이다.
 */
typedef struct sdptbSortExtSlot
{
    scPageID    mExtFstPID;
    UInt        mLength;
    UShort      mLocalGroupID;
} sdptbSortExtSlot;

/*
 * ExtDesc 정보 정의
 */
typedef struct sdptbExtSlot
{
    scPageID   mExtFstPID;
    UShort     mLength;
} sdptbExtSlot;

/* BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 합
 * 니다. 
 * 
 *  Temp Segement의 Extent들을 Mtx Commit 이후에 반환하도록 하기 위해
 * Pending Job으로 매답니다. Pending Job으로 매달기 위한 구조체가
 * sdptbFreeExtID입니다.
 */
typedef struct sdptbFreeExtID
{
    scSpaceID  mSpaceID;
    scPageID   mExtFstPID;
} sdptbFreeExtID;

/* Extent 연산에 따라 접근하는 Local Group Header가 다르다.
 * Extent 할당 연산 : Alloc LG Header 접근
 * Extent 해제 연산 : Dealloc LG Header 접근 */
#define SDPTB_LG_PINGPONG_COUNT    (2)

/*  LG에대한 freeness배열의 크기
 *  한개의 GG에 LG는 128개 이상을 가질수 없으므로 2면 충분하다.  */
#define SDPTB_LG_FN_ARRAY_SIZE     (2)

// LG에대한 freeness배열의 크기를 비트로 나타낸것
#define SDPTB_LG_FN_SIZE_IN_BITS               \
        (SDPTB_LG_FN_ARRAY_SIZE*ID_SIZEOF(ULong)*SDPTB_BITS_PER_BYTE ) //128

#define SDPTB_LG_CNT_MAX  SDPTB_LG_FN_SIZE_IN_BITS

//FID로부터 GGPID얻어내는 메크로
#define SDPTB_GET_GGHDR_PID_BY_FID( fid )        SD_CREATE_PID( fid, 0 )

#define SDPTB_FIRST_FID                         (0)

//GG hdr의 LG type필드에 적을 값
typedef enum sdptbAllocLGIdx
{
    SDPTB_ALLOC_LG_IDX_0 = 0,   // 앞의 LG가 alloc LG group
                                // (처음 LG를 만들었을때의type)
    SDPTB_ALLOC_LG_IDX_1,       // 뒤의 LG가 alloc LG group
    SDPTB_ALLOC_LG_IDX_CNT
}sdptbAllocLGIdx;

//alloc LG인지 dealloc LG인지를 구분하는 용도로 사용
typedef enum sdptbLGType {
    SDPTB_ALLOC_LG,
    SDPTB_DEALLOC_LG,
    SDPTB_LGTYPE_CNT
} sdptbLGType;

/*
 * extent에서 첫번째 PID값과 extent당 page갯수를 갖고
 * extent의 마지막 PID값을 얻는다.
 */
#define SDPTB_LAST_PID_OF_EXTENT(ext_first_pid ,pages_per_extent)   \
                            ( ext_first_pid + pages_per_extent -1)

/*
 * interface에 사용되는 RID type
 */
typedef enum sdptbRIDType
{
    SDPTB_RID_TYPE_TSS,
    SDPTB_RID_TYPE_UDS,
    SDPTB_RID_TYPE_MAX
} sdptbRIDType;

/* Local Group의 Extent 대한 가용 정보를 저장하는 자료구조. */
typedef struct sdptbLGFreeInfo
{
    ULong    mFreeExts;   //모든 Local Group에 걸쳐 할당 가능한 총 Extent 개수
    ULong    mBits[SDPTB_LG_FN_ARRAY_SIZE];
} sdptbLGFreeInfo;

#define SDPTB_RID_ARRAY_SIZE_FOR_UNDOTBS        (SDPTB_RID_TYPE_MAX)

typedef struct sdptbGGHdr
{
    /* A. Global Group 정보 (전체 Group의 익스텐트 할당 및 해제를 관리)
           Local Group 에서는 사용하지 않는다. */
    sdptbGGID           mGGID;        // global group id
    UInt                mPagesPerExt; // extent의 page uniform 개수

    /*
     * mHWM
     * GG의 모든 extent중에서 한번이상 할당된적이 있는 extent들중에서,
     * extent의 첫번째 PID가 가장큰 extent안의 마지막 페이지의 PID값.
     * 다시말하면,GG의 모든 page중에서 한번이상 할당된적이 있는
     * page들중에서, 가장큰 PID값
     * 이값은 증가만하며 감소하지 않.는.다.
     */
    scPageID            mHWM;
    UInt                mLGCnt;//총local group 개수,자동확장시 증가할 수 있다
    UInt                mTotalPages; //실제 데이타파일의 페이지 개수

    /*
     * alloc LG index
     *
     *    SDPTB_ALLOC_LG_IDX_0  -->   alloc LG page가 앞의것
     *    SDPTB_ALLOC_LG_IDX_1  -->   alloc LG page는 뒤의것
     */
    UInt                mAllocLGIdx;
    sdptbLGFreeInfo     mLGFreeness[ SDPTB_LG_PINGPONG_COUNT ];

    // UndoTBS의 TSS/Undo ExtDir PID 리스트
    sdpSglPIDListBase   mArrFreeExtDirList[ SDP_MAX_FREE_EXTDIR_LIST ];

    //Undo 테이블스페이스의 TSS 세그먼트의 Meta PID
    scPageID            mArrTSSegPID[ SDP_MAX_TSSEG_PID_CNT ];
    scPageID            mArrUDSegPID[ SDP_MAX_UDSEG_PID_CNT ];

} sdptbGGHdr;

// Local Group익스텐트 할당 및 해제를 관리한다.
typedef struct sdptbLGHdr
{
     sdptbLGID  mLGID;       // local group id
     UInt       mStartPID;   // 첫번째 Extent 시작 PID (fid, fpid )
     UInt       mHint;       // 다음 할당할 Extent의 비트 index
     UInt       mValidBits;  // group내에서 초기화된 비트갯수
                             // (비트 검색할때 대상이 되는 비트갯수)
     UInt       mFree;       // 할당가능한 free extent 갯수
     ULong      mBitmap[1];  // page 크기에 따라 나머지 공간만큼 사용한다.
} sdptbLGHdr;

/*
 *LG header를 초기화하는 redo routine 에서 사용할정보이다.
 */
typedef struct sdptbData4InitLGHdr
{
    UChar    mBitVal;   //세팅할 비트값
    UInt     mStartIdx; //mBitmap에서 세팅을 시작할 index
    UInt     mCount;    //sBitVal로 초기화할 비트열 갯수
                        //나머지 비트는 !sBitVal로 세트해야함.
} sdptbData4InitLGHdr;

//FID, LGID등을 발견하지 못했을때를 검사하는 magic number한다.
#define SDPTB_NOT_FOUND        (0xFFFF)

//LG에서 비트검색시 발견못했을경우에대한 magic number
#define SDPTB_BIT_NOT_FOUND   (0xFFFFFFFF)


#endif // _O_SDPTB_DEF_H_
