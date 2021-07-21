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
 * $Id: sdbDef.h 86110 2019-09-02 04:52:04Z et16 $
 *
 * Description :
 *
 * - Dirty Page Buffer 자료구조
 *
 **********************************************************************/

#ifndef _O_SDB_DEF_H_
#define _O_SDB_DEF_H_ 1

#include <smDef.h>
#include <iduLatch.h>
#include <smuList.h>
#include <smuUtility.h>
#include <iduMutex.h>

/* BCB의 초기 Page Type Value */
#define SDB_NULL_PAGE_TYPE    (0)

#define SDB_SMALL_BUFFER_SIZE (1024)

/* --------------------------------------------------------------------
 * PROJ-1665 : Corrupted Page Processing Policy 정의
 *     - FATAL : Page 상태가 corrupted 이면 fatal error 발생
 *     - ABORT : Page 상태가 corrupted 이면 abort error 발생
 *
 * Redo 시에 corrupted page들을 만나면 이를 corrupted page list로 관리한다.
 * Undo가 끝나고 corrupted page list의 각 page 속한 extent의 상태를
 * 검사하여 해당 extent가 free 상태이면 corrupted page list에서 삭제한다.
 * page가 속한 extent가 free 이면 page들도 모두 free 이므로
 * corrupted page 였다 하더라도 상관없기 때문이다.
 *
 * PROJ_1665 전에는 Redo 시에 corrupted page를 만나면 무조건 fatal로
 * 처리되었다. 그러나 Direct-Path INSERT는 commit 전에 모든 page들을
 * flush 하는데 만약 Partial Write되었다면 이는 commit 하지 못했을 것이다.
 * commit 하지 못한 transaction은 Undo 할 것이고 이때 Direct-Path INSERT를
 * 위해 할당받은 extent들은 모두 tablespace로 반환되어버린다.
 * tablespace 반환된 extent들은 모두 free 상태이고 이에 속한 page들도
 * 모두 free 상태가 되므로 corrupted page라고 해서 무조건 fatal 로
 * 처리해선 안된다.
 * ----------------------------------------------------------------- */

typedef enum
{
    SDB_CORRUPTED_PAGE_READ_FATAL = 0,
    SDB_CORRUPTED_PAGE_READ_ABORT,
    SDB_CORRUPTED_PAGE_READ_PASS
} sdbCorruptPageReadPolicy;

// PROJ-1665
typedef struct sdbReadPageInfo
{
    idBool                   aNoAbort;
    sdbCorruptPageReadPolicy aCorruptPagePolicy;
}sdbReadPageInfo;

/* --------------------------------------------------------------------
 * Buffering policy 정의 (BUG-17727)
 * ----------------------------------------------------------------- */

typedef enum
{
    // least recently used
    SDB_BUFFERING_LRU = 0,
    // most recently used
    SDB_BUFFERING_MRU
} sdbBufferingPolicy;


/* --------------------------------------------------------------------
 * Buffer Frame의 I/O 작업의 상태 정의
 * ----------------------------------------------------------------- */

typedef enum
{
    // 수행중 아님
    SDB_IO_NOTHING = 0,

    // 디스크로부터 페이지를 읽는 중
    // latch func vector에서 사용되므로 1이 되어야 한다.
    SDB_IO_READ = 1,

    // 디스크에 페이지를 쓰는 중
    SDB_IO_WRITE

} sdbIOState;

#if 0 //not used.
/* --------------------------------------------------------------------
 * 현재 트랜잭션이 요청 Event
 * ----------------------------------------------------------------- */

typedef enum
{
    SDB_EVENT_LATCH_FREE = 0,
    SDB_EVENT_LATCH_WAIT_S,
    SDB_EVENT_LATCH_WAIT_X,
    SDB_EVENT_LATCH_WAIT_RIO,
    SDB_EVENT_LATCH_WAIT_WIO,
    SDB_EVENT_LATCH_WAITALL_RIO
} sdbWaitEventType;
#endif

#if 0 //not used.
/* --------------------------------------------------------------------
 * Flush 대상의 Type
 * ----------------------------------------------------------------- */

typedef enum
{

    // LRU 리스트 flush
    SDB_FLUSH_LRU = 0,

    // Flush 리스트 flush
    SDB_FLUSH_FLUSH,

    SDB_FLUSH_NOTHING

} sdbFlushType;
#endif

