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
 * $Id: mtv.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtv.h>

mtvTable** mtv::table = NULL;

UInt mtv::mMaxConvCount = 1;  // BUG-21627

UInt mtv::setModules( UInt              aTo,
                      UInt              aFrom,
                      UInt**            aTarget,
                      const mtvModule** aModule )
{
    UInt sCount = 0;

    if( aTarget[aTo][aFrom] != ID_UINT_MAX )
    {
        if( aTarget[aTo][aFrom] == aTo )
        {
            if( aModule != NULL )
            {
                *aModule = table[aTo][aFrom].module;
            }
            sCount++;
        }
        else
        {
            sCount += setModules( aTarget[aTo][aFrom],
                                  aFrom,
                                  aTarget,
                                  aModule );
            sCount += setModules( aTo,
                                  aTarget[aTo][aFrom],
                                  aTarget,
                                  aModule != NULL ? aModule + sCount : NULL );
        }
    }

    return sCount;
}

IDE_RC mtv::estimateConvertInternal( mtfCalculateFunc* aConvert,
                                     mtcNode*          aNode,
                                     mtcStack*         aStack,
                                     const mtvModule** aModule,
                                     const mtvModule** aFence,
                                     mtcTemplate*      aTemplate )
{
    if( aModule >= aFence )
    {
        aStack->column = aTemplate->rows[aNode->table].columns;
        aTemplate->rows[aNode->table].columns++;
        IDE_TEST( estimateConvertInternal( aConvert - 1,
                                           aNode,
                                           aStack + 1,
                                           aModule - 1,
                                           aFence,
                                           aTemplate )
                  != IDE_SUCCESS );
        aTemplate->rows[aNode->table].columns--;

        IDE_TEST( (*aModule)->estimate( aNode,
                                        aTemplate,
                                        aStack,
                                        2,
                                        NULL )
                  != IDE_SUCCESS );
        *aConvert = aTemplate->rows[aNode->table].execute->calculate;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/* 이 함수는 mMtvTableCheck를 만들기 위해 존재한다.
char * getDataTypeNameById(UInt a)
{
    switch(a)
    {
        case (UInt)-5:
            return "Bigint";
        case (UInt)-2:
            return "Binary";
        case 30:
            return "Blob";
        case 31:
            return "BlobLocator";
        case 40:
            return "Clob";
        case 41:
            return "ClobLocator";
        case 16:
            return "Boolean";
        case (UInt)-7:
            return "Bit";
        case (UInt)-100:
            return "Varbit";
        case 1:
            return "Char";
        case 9:
            return "Date";
        case 8:
            return "Double";
        case 6:
            return "Float";
        case 20001:
            return "Byte";
        case 20002:
            return "Nibble";
        case 4:
            return "Integer";
        case 10:
            return "Interval";
        case 10001:
            return "List";
        case 0:
            return "Null";
        case 10002:
            return "Number";
        case 2:
            return "Numeric";
        case 7:
            return "Real";
        case 5:
            return "Smallint";
        case 12:
            return "Varchar";
        case (UInt)-999:
            return "None";
        case 30001:
            return "File";
        case 10003:
            return "Geometry";
        case (UInt)-8:
            return "Nchar";
        case (UInt)-9:
            return "Nvarchar";
        case 60:
            return "Echar";
        case 61:
            return "Evarchar";
        case 90:
            return "Undef";
        case 1000001:
            return "UDT MIN & ROWTYPE";
        case 1000002:
            return "Recordtype";
        case 1000003:
            return "Associative_array";
        case 1000004:
            return "UDT MAX & REF_CURSOR";
        default:
            return "invalid";
    }
}
*/
IDE_RC mtv::initialize( mtvModule *** aExtCvtModuleGroup,
                        UInt          aGroupCnt )
{
    UInt              i;
    UInt              sCnt;

    UInt              sInternalCvtModuleCnt;
    UInt              sExternalCvtModuleCnt;

    UInt              sStage = 0;
    UInt              sFrom;
    UInt              sTo;
    UInt              sIntermediate;
    mtvModule**       sModule;
    UInt**            sTarget;

    //------------------------------------
    // Run Time Covert Module 의 구축
    //------------------------------------

    // 내부 Convert Module의 개수
    sInternalCvtModuleCnt = 0;
    for (  sModule = (mtvModule**) mtv::mInternalModule;
           *sModule != NULL;
           sModule++ )
    {
        sInternalCvtModuleCnt++;
    }

    // 외부 Convert Module의 개수
    sExternalCvtModuleCnt = 0;
    for ( i = 0; i < aGroupCnt; i++ )
    {
        for (  sModule = aExtCvtModuleGroup[i];
               *sModule != NULL;
               sModule++ )
        {
            sExternalCvtModuleCnt++;
        }
    }

    // RunTime Convert Module의 메모리 할당
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MT,
                               ID_SIZEOF(mtvModule*)
                               * (sInternalCvtModuleCnt +
                                  sExternalCvtModuleCnt + 1),
                               (void**) &mtv::mAllModule )
             != IDE_SUCCESS);
    sStage = 1;

    // RunTime Convert Module의 구축
    idlOS::memcpy ( & mtv::mAllModule[0],
                    & mtv::mInternalModule[0],
                    ID_SIZEOF(mtvModule*) * sInternalCvtModuleCnt );

    sCnt = sInternalCvtModuleCnt;
    for ( i = 0; i < aGroupCnt; i++ )
    {
        for (  sModule = aExtCvtModuleGroup[i];
               *sModule != NULL;
               sModule++, sCnt++ )
        {
            mtv::mAllModule[sCnt] = *sModule;
        }
    }

    mtv::mAllModule[sCnt] = NULL;

    //------------------------------------
    //
    //------------------------------------

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MT,
                               ID_SIZEOF(mtvTable*) * mtd::getNumberOfModules(),
                               (void**)&table)
             != IDE_SUCCESS);
    sStage = 2;

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MT,
                               ID_SIZEOF(mtvTable)          *
                               mtd::getNumberOfModules() *
                               mtd::getNumberOfModules(),
                               (void**)&(table[0]))
             != IDE_SUCCESS);
    sStage = 3;

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MT,
                               ID_SIZEOF(UInt*)*mtd::getNumberOfModules(),
                               (void**)&sTarget)
             != IDE_SUCCESS);
    sStage = 4;

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MT,
                               ID_SIZEOF(UInt)              *
                               mtd::getNumberOfModules() *
                               mtd::getNumberOfModules(),
                               (void**)&(sTarget[0]))
             != IDE_SUCCESS);
    sStage = 5;

    for( sTo = 0; sTo < mtd::getNumberOfModules(); sTo++ )
    {
        table[sTo]   = table[0] + mtd::getNumberOfModules() * sTo;
        sTarget[sTo] = sTarget[0] + mtd::getNumberOfModules() * sTo;
        for( sFrom = 0; sFrom < mtd::getNumberOfModules(); sFrom++ )
        {
            table[sTo][sFrom].cost    = sTo != sFrom ? MTV_COST_INFINITE : 0;
            table[sTo][sFrom].count   = 0;
            table[sTo][sFrom].modules = NULL;
            table[sTo][sFrom].module  = NULL;
            sTarget[sTo][sFrom]       = sTo != sFrom ? ID_UINT_MAX : 0;
        }
    }

    for( sModule = mtv::mAllModule; *sModule != NULL; sModule++ )
    {
        sTo   = (*sModule)->to->no;
        sFrom = (*sModule)->from->no;
        IDE_TEST_RAISE( table[sTo][sFrom].count != 0,
                        ERR_CONVERSION_COLLISION );
        table[sTo][sFrom].cost   = (*sModule)->cost;
        table[sTo][sFrom].count  = 1;
        table[sTo][sFrom].module = *sModule;
        sTarget[sTo][sFrom]      = sTo;
    }

    for( sIntermediate = 0;
         sIntermediate < mtd::getNumberOfModules();
         sIntermediate++ )
    {
        for( sFrom = 0; sFrom < mtd::getNumberOfModules(); sFrom++ )
        {
            for( sTo = 0; sTo < mtd::getNumberOfModules(); sTo++ )
            {
                if( table[sTo][sFrom].cost >
                    table[sTo][sIntermediate].cost +
                    table[sIntermediate][sFrom].cost )
                {
                    table[sTo][sFrom].cost = table[sTo][sIntermediate].cost  +
                        table[sIntermediate][sFrom].cost;
                    sTarget[sTo][sFrom]    = sIntermediate;
                }
            }
        }
    }

    sStage = 6;

    for( sFrom = 0; sFrom < mtd::getNumberOfModules(); sFrom++ )
    {
        for( sTo = 0; sTo < mtd::getNumberOfModules(); sTo++ )
        {
            if( sTo != sFrom )
            {
                table[sTo][sFrom].count = setModules( sTo,
                                                      sFrom,
                                                      sTarget,
                                                      NULL );
                // BUG-21627
                if ( table[sTo][sFrom].count > mtv::mMaxConvCount )
                {
                    mtv::mMaxConvCount = table[sTo][sFrom].count;
                }
                if( table[sTo][sFrom].count != 0 )
                {
                    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MT,
                                               ID_SIZEOF(mtvModule*) * table[sTo][sFrom].count,
                                               (void**)&(table[sTo][sFrom].modules))
                             != IDE_SUCCESS);
                }
                table[sTo][sFrom].count = setModules(                    sTo,
                                                                         sFrom,
                                                                         sTarget,
                                                                         table[sTo][sFrom].modules );
/*
#if defined(DEBUG)
                if(table[sTo][sFrom].count >= 1)
                {
                    for(int k=0; k < table[sTo][sFrom].count; k++)
                    {
                        IDE_ASSERT(table[sTo][sFrom].modules[k] == mMtvTableCheck[sTo][sFrom][k]);
                    }
                }
                else
                {
                    IDE_ASSERT(table[sTo][sFrom].cost == MTV_COST_INFINITE);
                    IDE_ASSERT(table[sTo][sFrom].module == NULL);
                    IDE_ASSERT(table[sTo][sFrom].modules == NULL);
                }
#endif
*/
            }
        }
    }

