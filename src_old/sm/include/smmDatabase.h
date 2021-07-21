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
 **********************************************************************/

#ifndef _O_SMM_DATABSE_H_
#define _O_SMM_DATABSE_H_ 1

#include <smmDef.h>


class smmDatabase
{
private :
    smmDatabase(){};
    ~smmDatabase(){};

    // 데이터베이스의 메타정보
    // membase of DICTIONARY TBS
    static smmMemBase         *mDicMemBase;
    static smmMemBase          mMemBaseBackup; // for BUG-7592

    // SCN갱신시 잡는 Mutex
    static iduMutex            mMtxSCN;
    
public :

    static smSCN               mLstSystemSCN;

    static IDE_RC initialize();
    static IDE_RC destroy();
    
     // membase를 초기화한다.
    static IDE_RC initializeMembase( smmTBSNode * aTBSNode,
                                     SChar *      aDBName,
                                     vULong       aDbFilePageCount,
                                     vULong       aChunkPageCount,
                                     SChar *      aDBCharSet,
                                     SChar *      aNationalCharSet );

    /*-----------------------------
       Basic Interfaces for Membase
    -------------------------------*/
    // mDBName
    static inline SChar* getDBName(smmMemBase * aMemBase);
//    static inline void setDBName(smmMemBase * aMemBase,
//                                 SChar*       aDBName,
//                                 UInt         aNameLength);
    
//    // mProductSignature
//    static inline SChar* getProductSignature(smmMemBase * aMemBase);
//    static inline void setProductSignature(smmMemBase * aMemBase,
//                                           SChar*       aSignature);
            
//    // mDBFileSignature
//    static inline SChar* getDBFileSignature(smmMemBase * aMemBase);
//    static inline void setDBFileSignature(smmMemBase * aMemBase,
//                                          SChar*       aSignature);
    
//    // mVersionID
//    static inline UInt getVersionID(smmMemBase * aMemBase);
//    static inline void setVersionID(smmMemBase * aMemBase,
//                                    UInt         aVersionID);

//    // mCompileBit
//    static inline UInt getCompileBit(smmMemBase * aMemBase);
//    static inline void setCompileBit(smmMemBase * aMemBase,
//                                     UInt         aCompileBit);
    
//    // mBigEndian
//    static inline idBool getBigEndian(smmMemBase * aMemBase);
//    static inline void setBigEndian(smmMemBase * aMemBase,
//                                    idBool       aBigEndian);
    
//    // mLogSize
//    static inline vULong getLogSize(smmMemBase * aMemBase);
//    static inline void setLogSize(smmMemBase * aMemBase,
//                                  vULong       aLogSize);
    
    // mDBFilePageCount
    static inline vULong getDBFilePageCount(smmMemBase * aMemBase);
//    static inline void setDBFilePageCount(smmMemBase * aMemBase,
//                                          vULong       aDBFilePageCount);
    
    // mTxTBLSize
//    static inline UInt getTxTBLSize(smmMemBase * aMemBase);
    static inline void setTxTBLSize(smmMemBase * aMemBase,
                                    UInt         aTxTBLSize);

    // mDBFileCount
    static inline UInt getDBFileCount(smmMemBase * aMemBase, UInt aWhichDB);
//    static inline void setDBFileCount(smmMemBase * aMemBase,
//                                      UInt         aWhichDB,
//                                      UInt         aDBFileCount);
        
//    // mTimestamp
//    static inline struct timeval * getTimestamp(smmMemBase * aMemBase);
//    static inline void setTimestamp(smmMemBase * aMemBase,
//                                    timeval *    aTimestamp);
    
    // mAllocPersPageCount
    static inline vULong getAllocPersPageCount(smmMemBase * aMemBase);
/* not  used 
    static inline void setAllocPersPageCount(smmMemBase * aMemBase,
                                             vULong       aAllocPersPageCount);
*/    
    // mExpandChunkPageCnt
    static inline vULong getExpandChunkPageCnt(smmMemBase * aMemBase);
//    static inline void setExpandChunkPageCnt(smmMemBase * aMemBase,
//                                             vULong       aExpandChunkPageCnt);
    
    // mmCurrentExpandChunkCnt
    static inline vULong getCurrentExpandChunkCnt(smmMemBase * aMemBase);
//    static inline void setCurrentExpandChunkCnt(smmMemBase * aMemBase,
//                                                vULong       amCurrentExpandChunkCnt);
    
//    // mFreePageListCount
//    static inline UInt getFreePageListCount(smmMemBase * aMemBase);
//    static inline void setFreePageListCount(smmMemBase * aMemBase,
//                                            UInt         aFreePageListCount);
    
    // mFreePageLists
    static inline smmDBFreePageList getFreePageList(smmMemBase * aMemBase,
                                                    UInt         aFPListIdx);
    static inline void setFreePageList(smmMemBase * aMemBase,
                                       UInt         aFreePageListIdx,
                                       scPageID     aFirstFreePageID,
                                       vULong       aFreePageCount);

    // mSystemSCN
    static inline smSCN* getSystemSCN();
    static inline IDE_RC setSystemSCN( smSCN * aSystemSCN );
    