#if 0 //not used.
/* ------------------------------------------------------------------------
 * 청크의 Type : ( UNDO : For Undo Tablespace , NORMAL : Other Tablespaces )
 * ---------------------------------------------------------------------- */

typedef enum
{
    SDB_CHUNK_NORMAL = 0,

    SDB_CHUNK_UNDO

} sdbChunkType;
#endif

#if 0 //not used.
// flush 대상의 type 개수
// flush 대상 type이 추가될 때, 이것도 증가된다.
#define SDB_FLUSH_TYPE_COUNT    (SDB_FLUSH_FLUSH+1)
#endif
/* --------------------------------------------------------------------
 * BCB가 수정된 상태를 나타낸다.
 * 페이지가 수정되었을때, BCB는 수정된 시점의 LSN을 갖게된다.
 * ----------------------------------------------------------------- */
#if 0 //not used.
// BCB가 수정된 적이 없다.
#define SDB_NOT_MODIFIED   (0)
#endif
/* --------------------------------------------------------------------
 * 페이지를 얻기위한 대기 모드
 * ----------------------------------------------------------------- */

typedef enum
{
    // 대기 없이 페이지를 얻는다. 페이지를 얻지 못하면 에러를 return
    SDB_WAIT_NO = 0,

    // 누군가 latch를 잡고 있을 때, 공유 가능하면 얻고,
    // 그렇지 않으면 대기한다.
    SDB_WAIT_NORMAL

} sdbWaitMode;


/* --------------------------------------------------------------------
 * latch 모드
 * ----------------------------------------------------------------- */
typedef enum
{

    SDB_X_LATCH = 0,
    SDB_S_LATCH,
    SDB_NO_LATCH

} sdbLatchMode;

#if 0 //not used. 
/* --------------------------------------------------------------------
 * release 모드
 * mtx에서 savepoint까지 release하는 경우에는
 * 어떠한 페이지도 수정되지 않았음을 보장한다.
 * 이때는 flush list에 등록하지 않는다.
 * ----------------------------------------------------------------- */
typedef enum
{
    // 래치 release만 한다.
    SDB_JUST_RELEASE = 0,
    // 수정되었을 경우 이를 release시에 반영한다.
    SDB_REFLECT_MODIFICATION
} sdbReleaseMode;
#endif
typedef enum
{
    SDB_SINGLE_PAGE_READ = 0,
    SDB_MULTI_PAGE_READ
} sdbPageReadMode;

typedef struct sdbFrameHdr
{
    // 체크섬
    UInt     mCheckSum;

    // 마지막 수정된 LSN
    // flush시 write된다.
    smLSN    mPageLSN;

    // 테이블스페이스 아이디
    UShort   mSpaceID;

    // 더미더미
    SChar    mAlign[2];

    // 인덱스의 smo 진행여부를 확인할 수 있는
    // SMO 번호 . 처음에 0이다.
    ULong    mIndexSMONo;

    //BCB 포인터. 현재 Frame이 속해있는 BCB Pointer
    void*    mBCBPtr;

#ifndef COMPILE_64BIT
    SChar    mAlign[4];
#endif
} sdbFrameHdr;


#if 0 //not used.
/* ------------------------------------------------
 * double write header 정보를 저장하는 페이지.
 * space 1번의 0번 페이지에 space 정보와 함께
 * 저장된다.
 * ----------------------------------------------*/
#define SDB_DOUBLEWRITE_HDR_SPACE    SMI_ID_TABLESPACE_SYSTEM_DATA

#define SDB_DOUBLEWRITE_HDR_PAGE     SD_CREATE_PID(0,0)
#endif

// To implement PRJ-1539 : Buffer Manager Renewal
#if 0 //not used.
// 하나의 청크가 가질수 있는 최소 프레임 개수
// 해당 값보다 항상 커야 한다.
#define MIN_FRAME_COUNT_PER_CHUNK    (128)
#endif

#if 0 //not used.
// 생성될수 있는 언두 버퍼의 최대 크기를 지정한다.
// 전체 Undo 버퍼 공간에서 CREATE_UNDO_BUFFER_LIMIT 이상의
// CreatePage를 방지하기 위해서 사용된다.
#define CREATE_UNDO_BUFFER_LIMIT     (0.95)

