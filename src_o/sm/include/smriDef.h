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
 * $Id$
 *
 * Description :
 *
 * - Incremental chunk change tracking 자료구조
 * - 약어 정리
 *   CT     = change tracking
 *   Ext    = extent
 *   Bmp    = bitmap
 *   Emp    = empty
 *
 *   changeTracking파일의 구조
 *   http://nok.altibase.com/x/L4C0
 *
 **********************************************************************/
#ifndef _O_SMRI_DEF_H_
#define _O_SMRI_DEF_H_ 1

#include <smiDef.h>
#include <smDef.h>
#include <idTypes.h>
#include <idu.h>

#define SMRI_CT_FILE_NAME                       "changeTracking"

#define SMRI_INCREMENTAL_BACKUP_FILE_EXT        ".ibak"

#define SMRI_CT_BIT_CNT                         (8)

/*CTBody에서 EXTMAP block의 index*/
#define SMRI_CT_EXT_MAP_BLOCK_IDX               (0)

/*CT파일을 구성하는 block의 크기*/
#define SMRI_CT_BLOCK_SIZE                      (512)

/*CT파일의 header block의 위치*/
#define SMRI_CT_HDR_OFFSET                      (0)

/*한 CTBody가 가지고 있는 EXT의 수*/
#define SMRI_CT_EXT_CNT                         (320)

/*한 CTBody가 가지고 있는 BMPEXT의 수
 * (MetaEXT와 EmpEXT를 전체 EXT수에서 뺀다)*/
#define SMRI_CT_BMP_EXT_CNT                     (SMRI_CT_EXT_CNT - 2)

/*ExtMap의 크기*/
#define SMRI_CT_EXT_MAP_SIZE                    (SMRI_CT_BMP_EXT_CNT)

/*한 extent를 구성하는 block의 수*/
#define SMRI_CT_EXT_BLOCK_CNT                   (64)

#define SMRI_CT_EXT_META_BLOCK_CNT              (1)

#define SMRI_CT_EXT_BLOCK_CNT_EXCEPT_META_BLOCK  (SMRI_CT_EXT_BLOCK_CNT            \
                                                  - SMRI_CT_EXT_META_BLOCK_CNT)

/*한 BmpBlock이 가지는 bitmap의 크기*/
#define SMRI_CT_BMP_BLOCK_BITMAP_SIZE           (488)

#define SMRI_CT_DATAFILE_DESC_BLOCK_CNT         (SMRI_CT_EXT_BLOCK_CNT_EXCEPT_META_BLOCK)

/*Datafiel Desc block이 가지고있는 slot의 수*/
#define SMRI_CT_DATAFILE_DESC_SLOT_CNT          (3)

#define SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT  (3)

/*Datafile Desc block에 사용중이지 않은 slot이 있는지 확인하기위한 flag*/
#define SMRI_CT_DATAFILE_DESC_SLOT_FLAG_MASK    (0x7)

#define SMRI_CT_DATAFILE_DESC_SLOT_FLAG_FIRST   (0x1)
#define SMRI_CT_DATAFILE_DESC_SLOT_FLAG_SECOND  (0x2)
#define SMRI_CT_DATAFILE_DESC_SLOT_FLAG_THIRD   (0x4)                 


/*Datafile Desc slot에 있는 BmpEXT list에 대한 hint의 수*/
#define SMRI_CT_BMP_EXT_LIST_HINT_CNT           (9)

#define SMRI_CT_EXT_HDR_ID_MASK                 (0xFFFFFFC0)


/*differential backup을 위한 BmpExt list 번호*/
#define SMRI_CT_DIFFERENTIAL_LIST0              (0)

/*differential backup을 위한 BmpExt list 번호*/
#define SMRI_CT_DIFFERENTIAL_LIST1              (1)

/*cumulative backup을 위한 BmpExt list 번호*/
#define SMRI_CT_CUMULATIVE_LIST                 (2)


/*
 * CT파일이 가질수있는 body의 수
 *  - blockID값의 형이 UInt인 관계로 최대 209715개의 body를 가질수 있다.
 */
