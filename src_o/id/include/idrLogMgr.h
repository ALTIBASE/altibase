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
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#if !defined(_O_IDR_MGR_H_)
#define _O_IDR_MGR_H_ 1

#include <idv.h>
#include <idTypes.h>
#include <iduShmDef.h>
#include <idrDef.h>

class idrLogMgr
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static idrLSN writeLog( iduShmTxInfo   * aShmTxInfo,
                            idrModuleType    aModuleType,
                            idrModuleLogType aModuleLogType,
                            idShmAddr        aAddr,
                            UShort           aSize,
                            SChar          * aData );

    static idrLSN writeLog( iduShmTxInfo   * aShmTxInfo,
                            idrModuleType    aModuleType,
                            idrModuleLogType aModuleLogType,
                            UInt             aUImgInfoCnt,
                            idrUImgInfo    * aArrUImgInfo );

    static idrLSN writeLog( iduShmTxInfo   * aShmTxInfo,
                            idrModuleType    aModuleType,
                            idrModuleLogType aModuleLogType,
                            UShort           aSize,
                            SChar          * aData );

    static idrLSN writeNullNTALog( iduShmTxInfo   * aShmTxInfo,
                                   idrLSN           aPrvLSN,
                                   idrModuleType    aModuleType,
                                   idrModuleLogType aModuleLogType );

    static  idrLSN writeNTALog( iduShmTxInfo   * aShmTxInfo,
                                idrLSN           aPrvLSN,
                                idrModuleType    aModuleType,
                                idrModuleLogType aModuleLogType,
                                UInt             aUImgInfoCnt,
                                idrUImgInfo    * aArrUImgInfo );

    static  idrLSN writeNTALog( iduShmTxInfo   * aShmTxInfo,
                                idrLSN           aPrvLSN,
                                idrModuleType    aModuleType,
                                idrModuleLogType aModuleLogType,
                                UShort           aSize,
                                SChar          * aData );

    static inline void setLstLSN( iduShmTxInfo   * aShmTxInfo,
                                  idrLSN           aNewLstLSN );

    static inline idrLSN getLstLSN( iduShmTxInfo * aShmTxInfo );

    static  void readLog( iduShmTxInfo   * aShmTxInfo,
                          UInt             aOffset,
                          idrLogHead     * aLogHead,
                          SChar         ** aUndoImg );

    static  IDE_RC undoProcOrThr( idvSQL        * aStatistics,
                                  iduShmTxInfo  * aShmTxInfo,
                                  idrLSN          aLSN );

    static  inline void enableLogging() { mLoggingEnabled = ID_TRUE; }
    static  inline void disableLogging() { mLoggingEnabled = ID_FALSE; }

    static  inline UShort getLogSize( iduShmTxInfo   * aShmTxInfo,
                                      idrLSN           aLSN );

    static void updatePrvLSN4NTALog( iduShmTxInfo   * aShmTxInfo,
                                     idrLSN           aWriteLSN,
                                     idrLSN           aPrvLSN );

    static void registerUndoFunc( idrModuleType  aModuleType,
                                  idrUndoFunc  * aArrUndoFunc );

    static IDE_RC commit( idvSQL       * aStatistics,
                          iduShmTxInfo * aShmTxInfo );

    static IDE_RC abort( idvSQL       * aStatistics,
                         iduShmTxInfo * aShmTxInfo );

    static IDE_RC commit2Svp( idvSQL        * aStatistics,
                              iduShmTxInfo  * aShmTxInfo,
                              idrSVP        * aSavepoint );

    static IDE_RC abort2Svp( idvSQL         * aStatistics,
                             iduShmTxInfo   * aShmTxInfo,
                             idrSVP         * aSavepoint );

    static IDE_RC pushLatchOp( iduShmTxInfo        * aShmTxInfo,
                               void                * aObject,
                               iduShmSXLatchMode     aMode,
                               iduShmLatchInfo    ** aNewLatchInfo );

    static IDE_RC pushSetLatchOp( iduShmTxInfo        * aShmTxInfo,
                                  void                * aObject,
                                  iduShmSXLatchMode     aMode,
                                  iduShmLatchInfo    ** aNewLatchInfo );

    static IDE_RC releaseLatch( idvSQL       * aStatistics,
                                iduShmTxInfo * aShmTxInfo,
                                void         * aLatch );

    static IDE_RC releaseLatch2Svp( idvSQL        * aStatistics,
                                    iduShmTxInfo  * aShmTxInfo,
                                    idrSVP        * aSavepoint );

    static IDE_RC releaseSetLatch( idvSQL             * aStatistics,
                                   iduShmTxInfo       * aShmTxInfo );

    static IDE_RC clearLog2Svp( iduShmTxInfo   * aShmTxInfo,
                                idrSVP         * aSavepoint );

    static inline void setSavepoint( iduShmTxInfo   * aShmTxInfo,
                                     idrSVP         * aSavepoint );

    static inline void crtSavepoint( iduShmTxInfo   * aShmTxInfo,
                                     idrSVP         * aSavepoint,
                                     idrLSN           aUndoLsn );

    static inline void crtSavepoint( idrSVP * aSavepoint,
                                     UInt     aLatchIndex,
                                     idrLSN   aUndoLsn );

    static inline UInt getCurLatchStackIdx( iduShmTxInfo   * aShmTxInfo );

    static void clearLatchStack( iduShmTxInfo   * aShmTxInfo,
                                 UInt             aStackIdx,
                                 idBool           aLatchRelease );

    static void clearSetLatchStack( iduShmTxInfo   * aShmTxInfo,
                                    UInt             aStackIdx );

    static inline idrLSN getLstLSN4Latch( iduShmTxInfo  * aShmTxInfo );
    static inline iduShmLatchInfo* getLstLatchInfo( iduShmTxInfo  * aShmTxInfo );
    static inline UInt   getLatchCntInStack( iduShmTxInfo  * aShmTxInfo );
    static inline UInt   getSetLatchCntInStack( iduShmTxInfo   * aShmTxInfo );

    static idBool isLatchAcquired( iduShmTxInfo      * aShmTxInfo,
                                   void              * aLatch,
                                   iduShmSXLatchMode   aMode );

    static idBool isSetLatchAcquired( iduShmTxInfo      * aShmTxInfo,
                                      void              * aLatch,
                                      iduShmSXLatchMode   aMode );

    static void   dump( iduShmTxInfo * aShmTxInfo );
    static IDE_RC dumpAllTx4Process( UInt aLPID );
    static IDE_RC dump( UInt aLPID, UInt aShmTxID );

    static void disableUndoLog( iduShmTxInfo   * aShmTxInfo,
                                idrLSN           aLSN );

