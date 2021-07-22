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
 * $Id: smiStatistics.h
 * 
 * TASK-4990 changing the method of collecting index statistics
 * 수동 통계정보 수집 기능
 **********************************************************************/

#ifndef __O_SMI_STATISTICS_H__
#define __O_SMI_STATISTICS_H__

#include <smiDef.h>
#include <smiTrans.h>
#include <smp.h>
#include <sdp.h>
#include <svp.h>
#include <smc.h>
#include <smn.h>
#include <smx.h>
#include <sdn.h> 
#include <idtBaseThread.h>

/* TASK-4990 changing the method of collecting index statistics
 * 수동 통계정보 수집 기능
 * =======================================================================
 * 통계정보 Set/Get시 반드시 아래 함수를 사용해야함.
 * AtomicOperation으로의 전환도 시도했지만, 40Byte의 Value를 복제해야하는
 * Min/MaxValue의 존재로 불가능하였고, 기존의 PetersonAlgorithm을 보강하는
 * 형태로 구현되었음.
 *
 * Set : LOCK -> AtomicB++ -> Set -> AtomicA++ -> UNLOCK을
 * Get : while{ A=AtomicA; Get; B=AtomicB; if(A==B) break; }
 *
 * 현재 Index쪽 AtomicStructure와, 그외 GlobalAtomicStructure로 나뉘어서
 * 구현되어 있습니다. 일단 향후 목적은 Global로 모두 통합하는 것입니다.
 * 그러면 통계정보 설정시 하나의 Mutex만 사용하게 되지만, 어차피 수동 통계갱신
 * 방식이라면 통계정보 갱신이 자주 없으니 상관없습니다.
 *
 * 그러나 아직 기존의 누적 통계갱신 방식이 남아있기 때문에, 누적 통계갱신시
 * 변경하는 Index를 위해 Index별로 AtomicStructure를 나누어두었습니다.
 *
 * 참고로 GlobalAtomicOperation에서 SMI_GETSTAT만 있고, SMI_SETSTAT은 없는데
 * 이는 lock/unlock 함수에서 atomicA,B 변경 로직이 포함되어 있기 때문에
 * Lock/Unlock만 잘 호출되면 SMI_SETSTAT이란 매크로를 정의할 필요가
 * 없어서 그렇습니다. */

#define SMI_GETSTAT( aExeStmt )                    \
        while(1)                                   \
        {                                          \
            UInt             sAtomicA = 0;         \
            UInt             sAtomicB = 0;         \
            ID_SERIAL_BEGIN( sAtomicA = mAtomicA );\
            ID_SERIAL_EXEC( aExeStmt, 1 );         \
            ID_SERIAL_END(sAtomicB = mAtomicB);    \
                                                   \
            if(sAtomicA == sAtomicB)               \
            {                                      \
                break;                             \
            }                                      \
                                                   \
            idlOS::thr_yield();                    \
        }

#define SMI_INDEX_GETSTAT( aIdxHdr, aExeStmt )                       \
        while(1)                                                     \
        {                                                            \
            UInt             sAtomicA = 0;                           \
            UInt             sAtomicB = 0;                           \
            ID_SERIAL_BEGIN( sAtomicA = aIdxHdr->mHeader->mAtomicA );\
            ID_SERIAL_EXEC( aExeStmt, 1 );                           \
            ID_SERIAL_END(sAtomicB = aIdxHdr->mHeader->mAtomicB);    \
                                                                     \
            if(sAtomicA == sAtomicB)                                 \
            {                                                        \
                break;                                               \
            }                                                        \
                                                                     \
            idlOS::thr_yield();                                      \
        }

#define SMI_INDEX_SETSTAT( aIdxHdr, aExeStmt )                       \
    IDE_ASSERT( aIdxHdr->mStatMutex.lock( NULL ) == IDE_SUCCESS );   \
    IDE_ASSERT( aIdxHdr->mAtomicB == aIdxHdr->mAtomicA );            \
    ID_SERIAL_BEGIN( aIdxHdr->mAtomicB++; );                         \
    ID_SERIAL_EXEC( aExeStmt, 1 );                                   \
    ID_SERIAL_END( aIdxHdr->mAtomicA++; );                           \
    IDE_DASSERT( aIdxHdr->mAtomicA == aIdxHdr->mAtomicB );           \
    IDE_ASSERT( aIdxHdr->mStatMutex.unlock() == IDE_SUCCESS );

class smiStatistics : public idtBaseThread
{
public:
    /************************************************************
     * Automatic statistic gathering
     * Background Thread가 통계정보를 자동으로 수집한다.
     ************************************************************/
    smiStatistics() : idtBaseThread() {}
    static IDE_RC initializeStatic();
    static IDE_RC finalizeStatic();
    virtual void run(); /* 상속받은 main 실행 루틴 */

    static smiStatistics mStatThread;
    static idBool        mDone;

    /************************************************************
     * Gather
     * 통계정보를 조회하여 수집하고 저장한다
     ************************************************************/
    static IDE_RC gatherSystemStats( idBool   aSaveMemBase );
    static IDE_RC gatherTableStats( idvSQL * aStatistics,
                                    void   * aSmiTrans,
                                    void   * aTable, 
                                    SFloat   aPercentage, 
                                    SInt     aDegree,
                                    void   * aTotalTableArg,
                                    SChar    aNoInvalidate );
    static IDE_RC gatherIndexStats( idvSQL * aStatistics,
                                    void   * aSmiTrans,
                                    void   * aIndex,
                                    SFloat   aPercentage,
                                    SInt     aDegree,
                                    SChar    aNoInvalidate );
    