// Free된 Undo 버퍼의 최소 크기를 지정한다.
// 전체 Undo 버퍼 공간에서 FREE_UNDO_BUFFER_LIMIT 만큼의
// Free 공간을 보장하기 위해서 사용된다.
#define FREE_UNDO_BUFFER_LIMIT       (0.05)
#endif
typedef enum sdbDPathBCBState
{
    /* 초기에 페이지가 생성되었고, 디스크에 내려가면 안된다.*/
    SDB_DPB_APPEND = 0,
    /* 이 페이지에 대한 모든 Modify연산이 끝났다. 디스크에
     * 내리면된다.*/
    SDB_DPB_DIRTY,
    /* 현재 이 페이지는 Disk에 내릴 대상으로 선택되었다. */
    SDB_DPB_FLUSH
} sdbDPathBCBState;

struct sdbDPathBCB;
struct sdbDPathBuffInfo;
class  sdbDPathBFThread;

typedef struct sdbDPathBCB
{
    /* BCB들끼리의 링크드 리스트를 위해
     * 존재 */
    smuList           mNode;

    /* mPage의 SpaceID, PageID */
    scSpaceID         mSpaceID;
    scPageID          mPageID;
 
    /* BCB의 상태를 표시 */
    sdbDPathBCBState  mState;
    UChar*            mPage;
    sdbDPathBuffInfo* mDPathBuffInfo;

    /* 이 BCB에 대해서 로깅할지 결정. 만약 ID_TRUE면
     * mPage를 디스크에 내리기전에 먼저 Page Image로그를
     * 먼저 기록한다.
     * Logging 여부는 table 속성이므로 DPath Buffer 속성이 아니므로
     * Buffer 정보를 관리하는 sdbDPathBuffInfo에서 하지 않는것이 맞다. */
    idBool            mIsLogging;
} sdbDPathBCB;

typedef struct sdbDPathBuffInfo
{
    /* Direct Path Insert시 만들어진 Page의 갯수 */
    ULong             mTotalPgCnt;

    /* Flush Thread */
    sdbDPathBFThread *mFlushThread;

    /* Append Page List */
    smuList           mAPgList;

    /* Flush Request List */
    smuList           mFReqPgList;

    /* Flush Request List Mutex */
    iduMutex          mMutex;

    scSpaceID         mLstSpaceID;

    // 통계 처리를 위한 값 저장
    ULong             mAllocBuffPageTryCnt;
    ULong             mAllocBuffPageFailCnt;
    ULong             mBulkIOCnt;
} sdbDPathBuffInfo;

typedef struct sdbDPathBulkIOInfo
{
    /* 실제 Buffer의 시작주소 */
    UChar            *mRDIOBuffPtr;
    /* Direct IO를 위해서 iduProperty::getDirectIOPageSize로
       Align된 Buffer이 시작주소 */
    UChar            *mADIOBuffPtr;

    /* Buffer의 크기 */
    UInt              mDBufferSize;

    /* IO를 수행해야 될 Direct Path Buffer의
     * BCB들의 링크드 리스트이다.*/
    smuList           mBaseNode;

    /* 현재 mBaseNode에 연결되어 있는 BCB의 갯수
     * 추가될때마다 1씩 증가하고 Bulk IO수행시 IO가
     * 완료되면 0으로 된다.
     * */
    UInt              mIORequestCnt;

    /* 마지막으로 mBaseNode에 추가된 Page의
     * SpaceID와 PageID */
    scSpaceID         mLstSpaceID;
    scPageID          mLstPageID;
} sdbDPathBulkIOInfo;

/* TASK-4990 changing the method of collecting index statistics
 * MPR의 ParallelScan기능을 Sampling으로 이용하기 위해, 읽을 Extent인지 
 * 아닌지를 판단하는 CallbackFilter를 둔다. */
typedef idBool (*sdbMPRFilterFunc)( ULong   aExtSeq,
                                    void  * aFilterData );

typedef struct sdbMPRFilter4PScan
{
    ULong mThreadCnt;
    ULong mThreadID;
} sdbMPRFilter4PScan;

