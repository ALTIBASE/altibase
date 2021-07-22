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
 * $Id: $
 **********************************************************************/

#ifndef _O_SMI_TEMP_TABLE_H_
#define _O_SMI_TEMP_TABLE_H_ 1

#include <idu.h>
#include <smDef.h>
#include <smiDef.h>
#include <smiMisc.h>
#include <sdtDef.h>

class smiTempTable
{
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static IDE_RC drop(void   * aTable);


    static IDE_RC fetchFromGRID( void    * aTable,
                                 scGRID    aGRID,
                                 void    * aDestRowBuf );

    static IDE_RC getNullRow(void   * aTable,
                             UChar ** aNullRowPtr);
    static IDE_RC makeNullRow( smiTempTableHeader   * aHeader );
    static void   initStatsPtr(smiTempTableHeader * aHeader,
                               idvSQL             * aStatistics,
                               smiStatement       * aStatement );
    static smiTempTableStats * registWatchArray( smiTempTableStats * aStatsPtr );

    static void   accumulateStats( smiTempTableHeader * aTable );

    static void checkAndDump( smiTempTableHeader * aHeader );

public:
    /************************************************************
     * Build fixed table
     * FixedTable X$TEMPTABLE_STATS만드는데 사용된다.
     ************************************************************/
    static IDE_RC buildTempTableStatsRecord( idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * aDumpObj,
                                             iduFixedTableMemory * aMemory );

    /************************************************************
     * Build fixed table
     * FixedTable X$TEMPINFO를 만드는데 사용된다.
     ************************************************************/
    static IDE_RC buildTempInfoRecord( idvSQL              * /*aStatistics*/,
                                       void                * aHeader,
                                       void                * aDumpObj,
                                       iduFixedTableMemory * aMemory );


    /************************************************************
     * Build fixed table
     * FixedTable X$TEMPTABLE_OPER만드는데 사용된다.
     ************************************************************/
    static IDE_RC buildTempTableOprRecord( idvSQL              * /*aStatistics*/,
                                           void                * aHeader,
                                           void                * aDumpObj,
                                           iduFixedTableMemory * aMemory );

    static IDE_RC allocTempTableHdr( smiTempTableHeader ** aHeader )
    {
        return mTempTableHdrPool.alloc( (void**)aHeader );
    };

    static IDE_RC freeTempTableHdr( smiTempTableHeader * aHeader )
    {
        return mTempTableHdrPool.memfree( aHeader );
    };

private:
    /* 오류 발생시 TempTable의 자세한 내용들을 따로 파일로 Dump한다.
     * 그리고 Dump한 객체에 대한 정보를 altibase_dump.trc에도 출력한다. */
    static void   dumpToFile( smiTempTableHeader * aHeader );
    static void   dumpTempTableHeader( void   * aTableHeader, 
                                       SChar  * aOutBuf, 
                                       UInt     aOutSize );
    static void   dumpTempTableSegment( void   * aTableHeader, 
                                        SChar  * aOutBuf, 
                                        UInt     aOutSize );
    static void   dumpTempStats( smiTempTableStats * aTempStats,
                                 SChar             * aOutBuf, 
                                 UInt                aOutSize );

    static void   makeStatsPerf( smiTempTableStats      * aTempStats,
                                 smiTempTableStats4Perf * aPerf );

    static SChar               mOprName[][ SMI_TT_STR_SIZE ];
    static SChar               mTTStateName[][ SMI_TT_STR_SIZE ];

    static iduMemPool          mTempTableHdrPool;

    static smiTempTableStats   mGlobalStats;
    static smiTempTableIOStats mSortIOStats;
    static smiTempTableIOStats mHashIOStats;
    static smiTempTableStats * mTempTableStatsWatchArray;
    static UInt                mStatIdx;
};


#endif /* _O_SMI_TEMP_TABLE_H_ */