    /************************************************************
     * Get
     * 저장된 통계정보를 가져온다.
     ************************************************************/
    /*-------------------------System--------------------------*/
    static idBool isValidSystemStat()
    {
        return ( mSystemStat.mCreateTV > 0 ) ? ID_TRUE : ID_FALSE;
    }
    static IDE_RC getSystemStatSReadTime( idBool aCurrent, SDouble * aRet );
    static IDE_RC getSystemStatMReadTime( idBool aCurrent, SDouble * aRet );
    static IDE_RC getSystemStatDBFileMultiPageReadCount( idBool   aCurrent, 
                                                         SLong  * aRet );
    static IDE_RC getSystemStatHashTime( SDouble * aRet );
    static IDE_RC getSystemStatCompareTime( SDouble * aRet );
    static IDE_RC getSystemStatStoreTime( SDouble * aRet );
    /*-------------------------Table--------------------------*/
    static idBool isValidTableStat(void * aTable)
    {
        return (((smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable))->mStat.mCreateTV > 0 )
            ? ID_TRUE : ID_FALSE;
    }
    static idBool isValidTableStat(smcTableHeader * aTable)
    {
        return (aTable->mStat.mCreateTV > 0 )
            ? ID_TRUE : ID_FALSE;
    }
    static IDE_RC getTableStatNumRow( void     * aTable,
                                      idBool     aCurrent, 
                                      smiTrans * aSmiTrans,
                                      SLong    * aRet );
    static IDE_RC getTableStatNumPage( void   * aTable, 
                                       idBool   aCurrent, 
                                       SLong  * aRet );

    static IDE_RC getTableStatAverageRowLength( void * aTable, SLong * aRet );
    static IDE_RC getTableStatOneRowReadTime( void * aTable, SDouble * aRet );

    static void copyTableStats( const void * aDstTable, const void * aSrcTable, UInt * aSkipColumn, UInt aSkipCnt );
    static void copyIndexStats( const void * aDstIndex, const void * aSrcIndex );
    
    static IDE_RC removeColumnMinMax( const void * aTable, UInt aColumnID );

    static IDE_RC getTableAllStat( idvSQL        * aStatistics,
                                   smiTrans      * aTrans,
                                   void          * aTable,
                                   smiAllStat    * aAllStats,
                                   SFloat          aPercentage,
                                   idBool          aDynamicMode );

    /*-------------------------Index--------------------------*/
    inline static idBool isValidIndexStat(void * aIndex)
    {
        idBool           sRet = ID_FALSE;

        if( smnManager::getIsConsistentOfIndexHeader( aIndex )
            == ID_FALSE )
        {
            sRet = ID_FALSE;
        }
        else
        {
            if( smuProperty::getDBMSStatMethod() == SMU_DBMS_STAT_METHOD_AUTO )
            {
                /* Auto상태면, 통계정보 구축 시간이 당연히 0일테니
                 * Index만 Consistent하다면 무조건 유효함 */
                sRet = ID_TRUE;
            }
            else
            {
                sRet =  ( ((smnIndexHeader*)aIndex)->mStat.mCreateTV > 0 ) 
                    ? ID_TRUE : ID_FALSE; 
            }
        }

        return sRet;
    }
    static IDE_RC getIndexStatKeyCount( void   * aIndex, SLong  * aRet );
    static IDE_RC getIndexStatNumPage( void          * aIndex,
                                       idBool          aCurrent,
                                       SLong         * aRet );
    static IDE_RC getIndexStatNumDist( void      * aIndex,
                                       SLong     * aRet );

    static IDE_RC getIndexStatClusteringFactor( void * aIndex, SLong * aRet );
    static IDE_RC getIndexStatAvgSlotCnt( void * aIndex, SLong * aRet );
    static IDE_RC getIndexStatIndexHeight( void * aIndex, SLong * aRet );
    static IDE_RC getIndexStatMin( void * aIndex, void * aRet );
    static IDE_RC getIndexStatMax( void * aIndex, void * aRet );
    
    /*-------------------------Column--------------------------*/
    inline static idBool isValidColumnStat( void  * aTable, 
                                            UInt    aColumnID )
    {
        smiColumn * sColumn =
                        (smiColumn*)smcTable::getColumn( SMI_MISC_TABLE_HEADER(aTable), aColumnID );

        return ( sColumn->mStat.mCreateTV > 0 ) ? ID_TRUE : ID_FALSE; 
    }
    inline static idBool isValidColumnStat( smcTableHeader  * aTable,
                                            UInt              aColumnID )
    {
        smiColumn * sColumn =
                        (smiColumn*)smcTable::getColumn( aTable, aColumnID );

        return ( sColumn->mStat.mCreateTV > 0 ) ? ID_TRUE : ID_FALSE; 
    }

    static IDE_RC getColumnStatNumDist( void         * aTable,
                                        UInt           aColumnID,
                                        SLong        * aRet );
    static IDE_RC getColumnStatNumNull( void  * aTable,
                                        UInt    aColumnID, 
                                        SLong * aRet );
    static IDE_RC getColumnStatAverageColumnLength( void         * aTable,
                                                    UInt           aColumnID,
                                                    SLong        * aRet );
    static IDE_RC getColumnStatMin( void         * aTable,
                                    UInt           aColumnID,
                                    void         * aRet );
    static IDE_RC getColumnStatMax( void         * aTable,
                                    UInt           aColumnID,
                                    void         * aRet );

    /************************************************************
     * set___StatsByuser
     * 사용자로부터 받은 통계정보파라메터를 강제로 설정한다.
     ************************************************************/

    static IDE_RC setSystemStatsByUser( SDouble * aSReadTime,
                                        SDouble * aMReadTime,
                                        SLong   * aMReadPageCount,
                                        SDouble * aHashTime,
                                        SDouble * aCompareTime,
                                        SDouble * aStoreTime );

    static IDE_RC setTableStatsByUser( void    * aSmiTrans,
                                       void    * aTable, 
                                       SLong   * aNumRow,
                                       SLong   * aNumPage,
                                       SLong   * aAverageRowLength,
                                       SDouble * aOneRowReadTime,
                                       SChar     aNoInvalidate );