    /*-----------------------------
       Runtime Interfaces 
    -------------------------------*/
    static inline smSCN getLstSystemSCN();
    static inline smSCN inaccurateGetLstSystemSCN();
    static IDE_RC setLstSystemSCN( smSCN * aLstSystemSCNPtr, smSCN * aNewLstSystemSCNPtr );

    static  void   getViewSCN(smSCN *a_pSCN);
    static  IDE_RC getCommitSCN( void     * aTrans,
                                 idBool     aIsLegacyTrans,
                                 void     * aStatus );

    inline static  IDE_RC lockSCNMtx()
           { return mMtxSCN.lock( NULL ); }
    inline static  IDE_RC unlockSCNMtx()
           { return mMtxSCN.unlock(); }
    
    // System CommitSCN이 Valid한지 조사한다.
    static void validateCommitSCN();
    
    static  smmMemBase* getDicMemBase(){return mDicMemBase;};
    static  smmMemBase* setDicMemBase(smmMemBase * aDicMemBase)
    {
        return mDicMemBase = aDicMemBase;
    };
    static SChar* getDBName() 
    {
        IDE_DASSERT( mDicMemBase != NULL );
        return mDicMemBase->mDBname;
    };

    // PROJ-1579 NCHAR
    static SChar* getDBCharSet();
    static SChar* getNationalCharSet();
    
   
    /*-----------------------------
       Interfaces for Validation 
    -------------------------------*/
    static inline void makeMembaseBackup();
    static IDE_RC checkMembaseIsValid();
    static IDE_RC checkExpandChunkProps(smmMemBase * aMemBase);
    static IDE_RC checkVersion(smmMemBase *aMemBase);
    static void   dumpMembase();

    /*-----------------------------
       BUG-31862 resize transaction table without db migration
       Interfaces for Update mTxTblSize in mDicMemBase
    -------------------------------*/
#ifdef DEBUG
    static IDE_RC checkTransTblSize(smmMemBase * aMemBase);
#endif
    static IDE_RC refineTransTblSize(); 
};



//// mDBName
//inline SChar* smmDatabase::getDBName(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return aMemBase->mDBname;
//}
//inline void smmDatabase::setDBName(smmMemBase * aMemBase,
//                                   SChar*       aDBName,
//                                   UInt         aNameLength)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    IDE_DASSERT( aDBName != NULL );
//    
//    
//    idlOS::strncpy(aMemBase->mDBname, aDBName, aNameLength);
//    aMemBase->mDBname[ aNameLength ] = '\0';
//}
    
//// mProductSignature
//inline SChar* smmDatabase::getProductSignature(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return aMemBase->mProductSignature;
//}
//inline void smmDatabase::setProductSignature(smmMemBase * aMemBase,
//                                             SChar*       aSignature)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    IDE_DASSERT( aSignature != NULL );
//    
//    idlOS::strncpy(aMemBase->mProductSignature,
//                   aSignature,
//                   IDU_SYSTEM_INFO_LENGTH);
//    aMemBase->mProductSignature[ IDU_SYSTEM_INFO_LENGTH - 1 ] = '\0';
//}
            
//// mDBFileSignature
//inline SChar* smmDatabase::getDBFileSignature(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return aMemBase->mDBFileSignature;
//}
//inline void smmDatabase::setDBFileSignature(smmMemBase * aMemBase,
//                                            SChar*       aSignature)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    IDE_DASSERT( aSignature != NULL );
//    
//    idlOS::memcpy(aMemBase->mDBFileSignature,
//                  aSignature,
//                  IDU_SYSTEM_INFO_LENGTH);
//}

//// mVersionID
//inline UInt smmDatabase::getVersionID(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return aMemBase->mVersionID;
//}
//inline void smmDatabase::setVersionID(smmMemBase * aMemBase,
//                                      UInt         aVersionID)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    aMemBase->mVersionID = aVersionID;
//}

//// mCompileBit
//inline UInt smmDatabase::getCompileBit(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return aMemBase->mCompileBit;
//}
//inline void smmDatabase::setCompileBit(smmMemBase * aMemBase,
//                                       UInt         aCompileBit)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    aMemBase->mCompileBit = aCompileBit;
//}
    
//// mBigEndian
//inline idBool smmDatabase::getBigEndian(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return aMemBase->mBigEndian;
//}
//inline void smmDatabase::setBigEndian(smmMemBase * aMemBase,
//                                      idBool       aBigEndian)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    aMemBase->mBigEndian = aBigEndian;
//}
            
//// mLogSize
//inline vULong smmDatabase::getLogSize(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return aMemBase->mLogSize;
//}
//inline void smmDatabase::setLogSize(smmMemBase * aMemBase,
//                                    vULong       aLogSize)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    aMemBase->mLogSize = aLogSize;
//}
                     
// mDBFilePageCount
inline vULong smmDatabase::getDBFilePageCount(smmMemBase * aMemBase)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mDBFilePageCount;
}
//inline void smmDatabase::setDBFilePageCount(smmMemBase * aMemBase,
//                                            vULong       aDBFilePageCount)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    aMemBase->mDBFilePageCount = aDBFilePageCount;
//}