#define SMRI_CT_MAX_CT_BODY_CNT                 (128)
 
#define SMRI_CT_INVALID_BLOCK_ID                (ID_UINT_MAX)
#define SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX  (ID_UINT_MAX)

#define SMRI_CT_EXT_USE                         ((UChar)(1))
#define SMRI_CT_EXT_FREE                        ((UChar)(0))
 
#define SMRI_CT_FILE_HDR_BLOCK_ID               SMRI_CT_INVALID_BLOCK_ID

/*extent의 크기*/
#define SMRI_CT_EXT_SIZE                        (SMRI_CT_BLOCK_SIZE            \
                                                 * SMRI_CT_EXT_BLOCK_CNT)

#define SMRI_CT_CAN_ALLOC_DATAFILE_DESC_SLOT( aAllocSlotFlag )      \
    ( ((~aAllocSlotFlag) & (SMRI_CT_DATAFILE_DESC_SLOT_FLAG_MASK)) ?  \
      ID_TRUE : ID_FALSE )


/*-------------------------------------------------------------
 * CT block type
 *-----------------------------------------------------------*/
typedef enum smriCTBlockType
{
    SMRI_CT_FILE_HDR_BLOCK,
    SMRI_CT_EXT_MAP_BLOCK,
    SMRI_CT_DATAFILE_DESC_BLOCK,
    SMRI_CT_BMP_EXT_HDR_BLOCK,
    SMRI_CT_BMP_BLOCK,
    SMRI_CT_EMPTY_BLOCK
}smriCTBlockType;
 
/*-------------------------------------------------------------
 * CT DataFileDesc slot에 할당된 데이터파일의 tablespace type
 *-----------------------------------------------------------*/
typedef enum smriCTTBSType
{
    SMRI_CT_NONE,
    SMRI_CT_MEM_TBS,
    SMRI_CT_DISK_TBS
}smriCTTBSType;
 
/*-------------------------------------------------------------
 * slot의 변경된 페이지 추적 상태
 *-----------------------------------------------------------*/
typedef enum smriCTState
{
    SMRI_CT_TRACKING_DEACTIVE,
    SMRI_CT_TRACKING_ACTIVE
}smriCTState;
 
/*-------------------------------------------------------------
 * BMPExt 타입
 *-----------------------------------------------------------*/
typedef enum smriCTBmpExtType
{
    SMRI_CT_DIFFERENTIAL_BMP_EXT_0 = 0,
    SMRI_CT_DIFFERENTIAL_BMP_EXT_1 = 1,
    SMRI_CT_CUMULATIVE_BMP_EXT     = 2,
    SMRI_CT_FREE_BMP_EXT           = 3
}smriCTBmpExtType;
 
 
/*-------------------------------------------------------------
 * change tracking기능 활성화 여부 상태
 *-----------------------------------------------------------*/
typedef enum smriCTMgrState
{
    SMRI_CT_MGR_DISABLED,
    SMRI_CT_MGR_FILE_CREATING,
    SMRI_CT_MGR_ENABLED
}smriCTMgrState;

/*-------------------------------------------------------------
 * CT파일 헤더 상태
 *-----------------------------------------------------------*/
typedef enum smriCTHdrState
{
    SMRI_CT_MGR_STATE_NORMAL,
    SMRI_CT_MGR_STATE_FLUSH,
    SMRI_CT_MGR_STATE_EXTEND
}smriCTHdrState; 

/*-------------------------------------------------------------
 * logAnchor에 저장된 CT파일 정보
 *-----------------------------------------------------------*/
typedef struct smriCTFileAttr
{
    smLSN           mLastFlushLSN;
    SChar           mCTFileName[ SM_MAX_FILE_NAME ];
    smriCTMgrState  mCTMgrState;
}smriCTFileAttr;

 
/*-------------------------------------------------------------
 * Block header의 구조체(24 bytes)
 *-----------------------------------------------------------*/