public:
    static  idBool           mLoggingEnabled;
    static  idrUndoFunc    * mArrUndoFuncMap[IDR_MODULE_TYPE_MAX];
    static  SChar            mStrModuleType[IDR_MODULE_TYPE_MAX][100];
};

inline UShort idrLogMgr::getLogSize( iduShmTxInfo   * aShmTxInfo,
                                     idrLSN           aLSN )
{
    idrLogHead    * sLogHead;
    UShort           sLogSize;

    sLogHead = (idrLogHead*)(aShmTxInfo->mLogBuffer + aLSN);

    idlOS::memcpy( &sLogSize, &sLogHead->mSize, ID_SIZEOF(UShort) );

    return sLogSize;
}

inline void idrLogMgr::setLstLSN( iduShmTxInfo   * aShmTxInfo,
                                  idrLSN           aNewLstLSN )
{
    IDE_ASSERT( aNewLstLSN != IDR_NULL_LSN );

    aShmTxInfo->mLogOffset = aNewLstLSN;
}

inline idrLSN idrLogMgr::getLstLSN( iduShmTxInfo   * aShmTxInfo )
{
    idrLSN           sLsn = IDR_NULL_LSN;

    IDE_ASSERT( aShmTxInfo != NULL );

    sLsn = aShmTxInfo->mLogOffset;

    return sLsn;
}

inline void idrLogMgr::setSavepoint( iduShmTxInfo   * aShmTxInfo,
                                     idrSVP         * aSavepoint )
{
    IDE_DASSERT( aShmTxInfo != NULL );
    IDE_DASSERT( aShmTxInfo->mLogOffset != IDR_NULL_LSN );

    aSavepoint->mLSN           = aShmTxInfo->mLogOffset;
    aSavepoint->mLatchStackIdx = getCurLatchStackIdx( aShmTxInfo );
}

inline void idrLogMgr::crtSavepoint( iduShmTxInfo   * aShmTxInfo,
                                     idrSVP         * aSavepoint,
                                     idrLSN           aUndoLsn )
{
    aSavepoint->mLSN           = aUndoLsn;
    aSavepoint->mLatchStackIdx = getCurLatchStackIdx( aShmTxInfo );
}

inline void idrLogMgr::crtSavepoint( idrSVP * aSavepoint,
                                     UInt     aLatchIndex,
                                     idrLSN   aUndoLsn )
{
    aSavepoint->mLSN           = aUndoLsn;
    aSavepoint->mLatchStackIdx = aLatchIndex;
}

inline UInt idrLogMgr::getCurLatchStackIdx( iduShmTxInfo   * aShmTxInfo )
{
    return aShmTxInfo->mLatchStack.mCurSize;
}

inline idrLSN idrLogMgr::getLstLSN4Latch( iduShmTxInfo   * aShmTxInfo )
{
    UInt sLstLatchIdx = aShmTxInfo->mLatchStack.mCurSize - 1;

    return aShmTxInfo->mLatchStack.mArray[sLstLatchIdx].mLSN4Latch;
}

inline iduShmLatchInfo* idrLogMgr::getLstLatchInfo( iduShmTxInfo   * aShmTxInfo )
{
    UInt sLstLatchIdx;

    IDE_ASSERT( aShmTxInfo->mLatchStack.mCurSize != 0 );

    sLstLatchIdx = aShmTxInfo->mLatchStack.mCurSize - 1;

    return &aShmTxInfo->mLatchStack.mArray[sLstLatchIdx];
}

inline UInt idrLogMgr::getLatchCntInStack( iduShmTxInfo   * aShmTxInfo )
{
    return aShmTxInfo->mLatchStack.mCurSize;
}

inline UInt idrLogMgr::getSetLatchCntInStack( iduShmTxInfo   * aShmTxInfo )
{
    return aShmTxInfo->mSetLatchStack.mCurSize;
}

#endif /* _O_IDR_MGR_H_ */