// mTxTBLSize
//inline UInt smmDatabase::getTxTBLSize(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return aMemBase->mTxTBLSize;
//}
inline void smmDatabase::setTxTBLSize(smmMemBase * aMemBase,
                                      UInt         aTxTBLSize)
{
    IDE_DASSERT( aMemBase != NULL );
    
    aMemBase->mTxTBLSize = aTxTBLSize;
}

// mDBFileCount
inline UInt smmDatabase::getDBFileCount(smmMemBase * aMemBase, UInt aWhichDB)
{
    return aMemBase->mDBFileCount[aWhichDB];
}
//inline void smmDatabase::setDBFileCount(smmMemBase * aMemBase,
//                                        UInt         aWhichDB,
//                                        UInt         aDBFileCount)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    aMemBase->mDBFileCount[aWhichDB] = aDBFileCount;
//}
    
//// mTimestamp
//inline struct timeval * smmDatabase::getTimestamp(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return &aMemBase->mTimestamp;
//}
//inline void smmDatabase::setTimestamp(smmMemBase * aMemBase,
//                                      timeval *    aTimestamp)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    IDE_DASSERT( aTimestamp != NULL )
//    
//    idlOS::memcpy(&aMemBase->mTimestamp,
//                  aTimestamp,
//                  ID_SIZEOF(struct timeval));
//}
    
// mAllocPersPageCount
inline vULong smmDatabase::getAllocPersPageCount(smmMemBase * aMemBase)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mAllocPersPageCount;
}
/* not used 
inline void smmDatabase::setAllocPersPageCount(smmMemBase * aMemBase,
                                               vULong       aAllocPersPageCount)
{
    aMemBase->mAllocPersPageCount = aAllocPersPageCount;
}
*/
// mExpandChunkPageCnt
inline vULong smmDatabase::getExpandChunkPageCnt(smmMemBase * aMemBase)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mExpandChunkPageCnt;
}
//inline void smmDatabase::setExpandChunkPageCnt(smmMemBase * aMemBase,
//                                               vULong       aExpandChunkPageCnt)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    aMemBase->mExpandChunkPageCnt = aExpandChunkPageCnt;
//}

// mCurrentExpandChunkCnt
inline vULong smmDatabase::getCurrentExpandChunkCnt(smmMemBase * aMemBase)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mCurrentExpandChunkCnt;
}
//inline void smmDatabase::setCurrentExpandChunkCnt(smmMemBase * aMemBase,
//                                                  vULong       aCurrentExpandChunkCnt)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    aMemBase->mCurrentExpandChunkCnt = aCurrentExpandChunkCnt;
//}

//// mFreePageListCount
//inline UInt smmDatabase::getFreePageListCount(smmMemBase * aMemBase)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    return aMemBase->mFreePageListCount;
//}
//inline void smmDatabase::setFreePageListCount(smmMemBase * aMemBase,
//                                              UInt         aFreePageListCount)
//{
//    IDE_DASSERT( aMemBase != NULL );
//    
//    aMemBase->mFreePageListCount = aFreePageListCount;
//}

// mFreePageLists
inline smmDBFreePageList smmDatabase::getFreePageList(smmMemBase * aMemBase,
                                                      UInt         aFPListIdx)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mFreePageLists[aFPListIdx];
}
inline void smmDatabase::setFreePageList(smmMemBase * aMemBase,
                                         UInt         aFreePageListIdx,
                                         scPageID     aFirstFreePageID,
                                         vULong       aFreePageCount)
{
    IDE_DASSERT( aMemBase != NULL );
    
    aMemBase->mFreePageLists[aFreePageListIdx].mFirstFreePageID = aFirstFreePageID;
    aMemBase->mFreePageLists[aFreePageListIdx].mFreePageCount = aFreePageCount;
}

// mSystemSCN
inline smSCN* smmDatabase::getSystemSCN()
{
    IDE_DASSERT( mDicMemBase != NULL );
    return &(mDicMemBase->mSystemSCN); 
}

/* Service Phase로 상태전이 실시.
 * 이 함수가 불린 다음부터는
 * fillPCHEntry함수에서 PCH 가 NULL이 아니어도 서버를 죽이지 않는다.
 * Service Phase에서는 PCH가 NULL 이 아닌데도 fillPCHEntry가 불릴 수 있기 때문.
 */

inline smSCN smmDatabase::getLstSystemSCN()
{
    return acpAtomicGet64( &mLstSystemSCN );
}

inline smSCN smmDatabase::inaccurateGetLstSystemSCN()
{
    /* PROJ-2733 정확도 보다는 성능 하락 방지용 */
    return mLstSystemSCN;
}

inline void smmDatabase::makeMembaseBackup()
{
    IDE_DASSERT( mDicMemBase != NULL );
    idlOS::memcpy(&mMemBaseBackup, mDicMemBase, ID_SIZEOF(smmMemBase));

}


#endif //_O_SMM_DATABSE_H_