    static IDE_RC setIndexStatsByUser( void   * aSmiTrans,
                                       void   * aIndex, 
                                       SLong  * aKeyCount,
                                       SLong  * aNumPage,
                                       SLong  * aNumDist,
                                       SLong  * aAvgSlotCnt,
                                       SLong  * aClusteringFactor,
                                       SLong  * aIndexHeight,
                                       SChar    aNoInvalidate );

    static IDE_RC setColumnStatsByUser( void   * aSmiTrans,
                                        void   * aTable, 
                                        UInt     aColumnID, 
                                        SLong  * aNumDist,
                                        SLong  * aNumNull,
                                        SLong  * aAverageColumnLength,
                                        UChar  * aMinValue,
                                        UChar  * aMaxValue,
                                        SChar    aNoInvalidate );

    /************************************************************
     * BUG-38238 add statistical information initialize interface 
     * clear___Stats
     * 통계정보파라메터를 초기화한다.
     ************************************************************/

    static IDE_RC clearSystemStats( void );

    static IDE_RC clearTableStats( void    * aSmiTrans,
                                   void    * aTable, 
                                   SChar     aNoInvalidate );

    static IDE_RC clearIndexStats( void   * aSmiTrans,
                                   void   * aIndex, 
                                   SChar    aNoInvalidate );

    static void clearIndexStatInternal( smiIndexStat * aStat );

    static IDE_RC clearColumnStats( void   * aSmiTrans,
                                    void   * aTable, 
                                    UInt     aColumnID, 
                                    SChar    aNoInvalidate );

    /************************************************************
     * Store
     * Logging 하고 SetDirty하여 Durable하도록 저장한다.
     * 다만 'BeforeImage'는 저장하지 않는다. 어차피 통계이기 때문에
     * 실패하면 재수집하면 그만이기 때문이다.
     ************************************************************/
    static IDE_RC storeTableStat( void           * aSmiTrans,
                                  smcTableHeader * aTable );
    static IDE_RC storeColumnStat( void           * aSmiTrans,
                                   smcTableHeader * aTable,
                                   UInt             aColumnID );
    static IDE_RC storeIndexStat( void           * aSmiTrans,
                                  smcTableHeader * aHeader,
                                  smnIndexHeader * aIndex );

    static IDE_RC beginTotalTableStat( void    * aTotalTableHandle,
                                       SFloat    aPercentage,
                                       void   ** aTotalTableArg );

    static IDE_RC setTotalTableStat( void    * aHeader,
                                     void    * aTotalTableArg );

    static IDE_RC endTotalTableStat( void    * aTotalTableHandle,
                                     void    * aTotalTableArg,
                                     SChar     aNoInvalidate );

    /************************************************************
     * Analysis Functions
     * Table/Index 등을 조회/분석하는 통계정보 수집 연산을
     * 보조한다.
     ************************************************************/
    /*--------------------- Table & Column ---------------------*/
    /* 어차피 Column이건 Table의 Row건 건건마다 분석해야 하기 때문에
     * 동시에 분석한다. 그리고 DRDB건 MRDB건 Fetch한 Row형태는
     * 같기 때문에, 여기서 전부 분석할 수 있다. */
    static IDE_RC beginTableStat( smcTableHeader  * aHeader,
                                  SFloat            aPercentage,
                                  idBool            aDynamicMode,
                                  void           ** aTableArgument );

    static IDE_RC analyzeRow4Stat( smcTableHeader * aHeader,
                                   void           * aTableArgument,
                                   void           * aTotalTableArg,
                                   UChar          * aRow );

    static IDE_RC updateOneRowReadTime( void  * aTableArgument,
                                        SLong   aReadRowTime,
                                        SLong   aReadRowCnt );

    static IDE_RC updateSpaceUsage( void  * aTableArgument,
                                    SLong   aMetaSpace,
                                    SLong   aUsedSpace,
                                    SLong   aAgableSpace,
                                    SLong   aFreeSpace );

    static IDE_RC setTableStat( smcTableHeader * aHeader,
                                void           * aSmxTrans,
                                void           * aTableArgument,
                                smiAllStat     * aAllStats,
                                idBool           aDynamicMode );

    static IDE_RC endTableStat( smcTableHeader * aHeader,
                                void           * aTableArgument,
                                idBool           aDynamicMode );

    /*------------------------- Index -------------------------*/
    /* Index는 기존 통계정보 수집 방법 (Accumulated)을 유지
     * 해야 하기 때문에, 기존 방법에서 사용하는 통계정보 일부수정
     * 함수들 (incIndexNumDist등)도 지원한다.*/
    static IDE_RC beginIndexStat( smcTableHeader   * aTableHeader,
                                  smnIndexHeader   * aPerHeader,
                                  idBool             aDynamicMode );

    static IDE_RC setIndexStatWithoutMinMax( smnIndexHeader * aIndex,
                                             void           * aSmxTrans,
                                             smiIndexStat   * aStat,
                                             smiIndexStat   * aIndexStat,
                                             idBool           aDynamicMode,
                                             UInt             aStatFlag );

    static IDE_RC setIndexMinValue( smnIndexHeader * aIndex,
                                    void           * aSmxTrans,
                                    UChar          * aMinValue,
                                    UInt             aStatFlag );

    static IDE_RC setIndexMaxValue( smnIndexHeader * aIndex,
                                    void           * aSmxTrans,
                                    UChar          * aMaxValue,
                                    UInt             aStatFlag );

    static IDE_RC incIndexNumDist( smnIndexHeader * aIndex,
                                   void           * aSmxTrans,
                                   SLong            aNumDist );

    static IDE_RC endIndexStat( smcTableHeader   * aTableHeader,
                                smnIndexHeader   * aPerHeader,
                                idBool             aDynamicMode );

