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
 *
 * $Id: sdpscDef.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment 모듈의 공통 자료구조를 정의한
 * 헤더파일이다.
 *
 * Undo Tablespace는 TSS Segment와 Undo Segment들을 저장하고 관리한다.
 * TSS Segment와 Undo Segment는 CMS로 관리되며, Database 복구, 트랜잭션 복구,
 * Query에 대한 Read Consistency를 제공한다.
 *
 * CMS (Circular-List Managed Segment)는 FMS와 TMS와는 다른 페이지 할당
 * 알고리즘, Extent 할당 및 재사용 알고리즘, Extent Dir. Shrink 알고리즘들을
 * 구현하고 있다.
 *
 * CMS에 의해 관리되는 Undo Segment와 TSS Segment는 다음과 같은 특징을 가진다.
 *
 * (1) UDSEG와 TSSEG는 알티베이스 서버에 의해서 자동 생성되며,모두 예약된 이름을
 *     사용한다.
 * (2) Expired된 Extent Dir.는 재사용되거나 Shrink 될 수 있다.
 * (3) 운영중에 Segment가 자동으로 Shrink 된다.
 * (4) Segment의 HWM 개념이 존재하지 않는다.
 * (5) UDSEG와 TSSEG는 각각 Extent Dir.의 Extent 개수를 제한할 수 있는
 *     사용자 프로퍼티를 정의한다
 * (6) Steal 정책으로 세그먼트간의 Expire된 ExtDir를 공유할 수 있다.
 *
 ***********************************************************************/

# ifndef _O_SDPSC_DEF_H_
# define _O_SDPSC_DEF_H_ 1

# include <iduMutex.h>

# include <sdpDef.h>

/***********************************************************************
 * Segment Cache 자료구조 (Runtime 정보)
 *
 ***********************************************************************/
typedef struct sdpscSegCache
{
    sdpSegCCache     mCommon;            // 세그먼트 캐시 공통 헤더
} sdpscSegCache;

/***********************************************************************
 * ExtDir정보 자료구조
 ***********************************************************************/
typedef struct sdpscExtDirInfo
{
    // ExtDir의 PID
    scPageID          mExtDirPID;

    /* Cur. ExtDir의 Full 시 NewExtDir 필요 */
    idBool            mIsFull;

    /* ExtDir에서 새로운 Extent의 개수 */
    SShort            mTotExtCnt;

    /* ExtDir에 기록될 수 있는 최대 ExtDesc 개수 */
    UShort            mMaxExtCnt;

    // ExtDir의 Nxt. ExtDir PID
    scPageID          mNxtExtDirPID;

} sdpscExtDirInfo;

/***********************************************************************
 *  Extent Descriptor 정의
 ***********************************************************************/
typedef struct sdpscExtDesc
{
    scPageID       mExtFstPID;     // Extent 첫번째 페이지의 PID
    UInt           mLength;        // Extent의 페이지 개수
    scPageID       mExtFstDataPID; // Extent의 첫번째 Data 페이지의 PID
} sdpscExtDesc;


/***********************************************************************
 * Extent 할당 정보
 ***********************************************************************/
typedef struct sdpscAllocExtDirInfo
{
    /* 확장이후 새로운 Cur. ExtDir 페이지의 PID */
    scPageID          mNewExtDirPID;

    /* 확장이후 새로운 Nxt. ExtDir 페이지의 PID */
    scPageID          mNxtPIDOfNewExtDir;

    /* Shrink해야할 ExtDir */
    scPageID          mShrinkExtDirPID;

    /* TBS로부터 할당받은 ExtDir를 Cur.ExtDir의 Nxt.에 연결해야하는지 여부 */
    idBool            mIsAllocNewExtDir;

    sdpscExtDesc      mFstExtDesc;       /* 할당된 ExtDesc 정보 */

    sdRID             mFstExtDescRID;    /* 할당된 ExtDesc의 RID */

    UInt              mTotExtCntOfSeg;   /* 세그먼트의 총 ExtDesc 개수 */

    /* Shrink해야할 ExtDir 에 포한된 Extent 개수 */
    UShort            mExtCntInShrinkExtDir;

} sdpscAllocExtDirInfo;


