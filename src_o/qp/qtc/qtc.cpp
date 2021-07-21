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
 * $Id: qtc.cpp 90923 2021-05-31 01:27:54Z donovan.seo $
 *
 * Description :
 *     QP layer와 MT layer의 중간에 위치하는 layer로
 *     개념상 MT interface layer 역할을 한다.
 *
 *     여기에는 QP 전체에 걸쳐 공통적으로 사용되는 함수가 정의되어 있다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtd.h>
#include <mtc.h>
#include <qcg.h>
#include <qtc.h>
#include <qmn.h>
#include <qsv.h>
#include <qcuSqlSourceInfo.h>
#include <mtdTypes.h>
#include <qsf.h>
#include <qcgPlan.h>
#include <qcsModule.h>
#include <qmoOuterJoinOper.h>
#include <qcpManager.h>
#include <mtuProperty.h>
#include <qmsPreservedTable.h>
#include <qcmUser.h>
#include <qtcDef.h>
#include <qsxArray.h>
#include <qmv.h>
#include <qmvQTC.h>
#include <qsvProcStmts.h>

extern mtdModule mtdBoolean;
extern mtdModule mtdUndef;
extern mtdModule mtdBit;
extern mtdModule mtdVarbit;
extern mtdModule mtdByte;
extern mtdModule mtdVarbyte;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdEchar;
extern mtdModule mtdEvarchar;
extern mtdModule mtdNchar;
extern mtdModule mtdNvarchar;
extern mtdModule mtdNumeric;
extern mtdModule mtdFloat;

extern mtfModule mtfGetBlobValue;
extern mtfModule mtfGetClobValue;
extern mtfModule mtfList;
extern mtfModule mtfCast;
extern mtfModule mtfIsNull;
extern mtfModule mtfIsNotNull;
extern mtfModule mtfCase2;
extern mtfModule mtfDecode;
extern mtfModule mtfDigest;
extern mtfModule mtfDump;
extern mtfModule mtfNvl;
extern mtfModule mtfNvl2;
extern mtfModule mtfEqualAny;
extern mtfModule mtfEqual;
extern mtfModule mtfNotEqual;
extern mtfModule mtfLessThan;
extern mtfModule mtfLessEqual;
extern mtfModule mtfGreaterThan;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfLike;
extern mtfModule mtfNotLike;
extern mtfModule mtfLnnvl;
extern mtfModule mtfExists;
extern mtfModule mtfNotExists;
extern mtfModule mtfListagg;
extern mtfModule mtfPercentileCont;
extern mtfModule mtfPercentileDisc;
extern mtfModule mtfRank;
extern mtfModule mtfRankWithinGroup;    // BUG-41631
extern mtfModule mtfPercentRankWithinGroup;    // BUG-41771
extern mtfModule mtfCumeDistWithinGroup;       // BUG-41800
extern mtfModule qsfMCountModule;    // PROJ-2533
extern mtfModule qsfMFirstModule;
extern mtfModule qsfMLastModule;
extern mtfModule qsfMNextModule;
extern mtfModule mtfRatioToReport;
extern mtfModule mtfMin;
extern mtfModule mtfMinKeep;
extern mtfModule mtfMax;
extern mtfModule mtfMaxKeep;
extern mtfModule mtfSum;
extern mtfModule mtfSumKeep;
extern mtfModule mtfAvg;
extern mtfModule mtfAvgKeep;
extern mtfModule mtfCount;
extern mtfModule mtfCountKeep;
extern mtfModule mtfVariance;
extern mtfModule mtfVarianceKeep;
extern mtfModule mtfStddev;
extern mtfModule mtfStddevKeep;

qcDepInfo qtc::zeroDependencies;

smiCallBackFunc qtc::rangeFuncs[] = {
    qtc::rangeMinimumCallBack4Mtd,
    qtc::rangeMinimumCallBack4GEMtd,
    qtc::rangeMinimumCallBack4GTMtd,
    qtc::rangeMinimumCallBack4Stored,
    qtc::rangeMaximumCallBack4Mtd,
    qtc::rangeMaximumCallBack4LEMtd,
    qtc::rangeMaximumCallBack4LTMtd,
    qtc::rangeMaximumCallBack4Stored,
    qtc::rangeMinimumCallBack4IndexKey,
    qtc::rangeMaximumCallBack4IndexKey,
    NULL
};

mtdCompareFunc qtc::compareFuncs[] = {
    qtc::compareMinimumLimit,
    qtc::compareMaximumLimit4Mtd,
    qtc::compareMaximumLimit4Stored,
    NULL
};

/***********************************************************************
 * [qtc::templateRowFlags]
 *
 * Tuple Set의 각 Tuple은 크게 다음과 같은 네 종류로 구분할 수 있다.
 *    - MTC_TUPLE_TYPE_CONSTANT     : 상수 Tuple
 *    - MTC_TUPLE_TYPE_VARIABLE     : 가변 Tuple(Host변수를 포함)
 *    - MTC_TUPLE_TYPE_INTERMEDIATE : 중간 결과 Tuple
 *    - MTC_TUPLE_TYPE_TABLE        : Table Tuple
 *
 * Stored Procedure의 수행을 위하여 Tuple Set에 대한 Clone 작업이 필요할
 * 경우, 위와 같은 다양한 Tuple에 대하여 다양한 처리가 가능하다.
 * 해당 Const Structure에는 각 Tuple에 대한 처리 방법에 대한 기술을
 * 한 flag가 정의되어 있다.
 **********************************************************************/

/* BUG-44490 mtcTuple flag 확장 해야 합니다. */
/* 32bit flag 공간 모두 소모해 64bit로 증가 */
/* type을 UInt에서 ULong으로 변경 */
const ULong qtc::templateRowFlags[MTC_TUPLE_TYPE_MAXIMUM] = {
    /* MTC_TUPLE_TYPE_CONSTANT                                       */
    // 상수 Tuple의 경우,
    // Column 정보, Execute 정보, Row 정보는 변경되지 않는다.
    MTC_TUPLE_TYPE_CONSTANT|
    MTC_TUPLE_COLUMN_SET_TRUE|        /* COLUMN  : set pointer       */
    MTC_TUPLE_COLUMN_ALLOCATE_FALSE|  /* COLUMN  : no allocation     */
    MTC_TUPLE_COLUMN_COPY_FALSE|      /* COLUMN  : no copy           */
    MTC_TUPLE_EXECUTE_SET_TRUE|       /* EXECUTE : set pointer       */
    MTC_TUPLE_EXECUTE_ALLOCATE_FALSE| /* EXECUTE : no allocation     */
    MTC_TUPLE_EXECUTE_COPY_FALSE|     /* EXECUTE : no copy           */
    MTC_TUPLE_ROW_SET_TRUE|           /* ROW     : set pointer       */
    MTC_TUPLE_ROW_ALLOCATE_FALSE|     /* ROW     : no allocation     */
    MTC_TUPLE_ROW_COPY_FALSE,         /* ROW     : no copy           */

    /* MTC_TUPLE_TYPE_VARIABLE                                       */
    // Host 변수를 포함한 Tuple의 경우,
    // Column 정보, Execute 정보, Row 정보는 Host Binding이 이루어지기
    // 전까지 어떠한 정보도 결정되어 있지 않다.
    MTC_TUPLE_TYPE_VARIABLE|
    MTC_TUPLE_COLUMN_SET_TRUE|        /* COLUMN  : set pointer       */
    MTC_TUPLE_COLUMN_ALLOCATE_FALSE|  /* COLUMN  : no allocation     */
    MTC_TUPLE_COLUMN_COPY_FALSE|      /* COLUMN  : no copy           */
    MTC_TUPLE_EXECUTE_SET_FALSE|      /* EXECUTE : no set            */
    MTC_TUPLE_EXECUTE_ALLOCATE_TRUE|  /* EXECUTE : allocation        */
    MTC_TUPLE_EXECUTE_COPY_TRUE|      /* EXECUTE : copy              */
    MTC_TUPLE_ROW_SET_FALSE|          /* ROW     : no set            */
    MTC_TUPLE_ROW_ALLOCATE_FALSE|     /* ROW     : no allocation     */
    MTC_TUPLE_ROW_COPY_FALSE,         /* ROW     : no copy           */

    /* MTC_TUPLE_TYPE_INTERMEDIATE                                   */
    // 중간 결과를 의미하는 Tuple의 경우,
    // Column 정보, Execute 정보, Row 정보는 연산이 수행되기 전까지는
    // 그 정보의 정확성을 보장할 수 없다.
    MTC_TUPLE_TYPE_INTERMEDIATE|
    MTC_TUPLE_COLUMN_SET_TRUE|        /* COLUMN  : set pointer       */
    MTC_TUPLE_COLUMN_ALLOCATE_FALSE|  /* COLUMN  : no allocation     */
    MTC_TUPLE_COLUMN_COPY_FALSE|      /* COLUMN  : no copy           */
    MTC_TUPLE_EXECUTE_SET_FALSE|      /* EXECUTE : no set            */
    MTC_TUPLE_EXECUTE_ALLOCATE_TRUE|  /* EXECUTE : allocation        */
    MTC_TUPLE_EXECUTE_COPY_TRUE|      /* EXECUTE : copy              */
    MTC_TUPLE_ROW_SET_FALSE|          /* ROW     : no set            */
    MTC_TUPLE_ROW_ALLOCATE_TRUE|      /* ROW     : allocation        */
    MTC_TUPLE_ROW_COPY_FALSE,         /* ROW     : no copy           */

    /* MTC_TUPLE_TYPE_TABLE                                          */
    // Table 을 의미하는 Tuple의 경우,
    // Column 정보 및 Execute 정보는 변하지 않는다.
    // 그러나, Disk Variable Column을 처리하기 위해서는
    // Column 정보를 복사하여야 한다.
    MTC_TUPLE_TYPE_TABLE|
    MTC_TUPLE_COLUMN_SET_TRUE|        /* COLUMN  : set pointer       */
    MTC_TUPLE_COLUMN_ALLOCATE_FALSE|  /* COLUMN  : no allocation     */
    MTC_TUPLE_COLUMN_COPY_FALSE|      /* COLUMN  : no copy           */
    MTC_TUPLE_EXECUTE_SET_TRUE|       /* EXECUTE : set pointer       */
    MTC_TUPLE_EXECUTE_ALLOCATE_FALSE| /* EXECUTE : no allocation     */
    MTC_TUPLE_EXECUTE_COPY_FALSE|     /* EXECUTE : no copy           */
    MTC_TUPLE_ROW_SET_FALSE|          /* ROW     : no set            */
    MTC_TUPLE_ROW_ALLOCATE_FALSE|     /* ROW     : no allocation     */
    MTC_TUPLE_ROW_COPY_FALSE          /* ROW     : no copy           */
};

/* BUG-44382 clone tuple 성능개선 */
void qtc::setTupleColumnFlag( mtcTuple * aTuple,
                              idBool     aCopyColumn,
                              idBool     aMemsetRow )
{
    // column 복사가 필요한 경우 (column의 value buffer, offset등을 사용하는 경우)
    if ( aCopyColumn == ID_TRUE )
    {
        aTuple->lflag &= ~MTC_TUPLE_COLUMN_SET_MASK;
        aTuple->lflag |= MTC_TUPLE_COLUMN_SET_FALSE;
        aTuple->lflag &= ~MTC_TUPLE_COLUMN_ALLOCATE_MASK;
        aTuple->lflag |= MTC_TUPLE_COLUMN_ALLOCATE_TRUE;
        aTuple->lflag &= ~MTC_TUPLE_COLUMN_COPY_MASK;
        aTuple->lflag |= MTC_TUPLE_COLUMN_COPY_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    // row할당시 memset이 필요한 경우 (초기화가 필요한 경우)
    if ( aMemsetRow == ID_TRUE )
    {
        aTuple->lflag &= ~MTC_TUPLE_ROW_MEMSET_MASK;
        aTuple->lflag |= MTC_TUPLE_ROW_MEMSET_TRUE;
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC qtc::rangeMinimumCallBack4Mtd( idBool     * aResult,
                                      const void * aRow,
                                      void       *, /* aDirectKey */
                                      UInt        , /* aDirectKeyPartialSize */
                                      const scGRID,
                                      void       * aData )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta 관리를 위한 Minimum Key Range CallBack으로만 사용된다.
 *    따라서, Disk Variable Column에 대한 고려가 필요없다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt                 sOrder;
    qtcMetaRangeColumn * sData;
    mtdValueInfo         sValueInfo1;
    mtdValueInfo         sValueInfo2;

    sOrder = 0;

    for ( sData = (qtcMetaRangeColumn*)aData ;
          ( sOrder == 0 ) && ( sData != NULL ) ;
          sData = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = aRow;
        sValueInfo1.length = 0; // do not use
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }

    // Column 값이 Min Value값보다 오른쪽에 있거나 같을 경우
    // TRUE 값을 Setting한다.
    //     ------------- Column -------------
    //     |                                |
    //     V                                V
    //  Min Value                        Max Value
    *aResult = ( sOrder >= 0 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC qtc::rangeMinimumCallBack4GEMtd( idBool       * aResult,
                                        const void   * aRow,
                                        void         * aDirectKey,
                                        UInt           aDirectKeyPartialSize,
                                        const scGRID   aRid,
                                        void         * aData )
{
    return rangeMinimumCallBack4Mtd( aResult, 
                                     aRow, 
                                     aDirectKey, 
                                     aDirectKeyPartialSize, 
                                     aRid, 
                                     aData );
}

IDE_RC qtc::rangeMinimumCallBack4GTMtd( idBool       * aResult,
                                        const void   * aRow,
                                        void         *, /* aDirectKey */
                                        UInt          , /* aDirectKeyPartialSize */
                                        const scGRID   /* aRid */,
                                        void         * aData )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta 관리를 위한 Minimum Key Range CallBack으로만 사용된다.
 *    따라서, Disk Variable Column에 대한 고려가 필요없다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt                 sOrder;
    qtcMetaRangeColumn * sData;
    mtdValueInfo         sValueInfo1;
    mtdValueInfo         sValueInfo2;

    sOrder = 0;

    for ( sData = (qtcMetaRangeColumn*)aData ;
          ( sOrder == 0 ) && ( sData != NULL ) ;
          sData = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = aRow;
        sValueInfo1.length = 0; // do not use
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }

    // Column 값이 Min Value값보다 오른쪽에 있을 경우
    // TRUE 값을 Setting한다.
    //     ------------- Column -------------
    //     |                                |
    //     V                                V
    //  Min Value                        Max Value
    *aResult = ( sOrder > 0 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC qtc::rangeMinimumCallBack4Stored( idBool     * aResult,
                                         const void * aColVal,
                                         void       *, /* aDirectKey */
                                         UInt        , /* aDirectKeyPartialSize */
                                         const scGRID,
                                         void       * aData )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta 관리를 위한 Minimum Key Range CallBack으로만 사용된다.
 *    따라서, Disk Variable Column에 대한 고려가 필요없다.
 *
 * Implementation :
 *
 ***********************************************************************/

    const smiValue     * sColVal;
    SInt                 aOrder;
    qtcMetaRangeColumn * sData;
    mtdValueInfo         sValueInfo1;
    mtdValueInfo         sValueInfo2;
    
    sColVal = (const smiValue*)aColVal;

    for( aOrder  = 0,   sData  = (qtcMetaRangeColumn*)aData;
         aOrder == 0 && sData != NULL;
         sData   = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = sColVal[ sData->columnIdx ].value;
        sValueInfo1.length = sColVal[ sData->columnIdx ].length;
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        aOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }

    // Column 값이 Min Value값보다 오른쪽에 있거나 같을 경우
    // TRUE 값을 Setting한다.
    //     ------------- Column -------------
    //     |                                |
    //     V                                V
    //  Min Value                        Max Value
    *aResult = ( aOrder >= 0 ) ? ID_TRUE : ID_FALSE ;

    return IDE_SUCCESS;
}

SInt qtc::compareMinimumLimit( mtdValueInfo * /* aValueInfo1 */,
                               mtdValueInfo * /* aValueInfo2 */ )
{
    return 1;
}

SInt qtc::compareMaximumLimit4Mtd( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * /* aValueInfo2 */ )
{
    const void* sValue = mtc::value( aValueInfo1->column,
                                     aValueInfo1->value,
                                     aValueInfo1->flag );

    return (aValueInfo1->column->module->isNull( aValueInfo1->column,
                                                 sValue )
            != ID_TRUE ) ? -1 : 0 ;
}

SInt qtc::compareMaximumLimit4Stored( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * /* aValueInfo2 */ )
{
    if ( ( aValueInfo1->column->module->flag & MTD_VARIABLE_LENGTH_TYPE_MASK )
         == MTD_VARIABLE_LENGTH_TYPE_TRUE )
    {
        return ( aValueInfo1->length != 0 ) ? -1 : 0;
    }
    else
    {
        return ( idlOS::memcmp( aValueInfo1->value,
                                aValueInfo1->column->module->staticNull,
                                aValueInfo1->column->column.size )
                 != 0 ) ? -1 : 0;
    }
}

IDE_RC qtc::rangeMaximumCallBack4Mtd( idBool     * aResult,
                                      const void * aRow,
                                      void       *, /* aDirectKey */
                                      UInt        , /* aDirectKeyPartialSize */
                                      const scGRID /* aRid */,
                                      void       * aData )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta 관리를 위한 Maximum Key Range CallBack으로만 사용된다.
 *    따라서, Disk Variable Column에 대한 고려가 필요없다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt                 sOrder;
    qtcMetaRangeColumn * sData;
    mtdValueInfo         sValueInfo1;
    mtdValueInfo         sValueInfo2;

    sOrder = 0;

    for ( sData = (qtcMetaRangeColumn*)aData ;
          ( sOrder == 0 ) && ( sData != NULL ) ;
          sData = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = aRow;
        sValueInfo1.length = 0; // do not use
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }

    // Column 값이 Max Value값보다 왼쪽에 있거나 같을 경우
    // TRUE 값을 Setting한다.
    //     ------------- Column -------------
    //     |                                |
    //     V                                V
    //  Min Value                        Max Value

    *aResult = ( sOrder <= 0 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC qtc::rangeMaximumCallBack4LEMtd( idBool     * aResult,
                                        const void * aRow,
                                        void       * aDirectKey,
                                        UInt         aDirectKeyPartialSize,
                                        const scGRID aRid,
                                        void       * aData )
{
    return rangeMaximumCallBack4Mtd( aResult, 
                                     aRow, 
                                     aDirectKey,
                                     aDirectKeyPartialSize,
                                     aRid, 
                                     aData );
}

IDE_RC qtc::rangeMaximumCallBack4LTMtd( idBool     * aResult,
                                        const void * aRow,
                                        void       *, /* aDirectKey */
                                        UInt        , /* aDirectKeyPartialSize */
                                        const scGRID /* aRid */,
                                        void       * aData )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta 관리를 위한 Maximum Key Range CallBack으로만 사용된다.
 *    따라서, Disk Variable Column에 대한 고려가 필요없다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt                 sOrder;
    qtcMetaRangeColumn * sData;
    mtdValueInfo         sValueInfo1;
    mtdValueInfo         sValueInfo2;

    sOrder = 0;

    for ( sData = (qtcMetaRangeColumn*)aData ;
          ( sOrder == 0 ) && ( sData != NULL ) ;
          sData = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = aRow;
        sValueInfo1.length = 0; // do not use
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }

    // Column 값이 Max Value값보다 왼쪽에 있을 경우
    // TRUE 값을 Setting한다.
    //     ------------- Column -------------
    //     |                                |
    //     V                                V
    //  Min Value                        Max Value

    *aResult = ( sOrder < 0 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC qtc::rangeMaximumCallBack4Stored( idBool     * aResult,
                                         const void * aColVal,
                                         void       *, /* aDirectKey */
                                         UInt        , /* aDirectKeyPartialSize */
                                         const scGRID /* aRid */,
                                         void       * aData )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta 관리를 위한 Maximum Key Range CallBack으로만 사용된다.
 *    따라서, Disk Variable Column에 대한 고려가 필요없다.
 *
 * Implementation :
 *
 ***********************************************************************/

    const smiValue     * sColVal;
    SInt                 aOrder;
    qtcMetaRangeColumn * sData;
    mtdValueInfo         sValueInfo1;
    mtdValueInfo         sValueInfo2;

    sColVal = (const smiValue*)aColVal;

    for( aOrder  = 0,   sData  = (qtcMetaRangeColumn*)aData;
         aOrder == 0 && sData != NULL;
         sData   = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = sColVal[ sData->columnIdx ].value;
        sValueInfo1.length = sColVal[ sData->columnIdx ].length;
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use 
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        aOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }

    // Column 값이 Max Value값보다 왼쪽에 있거나 같을 경우
    // TRUE 값을 Setting한다.
    //     ------------- Column -------------
    //     |                                |
    //     V                                V
    //  Min Value                        Max Value

    *aResult = ( aOrder <= 0 ) ? ID_TRUE : ID_FALSE ;

    return IDE_SUCCESS;
}

/*
 * PROJ-2433
 * Direct key Index의 direct key와 mtd를 비교하는 range callback 함수
 * - index의 첫번째 컬럼만 direct key로 비교함
 * - partial direct key를 처리하는부분 추가
 */
IDE_RC qtc::rangeMinimumCallBack4IndexKey( idBool     * aResult,
                                           const void * aRow,
                                           void       * aDirectKey,
                                           UInt         aDirectKeyPartialSize,
                                           const scGRID,
                                           void       * aData )
{
    SInt                 sOrder;
    qtcMetaRangeColumn * sData;
    mtdValueInfo         sValueInfo1;
    mtdValueInfo         sValueInfo2;
    mtcColumn            sDummyColumn;

    sOrder = 0;

    for ( sData = (qtcMetaRangeColumn*)aData ;
          ( sOrder == 0 ) && ( sData != NULL ) ;
          sData = sData->next )
    {
        if ( aDirectKey != NULL )
        {
            /*
             * PROJ-2433 Direct Key Index
             * direct key가 NULL 이 아닌경우, 첫번째 컬럼은 direct key와 비교한다.
             *
             * - aDirectKeyPartialSize가 0 이 아닌경우,
             *   partial direct key 이므로 MTD_PARTIAL_KEY_ON 을 flag에 세팅한다.
             *
             * - partial direct key인 경우,
             *   그 결과가 정확한 값이 아니므로 두번째 컬럼 비교없이 바로 종료한다.
             */
            sDummyColumn.column.offset = 0;
            sDummyColumn.module = sData->valueDesc.module; /* BUG-40530 & valgirnd */

            sValueInfo1.column = &sDummyColumn;
            sValueInfo1.value  = aDirectKey;
            sValueInfo1.length = aDirectKeyPartialSize;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            if ( aDirectKeyPartialSize != 0 )
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_ON;
            }
            else
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_OFF;
            }

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );

            if ( aDirectKeyPartialSize != 0 )
            {
                /* partial key 이면,
                 * 다음 컬럼의 비교는의미없다. 바로끝낸다 */
                break;
            }
            else
            {
                aDirectKey = NULL; /* direct key는 한번만 사용됨 */
            }
        }
        else
        {
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aRow;
            sValueInfo1.length = 0;
            sValueInfo1.flag   = MTD_OFFSET_USE;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );
        }
    }

    // Column 값이 Min Value값보다 오른쪽에 있거나 같을 경우
    // TRUE 값을 Setting한다.
    //     ------------- Column -------------
    //     |                                |
    //     V                                V
    //  Min Value                        Max Value
    *aResult = ( sOrder >= 0 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC qtc::rangeMaximumCallBack4IndexKey( idBool     * aResult,
                                           const void * aRow,
                                           void       * aDirectKey,
                                           UInt         aDirectKeyPartialSize,
                                           const scGRID /* aRid */,
                                           void       * aData )
{
    SInt                 sOrder;
    qtcMetaRangeColumn * sData;
    mtdValueInfo         sValueInfo1;
    mtdValueInfo         sValueInfo2;
    mtcColumn            sDummyColumn;

    sOrder = 0;

    for ( sData = (qtcMetaRangeColumn*)aData ;
          ( sOrder == 0 ) && ( sData != NULL ) ;
          sData = sData->next )
    {
        if ( aDirectKey != NULL )
        {
            /*
             * PROJ-2433 Direct Key Index
             * direct key가 NULL 이 아닌경우, 첫번째 컬럼은 direct key와 비교한다.
             *
             * - aDirectKeyPartialSize가 0 이 아닌경우,
             *   partial direct key 이므로 MTD_PARTIAL_KEY_ON 을 flag에 세팅한다.
             *
             * - partial direct key인 경우,
             *   그 결과가 정확한 값이 아니므로 두번째 컬럼 비교없이 바로 종료한다.
             */
            sDummyColumn.column.offset = 0;
            sDummyColumn.module = sData->valueDesc.module; /* BUG-40530 & valgirnd */

            sValueInfo1.column = &sDummyColumn;
            sValueInfo1.value  = aDirectKey;
            sValueInfo1.length = aDirectKeyPartialSize;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            if ( aDirectKeyPartialSize != 0 )
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_ON;
            }
            else
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_OFF;
            }

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );

            if ( aDirectKeyPartialSize != 0 )
            {
                /* partial key 이면,
                 * 다음 컬럼의 비교는의미없다. 바로끝낸다 */
                break;
            }
            else
            {
                aDirectKey = NULL; /* direct key는 한번만 사용됨 */
            }
        }
        else
        {
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aRow;
            sValueInfo1.length = 0; // do not use
            sValueInfo1.flag   = MTD_OFFSET_USE;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0; // do not use
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );
        }
    }

    // Column 값이 Max Value값보다 왼쪽에 있거나 같을 경우
    // TRUE 값을 Setting한다.
    //     ------------- Column -------------
    //     |                                |
    //     V                                V
    //  Min Value                        Max Value

    *aResult = ( sOrder <= 0 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC qtc::initConversionNodeIntermediate( mtcNode** aConversionNode,
                                            mtcNode*  aNode,
                                            void*     aInfo )
{
/***********************************************************************
 *
 * Description :
 *
 *    Conversion Node를 Intermediate Tuple에 생성하고 초기화함
 *    mtcCallBack.initConversionNode 의 함수 포인터에 셋팅됨.
 *    HOST 변수가 없는 경우의 Conversion을 위해 사용한다.
 *    Prepare 시점(P-V-O)에서 사용됨
 *
 *    Ex) int1 = double1
 *
 * Implementation :
 *
 *    Conversion Node를 위한 공간을 할당받고, 대상 Node를 복사함.
 *    Tuple Set에서 INTERMEDIATE TUPLE의 공간을 사용함.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::initConversionNodeIntermediate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcCallBackInfo* sInfo;

    sInfo = (qtcCallBackInfo*)aInfo;

    IDU_LIMITPOINT("qtc::initConversionNodeIntermediate::malloc");
    IDE_TEST(sInfo->memory->alloc( idlOS::align8((UInt)ID_SIZEOF(qtcNode)),
                                   (void**)aConversionNode )
             != IDE_SUCCESS);

    *(qtcNode*)*aConversionNode = *(qtcNode*)aNode;

    // PROJ-1362
    (*aConversionNode)->baseTable = aNode->baseTable;
    (*aConversionNode)->baseColumn = aNode->baseColumn;

    IDE_TEST( qtc::nextColumn( sInfo->memory,
                               (qtcNode*)*aConversionNode,
                               sInfo->tmplate->stmt,
                               sInfo->tmplate,
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               1 )
              != IDE_SUCCESS );

    /* BUG-44526 INTERMEDIATE Tuple Row를 초기화하지 않아서, valgrind warning 발생 */
    // 초기화가 필요함
    setTupleColumnFlag( &(sInfo->tmplate->tmplate.rows[((qtcNode *)*aConversionNode)->node.table]),
                        ID_FALSE,
                        ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::estimateNode( qtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          mtcCallBack* aCallBack )
{
/***********************************************************************
 *
 * Description :
 *    해당 Node의 estimate 를 수행함
 *
 * Implementation :
 *    해당 Node의 estimate를 수행하고
 *    PRIOR Column에 대한 Validation을 수행함.
 *
 ***********************************************************************/
#define IDE_FN "IDE_RC qtc::estimateNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcuSqlSourceInfo      sqlInfo;
    qtcCallBackInfo*      sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    qcStatement*          sStatement = sCallBackInfo->statement;
    qmsSFWGH*             sSFWGHOfCallBack = sCallBackInfo->SFWGH;
    qmsQuerySet         * sQuerySetCallBack = sCallBackInfo->querySet;
    UInt                  sSqlCode;

    IDE_TEST_RAISE( (SInt)(aNode->node.lflag&MTC_NODE_ARGUMENT_COUNT_MASK) >=
                    aRemain,
                    ERR_STACK_OVERFLOW );

    // 해당 Node의 estimate 를 수행함.
    if ( aNode->node.module->estimate( (mtcNode*)aNode,
                                       aTemplate,
                                       aStack,
                                       aRemain,
                                       aCallBack )
         != IDE_SUCCESS )
    {
        // subquery인 경우 QP수준에서 에러를 처리했으므로 skip한다.
        IDE_TEST( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                  == MTC_NODE_OPERATOR_SUBQUERY );

        sqlInfo.setSourceInfo( ((qcTemplate*)aTemplate)->stmt,
                               & aNode->position );
        IDE_RAISE( ERR_PASS );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-39683
    // 인자가 undef type이면 리턴도 undef type이어야 한다.
    if ( ( ( aNode->node.lflag & MTC_NODE_UNDEF_TYPE_MASK )
           == MTC_NODE_UNDEF_TYPE_EXIST ) &&
         ( ( ( aNode->node.module->lflag & MTC_NODE_OPERATOR_MASK )
             == MTC_NODE_OPERATOR_FUNCTION ) ||
           ( ( aNode->node.module->lflag & MTC_NODE_OPERATOR_MASK )
             == MTC_NODE_OPERATOR_AGGREGATION ) ) &&
         ( aStack->column->module->id != MTD_LIST_ID ) )
    {
        IDE_TEST( mtc::initializeColumn( aStack->column,
                                         & mtdUndef,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------------------------------------
    // Hierarchy 구문이 없는 질의에서 PRIOR keyword를 사용할 수 없다.
    // 이에 대한 Validation을 수행함
    //----------------------------------------------------------------
    if (sSFWGHOfCallBack != NULL)
    {
        if ( ( ( aNode->lflag & QTC_NODE_PRIOR_MASK )
               == QTC_NODE_PRIOR_EXIST )
             && ( sSFWGHOfCallBack->hierarchy == NULL ) )
        {
            sqlInfo.setSourceInfo( sStatement,
                                   & aNode->position );
            IDE_RAISE( ERR_PRIOR_WITHOUT_CONNECTBY );
        }
    }
  
    // PROJ-2462 Reuslt Cache
    if ( sQuerySetCallBack != NULL )
    {
        // PROJ-2462 Result Cache
        if ( ( ( aNode->node.lflag & MTC_NODE_VARIABLE_MASK )
               == MTC_NODE_VARIABLE_TRUE ) ||
             ( ( aNode->lflag & QTC_NODE_LOB_COLUMN_MASK )
               == QTC_NODE_LOB_COLUMN_EXIST ) ||
             ( ( aNode->lflag & QTC_NODE_COLUMN_RID_MASK )
               == QTC_NODE_COLUMN_RID_EXIST ) )
        {
            sQuerySetCallBack->lflag &= ~QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
            sQuerySetCallBack->lflag |= QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-2204 Join Update, Delete
    if ( sSFWGHOfCallBack != NULL )
    {
        switch ( sSFWGHOfCallBack->validatePhase )
        {
            case QMS_VALIDATE_FROM:
                IDE_TEST( qmsPreservedTable::addOnCondPredicate( sStatement,
                                                                 sSFWGHOfCallBack,
                                                                 sCallBackInfo->from,
                                                                 aNode )
                          != IDE_SUCCESS );
                break;
 
            case QMS_VALIDATE_WHERE:
                IDE_TEST( qmsPreservedTable::addPredicate( sStatement,
                                                           sSFWGHOfCallBack,
                                                           aNode )
                          != IDE_SUCCESS );
                break;
 
            default:
                break;
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_PRIOR_WITHOUT_CONNECTBY );
    {
        (void)sqlInfo.init(sCallBackInfo->memory);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_PRIOR_WITHOUT_CONNECTBY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_PASS );
    {
        // sqlSourceInfo가 없는 error라면.
        if ( ideHasErrorPosition() == ID_FALSE )
        {
            sSqlCode = ideGetErrorCode();

            (void)sqlInfo.initWithBeforeMessage(sCallBackInfo->memory);

            IDE_SET(
                ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                                sqlInfo.getBeforeErrMessage(),
                                sqlInfo.getErrMessage()));
            (void)sqlInfo.fini();

            // overwrite wrapped errorcode to original error code.
            ideGetErrorMgr()->Stack.LastError = sSqlCode;
        }
        else
        {
            // Nothing to do.
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

// To Fix BUG-11921(A3) 11920(A4)
//
// estimateInternal 에서 arguments에 대해
// recursive하게 estimateInternal 를 하나씩 호출해주기 전에,
// 만약 사용자가 지정하지 않은 기본 인자를 aNode->arguments 에
// 덧붙여 주어야 하는지 판단하고, 필요하다면 디폴트 값들 가리키는
// qtcNode들을 만들어서 aNode->arguments의 맨 끝에 매달도록 한다.

IDE_RC qtc::appendDefaultArguments( qtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack )
{
    qtcNode  * sNode;
    mtcStack * sStack;
    SInt       sRemain;

    if ( aNode->node.module == & qtc::spFunctionCallModule )
    {
        // BUG-41262 PSM overloading
        // PSM 인자의 경우 미리 estimate 를 해야 overloading 이 가능하다.
        // 모든 경우에 미리 처리할수 없으므로 column 만 처리한다.

        // BUG-44518 Stack 을 제대로 설정해야 한다.
        for( sNode  = (qtcNode*)aNode->node.arguments,
             sStack = aStack + 1, sRemain = aRemain - 1;
             sNode != NULL;
             sNode  = (qtcNode*)sNode->node.next, sStack++, sRemain-- )
        {
            if ( sNode->node.module == &qtc::columnModule )
            {
                // PROJ-2533
                // array의 index에 primitive type의 변수만 오는것은 아니기 때문에,
                // arguments에 대한 estimate가 필요하다.
                // e.g. func1( arrayVar[ func2() ] )
                IDE_TEST( estimateInternal( sNode,
                                            aTemplate,
                                            sStack,
                                            sRemain,
                                            aCallBack )
                          != IDE_SUCCESS );
            }
            else
            {
                // nothing to do.
            }
        }

        IDE_TEST( qsv::createExecParseTreeOnCallSpecNode( aNode, aCallBack )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::addLobValueFuncForSP( qtcNode      * aNode,
                                  mtcTemplate  * aTemplate,
                                  mtcStack     * aStack,
                                  SInt        /* aRemain */,
                                  mtcCallBack  * aCallBack )
{
    qcTemplate  * sTemplate = (qcTemplate*) aTemplate;
    qtcNode     * sNewNode[2];
    qtcNode     * sNode;
    qtcNode     * sPrevNode = NULL;
    mtfModule   * sGetLobModule = NULL;
    mtcColumn   * sColumn;
    mtcStack    * sStack;
    mtcStack      sInternalStack[2];

    if ( aNode->node.module == & qtc::spFunctionCallModule )
    {
        for( sNode  = (qtcNode*)aNode->node.arguments, sStack = aStack + 1;
             sNode != NULL;
             sNode  = (qtcNode*)sNode->node.next, sStack++ )
        {
            sColumn = QTC_TMPL_COLUMN( sTemplate, (qtcNode*) sNode );
            
            if ( QTC_TEMPLATE_IS_COLUMN( sTemplate, (qtcNode*) sNode ) == ID_TRUE )
            {
                if ( sColumn->module->id == MTD_BLOB_ID )
                {
                    sGetLobModule = & mtfGetBlobValue;
                }
                else if ( sColumn->module->id == MTD_CLOB_ID )
                {
                    sGetLobModule = & mtfGetClobValue;
                }
                else
                {
                    sGetLobModule = NULL;
                }

                if ( sGetLobModule != NULL )
                {
                    // get_lob_value 함수를 만든다.
                    IDE_TEST( makeNode( sTemplate->stmt,
                                        sNewNode,
                                        & sNode->columnName,
                                        sGetLobModule )
                              != IDE_SUCCESS );
                    
                    // 함수를 연결한다.
                    sNewNode[0]->node.arguments = (mtcNode*)sNode;
                    sNewNode[0]->node.next = sNode->node.next;
                    sNewNode[0]->node.arguments->next = NULL;

                    sNewNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
                    sNewNode[0]->node.lflag |= 1;
                    
                    qtc::dependencySetWithDep( & sNewNode[0]->depInfo,
                                               & sNode->depInfo );

                    // input argument
                    sInternalStack[1] = *sStack;
                    
                    // get_lob_value는 stack을 1개만 사용해야한다.
                    IDE_TEST( estimateNode( sNewNode[0],
                                            aTemplate,
                                            sInternalStack,
                                            2,
                                            aCallBack )
                              != IDE_SUCCESS );

                    // result
                    *sStack = sInternalStack[0];

                    if ( sPrevNode != NULL )
                    {
                        sPrevNode->node.next = (mtcNode*)sNewNode[0];
                    }
                    else
                    {
                        aNode->node.arguments = (mtcNode*)sNewNode[0];
                    }

                    sPrevNode = sNewNode[0];
                    sNode = sNewNode[0];
                }
                else
                {
                    sPrevNode = sNode;
                }
            }
            else
            {
                sPrevNode = sNode;
            }
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::estimateInternal( qtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              mtcCallBack* aCallBack )
{
/***********************************************************************
 *
 * Description :
 *    Node Tree를 estimate한다.
 *
 * Implementation :
 *    Node Tree를 Traverse하면 Node Tree전체를 estimate 한다.
 *    Aggregation 연산자와 같은 특수한 연산자는 그 특징에 맞게 분류한다.
 *    (1) Argument Node들에 대한 estimate 수행
 *    (2) over clause 절에 대한 estimate 수행
 *    (3) 현재 Node에 대한 estimate 수행
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::estimateInternal"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode           * sNode;
    mtcStack          * sStack;
    SInt                sRemain;
    qtcCallBackInfo   * sInfo;
    ULong               sLflag;
    idBool              sIsAggrNode;
    idBool              sIsAnalyticFuncNode;
    UInt                sSqlCode;
    qcuSqlSourceInfo    sSqlInfo;
    qtcNode           * sPassNode;
    mtcNode           * sNextNodePtr;
    mtcNode           * sPassArgNode;
    qtcOverColumn     * sOverColumn;

    // PROJ-2533
    // array(columnModule/member function) 인지
    // proc/func(spFunctionCallModule)인지를 estimate전에 결정 해야한다.
    IDE_TEST( qmvQTC::changeModuleToArray ( aNode,
                                            aCallBack ) 
              != IDE_SUCCESS);

    // Node의 dependencies 초기화
    dependencyClear( & aNode->depInfo );

    // sLflag 초기화
    sLflag = MTC_NODE_BIND_TYPE_TRUE;

    if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_AGGREGATION  )
    {
        sInfo = (qtcCallBackInfo*)aCallBack->info;
        sIsAggrNode = ID_TRUE;
    }
    else
    {
        sInfo = NULL;
        sIsAggrNode = ID_FALSE;
    }

    // BUG-41243 
    // spFunctionCallModule이 아닌 경우, Name-based Argument가 존재하면 안 된다.
    // (e.g. replace(P1=>'P', 'a')를 하면 P1, 'P', 'a' 3개의 argument로 인식)
    IDE_TEST_RAISE( ( aNode->node.module != & qtc::spFunctionCallModule ) &&
                    ( qtc::hasNameArgumentForPSM( aNode ) == ID_TRUE ),
                    NAME_NOTATION_NOT_ALLOWED );

    // To Fix BUG-11921(A3) 11920(A4)
    // Argument Node들에 대한 estimateInternal 수행 전에 Default Argument를 생성한다.
    IDE_TEST( appendDefaultArguments(aNode, aTemplate, aStack,
                                     aRemain, aCallBack   )
              != IDE_SUCCESS );


    // Analytic Function에 속하는 칼럼인지 여부 정보 설정
    if ( aNode->overClause != NULL )
    {
        sIsAnalyticFuncNode = ID_TRUE;
        aNode->lflag &= ~QTC_NODE_ANAL_FUNC_COLUMN_MASK;
        aNode->lflag |= QTC_NODE_ANAL_FUNC_COLUMN_TRUE;
    }
    else
    {
        // analytic function에 속하는 칼럼인 경우
        if ( ( aNode->lflag & QTC_NODE_ANAL_FUNC_COLUMN_MASK )
             == QTC_NODE_ANAL_FUNC_COLUMN_TRUE )
        {
            sIsAnalyticFuncNode = ID_TRUE;
        }
        else
        {   
            sIsAnalyticFuncNode = ID_FALSE;
        }
    }
   
    //-----------------------------------------
    // Argument Node들에 대한 estimateInternal 수행
    //-----------------------------------------

    for( sNode  = (qtcNode*)aNode->node.arguments,
             sStack = aStack + 1, sRemain = aRemain - 1;
         sNode != NULL;
         sNode  = (qtcNode*)sNode->node.next, sStack++, sRemain-- )
    {       
        // BUG-44367
        // prevent thread stack overflow
        IDE_TEST_RAISE( sRemain < 1, ERR_STACK_OVERFLOW );

        /* PROJ-2197 PSM Renewal */
        if( (aNode->lflag & QTC_NODE_COLUMN_CONVERT_MASK)
            == QTC_NODE_COLUMN_CONVERT_TRUE )
        {
            sNode->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
        }
        else
        {
            /* nothing to do */
        }

        if( sIsAnalyticFuncNode == ID_TRUE )
        {
            sNode->lflag &= ~QTC_NODE_ANAL_FUNC_COLUMN_MASK;
            sNode->lflag |= QTC_NODE_ANAL_FUNC_COLUMN_TRUE;
        }
        else
        {
            // nothing to do 
        }

        //-----------------------------------------------------
        // BUG-28223 CASE expr WHEN .. THEN .. 구문의 expr에 subquery 사용시 에러발생
        //  즉, <SIMPLE CASE>의 expr에 subquery 사용시 에러발생.
        // 
        // 아래 코드가 이상해 보이겠지만,
        // branch에서 패치 호환성 문제로
        // 아래 구문을 에러처리할 수 없고 부득이 지원해야 하는 관계로
        // 꼼수처럼 보이는 코드가 추가됨.
        // 
        // 예) case ( select count(*) from dual )
        //     when 0 then '000'
        //     when 1 then '111'
        //     else 'asdf'
        //     위 쿼리는 파싱과정에서 노드복사를 통해 다음과 같은 노드가 만들어진다.
        //     하지만, 현재 구조에서 subQuery는 노드복사를 할 수 없다.
        // 
        //      [ CASE ] <--MTC_NODE_CASE_TYPE_SIMPLE 플래그가 지정됨.
        //          |
        //          |                     / pass node를 달아야 한다는 플래그가 지정됨.
        //          |                    / (QTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_TRUE)
        //          |                   \/
        //         [=]  ---  ['000'] - [=] --- ['111'] -  ['asdf']
        //          |                   |       
        //          |                   |
        //        [subQ] - [0]        [subQ] - [1]
        //
        //     estimate과정에서
        //     CASE노드의 첫번째 = (즉, [subQ]=[0])에서만 subQuery의 estimate를 수행하고,
        //     이후의 = (즉, [subQ]=[1])은 첫번째 subQuery를 PASS node로 연결하고,
        //     subQuery는 중복 estimate되지 않도록 한다.
        //     따라서, subQuery를 복사하지 않고 처리할 수 있다.
        //
        //      [ CASE ]
        //          |
        //         [=]  ---  ['000'] - [=] --- ['111'] -  ['asdf']
        //          |                   |
        //          |                [PASS] - [1]
        //          |-------------------|
        //        [subQ] - [0]      
        //
        //-----------------------------------------------------
        if( ( aNode->node.lflag & MTC_NODE_CASE_TYPE_MASK ) == MTC_NODE_CASE_TYPE_SIMPLE )
        {
            if( ( ( ((qtcNode*)(aNode->node.arguments))->lflag & QTC_NODE_SUBQUERY_MASK )
                == QTC_NODE_SUBQUERY_EXIST )
                &&
                ( ( sNode->node.lflag & MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_MASK )
                  == MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_TRUE ) ) 
            {
                sNextNodePtr = sNode->node.arguments->next;

                sInfo = (qtcCallBackInfo*)aCallBack->info;

                IDE_TEST( qtc::makePassNode(
                              sInfo->statement,
                              NULL,
                              (qtcNode*)(aNode->node.arguments->arguments),
                              &sPassNode ) != IDE_SUCCESS );

                sPassNode->node.lflag &= ~MTC_NODE_INDIRECT_MASK;
                sPassNode->node.lflag |= MTC_NODE_INDIRECT_FALSE;

                // BUG-28446 [valgrind], BUG-38133
                // qtcCalculate_Pass(mtcNode*, mtcStack*, int, void*, mtcTemplate*)
                // (qtcPass.cpp:333)
                // SIMPLE CASE처리를 위해 필요에 의해 생성된 PASS NODE임을 표시
                // skipModule이며 size는 0이다.
                sPassNode->node.lflag &= ~MTC_NODE_CASE_EXPRESSION_PASSNODE_MASK;
                sPassNode->node.lflag |= MTC_NODE_CASE_EXPRESSION_PASSNODE_TRUE;
                
                sPassNode->node.next = sNextNodePtr;
                sNode->node.arguments = (mtcNode*)sPassNode;

                // BUG-44518 order by 구문의 ESTIMATE 중복 수행하면 안됩니다.
                // Alias 가 있을때 fatal 이 발생할수 있습니다.
                // select i1 as i2 , i2 as i1 from t1 order by func1(i1,1);
                // SELECT  i1+i1 AS i1 FROM t1 order by func1(i1,1);
                if ( (sNode->lflag & QTC_NODE_ORDER_BY_ESTIMATE_MASK)
                    == QTC_NODE_ORDER_BY_ESTIMATE_FALSE )
                {
                    IDE_TEST( estimateInternal( sNode,
                                                aTemplate,
                                                sStack,
                                                sRemain,
                                                aCallBack )
                            != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do.
                }
            }
            else
            {
                // BUG-28223 CASE expr WHEN .. THEN .. 구문의 expr에 subquery 사용시 에러발생
                // <SIMPLE CASE>의 expr가 subquery가 아닌 경우,
                // PASS NODE를 만들라는 플래그를 삭제한다.
                sNode->node.lflag &= ~MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_MASK;
                sNode->node.lflag |= MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_FALSE;

                // BUG-44518 order by 구문의 ESTIMATE 중복 수행하면 안됩니다.
                // Alias 가 있을때 fatal 이 발생할수 있습니다.
                // select i1 as i2 , i2 as i1 from t1 order by func1(i1,1);
                // SELECT  i1+i1 AS i1 FROM t1 order by func1(i1,1);
                if ( (sNode->lflag & QTC_NODE_ORDER_BY_ESTIMATE_MASK)
                    == QTC_NODE_ORDER_BY_ESTIMATE_FALSE )
                {
                    IDE_TEST( estimateInternal( sNode,
                                                aTemplate,
                                                sStack,
                                                sRemain,
                                                aCallBack )
                              != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do.
                }
            }
        }
        else
        {
            if( ( ( aNode->node.lflag & MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_MASK )
                == MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_TRUE ) &&
                ( (qtcNode*)(aNode->node.arguments) == sNode ) )
            {
                sPassArgNode =
                    qtc::getCaseSubExpNode( (mtcNode*)(sNode->node.arguments) );

                sStack->column = aTemplate->rows[sPassArgNode->table].columns +
                    sPassArgNode->column;
            }
            else
            {
                /* PSM Array의 Index Node는 bind 변수로 변경하지 않는다.
                 * 왜냐하면 Array그대로 binding 하기 때문이다.
                 * ex) SELECT I1 INTO V1 FROM T1 WHERE I1 = ARR1[INDEX1];
                 *                                          ^^   ^^
                 *  -> SELECT I1         FROM T1 WHERE I1 = ?           ;
                 *     ?는 ARR1[INDEX1] */
                if( ( ( aNode->lflag & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK )
                      == QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST ) &&
                    ( ( sNode->lflag & QTC_NODE_COLUMN_CONVERT_MASK )
                      == QTC_NODE_COLUMN_CONVERT_TRUE ) )
                {
                    sNode->lflag &= ~QTC_NODE_COLUMN_CONVERT_MASK;
                    sNode->lflag |= QTC_NODE_COLUMN_CONVERT_FALSE;
                }

                // BUG-44518 order by 구문의 ESTIMATE 중복 수행하면 안됩니다.
                // Alias 가 있을때 fatal 이 발생할수 있습니다.
                // select i1 as i2 , i2 as i1 from t1 order by func1(i1,1);
                // SELECT  i1+i1 AS i1 FROM t1 order by func1(i1,1);
                if ( (sNode->lflag & QTC_NODE_ORDER_BY_ESTIMATE_MASK)
                    == QTC_NODE_ORDER_BY_ESTIMATE_FALSE )
                {
                    IDE_TEST( estimateInternal( sNode,
                                                aTemplate,
                                                sStack,
                                                sRemain,
                                                aCallBack )
                              != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do.
                }
            }
        }

        //------------------------------------------------------
        // Argument의 정보 중 필요한 정보를 모두 추출한다.
        //    [Index 사용 가능 정보]
        //     aNode->module->mask : 하위 Node중 column이 있을 경우,
        //     하위 노드의 flag은 index를 사용할 수 있음이 Setting되어 있음.
        //     이 때, 연산자 노드의 특성을 의미하는 mask를 이용해 flag을
        //     재생성함으로서 index를 탈 수 있음을 표현할 수 있다.
        //------------------------------------------------------

        aNode->node.lflag |=
            sNode->node.lflag & aNode->node.module->lmask & MTC_NODE_MASK;
        aNode->lflag |= sNode->lflag & QTC_NODE_MASK;

        //------------------------------------------------------
        // PROJ-1492
        // Argument의 bind관련 lflag를 모두 모은다.
        //    1. BIND_TYPE_FALSE가 하나라도 있으면 BIND_TYPE_FALSE가 된다.
        //------------------------------------------------------

        if( ( sNode->node.lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST )
        {
            if( ( sNode->node.lflag & MTC_NODE_BIND_TYPE_MASK ) ==
                MTC_NODE_BIND_TYPE_FALSE )
            {
                sLflag &= ~MTC_NODE_BIND_TYPE_MASK;
                sLflag |= MTC_NODE_BIND_TYPE_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }

        // Argument의 dependencies를 모두 포함한다.
        IDE_TEST( dependencyOr( & aNode->depInfo,
                                & sNode->depInfo,
                                & aNode->depInfo ) != IDE_SUCCESS );
    }

    if( ( aNode->node.lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST )
    {
        if( QTC_IS_TERMINAL( aNode ) == ID_FALSE )
        {
            //------------------------------------------------------
            // PROJ-1492
            // 하위 노드가 있는 해당 Node의 bind관련 lflag를 설정한다.
            //    1. Argument에 BIND_TYPE_FALSE가 하나라도 있으면 BIND_TYPE_FALSE가 된다.
            //       (단, 해당 Node가 CAST함수 노드일 경우 항상 BIND_TYPE_TRUE가 된다.)
            //------------------------------------------------------
            aNode->node.lflag |= sLflag & MTC_NODE_BIND_TYPE_MASK;

            if( aNode->node.module == & mtfCast )
            {
                aNode->node.lflag |= MTC_NODE_BIND_TYPE_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sInfo = (qtcCallBackInfo*)aCallBack->info;
            if ( (sInfo->statement != NULL) &&
                 (sInfo->statement->spvEnv->createProc != NULL) )
            {
                IDE_TEST( qsvProcStmts::makeUsingParam( NULL,
                                                        aNode,
                                                        aCallBack )
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------------
    // func1(c1) 처럼 lob column을 인자로 받는 function들을 처리하기 위하여
    // func1(get_clob_value(c1)) 형식으로 get_clob_value 함수를 붙여넣는다.
    //-----------------------------------------
    
    IDE_TEST( addLobValueFuncForSP( aNode,
                                    aTemplate,
                                    aStack,
                                    aRemain,
                                    aCallBack )
              != IDE_SUCCESS );
    
    //-----------------------------------------
    // 현재 Node에 대한 estimate 수행
    //-----------------------------------------

    IDE_TEST( estimateNode( aNode,
                            aTemplate,
                            aStack,
                            aRemain,
                            aCallBack )
              != IDE_SUCCESS );

    //----------------------------------------------------
    // over 절이 있으면 analytic function 임
    // validation 및 partition by column에 대한 estimate
    //----------------------------------------------------

    if( aNode->overClause != NULL )
    {
        IDE_TEST( estimate4OverClause( aNode,
                                       aTemplate,
                                       aStack,
                                       aRemain,
                                       aCallBack )
                  != IDE_SUCCESS );

        // BUG-27457
        // analytic function이 있음을 설정
        aNode->lflag &= ~QTC_NODE_ANAL_FUNC_MASK;
        aNode->lflag |= QTC_NODE_ANAL_FUNC_EXIST;

        // BUG-41013
        // over 절 안에 사용된 _prowid 도 flag 세팅이 필요하다.
        for ( sOverColumn = aNode->overClause->overColumn;
              sOverColumn != NULL;
              sOverColumn = sOverColumn->next )
        {
            if ( (sOverColumn->node->lflag & QTC_NODE_COLUMN_RID_MASK)
                 == QTC_NODE_COLUMN_RID_EXIST )
            {
                aNode->lflag &= ~QTC_NODE_COLUMN_RID_MASK;
                aNode->lflag |= QTC_NODE_COLUMN_RID_EXIST;

                break;
            }
        }
    }
    else
    {
        if ( ( aNode->node.lflag & MTC_NODE_FUNCTION_ANALYTIC_MASK )
             == MTC_NODE_FUNCTION_ANALYTIC_TRUE )
        {
            sSqlInfo.setSourceInfo( sInfo->statement,
                                    & aNode->position );
            IDE_RAISE( ERR_MISSING_OVER_IN_WINDOW_FUNCTION );
        }
        else
        {
            // Nothing to do.
        }
    }

    // PROJ-1789 PROWID
    // Aggregation 함수에 PROWID 사용 불가
    // BUG-41013 sum(_prowid) + 1 의 경우에 제대로 막지 못하고 있다.
    // 검사하는 위치를 변경함
    if ((aNode->node.lflag & MTC_NODE_OPERATOR_MASK)
        == MTC_NODE_OPERATOR_AGGREGATION)
    {
        if ((aNode->lflag & QTC_NODE_COLUMN_RID_MASK)
            == QTC_NODE_COLUMN_RID_EXIST)
        {
            IDE_RAISE(ERR_PROWID_NOT_SUPPORTED);
        }
    }

    //------------------------------------------------------
    // BUG-16000
    // Column이나 Function의 Type이 Lob or Binary Type이면 flag설정
    //------------------------------------------------------

    aNode->lflag &= ~QTC_NODE_BINARY_MASK;

    if ( isEquiValidType( aNode, aTemplate ) == ID_FALSE )
    {
        aNode->lflag |= QTC_NODE_BINARY_EXIST;
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------------------
    // PROJ-1404
    // variable built-in function을 사용한 경우 설정한다.
    //------------------------------------------------------

    if ( ( aNode->node.lflag & MTC_NODE_VARIABLE_MASK )
         == MTC_NODE_VARIABLE_TRUE )
    {
        aNode->lflag &= ~QTC_NODE_VAR_FUNCTION_MASK;
        aNode->lflag |= QTC_NODE_VAR_FUNCTION_EXIST;
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------------
    // PROJ-1653 Outer Join Operator (+)
    // terminal node 중에서 컬럼노드만이 (+)를 사용할 수 있다.
    //-----------------------------------------
    if ( ( ( aNode->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
           == QTC_NODE_JOIN_OPERATOR_EXIST )
         &&
         ( ( aNode->lflag & QTC_NODE_PRIOR_MASK)  // BUG-34370 prior column은 skip
           == QTC_NODE_PRIOR_ABSENT )
         &&
         ( QTC_IS_TERMINAL(aNode) == ID_TRUE ) )
    {
        if ( QTC_TEMPLATE_IS_COLUMN((qcTemplate*)aTemplate,aNode) == ID_TRUE )
        {
            dependencyAndJoinOperSet( aNode->node.table, & aNode->depInfo );
        }
        else
        {
            sInfo = (qtcCallBackInfo*)aCallBack->info;                

            if ( sInfo != NULL )
            {
                sSqlInfo.setSourceInfo( sInfo->statement,
                                        & aNode->position );
                IDE_RAISE(ERR_NOT_ALLOW_JOIN_OPER_WITH_NON_COLUMN);
            }
            else
            {
                IDE_RAISE(ERR_NOT_ALLOW_JOIN_OPER_WITH_NON_COLUMN2);
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------------
    // 특수한 연산자에 처리
    //-----------------------------------------
    
    if( ((aNode->node.lflag & MTC_NODE_OPERATOR_MASK) ==
         MTC_NODE_OPERATOR_AGGREGATION) &&
        ((aNode->lflag & QTC_NODE_AGGR_ESTIMATE_MASK) ==
         QTC_NODE_AGGR_ESTIMATE_FALSE) &&
        (sIsAggrNode == ID_TRUE) )
    {
        //----------------------------------------------------
        // [ 해당 Node가 Aggregation 연산자인 경우 ]
        //
        // 일반 Aggregation과 Nested Aggregation을 분류한다.
        // 또한, 삼중 중첩 Aggregation은 Validation Error로 걸러낸다.
        //----------------------------------------------------

        IDE_TEST( estimateAggregation( aNode, aCallBack )
                  != IDE_SUCCESS );

    }
    else
    {
        // Nothing To Do
    }

    //------------------------------------------------------
    // [ 일반 연산자인 경우 ]
    // Constant Expression에 대한 선처리를 시도한다.
    //------------------------------------------------------

    sInfo = (qtcCallBackInfo*)aCallBack->info;
    if ( sInfo->statement != NULL )
    {
        // 정상적인 Validation 과정인 경우
        IDE_TEST( preProcessConstExpr( sInfo->statement,
                                       aNode,
                                       aTemplate,
                                       aStack,
                                       aRemain,
                                       aCallBack )
                  != IDE_SUCCESS );

        if( ( aNode->node.lflag & MTC_NODE_REESTIMATE_MASK )
            == MTC_NODE_REESTIMATE_TRUE )
        {
            if ( aNode->node.module->estimate( (mtcNode*)aNode,
                                               aTemplate,
                                               aStack,
                                               aRemain,
                                               aCallBack )
                 != IDE_SUCCESS )
            {
                sSqlInfo.setSourceInfo( sInfo->statement,
                                        & aNode->position );
                IDE_RAISE( ERR_PASS );
            }
            else
            {
                // Nothing To Do
            }

            // PROJ-1413
            // re-estimate를 수행했으므로 다음 estimate를 위해
            // re-estimate를 꺼둔다.
            aNode->node.lflag &= ~MTC_NODE_REESTIMATE_MASK;
            aNode->node.lflag |= MTC_NODE_REESTIMATE_FALSE;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PASS );
    {
        // sqlSourceInfo가 없는 error라면.
        if ( ideHasErrorPosition() == ID_FALSE )
        {
            sSqlCode = ideGetErrorCode();

            (void)sSqlInfo.initWithBeforeMessage(sInfo->memory);
            IDE_SET(
                ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                                sSqlInfo.getBeforeErrMessage(),
                                sSqlInfo.getErrMessage()));
            (void)sSqlInfo.fini();

            // overwrite wrapped errorcode to original error code.
            ideGetErrorMgr()->Stack.LastError = sSqlCode;
        }
        else
        {
            // Nothing to do.
        }
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_JOIN_OPER_WITH_NON_COLUMN );
    {
        (void)sSqlInfo.init(sInfo->memory);

        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOW_JOIN_OPER_WITH_NON_COLUMN,
                                 sSqlInfo.getErrMessage() ));
        (void)sSqlInfo.fini();
    }    
    IDE_EXCEPTION( ERR_NOT_ALLOW_JOIN_OPER_WITH_NON_COLUMN2 );
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOW_JOIN_OPER_WITH_NON_COLUMN,
                                 "" ));
    }
    IDE_EXCEPTION( ERR_MISSING_OVER_IN_WINDOW_FUNCTION );
    {
        (void)sSqlInfo.init(sInfo->memory);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_MISSING_OVER_IN_WINDOW_FUNCTION,
                                sSqlInfo.getErrMessage()));
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION(ERR_PROWID_NOT_SUPPORTED)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_PROWID_NOT_SUPPORTED));
    }
    IDE_EXCEPTION( NAME_NOTATION_NOT_ALLOWED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_PARAM_NAME_NOTATION_NOW_ALLOWED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

idBool
qtc::isEquiValidType( qtcNode     * aNode,
                      mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     BUG-16000
 *     Equal연산이 가능한 타입인지 검사한다.
 *     Column이나 Function의 Type이 Lob or Binary Type이면 ID_FALSE를 반환
 *     (BUG: MT function만 가능, PSM function의 Type은 검사하지 못함)
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qtc::isEquiValidType"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool              sReturn = ID_TRUE;
    mtcNode           * sNode;
    const mtdModule   * sModule;

    if ( (aNode->lflag & QTC_NODE_BINARY_MASK)
         == QTC_NODE_BINARY_EXIST )
    {
        sReturn = ID_FALSE;
    }
    else
    {
        switch ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        {
            case MTC_NODE_OPERATOR_SUBQUERY:
                for ( sNode = aNode->node.arguments;
                      sNode != NULL;
                      sNode = sNode->next )
                {
                    sReturn = isEquiValidType( (qtcNode*) sNode,
                                               aTemplate );

                    if ( sReturn == ID_FALSE )
                    {
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                break;

            case MTC_NODE_OPERATOR_FUNCTION:
                sModule = aTemplate->rows[aNode->node.table].
                    columns[aNode->node.column].module;

                sReturn = mtf::isEquiValidType( sModule );
                break;

            case MTC_NODE_OPERATOR_MISC:
                if ( (aNode->node.module == & qtc::columnModule) ||
                     (aNode->node.module == & qtc::valueModule) )
                {
                    sModule = aTemplate->rows[aNode->node.table].
                        columns[aNode->node.column].module;

                    sReturn = mtf::isEquiValidType( sModule );
                }
                else
                {
                    // Nothing to do.
                }
                break;

            default:
                break;
        }
    }

    return sReturn;

#undef IDE_FN
}

IDE_RC
qtc::registerTupleVariable( qcStatement    * aStatement,
                            qcNamePosition * aPosition )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *               tuple variable을 등록한다.
 *
 * Implementation :
 *     $$로 시작하는 tuple variable만 등록한다.
 *
 **********************************************************************/
#define IDE_FN "qtc::registerTupleVariable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcTupleVarList    * sTupleVariable;

    IDE_DASSERT( QC_IS_NULL_NAME( (*aPosition) ) == ID_FALSE );

    if ( ( aPosition->size > QC_TUPLE_VAR_HEADER_SIZE ) &&
         ( idlOS::strMatch( aPosition->stmtText + aPosition->offset,
                            QC_TUPLE_VAR_HEADER_SIZE,
                            QC_TUPLE_VAR_HEADER,
                            QC_TUPLE_VAR_HEADER_SIZE ) == 0 ) )
    {
        IDU_LIMITPOINT("qtc::registerTupleVariable::malloc");
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qcTupleVarList, & sTupleVariable )
                  != IDE_SUCCESS );

        SET_POSITION( sTupleVariable->name, (*aPosition) );
        sTupleVariable->next = QC_SHARED_TMPLATE(aStatement)->tupleVarList;

        QC_SHARED_TMPLATE(aStatement)->tupleVarList = sTupleVariable;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qtc::estimateAggregation( qtcNode     * aNode,
                          mtcCallBack * aCallBack )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Expression에 대한 estimate 를
 *    수행한다.
 *
 * Implementation :
 *     [해당 Node가 Aggregation 연산자인 경우]
 *     - Aggregation 종류
 *       (1) 일반 Aggregation
 *       (2) Nested Aggregation
 *       (3) Analytic Function 
 *
 ***********************************************************************/

#define IDE_FN "estimateAggregation"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode             * sNode;
    qtcCallBackInfo     * sInfo;
    qmsSFWGH            * sSFWGH;
    qmsAggNode          * sAggNode;
    qmsAnalyticFunc     * sAnalyticFunc;
    qcDepInfo             sResDependencies;
    qcuSqlSourceInfo      sqlInfo;
    idBool                sOuterHavingCase = ID_FALSE;

    // 하위 Node가 Nested Aggregation이면 삼중 중첩이며
    // 이는 Validation Error
    IDE_TEST_RAISE( ( aNode->lflag & QTC_NODE_AGGREGATE2_MASK )
                    == QTC_NODE_AGGREGATE2_EXIST,
                    ERR_INVALID_AGGREGATION );

    // BUG-16000
    // Aggregation 연산은 Lob or Binary Type을 인자로 받을 수 없다.
    // 단, distinct가 없는 count연산은 예외
    if ( ( (aNode->node.lflag & MTC_NODE_DISTINCT_MASK) ==
           MTC_NODE_DISTINCT_FALSE ) &&
         ( ( aNode->node.module == & mtfCount ) ||
           ( aNode->node.module == & mtfCountKeep ) ) )
    {
        /* PROJ-2528 KeepAggregaion
         * Count가 lob을 허용하는것과 같이
         * CountKeep도 첫 번째 Argument는 lob을 허용하지만 두 번째
         * 즉 keep의 order by에 쓰인 lob은 허용할 수 없다.
         * lob은 비교대상이 아니기 때문이다.
         */
        if ( aNode->node.module == &mtfCountKeep )
        {
            /* BUG-49029 */
            if ( aNode->node.arguments != NULL )
            {
                for ( sNode  = (qtcNode*)aNode->node.arguments->next;
                      sNode != NULL;
                      sNode  = (qtcNode*)sNode->node.next )
                {
                    IDE_TEST_RAISE( (sNode->lflag & QTC_NODE_BINARY_MASK) ==
                                    QTC_NODE_BINARY_EXIST,
                                    ERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_DISTINCT );
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        for( sNode  = (qtcNode*)aNode->node.arguments;
             sNode != NULL;
             sNode  = (qtcNode*)sNode->node.next )
        {
            IDE_TEST_RAISE(
                (sNode->lflag & QTC_NODE_BINARY_MASK) ==
                QTC_NODE_BINARY_EXIST,
                ERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_DISTINCT );
        }
    }

    sInfo = (qtcCallBackInfo*)aCallBack->info;
    sSFWGH = sInfo->SFWGH;

    //---------------------------------------------------
    // Argument의 dependencies를 이용해 해당 SFWGH를 찾는다.
    //---------------------------------------------------

    while (sSFWGH != NULL)
    {
        // check dependency
        qtc::dependencyClear( & sResDependencies );
        if( aNode->node.arguments != NULL )
        {
            qtc::dependencyAnd(
                & ((qtcNode *)(aNode->node.arguments))->depInfo,
                & sSFWGH->depInfo,
                & sResDependencies );
        }
        if ( qtc::dependencyEqual( & sResDependencies,
                                   & qtc::zeroDependencies ) == ID_TRUE )
        {
            sSFWGH = sSFWGH->outerQuery;
            continue;
        }
        else
        {
            break;
        }
    }

    if (sSFWGH == NULL)
    {
        // 다음과 같은 경우가 이에 해당한다.
        // SUM( 1 ), COUNT(*)
        // 즉, 정상적인 경우라 할 수 있다.
        sSFWGH = sInfo->SFWGH;
    }

    if (sSFWGH == NULL)
    {
        // order by에 칼럼에 대한 estimate 인 경우
        if ( aNode->overClause == NULL )
        {
            sqlInfo.setSourceInfo( sInfo->statement,
                                   & aNode->position );
            IDE_RAISE(ERR_NOT_ALLOWED_AGGREGATION);
        }
        else
        {
            // BUG-21807
            // analytic function은 aggregate function이 아니며,
            // order by 구문에 올 수 있음
        }
    }
    else
    {
        // outer HAVING, local (TARGET/WHERE)
        if( sSFWGH != sInfo->SFWGH )
        {
            if( sSFWGH->validatePhase == QMS_VALIDATE_HAVING )
            {
                sOuterHavingCase = ID_TRUE;
            }
        }
    }

    //---------------------------------------------------
    // TODO - 버그 수정에 대한 코드 반영해야 함.
    // PR-6353과 관련한 BUG가 존재함.
    //---------------------------------------------------

    if( sOuterHavingCase == ID_TRUE )
    {
        //--------------------------
        // special case for OUTER/HAVING
        //--------------------------
        IDU_LIMITPOINT("qtc::estimateAggregation::malloc1");
        IDE_TEST(STRUCT_ALLOC(sInfo->memory, qmsAggNode, (void**)&sAggNode)
                 != IDE_SUCCESS);

        sAggNode->aggr = aNode;
        
        if( ( aNode->lflag & QTC_NODE_AGGREGATE_MASK )
            == QTC_NODE_AGGREGATE_EXIST )
        {
            sNode = (qtcNode*)aNode->node.arguments;

            if ( sNode != NULL )
            {
                if ( ( sNode->lflag & QTC_NODE_ANAL_FUNC_MASK )
                     == QTC_NODE_ANAL_FUNC_ABSENT )
                {
                    aNode->lflag |= QTC_NODE_AGGREGATE2_EXIST;

                    sAggNode->next           = sInfo->SFWGH->aggsDepth1;
                    sInfo->SFWGH->aggsDepth1 = sAggNode;
                }
                else
                {
                    // BUG-21808
                    // aggregation의 argument로 analytic function이
                    // 올 수 없음
                    // ( aggregation 수행 후에 analytic function이
                    // 수행되기 때문)
                    sqlInfo.setSourceInfo( sInfo->statement,
                            &sNode->position );
                    IDE_RAISE( ERR_INVALID_WINDOW_FUNCTION );
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            aNode->lflag |= QTC_NODE_AGGREGATE_EXIST;

            sAggNode->next     = sSFWGH->aggsDepth1;
            sSFWGH->aggsDepth1 = sAggNode;
            sInfo->SFWGH->outerHavingCase = ID_TRUE;
        }
    }
    else
    {
        if ( aNode->overClause != NULL )
        {
            //--------------------------
            // Analytic Function인 경우 ( PROJ-1762 )
            //--------------------------
            aNode->lflag |= QTC_NODE_AGGREGATE_EXIST;
            
            IDU_LIMITPOINT("qtc::estimateAggregation::malloc2");
            IDE_TEST(STRUCT_ALLOC(sInfo->memory,
                                  qmsAnalyticFunc,
                                  (void**)&sAnalyticFunc)
                     != IDE_SUCCESS);
            
            sAnalyticFunc->analyticFuncNode = aNode;
            sAnalyticFunc->next = sInfo->querySet->analyticFuncList;
            sInfo->querySet->analyticFuncList = sAnalyticFunc;
        }
        else
        {
            //--------------------------
            // normal case
            //--------------------------
            
            IDU_LIMITPOINT("qtc::estimateAggregation::malloc3");
            IDE_TEST(STRUCT_ALLOC(sInfo->memory, qmsAggNode, (void**)&sAggNode)
                     != IDE_SUCCESS);
            
            sAggNode->aggr = aNode;
            
            if( (aNode->lflag & QTC_NODE_AGGREGATE_MASK )
                == QTC_NODE_AGGREGATE_EXIST )
            {
                sNode = (qtcNode*)aNode->node.arguments;
                
                if ( sNode != NULL )
                {
                    if ( ( sNode->lflag & QTC_NODE_ANAL_FUNC_MASK )
                         == QTC_NODE_ANAL_FUNC_ABSENT )
                    {
                        // 하위 Node에 Aggregation이 존재하고,
                        // analytic function의 aggregation이 아닌 경우,
                        // Nested Aggregation임을 설정함.
                        aNode->lflag |= QTC_NODE_AGGREGATE2_EXIST;
                    
                        // set all child aggregation with the indexArgument value of 1.
                        IDE_TEST( setSubAggregation(
                                      (qtcNode*)(sAggNode->aggr->node.arguments) )
                                  != IDE_SUCCESS );
                    
                        sAggNode->next           = sInfo->SFWGH->aggsDepth2;
                        sInfo->SFWGH->aggsDepth2 = sAggNode;
                    }
                    else
                    {
                        // BUG-21808
                        // aggregation의 argument로 analytic function이
                        // 올 수 없음
                        sqlInfo.setSourceInfo( sInfo->statement,
                                               & sNode->position );
                        IDE_RAISE( ERR_INVALID_WINDOW_FUNCTION );
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                //------------------------------------------------------
                // 하위 Node에 Aggregation이 없을 경우,
                // 일반 Aggregation임을 설정함.
                // TODO -
                // 다음과 같이 GROUP BY Column에 대한 Aggregation일 경우,
                // Nested Aggregation으로 설정하여야 한다.
                //
                // Ex)  SELECT SUM(I1), MAX(SUM(I2)) FROM T1 GROUP BY I1;
                //             ^^^^^^^
                //------------------------------------------------------
                aNode->lflag |= QTC_NODE_AGGREGATE_EXIST;
                
                if( sInfo->SFWGH->validatePhase == QMS_VALIDATE_HAVING )
                {
                    aNode->indexArgument = 1;
                }
                
                sAggNode->next           = sInfo->SFWGH->aggsDepth1;
                sInfo->SFWGH->aggsDepth1 = sAggNode;
            }
        }
    }

    /* BUG-35674 */
    aNode->lflag &= ~QTC_NODE_AGGR_ESTIMATE_MASK;
    aNode->lflag |=  QTC_NODE_AGGR_ESTIMATE_TRUE;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_WINDOW_FUNCTION );
    {
        (void)sqlInfo.init(sInfo->memory);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_INVALID_WINDOW_FUNCTION,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_AGGREGATION );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_INVALID_AGGREGATION));
    }
    IDE_EXCEPTION( ERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_DISTINCT );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_DISTINCT));
    }
    IDE_EXCEPTION(ERR_NOT_ALLOWED_AGGREGATION)
    {
        (void)sqlInfo.init(sInfo->memory);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_ALLOWED_AGGREGATION,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qtc::estimate4OverClause( qtcNode     * aNode,
                          mtcTemplate * aTemplate,
                          mtcStack    * aStack,
                          SInt          aRemain,
                          mtcCallBack * aCallBack )
{
/***********************************************************************
 *
 * Description :
 *    Analytic Function Expression에 대한 estimate 를
 *    수행한다.
 *
 * Implementation :
 *    (1) Aggregation 연산 중, sum 연산 만 가능
 *    (2) Partition By Column에 analytic function이 올 수 없음
 *
 ***********************************************************************/

#define IDE_FN "estimate4OverClause"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcCallBackInfo  * sCallBackInfo;
    qcuSqlSourceInfo   sSqlInfo;
    qtcNode          * sCurArgument;
    qtcOverColumn    * sCurOverColumn;
    mtcStack         * sStack;
    SInt               sRemain;

    sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);

    // BUG-27457
    IDE_TEST_RAISE( sCallBackInfo->querySet == NULL,
                    ERR_INVALID_CALLBACK );
    
    // Aggregation만 가능
    if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
         != MTC_NODE_OPERATOR_AGGREGATION )
    {
        sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                & aNode->position );
        
        IDE_RAISE( ERR_NOT_SUPPORTED_FUNCTION );
    }
    else
    {
        // nothing to do 
    }

    // BUG-33663 Ranking Function
    // ranking function은 order by expression이 반드시 있어야 함
    if ( ( ( aNode->node.lflag & MTC_NODE_FUNCTION_RANKING_MASK )
           == MTC_NODE_FUNCTION_RANKING_TRUE )
         &&
         ( aNode->overClause->orderByColumn == NULL ) )
    {
        sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                & aNode->position );
        
        IDE_RAISE( ERR_MISSING_ORDER_IN_WINDOW_FUNCTION );
    }
    else
    {
        // nothing to do
    }

    /* BUG-43087 support ratio_to_report
     */
    if ( aNode->overClause->orderByColumn != NULL )
    {
        if ( ( aNode->node.module == &mtfRatioToReport ) ||
             ( aNode->node.module == &mtfMinKeep ) ||
             ( aNode->node.module == &mtfMaxKeep ) ||
             ( aNode->node.module == &mtfSumKeep ) ||
             ( aNode->node.module == &mtfAvgKeep ) ||
             ( aNode->node.module == &mtfCountKeep ) ||
             ( aNode->node.module == &mtfVarianceKeep ) ||
             ( aNode->node.module == &mtfStddevKeep ) )
        {
            sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                    &aNode->overClause->orderByColumn->node->position );
            IDE_RAISE( ERR_CANNOT_USE_ORDER_BY_CALUSE );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    // Analytic Function은 Target과 Order By 절에만 올 수 있음
    if ( ( sCallBackInfo->querySet->processPhase != QMS_VALIDATE_TARGET ) &&
         ( sCallBackInfo->querySet->processPhase != QMS_VALIDATE_ORDERBY ) )
    {
        sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                & aNode->position );
        
        IDE_RAISE( ERR_INVALID_WINDOW_FUNCTION );
    }
    else
    {
        // nothing to do 
    }

    // Argument Column들에 analytic function이 올수 없음
    for ( sCurArgument = (qtcNode*)aNode->node.arguments;
          sCurArgument != NULL;
          sCurArgument = (qtcNode*)sCurArgument->node.next )
    {
        if ( ( sCurArgument->lflag & QTC_NODE_ANAL_FUNC_MASK )
             == QTC_NODE_ANAL_FUNC_EXIST )
        {
            sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                    & sCurArgument->position );
            IDE_RAISE( ERR_INVALID_WINDOW_FUNCTION );
        }
        else
        {
            // Nothing to do.
        }
    }

    // Partition By column 들에 대한 estimate
    for ( sCurOverColumn = aNode->overClause->overColumn, 
            sStack = aStack + 1, sRemain = aRemain - 1;
          sCurOverColumn != NULL;
          sCurOverColumn = sCurOverColumn->next )
    {
        sCurOverColumn->node->lflag &= ~QTC_NODE_ANAL_FUNC_COLUMN_MASK;
        sCurOverColumn->node->lflag |= QTC_NODE_ANAL_FUNC_COLUMN_TRUE;

        /* BUG-39678
           over절에 사용한 psm 변수를 bind 변수로 치환 */
        sCurOverColumn->node->lflag &= ~QTC_NODE_PROC_VAR_MASK;
        sCurOverColumn->node->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;

        IDE_TEST( estimateInternal( sCurOverColumn->node,
                                    aTemplate,
                                    sStack,
                                    sRemain,
                                    aCallBack )
                  != IDE_SUCCESS );

        /* TASK-7219 Shard Transformer Refactoring */
        aNode->lflag |= sCurOverColumn->node->lflag & QTC_NODE_MASK;

        // BUG-32358 리스트 타입의 사용 여부 확인
        if ( (sCurOverColumn->node->node.lflag & MTC_NODE_OPERATOR_MASK) ==
             MTC_NODE_OPERATOR_LIST )
        {
            sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                   &sCurOverColumn->node->position );
            IDE_RAISE(ERR_INVALID_WINDOW_FUNCTION);
        }
        else
        {
            // Nothing to do.
        }

        // 서브쿼리가 사용 되었을때 타겟 컬럼이 두개이상인지 확인
        if ( ( sCurOverColumn->node->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_SUBQUERY )
        {
            if ( ( sCurOverColumn->node->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 1 )
            {
                sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                        & sCurOverColumn->node->position );
                IDE_RAISE(ERR_INVALID_WINDOW_FUNCTION);
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        // BUG-35670 over 절에 lob, geometry type 사용 불가
        if ( (sCurOverColumn->node->lflag & QTC_NODE_BINARY_MASK) ==
             QTC_NODE_BINARY_EXIST)
        {
            sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                    &sCurOverColumn->node->position );
            IDE_RAISE(ERR_INVALID_WINDOW_FUNCTION);
        }
        else
        {
            // Nothing to do.
        }

        // Partition By Column에 analytic function이 올 수 없음
        if ( ( sCurOverColumn->node->lflag & QTC_NODE_ANAL_FUNC_MASK )
             == QTC_NODE_ANAL_FUNC_EXIST )
        {
            sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                    & sCurOverColumn->node->position );
            IDE_RAISE( ERR_INVALID_WINDOW_FUNCTION );
        }
        else
        {
            // Nothing to do.
        }

        // partition by column의 dependencies를 모두 포함한다.
        IDE_TEST( dependencyOr( & aNode->depInfo,
                                & sCurOverColumn->node->depInfo,
                                & aNode->depInfo )
                  != IDE_SUCCESS );
    }
 
    /* PROJ-1805 Windowing Clause Support
     * window Start Point and End Point estimate
     */
    if ( aNode->overClause->window != NULL )
    {
        if ( ( aNode->node.lflag & MTC_NODE_FUNCTION_WINDOWING_MASK )
             == MTC_NODE_FUNCTION_WINDOWING_FALSE )
        {
            sSqlInfo.setSourceInfo( sCallBackInfo->statement,
                                    &aNode->position );
            IDE_RAISE( ERR_INVALID_WINDOW_CLAUSE_FUNCTION );
        }
        else
        {
            /* Nothing to do */
        }
        if ( aNode->overClause->window->rowsOrRange == QTC_OVER_WINODW_ROWS )
        {
            if ( aNode->overClause->window->start != NULL )
            {
                if ( aNode->overClause->window->start->value != NULL )
                {
                    IDE_TEST_RAISE( aNode->overClause->window->start->value->type
                                    != QTC_OVER_WINDOW_VALUE_TYPE_NUMBER,
                                    ERR_WINDOW_MISMATCHED_VALUE_TYPE );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
            if ( aNode->overClause->window->end != NULL )
            {
                if ( aNode->overClause->window->end->value != NULL )
                {
                    IDE_TEST_RAISE( aNode->overClause->window->end->value->type
                                    != QTC_OVER_WINDOW_VALUE_TYPE_NUMBER,
                                    ERR_WINDOW_MISMATCHED_VALUE_TYPE );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORTED_FUNCTION );
    {
        (void)sSqlInfo.init(sCallBackInfo->memory);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_UNSUPPORTED_FUNCTION,
                                sSqlInfo.getErrMessage()));
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_WINDOW_FUNCTION );
    {
        (void)sSqlInfo.init(sCallBackInfo->memory);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_INVALID_WINDOW_FUNCTION,
                                sSqlInfo.getErrMessage()));
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_MISSING_ORDER_IN_WINDOW_FUNCTION );
    {
        (void)sSqlInfo.init(sCallBackInfo->memory);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_MISSING_ORDER_IN_WINDOW_FUNCTION,
                                sSqlInfo.getErrMessage()));
        (void)sSqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_CALLBACK )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtc::estimate4OverClause",
                                  "Invalid callback" ));
    }
    IDE_EXCEPTION( ERR_INVALID_WINDOW_CLAUSE_FUNCTION )
    {
        (void)sSqlInfo.init(sCallBackInfo->memory);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_WINDOW_CLAUSE_FUNC,
                                sSqlInfo.getErrMessage()));
        (void)sSqlInfo.fini();
    }

    IDE_EXCEPTION( ERR_WINDOW_MISMATCHED_VALUE_TYPE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_MISMATCHED_VALUE_TYPE ) )
    }
    IDE_EXCEPTION( ERR_CANNOT_USE_ORDER_BY_CALUSE );
    {
        (void)sSqlInfo.init(sCallBackInfo->memory);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_CANNOT_USE_ORDER_BY_CLAUSE,
                                sSqlInfo.getErrMessage()));
        (void)sSqlInfo.fini();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeConversionNode( qtcNode*         aNode,
                                qcStatement*     aStatement,
                                qcTemplate*      aTemplate,
                                const mtdModule* aModule )
{
/***********************************************************************
 *
 * Description :
 *    DML등에서 Destine Column의 정보에 맞도록 Value및 Column을
 *    Conversion 한다.
 *    다음과 같은 DML에서 Conversion Node를 만들기 위하여 사용한다.
 *        - INSERT INTO T1(double_1) VALUES ( 1 );
 *                                            ^^
 * Implementation :
 *    적절한 Conversion Node를 생성한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeConversionNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcNode*        sNode;
    mtcNode*        sConversionNode;
    const mtvTable* sTable;
    ULong           sCost;
    mtcStack        sStack[2];

    qtcCallBackInfo sCallBackInfo = {
        aTemplate,               // Template
        QC_QMP_MEM(aStatement),  // Memory 관리자
        NULL,                // Statement
        NULL,                // Query Set
        NULL,                // SFWGH
        NULL                 // From 정보
    };

    mtcCallBack sCallBack = {
        &sCallBackInfo,                 // CallBack 정보
        MTC_ESTIMATE_ARGUMENTS_DISABLE, // Child에 대한 Estimation Disable
        qtc::alloc,                     // Memory 할당 함수
        initConversionNodeIntermediate  // Conversion Node 생성 함수
    };

    for( sNode = &aNode->node;
         ( sNode->lflag&MTC_NODE_INDIRECT_MASK ) == MTC_NODE_INDIRECT_TRUE;
         sNode = sNode->arguments ) ;

    //---------------------------------------------------------
    // 해당 Node와 Column 사이에 Conversion이 필요한지를 검사하고,
    // 필요할 경우, Conversion Node를 생성한다.
    // INSERT INTO T1(double_1) VALUES (3);
    //                ^^^^^^^^          ^^
    //                |                 |
    //               Column 정보       현재 Node
    //
    // PROJ-1492
    // 호스트 변수가 존재하더라도 그 타입을 알 수 있는 경우
    // Conversion Node를 생성한다.
    //---------------------------------------------------------

    sCost = 0;

    sStack[0].column = QTC_TMPL_COLUMN( aTemplate, (qtcNode*)sNode );

    if ( ( ( sStack[0].column->module->id == MTD_BLOB_LOCATOR_ID ) &&
           ( aModule->id == MTD_BLOB_ID ) )
         ||
         ( ( sStack[0].column->module->id == MTD_CLOB_LOCATOR_ID ) &&
           ( aModule->id == MTD_CLOB_ID ) ) )
    {
        /* DML에서 lob_locator를 직접처리하므로
         * lob_locator를 lob type으로 conversion할 필요가 없다.
         *
         * Nothing to do.
         */
    }
    else
    {
        IDE_TEST( mtv::tableByNo( &sTable,
                                  aModule->no,
                                  sStack[0].column->module->no )
                  != IDE_SUCCESS );

        sCost += sTable->cost;

        IDE_TEST_RAISE( sCost >= MTV_COST_INFINITE, ERR_CONVERT );

        //----------------------------------------------
        // Conversion 생성 함수를 호출한다.
        // 위에서 정의한 CallBack 정보를 이용해 Conversion을 생성한다.
        //----------------------------------------------

        IDE_TEST( mtf::makeConversionNode( &sConversionNode,
                                           sNode,
                                           & aTemplate->tmplate,
                                           sStack,
                                           &sCallBack,
                                           sTable )
                  != IDE_SUCCESS );

        //----------------------------------------------
        // Conversion을 연결한다.
        //----------------------------------------------

        if( sNode->conversion == NULL )
        {
            if( sConversionNode != NULL )
            {
                sNode->lflag &= ~(MTC_NODE_INDEX_MASK);
                sNode->lflag |= MTC_NODE_INDEX_UNUSABLE;
            }
            sNode->conversion = sConversionNode;
            sNode->cost += sCost;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

SInt qtc::getCountBitSet( qcDepInfo * aOperand1 )
{
/***********************************************************************
 *
 * Description :
 *    해당 dependencies내에 포함된 dependency의 개수를 리턴한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "SInt qtc::getCountBitSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return aOperand1->depCount;

#undef IDE_FN
}


SInt qtc::getCountJoinOperator( qcDepInfo  * aOperand )
{
/***********************************************************************
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *    해당 dependencies내에 포함된 Outer join Operator 의 개수를 리턴한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt   i;
    SInt   sJoinOperCount = 0;

    for ( i = 0 ; i < aOperand->depCount ; i++ )
    {
        if ( QMO_JOIN_OPER_EXISTS( aOperand->depJoinOper[i] ) == ID_TRUE )
        {
            sJoinOperCount++;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sJoinOperCount;
}

void
qtc::dependencyAndJoinOperSet( UShort      aTupleID,
                               qcDepInfo * aOperand1 )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *
 *    Internal Tuple ID 로 dependencies를 Setting
 *    Set If Outer Join Operator Exists
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    aOperand1->depCount = 1;
    aOperand1->depend[0] = aTupleID;

    aOperand1->depJoinOper[0] = QMO_JOIN_OPER_TRUE;
}


void qtc::dependencyJoinOperAnd( qcDepInfo * aOperand1,
                                 qcDepInfo * aOperand2,
                                 qcDepInfo * aResult )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *
 *    AND of (Dependencies & Join Oper)를 구함
 *    depend 정보와 outer join operator 정보가 모두 같은 것에 대해서만 return.
 *    outer join operator 에서 QMO_JOIN_OPER_TRUE 와 QMO_JOIN_OPER_BOTH 는
 *    같은 것으로 간주한다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    qcDepInfo sResult;

    UInt sLeft;
    UInt sRight;

    sResult.depCount = 0;

    for ( sLeft = 0, sRight = 0;
          ( (sLeft < aOperand1->depCount) && (sRight < aOperand2->depCount) );
        )
    {
        if ( ( aOperand1->depend[sLeft] == aOperand2->depend[sRight] )
          && ( QMO_JOIN_OPER_EQUALS( aOperand1->depJoinOper[sLeft],
                                    aOperand2->depJoinOper[sRight] ) ) )
        {
            sResult.depend[sResult.depCount] = aOperand1->depend[sLeft];
            sResult.depJoinOper[sResult.depCount] = aOperand1->depJoinOper[sLeft];
            sResult.depCount++;

            sLeft++;
            sRight++;
        }
        else
        {
            if ( aOperand1->depend[sLeft] > aOperand2->depend[sRight] )
            {
                sRight++;
            }
            else
            {
                sLeft++;
            }
        }
    }

    qtc::dependencySetWithDep( aResult, & sResult );
}


idBool
qtc::dependencyJoinOperEqual( qcDepInfo * aOperand1,
                              qcDepInfo * aOperand2 )
{
/***********************************************************************
 *
 * PROJ-1653 Outer Join Operator (+)
 *
 * Description :
 *
 *    Dependencies & Outer Join Operator Status 가 모두 정확히
 *    동일한 지를 판단
 *
 * Implementation :
 *    주의할 것은 outer join operator 값도 정확히 같아야한다는 것이다.
 *    QMO_JOIN_OPER_TRUE != QMO_JOIN_OPER_BOTH 이다.
 *    만약, QMO_JOIN_OPER_TRUE 와 QMO_JOIN_OPER_BOTH 를 같은 것으로 간주하려면
 *    별도의 함수를 만들거나 이 함수에서 argument 를 추가하여 수정한다.
 *
 *
 ***********************************************************************/

    UInt   i;
    idBool sRet = ID_TRUE;

    if ( aOperand1->depCount != aOperand2->depCount )
    {
        sRet = ID_FALSE;
    }
    else
    {
        for ( i = 0; i < aOperand1->depCount; i++ )
        {
            if ( ( aOperand1->depend[i] != aOperand2->depend[i] )
              || ( aOperand1->depJoinOper[i] != aOperand2->depJoinOper[i] ) )
            {
                sRet = ID_FALSE;
                break;
            }
            else
            {
                // Nothing to do. 
            }
        }
    }

    return sRet;
}


void qtc::getJoinOperCounter( qcDepInfo  * aOperand1,
                              qcDepInfo  * aOperand2 )
{
/***********************************************************************
 *
 * Description :
 *
 *    dep 정보중 Outer join operator 의 유무가 반대인 dep 를 리턴
 *
 * Implementation :
 *
 ***********************************************************************/

    aOperand1->depCount = 2;

    aOperand1->depend[0] = aOperand2->depend[0];
    aOperand1->depend[1] = aOperand2->depend[1];
    aOperand1->depJoinOper[0] = aOperand2->depJoinOper[1]; // 반대
    aOperand1->depJoinOper[1] = aOperand2->depJoinOper[0]; // 반대
}


idBool
qtc::isOneTableOuterEachOther( qcDepInfo   * aDepInfo )
{
/***********************************************************************
 *
 * Description :
 *
 *    하나의 predicate 에서 하나의 dependency table 이 서로
 *    outer join 되는지 검사
 *    (t1.i1(+) = t1.i2)
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt     i;
    idBool   sRet;

    sRet = ID_FALSE;

    for ( i = 0 ;
          i < aDepInfo->depCount ;
          i++ )
    {
        if ( ( aDepInfo->depJoinOper[i] & QMO_JOIN_OPER_MASK )
                == QMO_JOIN_OPER_BOTH )
        {
            sRet = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sRet;
}


UInt
qtc::getDependTable( qcDepInfo  * aDepInfo,
                     UInt         aIdx )
{
    return aDepInfo->depend[aIdx];
}


UChar
qtc::getDependJoinOper( qcDepInfo  * aDepInfo,
                        UInt         aIdx )
{
    return aDepInfo->depJoinOper[aIdx];
}


SInt qtc::getPosFirstBitSet( qcDepInfo * aOperand1 )
{
/***********************************************************************
 *
 * Description :
 *
 *    Dependencies에 등장하는 최초 Dependency 값을 리턴한다.
 *
 * Implementation :
 *
 *    최초 Dependency 값을 Return하고,
 *    Dependency가 존재하지 않을 경우,
 *    QTC_DEPENDENCIES_END(-1) 를 리턴한다.
 *
 ***********************************************************************/

#define IDE_FN "SInt qtc::getPosFirstBitSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( aOperand1->depCount == 0 )
    {
        return QTC_DEPENDENCIES_END;
    }
    else
    {
        return aOperand1->depend[0];
    }

#undef IDE_FN
}

SInt qtc::getPosNextBitSet( qcDepInfo * aOperand1,
                            UInt   aPos )
{
/***********************************************************************
 *
 * Description :
 *
 *    Dependencies에서 다음 위치에 등장하는 Dependency 값을 리턴한다.
 *
 * Implementation :
 *
 *    현재 위치(aPos)으로부터 다음에 존재하는 Dependency 값을 Return하고,
 *    Dependency가 존재하지 않을 경우,
 *    QTC_DEPENDENCIES_END(-1) 를 리턴한다.
 *
 ***********************************************************************/

#define IDE_FN "SInt qtc::getPosNextBitSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;

    for ( i = 0; i < aOperand1->depCount; i++ )
    {
        if ( (UInt) aOperand1->depend[i] == aPos )
        {
            break;
        }
        else
        {
            // Go Go
        }
    }

    if ( (i + 1) < aOperand1->depCount )
    {
        return aOperand1->depend[i+1];
    }
    else
    {
        return QTC_DEPENDENCIES_END;
    }

#undef IDE_FN
}

IDE_RC qtc::alloc( void* aInfo,
                   UInt  aSize,
                   void** aMemPtr)
{
/***********************************************************************
 *
 * Description :
 *
 *    주어진 Memory 관리자를 이용하여 Memory를 할당한다.
 *
 * Implementation :
 *
 *    CallBack information으로부터 memory 관리자를 획득하고,
 *    이로부터 Memory를 할당한다.
 *
 ***********************************************************************/

#define IDE_FN "void* qtc::alloc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return ((qtcCallBackInfo*)aInfo)->memory->alloc( aSize, aMemPtr);

#undef IDE_FN
}

IDE_RC qtc::getOpenedCursor( mtcTemplate*     aTemplate,
                             UShort           aTableID,
                             smiTableCursor** aCursor,
                             UShort         * aOrgTableID,
                             idBool*          aFound )
{
/***********************************************************************
 *
 * Description :
 *
 *    tableID로 커서정보를 가져온다.
 *
 * Implementation :
 *
 *    qmcCursor에 저장된 커서정보를 tableID로 획득하고,
 *    이로부터 lob-locator를 open한다.
 *
 ***********************************************************************/

#define IDE_FN "void* qtc::getOpenedCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcTemplate* sTemplate;

    sTemplate = (qcTemplate*) aTemplate;

    if( ( aTemplate->rows[aTableID].lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
        == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
    {
        *aOrgTableID = aTemplate->rows[aTableID].partitionTupleID;
    }
    else
    {
        *aOrgTableID = aTableID;
    }

    return ( sTemplate->cursorMgr->getOpenedCursor( *aOrgTableID, aCursor, aFound ) );

#undef IDE_FN
}

IDE_RC qtc::addOpenedLobCursor( mtcTemplate  * aTemplate,
                                smLobLocator   aLocator )
{
/***********************************************************************
 *
 * Description : BUG-40427
 *
 * Implementation :
 *
 *    qmcCursor에 open된 lob locator 1개를 등록한다.
 *    최초로 open된 lob locator 1개만 필요하므로,
 *    이미 등록된 경우 qmcCursor에서는 아무 일도 하지 않는다.
 *
 ***********************************************************************/

#define IDE_FN "void* qtc::addOpenedLobCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcTemplate* sTemplate;

    sTemplate = (qcTemplate*) aTemplate;

    /* BUG-38290
     * addOpenedLobCursor 는 lob locator 를 얻을 때 실행되므로
     * parallel query 의 대상이 아니어서 동시성 문제가 발생하지 않는다.
     */
    return ( sTemplate->cursorMgr->addOpenedLobCursor( aLocator ) );

#undef IDE_FN
}

idBool qtc::isBaseTable( mtcTemplate * aTemplate,
                         UShort        aTable )
{
    qmsFrom  * sFrom;
    idBool     sIsBaseTable = ID_FALSE;

    if ( aTable < aTemplate->rowCount )
    {
        sFrom = ((qcTemplate*)aTemplate)->tableMap[aTable].from;

        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
        if ( sFrom != NULL )
        {
            // BUG-38943 view는 base table이 아니다.
            if ( sFrom->tableRef->view == NULL )
            {
                sIsBaseTable = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return sIsBaseTable;
}

IDE_RC qtc::closeLobLocator( smLobLocator  aLocator )
{
/***********************************************************************
 *
 * Description :
 *
 *    locator를 닫는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "void* qtc::closeLobLocator"

    return qmx::closeLobLocator( NULL, /* idvSQL* */
                                 aLocator );

#undef IDE_FN
}

IDE_RC qtc::nextRow( iduVarMemList * aMemory,
                     qcStatement   * aStatement,
                     qcTemplate    * aTemplate,
                     ULong           aFlag )
{
/***********************************************************************
 *
 * Description :
 *
 *    Tuple Set에서 주어진 Tuple 종류의 다음 Tuple을 할당받는다.
 *
 * Implementation :
 *
 *    aFlag 인자를 이용하여 다음과 같은 Tuple 종류를 판단하고,
 *    아래와 같이 각각의 Tuple에 따라 새로운 Tuple을 할당받는다.
 *        - MTC_TUPLE_TYPE_CONSTANT
 *        - MTC_TUPLE_TYPE_VARIABLE
 *        - MTC_TUPLE_TYPE_INTERMEDIATE
 *    MTC_TUPLE_TYPE_TABLE의 경우, 해당 함수가 호출되지 않는다.
 *
 ***********************************************************************/

    idBool    sFirst;
    UShort    sCurRowID = ID_USHORT_MAX;

    aFlag &= MTC_TUPLE_TYPE_MASK;

    IDE_TEST_RAISE( aTemplate->tmplate.rowCount >= MTC_TUPLE_ROW_MAX_CNT,
                    ERR_TUPLE_SHORTAGE );

    // PROJ-1358 Tuple Set을 자동으로 확장한다.
    if ( aTemplate->tmplate.rowCount >= aTemplate->tmplate.rowArrayCount )
    {
        // 할당된 공간보다 더 큰 공간이 필요한 경우
        // 현재 rowArrayCount 만큼 더 확장한다. (2배로 확장)
        IDE_TEST( increaseInternalTuple( aStatement,
                                         aTemplate->tmplate.rowArrayCount )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // 최초로 해당 Tuple이 할당되는 지를 판단하고,
    // 해당 Tuple종류가 현재 사용하고 있는 위치를 설정한다.

    sFirst = aTemplate->tmplate.currentRow[aFlag] == ID_USHORT_MAX? ID_TRUE : ID_FALSE ;

    aTemplate->tmplate.currentRow[aFlag] = aTemplate->tmplate.rowCount;

    sCurRowID = aTemplate->tmplate.currentRow[aFlag];

    // 해당 Tuple의 정보를 초기화한다.
    /* BUGBUG: columnMaximum이 지나치게 크게 책정되어 있습니다. */
    aTemplate->tmplate.rows[sCurRowID].lflag
        = templateRowFlags[aFlag];
    aTemplate->tmplate.rows[sCurRowID].columnCount   = 0;
    // PROJ-1362  SMI_COLUMN_ID_MAXIMUM이 256으로 줄어서,
    // 256개 이상의 컬럼을 갖는 View생성이 실패할수 있어서
    // SMI_COLUMN_ID_MAXIMUM 대신 1024값을 갖는 별도의 define을 사용.
    aTemplate->tmplate.rows[sCurRowID].columnMaximum = MTC_TUPLE_COLUMN_ID_MAXIMUM;

    // PROJ-1502 PARTITIONED DISK TABLE
    aTemplate->tmplate.rows[sCurRowID].partitionTupleID = sCurRowID;

    // memset for BUG-4953
    switch( aFlag )
    {
        //-------------------------------------------------
        // CONSTANT TUPLE에 대한 Tuple 초기화
        //-------------------------------------------------
        case MTC_TUPLE_TYPE_CONSTANT:
            {
                aTemplate->tmplate.rows[sCurRowID].rowOffset  = 0;
                if( sFirst == ID_TRUE )
                {
                    // 최초 CONSTANT 처리시에는 4096 Bytes 공간을 할당함.
                    aTemplate->tmplate.rows[sCurRowID].rowMaximum
                        = QC_CONSTANT_FIRST_ROW_SIZE;
                }
                else
                {
                    // 이후의 CONSTANT 처리시에는 65536 Bytes 공간을 할당함.
                    aTemplate->tmplate.rows[sCurRowID].rowMaximum
                        = QC_CONSTANT_ROW_SIZE;
                }

                // Column 공간의 할당
                // To fix PR-14793 column 메모리는 초기화 되어야 함.
                IDU_LIMITPOINT("qtc::nextRow::malloc1");
                IDE_TEST( aMemory->cralloc(
                              idlOS::align8((UInt)ID_SIZEOF(mtcColumn))
                              * aTemplate->tmplate.rows[sCurRowID].columnMaximum,
                              (void**)&(aTemplate->tmplate.rows[sCurRowID].columns) )
                          != IDE_SUCCESS);

                // PROJ-1473
                IDE_TEST( qtc::allocAndInitColumnLocateInTuple(
                              aMemory,
                              &(aTemplate->tmplate),
                              sCurRowID )
                          != IDE_SUCCESS );

                // A3에서는 Execute에 대한 공간 할당은 Parsing후에 이루어짐
                //     qci2::fixAfterParsing()
                // 그러나, A4에서는 Constant Expression의 처리를 위해
                // 미리 할당받음
                IDU_LIMITPOINT("qtc::nextRow::malloc2");
                IDE_TEST( aMemory->alloc(
                              ID_SIZEOF(mtcExecute)
                              * aTemplate->tmplate.rows[sCurRowID].columnMaximum,
                              (void**)&(aTemplate->tmplate.rows[sCurRowID].execute) )
                          != IDE_SUCCESS);

                // Row 공간의 할당
                IDU_LIMITPOINT("qtc::nextRow::malloc3");
                IDE_TEST(aMemory->alloc(
                             aTemplate->tmplate.rows[sCurRowID].rowMaximum,
                             (void**)&(aTemplate->tmplate.rows[sCurRowID].row) )
                         != IDE_SUCCESS);

                break;
            }
            //-------------------------------------------------
            // VARIABLE TUPLE에 대한 Tuple 초기화
            //-------------------------------------------------
        case MTC_TUPLE_TYPE_VARIABLE:
            {
                if( sFirst == ID_TRUE  && aTemplate->tmplate.variableRow == ID_USHORT_MAX )
                {
                    // 최초 Binding이 필요한 Tuple의 위치를 저장함.
                    aTemplate->tmplate.variableRow = aTemplate->tmplate.rowCount;
                }
                aTemplate->tmplate.rows[sCurRowID].rowOffset  = 0;
                aTemplate->tmplate.rows[sCurRowID].rowMaximum = 0;

                // Column 공간의 할당
                // To fix PR-14793 column 메모리는 초기화 되어야 함.
                IDU_LIMITPOINT("qtc::nextRow::malloc4");
                IDE_TEST( aMemory->cralloc(
                              idlOS::align8((UInt)ID_SIZEOF(mtcColumn))
                              * aTemplate->tmplate.rows[sCurRowID].columnMaximum,
                              (void**)&(aTemplate->tmplate.rows[sCurRowID].columns) )
                          != IDE_SUCCESS);

                // PROJ-1473
                IDE_TEST( qtc::allocAndInitColumnLocateInTuple(
                              aMemory,
                              &(aTemplate->tmplate),
                              sCurRowID )
                    != IDE_SUCCESS );

                // Execute 공간의 할당
                IDU_LIMITPOINT("qtc::nextRow::malloc5");
                IDE_TEST( aMemory->alloc(
                              ID_SIZEOF(mtcExecute)
                              * aTemplate->tmplate.rows[sCurRowID].columnMaximum,
                              (void**)&(aTemplate->tmplate.rows[sCurRowID].execute) )
                          != IDE_SUCCESS);

                // Row공간은 Binding후에 처리된다.

                break;
            }
            //-------------------------------------------------
            // INTERMEDIATE TUPLE에 대한 Tuple 초기화
            //-------------------------------------------------
        case MTC_TUPLE_TYPE_INTERMEDIATE:
            {
                aTemplate->tmplate.rows[sCurRowID].rowOffset  = 0;
                aTemplate->tmplate.rows[sCurRowID].rowMaximum = 0;

                // Column 공간의 할당
                // To fix PR-14793 column 메모리는 초기화 되어야 함.
                IDU_LIMITPOINT("qtc::nextRow::malloc6");
                IDE_TEST( aMemory->cralloc(
                              idlOS::align8((UInt)ID_SIZEOF(mtcColumn))
                              * aTemplate->tmplate.rows[sCurRowID].columnMaximum,
                              (void**)&(aTemplate->tmplate.rows[sCurRowID].columns) )
                          != IDE_SUCCESS);

                // PROJ-1473
                IDE_TEST( qtc::allocAndInitColumnLocateInTuple(
                              aMemory,
                              &(aTemplate->tmplate),
                              sCurRowID )
                          != IDE_SUCCESS );

                // Execute 공간의 할당
                IDU_LIMITPOINT("qtc::nextRow::malloc7");
                IDE_TEST( aMemory->alloc(
                              ID_SIZEOF(mtcExecute)
                              * aTemplate->tmplate.rows[sCurRowID].columnMaximum,
                              (void**) & (aTemplate->tmplate.rows[sCurRowID].execute))
                          != IDE_SUCCESS);

                // Row 공간은 Validation후에 처리됨
                // qtc::fixAfterValidation()

                break;
            }
        default:
            {
                IDE_RAISE( ERR_NOT_APPLICABLE );
                break;
            }
    } // end of switch

    aTemplate->tmplate.rows[sCurRowID].ridExecute = &gQtcRidExecute;
    aTemplate->tmplate.rowCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TUPLE_SHORTAGE );
    IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_TUPLE_SHORTAGE));

    IDE_EXCEPTION( ERR_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_FATAL_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::nextLargeConstColumn( iduVarMemList * aMemory,
                                  qtcNode       * aNode,
                                  qcStatement   * aStatement,
                                  qcTemplate    * aTemplate,
                                  UInt            aSize )
{
#define IDE_FN "IDE_RC qtc::nextLargeConstColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UShort sCurRowID = ID_USHORT_MAX;

    ULong sFlag = MTC_TUPLE_TYPE_CONSTANT;

    IDE_TEST_RAISE( aTemplate->tmplate.rowCount >= MTC_TUPLE_ROW_MAX_CNT,
                    ERR_TUPLE_SHORTAGE );

    // PROJ-1358 Tuple Set을 자동으로 확장한다.
    if ( aTemplate->tmplate.rowCount >= aTemplate->tmplate.rowArrayCount )
    {
        // 할당된 공간보다 더 큰 공간이 필요한 경우
        // 현재 rowArrayCount 만큼 더 확장한다. (2배로 확장)
        IDE_TEST( increaseInternalTuple( aStatement,
                                         aTemplate->tmplate.rowArrayCount )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    sCurRowID = aTemplate->tmplate.rowCount;

    // 해당 Tuple의 정보를 초기화한다.
    aTemplate->tmplate.rows[sCurRowID].lflag = templateRowFlags[sFlag];
    aTemplate->tmplate.rows[sCurRowID].columnCount   = 0;
    aTemplate->tmplate.rows[sCurRowID].columnMaximum = 1;

    // PROJ-1502 PARTITIONED DISK TABLE
    aTemplate->tmplate.rows[sCurRowID].partitionTupleID = sCurRowID;

    //-------------------------------------------------
    // CONSTANT TUPLE에 대한 Tuple 초기화
    //-------------------------------------------------
    aTemplate->tmplate.rows[sCurRowID].rowOffset  = 0;

    aTemplate->tmplate.rows[sCurRowID].rowMaximum = aSize;

    // Column 공간의 할당
    // To fix PR-14793 column 메모리는 초기화 되어야 함.
    IDU_LIMITPOINT("qtc::nextLargeConstColumn::malloc1");
    IDE_TEST( aMemory->cralloc(
                  idlOS::align8((UInt)ID_SIZEOF(mtcColumn))
                  * aTemplate->tmplate.rows[sCurRowID].columnMaximum,
                  (void**)&(aTemplate->tmplate.rows[sCurRowID].columns) )
              != IDE_SUCCESS);

    // PROJ-1473
    IDE_TEST( qtc::allocAndInitColumnLocateInTuple(
                  aMemory,
                  &(aTemplate->tmplate),
                  sCurRowID )
              != IDE_SUCCESS );
    
    // A3에서는 Execute에 대한 공간 할당은 Parsing후에 이루어짐
    //     qci2::fixAfterParsing()
    // 그러나, A4에서는 Constant Expression의 처리를 위해
    // 미리 할당받음
    IDU_LIMITPOINT("qtc::nextLargeConstColumn::malloc2");
    IDE_TEST( aMemory->cralloc(
                  ID_SIZEOF(mtcExecute)
                  * aTemplate->tmplate.rows[sCurRowID].columnMaximum,
                  (void**)&(aTemplate->tmplate.rows[sCurRowID].execute) )
              != IDE_SUCCESS);

    // Row 공간의 할당
    IDU_LIMITPOINT("qtc::nextLargeConstColumn::malloc3");
    IDE_TEST(aMemory->cralloc(
                 aTemplate->tmplate.rows[sCurRowID].rowMaximum,
                 (void**)&(aTemplate->tmplate.rows[sCurRowID].row) )
             != IDE_SUCCESS);

    // 새로운 Column 공간을 할당받는다.
    aNode->node.table  = sCurRowID;
    aNode->node.column = aTemplate->tmplate.rows[sCurRowID].columnCount;

    aTemplate->tmplate.rows[sCurRowID].columnCount += 1;

    aTemplate->tmplate.rows[aNode->node.table].lflag &=
        ~(MTC_TUPLE_ROW_SKIP_MASK);
    aTemplate->tmplate.rows[aNode->node.table].lflag |=
        MTC_TUPLE_ROW_SKIP_FALSE;

    aTemplate->tmplate.rowCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TUPLE_SHORTAGE );
    IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_TUPLE_SHORTAGE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::nextColumn( iduVarMemList * aMemory,
                        qtcNode       * aNode,
                        qcStatement   * aStatement,
                        qcTemplate    * aTemplate,
                        ULong           aFlag,
                        UInt            aColumns)
{
/***********************************************************************
 *
 * Description :
 *
 *    주어진 Tuple에서 다음 Column을 할당받는다.
 *
 * Implementation :
 *
 *    aFlag 인자를 이용하여 다음과 같은 Tuple 종류를 판단하고,
 *    아래와 같이 각각의 Tuple에 따라 Column을 할당받는다.
 *        - MTC_TUPLE_TYPE_CONSTANT
 *        - MTC_TUPLE_TYPE_VARIABLE
 *        - MTC_TUPLE_TYPE_INTERMEDIATE
 *    MTC_TUPLE_TYPE_TABLE의 경우, 해당 함수가 호출되지 않는다.
 *
 *    해당 Node(aNode)에 Table ID와 Column ID를 부여함으로서,
 *    Column 공간을 할당받는다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::nextColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UShort  sCurrRowID;

    aFlag &= MTC_TUPLE_TYPE_MASK;

    // 아직 해당 Tuple Row가 없는 경우 새로운 Row를 할당받는다.
    if( aTemplate->tmplate.currentRow[aFlag] == ID_USHORT_MAX )
    {
        IDE_TEST( qtc::nextRow( aMemory, aStatement, aTemplate, aFlag )
                  != IDE_SUCCESS );
    }

    sCurrRowID = aTemplate->tmplate.currentRow[aFlag];

    // Column 공간이 부족한 경우, 새로운 Row를 할당받는다.
    if( aTemplate->tmplate.rows[sCurrRowID].columnCount + aColumns
        > aTemplate->tmplate.rows[sCurrRowID].columnMaximum )
    {
        IDE_TEST_RAISE( (aFlag == MTC_TUPLE_TYPE_VARIABLE) &&
                        (aTemplate->tmplate.variableRow ==
                         aTemplate->tmplate.currentRow[aFlag]),
                        ERR_HOST_VAR_LIMIT );
          
        IDE_TEST( qtc::nextRow( aMemory, aStatement, aTemplate, aFlag )
                  != IDE_SUCCESS );

        sCurrRowID = aTemplate->tmplate.currentRow[aFlag];

        IDE_TEST_RAISE( aTemplate->tmplate.rows[sCurrRowID].columnCount
                        + aColumns >
                        aTemplate->tmplate.rows[sCurrRowID].columnMaximum,
                        ERR_TUPLE_SHORTAGE );
    }

    // 새로운 Column 공간을 할당받는다.
    aNode->node.table  = aTemplate->tmplate.currentRow[aFlag];
    aNode->node.column = aTemplate->tmplate.rows[sCurrRowID].columnCount;

    aTemplate->tmplate.rows[sCurrRowID].columnCount += aColumns;

    aTemplate->tmplate.rows[aNode->node.table].lflag &=
        ~(MTC_TUPLE_ROW_SKIP_MASK);
    aTemplate->tmplate.rows[aNode->node.table].lflag |=
        MTC_TUPLE_ROW_SKIP_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TUPLE_SHORTAGE );
    {    
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_TUPLE_SHORTAGE));
    }
    IDE_EXCEPTION( ERR_HOST_VAR_LIMIT );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_HOST_VAR_LIMIT_EXCEED,
                                (SInt)aTemplate->tmplate.rows[sCurrRowID].columnMaximum));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qtc::nextTable( UShort          *aRow,
                       qcStatement     *aStatement,
                       qcmTableInfo    *aTableInfo,
                       idBool           aIsDiskTable,
                       UInt             aNullableFlag ) // PR-13597
{
/***********************************************************************
 *
 * Description :
 *
 *    Table을 위한 Tuple 공간을 할당받는다.
 *
 * Implementation :
 *
 *    일반 Table에 대한 공간일 경우(aTableInfo != NULL),
 *        Meta Cache의 Column 정보를 이용하여 Tuple의 Column 정보를 구축
 *        - Disk Table을 위한 고려가 되어야 함.
 *             : Column 정보를 복사해야 함.
 *             : Tuple에 Disk/Memory인지의 정보를 설정해야 함.
 *    일반 Table이 아닌 경우(aTableInfo == NULL),
 *        Column 정보는 Execution 시점에 결정된다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::nextTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UShort         sCurRowID = ID_USHORT_MAX;
    qcTemplate*    sTemplate = QC_SHARED_TMPLATE(aStatement);
    UShort         i;


    IDE_TEST_RAISE( sTemplate->tmplate.rowCount >= MTC_TUPLE_ROW_MAX_CNT,
                    ERR_TUPLE_SHORTAGE );

    // PROJ-1358 Tuple Set을 자동으로 확장한다.
    if ( sTemplate->tmplate.rowCount >= sTemplate->tmplate.rowArrayCount )
    {
        // 할당된 공간보다 더 큰 공간이 필요한 경우
        // 현재 rowArrayCount 만큼 더 확장한다. (2배로 확장)
        IDE_TEST( increaseInternalTuple( aStatement,
                                         sTemplate->tmplate.rowArrayCount )
                  != IDE_SUCCESS );
    }

    // Tuple ID를 할당한다.
    sTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_TABLE]
        = sCurRowID
        = sTemplate->tmplate.rowCount;

    // PROJ-1502 PARTITIONED DISK TABLE
    sTemplate->tmplate.rows[sCurRowID].partitionTupleID = sCurRowID;

    // PROJ-2205 rownum in DML
    sTemplate->tmplate.rows[sCurRowID].cursorInfo = NULL;

    if( aTableInfo != NULL )
    {
        //--------------------------------------------------------
        // 일반 Table인 경우,
        // Column 정보를 구축하고, Execute를 위한 공간을 할당한다.
        //--------------------------------------------------------

        sTemplate->tmplate.rows[sCurRowID].lflag
            = templateRowFlags[MTC_TUPLE_TYPE_TABLE];
        sTemplate->tmplate.rows[sCurRowID].columnCount
            = sTemplate->tmplate.rows[sCurRowID].columnMaximum
            = aTableInfo->columnCount;

        //------------------------------------------------------------
        // Column 정보의 복사
        // Disk Variable Column을 처리하기 위해서는 Column 정보를 복사해야 함.
        // 참조) Memory Table의 경우, 별도로 복사할 필요가 없으나
        //       Stored Procedure를 위한 Tuple Set 복사를 위해
        //       Tuple Set구조를 지나치게 세분화하는 문제가 발생하게 된다.
        //       ( mtc::cloneTuple(), qtc::templateRowFlags )
        //------------------------------------------------------------

        IDU_LIMITPOINT("qtc::nextTable::malloc1");
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtcColumn)
                                                * sTemplate->tmplate.rows[sCurRowID].columnCount,
                                                (void**) & (sTemplate->tmplate.rows[sCurRowID].columns) )
                 != IDE_SUCCESS );

        for( i = 0; i < sTemplate->tmplate.rows[sCurRowID].columnCount; i++ )
        {
            idlOS::memcpy( (void *) &(sTemplate->tmplate.rows[sCurRowID].columns[i]),
                           (void *) (aTableInfo->columns[i].basicInfo),
                           ID_SIZEOF(mtcColumn) );

            // PR-13597
            if( aNullableFlag == MTC_COLUMN_NOTNULL_FALSE )
            {
                sTemplate->tmplate.rows[sCurRowID].columns[i].flag &=
                    ~(MTC_COLUMN_NOTNULL_MASK);

                sTemplate->tmplate.rows[sCurRowID].columns[i].flag |=
                    (MTC_COLUMN_NOTNULL_FALSE);
            }
            else
            {
                // 그냥 원래의 값으로 둠.
            }

            /* PROJ-2160 */
            if ( (aTableInfo->columns[i].basicInfo)->type.dataTypeId == MTD_GEOMETRY_ID )
            {
                sTemplate->tmplate.rows[sCurRowID].lflag &= ~(MTC_TUPLE_ROW_GEOMETRY_MASK);
                sTemplate->tmplate.rows[sCurRowID].lflag |= (MTC_TUPLE_ROW_GEOMETRY_TRUE);

                /* BUG-44382 clone tuple 성능개선 */
                /* geometry tuple에만 column을 복사한다. */
                if ( ((aTableInfo->columns[i].basicInfo)->column.flag & SMI_COLUMN_TYPE_MASK)
                     == SMI_COLUMN_TYPE_VARIABLE_LARGE )
                {
                    // 복사가 필요함
                    setTupleColumnFlag( &(sTemplate->tmplate.rows[sCurRowID]),
                                        ID_TRUE,
                                        ID_FALSE );
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // nothing todo
            }
        }

        // PROJ-1473
        IDE_TEST( qtc::allocAndInitColumnLocateInTuple(
                      QC_QMP_MEM(aStatement),
                      &(sTemplate->tmplate),
                      sCurRowID )
            != IDE_SUCCESS );

        // Execute 정보의 Setting을 위한 메모리 할당
        IDU_LIMITPOINT("qtc::nextTable::malloc2");
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtcExecute)
                                                * sTemplate->tmplate.rows[sCurRowID].columnCount,
                                                (void**)&(sTemplate->tmplate.rows[sCurRowID].execute))
                 != IDE_SUCCESS);


        //----------------------------------------------
        // [Cursor Flag 의 설정]
        // 접근하는 테이블의 종류에 따라, Ager의 동작이 다르게 된다.
        // 이를 위해 Memory, Disk, Memory와 Disk에 접근하는 지를
        // 정확하게 판단하여야 한다.
        // 질의문에 여러개의 다른 테이블이 존재할 수 있으므로,
        // 해당 Cursor Flag을 ORing하여 그 정보가 누적되게 한다.
        //----------------------------------------------

        // Disk/Memory Table 여부를 설정
        if(ID_TRUE == aIsDiskTable)
        {
            // Disk Table인 경우
            sTemplate->tmplate.rows[sCurRowID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
            sTemplate->tmplate.rows[sCurRowID].lflag |= MTC_TUPLE_STORAGE_DISK;

            // Cursor Flag의 누적
            sTemplate->smiStatementFlag |= SMI_STATEMENT_DISK_CURSOR;
        }
        else
        {
            // Memory Table인 경우
            sTemplate->tmplate.rows[sCurRowID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
            sTemplate->tmplate.rows[sCurRowID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

            // Cursor Flag의 누적
            sTemplate->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
        }
    }
    else
    {
        //--------------------------------------------------------
        // 임시 Table 영역으로,
        // Execution 시점에 모든 정보가 결정된다.
        //--------------------------------------------------------
        sTemplate->tmplate.rows[sCurRowID].lflag = 0;
        sTemplate->tmplate.rows[sCurRowID].lflag &= ~MTC_TUPLE_TYPE_MASK;
        sTemplate->tmplate.rows[sCurRowID].lflag |= MTC_TUPLE_TYPE_TABLE;
        sTemplate->tmplate.rows[sCurRowID].lflag &= ~MTC_TUPLE_ROW_SKIP_MASK;
        sTemplate->tmplate.rows[sCurRowID].lflag |= MTC_TUPLE_ROW_SKIP_TRUE;

        sTemplate->tmplate.rows[sCurRowID].columnCount
            = sTemplate->tmplate.rows[sCurRowID].columnMaximum
            = 0;
        /* BUG-46023 fix */
        sTemplate->tmplate.rows[sCurRowID].columns = NULL;
        sTemplate->tmplate.rows[sCurRowID].columnLocate = NULL;
        sTemplate->tmplate.rows[sCurRowID].execute = NULL;
    }

    // PROJ-1789 PROWID
    sTemplate->tmplate.rows[sCurRowID].ridExecute = &gQtcRidExecute;

    // set out param
    *aRow = sTemplate->tmplate.rowCount;
    // rowCount 증가시킴
    sTemplate->tmplate.rowCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TUPLE_SHORTAGE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_TUPLE_SHORTAGE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qtc::increaseInternalTuple( qcStatement* aStatement,
                            UShort       aIncreaseCount )
{
/***********************************************************************
 *
 * Description :
 *
 *     PROJ-1358
 *     Internal Tuple Set을 확장한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qtc::increaseInternalTuple"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt       sNewArrayCnt;
    UInt       sOldArrayCnt;
    mtcTuple * sNewTuple;
    mtcTuple * sOldTuple;

    qcTableMap * sNewTableMap;
    qcTableMap * sOldTableMap;
    qcTemplate * sTemplate = NULL;
    
    //---------------------------------------------
    // 적합성 검사
    //---------------------------------------------

    // BUG-45666 DDL 많은 수의 partition table에 대한 fk constraint 생성 시
    // execution 단계에서의 tuple set이 필요.
    if ( QC_SHARED_TMPLATE(aStatement) == NULL )
    {
        if ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK )
             == QCI_STMT_MASK_DDL )
        {
            sTemplate = QC_PRIVATE_TMPLATE(aStatement);
        }
        else
        {
            // BUG-21627
            // execution시에 tuple 확장은 plan을 변경시키므로 허용하지 않는다.
            IDE_RAISE( ERR_TUPLE_SHORTAGE );
        }
    }
    else
    {
        sTemplate = QC_SHARED_TMPLATE(aStatement);
    }
    
    IDE_DASSERT( sTemplate->tmplate.rowCount <=
                 sTemplate->tmplate.rowArrayCount );

    //---------------------------------------------
    // 새로운 Internal Tuple의 공간 확보
    //---------------------------------------------

    // To Fix PR-12659
    // Internal Tuple Set은 allocStatement() 시점에 무조건 할당받지 않고,
    // 필요에 의하여 할당받도록 한다.

    if ( aIncreaseCount == 0 )
    {
        // 최초 할당 받는 경우
        sNewArrayCnt = MTC_TUPLE_ROW_INIT_CNT;
    }
    else
    {
        // BUG-21627
        // aIncreaseCount만큼 확장함
        sNewArrayCnt = (UInt)sTemplate->tmplate.rowArrayCount + (UInt)aIncreaseCount;
    }

    /* BUG-48299 */
    if ( sNewArrayCnt == MTC_TUPLE_ROW_MAX_CNT + 1 )
    {
        sNewArrayCnt = MTC_TUPLE_ROW_MAX_CNT;
    }

    IDE_TEST_RAISE( sNewArrayCnt > MTC_TUPLE_ROW_MAX_CNT, ERR_TUPLE_SHORTAGE );

    IDU_LIMITPOINT("qtc::increaseInternalTuple::malloc1");
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtcTuple) * sNewArrayCnt,
                                             (void**) & sNewTuple )
              != IDE_SUCCESS );

    //---------------------------------------------
    // Internal Tuple의 정보 설정
    // 기존의 공간은 재사용하지 않음.
    //---------------------------------------------

    sOldArrayCnt = sTemplate->tmplate.rowArrayCount;

    if ( sOldArrayCnt == 0 )
    {
        // To Fix PR-12659
        // 최초 할당받는 경우로 이전 Internal Tuple Set이 존재하지 않음.
    }
    else
    {
        sOldTuple = sTemplate->tmplate.rows;
        
        // 기존 Internal Tuple 정보의 복사
        idlOS::memcpy( sNewTuple,
                       sOldTuple,
                       ID_SIZEOF(mtcTuple) * sOldArrayCnt );
        
        // 이전 tuple은 해제한다.
        IDE_TEST( QC_QMP_MEM(aStatement)->free( (void*) sOldTuple )
                  != IDE_SUCCESS );
    }

    sTemplate->tmplate.rowArrayCount = sNewArrayCnt;
    sTemplate->tmplate.rows = sNewTuple;

    //---------------------------------------------
    // 새로운 Table Map의 공간 확보
    //---------------------------------------------

    IDU_LIMITPOINT("qtc::increaseInternalTuple::malloc2");
    IDE_TEST( QC_QMP_MEM(aStatement)->cralloc(
                  ID_SIZEOF(qcTableMap) * sNewArrayCnt,
                  (void**) & sNewTableMap )
              != IDE_SUCCESS );

    if ( sOldArrayCnt == 0 )
    {
        // To Fix PR-12659
        // 최초 할당받는 경우로 이전 Internal Tuple Set이 존재하지 않음.
    }
    else
    {
        sOldTableMap = sTemplate->tableMap;

        // 기존 Table Map 정보의 복사
        idlOS::memcpy( sNewTableMap,
                       sOldTableMap,
                       ID_SIZEOF(qcTableMap) * sOldArrayCnt );

        // 이전 tableMap은 해제한다.
        IDE_TEST( QC_QMP_MEM(aStatement)->free( (void*) sOldTableMap )
                  != IDE_SUCCESS );
    }

    sTemplate->tableMap = sNewTableMap;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TUPLE_SHORTAGE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_TUPLE_SHORTAGE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::smiCallBack( idBool       * aResult,
                         const void   * aRow,
                         void         *, /* aDirectKey */
                         UInt          , /* aDirectKeyPartialSize */
                         const scGRID   aRid,  /* BUG-16318 */
                         void         * aData )
{
/***********************************************************************
 *
 * Description :
 *
 *    Storage Manager에 의해 호출되는 Filter를 위한 CallBack 함수
 *    CallBack Filter의 기본 단위이며, smiCallBackAnd와 함께
 *    여러 개의 CallBack을 구축 가능하다.
 *    Filter가 하나만 있을 경우, 호출된다.
 *
 *    BUG-16318
 *    Lob지원 함수의 경우 Locator를 열기위해 rid를 필요로 하므로
 *    aRid가 추가되었다.
 *
 * Implementation :
 *
 *    Filter의 수행 결과를 리턴한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::smiCallBack"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcSmiCallBackData* sData = (qtcSmiCallBackData*)aData;
    void*               sValueTemp;

    /* BUGBUG: const void* to void* */
    sData->table->row = (void*)aRow;
    sData->table->rid = aRid;
    sData->table->modify++;

    // BUG-33674
    IDE_TEST_RAISE( sData->tmplate->stackRemain < 1, ERR_STACK_OVERFLOW );
    
    IDE_TEST( sData->calculate( sData->node,
                                sData->tmplate->stack,
                                sData->tmplate->stackRemain,
                                sData->calculateInfo,
                                sData->tmplate )
              != IDE_SUCCESS );

    /* PROJ-2180
       qp 에서는 mtc::value 를 사용한다.
       다만 여기에서는 성능을 위해서 valueForModule 를 사용한다. */
    sValueTemp = (void*)mtd::valueForModule( (smiColumn*)sData->column,
                                             sData->tuple->row,
                                             MTD_OFFSET_USE,
                                             sData->column->module->staticNull );

    // BUG-34321 return value optimization
    return sData->isTrue( aResult,
                          sData->column,
                          sValueTemp );

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::smiCallBackAnd( idBool       * aResult,
                            const void   * aRow,
                            void         *, /* aDirectKey */
                            UInt          , /* aDirectKeyPartialSize */
                            const scGRID   aRid,  /* BUG-16318 */
                            void         * aData )
{
/***********************************************************************
 *
 * Description :
 *
 *    Storage Manager에 의해 호출되는 Filter를 위한 CallBack 함수
 *    2개 이상의 Filter가 있을 경우, 사용된다.
 *
 *    BUG-16318
 *    Lob지원 함수의 경우 Locator를 열기위해 rid를 필요로 하므로
 *    aRid가 추가되었다.
 *
 * Implementation :
 *
 *    단위 Filter의 수행 결과가 TRUE일 경우,
 *    다음 Filter를 수행한다.
 *    A4에서는 3개의 Filter가 조합될 수 있다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::smiCallBack"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcSmiCallBackDataAnd* sData = (qtcSmiCallBackDataAnd*)aData;

    // 첫 번째 Filter 수행
    IDE_TEST( smiCallBack( aResult,
                           aRow,
                           NULL,
                           0,
                           aRid,
                           sData->argu1 )
              != IDE_SUCCESS );

    if (*aResult == ID_TRUE && sData->argu2 != NULL )
    {
        // 두 번째 Filter 수행
        IDE_TEST( smiCallBack( aResult,
                               aRow,
                               NULL,
                               0,
                               aRid,
                               sData->argu2 )
                  != IDE_SUCCESS );

        if ( *aResult == ID_TRUE && sData->argu3 != NULL )
        {
            // 세 번째 Filter 수행
            IDE_TEST( smiCallBack( aResult,
                                   aRow,
                                   NULL,
                                   0,
                                   aRid,
                                   sData->argu3 )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

void qtc::setSmiCallBack( qtcSmiCallBackData* aData,
                          qtcNode*            aNode,
                          qcTemplate*         aTemplate,
                          UInt                aTable )
{
/***********************************************************************
 *
 * Description :
 *
 *    Storage Manager에서 사용할 수 있도록 CallBack Data를 구성함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::setSmiCallBack"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aData->node          = (mtcNode*)aNode;
    aData->tmplate       = &aTemplate->tmplate;
    aData->table         = aData->tmplate->rows + aTable;
    aData->tuple         = aData->tmplate->rows + aNode->node.table;
    aData->calculate     = aData->tuple->execute[aNode->node.column].calculate;
    aData->calculateInfo =
        aData->tuple->execute[aNode->node.column].calculateInfo;
    aData->column        = aData->tuple->columns + aNode->node.column;
    aData->isTrue        = aData->column->module->isTrue;

#undef IDE_FN
}

void qtc::setSmiCallBackAnd( qtcSmiCallBackDataAnd* aData,
                             qtcSmiCallBackData*    aArgu1,
                             qtcSmiCallBackData*    aArgu2,
                             qtcSmiCallBackData*    aArgu3 )
{
/***********************************************************************
 *
 * Description :
 *
 *    Storage Manager에서 사용할 수 있도록
 *    여러 개의 CallBack Data를 조합함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::setSmiCallBack"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aData->argu1 = aArgu1;
    aData->argu2 = aArgu2;
    aData->argu3 = aArgu3;

#undef IDE_FN
}

void qtc::setCharValue( mtdCharType* aValue,
                        mtcColumn*   aColumn,
                        SChar*       aString )
{
/***********************************************************************
 *
 * Description :
 *
 *    주어진 String으로부터 CHAR type value를 생성함.
 *
 * Implementation :
 *    Data가 없는 부분은 ' '로 canonize한다.
 *
 ***********************************************************************/

#define IDE_FN "void qtc::setCharValue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aValue->length = aColumn->precision;
    idlOS::memset( aValue->value, 0x20, aValue->length );
    idlOS::memcpy( aValue->value, aString, idlOS::strlen( (char*)aString ) );

#undef IDE_FN
}

void qtc::setVarcharValue( mtdCharType* aValue,
                           mtcColumn*,
                           SChar*       aString,
                           SInt         aLength )
{
/***********************************************************************
 *
 * Description :
 *
 *    주어진 String으로부터 VARCHAR type value를 생성함.
 *
 * Implementation :
 *
 *    실제 길이만큼만 관리한다.
 *
 ***********************************************************************/

#define IDE_FN "void qtc::setCharValue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( aLength > 0 )
    {
        aValue->length = aLength;
    }
    else
    {
        aValue->length = idlOS::strlen( (char*)aString );
    }

    idlOS::memcpy( aValue->value, aString, aValue->length );

#undef IDE_FN
}

void qtc::initializeMetaRange( smiRange * aRange,
                               UInt       aCompareType )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta Key Range를 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

    aRange->prev = aRange->next = NULL;

    if ( aCompareType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ||
         aCompareType == MTD_COMPARE_MTDVAL_MTDVAL )
    {
        aRange->minimum.callback = rangeMinimumCallBack4Mtd;
        aRange->maximum.callback = rangeMaximumCallBack4Mtd;
    }
    else 
    {
        if ( ( aCompareType == MTD_COMPARE_STOREDVAL_MTDVAL ) ||
             ( aCompareType == MTD_COMPARE_STOREDVAL_STOREDVAL ) )
        {
            aRange->minimum.callback = rangeMinimumCallBack4Stored;
            aRange->maximum.callback = rangeMaximumCallBack4Stored;
        }
        else
        {
            /* PROJ-2433 */
            aRange->minimum.callback = rangeMinimumCallBack4IndexKey;
            aRange->maximum.callback = rangeMaximumCallBack4IndexKey;
        }
    }

    aRange->minimum.data        = NULL;
    aRange->maximum.data        = NULL;
}

void qtc::setMetaRangeIsNullColumn(qtcMetaRangeColumn* aRangeColumn,
                                   const mtcColumn*    aColumnDesc,
                                   UInt                aColumnIdx )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta를 위한Is null Key Range 정보를 구성
 *
 * Implementation :
 *    Column 정보와 Value 정보를 이용하여 Key Range 정보를 구성
 *    즉, 하나의 Column에 대한 key range 정보가 구성된다.
 *
 ***********************************************************************/
    
    aRangeColumn->next                    = NULL;

    if ( ( aColumnDesc->column.flag & SMI_COLUMN_STORAGE_MASK )
         == SMI_COLUMN_STORAGE_DISK )
    {
        aRangeColumn->compare = qtc::compareMaximumLimit4Stored;
    }
    else
    {
        aRangeColumn->compare = qtc::compareMaximumLimit4Mtd;
    }
    
    aRangeColumn->columnIdx               = aColumnIdx;
    aRangeColumn->columnDesc              = *aColumnDesc;
//    aRangeColumn->valueDesc               = NULL;
    aRangeColumn->value                   = NULL;

}

void qtc::setMetaRangeColumn( qtcMetaRangeColumn* aRangeColumn,
                              const mtcColumn*    aColumnDesc,
                              const void*         aValue,
                              UInt                aOrder,
                              UInt                aColumnIdx )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta를 위한 Key Range 정보를 구성
 *
 * Implementation :
 *    Column 정보와 Value 정보를 이용하여 Key Range 정보를 구성
 *    즉, 하나의 Column에 대한 key range 정보가 구성된다.
 *
 ***********************************************************************/
    UInt    sCompareType;

    // PROJ-1872
    // index가 있는 칼럼에 meta range를 쓰게 되며
    // disk index column의 compare는 stored type과 mt type 간의 비교이다.
    if( ( aColumnDesc->column.flag & SMI_COLUMN_STORAGE_MASK )
        == SMI_COLUMN_STORAGE_DISK )
    {
        sCompareType = MTD_COMPARE_STOREDVAL_MTDVAL;
    }
    else
    {
        if( (aColumnDesc->column.flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_FIXED )
        {
            sCompareType = MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL;
        }
        else
        {
            IDE_DASSERT( ( (aColumnDesc->column.flag & SMI_COLUMN_TYPE_MASK)
                           == SMI_COLUMN_TYPE_VARIABLE ) ||
                         ( (aColumnDesc->column.flag & SMI_COLUMN_TYPE_MASK)
                           == SMI_COLUMN_TYPE_VARIABLE_LARGE ) );

            sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
        }
    }

    aRangeColumn->next                    = NULL;
    aRangeColumn->compare                 =
        (aOrder == SMI_COLUMN_ORDER_ASCENDING) ?
        aColumnDesc->module->keyCompare[sCompareType]
                                       [MTD_COMPARE_ASCENDING] :
        aColumnDesc->module->keyCompare[sCompareType]
                                       [MTD_COMPARE_DESCENDING];

    aRangeColumn->columnIdx               = aColumnIdx;
    MTC_COPY_COLUMN_DESC( &(aRangeColumn->columnDesc), aColumnDesc );
    MTC_COPY_COLUMN_DESC( &(aRangeColumn->valueDesc), aColumnDesc );
    aRangeColumn->valueDesc.column.offset = 0;
    aRangeColumn->valueDesc.column.flag &= ~SMI_COLUMN_TYPE_MASK;
    aRangeColumn->valueDesc.column.flag |= SMI_COLUMN_TYPE_FIXED;
    aRangeColumn->value                   = aValue;
}

void qtc::changeMetaRangeColumn( qtcMetaRangeColumn* aRangeColumn,
                                 const mtcColumn*    aColumnDesc,
                                 UInt                aColumnIdx )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *
 *    Meta를 위한 Key Range 정보를 변경
 *    Key Range의 column정보를 각각의 파티션에 맞는 컬럼으로 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    aRangeColumn->columnIdx               = aColumnIdx;
    MTC_COPY_COLUMN_DESC( &(aRangeColumn->columnDesc), aColumnDesc );
    MTC_COPY_COLUMN_DESC( &(aRangeColumn->valueDesc), aColumnDesc );
    aRangeColumn->valueDesc.column.offset = 0;
    aRangeColumn->valueDesc.column.flag &= ~SMI_COLUMN_TYPE_MASK;
    aRangeColumn->valueDesc.column.flag |= SMI_COLUMN_TYPE_FIXED;
}


void qtc::addMetaRangeColumn( smiRange*           aRange,
                              qtcMetaRangeColumn* aRangeColumn )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta를 위한 하나의 Key Range정보를 smiRange에 등록함
 *
 * Implementation :
 *
 *    Meta에 대한 KeyRange는 '='에 대한 Range만이 존재하며,
 *    Key Range구간이 여러개 존재하는 Multiple Key Range를 사용하지 않는다.
 *    따라서, Minimum Range와 Maximum Range는 동일하다는 가정하에서,
 *    Range를 연결하게 된다.
 *
 *    최초 Column에 대한 Range는 minimum/maximum range로,
 *    다음 Column에 대한 Range는 뒤로 연결한다.
 *
 *    두 개의 Range가 연결된 경우의 구성은 다음과 같다
 *    ex) A[user_id = 'abc'] and B[table_name = 't1']
 *
 *        smiRange->min-------          ->max
 *                           |             |
 *                           V             V
 *                        [A Range] --> [B Range]
 *
 *        max range의 경우, 다음 Range의 추가를 위해 관리되며,
 *        Range의 추가가 더 이상 없을 경우, qtc::fixMetaRange()의 호출을
 *        통해, min과 동일한 위치를 가리키게 된다.
 *
 ***********************************************************************/

#define IDE_FN "void qtc::addMetaRangeColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if( aRange->minimum.data == NULL )
    {

        aRange->minimum.data =
            aRange->maximum.data = aRangeColumn;
    }
    else
    {
        ((qtcMetaRangeColumn*)aRange->maximum.data)->next = aRangeColumn;
        aRange->maximum.data                              = aRangeColumn;
    }

#undef IDE_FN
}

void qtc::fixMetaRange( smiRange* aRange )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta를 위한 smiRange의 Key Range 구성을 완료함.
 *
 * Implementation :
 *
 *    qtc::addMetaRangeColumn()에서 살펴 보았듯이, maximum 정보는
 *    Range의 추가를 위해 가장 마지막 Range를 관리하고 있다.
 *    이를 최초 위치로 돌려 Key Range 구성을 완료하게 된다.
 *
 ***********************************************************************/

#define IDE_FN "void qtc::fixMetaRange"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aRange->maximum.data = aRange->minimum.data;

#undef IDE_FN
}

IDE_RC qtc::fixAfterParsing( qcTemplate    * aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    Parsing이 완료된 후, Tuple Set을 위한 처리를 함
 *
 * Implementation :
 *
 *    CONSTANT TUPLE의 Execute를 위한 공간을 할당받는다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::fixAfterParsing"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_VARIABLE] = ID_USHORT_MAX;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qtc::createColumn( qcStatement*    aStatement,
                          qcNamePosition* aPosition,
                          mtcColumn**     aColumn,
                          UInt            aArguments,
                          qcNamePosition* aPrecision,
                          qcNamePosition* aScale,
                          SInt            aScaleSign,
                          idBool          aIsForSP )
{
/***********************************************************************
 *
 * Description :
 *
 *    CREATE TABLE 구문 등과 같이 COLUMN TYPE의 정의를 이용하여
 *    Column 정보를 생성한다.
 *
 * Implementation :
 *
 *    Column 정의를 위한 공간을 할당받고,
 *    Column Type의 data type 모듈을 찾아, precision 및 scale을 설정한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::createColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool            sIsTimeStamp = ID_FALSE;
    SInt              sPrecision;
    SInt              sScale;
    const mtdModule * sModule;

    // BUG-27034 Code Sonar
    IDE_TEST_RAISE( (aPosition->offset < 0) || (aPosition->size < 0),
                    ERR_EMPTY_DATATYPE_POSITION );

    if ( aArguments == 0 )
    {
        if ( idlOS::strMatch( aPosition->stmtText + aPosition->offset,
                              aPosition->size,
                              "TIMESTAMP",
                              9) == 0 )
        {
            sIsTimeStamp = ID_TRUE;
        }
    }

    if ( sIsTimeStamp == ID_FALSE )
    {
        // BUG-16233
        IDU_LIMITPOINT("qtc::createColumn::malloc1");
        IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, aColumn)
                 != IDE_SUCCESS);

        // initialize : mtc::initializeColumn()에서 arguements로 초기화됨
        (*aColumn)->flag = 0;

        // find mtdModule
        // mtdModule이 mtdNumber Type인 경우,
        // arguement에 따라 mtdFloat 또는 mtdNummeric으로 초기화 해야함
        IDE_TEST(
            mtd::moduleByName( & sModule,
                               (void*)(aPosition->stmtText+aPosition->offset),
                               aPosition->size )
            != IDE_SUCCESS );

        if ( sModule->id == MTD_NUMBER_ID )
        {
            if ( aArguments == 0 )
            {
                IDE_TEST( mtd::moduleByName( & sModule, (void*)"FLOAT", 5 )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( mtd::moduleByName( & sModule, (void*)"NUMERIC", 7 )
                          != IDE_SUCCESS );
            }
        }
        else if ( (sModule->id == MTD_BLOB_ID) ||
                  (sModule->id == MTD_CLOB_ID) )
        {
            // PROJ-1362
            // clob에 arguments는 항상 0이어야 한다.
            // 그러나 estimate시에는 1도 허용하기 때문에
            // 고의로 에러를 내기 위해 2로 바꾼다.
            if ( aArguments != 0 )
            {
                aArguments = 2;
            }
            else
            {
                if ( aIsForSP != ID_TRUE )
                {
                    /* BUG-36429 LOB Column을 생성하는 경우, Precision을 0으로 지정한다. */
                    aArguments = 1;
                }
                else
                {
                    /* BUG-36429 LOB Value를 생성하는 경우, Precision을 프라퍼티 값으로 지정한다. */
                }
            }
        }
        else if ( (sModule->id == MTD_ECHAR_ID) ||
                  (sModule->id == MTD_EVARCHAR_ID) )
        {
            // PROJ-2002 Column Security
            // create시 echar, evarchar를 명시할 수 없다.
            IDE_RAISE( ERR_INVALID_ENCRYPTION_DATATYPE );
        }
        else
        {
            // nothing to do
        }

        if ( aPrecision != NULL )
        {
            IDE_TEST_RAISE( (aPrecision->offset < 0) || (aPrecision->size < 0),
                            ERR_EMPTY_PRECISION_POSITION );
            
            sPrecision = (SInt)idlOS::strToUInt(
                (UChar*)(aPrecision->stmtText+aPrecision->offset),
                aPrecision->size );
        }
        else
        {
            sPrecision = 0;
        }

        if ( aScale != NULL )
        {
            IDE_TEST_RAISE( (aScale->offset < 0) || (aScale->size < 0),
                            ERR_EMPTY_SCALE_POSITION );
            
            sScale = (SInt)idlOS::strToUInt(
                (UChar*)(aScale->stmtText+aScale->offset),
                aScale->size ) * aScaleSign;
        }
        else
        {
            sScale = 0;
        }

        IDE_TEST( mtc::initializeColumn(
                      *aColumn,
                      sModule,
                      aArguments,
                      sPrecision,
                      sScale )
                  != IDE_SUCCESS );
    }
    else
    {
        // in case of TIMESTAMP
        IDE_TEST( qtc::createColumn4TimeStamp( aStatement, aColumn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ENCRYPTION_DATATYPE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_INVALID_ENCRYPTION_DATATYPE));
    }
    IDE_EXCEPTION( ERR_EMPTY_DATATYPE_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtc::createColumn",
                                  "Nameposition of data type is empty" ));
    }
    IDE_EXCEPTION( ERR_EMPTY_PRECISION_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtc::createColumn",
                                  "Nameposition of precision is empty" ));
    }
    IDE_EXCEPTION( ERR_EMPTY_SCALE_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtc::createColumn",
                                  "Nameposition of scale is empty" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::createColumn( qcStatement*    aStatement,
                          mtdModule  *    aModule,
                          mtcColumn**     aColumn,
                          UInt            aArguments,
                          qcNamePosition* aPrecision,
                          qcNamePosition* aScale,
                          SInt            aScaleSign )
{
/***********************************************************************
 *
 * Description :
 *
 *    주어진 Column의 data type module을 이용하여
 *    Column 정보를 생성한다.
 *
 * Implementation :
 *
 *    Column 정의를 위한 공간을 할당받고,
 *    주어진 module 정보를 설정하고, precision 및 scale을 설정한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::createColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SInt sPrecision;
    SInt sScale;

    // BUG-16233
    IDU_LIMITPOINT("qtc::createColumn::malloc2");
    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, aColumn)
             != IDE_SUCCESS);

    // initialize : mtc::initializeColumn()에서 arguements로 초기화됨
    (*aColumn)->flag = 0;

    // PROJ-1362
    // clob에 arguments는 항상 0이어야 한다.
    // 그러나 estimate시에는 1도 허용하기 때문에
    // 고의로 에러를 내기 위해 2로 바꾼다.
    if ( (aModule->id == MTD_BLOB_ID) ||
         (aModule->id == MTD_CLOB_ID) )
    {
        if ( aArguments != 0 )
        {
            aArguments = 2;
        }
        else
        {
            // Nothing to do.
        }
    }
    else if ( (aModule->id == MTD_ECHAR_ID) ||
              (aModule->id == MTD_EVARCHAR_ID) )
    {
        // PROJ-2002 Column Security
        // create시 echar, evarchar를 명시할 수 없다.
        IDE_RAISE( ERR_INVALID_ENCRYPTION_DATATYPE );
    }
    else
    {
        // Nothing to do.
    }

    sPrecision = ( aPrecision != NULL ) ?
        (SInt)idlOS::strToUInt( (UChar*)(aPrecision->stmtText+aPrecision->offset),
                                aPrecision->size ) : 0 ;

    sScale     = ( aScale     != NULL ) ?
        (SInt)idlOS::strToUInt( (UChar*)(aScale->stmtText+aScale->offset),
                                aScale->size ) * aScaleSign : 0 ;

    IDE_TEST( mtc::initializeColumn(
                  *aColumn,
                  aModule,
                  aArguments,
                  sPrecision,
                  sScale )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ENCRYPTION_DATATYPE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_INVALID_ENCRYPTION_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::createColumn4TimeStamp( qcStatement*    aStatement,
                                    mtcColumn**     aColumn )
{
#define IDE_FN "IDE_RC qtc::createColumn4TimeStamp"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // make BYTE ( 8 ) for TIMESTAMP

    SInt sPrecision;
    SInt sScale;

    // BUG-16233
    IDU_LIMITPOINT("qtc::createColumn4TimeStamp::malloc");
    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, aColumn)
             != IDE_SUCCESS);

    // initialize : mtc::initializeColum()에서 arguements로 초기화됨
    //(*aColumn)->flag = 0;

    // precision, scale 설정
    sPrecision = QC_BYTE_PRECISION_FOR_TIMESTAMP;
    sScale     = 0;

    // aColumn의 초기화
    // : dataType은 byte, language는  session의 language로 설정
    IDE_TEST( mtc::initializeColumn(
                  *aColumn,
                  MTD_BYTE_ID,
                  1,
                  sPrecision,
                  sScale )
              != IDE_SUCCESS );

    (*aColumn)->flag &= ~MTC_COLUMN_TIMESTAMP_MASK;
    (*aColumn)->flag |= MTC_COLUMN_TIMESTAMP_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::changeColumn4SRID( qcStatement     * /*aStatement*/,
                               qdExtColumnAttr * aColumnAttr,
                               mtcColumn       * aColumn )
{
    IDE_ASSERT( aColumn     != NULL );
    IDE_ASSERT( aColumnAttr != NULL );

    // srid는 geometry만 지원함
    IDE_TEST_RAISE( aColumn->module->id != MTD_GEOMETRY_ID,
                    ERR_INVALID_SRID_DATATYPE );

    // srid를 저장
    aColumn->mColumnAttr.mSridAttr.mSrid = aColumnAttr->mSrid;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SRID_DATATYPE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_SRID_DATATYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::changeColumn4Encrypt( qcStatement     * aStatement,
                                  qdExtColumnAttr * aColumnAttr,
                                  mtcColumn       * aColumn )
{
    const mtdModule * sModule;
    SChar             sPolicy[MTC_POLICY_NAME_SIZE + 1] = { 0, };
    idBool            sIsExist;
    idBool            sIsSalt;
    idBool            sIsEncodeECC;
    UInt              sEncryptedSize;
    UInt              sECCSize;
    qcuSqlSourceInfo  sqlInfo;

    IDE_ASSERT( aColumn     != NULL );
    IDE_ASSERT( aColumnAttr != NULL );

    // encryption type은 char, varchar만 지원함
    IDE_TEST_RAISE( ( aColumn->module->id != MTD_CHAR_ID ) &&
                    ( aColumn->module->id != MTD_VARCHAR_ID ),
                    ERR_INVALID_ENCRYPTION_DATATYPE );

    // policy name 길이 검사
    if ( aColumnAttr->mPolicyPosition.size > MTC_POLICY_NAME_SIZE )
    {
        sqlInfo.setSourceInfo( aStatement, & aColumnAttr->mPolicyPosition );
        IDE_RAISE( ERR_TOO_LONG_POLICY_NAME );
    }
    else
    {
        // Nothing to do.
    }
    
    if ( aColumnAttr->mPolicyPosition.size == 0 )
    {
        IDE_RAISE( ERR_NOT_EXIST_POLICY );
    }
    else
    {
        // Nothing to do.
    }
    
    if ( aColumn->module->id == MTD_CHAR_ID )
    {
        IDE_TEST( mtd::moduleById( & sModule, MTD_ECHAR_ID )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mtd::moduleById( & sModule, MTD_EVARCHAR_ID )
                  != IDE_SUCCESS );
    }

    // policy 검사
    QC_STR_COPY( sPolicy, aColumnAttr->mPolicyPosition );

    IDE_TEST( qcsModule::getPolicyInfo( sPolicy,
                                        & sIsExist,
                                        & sIsSalt,
                                        & sIsEncodeECC )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsExist == ID_FALSE,
                    ERR_NOT_EXIST_POLICY );

    IDE_TEST_RAISE( sIsEncodeECC == ID_TRUE,
                    ERR_INVALID_POLICY );

    // aColumn의 초기화
    IDE_TEST( mtc::initializeColumn(
                  aColumn,
                  sModule,
                  1,
                  aColumn->precision,  // 0
                  0 )
              != IDE_SUCCESS );
    
    // encryted size 검사
    IDE_TEST( qcsModule::getEncryptedSize( sPolicy,
                                           aColumn->precision,  // 1
                                           & sEncryptedSize )
              != IDE_SUCCESS );

    IDE_TEST( qcsModule::getECCSize( aColumn->precision,  // 1
                                     & sECCSize )
              != IDE_SUCCESS );

    // aColumn의 두번째 초기화
    IDE_TEST( mtc::initializeEncryptColumn(
                  aColumn,
                  sPolicy,
                  sEncryptedSize,
                  sECCSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ENCRYPTION_DATATYPE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_INVALID_ENCRYPTION_DATATYPE));
    }
    IDE_EXCEPTION( ERR_TOO_LONG_POLICY_NAME );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_TOO_LONG_POLICY_NAME,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_POLICY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NOT_EXIST_POLICY, sPolicy));
    }
    IDE_EXCEPTION( ERR_INVALID_POLICY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_POLICY, sPolicy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::changeColumn4Decrypt( mtcColumn    * aColumn )
{
#define IDE_FN "IDE_RC qtc::changeColumn4Decrypt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule   * sModule;
    qcuSqlSourceInfo    sqlInfo;

    IDE_ASSERT( aColumn != NULL );

    // encryption type은 char, varchar만 지원함
    IDE_TEST_RAISE( ( aColumn->module->id != MTD_ECHAR_ID ) &&
                    ( aColumn->module->id != MTD_EVARCHAR_ID ),
                    ERR_INVALID_ENCRYPTION_DATATYPE );

    if ( aColumn->module->id == MTD_ECHAR_ID )
    {
        IDE_TEST( mtd::moduleById( & sModule, MTD_CHAR_ID )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mtd::moduleById( & sModule, MTD_VARCHAR_ID )
                  != IDE_SUCCESS );
    }

    // aColumn의 초기화
    IDE_TEST( mtc::initializeColumn(
                  aColumn,
                  sModule,
                  1,
                  aColumn->precision,
                  0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ENCRYPTION_DATATYPE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_INVALID_ENCRYPTION_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::changeColumn4Decrypt( mtcColumn    * aSrcColumn,
                                  mtcColumn    * aDestColumn )
{
/***********************************************************************
 *
 * Description : PROJ-1584 DML Return Clause
 *               DestColumn : Plain Column
 *               SrcColumn  : 암호화 Column
 *
 *               암호화 Column을 Decrypt 하여 initializeColumn 수행.
 * Implementation :
 *
 **********************************************************************/
    const mtdModule   * sModule;

    IDE_ASSERT( aSrcColumn != NULL );

    // encryption type은 char, varchar만 지원함
    IDE_TEST_RAISE( ( aSrcColumn->module->id != MTD_ECHAR_ID ) &&
                    ( aSrcColumn->module->id != MTD_EVARCHAR_ID ),
                    ERR_INVALID_ENCRYPTION_DATATYPE );

    if ( aSrcColumn->module->id == MTD_ECHAR_ID )
    {
        IDE_TEST( mtd::moduleById( & sModule, MTD_CHAR_ID )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mtd::moduleById( & sModule, MTD_VARCHAR_ID )
                  != IDE_SUCCESS );
    }

    // aColumn의 초기화
    IDE_TEST( mtc::initializeColumn(
                  aDestColumn,
                  sModule,
                  1,
                  aSrcColumn->precision,
                  0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ENCRYPTION_DATATYPE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_INVALID_ENCRYPTION_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::addOrArgument( qcStatement* aStatement,
                           qtcNode**    aNode,
                           qtcNode**    aArgument1,
                           qtcNode**    aArgument2 )
{
/***********************************************************************
 *
 * Description :
 *
 *    Parsing 단계에서 OR keyword를 만났을 때, 해당 Node를 생성한다.
 *
 * Implementation :
 *
 *    두 개의 argument를 OR Node의 argument로 연결한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::addOrArgument"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    extern mtfModule mtfOr;

    if( ( aArgument1[0]->node.lflag &
          ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK )
            ) == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_OR ) )
    {
        // OR가 중첩된 경우, 기존의 OR Node를 사용한다.
        // 다음과 같은 연결 정보가 구성된다.
        //
        //       aNode[0]                            aNode[1]
        //         |                                    |
        //         V                                    |
        //       [OR Node]                              |
        //         |                                    |
        //         V                                    V
        //       [Argument]---->[Argument]-- ... -->[Argument2]
        //
        // 위와 같이 aNode[1]에 다음 argument를 추가하여
        // 중첩 OR를 처리한다.

        aNode[0] = aArgument1[0];
        aNode[1] = aArgument1[1];
        IDE_TEST( qtc::addArgument( aNode, aArgument2 ) != IDE_SUCCESS );
    }
    else
    {
        // 새로운 OR Node를 생성한다.
        IDU_LIMITPOINT("qtc::addOrArgument::malloc");
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
                 != IDE_SUCCESS);
        // OR Node 초기화
        QTC_NODE_INIT( aNode[0] );        

        aNode[1] = NULL;

        IDE_TEST_RAISE( aNode[0] == NULL, ERR_MEMORY_SHORTAGE );

        // OR Node 정보 설정
        aNode[0]->node.lflag          = mtfOr.lflag
                                      & ~MTC_NODE_COLUMN_COUNT_MASK;
        aNode[0]->position.stmtText   = aArgument1[0]->position.stmtText;
        aNode[0]->position.offset     = aArgument1[0]->position.offset;
        aNode[0]->position.size       = aArgument2[0]->position.offset
                                        + aArgument2[0]->position.size
                                        - aArgument1[0]->position.offset;

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   mtfOr.lflag & MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );

        aNode[0]->node.module = &mtfOr;

        // Argument를 연결한다.
        IDE_TEST( qtc::addArgument( aNode, aArgument1 ) != IDE_SUCCESS );
        IDE_TEST( qtc::addArgument( aNode, aArgument2 ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_SHORTAGE );
    IDE_SET(ideSetErrorCode(mtERR_FATAL_MEMORY_SHORTAGE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::addAndArgument( qcStatement* aStatement,
                            qtcNode**    aNode,
                            qtcNode**    aArgument1,
                            qtcNode**    aArgument2 )
{
/***********************************************************************
 *
 * Description :
 *
 *    Parsing 단계에서 AND keyword를 만났을 때, 해당 Node를 생성한다.
 *
 * Implementation :
 *
 *    두 개의 argument를 OR Node의 argument로 연결한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::addAndArgument"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    extern mtfModule mtfAnd;

    if( ( aArgument1[0]->node.lflag &
          ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK )
            ) == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
    {
        // AND가 중첩된 경우, 기존의 AND Node를 사용한다.
        // 다음과 같은 연결 정보가 구성된다.
        //
        //       aNode[0]                            aNode[1]
        //         |                                    |
        //         V                                    |
        //       [AND Node]                              |
        //         |                                    |
        //         V                                    V
        //       [Argument]---->[Argument]-- ... -->[Argument2]
        //
        // 위와 같이 aNode[1]에 다음 argument를 추가하여
        // 중첩 AND를 처리한다.

        aNode[0] = aArgument1[0];
        aNode[1] = aArgument1[1];
        IDE_TEST( qtc::addArgument( aNode, aArgument2 ) != IDE_SUCCESS );
    }
    else
    {
        // 새로운 AND Node를 생성한다.
        IDU_LIMITPOINT("qtc::addAndArgument::malloc");
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
                 != IDE_SUCCESS);

        // AND Node 초기화
        QTC_NODE_INIT( aNode[0] );
        aNode[1] = NULL;

        IDE_TEST_RAISE( aNode[0] == NULL, ERR_MEMORY_SHORTAGE );

        // AND Node 정보 설정
        aNode[0]->node.lflag          = mtfAnd.lflag
                                      & ~MTC_NODE_COLUMN_COUNT_MASK;
        aNode[0]->position.stmtText   = aArgument1[0]->position.stmtText;
        aNode[0]->position.offset     = aArgument1[0]->position.offset;
        aNode[0]->position.size       = aArgument2[0]->position.offset
                                        + aArgument2[0]->position.size
                                        - aArgument1[0]->position.offset;

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   mtfAnd.lflag & MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );

        aNode[0]->node.module = &mtfAnd;

        // Argument를 연결한다.
        IDE_TEST( qtc::addArgument( aNode, aArgument1 ) != IDE_SUCCESS );
        IDE_TEST( qtc::addArgument( aNode, aArgument2 ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_SHORTAGE );
    IDE_SET(ideSetErrorCode(mtERR_FATAL_MEMORY_SHORTAGE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::lnnvlNode( qcStatement  * aStatement,
                       qtcNode      * aNode )
{
/***********************************************************************
 *
 * Description :
 *     BUG-36125
 *     LNNVL함수를 주어진 predicate에 적용시킨다.
 *
 * Implementation :
 *     논리 연산자는 notNode에서와 동일하게 counter operator로 변환하고,
 *     그 외 각 predicatea들에는 LNNVL을 씌운다.
 *     ex) i1 < 0 AND i1 > 1 => LNNVL(i1 <0) OR LNNVL(i1 > 1)
 *
 *     BUG-41294
 *     ROWNUM Predicate이 Copy해 간 qtcNodeTree와 구분되어야 하므로
 *     논리 연산자라도 복사 후 counter operator로 변경해야 한다.
 *
 *     [ 참고 ]
 *     이 함수의 caller는 2개가 있는데, 각자 estimate를 수행한다.
 *     - qtc::notNode() => Parsing Phase에서 estimate
 *     - qmoDnfMgr::makeDnfNotNormal=> lnnvlNode() 직후 estimate
 *
 ***********************************************************************/

    qtcNode*            sNode;
    qcuSqlSourceInfo    sqlInfo;

    if( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
          == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        // 논리연산자
        if( aNode->node.module->counter != NULL )
        {
            IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                       aNode,
                                       aStatement,
                                       QC_SHARED_TMPLATE(aStatement),
                                       MTC_TUPLE_TYPE_INTERMEDIATE,
                                       1 )
                      != IDE_SUCCESS );

            aNode->node.module = aNode->node.module->counter;

            aNode->node.lflag &= ~MTC_NODE_OPERATOR_MASK;
            aNode->node.lflag |= aNode->node.module->lflag & MTC_NODE_OPERATOR_MASK;
            aNode->node.lflag |= aNode->node.module->lflag & MTC_NODE_PRINT_FMT_MASK;

            aNode->node.lflag &= ~MTC_NODE_GROUP_MASK;
            aNode->node.lflag |= aNode->node.module->lflag & MTC_NODE_GROUP_MASK;

            for( sNode  = (qtcNode*)aNode->node.arguments;
                 sNode != NULL;
                 sNode  = (qtcNode*)sNode->node.next )
            {
                IDE_TEST( lnnvlNode( aStatement, sNode ) != IDE_SUCCESS );
            }
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement, &aNode->position );
            IDE_RAISE( ERR_INVALID_ARGUMENT_TYPE );
        }
    }
    else
    {
        // 비교 연산자
        // sNode를 할당하여 aNode를 복사하고, aNode에는 LNNVL을 생성한다.

        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sNode)
                 != IDE_SUCCESS);

        idlOS::memcpy( sNode, aNode, ID_SIZEOF( qtcNode ) );

        QTC_NODE_INIT( aNode );

        // Node 정보 설정
        aNode->node.module = &mtfLnnvl;
        aNode->node.lflag  = mtfLnnvl.lflag & ~MTC_NODE_COLUMN_COUNT_MASK;

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode,
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   mtfLnnvl.lflag & MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );

        // aNode에 생성된 LNNVL의 argument로 sNode를 설정한다.
        aNode->node.arguments  = (mtcNode *)sNode;

        aNode->node.next = (mtcNode *)sNode->node.next;
        sNode->node.next = NULL;

        IDE_TEST( estimateNodeWithArgument( aStatement,
                                            aNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARGUMENT_TYPE );
    (void)sqlInfo.init(aStatement->qmeMem);
    IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ARGUMENT_TYPE,
                              sqlInfo.getErrMessage() ) );
    (void)sqlInfo.fini();

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::notNode( qcStatement* aStatement,
                     qtcNode**    aNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Parsing 단계에서 NOT keyword를 만났을 때의 처리를 한다.
 *
 * Implementation :
 *
 *    별도의 NOT Node를 생성하지 않고 Node Tree를 Traverse하며,
 *    논리 연산자 및 비교 연산자에 대하여
 *    NOT에 해당하는 Counter 연산자로 변경한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::notNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode*            sNode;
    qcuSqlSourceInfo    sqlInfo;

    if( aNode[0]->node.module->counter != NULL )
    {
        aNode[0]->node.module = aNode[0]->node.module->counter;

        aNode[0]->node.lflag &= ~MTC_NODE_OPERATOR_MASK;
        aNode[0]->node.lflag |=
                          aNode[0]->node.module->lflag & MTC_NODE_OPERATOR_MASK;
        // BUG-19180
        aNode[0]->node.lflag |=
                          aNode[0]->node.module->lflag & MTC_NODE_PRINT_FMT_MASK;

        // PROJ-1718 Subquery unnesting
        aNode[0]->node.lflag &= ~MTC_NODE_GROUP_MASK;
        aNode[0]->node.lflag |=
                          aNode[0]->node.module->lflag & MTC_NODE_GROUP_MASK;

        if ( ( aNode[0]->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            for( sNode  = (qtcNode*)aNode[0]->node.arguments;
                 sNode != NULL;
                 sNode  = (qtcNode*)sNode->node.next )
            {
                IDE_TEST( notNode( aStatement, &sNode ) != IDE_SUCCESS );
            }
        }
        else
        {
            // Nothing To Do
            // A3에서는 Subquery에 Predicate의 종류를 유지하였지만,
            // A4에서는 유지하지 않는다.
        }
    }
    else
    {
        // BUG-36125 NOT LNNVL(condition)은 LNNVL(LNNVL(condition))로 변환한다.
        if( aNode[0]->node.module == &mtfLnnvl )
        {
            IDE_TEST( lnnvlNode( aStatement, aNode[0] ) != IDE_SUCCESS );
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement, &aNode[0]->position );
            IDE_RAISE( ERR_INVALID_ARGUMENT_TYPE );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARGUMENT_TYPE );
    (void)sqlInfo.init(aStatement->qmeMem);
    IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ARGUMENT_TYPE,
                              sqlInfo.getErrMessage() ) );
    (void)sqlInfo.fini();

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::priorNode( qcStatement* aStatement,
                       qtcNode**    aNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Parsing 단계에서 PRIOR keyword를 만났을 때의 처리를 한다.
 *
 * Implementation :
 *
 *       ex) PRIOR (i1 + 1) : 자신 뿐 아니라, argument에도 prior를
 *           설정하여야 한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::priorNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode* sNode;

    aNode[0]->lflag |= QTC_NODE_PRIOR_EXIST;

    // Prior Column은 Column임에도 불구하고 그 특성 상
    // 상수에 가까운 특성을 갖는다.  즉, Indexable Predicate의 분류 등과
    // Key Range생성에 대한 판단을 할 때 상수로 취급되어야 한다.
    //     Ex) WHERE i1 = prior i2
    //         : prior i2는 index를 사용할 수 있는 column이
    //           아니나, 이러한 구분이 용이하지 한다.
    // 이러한 처리를 용이하게 하기 위해, 다음과 같이
    // Dependency 및 Indexable에 대한 처리를 한다.

    // Dependencies 를 제거
    qtc::dependencyClear( & aNode[0]->depInfo );

    // 인덱스를 사용할 수 없는 Column임을 표시.
    aNode[0]->node.lflag &= ~MTC_NODE_INDEX_MASK;
    aNode[0]->node.lflag |= MTC_NODE_INDEX_UNUSABLE;

    for( sNode  = (qtcNode*)aNode[0]->node.arguments;
         sNode != NULL;
         sNode  = (qtcNode*)sNode->node.next )
    {
        IDE_TEST( priorNode( aStatement, &sNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::priorNodeSetWithNewTuple( qcStatement* aStatement,
                                      qtcNode**    aNode,
                                      UShort       aOriginalTable,
                                      UShort       aTable )
{
/***********************************************************************
 *
 * Description :
 *
 *    Optimization 단계에서 처리되며,
 *    PRIOR Node를 새로운 Tuple을 기준으로 재구성한다.
 *
 *
 * Implementation :
 *
 *   해당 Node를 Traverse하면서,
 *   PRIOR Node의 Table ID를 새로운 Table ID(aTable)로 변경한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::priorNodeSetWithNewTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode* sNode;

    if( (aNode[0]->lflag & QTC_NODE_PRIOR_MASK) ==
        QTC_NODE_PRIOR_EXIST &&
        (aNode[0]->node.table == aOriginalTable ) )
    {
        aNode[0]->node.table = aTable;

        // set execute
        IDE_TEST( qtc::estimateNode( aStatement, aNode[0] )
                  != IDE_SUCCESS );
    }

    for( sNode  = (qtcNode*)aNode[0]->node.arguments;
         sNode != NULL;
         sNode  = (qtcNode*)sNode->node.next )
    {
        IDE_TEST( priorNodeSetWithNewTuple( aStatement,
                                            &sNode,
                                            aOriginalTable,
                                            aTable ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeNode( qcStatement*    aStatement,
                      qtcNode**       aNode,
                      qcNamePosition* aPosition,
                      const UChar*    aOperator,
                      UInt            aOperatorLength )
{
/***********************************************************************
 *
 * Description :
 *
 *    연산자를 위한 Node를 생성함.
 *
 * Implementation :
 *
 *    주어진 String(aOperator)으로부터 해당 function module을 획득하고,
 *    이를 이용하여 Node를 생성한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtfModule* sModule;
    idBool           sExist;

    // Node 생성
    IDU_LIMITPOINT("qtc::makeNode::malloc1");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
             != IDE_SUCCESS);

    // Node 초기화
    QTC_NODE_INIT( aNode[0] );
    aNode[1] = NULL;

    // Node 정보 설정
    // if you change this code, you should change qmvQTC::setColumnID
    if( aOperatorLength != 0 )
    {
        IDE_TEST( mtf::moduleByName( &sModule,
                                     &sExist,
                                     (void*)aOperator,
                                     aOperatorLength ) != IDE_SUCCESS );
        if ( sExist == ID_FALSE )
        {
            // if no module is found, use stored function module.
            // only in unified_invocation in qcply.y
            sModule = & qtc::spFunctionCallModule;
        }

        aNode[0]->node.lflag  = sModule->lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
        aNode[0]->node.module = sModule;
        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );
        
        // set base table and column ID
        aNode[0]->node.baseTable = aNode[0]->node.table;
        aNode[0]->node.baseColumn = aNode[0]->node.column;
    }
    else
    {
        aNode[0]->node.lflag  = 0;
        aNode[0]->node.module = NULL;
    }

    if( &qtc::spFunctionCallModule == aNode[0]->node.module )
    {
        // fix BUG-14257
        aNode[0]->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
        aNode[0]->lflag |= QTC_NODE_PROC_FUNCTION_TRUE;
    }

    aNode[0]->position            = *aPosition;
    aNode[0]->columnName          = *aPosition;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeNodeForMemberFunc( qcStatement     * aStatement,
                                   qtcNode        ** aNode,
                                   qcNamePosition  * aPositionStart,
                                   qcNamePosition  * aPositionEnd,
                                   qcNamePosition  * aUserNamePos,
                                   qcNamePosition  * aTableNamePos,
                                   qcNamePosition  * aColumnNamePos,
                                   qcNamePosition  * aPkgNamePos )
{
/***********************************************************************
 *
 * Description : PROJ-1075
 *       array type의 member function을 고려하여 node를 생성한다.
 *       일반 function은 검색하지 않는다.
 *
 * Implementation :
 *       이 함수로 올 수 있는 경우의 함수 유형
 *           (1) arrvar_name.memberfunc_name()            - aUserNamePos(x)
 *           (2) user_name.spfunc_name()                  - aUserNamePos(x)
 *           (3) label_name.arrvar_name.memberfunc_name() - 모두 존재
 *           (4) user_name.package_name.spfunc_name()     - 모두 존재
 *            // BUG-38243 support array method at package
 *           (5) user_name.package_name.array_name.memberfunc_name
 *                -> (5)는 항상 memberber function일 수 밖에 없음
 ***********************************************************************/

    const mtfModule* sModule;
    idBool           sExist;
    SChar            sBuffer[1024];
    SChar            sNameBuffer[256];
    UInt             sLength;

    // Node 생성
    IDU_LIMITPOINT("qtc::makeNodeForMemberFunc::malloc");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
             != IDE_SUCCESS);

    // Node 초기화
    QTC_NODE_INIT( aNode[0] );
    aNode[1] = NULL;

    // 적합성 검사. 적어도 tableName, columnName이 존재해야 함.
    IDE_DASSERT( QC_IS_NULL_NAME( (*aTableNamePos) ) == ID_FALSE );
    IDE_DASSERT( QC_IS_NULL_NAME( (*aColumnNamePos) ) == ID_FALSE );

    if ( QC_IS_NULL_NAME( (*aPkgNamePos) ) == ID_TRUE )
    {
        // Node 정보 설정
        IDE_TEST( qsf::moduleByName( &sModule,
                                     &sExist,
                                     (void*)(aColumnNamePos->stmtText +
                                             aColumnNamePos->offset),
                                     aColumnNamePos->size )
                  != IDE_SUCCESS );
        
        if ( sExist == ID_FALSE )
        {
            sModule = & qtc::spFunctionCallModule;
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        IDE_TEST( qsf::moduleByName( &sModule,
                                     &sExist,
                                     (void*)(aPkgNamePos->stmtText +
                                             aPkgNamePos->offset),
                                     aPkgNamePos->size )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            sLength = aPkgNamePos->size;
            
            if( sLength >= ID_SIZEOF(sNameBuffer) )
            {
                sLength = ID_SIZEOF(sNameBuffer) - 1;
            }
            else
            {
                /* Nothing to do. */
            }
            
            idlOS::memcpy( sNameBuffer,
                           (SChar*) aPkgNamePos->stmtText + aPkgNamePos->offset,
                           sLength );
            sNameBuffer[sLength] = '\0';
            idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                             "(Name=\"%s\")", sNameBuffer );
        
            IDE_RAISE( ERR_NOT_FOUND );
        }
        else
        {
            /* Nothing to do. */
        }
    }

    aNode[0]->node.lflag  = sModule->lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
    aNode[0]->node.module = sModule;

    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                               aNode[0],
                               aStatement,
                               QC_SHARED_TMPLATE(aStatement),
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK )
              != IDE_SUCCESS );

    // set base table and column ID
    aNode[0]->node.baseTable = aNode[0]->node.table;
    aNode[0]->node.baseColumn = aNode[0]->node.column;
    
    if( &qtc::spFunctionCallModule == aNode[0]->node.module )
    {
        // fix BUG-14257
        aNode[0]->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
        aNode[0]->lflag |= QTC_NODE_PROC_FUNCTION_TRUE;
    }
    
    aNode[0]->userName   = *aUserNamePos;
    aNode[0]->tableName  = *aTableNamePos;
    aNode[0]->columnName = *aColumnNamePos;
    aNode[0]->pkgName    = *aPkgNamePos;

    aNode[0]->position.stmtText = aPositionStart->stmtText;
    aNode[0]->position.offset   = aPositionStart->offset;
    aNode[0]->position.size     = aPositionEnd->offset + aPositionEnd->size
                                - aPositionStart->offset;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_FUNCTION_MODULE_NOT_FOUND,
                                  sBuffer ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC qtc::makeNode( qcStatement*    aStatement,
                      qtcNode**       aNode,
                      qcNamePosition* aPosition,
                      mtfModule*      aModule )
{
/***********************************************************************
 *
 * Description :
 *
 *    Module 정보(aModule)로부터 Node를 생성함.
 *
 * Implementation :
 *
 *    주어진 Module 정보(aModule)를 이용하여 Node를 생성한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // Node 생성
    IDU_LIMITPOINT("qtc::makeNode::malloc2");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
             != IDE_SUCCESS);

    // Node 초기화
    QTC_NODE_INIT( aNode[0] );
    aNode[1] = NULL;

    // Node 정보 설정
    aNode[0]->node.lflag  = aModule->lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
    aNode[0]->node.module = aModule;

    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                               aNode[0],
                               aStatement,
                               QC_SHARED_TMPLATE(aStatement),
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               aModule->lflag & MTC_NODE_COLUMN_COUNT_MASK )
              != IDE_SUCCESS );
    
    // set base table and column ID
    aNode[0]->node.baseTable = aNode[0]->node.table;
    aNode[0]->node.baseColumn = aNode[0]->node.column;
    
    aNode[0]->position            = *aPosition;
    aNode[0]->columnName          = *aPosition;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qtc::addArgument( qtcNode** aNode,
                         qtcNode** aArgument )
{
/***********************************************************************
 *
 * Description :
 *
 *    해당 Node(aNode)에 Argument를 추가함.
 *
 * Implementation :
 *
 *    해당 Node에 Argument를 추가한다.
 *    Argument가 전혀 없는 경우와 Argument가 이미 존재하는 경우를
 *    구분하여 처리한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::addArgument"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcNamePosition sPosition;

    IDE_TEST_RAISE( ( aNode[0]->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK )
                    == MTC_NODE_ARGUMENT_COUNT_MAXIMUM,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    if( aNode[1] == NULL )
    {
        aNode[0]->node.arguments = (mtcNode*)aArgument[0];
    }
    else
    {
        aNode[1]->node.next = (mtcNode*)aArgument[0];
    }
    aNode[0]->node.lflag++;
    aNode[1] = aArgument[0];

    if( aNode[0]->position.offset < aNode[1]->position.offset )
    {
        sPosition.stmtText = aNode[0]->position.stmtText;
        sPosition.offset   = aNode[0]->position.offset;
        sPosition.size     = aNode[1]->position.offset + aNode[1]->position.size
                           - sPosition.offset;
    }
    else
    {
        sPosition.stmtText = aNode[1]->position.stmtText;
        sPosition.offset   = aNode[1]->position.offset;
        sPosition.size     = aNode[0]->position.offset + aNode[0]->position.size
                           - sPosition.offset;
    }
    aNode[0]->position = sPosition;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::addWithinArguments( qcStatement  * aStatement,
                                qtcNode     ** aNode,
                                qtcNode     ** aArguments )
{
/***********************************************************************
 *
 * Description : PROJ-2527 WITHIN GROUP AGGR
 *
 *    해당 Node(aNode)에 Function Arguments를 추가함.
 *
 *    BUG-41771 필독 사항
 *      현재(2015.06.15) PERCENT_RANK 함수는
 *                      Aggregation (WITHIN GROUP) 함수만 있고,
 *                      Analytic (OVER) 함수가 없다.
 *
 *      PERCENT_RANK() OVER(..) 함수를 추가한다면,
 *         PERCENT_RANK() OVER(..)           --> 함수명 : "PERCENT_RANK"
 *                                               모듈명 : mtfPercentRank
 *         PERCENT_RANK(..) WITHIN GROUP(..) --> 함수명 : "PERCENT_RANK_WITHIN_GROUP"
 *                                               모듈명 : mtfPercentRankWithinGroup
 *        이렇게 작업하고, 아래의 코드 중,
 *
 *        ( aNode[0]->node.module != &mtfPercentRankWithinGroup ), // BUG-41771
 *        -->
 *        ( aNode[0]->node.module != &mtfPercentRank ),
 *
 *        로 수정한다.
 *
 *      ( qtc::changeWithinGroupNode 코드도 필독 )
 *
 *    BUG-41800
 *      현재(2015.06.15) CUME_DIST 함수는
 *                      Aggregation (WITHIN GROUP) 함수만 있고,
 *                      Analytic (OVER) 함수가 없다.
 *
 *      CUME_DIST() OVER(..) 함수를 추가한다면,
 *         CUME_DIST() OVER(..)           --> 함수명 : "CUME_DIST"
 *                                            모듈명 : mtfCumeDist
 *         CUME_DIST(..) WITHIN GROUP(..) --> 함수명 : "CUME_DIST_WITHIN_GROUP"
 *                                            모듈명 : mtfCumeDistWithinGroup
 *        이렇게 작업하고, 아래의 코드 중,
 *
 *        ( aNode[0]->node.module != &mtfCumeDistWithinGroup ), // BUG-417800
 *        -->
 *        ( aNode[0]->node.module != &mtfCumeDist ),
 *
 *        로 수정한다.
 *
 *      ( qtc::changeWithinGroupNode 코드도 필독 )
 *
 * Implementation :
 *
 *    aNode에 within group 인자를 funcArguments에 추가하고
 *    aNode->node.arguments에 복사 연결한다.
 *
 ***********************************************************************/

    qtcNode   * sCopyNode;
    qtcNode   * sNode;
    UInt        sCount = 0;

    IDE_TEST_RAISE( ( aNode[0]->node.module != &mtfListagg ) &&
                    ( aNode[0]->node.module != &mtfPercentileCont ) &&
                    ( aNode[0]->node.module != &mtfPercentileDisc ) &&
                    ( aNode[0]->node.module != &mtfRank ) &&                   // BUG-41631
                    ( aNode[0]->node.module != &mtfPercentRankWithinGroup ) && // BUG-41771
                    ( aNode[0]->node.module != &mtfCumeDistWithinGroup ),      // BUG-41800
                    ERR_NOT_ALLOWED_WITHIN_GROUP );
    
    if ( aArguments[0]->node.module != NULL )
    {
        // one expression
        aNode[0]->node.funcArguments = &(aArguments[0]->node);
        sCount = 1;
    }
    else
    {
        // list expression
        aNode[0]->node.funcArguments = aArguments[0]->node.arguments;
        sCount = ( aArguments[0]->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK );
    }
    
    // 최대 갯수는 넘을 수 없다.
    IDE_TEST_RAISE( ( aNode[0]->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) + sCount >
                    MTC_NODE_ARGUMENT_COUNT_MAXIMUM,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    // funcArguments의 node.lflag를 보존하기 위해 복사한다.
    IDE_TEST( copyNodeTree( aStatement,
                            (qtcNode*) aNode[0]->node.funcArguments,
                            & sCopyNode,
                            ID_TRUE,
                            ID_FALSE )
              != IDE_SUCCESS );

    // arguments에 붙여넣는다.
    if ( aNode[0]->node.arguments == NULL )
    {
        aNode[0]->node.arguments = (mtcNode*) sCopyNode;
    }
    else
    {
        for ( sNode = (qtcNode*) aNode[0]->node.arguments;
              sNode->node.next != NULL;
              sNode = (qtcNode*) sNode->node.next );

        sNode->node.next = (mtcNode*) sCopyNode;
    }

    //argument count를 증가
    aNode[0]->node.lflag += sCount;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOWED_WITHIN_GROUP );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_ALLOWED_WITHIN_GROUP ) );

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::modifyQuantifiedExpression( qcStatement* aStatement,
                                        qtcNode**    aNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Parsing 단계에서
 *    Quantified Expression 처리를 위해 생성한 잉여 Node를 제거한다.
 *
 * Implementation :
 *
 *    Parsing을 위해 생성한 잉여 Node가 존재할 경우, 이를 제거한다.
 *
 *    ex) i1 in ( select a1 ... )
 *
 *          [IN]                          [IN]
 *           |                             |
 *           V                             V
 *          [i1] --> [잉여 Node]    ==>   [i1] --> [subquery]
 *                       |
 *                       V
 *                    [subquery]
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::modifyQuantifiedExpression"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode*  sArgument1;
    qtcNode*  sArgument2;
    qtcNode*  sArgument;

    sArgument1 = (qtcNode*)aNode[0]->node.arguments;
    sArgument2 = (qtcNode*)sArgument1->node.next;

    if( ( sArgument2->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 &&
        sArgument2->node.module == &mtfList )
    {
        sArgument = (qtcNode*)sArgument2->node.arguments;
        if( sArgument->node.module              == &subqueryModule ||
            ( sArgument->node.module              == &mtfList      &&
              ( sArgument1->node.module           != &mtfList      ||
                sArgument->node.arguments->module == &mtfList ) ) )
        {
            sArgument1->node.next   = (mtcNode*)&sArgument->node;
            aNode[1]                = sArgument;

            IDE_TEST( mtc::initializeColumn(
                          QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sArgument2->node.table].columns +
                          sArgument2->node.column,
                          & qtc::skipModule,
                          0,
                          0,
                          0 )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeValue( qcStatement*    aStatement,
                       qtcNode**       aNode,
                       const UChar*    aTypeName,
                       UInt            aTypeLength,
                       qcNamePosition* aPosition,
                       const UChar*    aValue,
                       UInt            aValueLength,
                       mtcLiteralType  aLiteralType )
{
/***********************************************************************
 *
 * Description :
 *
 *    Value를 위한 Node를 생성함.
 *
 * Implementation :
 *
 *   Data Type 정보(aTypeName)과 Value 정보(aValue)를 이용하여
 *   Value Node를 생성한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeValue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule* sModule;
    IDE_RC           sResult;
    mtcTemplate    * sMtcTemplate;
    mtcColumn      * sColumn;
    UInt             sMtcColumnFlag = 0;

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    // Node 생성
    IDU_LIMITPOINT("qtc::makeValue::malloc");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
             != IDE_SUCCESS);

    // Node 초기화
    QTC_NODE_INIT( aNode[0] );
    aNode[1] = NULL;

    // Node 정보 설정
    aNode[0]->node.lflag          = valueModule.lflag
                                  & ~MTC_NODE_COLUMN_COUNT_MASK;
    aNode[0]->position            = *aPosition;
    aNode[0]->columnName          = *aPosition;
    aNode[0]->node.module         = &valueModule;

    IDE_TEST( mtd::moduleByName( &sModule, (void*)aTypeName, aTypeLength )
              != IDE_SUCCESS );

    sResult = qtc::nextColumn( QC_QMP_MEM(aStatement),
                               aNode[0],
                               aStatement,
                               QC_SHARED_TMPLATE(aStatement),
                               MTC_TUPLE_TYPE_CONSTANT,
                               valueModule.lflag & MTC_NODE_COLUMN_COUNT_MASK );

    // PROJ-1579 NCHAR
    // 리터럴의 종류에 따라서 아래와 같이 분류한다.
    // NCHAR 리터럴    : N'안'
    // 유니코드 리터럴 : U'\C548'
    // 일반 리터럴     : 'ABC' or CHAR'ABC'
    if( aLiteralType == MTC_COLUMN_NCHAR_LITERAL )
    {
        sMtcColumnFlag |= MTC_COLUMN_LITERAL_TYPE_NCHAR;
    }
    else if( aLiteralType == MTC_COLUMN_UNICODE_LITERAL )
    {
        sMtcColumnFlag |= MTC_COLUMN_LITERAL_TYPE_UNICODE;
    }
    else
    {
        // mtcColumn.flag = 0으로 초기화
        sMtcColumnFlag = MTC_COLUMN_LITERAL_TYPE_NORMAL;
    }

    if( sResult == IDE_SUCCESS )
    {
        // To Fix BUG-12607
        sColumn = sMtcTemplate->rows[aNode[0]->node.table].columns
            + sMtcTemplate->rows[aNode[0]->node.table].columnCount - 1;


        IDE_TEST( mtc::initializeColumn(
                      sColumn,
                      sModule,
                      0,
                      0,
                      0 )
                  != IDE_SUCCESS );

        // PROJ-1579 NCHAR
        // 파싱 단계에서 LITERAL_TYPE_NCHAR와 LITERAL_TYPE_UNICODE가
        // 설정될 수 있다.
        sColumn->flag |= sMtcColumnFlag;

        // To Fix BUG-12925
        IDE_TEST(
            sColumn->module->value(
                sMtcTemplate,
                sMtcTemplate->rows[aNode[0]->node.table].columns
                + sMtcTemplate->rows[aNode[0]->node.table].columnCount - 1,
                sMtcTemplate->rows[aNode[0]->node.table].row,
                & sMtcTemplate->rows[aNode[0]->node.table].rowOffset,
                sMtcTemplate->rows[aNode[0]->node.table].rowMaximum,
                (const void*) aValue,
                aValueLength,
                & sResult )
            != IDE_SUCCESS );
    }

    //---------------------------------------------------------
    // sResult != IDE_SUCCESS인 경우는 두 경우가 있다.
    // qtc::nextColumn()으로 Next Column을 할당받지 못한 경우,
    // sModule->value() 호출 시 남은 row 공간이 부족한 경우
    //---------------------------------------------------------
    if( sResult != IDE_SUCCESS )
    {
        IDE_TEST( qtc::nextRow( QC_QMP_MEM(aStatement),
                                aStatement,
                                QC_SHARED_TMPLATE(aStatement),
                                MTC_TUPLE_TYPE_CONSTANT )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_CONSTANT,
                                   valueModule.lflag &
                                   MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );

        // To Fix BUG-12607
        sColumn = sMtcTemplate->rows[aNode[0]->node.table].columns
            + sMtcTemplate->rows[aNode[0]->node.table].columnCount - 1;

        IDE_TEST( mtc::initializeColumn(
                      sColumn,
                      sModule,
                      0,
                      0,
                      0 )
                  != IDE_SUCCESS );

        // PROJ-1579 NCHAR
        // 파싱 단계에서 LITERAL_TYPE_NCHAR와 LITERAL_TYPE_UNICODE가
        // 설정될 수 있다.
        sColumn->flag |= sMtcColumnFlag;

        // To Fix BUG-12925
        IDE_TEST(
            sColumn->module->value(
                sMtcTemplate,
                sMtcTemplate->rows[aNode[0]->node.table].columns
                + sMtcTemplate->rows[aNode[0]->node.table].columnCount - 1,
                sMtcTemplate->rows[aNode[0]->node.table].row,
                & sMtcTemplate->rows[aNode[0]->node.table].rowOffset,
                sMtcTemplate->rows[aNode[0]->node.table].rowMaximum,
                (const void*) aValue,
                aValueLength,
                & sResult )
            != IDE_SUCCESS );

        IDE_TEST_RAISE( sResult != IDE_SUCCESS, ERR_INVALID_LITERAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeNullValue( qcStatement     * aStatement,
                           qtcNode        ** aNode,
                           qcNamePosition  * aPosition )
{
/***********************************************************************
 *
 * Description : BUG-38952 type null
 *    NULL Value를 위한 Node를 생성함.
 *
 * Implementation :
 *    TYPE_NULL property에 따라 null type의 NULL을 생성하거나
 *    varchar type의 NULL을 생성한다.
 *
 ***********************************************************************/

    if ( QCU_TYPE_NULL == 0 )
    {
        IDE_TEST( makeValue( aStatement,
                             aNode,
                             (const UChar*)"NULL",
                             4,
                             aPosition,
                             (UChar*)( aPosition->stmtText + aPosition->offset ),
                             aPosition->size,
                             MTC_COLUMN_NORMAL_LITERAL )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( makeValue( aStatement,
                             aNode,
                             (const UChar*)"VARCHAR",
                             7,
                             aPosition,
                             (UChar*)( aPosition->stmtText + aPosition->offset ),
                             0,
                             MTC_COLUMN_NORMAL_LITERAL )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::makeProcVariable( qcStatement*     aStatement,
                              qtcNode**        aNode,
                              qcNamePosition*  aPosition,
                              mtcColumn *      aColumn,
                              UInt             aProcVarOp )
{
/***********************************************************************
 *
 * Description :
 *
 *    Procedure Variable을 위한 Node를 생성함.
 *
 * Implementation :
 *
 *    Column 정보(aColumn)를 이용하여 Procedure Variable을 위한
 *    Node를 생성한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeProcVariable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcTemplate       * sQcTemplate;
    mtcTemplate      * sMtcTemplate;
    mtcTuple         * sMtcTuple;
    UShort             sCurrRowID;
    UShort             sColumnIndex;
    UInt               sOffset;

    // Node 생성
    IDU_LIMITPOINT("qtc::makeProcVariable::malloc");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
             != IDE_SUCCESS);

    // Node 초기화
    QTC_NODE_INIT( aNode[0] );
    aNode[1] = NULL;

    // Node 정보 설정
    aNode[0]->node.lflag          = columnModule.lflag
                                  & ~MTC_NODE_COLUMN_COUNT_MASK;
    aNode[0]->position            = *aPosition;
    aNode[0]->columnName          = *aPosition;
    aNode[0]->node.module         = &columnModule;
    aNode[0]->node.table          = ID_USHORT_MAX;
    aNode[0]->node.column         = ID_USHORT_MAX;

    sQcTemplate = QC_SHARED_TMPLATE(aStatement);
    sMtcTemplate = & sQcTemplate->tmplate;

    if ( aProcVarOp & QTC_PROC_VAR_OP_NEXT_COLUMN )
    {
        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
         * Intermediate Tuple Row가 있고 비어 있지 않은 상태에서,
         * Intermediate Tuple Row에 Lob Column을 할당할 때,
         * (Old Offset + New Size) > Property 이면,
         * 새로운 Intermediate Tuple Row를 할당한다.
         */
        if ( ( sQcTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE] != ID_USHORT_MAX ) &&
             ( aColumn != NULL ) )
        {
            sCurrRowID = sQcTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE];

            sMtcTuple = &(sQcTemplate->tmplate.rows[sCurrRowID]);
            if ( sMtcTuple->columnCount != 0 )
            {
                if ( ( aColumn->type.dataTypeId == MTD_BLOB_ID ) ||
                     ( aColumn->type.dataTypeId == MTD_CLOB_ID ) )
                {
                    for( sColumnIndex = 0, sOffset = 0;
                         sColumnIndex < sMtcTuple->columnCount;
                         sColumnIndex++ )
                    {
                        if ( sMtcTuple->columns[sColumnIndex].module != NULL )
                        {
                            sOffset = idlOS::align( sOffset,
                                                    sMtcTuple->columns[sColumnIndex].module->align );
                            sOffset += sMtcTuple->columns[sColumnIndex].column.size;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }

                    if ( (sOffset + (UInt)aColumn->column.size ) > QCU_INTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT )
                    {
                        IDE_TEST( qtc::nextRow( QC_QMP_MEM(aStatement),
                                                aStatement,
                                                sQcTemplate,
                                                MTC_TUPLE_TYPE_INTERMEDIATE )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   sQcTemplate,
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   valueModule.lflag &
                                   MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );

        // for rule_data_type in qcply.y
        if ( aColumn != NULL )
        {
            sMtcTemplate->rows[aNode[0]->node.table]
                .columns[ aNode[0]->node.column ] = *aColumn ;
        }

        if ( aProcVarOp & QTC_PROC_VAR_OP_SKIP_MODULE )
        {
            IDE_TEST( mtc::initializeColumn(
                          sMtcTemplate->rows[aNode[0]->node.table].columns
                          + aNode[0]->node.column,
                          & qtc::skipModule,
                          0,
                          0,
                          0 )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // no column is
        IDE_DASSERT( aColumn == NULL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeInternalProcVariable( qcStatement*    aStatement,
                                      qtcNode**       aNode,
                                      qcNamePosition* aPosition,
                                      mtcColumn *     aColumn,
                                      UInt            aProcVarOp
    )
{
/***********************************************************************
 *
 * Description :
 *    For PR-11391
 *    Internal Procedure Variable을 위한 Node를 생성함.
 *    Internal Procedure Variable은 table의 column인지 procedure variable인지
 *    검사하면 안됨.
 * Implementation :
 *
 *    Column 정보(aColumn)를 이용하여 Procedure Variable을 위한
 *    Node를 생성한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeInternalProcVariable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcTemplate * sMtcTemplate;

    // Node 생성
    IDU_LIMITPOINT("qtc::makeInternalProcVariable::malloc");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
             != IDE_SUCCESS);

    // Node 초기화
    QTC_NODE_INIT( aNode[0] );
    aNode[1] = NULL;

    // Node 정보 설정
    aNode[0]->node.lflag          = columnModule.lflag
                                  & ~MTC_NODE_COLUMN_COUNT_MASK;
    aNode[0]->lflag               = QTC_NODE_INTERNAL_PROC_VAR_EXIST;

    if( aPosition == NULL )
    {
        SET_EMPTY_POSITION(aNode[0]->position);
        SET_EMPTY_POSITION(aNode[0]->columnName);
    }
    else
    {
        aNode[0]->position            = *aPosition;
        aNode[0]->columnName          = *aPosition;
    }

    aNode[0]->node.module         = &columnModule;

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    if ( aProcVarOp & QTC_PROC_VAR_OP_NEXT_COLUMN )
    {
        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   valueModule.lflag &
                                   MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );

        // for rule_data_type in qcply.y
        if ( aColumn != NULL )
        {
            sMtcTemplate->rows[aNode[0]->node.table]
                .columns[ aNode[0]->node.column ] = *aColumn ;
        }

        if ( aProcVarOp & QTC_PROC_VAR_OP_SKIP_MODULE )
        {
            IDE_TEST( mtc::initializeColumn(
                          sMtcTemplate->rows[aNode[0]->node.table].columns
                          + aNode[0]->node.column,
                          & qtc::skipModule,
                          0,
                          0,
                          0 )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // no column is
        IDE_DASSERT( aColumn == NULL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeVariable( qcStatement*    aStatement,
                          qtcNode**       aNode,
                          qcNamePosition* aPosition )
{
/***********************************************************************
 *
 * Description :
 *
 *    Host Variable을 위한 Node를 생성함.
 *
 * Implementation :
 *
 *    Host 변수가 존재함을 Setting하고,
 *    Value Module로 연산자 Module을 Setting한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeVariable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStmtListMgr     * sStmtListMgr;
    qcNamedVarNode    * sNamedVarNode;
    qcNamedVarNode    * sTailVarNode;
    idBool              sIsFound = ID_FALSE;
    idBool              sIsSameVar = ID_FALSE;
    UInt                sHostVarCnt;    
    UInt                sHostVarIdx;
    qcBindNode        * sBindNode;
    qtcNode           * sNode;

    IDE_ASSERT( aStatement->myPlan->stmtListMgr != NULL );

    sStmtListMgr = aStatement->myPlan->stmtListMgr;

    // Node 생성
    IDU_LIMITPOINT("qtc::makeVariable::malloc");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
             != IDE_SUCCESS);

    // Node 초기화
    QTC_NODE_INIT( aNode[0] );
    aNode[1] = NULL;

    // Node 정보 설정
    aNode[0]->node.lflag          = ( valueModule.lflag
                                  | MTC_NODE_BIND_EXIST )
                                  & ~MTC_NODE_COLUMN_COUNT_MASK;
    aNode[0]->position            = *aPosition;
    aNode[0]->columnName          = *aPosition;
    aNode[0]->node.module         = &valueModule;

    sHostVarCnt = qcg::getBindCount( aStatement );
    
    // PROJ-2415 Grouping Sets Clause
    for ( sHostVarIdx = sStmtListMgr->mHostVarOffset; /* TASK-7219 */
          sHostVarIdx < sHostVarCnt;
          sHostVarIdx++ )
    {
        if ( sStmtListMgr->hostVarOffset[ sHostVarIdx ] == aPosition->offset )
        {
            // reparsing 할때 이미 할당된 column을 다시 사용 한다.
            aNode[0]->node.table  = QC_SHARED_TMPLATE(aStatement)->tmplate.variableRow;
            aNode[0]->node.column = sHostVarIdx;

            sIsSameVar = ID_TRUE;

            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sIsSameVar == ID_FALSE )
    {
        // 최초 parsing 할때
        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_VARIABLE,
                                   valueModule.lflag & MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );
        
        aStatement->myPlan->stmtListMgr->hostVarOffset[ aNode[0]->node.column ] =
            aPosition->offset;
        aStatement->myPlan->stmtListMgr->mHostVarNode[ aNode[0]->node.column ] =
            aNode[0]; /* TASK-7219 */

        IDE_DASSERT( aNode[0]->node.column == sHostVarCnt );
    }
    else
    {
        aStatement->myPlan->stmtListMgr->mHostVarNode[ aNode[0]->node.column ] =
            aNode[0]; /* TASK-7219 */
    }

    // BUG-36986
    // PSM 에 한해 parsing 과정에서 variable 에 대한 qtcNode 생성시
    // 중복회피를 위해 table, column position 은 기존의 것으로 세팅한다.
    // BUGBUG : 일반적으로 CLI, JDBC, ODBC 등에서
    //          name base biding 을 지원하게 되면 수정해야 함

    // reparsing 할 때 column position 이 순차적이므로
    // makeVariable 이 끝난 후 QCP_SET_HOST_VAR_OFFSET 의 인자로 쓰기 위해
    // 원래의 column position 을 baseColumn 에 copy 한다.
    aNode[0]->node.baseColumn = aNode[0]->node.column;

    if( aStatement->calledByPSMFlag == ID_TRUE )
    {
        // BUG-41248 DBMS_SQL package
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qcBindNode,
                                &sBindNode )
                  != IDE_SUCCESS );
 
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qtcNode,
                                &sNode )
                  != IDE_SUCCESS);

        *sNode = *aNode[0];

        sBindNode->mVarNode = sNode;

        sBindNode->mNext = aStatement->myPlan->bindNode;

        aStatement->myPlan->bindNode = sBindNode;
 
        if( aStatement->namedVarNode == NULL )
        {
            // 최초생성
            IDE_TEST( STRUCT_ALLOC( QC_QME_MEM( aStatement ),
                                    qcNamedVarNode,
                                    &aStatement->namedVarNode )
                      != IDE_SUCCESS );

            aStatement->namedVarNode->varNode = aNode[0];
            aStatement->namedVarNode->next    = NULL;
        }
        else
        {
            // 기존에 등록된 variable 검사
            for( sNamedVarNode = aStatement->namedVarNode;
                 sNamedVarNode != NULL;
                 sNamedVarNode = sNamedVarNode->next )
            {
                sTailVarNode = sNamedVarNode;

                if( ( idlOS::strMatch( aNode[0]->columnName.stmtText + aNode[0]->columnName.offset,
                                       aNode[0]->columnName.size,
                                       "?", 1 ) != 0 )
                    &&
                    ( idlOS::strMatch( aNode[0]->columnName.stmtText + aNode[0]->columnName.offset,
                                       aNode[0]->columnName.size,
                                       sNamedVarNode->varNode->columnName.stmtText
                                         + sNamedVarNode->varNode->columnName.offset,
                                       sNamedVarNode->varNode->columnName.size ) == 0 ) )

                {
                    // Same host variable found.
                    sIsFound = ID_TRUE;
                    break;
                }
            }

            if( sIsFound == ID_TRUE )
            {
                // 동일한 host variable 의 table, column position set
                aNode[0]->node.table  = sNamedVarNode->varNode->node.table;
                aNode[0]->node.column = sNamedVarNode->varNode->node.column;
            }
            else
            {
                // namedVarNode 에 추가
                IDE_TEST( STRUCT_ALLOC( QC_QME_MEM( aStatement ),
                                        qcNamedVarNode,
                                        &sNamedVarNode )
                          != IDE_SUCCESS );

                sNamedVarNode->varNode = aNode[0];
                sNamedVarNode->next    = NULL;
                sTailVarNode->next     = sNamedVarNode;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qtc::makeColumn( qcStatement*    aStatement,
                        qtcNode**       aNode,
                        qcNamePosition* aUserPosition,
                        qcNamePosition* aTablePosition,
                        qcNamePosition* aColumnPosition,
                        qcNamePosition* aPkgPosition )
{
/***********************************************************************
 *
 * Description :
 *
 *    Column을 위한 Node를 생성함.
 *
 * Implementation :
 *
 *    User Name, Table Name의 존재 유무에 따라 각각에 알맞은
 *    정보를 설정하고, Column Module로 연산자 모듈을 setting한다.
 *
 ***********************************************************************/

    // Node 생성
    IDU_LIMITPOINT("qtc::makeColumn::malloc");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
             != IDE_SUCCESS);

    // Node 초기화
    QTC_NODE_INIT( aNode[0] );
    aNode[1] = NULL;

    // Node 정보 설정
    aNode[0]->node.lflag          = columnModule.lflag
                                  & ~MTC_NODE_COLUMN_COUNT_MASK;

    // PROJ-1073 Package
    if( ( aUserPosition != NULL ) &&
        ( aTablePosition != NULL ) &&
        ( aColumnPosition != NULL ) &&
        ( aPkgPosition != NULL ) )
    {
        // ----------------------------------
        // | USER | TABLE | COLUMN | PACKAGE
        // ----------------------------------
        // |  O   |   O   |   O    |   O  
        // ----------------------------------
        aNode[0]->position.stmtText = aUserPosition->stmtText;
        aNode[0]->position.offset   = aUserPosition->offset;
        aNode[0]->position.size     = aPkgPosition->offset
                                    + aPkgPosition->size
                                    - aUserPosition->offset;
        aNode[0]->userName          = *aUserPosition;
        aNode[0]->tableName         = *aTablePosition;
        aNode[0]->columnName        = *aColumnPosition;
        aNode[0]->pkgName           = *aPkgPosition;
    }
    // fix BUG-19185
    else if( (aUserPosition != NULL) &&
             (aTablePosition != NULL) &&
             (aColumnPosition != NULL) )
    {
        // ----------------------------------
        // | USER | TABLE | COLUMN | PACKAGE
        // ----------------------------------
        // |  O   |   O   |   O    |    X
        // ----------------------------------
        aNode[0]->position.stmtText = aUserPosition->stmtText;
        aNode[0]->position.offset   = aUserPosition->offset;
        aNode[0]->position.size     = aColumnPosition->offset
                                    + aColumnPosition->size
                                    - aUserPosition->offset;
        aNode[0]->userName          = *aUserPosition;
        aNode[0]->tableName         = *aTablePosition;
        aNode[0]->columnName        = *aColumnPosition;
    }
    else if( (aTablePosition != NULL) && (aColumnPosition != NULL)  )
    {
        // ----------------------------------
        // | USER | TABLE | COLUMN | PACKAGE 
        // ----------------------------------
        // |  X   |   O   |   O    |   X
        // ----------------------------------
        aNode[0]->position.stmtText = aTablePosition->stmtText;
        aNode[0]->position.offset   = aTablePosition->offset;
        aNode[0]->position.size     = aColumnPosition->offset
                                    + aColumnPosition->size
                                    - aTablePosition->offset;
        aNode[0]->tableName         = *aTablePosition;
        aNode[0]->columnName        = *aColumnPosition;
    }
    else if ( aColumnPosition != NULL )
    {
        // ----------------------------------
        // | USER | TABLE | COLUMN | PACKAGE
        // ----------------------------------
        // |  X   |   X   |   O    |   X
        // ----------------------------------
        aNode[0]->position          = *aColumnPosition;
        aNode[0]->columnName        = *aColumnPosition;
    }
    else
    {
        // nothing to do 
    }

    aNode[0]->node.module = &columnModule;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::makeAssign( qcStatement * aStatement,
                        qtcNode     * aNode,
                        qtcNode     * aArgument )
{
/***********************************************************************
 *
 * Description :
 *    Assign을 위한 Node를 생성함.
 *    Indirect와 달리 aNode를 초기화하지 않고 그대로 이용한다.
 *
 * Implementation :
 *    TODO - 외부에서 Argument를 처리하지 않도록 수정해야 함.
 *    즉, makeIndirect와 같이 argument에 대한 연결을 고려해야 하며,
 *    Host 변수 binding등을 고려하여,
 *    argument의 node.flag 정보등을 estimateInternal()과 같이
 *    승계할 수 있도록 해야 한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeAssign"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // Node 정보 설정
    aNode->node.module    = &assignModule;
    aNode->node.arguments = &aArgument->node;
    aNode->node.lflag     = ( assignModule.lflag
                              & ~MTC_NODE_COLUMN_COUNT_MASK ) | 1;
    
    aNode->overClause     = NULL;

    // To Fix BUG-10887
    qtc::dependencySetWithDep( & aNode->depInfo, & aArgument->depInfo );

    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                               aNode,
                               aStatement,
                               QC_SHARED_TMPLATE(aStatement),
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               1 )
              != IDE_SUCCESS );

    IDE_TEST( qtc::estimateNode( aStatement,
                                 aNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeIndirect( qcStatement* aStatement,
                          qtcNode*     aNode,
                          qtcNode*     aArgument )
{
/***********************************************************************
 *
 * Description :
 *
 *    Indirection을 위한 Node를 생성함.
 *    (참조, qtcIndirect.cpp)
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeIndirect"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    QTC_NODE_INIT( aNode );

    aNode->node.module    = &indirectModule;
    aNode->node.arguments = &aArgument->node;
    aNode->node.lflag     = ( indirectModule.lflag
                              & ~MTC_NODE_COLUMN_COUNT_MASK ) | 1;

    // To Fix BUG-10887
    qtc::dependencySetWithDep( & aNode->depInfo, & aArgument->depInfo );

    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                               aNode,
                               aStatement,
                               QC_SHARED_TMPLATE(aStatement),
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               1 )
              != IDE_SUCCESS );

    IDE_TEST( qtc::estimateNode( aStatement,
                                 aNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qtc::makePassNode( qcStatement* aStatement,
                   qtcNode *    aCurrentNode,
                   qtcNode *    aArgumentNode,
                   qtcNode **   aPassNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Pass Node를 생성함.
 *    (참조, qtcPass.cpp)
 *
 * Implementation :
 *
 *    Current Node가 존재할 경우, 이 공간을 Pass Node로 대체함.
 *    Current Node가 없을 경우, 새로운 Pass Node를 생성함.
 *
 *    Current Node가 존재하는 경우
 *        - SELECT i1 + 1 FROM T1 GROUP BY i1 + 1 HAVING i1 + 1 > ?;
 *                 ^^^^^^                                ^^^^^^
 *        - 해당 i1 + 1을 Pass Node로 대체하고 Pass Node의 argument로
 *          GROUP BY i1 + 1의 (i1 + 1)을 취하게 된다.
 *        - Pass Node에 Conversion이 존재할 수 있으며, indirection 되지
 *          않도록 해야 한다.
 *    Current Node 가 없는 경우
 *        - SELECT i1 + 1 FROM T1 ORDER BY 1;
 *        - Sorting을 위해 새로 생성한 (i1 + 1)을 Argument로 하여 Pass
 *          Node 를 생성하게 된다.
 *        - Pass Node에 Conversion이 존재하지 않으며,
 *          indirection이 되도록 하여야 한다.
 *
 ***********************************************************************/

#define IDE_FN "qtc::makePassNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode * sPassNode;

    if ( aCurrentNode != NULL )
    {
        // Pass Node 기본 정보 Setting
        aCurrentNode->node.module = &passModule;

        // flag 정보 변경
        // 기존 정보는 그대로 유지하고, pass node의 정보만 추가
        aCurrentNode->node.lflag &= ~MTC_NODE_COLUMN_COUNT_MASK;
        aCurrentNode->node.lflag |= 1;

        aCurrentNode->node.lflag &= ~MTC_NODE_OPERATOR_MASK;
        aCurrentNode->node.lflag |= MTC_NODE_OPERATOR_MISC;

        // Indirection이 아님을 Setting
        aCurrentNode->node.lflag &= ~MTC_NODE_INDIRECT_MASK;
        aCurrentNode->node.lflag |= MTC_NODE_INDIRECT_FALSE;

        // PROJ-1473
        aCurrentNode->node.lflag &= ~MTC_NODE_REMOVE_ARGUMENTS_MASK;
        aCurrentNode->node.lflag |= MTC_NODE_REMOVE_ARGUMENTS_TRUE;

        sPassNode = aCurrentNode;
    }
    else
    {
        // 새로운 Pass Node 생성
        IDU_LIMITPOINT("qtc::makePassNode::malloc");
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, & sPassNode)
                 != IDE_SUCCESS);

        // Argument Node를 그대로 복사
        idlOS::memcpy( sPassNode, aArgumentNode, ID_SIZEOF(qtcNode) );

        // Pass Node 기본 정보 Setting
        sPassNode->node.module    = &passModule;

        // flag 정보 변경
        // Argument 노드의 기존 정보는 그대로 유지하고,
        // Pass node의 정보만 추가
        sPassNode->node.lflag &= ~MTC_NODE_COLUMN_COUNT_MASK;
        sPassNode->node.lflag |= 1;

        sPassNode->node.lflag &= ~MTC_NODE_OPERATOR_MASK;
        sPassNode->node.lflag |= MTC_NODE_OPERATOR_MISC;

        // Indirection임을 Setting
        sPassNode->node.lflag &= ~MTC_NODE_INDIRECT_MASK;
        sPassNode->node.lflag |= MTC_NODE_INDIRECT_TRUE;

        sPassNode->node.conversion     = NULL;
        sPassNode->node.leftConversion = NULL;
        sPassNode->node.funcArguments  = NULL;
        sPassNode->node.orgNode        = NULL;
        sPassNode->node.next           = NULL;
        sPassNode->node.cost           = 0;
        sPassNode->subquery            = NULL;
        sPassNode->indexArgument       = 0;
    }

    // 새로운 ID Setting
    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                               sPassNode,
                               aStatement,
                               QC_SHARED_TMPLATE(aStatement),
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               1 )
              != IDE_SUCCESS );

    sPassNode->node.arguments = (mtcNode*) aArgumentNode;

    // set base table and column ID
    sPassNode->node.baseTable = aArgumentNode->node.baseTable;
    sPassNode->node.baseColumn = aArgumentNode->node.baseColumn;

    sPassNode->overClause = NULL;

    IDE_TEST( qtc::estimateNode( aStatement,
                                 sPassNode ) != IDE_SUCCESS );

    *aPassNode = sPassNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qtc::preProcessConstExpr( qcStatement  * aStatement,
                          qtcNode      * aNode,
                          mtcTemplate  * aTemplate,
                          mtcStack     * aStack,
                          SInt           aRemain,
                          mtcCallBack  * aCallBack )
{
/***********************************************************************
 *
 * Description :
 *    Validation 단계에서 항상 일정한 값을 갖게 되는
 *    Constant Expression에 대하여 미리 수행한다.
 *
 *    이러한 처리는 다음과 같은 이점을 얻기 위해서이다.
 *        1.  Expression의 반복 수행 방지
 *            ex) i1 = 1 + 1
 *                ^^^^^^^^^^
 *          동일 결과를 생산함에도 불구하고 계속 반복 수행되는 것을
 *          방지하기 위함이다.
 *        2.  Selectivity 추출
 *            ex) double1 > 3
 *                         ^^
 *           위와 같이 Column Type이 double이고 Value Type이 integer일
 *           경우 3값이 double형으로 변환되어야 selectivity를 계산할 수
 *           있다.  이러한 predicate의 selectivity를 추출하기 위해 Value
 *           영역에 대한 값을 미리 계산함으로서 그 효과를 얻을 수 있다.
 *        3.  Fixed Key Range 사용 효과의 증대
 *            ex) double1 = 3 + 4
 *                          ^^^^^
 *            앞의 경우와 마찬가지로 Value영역은 연산이 필요하고,
 *            값의 conversion되어야 하기 때문에
 *            Key Range를 미리 생성할 수 없다.  그러나, 이는 연산의 결과와
 *            Conversion 결과가 항상 일정하기 때문에 Fixed Key Range로
 *            생성할 수 있게 된다.
 *
 *    이러한 처리가 가능하기 위해서는 다음과 같은 문제를 해결하여야 한다.
 *
 *        - 일반 상수는 Tuple Set의 [Constant 영역]에 저장된다.  이 영역은
 *          절대 변하지 않는 영역이기 때문에 값을 저장하기 위한 메모리 공간이
 *          할당되어 있다.
 *          그러나, 연산 또는  Conversion은 그 결과가 가변적이며 Data Type
 *          또한 가변적일 수 있으므로 [Intermediate 영역]에서 관리되며,
 *          값을 위한 메모리를 Execution 시점에 할당받게 된다.
 *          즉, 항상 같은 결과를 생성하는 연산이라 할 지라도 값을 관리할
 *          공간이 없기 때문에 미리 수행할 수 없게 된다.
 *        - 따라서, Constant Expression에 대한 선처리를 위해서는
 *          Intermediate 영역에서 관리되는 연산을 Constant 영역으로
 *          이동시켜야 하며, 연산의 수행과 Node간의 연결 관계의 표현,
 *          연산의 속성 변경(일반 상수형)을 고려하여야 한다.
 *
 * Implementation :
 *     Constant Expression의 선처리가 가능한 조건
 *        - 호스트 변수가 없어야 한다.
 *            * i1 = 1 + ?
 *                  ^^^^^^
 *              위와 같이 Host 변수가 있는 경우 그 연산 결과를 예단할 수 없다.
 *            * i1 + ? = 1 + 1
 *                       ^^^^^
 *              연산 결과는 일정하지만, 좌측의 호스트 변수로 인해 데이터
 *              타입이 변할 수도 있다.
 *        - 연산 및 Conversion이 발생해야 한다.
 *            * i1 = 1
 *                   ^^
 *              이미 [Constant 영역]에 존재하며 처리할 이유가 없다.
 *        - Aggregation이 없어야 한다.
 *            * i1 = 1 + SUM(1)
 *                   ^^^^^^^^^^
 *              상수 형태로 보이나, Record의 개수에 따라 결과가 달라진다.
 *        - Subquery가 없어야 한다.
 *            * i1 = 1 + (select sum(a1) from t2 );
 *                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *              Dependencies가 존재하지 않아 상수처럼 판단될 수 있다.
 *        - 상수로만 구성되어야 한다.
 *            * i1 = 1 + 1
 *        - PRIOR Column이 없어야 한다.
 *            * i1 = prior i2 + 1
 *                   ^^^^^^^^^^^^
 *              Dependencies가 없어 상수처럼 보이나, Record의 변화에 따라
 *              결과가 바뀐다.
 *
 *    처리 절차
 *
 *        - 현재 노드가 아닌 Argument에 대해서만 처리한다.
 *            i1 + ? = 1 + 1
 *                     ^^^^^
 *            즉, (1 + 1)에 대한 처리는 (+) 노드가 아닌 (=) 노드에서
 *            처리되어야 한다.
 *        - 처리 가능 여부를 판단한다.
 *            - 현재 노드를 기준으로 다음을 판단한다.
 *                - Host 변수가 존재하지 않아야 한다.
 *                - Argument가 있어야 한다.
 *                - subquery가 아니어야 한다.
 *                - List가 아니어야 한다.
 *            - 현재 노드와 Argument를 기준으로 다음을 판단한다.
 *                - dependecies가 zero이어야 한다.
 *                - argument나 conversion이 있어야 한다.
 *                - prior가 없어야 한다.
 *                - aggregation이 없어야 한다.
 *                - subquery가 없어야 한다.
 *                - list에 대한 처리를 고려해야 한다.
 *        - 위 조건을 만족할 경우 해당 argument 노드를 위한
 *          Constant 영역을 할당한다.
 *            - Argument가 있다면 자신을 위한 영역 할당
 *            - Conversion이 있다면 모든 Conversion을 위한 영역 할당
 *              double1 = 1 + 1
 *                        ^^^^^
 *              [+]--conv-->[bigint=>double]--conv-->[int=>bigint]
 *               |
 *               V
 *              [1]-->[1]
 *
 *              위와 같은 경우 자신을 위한 영역 1, Conversion을 위한 영역 2개,
 *              총 3개의 영역을 할당받는다.
 *
 *        - Argument 노드의 연산을 수행한다.
 *             연산을 수행함으로서 Constant 영역에 결과가 기록된다.
 *        - Node의 연결 관계를 보정
 *
 *             [=]
 *              |
 *              V            |-conv-->[B=>D]--conv-->[I=>B]
 *             [double1]--->[+]
 *                           |
 *                           V
 *                          [1]-->[1]
 *
 *                               ||
 *                              \  /
 *                               \/
 *
 *             [=]
 *              |
 *              V
 *             [double1]-->[B=>D]
 *
 *         - Value 모듈로 전환
 *             연결 관계에 의해 선택된 Node를 Value Module로 전환한다.
 *
 *   위와 같은 처리 과정은 ::estimateInternal()에 의해
 *   상위 노드로 올라가면서 처리되게 된다.
 *
 ***********************************************************************/

#define IDE_FN "qtc::preProcessConstExpr"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode * sCurNode;
    qtcNode * sPrevNode;
    qtcNode * sListCurNode;
    qtcNode * sListPrevNode;
    qtcNode * sResultNode;
    qtcNode * sOrgNode;

    mtcStack * sStack;
    SInt       sRemain;
    mtcStack * sListStack;
    SInt       sListRemain;

    idBool     sAbleProcess;

    //-------------------------------------------
    // 현재 노드를 이용한 적합성 검사
    //-------------------------------------------

    if ( (aNode->subquery == NULL) &&
         (aNode->node.arguments != NULL) &&
         (aNode->node.module != &mtfList) )
    {
        //-------------------------------------------
        // 각 Argument에 대한 Pre-Processing 처리
        //-------------------------------------------

        for ( sCurNode = (qtcNode*) aNode->node.arguments, sPrevNode = NULL,
                  sStack = aStack + 1, sRemain = aRemain -1;
              sCurNode != NULL;
              sCurNode = (qtcNode*) sCurNode->node.next, sStack++, sRemain-- )
        {
            // BUG-40892 host 변수가 있을때 runConstExpr 를 수행하지 못함
            // host 변수를 체크하는 위치를 변경하여 가능한 runConstExpr를 수행함
            if( MTC_NODE_IS_DEFINED_VALUE( (mtcNode*)sCurNode ) == ID_TRUE )
            {
                // Argument의 적합성 검사
                IDE_TEST( isConstExpr( QC_SHARED_TMPLATE(aStatement),
                                       sCurNode,
                                       &sAbleProcess )
                          != IDE_SUCCESS );
            }
            else
            {
                sAbleProcess = ID_FALSE;
            }

            if ( sAbleProcess == ID_TRUE )
            {
                if ( sCurNode->node.module == & mtfList )
                {
                    // List인 경우
                    // List의 argument에 대하여 선처리를 수행함.
                    for ( sListCurNode = (qtcNode*) sCurNode->node.arguments,
                              sListPrevNode = NULL,
                              sListStack = sStack + 1,
                              sListRemain = sRemain - 1;
                          sListCurNode != NULL;
                          sListCurNode = (qtcNode*) sListCurNode->node.next,
                              sListStack++, sListRemain-- )
                    {
                        // 상수화하기 전 원래 노드를 백업 한다.
                        IDU_LIMITPOINT("qtc::preProcessConstExpr::malloc1");
                        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qtcNode, & sOrgNode )
                                  != IDE_SUCCESS);

                        idlOS::memcpy( sOrgNode, sListCurNode, ID_SIZEOF(qtcNode) );

                        // 상수 Expression을 수행함
                        IDE_TEST( runConstExpr( aStatement,
                                                QC_SHARED_TMPLATE(aStatement),
                                                sListCurNode,
                                                aTemplate,
                                                sListStack,
                                                sListRemain,
                                                aCallBack,
                                                & sResultNode )
                                  != IDE_SUCCESS );

                        if (sListPrevNode != NULL )
                        {
                            sListPrevNode->node.next = (mtcNode*) sResultNode;
                        }
                        else
                        {
                            sCurNode->node.arguments = (mtcNode*) sResultNode;
                        }
                        sResultNode->node.next = sListCurNode->node.next;
                        sResultNode->node.conversion = NULL;

                        sResultNode->node.arguments =
                            sListCurNode->node.arguments;
                        sResultNode->node.leftConversion =
                            sListCurNode->node.leftConversion;
                        sResultNode->node.orgNode =
                            (mtcNode*) sOrgNode;
                        sResultNode->node.funcArguments =
                            sListCurNode->node.funcArguments;
                        sResultNode->node.baseTable =
                            sListCurNode->node.baseTable;
                        sResultNode->node.baseColumn =
                            sListCurNode->node.baseColumn;

                        sListPrevNode = sResultNode;
                    }

                    // PROJ-1436
                    // LIST 자체를 상수화하지는 않는다. LIST를 상수화하면
                    // LIST에 대한 stack을 상수 tuple에 기록하게 되므로 상수
                    // tuple의 row가 오염된다.
                    sPrevNode = sCurNode;
                }
                else
                {
                    // 상수화하기 전 원래 노드를 백업 한다.
                    IDU_LIMITPOINT("qtc::preProcessConstExpr::malloc2");
                    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qtcNode, & sOrgNode )
                              != IDE_SUCCESS);
                    
                    idlOS::memcpy( sOrgNode, sCurNode, ID_SIZEOF(qtcNode) );
                    
                    // 상수 Expression을 수행함
                    IDE_TEST( runConstExpr( aStatement,
                                            QC_SHARED_TMPLATE(aStatement),
                                            sCurNode,
                                            aTemplate,
                                            sStack,
                                            sRemain,
                                            aCallBack,
                                            & sResultNode ) != IDE_SUCCESS );
                    
                    // 연결관계 정리
                    if ( sPrevNode != NULL )
                    {
                        sPrevNode->node.next = (mtcNode *) sResultNode;
                    }
                    else
                    {
                        aNode->node.arguments = (mtcNode *) sResultNode;
                    }
                    sResultNode->node.next = sCurNode->node.next;
                    sResultNode->node.conversion = NULL;
                    
                    // List등의 처리를 위하여 Argument정보는 남겨 둔다.
                    sResultNode->node.arguments =
                        sCurNode->node.arguments;
                    sResultNode->node.leftConversion =
                        sCurNode->node.leftConversion;
                    sResultNode->node.orgNode =
                        (mtcNode*) sOrgNode;
                    sResultNode->node.funcArguments =
                        sCurNode->node.funcArguments;
                    sResultNode->node.baseTable =
                        sCurNode->node.baseTable;
                    sResultNode->node.baseColumn =
                        sCurNode->node.baseColumn;

                    // 이미 치환된 Node를 이용하여야 한다.
                    sPrevNode = sResultNode;
                }
            }
            else
            {
                // 사전 처리할 수 없음.
                sPrevNode = sCurNode;
                continue;
            }
        } // end of for
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qtc::setColumnExecutionPosition( mtcTemplate * aTemplate,
                                 qtcNode     * aNode,
                                 qmsSFWGH    * aColumnSFWGH,
                                 qmsSFWGH    * aCurrentSFWGH )
{
/***********************************************************************
 *
 * Description : column 수행 위치 설정
 *
 * Implementation :
 *     ex) SELECT i1 FROM t1 ORDER BY i1;
 *         i1 column에는 target과 order by에서 수행된다는 정보가 설정됨
 *
 *      aColumnSFWGH : 실제 column이 속해 있는 SFWGH
 *     aCurrentSFWGH : 현재 처리중인 SFWGH
 ***********************************************************************/

    mtcColumn       * sMtcColumn;

    if( (aColumnSFWGH != NULL) &&
        (aCurrentSFWGH != NULL) )
    {
        sMtcColumn = QTC_TUPLE_COLUMN(&aTemplate->rows[aNode->node.table],
                                      aNode);

        // 1) 질의에 사용된 컬럼인지의 정보를 저장한다.
        sMtcColumn->flag |= MTC_COLUMN_USE_COLUMN_TRUE;

        /*
         * PROJ-1789 PROWID
         * 현재는 _PROWID가 target에 있는지 where절에 있는지 구분 X
         */
        if( aNode->node.column == MTC_RID_COLUMN_ID )
        {
            aTemplate->rows[aNode->node.table].lflag
                &= ~MTC_TUPLE_TARGET_RID_MASK;
            aTemplate->rows[aNode->node.table].lflag
                |= MTC_TUPLE_TARGET_RID_EXIST;
        }

        // 2) 컬럼의 위치 정보 설정
        if( aColumnSFWGH->thisQuerySet != NULL )
        {
            // 2) 컬럼의 위치가 target인지의정보를저장한다.
            if( aColumnSFWGH->thisQuerySet->processPhase
                == QMS_VALIDATE_TARGET )
            {
                // BUG-37841
                // window function에서 참조하는 컬럼은 target절 이외에서도 사용한다고
                // 설정하여 view push projection에서 제거되지 않도록 한다.
                if ( ( aNode->lflag & QTC_NODE_ANAL_FUNC_COLUMN_MASK )
                     == QTC_NODE_ANAL_FUNC_COLUMN_FALSE )
                {
                    sMtcColumn->flag
                        |= MTC_COLUMN_USE_TARGET_TRUE;
                }
                else
                {
                    sMtcColumn->flag
                        |= MTC_COLUMN_USE_NOT_TARGET_TRUE;
                }
            }
            else
            {
                sMtcColumn->flag
                    |= MTC_COLUMN_USE_NOT_TARGET_TRUE;
            }

            if( aColumnSFWGH->thisQuerySet->processPhase
                == QMS_VALIDATE_WHERE )
            {
                sMtcColumn->flag
                    |= MTC_COLUMN_USE_WHERE_TRUE;
            }
            else
            {
                // Nothing to do.
            }
  
            if( aColumnSFWGH->thisQuerySet->processPhase
                == QMS_VALIDATE_SET )
            {
                sMtcColumn->flag
                    |= MTC_COLUMN_USE_SET_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            // BUG-25470
            // OUTER COLUMN REFERENCE가 있는 경우 flag세팅한다.
            if( aColumnSFWGH != aCurrentSFWGH )
            {
                sMtcColumn->flag
                    |= MTC_COLUMN_OUTER_REFERENCE_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // SELECT 구문 외의 구문 칼럼인 경우
            // ex) DELETE 구문의 where 절 column
            // nothing to do
        }

        //-----------------------------------
        // LOB, GEOMETRY TYPE 등
        // binary 컬럼이 포함된 질의문은
        // rid 저장방식으로 처리한다.
        //-----------------------------------
        if( qtc::isEquiValidType(aNode, aTemplate)
            == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            aTemplate->rows[aNode->node.table].lflag
                &= ~MTC_TUPLE_BINARY_COLUMN_MASK;
            aTemplate->rows[aNode->node.table].lflag
                |= MTC_TUPLE_BINARY_COLUMN_EXIST;
        }

        // materialize 방식이 Push Projection 인 경우
        if( ( ( aTemplate->rows[aNode->node.table].lflag &
                MTC_TUPLE_STORAGE_MASK )
              == MTC_TUPLE_STORAGE_DISK )
            ||
            ( ( aTemplate->rows[aNode->node.table].lflag &
                MTC_TUPLE_VIEW_MASK )
              == MTC_TUPLE_VIEW_TRUE ) )
        {
            if( aColumnSFWGH->hints->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
            {
                // tuple set에도
                // 레코드저장방식으로 처리되어야 할
                // tuple임을 저장한다.
                // 이후, rid 또는 record 저장방식의 처리를
                // hint 또는 memory table 구분에 따른 복잡함을
                // 줄이기 위해서.
                if (((aTemplate->rows[aNode->node.table].lflag &
                      MTC_TUPLE_BINARY_COLUMN_MASK) ==
                     MTC_TUPLE_BINARY_COLUMN_ABSENT) &&
                    ((aTemplate->rows[aNode->node.table].lflag &
                      MTC_TUPLE_TARGET_RID_MASK) ==
                     MTC_TUPLE_TARGET_RID_ABSENT))
                {
                    aTemplate->rows[aNode->node.table].lflag
                        &= ~MTC_TUPLE_MATERIALIZE_MASK;
                    aTemplate->rows[aNode->node.table].lflag
                        |= MTC_TUPLE_MATERIALIZE_VALUE;
                }
                else
                {
                    // BUG-35585
                    // tuple뿐만아니라 SFWGH, querySet도 rid로 처리하게 한다.
                    aColumnSFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;

                    aTemplate->rows[aNode->node.table].lflag
                        &= ~MTC_TUPLE_MATERIALIZE_MASK;
                    aTemplate->rows[aNode->node.table].lflag
                        |= MTC_TUPLE_MATERIALIZE_RID;
                }
            }
            else
            {
                aTemplate->rows[aNode->node.table].lflag
                    &= ~MTC_TUPLE_MATERIALIZE_MASK;
                aTemplate->rows[aNode->node.table].lflag
                    |= MTC_TUPLE_MATERIALIZE_RID;
            }
        }
        else
        {
            // DISK와 VIEW가 아닌 경우
            // Nothing To Do
        }
    }
    else
    {
        // SFWGH가 NULL 인 경우
        // Nothing To Do 
    }

    return IDE_SUCCESS;
}


void qtc::checkLobAndEncryptColumn( mtcTemplate * aTemplate,
                                    mtcNode     * aNode )
{
    qtcNode     * sNode      = NULL;
    qcStatement * sStatement = NULL;
    mtdModule   * sMtdModule = NULL;

    sNode      = ( qtcNode * )aNode;
    sStatement = ( ( qcTemplate * )aTemplate )->stmt;

    /* PROJ-2462 ResultCache */
    if ( ( sNode->node.column != ID_USHORT_MAX ) &&
         ( sNode->node.table  != ID_USHORT_MAX ) )
    {
        sMtdModule = (mtdModule *)aTemplate->rows[sNode->node.table].columns[sNode->node.column].module;
        if ( sMtdModule != NULL )
        {
            // PROJ-2462 Result Cache
            if ( ( sMtdModule->flag & MTD_ENCRYPT_TYPE_MASK )
                 == MTD_ENCRYPT_TYPE_TRUE )
            {
                /* BUG-45626 Encrypt Column View Compile Fatal */
                if ( QC_SHARED_TMPLATE(sStatement) != NULL )
                {
                    QC_SHARED_TMPLATE(sStatement)->resultCache.flag &= ~QC_RESULT_CACHE_DISABLE_MASK;
                    QC_SHARED_TMPLATE(sStatement)->resultCache.flag |= QC_RESULT_CACHE_DISABLE_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }

            // PROJ-2462 Result Cache
            if ( ( sMtdModule->flag & MTD_COLUMN_TYPE_MASK )
                 == MTD_COLUMN_TYPE_LOB )
            {
                sNode->lflag &= ~QTC_NODE_LOB_COLUMN_MASK;
                sNode->lflag |= QTC_NODE_LOB_COLUMN_EXIST;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }
}

idBool qtc::isConstValue( qcTemplate  * aTemplate,
                          qtcNode     * aNode )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                constant value(정적 고정 표현식)인지 아닌지 검사.
 *
 *  Implementation :
 *            (1) conversion이 없어야 한다.
 *               - 위에서 이미 conversion붙여서 calculate한 것임.
 *            (2) valueModule이어야 한다.
 *               - 계산된 값은 하나의 특정 값으로 저장됨.
 *            (3) constant tuple 영역에 있어야 한다.
 *               - constant expression처리가 되었다면
 *                 constant tuple영역에 값이 생긴다.
 *
 ***********************************************************************/

    if ( ( aNode->node.conversion == NULL ) &&
         ( aNode->node.module == & qtc::valueModule ) &&
         ( (aTemplate->tmplate.rows[aNode->node.table].lflag &
            MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_CONSTANT ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

idBool qtc::isHostVariable( qcTemplate  * aTemplate,
                            qtcNode     * aNode )
{
/***********************************************************************
 *
 *  Description : host variable node인지 검사한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    if ( ( aNode->node.module == & qtc::valueModule ) &&
         ( (aTemplate->tmplate.rows[aNode->node.table].lflag &
            MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_VARIABLE ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

IDE_RC
qtc::isConstExpr( qcTemplate  * aTemplate,
                  qtcNode     * aNode,
                  idBool      * aResult )
{
/***********************************************************************
 *
 * Description :
 *
 *    Constant Expression인지의 판단
 *
 * Implementation :
 *
 *     [Argument의 적합성 검사]
 *          - argument나 conversion이 있어야 한다.
 *          - dependecies가 zero이어야 한다.
 *          - prior가 없어야 한다.
 *          - aggregation이 없어야 한다.
 *          - subquery가 없어야 한다.
 *          - 이미 conversion이 발생한 노드가 아니어야 한다.
 *          - column이 아니어야 한다.
 *            : PROJ-1075 array type은 column node에 argument가 올 수 있음.
 *
 *     [제약 조건의 정리]
 *          - Argument가 있다면, 모든 argument가 상수이어야 한다.
 *          - Argument가 없고 Conversion이 있다면,
 *          - 현재 Node가 상수이어야 한다.
 *
 ***********************************************************************/
#define IDE_FN "qtc::isConstExpr"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode * sCurNode;
    qtcNode * sArgNode;

    idBool sAbleProcess;

    sAbleProcess = ID_TRUE;
    sCurNode = aNode;

    // BUG-15995
    // random, sendmsg 같은 함수들은 constant로
    // 처리되어서는 안됨.
    if ( (sCurNode->node.module->lflag & MTC_NODE_VARIABLE_MASK)
         == MTC_NODE_VARIABLE_FALSE )
    {
        if ( sCurNode->node.arguments != NULL )
        {
            // To Fix PR-8724
            // SUM(4) 와 같이 Aggregation인 경우에는
            // 선처리해서는 안됨.
            // PROJ-1075
            // column node인 경우 argument가 있다 하더라도
            // constant expression이 될 수 없음.
            if ( ( sCurNode->subquery == NULL )
                 &&
                 ( (aNode->lflag & QTC_NODE_AGGREGATE_MASK)
                   == QTC_NODE_AGGREGATE_ABSENT )
                 &&
                 ( (aNode->lflag & QTC_NODE_AGGREGATE2_MASK)
                   == QTC_NODE_AGGREGATE2_ABSENT )
                 &&
                 ( (aNode->lflag & QTC_NODE_CONVERSION_MASK)
                   != QTC_NODE_CONVERSION_TRUE )
                 &&
                 ( aNode->node.module != &qtc::columnModule )
                 &&
                 ( (aNode->node.module->lflag & MTC_NODE_COLUMN_COUNT_MASK)
                   == 1 )
                 )
            {
                // Argument가 있는 경우
                for ( sArgNode = (qtcNode*) sCurNode->node.arguments;
                      sArgNode != NULL;
                      sArgNode = (qtcNode*) sArgNode->node.next )
                {
                    if ( ( sArgNode->node.module == & qtc::valueModule ) &&
                         ( (aTemplate->tmplate.rows[sArgNode->node.table].lflag &
                            MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_CONSTANT ) )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        sAbleProcess = ID_FALSE;
                        break;
                    }
                }
            }
            else
            {
                sAbleProcess = ID_FALSE;
            }
        }
        else
        {
            // Argument가 없는 경우
            if ( ( sCurNode->node.conversion != NULL ) &&
                 ( sCurNode->node.module == & qtc::valueModule ) &&
                 ( (aTemplate->tmplate.rows[sCurNode->node.table].lflag &
                    MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_CONSTANT ) )
            {
                // Nothing To Do
            }
            else
            {
                sAbleProcess = ID_FALSE;
            }
        }
    }
    else
    {
        // 항상 Variable로 처리되어야 하는 경우
        sAbleProcess = ID_FALSE;
    }

    *aResult = sAbleProcess;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qtc::runConstExpr( qcStatement * aStatement,
                   qcTemplate  * aTemplate,
                   qtcNode     * aNode,
                   mtcTemplate * aMtcTemplate,
                   mtcStack    * aStack,
                   SInt          aRemain,
                   mtcCallBack * aCallBack,
                   qtcNode    ** aResultNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Constant Expression을 수행하고 변환된 결과 노드를
 *    리턴한다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "qtc::runConstExpr"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode * sCurNode;
    qtcNode * sConvertNode;
    mtcStack * sStack;
    SInt       sRemain;

    qtcCallBackInfo * sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    qcuSqlSourceInfo  sqlInfo;
    UInt              sSqlCode;

    sCurNode = aNode;

    sStack = aTemplate->tmplate.stack;
    sRemain = aTemplate->tmplate.stackRemain;

    // 현재 노드를 위한 Constant 영역 할당
    if ( sCurNode->node.arguments != NULL )
    {
        IDE_TEST( getConstColumn( aStatement,
                                  aTemplate,
                                  sCurNode ) != IDE_SUCCESS );
    }

    // Conversion을 위한 Constant 영역 할당
    for ( sConvertNode = (qtcNode*) sCurNode->node.conversion;
          sConvertNode != NULL;
          sConvertNode = (qtcNode*) sConvertNode->node.conversion )
    {
        IDE_TEST( getConstColumn( aStatement,
                                  aTemplate,
                                  sConvertNode ) != IDE_SUCCESS );
    }

    // PROJ-1346
    // list conversion은 현재 노드의 상수화와 무관하여 주석처리하였다.
    // runConstExpr 수행후에 원래 노드의 leftConversion을 연결한다.
    
    // To Fix PR-12938
    // Left Conversion이 존재할 경우 이에 대한 Constant 영역을 할당함.
    // Left Conversion의 연속된 conversion은 node.conversion임.
    //for ( sConvertNode = (qtcNode*) sCurNode->node.leftConversion;
    //      sConvertNode != NULL;
    //      sConvertNode = (qtcNode*) sConvertNode->node.conversion )
    //{
    //    IDE_TEST( getConstColumn( aStatement,
    //                              aTemplate,
    //                              sConvertNode ) != IDE_SUCCESS );
    //}

    // Constant Expression에 대한 연산을 수행
    // Value영역에 값이 저장된다.
    aTemplate->tmplate.stack = aStack;
    aTemplate->tmplate.stackRemain = aRemain;

    if ( qtc::calculate( sCurNode,
                         aTemplate )
         != IDE_SUCCESS )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sCurNode->position );
        IDE_RAISE( ERR_PASS );
    }
    else
    {
        // Nothing to do.
    }

    // 최종 연산의 결과에 해당하는 노드 획득
    sConvertNode = (qtcNode *)
        mtf::convertedNode( (mtcNode *) sCurNode,
                            & aTemplate->tmplate );

    // Value Module로 치환
    if ( sConvertNode->node.module != &valueModule )
    {
        sConvertNode->node.module = &valueModule;

        // 노드변환이 발생함을 플래그로 설정
        sConvertNode->lflag &= ~QTC_NODE_CONVERSION_MASK;
        sConvertNode->lflag |= QTC_NODE_CONVERSION_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1413
    // Value Name의 보존
    // 이미 생성된 conversion 노드들에 주어진 position이 아니라
    // 원래 노드가 가진 position 정보를 유지해야 한다.
    if ( sCurNode != sConvertNode )
    {
        SET_POSITION( sConvertNode->position, sCurNode->position );
        SET_POSITION( sConvertNode->userName, sCurNode->userName );
        SET_POSITION( sConvertNode->tableName, sCurNode->tableName );
        SET_POSITION( sConvertNode->columnName, sCurNode->columnName );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST(
        qtc::estimateNode( sConvertNode,
                           aMtcTemplate,
                           aStack,
                           aRemain,
                           aCallBack )
        != IDE_SUCCESS );

    *aResultNode = sConvertNode;

    aTemplate->tmplate.stack = sStack;
    aTemplate->tmplate.stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PASS );
    {
        // sqlSourceInfo가 없는 error라면.
        if ( ideHasErrorPosition() == ID_FALSE )
        {
            sSqlCode = ideGetErrorCode();

            (void)sqlInfo.initWithBeforeMessage(sCallBackInfo->memory);
            IDE_SET(
                ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                                sqlInfo.getBeforeErrMessage(),
                                sqlInfo.getErrMessage()));
            (void)sqlInfo.fini();

            // overwrite wrapped errorcode to original error code.
            ideGetErrorMgr()->Stack.LastError = sSqlCode;
        }
        else
        {
            // Nothing to do.
        }
    }
    IDE_EXCEPTION_END;

    aTemplate->tmplate.stack = sStack;
    aTemplate->tmplate.stackRemain = sRemain;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qtc::getConstColumn( qcStatement * aStatement,
                     qcTemplate  * aTemplate,
                     qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Constant Expression의 사전 처리를 위한
 *    Constant Column영역의 회득
 *
 * Implementation :
 *
 *
 ***********************************************************************/
#define IDE_FN "qtc::getConstColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_RC sResult;

    UShort sOrgTupleID;
    UShort sOrgColumnID;
    UShort sNewTupleID;
    UShort sNewColumnID;
    UInt   sColumnCnt;

    mtcTemplate * sMtcTemplate;

    sMtcTemplate = & aTemplate->tmplate;

    // 기존 정보의 저장
    sOrgTupleID = aNode->node.table;
    sOrgColumnID = aNode->node.column;
    sColumnCnt = aNode->node.module->lflag & MTC_NODE_COLUMN_COUNT_MASK;

    // 적합성 검사
    IDE_DASSERT( sColumnCnt == 1 );

    // 새로운 Constant 영역의 할당
    sResult = qtc::nextColumn( QC_QMP_MEM(aStatement),
                               aNode,
                               aStatement,
                               aTemplate,
                               MTC_TUPLE_TYPE_CONSTANT,
                               sColumnCnt );

    // PROJ-1358
    // Internal Tuple의 증가로 기존의 Tuple Pointer가 변경될 수 있다.
    sNewTupleID = aNode->node.table;

    sMtcTemplate->rows[sNewTupleID].rowOffset = idlOS::align(
        sMtcTemplate->rows[sNewTupleID].rowOffset,
        sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID].module->align );

    if ( (sResult == IDE_SUCCESS) &&
         ( sMtcTemplate->rows[sNewTupleID].rowOffset
           +  sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID].column.size
           >  sMtcTemplate->rows[sNewTupleID].rowMaximum ) )
    {
        // Constant Tuple의 공간 크기 검사
        sResult = IDE_FAILURE;
    }

    // 공간이 부족할 경우 새로이 Constant Tuple 할당 후 처리
    if( sResult != IDE_SUCCESS )
    {
        // PROJ-1583 large geometry
        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
        if( (sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID].module->id == MTD_GEOMETRY_ID) ||
            (sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID].module->id == MTD_BINARY_ID) ||
            (sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID].module->id == MTD_BLOB_ID) ||
            (sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID].module->id == MTD_CLOB_ID) )
        {
            IDE_TEST( qtc::nextLargeConstColumn( QC_QMP_MEM(aStatement),
                                                 aNode,
                                                 aStatement,
                                                 aTemplate,
                                                 sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID].column.size )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qtc::nextRow( QC_QMP_MEM(aStatement),
                                    aStatement,
                                    aTemplate,
                                    MTC_TUPLE_TYPE_CONSTANT )
                      != IDE_SUCCESS );

            IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                       aNode,
                                       aStatement,
                                       aTemplate,
                                       MTC_TUPLE_TYPE_CONSTANT,
                                       sColumnCnt )
                      != IDE_SUCCESS );
        }

        // PROJ-1358
        // Internal Tuple의 증가로 기존의 Tuple Pointer가 변경될 수 있다.
        sNewTupleID = aNode->node.table;

        sMtcTemplate->rows[sNewTupleID].rowOffset = idlOS::align(
            sMtcTemplate->rows[sNewTupleID].rowOffset,
            sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID].module
            ->align );

        // 적합성 검사
        IDE_DASSERT(
            sMtcTemplate->rows[sNewTupleID].rowOffset
            + sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID].column.size
            <=  sMtcTemplate->rows[sNewTupleID].rowMaximum );
    }

    // 연산을 수행할 수 있도록
    // 기존 정보를 새로 할당 받은 Contant 영역에 복사
    sNewColumnID = aNode->node.column;

    idlOS::memcpy( & sMtcTemplate->rows[sNewTupleID].columns[sNewColumnID],
                   &  sMtcTemplate->rows[sOrgTupleID].columns[sOrgColumnID],
                   ID_SIZEOF(mtcColumn) * sColumnCnt );
    idlOS::memcpy( &  sMtcTemplate->rows[sNewTupleID].execute[sNewColumnID],
                   &  sMtcTemplate->rows[sOrgTupleID].execute[sOrgColumnID],
                   ID_SIZEOF(mtcExecute) * sColumnCnt );

    // Column의 offset및 Tuple의 offset 재조정
    sMtcTemplate->rows[sNewTupleID].columns[sNewColumnID].column.offset
        =  sMtcTemplate->rows[sNewTupleID].rowOffset;
    sMtcTemplate->rows[sNewTupleID].rowOffset
        +=  sMtcTemplate->rows[sNewTupleID].columns[sNewColumnID].column.size;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qtc::makeConstantWrapper( qcStatement * aStatement,
                          qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Host Constant Wrapper Node를 생성함.
 *    (참조, qtcConstantWrapper.cpp)
 *
 * Implementation :
 *
 *    현재 Node를 복사하고,
 *    현재 Node를 Constant Wrapper Node로 대체함.
 *    이렇게 함으로서 외부에서 별도의 고려 없이 처리가 가능함.
 *
 *    [aNode]      =>      [Wrapper]
 *                             |
 *                             V
 *                         ['aNode'] : 복사된 Node
 *
 ***********************************************************************/

#define IDE_FN "qtc::makeConstantWrapper"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode     * sNode;
    mtcTemplate * sTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;
    UShort        sVariableRow;

    //---------------------------------------
    // 입력된 Node를 복사
    //---------------------------------------

    if( aNode->node.module == & hostConstantWrapperModule )
    {
        return IDE_SUCCESS;
    }

    IDU_LIMITPOINT("qtc::makeConstantWrapper::malloc");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, & sNode )
             != IDE_SUCCESS);

    idlOS::memcpy( sNode, aNode, ID_SIZEOF(qtcNode) );

    //---------------------------------------
    // 현재 Node를 Wrapper Node로 대체함.
    //---------------------------------------

    aNode->node.module = & hostConstantWrapperModule;
    aNode->node.conversion     = NULL;
    aNode->node.leftConversion = NULL;
    aNode->node.funcArguments  = NULL;
    aNode->node.orgNode        = NULL;
    aNode->node.arguments      = & sNode->node; // 복사한 노드를 연결
    aNode->node.cost           = 0;
    aNode->subquery            = NULL;
    aNode->indexArgument       = 0;

    // fix BUG-11545
    // 복사된 노드(wrapper node의 arguments)의 next의 연결은 끊는다.
    // constantWrapper노드의 next에 이 next의 연결정보를 유지.
    sNode->node.next           = NULL;

    aNode->node.lflag  = hostConstantWrapperModule.lflag;
    aNode->node.lflag |=
        sNode->node.lflag & aNode->node.module->lmask & MTC_NODE_MASK;
    aNode->lflag |= sNode->lflag & QTC_NODE_MASK;

    // PROJ-1492
    // bind관련 lflag를 복사한다.
    aNode->node.lflag |= sNode->node.lflag & MTC_NODE_BIND_TYPE_MASK;

    //---------------------------------------
    // Node ID를 새로 할당받음
    //---------------------------------------

    // fix BUG-18868
    // BUG-17506의 수정으로 (sysdate, PSM변수, Bind변수)도
    // execution 중에 constant로 처리할 수 있게 되었다.
    // 그런데 호스트 변수를 사용하지 않는 질의의 경우 파싱과정에서
    // 호스트 변수를 위한 노드를 만들지 않는다.
    // 하지만 constant wrapper 노드도 variable tuple type을 사용하기 때문에
    // nextColumn() 호출 후에 variableRow의 값이 호스트 변수가 있는 것처럼
    // 변경되게 된다.
    // 이 값을 nextColumn() 호출 후에 원복해준다.

    sVariableRow = sTemplate->variableRow;

    IDE_TEST( qtc::nextColumn(
                  QC_QMP_MEM(aStatement),
                  aNode,
                  aStatement,
                  QC_SHARED_TMPLATE(aStatement),
                  MTC_TUPLE_TYPE_VARIABLE,
                  aNode->node.module->lflag & MTC_NODE_COLUMN_COUNT_MASK )
              != IDE_SUCCESS );

    // fix BUG-18868
    sTemplate->variableRow = sVariableRow;

    IDE_TEST( qtc::estimateNode( aStatement,
                                 aNode )
              != IDE_SUCCESS );

    //---------------------------------------
    // Execute 여부를 표현할 정보 공간의 위치를 지정함.
    //---------------------------------------

    aNode->node.info = sTemplate->execInfoCnt;
    sTemplate->execInfoCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qtc::optimizeHostConstExpression( qcStatement * aStatement,
                                  qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Node Tree 내에서 Host Constant Expression을 찾아내고,
 *    이에 대하여 Constant Wrapper Node를 만듦.
 *    (참조, qtcConstantWrapper.cpp)
 *
 * Implementation :
 *
 *    다음 경우를 Host Constant Expression으로 판단한다.
 *        - Dependencies가 0이어야 함.
 *        - Argument가 있거나 Conversion이 있어야 함.
 *        - Ex)
 *            - 5 + ?         [+] : Host Constant Expression
 *                             |
 *                             V
 *                            [5]---->[?]
 *
 *            - double = ?    [=]
 *                             |              t-->[conv]
 *                             V              |
 *                            [double]-------[?]  : Host Constant Expression
 *
 *    다음과 같은 경우는 더 이상 진행하지 않는다.
 *        - HOST 변수가 없는 경우
 *        - Indirection이 있는 경우
 *        - List형인 경우
 *        - 이는 Host Constant Expression이라 하더라도,
 *          결과가 하나 이상일 수 있기 때문이다.
 *
 *
 ***********************************************************************/

#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode * sNode;

    if ( ( (aNode->node.lflag & MTC_NODE_INDIRECT_MASK)
           == MTC_NODE_INDIRECT_TRUE )                // Indirection인 경우
         ||
         ( aNode->node.module == & mtfList )          // List 인 경우
         ||
         ( aNode->node.module == & subqueryModule )   // Subquery인 경우
         ||
         ( ( (aNode->lflag & QTC_NODE_AGGREGATE_MASK) // Aggregation이 존재
             == QTC_NODE_AGGREGATE_EXIST ) ||
           ( (aNode->lflag & QTC_NODE_AGGREGATE2_MASK)
             == QTC_NODE_AGGREGATE2_EXIST ) ) )
    {
        // Nothing To Do
        // 더 이상 진행하지 않는다.
    }
    else
    {
        // BUG-17506
        // Execution중에는 상수로 취급할 수 있는 노드를 Dynamic Constant라고
        // 부른다. (sysdate, PSM변수, Bind변수)
        if ( QTC_IS_DYNAMIC_CONSTANT( aNode ) == ID_TRUE )
        {
            if ( ( qtc::dependencyEqual( & aNode->depInfo,
                                         & qtc::zeroDependencies )
                   == ID_TRUE ) &&
                 ( aNode->node.arguments != NULL ||
                   aNode->node.conversion != NULL )
                 )
            {
                // Constant Expression인 경우로 Wrapper Node를 생성하고
                // 더 이상 진행하지 않는다.
                IDE_TEST( qtc::makeConstantWrapper( aStatement, aNode )
                          != IDE_SUCCESS );
            }
            else
            {
                // 하위 Node 영역에 대한 Traverse를 진행한다.
                for ( sNode = (qtcNode*) aNode->node.arguments;
                      sNode != NULL;
                      sNode = (qtcNode*) sNode->node.next )
                {
                    IDE_TEST( qtc::optimizeHostConstExpression( aStatement, sNode )
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            // Nothing To Do
            // 더 이상 진행하지 않는다.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qtc::makeSubqueryWrapper( qcStatement * aStatement,
                          qtcNode     * aSubqueryNode,
                          qtcNode    ** aWrapperNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Subquery Wrapper Node를 생성함.
 *    (참조, qtcSubqueryWrapper.cpp)
 *
 * Implementation :
 *    반드시 Subquery Node만을 인자로 받는다.
 *
 *                           [Wrapper]
 *                             |
 *                             V
 *    [aSubqueryNode]   =>   [aSubqueryNode]
 *
 ***********************************************************************/

#define IDE_FN "qtc::makeSubqueryWrapper"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode * sWrapperNode;
    mtcTemplate * sTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    // 반드시 Subquery Node를 인자로 받는다.
    IDE_DASSERT( aSubqueryNode->subquery != NULL );

    //---------------------------------------
    // 입력된 Subquery Node를 복사
    // dependencies및 flag 정보를 유지하기 위함.
    //---------------------------------------

    IDU_LIMITPOINT("qtc::makeSubqueryWrapper::malloc");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, & sWrapperNode )
             != IDE_SUCCESS);

    idlOS::memcpy( sWrapperNode, aSubqueryNode, ID_SIZEOF(qtcNode) );

    //---------------------------------------
    // Wrapper Node의 정보를 설정
    //---------------------------------------

    sWrapperNode->node.module = & subqueryWrapperModule;
    sWrapperNode->node.conversion     = NULL;
    sWrapperNode->node.leftConversion = NULL;
    sWrapperNode->node.funcArguments  = NULL;
    sWrapperNode->node.orgNode        = NULL;
    sWrapperNode->node.arguments      = (mtcNode*) aSubqueryNode;
    sWrapperNode->node.next           = NULL;
    sWrapperNode->node.cost           = 0;
    sWrapperNode->subquery            = NULL;
    sWrapperNode->indexArgument       = 0;

    // flag 정보 변경
    // 기존 정보는 그대로 유지하고, pass node의 정보만 추가
    sWrapperNode->node.lflag &= ~MTC_NODE_COLUMN_COUNT_MASK;
    sWrapperNode->node.lflag |= 1;

    sWrapperNode->node.lflag &= ~MTC_NODE_OPERATOR_MASK;
    sWrapperNode->node.lflag |= MTC_NODE_OPERATOR_MISC;

    // Indirection 임을 Setting
    sWrapperNode->node.lflag &= ~MTC_NODE_INDIRECT_MASK;
    sWrapperNode->node.lflag |= MTC_NODE_INDIRECT_TRUE;

    //---------------------------------------
    // Node ID를 새로 할당받음
    //---------------------------------------

    IDE_TEST(
        qtc::nextColumn(
            QC_QMP_MEM(aStatement),
            sWrapperNode,
            aStatement,
            QC_SHARED_TMPLATE(aStatement),
            MTC_TUPLE_TYPE_INTERMEDIATE,
            sWrapperNode->node.module->lflag & MTC_NODE_COLUMN_COUNT_MASK )
        != IDE_SUCCESS );

    IDE_TEST( qtc::estimateNode( aStatement,
                                 sWrapperNode )
              != IDE_SUCCESS );

    //---------------------------------------
    // Execute 여부를 표현할 정보 공간의 위치를 지정함.
    //---------------------------------------

    sWrapperNode->node.info = sTemplate->execInfoCnt;
    sTemplate->execInfoCnt++;

    *aWrapperNode = sWrapperNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aWrapperNode = NULL;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeTargetColumn( qtcNode* aNode,
                              UShort   aTable,
                              UShort   aColumn )
{
/***********************************************************************
 *
 * Description :
 *
 *    Asterisk Target Column을 위한 Node를 생성함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeTargetColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    QTC_NODE_INIT( aNode );
    
    aNode->node.module         = &columnModule;
    aNode->node.table          = aTable;
    aNode->node.column         = aColumn;
    aNode->node.baseTable      = aTable;
    aNode->node.baseColumn     = aColumn;
    aNode->node.lflag          = columnModule.lflag
                               & ~MTC_NODE_COLUMN_COUNT_MASK;
    
    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qtc::makeInternalColumn( qcStatement* aStatement,
                                UShort       aTable,
                                UShort       aColumn,
                                qtcNode**    aNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    SET등의 표현을 위해 중간 Column을 생성함.
 *
 *    ex) SELECT i1 FROM T1 INTERSECT SELECT a1 FROM T2;
 *    SET의 Target을 위한 별도의 Column 정보를 구성하여야 한다.
 *
 * Implementation :
 *
 *    정보가 없는 Column Node를 생성한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::makeInternalColumn"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode         * sNode[2] = {NULL,NULL};
    qcNamePosition    sColumnPos;

    SET_EMPTY_POSITION( sColumnPos );

    IDE_TEST( qtc::makeColumn( aStatement, sNode, NULL, NULL, &sColumnPos, NULL )
              != IDE_SUCCESS);

    sNode[0]->node.table  = aTable;
    sNode[0]->node.column = aColumn;

    *aNode = sNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aNode = NULL;

    return IDE_FAILURE;

#undef IDE_FN
}

void qtc::resetTupleOffset( mtcTemplate* aTemplate, UShort aTupleID )
{
/***********************************************************************
 *
 * Description :
 *
 *    해당 Tuple의 최대 offset을 재조정한다.
 *
 * Implementation :
 *
 *    Alignment를 고려하여 해당 Tuple의 최대 Offset을 재조정한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::resetTupleOffset"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt          sColumn;
    UInt          sOffset;

    for( sColumn = 0, sOffset = 0;
         sColumn < aTemplate->rows[aTupleID].columnCount;
         sColumn++ )
    {
        sOffset = idlOS::align(
            sOffset,
            aTemplate->rows[aTupleID].columns[sColumn].module->align );
        aTemplate->rows[aTupleID].columns[sColumn].column.offset = sOffset;
        sOffset +=  aTemplate->rows[aTupleID].columns[sColumn].column.size;

        // To Fix PR-8528
        // 임의로 생성되는 Tuple의 경우 Column ID를 임의로 설정한다.
        // 절대 존재할 수 없는 Table의 ID(0)를 기준으로 설정한다.
        aTemplate->rows[aTupleID].columns[sColumn].column.id = sColumn;
    }

    aTemplate->rows[aTupleID].rowOffset =
        aTemplate->rows[aTupleID].rowMaximum = sOffset;

#undef IDE_FN
}

IDE_RC qtc::allocIntermediateTuple( qcStatement* aStatement,
                                    mtcTemplate* aTemplate,
                                    UShort       aTupleID,
                                    SInt         aColCount)
{
/***********************************************************************
 *
 * Description :
 *
 *    Intermediate Tuple을 위한 공간을 할당한다.
 *
 * Implementation :
 *
 *    Intermediate Tuple을 위한 Column및 Execute공간을 확보한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::allocIntermediateTuple"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if (aColCount > 0)
    {
        aTemplate->rows[aTupleID].lflag
            = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];
        aTemplate->rows[aTupleID].columnCount   = aColCount;
        aTemplate->rows[aTupleID].columnMaximum = aColCount;

        /* BUG-44382 clone tuple 성능개선 */
        // 복사가 필요함
        setTupleColumnFlag( &(aTemplate->rows[aTupleID]),
                            ID_TRUE,
                            ID_FALSE );

        IDU_LIMITPOINT("qtc::allocIntermediateTuple::malloc1");
        IDE_TEST(
            QC_QMP_MEM(aStatement)->alloc(
                idlOS::align8((UInt)ID_SIZEOF(mtcColumn)) * aColCount,
                (void**)&(aTemplate->rows[aTupleID].columns))
            != IDE_SUCCESS);

        // PROJ-1473
        IDE_TEST(
            qtc::allocAndInitColumnLocateInTuple( QC_QMP_MEM(aStatement),
                                                  aTemplate,
                                                  aTupleID )
            != IDE_SUCCESS );

        IDU_LIMITPOINT("qtc::allocIntermediateTuple::malloc2");
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                     ID_SIZEOF(mtcExecute) * aColCount,
                     (void**) & (aTemplate->rows[aTupleID].execute))
                 != IDE_SUCCESS);

        aTemplate->rows[aTupleID].rowOffset  = 0;
        aTemplate->rows[aTupleID].rowMaximum = 0;
        aTemplate->rows[aTupleID].row        = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::changeNode( qcStatement*    aStatement,
                        qtcNode**       aNode,
                        qcNamePosition* aPosition,
                        qcNamePosition* aPositionEnd )
{
/***********************************************************************
 *
 * Description :
 *
 *    List Expression의 정보를 재조정한다.
 *
 * Implementation :
 *
 *    List Expression의 의미를 갖도록 정보를 Setting하며,
 *    해당 String의 위치를 재조정한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::changeNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtfModule* sModule;
    mtcNode*         sNode;
    idBool           sExist;

    IDE_TEST( mtf::moduleByName( &sModule,
                                 &sExist,
                                 (void*)(aPosition->stmtText+aPosition->offset),
                                 aPosition->size )
              != IDE_SUCCESS );
    
    if ( sExist == ID_FALSE )
    {
        sModule = & qtc::spFunctionCallModule;
    }

    // sModule이 mtfList가 아닌 경우는 함수형식으로 사용된 경우임.
    // 이 경우 node change가 일어남.
    // ex) sum(i1) or to_date(i1, 'yy-mm-dd')
    if( (sModule != &mtfList) || (aNode[0]->node.module == NULL) )
    {
        // node의 module이 null이 아닌 경우는 단일 expression인 경우.
        // ex) sum(i1)
        //
        // i1자체가 달려있기 때문에 다음과 같이 새로 node를 생성.
        //
        // i1   =>   ( )
        //            |
        //            i1
        if( aNode[0]->node.module != NULL )
        {
            sNode    = &aNode[0]->node;

            // Node 생성
            IDU_LIMITPOINT("qtc::changeNode::malloc");
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
                     != IDE_SUCCESS);

            // Node 초기화
            QTC_NODE_INIT( aNode[0] );
            aNode[1] = NULL;

            aNode[0]->node.arguments      = sNode;
            aNode[0]->node.lflag          = 1;
        }

        // node의 module이 null인 경우는 list형식으로 expression이 매달려 있는 경우.
        // node의 module이 null이 아닌 경우 이미 위에서 빈 node를 만들어 놓았음.
        // ex) to_date( i1, 'yy-mm-dd' )
        //
        // list형식인 경우 최상위에 빈 node가 달려있으므로
        // 다음과 같이 빈 node를 change
        //
        //  ( )                    to_date
        //   |                =>     |
        //   i1 - 'yy-mm-dd'        i1 - 'yy-mm-dd'
        aNode[0]->columnName       = *aPosition;

        aNode[0]->node.lflag  |= sModule->lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
        aNode[0]->node.module  = sModule;

        if( &qtc::spFunctionCallModule == aNode[0]->node.module )
        {
            // fix BUG-14257
            aNode[0]->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
            aNode[0]->lflag |= QTC_NODE_PROC_FUNCTION_TRUE;
        }

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );
    }
    else
    {
        // BUG-38935 display name을 위해 설정한다.
        aNode[0]->node.lflag &= ~MTC_NODE_REMOVE_ARGUMENTS_MASK;
        aNode[0]->node.lflag |= MTC_NODE_REMOVE_ARGUMENTS_TRUE;
    }
    
    aNode[0]->position.stmtText = aPosition->stmtText;
    aNode[0]->position.offset   = aPosition->offset;
    aNode[0]->position.size     = aPositionEnd->offset + aPositionEnd->size
                                - aPosition->offset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::changeNodeByModule( qcStatement*    aStatement,
                                qtcNode**       aNode,
                                mtfModule*      aModule,
                                qcNamePosition* aPosition,
                                qcNamePosition* aPositionEnd )
{
/***********************************************************************
 *
 * Description : BUG-41310
 *
 *    List Expression의 정보를 재조정한다.
 *
 * Implementation :
 *
 *    List Expression의 의미를 갖도록 정보를 Setting하며,
 *    해당 String의 위치를 재조정한다.
 *
 ***********************************************************************/
    const mtfModule* sModule;
    mtcNode*         sNode;

    IDE_DASSERT( aModule != NULL );

    sModule = aModule;

    // sModule이 mtfList가 아닌 경우는 함수형식으로 사용된 경우임.
    // 이 경우 node change가 일어남.
    // ex) sum(i1) or to_date(i1, 'yy-mm-dd')
    if ( ( sModule != &mtfList ) || ( aNode[0]->node.module == NULL ) )
    {
        // node의 module이 null이 아닌 경우는 단일 expression인 경우.
        // ex) sum(i1)
        //
        // i1자체가 달려있기 때문에 다음과 같이 새로 node를 생성.
        //
        // i1   =>   ( )
        //            |
        //            i1
        if ( aNode[0]->node.module != NULL )
        {
            sNode    = &aNode[0]->node;

            // Node 생성
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ), qtcNode, &( aNode[0] ) )
                     != IDE_SUCCESS );

            // Node 초기화
            QTC_NODE_INIT( aNode[0] );
            aNode[1] = NULL;

            aNode[0]->node.arguments      = sNode;
            aNode[0]->node.lflag          = 1;
        }

        // node의 module이 null인 경우는 list형식으로 expression이 매달려 있는 경우.
        // node의 module이 null이 아닌 경우 이미 위에서 빈 node를 만들어 놓았음.
        // ex) to_date( i1, 'yy-mm-dd' )
        //
        // list형식인 경우 최상위에 빈 node가 달려있으므로
        // 다음과 같이 빈 node를 change
        //
        //  ( )                    to_date
        //   |                =>     |
        //   i1 - 'yy-mm-dd'        i1 - 'yy-mm-dd'
        aNode[0]->columnName       = *aPosition;

        aNode[0]->node.lflag  |= sModule->lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
        aNode[0]->node.module  = sModule;

        if( &qtc::spFunctionCallModule == aNode[0]->node.module )
        {
            // fix BUG-14257
            aNode[0]->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
            aNode[0]->lflag |= QTC_NODE_PROC_FUNCTION_TRUE;
        }

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM( aStatement ),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE( aStatement ),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );
    }
    else
    {
        // BUG-38935 display name을 위해 설정한다.
        aNode[0]->node.lflag &= ~MTC_NODE_REMOVE_ARGUMENTS_MASK;
        aNode[0]->node.lflag |= MTC_NODE_REMOVE_ARGUMENTS_TRUE;
    }
    
    aNode[0]->position.stmtText = aPosition->stmtText;
    aNode[0]->position.offset   = aPosition->offset;
    aNode[0]->position.size     = aPositionEnd->offset + aPositionEnd->size
                                - aPosition->offset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::changeNodeForMemberFunc( qcStatement    * aStatement,
                                     qtcNode       ** aNode,
                                     qcNamePosition * aPositionStart,
                                     qcNamePosition * aPositionEnd,
                                     qcNamePosition * aUserNamePos,
                                     qcNamePosition * aTableNamePos,
                                     qcNamePosition * aColumnNamePos,
                                     qcNamePosition * aPkgNamePos,
                                     idBool           aIsBracket )
{
/**********************************************************************************
 *
 * Description : PROJ-1075
 *    List Expression의 정보를 재조정한다.
 *    일반 function은 찾지 않고 항상 member function을 먼저 검색한다.
 *
 * Implementation :
 *    List Expression의 의미를 갖도록 정보를 Setting하며,
 *    해당 String의 위치를 재조정한다.
 *    이 함수로 올 수 있는 경우의 함수 유형
 *        (1) arrvar_name.memberfunc_name( list_expr )  - aUserNamePos(X)
 *        (2) user_name.spfunc_name( list_expr )        - aUserNamePos(X)
 *        (3) label_name.arrvar_name.memberfunc_name( list_expr ) - 모두 존재
 *        (4) user_name.package_name.spfunc_name( list_exor ) - 모두 존재
 *        // BUG-38243 support array method at package
 *        (5) user_name.package_name.arrvar_name.memberfunc_name( list_expr )
 *             -> (5)는 항상 member func일 수 밖에 없음
 *********************************************************************************/

    const mtfModule* sModule;
    mtcNode*         sNode;
    idBool           sExist;
    SChar            sBuffer[1024];
    SChar            sNameBuffer[256];
    UInt             sLength;

    if ( aIsBracket == ID_FALSE )
    {
        // 적합성 검사. 적어도 tableName, columnName이 존재해야 함.
        IDE_DASSERT( QC_IS_NULL_NAME( (*aTableNamePos) ) == ID_FALSE );
        IDE_DASSERT( QC_IS_NULL_NAME( (*aColumnNamePos) ) == ID_FALSE );

        if ( QC_IS_NULL_NAME((*aPkgNamePos)) == ID_TRUE )
        {
            // spFunctionCall이 될 수도 있음.
            IDE_TEST( qsf::moduleByName( &sModule,
                                         &sExist,
                                         (void*)(aColumnNamePos->stmtText +
                                                 aColumnNamePos->offset),
                                         aColumnNamePos->size )
                      != IDE_SUCCESS );

            if ( ( sExist == ID_FALSE ) ||
                 ( sModule == &qsfMCountModule ) ||
                 ( sModule == &qsfMFirstModule ) ||
                 ( sModule == &qsfMLastModule ) ||
                 ( sModule == &qsfMNextModule ) )
            {
                sModule = & qtc::spFunctionCallModule;
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            // (5)의 경우 무조건 member function이어야만 함
            IDE_TEST( qsf::moduleByName( &sModule,
                                         &sExist,
                                         (void*)(aPkgNamePos->stmtText +
                                                 aPkgNamePos->offset),
                                         aPkgNamePos->size )
                      != IDE_SUCCESS );

            if ( sExist == ID_FALSE )
            {
                sLength = aPkgNamePos->size;

                if ( sLength >= ID_SIZEOF(sNameBuffer) )
                {
                    sLength = ID_SIZEOF(sNameBuffer) - 1;
                }
                else
                {
                    /* Nothing to do. */
                }

                idlOS::memcpy( sNameBuffer,
                               (SChar*) aPkgNamePos->stmtText + aPkgNamePos->offset,
                               sLength );
                sNameBuffer[sLength] = '\0';
                idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                                 "(Name=\"%s\")", sNameBuffer );

                IDE_RAISE( ERR_NOT_FOUND );
            }
            else
            {
                /* Nothing to do. */
            }
        }

        // node의 module이 null이 아닌 경우는 단일 expression인 경우.
        // ex) sum(i1)
        //
        // i1자체가 달려있기 때문에 다음과 같이 새로 node를 생성.
        //
        // i1   =>   ( )
        //            |
        //            i1
        if ( aNode[0]->node.module != NULL )
        {
            sNode    = &aNode[0]->node;

            // Node 생성
            IDU_LIMITPOINT("qtc::changeNodeForMemberFunc::malloc");
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &(aNode[0]))
                     != IDE_SUCCESS);

            // Node 초기화
            QTC_NODE_INIT( aNode[0] );
            aNode[1] = NULL;

            // Node 정보 설정
            aNode[0]->node.arguments      = sNode;
            aNode[0]->node.lflag          = 1;
        }
        else
        {
            // Nothing to do.
        }

        // node의 module이 null인 경우는 list형식으로 expression이 매달려 있는 경우.
        // node의 module이 null이 아닌 경우 이미 위에서 빈 node를 만들어 놓았음.
        // ex) to_date( i1, 'yy-mm-dd' )
        //
        // list형식인 경우 최상위에 빈 node가 달려있으므로
        // 다음과 같이 빈 node를 change
        //
        //  ( )                    to_date
        //   |                =>     |
        //   i1 - 'yy-mm-dd'        i1 - 'yy-mm-dd'
        aNode[0]->node.lflag  |= sModule->lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
        aNode[0]->node.module  = sModule;

        if ( &qtc::spFunctionCallModule == aNode[0]->node.module )
        {
            // fix BUG-14257
            aNode[0]->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
            aNode[0]->lflag |= QTC_NODE_PROC_FUNCTION_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );
    } // aIsBracket == ID_FALSE
    else // aIsBracket == ID_TRUE
    {
        aNode[0]->node.module = &qtc::columnModule;
        aNode[0]->node.lflag |= qtc::columnModule.lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
    }

    aNode[0]->userName   = *aUserNamePos;
    aNode[0]->tableName  = *aTableNamePos;
    aNode[0]->columnName = *aColumnNamePos;
    aNode[0]->pkgName    = *aPkgNamePos;

    aNode[0]->position.stmtText = aPositionStart->stmtText;
    aNode[0]->position.offset   = aPositionStart->offset;
    aNode[0]->position.size     = aPositionEnd->offset + aPositionEnd->size - aPositionStart->offset;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_FUNCTION_MODULE_NOT_FOUND,
                                  sBuffer ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::changeIgnoreNullsNode( qtcNode     * aNode,
                                   idBool      * aChanged )
{
/***********************************************************************
 *
 * Description :
 *
 *    analytic function의 ignore nulls 함수로 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::changeIgnoreNullsNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SChar            sModuleName[QC_MAX_OBJECT_NAME_LEN + 1];
    const mtfModule* sModule;
    idBool           sExist;

    idlOS::snprintf( sModuleName,
                     QC_MAX_OBJECT_NAME_LEN + 1,
                     "%s_IGNORE_NULLS",
                     (SChar*) aNode->node.module->names->string );

    IDE_TEST( mtf::moduleByName( &sModule,
                                 &sExist,
                                 (void*)sModuleName,
                                 idlOS::strlen(sModuleName) )
              != IDE_SUCCESS );

    if ( sExist == ID_TRUE )
    {
        aNode->node.module = sModule;
        
        *aChanged = ID_TRUE;
    }
    else
    {
        *aChanged = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::changeWithinGroupNode( qcStatement * aStatement,
                                   qtcNode     * aNode,
                                   qtcOver     * aOver )
{
/*************************************************************************
 *
 * Description :
 *    BUG-41631
 *    같은 이름의 Within Group Aggregation 함수로 변경한다.
 *
 *    ------------------------------------------------------------------------
 *    BUG-41771 PERCENT_RANK WITHIN GROUP
 *      RANK 에 대해 보면,
 *        RANK(..) WITHIN GROUP --> 함수명 : "RANK_WITHIN_GROUP"
 *                                  모듈명 : mtfRankWithinGroup
 *        RANK() OVER()         --> 함수명 : "RANK"
 *                                  모듈명 : mtfRank
 *
 *       RANK는 WITHIIN GROUP 절과 사용 시, 그것의 모듈이 이곳에서,
 *       mtfRank -> mtfRankWithinGroup
 *       이렇게 교체된다.
 *
 *       그러나, PERCENT_RANK 는
 *       RANK 처럼 Over 절을 사용하는, Analytic 모듈이 없어, 바로 mtfPercentRankWithinGroup 이
 *       달리게 되어 이곳에서 모듈을 교체할 필요가 없다.(이것밖에 없으니 할수도 없다.)
 *       함수명을 'PERCENT_RANK_WITHIN_GROUP' 이 아닌 'PERCENT_RANK' 로 한 이유는,
 *       파서가 sql 문에서 함수명을 얻도록 하기 위함이다.
 *
 *       PERCENT_RANK() OVER(..)  함수가 추가된다면,
 *         PERCENT_RANK() OVER(..)           --> 함수명 : "PERCENT_RANK"
 *                                               모듈명 : mtfPercentRank
 *         PERCENT_RANK(..) WITHIN GROUP(..) --> 함수명 : "PERCENT_RANK_WITHIN_GROUP"
 *                                               모듈명 : mtfPercentRankWithinGroup
 *       이런 형태로 작업한다.
 *
 *       이후에는, PERCENT_RANK 에 대한 기본 모듈은,
 *       WITHIN GROUP 절/OVER 절 구분없이 mtfPercentRank 가 달리게 되며,
 *       따라서, WITHIN GROUP 절과 사용되면 이곳에서 mtfPercentRankWithinGroup 로
 *       교체해준다.
 *
 *       PERCENT_RANK() OVER(..)
 *             |
 *             |
 *          mtfPercentRank
 *
 *       PERCENT_RANK(..) WITHIN GROUP(..)
 *             |
 *             |
 *          mtfPercentRank --> mtfPercentRankWithinGroup
 *
 *       ex)
 *       if ( aNode->node.module == &mtfRank )
 *       {
 *           sModule = &mtfRankWithinGroup;
             sChanged = ID_TRUE;
 *       }
 *       else if ( aNode->node.module == &mtfPercentRank )
 *       {
 *           sModule = &mtfPercentRankWithinGroup;
 *           sChanged = ID_TRUE;
 *       }
 *       else
 *       {
 *           // Nothing to do
 *       }
 *
 *    ------------------------------------------------------------------------
 *    BUG-41800 CUME_DIST WITHIN GROUP
 *      PERCENT_RANK 와 마찬가지로,
 *        CUME_DIST() OVER(..)  함수가 추가된다면,
 *          CUME_DIST() OVER(..)           --> 함수명 : "CUME_DIST"
 *                                             모듈명 : mtfCumeDist
 *          CUME_DIST(..) WITHIN GROUP(..) --> 함수명 : "CUME_DIST_WITHIN_GROUP"
 *                                             모듈명 : mtfCumeDistWithinGroup
 *       이런 형태로 작업한다.
 *
 *       이후에는, CUME_DIST 에 대한 기본 모듈은,
 *       WITHIN GROUP 절/OVER 절 구분없이 mtfCumeDist 가 달리게 되며,
 *       따라서, WITHIN GROUP 절과 사용되면 이곳에서 mtfCumeDistWithinGroup 로
 *       교체해준다.
 *
 *       CUME_DIST() OVER(..)
 *             |
 *             |
 *          mtfCumeDist
 *
 *       CUME_DIST(..) WITHIN GROUP(..)
 *             |
 *             |
 *          mtfCumeDist --> mtfCumeDistWithinGroup
 *
 *       ex)
 *       if ( aNode->node.module == &mtfRank )
 *       {
 *           sModule = &mtfRankWithinGroup;
             sChanged = ID_TRUE;
 *       }
 *       else if ( aNode->node.module == &mtfPercentRank )
 *       {
 *           sModule = &mtfPercentRankWithinGroup;
 *           sChanged = ID_TRUE;
 *       }
 *       else if ( aNode->node.module == &mtfCumeDist )
 *       {
 *           sModule = &mtfCumeDistWithinGroup;
 *           sChanged = ID_TRUE;
 *       }
 *
 * Implementation :
 *
 **************************************************************************/

    const mtfModule *   sModule;
    idBool              sChanged;
    ULong               sNodeArgCnt;
    qcuSqlSourceInfo    sqlInfo;

    sChanged  = ID_FALSE;

    if ( aNode->node.module == &mtfRank )
    {
        sModule = &mtfRankWithinGroup;
        sChanged = ID_TRUE;
    }
    else
    {
        // Nothing to do
    }

    if ( sChanged == ID_TRUE )
    {
        // 인자 갯수 백업
        sNodeArgCnt = aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

        // 이전 module Flag 제거
        aNode->node.lflag &= ~(aNode->node.module->lflag);

        // 새 module flag 설치
        aNode->node.lflag |= sModule->lflag & ~MTC_NODE_ARGUMENT_COUNT_MASK;

        // 백업된 인자 갯수 복원
        aNode->node.lflag |= sNodeArgCnt;

        aNode->node.module = sModule;
    }
    else
    {
        // Nothing to do
    }

    if ( aOver != NULL )
    {
        if ( qtc::canHasOverClause( aNode ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aNode->position );
            IDE_RAISE( ERR_NOT_ALLOWED_OVER_CLAUSE );
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOWED_OVER_CLAUSE );
    {
        (void)sqlInfo.init( QC_QMP_MEM( aStatement ) );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_OVER_CLAUSE_NOT_ALLOWED,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qtc::canHasOverClause( qtcNode * aNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    Over (..) 구문을 사용할 수 있는 Within Group 함수인지 확인한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool  sAllowed = ID_TRUE;

    if ( (aNode->node.module == &mtfRankWithinGroup) ||
         (aNode->node.module == &mtfPercentRankWithinGroup) ||
         (aNode->node.module == &mtfCumeDistWithinGroup) )
    {
        sAllowed = ID_FALSE;
    }
    else
    {
        // Nothing to do
    }

    return sAllowed;
}

IDE_RC qtc::getBigint( SChar*          aStmtText,
                       SLong*          aValue,
                       qcNamePosition* aPosition )
{
/***********************************************************************
 *
 * Description :
 *
 *    주어진 String을 이용해 BIGINT Value를 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::getBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    extern mtdModule mtdBigint;

    mtcColumn sColumn;
    UInt      sOffset;
    IDE_RC    sResult;

    sOffset = 0;

    // To Fix BUG-12607
    IDE_TEST( mtc::initializeColumn( & sColumn,
                                     & mtdBigint,
                                     0,           // arguments
                                     0,           // precision
                                     0 )          // scale
              != IDE_SUCCESS );

    IDE_TEST( mtdBigint.value( NULL,
                               &sColumn,
                               (void*)aValue,
                               &sOffset,
                               ID_SIZEOF(SLong),
                               (void*)(aStmtText+aPosition->offset),
                               aPosition->size,
                               &sResult )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sResult != IDE_SUCCESS, ERR_NOT_APPLICABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_FATAL_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::fixAfterValidation( iduVarMemList * aMemory,
                                qcTemplate    * aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    Validation 완료 후의 처리
 *
 * Implementation :
 *
 *    Intermediate Tuple에 대하여 row 공간을 할당받는다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::fixAfterValidation"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UShort        sRow     = 0;
    UShort        sColumn  = 0;
    UInt          sOffset  = 0;
    ULong         sRowSize = ID_ULONG(0);

    void        * sRowPtr;
    mtdDateType * sValue;
    mtcColumn   * sSysdateColumn;

    for( sRow = 0; sRow < aTemplate->tmplate.rowCount; sRow++ )
    {
        if( ( aTemplate->tmplate.rows[sRow].lflag &
              ( MTC_TUPLE_ROW_SKIP_MASK | MTC_TUPLE_TYPE_MASK ) ) ==
            ( MTC_TUPLE_ROW_SKIP_FALSE | MTC_TUPLE_TYPE_INTERMEDIATE ) )
        {
            aTemplate->tmplate.rows[sRow].lflag |= MTC_TUPLE_ROW_SKIP_TRUE;

            for( sColumn = 0, sOffset = 0;
                 sColumn < aTemplate->tmplate.rows[sRow].columnCount;
                 sColumn++ )
            {
                if( aTemplate->tmplate.rows[sRow].columns[sColumn].module != NULL )
                {
                    sOffset = idlOS::align( sOffset,
                                            aTemplate->tmplate.rows[sRow].
                                            columns[sColumn].module->align );
                }
                else
                {
                    // 프로시저의 0번 tmplate에는 초기화 되지 않은 컬럼이
                    // 존재할 수 있다.
                    // 컬럼 할당시에 cralloc을 사용하기 때문에 module은
                    // NULL의 값을 갖게 된다.

                    // 예)
                    // create or replace procedure proc1 as
                    // v1 integer;
                    // v2 integer;
                    // begin
                    // select * from t1 where a = v1 and b = v2;
                    // end;
                    // /
                    continue;
                }
                aTemplate->tmplate.rows[sRow].columns[sColumn].column.offset =
                        sOffset;

                sRowSize = (ULong)sOffset + (ULong)aTemplate->tmplate.rows[sRow].columns[sColumn].column.size;
                IDE_TEST_RAISE( sRowSize > (ULong)ID_UINT_MAX, ERR_EXCEED_TUPLE_ROW_MAX_SIZE );

                sOffset = (UInt)sRowSize;

                // To Fix PR-8528
                // 임의로 생성되는 Tuple의 경우 Column ID를 임의로 설정한다.
                // 절대 존재할 수 없는 Table의 ID(0)를 기준으로 설정한다.
                aTemplate->tmplate.rows[sRow].columns[sColumn].column.id =
                        sColumn;
            }

            aTemplate->tmplate.rows[sRow].rowOffset  =
            aTemplate->tmplate.rows[sRow].rowMaximum = sOffset;

            if( sOffset != 0 )
            {
                // BUG-15548
                IDU_LIMITPOINT("qtc::fixAfterValidation::malloc2");
                IDE_TEST(
                    aMemory->cralloc(
                        sOffset,
                        (void**)&(aTemplate->tmplate.rows[sRow].row))
                    != IDE_SUCCESS);
            }
            else
            {
                aTemplate->tmplate.rows[sRow].row = NULL;
            }
        }
    }

    aTemplate->tmplate.rowCountBeforeBinding = aTemplate->tmplate.rowCount;

    if( aTemplate->sysdate != NULL )
    {
        // fix BUG-10719 UMR
        sRowPtr = aTemplate->tmplate.rows[aTemplate->sysdate->node.table].row;
        sSysdateColumn = &(aTemplate->tmplate.
                           rows[aTemplate->sysdate->node.table].
                           columns[aTemplate->sysdate->node.column]);
        sValue = (mtdDateType*)mtc::value(
            sSysdateColumn, sRowPtr, MTD_OFFSET_USE );
        sValue->year         = 0;
        sValue->mon_day_hour = 0;
        sValue->min_sec_mic  = 0;
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_TUPLE_ROW_MAX_SIZE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtc::fixAfterValidation",
                                  "tuple row size is larger than 4GB" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

void qtc::fixAfterValidationForCreateInvalidView( qcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    Invalid View에 대한 처리
 *
 * Implementation :
 *
 *    불필요한 Memory를 할당받지 않도록 Intermediate Tuple 을 초기화한다.
 *
 ***********************************************************************/

    UShort  sRow;

    for( sRow = 0; sRow < aTemplate->tmplate.rowCount; sRow++ )
    {
        if( ( aTemplate->tmplate.rows[sRow].lflag &
              ( MTC_TUPLE_ROW_SKIP_MASK | MTC_TUPLE_TYPE_MASK ) ) ==
            ( MTC_TUPLE_ROW_SKIP_FALSE | MTC_TUPLE_TYPE_INTERMEDIATE ) )
        {
            /* BUG-40858 */
            if (aTemplate->sysdate != NULL)
            {
                if (aTemplate->sysdate->node.table != sRow)
                {
                    aTemplate->tmplate.rows[sRow].columnCount = 0;
                }
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                aTemplate->tmplate.rows[sRow].columnCount = 0;
            }
        }
    }
}

IDE_RC qtc::setVariableTupleRowSize( qcTemplate    * aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *
 *    Type Binding 완료 후의 처리
 *
 * Implementation :
 *
 *    Variable Tuple 의 row 의 offset 과 max size 를 세팅한다.
 *
 ***********************************************************************/

    UShort  sRow;
    UShort  sColumn;
    UInt    sOffset;

#ifdef DEBUG
    UShort  sCount;

    for( sRow = 0, sCount = 0; sRow < aTemplate->tmplate.rowCount; sRow++ )
    {
        if( ( aTemplate->tmplate.rows[sRow].lflag & MTC_TUPLE_TYPE_MASK ) ==
                  MTC_TUPLE_TYPE_VARIABLE )
        {
            sCount++;
        }
    }

    // Variable Tuple row count 는 항상 1 이다.
    IDE_DASSERT( sCount == 1 );
#endif

    sRow = aTemplate->tmplate.variableRow;

    for( sColumn = 0, sOffset = 0;
         sColumn < aTemplate->tmplate.rows[sRow].columnCount;
         sColumn++ )
    {
        IDE_TEST_RAISE( aTemplate->tmplate.rows[sRow].
                        columns[sColumn].module == NULL,
                        ERR_UNINITIALIZED_MODULE );

        sOffset = idlOS::align(
            sOffset,
            aTemplate->tmplate.rows[sRow].columns[sColumn].module->align );
        aTemplate->tmplate.rows[sRow].columns[sColumn].column.offset =
            sOffset;
        sOffset +=
            aTemplate->tmplate.rows[sRow].columns[sColumn].column.size;
    }

    aTemplate->tmplate.rows[sRow].rowOffset =
        aTemplate->tmplate.rows[sRow].rowMaximum = sOffset;

    aTemplate->tmplate.rows[sRow].row = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNINITIALIZED_MODULE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtc::setVariableTupleRowSize",
                                  "uninitialied module" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::estimateNode( qcStatement* aStatement,
                          qtcNode*     aNode )
{
/***********************************************************************
 *
 * Description :
 *    개념상 필요에 의해 추가되는 Node들에 대한 Estimation을 수행한다.
 *    예를 들어, Indirect Node, Prior Node 등이 이에 해당한다.
 *
 * Implementation :
 *    TODO - 개념상 Private Function으로 구현된 함수이다.
 *    외부에서 사용하지 않도록 해야 함.
 *    대체 함수로 다음 두 함수 중 하나를 선택하여 사용하도록 해야 함.
 *        - qtc::estimateNodeWithoutArgument()
 *        - qtc::estimateNodeWithArgument()
 *
 ***********************************************************************/

#define IDE_FN "qtc::estimateNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qtc::estimateNode"));

    qtcCallBackInfo sCallBackInfo = {
        QC_SHARED_TMPLATE(aStatement),
        QC_QMP_MEM(aStatement),
        NULL,
        NULL,
        NULL,
        NULL
    };
    mtcCallBack sCallBack = {
        &sCallBackInfo,
        MTC_ESTIMATE_ARGUMENTS_ENABLE, // TODO - DISABLE로 처리해야 할듯.
        qtc::alloc,
        NULL
    };

    IDE_TEST( qtc::estimateNode( aNode,
                                 & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                 QC_SHARED_TMPLATE(aStatement)->tmplate.stack,
                                 QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount,
                                 &sCallBack )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::estimateNodeWithoutArgument( qcStatement* aStatement,
                                         qtcNode*     aNode )
{
/***********************************************************************
 *
 * Description :
 *    Argument의 정보에 관계 없이 Estimate 를 수행한다.
 *    AND, OR등과 같이 Argument의 종류가 큰 의미를 갖지 않는
 *    Node들을 생성할 경우 이를 호출한다.
 *
 * Implementation :
 *
 *    TODO - 수정 검토 필요
 *    Argument를 참조하기 않기 때문에 Conversion등이 발생할 수 없다.
 *
 ***********************************************************************/

#define IDE_FN "qtc::estimateNodeWithoutArgument"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qtc::estimateNodeWithoutArgument"));

    qtcCallBackInfo sCallBackInfo = {
        QC_SHARED_TMPLATE(aStatement),  // Template
        QC_QMP_MEM(aStatement),   // Memory 관리자
        NULL,                 // Statement
        NULL,                 // Query Set
        NULL,                 // SFWGH
        NULL                  // From 정보
    };
    mtcCallBack sCallBack = {
        &sCallBackInfo,                  // CallBack 정보
        MTC_ESTIMATE_ARGUMENTS_DISABLE,  // Child에 대한 Estimation Disable
        qtc::alloc,                      // Memory 할당 함수
        initConversionNodeIntermediate   // Conversion Node 생성 함수
        // TODO - Conversion이 발생할 수 없다.
        // 따라서, Conversion Node 생성 함수들은 모두 NULL로 Setting되어야
        // 그 개념이 정확하다.
    };

    qtcNode * sNode;
    UInt      sArgCnt;
    ULong     sLflag;

    // Argument를 이용한 Dependencies 및 Flag 설정
    qtc::dependencyClear( & aNode->depInfo );

    // sLflag 초기화
    sLflag = MTC_NODE_BIND_TYPE_TRUE;

    for ( sNode = (qtcNode *) aNode->node.arguments, sArgCnt = 0;
          sNode != NULL;
          sNode = (qtcNode *) sNode->node.next, sArgCnt++ )
    {
        aNode->node.lflag |=
            sNode->node.lflag & aNode->node.module->lmask & MTC_NODE_MASK;
        aNode->lflag |= sNode->lflag & QTC_NODE_MASK;

        // PROJ-1404
        // variable built-in function을 사용한 경우 설정한다.
        if ( ( sNode->node.lflag & MTC_NODE_VARIABLE_MASK )
             == MTC_NODE_VARIABLE_TRUE )
        {
            aNode->lflag &= ~QTC_NODE_VAR_FUNCTION_MASK;
            aNode->lflag |= QTC_NODE_VAR_FUNCTION_EXIST;
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-1492
        // Argument의 bind관련 lflag를 모두 모은다.
        if( ( sNode->node.lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST )
        {
            if( ( sNode->node.lflag & MTC_NODE_BIND_TYPE_MASK ) ==
                MTC_NODE_BIND_TYPE_FALSE )
            {
                sLflag &= ~MTC_NODE_BIND_TYPE_MASK;
                sLflag |= MTC_NODE_BIND_TYPE_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }

        // Argument의 dependencies를 모두 포함한다.
        IDE_TEST( dependencyOr( & aNode->depInfo,
                                & sNode->depInfo,
                                & aNode->depInfo )
                  != IDE_SUCCESS );
    }

    if( ( ( aNode->node.lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) &&
        ( QTC_IS_TERMINAL( aNode ) == ID_FALSE ) )
    {
        // PROJ-1492
        // 하위 노드가 있는 해당 Node의 bind관련 lflag를 설정한다.
        aNode->node.lflag |= sLflag & MTC_NODE_BIND_TYPE_MASK;

        if( aNode->node.module == & mtfCast )
        {
            aNode->node.lflag |= MTC_NODE_BIND_TYPE_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    aNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
    if ( (aNode->node.module->lflag & MTC_NODE_LOGICAL_CONDITION_MASK)
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        aNode->node.lflag |= 1;
    }
    else
    {
        aNode->node.lflag |= sArgCnt;
    }

    IDE_TEST( qtc::estimateNode(aNode,
                                & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                QC_SHARED_TMPLATE(aStatement)->tmplate.stack,
                                QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount,
                                &sCallBack )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::estimateNodeWithArgument( qcStatement* aStatement,
                                      qtcNode*     aNode )
{
/***********************************************************************
 *
 * Description :
 *    이미 estimate 된 Argument의 정보를 이용하여 Estimate 한다.
 *    Argument에 대하여 추가적인 Conversion이 필요할 경우 사용한다.
 *
 * Implementation :
 *
 *    이미 estimate된 정보를 이용하여 처리하기 때문에,
 *    Argument에 대하여 traverse하면서 처리하지 않는다.
 *    TODO - 추후 Binding에 대한 고려가 필요하다.
 *
 ***********************************************************************/

#define IDE_FN "qtc::estimateNodeWithArgument"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qtc::estimateNodeWithArgument"));

    qtcCallBackInfo sCallBackInfo = {
        QC_SHARED_TMPLATE(aStatement),  // Template
        QC_QMP_MEM(aStatement),   // Memory 관리자
        NULL,                 // Statement
        NULL,                 // Query Set
        NULL,                 // SFWGH
        NULL                  // From 정보
    };
    mtcCallBack sCallBack = {
        &sCallBackInfo,                  // CallBack 정보
        MTC_ESTIMATE_ARGUMENTS_ENABLE,   // Child에 대한 Estimation이 필요함
        qtc::alloc,                      // Memory 할당 함수
        initConversionNodeIntermediate   // Conversion Node 생성 함수
    };

    qmsParseTree      * sParseTree;
    qmsSFWGH          * sSFWGH;
    qtcNode           * sNode;
    qtcNode           * sOrgNode;
    mtcStack          * sStack;
    mtcStack          * sOrgStack;
    SInt                sRemain;
    SInt                sOrgRemain;
    UInt                sArgCnt;
    ULong               sLflag;

    // estimate arguments
    for( sNode  = (qtcNode*)aNode->node.arguments,
             sStack = (QC_SHARED_TMPLATE(aStatement)->tmplate.stack) + 1,
             sRemain = (QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount) - 1;
         sNode != NULL;
         sNode  = (qtcNode*)sNode->node.next, sStack++, sRemain-- )
    {
        // BUG-33674
        IDE_TEST_RAISE( sRemain < 1, ERR_STACK_OVERFLOW );
        
        // INDIRECT 가 있는 경우 하위 Node의 column 정보를 사용해야 함
        // TODO - Argument에 이미 Conversion이 있을 경우에 대한 고려가 없음
        for( sOrgNode = sNode;
             ( sOrgNode->node.lflag & MTC_NODE_INDIRECT_MASK )
                 == MTC_NODE_INDIRECT_TRUE;
             sOrgNode = (qtcNode *) sOrgNode->node.arguments ) ;

        // PROJ-1718 Subquery unnesting
        // List type이 argument인 경우 반드시 argument도 estimate해야 한다.
        if( QTC_IS_LIST( sOrgNode ) == ID_TRUE )
        {
            sOrgStack  = QC_SHARED_TMPLATE(aStatement)->tmplate.stack;
            sOrgRemain = QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount;

            QC_SHARED_TMPLATE(aStatement)->tmplate.stack      = sStack;
            QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount = sRemain;

            if( sOrgNode->node.module == &mtfList )
            {
                // List
                IDE_TEST( estimateNodeWithArgument( aStatement,
                                                    sOrgNode )
                          != IDE_SUCCESS );
            }
            else
            {
                // BUG-38415 Subquery Argument
                // 이 경우, SELECT 구문이어야 한다.
                IDE_ERROR( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT );

                sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;
                sSFWGH     = sParseTree->querySet->SFWGH;

                IDE_TEST( estimateNodeWithSFWGH( aStatement,
                                                 sSFWGH,
                                                 sOrgNode )
                          != IDE_SUCCESS );
            }

            QC_SHARED_TMPLATE(aStatement)->tmplate.stack      = sOrgStack;
            QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount = sOrgRemain;
        }
        else
        {
            // Nothing to do.
        }

        sStack->column =
            QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sOrgNode->node.table].columns
            + sOrgNode->node.column;
    }

    // Argument를 이용한 Dependencies 및 Flag 설정
    qtc::dependencyClear( & aNode->depInfo );

    // sLflag 초기화
    sLflag = MTC_NODE_BIND_TYPE_TRUE;

    for ( sNode = (qtcNode *) aNode->node.arguments, sArgCnt = 0;
          sNode != NULL;
          sNode = (qtcNode *) sNode->node.next, sArgCnt++ )
    {
        aNode->node.lflag |=
            sNode->node.lflag & aNode->node.module->lmask & MTC_NODE_MASK;
        aNode->lflag |= sNode->lflag & QTC_NODE_MASK;

        // PROJ-1404
        // variable built-in function을 사용한 경우 설정한다.
        if ( ( sNode->node.lflag & MTC_NODE_VARIABLE_MASK )
             == MTC_NODE_VARIABLE_TRUE )
        {
            aNode->lflag &= ~QTC_NODE_VAR_FUNCTION_MASK;
            aNode->lflag |= QTC_NODE_VAR_FUNCTION_EXIST;
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-1492
        // Argument의 bind관련 lflag를 모두 모은다.
        if( ( sNode->node.lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST )
        {
            if( ( sNode->node.lflag & MTC_NODE_BIND_TYPE_MASK ) ==
                MTC_NODE_BIND_TYPE_FALSE )
            {
                sLflag &= ~MTC_NODE_BIND_TYPE_MASK;
                sLflag |= MTC_NODE_BIND_TYPE_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }

        // Argument의 dependencies를 모두 포함한다.
        IDE_TEST( dependencyOr( & aNode->depInfo,
                                & sNode->depInfo,
                                & aNode->depInfo )
                  != IDE_SUCCESS );
    }

    if( ( ( aNode->node.lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) &&
        ( QTC_IS_TERMINAL( aNode ) == ID_FALSE ) )
    {
        // PROJ-1492
        // 하위 노드가 있는 해당 Node의 bind관련 lflag를 설정한다.
        aNode->node.lflag |= sLflag & MTC_NODE_BIND_TYPE_MASK;

        if( aNode->node.module == & mtfCast )
        {
            aNode->node.lflag |= MTC_NODE_BIND_TYPE_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    aNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;

    if ( (aNode->node.module->lflag & MTC_NODE_LOGICAL_CONDITION_MASK)
         == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        aNode->node.lflag |= 1;
    }
    else
    {
        aNode->node.lflag |= sArgCnt;
    }

    // estimate root node
    IDE_TEST( qtc::estimateNode( aNode,
                                 & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                 QC_SHARED_TMPLATE(aStatement)->tmplate.stack,
                                 QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount,
                                 &sCallBack )
              != IDE_SUCCESS);

    // BUG-42113 LOB type 에 대한 subquery 변환이 수행되어야 합니다.
    // estimateNode 의 결과에 따라서 재 설정해주어야 한다.
    // Ex : LENGTH( lobColumn )
    aNode->lflag &= ~QTC_NODE_BINARY_MASK;

    if ( isEquiValidType( aNode, & QC_SHARED_TMPLATE(aStatement)->tmplate ) == ID_FALSE )
    {
        aNode->lflag |= QTC_NODE_BINARY_EXIST;
    }
    else
    {
        aNode->lflag |= QTC_NODE_BINARY_ABSENT;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::estimateWindowFunction( qcStatement * aStatement,
                                    qmsSFWGH    * aSFWGH,
                                    qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery unnesting
 *     Transformation 과정에서 새로 생성된 window function을
 *     estimation한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcCallBackInfo sCallBackInfo = {
        QC_SHARED_TMPLATE(aStatement),  // Template
        QC_QMP_MEM(aStatement),   // Memory 관리자
        NULL,                 // Statement
        aSFWGH->thisQuerySet, // Query Set
        NULL,                 // SFWGH
        NULL                  // From 정보
    };
    mtcCallBack sCallBack = {
        &sCallBackInfo,                  // CallBack 정보
        MTC_ESTIMATE_ARGUMENTS_ENABLE,   // Child에 대한 Estimation이 필요함
        qtc::alloc,                      // Memory 할당 함수
        initConversionNodeIntermediate   // Conversion Node 생성 함수
    };

    qtcNode         * sNode;
    qtcNode         * sOrgNode;
    mtcStack        * sStack;
    qmsProcessPhase   sProcessPhase;
    SInt              sRemain;
    UInt              sArgCnt;
    ULong             sLflag;

    IDE_ERROR( aNode->overClause != NULL );
    IDE_ERROR( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                    == MTC_NODE_OPERATOR_AGGREGATION );

    for( sNode  = (qtcNode*)aNode->node.arguments,
             sStack = (QC_SHARED_TMPLATE(aStatement)->tmplate.stack) + 1,
             sRemain = (QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount) - 1;
         sNode != NULL;
         sNode  = (qtcNode*)sNode->node.next, sStack++, sRemain-- )
    {
        // BUG-33674
        IDE_TEST_RAISE( sRemain < 1, ERR_STACK_OVERFLOW );
        
        // INDIRECT 가 있는 경우 하위 Node의 column 정보를 사용해야 함
        // TODO - Argument에 이미 Conversion이 있을 경우에 대한 고려가 없음
        for( sOrgNode = sNode;
             ( sOrgNode->node.lflag & MTC_NODE_INDIRECT_MASK )
                 == MTC_NODE_INDIRECT_TRUE;
             sOrgNode = (qtcNode *) sOrgNode->node.arguments ) ;

        sStack->column =
            QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sOrgNode->node.table].columns
            + sOrgNode->node.column;
    }

    // Argument를 이용한 Dependencies 및 Flag 설정
    qtc::dependencyClear( & aNode->depInfo );

    // sLflag 초기화
    sLflag = MTC_NODE_BIND_TYPE_TRUE;

    for ( sNode = (qtcNode *) aNode->node.arguments, sArgCnt = 0;
          sNode != NULL;
          sNode = (qtcNode *) sNode->node.next, sArgCnt++ )
    {
        aNode->node.lflag |=
            sNode->node.lflag & aNode->node.module->lmask & MTC_NODE_MASK;
        aNode->lflag |= sNode->lflag & QTC_NODE_MASK;

        // PROJ-1404
        // variable built-in function을 사용한 경우 설정한다.
        if ( ( sNode->node.lflag & MTC_NODE_VARIABLE_MASK )
             == MTC_NODE_VARIABLE_TRUE )
        {
            aNode->lflag &= ~QTC_NODE_VAR_FUNCTION_MASK;
            aNode->lflag |= QTC_NODE_VAR_FUNCTION_EXIST;
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-1492
        // Argument의 bind관련 lflag를 모두 모은다.
        if( ( sNode->node.lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST )
        {
            if( ( sNode->node.lflag & MTC_NODE_BIND_TYPE_MASK ) ==
                MTC_NODE_BIND_TYPE_FALSE )
            {
                sLflag &= ~MTC_NODE_BIND_TYPE_MASK;
                sLflag |= MTC_NODE_BIND_TYPE_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }

        // Argument의 dependencies를 모두 포함한다.
        IDE_TEST( dependencyOr( & aNode->depInfo,
                                & sNode->depInfo,
                                & aNode->depInfo )
                  != IDE_SUCCESS );
    }

    if( ( ( aNode->node.lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) &&
        ( QTC_IS_TERMINAL( aNode ) == ID_FALSE ) )
    {
        // PROJ-1492
        // 하위 노드가 있는 해당 Node의 bind관련 lflag를 설정한다.
        aNode->node.lflag |= sLflag & MTC_NODE_BIND_TYPE_MASK;

        if( aNode->node.module == & mtfCast )
        {
            aNode->node.lflag |= MTC_NODE_BIND_TYPE_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    aNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
    aNode->node.lflag |= sArgCnt;

    // estimate root node
    IDE_TEST( qtc::estimateNode( aNode,
                                 & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                 QC_SHARED_TMPLATE(aStatement)->tmplate.stack,
                                 QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount,
                                 &sCallBack )
              != IDE_SUCCESS);

    // Window function은 SELECT/ORDER BY절에서만 사용 가능하므로
    // 일시적으로 현재의 validation 단계를 SELECT절로 전환한다.
    sProcessPhase = aSFWGH->thisQuerySet->processPhase;
    aSFWGH->thisQuerySet->processPhase = QMS_VALIDATE_TARGET;

    IDE_TEST( estimate4OverClause( aNode,
                                   & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                   QC_SHARED_TMPLATE(aStatement)->tmplate.stack,
                                   QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount,
                                   &sCallBack )
              != IDE_SUCCESS );

    aNode->lflag &= ~QTC_NODE_ANAL_FUNC_MASK;
    aNode->lflag |= QTC_NODE_ANAL_FUNC_EXIST;

    IDE_TEST( estimateAggregation( aNode, &sCallBack )
              != IDE_SUCCESS );

    aSFWGH->thisQuerySet->processPhase = sProcessPhase;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::estimateConstExpr( qtcNode     * aNode,
                               qcTemplate  * aTemplate,
                               qcStatement * aStatement )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                최상위 노드에 대한 constant expression처리
 *
 *  Implementation :
 *                최상위 노드가 aggregation이 아닌 경우
 *                constant expression처리가 가능하면 처리
 *
 ***********************************************************************/

    qtcNode           * sResultNode;
    qtcNode           * sOrgNode;
    idBool              sAbleProcess;

    qtcCallBackInfo sCallBackInfo = {
        aTemplate,
        QC_QMP_MEM(aTemplate->stmt),
        aStatement,
        NULL,
        NULL,
        NULL
    };

    mtcCallBack sCallBack = {
        &sCallBackInfo,
        MTC_ESTIMATE_ARGUMENTS_ENABLE,
        alloc,
        initConversionNodeIntermediate
    };

    if ( ( MTC_NODE_IS_DEFINED_VALUE( & aNode->node ) == ID_TRUE )
         &&
         (aNode->subquery == NULL) // : A4 only
         // ( (aNode->flag & QTC_NODE_SUBQUERY_MASK )
         //   != QTC_NODE_SUBQUERY_EXIST ) // : A3 only
         // PSM function
         &&
         (aNode->node.module != &mtfList) )
    {
        // Argument의 적합성 검사
        IDE_TEST( isConstExpr( aTemplate, aNode, &sAbleProcess )
                  != IDE_SUCCESS );

        if ( sAbleProcess == ID_TRUE )
        {
            // 상수화하기 전 원래 노드를 백업 한다.
            IDU_LIMITPOINT("qtc::estimateConstExpr::malloc");
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qtcNode, & sOrgNode )
                      != IDE_SUCCESS);

            idlOS::memcpy( sOrgNode, aNode, ID_SIZEOF(qtcNode) );

            // 상수 Expression을 수행함
            IDE_TEST( runConstExpr( aStatement,
                                    aTemplate,
                                    aNode,
                                    &aTemplate->tmplate,
                                    aTemplate->tmplate.stack,
                                    aTemplate->tmplate.stackRemain,
                                    &sCallBack,
                                    & sResultNode ) != IDE_SUCCESS );

            sResultNode->node.next = aNode->node.next;
            sResultNode->node.conversion = NULL;

//          sResultNode->node.arguments = aNode->node.arguments;
            sResultNode->node.arguments = NULL;
            sResultNode->node.leftConversion =
                aNode->node.leftConversion;
            sResultNode->node.orgNode =
                (mtcNode*) sOrgNode;
            sResultNode->node.funcArguments =
                aNode->node.funcArguments;
            sResultNode->node.baseTable =
                aNode->node.baseTable;
            sResultNode->node.baseColumn =
                aNode->node.baseColumn;

            //fix BUG-18119
            if( aNode != sResultNode )
            {
                idlOS::memcpy(aNode, sResultNode, ID_SIZEOF(qtcNode));
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC qtc::estimate( qtcNode*     aNode,
                      qcTemplate*  aTemplate,
                      qcStatement* aStatement,
                      qmsQuerySet* aQuerySet,
                      qmsSFWGH*    aSFWGH,
                      qmsFrom*     aFrom )
{
/***********************************************************************
 *
 * Description :
 *
 *    Node Tree에 대한 Estimation을 수행함.
 *
 * Implementation :
 *
 *    Node Tree를 모두 Traverse하면서 Estimate 를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::estimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcCallBackInfo sCallBackInfo = {
        aTemplate,
        QC_QMP_MEM(aTemplate->stmt),
        aStatement,
        aQuerySet,
        aSFWGH,
        aFrom
    };
    mtcCallBack sCallBack = {
        &sCallBackInfo,
        MTC_ESTIMATE_ARGUMENTS_ENABLE,
        alloc,
        initConversionNodeIntermediate
    };

    IDE_TEST( estimateInternal( aNode,
                                & aTemplate->tmplate,
                                aTemplate->tmplate.stack,
                                aTemplate->tmplate.stackRemain,
                                &sCallBack )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::getSortColumnPosition( qmsSortColumns* aSortColumn,
                                   qcTemplate*     aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    ORDER BY indicator에 대한 정보를 설정함.
 *
 * Implementation :
 *
 *    ORDER BY Expression 이 value integer일 경우,
 *    이를 Indicator로 해석한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::getSortColumnPosition"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    extern mtdModule mtdSmallint;

    qtcNode*         sNode;
    const mtcColumn* sColumn;

    aSortColumn->targetPosition = -1;
    sNode                       = aSortColumn->sortColumn;

    if ( isConstValue( aTemplate, sNode ) == ID_TRUE )
    {
        sColumn = QTC_TMPL_COLUMN( aTemplate, sNode );
        
        if( sColumn->module == &mtdSmallint )
        {
            // To Fix PR-8015
            aSortColumn->targetPosition =
                *(SShort*)QTC_TMPL_FIXEDDATA( aTemplate, sNode );
        }
        else 
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

#undef IDE_FN
}


IDE_RC qtc::initialize( qtcNode*    aNode,
                        qcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    해당 Aggregation Node의 초기화를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::initialize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // BUG-33674
    IDE_TEST_RAISE( aTemplate->tmplate.stackRemain < 1, ERR_STACK_OVERFLOW );

    // BUG-34321 return value optimization
    return aTemplate->tmplate.rows[aNode->node.table].
        execute[aNode->node.column].
        initialize( (mtcNode*)aNode,
                    aTemplate->tmplate.stack,
                    aTemplate->tmplate.stackRemain,
                    aTemplate->tmplate.rows[aNode->node.table].
                        execute[aNode->node.column].
                        calculateInfo,
                    &aTemplate->tmplate );

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::aggregate( qtcNode*    aNode,
                       qcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    해당 Aggregation Node의 집합 연산을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::aggregate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // BUG-33674
    IDE_TEST_RAISE( aTemplate->tmplate.stackRemain < 1, ERR_STACK_OVERFLOW );

    // BUG-34321 return value optimization
    return aTemplate->tmplate.rows[aNode->node.table].
        execute[aNode->node.column].
        aggregate( (mtcNode*)aNode,
                   aTemplate->tmplate.stack,
                   aTemplate->tmplate.stackRemain,
                   aTemplate->tmplate.rows[aNode->node.table].
                       execute[aNode->node.column].
                       calculateInfo,
                   &aTemplate->tmplate );

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::aggregateWithInfo( qtcNode*    aNode,
                               void*       aInfo,
                               qcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    해당 Aggregation Node의 집합 연산을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::aggregate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // BUG-33674
    IDE_TEST_RAISE( aTemplate->tmplate.stackRemain < 1, ERR_STACK_OVERFLOW );

    // BUG-34321 return value optimization
    return aTemplate->tmplate.rows[aNode->node.table].
        execute[aNode->node.column].
        aggregate( (mtcNode*)aNode,
                   aTemplate->tmplate.stack,
                   aTemplate->tmplate.stackRemain,
                   aInfo,
                   &aTemplate->tmplate );

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::finalize( qtcNode*    aNode,
                      qcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    해당 Aggregation Node의 마무리 작업을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::finalize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // BUG-33674
    IDE_TEST_RAISE( aTemplate->tmplate.stackRemain < 1, ERR_STACK_OVERFLOW );

    // BUG-34321 return value optimization
    return aTemplate->tmplate.rows[aNode->node.table].
        execute[aNode->node.column].
        finalize( (mtcNode*)aNode,
                  aTemplate->tmplate.stack,
                  aTemplate->tmplate.stackRemain,
                  aTemplate->tmplate.rows[aNode->node.table].
                      execute[aNode->node.column].
                      calculateInfo,
                  &aTemplate->tmplate );

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::calculate(qtcNode* aNode, qcTemplate* aTemplate)
{
/***********************************************************************
 *
 * Description :
 *
 *    해당 Node의 연산을 수행한다.
 *
 * Implementation :
 *
 *    해당 Node의 연산을 수행하고,
 *    Conversion이 있을 경우, Conversion 처리를 수행한다.
 *
 ***********************************************************************/

    /*
      프로시저 실행시에 테이블의 컬럼 개수가 줄어든 경우 에러 처리한다.
      Assign 문의 경우 동적으로 형변환이 이루어 지기 때문에 호환되지 않는
      타입으로 변경되는 경우 실행중에 에러가 발생한다.

      예)
      drop table t2;
      create table t2 ( a integer, b integer );

      create or replace procedure proc1
      as
      v1 t2%rowtype;
      begin

      v1.a := 1;
      v1.b := 999;

      println( v1.a );
      println( v1.b );

      end;
      /

      drop table t2;
      create table t2 ( a integer );

      exec proc1;
    */

    // BUG-33674
    IDE_TEST_RAISE( aTemplate->tmplate.stackRemain < 1, ERR_STACK_OVERFLOW );

    if( aNode->node.column != MTC_RID_COLUMN_ID )
    {
        IDE_TEST_RAISE(aNode->node.column >=
                       aTemplate->tmplate.rows[aNode->node.table].columnCount,
                       ERR_MODIFIED);

        IDE_TEST(aTemplate->tmplate.rows[aNode->node.table].
                 execute[aNode->node.column].calculate(
                     (mtcNode*)aNode,
                     aTemplate->tmplate.stack,
                     aTemplate->tmplate.stackRemain,
                     aTemplate->tmplate.rows[aNode->node.table].
                         execute[aNode->node.column].
                         calculateInfo,
                     &aTemplate->tmplate)
                 != IDE_SUCCESS);
    }
    else
    {
        /* rid pseudo column */
        IDE_TEST(aTemplate->tmplate.rows[aNode->node.table].
                 ridExecute->calculate((mtcNode*)aNode,
                                       aTemplate->tmplate.stack,
                                       aTemplate->tmplate.stackRemain,
                                       NULL,
                                       &aTemplate->tmplate)
                 != IDE_SUCCESS);
    }

    if (aNode->node.conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( (mtcNode*)aNode,
                                         aTemplate->tmplate.stack,
                                         aTemplate->tmplate.stackRemain,
                                         NULL,
                                         &aTemplate->tmplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MODIFIED );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_NOT_EXISTS_COLUMN ) );
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::judge( idBool*     aJudgement,
                   qtcNode*    aNode,
                   qcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    해당 Node의 논리값을 판단한다.
 *
 * Implementation :
 *
 *    해당 Node의 연산을 수행하고,
 *    논리값이 TRUE인지를 판단한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::judge"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( calculate( aNode, aTemplate ) != IDE_SUCCESS );

    // BUG-34321 return value optimization
    return aTemplate->tmplate.stack->column->module->isTrue(
                        aJudgement,
                        aTemplate->tmplate.stack->column,
                        aTemplate->tmplate.stack->value );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/* PROJ-2209 DBTIMEZONE */
/***********************************************************************
 *
 * Description :
 *
 *    수행 시점의 System Time을 획득.
 *
 * Implementation :
 *
 *    gettimeofday()를 이용해 System Time을 획득하고,
 *    이를 Data Type의 정보로 변환시킴
 *
 *    1. unixdate    : gmtime_r() , makeDate()
 *    2. currentdate : gmtime_r() , makeDate() + addSecond( session tz offset )
 *    3. sysdate     : gmtime_r() , makeDate() + addSecond( os tz offset )
 *
 ***********************************************************************/
IDE_RC qtc::setDatePseudoColumn( qcTemplate * aTemplate )
{
    PDL_Time_Value sTimevalue;
    struct tm      sGlobaltime;
    time_t         sTime;
    mtcNode*       sNode;
    mtdDateType*   sDate = NULL;
    mtdDateType    sTmpdate;

    if ( ( aTemplate->unixdate != NULL ) ||
         ( aTemplate->sysdate  != NULL ) ||
         ( aTemplate->currentdate != NULL ) )
    {
        sTimevalue = idlOS::gettimeofday();

        sTime = (time_t)sTimevalue.sec();
        idlOS::gmtime_r( &sTime, &sGlobaltime );

        IDE_TEST( mtdDateInterface::makeDate( &sTmpdate,
                                              (SShort)sGlobaltime.tm_year + 1900,
                                              sGlobaltime.tm_mon + 1,
                                              sGlobaltime.tm_mday,
                                              sGlobaltime.tm_hour,
                                              sGlobaltime.tm_min,
                                              sGlobaltime.tm_sec,
                                              sTimevalue.usec() )
                  != IDE_SUCCESS );

        // set UNIX_DATE
        if ( aTemplate->unixdate != NULL )
        {
            sNode = &aTemplate->unixdate->node;
            sDate = (mtdDateType*)
                        ( (UChar*) aTemplate->tmplate.rows[sNode->table].row
                          + aTemplate->tmplate.rows[sNode->table].columns[sNode->column].column.offset );

            *sDate = sTmpdate;
        }
        else
        {
            /* nothing to do. */
        }

        // set CURRENT_DATE
        if ( aTemplate->currentdate != NULL )
        {
            sNode = &aTemplate->currentdate->node;
            sDate = (mtdDateType*)
                        ( (UChar*) aTemplate->tmplate.rows[sNode->table].row
                          + aTemplate->tmplate.rows[sNode->table].columns[sNode->column].column.offset );

            IDE_TEST( mtdDateInterface::addSecond( sDate,
                                                   &sTmpdate,
                                                   QCG_GET_SESSION_TIMEZONE_SECOND( aTemplate->stmt ),
                                                   0 )
            != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do
        }

        // set SYSDATE
        if ( aTemplate->sysdate != NULL )
        {
            sNode = &aTemplate->sysdate->node;
            sDate = (mtdDateType*)
                        ( (UChar*) aTemplate->tmplate.rows[sNode->table].row
                          + aTemplate->tmplate.rows[sNode->table].columns[sNode->column].column.offset );

            /* PROJ-2207 Password policy support
               sysdate_for_natc가 실패하면 sysdate를 반환한다. */
            if ( QCU_SYSDATE_FOR_NATC[0] != '\0' )
            {
                sDate->year         = 0;
                sDate->mon_day_hour = 0;
                sDate->min_sec_mic  = 0;

                IDE_TEST_CONT( mtdDateInterface::toDate( sDate,
                                                         (UChar*)QCU_SYSDATE_FOR_NATC,
                                                         idlOS::strlen(QCU_SYSDATE_FOR_NATC),
                                                         (UChar*)aTemplate->tmplate.dateFormat,
                                                         idlOS::strlen(aTemplate->tmplate.dateFormat) )
                               == IDE_SUCCESS, NORMAL_EXIT );
            }
            else
            {
                /* nothing to do. */
            }

            IDE_TEST( mtdDateInterface::addSecond( sDate,
                                                   &sTmpdate,
                                                   MTU_OS_TIMEZONE_SECOND,
                                                   0 )
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do. */
        }

    }
    else
    {
        /* nothing to do. */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qtc::sysdate( mtdDateType* aDate )
{
/***********************************************************************
 *
 * Description :
 *
 *    수행 시점의 System Time을 획득함.
 *
 * Implementation :
 *
 *    gettimeofday()를 이용해 System Time을 획득하고,
 *    이를 Data Type의 정보로 변환시킴
 *
 ***********************************************************************/

#define IDE_FN "void qtc::sysdate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    PDL_Time_Value sTimevalue;
    struct tm      sLocaltime;
    time_t         sTime;
    mtdDateType    sTmpdate;

    sTimevalue = idlOS::gettimeofday();
    sTime      = (time_t)sTimevalue.sec();
    idlOS::gmtime_r( &sTime, &sLocaltime );

    IDE_TEST( mtdDateInterface::makeDate( &sTmpdate,
                                          (SShort)sLocaltime.tm_year + 1900,
                                          sLocaltime.tm_mon + 1,
                                          sLocaltime.tm_mday,
                                          sLocaltime.tm_hour,
                                          sLocaltime.tm_min,
                                          sLocaltime.tm_sec,
                                          sTimevalue.usec() )
              != IDE_SUCCESS );

    IDE_TEST( mtdDateInterface::addSecond( aDate,
                                           &sTmpdate,
                                           MTU_OS_TIMEZONE_SECOND,
                                           0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC qtc::initSubquery( mtcNode*     aNode,
                          mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *
 *    Subquery Node의 Plan 초기화
 *
 * Implementation :
 *
 *
 ***********************************************************************/
#define IDE_FN "IDE_RC qtc::initSubquery"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmnPlan* sPlan;

    sPlan = ((qtcNode*)aNode)->subquery->myPlan->plan;

    return sPlan->init( (qcTemplate*)aTemplate, sPlan );

#undef IDE_FN
}

IDE_RC qtc::fetchSubquery( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           idBool*      aRowExist )
{
/***********************************************************************
 *
 * Description :
 *
 *    Subquery Node의 Plan 수행
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::fetchSubquery"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmnPlan*   sPlan;
    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;

    sPlan = ((qtcNode*)aNode)->subquery->myPlan->plan;

    IDE_TEST( sPlan->doIt( (qcTemplate*)aTemplate, sPlan, &sFlag )
              != IDE_SUCCESS );

    if( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        *aRowExist = ID_TRUE;
    }
    else
    {
        *aRowExist = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::finiSubquery( mtcNode*     /* aNode */,
                          mtcTemplate* /* aTemplate */)
{
/***********************************************************************
 *
 * Description :
 *
 *    Subquery Node의 Plan 종료
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::finiSubquery"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qtc::setCalcSubquery( mtcNode     * aNode,
                             mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-2448 Subquery caching
 *
 *    Subquery Node의 calculate 함수 설정
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_ERROR_RAISE( aNode->arguments != NULL, ERR_UNEXPECTED_CACHE_ERROR );
    IDE_ERROR_RAISE( aNode->arguments->module == &qtc::subqueryModule, ERR_UNEXPECTED_CACHE_ERROR );

    if ( aNode->module == &mtfExists )
    {
        aTemplate->rows[aNode->arguments->table].execute[aNode->arguments->column].calculate
            = qtcSubqueryCalculateExists;
    }
    else if ( aNode->module == &mtfNotExists )
    {
        aTemplate->rows[aNode->arguments->table].execute[aNode->arguments->column].calculate
            = qtcSubqueryCalculateNotExists;
    }
    else
    {
        IDE_ERROR_RAISE( 0, ERR_UNEXPECTED_CACHE_ERROR );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_CACHE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtc::setCalcSubquery",
                                  "Unexpected execution cache error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::cloneQTCNodeTree4Partition(
    iduVarMemList * aMemory,
    qtcNode*        aSource,
    qtcNode**       aDestination,
    UShort          aSourceTable,
    UShort          aDestTable,
    idBool          aContainRootsNext )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *    source(partitioned table)의 노드를 destination(partition) 노드로
 *    변환하여 복사함
 *
 *    - root의 next는 복사하지 않음
 *    - conversion은 clear
 *
 * Implementation :
 *
 *    Source Node Tree를 Traverse하면서 Node를 복사한다.
 *
 ***********************************************************************/

    qtcNode * sNode;

    IDU_LIMITPOINT("qtc::cloneQTCNodeTree4Partition::malloc");
    IDE_TEST(STRUCT_ALLOC(aMemory, qtcNode, &sNode)
             != IDE_SUCCESS);

    idlOS::memcpy( sNode, aSource, ID_SIZEOF(qtcNode) );

    sNode->node.conversion = NULL;
    sNode->node.leftConversion = NULL;

    if ((sNode->node.module == &qtc::columnModule) ||
        (sNode->node.module == &gQtcRidModule))
    {
        // source와 dest가 다를 때에만 수행.
        if( (sNode->node.table == aSourceTable) &&
            (aSourceTable != aDestTable) )
        {
            sNode->node.table = aDestTable;
        }
        else
        {
            // Nothing to do.
        }

        // BUG-27291
        // 이미 estimate했으므로 estimate할 필요없음.
        sNode->lflag &= ~QTC_NODE_COLUMN_ESTIMATE_MASK;
        sNode->lflag |= QTC_NODE_COLUMN_ESTIMATE_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    if( aSource->node.arguments != NULL )
    {
        if( ( aSource->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_SUBQUERY )
        {
            // Subquery노드일 경우엔 arguments를 복사하지 않는다.
        }
        else
        {
            IDE_TEST( cloneQTCNodeTree4Partition(
                          aMemory,
                          (qtcNode *)aSource->node.arguments,
                          (qtcNode **)(&(sNode->node.arguments)),
                          aSourceTable,
                          aDestTable,
                          ID_TRUE )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    // To fix BUG-21243
    // pass node는 복사하지 않고 삭제한다.
    // view target column을 복사하는 경우 passnode까지 복사되어서
    // view push selection을 하는 경우 잘못된 결과를 읽음.
    if( aSource->node.module == &qtc::passModule )
    {
        idlOS::memcpy( sNode,
                       sNode->node.arguments,
                       ID_SIZEOF(qtcNode) );
    }
    else
    {
        // Nothing to do.
    }
    
    if( aSource->node.next != NULL )
    {
        if( aContainRootsNext == ID_TRUE )
        {
            IDE_TEST( cloneQTCNodeTree4Partition(
                          aMemory,
                          (qtcNode *)aSource->node.next,
                          (qtcNode **)(&(sNode->node.next)),
                          aSourceTable,
                          aDestTable,
                          ID_TRUE )
                      != IDE_SUCCESS );
        }
        else
        {
            sNode->node.next = NULL;
        }
    }
    else
    {
        // Nothing to do.
    }
    
    *aDestination = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::cloneQTCNodeTree( iduVarMemList * aMemory,
                              qtcNode       * aSource,
                              qtcNode      ** aDestination,
                              idBool          aContainRootsNext,
                              idBool          aConversionClear,
                              idBool          aConstCopy,
                              idBool          aConstRevoke )
{
/***********************************************************************
 *
 * Description :
 *
 *    Source Node를 Desination으로 복사함.
 *
 *    - aContainRootsNext: ID_TRUE이면 aDestination의 next도 모두 복사
 *    - aConversionClear: ID_TRUE이면 conversion을 모두 NULL로 세팅
 *                        ( push selection에서 사용함)
 *    - aConstCopy       : ID_TRUE이면 constant node라도 복사한다.
 *    - aConstRevoke     : ID_TRUE이면 runConstExpr로 constant처리되기
 *                         전의 원래 노드로 원복하여 복사한다.
 *
 * Implementation :
 *
 *    Source Node Tree를 Traverse하면서 Node를 복사한다.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::cloneQTCNodeTree"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode * sNode;
    qtcNode * sSrcNode;

    IDE_ASSERT( aSource != NULL );
    
    //--------------------------------------------------
    // clone 대상 source node
    //--------------------------------------------------
    if ( ( ( aSource->lflag & QTC_NODE_CONVERSION_MASK )
           == QTC_NODE_CONVERSION_TRUE ) &&
         ( aConstRevoke == ID_TRUE ) )
    {
        //--------------------------------------------------
        // PROJ-1404
        // 이미 constant처리된 노드는 원복하여 복사한다.
        //--------------------------------------------------
        IDE_DASSERT( aConstCopy == ID_TRUE );
        IDE_DASSERT( aSource->node.orgNode != NULL );

        sSrcNode = (qtcNode*)aSource->node.orgNode;
    }
    else
    {
        sSrcNode = aSource;
    }
    
    //--------------------------------------------------
    // source node 복사
    //--------------------------------------------------
    IDU_LIMITPOINT("qtc::cloneQTCNodeTree::malloc");
    IDE_TEST(STRUCT_ALLOC(aMemory, qtcNode, &sNode) != IDE_SUCCESS);

    idlOS::memcpy( sNode, sSrcNode, ID_SIZEOF(qtcNode) );

    if( aConversionClear == ID_TRUE )
    {
        sNode->node.conversion = NULL;
        sNode->node.leftConversion = NULL;
    }
    else
    {
        // Nothing to do...
    }

    // PROJ-1404
    // column에 대해 두 번 estimate하지 않는다.
    if((sSrcNode->node.module == & qtc::columnModule) ||
       (sSrcNode->node.module == &gQtcRidModule))
    {
        sNode->lflag &= ~QTC_NODE_COLUMN_ESTIMATE_MASK;
        sNode->lflag |= QTC_NODE_COLUMN_ESTIMATE_TRUE;
    }
    else
    {
        // Nothing to do.
    }    

    //--------------------------------------------------
    // source node의 argument가 있으면 이를 복사
    //--------------------------------------------------

    if( sSrcNode->node.arguments != NULL )
    {
        if( ( sSrcNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_SUBQUERY )
        {
            // Subquery노드일 경우엔 arguments를 복사하지 않는다.
        }
        else
        {
            // BUG-22045
            // aSource->node.arguments->next까지 고려하기 위해서 aSource로
            // dependency를 검사한다.
            if( ( qtc::dependencyEqual( & sSrcNode->depInfo,
                                        & qtc::zeroDependencies )
                  == ID_FALSE )
                ||
                ( aConstCopy == ID_TRUE ) )
            {
                IDE_TEST( cloneQTCNodeTree( aMemory,
                                            (qtcNode *)sSrcNode->node.arguments,
                                            (qtcNode **)(&(sNode->node.arguments)),
                                            ID_TRUE,
                                            aConversionClear,
                                            aConstCopy,
                                            aConstRevoke )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do...
            }
        }
    }
    else
    {
        // Nothing to do.
    }
    
    // To fix BUG-21243
    // pass node는 복사하지 않고 삭제한다.
    // view target column을 복사하는 경우 passnode까지 복사되어서
    // view push selection을 하는 경우 잘못된 결과를 읽음.
    // 안전을 위해 conversion을 띠어내는 경우만 passnode를 삭제.
    if( (aConversionClear == ID_TRUE) &&
        (sSrcNode->node.module == &qtc::passModule) )
    {
        idlOS::memcpy( sNode, sNode->node.arguments, ID_SIZEOF(qtcNode) );
    }
    else
    {
        // Nothing to do.
    }

    //--------------------------------------------------
    // source의 next가 있으면 이를 복사
    // 
    // 이때, constant처리된 노드도 orgNode가 아닌
    // 원래 source의 next를 따라가야한다.
    // 이는 aSource->node.orgNode의 next가 아니라
    // 원래 source의 link를 지기키 위함이다. ( BUG-22927 )
    //--------------------------------------------------
        
    if( aSource->node.next != NULL )
    {
        if( aContainRootsNext == ID_TRUE )
        {
            if( ( qtc::dependencyEqual(
                      &((qtcNode*)aSource->node.next)->depInfo,
                      & qtc::zeroDependencies )
                  == ID_FALSE )
                ||
                ( aConstCopy == ID_TRUE ) )
            {
                IDE_TEST( cloneQTCNodeTree( aMemory,
                                            (qtcNode *)aSource->node.next,
                                            (qtcNode **)(&(sNode->node.next)),
                                            ID_TRUE,
                                            aConversionClear,
                                            aConstCopy,
                                            aConstRevoke )
                          != IDE_SUCCESS );
            }
            else
            {
                // nothing to do 
            }
        }
        else
        {
            sNode->node.next = NULL;
        }
    }
    else
    {
        sNode->node.next = NULL;
    }

    *aDestination = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::copyNodeTree( qcStatement*    aStatement,
                          qtcNode*        aSource,
                          qtcNode**       aDestination,
                          idBool          aContainRootsNext,
                          idBool          aSubqueryCopy )
{
/***********************************************************************
 *
 * Description : parsing과정에서 필요한 nodetree를 복사한다.
 *
 *
 * Implementation : subquery가 오는 경우는 에러.
 *
 ***********************************************************************/

    qtcNode          * sNode;
    qtcNode          * sSrcNode;

    IDE_ASSERT( aSource != NULL );
    
    //--------------------------------------------------
    // clone 대상 source node
    //--------------------------------------------------
    
    sSrcNode = aSource;

    //--------------------------------------------------
    // source node 복사
    //--------------------------------------------------
    
    if ( ( ( sSrcNode->node.lflag & MTC_NODE_OPERATOR_MASK )
           == MTC_NODE_OPERATOR_SUBQUERY )
         &&
         ( aSubqueryCopy == ID_TRUE ) )
    {
        // subquery node 복사
        IDE_TEST( copySubqueryNodeTree( aStatement,
                                        sSrcNode,
                                        & sNode )
                  != IDE_SUCCESS );
    }
    else
    {
        IDU_LIMITPOINT("qtc::copyNodeTree::malloc");
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                qtcNode,
                                &sNode )
                  != IDE_SUCCESS );
        
        idlOS::memcpy( sNode, sSrcNode, ID_SIZEOF(qtcNode) );
    }

    //--------------------------------------------------
    // source node의 argument가 있으면 이를 복사
    //--------------------------------------------------

    if( sSrcNode->node.arguments != NULL )
    {
        IDE_TEST( copyNodeTree( aStatement,
                                (qtcNode *)sSrcNode->node.arguments,
                                (qtcNode **)(&(sNode->node.arguments)),
                                ID_TRUE,
                                aSubqueryCopy )
                  != IDE_SUCCESS );
    }
    else
    {
        sNode->node.arguments = NULL;
    }
    
    //--------------------------------------------------
    // source의 next가 있으면 이를 복사
    //--------------------------------------------------
        
    if( sSrcNode->node.next != NULL )
    {
        if( aContainRootsNext == ID_TRUE )
        {
            IDE_TEST( copyNodeTree( aStatement,
                                    (qtcNode *)sSrcNode->node.next,
                                    (qtcNode **)(&(sNode->node.next)),
                                    ID_TRUE,
                                    aSubqueryCopy )
                      != IDE_SUCCESS );
        }
        else
        {
            sNode->node.next = NULL;
        }
    }
    else
    {
        sNode->node.next = NULL;
    }

    *aDestination = sNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::copySubqueryNodeTree( qcStatement  * aStatement,
                                  qtcNode      * aSource,
                                  qtcNode     ** aDestination )
{
/***********************************************************************
 *
 * Description : parsing과정에서 필요한 subquery nodetree를 복사한다.
 *
 * Implementation : 
 *
 ***********************************************************************/

    qdDefaultParseTree  * sDefaultParseTree;
    qcStatement           sStatement;

    IDE_ASSERT( aSource != NULL );
    IDE_ASSERT( ( aSource->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_SUBQUERY );
    
    //--------------------------------------------------
    // parsing 준비
    //--------------------------------------------------

    // set meber of qcStatement
    QC_SET_STATEMENT( (&sStatement), aStatement, NULL );

    //--------------------------------------------------
    // parsing subquery
    //--------------------------------------------------
    sStatement.myPlan->stmtText = aSource->position.stmtText;
    sStatement.myPlan->stmtTextLen = idlOS::strlen( aSource->position.stmtText );
    
    // parsing subquery
    IDE_TEST( qcpManager::parsePartialForSubquery( &sStatement,
                                                   aSource->position.stmtText,
                                                   aSource->position.offset,
                                                   aSource->position.size )
              != IDE_SUCCESS );

    //--------------------------------------------------
    // assign subquery node
    //--------------------------------------------------
    
    sDefaultParseTree = (qdDefaultParseTree*) sStatement.myPlan->parseTree;
    *aDestination = sDefaultParseTree->defaultValue;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qtc::isConstNode4OrderBy( qtcNode * aNode )
{
/***********************************************************************
 *
 * Description :
 *    상수 노드인지 검사한다.
 *    BUG-25528 PSM변수와 host변수는 상수가 아닌 것으로 판단하도록 변경
 *    Constant Node에 대한 완전한 정의라고 보기 어려우므로
 *    qmvOrderBy::disconnectConstantNode() 판단을 위해서만 사용
 *
 * Implementation :
 *    상수가 아닌 노드들
 *    - column
 *    - SUM(4), COUNT(*)
 *    - subquery
 *    - user-defined function (variable function이라고 가정한다.)
 *    - sequence
 *    - prior
 *    - level, rownum
 *    - variable built-in function (random, sendmsg)
 *    - PSM 변수
 *    - host 변수
 *
 *    상수 노드들
 *    - value
 *    - sysdate
 *
 ***********************************************************************/

    idBool  sIsConstNode = ID_FALSE;

    if( qtc::dependencyEqual( &aNode->depInfo,
                              &qtc::zeroDependencies ) == ID_TRUE )
    {
        if ( ( ( aNode->lflag & QTC_NODE_AGGREGATE_MASK )
               == QTC_NODE_AGGREGATE_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_AGGREGATE2_MASK )
               == QTC_NODE_AGGREGATE2_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_SUBQUERY_MASK )
               == QTC_NODE_SUBQUERY_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_PROC_FUNCTION_MASK )
               == QTC_NODE_PROC_FUNCTION_FALSE )
             &&
             ( ( aNode->lflag & QTC_NODE_SEQUENCE_MASK )
               == QTC_NODE_SEQUENCE_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_PRIOR_MASK )
               == QTC_NODE_PRIOR_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_LEVEL_MASK )
               == QTC_NODE_LEVEL_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_ROWNUM_MASK )
               == QTC_NODE_ROWNUM_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_VAR_FUNCTION_MASK )
               == QTC_NODE_VAR_FUNCTION_ABSENT )
            &&
            ( ( aNode->lflag & QTC_NODE_PROC_VAR_MASK )
              == QTC_NODE_PROC_VAR_ABSENT )
            &&
            ( ( (aNode->node).lflag & MTC_NODE_BIND_MASK )
              == MTC_NODE_BIND_ABSENT )
             )
        {
            sIsConstNode = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return sIsConstNode;
}

idBool qtc::isConstNode4LikePattern( qcStatement * aStatement,
                                     qtcNode     * aNode,
                                     qcDepInfo   * aOuterDependencies )
{
/***********************************************************************
 *
 * Description :
 *    BUG-25594
 *    like pattern string이 상수 노드인지 검사한다.
 *
 * Implementation :
 *    상수가 아닌 노드들
 *    - column
 *    - SUM(4), COUNT(*)
 *    - subquery
 *    - user-defined function (variable function이라고 가정한다.)
 *    - sequence
 *    - prior
 *    - level, rownum
 *    - variable built-in function (random, sendmsg)
 *
 *    상수 노드들
 *    - PSM 변수
 *    - host 변수
 *    - value
 *    - sysdate
 *    - outer column ( BUG-43495 )
 *
 ***********************************************************************/

    idBool  sIsConstNode = ID_FALSE;

    if( qtc::dependencyEqual( &aNode->depInfo,
                              &qtc::zeroDependencies ) == ID_TRUE )
    {
        if ( ( ( aNode->lflag & QTC_NODE_AGGREGATE_MASK )
               == QTC_NODE_AGGREGATE_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_AGGREGATE2_MASK )
               == QTC_NODE_AGGREGATE2_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_SUBQUERY_MASK )
               == QTC_NODE_SUBQUERY_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_PROC_FUNCTION_MASK )
               == QTC_NODE_PROC_FUNCTION_FALSE )
             &&
             ( ( aNode->lflag & QTC_NODE_SEQUENCE_MASK )
               == QTC_NODE_SEQUENCE_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_PRIOR_MASK )
               == QTC_NODE_PRIOR_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_LEVEL_MASK )
               == QTC_NODE_LEVEL_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_ROWNUM_MASK )
               == QTC_NODE_ROWNUM_ABSENT )
             &&
             ( ( aNode->lflag & QTC_NODE_VAR_FUNCTION_MASK )
               == QTC_NODE_VAR_FUNCTION_ABSENT )
             )
        {
            sIsConstNode = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        /* BUG-43495
           outer column이면 상수로 판단 */
        if ( (qtc::dependencyContains( aOuterDependencies,
                                       &aNode->depInfo )
              == ID_TRUE) &&
             (QCU_OPTIMIZER_LIKE_INDEX_SCAN_WITH_OUTER_COLUMN_DISABLE == 0) )
        {
            sIsConstNode = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_LIKE_INDEX_SCAN_WITH_OUTER_COLUMN_DISABLE );
    }

    return sIsConstNode;
}

IDE_RC qtc::copyAndForDnfFilter( qcStatement* aStatement,
                                 qtcNode*     aSource,
                                 qtcNode**    aDestination,
                                 qtcNode**    aLast )
{
/***********************************************************************
 *
 * Description :
 *
 *    DNF Filter를 위해 필요한 Node를 복사함.
 *
 * Implementation :
 *
 *    DNF Filter를 구성하기 위하여 최상위 AND Node(aSource)를 복사하고,
 *    하위의 DNF_NOT Node를 복사하여 이를 연결한다.
 *
 *        [AND]       : aSource
 *          |
 *          V
 *        [DNF_NOT]
 *
 *    TODO - A4에서는 보다 명확하게 모듈화되어 처리되야 함.
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::copyAndForDnfFilter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qtcNode * sNode;
    qtcNode * sFirstNode;
    qtcNode * sTrvsNode;
    qtcNode * sPrevNode;

    IDU_LIMITPOINT("qtc::copyAndForDnfFilter::malloc1");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sNode)
             != IDE_SUCCESS);

    idlOS::memcpy( sNode, aSource, ID_SIZEOF(qtcNode) );
    sFirstNode = sNode;

    sPrevNode = NULL;
    sTrvsNode = (qtcNode*)(aSource->node.arguments);
    while( sTrvsNode != NULL )
    {
        IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM( aStatement ),
                                         sTrvsNode,
                                         &sNode,
                                         ID_FALSE,
                                         ID_FALSE,
                                         ID_FALSE,
                                         ID_FALSE )
                                         != IDE_SUCCESS );

        sNode->node.next = NULL;

        if( sPrevNode != NULL )
        {
            sPrevNode->node.next = (mtcNode *)sNode;
        }
        else
        {
            sFirstNode->node.arguments = (mtcNode *)sNode;
        }

        sPrevNode = sNode;
        sTrvsNode = (qtcNode*)(sTrvsNode->node.next);
    }

    *aDestination = sFirstNode;
    *aLast        = sPrevNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qtc::makeSameDataType4TwoNode( qcStatement * aStatement,
                               qtcNode     * aNode1,
                               qtcNode     * aNode2 )
{
/***********************************************************************
 *
 * Description :
 *     두 Node의 구조 변경 없이 동일한 Data Type의 결과를
 *     얻을 수 있도록 변경한다.
 *
 * Implementation :
 *
 *     다음과 같은 개념을 이용해 각 SELECT 구문의 Target을
 *     Conversion시켜 서로 다른 Type 의 Target이라 할 지라도
 *     SET 관련 구문을 사용할 수 있도록 함.
 *
 *     아래 그림과 같이 가상의 (=) 연산자를 구성하여,
 *     Target의 각 Column을 연결한 후,
 *     (=)에 대해서 estimate 함으로서 양쪽 Column이 동일한
 *     Data Type 이 되도록 Type Conversion시킨다.
 *             ( = )
 *               |
 *               V
 *             (indierct) --------> (indirect)
 *               |                      |
 *               V                      V
 *       SELECT (column)      SELECT (column)
 *
 *
 ***********************************************************************/

#define IDE_FN "qtc::makeSameDataType4TwoNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qtc::makeSameDataType4TwoNode"));

    // TYPE이 다른 Target에 대한 Conversion을 위하여
    // 필요한 지역 변수임.
    qtcNode         * sEqualNode[2];
    qcNamePosition    sNullPosition;
    qtcNode         * sLeftIndirect;
    qtcNode         * sRightIndirect;

    SET_EMPTY_POSITION( sNullPosition );

    IDE_TEST( qtc::makeNode( aStatement,
                             sEqualNode,
                             & sNullPosition,
                             (const UChar *) "=",
                             1 ) != IDE_SUCCESS );

    //fix BUG-17610
    sEqualNode[0]->node.lflag |= MTC_NODE_EQUI_VALID_SKIP_TRUE;

    // 1. Left의 SELECT Target을 연결함
    IDU_LIMITPOINT("qtc::makeSameDataType4TwoNode::malloc1");
    IDE_TEST ( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                             qtcNode,
                             & sLeftIndirect )!= IDE_SUCCESS);
    IDE_TEST( qtc::makeIndirect( aStatement,
                                 sLeftIndirect,
                                 aNode1 )
              != IDE_SUCCESS );

    // 2. Right의 SELECT Target을 연결함
    IDU_LIMITPOINT("qtc::makeSameDataType4TwoNode::malloc2");
    IDE_TEST ( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                             qtcNode,
                             & sRightIndirect )!= IDE_SUCCESS);
    IDE_TEST( qtc::makeIndirect( aStatement,
                                 sRightIndirect,
                                 aNode2 )
              != IDE_SUCCESS );

    // 3. (=) 연산자에 Left와 Right Target을 Indirect Node 을
    //    중간에 두어 연결함.
    sEqualNode[0]->node.arguments = (mtcNode *) sLeftIndirect;
    sEqualNode[0]->node.arguments->next = (mtcNode *) sRightIndirect;

    // 4. estimate 호출을 하여, Left Target과 Right Target의
    //    Data Type 을 통일시킴
    IDE_TEST(qtc::estimateNodeWithArgument( aStatement,
                                            sEqualNode[0] )
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::makeLeftDataType4TwoNode( qcStatement * aStatement,
                                      qtcNode     * aNode1,
                                      qtcNode     * aNode2 )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *     두 Node의 구조 변경 없이 왼쪽과 동일한 Data Type의 결과를
 *     얻을 수 있도록 변경한다.
 *
 * Implementation :
 *
 *     아래 그림과 같이 cast 연산자를 구성하여, 오른쪽의 target column에
 *     왼쪽과 동일한 type이 되도록 conversion node를 생성한다.
 *
 *                                 CAST( .. as column1_type)
 *                                       |
 *                                       V
 *                                   (indierct)
 *                                       |
 *                                       V
 *       SELECT (column1)      SELECT (column2)
 *
 ***********************************************************************/

    // TYPE이 다른 Target에 대한 Conversion을 위하여
    // 필요한 지역 변수임.
    qtcNode         * sCastNode[2];
    qcNamePosition    sNullPosition;
    qtcNode           sIndirect;

    SET_EMPTY_POSITION( sNullPosition );

    IDE_TEST( qtc::makeNode( aStatement,
                             sCastNode,
                             & sNullPosition,
                             (const UChar *) "CAST",
                             4 ) != IDE_SUCCESS );

    // SELECT Target을 연결함
    IDE_TEST( qtc::makeIndirect( aStatement,
                                 & sIndirect,
                                 aNode2 )
              != IDE_SUCCESS );

    // cast 연산자에 연결함
    sCastNode[0]->node.arguments = (mtcNode *) & sIndirect;
    sCastNode[0]->node.funcArguments = (mtcNode *) aNode1;

    // estimate 호출을 하여, Data Type 을 통일시킴
    IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                             sCastNode[0] )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::makeRecursiveTargetInfo( qcStatement * aStatement,
                                     qtcNode     * aWithNode,
                                     qtcNode     * aViewNode,
                                     qcmColumn   * aColumnInfo )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *
 * Implementation :
 *     발산하는 target과 수렴하는 target을 구별하여 target column의
 *     precision을 보정한다.
 *
 ***********************************************************************/

    mtcColumn       * sWithColumn = NULL;
    mtcColumn       * sViewColumn = NULL;
    const mtdModule * sModule = NULL;

    sWithColumn = QTC_STMT_COLUMN( aStatement, aWithNode );
    sViewColumn = QTC_STMT_COLUMN( aStatement, aViewNode  );

    //---------------------------------------------
    // type은 이미 같다.
    //---------------------------------------------

    IDE_DASSERT( sViewColumn->module->id == sWithColumn->module->id );

    //---------------------------------------------
    // columnInfo의 type,precision을 조정한다.
    //---------------------------------------------
    
    if ( sWithColumn->column.size < sViewColumn->column.size )
    {
        // 발산하는 경우 몇가지 type에 대해 변경한다.
        // bit->varbit, byte->varbyte, char->varchar, echar->evarchar,
        // nchar->nvarchar, numeric->float
        sModule = sViewColumn->module;

        if ( sModule == &mtdBit )
        {
            sModule = &mtdVarbit;
        }
        else if ( sModule == &mtdByte )
        {
            sModule = &mtdVarbyte;
        }
        else if ( sModule == &mtdChar )
        {
            sModule = &mtdVarchar;
        }
        else if ( sModule == &mtdEchar )
        {
            sModule = &mtdEvarchar;
        }
        else if ( sModule == &mtdNchar )
        {
            sModule = &mtdNvarchar;
        }
        else if ( sModule == &mtdNumeric )
        {
            sModule = &mtdFloat;
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_DASSERT( ( sModule->flag & MTD_CREATE_PARAM_MASK )
                     != MTD_CREATE_PARAM_NONE );
        
        // 해당 type의 가장 큰 precision으로 설정한다.
        IDE_TEST( mtc::initializeColumn( aColumnInfo->basicInfo,
                                         sModule,
                                         1,
                                         sModule->maxStorePrecision,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        // 수렴하는 경우, 큰 것으로 설정한다.
        mtc::copyColumn( aColumnInfo->basicInfo, sWithColumn );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//-------------------------------------------------
// PROJ-1358
// Bit Dependency 연산을 바뀐 자료 구조로 표현함.
//-------------------------------------------------

void
qtc::dependencySet( UShort      aTupleID,
                    qcDepInfo * aOperand1 )
{
/***********************************************************************
 *
 * Description :
 *
 *    Internal Tuple ID 로 dependencies를 Setting
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "qtc::dependencySet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aOperand1->depCount = 1;
    aOperand1->depend[0] = aTupleID;

    // PROJ-1653 Outer Join Operator (+)
    aOperand1->depJoinOper[0] = QMO_JOIN_OPER_FALSE;

#undef IDE_FN
}


void
qtc::dependencyChange( UShort      aSourceTupleID,
                       UShort      aDestTupleID,
                       qcDepInfo * aOperand1,
                       qcDepInfo * aResult )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *
 *    partitioned table의 tuple id를
 *    partition의 tuple id로 변경한다.
 *    partition그래프 하위 selection graph는 from의
 *    dependency를 따르지만, partitioned table의 dependency를 따르지
 *    않고 partition의 dependency를 따르도록 변경해 주어야 한다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    qcDepInfo sResult;
    idBool    sSourceExist = ID_FALSE;
    idBool    sDestExist = ID_FALSE;
    UInt      i;

    if ( aSourceTupleID != aDestTupleID )
    {
        // source tuple id와 dest tuple id가 다르다.
        sResult.depCount = 0;

        for ( i = 0; i < aOperand1->depCount; i++ )
        {
            if ( aOperand1->depend[i] == aSourceTupleID )
            {
                sSourceExist = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            if ( aOperand1->depend[i] == aDestTupleID )
            {
                sDestExist = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sSourceExist == ID_TRUE )
        {
            // source tuple id가 있다.

            if ( sDestExist == ID_TRUE )
            {
                // dest tuple id가 이미 있다. source tuple id가 있다면 제거한다.
                for ( i = 0; i < aOperand1->depCount; i++ )
                {
                    if ( aOperand1->depend[i] != aSourceTupleID )
                    {
                        sResult.depend[sResult.depCount] = aOperand1->depend[i];
                        sResult.depCount++;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                // dest tuple id가 없다. source tuple id를 바꾼다.
                // (단, 정렬 순서를 유지해야 한다.)

                if ( aSourceTupleID < aDestTupleID )
                {
                    for ( i = 0;
                          ( i < aOperand1->depCount ) &&
                              ( aOperand1->depend[i] < aSourceTupleID );
                          i++ )
                    {
                        sResult.depend[sResult.depCount] = aOperand1->depend[i];
                        sResult.depCount++;
                    }

                    // source tuple id를 제외한다.
                    i++;

                    for ( ;
                          ( i < aOperand1->depCount ) &&
                              ( aOperand1->depend[i] < aDestTupleID );
                          i++ )
                    {
                        sResult.depend[sResult.depCount] = aOperand1->depend[i];
                        sResult.depCount++;
                    }

                    // dest tuple id를 추가한다.
                    sResult.depend[sResult.depCount] = aDestTupleID;
                    sResult.depCount++;

                    for ( ;
                          i < aOperand1->depCount;
                          i++ )
                    {
                        sResult.depend[sResult.depCount] = aOperand1->depend[i];
                        sResult.depCount++;
                    }
                }
                else
                {
                    for ( i = 0;
                          ( i < aOperand1->depCount ) &&
                              ( aOperand1->depend[i] < aDestTupleID );
                          i++ )
                    {
                        sResult.depend[sResult.depCount] = aOperand1->depend[i];
                        sResult.depCount++;
                    }

                    // dest tuple id를 추가한다.
                    sResult.depend[sResult.depCount] = aDestTupleID;
                    sResult.depCount++;

                    for ( ;
                          ( i < aOperand1->depCount ) &&
                              ( aOperand1->depend[i] < aSourceTupleID );
                          i++ )
                    {
                        sResult.depend[sResult.depCount] = aOperand1->depend[i];
                        sResult.depCount++;
                    }

                    // source tuple id를 제외한다.
                    i++;

                    for ( ;
                          i < aOperand1->depCount;
                          i++ )
                    {
                        sResult.depend[sResult.depCount] = aOperand1->depend[i];
                        sResult.depCount++;
                    }
                }
            }

            qtc::dependencySetWithDep( aResult, & sResult );
        }
        else
        {
            // source tuple id가 없다.
            qtc::dependencySetWithDep( aResult, aOperand1 );
        }
    }
    else
    {
        // source tuple id와 dest tuple id가 같다.
        qtc::dependencySetWithDep( aResult, aOperand1 );
    }
}

void qtc::dependencySetWithDep( qcDepInfo * aOperand1,
                                qcDepInfo * aOperand2 )
{
/***********************************************************************
 *
 * Description :
 *
 *    Dependencies로 dependencies를 Setting
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::dependencySetWithDep"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_ASSERT( aOperand1 != NULL );
    IDE_ASSERT( aOperand2 != NULL );
    
    if ( aOperand1 != aOperand2 )
    {
        aOperand1->depCount = aOperand2->depCount;

        if ( aOperand2->depCount > 0 )
        {
            idlOS::memcpy( aOperand1->depend,
                           aOperand2->depend,
                           ID_SIZEOF(UShort) * aOperand2->depCount );

            // PROJ-1653 Outer Join Operator (+)
            idlOS::memcpy( aOperand1->depJoinOper,
                           aOperand2->depJoinOper,
                           ID_SIZEOF(UChar) * aOperand2->depCount );
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

#undef IDE_FN
}

void qtc::dependencyAnd( qcDepInfo * aOperand1,
                         qcDepInfo * aOperand2,
                         qcDepInfo * aResult )
{
/***********************************************************************
 *
 * Description :
 *
 *    AND Dependencies를 구함
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::dependencyAnd"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcDepInfo sResult;

    UInt sLeft;
    UInt sRight;

    sResult.depCount = 0;

    for ( sLeft = 0, sRight = 0;
          ( (sLeft < aOperand1->depCount) && (sRight < aOperand2->depCount) );
        )
    {
        if ( aOperand1->depend[sLeft]
             == aOperand2->depend[sRight] )
        {
            sResult.depend[sResult.depCount] = aOperand1->depend[sLeft];

            // PROJ-1653 Outer Join Operator (+)
            sResult.depJoinOper[sResult.depCount] = aOperand1->depJoinOper[sLeft] & aOperand2->depJoinOper[sRight];

            sResult.depCount++;

            sLeft++;
            sRight++;
        }
        else
        {
            if ( aOperand1->depend[sLeft] > aOperand2->depend[sRight] )
            {
                sRight++;
            }
            else
            {
                sLeft++;
            }
        }
    }

    qtc::dependencySetWithDep( aResult, & sResult );

#undef IDE_FN
}

IDE_RC
qtc::dependencyOr( qcDepInfo * aOperand1,
                   qcDepInfo * aOperand2,
                   qcDepInfo * aResult )
{
/***********************************************************************
 *
 * Description :
 *
 *    OR Dependencies를 구함
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "qtc::dependencyOr"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcDepInfo sResult;

    UInt sLeft;
    UInt sRight;

    IDE_ASSERT( aOperand1 != NULL );
    IDE_ASSERT( aOperand2 != NULL );
    IDE_ASSERT( aOperand1->depCount <= QC_MAX_REF_TABLE_CNT );
    IDE_ASSERT( aOperand2->depCount <= QC_MAX_REF_TABLE_CNT );
    
    sResult.depCount = 0;

    for ( sLeft = 0, sRight = 0;
          (sLeft < aOperand1->depCount) && (sRight < aOperand2->depCount);
        )
    {
        IDE_TEST_RAISE( sResult.depCount >= QC_MAX_REF_TABLE_CNT,
                        err_too_many_table_ref );

        if ( aOperand1->depend[sLeft] == aOperand2->depend[sRight] )
        {
            sResult.depend[sResult.depCount] = aOperand1->depend[sLeft];

            // PROJ-1653 Outer Join Operator (+)
            sResult.depJoinOper[sResult.depCount] = aOperand1->depJoinOper[sLeft] | aOperand2->depJoinOper[sRight];

            sLeft++;
            sRight++;

        }
        else
        {
            if ( aOperand1->depend[sLeft] > aOperand2->depend[sRight] )
            {
                sResult.depend[sResult.depCount] = aOperand2->depend[sRight];
                // PROJ-1653 Outer Join Operator (+)
                sResult.depJoinOper[sResult.depCount] = aOperand2->depJoinOper[sRight];

                sRight++;
            }
            else
            {
                sResult.depend[sResult.depCount] = aOperand1->depend[sLeft];
                // PROJ-1653 Outer Join Operator (+)
                sResult.depJoinOper[sResult.depCount] = aOperand1->depJoinOper[sLeft];

                sLeft++;
            }
        }

        sResult.depCount++;
    }

    // Left Operand에 남아 있는 것 Or-ing
    for ( ; sLeft < aOperand1->depCount; sLeft++ )
    {
        IDE_TEST_RAISE( sResult.depCount >= QC_MAX_REF_TABLE_CNT,
                        err_too_many_table_ref );

        sResult.depend[sResult.depCount] = aOperand1->depend[sLeft];
        // PROJ-1653 Outer Join Operator (+)
        sResult.depJoinOper[sResult.depCount] = aOperand1->depJoinOper[sLeft];
        sResult.depCount++;
    }

    // Right Operand에 남아 있는 것 Or-ing
    for ( ; sRight < aOperand2->depCount; sRight++ )
    {
        // To Fix PR-12758
        // 검사를 먼저 수행하여야 ABW가 발생하지 않음.
        IDE_TEST_RAISE( sResult.depCount >= QC_MAX_REF_TABLE_CNT,
                        err_too_many_table_ref );

        sResult.depend[sResult.depCount] = aOperand2->depend[sRight];
        // PROJ-1653 Outer Join Operator (+)
        sResult.depJoinOper[sResult.depCount] = aOperand2->depJoinOper[sRight];
        sResult.depCount++;
    }

    qtc::dependencySetWithDep( aResult, & sResult );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_too_many_table_ref);
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QTC_TOO_MANY_TABLES ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

void qtc::dependencyRemove( UShort      aTupleID,
                            qcDepInfo * aOperand1,
                            qcDepInfo * aResult )
{
/***********************************************************************
 *
 * Description :
 *     aOperand1에서 aTupleID의 dependency를 제거
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::dependencyRemove"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcDepInfo sResult;
    UInt      i;

    sResult.depCount = 0;

    for ( i = 0; i < aOperand1->depCount; i++ )
    {
        if ( aOperand1->depend[i] != aTupleID )
        {
            sResult.depend[sResult.depCount] = aOperand1->depend[i];
            // PROJ-1653 Outer Join Operator (+)
            sResult.depJoinOper[sResult.depCount] = aOperand1->depJoinOper[i];
            sResult.depCount++;
        }
        else
        {
            // Nothing to do.
        }
    }

    qtc::dependencySetWithDep( aResult, & sResult );

#undef IDE_FN
}

void qtc::dependencyClear( qcDepInfo * aOperand1 )
{
/***********************************************************************
 *
 * Description :
 *
 *    Dependencies를 초기화함.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::dependencyClear"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aOperand1->depCount = 0;

#undef IDE_FN
}

idBool qtc::dependencyEqual( qcDepInfo * aOperand1,
                             qcDepInfo * aOperand2 )
{
/***********************************************************************
 *
 * Description :
 *
 *    Dependencies가 동일한 지를 판단
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::dependencyEqual"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("IDE_RC qtc::dependencyEqual"));

    UInt i;

    if ( aOperand1->depCount != aOperand2->depCount )
    {
        return ID_FALSE;
    }
    else
    {
        for ( i = 0; i < aOperand1->depCount; i++ )
        {
            if ( aOperand1->depend[i] != aOperand2->depend[i] )
            {
                return ID_FALSE;
            }
        }
    }

    return ID_TRUE;

#undef IDE_FN
}

idBool qtc::haveDependencies( qcDepInfo * aOperand1 )
{
/***********************************************************************
 *
 * Description :
 *
 *    Dependency 가 존재하는지를 판단
 *
 * Implementation :
 *
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::haveDependencies"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( aOperand1->depCount == 0 )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }

#undef IDE_FN
}

idBool qtc::dependencyContains( qcDepInfo * aSubject, qcDepInfo * aTarget )
{
/***********************************************************************
 *
 * Description :
 *
 *     aTarget의 Dependency가 aSubject의 Dependency에 포함되는지 판단
 *
 * Implementation :
 *
 *
 **********************************************************************/

#define IDE_FN "IDE_RC qtc::dependencyContains"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt sLeft;
    UInt sRight;

    sRight = 0;

    if ( aSubject->depCount >= aTarget->depCount )
    {
        for ( sLeft = 0, sRight = 0;
              (sRight < aTarget->depCount) && (sLeft < aSubject->depCount) ; )
        {
            if ( aSubject->depend[sLeft] == aTarget->depend[sRight] )
            {
                sLeft++;
                sRight++;
            }
            else
            {
                if ( aSubject->depend[sLeft] > aTarget->depend[sRight] )
                {
                    break;
                }
                else
                {
                    sLeft++;
                }
            }
        }
    }
    else
    {
        // 포함관계를 만족할 수 없음.
    }

    if ( sRight == aTarget->depCount )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }

#undef IDE_FN
}

void qtc::dependencyMinus( qcDepInfo * aOperand1,
                           qcDepInfo * aOperand2,
                           qcDepInfo * aResult )
{
/***********************************************************************
 *
 * Description :
 *
 *     ( aOperand1 - aOperand2 ) = aOperand1;
 * 
 *     aOperand1에서 aOperand2에 겹치는 dependency는 제거
 *
 * Implementation :
 *
 **********************************************************************/

    qcDepInfo sResultDepInfo;
    UInt      i;

    qtc::dependencyClear( & sResultDepInfo );
    qtc::dependencySetWithDep( & sResultDepInfo, aOperand1 );

    for ( i = 0; i < aOperand2->depCount; i++ )
    {
        dependencyRemove( aOperand2->depend[i],
                          & sResultDepInfo, 
                          & sResultDepInfo );
    }

    qtc::dependencySetWithDep( aResult, & sResultDepInfo );
}

IDE_RC qtc::isEquivalentExpression(
    qcStatement     * aStatement,
    qtcNode         * aNode1,
    qtcNode         * aNode2,
    idBool          * aIsTrue )
{
#define IDE_FN "qtc::isEquivalentExpression"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qtc::isEquivalentExpression"));

    qsExecParseTree * sExecPlanTree1;
    qsExecParseTree * sExecPlanTree2;
    qtcNode         * sArgu1;
    qtcNode         * sArgu2;
    mtcColumn       * sColumn1;
    mtcColumn       * sColumn2;
    mtcNode         * sNode1;
    mtcNode         * sNode2;
    qtcOverColumn   * sCurOverColumn1;
    qtcOverColumn   * sCurOverColumn2;
    SInt              sResult;
    idBool            sIsTrue;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    UInt              sFlag1;
    UInt              sFlag2;

    if ( (aNode1 != NULL) && (aNode2 != NULL) )
    {   
        // PROJ-2242 : constant filter 비교를 위한 조건 추가
        // random 함수등이 수행된 경우는 배제

        // PROJ-2415 Grouping Sets Clause
        // 기존 Null Value와 Empty Group으로 생성 된 Null Value가 비교되는 것 을 막기위해
        // QTC_NODE_EMPTY_GROUP_TRUE 를 검사한다.
        if ( ( aNode1->node.arguments != NULL ) &&
             ( aNode2->node.arguments != NULL ) &&
             ( ( aNode1->lflag & QTC_NODE_CONVERSION_MASK )
                == QTC_NODE_CONVERSION_FALSE ) &&
             ( ( aNode2->lflag & QTC_NODE_CONVERSION_MASK )
                == QTC_NODE_CONVERSION_FALSE ) &&
             ( ( ( aNode1->lflag & QTC_NODE_EMPTY_GROUP_MASK )
                 != QTC_NODE_EMPTY_GROUP_TRUE ) &&
               ( ( aNode2->lflag & QTC_NODE_EMPTY_GROUP_MASK )
                 != QTC_NODE_EMPTY_GROUP_TRUE ) ) ) 
        {
            // non-terminal node

            if ( isSameModule(aNode1,aNode2) == ID_TRUE )
            {
                sIsTrue = ID_TRUE;

                //-------------------------------
                // arguments 검사
                //-------------------------------

                sArgu1 = (qtcNode *)(aNode1->node.arguments);
                sArgu2 = (qtcNode *)(aNode2->node.arguments);

                while( (sArgu1 != NULL) && (sArgu2 != NULL) )
                {
                    IDE_TEST(isEquivalentExpression(
                                 aStatement, sArgu1, sArgu2, &sIsTrue)
                             != IDE_SUCCESS);

                    if (sIsTrue == ID_TRUE)
                    {
                        sArgu1 = (qtcNode *)(sArgu1->node.next);
                        sArgu2 = (qtcNode *)(sArgu2->node.next);
                    }
                    else
                    {
                        break; // exit while loop
                    }
                }

                if ( (sArgu1 != NULL) || (sArgu2 != NULL) )
                {
                    // arguments->next의 수가 맞지 않는 경우
                    sIsTrue = ID_FALSE;
                }
                else
                {
                    // Nothing to do.
                }

                if( sIsTrue == ID_FALSE )
                {
                    // BUG-31777
                    // 교환법칙 가능한 +, * 연산자의 경우 두 비교 대상 expression의
                    // argument 순서를 바꿔가며 다시 비교해본다.
                    if( ( aNode1->node.module->lflag & MTC_NODE_COMMUTATIVE_MASK ) ==
                            MTC_NODE_COMMUTATIVE_TRUE )
                    {
                        sArgu1 = (qtcNode *)(aNode1->node.arguments);
                        sArgu2 = (qtcNode *)(aNode2->node.arguments->next);

                        IDE_TEST( isEquivalentExpression(
                                      aStatement, sArgu1, sArgu2, &sIsTrue )
                                != IDE_SUCCESS);

                        if( sIsTrue == ID_TRUE )
                        {
                            sArgu1 = (qtcNode *)(aNode1->node.arguments->next);
                            sArgu2 = (qtcNode *)(aNode2->node.arguments);

                            IDE_TEST( isEquivalentExpression(
                                          aStatement, sArgu1, sArgu2, &sIsTrue )
                                    != IDE_SUCCESS);
                        }
                    }
                }

                if ( sIsTrue == ID_TRUE )
                {
                    // PROJ-2179
                    // Aggregate function에서 DISTINCT 절의 존재 여부가 같아야 한다.
                    if( ( aNode1->node.lflag & MTC_NODE_DISTINCT_MASK ) !=
                        ( aNode2->node.lflag & MTC_NODE_DISTINCT_MASK ) )
                    {
                        sIsTrue = ID_FALSE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    /* BUG-46805 */
                    if ( ( aNode1->node.lflag & MTC_NODE_OPERATOR_MASK )
                        == MTC_NODE_OPERATOR_SUBQUERY ) // subquery
                    {
                        // BUG-48381 copy된 subquery는 동일한 서브쿼리이다.
                        if ( ( aNode1->node.table != aNode2->node.table ) ||
                             ( aNode1->node.column != aNode2->node.column ) )
                        {
                            if ( ( aNode1->subquery->myPlan->graph != NULL ) &&
                                 ( aNode2->subquery->myPlan->graph != NULL ) )
                            {
                                // BUG-48381 key predicate 중에 set op가 포함된 서브쿼리 프리디킷이 있고
                                //           key predicate가 DNF로 풀릴 경우 비>정상종료
                                if ( ( aNode1->subquery->myPlan->graph->myQuerySet->SFWGH != NULL ) &&
                                     ( aNode2->subquery->myPlan->graph->myQuerySet->SFWGH != NULL ) )
                                {
                                    if ( ( aNode1->subquery->myPlan->graph->myQuerySet->SFWGH->where != NULL ) ||
                                         ( aNode2->subquery->myPlan->graph->myQuerySet->SFWGH->where != NULL ) )
                                    {
                                        if ( ( aNode1->subquery->myPlan->graph->myQuerySet->SFWGH->where != NULL ) &&
                                             ( aNode2->subquery->myPlan->graph->myQuerySet->SFWGH->where != NULL ) )
                                        {
                                            sArgu1 = (qtcNode *)aNode1->subquery->myPlan->graph->myQuerySet->SFWGH->where;
                                            sArgu2 = (qtcNode *)aNode2->subquery->myPlan->graph->myQuerySet->SFWGH->where;

                                            while ( ( sArgu1 != NULL ) && ( sArgu2 != NULL ) )
                                            {
                                                IDE_TEST(isEquivalentExpression( aStatement, sArgu1, sArgu2, &sIsTrue)
                                                         != IDE_SUCCESS);

                                                if ( sIsTrue == ID_TRUE)
                                                {
                                                    if ( ( sArgu1->node.lflag & MTC_NODE_OPERATOR_MASK )
                                                         == MTC_NODE_OPERATOR_SUBQUERY ) // subquery
                                                    {
                                                        sIsTrue = ID_FALSE;
                                                        break;
                                                    }
                                                    else
                                                    {
                                                        sArgu1 = (qtcNode *)(sArgu1->node.next);
                                                        sArgu2 = (qtcNode *)(sArgu2->node.next);
                                                    }
                                                }
                                                else
                                                {
                                                    break; // exit while loop
                                                }
                                            }

                                            if ( (sArgu1 != NULL) || (sArgu2 != NULL) )
                                            {
                                                // arguments->next의 수가 맞지 않는 경우
                                                sIsTrue = ID_FALSE;
                                            }
                                            else
                                            {
                                                // Nothing to do.
                                            }
                                        }
                                        else
                                        {
                                            sIsTrue = ID_FALSE;
                                        }
                                    }
                                    else
                                    {
                                        /* Nothing to do */
                                    }
                                }
                                else
                                {
                                    /* Nothing to do */
                                }
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    // Nothing to do.
                }

                *aIsTrue = sIsTrue;
            }
            else
            {
                *aIsTrue = ID_FALSE;
            }
        }
        // PROJ-2415 Grouping Sets Clause
        // 기존 Null Value와 Empty Group으로 생성 된 Null Value가 비교되는 것 을 막기위해
        // QTC_NODE_EMPTY_GROUP_TRUE 를 검사한다.

        // PROJ-2242 : constant filter 비교를 위한 조건 추가
        // random 함수등이 수행된 경우는 배제
        else if ( ( ( (aNode1->node.arguments == NULL) &&
                      (aNode2->node.arguments == NULL) )
                    ||
                    ( ( ( aNode1->lflag & QTC_NODE_CONVERSION_MASK )
                        == QTC_NODE_CONVERSION_TRUE ) &&
                      ( ( aNode2->lflag & QTC_NODE_CONVERSION_MASK)
                        == QTC_NODE_CONVERSION_TRUE ) ) ) &&
                  ( ( ( aNode1->lflag & QTC_NODE_EMPTY_GROUP_MASK )
                      != QTC_NODE_EMPTY_GROUP_TRUE ) &&
                    ( ( aNode2->lflag & QTC_NODE_EMPTY_GROUP_MASK )
                      != QTC_NODE_EMPTY_GROUP_TRUE ) ) )
        {
            // terminal node

            if ( isSameModule(aNode1,aNode2) == ID_TRUE )
            {
                sIsTrue = ID_TRUE;
                
                //-------------------------------
                // terminal nodes 검사
                //-------------------------------
                
                // terminal nodes : subquery,
                //                  host variable,
                //                  value,
                //                  column
                if ( ( aNode1->node.lflag & MTC_NODE_OPERATOR_MASK )
                    == MTC_NODE_OPERATOR_SUBQUERY ) // subquery
                {
                    sIsTrue = ID_FALSE;
                }
                else
                {
                    sColumn1 =
                        & ( QC_SHARED_TMPLATE(aStatement)->tmplate.
                            rows[aNode1->node.table].
                            columns[aNode1->node.column]);
                    sColumn2 =
                        & ( QC_SHARED_TMPLATE(aStatement)->tmplate.
                            rows[aNode2->node.table].
                            columns[aNode2->node.column]);

                    if (aNode1->node.module == &(qtc::valueModule))
                    {
                        if ( ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.
                                 rows[aNode1->node.table].lflag
                                 & MTC_TUPLE_TYPE_MASK )
                               == MTC_TUPLE_TYPE_VARIABLE )            ||
                             ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.
                                 rows[aNode2->node.table].lflag
                                 & MTC_TUPLE_TYPE_MASK )
                               == MTC_TUPLE_TYPE_VARIABLE ) )
                        {
                            // BUG-22045
                            // variable tuple이라 하더라도 bind tuple인 경우는 검사가능하다.
                            if ( ( aNode1->node.table ==
                                   QC_SHARED_TMPLATE(aStatement)->tmplate.variableRow ) &&
                                 ( aNode1->node.table == aNode2->node.table ) &&
                                 ( aNode1->node.column == aNode2->node.column ) )
                            {
                                // Nothing to do.
                            }
                            else
                            {
                                sIsTrue = ID_FALSE;
                            }
                        }
                        else
                        {
                            /* PROJ-1361 : data module과 language module 분리
                               if (sColumn1->type.type == sColumn2->type.type &&
                               sColumn1->type.language ==
                               sColumn2->type.language)
                            */
                            if ( sColumn1->type.dataTypeId == MTD_GEOMETRY_ID ||
                                 sColumn2->type.dataTypeId == MTD_GEOMETRY_ID )
                            {
                                /* PROJ-2242 : CSE
                                   ex) WHERE OVERLAPS(F2, GEOMETRY'MULTIPOINT(0 0)')
                                          OR OVERLAPS(F2, GEOMETRY'MULTIPOINT(1 1, 2 2)')
                                                          |
                                                   아래 조건에서 두 노드를 같다고 판단
                                */
                                sIsTrue = ID_FALSE;
                            }
                            else if ( sColumn1->type.dataTypeId == sColumn2->type.dataTypeId )
                            {
                                // compare value
                                sValueInfo1.column = sColumn1;
                                sValueInfo1.value  = QC_SHARED_TMPLATE(aStatement)->tmplate.
                                                     rows[aNode1->node.table].row;
                                sValueInfo1.flag   = MTD_OFFSET_USE;

                                sValueInfo2.column = sColumn2;
                                sValueInfo2.value  = QC_SHARED_TMPLATE(aStatement)->tmplate.
                                                     rows[aNode2->node.table].row;
                                sValueInfo2.flag   = MTD_OFFSET_USE;

                                sResult = sColumn1->module->
                                    keyCompare[MTD_COMPARE_MTDVAL_MTDVAL][MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                                                  &sValueInfo2 );

                                if (sResult == 0)
                                {
                                    // Nothing to do.
                                }
                                else
                                {
                                    sIsTrue = ID_FALSE;
                                }
                            }
                            else
                            {
                                sIsTrue = ID_FALSE;
                            }
                        }
                    }
                    else if (aNode1->node.module == &(qtc::columnModule))
                    {
                        // column
                        if ( (aNode1->node.table == aNode2->node.table) &&
                             (aNode1->node.column == aNode2->node.column) )
                        {
                            // PROJ-2179
                            // PRIOR 절의 존재 여부가 같아야 한다.
                            if( ( aNode1->lflag & QTC_NODE_PRIOR_MASK ) !=
                                ( aNode2->lflag & QTC_NODE_PRIOR_MASK ) )
                            {
                                sIsTrue = ID_FALSE;
                            }
                            else
                            {
                                // Nothing to do.
                            }

                            // PROJ-2527 WITHIN GROUP AGGR
                            // WITHIN GROUP 의 OREDER BY direction에 따라 같은지 구분..
                            if( ( aNode1->node.lflag & MTC_NODE_WITHIN_GROUP_ORDER_MASK ) !=
                                ( aNode2->node.lflag & MTC_NODE_WITHIN_GROUP_ORDER_MASK ) )
                            {
                                sIsTrue = ID_FALSE;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                        else
                        {
                            // BUG-31961
                            // Procedure 변수는 procedure template의 column을 비교한다.
                            if ( ( ( aNode1->lflag & QTC_NODE_PROC_VAR_MASK )
                                     == QTC_NODE_PROC_VAR_EXIST ) &&
                                 ( ( aNode2->lflag & QTC_NODE_PROC_VAR_MASK )
                                     == QTC_NODE_PROC_VAR_EXIST ) )
                            {
                                sNode1 = (mtcNode *)QC_SHARED_TMPLATE(aStatement)->tmplate.
                                    rows[aNode1->node.table].execute[aNode1->node.column].calculateInfo;
                                sNode2 = (mtcNode *)QC_SHARED_TMPLATE(aStatement)->tmplate.
                                    rows[aNode2->node.table].execute[aNode2->node.column].calculateInfo;

                                if( ( sNode1 != NULL ) && ( sNode2 != NULL ) )
                                {
                                    if( (sNode1->table  != sNode2->table) ||
                                        (sNode1->column != sNode2->column) )
                                    {
                                        sIsTrue = ID_FALSE;
                                    }
                                    else
                                    {
                                        // Nothing to do.
                                    }
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                            else
                            {
                                sIsTrue = ID_FALSE;
                            }
                        }
                    }
                    else if (aNode1->node.module == &(qtc::spFunctionCallModule))
                    {
                        // BUG-21065
                        // PSM인 경우 동일 expression인지 검사한다.
                        sExecPlanTree1 = (qsExecParseTree*)
                            aNode1->subquery->myPlan->parseTree;
                        sExecPlanTree2 = (qsExecParseTree*)
                            aNode2->subquery->myPlan->parseTree;

                        if ( (sExecPlanTree1->procOID == sExecPlanTree2->procOID) &&
                             (sExecPlanTree1->subprogramID == sExecPlanTree2->subprogramID) )
                        {
                            if ( (aStatement->spvEnv->createProc == NULL) &&
                                 (aStatement->spvEnv->createPkg == NULL) &&
                                 (sExecPlanTree1->procOID == QS_EMPTY_OID) )
                            {
                                sIsTrue = ID_FALSE;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                        else
                        {
                            sIsTrue = ID_FALSE;
                        }
                    }
                    else if (aNode1->node.module == &mtfCount )
                    {
                        // PROJ-2179 COUNT(*)와 같이 인자가 없는 aggregate functino
                        // Nothing to do.
                    }
                    else
                    {
                        // BUG-34382
                        // 나머지 모듈은 table, column 으로 비교한다.
                        if ( (aNode1->node.table == aNode2->node.table) &&
                             (aNode1->node.column == aNode2->node.column) )
                        {
                            sIsTrue = ID_TRUE;
                        }
                        else
                        {
                            sIsTrue = ID_FALSE;
                        }
                    }
                }

                *aIsTrue = sIsTrue;
            }
            else
            {
                *aIsTrue = ID_FALSE;
            }
        }
        else
        {
            *aIsTrue = ID_FALSE;
        }
    }
    else
    {
        *aIsTrue = ID_FALSE;
    }

    //-------------------------------
    // over절 검사
    //-------------------------------    
    if( *aIsTrue == ID_TRUE )
    {
        if ( ( aNode1->overClause != NULL ) &&
             ( aNode2->overClause != NULL ) )
        {
            // 두 노드 모두 over절이 있는 경우
            
            sCurOverColumn1 = aNode1->overClause->overColumn;
            sCurOverColumn2 = aNode2->overClause->overColumn;
    
            while( (sCurOverColumn1 != NULL) && (sCurOverColumn2 != NULL) )
            {
                IDE_TEST( isEquivalentExpression(
                              aStatement,
                              sCurOverColumn1->node,
                              sCurOverColumn2->node,
                              &sIsTrue )
                          != IDE_SUCCESS);

                // PROJ-2179
                // OVER절의 ORDER BY의 조건을 확인한다.

                sFlag1 = ( sCurOverColumn1->flag & QTC_OVER_COLUMN_MASK );
                sFlag2 = ( sCurOverColumn2->flag & QTC_OVER_COLUMN_MASK );

                if( sFlag1 != sFlag2 )
                {
                    sIsTrue = ID_FALSE;
                }
                else
                {
                    if( sFlag1 == QTC_OVER_COLUMN_ORDER_BY )
                    {
                        sFlag1 = ( sCurOverColumn1->flag & QTC_OVER_COLUMN_ORDER_MASK );
                        sFlag2 = ( sCurOverColumn2->flag & QTC_OVER_COLUMN_ORDER_MASK );

                        if( sFlag1 != sFlag2 )
                        {
                            sIsTrue = ID_FALSE;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                if (sIsTrue == ID_TRUE)
                {
                    sCurOverColumn1 = sCurOverColumn1->next;
                    sCurOverColumn2 = sCurOverColumn2->next;
                }
                else
                {
                    break; // exit while loop
                }
            }

            if ( (sCurOverColumn1 != NULL) || (sCurOverColumn2 != NULL) )
            {
                // overColumn->next의 수가 맞지 않는 경우
                sIsTrue = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ( ( aNode1->overClause == NULL ) &&
                  ( aNode2->overClause == NULL ) )
        {
            // 두 노드 모두 over절이 없는 경우
            
            // Nothing to do.
        }
        else
        {
            // 어느 한쪽에만 over절이 있는 경우
            
            sIsTrue = ID_FALSE;
        }

        *aIsTrue = sIsTrue;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::isEquivalentPredicate( qcStatement * aStatement,
                                   qtcNode     * aPredicate1,
                                   qtcNode     * aPredicate2,
                                   idBool      * aResult )
{
    qtcNode   * sArg1;
    qtcNode   * sArg2;
    mtfModule * sOperator;

    IDE_DASSERT( aPredicate1 != NULL );
    IDE_DASSERT( aPredicate2 != NULL );
    IDE_DASSERT( ( aPredicate1->node.lflag & MTC_NODE_COMPARISON_MASK )
                    == MTC_NODE_COMPARISON_TRUE );
    IDE_DASSERT( ( aPredicate2->node.lflag & MTC_NODE_COMPARISON_MASK )
                    == MTC_NODE_COMPARISON_TRUE );

    if( isSameModule( aPredicate1, aPredicate2 ) == ID_TRUE )
    {
        sArg1 = (qtcNode *)aPredicate1->node.arguments;
        sArg2 = (qtcNode *)aPredicate2->node.arguments;

        while( ( sArg1 != NULL ) && ( sArg2 != NULL ) )
        {
            IDE_TEST( isEquivalentExpression( aStatement,
                                              sArg1,
                                              sArg2,
                                              aResult )
                      != IDE_SUCCESS );

            if( *aResult == ID_FALSE )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
            sArg1 = (qtcNode *)sArg1->node.next;
            sArg2 = (qtcNode *)sArg2->node.next;
        }

        if( *aResult == ID_TRUE )
        {
            if( ( sArg1 == NULL ) && ( sArg2 == NULL ) )
            {
                // Argument들의 개수가 동일한 경우
            }
            else
            {
                // Argument들의 개수가 다른 경우
                *aResult = ID_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        *aResult = ID_FALSE;
    }

    if( *aResult == ID_FALSE )
    {
        /******************************************************
         * 다음은 각각 동일한 조건이므로 확인한다.
         * | A = B  | B = A  |
         * | A <> B | B <> A |
         * | A < B  | B > A  |
         * | A <= B | B >= A |
         * | A > B  | B < A  |
         * | A >= B | B <= A |
         *****************************************************/
        if( aPredicate1->node.module == &mtfEqual )
        {
            sOperator = &mtfEqual;
        }
        else if( aPredicate1->node.module == &mtfNotEqual )
        {
            sOperator = &mtfNotEqual;
        }
        else if( aPredicate1->node.module == &mtfLessThan )
        {
            sOperator = &mtfGreaterThan;
        }
        else if( aPredicate1->node.module == &mtfLessEqual )
        {
            sOperator = &mtfGreaterEqual;
        }
        else if( aPredicate1->node.module == &mtfGreaterThan )
        {
            sOperator = &mtfLessThan;
        }
        else if( aPredicate1->node.module == &mtfGreaterEqual )
        {
            sOperator = &mtfLessEqual;
        }
        else
        {
            sOperator = NULL;
        }

        if( aPredicate2->node.module == sOperator )
        {
            sArg1 = (qtcNode *)aPredicate1->node.arguments;
            sArg2 = (qtcNode *)aPredicate2->node.arguments->next;

            IDE_TEST( isEquivalentExpression( aStatement,
                                              sArg1,
                                              sArg2,
                                              aResult )
                      != IDE_SUCCESS );

            if( *aResult == ID_TRUE )
            {
                sArg1 = (qtcNode *)aPredicate1->node.arguments->next;
                sArg2 = (qtcNode *)aPredicate2->node.arguments;

                IDE_TEST( isEquivalentExpression( aStatement,
                                                  sArg1,
                                                  sArg2,
                                                  aResult )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : (parsing은 했으나) estimate하기 전의 expression에 대해 equivalent 검사
 *
 ***********************************************************************/
IDE_RC qtc::isEquivalentExpressionByName( qtcNode * aNode1,
                                          qtcNode * aNode2,
                                          idBool  * aIsEquivalent )
{
    qtcNode         * sArgu1;
    qtcNode         * sArgu2;
    qtcOverColumn   * sCurOverColumn1;
    qtcOverColumn   * sCurOverColumn2;
    UInt              sFlag1;
    UInt              sFlag2;
    idBool            sIsEquivalent;

    if ( ( aNode1 != NULL ) && ( aNode2 != NULL ) )
    {
        /* PROJ-2415 Grouping Sets Clause
         * 기존 Null Value와 Empty Group으로 생성 된 Null Value가 비교되는 것 을 막기위해
         * QTC_NODE_EMPTY_GROUP_TRUE 를 검사한다.
         */
        if ( ( aNode1->node.arguments != NULL ) &&
             ( aNode2->node.arguments != NULL ) &&
             ( ( aNode1->lflag & QTC_NODE_EMPTY_GROUP_MASK ) != QTC_NODE_EMPTY_GROUP_TRUE ) &&
             ( ( aNode2->lflag & QTC_NODE_EMPTY_GROUP_MASK ) != QTC_NODE_EMPTY_GROUP_TRUE ) )
        {
            /* Non-terminal Node */

            if ( isSameModuleByName( aNode1,aNode2 ) == ID_TRUE )
            {
                sIsEquivalent = ID_TRUE;

                //-------------------------------
                // arguments 검사
                //-------------------------------

                sArgu1 = (qtcNode *)(aNode1->node.arguments);
                sArgu2 = (qtcNode *)(aNode2->node.arguments);

                while( ( sArgu1 != NULL ) && ( sArgu2 != NULL ) )
                {
                    IDE_TEST( isEquivalentExpressionByName( sArgu1, sArgu2, &sIsEquivalent )
                              != IDE_SUCCESS );

                    if ( sIsEquivalent == ID_TRUE )
                    {
                        sArgu1 = (qtcNode *)(sArgu1->node.next);
                        sArgu2 = (qtcNode *)(sArgu2->node.next);
                    }
                    else
                    {
                        break;
                    }
                }
                if ( ( sArgu1 != NULL ) || ( sArgu2 != NULL ) )
                {
                    sIsEquivalent = ID_FALSE;
                }
                else
                {
                    /* Nothing to do */
                }

                if ( sIsEquivalent == ID_FALSE )
                {
                    /* BUG-31777
                     * 교환법칙 가능한 +, * 연산자의 경우 두 비교 대상 expression의
                     * argument 순서를 바꿔가며 다시 비교해본다.
                     */
                    if ( ( aNode1->node.module->lflag & MTC_NODE_COMMUTATIVE_MASK ) ==
                                                        MTC_NODE_COMMUTATIVE_TRUE )
                    {
                        sArgu1 = (qtcNode *)(aNode1->node.arguments);
                        sArgu2 = (qtcNode *)(aNode2->node.arguments->next);

                        IDE_TEST( isEquivalentExpressionByName( sArgu1, sArgu2, &sIsEquivalent )
                                  != IDE_SUCCESS );

                        if ( sIsEquivalent == ID_TRUE )
                        {
                            sArgu1 = (qtcNode *)(aNode1->node.arguments->next);
                            sArgu2 = (qtcNode *)(aNode2->node.arguments);

                            IDE_TEST( isEquivalentExpressionByName( sArgu1, sArgu2, &sIsEquivalent )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    /* Nothing to do */
                }

                if ( sIsEquivalent == ID_TRUE )
                {
                    /* PROJ-2179
                     * Aggregate function에서 DISTINCT 절의 존재 여부가 같아야 한다.
                     */
                    if ( ( aNode1->node.lflag & MTC_NODE_DISTINCT_MASK ) !=
                         ( aNode2->node.lflag & MTC_NODE_DISTINCT_MASK ) )
                    {
                        sIsEquivalent = ID_FALSE;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    /* Nothing to do */
                }

                *aIsEquivalent = sIsEquivalent;
            }
            else
            {
                *aIsEquivalent = ID_FALSE;
            }
        }
        /* PROJ-2415 Grouping Sets Clause
         * 기존 Null Value와 Empty Group으로 생성 된 Null Value가 비교되는 것 을 막기위해
         * QTC_NODE_EMPTY_GROUP_TRUE 를 검사한다.
         */
        else if ( ( aNode1->node.arguments == NULL ) &&
                  ( aNode2->node.arguments == NULL ) &&
                  ( ( aNode1->lflag & QTC_NODE_EMPTY_GROUP_MASK ) != QTC_NODE_EMPTY_GROUP_TRUE ) &&
                  ( ( aNode2->lflag & QTC_NODE_EMPTY_GROUP_MASK ) != QTC_NODE_EMPTY_GROUP_TRUE ) )
        {
            /* Terminal Node */

            if ( isSameModuleByName( aNode1, aNode2 ) == ID_TRUE )
            {
                sIsEquivalent = ID_TRUE;

                if ( ( aNode1->node.lflag & MTC_NODE_OPERATOR_MASK )
                                         == MTC_NODE_OPERATOR_SUBQUERY ) /* subquery */
                {
                    sIsEquivalent = ID_FALSE;
                }
                else
                {
                    if (aNode1->node.module == &(qtc::valueModule))
                    {
                        if ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->columnName, aNode2->columnName ) != ID_TRUE )
                        {
                            sIsEquivalent = ID_FALSE;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else if ( aNode1->node.module == &(qtc::columnModule) )
                    {
                        /* PROJ-2179
                         * PRIOR 절의 존재 여부가 같아야 한다.
                         */
                        if( ( aNode1->lflag & QTC_NODE_PRIOR_MASK ) !=
                            ( aNode2->lflag & QTC_NODE_PRIOR_MASK ) )
                        {
                            sIsEquivalent = ID_FALSE;
                        }
                        else
                        {
                            if ( ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->userName, aNode2->userName ) != ID_TRUE ) ||
                                 ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->tableName, aNode2->tableName ) != ID_TRUE ) ||
                                 ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->columnName, aNode2->columnName ) != ID_TRUE ) ||
                                 ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->pkgName, aNode2->pkgName ) != ID_TRUE ) )
                            {
                                sIsEquivalent = ID_FALSE;
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        }
                    }
                    else if ( aNode1->node.module == &(qtc::spFunctionCallModule) )
                    {
                        if ( ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->userName, aNode2->userName ) != ID_TRUE ) ||
                             ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->tableName, aNode2->tableName ) != ID_TRUE ) ||
                             ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->columnName, aNode2->columnName ) != ID_TRUE ) ||
                             ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->pkgName, aNode2->pkgName ) != ID_TRUE ) )
                        {
                            sIsEquivalent = ID_FALSE;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else if ( aNode1->node.module == &mtfCount )
                    {
                        /* PROJ-2179 COUNT(*)와 같이 인자가 없는 aggregate function */
                        /* Nothing to do */
                    }
                    else
                    {
                        if ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->position, aNode2->position ) != ID_TRUE )
                        {
                            sIsEquivalent = ID_FALSE;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                }

                *aIsEquivalent = sIsEquivalent;
            }
            else
            {
                *aIsEquivalent = ID_FALSE;
            }
        }
        else
        {
            *aIsEquivalent = ID_FALSE;
        }
    }
    else
    {
        *aIsEquivalent = ID_FALSE;
    }

    //-------------------------------
    // over절 검사
    //-------------------------------
    if ( *aIsEquivalent == ID_TRUE )
    {
        if ( ( aNode1->overClause != NULL ) &&
             ( aNode2->overClause != NULL ) )
        {
            /* 두 노드 모두 over절이 있는 경우 */

            sCurOverColumn1 = aNode1->overClause->overColumn;
            sCurOverColumn2 = aNode2->overClause->overColumn;

            while ( ( sCurOverColumn1 != NULL ) && ( sCurOverColumn2 != NULL ) )
            {
                IDE_TEST( isEquivalentExpressionByName(
                              sCurOverColumn1->node,
                              sCurOverColumn2->node,
                              &sIsEquivalent )
                          != IDE_SUCCESS );

                /* PROJ-2179
                 * OVER절의 ORDER BY의 조건을 확인한다.
                 */

                sFlag1 = sCurOverColumn1->flag & QTC_OVER_COLUMN_MASK;
                sFlag2 = sCurOverColumn2->flag & QTC_OVER_COLUMN_MASK;

                if ( sFlag1 != sFlag2 )
                {
                    sIsEquivalent = ID_FALSE;
                }
                else
                {
                    if ( sFlag1 == QTC_OVER_COLUMN_ORDER_BY )
                    {
                        sFlag1 = sCurOverColumn1->flag & QTC_OVER_COLUMN_ORDER_MASK;
                        sFlag2 = sCurOverColumn2->flag & QTC_OVER_COLUMN_ORDER_MASK;

                        if ( sFlag1 != sFlag2 )
                        {
                            sIsEquivalent = ID_FALSE;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                if ( sIsEquivalent == ID_TRUE )
                {
                    sCurOverColumn1 = sCurOverColumn1->next;
                    sCurOverColumn2 = sCurOverColumn2->next;
                }
                else
                {
                    break; /* exit while loop */
                }
            }

            if ( ( sCurOverColumn1 != NULL ) || ( sCurOverColumn2 != NULL ) )
            {
                /* overColumn->next의 수가 맞지 않는 경우 */
                sIsEquivalent = ID_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else if ( ( aNode1->overClause == NULL ) &&
                  ( aNode2->overClause == NULL ) )
        {
            /* 두 노드 모두 over절이 없는 경우 */
            /* Nothing to do */
        }
        else
        {
            /* 어느 한쪽에만 over절이 있는 경우 */
            sIsEquivalent = ID_FALSE;
        }

        *aIsEquivalent = sIsEquivalent;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : module이 같은지 비교(isEquivalentExpressionByName에서 사용)
 *
 ***********************************************************************/
idBool qtc::isSameModuleByName( qtcNode * aNode1,
                                qtcNode * aNode2 )
{
    idBool sIsSame;

    if ( aNode1->node.module == aNode2->node.module )
    {
        if ( aNode1->node.module == &(qtc::spFunctionCallModule) )
        {
            if ( ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->userName, aNode2->userName ) != ID_TRUE ) ||
                 ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->tableName, aNode2->tableName ) != ID_TRUE ) ||
                 ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->columnName, aNode2->columnName ) != ID_TRUE ) ||
                 ( QC_IS_NAME_MATCHED_OR_EMPTY( aNode1->pkgName, aNode2->pkgName ) != ID_TRUE ) )
            {
                sIsSame = ID_FALSE;
            }
            else
            {
                sIsSame = ID_TRUE;
            }
        }
        else
        {
            sIsSame = ID_TRUE;
        }
    }
    else
    {
        sIsSame = ID_FALSE;
    }

    return sIsSame;
}

IDE_RC qtc::changeNodeFromColumnToSP( qcStatement * aStatement,
                                      qtcNode     * aNode,
                                      mtcTemplate * aTemplate,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      mtcCallBack * aCallBack,
                                      idBool      * aFindColumn )
{
/***********************************************************************
 *
 * Description :
 *    column node를 stored function node로 변경한다.
 *
 * Implementation :
 *    1. change module
 *    2. estimate node
 *
 ***********************************************************************/

#define IDE_FN "IDE_RC qtc::changeNodeFromColumnToSP"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt                  sErrCode;

    // update module pointer
    aNode->node.module = & qtc::spFunctionCallModule;
    aNode->node.lflag  = qtc::spFunctionCallModule.lflag &
                                    ~MTC_NODE_COLUMN_COUNT_MASK;

    aNode->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
    aNode->lflag |= QTC_NODE_PROC_FUNCTION_TRUE;


    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                               aNode,
                               aStatement,
                               QC_SHARED_TMPLATE(aStatement),
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               qtc::spFunctionCallModule.lflag &
                                   MTC_NODE_COLUMN_COUNT_MASK )
        != IDE_SUCCESS );

    if (qtc::estimateInternal( aNode,
                               aTemplate,
                               aStack,
                               aRemain,
                               aCallBack ) == IDE_SUCCESS )
    {
        *aFindColumn = ID_TRUE;
    }
    else
    {
        sErrCode = ideGetErrorCode();

        // PROJ-1083 Package
        // sErrCode 추가
        if ( sErrCode == qpERR_ABORT_QSV_NOT_EXIST_PROC_SQLTEXT ||
             sErrCode == qpERR_ABORT_QCM_NOT_EXIST_USER ||
             sErrCode == qpERR_ABORT_QSV_INVALID_IDENTIFIER ||
             sErrCode == qpERR_ABORT_QSV_NOT_EXIST_PKG )
        {
            IDE_CLEAR();
        }
        else
        {
            IDE_RAISE( ERR_PROC_VALIDATE );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_PROC_VALIDATE)
    {
        // Do not set error code
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::setSubAggregation( qtcNode * aNode )
{
#define IDE_FN "IDE_RC qtc::setSubAggregation"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if( ( aNode->lflag & QTC_NODE_AGGREGATE_MASK )
        == QTC_NODE_AGGREGATE_EXIST )
    {
        aNode->indexArgument = 1;
    }
    if( aNode->node.next != NULL )
    {
        IDE_TEST( setSubAggregation( (qtcNode*)(aNode->node.next) ) != IDE_SUCCESS );
    }
    if( aNode->node.arguments != NULL )
    {
        IDE_TEST( setSubAggregation( (qtcNode*)(aNode->node.arguments) ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qtc::getDataOffset( qcStatement * aStatement,
                           UInt          aSize,
                           UInt        * aOffsetPtr )
{
/***********************************************************************
 *
 * Description : aOffsetPtr에 template의 data 위치를 확보한다.
 *               template의 dataSize의 consistency를 보장하기 위해
 *               이 함수를 통해서 offset 위치를 얻어야 한다.
 *
 * Implementation : datasize는 항상 8바이트 align되어 있도록 한다.
 *
 **********************************************************************/

#define IDE_FN "IDE_RC qtc::getDataOffset"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( aStatement != NULL );

    *aOffsetPtr = QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize;
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize += aSize;

    // align 조정
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize =
        idlOS::align8( QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize );

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qtc::addSDF( qcStatement * aStatement, qmoScanDecisionFactor * aSDF )
{
    qmoScanDecisionFactor * sLast;
    qcStatement           * sTopStatement;

    IDE_DASSERT( aStatement != NULL );

    // Optimize 과정에서 view등은 하위 statement로 만들어진다.
    // 하위 statement의 sdf에 추가하면 안되고,
    // 항상 최 상위 statement에서 sdf를 관리해야 하므로
    // 상위 statement를 가져온다.

    sTopStatement = QC_SHARED_TMPLATE(aStatement)->stmt;

    if( sTopStatement->myPlan->scanDecisionFactors == NULL )
    {
        sTopStatement->myPlan->scanDecisionFactors = aSDF;
    }
    else
    {
        for( sLast = sTopStatement->myPlan->scanDecisionFactors;
             sLast->next != NULL;
             sLast = sLast->next ) ;

        sLast->next = aSDF;
    }

    return IDE_SUCCESS;
}

IDE_RC qtc::removeSDF( qcStatement * aStatement, qmoScanDecisionFactor * aSDF )
{
    qmoScanDecisionFactor * sPrev;
    qcStatement           * sTopStatement;

    IDE_DASSERT( aStatement != NULL );

    // Optimize 과정에서 view등은 하위 statement로 만들어진다.
    // 하위 statement의 sdf에 추가하면 안되고,
    // 항상 최 상위 statement에서 sdf를 관리해야 하므로
    // 상위 statement를 가져온다.
    sTopStatement = QC_SHARED_TMPLATE(aStatement)->stmt;

    IDE_ASSERT( sTopStatement->myPlan->scanDecisionFactors != NULL );

    if( sTopStatement->myPlan->scanDecisionFactors == aSDF )
    {
        sTopStatement->myPlan->scanDecisionFactors = aSDF->next;
    }
    else
    {
        for( sPrev = sTopStatement->myPlan->scanDecisionFactors;
             sPrev->next != aSDF;
             sPrev = sPrev->next )
        {
            IDE_DASSERT( sPrev != NULL );
        }

        sPrev->next = aSDF->next;
    }

    return IDE_SUCCESS;
}

idBool qtc::getConstPrimitiveNumberValue( qcTemplate  * aTemplate,
                                          qtcNode     * aNode,
                                          SLong       * aNumberValue )
{
/***********************************************************************
 *
 * Description : PROJ-1405 ROWNUM
 *               정수형 constant value의 값을 가져온다.
 *
 * Implementation :
 *
 **********************************************************************/

    const mtcColumn  * sColumn;
    const void       * sValue;
    SLong              sNumberValue;
    idBool             sIsNumber;

    sNumberValue = 0;
    sIsNumber = ID_FALSE;

    if ( isConstValue( aTemplate, aNode ) == ID_TRUE )
    {
        sColumn = QTC_TMPL_COLUMN( aTemplate, aNode );
        sValue  = QTC_TMPL_FIXEDDATA( aTemplate, aNode );

        if ( sColumn->module->isNull( sColumn, sValue ) == ID_TRUE )
        {
            // Nothing to do.
        }
        else
        {
            // BUG-17791
            if ( sColumn->module->id == MTD_SMALLINT_ID )
            {
                sIsNumber = ID_TRUE;
                sNumberValue = (SLong) (*(mtdSmallintType*)sValue);
            }
            else if ( sColumn->module->id == MTD_INTEGER_ID )
            {
                sIsNumber = ID_TRUE;
                sNumberValue = (SLong) (*(mtdIntegerType*)sValue);
            }
            else if ( sColumn->module->id == MTD_BIGINT_ID )
            {
                sIsNumber = ID_TRUE;
                sNumberValue = (SLong) (*(mtdBigintType*)sValue);
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    *aNumberValue = sNumberValue;

    return sIsNumber;
}

//PROJ-1583 large geometry
UInt
qtc::getSTObjBufSize( mtcCallBack * aCallBack )
{
    qtcCallBackInfo * sCallBackInfo    = (qtcCallBackInfo*)(aCallBack->info);

    // BUG-27619
    qcStatement*      sStatement       = sCallBackInfo->tmplate->stmt;
    qmsSFWGH*         sSFWGHOfCallBack = sCallBackInfo->SFWGH;
    qmsHints        * sHints           = NULL;
    qcSharedPlan    * sMyPlan          = NULL;
    UInt              sObjBufSize      = QMS_NOT_DEFINED_ST_OBJECT_BUFFER_SIZE;

    IDE_DASSERT( sStatement != NULL );
    sMyPlan = sStatement->myPlan;

    if( sSFWGHOfCallBack != NULL )
    {
        sHints = sSFWGHOfCallBack->hints;
    }

    if( sHints != NULL )
    {
        if( sHints->STObjBufSize != QMS_NOT_DEFINED_ST_OBJECT_BUFFER_SIZE )
        {
            sObjBufSize = sHints->STObjBufSize;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    // To Fix BUG-32072

    if ( sObjBufSize == QMS_NOT_DEFINED_ST_OBJECT_BUFFER_SIZE )
    {
        if ( sMyPlan != NULL )
        {
            if ( sMyPlan->planEnv != NULL )
            {
                sObjBufSize = sMyPlan->planEnv->planProperty.STObjBufSize;
            }
            else
            {
                // Nothing to do
            }
            
        }
        else
        {
            // Nothing to do 
        }
    }
    else
    {
        //Nothing to do
    }

    if ( sObjBufSize == QMS_NOT_DEFINED_ST_OBJECT_BUFFER_SIZE )
    {
        // BUG-27619
        if (sStatement != NULL)
        {
            
            sObjBufSize = QCG_GET_SESSION_ST_OBJECT_SIZE( sStatement );

            // environment의 기록
            qcgPlan::registerPlanProperty( sStatement,
                                           PLAN_PROPERTY_ST_OBJECT_SIZE );
        }
        else
        {
            sObjBufSize = QCU_ST_OBJECT_BUFFER_SIZE;
        }
    }
    else
    {
        // Nothing to do.
    }

    return sObjBufSize;
}

IDE_RC
qtc::allocAndInitColumnLocateInTuple( iduVarMemList * aMemory,
                                      mtcTemplate   * aTemplate,
                                      UShort          aRowID )
{
    UShort   i;

    IDU_LIMITPOINT("qtc::allocAndInitColumnLocateInTuple::malloc");
    IDE_TEST(
        aMemory->cralloc(
            idlOS::align8((UInt)ID_SIZEOF(mtcColumnLocate))
            * aTemplate->rows[aRowID].columnMaximum,
            (void**)
            &(aTemplate->rows[aRowID].columnLocate ) )
        != IDE_SUCCESS );

    for( i = 0;
         i < aTemplate->rows[aRowID].columnMaximum;
         i++ )
    {
        aTemplate->rows[aRowID].columnLocate[i].table
            = aRowID;
        aTemplate->rows[aRowID].columnLocate[i].column
            = i;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::setBindParamInfoByNode( qcStatement * aStatement,
                                    qtcNode     * aColumnNode,
                                    qtcNode     * aBindNode )
{
/***********************************************************************
 * 
 * Description : Bind Param Info 정보를 설정함
 *
 * Implementation :
 *    aColumnNode를 통해 Column Info를 획득하여
 *    aBindNode에 대응되는 BindParam 정보를 설정한다.
 ***********************************************************************/

    qciBindParam * sBindParam;
    mtcColumn    * sColumnInfo = NULL;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aColumnNode != NULL );
    IDE_DASSERT( aBindNode != NULL );
    
    //-------------------
    // Column Info 획득
    //-------------------
    
    sColumnInfo = QTC_STMT_COLUMN( aStatement, aColumnNode );
    
    //-------------------
    // Column Info를 보고 Bind Parameter 정보 설정
    //-------------------
    sBindParam = &aStatement->myPlan->sBindParam[aBindNode->node.column].param;

    // PROJ-2002 Column Security
    // 암호 데이타 타입은 원본 데이타 타입으로 Bind Parameter 정보 설정
    if( sColumnInfo->type.dataTypeId == MTD_ECHAR_ID )
    {
        sBindParam->type      = MTD_CHAR_ID;
        sBindParam->language  = sColumnInfo->type.languageId;
        sBindParam->arguments = 1;
        sBindParam->precision = sColumnInfo->precision;
        sBindParam->scale     = 0;
    }
    else if( sColumnInfo->type.dataTypeId == MTD_EVARCHAR_ID )
    {
        sBindParam->type      = MTD_VARCHAR_ID;
        sBindParam->language  = sColumnInfo->type.languageId;
        sBindParam->arguments = 1;
        sBindParam->precision = sColumnInfo->precision;
        sBindParam->scale     = 0;
    }
    /* BUG-43294 The server raises an assertion becauseof bindings to NULL columns */
    else if( sColumnInfo->type.dataTypeId == MTD_NULL_ID )
    {
        sBindParam->type      = MTD_VARCHAR_ID;
        sBindParam->language  = sColumnInfo->type.languageId;
        sBindParam->arguments = 1;
        sBindParam->precision = 1;
        sBindParam->scale     = 0;
    }
    else
    {
        sBindParam->type      = sColumnInfo->type.dataTypeId;
        sBindParam->language  = sColumnInfo->type.languageId;
        sBindParam->arguments = sColumnInfo->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK;
        sBindParam->precision = sColumnInfo->precision;
        sBindParam->scale     = sColumnInfo->scale;
    }
    
    return IDE_SUCCESS;
}

IDE_RC qtc::setDescribeParamInfo4Where( qcStatement     * aStatement,
                                        qtcNode         * aNode )
{
/***********************************************************************
 * 
 * Description : WHERE 절에서 DescribeParamInfo 설정 가능한 경우,
 *               이를 찾아 설정
 *
 * Implementation :
 *    WHERE 절에서 DescribeParamInfo 설정 가능한 경우,
 *    - 하위 node에 bind node가 있음
 *    - 비교 연산자임  ( =, >, <, >=, <= )
 *    - (Column)(비교연산자)(Bind) or (Bind)(비교연산자)(Column) 형태임
 *
 *    현재 node를 next를 따라 진행하면서 아래와 같은 작업 수행함
 *    (1) 하위 node에 bind node가 있는지 검사
 *    (2) 비교 연산자인지 검사
 *        - 비교 연산자 이면 (3) 으로 진행
 *        - 비교 연산자가 아니면, 현재 node의 argument에 대하여
 *          recursive하게 (1)을 수행함
 *    (3) (Column)(비교연산자)(Bind) or (Bind)(비교연산자)(Column) 형태
 *         이면 Column정보를 획득하여 Bind Param 정보 설정
 *
 ***********************************************************************/

    qtcNode      * sCurNode;
    qtcNode      * sLeftOperand;
    qtcNode      * sRightOperand;
    qtcNode      * sColumnNode;
    qtcNode      * sBindNode;

    for ( sCurNode = aNode;
          sCurNode != NULL;
          sCurNode = (qtcNode*)sCurNode->node.next )
    {
        if ( ( sCurNode->node.lflag & MTC_NODE_BIND_MASK )
             == MTC_NODE_BIND_EXIST )
        {
            //----------------------------
            // bind node가 있는 경우,
            //----------------------------

            // BUG-34327
            // >ALL도 MTC_NODE_OPERATOR_GREATER이므로 flag로 비교하지 않고
            // mtfModule로 직접 비교한다.
            if ( ( sCurNode->node.module == & mtfEqual )       ||
                 ( sCurNode->node.module == & mtfNotEqual )    ||
                 ( sCurNode->node.module == & mtfLessThan )    ||
                 ( sCurNode->node.module == & mtfLessEqual )   ||
                 ( sCurNode->node.module == & mtfGreaterThan ) ||
                 ( sCurNode->node.module == & mtfGreaterEqual ) )
            {
                //----------------------------
                // 현재 node가 비교 연산자인 경우 ( =, !=, >, <, >=, <= )
                //----------------------------

                sLeftOperand = (qtcNode*)sCurNode->node.arguments;
                sRightOperand = (qtcNode*)sCurNode->node.arguments->next;

                //----------------------------
                // (Column)(비교연산자)(Bind) or
                // (Bind)(비교연산자)(Column) 형태를 찾음
                //----------------------------
                sColumnNode = NULL;
                sBindNode   = NULL;
                
                if ( ( QTC_IS_TERMINAL( sLeftOperand ) == ID_TRUE ) &&
                     ( QTC_IS_TERMINAL( sRightOperand ) == ID_TRUE ) )
                {
                    if ( ( sLeftOperand->node.lflag & MTC_NODE_BIND_MASK )
                         == MTC_NODE_BIND_EXIST )
                    {
                        // Left operand가 bind node인 경우,
                        // Right operand는 column 이어야 함
                        if ( QTC_IS_COLUMN( aStatement, sRightOperand )
                             == ID_TRUE )
                        {
                            sColumnNode = sRightOperand;
                            sBindNode   = sLeftOperand;
                        }
                        else
                        {
                            // constant 연산자, bind node 등등의
                            // 경우임
                        }
                    }
                    else
                    {
                        // Right operand가 bind node인 경우,
                        // Left operand가 column 이어야 함
                        if ( QTC_IS_COLUMN( aStatement, sLeftOperand )
                             == ID_TRUE )
                        {
                            sColumnNode = sLeftOperand;
                            sBindNode   = sRightOperand;
                        }
                        else
                        {
                            // constant 연산자, bind node 등등의
                            // 경우임
                        }
                    }
                }
                else
                {    
                    // left operand 또는 right operand가
                    // terminal이 아닌 경우,
                    // next node로 진행
                }

                //-------------------
                // Bind Param 정보 설정
                //-------------------

                if ( ( sColumnNode != NULL ) && ( sBindNode != NULL ) )
                {
                    // (Column)(비교연산자)(Bind) or
                    // (Bind)(비교연산자)(Column) 형태를 찾은 경우,
                    // BindParamInfo 설정
                    IDE_TEST( setBindParamInfoByNode( aStatement,
                                                      sColumnNode,
                                                      sBindNode )
                              != IDE_SUCCESS );
                    
                    sColumnNode = NULL;
                    sBindNode = NULL;
                }
                else
                {
                    // nothing to do 
                    // (ex. 1 = ?, ? = ? etc... )
                }
            }
            else
            {
                // nothing to do 
            }
            
            if ( ( sCurNode->node.module == & mtfLike ) ||
                 ( sCurNode->node.module == & mtfNotLike ) )
            {
                //----------------------------
                // 현재 node가 like 연산자인 경우 (like, not like)
                //----------------------------
                    
                sLeftOperand = (qtcNode*)sCurNode->node.arguments;
                sRightOperand = (qtcNode*)sCurNode->node.arguments->next;

                //----------------------------
                // (Column)(비교연산자)(Bind) 형태를 찾음
                //----------------------------
                sColumnNode = NULL;
                sBindNode   = NULL;
                
                if ( ( QTC_IS_TERMINAL( sLeftOperand ) == ID_TRUE ) &&
                     ( QTC_IS_TERMINAL( sRightOperand ) == ID_TRUE ) )
                {
                    if ( ( sRightOperand->node.lflag & MTC_NODE_BIND_MASK )
                         == MTC_NODE_BIND_EXIST )
                    {
                        // Right operand가 bind node인 경우,
                        // Left operand가 column 이어야 함
                        if ( QTC_IS_COLUMN( aStatement, sLeftOperand )
                             == ID_TRUE )
                        {
                            sColumnNode = sLeftOperand;
                            sBindNode   = sRightOperand;
                        }
                        else
                        {
                            // constant 연산자, bind node 등등의
                            // 경우임
                        }
                    }
                    else
                    {
                        // Left operand가 bind node인 경우
                    }
                }
                else
                {    
                    // left operand 또는 right operand가
                    // terminal이 아닌 경우,
                    // next node로 진행
                }
                    
                //-------------------
                // Bind Param 정보 설정
                //-------------------

                if ( ( sColumnNode != NULL ) && ( sBindNode != NULL ) )
                {
                    // (Column)(비교연산자)(Bind) or
                    // (Bind)(비교연산자)(Column) 형태를 찾은 경우,
                    // BindParamInfo 설정
                    IDE_TEST( setBindParamInfoByNode( aStatement,
                                                      sColumnNode,
                                                      sBindNode )
                              != IDE_SUCCESS );
                        
                    sColumnNode = NULL;
                    sBindNode = NULL;
                }
                else
                {
                    // nothing to do 
                }
            }
            else
            {
                // nothing to do
            }
            
            if ( sCurNode->node.module == & mtfCast )
            {
                //----------------------------
                // 현재 node가 cast 연산자인 경우
                //----------------------------
                    
                sLeftOperand = (qtcNode*)sCurNode->node.arguments;

                //----------------------------
                // cast((Bind) as (Column)) 형태를 찾음
                //----------------------------
                sColumnNode = NULL;
                sBindNode   = NULL;
                        
                if ( ( QTC_IS_TERMINAL( sLeftOperand ) == ID_TRUE )
                     &&
                     ( (sLeftOperand->node.lflag & MTC_NODE_BIND_MASK)
                       == MTC_NODE_BIND_EXIST ) )
                {
                    // Left operand가 bind node인 경우,
                    sColumnNode = (qtcNode*)sCurNode->node.funcArguments;
                    sBindNode   = sLeftOperand;
                }
                else
                {
                    // nothing to do
                }
                        
                //-------------------
                // Bind Param 정보 설정
                //-------------------

                if ( ( sColumnNode != NULL ) && ( sBindNode != NULL ) )
                {
                    // (Column)(비교연산자)(Bind) or
                    // (Bind)(비교연산자)(Column) 형태를 찾은 경우,
                    // BindParamInfo 설정
                    IDE_TEST( setBindParamInfoByNode( aStatement,
                                                      sColumnNode,
                                                      sBindNode )
                              != IDE_SUCCESS );
                        
                    sColumnNode = NULL;
                    sBindNode = NULL;
                }
                else
                {
                    // nothing to do 
                }
            }
            else
            {
                // nothing to do 
            }
            
            // 비교 연산자가 아닌 경우,
            IDE_TEST(
                setDescribeParamInfo4Where(
                    aStatement,
                    (qtcNode*)sCurNode->node.arguments )
                != IDE_SUCCESS );
        }
        else
        {
            // bind node가 없는 경우,
            // nothing to do 
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::fixAfterOptimization( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *
 *    Optimization 완료 후의 처리
 *
 * Implementation :
 *
 *    Tuple에 할당된 불필요한 공간을 해제하여 최적화한다.
 *
 ***********************************************************************/

    qcTemplate  * sTemplate = QC_SHARED_TMPLATE(aStatement);
    mtcTuple    * sTuple;
    mtcColumn   * sNewColumns;
    mtcColumn   * sOldColumns;
    mtcExecute  * sNewExecute;
    mtcExecute  * sOldExecute;
    UShort        sRow;

    // BUG-34350
    if( QCU_OPTIMIZER_REFINE_PREPARE_MEMORY == 1 )
    {
        for ( sRow = 0; sRow < sTemplate->tmplate.rowCount; sRow++ )
        {
            sTuple = & sTemplate->tmplate.rows[sRow];
 
            IDE_DASSERT( sTuple->columnCount <= sTuple->columnMaximum );
 
            //------------------------------------------------
            // 사용하지 않은 column, execute를 제거
            //------------------------------------------------
 
            if ( sTuple->columnCount < sTuple->columnMaximum )
            {
                sOldColumns = sTuple->columns;
                sOldExecute = sTuple->execute;
 
                if ( sTuple->columnCount == 0 )
                {
                    // mtcColumn
                    IDE_TEST( QC_QMP_MEM(aStatement)->free( sOldColumns )
                              != IDE_SUCCESS );
                    sNewColumns = NULL;
 
                    // mtcEecute
                    IDE_TEST( QC_QMP_MEM(aStatement)->free( sOldExecute )
                              != IDE_SUCCESS );
                    sNewExecute = NULL;
                }
                else
                {
                    // mtcColumn
                    IDU_LIMITPOINT("qtc::fixAfterOptimization::malloc1");
                    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtcColumn) * sTuple->columnCount,
                                                             (void**) & sNewColumns )
                              != IDE_SUCCESS );
 
                    idlOS::memcpy( (void*) sNewColumns,
                                   (void*) sOldColumns,
                                   ID_SIZEOF(mtcColumn) * sTuple->columnCount );
 
                    IDE_TEST( QC_QMP_MEM(aStatement)->free( sOldColumns )
                              != IDE_SUCCESS );
 
                    // mtcEecute
                    IDU_LIMITPOINT("qtc::fixAfterOptimization::malloc2");
                    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(mtcExecute) * sTuple->columnCount,
                                                             (void**) & sNewExecute )
                              != IDE_SUCCESS );
 
                    idlOS::memcpy( (void*) sNewExecute,
                                   (void*) sOldExecute,
                                   ID_SIZEOF(mtcExecute) * sTuple->columnCount );
 
                    IDE_TEST( QC_QMP_MEM(aStatement)->free( sOldExecute )
                              != IDE_SUCCESS );
                }
 
                sTuple->columnMaximum = sTuple->columnCount;
                sTuple->columns = sNewColumns;
                sTuple->execute = sNewExecute;  
            }
            else
            {
                // Nothing to do.
            }
 
            //------------------------------------------------
            // mtcColumnLocate를 제거
            //------------------------------------------------
 
            if ( sTuple->columnLocate != NULL )
            {
                // mtcColumnLocate는 optimization이후에는
                // 더이상 사용되지 않으므로 삭제한다.
                IDE_TEST( QC_QMP_MEM(aStatement)->free( sTuple->columnLocate )
                          != IDE_SUCCESS );
 
                sTuple->columnLocate = NULL;
            }
            else
            {
                // Nothing to do.
            }

            /* BUG-42639 Fixed Table + Disk Temp 사용시FATAL
             * Fiexed Table만 있는 쿼리의 경우 Transaction 없이 동작하도록
             * 했는데 Disk Temp 생성시에는 Transaction이 필요하다
             */
            if ( ( sTuple->lflag & MTC_TUPLE_STORAGE_MASK )
                 == MTC_TUPLE_STORAGE_DISK )
            {
                QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
                QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        // Skip refining prepare memory
        // It reduces prepare time but causes wasting prepare memory.

        /* BUG-42639 Fixed Table + Disk Temp 사용시FATAL
         * Fiexed Table만 있는 쿼리의 경우 Transaction 없이 동작하도록
         * 했는데 Disk Temp 생성시에는 Transaction이 필요하다
         */
        for ( sRow = 0; sRow < sTemplate->tmplate.rowCount; sRow++ )
        {
            sTuple = & sTemplate->tmplate.rows[sRow];

            IDE_DASSERT( sTuple->columnCount <= sTuple->columnMaximum );

            if ( ( sTuple->lflag & MTC_TUPLE_STORAGE_MASK )
                 == MTC_TUPLE_STORAGE_DISK )
            {
                QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
                QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool
qtc::isSameModule( qtcNode* aNode1,
                   qtcNode* aNode2 )
{
/***********************************************************************
 *
 * Description : module이 같은지 비교(isEquivalentExpression에서 사용)
 *               To fix BUG-21441
 *
 * Implementation :
 *    (1) module이 다르면 false
 *    (2) module은 같지만, sp인경우 oid가 다르면 false
 *    (3) module이 같고, 만약 sp인 경우 oid가 같으면 true
 *
 ***********************************************************************/

    idBool sIsSame;

    if( aNode1->node.module == aNode2->node.module )
    {
        if( aNode1->node.module ==
            &(qtc::spFunctionCallModule) )
        {
            IDE_ASSERT( (aNode1->subquery != NULL) &&
                        (aNode2->subquery != NULL) );

            if( ((qsExecParseTree*)
                 (aNode1->subquery->myPlan->parseTree))->procOID ==
                ((qsExecParseTree*)
                 (aNode2->subquery->myPlan->parseTree))->procOID )
            {
                sIsSame = ID_TRUE;
            }
            else
            {
                sIsSame = ID_FALSE;
            }
        }
        else
        {
            sIsSame = ID_TRUE;
        }
    }
    else
    {
        sIsSame = ID_FALSE;
    }
    
    return sIsSame;
}

mtcNode*
qtc::getCaseSubExpNode( mtcNode* aNode )
{
/***********************************************************************
 *
 * Description : case expression node반환
 *
 * BUG-28223 CASE expr WHEN .. THEN .. 구문의 expr에 subquery 사용시 에러발생
 *
 * Implementation :
 *
 ***********************************************************************/  

    mtcNode* sNode;
    mtcNode* sConversion;
    
    for( sNode = aNode;
         ( sNode->lflag & MTC_NODE_INDIRECT_MASK ) == MTC_NODE_INDIRECT_TRUE;
         sNode = sNode->arguments ) ;
    
    sConversion = sNode->conversion;
    
    if ( sConversion != NULL )
    {
        if( sConversion->info != ID_UINT_MAX )
        {
            sConversion = sNode;            
        }
        if( sConversion != NULL )
        {
            sNode = sConversion;
        }
    }
    
    return sNode;
}

idBool qtc::isQuotedName( qcNamePosition * aPosition )
{
    idBool  sIsQuotedName = ID_FALSE;
    
    if ( aPosition != NULL )
    {
        if ( (aPosition->stmtText != NULL)
             &&
             (aPosition->offset > 0)
             &&
             (aPosition->size > 0) )
        {
            if ( aPosition->stmtText[aPosition->offset - 1] == '"' )
            {
                // stmtText는 NTS로 1byte는 더 읽어도 괜찮다.
                if ( aPosition->stmtText[aPosition->offset + aPosition->size] == '"' )
                {
                    sIsQuotedName = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return sIsQuotedName;
}

/**
 * PROJ-2208 getNLSCurrencyCallback
 *
 *  MT 쪽에 등록되는 callback 함수로 mtcTemplate를 인자로 받아 세션을 찾고 이 세션에
 *  해당하는 Currency를 찾아서 설정한다. 만약 세션이 존재하지 않으면
 *  Property 값을 설정한다.
 *
 */
IDE_RC qtc::getNLSCurrencyCallback( mtcTemplate * aTemplate,
                                    mtlCurrency * aCurrency )
{
    qcTemplate * sTemplate = ( qcTemplate * )aTemplate;
    SChar      * sTemp = NULL;
    SInt         sLen  = 0;

    sTemp = QCG_GET_SESSION_ISO_CURRENCY( sTemplate->stmt );

    idlOS::memcpy( aCurrency->C, sTemp, MTL_TERRITORY_ISO_LEN );
    aCurrency->C[MTL_TERRITORY_ISO_LEN] = 0;

    sTemp = QCG_GET_SESSION_CURRENCY( sTemplate->stmt );
    sLen  = idlOS::strlen( sTemp );

    if ( sLen <= MTL_TERRITORY_CURRENCY_LEN )
    {
        idlOS::memcpy( aCurrency->L, sTemp, sLen );
        aCurrency->L[sLen] = 0;
        aCurrency->len = sLen;
    }
    else
    {
        idlOS::memcpy( aCurrency->L, sTemp, MTL_TERRITORY_CURRENCY_LEN );
        aCurrency->L[MTL_TERRITORY_CURRENCY_LEN] = 0;
        aCurrency->len = MTL_TERRITORY_CURRENCY_LEN;
    }

    sTemp = QCG_GET_SESSION_NUMERIC_CHAR( sTemplate->stmt );
    aCurrency->D = *( sTemp );
    aCurrency->G = *( sTemp + 1 );

    return IDE_SUCCESS;
}

/**
 * PROJ-1353
 *
 *   GROUPING, GROUPING_ID 와 같이 특수하게 2번 estimate가 필요한경우 바로 노드를
 *   estimate하기 위해 필요하다. estimateInternal 이 2번 수행되면 꼬이게 되는 것을
 *   방지하기 위해서임.
 */
IDE_RC qtc::estimateNodeWithSFWGH( qcStatement * aStatement,
                                   qmsSFWGH    * aSFWGH,
                                   qtcNode     * aNode )
{
    qtcCallBackInfo sCallBackInfo = {
        QC_SHARED_TMPLATE(aStatement),
        QC_QMP_MEM(aStatement),
        aStatement,
        NULL,
        aSFWGH,
        NULL
    };
    mtcCallBack sCallBack = {
        &sCallBackInfo,
        MTC_ESTIMATE_ARGUMENTS_ENABLE,
        alloc,
        initConversionNodeIntermediate
    };

    qcTemplate * sTemplate = QC_SHARED_TMPLATE(aStatement);


    IDE_TEST( estimateNode( aNode,
                            (mtcTemplate *)sTemplate,
                            sTemplate->tmplate.stack,
                            sTemplate->tmplate.stackRemain,
                            &sCallBack )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::getLoopCount( qcTemplate  * aTemplate,
                          qtcNode     * aLoopNode,
                          SLong       * aCount )
{
/***********************************************************************
 *
 * Description :
 *    LOOP clause의 expression을 연산하여 값을 반환한다.
 * 
 * Implementation :
 *
 ***********************************************************************/
    
    const mtcColumn  * sColumn;
    const void       * sValue;

    IDE_TEST( qtc::calculate( aLoopNode, aTemplate )
              != IDE_SUCCESS );
    
    sColumn = aTemplate->tmplate.stack->column;
    sValue  = aTemplate->tmplate.stack->value;

    if ( sColumn->module->id == MTD_BIGINT_ID )
    {
        if ( sColumn->module->isNull( sColumn, sValue ) == ID_TRUE )
        {
            *aCount = 0;
        }
        else
        {
            *aCount = (SLong)(*(mtdBigintType*)sValue);
        }
    }
    else if ( sColumn->module->id == MTD_LIST_ID )
    {
        *aCount = sColumn->precision;
    }
    else if ( sColumn->module->id == MTD_ASSOCIATIVE_ARRAY_ID )
    {
        // PROJ-1904 Extend UDT
        IDE_TEST_RAISE( *(qsxArrayInfo**)sValue == NULL, ERR_INVALID_ARRAY );

        *aCount = qsxArray::getElementsCount( *(qsxArrayInfo**)sValue );
    }
    else if ( ( sColumn->module->id == MTD_RECORDTYPE_ID ) ||
              ( sColumn->module->id == MTD_ROWTYPE_ID ) )
    {
        *aCount = 1;
    }
    else
    {
        // 이미 validation에서 검사했으므로
        // 여기로 들어올 경우는 없다.
        IDE_RAISE( ERR_LOOP_TYPE );
    }

    IDE_TEST_RAISE( *aCount < 0, ERR_LOOP_VALUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LOOP_TYPE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtc::getLoopCount",
                                  "invalid type" ));
    }
    IDE_EXCEPTION( ERR_LOOP_VALUE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_LOOP_VALUE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_ARRAY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_UNINITIALIZED_ARRAY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qtc::hasNameArgumentForPSM( qtcNode * aNode )
{
    qtcNode * sCurrArg = NULL;
    idBool    sExist   = ID_FALSE;

    for ( sCurrArg = (qtcNode *)(aNode->node.arguments);
          sCurrArg != NULL;
          sCurrArg = (qtcNode *)(sCurrArg->node.next) )
    {
        if ( ( sCurrArg->lflag & QTC_NODE_SP_PARAM_NAME_MASK )
               == QTC_NODE_SP_PARAM_NAME_TRUE )
        {
            sExist = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sExist;
}

IDE_RC qtc::changeNodeForArray( qcStatement*    aStatement,
                                qtcNode**       aNode,
                                qcNamePosition* aPosition,
                                qcNamePosition* aPositionEnd,
                                idBool          aIsBracket )
{
/***********************************************************************
 *
 * Description : PROJ-2533
 *    List Expression의 정보를 재조정한다.
 *    name( arguments ) 형태의 function/array에 대해 정보를 재조정한다.
 *    name[ index ] 형태의 array에 대한 정보를 재조정한다.
 *
 * Implementation :
 *    List Expression의 의미를 갖도록 정보를 Setting하며,
 *    해당 String의 위치를 재조정한다.
 *    이 함수로 올 수 있는 경우의 유형
 *        (1) func_name ()
 *            proc_name ()
 *            func_name ( list_expr )
 *            proc_name ( list_expr )
 *             * user-defined function, window function, built-in function
 *        (2) arrvar_name ( list_expr ) -> spFunctionCallModule
 *            arrvar_name [ list_expr ] -> columnModule
 *        (3) winddow function
 *            w_func()  over cluse
 *            w_func( list_expr ) within_clause  ignore_nulls  over_clause
 ***********************************************************************/
    const mtfModule * sModule = NULL;
    idBool            sExist = ID_FALSE;

    if ( aIsBracket == ID_FALSE )
    {
        IDE_TEST( mtf::moduleByName( &sModule,
                                     &sExist,
                                     (void*)(aPosition->stmtText +aPosition->offset),
                                     aPosition->size )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            sModule = & qtc::spFunctionCallModule;
        }
        else
        {
            // Nothing to do.
        }

        // sModule이 mtfList가 아닌 경우는 함수형식으로 사용된 경우임.
        // 이 경우 node change가 일어남.
        // ex) sum(i1) or to_date(i1, 'yy-mm-dd')
        if ( (sModule != &mtfList) || (aNode[0]->node.module == NULL) )
        {
            IDE_DASSERT( aNode[0]->node.module == NULL );

            // node의 module이 null인 경우는 list형식으로
            // expression이 매달려 있는 경우.
            // node의 module이 null이 아닌 경우,
            //  이미 위에서 빈 node를 만들어 놓았음.
            // ex) to_date( i1, 'yy-mm-dd' )
            //
            // list형식인 경우 최상위에 빈 node가 달려있으므로
            // 다음과 같이 빈 node를 change
            //
            //  ( )                    to_date
            //   |                =>     |
            //   i1 - 'yy-mm-dd'        i1 - 'yy-mm-dd'
            aNode[0]->node.lflag  |= sModule->lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
            aNode[0]->node.module  = sModule;

            if ( &qtc::spFunctionCallModule == aNode[0]->node.module )
            {
                // fix BUG-14257
                aNode[0]->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
                aNode[0]->lflag |= QTC_NODE_PROC_FUNCTION_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                       aNode[0],
                                       aStatement,
                                       QC_SHARED_TMPLATE(aStatement),
                                       MTC_TUPLE_TYPE_INTERMEDIATE,
                                       (sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK) )
                      != IDE_SUCCESS );
        }
        else
        {
            // BUG-38935 display name을 위해 설정한다.
            aNode[0]->node.lflag &= ~MTC_NODE_REMOVE_ARGUMENTS_MASK;
            aNode[0]->node.lflag |= MTC_NODE_REMOVE_ARGUMENTS_TRUE;
        }
    }
    else
    {
        aNode[0]->node.module = &qtc::columnModule;
        aNode[0]->node.lflag |= qtc::columnModule.lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
    }

    aNode[0]->columnName        = *aPosition;
    aNode[0]->position.stmtText = aPosition->stmtText;
    aNode[0]->position.offset   = aPosition->offset;
    aNode[0]->position.size     = aPositionEnd->offset + aPositionEnd->size - aPosition->offset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::changeColumn( qtcNode        ** aNode,
                          qcNamePosition  * aPositionStart,
                          qcNamePosition  * aPositionEnd,
                          qcNamePosition  * aUserNamePos,
                          qcNamePosition  * aTableNamePos,
                          qcNamePosition  * aColumnNamePos,
                          qcNamePosition  * aPkgNamePos)
{
/*******************************************************************
 * Description :
 *     PROJ-2533 array 변수를 고려한 changeNode
 *     column을 위한 정보로 재조정한다.
 *
 * Implementation :
 *
 *******************************************************************/
    IDE_DASSERT( aNode[0]->node.module == NULL );
 
    aNode[0]->node.module = &qtc::columnModule;
    aNode[0]->node.lflag |= qtc::columnModule.lflag & ~MTC_NODE_COLUMN_COUNT_MASK;

    aNode[0]->userName   = *aUserNamePos;
    aNode[0]->tableName  = *aTableNamePos;
    aNode[0]->columnName = *aColumnNamePos;
    aNode[0]->pkgName    = *aPkgNamePos;

    aNode[0]->position.stmtText = aPositionStart->stmtText;
    aNode[0]->position.offset   = aPositionStart->offset;
    aNode[0]->position.size     = aPositionEnd->offset + aPositionEnd->size - aPositionStart->offset;

    return IDE_SUCCESS;
}

IDE_RC qtc::addKeepArguments( qcStatement  * aStatement,
                              qtcNode     ** aNode,
                              qtcKeepAggr  * aKeep )
{
    qtcNode         * sCopyNode = NULL;
    qtcNode         * sNode     = NULL;
    UInt              sCount    = 0;
    qcuSqlSourceInfo  sSqlInfo;
    qcNamePosition    sPosition;
    qtcNode         * sNewNode[2];

    if ( ( aNode[0]->node.module != &mtfMin ) &&
         ( aNode[0]->node.module != &mtfMax ) &&
         ( aNode[0]->node.module != &mtfSum ) &&
         ( aNode[0]->node.module != &mtfAvg ) &&
         ( aNode[0]->node.module != &mtfCount ) &&
         ( aNode[0]->node.module != &mtfVariance ) &&
         ( aNode[0]->node.module != &mtfStddev ) )
    {
        sSqlInfo.setSourceInfo( aStatement, &aKeep->mKeepPosition );
        IDE_RAISE( ERR_SYNTAX );
    }
    else
    {
        /* Nothing to do */
    }

    SET_EMPTY_POSITION( sPosition );
    IDE_TEST( qtc::makeValue( aStatement,
                              sNewNode,
                              ( const UChar * )"CHAR",
                              4,
                              &sPosition,
                              ( const UChar * )&aKeep->mOption,
                              1,
                              MTC_COLUMN_NORMAL_LITERAL )
              != IDE_SUCCESS );

    aNode[0]->node.funcArguments = &sNewNode[0]->node;
    sNewNode[0]->node.arguments = NULL;
    sNewNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;

    if ( aKeep->mExpression[0]->node.module != NULL )
    {
        sCount = 2;
        sNewNode[0]->node.next = (mtcNode *)&aKeep->mExpression[0]->node;
        sNewNode[0]->node.lflag += 1;
    }
    else
    {
        sCount = ( aKeep->mExpression[0]->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) + 1;
        IDE_TEST_RAISE( ( aNode[0]->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) + sCount >
                        MTC_NODE_ARGUMENT_COUNT_MAXIMUM,
                        ERR_INVALID_FUNCTION_ARGUMENT );
        sNewNode[0]->node.next = aKeep->mExpression[0]->node.arguments;
        sNewNode[0]->node.lflag += ( sCount - 1 );
    }

    // funcArguments의 node.lflag를 보존하기 위해 복사한다.
    IDE_TEST( copyNodeTree( aStatement,
                            (qtcNode*) aNode[0]->node.funcArguments,
                            & sCopyNode,
                            ID_TRUE,
                            ID_FALSE )
              != IDE_SUCCESS );

    // arguments에 붙여넣는다.
    if ( aNode[0]->node.arguments == NULL )
    {
        aNode[0]->node.arguments = (mtcNode*) sCopyNode;
    }
    else
    {
        for ( sNode = (qtcNode*) aNode[0]->node.arguments;
              sNode->node.next != NULL;
              sNode = (qtcNode*) sNode->node.next );

        sNode->node.next = (mtcNode*) sCopyNode;
    }

    //argument count를 증가
    aNode[0]->node.lflag += sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SYNTAX )
    {
        ( void )sSqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_SYNTAX, sSqlInfo.getErrMessage() ));
        ( void )sSqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::changeKeepNode( qcStatement  * aStatement,
                            qtcNode     ** aNode )
{
    const mtfModule *   sModule     = NULL;
    ULong               sNodeArgCnt = 0;
    UShort              sCurrRowID  = 0;
    qcTemplate        * sTemplate   = NULL;

    if ( aNode[0]->node.module == &mtfMin )
    {
        sModule = &mtfMinKeep;
    }
    else if ( aNode[0]->node.module == &mtfMax )
    {
        sModule = &mtfMaxKeep;
    }
    else if ( aNode[0]->node.module == &mtfSum )
    {
        sModule = &mtfSumKeep;
    }
    else if ( aNode[0]->node.module == &mtfAvg )
    {
        sModule = &mtfAvgKeep;
    }
    else if ( aNode[0]->node.module == &mtfCount )
    {
        sModule = &mtfCountKeep;
    }
    else if ( aNode[0]->node.module == &mtfVariance )
    {
        sModule = &mtfVarianceKeep;
    }
    else if ( aNode[0]->node.module == &mtfStddev )
    {
        sModule = &mtfStddevKeep;
    }
    else
    {
        IDE_RAISE( ERR_INVALID_FUNC );
    }

    // 인자 갯수 백업
    sNodeArgCnt = aNode[0]->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    // 이전 module Flag 제거
    aNode[0]->node.lflag &= ~(aNode[0]->node.module->lflag);

    // 새 module flag 설치
    aNode[0]->node.lflag |= sModule->lflag & ~MTC_NODE_ARGUMENT_COUNT_MASK;

    // 백업된 인자 갯수 복원
    aNode[0]->node.lflag |= sNodeArgCnt;

    aNode[0]->node.module = sModule;

    sTemplate = QC_SHARED_TMPLATE( aStatement );
    sCurrRowID = sTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE];
    if ( sTemplate->tmplate.rows[sCurrRowID].columnCount + 1 >
         sTemplate->tmplate.rows[sCurrRowID].columnMaximum )
    {
        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   aNode[0],
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );
    }
    else
    {
        sTemplate->tmplate.rows[sCurrRowID].columnCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNC )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qtc::changeKeepNode",
                                  "Invalid Function" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-45745
idBool qtc::isSameType( mtcColumn * aColumn1,
                        mtcColumn * aColumn2 )
{
    idBool  sIsSameType = ID_FALSE;
    
    if ( mtc::isSameType( aColumn1, aColumn2 ) )
    {
        sIsSameType = ID_TRUE;
    }
    else
    {
        sIsSameType = ID_FALSE;
    }
    
    return sIsSameType;
}

IDE_RC qtc::estimateSerializeFilter( qcStatement * aStatement,
                                     mtcNode     * aNode,
                                     UInt        * aCount )
{
    qcTemplate           * sTemplate  = QC_SHARED_TMPLATE( aStatement );
    mtcNode              * sNode      = NULL;
    mtcNode              * sArgument  = NULL;
    mtxSerialExecuteFunc   sExecute   = mtx::calculateNA;
    UChar                  sEntryType = QTC_ENTRY_TYPE_NONE;
    UInt                   sArgCount  = 0;
    UInt                   sCount     = 0;

    IDE_TEST( checkSerializeFilterType( sTemplate,
                                        aNode,
                                        &( sArgument ),
                                        &( sExecute ),
                                        &( sEntryType ) )
              != IDE_SUCCESS );

    for ( sNode  = sArgument;
          sNode != NULL;
          sNode  = sNode->next )
    {
        IDE_TEST( estimateSerializeFilter( aStatement,
                                           sNode,
                                           aCount )
                  != IDE_SUCCESS );

        sArgCount += 1;
    }

    if ( ( sEntryType == QTC_ENTRY_TYPE_AND ) ||
         ( sEntryType == QTC_ENTRY_TYPE_OR  ) )
    {
        sCount += sArgCount;
    }
    else
    {
        IDE_TEST( sArgCount > 2 );

        sCount += 1;
    }

    for ( sNode  = aNode->conversion;
          sNode != NULL;
          sNode  = sNode->conversion )
    {
        sExecute = sTemplate->tmplate.rows[sNode->table].execute[sNode->column].mSerialExecute;

        IDE_TEST( sExecute == mtx::calculateNA );

        sCount += 1;
    }

    *aCount += sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::allocateSerializeFilter( qcStatement          * aStatement,
                                     UInt                   aCount,
                                     mtxSerialFilterInfo ** aInfo )
{
    IDU_FIT_POINT( "qtc::allocateSerializeFilter::alloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( mtxSerialFilterInfo ) * aCount,
                                               (void **) aInfo )
              != IDE_SUCCESS );

    QTC_SET_SERIAL_FILTER_INFO( (*aInfo),
                                NULL,
                                mtx::calculateNA,
                                0,
                                NULL,
                                NULL );

    QTC_SET_ENTRY_HEADER( (*aInfo)->mHeader,
                          0,
                          0,
                          0,
                          0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::recursiveSerializeFilter( qcStatement          * aStatement,
                                      mtcNode              * aNode,
                                      mtxSerialFilterInfo  * aFense,
                                      mtxSerialFilterInfo ** aInfo )
{
    qcTemplate           * sTemplate    = QC_SHARED_TMPLATE( aStatement );
    mtcNode              * sNode        = NULL;
    mtcNode              * sArgument    = NULL;
    UInt                   sIndex       = 0;
    mtxSerialFilterInfo  * sOffset[2]   = { NULL, NULL };
    UChar                  sEntryType   = QTC_ENTRY_TYPE_NONE;
    mtxSerialExecuteFunc   sExecute     = mtx::calculateNA;
    mtxSerialFilterInfo  * sPrev        = NULL;

    IDE_TEST( checkSerializeFilterType( sTemplate,
                                        aNode,
                                        &( sArgument ),
                                        &( sExecute ),
                                        &( sEntryType ) )
              != IDE_SUCCESS );

    /* Host Constant Wrapper
     *  \
     *   [CHECK] [?] [?] [?]
     *     \       \   \   \
     *      sPrev    Unknown
     */
    if ( sEntryType == QTC_ENTRY_TYPE_CHECK )
    {
        IDE_TEST( setSerializeFilter( QTC_ENTRY_TYPE_CHECK,
                                      QTC_ENTRY_COUNT_ONE,
                                      aNode,
                                      sExecute,
                                      NULL,
                                      NULL,
                                      aFense,
                                      aInfo )
                  != IDE_SUCCESS );

        sPrev = (*aInfo);
    }
    else
    {
        sPrev = NULL;
    }

    for ( sNode  = sArgument;
          sNode != NULL;
          sNode  = sNode->next )
    {
        IDE_TEST( recursiveSerializeFilter( aStatement ,
                                            sNode,
                                            aFense,
                                            aInfo )
                  != IDE_SUCCESS );

        if ( ( sEntryType == QTC_ENTRY_TYPE_AND ) ||
             ( sEntryType == QTC_ENTRY_TYPE_OR ) )
        {
            IDE_TEST( setSerializeFilter( sEntryType,
                                          QTC_ENTRY_COUNT_TWO,
                                          aNode,
                                          sExecute,
                                          (*aInfo),
                                          NULL,
                                          aFense,
                                          aInfo )
                      != IDE_SUCCESS );

            /* Linking AND / OR
             *  \
             *   [OP1] [AND'] [OP2] [AND''] [OP 3] [AND''']
             *             ^         /   ^         /
             *              \ mRight      \ mRight
             */
            if ( sPrev == NULL )
            {
                sPrev = (*aInfo);
            }
            else
            {
                (*aInfo)->mRight = sPrev;
                sPrev            = (*aInfo);
            }
        }
        else
        {
            IDE_TEST( sIndex >= 2 );

            sOffset[sIndex++] = (*aInfo);
        }
    }

    IDE_TEST( setParentSerializeFilter( sEntryType,
                                        sIndex,
                                        aNode,
                                        sExecute,
                                        sOffset[0],
                                        sOffset[1],
                                        aFense,
                                        aInfo )
                      != IDE_SUCCESS );

    IDE_TEST( adjustSerializeFilter( sEntryType,
                                     sPrev,
                                     aInfo )
                   != IDE_SUCCESS );

    if ( aNode->conversion != NULL )
    {
        IDE_TEST( recursiveConversionSerializeFilter( aStatement ,
                                                      aNode->conversion,
                                                      aFense,
                                                      aInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::recursiveConversionSerializeFilter( qcStatement          * aStatement,
                                                mtcNode              * aNode,
                                                mtxSerialFilterInfo  * aFense,
                                                mtxSerialFilterInfo ** aInfo )
{
    qcTemplate         * sTemplate  = QC_SHARED_TMPLATE( aStatement );
    UChar                sEntryType = QTC_ENTRY_TYPE_NONE;
    mtxSerialExecuteFunc sExecute   = mtx::calculateNA;

    if ( aNode->conversion != NULL )
    {
        IDE_TEST( recursiveConversionSerializeFilter( aStatement ,
                                                      aNode->conversion,
                                                      aFense,
                                                      aInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( checkConversionSerializeFilterType( sTemplate,
                                                  aNode,
                                                  (*aInfo)->mNode,
                                                  &( sExecute ),
                                                  &( sEntryType ) )
              != IDE_SUCCESS );

    IDE_TEST( setConversionSerializeFilter( sEntryType,
                                            aNode,
                                            sExecute,
                                            aFense,
                                            aInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::checkSerializeFilterType( qcTemplate           * aTemplate,
                                      mtcNode              * aNode,
                                      mtcNode             ** aArgument,
                                      mtxSerialExecuteFunc * aExecute,
                                      UChar                * aEntryType )
{
    mtcNode            * sArgument  = aNode->arguments;
    UChar                sEntryType = QTC_ENTRY_TYPE_NONE;
    mtxSerialExecuteFunc sExecute   = mtx::calculateNA;

    if ( ( aNode->module->lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_AND )
    {
        IDE_TEST( sArgument == NULL );

        if ( sArgument->next != NULL )
        {
            sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
            sEntryType = QTC_ENTRY_TYPE_AND;
        }
        else
        {
            sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
            sEntryType = QTC_ENTRY_TYPE_AND_SINGLE;
        }
    }
    else if ( ( aNode->module->lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_OR )
    {
        IDE_TEST( sArgument == NULL );

        if ( sArgument->next != NULL )
        {
            sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
            sEntryType = QTC_ENTRY_TYPE_OR;
        }
        else
        {
            sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
            sEntryType = QTC_ENTRY_TYPE_OR_SINGLE;
        }
    }
    else if ( ( aNode->module->lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_NOT )
    {
        sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
        sEntryType = QTC_ENTRY_TYPE_NOT;
    }
    else
    {
        IDE_TEST( checkNodeModuleForNormal( aTemplate,
                                            aNode,
                                            &( sArgument ),
                                            &( sExecute ),
                                            &( sEntryType ) )
                  != IDE_SUCCESS );
    }

    *aExecute   = sExecute;
    *aArgument  = sArgument;
    *aEntryType = sEntryType;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::checkNodeModuleForNormal( qcTemplate           * aTemplate,
                                      mtcNode              * aNode,
                                      mtcNode             ** aArgument,
                                      mtxSerialExecuteFunc * aExecute,
                                      UChar                * aEntryType )
{
    mtcNode            * sArgument  = aNode->arguments;
    UChar                sEntryType = QTC_ENTRY_TYPE_NONE;
    mtxSerialExecuteFunc sExecute   = mtx::calculateNA;

    if ( aNode->module == &gQtcRidModule )
    {
        sExecute   = aTemplate->tmplate.rows[aNode->table].ridExecute->mSerialExecute;
        sEntryType = QTC_ENTRY_TYPE_RID;

        IDE_TEST( sExecute == mtx::calculateNA );
    }
    else if ( aNode->module == &qtc::columnModule )
    {
        sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
        sEntryType = QTC_ENTRY_TYPE_COLUMN;

        IDE_TEST( sExecute == mtx::calculateNA );
    }
    else if ( aNode->module == &qtc::valueModule )
    {
        sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
        sEntryType = QTC_ENTRY_TYPE_VALUE;

        if ( ( aNode->orgNode != NULL ) &&
             ( sArgument      != NULL ) )
        {
            sArgument = NULL;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        IDE_TEST( checkNodeModuleForEtc( aTemplate,
                                         aNode,
                                         &( sArgument ),
                                         &( sExecute ),
                                         &( sEntryType ) )
                  != IDE_SUCCESS );
    }

    *aExecute   = sExecute;
    *aArgument  = sArgument;
    *aEntryType = sEntryType;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::checkNodeModuleForEtc( qcTemplate           * aTemplate,
                                   mtcNode              * aNode,
                                   mtcNode             ** aArgument,
                                   mtxSerialExecuteFunc * aExecute,
                                   UChar                * aEntryType )
{
    mtcNode            * sArgument  = aNode->arguments;
    UChar                sEntryType = QTC_ENTRY_TYPE_NONE;
    mtxSerialExecuteFunc sExecute   = mtx::calculateNA;

    if ( aNode->module == &qtc::hostConstantWrapperModule )
    {
        sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
        sEntryType = QTC_ENTRY_TYPE_CHECK;
    }
    else if ( aNode->module == &mtfLnnvl )
    {
        sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
        sEntryType = QTC_ENTRY_TYPE_LNNVL;
    }
    else
    {
        IDE_TEST( sArgument == NULL );

        if ( sArgument->next != NULL )
        {
            sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
            sEntryType = QTC_ENTRY_TYPE_FUNCTION;
        }
        else
        {
            sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;
            sEntryType = QTC_ENTRY_TYPE_SINGLE;
        }

        IDE_TEST( sExecute == mtx::calculateNA );
    }

    *aExecute   = sExecute;
    *aArgument  = sArgument;
    *aEntryType = sEntryType;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::checkConversionSerializeFilterType( qcTemplate           * aTemplate,
                                                mtcNode              * aNode,
                                                mtcNode              * aArgument,
                                                mtxSerialExecuteFunc * aExecute,
                                                UChar                * aEntryType )
{
    UChar                sEntryType = QTC_ENTRY_TYPE_NONE;
    mtcColumn          * sColumn    = aTemplate->tmplate.rows[aNode->table].columns + aNode->column;
    mtcColumn          * sArgument  = aTemplate->tmplate.rows[aArgument->table].columns + aArgument->column;
    mtxSerialExecuteFunc sExecute   = aTemplate->tmplate.rows[aNode->table].execute[aNode->column].mSerialExecute;

    IDE_DASSERT( ( sColumn   != NULL ) &&
                 ( sArgument != NULL ) );

    IDE_TEST( sExecute == mtx::calculateNA );

    switch( sColumn->module->id )
    {
        case MTD_NCHAR_ID:
        case MTD_NVARCHAR_ID:
            switch( sArgument->module->id )
            {
                case MTD_CHAR_ID:
                case MTD_VARCHAR_ID:
                    sEntryType = QTC_ENTRY_TYPE_CONVERT_CHAR;

                    break;

                default:
                    sEntryType = QTC_ENTRY_TYPE_CONVERT;

                    break;
            }

            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            switch( sArgument->module->id )
            {
                case MTD_NCHAR_ID:
                case MTD_NVARCHAR_ID:
                    sEntryType = QTC_ENTRY_TYPE_CONVERT_CHAR;

                    break;

                case MTD_DATE_ID:
                    sEntryType = QTC_ENTRY_TYPE_CONVERT_DATE;

                    break;

                default:
                    sEntryType = QTC_ENTRY_TYPE_CONVERT;

                    break;
            }

            break;

        case MTD_DATE_ID:
            sEntryType = QTC_ENTRY_TYPE_CONVERT_DATE;

            break;

        default:
            sEntryType = QTC_ENTRY_TYPE_CONVERT;

            break;
    }

    *aExecute   = sExecute;
    *aEntryType = sEntryType;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::adjustSerializeFilter( UChar                  aType,
                                   mtxSerialFilterInfo  * aPrev,
                                   mtxSerialFilterInfo ** aInfo )
{
    mtxSerialFilterInfo * sAnd  = (*aInfo)->mRight;
    mtxSerialFilterInfo * sPrev = NULL;
    mtxSerialFilterInfo * sLast = (*aInfo);

    switch( aType )
    {
        case QTC_ENTRY_TYPE_CHECK:
            IDE_TEST_RAISE( ( ( aPrev    == NULL ) ||
                              ( (*aInfo) == NULL ) ),
                            INVALID_ACCESS_FILTER );

            /* Host Constant Wrapper
             *  \
             *   [CHECK] [OP1] [OP2] [OP3]
             *     \                     \
             *      sPrev                 aInfo = sLast
             *
             * After Adjust
             *  \
             *   [CHECK] [OP1] [OP2] [OP3]
             *     \                  ^
             *      mLeft -----------/
             */
            aPrev->mLeft = sLast;

            break;

        case QTC_ENTRY_TYPE_AND:
        case QTC_ENTRY_TYPE_OR:
            IDE_TEST_RAISE( (*aInfo) == NULL,
                            INVALID_ACCESS_FILTER );

            /* AND / OR Optimize
             *  \
             *   [OP1] [AND'] [OP2] [AND''] [OP 3] [AND''']
             *             ^         /   ^         /      \
             *              \ mRight      \ mRight         aInfo = sLast
             *
             * After Adjust
             *  \
             *   [OP1] [AND'] [OP2] [AND''] [OP 3] [AND''']
             *          \            \              \     ^
             *           mRight ----- mRight ------- mRight = sLast
             */
            while ( sAnd != NULL )
            {
                sPrev        = sAnd->mRight;
                sAnd->mRight = sLast;
                sAnd         = sPrev;
            }

            sLast->mRight = sLast;

            break;

        case QTC_ENTRY_TYPE_RID:
        case QTC_ENTRY_TYPE_COLUMN:
        case QTC_ENTRY_TYPE_VALUE:
        case QTC_ENTRY_TYPE_FUNCTION:
        case QTC_ENTRY_TYPE_SINGLE:
        case QTC_ENTRY_TYPE_AND_SINGLE:
        case QTC_ENTRY_TYPE_OR_SINGLE:
        case QTC_ENTRY_TYPE_NOT:
        case QTC_ENTRY_TYPE_LNNVL:

            break;

        default:
            IDE_TEST( 1 );

            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ACCESS_FILTER )
    {
        ideLog::log( IDE_QP_0,
                     "Serial Filter Failure: %s: %s",
                     "qtc::adjustSerializeFilter",
                     "invalid access filter" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::setConversionSerializeFilter( UChar                  aType,
                                          mtcNode              * aNode,
                                          mtxSerialExecuteFunc   aExecute,
                                          mtxSerialFilterInfo  * aFense,
                                          mtxSerialFilterInfo ** aInfo )
{
    switch( aType )
    {
        case QTC_ENTRY_TYPE_CONVERT:
            IDE_TEST( setSerializeFilter( aType,
                                          QTC_ENTRY_COUNT_ONE,
                                          aNode,
                                          aExecute,
                                          (*aInfo),
                                          NULL,
                                          aFense,
                                          aInfo )
                  != IDE_SUCCESS );

            break;

        case QTC_ENTRY_TYPE_CONVERT_CHAR:
            IDE_TEST( setSerializeFilter( aType,
                                          QTC_ENTRY_COUNT_TWO,
                                          aNode,
                                          aExecute,
                                          (*aInfo),
                                          NULL,
                                          aFense,
                                          aInfo )
                  != IDE_SUCCESS );

            break;

        case QTC_ENTRY_TYPE_CONVERT_DATE:
            IDE_TEST( setSerializeFilter( aType,
                                          QTC_ENTRY_COUNT_THREE,
                                          aNode,
                                          aExecute,
                                          (*aInfo),
                                          NULL,
                                          aFense,
                                          aInfo )
                      != IDE_SUCCESS );

            break;

        case QTC_ENTRY_TYPE_RID:
        case QTC_ENTRY_TYPE_COLUMN:
        case QTC_ENTRY_TYPE_VALUE:
        case QTC_ENTRY_TYPE_FUNCTION:
        case QTC_ENTRY_TYPE_SINGLE:
        case QTC_ENTRY_TYPE_AND_SINGLE:
        case QTC_ENTRY_TYPE_OR_SINGLE:
        case QTC_ENTRY_TYPE_NOT:
        case QTC_ENTRY_TYPE_LNNVL:
        case QTC_ENTRY_TYPE_CHECK:
        case QTC_ENTRY_TYPE_AND:
        case QTC_ENTRY_TYPE_OR:
        default:

            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::setParentSerializeFilter( UChar                  aType,
                                      UChar                  aCount,
                                      mtcNode              * aNode,
                                      mtxSerialExecuteFunc   aExecute,
                                      mtxSerialFilterInfo  * aLeft,
                                      mtxSerialFilterInfo  * aRight,
                                      mtxSerialFilterInfo  * aFense,
                                      mtxSerialFilterInfo ** aInfo  )
{
    switch( aType )
    {
        case QTC_ENTRY_TYPE_VALUE:
            IDE_TEST( setSerializeFilter( aType,
                                          QTC_ENTRY_COUNT_ZERO,
                                          aNode,
                                          aExecute,
                                          NULL,
                                          NULL,
                                          aFense,
                                          aInfo )
                      != IDE_SUCCESS );

            break;

        case QTC_ENTRY_TYPE_RID:
            IDE_TEST( setSerializeFilter( aType,
                                          QTC_ENTRY_COUNT_ONE,
                                          aNode,
                                          aExecute,
                                          NULL,
                                          NULL,
                                          aFense,
                                          aInfo )
                      != IDE_SUCCESS );

            break;

        case QTC_ENTRY_TYPE_COLUMN:
            IDE_TEST( setSerializeFilter( aType,
                                          QTC_ENTRY_COUNT_TWO,
                                          aNode,
                                          aExecute,
                                          NULL,
                                          NULL,
                                          aFense,
                                          aInfo )
                      != IDE_SUCCESS );

            break;

        case QTC_ENTRY_TYPE_FUNCTION:
            IDE_TEST( setSerializeFilter( aType,
                                          aCount,
                                          aNode,
                                          aExecute,
                                          aLeft,
                                          aRight,
                                          aFense,
                                          aInfo )
                      != IDE_SUCCESS );

            break;

        case QTC_ENTRY_TYPE_AND_SINGLE:
        case QTC_ENTRY_TYPE_OR_SINGLE:
        case QTC_ENTRY_TYPE_NOT:
        case QTC_ENTRY_TYPE_LNNVL:
        case QTC_ENTRY_TYPE_SINGLE:
            IDE_TEST( setSerializeFilter( aType,
                                          QTC_ENTRY_COUNT_ONE,
                                          aNode,
                                          aExecute,
                                          (*aInfo),
                                          NULL,
                                          aFense,
                                          aInfo )
                      != IDE_SUCCESS );

            break;

        case QTC_ENTRY_TYPE_CHECK:
        case QTC_ENTRY_TYPE_AND:
        case QTC_ENTRY_TYPE_OR:
        default:

            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qtc::setSerializeFilter( UChar                  aType,
                                UChar                  aCount,
                                mtcNode              * aNode,
                                mtxSerialExecuteFunc   aExecute,
                                mtxSerialFilterInfo  * aLeft,
                                mtxSerialFilterInfo  * aRight,
                                mtxSerialFilterInfo  * aFense,
                                mtxSerialFilterInfo ** aInfo )
{
    mtxSerialFilterInfo * sInfo   = (*aInfo);
    mtxSerialFilterInfo * sLeft   = aLeft;
    mtxSerialFilterInfo * sRight  = aRight;
    UInt                  sNumber = 1;
    UChar                 sSize   = aCount;
    UInt                  sOffset = 0;

    switch( aType )
    {
        case QTC_ENTRY_TYPE_CHECK:
            sSize += QTC_ENTRY_SIZE_ONE;
            break;

        case QTC_ENTRY_TYPE_VALUE:
        case QTC_ENTRY_TYPE_RID:
        case QTC_ENTRY_TYPE_COLUMN:
        case QTC_ENTRY_TYPE_AND:
        case QTC_ENTRY_TYPE_OR:
        case QTC_ENTRY_TYPE_AND_SINGLE:
        case QTC_ENTRY_TYPE_OR_SINGLE:
        case QTC_ENTRY_TYPE_NOT:
        case QTC_ENTRY_TYPE_LNNVL:
            sSize += QTC_ENTRY_SIZE_TWO;
            break;

        case QTC_ENTRY_TYPE_FUNCTION:
        case QTC_ENTRY_TYPE_SINGLE:
        case QTC_ENTRY_TYPE_CONVERT:
        case QTC_ENTRY_TYPE_CONVERT_CHAR:
        case QTC_ENTRY_TYPE_CONVERT_DATE:
            sSize += QTC_ENTRY_SIZE_THREE;
            break;

        default:
            sSize += QTC_ENTRY_SIZE_ZERO;
            break;
    }

    if ( sInfo->mHeader.mId == 0 )
    {
        QTC_SET_SERIAL_FILTER_INFO( sInfo,
                                    aNode,
                                    aExecute,
                                    sOffset,
                                    sLeft,
                                    sRight );

        QTC_SET_ENTRY_HEADER( sInfo->mHeader,
                              sNumber,
                              aType,
                              sSize,
                              aCount );
    }
    else
    {
        sNumber = sInfo->mHeader.mId + 1;
        sOffset = sInfo->mOffset + sInfo->mHeader.mSize;

        IDE_TEST_RAISE( sNumber >= ID_USHORT_MAX,
                        TOO_MANY_FILTER );

        *aInfo = *aInfo + 1;

        IDE_TEST_RAISE( (*aInfo) > aFense,
                        INVALID_ACCESS_FILTER );

        sInfo = (*aInfo);

        QTC_SET_SERIAL_FILTER_INFO( sInfo,
                                    aNode,
                                    aExecute,
                                    sOffset,
                                    sLeft,
                                    sRight );

        QTC_SET_ENTRY_HEADER( sInfo->mHeader,
                              sNumber,
                              aType,
                              sSize,
                              aCount );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( TOO_MANY_FILTER )
    {
        ideLog::log( IDE_QP_0,
                     "Serial Filter Failure: %s: %s",
                     "qtc::setSerializeFilter",
                     "too many filter node" );
    }
    IDE_EXCEPTION( INVALID_ACCESS_FILTER )
    {
        ideLog::log( IDE_QP_0,
                     "Serial Filter Failure: %s: %s",
                     "qtc::setSerializeFilter",
                     "invalid access filter" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