    /************************************************************
     * Build fixed table
     * FixedTable X$DBMS_STATS을 만드는데 사용된다.
     ************************************************************/
    static IDE_RC buildDBMSStatRecord( idvSQL              * /*aStatistics*/,
                                       void                * aHeader,
                                       void                * aDumpObj,
                                       iduFixedTableMemory * aMemory );
    static IDE_RC buildSystemRecord( void                * aHeader,
                                     iduFixedTableMemory * aMemory );
    static IDE_RC buildTableRecord( smcTableHeader      * aTableHeader,
                                    void                * aHeader,
                                    iduFixedTableMemory * aMemory );
    static IDE_RC buildIndexRecord( smcTableHeader      * aTableHeader,
                                    smnIndexHeader      * aIndexHeader,
                                    void                * aHeader,
                                    iduFixedTableMemory * aMemory );
    static IDE_RC buildColumnRecord( smcTableHeader      * aTableHeader,
                                     smiColumn           * aColumn,
                                     void                * aHeader,
                                     iduFixedTableMemory * aMemory );

private:
    /* Table통계정보를 Trace파일에 출력함 */
    static IDE_RC traceTableStat( smcTableHeader  * aHeader,
                                  const SChar     * aTitle );

    /* Index통계정보를 Trace파일에 출력함 */
    static IDE_RC traceIndexStat( smcTableHeader  * aTableHeader,
                                  smnIndexHeader  * aPersHeader,
                                  const SChar     * aTitle );

    /* Key2String 함수를 이용해, Value를 String으로 변환함 */
    static IDE_RC getValueString( smiColumn * aColumn,
                                  UChar     * aValue,
                                  UInt        aBufferLength,
                                  SChar     * aBuffer );

    /* for Analyze Column stat 
     * 기존의 값과 비교하여 Min 또는 Max값이면, 설정해줌. */
    static IDE_RC compareAndSetMinMaxValue( smiStatTableArgument * aTableArgument,
                                            UChar                * aRow,
                                            smiColumn            * aColumn,
                                            UInt                   aColumnIdx,
                                            UInt                   aColumnSize );

    /* 실제로 통계정보를 설정(대입) 함.
     * - BUGBUG - 
     * Peterson Algorithm 사용을 위해 Macro썼을때, 
     * 자꾸 Syntax를 VI가 잘못 읽어서 Warning이 뜨기에 추가한 함수
     * 따라서 기존의 통계 누적방식이 없어지면 제거 가능 */
    static void setIndexStatInternal( smiIndexStat * aStat,
                                      SLong        * aKeyCount,
                                      SLong        * aNumPage,
                                      SLong        * aNumDist,
                                      SLong        * aAvgSlotCnt,
                                      SLong        * aClusteringFactor,
                                      SLong        * aIndexHeight,
                                      SLong        * aMetaSpace,
                                      SLong        * aUsedSpace,
                                      SLong        * aAgableSpace,
                                      SLong        * aFreeSpace,
                                      SFloat       * aSampleSize );

    /* QP쪽에서는 smiTarns가 인자로 오고, SM에서는 smxTrans를 인자로
     * 보내기에, Internal 함수를 따로 둠. */
    static IDE_RC gatherTableStatInternal( idvSQL   * aStatistics,
                                           void     * aSmxTrans,
                                           void     * aTable, 
                                           SFloat     aPercentage, 
                                           SInt       aDegree,
                                           void     * aTotalTableArg,
                                           SChar      aNoInvalidate );

    static IDE_RC gatherIndexStatInternal( idvSQL       * aStatistics,
                                           void         * aSmxTrans,
                                           void         * aIndex,
                                           SFloat         aPercentage,
                                           SInt           aDegree,
                                           SChar          aNoInvalidate );

    static IDE_RC lock4GatherStat( void           * aSmxTrans,
                                   smcTableHeader * aTableHeader,
                                   idBool           aNeedIXLock );
    static IDE_RC unlock4GatherStat( smcTableHeader * aTableHeader );

    static SFloat getAdjustedPercentage( SFloat aPercentage,
                                         SLong  aTargetSize );
    static SInt   getAdjustedDegree( SInt         aDegree );

    static void   checkNoInvalidate( smcTableHeader * aTarget,
                                     SChar            aNoInvalidate );

    /* PROJ-2339 */
    static IDE_RC analyzeRow4TotalStat( smiStatColumnArgument * aTotalColArg, 
                                        smiStatColumnArgument * aCurrColArg,
                                        SChar                 * aValue, 
                                        UInt                    aHashValue, 
                                        UInt                    aColumnSize );

    /* Peterson Algorithm을 고려해야함. 파일 최상단 주석 참조. */
    inline static IDE_RC lock4SetStat()
    { 
        IDE_TEST( mStatMutex.lock(NULL) != IDE_SUCCESS );
        IDE_ERROR( mAtomicA == mAtomicB );
        mAtomicB ++; 

        return IDE_SUCCESS;

        IDE_EXCEPTION_END;

        return IDE_FAILURE;
    }
    inline static IDE_RC unlock4SetStat()
    { 
        mAtomicA ++; 
        IDE_TEST( mStatMutex.unlock() != IDE_SUCCESS );

        return IDE_SUCCESS;

        IDE_EXCEPTION_END;

        return IDE_FAILURE;
    }

    static smiSystemStat            mSystemStat;
    static iduMutex                 mStatMutex; 
    static UInt                     mAtomicA;
    static UInt                     mAtomicB;
};