/***********************************************************************
 * Segment Control Header
 *
 * Segment Meta Header 페이지에 위치한다.
 ***********************************************************************/
typedef struct sdpscSegCntlHdr
{
    sdpSegType  mSegType;       // Segment 타입
    sdpSegState mSegState;      // Segment State
} sdpscSegCntlHdr;


/***********************************************************************
 * Extent Control Header 정의
 *
 * Segment Meta Header 페이지에 위치하며, Segment의 Extent 정보를 관리한다.
 ***********************************************************************/
typedef struct sdpscExtCntlHdr
{
    UInt           mTotExtCnt;         // Segment에 할당된 Extent 총 개수
    scPageID       mLstExtDir;         // 마지막 ExtDir 페이지의 PID
    UInt           mTotExtDirCnt;      // Extent Map의 총 개수 (SegHdr 제외한개수)
    UInt           mPageCntInExt;      // Extent의 페이지 개수
} sdpscExtCntlHdr;


/***********************************************************************
 * Extent Directory Page Map 정의
 ***********************************************************************/
typedef struct sdpscExtDirMap
{
    sdpscExtDesc  mExtDesc[1]; // Extent Descriptor
} sdpscExtDirMap;


/***********************************************************************
 * Extent Dir Control Header 정의
 ***********************************************************************/
typedef struct sdpscExtDirCntlHdr
{
    UShort       mExtCnt;        // 페이지내의 Extent 개수
    scPageID     mNxtExtDir;     // 다음 Extent Map 페이지의 PID
    UShort       mMaxExtCnt;     // 최대 Extent개수 저장
    scOffset     mMapOffset;     // 페이지 내의 Extent Map의 Offset
    smSCN        mLstCommitSCN;  // 마지막 사용한 커밋한 트랜잭션의 CommitSCN
                                 // LatestCommitSCN
    smSCN        mFstDskViewSCN; // 사용했던 혹은 사용중인 트랜잭션의
                                 // First Disk ViewSCN
} sdpscExtDirHdr;


/***********************************************************************
 * Segment Header 페이지 정의
 ***********************************************************************/
typedef struct sdpscSegMetaHdr
{
    sdpscSegCntlHdr        mSegCntlHdr;     // Segment Control Header
    sdpscExtCntlHdr        mExtCntlHdr;     // Extent Control Header
    sdpscExtDirCntlHdr     mExtDirCntlHdr;  // ExtDir Control Header
    scPageID               mArrMetaPID[ SDP_MAX_SEG_PID_CNT ]; // Segment Meta PID
} sdpscSegMetaHdr;

/***********************************************************************
 * 유효하지 않은 순번
 ***********************************************************************/
# define SDPSC_INVALID_IDX         (-1)

/* Page BitSet을 1바이트로 정의한다. */
typedef UChar sdpscPageBitSet;

/***********************************************************************
 * 페이지의 Page BitSet의 페이지 종류를 정의한다.
 ***********************************************************************/
# define SDPSC_BITSET_PAGETP_MASK          (0x80)
# define SDPSC_BITSET_PAGETP_META          (0x80)
# define SDPSC_BITSET_PAGETP_DATA          (0x00)

/***********************************************************************
 * 페이지의 Page BitSet의 페이지 가용도를 정의한다.
 ***********************************************************************/
# define SDPSC_BITSET_PAGEFN_MASK   (0x7F)
# define SDPSC_BITSET_PAGEFN_UNF    (0x00)
# define SDPSC_BITSET_PAGEFN_FMT    (0x01)
# define SDPSC_BITSET_PAGEFN_FUL    (0x02)

/*
 * ExtDir 의 상태값 정의
 */
typedef enum sdpscExtDirState
{
    SDPSC_EXTDIR_EXPIRED  = 0, // 재사용가능한 페이지 상태
    SDPSC_EXTDIR_UNEXPIRED,    // 재사용할 수 없는 페이지 상태
    SDPSC_EXTDIR_PREPARED      // prepareNewPage4Append에 의해서 확보된
                               // 아직 사용되지 않은 페이지로 재사용할 수 없는 상태
} sdpscExtDirState;

# endif // _O_SDPSC_DEF_H_
