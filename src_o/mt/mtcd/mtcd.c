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
 

/***********************************************************************
 * $Id: mtd.cpp 36964 2009-11-26 01:41:30Z linkedlist $
 **********************************************************************/

#include <mtce.h>
#include <mtcd.h>
#include <mtcl.h>
#include <mtcc.h>

#include <mtcdTypes.h>
#include <mtclCollate.h>

extern mtdModule mtcdBigint;
extern mtdModule mtcdBinary;
extern mtdModule mtcdBit;
extern mtdModule mtcdVarbit;
extern mtdModule mtcdBlob;
extern mtdModule mtcdBoolean;
extern mtdModule mtcdChar;
extern mtdModule mtcdDate;
extern mtdModule mtcdDouble;
extern mtdModule mtcdFloat;
extern mtdModule mtcdInteger;
extern mtdModule mtcdInterval;
extern mtdModule mtcdList;
extern mtdModule mtcdNull;
extern mtdModule mtcdNumber;
extern mtdModule mtcdNumeric;
extern mtdModule mtcdReal;
extern mtdModule mtcdSmallint;
extern mtdModule mtcdVarchar;
extern mtdModule mtcdByte;
extern mtdModule mtcdNibble;
extern mtdModule mtcdClob;
//extern mtdModule mtdBlobLocator;  // PROJ-1362
//extern mtdModule mtdClobLocator;  // PROJ-1362
extern mtdModule mtcdNchar;        // PROJ-1579 NCHAR
extern mtdModule mtcdNvarchar;     // PROJ-1579 NCHAR
extern mtdModule mtcdEchar;        // PROJ-2002 Column Security
extern mtdModule mtcdEvarchar;     // PROJ-2002 Column Security
extern mtdModule mtcdVarbyte;      // BUG-40973

const mtlModule* mtlDBCharSet;
const mtlModule* mtlNationalCharSet;

mtdModule * mtdAllModule[MTD_MAX_DATATYPE_CNT];
const mtdModule* mtdInternalModule[] = {
    &mtcdBigint,
    &mtcdBit,
    &mtcdVarbit,
    &mtcdBlob,
    &mtcdBoolean,
    &mtcdChar,
    &mtcdDate,
    &mtcdDouble,
    &mtcdFloat,
    &mtcdInteger,
    &mtcdInterval,
    &mtcdList,
    &mtcdNull,
    &mtcdNumber,
    &mtcdNumeric,
    &mtcdReal,
    &mtcdSmallint,
    &mtcdVarchar,
    &mtcdByte,
    &mtcdNibble,
    &mtcdClob,
//    &mtdBlobLocator,  // PROJ-1362
//    &mtdClobLocator,  // PROJ-1362
    &mtcdBinary,       // PROJ-1583, PR-15722
    &mtcdNchar,        // PROJ-1579 NCHAR
    &mtcdNvarchar,     // PROJ-1579 NCHAR
    &mtcdEchar,        // PROJ-2002 Column Security
    &mtcdEvarchar,     // PROJ-2002 Column Security
    &mtcdVarbyte,
    NULL
};

const mtdCompareFunc mtdCompareNumberGroupBigintFuncs[3][2] = {
    {
        mtdCompareNumberGroupBigintMtdMtdAsc,
        mtdCompareNumberGroupBigintMtdMtdDesc,
    }
    ,
    {
        mtdCompareNumberGroupBigintStoredMtdAsc,
        mtdCompareNumberGroupBigintStoredMtdDesc,
    }
    ,
    {
        mtdCompareNumberGroupBigintStoredStoredAsc,
        mtdCompareNumberGroupBigintStoredStoredDesc,
    }
};

const mtdCompareFunc mtdCompareNumberGroupDoubleFuncs[3][2] = {
    {
        mtdCompareNumberGroupDoubleMtdMtdAsc,
        mtdCompareNumberGroupDoubleMtdMtdDesc,
    }
    ,
    {
        mtdCompareNumberGroupDoubleStoredMtdAsc,
        mtdCompareNumberGroupDoubleStoredMtdDesc,
    }
    ,
    {
        mtdCompareNumberGroupDoubleStoredStoredAsc,
        mtdCompareNumberGroupDoubleStoredStoredDesc,
    }
};

const mtdCompareFunc mtdCompareNumberGroupNumericFuncs[3][2] = {
    {
        mtdCompareNumberGroupNumericMtdMtdAsc,
        mtdCompareNumberGroupNumericMtdMtdDesc,
    }
    ,
    {
        mtdCompareNumberGroupNumericStoredMtdAsc,
        mtdCompareNumberGroupNumericStoredMtdDesc,
    }
    ,
    {
        mtdCompareNumberGroupNumericStoredStoredAsc,
        mtdCompareNumberGroupNumericStoredStoredDesc,
    }
};

typedef struct mtdIdIndex {
    const mtdModule* module;
} mtdIdIndex;

typedef struct mtdNameIndex {
    const mtcName*   name;
    const mtdModule* module;
} mtdNameIndex;

static acp_uint32_t  mtdNumberOfModules;

static mtdIdIndex*   mtdModulesById = NULL;

static acp_uint32_t  mtdNumberOfModulesByName;

static mtdNameIndex* mtdModulesByName = NULL;

// PROJ-1364
// 숫자형 계열간 비교시 아래 conversion matrix에 의하여
// 비교기준 data type이 정해진다.
// ------------------------------------------
//        |  N/A   | 정수형 | 실수형 | 지수형
// ------------------------------------------
//   N/A  |  N/A   |  N/A   |  N/A   |  N/A
// ------------------------------------------
// 정수형 |  N/A   | 정수형 | 실수형 | 지수형
// ------------------------------------------
// 실수형 |  N/A   | 실수형 | 실수형 | 실수형
// ------------------------------------------
// 지수형 |  N/A   | 지수형 | 실수형 | 지수형
// ------------------------------------------
const acp_uint32_t mtdComparisonNumberType[4][4] = {
    { MTD_NUMBER_GROUP_TYPE_NONE,
      MTD_NUMBER_GROUP_TYPE_NONE,
      MTD_NUMBER_GROUP_TYPE_NONE,
      MTD_NUMBER_GROUP_TYPE_NONE },
    { MTD_NUMBER_GROUP_TYPE_NONE,
      MTD_NUMBER_GROUP_TYPE_BIGINT,     // 정수형
      MTD_NUMBER_GROUP_TYPE_DOUBLE,     // 실수형
      MTD_NUMBER_GROUP_TYPE_NUMERIC },  // 지수형
    { MTD_NUMBER_GROUP_TYPE_NONE,
      MTD_NUMBER_GROUP_TYPE_DOUBLE,     // 실수형
      MTD_NUMBER_GROUP_TYPE_DOUBLE,     // 실수형
      MTD_NUMBER_GROUP_TYPE_DOUBLE },   // 실수형
    { MTD_NUMBER_GROUP_TYPE_NONE,
      MTD_NUMBER_GROUP_TYPE_NUMERIC,    // 지수형
      MTD_NUMBER_GROUP_TYPE_DOUBLE,     // 실수형
      MTD_NUMBER_GROUP_TYPE_NUMERIC }   // 지수형
};


// To Remove Warning
/*
  static int mtdCompareById( const mtdIdIndex* aModule1,
  const mtdIdIndex* aModule2 )
  {
  if( aModule1->module->id.type > aModule2->module->id.type )
  {
  return 1;
  }
  if( aModule1->module->id.type < aModule2->module->id.type )
  {
  return -1;
  }
  if( aModule1->module->id.language > aModule2->module->id.language )
  {
  return 1;
  }
  if( aModule1->module->id.language < aModule2->module->id.language )
  {
  return -1;
  }

  return 0;
  }
*/


static int mtdCompareByName( const mtdNameIndex* aModule1,
                             const mtdNameIndex* aModule2 )
{
    int sOrder;

    sOrder = mtcStrMatch( aModule1->name->string,
                          aModule1->name->length,
                          aModule2->name->string,
                          aModule2->name->length );
    return sOrder;
}

acp_uint32_t mtdGetNumberOfModules( void )
{
    return mtdNumberOfModules;
}

ACI_RC mtdIsTrueNA( acp_bool_t*      aTemp1,
                    const mtcColumn* aTemp2,
                    const void*      aTemp3,
                    acp_uint32_t     aTemp4)
{
    ACP_UNUSED(aTemp1);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
    ACP_UNUSED(aTemp4);
    
    aciSetErrorCode(mtERR_FATAL_NOT_APPLICABLE);

    return ACI_FAILURE;
}

ACI_RC mtdCanonizeDefault( const mtcColumn* aCanon ,
                           void**           aCanonized,
                           mtcEncryptInfo * aCanonInfo,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo,
                           mtcTemplate*     aTemplate)
{
    ACP_UNUSED(aCanon);
    ACP_UNUSED(aCanonInfo);
    ACP_UNUSED(aColumn);
    ACP_UNUSED(aColumnInfo);
    ACP_UNUSED(aTemplate);  
  
    *aCanonized = aValue;

    return ACI_SUCCESS;
}