typedef struct smriCTBlockHdr
{
    /*block checksum*/
    UInt            mCheckSum;

    /*block ID*/
    UInt            mBlockID;

    /*block이 속한 extent header의 block ID*/
    UInt            mExtHdrBlockID;

    /*block의 종류*/
    smriCTBlockType    mBlockType;

    /*block에 대한 mutex*/
    iduMutex        mMutex;

#ifndef COMPILE_64BIT
    UChar           mAlign[4];
#endif
}smriCTBlockHdr;

 
/*-------------------------------------------------------------
 * CT file header block의 구조체(512 bytes)
 *-----------------------------------------------------------*/
typedef struct smriCTFileHdrBlock
{
    /*block header*/
    smriCTBlockHdr      mBlockHdr;

    /*CTBody의 수*/
    UInt                mCTBodyCNT;

    /*incremental backup단위 의 크기(페이지 수)*/
    UInt                mIBChunkSize;

    /*flush관련 조건변수*/
    iduCond             mFlushCV;

    /*extend관련 조건변수*/
    iduCond             mExtendCV;
               
    /*disk로 flush된 가장 최근의 check point LSN*/
    smLSN               mLastFlushLSN;

    /*현재페이지 변경정보를 추적하는 database 이름*/
    SChar               mDBName[SM_MAX_DB_NAME];

#ifndef COMPILE_64BIT
    UChar               mAlign[336];
#else
    UChar               mAlign[328];
#endif
}smriCTFileHdrBlock;
 
/*-------------------------------------------------------------
 * extent map block의 구조체(512 bytes)
 *-----------------------------------------------------------*/
typedef struct smriCTExtMapBlock
{
    //block header
    smriCTBlockHdr  mBlockHdr;

    UInt            mCTBodyID;

    smLSN           mFlushLSN;

    //extent map 공간
    UChar           mExtentMap[SMRI_CT_EXT_MAP_SIZE];

    UChar           mAlign[158];
}smriCTExtMapBlock;
 
/*-------------------------------------------------------------
 * BMPExt List의 구조체(40 bytes)
 *-----------------------------------------------------------*/
typedef struct smriCTBmpExtList
{
    UInt           mSetBitCount;
    /*BMPExt List의 hint, 일정개수의 blockID를 저장하는 배열로
      slot에할당된순서대로배열에BmpExtHdr의 blockID가저장된다*/
    UInt           mBmpExtListHint[SMRI_CT_BMP_EXT_LIST_HINT_CNT];
}smriCTBmpExtList;

/*-------------------------------------------------------------
 * datafile descriptor slot의 구조체(160 bytes)
 *-----------------------------------------------------------*/
typedef struct smriCTDataFileDescSlot
{
    /*datafile descriptor slot ID*/
    smiDataFileDescSlotID       mSlotID;

    /*페이지 변경에 대한 추적 활성화 여부*/
    smriCTState                 mTrackingState;

    /*추적중인 datafile의 종류*/
    smriCTTBSType               mTBSType;

    /*추적중인 datafile의 페이지 크기*/
    UInt                        mPageSize;

    /*slot에 할당된 bitmap extent의 수*/
    UShort                      mBmpExtCnt;

    /*현재 추적중인 bitmap extent list의 ID*/
    UShort                      mCurTrackingListID;

    /*bitmap extent list(differential0, differential1, cumulative)*/
    smriCTBmpExtList            mBmpExtList[SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT];

    /*datafile의 ID*/
    /*BUG-33931 FileID check is being wrong during changetracking*/
    UInt                        mFileID;

    /*table space ID*/
    scSpaceID                   mSpaceID;

    idBool                      mDummy;

    UChar                       mAlign[2];
}smriCTDataFileDescSlot;

/*-------------------------------------------------------------
 * datafile descriptor block의 구조체(512 bytes)
 *-----------------------------------------------------------*/
typedef struct smriCTDataFileDescBlock
{
    /*block header*/
    smriCTBlockHdr          mBlockHdr;

    /*datafile descriptor slot이 사용중인지 확인하기 위한 flag*/
    UInt                    mAllocSlotFlag;
 
    /*datafile descriptor slot공간*/
    smriCTDataFileDescSlot  mSlot[SMRI_CT_DATAFILE_DESC_SLOT_CNT];

    UChar                   mAlign[4];
}smriCTDataFileDescBlock;
 