/* make mtvTableCheck
 * PROJ-2183 MT 형변환 개선 작업에서 mt팀과 코드 리뷰 후, 항상 형변환테이블이 mtvTableCheck형태로
 * 만들어지는 것을 검증하기 위해 만들어졌다. debug모드에서 체크하도록한다.
 * 하지만, 데이터타입이 하나 추가될 때마다 이 테이블을 유지보수해야하는 번거로움이 있으므로
 * 계속 유지하는 것에 대해서는 차후 담당팀에서 다시한번 생각해 볼 필요가 있다.

    ideLog::logMessage( IDE_SERVER_0, "{\n" );
    for( sFrom = 0; sFrom < mtd::getNumberOfModules(); sFrom++ )
    {
        mtdModule * sModule = NULL;
        mtd::moduleByNo((const mtdModule**)&sModule, sFrom);
        ideLog::logMessage( IDE_SERVER_0, "    /+ %s +/\n", getDataTypeNameById(sModule->id) );  // remove backslash
        ideLog::logMessage( IDE_SERVER_0, "    {\n" );
        for( sTo = 0; sTo < mtd::getNumberOfModules(); sTo++ )
        {
            ideLog::logMessage( IDE_SERVER_0, "        { " );

            for(int k=0; k < mtv::mMaxConvCount; k++)
            {
                if(k == mtv::mMaxConvCount - 1)
                {
                    if(k == table[sFrom][sTo].count - 1)
                    {
                        ideLog::logMessage( IDE_SERVER_0, "&mtv%s2%s ",
                                            getDataTypeNameById(((table[sFrom][sTo].modules[k])->from)->id),
                                            getDataTypeNameById(((table[sFrom][sTo].modules[k])->to)->id) );
                    }
                    else
                    {
                        ideLog::logMessage( IDE_SERVER_0, "NULL ");
                    }
                }
                else
                {
                    if(k < table[sFrom][sTo].count)
                    {
                        ideLog::logMessage( IDE_SERVER_0, "&mtv%s2%s, ",
                                            getDataTypeNameById(((table[sFrom][sTo].modules[k])->from)->id),
                                            getDataTypeNameById(((table[sFrom][sTo].modules[k])->to)->id) );
                    }
                    else
                    {
                        ideLog::logMessage( IDE_SERVER_0, "NULL, ");
                    }
                }
            }

            if( sTo == mtd::getNumberOfModules() - 1 )
            {
                ideLog::logMessage( IDE_SERVER_0, "}\n" );
            }
            else
            {
                ideLog::logMessage( IDE_SERVER_0, "},\n" );
            }
        }
        if( sFrom == mtd::getNumberOfModules() - 1)
        {
            ideLog::logMessage( IDE_SERVER_0, "    }\n" );
        }
        else
        {
            ideLog::logMessage( IDE_SERVER_0, "    },\n" );
        }
    }
    ideLog::logMessage( IDE_SERVER_0, "}\n" );
*/

    IDE_TEST(iduMemMgr::free(sTarget[0]) != IDE_SUCCESS);
    sTarget[0] = NULL;

    IDE_TEST(iduMemMgr::free(sTarget) != IDE_SUCCESS);
    sTarget = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_COLLISION );
    IDE_SET(ideSetErrorCode(mtERR_FATAL_CONVERSION_COLLISION));

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 6:
            for( sFrom = 0; sFrom < mtd::getNumberOfModules(); sFrom++ )
            {
                for( sTo = 0; sTo < mtd::getNumberOfModules(); sTo++ )
                {
                    if( table[sTo][sFrom].modules != NULL )
                    {
                        (void)iduMemMgr::free(table[sTo][sFrom].modules);
                        table[sTo][sFrom].modules = NULL;
                    }
                }
            }
        case 5:
            (void)iduMemMgr::free(sTarget[0]);
            sTarget[0] = NULL;

        case 4:
            (void)iduMemMgr::free(sTarget);
            sTarget = NULL;

        case 3:
            (void)iduMemMgr::free(table[0]);
            table[0] = NULL;

        case 2:
            (void)iduMemMgr::free(table);
            table = NULL;

        case 1:
            (void) iduMemMgr::free(mtv::mAllModule);
            mtv::mAllModule = NULL;

        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC mtv::finalize( void )
{
    UInt sFrom;
    UInt sTo;

    if( table != NULL )
    {
        for( sFrom = 0; sFrom < mtd::getNumberOfModules(); sFrom++ )
        {
            for( sTo = 0; sTo < mtd::getNumberOfModules(); sTo++ )
            {
                if( table[sTo][sFrom].modules != NULL )
                {
                    IDE_TEST(iduMemMgr::free(table[sTo][sFrom].modules )
                             != IDE_SUCCESS);
                    table[sTo][sFrom].modules = NULL;
                }
            }
        }

        IDE_TEST(iduMemMgr::free(table[0])
                 != IDE_SUCCESS);
        table[0] = NULL;

        IDE_TEST(iduMemMgr::free(table)
                 != IDE_SUCCESS);
        table = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC mtv::tableByNo( const mtvTable** aTable,
                       UInt             aTo,
                       UInt             aFrom )
{
    IDE_TEST_RAISE( aTo   >= mtd::getNumberOfModules() ||
                    aFrom >= mtd::getNumberOfModules(),
                    ERR_NOT_FOUND );

    *aTable = &table[aTo][aFrom];

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_MODULE_NOT_FOUND,""));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtv::estimateConvert4Server( iduMemory*   aMemory,
                                    mtvConvert** aConvert,
                                    mtcId        aDestinationId,
                                    mtcId        aSourceId,
                                    UInt         aSourceArgument,
                                    SInt         aSourcePrecision,
                                    SInt         aSourceScale,
                                    mtcTemplate* aTemplate )
{
    const mtdModule* sDestination;
    const mtdModule* sSource;
    const mtvTable*  sTable;
    mtvConvert*      sConvert;
    mtcNode          sNode;
    mtcTemplate      sTemplate;
    mtcTuple         sTuple;
    mtcExecute       sExecute;
    SInt             sCount;
    UInt             sSize;
    UInt             sECCSize;

    IDE_TEST( mtd::moduleById( &sDestination, aDestinationId.dataTypeId )
              != IDE_SUCCESS );

    IDE_TEST( mtd::moduleById( &sSource, aSourceId.dataTypeId )
              != IDE_SUCCESS );

    sTable = &table[sDestination->no][sSource->no];

    IDE_TEST_RAISE( sTable->count == 0, ERR_NOT_APPLICABLE );

    IDE_TEST(aMemory->alloc( ID_SIZEOF(mtvConvert) +
                             ID_SIZEOF(mtfCalculateFunc*) * sTable->count +
                             ID_SIZEOF(mtcColumn) * ( sTable->count + 1 ) +
                             ID_SIZEOF(mtcStack)  * ( sTable->count + 1 ),
                             (void**)&sConvert)
             != IDE_SUCCESS);

    sConvert->count   = sTable->count;
    sConvert->convert = (mtfCalculateFunc*)(sConvert+1);
    sConvert->columns = (mtcColumn*)(sConvert->convert+sTable->count);
    sConvert->stack   = (mtcStack*)(sConvert->columns+sTable->count+1);

    //fix BUG-16270
    sNode.module   = NULL;
    sNode.table    = 0;
    sNode.column   = 0;
    sNode.lflag    = 0;

    // BUG-16078
    // estimate에 필요한 callback함수를 연결한다.
    sTemplate.getOpenedCursor = aTemplate->getOpenedCursor;
    sTemplate.addOpenedLobCursor = aTemplate->addOpenedLobCursor;
    sTemplate.isBaseTable     = aTemplate->isBaseTable;
    sTemplate.closeLobLocator = aTemplate->closeLobLocator;
    sTemplate.getSTObjBufSize = aTemplate->getSTObjBufSize;

    // PROJ-2002 Column Security
    sTemplate.encrypt          = aTemplate->encrypt;
    sTemplate.decrypt          = aTemplate->decrypt;
    sTemplate.encodeECC        = aTemplate->encodeECC;
    sTemplate.getDecryptInfo   = aTemplate->getDecryptInfo;
    sTemplate.getECCInfo       = aTemplate->getECCInfo;
    sTemplate.getECCSize       = aTemplate->getECCSize;
    
    // PROJ-1358
    // 실행 시점의 conversion을 위하여 가상 Template을 생성한다.
    sTemplate.rows          = & sTuple;
    sTemplate.rowArrayCount = 1;
    sTemplate.rowCount      = 0;
    sTemplate.dateFormat    = aTemplate->dateFormat;
    sTemplate.dateFormatRef = aTemplate->dateFormatRef;

    /* PROJ-2208 Multi Currency */
    sTemplate.nlsCurrency    = aTemplate->nlsCurrency;
    sTemplate.nlsCurrencyRef = aTemplate->nlsCurrencyRef;

    // BUG-37247
    sTemplate.groupConcatPrecisionRef = aTemplate->groupConcatPrecisionRef;

    // BUG-41944
    sTemplate.arithmeticOpMode    = aTemplate->arithmeticOpMode;
    sTemplate.arithmeticOpModeRef = aTemplate->arithmeticOpModeRef;

    sTuple.columns = sConvert->columns;
    sTuple.execute = &sExecute;

    sConvert->stack[sTable->count].column = sTuple.columns + sTable->count;
    /*
      IDE_TEST( sSource->estimate( sConvert->stack[sTable->count].column,
      aSourceArgument,
      aSourcePrecision,
      aSourceScale )
      != IDE_SUCCESS );
    */

    IDE_TEST( mtc::initializeColumn( sConvert->stack[sTable->count].column,
                                     sSource,
                                     aSourceArgument,
                                     aSourcePrecision,
                                     aSourceScale )
              != IDE_SUCCESS );
    
    if ( ( aSourceId.dataTypeId == MTD_ECHAR_ID ) ||
         ( aSourceId.dataTypeId == MTD_EVARCHAR_ID ) )
    {
        IDE_TEST( aTemplate->getECCSize( aSourcePrecision,
                                         & sECCSize )
                  != IDE_SUCCESS );
        
        IDE_TEST( mtc::initializeEncryptColumn(
                      sConvert->stack[sTable->count].column,
                      (const SChar*) "",
                      aSourcePrecision,
                      sECCSize )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( estimateConvertInternal( sConvert->convert + sTable->count - 1,
                                       &sNode,
                                       sConvert->stack,
                                       sTable->modules + sTable->count - 1,
                                       sTable->modules,
                                       & sTemplate )
              != IDE_SUCCESS );

    for( sCount = 0, sSize = 0; sCount < (SInt)sTable->count; )
    {
        sSize += sConvert->columns[sCount].column.size;
        sSize = idlOS::align( sSize,
                              sConvert->columns[++sCount].module->align );
    }
    sSize += sConvert->columns[sCount].column.size;

    IDE_TEST(aMemory->alloc( sSize, (void**)&(sConvert->stack->value) )
             != IDE_SUCCESS);

    for( sCount = 0, sSize = 0; sCount < (SInt)sTable->count; )
    {
        sConvert->stack[sCount].value =
            (void*)( (UChar*)sConvert->stack[0].value + sSize );
        sSize += sConvert->columns[sCount].column.size;
        sSize = idlOS::align( sSize,
                              sConvert->columns[++sCount].module->align );
    }
    
    sConvert->stack[sCount].value =
        (void*)( (UChar*)sConvert->stack[0].value + sSize );

    *aConvert = sConvert;

    // PROJ-1436
    // dateFormat을 참조했음을 표시한다.
    aTemplate->dateFormatRef = sTemplate.dateFormatRef;
    
    /* PROJ-2208 Multi Currency */
    aTemplate->nlsCurrencyRef = sTemplate.nlsCurrencyRef;

    // BUG-37247
    aTemplate->groupConcatPrecisionRef = sTemplate.groupConcatPrecisionRef;
    
    // BUG-41944
    aTemplate->arithmeticOpMode    = sTemplate.arithmeticOpMode;
    aTemplate->arithmeticOpModeRef = sTemplate.arithmeticOpModeRef;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtv::estimateConvert4Server( iduVarMemList * aMemory,
                                    mtvConvert   ** aConvert,
                                    mtcId           aDestinationId,
                                    mtcId           aSourceId,
                                    UInt            aSourceArgument,
                                    SInt            aSourcePrecision,
                                    SInt            aSourceScale,
                                    mtcTemplate   * aTemplate )
{
    const mtdModule* sDestination;
    const mtdModule* sSource;
    const mtvTable*  sTable;
    mtvConvert*      sConvert;
    mtcNode          sNode;
    mtcTemplate      sTemplate;
    mtcTuple         sTuple;
    mtcExecute       sExecute;
    SInt             sCount;
    UInt             sSize;
    UInt             sECCSize;

    IDE_TEST( mtd::moduleById( &sDestination, aDestinationId.dataTypeId )
              != IDE_SUCCESS );

    IDE_TEST( mtd::moduleById( &sSource, aSourceId.dataTypeId )
              != IDE_SUCCESS );

    sTable = &table[sDestination->no][sSource->no];

    IDE_TEST_RAISE( sTable->count == 0, ERR_NOT_APPLICABLE );

    IDE_TEST(aMemory->alloc( ID_SIZEOF(mtvConvert) +
                             ID_SIZEOF(mtfCalculateFunc*) * sTable->count +
                             ID_SIZEOF(mtcColumn) * ( sTable->count + 1 ) +
                             ID_SIZEOF(mtcStack)  * ( sTable->count + 1 ),
                             (void**)&sConvert)
             != IDE_SUCCESS);

    sConvert->count   = sTable->count;
    sConvert->convert = (mtfCalculateFunc*)(sConvert+1);
    sConvert->columns = (mtcColumn*)(sConvert->convert+sTable->count);
    sConvert->stack   = (mtcStack*)(sConvert->columns+sTable->count+1);

    //fix BUG-16270
    sNode.module   = NULL;
    sNode.table    = 0;
    sNode.column   = 0;
    sNode.lflag    = 0;

    // BUG-16078
    // estimate에 필요한 callback함수를 연결한다.
    sTemplate.getOpenedCursor = aTemplate->getOpenedCursor;
    sTemplate.addOpenedLobCursor = aTemplate->addOpenedLobCursor;
    sTemplate.isBaseTable     = aTemplate->isBaseTable;
    sTemplate.closeLobLocator = aTemplate->closeLobLocator;
    sTemplate.getSTObjBufSize = aTemplate->getSTObjBufSize;

    // PROJ-2002 Column Security
    sTemplate.encrypt          = aTemplate->encrypt;
    sTemplate.decrypt          = aTemplate->decrypt;
    sTemplate.encodeECC        = aTemplate->encodeECC;
    sTemplate.getDecryptInfo   = aTemplate->getDecryptInfo;
    sTemplate.getECCInfo       = aTemplate->getECCInfo;
    sTemplate.getECCSize       = aTemplate->getECCSize;
    
    // PROJ-1358
    // 실행 시점의 conversion을 위하여 가상 Template을 생성한다.
    sTemplate.rows          = & sTuple;
    sTemplate.rowArrayCount = 1;
    sTemplate.rowCount      = 0;
    sTemplate.dateFormat    = aTemplate->dateFormat;
    sTemplate.dateFormatRef = aTemplate->dateFormatRef;

    /* PROJ-2208 Multi Currency */
    sTemplate.nlsCurrency     = aTemplate->nlsCurrency;
    sTemplate.nlsCurrencyRef  = aTemplate->nlsCurrencyRef;

    // BUG-37247
    sTemplate.groupConcatPrecisionRef = aTemplate->groupConcatPrecisionRef;
    
    // BUG-41944
    sTemplate.arithmeticOpMode    = aTemplate->arithmeticOpMode;
    sTemplate.arithmeticOpModeRef = aTemplate->arithmeticOpModeRef;
    
    sTuple.columns = sConvert->columns;
    sTuple.execute = &sExecute;

    sConvert->stack[sTable->count].column = sTuple.columns + sTable->count;
    /*
      IDE_TEST( sSource->estimate( sConvert->stack[sTable->count].column,
      aSourceArgument,
      aSourcePrecision,
      aSourceScale )
      != IDE_SUCCESS );
    */
    
    IDE_TEST( mtc::initializeColumn( sConvert->stack[sTable->count].column,
                                     sSource,
                                     aSourceArgument,
                                     aSourcePrecision,
                                     aSourceScale )
              != IDE_SUCCESS );
    
    if ( ( aSourceId.dataTypeId == MTD_ECHAR_ID ) ||
         ( aSourceId.dataTypeId == MTD_EVARCHAR_ID ) )
    {
        IDE_TEST( aTemplate->getECCSize( aSourcePrecision,
                                         & sECCSize )
                  != IDE_SUCCESS );
        
        IDE_TEST( mtc::initializeEncryptColumn(
                      sConvert->stack[sTable->count].column,
                      (const SChar*) "",
                      aSourcePrecision,
                      sECCSize )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( estimateConvertInternal( sConvert->convert + sTable->count - 1,
                                       &sNode,
                                       sConvert->stack,
                                       sTable->modules + sTable->count - 1,
                                       sTable->modules,
                                       & sTemplate )
              != IDE_SUCCESS );

    for( sCount = 0, sSize = 0; sCount < (SInt)sTable->count; )
    {
        sSize += sConvert->columns[sCount].column.size;
        sSize = idlOS::align( sSize,
                              sConvert->columns[++sCount].module->align );
    }
    sSize += sConvert->columns[sCount].column.size;

    IDE_TEST(aMemory->alloc( sSize, (void**)&(sConvert->stack->value) )
             != IDE_SUCCESS);

    for( sCount = 0, sSize = 0; sCount < (SInt)sTable->count; )
    {
        sConvert->stack[sCount].value =
            (void*)( (UChar*)sConvert->stack[0].value + sSize );
        sSize += sConvert->columns[sCount].column.size;
        sSize = idlOS::align( sSize,
                              sConvert->columns[++sCount].module->align );
    }
    
    sConvert->stack[sCount].value =
        (void*)( (UChar*)sConvert->stack[0].value + sSize );

    *aConvert = sConvert;

    // PROJ-1436
    // dateFormat을 참조했음을 표시한다.
    aTemplate->dateFormatRef = sTemplate.dateFormatRef;

    /* PROJ-2208 Multi Currency */
    aTemplate->nlsCurrencyRef = sTemplate.nlsCurrencyRef;

    // BUG-37247
    aTemplate->groupConcatPrecisionRef = sTemplate.groupConcatPrecisionRef;
    
    // BUG-41944
    aTemplate->arithmeticOpMode    = sTemplate.arithmeticOpMode;
    aTemplate->arithmeticOpModeRef = sTemplate.arithmeticOpModeRef;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mtv::estimateConvert4Cli( mtvConvert** aConvert,
                                 mtcId        aDestinationId,
                                 mtcId        aSourceId,
                                 UInt         aSourceArgument,
                                 SInt         aSourcePrecision,
                                 SInt         aSourceScale,
                                 mtcTemplate* aTemplate )
{
    const mtdModule* sDestination;
    const mtdModule* sSource;
    const mtvTable*  sTable;
    mtvConvert*      sConvert;
    mtcNode          sNode;
    mtcTemplate      sTemplate;
    mtcTuple         sTuple;
    mtcExecute       sExecute;
    SInt             sCount;
    UInt             sSize;
    UInt             sECCSize;

    IDE_TEST( mtd::moduleById( &sDestination, aDestinationId.dataTypeId )
              != IDE_SUCCESS );

    IDE_TEST( mtd::moduleById( &sSource, aSourceId.dataTypeId )
              != IDE_SUCCESS );

    sTable = &table[sDestination->no][sSource->no];

    IDE_TEST_RAISE( sTable->count == 0, ERR_NOT_APPLICABLE );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MT,
                               ID_SIZEOF(mtvConvert) +
                               ID_SIZEOF(mtfCalculateFunc*) * sTable->count +
                               ID_SIZEOF(mtcColumn) * ( sTable->count + 1 ) +
                               ID_SIZEOF(mtcStack)  * ( sTable->count + 1 ),
                               (void**)&sConvert)
             != IDE_SUCCESS);

    sConvert->count   = sTable->count;
    sConvert->convert = (mtfCalculateFunc*)(sConvert+1);
    sConvert->columns = (mtcColumn*)(sConvert->convert+sTable->count);
    sConvert->stack   = (mtcStack*)(sConvert->columns+sTable->count+1);

    sNode.table    = 0;
    sNode.column   = 0;
    sNode.lflag    = 0;

    // BUG-16078
    // estimate에 필요한 callback함수를 연결한다.
    sTemplate.getOpenedCursor = aTemplate->getOpenedCursor;
    sTemplate.addOpenedLobCursor = aTemplate->addOpenedLobCursor;
    sTemplate.isBaseTable     = aTemplate->isBaseTable;
    sTemplate.closeLobLocator = aTemplate->closeLobLocator;
    sTemplate.getSTObjBufSize = aTemplate->getSTObjBufSize;

    // PROJ-2002 Column Security
    sTemplate.encrypt          = aTemplate->encrypt;
    sTemplate.decrypt          = aTemplate->decrypt;
    sTemplate.encodeECC        = aTemplate->encodeECC;
    sTemplate.getDecryptInfo   = aTemplate->getDecryptInfo;
    sTemplate.getECCInfo       = aTemplate->getECCInfo;
    sTemplate.getECCSize       = aTemplate->getECCSize;
    
    // PROJ-1358
    // 실행 시점의 conversion을 위하여 가상 Template을 생성한다.
    sTemplate.rows          = & sTuple;
    sTemplate.rowArrayCount = 1;
    sTemplate.rowCount      = 0;
    sTemplate.dateFormat    = aTemplate->dateFormat;
    sTemplate.dateFormatRef = aTemplate->dateFormatRef;

    /* PROJ-2208 Multi Currency */
    sTemplate.nlsCurrency    = aTemplate->nlsCurrency;
    sTemplate.nlsCurrencyRef = aTemplate->nlsCurrencyRef;

    // BUG-37247
    sTemplate.groupConcatPrecisionRef = aTemplate->groupConcatPrecisionRef;
    
    // BUG-41944
    sTemplate.arithmeticOpMode    = aTemplate->arithmeticOpMode;
    sTemplate.arithmeticOpModeRef = aTemplate->arithmeticOpModeRef;
    
    sTuple.columns = sConvert->columns;
    sTuple.execute = &sExecute;

    sConvert->stack[sTable->count].column = sTuple.columns + sTable->count;
    /*
      IDE_TEST( sSource->estimate( sConvert->stack[sTable->count].column,
      aSourceArgument,
      aSourcePrecision,
      aSourceScale )
      != IDE_SUCCESS );
    */
    
    IDE_TEST( mtc::initializeColumn( sConvert->stack[sTable->count].column,
                                     sSource,
                                     aSourceArgument,
                                     aSourcePrecision,
                                     aSourceScale )
              != IDE_SUCCESS );
    
    if ( ( aSourceId.dataTypeId == MTD_ECHAR_ID ) ||
         ( aSourceId.dataTypeId == MTD_EVARCHAR_ID ) )
    {
        IDE_TEST( aTemplate->getECCSize( aSourcePrecision,
                                         & sECCSize )
                  != IDE_SUCCESS );
        
        IDE_TEST( mtc::initializeEncryptColumn(
                      sConvert->stack[sTable->count].column,
                      (const SChar*) "",
                      aSourcePrecision,
                      sECCSize )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( estimateConvertInternal( sConvert->convert + sTable->count - 1,
                                       &sNode,
                                       sConvert->stack,
                                       sTable->modules + sTable->count - 1,
                                       sTable->modules,
                                       & sTemplate )
              != IDE_SUCCESS );

    for( sCount = 0, sSize = 0; sCount < (SInt)sTable->count; )
    {
        sSize += sConvert->columns[sCount].column.size;
        sSize = idlOS::align( sSize,
                              sConvert->columns[++sCount].module->align );
    }
    sSize += sConvert->columns[sCount].column.size;

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MT,
                               sSize,
                               (void**)&(sConvert->stack->value))
             != IDE_SUCCESS);

    for( sCount = 0, sSize = 0; sCount < (SInt)sTable->count; )
    {
        sConvert->stack[sCount].value =
            (void*)( (UChar*)sConvert->stack[0].value + sSize );
        sSize += sConvert->columns[sCount].column.size;
        sSize = idlOS::align( sSize,
                              sConvert->columns[++sCount].module->align );
    }
    
    sConvert->stack[sCount].value =
        (void*)( (UChar*)sConvert->stack[0].value + sSize );

    *aConvert = sConvert;

    // PROJ-1436
    // dateFormat을 참조했음을 표시한다.
    aTemplate->dateFormatRef = sTemplate.dateFormatRef;

    /* PROJ-2208 Multi Currency */
    aTemplate->nlsCurrencyRef = sTemplate.nlsCurrencyRef;

    // BUG-37247
    aTemplate->groupConcatPrecisionRef = sTemplate.groupConcatPrecisionRef;
    
    // BUG-41944
    aTemplate->arithmeticOpMode    = sTemplate.arithmeticOpMode;
    aTemplate->arithmeticOpModeRef = sTemplate.arithmeticOpModeRef;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtv::executeConvert( mtvConvert* aConvert, mtcTemplate* aTemplate )
{
    mtcStack* sStack;
    UInt      sCount;

    for( sCount = 0, sStack = aConvert->stack + aConvert->count - 1;
         sCount < aConvert->count;
         sCount++, sStack-- )
    {
        IDE_TEST( aConvert->convert[sCount]( NULL,
                                             sStack,
                                             0,
                                             NULL,
                                             aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtv::freeConvert( mtvConvert* aConvert )
{
    IDE_TEST(iduMemMgr::free(aConvert->stack->value)
             != IDE_SUCCESS);
    aConvert->stack->value = NULL;

    IDE_TEST(iduMemMgr::free(aConvert)
             != IDE_SUCCESS);
    aConvert = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtv::float2String( UChar*          aBuffer,
                          UInt            aBufferLength,
                          UInt*           aLength,
                          mtdNumericType* aNumeric )
{
    UChar  sString[50];
    SInt   sLength;
    SInt   sExponent;
    SInt   sIterator;
    SInt   sFence;

    *aLength = 0;

    if( aNumeric->length != 0 )
    {
        if( aNumeric->length == 1 )
        {
            IDE_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
            aBuffer[0] = '0';
            (*aLength)++;
        }
        else
        {
            if( aNumeric->signExponent & 0x80 )
            {
                sExponent = ( (SInt)( aNumeric->signExponent & 0x7F ) - 64 )
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
                IDE_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
                aBuffer[0] = '-';
                aBuffer++;
                (*aLength)++;
                aBufferLength--;
                sExponent = ( 64 - (SInt)( aNumeric->signExponent & 0x7F ) )
                    << 1;
                sLength = 0;
                if( aNumeric->mantissa[0] >= 90 )
                {
                    sExponent--;
                    sString[sLength] = '0' + 99 - (SInt)aNumeric->mantissa[0];
                    sLength++;
                }
                else
                {
                    sString[sLength] = '0' + ( 99 - (SInt)aNumeric->mantissa[0] ) / 10;
                    sLength++;
                    sString[sLength] = '0' + ( 99 - (SInt)aNumeric->mantissa[0] ) % 10;
                    sLength++;
                }
                for( sIterator = 1, sFence = aNumeric->length - 1;
                     sIterator < sFence;
                     sIterator++ )
                {
                    sString[sLength] = '0' + ( 99 - (SInt)aNumeric->mantissa[sIterator] ) / 10;
                    sLength++;
                    sString[sLength] = '0' + ( 99 - (SInt)aNumeric->mantissa[sIterator] ) % 10;
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
                    if( sExponent <= (SInt)aBufferLength )
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
                    if( sLength + 1 <= (SInt)aBufferLength )
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
                if( sLength - sExponent + 2 <= (SInt)aBufferLength )
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
            IDE_TEST_RAISE( (SInt)aBufferLength < sLength + 1,
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
            IDE_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
            aBuffer[0] = 'E';
            aBuffer++;
            aBufferLength--;
            (*aLength)++;
            IDE_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
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
                IDE_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
                aBuffer[0] = '0' + sExponent / 100;
                aBuffer++;
                aBufferLength--;
                (*aLength)++;
            }
            if( sExponent >= 10 )
            {
                IDE_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
                aBuffer[0] = '0' + sExponent / 10 % 10;
                aBuffer++;
                aBufferLength--;
                (*aLength)++;
            }
            IDE_TEST_RAISE( aBufferLength < 1, ERR_INVALID_LENGTH );
            aBuffer[0] = '0' + sExponent % 10;
            aBuffer++;
            aBufferLength--;
            (*aLength)++;
        }
    }

  success:

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtv::character2NativeR( mtdCharType* aInChar, mtdDoubleType* aOutDouble )
{
    SChar           sTemp[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType* sNumeric;

    sNumeric = (mtdNumericType*)sTemp;

    IDE_TEST( mtc::makeNumeric( sNumeric,
                                MTD_FLOAT_MANTISSA_MAXIMUM,
                                aInChar->value,
                                aInChar->length )
              != IDE_SUCCESS );

    mtc::numeric2Double( aOutDouble, sNumeric );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtv::character2NativeN( mtdCharType* aInChar, mtdBigintType* aOutBigint )
{
    SChar           sTemp[MTD_NUMERIC_SIZE_MAXIMUM];
    mtdNumericType* sNumeric;

    sNumeric = (mtdNumericType*)sTemp;

    IDE_TEST( mtc::makeNumeric( sNumeric,
                                MTD_FLOAT_MANTISSA_MAXIMUM,
                                aInChar->value,
                                aInChar->length )
              != IDE_SUCCESS );

    if( sNumeric->length > 1 )
    {
        IDE_TEST( mtc::numeric2Slong ( (SLong*)aOutBigint, sNumeric )
                  != IDE_SUCCESS );
    }
    else
    {
        *aOutBigint = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* aOutChar는 char 20으로 estimate되어있다.
 */
IDE_RC mtv::nativeN2Character( mtdBigintType aInBigint, mtdCharType* aOutChar )
{
    SInt  sIterator;
    SChar sBuffer[32];

    if( aInBigint >= 0 )
    {
        sIterator = 0;
        do
        {
            sBuffer[sIterator] = ( aInBigint % 10 ) + '0';
            aInBigint /= 10;
            sIterator++;
        }
        while( aInBigint != 0 );
        for( aOutChar->length = 0,
                 sIterator--;
             sIterator >= 0;
             aOutChar->length++,
                 sIterator-- )
        {
            aOutChar->value[aOutChar->length] = sBuffer[sIterator];
        }
    }
    else
    {
        sIterator = 0;
        do
        {
            sBuffer[sIterator] = '0' - ( aInBigint % 10 );
            aInBigint /= 10;
            sIterator++;
        }
        while( aInBigint != 0 );
        aOutChar->value[0] = '-';
        for( aOutChar->length = 1,
                 sIterator--;
             sIterator >= 0;
             aOutChar->length++,
                 sIterator-- )
        {
            aOutChar->value[aOutChar->length] = sBuffer[sIterator];
        }
    }

    return IDE_SUCCESS;
}

/* aOutChar는 char 22로 estimate되어있다.
 */
IDE_RC mtv::nativeR2Character( mtdDoubleType aInDouble, mtdCharType* aOutChar )
{
    SInt   sLength;
    SChar  sBuffer[32];
#ifdef VC_WIN32
    SChar* sTarget;
    SInt   sDest;
#endif

    // BUG-17025
    if ( aInDouble == (mtdDoubleType)0 )
    {
        aInDouble = (mtdDoubleType)0;
    }
    else
    {
        // Nothing to do.
    }

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "%"ID_DOUBLE_G_FMT"",
                     (SDouble)aInDouble );
#ifdef VC_WIN32 // ex> UNIX:WIN32 = 3.06110e+04:3.06110e+004
    sTarget = strstr( sBuffer, "+0" );
    if ( sTarget == NULL )
    {
        if ( strstr( sBuffer, "-0." ) != NULL )
        {
            sTarget = NULL;
        }
        else
        {
            sTarget = strstr( sBuffer, "-0" );
        }
    }
    if ( sTarget != NULL )
    {
        sDest = (int)( sTarget - sBuffer );
        sTarget = sBuffer + sDest;
        idlOS::memmove(sTarget+1, sTarget+2, idlOS::strlen( sTarget+2)+1 );
    }
#endif
    sLength = idlOS::strlen( sBuffer );

    IDE_ASSERT( sLength <= 22 );

    idlOS::memcpy( aOutChar->value, sBuffer, sLength );

    aOutChar->length = (UShort)sLength;

    return IDE_SUCCESS;
}

IDE_RC mtv::numeric2NativeN( mtdNumericType* aInNumeric, mtdBigintType* aOutBigint )
{
    if( aInNumeric->length > 1 )
    {
        IDE_TEST( mtc::numeric2Slong( (SLong*)aOutBigint,
                                      aInNumeric )
                  != IDE_SUCCESS );
    }
    else
    {
        *aOutBigint = 0;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtv::nativeN2Numeric( mtdBigintType aInBigint, mtdNumericType* aOutNumeric )
{
    mtc::makeNumeric( aOutNumeric, (SLong)aInBigint);

    return IDE_SUCCESS;
}