ACI_RC mtdInitializeModule( mtdModule*   aModule,
                            acp_uint32_t aNo )
{
    /***********************************************************************
     *
     * Description : mtd module의 초기화
     *
     * Implementation :
     *
     ***********************************************************************/
    aModule->no = aNo;

    return ACI_SUCCESS;

    //ACI_EXCEPTION_END;

    //return ACI_FAILURE;
}

ACI_RC mtdInitialize( mtdModule*** aExtTypeModuleGroup,
                      acp_uint32_t aGroupCnt )
{
    /***********************************************************************
     *
     * Description : mtd initialize
     *
     * Implementation : data type 정보 구축
     *
     *   mtdModulesById
     *    --------------------------
     *   | mtdIdIndex     | module -|---> mtdBigInt
     *    --------------------------
     *   | mtdIdIndex     | module -|---> mtdBlob
     *    --------------------------
     *   |      ...                 | ...
     *    --------------------------
     *
     *   mtdModuleByName
     *    ------------------------
     *   | mtdNameIndex | name   -|---> BIGINT
     *   |              | module -|---> mtdBigInt
     *    ------------------------
     *   | mtdNameIndex | name   -|---> BLOB
     *   |              | module -|---> mtdBlob
     *    ------------------------
     *   |      ...               | ...
     *    ------------------------
     *
     ***********************************************************************/

    acp_uint32_t   i;
    acp_uint32_t   sStage = 0;
    acp_sint32_t   sModuleIndex = 0;

    mtdModule**    sModule;
    const mtcName* sName;

    //---------------------------------------------------------
    // 실제 mtdModule의 개수, mtd module name에 따른 mtdModule 개수 구함
    // mtdModule은 한개 이상의 이름을 가질 수 있음
    //     ex ) mtdNumeric :  NUMERIC과 DECIMAL 두 개의 name을 가짐
    //          mtdVarchar : VARCHAR와 VARCHAR2 두 개의 name을 가짐
    //---------------------------------------------------------

    mtdNumberOfModules       = 0;
    mtdNumberOfModulesByName = 0;

    //---------------------------------------------------------------
    // 내부 Data Type의 초기화
    //---------------------------------------------------------------

    for( sModule = (mtdModule**) mtdInternalModule; *sModule != NULL; sModule++ )
    {
        mtdAllModule[mtdNumberOfModules] = *sModule;
    
        (*sModule)->initialize( mtdNumberOfModules );
        mtdNumberOfModules++;

        ACE_ASSERT( mtdNumberOfModules < MTD_MAX_DATATYPE_CNT );

        for( sName = (*sModule)->names; sName != NULL; sName = sName->next )
        {
            mtdNumberOfModulesByName++;
        }
    }

//---------------------------------------------------------------
// 외부 Data Type의 초기화
//---------------------------------------------------------------
 
    for ( i = 0; i < aGroupCnt; i++ )
    {
        for( sModule   = aExtTypeModuleGroup[i];
             *sModule != NULL;
             sModule++ )
        {
            mtdAllModule[mtdNumberOfModules] = *sModule;
            (*sModule)->initialize( mtdNumberOfModules );
            mtdNumberOfModules++;
            ACE_ASSERT( mtdNumberOfModules < MTD_MAX_DATATYPE_CNT );

            for( sName = (*sModule)->names;
                 sName != NULL;
                 sName = sName->next )
            {
                mtdNumberOfModulesByName++;
            }
        }
    }

    mtdAllModule[mtdNumberOfModules] = NULL;

//---------------------------------------------------------
// mtdModulesById 와 mtdModulesByName을 구성
//---------------------------------------------------------

    ACI_TEST(acpMemCalloc((void**)&mtdModulesById,
                          128,
                          sizeof(mtdIdIndex))
             != ACP_RC_SUCCESS);

    sStage = 1;

    ACI_TEST(acpMemAlloc((void**)&mtdModulesByName,
                         sizeof(mtdNameIndex) *
                         mtdNumberOfModulesByName)
             != ACP_RC_SUCCESS);
    sStage = 2;
    
    mtdNumberOfModulesByName = 0;
    for( sModule = (mtdModule**) mtdAllModule; *sModule != NULL; sModule++ )
    {
        // mtdModuleById 구성
        sModuleIndex = ( (*sModule)->id & 0x003F );
        mtdModulesById[sModuleIndex].module = *sModule;

        // mtdModuleByName 구성
        for( sName = (*sModule)->names; sName != NULL; sName = sName->next )
        {
            mtdModulesByName[mtdNumberOfModulesByName].name   = sName;
            mtdModulesByName[mtdNumberOfModulesByName].module = *sModule;
            mtdNumberOfModulesByName++;
        }
    }
    acpSortQuickSort( mtdModulesByName, mtdNumberOfModulesByName,
                      sizeof(mtdNameIndex), (acp_sint32_t(*)(const void *, const void *)) mtdCompareByName );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    switch( sStage )
    {
        case 2:
            acpMemFree(mtdModulesByName);
            mtdModulesByName = NULL;
        case 1:
            acpMemFree(mtdModulesById);
            mtdModulesById = NULL;
        default:
            break;
    }

    return ACI_FAILURE;
}

ACI_RC mtdFinalize( void )
{
    /***********************************************************************
     *
     * Description : mtd finalize
     *
     * Implementation : data type 정보 저장된 메모리 공간 해제
     *
     ***********************************************************************/

    if( mtdModulesByName != NULL )
    {
        acpMemFree(mtdModulesByName);

        mtdModulesByName = NULL;
    }
    if( mtdModulesById != NULL )
    {
        acpMemFree(mtdModulesById);

        mtdModulesById = NULL;
    }

    return ACI_SUCCESS;

/*    ACI_EXCEPTION_END;

return ACI_FAILURE;*/
}