/*-------------------------------------------------------------
 * Bmp Ext Hdr block의 구조체(512 bytes)
 *-----------------------------------------------------------*/
typedef struct smriCTBmpExtHdrBlock
{
    /*block header*/
    smriCTBlockHdr              mBlockHdr;

    /*prev bitmap extent header block ID*/
    UInt                        mPrevBmpExtHdrBlockID;

    /*next bitmap extent header block ID*/
    UInt                        mNextBmpExtHdrBlockID;

    /*cumulative bitmap extent header block ID*/
    UInt                        mCumBmpExtHdrBlockID;

    /*bitmap extent가 추적중인 bitmap 정보의 타입(differential, cumulative)*/
    smriCTBmpExtType            mType;

    /*bitmap extent가 할당되어있는 datafile descriptor slot의 ID*/
    smiDataFileDescSlotID        mDataFileDescSlotID;

    /*bitmap extent가 datafile descriptor slot에 할당된 순번*/
    UInt                        mBmpExtSeq;

    smLSN                       mFlushLSN;

    /*bitmap extent에 대한 latch*/
    /* BUG-33899 fail to create changeTracking file in window */
    union {
        iduLatch                    mLatch;
        UChar                       mAlign[448];
    };
}smriCTBmpExtHdrBlock;

/*-------------------------------------------------------------
 * BmpBlock의 구조체(512 bytes)
 *-----------------------------------------------------------*/
typedef struct smriCTBmpBlock
{
    /*block header*/
    smriCTBlockHdr    mBlockHdr;

    /*bitmap 공간*/
    SChar             mBitmap[SMRI_CT_BMP_BLOCK_BITMAP_SIZE];
}smriCTBmpBlock;

/*-------------------------------------------------------------
 * meta extent의 구조체
 *-----------------------------------------------------------*/
typedef struct smriCTMetaExt
{
    /*extent map block*/
    smriCTExtMapBlock       mExtMapBlock;

    /*data descriptor blocks*/
    smriCTDataFileDescBlock mDataFileDescBlock[SMRI_CT_EXT_BLOCK_CNT_EXCEPT_META_BLOCK];
}smriCTMetaExt;
 
/*-------------------------------------------------------------
 * Bitmap extent의 구조체
 *-----------------------------------------------------------*/
typedef struct smriCTBmpExt
{
    /*bitmap extent header block*/
    smriCTBmpExtHdrBlock    mBmpExtHdrBlock;

    /*bitmap blocks*/
    smriCTBmpBlock          mBmpBlock[SMRI_CT_EXT_BLOCK_CNT_EXCEPT_META_BLOCK];
}smriCTBmpExt;
 
/*-------------------------------------------------------------
 * Empty extent의 구조체
 *-----------------------------------------------------------*/
typedef struct smriCTEmptyExt
{
    /*empty blocks*/
    smriCTBmpBlock           mEmptyBlock[SMRI_CT_EXT_BLOCK_CNT];
}smriCTEmptyExt;

/*-------------------------------------------------------------
 * CTBody의 구조체
 *-----------------------------------------------------------*/
typedef struct smriCTBody
{
    /* meta 영역 */
    smriCTMetaExt           mMetaExtent;

    /* bitmap 영역 */
    smriCTBmpExt            mBmpExtent[SMRI_CT_BMP_EXT_CNT];

    /*empty 영역*/
    smriCTEmptyExt          mEmptyExtent;

    /*block단위로 접근하기 위한 self 포인터*/
    smriCTBmpBlock        * mBlock;
}smriCTBody;

/*BI파일 내에서의 header offset*/
#define SMRI_BI_HDR_OFFSET                  (0)                            
/*INVALID backup info slot index*/
#define SMRI_BI_INVALID_SLOT_IDX            (ID_UINT_MAX)                  

#define SMRI_BI_INVALID_SLOT_CNT            (SMRI_BI_INVALID_SLOT_IDX)                  

#define SMRI_BI_FILE_HDR_SIZE               (512)

#define SMRI_BI_MAX_BACKUP_FILE_NAME_LEN    (SM_MAX_FILE_NAME)