typedef struct sdbMPRFilter4SamplingAndPScan
{
    /* for Parallel Scan */
    ULong mThreadCnt;
    ULong mThreadID;

    /* for Sampling */
    SFloat mPercentage;
} sdbMPRFilter4SamplingAndPScan;


/****************************************************************************
 * PROJ-2102 Fast Secondaty Buffer  
 ****************************************************************************/
typedef enum
{
    SD_LAYER_BUFFER_POOL,      /* Primary  Buffer에서 호출됨 */
    SD_LAYER_SECONDARY_BUFFER  /* Secondary Buffer 에서 호출됨 */
}sdLayerState;

/*******************************************************************************
 * BCB 공통으로 사용되는 부분을 정의함
 ******************************************************************************/
#define  SD_BCB_PARAMETERS                                                    \
    /* Page가 저장된 Space */                                                  \
    scSpaceID       mSpaceID;                                                  \
                                                                               \
    /* Space안에서 페이지 Number */                                            \
    scPageID        mPageID;                                                   \
                                                                               \
    /* sdbBCBHash를 위한 자료구조 */                                           \
    smuList         mHashItem;                                                 \
                                                                               \
    /* 어느 해시에 속해 있는지 식별자 */                                       \
    UInt            mHashBucketNo;                                             \
                                                                               \
    /* 위의 리스트와는 별개로 SDB_BCB_DIRTY또는 SDB_BCB_REDIRTY인 경우에는     \
     * check point list에도 속하게 된다. checkpoint list를 위한 자료구조  */   \
    smuList         mCPListItem;                                               \
                                                                               \
    /* 다중화된 check point 리스트 중에서 자신이 속한 리스트의 식별자 */       \
    UInt            mCPListNo;                                                 \
                                                                               \
    /* Page가 최초에 Dirty로 등록될때, System의 Last LSN */                    \
    smLSN           mRecoveryLSN;                                              \

typedef struct sdBCB
{
    SD_BCB_PARAMETERS
}sdBCB;


#define SDB_INIT_BCB_LIST( aBCB ) {                                            \
        SMU_LIST_INIT_NODE(&( ((sdbBCB*)aBCB)->mBCBListItem) );                \
        ((sdbBCB*)aBCB)->mBCBListType = SDB_BCB_NO_LIST;                       \
        ((sdbBCB*)aBCB)->mBCBListNo   = SDB_BCB_LIST_NONE;                     \
    }

#define SDB_INIT_CP_LIST( aBCB ) {                                             \
        SMU_LIST_INIT_NODE( &( ((sdBCB*)aBCB)->mCPListItem) );                 \
        ((sdBCB*)aBCB)->mCPListNo    = SDB_CP_LIST_NONE;                       \
    }

#define SDB_INIT_HASH_CHAIN_LIST( aBCB ) {                                     \
        SMU_LIST_INIT_NODE( &((sdBCB*)aBCB)->mHashItem );                      \
    }

#define SD_GET_CPLIST_NO( aBCB )                                               \
    (((sdBCB*)aBCB)->mCPListNo)

#define SD_GET_BCB_RECOVERYLSN( aBCB )                                         \
    (((sdBCB*)aBCB)->mRecoveryLSN)                       

#define SD_GET_BCB_SPACEID( aBCB )                                             \
    (((sdBCB*)aBCB)->mSpaceID)

#define SD_GET_BCB_PAGEID( aBCB )                                              \
    (((sdBCB*)aBCB)->mPageID)

#define SD_GET_BCB_CPLISTNO( aBCB )                                            \
    (((sdBCB*)aBCB)->mCPListNo)

#define SD_GET_BCB_CPLISTITEM( aBCB )                                          \
    (&( ((sdBCB*)aBCB)->mCPListItem ))

#define SD_GET_BCB_HASHITEM( aBCB )                                            \
    (&( ((sdBCB*)aBCB)->mHashItem ))

#define SD_GET_BCB_HASHBUCKETNO( aBCB )                                        \
    (((sdBCB*)aBCB)->mHashBucketNo)



// Flush한 Page의 File을 Sync 하기 위한 정보를 담는다.
typedef struct sdbSyncFileInfo
{
    scSpaceID mSpaceID;
    sdFileID  mFileID;
} sdbSyncFileInfo;

#endif  // _O_SDB_DEF_H_