ACI_RC mtdModuleByName( const mtdModule** aModule,
                        const void*       aName,
                        acp_uint32_t      aLength )
{
/***********************************************************************
 *
 * Description : data type name으로 mtd module 검색
 *
 * Implementation :
 *    mtdModuleByName에서 해당 data type name과 동일한 mtd module을 찾아줌
 *
 ***********************************************************************/

    mtdNameIndex sKey;
    mtdModule    sModule;
    mtcName      sName;
    acp_sint32_t sMinimum;
    acp_sint32_t sMaximum;
    acp_sint32_t sMedium;
    acp_sint32_t sFound;

    sName.length        = aLength;
    sName.string        = (void*)aName;
    sKey.name           = &sName;
    sKey.module         = &sModule;

    sMinimum = 0;
    sMaximum = mtdNumberOfModulesByName - 1;

    do{
        sMedium = (sMinimum + sMaximum) >> 1;
        if( mtdCompareByName( mtdModulesByName + sMedium, &sKey ) >= 0 )
        {
            sMaximum = sMedium - 1;
            sFound   = sMedium;
        }
        else
        {
            sMinimum = sMedium + 1;
            sFound   = sMinimum;
        }
    }while( sMinimum <= sMaximum );

    ACI_TEST_RAISE( sFound >= (acp_sint32_t)mtdNumberOfModulesByName, ERR_NOT_FOUND );

    *aModule = mtdModulesByName[sFound].module;

    ACI_TEST_RAISE( mtcStrMatch( mtdModulesByName[sFound].name->string,
                                 mtdModulesByName[sFound].name->length,
                                 aName,
                                 aLength ) != 0,
                    ERR_NOT_FOUND );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_NOT_FOUND )
    {
        char sBuffer[1024];
        char sNameBuffer[256];

        if( aLength >= sizeof(sNameBuffer) )
        {
            aLength = sizeof(sNameBuffer) - 1;
        }
        acpMemCpy( sNameBuffer, aName, aLength );
        sNameBuffer[aLength] = '\0';
        acpSnprintf( sBuffer, sizeof(sBuffer),
                     "(Name=\"%s\")",
                     sNameBuffer );
        
        aciSetErrorCode(mtERR_ABORT_DATATYPE_MODULE_NOT_FOUND,
                        sBuffer);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdModuleById( const mtdModule** aModule,
                      acp_uint32_t      aId )
{
/***********************************************************************
 *
 * Description : data type으로 mtd module 검색
 *
 * Implementation :
 *    mtdModuleById에서 해당 data type과 동일한 mtd module을 찾아줌
 *
 ***********************************************************************/
    acp_sint32_t       sFound;

    sFound = ( aId & 0x003F );

    *aModule = mtdModulesById[sFound].module;

    ACI_TEST_RAISE(*aModule == NULL, ERR_NOT_FOUND);
    ACI_TEST_RAISE( (*aModule)->id != aId, ERR_NOT_FOUND );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_NOT_FOUND )
    {
        char sBuffer[1024];
        acpSnprintf( sBuffer, sizeof(sBuffer),
                     "(type=%"ACI_UINT32_FMT")", aId );
        aciSetErrorCode(mtERR_ABORT_DATATYPE_MODULE_NOT_FOUND,
                        sBuffer);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdModuleByNo( const mtdModule** aModule,
                      acp_uint32_t      aNo )
{
/***********************************************************************
 *
 * Description : data type no로 mtd module 검색
 *
 * Implementation :
 *    mtd module number로 해당 mtd module을 찾아줌
 *
 ***********************************************************************/

    ACI_TEST_RAISE( aNo >= mtdNumberOfModules, ERR_NOT_FOUND );

    *aModule = mtdAllModule[aNo];

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_NOT_FOUND )
    {
        char sBuffer[1024];
        acpSnprintf( sBuffer, sizeof(sBuffer),
                     "(no=%"ACI_UINT32_FMT")", aNo );
        aciSetErrorCode(mtERR_ABORT_DATATYPE_MODULE_NOT_FOUND,
                        sBuffer);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_bool_t mtdIsNullDefault( const mtcColumn* aTemp,
                             const void*      aTemp2,
                             acp_uint32_t     aTemp3)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
    
    return ACP_FALSE;
}

ACI_RC mtdEncodeCharDefault( mtcColumn*    aColumn, 
                             void*         aValue,
                             acp_uint32_t  aValueSize ,
                             acp_uint8_t*  aCompileFmt ,
                             acp_uint32_t  aCompileFmtLen ,
                             acp_uint8_t*  aText,
                             acp_uint32_t* aTextLen,
                             ACI_RC*       aRet )
{
    acp_uint32_t sStringLen;
    mtdCharType* sCharVal;

    ACP_UNUSED(aColumn);
    ACP_UNUSED(aValueSize);
    ACP_UNUSED(aCompileFmt);
    ACP_UNUSED(aCompileFmtLen);
    
    //----------------------------------
    // Parameter Validation
    //----------------------------------

    ACE_ASSERT( aValue != NULL );
    ACE_ASSERT( aText != NULL );
    ACE_ASSERT( *aTextLen > 0 );
    ACE_ASSERT( aRet != NULL );

    //----------------------------------
    // Initialization
    //----------------------------------

    aText[0] = '\0';
    sStringLen = 0;
    sCharVal = (mtdCharType*)aValue;

    //----------------------------------
    // Set String
    //----------------------------------

    // To Fix BUG-16801
    if ( mtcdChar.isNull( NULL, aValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sStringLen = ( *aTextLen > sCharVal->length )
            ? sCharVal->length : (*aTextLen - 1);
        acpMemCpy(aText, sCharVal->value, sStringLen );
    }

    //----------------------------------
    // Finalization
    //----------------------------------

    aText[sStringLen] = '\0';
    *aTextLen = sStringLen;

    ACI_TEST( sStringLen < sCharVal->length );

    *aRet = ACI_SUCCESS;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    *aRet = ACI_FAILURE;

    return ACI_SUCCESS;
}

ACI_RC mtdEncodeNumericDefault( mtcColumn*    aColumn ,
                                void*         aValue,
                                acp_uint32_t  aValueSize ,
                                acp_uint8_t*  aCompileFmt ,
                                acp_uint32_t  aCompileFmtLen ,
                                acp_uint8_t*  aText,
                                acp_uint32_t* aTextLen,
                                ACI_RC*       aRet )
{
    acp_uint32_t sStringLen;

    ACP_UNUSED(aColumn);
    ACP_UNUSED(aValueSize);
    ACP_UNUSED(aCompileFmt);
    ACP_UNUSED(aCompileFmtLen);
    
    //----------------------------------
    // Parameter Validation
    //----------------------------------

    ACE_ASSERT( aValue != NULL );
    ACE_ASSERT( aText != NULL );
    ACE_ASSERT( *aTextLen > 0 );
    ACE_ASSERT( aRet != NULL );

    //----------------------------------
    // Initialization
    //----------------------------------

    aText[0] = '\0';
    sStringLen = 0;

    //----------------------------------
    // Set String
    //----------------------------------

    // To Fix BUG-16801
    if ( mtcdNumeric.isNull( NULL, aValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        // To Fix BUG-16802 : copy from mtvCalculate_Float2Varchar()
        ACI_TEST( mtdFloat2String( aText,
                                   47,
                                   & sStringLen,
                                   (mtdNumericType*) aValue )
                  != ACI_SUCCESS );
    }

    //----------------------------------
    // Finalization
    //----------------------------------

    aText[sStringLen] = '\0';
    *aTextLen = sStringLen;

    *aRet = ACI_SUCCESS;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    *aRet = ACI_FAILURE;

    return ACI_FAILURE;
}

ACI_RC mtdDecodeDefault( mtcTemplate*  aTemplate,
                         mtcColumn*    aColumn,
                         void*         aValue,
                         acp_uint32_t* aValueSize,
                         acp_uint8_t*  aCompileFmt ,
                         acp_uint32_t  aCompileFmtLen ,
                         acp_uint8_t*  aText,
                         acp_uint32_t  aTextLen,
                         ACI_RC*        aRet )
{
    acp_uint32_t sValueOffset = 0;

    ACP_UNUSED(aCompileFmt);
    ACP_UNUSED(aCompileFmtLen);
        
    return aColumn->module->value(aTemplate,
                                  aColumn,
                                  aValue,
                                  &sValueOffset,
                                  *aValueSize,
                                  aText,
                                  aTextLen,
                                  aRet) ;
}

ACI_RC mtdEncodeNA( mtcColumn*    aColumn ,
                    void*         aValue ,
                    acp_uint32_t  aValueSize ,
                    acp_uint8_t*  aCompileFmt ,
                    acp_uint32_t  aCompileFmtLen ,
                    acp_uint8_t*  aText,
                    acp_uint32_t* aTextLen,
                    ACI_RC*       aRet )
{
    // To fix BUG-14235
    // return is ACI_SUCCESS.
    // but encoding result is ACI_FAILURE.
    ACP_UNUSED(aColumn);
    ACP_UNUSED(aValue);
    ACP_UNUSED(aValueSize);
    ACP_UNUSED(aCompileFmt);
    ACP_UNUSED(aCompileFmtLen);

    aText[0] = '\0';
    *aTextLen = 0;

    *aRet = ACI_FAILURE;

    return ACI_SUCCESS;
}

ACI_RC mtdCompileFmtDefault( mtcColumn*    aColumn ,
                             acp_uint8_t*  aCompiledFmt ,
                             acp_uint32_t* aCompiledFmtLen ,
                             acp_uint8_t*  aFormatString ,
                             acp_uint32_t  aFormatStringLength ,
                             ACI_RC*       aRet )
{
    ACP_UNUSED(aColumn);
    ACP_UNUSED(aCompiledFmt);
    ACP_UNUSED(aCompiledFmtLen);    
    ACP_UNUSED(aFormatString);
    ACP_UNUSED(aFormatStringLength);

    *aRet = ACI_SUCCESS;
    return ACI_SUCCESS;
}

ACI_RC mtdValueFromOracleDefault( mtcColumn*    aColumn,
                                  void*         aValue,
                                  acp_uint32_t* aValueOffset,
                                  acp_uint32_t  aValueSize,
                                  const void*   aOracleValue,
                                  acp_sint32_t  aOracleLength,
                                  ACI_RC*       aResult )
{
    ACP_UNUSED(aColumn);
    ACP_UNUSED(aValue);
    ACP_UNUSED(aValueOffset);    
    ACP_UNUSED(aValueSize);
    ACP_UNUSED(aOracleValue);
    ACP_UNUSED(aOracleLength);
    ACP_UNUSED(aResult);

    aciSetErrorCode(mtERR_ABORT_NOT_APPLICABLE);

    return ACI_FAILURE;
}

ACI_RC
mtdMakeColumnInfoDefault( void*        aStmt ,
                          void*        aTableInfo ,
                          acp_uint32_t aIdx )
{
    ACP_UNUSED(aStmt);
    ACP_UNUSED(aTableInfo);
    ACP_UNUSED(aIdx);
    
    return ACI_SUCCESS;
}

acp_double_t
mtdSelectivityNA( void* aColumnMax ,
                  void* aColumnMin ,
                  void* aValueMax  ,
                  void* aValueMin   )
{
    // Selectivity를 추출할 수 없는 경우로,
    // 이 함수가 호출되어서는 안됨.
    /*   ACI_CALLBACK_FATAL ( "mtdSelectivityNA() : "
         "This Data Type does not Support Selectivity" );BUGBUG*/
    ACP_UNUSED(aColumnMax);
    ACP_UNUSED(aColumnMin);
    ACP_UNUSED(aValueMax);
    ACP_UNUSED(aValueMin);
    
    return 1;
}

acp_double_t
mtdSelectivityDefault( void* aColumnMax ,
                       void* aColumnMin ,
                       void* aValueMax  ,
                       void* aValueMin  )
{
/*******************************************************************
 * Definition
 *      PROJ-1579 NCHAR
 *
 * Description
 *      멀티바이트 캐릭터 셋에 대한 selectivity 측정 시,
 *      CHAR 타입에서 사용하고 있는 mtdSelectivityChar() 함수를 사용할 경우
 *      완전히 잘못된 값이 나올 수 있다.
 *      따라서, 모든 캐릭터 셋에 대한 selectivity 를 구하는 함수를 따로
 *      구현하기 전까지는 멀티바이트 캐릭터 셋에 대해서는
 *      default selectivity 를 사용하도록 한다.
 *
 *******************************************************************/

    ACP_UNUSED(aColumnMax);
    ACP_UNUSED(aColumnMin);
    ACP_UNUSED(aValueMax);
    ACP_UNUSED(aValueMin);
    
    return MTD_DEFAULT_SELECTIVITY;
}

/*******************************************************************
 * Definition
 *    mtdModule에서 사용할 value 함수
 *
 * Description
 *    mtc::value는 상위 레이어(QP, MM)에서 사용할 수 있는 범용 value 함수
 *    mtd::valueForModule은 mtd에서만 사용한다.
 *    네번째 인자인 aDefaultNull에 mtdModule.staticNull을 assign한다.
 *
 * by kumdory, 2005-03-15
 *
 *******************************************************************/
const void* mtdValueForModule( const void*  aColumn,
                               const void*  aRow,
                               acp_uint32_t aFlag,
                               const void*  aDefaultNull )
{
    const void* sValue;

    ACP_UNUSED(aColumn);
    ACP_UNUSED(aDefaultNull);
    
    /*BUG-24052*/
    sValue = NULL;

    if( ( aFlag & MTD_OFFSET_MASK ) == MTD_OFFSET_USELESS )
    {
        sValue = aRow;
    }
    else
    {
        ACE_ASSERT(0);
    }

    return sValue;
}

acp_uint32_t
mtdGetDefaultIndexTypeID( const mtdModule* aModule )
{
    return (acp_uint32_t) aModule->idxType[0];
}

acp_bool_t
mtdIsUsableIndexType( const mtdModule* aModule,
                      acp_uint32_t     aIndexType )
{
    acp_uint32_t i;
    acp_bool_t sResult;

    sResult = ACP_FALSE;

    for ( i = 0; i < MTD_MAX_USABLE_INDEX_CNT; i++ )
    {
        if ( (acp_uint32_t) aModule->idxType[i] == aIndexType )
        {
            sResult = ACP_TRUE;
            break;
        }
        else
        {
            // Nothing To Do
        }
    }

    return sResult;
}

ACI_RC
mtdAssignNullValueById( acp_uint32_t  aId,
                        void**        aValue,
                        acp_uint32_t* aSize )
{
/***********************************************************************
 *
 * Description : data type으로 mtd module의 staticNull 지정
 *
 * Implementation : ( PROJ-1558 )
 *
 ***********************************************************************/
    mtdModule  * sModule;

    ACI_TEST( mtdModuleById( (const mtdModule **) & sModule,
                             aId )
              != ACI_SUCCESS );

    *aValue = sModule->staticNull;
    *aSize = sModule->actualSize( NULL,
                                  *aValue,
                                  MTD_OFFSET_USELESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC
mtdCheckNullValueById( acp_uint32_t aId,
                       void*        aValue,
                       acp_bool_t*  aIsNull )
{
/***********************************************************************
 *
 * Description : data type으로 mtd module의 isNull 검사
 *
 * Implementation : ( PROJ-1558 )
 *
 ***********************************************************************/
    mtdModule  * sModule;

    ACI_TEST( mtdModuleById( (const mtdModule **) & sModule,
                             aId )
              != ACI_SUCCESS );

    if ( sModule->isNull( NULL,
                          aValue,
                          MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        *aIsNull = ACP_TRUE;
    }
    else
    {
        *aIsNull = ACP_FALSE;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdStoredValue2MtdValueNA( acp_uint32_t aColumnSize ,
                                  void*        aDestValue ,
                                  acp_uint32_t aDestValueOffset ,
                                  acp_uint32_t aLength ,
                                  const void*  aValue  )
{
/***********************************************************************
 * PROJ-1705
 * 저장되지 않는 타입에 대한 처리   
 ***********************************************************************/
    
    ACP_UNUSED(aColumnSize);
    ACP_UNUSED(aDestValue);
    ACP_UNUSED(aDestValueOffset);
    ACP_UNUSED(aLength);
    ACP_UNUSED(aValue);
    
    ACE_ASSERT( 0 );

    return ACI_SUCCESS;
}


acp_uint32_t mtdNullValueSizeNA()
{
    aciSetErrorCode(mtERR_ABORT_NOT_APPLICABLE);
    ACE_ASSERT( 0 );
    
    return 0;    
}

acp_uint32_t mtdHeaderSizeNA()
{
    aciSetErrorCode(mtERR_ABORT_NOT_APPLICABLE);
    ACE_ASSERT( 0 );
    
    return 0;    
}

acp_uint32_t mtdHeaderSizeDefault()
{
    return 0;    
}

ACI_RC mtdModifyNls4MtdModule()
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1579 NCHAR
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_char_t       * sDBCharSetStr       = NULL;
    acp_char_t       * sNationalCharSetStr = NULL;

    //---------------------------------------------------------
    // DB 캐릭터 셋과 내셔널 캐릭터 셋 세팅
    //---------------------------------------------------------

    // CallBack 함수에 달려있는 smiGetDBCharSet()을 이용해서 가져온다.
    sDBCharSetStr = getDBCharSet();
    sNationalCharSetStr = getNationalCharSet();

    ACI_TEST( mtlModuleByName( (const mtlModule **) & mtlDBCharSet,
                               sDBCharSetStr,
                               acpCStrLen(sDBCharSetStr, ACP_UINT32_MAX) )
              != ACI_SUCCESS );

    ACI_TEST( mtlModuleByName( (const mtlModule **) & mtlNationalCharSet,
                               sNationalCharSetStr,
                               acpCStrLen(sNationalCharSetStr, ACP_UINT32_MAX) )
              != ACI_SUCCESS );

    ACE_ASSERT( mtlDBCharSet != NULL );
    ACE_ASSERT( mtlNationalCharSet != NULL );

    //----------------------
    // language 정보 재설정
    //----------------------

    // CHAR
    mtcdChar.column->type.languageId = mtlDBCharSet->id;
    mtcdChar.column->language = mtlDBCharSet;

    // VARCHAR
    mtcdVarchar.column->type.languageId = mtlDBCharSet->id;
    mtcdVarchar.column->language = mtlDBCharSet;

    // CLOB
    mtcdClob.column->type.languageId = mtlDBCharSet->id;
    mtcdClob.column->language = mtlDBCharSet;

    // CLOBLOCATOR
/*    mtdClobLocator.column->type.languageId = mtlDBCharSet->id;
      mtdClobLocator.column->language = mtlDBCharSet;*/

    // NCHAR
    mtcdNchar.column->type.languageId = mtlNationalCharSet->id;
    mtcdNchar.column->language = mtlNationalCharSet;

    // NVARCHAR
    mtcdNvarchar.column->type.languageId = mtlNationalCharSet->id;
    mtcdNvarchar.column->language = mtlNationalCharSet;

    // PROJ-2002 Column Security
    // ECHAR
    mtcdEchar.column->type.languageId = mtlNationalCharSet->id;
    mtcdEchar.column->language = mtlNationalCharSet;

    // EVARCHAR
    mtcdEvarchar.column->type.languageId = mtlNationalCharSet->id;
    mtcdEvarchar.column->language = mtlNationalCharSet;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdGetPrecisionNA( const mtcColumn* aColumn ,
                          const void*      aRow ,
                          acp_uint32_t     aFlag ,
                          acp_sint32_t*    aPrecision ,
                          acp_sint32_t*    aScale  )
{
    ACP_UNUSED(aColumn);
    ACP_UNUSED(aRow);
    ACP_UNUSED(aFlag);
    ACP_UNUSED(aPrecision);
    ACP_UNUSED(aScale);

    aciSetErrorCode(mtERR_ABORT_NOT_APPLICABLE);
    return ACI_FAILURE;
}


//---------------------------------------------------------
// compareNumberGroupBigint
//---------------------------------------------------------

void
mtdConvertToBigintType4MtdValue( mtdValueInfo*  aValueInfo,
                                 mtdBigintType* aBigintValue )
{
/***********************************************************************
 *
 * Description : 해당 data type의 value를 bigint type으로 conversion
 *
 * Implementation : ( PROJ-1364 )
 *
 *  이 함수로 들어올 수 있는 data type은 다음과 같다.
 *
 *  정수형 : BIGINT, INTEGER, SMALLINT
 *
 ***********************************************************************/

    ACP_UNUSED(aValueInfo);
    ACP_UNUSED(aBigintValue);

    ACE_ASSERT(0);
}

void
mtdConvertToBigintType4StoredValue( mtdValueInfo*  aValueInfo,
                                    mtdBigintType* aBigintValue )
{
/***********************************************************************
 *
 * Description : 해당 data type의 value를 bigint type으로 conversion
 * 
 * Implementation : ( PROJ-1364 )
 *
 *  이 함수로 들어올 수 있는 data type은 다음과 같다.
 *
 *  정수형 : BIGINT, INTEGER, SMALLINT
 * 
 *  mtdValueInfo의 value는 Page에 저장된 value를 가리킴 
 *  따라서 align 되지 않았을 수도 있기 때문에 byte assign 하여야 함 
 ***********************************************************************/
    
    const mtcColumn* sColumn;
    mtdSmallintType  sSmallintValue;
    mtdIntegerType   sIntegerValue;
    
    sColumn    = aValueInfo->column;

    if ( sColumn->type.dataTypeId == MTD_SMALLINT_ID )
    {
        MTC_SHORT_BYTE_ASSIGN( &sSmallintValue, aValueInfo->value );

        if ( sSmallintValue == *((mtdSmallintType*)mtcdSmallint.staticNull) )
        {
            *aBigintValue = *((mtdBigintType*)mtcdBigint.staticNull);
        }
        else
        {
            *aBigintValue = (mtdBigintType)sSmallintValue;
        }
    }
    else if ( aValueInfo->column->type.dataTypeId == MTD_INTEGER_ID )
    {
        MTC_INT_BYTE_ASSIGN( &sIntegerValue, aValueInfo->value );
        
        if ( sIntegerValue == *((mtdIntegerType*)mtcdInteger.staticNull) )
        {
            *aBigintValue = *((mtdBigintType*)mtcdBigint.staticNull);
        }
        else
        {
            *aBigintValue = (mtdBigintType)sIntegerValue;
        }
    }
    else if ( aValueInfo->column->type.dataTypeId == MTD_BIGINT_ID )
    {
        MTC_LONG_BYTE_ASSIGN( aBigintValue, aValueInfo->value );
    }
    else
    {
        ACE_ASSERT(0);
    }
}

acp_sint32_t
mtdCompareNumberGroupBigintMtdMtdAsc( mtdValueInfo* aValueInfo1,
                                      mtdValueInfo* aValueInfo2 )
{
    mtdBigintType sBigintValue1;
    mtdBigintType sBigintValue2;
    
    mtdConvertToBigintType4MtdValue( aValueInfo1, &sBigintValue1 );
    mtdConvertToBigintType4MtdValue( aValueInfo2, &sBigintValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sBigintValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sBigintValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdBigint.keyCompare[MTD_COMPARE_MTDVAL_MTDVAL]
        [MTD_COMPARE_ASCENDING]( aValueInfo1,
                                 aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupBigintMtdMtdDesc( mtdValueInfo* aValueInfo1,
                                       mtdValueInfo* aValueInfo2 )
{
    mtdBigintType sBigintValue1;
    mtdBigintType sBigintValue2;

    mtdConvertToBigintType4MtdValue( aValueInfo1, &sBigintValue1 );
    mtdConvertToBigintType4MtdValue( aValueInfo2, &sBigintValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sBigintValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sBigintValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdBigint.keyCompare[MTD_COMPARE_MTDVAL_MTDVAL]
        [MTD_COMPARE_DESCENDING]( aValueInfo1,
                                  aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupBigintStoredMtdAsc( mtdValueInfo* aValueInfo1,
                                         mtdValueInfo* aValueInfo2 )
{
    mtdBigintType sBigintValue1;
    mtdBigintType sBigintValue2;

    mtdConvertToBigintType4StoredValue( aValueInfo1, &sBigintValue1 );
    mtdConvertToBigintType4MtdValue( aValueInfo2, &sBigintValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sBigintValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sBigintValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;

    return mtcdBigint.keyCompare[MTD_COMPARE_STOREDVAL_MTDVAL]
        [MTD_COMPARE_ASCENDING]( aValueInfo1,
                                 aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupBigintStoredMtdDesc( mtdValueInfo* aValueInfo1,
                                          mtdValueInfo* aValueInfo2 )
{
    mtdBigintType sBigintValue1;
    mtdBigintType sBigintValue2;

    mtdConvertToBigintType4StoredValue( aValueInfo1, &sBigintValue1 );
    mtdConvertToBigintType4MtdValue( aValueInfo2, &sBigintValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sBigintValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sBigintValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdBigint.keyCompare[MTD_COMPARE_STOREDVAL_MTDVAL]
        [MTD_COMPARE_DESCENDING]( aValueInfo1,
                                  aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupBigintStoredStoredAsc( mtdValueInfo* aValueInfo1,
                                            mtdValueInfo* aValueInfo2 )
{
    mtdBigintType sBigintValue1;
    mtdBigintType sBigintValue2;
    
    mtdConvertToBigintType4StoredValue( aValueInfo1, &sBigintValue1 );
    mtdConvertToBigintType4StoredValue( aValueInfo2, &sBigintValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sBigintValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sBigintValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdBigint.keyCompare[MTD_COMPARE_STOREDVAL_STOREDVAL]
        [MTD_COMPARE_ASCENDING]( aValueInfo1,
                                 aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupBigintStoredStoredDesc( mtdValueInfo* aValueInfo1,
                                             mtdValueInfo* aValueInfo2 )
{
    mtdBigintType sBigintValue1;
    mtdBigintType sBigintValue2;
    
    mtdConvertToBigintType4StoredValue( aValueInfo1, &sBigintValue1 );
    mtdConvertToBigintType4StoredValue( aValueInfo2, &sBigintValue2 ); 

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sBigintValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sBigintValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdBigint.keyCompare[MTD_COMPARE_STOREDVAL_STOREDVAL]
        [MTD_COMPARE_DESCENDING]( aValueInfo1,
                                  aValueInfo2 );
}

//---------------------------------------------------------
// compareNumberGroupDouble
//---------------------------------------------------------

void
mtdConvertToDoubleType4MtdValue( mtdValueInfo*  aValueInfo,
                                 mtdDoubleType* aDoubleValue )
{
/***********************************************************************
 *
 * Description : 해당 data type의 value를 double type으로 conversion
 *
 * Implementation : ( PROJ-1364 )
 *
 *   이 함수로 들어올 수 있는 data type은 다음과 같다.
 *
 *   정수형 : BIGINT, INTEGER, SMALLINT
 *   실수형 : DOUBLE, REAL
 *   지수형 : 고정소수점형 : NUMERIC, DECIMAL, NUMBER(P), NUMER(P,S)
 *            부정소수점형 : FLOAT, NUMBER
 *
 ***********************************************************************/

    ACP_UNUSED(aValueInfo);
    ACP_UNUSED(aDoubleValue);

    ACE_ASSERT(0);
}

void
mtdConvertToDoubleType4StoredValue( mtdValueInfo*  aValueInfo,
                                    mtdDoubleType* aDoubleValue )
{
/***********************************************************************
 *
 * Description : 해당 data type의 value를 double type으로 conversion
 *
 * Implementation : ( PROJ-1364 )
 *
 *   이 함수로 들어올 수 있는 data type은 다음과 같다.
 *
 *   정수형 : BIGINT, INTEGER, SMALLINT
 *   실수형 : DOUBLE, REAL
 *   지수형 : 고정소수점형 : NUMERIC, DECIMAL, NUMBER(P), NUMER(P,S)
 *            부정소수점형 : FLOAT, NUMBER
 *
 ***********************************************************************/

    ACP_UNUSED(aValueInfo);
    ACP_UNUSED(aDoubleValue);

    ACE_ASSERT(0);
}

acp_sint32_t
mtdCompareNumberGroupDoubleMtdMtdAsc( mtdValueInfo* aValueInfo1,
                                      mtdValueInfo* aValueInfo2 )
{
    mtdDoubleType sDoubleValue1;
    mtdDoubleType sDoubleValue2;

    mtdConvertToDoubleType4MtdValue( aValueInfo1, &sDoubleValue1 );
    mtdConvertToDoubleType4MtdValue( aValueInfo2, &sDoubleValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sDoubleValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sDoubleValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdDouble.keyCompare[MTD_COMPARE_MTDVAL_MTDVAL]
        [MTD_COMPARE_ASCENDING]( aValueInfo1,
                                 aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupDoubleMtdMtdDesc( mtdValueInfo* aValueInfo1,
                                       mtdValueInfo* aValueInfo2 )
{
    mtdDoubleType sDoubleValue1;
    mtdDoubleType sDoubleValue2;
    
    mtdConvertToDoubleType4MtdValue( aValueInfo1, &sDoubleValue1 );
    mtdConvertToDoubleType4MtdValue( aValueInfo2, &sDoubleValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sDoubleValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sDoubleValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdDouble.keyCompare[MTD_COMPARE_MTDVAL_MTDVAL]
        [MTD_COMPARE_DESCENDING]( aValueInfo1,
                                  aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupDoubleStoredMtdAsc( mtdValueInfo* aValueInfo1,
                                         mtdValueInfo* aValueInfo2 )
{
    mtdDoubleType sDoubleValue1;
    mtdDoubleType sDoubleValue2;

    mtdConvertToDoubleType4StoredValue( aValueInfo1, &sDoubleValue1 );
    mtdConvertToDoubleType4MtdValue( aValueInfo2, &sDoubleValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sDoubleValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sDoubleValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdDouble.keyCompare[MTD_COMPARE_STOREDVAL_MTDVAL]
        [MTD_COMPARE_ASCENDING]( aValueInfo1,
                                 aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupDoubleStoredMtdDesc( mtdValueInfo* aValueInfo1,
                                          mtdValueInfo* aValueInfo2 )
{
    mtdDoubleType sDoubleValue1;
    mtdDoubleType sDoubleValue2;

    mtdConvertToDoubleType4StoredValue( aValueInfo1, &sDoubleValue1 );
    mtdConvertToDoubleType4MtdValue( aValueInfo2, &sDoubleValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sDoubleValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sDoubleValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;

    return mtcdDouble.keyCompare[MTD_COMPARE_STOREDVAL_MTDVAL]
        [MTD_COMPARE_DESCENDING]( aValueInfo1,
                                  aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupDoubleStoredStoredAsc( mtdValueInfo* aValueInfo1,
                                            mtdValueInfo* aValueInfo2 )
{
    mtdDoubleType sDoubleValue1;
    mtdDoubleType sDoubleValue2;

    mtdConvertToDoubleType4StoredValue( aValueInfo1, &sDoubleValue1 );
    mtdConvertToDoubleType4StoredValue( aValueInfo2, &sDoubleValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sDoubleValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sDoubleValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;

    return mtcdDouble.keyCompare[MTD_COMPARE_STOREDVAL_STOREDVAL]
        [MTD_COMPARE_ASCENDING]( aValueInfo1,
                                 aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupDoubleStoredStoredDesc( mtdValueInfo* aValueInfo1,
                                             mtdValueInfo* aValueInfo2 )
{
    mtdDoubleType sDoubleValue1;
    mtdDoubleType sDoubleValue2;

    mtdConvertToDoubleType4StoredValue( aValueInfo1, &sDoubleValue1 );
    mtdConvertToDoubleType4StoredValue( aValueInfo2, &sDoubleValue2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = &sDoubleValue1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = &sDoubleValue2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;

    return mtcdDouble.keyCompare[MTD_COMPARE_STOREDVAL_STOREDVAL]
        [MTD_COMPARE_DESCENDING]( aValueInfo1,
                                  aValueInfo2 );
}

//---------------------------------------------------------
// compareNumberGroupNumeric
//---------------------------------------------------------

void 
mtdConvertToNumericType4MtdValue( mtdValueInfo*    aValueInfo,
                                  mtdNumericType** aNumericValue )
{
/***********************************************************************
 *
 * Description : 해당 data type의 value를 numeric type으로 conversion
 *
 * Implementation : ( PROJ-1364 )
 *
 *   이 함수로 들어올 수 있는 data type은 다음과 같다.
 *
 *   정수형 : BIGINT, INTEGER, SMALLINT
 *   지수형 : 고정소수점형 : NUMERIC, DECIMAL, NUMBER(P), NUMER(P,S)
 *            부정소수점형 : FLOAT, NUMBER
 *
 *   정수형계열은 (정수형==>BIGINT==>NUMERIC)으로 변환수행
 *   지수형계열은 모두 mtdNumericType을 사용하므로
 *   해당 pointer만 얻어온다.
 *
 ***********************************************************************/

    ACP_UNUSED(aValueInfo);
    ACP_UNUSED(aNumericValue);
    
    ACE_ASSERT(0);
}

void
mtdConvertToNumericType4StoredValue( mtdValueInfo*    aValueInfo,
                                     mtdNumericType** aNumericValue,
                                     acp_uint8_t*     aLength,
                                     acp_uint8_t**    aSignExponentMantissa)
{
/***********************************************************************
 *
 * Description : 해당 data type의 value를 numeric type으로 conversion
 *
 * Implementation : ( PROJ-1364 )
 *
 *   이 함수로 들어올 수 있는 data type은 다음과 같다.
 *
 *   정수형 : BIGINT, INTEGER, SMALLINT
 *   지수형 : 고정소수점형 : NUMERIC, DECIMAL, NUMBER(P), NUMER(P,S)
 *            부정소수점형 : FLOAT, NUMBER
 *
 *   정수형계열은 (정수형==>BIGINT==>NUMERIC)으로 변환수행
 *   지수형계열은 모두 mtdNumericType을 사용하므로
 *   해당 pointer만 얻어온다.
 *
 ***********************************************************************/

    ACP_UNUSED(aValueInfo);
    ACP_UNUSED(aNumericValue);
    ACP_UNUSED(aLength);
    ACP_UNUSED(aSignExponentMantissa);
    
    ACE_ASSERT(0);
}

acp_sint32_t
mtdCompareNumberGroupNumericMtdMtdAsc( mtdValueInfo* aValueInfo1,
                                       mtdValueInfo* aValueInfo2 )
{
    // 정수형계열에서 NUMERIC 으로 변환시, 컬럼크기 결정
    // 정수형의 numeric type으로의 변환은
    // 정수형==>BIGINT==>NUMERIC 의 변환 과정을 거치며,
    // BIGINT==>NUMERIC으로의 변환과정시,
    // NUMERIC type을 만들 공간
    acp_char_t      sNumericValue1[MTD_NUMERIC_SIZE_MAXIMUM];
    acp_char_t      sNumericValue2[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType* sNumericValuePtr1;
    mtdNumericType* sNumericValuePtr2;

    // 지수형계열에서 NUMERIC type으로의 변환시는
    // 동일한 data type 구조인  mtdNumericType을 사용하기때문에
    // 해당 pointer만 얻어오게 되며(memcpy를 하지 않아도 됨),
    // 정수형계열에서 NUMERIC 으로 변환시는
    // numeric type을 만들 공간이 필요.
    sNumericValuePtr1 = (mtdNumericType*)sNumericValue1;
    sNumericValuePtr2 = (mtdNumericType*)sNumericValue2;
    
    mtdConvertToNumericType4MtdValue( aValueInfo1, &sNumericValuePtr1 );
    mtdConvertToNumericType4MtdValue( aValueInfo2, &sNumericValuePtr2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = sNumericValuePtr1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = sNumericValuePtr2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdNumeric.keyCompare[MTD_COMPARE_MTDVAL_MTDVAL]
        [MTD_COMPARE_ASCENDING]( aValueInfo1,
                                 aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupNumericMtdMtdDesc( mtdValueInfo* aValueInfo1,
                                        mtdValueInfo* aValueInfo2 )
{
    // 정수형계열에서 NUMERIC 으로 변환시, 컬럼크기 결정
    // 정수형의 numeric type으로의 변환은
    // 정수형==>BIGINT==>NUMERIC 의 변환 과정을 거치며,
    // BIGINT==>NUMERIC으로의 변환과정시,
    // NUMERIC type을 만들 공간
    acp_char_t      sNumericValue1[MTD_NUMERIC_SIZE_MAXIMUM];
    acp_char_t      sNumericValue2[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType* sNumericValuePtr1;
    mtdNumericType* sNumericValuePtr2;

    // 지수형계열에서 NUMERIC type으로의 변환시는
    // 동일한 data type 구조인  mtdNumericType을 사용하기때문에
    // 해당 pointer만 얻어오게 되며(memcpy를 하지 않아도 됨),
    // 정수형계열에서 NUMERIC 으로 변환시는
    // numeric type을 만들 공간이 필요.
    sNumericValuePtr1 = (mtdNumericType*)sNumericValue1;
    sNumericValuePtr2 = (mtdNumericType*)sNumericValue2;
    
    mtdConvertToNumericType4MtdValue( aValueInfo1,
                                      &sNumericValuePtr1 );
    mtdConvertToNumericType4MtdValue( aValueInfo2,
                                      &sNumericValuePtr2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = sNumericValuePtr1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = sNumericValuePtr2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdNumeric.keyCompare[MTD_COMPARE_MTDVAL_MTDVAL]
        [MTD_COMPARE_DESCENDING]( aValueInfo1,
                                  aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupNumericStoredMtdAsc( mtdValueInfo* aValueInfo1,
                                          mtdValueInfo* aValueInfo2 )
{
    // 정수형계열에서 NUMERIC 으로 변환시, 컬럼크기 결정
    // 정수형의 numeric type으로의 변환은
    // 정수형==>BIGINT==>NUMERIC 의 변환 과정을 거치며,
    // BIGINT==>NUMERIC으로의 변환과정시,
    // NUMERIC type을 만들 공간
    acp_char_t      sNumericValue1[MTD_NUMERIC_SIZE_MAXIMUM];
    acp_char_t      sNumericValue2[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType* sNumericValuePtr1;
    mtdNumericType* sNumericValuePtr2;
    acp_uint8_t     sLength1 = 0;
    acp_uint8_t*    sSignExponentMantissa1 = 0;

    // 지수형계열에서 NUMERIC type으로의 변환시는
    // 동일한 data type 구조인  mtdNumericType을 사용하기때문에
    // 해당 pointer만 얻어오게 되며(memcpy를 하지 않아도 됨),
    // 정수형계열에서 NUMERIC 으로 변환시는
    // numeric type을 만들 공간이 필요.
    sNumericValuePtr1 = (mtdNumericType*)sNumericValue1;
    sNumericValuePtr2 = (mtdNumericType*)sNumericValue2;

    mtdConvertToNumericType4StoredValue( aValueInfo1,
                                         &sNumericValuePtr1,
                                         &sLength1,
                                         &sSignExponentMantissa1 );
    
    mtdConvertToNumericType4MtdValue( aValueInfo2,
                                      &sNumericValuePtr2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = sSignExponentMantissa1;
    aValueInfo1->length = sLength1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;
    
    aValueInfo2->column = NULL;
    aValueInfo2->value  = sNumericValuePtr2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdNumeric.keyCompare[MTD_COMPARE_STOREDVAL_MTDVAL]
        [MTD_COMPARE_ASCENDING]( aValueInfo1,
                                 aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupNumericStoredMtdDesc( mtdValueInfo* aValueInfo1,
                                           mtdValueInfo* aValueInfo2 )
{
    // 정수형계열에서 NUMERIC 으로 변환시, 컬럼크기 결정
    // 정수형의 numeric type으로의 변환은
    // 정수형==>BIGINT==>NUMERIC 의 변환 과정을 거치며,
    // BIGINT==>NUMERIC으로의 변환과정시,
    // NUMERIC type을 만들 공간
    acp_char_t      sNumericValue1[MTD_NUMERIC_SIZE_MAXIMUM];
    acp_char_t      sNumericValue2[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType* sNumericValuePtr1 = NULL;
    mtdNumericType* sNumericValuePtr2 = NULL;
    acp_uint8_t     sLength1 = 0;
    acp_uint8_t*    sSignExponentMantissa1 = 0;

    // 지수형계열에서 NUMERIC type으로의 변환시는
    // 동일한 data type 구조인  mtdNumericType을 사용하기때문에
    // 해당 pointer만 얻어오게 되며(memcpy를 하지 않아도 됨),
    // 정수형계열에서 NUMERIC 으로 변환시는
    // numeric type을 만들 공간이 필요.
    sNumericValuePtr1 = (mtdNumericType*)sNumericValue1;
    sNumericValuePtr2 = (mtdNumericType*)sNumericValue2;

    mtdConvertToNumericType4StoredValue( aValueInfo1,
                                         &sNumericValuePtr1,
                                         &sLength1,
                                         &sSignExponentMantissa1 );
    
    mtdConvertToNumericType4MtdValue( aValueInfo2,
                                      &sNumericValuePtr2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = sSignExponentMantissa1;
    aValueInfo1->length = sLength1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;
    
    aValueInfo2->column = NULL;
    aValueInfo2->value  = sNumericValuePtr2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdNumeric.keyCompare[MTD_COMPARE_STOREDVAL_MTDVAL]
        [MTD_COMPARE_DESCENDING]( aValueInfo1,
                                  aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupNumericStoredStoredAsc( mtdValueInfo* aValueInfo1,
                                             mtdValueInfo* aValueInfo2 )
{
    // 정수형계열에서 NUMERIC 으로 변환시, 컬럼크기 결정
    // 정수형의 numeric type으로의 변환은
    // 정수형==>BIGINT==>NUMERIC 의 변환 과정을 거치며,
    // BIGINT==>NUMERIC으로의 변환과정시,
    // NUMERIC type을 만들 공간
    acp_char_t      sNumericValue1[MTD_NUMERIC_SIZE_MAXIMUM];
    acp_char_t      sNumericValue2[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType* sNumericValuePtr1;
    mtdNumericType* sNumericValuePtr2;
    acp_uint8_t     sLength1= 0;
    acp_uint8_t*    sSignExponentMantissa1 = 0;
    acp_uint8_t     sLength2 = 0;
    acp_uint8_t*    sSignExponentMantissa2 = 0;    

    // 지수형계열에서 NUMERIC type으로의 변환시는
    // 동일한 data type 구조인  mtdNumericType을 사용하기때문에
    // 해당 pointer만 얻어오게 되며(memcpy를 하지 않아도 됨),
    // 정수형계열에서 NUMERIC 으로 변환시는
    // numeric type을 만들 공간이 필요.
    sNumericValuePtr1 = (mtdNumericType*)sNumericValue1;
    sNumericValuePtr2 = (mtdNumericType*)sNumericValue2;
    
    mtdConvertToNumericType4StoredValue( aValueInfo1,
                                         &sNumericValuePtr1,
                                         &sLength1,
                                         &sSignExponentMantissa1 );
    
    mtdConvertToNumericType4StoredValue( aValueInfo2,
                                         &sNumericValuePtr2,
                                         &sLength2,
                                         &sSignExponentMantissa2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = sSignExponentMantissa1;
    aValueInfo1->length = sLength1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = sSignExponentMantissa2;
    aValueInfo1->length = sLength2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdNumeric.keyCompare[MTD_COMPARE_STOREDVAL_STOREDVAL]
        [MTD_COMPARE_ASCENDING]( aValueInfo1,
                                 aValueInfo2 );
}

acp_sint32_t
mtdCompareNumberGroupNumericStoredStoredDesc( mtdValueInfo* aValueInfo1,
                                              mtdValueInfo* aValueInfo2 )
{
    // 정수형계열에서 NUMERIC 으로 변환시, 컬럼크기 결정
    // 정수형의 numeric type으로의 변환은
    // 정수형==>BIGINT==>NUMERIC 의 변환 과정을 거치며,
    // BIGINT==>NUMERIC으로의 변환과정시,
    // NUMERIC type을 만들 공간
    acp_char_t      sNumericValue1[MTD_NUMERIC_SIZE_MAXIMUM];
    acp_char_t      sNumericValue2[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType* sNumericValuePtr1;
    mtdNumericType* sNumericValuePtr2;
    acp_uint8_t     sLength1 = 0;
    acp_uint8_t*    sSignExponentMantissa1;
    acp_uint8_t     sLength2 = 0;
    acp_uint8_t*    sSignExponentMantissa2 = 0;

    // 지수형계열에서 NUMERIC type으로의 변환시는
    // 동일한 data type 구조인  mtdNumericType을 사용하기때문에
    // 해당 pointer만 얻어오게 되며(memcpy를 하지 않아도 됨),
    // 정수형계열에서 NUMERIC 으로 변환시는
    // numeric type을 만들 공간이 필요.
    sNumericValuePtr1 = (mtdNumericType*)sNumericValue1;
    sNumericValuePtr2 = (mtdNumericType*)sNumericValue2;
    
    mtdConvertToNumericType4StoredValue( aValueInfo1,
                                         &sNumericValuePtr1,
                                         &sLength1,
                                         &sSignExponentMantissa1 );
    
    mtdConvertToNumericType4StoredValue( aValueInfo2,
                                         &sNumericValuePtr2,
                                         &sLength2,
                                         &sSignExponentMantissa2 );

    aValueInfo1->column = NULL;
    aValueInfo1->value  = sSignExponentMantissa2;
    aValueInfo1->length = sLength1;
    aValueInfo1->flag   = MTD_OFFSET_USELESS;

    aValueInfo2->column = NULL;
    aValueInfo2->value  = sSignExponentMantissa2;
    aValueInfo2->length = sLength2;
    aValueInfo2->flag   = MTD_OFFSET_USELESS;
    
    return mtcdNumeric.keyCompare[MTD_COMPARE_STOREDVAL_STOREDVAL]
        [MTD_COMPARE_DESCENDING]( aValueInfo1,
                                  aValueInfo2 );
}

mtdCompareFunc 
mtdFindCompareFunc( mtcColumn*   aColumn,
                    mtcColumn*   aValue,
                    acp_uint32_t aCompValueType,
                    acp_uint32_t aDirection )
{
/***********************************************************************
 *
 * Description : 동일계열에 속하는 서로 다른 data type간의 비교함수
 *               찾아주는 함수 
 *
 * Implementation : ( PROJ-1364 )
 *
 *   각 계열간 비교함수를 호출해서, 비교연산을 수행한다.
 *
 *   1. 문자형계열
 *      char와 varchar간의 비교
 *
 *   2. 숫자형계열
 *      (1) 비교기준 data type이 정수형
 *      (2) 비교기준 data type이 실수형
 *      (3) 비교기준 data type이 지수형
 *
 *      숫자형 계열간 비교시 아래 conversion matrix에 의하여
 *      비교기준 data type이 정해진다.
 *      ------------------------------------------
 *             |  N/A   | 정수형 | 실수형 | 지수형
 *      ------------------------------------------
 *        N/A  |  N/A   |  N/A   |  N/A   |  N/A
 *      ------------------------------------------
 *      정수형 |  N/A   | 정수형 | 실수형 | 지수형
 *      ------------------------------------------
 *      실수형 |  N/A   | 실수형 | 실수형 | 실수형
 *      ------------------------------------------
 *      지수형 |  N/A   | 지수형 | 실수형 | 지수형
 *      ------------------------------------------
 *
 * << data type의 분류 >>
 * -----------------------------------------------------------------------
 *            |                                                | 대표type
 * ----------------------------------------------------------------------
 * 문자형계열 | CHAR, VARCHAR                                  | VARCHAR
 * ----------------------------------------------------------------------
 * 숫자형계열 | Native | 정수형 | BIGINT, INTEGER, SMALLINT    | BIGINT
 *            |        |-------------------------------------------------
 *            |        | 실수형 | DOUBLE, REAL                 | DOUBLE
 *            -----------------------------------------------------------
 *            | Non-   | 고정소수점형 | NUMERIC, DECIMAL,      |
 *            | Native |              | NUMBER(p), NUMBER(p,s) | NUMERIC
 *            |(지수형)|----------------------------------------
 *            |        | 부정소수점형 | FLOAT, NUMBER          |
 * ----------------------------------------------------------------------
 *
 ***********************************************************************/
    
    acp_uint32_t   sType;
    acp_uint32_t   sColumn1GroupFlag = 0;
    acp_uint32_t   sColumn2GroupFlag = 0;
    mtdCompareFunc sCompare = NULL;

    if( ( aColumn->module->flag & MTD_GROUP_MASK )
        == MTD_GROUP_TEXT )
    {
        //-------------------------------
        // 문자형 계열
        //-------------------------------

        sCompare = mtcdVarchar.keyCompare[aCompValueType][aDirection];
    }
    else
    {
        //------------------------------
        // 숫자형 계열
        //------------------------------

        sColumn1GroupFlag =
            ( aColumn->module->flag &
              MTD_NUMBER_GROUP_TYPE_MASK ) >> MTD_NUMBER_GROUP_TYPE_SHIFT;
        sColumn2GroupFlag =
            ( aValue->module->flag &
              MTD_NUMBER_GROUP_TYPE_MASK ) >> MTD_NUMBER_GROUP_TYPE_SHIFT;

        sType =
            mtdComparisonNumberType[sColumn1GroupFlag][sColumn2GroupFlag];

        switch( sType )
        {
            case ( MTD_NUMBER_GROUP_TYPE_BIGINT ) :
                // 비교기준 data type이 정수형

                sCompare = mtdCompareNumberGroupBigintFuncs[aCompValueType]
                [aDirection];
                break;
            case ( MTD_NUMBER_GROUP_TYPE_DOUBLE ) :
                // 비교기준 data type이 실수형

                sCompare = mtdCompareNumberGroupDoubleFuncs[aCompValueType]
                [aDirection];

                break;
            case ( MTD_NUMBER_GROUP_TYPE_NUMERIC ) :
                // 비교기준 data type이 지수형

                sCompare = mtdCompareNumberGroupNumericFuncs[aCompValueType]
                [aDirection];

                break;
            default :
                ACE_ASSERT(0);
        }
    }

    return sCompare;
}

ACI_RC mtdFloat2String( acp_uint8_t*    aBuffer,
                        acp_uint32_t    aBufferLength,
                        acp_uint32_t*   aLength,
                        mtdNumericType* aNumeric )
{
    acp_uint8_t  sString[50];
    acp_sint32_t sLength;
    acp_sint32_t sExponent;
    acp_sint32_t sIterator;
    acp_sint32_t sFence;

    *aLength = 0;

    if( aNumeric->length != 0 )
    {
        if( aNumeric->length == 1 )
        {
            ACI_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
            aBuffer[0] = '0';
            (*aLength)++;
        }
        else
        {
            if( aNumeric->signExponent & 0x80 )
            {
                sExponent = ( (acp_sint32_t)( aNumeric->signExponent & 0x7F ) - 64 )
                    <<  1;
                sLength = 0;
                if( aNumeric->mantissa[0] < 10 )
                {
                    sExponent--;
                    sString[sLength] = '0' + aNumeric->mantissa[0];
                    sLength++;
                }
                else
                {
                    sString[sLength] = '0' + aNumeric->mantissa[0] / 10;
                    sLength++;
                    sString[sLength] = '0' + aNumeric->mantissa[0] % 10;
                    sLength++;
                }
                for( sIterator = 1, sFence = aNumeric->length - 1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    sString[sLength] = '0' + aNumeric->mantissa[sIterator] / 10;
                    sLength++;
                    sString[sLength] = '0' + aNumeric->mantissa[sIterator] % 10;
                    sLength++;
                }
                if( sString[sLength - 1] == '0' )
                {
                    sLength--;
                }
            }
            else
            {
                ACI_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
                aBuffer[0] = '-';
                aBuffer++;
                (*aLength)++;
                aBufferLength--;
                sExponent = ( 64 - (acp_sint32_t)( aNumeric->signExponent & 0x7F ) )
                    << 1;
                sLength = 0;
                if( aNumeric->mantissa[0] >= 90 )
                {
                    sExponent--;
                    sString[sLength] = '0' + 99 - (acp_sint32_t)aNumeric->mantissa[0];
                    sLength++;
                }
                else
                {
                    sString[sLength] = '0' + ( 99 - (acp_sint32_t)aNumeric->mantissa[0] ) / 10;
                    sLength++;
                    sString[sLength] = '0' + ( 99 - (acp_sint32_t)aNumeric->mantissa[0] ) % 10;
                    sLength++;
                }
                for( sIterator = 1, sFence = aNumeric->length - 1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    sString[sLength] = '0' + ( 99 - (acp_sint32_t)aNumeric->mantissa[sIterator] ) / 10;
                    sLength++;
                    sString[sLength] = '0' + ( 99 - (acp_sint32_t)aNumeric->mantissa[sIterator] ) % 10;
                    sLength++;
                }
                if( sString[sLength - 1] == '0' )
                {
                    sLength--;
                }
            }
            if( sExponent > 0 )
            {
                if( sLength <= sExponent )
                {
                    if( sExponent <= (acp_sint32_t)aBufferLength )
                    {
                        for( sIterator = 0; sIterator < sLength; sIterator++ )
                        {
                            aBuffer[sIterator] = sString[sIterator];
                        }
                        for( ; sIterator < sExponent; sIterator++ )
                        {
                            aBuffer[sIterator] = '0';
                        }
                        *aLength += sExponent;
                        goto success;
                    }
                }
                else
                {
                    if( sLength + 1 <= (acp_sint32_t)aBufferLength )
                    {
                        for( sIterator = 0; sIterator < sExponent; sIterator++ )
                        {
                            aBuffer[sIterator] = sString[sIterator];
                        }
                        aBuffer[sIterator] = '.';
                        aBuffer++;
                        for( ; sIterator < sLength; sIterator++ )
                        {
                            aBuffer[sIterator] = sString[sIterator];
                        }
                        *aLength += sLength + 1;
                        goto success;
                    }
                }
            }
            else
            {
                //fix BUG-18163
                if( sLength - sExponent + 2 <= (acp_sint32_t)aBufferLength )
                {
                    sExponent = -sExponent;

                    aBuffer[0] = '0';
                    aBuffer[1] = '.';
                    aBuffer += 2;

                    for( sIterator = 0; sIterator < sExponent; sIterator++ )
                    {
                        aBuffer[sIterator] = '0';
                    }
                    aBuffer += sIterator;
                    for( sIterator = 0; sIterator < sLength; sIterator++ )
                    {
                        aBuffer[sIterator] = sString[sIterator];
                    }
                    *aLength += sLength + sExponent + 2;
                    goto success;
                }
            }
            sExponent--;
            ACI_TEST_RAISE( (acp_sint32_t)aBufferLength < sLength + 1,
                            ERR_INVALID_LENGTH );
            aBuffer[0] = sString[0];
            aBuffer[1] = '.';
            aBuffer++;
            aBufferLength--;
            for( sIterator = 1; sIterator < sLength; sIterator++ )
            {
                aBuffer[sIterator] = sString[sIterator];
            }
            aBuffer       += sLength;
            aBufferLength -= sLength;
            *aLength      += sLength + 1;
            ACI_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
            aBuffer[0] = 'E';
            aBuffer++;
            aBufferLength--;
            (*aLength)++;
            ACI_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
            if( sExponent >= 0 )
            {
                aBuffer[0] = '+';
            }
            else
            {
                sExponent = -sExponent;
                aBuffer[0] = '-';
            }
            aBuffer++;
            aBufferLength--;
            (*aLength)++;
            if( sExponent >= 100 )
            {
                ACI_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
                aBuffer[0] = '0' + sExponent / 100;
                aBuffer++;
                aBufferLength--;
                (*aLength)++;
            }
            if( sExponent >= 10 )
            {
                ACI_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
                aBuffer[0] = '0' + sExponent / 10 % 10;
                aBuffer++;
                aBufferLength--;
                (*aLength)++;
            }
            ACI_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
            aBuffer[0] = '0' + sExponent % 10;
            aBuffer++;
            aBufferLength--;
            (*aLength)++;
        }
    }

  success:

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