inline IDE_RC smiStatistics::getSystemStatSReadTime( idBool     aCurrent, 
                                                     SDouble  * aRet )
{
    IDE_ERROR( aRet != NULL );

    if( aCurrent == ID_TRUE )
    {
        *aRet = sdbBufferMgr::getBufferPoolStat()->getSingleReadPerf();
    }
    else
    {
        if( isValidSystemStat() == ID_FALSE )
        {
            IDE_TEST( gatherSystemStats(ID_TRUE) != IDE_SUCCESS );
        }
        SMI_GETSTAT( *aRet = mSystemStat.mSReadTime );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smiStatistics::getSystemStatMReadTime( idBool     aCurrent, 
                                                     SDouble  * aRet )
{
    IDE_ERROR( aRet != NULL );

    if( aCurrent == ID_TRUE )
    {
        *aRet = sdbBufferMgr::getBufferPoolStat()->getMultiReadPerf();
    }
    else
    {
        if( isValidSystemStat() == ID_FALSE )
        {
            IDE_TEST( gatherSystemStats(ID_TRUE) != IDE_SUCCESS );
        }
        SMI_GETSTAT( *aRet = mSystemStat.mMReadTime );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smiStatistics::getSystemStatDBFileMultiPageReadCount( 
    idBool   aCurrent, 
    SLong  * aRet )
{
    IDE_ERROR( aRet != NULL );

    if( aCurrent == ID_TRUE )
    {
        *aRet = smuProperty::getDBFileMutiReadCnt();
    }
    else
    {
        if( isValidSystemStat() == ID_FALSE )
        {
            IDE_TEST( gatherSystemStats(ID_TRUE) != IDE_SUCCESS );
        }
        SMI_GETSTAT( *aRet = mSystemStat.mDBFileMultiPageReadCount );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smiStatistics::getSystemStatHashTime( SDouble  * aRet )
{
    IDE_ERROR( aRet != NULL );

    if( isValidSystemStat() == ID_TRUE )
    {
        SMI_GETSTAT( *aRet = mSystemStat.mHashTime );
    }
    else
    {
        *aRet = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smiStatistics::getSystemStatCompareTime( SDouble  * aRet )
{
    IDE_ERROR( aRet != NULL );

    if( isValidSystemStat() == ID_TRUE )
    {
        SMI_GETSTAT( *aRet = mSystemStat.mCompareTime );
    }
    else
    {
        *aRet = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smiStatistics::getSystemStatStoreTime( SDouble  * aRet )
{
    IDE_ERROR( aRet != NULL );

    if( isValidSystemStat() == ID_TRUE )
    {
        SMI_GETSTAT( *aRet = mSystemStat.mStoreTime );
    }
    else
    {
        *aRet = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * DESCRIPTION : 
 *     Table의 Row 개수를 반환합니다.
 *
 *  aTable               - [IN]  대상 테이블
 *  aCurrent             - [IN]  '현재시점'의 통계를 가져올 것인가?
 *  aSmiTrans            - [IN]  '현재시점'의 값을 가져올때, Transaction이
 *                               삽입/삭제한 Row도 고려해야 한다.
 *  aRet                 - [OUT] 통계값
 ************************************************************************/
inline IDE_RC smiStatistics::getTableStatNumRow( void       * aTable,
                                                 idBool       aCurrent, 
                                                 smiTrans   * aSmiTrans,
                                                 SLong      * aRet )
{
    smcTableHeader    * sHeader       = NULL;
    smxTrans          * sTrans;
    SLong               sRecCntOfTableInfo;
    void              * sDPathSegInfo;

    if(aTable != NULL)
    {
        sHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);

        if( aCurrent == ID_TRUE )
        {
            IDE_TEST( smcTable::getRecordCount( sHeader, 
                                                (ULong*)aRet ) != IDE_SUCCESS );
            if (aSmiTrans != NULL)
            {
                sTrans = (smxTrans*)(aSmiTrans->getTrans());

                sDPathSegInfo = sdcDPathInsertMgr::getDPathSegInfo(
                    sTrans,
                    sHeader->mSelfOID );

                IDE_TEST_RAISE( sDPathSegInfo != NULL, 
                                ERROR_DML_AFTER_INSERT_APPEND );

                IDE_TEST( sTrans->getRecordCountFromTableInfo( sHeader->mSelfOID,
                                                               &sRecCntOfTableInfo)
                          != IDE_SUCCESS );

                *aRet += sRecCntOfTableInfo;
            }
            else
            {
                // Nothing to do
            }
        }
        else
        {
            if ( isValidTableStat( sHeader ) == ID_TRUE )
            {
                SMI_GETSTAT( *aRet = sHeader->mStat.mNumRow );
            }
            else
            {
                *aRet = SMI_STAT_NULL;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_DML_AFTER_INSERT_APPEND );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_DML_AFTER_INSERT_APPEND) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * DESCRIPTION : 
 *     Table의 page개수를 반환합니다.
 *
 *  aTable               - [IN]  대상 테이블
 *  aCurrent             - [IN]  '현재시점'의 통계를 가져올 것인가?
 *  aRet                 - [OUT] 통계값
 ************************************************************************/
inline IDE_RC smiStatistics::getTableStatNumPage( void   * aTable, 
                                                  idBool   aCurrent, 
                                                  SLong  * aRet )
{
    smcTableHeader  * sHeader       = NULL;
    UInt              sTableType;
    sdpSegMgmtOp    * sSegMgmtOP;
    sdpSegInfo        sSegInfo;
    SLong             sRet = 0;

    sHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);

    if( aCurrent == ID_TRUE )
    {
        sTableType = sHeader->mFlag & SMI_TABLE_TYPE_MASK;
        switch( sTableType )
        {
        case SMI_TABLE_META:
        case SMI_TABLE_MEMORY:
        case SMI_TABLE_REMOTE:
            sRet = smpManager::getAllocPageCount( &(sHeader->mFixed.mMRDB) ) +
                   smpManager::getAllocPageCount( sHeader->mVar.mMRDB ) ;
            break;
        case SMI_TABLE_VOLATILE:
            sRet = svpManager::getAllocPageCount( &(sHeader->mFixed.mVRDB) ) +
                   svpManager::getAllocPageCount( sHeader->mVar.mVRDB ) ;
            break;
        case SMI_TABLE_DISK:
            sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( sHeader->mSpaceID );
            // codesonar::Null Pointer Dereference
            IDE_ERROR( sSegMgmtOP != NULL );

            IDE_TEST( sSegMgmtOP->mGetSegInfo( NULL,
                                               sHeader->mSpaceID,
                                               sdpSegDescMgr::getSegPID( &(sHeader->mFixed.mDRDB) ),
                                               NULL, /* aTableHeader */
                                               &sSegInfo )
                      != IDE_SUCCESS );
            sRet = sSegInfo.mFmtPageCnt;
            break;
        case SMI_TABLE_FIXED:
            sRet = 0;
            break;
        case SMI_TABLE_TEMP_LEGACY:
        default:
            IDE_ERROR_MSG( 0,
                           "TableOID  :%"ID_UINT64_FMT
                           "TableType :%"ID_UINT64_FMT,
                           sHeader->mSelfOID,
                           sTableType );
            break;
        }
    }
    else
    {
        if ( isValidTableStat( sHeader ) == ID_TRUE )
        {
            SMI_GETSTAT( sRet = sHeader->mStat.mNumPage; );
        }
        else
        {
            sRet = SMI_STAT_NULL;
        }
    }

    *aRet = sRet;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smiStatistics::getTableStatAverageRowLength( void * aTable, SLong * aRet )
{
    smcTableHeader  * sHeader;

    sHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);

    if( isValidTableStat( aTable ) == ID_TRUE )
    {
        SMI_GETSTAT( *aRet = sHeader->mStat.mAverageRowLen; );
    }
    else
    {
        *aRet = SMI_STAT_NULL;
    }

    return IDE_SUCCESS;
}

inline IDE_RC smiStatistics::getTableStatOneRowReadTime( void * aTable, SDouble * aRet )
{
    smcTableHeader  * sHeader;

    sHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER(aTable);

    if( isValidTableStat( aTable ) == ID_TRUE )
    {
        SMI_GETSTAT( *aRet = sHeader->mStat.mOneRowReadTime; );
    }
    else
    {
        *aRet = SMI_STAT_READTIME_NULL;
    }

    return IDE_SUCCESS;
}

inline IDE_RC smiStatistics::getIndexStatKeyCount( void * aIndex, SLong * aRet )
{
    smnIndexHeader * sHeader;

    if( isValidIndexStat( aIndex ) == ID_TRUE )
    {
        sHeader = (smnIndexHeader*)aIndex;

        SMI_INDEX_GETSTAT( sHeader,
                           *aRet = sHeader->mStat.mKeyCount );
    }
    else
    {
        *aRet = SMI_STAT_NULL;
    }

    return IDE_SUCCESS;
}

/************************************************************************
 * DESCRIPTION : 
 *     Index의 page개수를 반환합니다.
 *
 *  aTable               - [IN]  대상 테이블
 *  aCurrent             - [IN]  '현재시점'의 통계를 가져올 것인가?
 *  aRet                 - [OUT] 통계값
 ************************************************************************/
inline IDE_RC smiStatistics::getIndexStatNumPage( void          * aIndex,
                                                  idBool          aCurrent,
                                                  SLong         * aRet )
{
    sdnRuntimeHeader * sDRDBHeader;
    smnIndexHeader   * sIndexHeader;
    smcTableHeader   * sTableHeader;
    UInt               sTableType;
    sdpSegMgmtOp     * sSegMgmtOP;
    sdpSegmentDesc   * sSegDesc;
    sdpSegInfo         sSegInfo;
    SLong              sRet = 0;

    if( aCurrent == ID_TRUE )
    {
        if( smnManager::getIsConsistentOfIndexHeader( aIndex )
            == ID_FALSE )
        {
            /* Index가 Inconsistent하기 때문에 0을 설정해줌.
             * 여기서 예외처리하면 QP가 Plan을 구축하는 과정에서
             * 실패하기 때문에, 아예 정상적인 다른 Index도 접근을 못함
             * 따라서 NumDist를 낮은 값으로 하여 일단 이 Index에 대한
             * 접근 우선순위를 낮춤. 어차피 잘못되면 실행 과정에서 예외
             * 처리됨 */
            sRet = 0;
        }
        else
        {
            sIndexHeader   = (smnIndexHeader*)aIndex;
            IDE_TEST( smcTable::getTableHeaderFromOID( sIndexHeader->mTableOID,
                                                       (void**)&sTableHeader )
                      != IDE_SUCCESS );

            sTableType = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;
            switch( sTableType )
            {
            case SMI_TABLE_META:
            case SMI_TABLE_MEMORY:
            case SMI_TABLE_VOLATILE:
            case SMI_TABLE_REMOTE:                
                sRet = ((smnbHeader*)sIndexHeader->mHeader)->nodeCount;
                break;
            case SMI_TABLE_DISK:
                sDRDBHeader = ((sdnRuntimeHeader*)sIndexHeader->mHeader);
                sSegDesc    = &sDRDBHeader->mSegmentDesc;
                sSegMgmtOP  = sdpSegDescMgr::getSegMgmtOp( sSegDesc );
                // codesonar::Null Pointer Dereference
                IDE_ERROR( sSegMgmtOP != NULL );
            
                IDE_TEST( sSegMgmtOP->mGetSegInfo(
                                        NULL,
                                        sDRDBHeader->mIndexTSID,
                                        sdpSegDescMgr::getSegPID( sSegDesc ),
                                        NULL, /* aTableHeader */
                                        &sSegInfo )
                          != IDE_SUCCESS );
                sRet = sSegInfo.mFmtPageCnt;
                break;
            case SMI_TABLE_FIXED:
                sRet = 0;
                break;
            case SMI_TABLE_TEMP_LEGACY:
            default:
                IDE_ERROR_MSG( 0,
                               "TableOID  :%"ID_UINT64_FMT
                               "TableType :%"ID_UINT64_FMT,
                               sTableHeader->mSelfOID,
                               sTableType );
                break;
            }
        } /* if( isConsistentIndex ) */
    }
    else
    {
        if( isValidIndexStat( aIndex ) == ID_TRUE )
        {
            sIndexHeader   = (smnIndexHeader*)aIndex;

            SMI_INDEX_GETSTAT( sIndexHeader,
                               sRet = sIndexHeader->mStat.mNumPage );
        }
        else
        {
            sRet = SMI_STAT_NULL;
        }
    }

    *aRet = sRet;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smiStatistics::getIndexStatNumDist( void      * aIndex,
                                                  SLong     * aRet )
{
    smnIndexHeader * sHeader;
    smcTableHeader * sTableHeader;

    if( isValidIndexStat( aIndex ) == ID_TRUE )
    {
        sHeader = (smnIndexHeader*)aIndex;

        if( ( smuProperty::getDBMSStatMethod() == 
              SMU_DBMS_STAT_METHOD_AUTO ) &&
            ( ( sHeader->mFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK ) &&
            ( ( sHeader->mFlag & SMI_INDEX_TYPE_MASK ) ==
               SMI_INDEX_TYPE_PRIMARY_KEY ) )
        {
            IDE_TEST( smcTable::getTableHeaderFromOID( sHeader->mTableOID,
                                                       (void**)&sTableHeader )
                      != IDE_SUCCESS );

            /* Unique이고 Auto일 경우, Cardinality 갱신을 하지 않는다 */
            IDE_TEST( getTableStatNumRow( sTableHeader - SMP_SLOT_HEADER_SIZE, // BUG-42323
                                          ID_TRUE, /* aCurrent */
                                          NULL,    /* aTrans */
                                          aRet )
                      != IDE_SUCCESS );
        }
        else
        {
            SMI_INDEX_GETSTAT( sHeader,
                               *aRet = sHeader->mStat.mNumDist );
        }
    }
    else
    {
        *aRet = SMI_STAT_NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smiStatistics::getIndexStatMin( void * aIndex, void * aRet )
{
    smnIndexHeader * sHeader;
    UChar          * sTarget;

    if( isValidIndexStat( aIndex ) == ID_TRUE )
    {
        sHeader = (smnIndexHeader*)aIndex;

        if((sHeader->mColumnFlags[0] & SMI_COLUMN_ORDER_MASK)
           == SMI_COLUMN_ORDER_ASCENDING)
        {
            sTarget = (UChar*)sHeader->mStat.mMinValue;
        }
        else
        {
            sTarget = (UChar*)sHeader->mStat.mMaxValue;
        }

        SMI_INDEX_GETSTAT( sHeader,
                           idlOS::memcpy( aRet,
                                          sTarget,
                                          MAX_MINMAX_VALUE_SIZE ) );
    }
    else
    {
        idlOS::memset( aRet, 0, MAX_MINMAX_VALUE_SIZE );
    }

    return IDE_SUCCESS;
}

inline IDE_RC smiStatistics::getIndexStatMax( void * aIndex, void * aRet )
{
    smnIndexHeader * sHeader;
    UChar          * sTarget;

    if( isValidIndexStat( aIndex ) == ID_TRUE )
    {
        sHeader = (smnIndexHeader*)aIndex;

        if((sHeader->mColumnFlags[0] & SMI_COLUMN_ORDER_MASK)
           == SMI_COLUMN_ORDER_ASCENDING)
        {
            sTarget = (UChar*)sHeader->mStat.mMaxValue;
        }
        else
        {
            sTarget = (UChar*)sHeader->mStat.mMinValue;
        }

        SMI_INDEX_GETSTAT( sHeader,
                           idlOS::memcpy( aRet,
                                          sTarget,
                                          MAX_MINMAX_VALUE_SIZE ) );
    }
    else
    {
        idlOS::memset( aRet, 0, MAX_MINMAX_VALUE_SIZE );
    }

    return IDE_SUCCESS;
}

inline IDE_RC smiStatistics::getIndexStatIndexHeight( void  * aIndex, 
                                                      SLong * aRet )
{
    smnIndexHeader * sHeader;

    if( isValidIndexStat( aIndex ) == ID_TRUE )
    {
        sHeader = (smnIndexHeader*)aIndex;

        SMI_INDEX_GETSTAT( sHeader,
                           *aRet = sHeader->mStat.mIndexHeight );
    }
    else
    {
        *aRet = SMI_STAT_NULL;
    }

    return IDE_SUCCESS;
}

inline IDE_RC smiStatistics::getIndexStatClusteringFactor( void * aIndex, SLong * aRet )
{
    smnIndexHeader * sHeader;

    if( isValidIndexStat( aIndex ) == ID_TRUE )
    {
        sHeader = (smnIndexHeader*)aIndex;
        SMI_INDEX_GETSTAT( sHeader, 
                           *aRet = sHeader->mStat.mClusteringFactor );
    }
    else
    {
        *aRet = SMI_STAT_NULL;
    }

    return IDE_SUCCESS;
}

inline IDE_RC smiStatistics::getIndexStatAvgSlotCnt( void * aIndex, SLong * aRet )
{
    smnIndexHeader * sHeader;

    if( isValidIndexStat( aIndex ) == ID_TRUE )
    {
        sHeader = (smnIndexHeader*)aIndex;
        SMI_INDEX_GETSTAT( sHeader, 
                           *aRet = sHeader->mStat.mAvgSlotCnt );
    }
    else
    {
        *aRet = SMI_STAT_NULL;
    }

    return IDE_SUCCESS;
}

inline IDE_RC smiStatistics::getColumnStatNumDist( void         * aTable,
                                                   UInt           aColumnID,
                                                   SLong        * aRet )
{
    smiColumn        * sColumn;

    if( isValidColumnStat( aTable, aColumnID ) == ID_TRUE )
    {    
        sColumn = (smiColumn*)smcTable::getColumn( SMI_MISC_TABLE_HEADER(aTable), aColumnID );

        SMI_GETSTAT( *aRet = sColumn->mStat.mNumDist );
    }    
    else 
    {    
        *aRet = SMI_STAT_NULL;
    }    

    return IDE_SUCCESS;
}

inline IDE_RC smiStatistics::getColumnStatNumNull( void         * aTable,
                                                   UInt           aColumnID,
                                                   SLong        * aRet )
{
    smiColumn        * sColumn;

    if( isValidColumnStat( aTable, aColumnID ) == ID_TRUE )
    {
        sColumn = (smiColumn*)smcTable::getColumn( SMI_MISC_TABLE_HEADER(aTable), aColumnID );

        SMI_GETSTAT( *aRet = sColumn->mStat.mNumNull; );
    }
    else
    {
        *aRet = SMI_STAT_NULL;
    }

    return IDE_SUCCESS;
}
inline IDE_RC smiStatistics::getColumnStatAverageColumnLength( void         * aTable,
                                                               UInt           aColumnID,
                                                               SLong        * aRet )
{
    smiColumn        * sColumn;

    if( isValidColumnStat( aTable, aColumnID ) == ID_TRUE )
    {
        sColumn = (smiColumn*)smcTable::getColumn( SMI_MISC_TABLE_HEADER(aTable), aColumnID );

        SMI_GETSTAT( *aRet = sColumn->mStat.mAverageColumnLen );
    }
    else
    {
        *aRet = SMI_STAT_NULL;
    }
    return IDE_SUCCESS;
}
inline IDE_RC smiStatistics::getColumnStatMin( void         * aTable,
                                               UInt           aColumnID,
                                               void         * aRet )
{
    smiColumn        * sColumn;

    if( isValidColumnStat( aTable, aColumnID ) == ID_TRUE )
    {
        sColumn = (smiColumn*)smcTable::getColumn( SMI_MISC_TABLE_HEADER(aTable), aColumnID );

        SMI_GETSTAT( idlOS::memcpy( aRet,
                                    sColumn->mStat.mMinValue,
                                    MAX_MINMAX_VALUE_SIZE ) );
    }
    else
    {
        idlOS::memset( aRet, 0, MAX_MINMAX_VALUE_SIZE );
    }

    return IDE_SUCCESS;
}
inline IDE_RC smiStatistics::getColumnStatMax( void         * aTable,
                                               UInt           aColumnID,
                                               void         * aRet )
{
    smiColumn        * sColumn;

    if( isValidColumnStat( aTable, aColumnID ) == ID_TRUE )
    {
        sColumn = (smiColumn*)smcTable::getColumn( SMI_MISC_TABLE_HEADER(aTable), aColumnID );

        SMI_GETSTAT( idlOS::memcpy( aRet,
                                    sColumn->mStat.mMaxValue,
                                    MAX_MINMAX_VALUE_SIZE ) );
    }
    else
    {
        idlOS::memset( aRet, 0, MAX_MINMAX_VALUE_SIZE );
    }
    return IDE_SUCCESS;
}

#define EPSILON (1e-8)  // 정밀도.
/* 적당한 Sampling 값을 Page크기 및 Base크기 바탕으로 계산함 */
inline SFloat smiStatistics::getAdjustedPercentage( SFloat aPercentage,
                                                    SLong  aTargetSize )
{
    SFloat sRet = idlOS::fabs(aPercentage);

    if ( sRet < EPSILON )
    {
        if( aTargetSize < smuProperty::getDBMSStatSamplingBaseCnt()  )
        {
            sRet = 1.0f;
        }
        else
        {
            /* Automatic하게 결정함 */
            sRet = 1.0f / 
                idlOS::sqrt( ((SFloat)aTargetSize) 
                             / smuProperty::getDBMSStatSamplingBaseCnt() );
        }
    }
    else
    {
        /* nothing to do */
    }

    /* BUG-34240 [SM] sampling percentage that is greater then 1.0
     *           in GATHER_INDEX_STATS must be adjusted.
     * sRet는 0.0 ~ 1.0 사이의 값이어야 한다.
     * 이 범위를 넘어가는 경우 0.0 또는 1.0으로 보정해준다. */
    if ( sRet < 0.0f )
    {
        sRet = 0.0f;
    }
    else
    {
        if ( sRet > 1.0f )
        {
            sRet = 1.0f;
        }
        else
        {
            ; /* nothing to do */
        }
    }

    return sRet;
}

/* ParallelDegree를 가져옴 */
inline SInt smiStatistics::getAdjustedDegree( SInt         aDegree )
{
    SInt sRet = 0;

    if( aDegree == 0 )
    {
        sRet = (SInt)smuProperty::getDBMSStatParallelDegree();
    }
    else
    {
        sRet = aDegree;
    }

    return sRet;
}

/* NoInvalidate 옵션에 따라, Table의 Plan 재구축 여부를 판단함 */
inline void smiStatistics::checkNoInvalidate( smcTableHeader * aTarget,
                                              SChar            aNoInvalidate )
{
   if( aNoInvalidate == ID_FALSE )
    {
        if( ( aTarget->mFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_META )
        {
            /* META Table은 Rebuild를 하지 않는다. 왜냐하면 gather... 종류는
             * Meta를 이용하는데, 이 Meta가 Rebuild 되면 무한 Rebuild가
             * 일어날 수 있기 때문이다. */
        }
        else
        {
            smcTable::changeTableSCNForRebuild( aTarget->mSelfOID );
        }
    }
}

#endif /* __O_SMI_STATISTICS_H__ */