#define SMRI_BI_FILE_NAME                   "backupinfo"

#define SMRI_BI_INVALID_TAG_NAME            ((SChar*)"INVALID_TAG")

#define SMRI_BI_INVALID_BACKUP_PATH         ((SChar*)"INVALID_BACKUP_PATH")

#define SMRI_BI_TIME_LEN                    (24)

#define SMRI_BI_BACKUP_TAG_PREFIX           "TAG_"

/*-------------------------------------------------------------
 * 백업 대상
 *-----------------------------------------------------------*/
typedef enum smriBIBackupTarget
{
    SMRI_BI_BACKUP_TARGET_NONE,
    SMRI_BI_BACKUP_TARGET_DATABASE,
    SMRI_BI_BACKUP_TARGET_TABLESPACE
}smriBIBackupTarget;
 
typedef enum smriBIMgrState
{
    SMRI_BI_MGR_FILE_REMOVED,
    SMRI_BI_MGR_FILE_CREATED,
    SMRI_BI_MGR_INITIALIZED,
    SMRI_BI_MGR_DESTROYED
}smriBIMgrState;

/*-------------------------------------------------------------
 * logAnchor에 저장된 CT파일 정보
 *-----------------------------------------------------------*/
typedef struct smriBIFileAttr
{
    smLSN           mLastBackupLSN;
    smLSN           mBeforeBackupLSN;
    SChar           mBIFileName[ SM_MAX_FILE_NAME ];
    smriBIMgrState  mBIMgrState;
    SChar           mBackupDir[ SM_MAX_FILE_NAME ];
    UInt            mDeleteArchLogFileNo;
}smriBIFileAttr;

/*-------------------------------------------------------------
 * BI file header (512 bytes)
 *-----------------------------------------------------------*/
typedef struct smriBIFileHdr
{
    /*파일 헤더의 체크섬*/
    UInt            mCheckSum;
    
    /*backupinfo slot의 전체 갯수*/
    UInt            mBISlotCnt;
    
    /*데이터베이스 이름*/
    SChar           mDBName[SM_MAX_DB_NAME];
    
    /*백업이 완료된 LSN*/
    smLSN           mLastBackupLSN;

    UChar           mAlign[368];
} smriBIFileHdr;

/*-------------------------------------------------------------
 * BI slot (676 bytes )
 *-----------------------------------------------------------*/
typedef struct smriBISlot
{
    /*backupinfo slot의 체크섬*/
    UInt                mCheckSum;

    /*백업 시작 시간*/
    UInt                mBeginBackupTime;

    /*백업 완료 시간*/
    UInt                mEndBackupTime;

    /*파일에 저장된 incremental backup chunk의 수*/
    UInt                mIBChunkCNT;

    /*백업 대상(database or table space)*/
    smriBIBackupTarget  mBackupTarget;

    /*백업 수행 level*/
    smiBackupLevel      mBackupLevel;

    /*incremental backup단위 크기*/
    UInt                mIBChunkSize;

    /*백업 타입(full, differential, cumulative)*/
    UShort              mBackupType;

    /*백업된 데이터파일의 table space ID*/
    scSpaceID           mSpaceID;

    union
    {
        /*disk table space의 파일 ID*/
        sdFileID        mFileID;

        /*memory table space의 파일 num*/
        UInt            mFileNo;
    };

    /*백업수행 시점의 원본 파일 크기*/
    ULong               mOriginalFileSize;

    /* chkpt image에 속한 페이지의 갯수
     * control단계에서는 Membase가 로드되지 않은 상태라 Membase에 대한 접근이
     * 불가하다. incremental backup 수행시 Membase의 mDBFilePageCnt값을
     * 저장한다.
     */
    vULong              mDBFilePageCnt;

    /*백업의 태그이름*/
    SChar               mBackupTag[SMI_MAX_BACKUP_TAG_NAME_LEN];

    /*백업 파일의 이름*/
    SChar               mBackupFileName[SMRI_BI_MAX_BACKUP_FILE_NAME_LEN];
}smriBISlot;


#endif /*_O_SMRI_DEF_H_*/

