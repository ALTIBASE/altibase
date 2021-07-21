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
 * $Id: mtfDigest.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <idsSHA1.h>
#include <idsSHA256.h>
#include <idsSHA512.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfDigest;

extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;

static mtcName mtfDigestFunctionName[1] = {
    { NULL, 6, (void*)"DIGEST" }
};

static IDE_RC mtfDigestEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfDigest = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDigestFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDigestEstimate
};

static IDE_RC mtfDigestCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDigestCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDigestEstimate( mtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt      /* aRemain */,
                          mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];
    mtcNode*         sAlgorithmNode;
    mtcColumn*       sAlgorithmColumn;
    mtdCharType*     sAlgorithm;
    void*            sValueTemp;
    SInt             sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

    sModules[1] = &mtdChar;

    if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK ) 
         == MTC_NODE_REESTIMATE_FALSE )
    {
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );

        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdVarchar,
                                         1,
                                         128,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        /* DIGEST( 'ABCD', 'SHA-1' ) 
         * DIGEST( 'ABCD', 'SHA-256' )
         * 과 같이 validation 단계에서 적용할 알고리즘을 알 수 있는 경우는 
         * 알고리즘에 맞게 Precision을 변경한다. 
         *
         * DIGEST 
         *   |
         * ['ABCD'] - ['SHA-1'] 
         *            ^ sAlgorithm */ 

        sAlgorithmNode = mtf::convertedNode( aNode->arguments->next, 
                                             aTemplate );

        if ( ( sAlgorithmNode == aNode->arguments->next ) && 
             ( ( aTemplate->rows[sAlgorithmNode->table].lflag 
                 & MTC_TUPLE_TYPE_MASK )
               == MTC_TUPLE_TYPE_CONSTANT ) )
        {
            sAlgorithmColumn = &(aTemplate->rows[sAlgorithmNode->table].
                                 columns[sAlgorithmNode->column]);

            sValueTemp = (void*) mtd::valueForModule(
                (smiColumn*) sAlgorithmColumn,
                aTemplate->rows[sAlgorithmNode->table].row,
                MTD_OFFSET_USE,
                sAlgorithmColumn->module->staticNull );

            /* 값이 NULL 인 경우 에러 */
            IDE_TEST_RAISE ( sAlgorithmColumn->module->isNull( sAlgorithmColumn, sValueTemp ) 
                             == ID_TRUE,
                             INVALID_DIGEST_ALGORITHM );

            sAlgorithm = (mtdCharType*) sValueTemp;
            
            if( (sAlgorithm->length == 4) &&
                (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                                 (const SChar*)"SHA1",
                                 4 ) == 0) )
            {
                sPrecision = 40;
            }
            else if( (sAlgorithm->length == 5) &&
                     (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                                      (const SChar*)"SHA-1",
                                      5 ) == 0) )
            {
                sPrecision = 40;
            }
            else if( (sAlgorithm->length == 6) &&
                     (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                                      (const SChar*)"SHA256",
                                      6 ) == 0) )
            {
                sPrecision = 64;
            }
            else if( (sAlgorithm->length == 7) &&
                     (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                                      (const SChar*)"SHA-256",
                                      7 ) == 0) )
            {
                sPrecision = 64;
            }
            else if( (sAlgorithm->length == 6) &&
                     (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                                      (const SChar*)"SHA512",
                                      6 ) == 0) )
            {
                sPrecision = 128;
            }
            else if( (sAlgorithm->length == 7) &&
                     (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                                      (const SChar*)"SHA-512",
                                      7 ) == 0) )
            {
                sPrecision = 128;
            }
            else if( (sAlgorithm->length == 3) &&
                     (idlOS::strncmp( (const SChar *)sAlgorithm->value,
                                      (const SChar*)"MD5",
                                      3 ) == 0) )
            {
                IDE_RAISE( NOT_SUPPORTED_YET );
            }
            else if( (sAlgorithm->length == 3) &&
                     (idlOS::strncmp( (const SChar *)sAlgorithm->value,
                                      (const SChar*)"MD2",
                                      3 ) == 0) )
            {
                IDE_RAISE( NOT_SUPPORTED_YET );
            }
            else
            {
                IDE_RAISE( INVALID_DIGEST_ALGORITHM );
            }

            IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                             &mtdVarchar,
                                             1,
                                             sPrecision,
                                             0 )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do 
        }
    }

    if ( ( MTC_NODE_IS_DEFINED_VALUE( aNode->arguments->next ) == ID_TRUE )
         &&
         ( ( ( aTemplate->rows[aNode->arguments->next->table].lflag
               & MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_CONSTANT ) ||
           ( ( aTemplate->rows[aNode->arguments->next->table].lflag
               & MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            
        // BUG-38070 undef type으로 re-estimate하지 않는다.
        if ( ( aTemplate->variableRow != ID_USHORT_MAX ) &&
             ( ( aNode->lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) )
        {
            if ( aTemplate->rows[aTemplate->variableRow].
                 columns->module->id == MTD_UNDEF_ID )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
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
    }
    else
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( NOT_SUPPORTED_YET );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_SUPPORTED_YET));

    IDE_EXCEPTION( INVALID_DIGEST_ALGORITHM );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_DIGEST_ALGORITHM));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfDigestCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Digest Calculate
 *
 * Implementation :
 *    DIGEST( message, algorithm )
 *
 ***********************************************************************/
    
    mtdCharType*     sDigest;
    mtdCharType*     sMessage;
    mtdCharType*     sAlgorithm;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sDigest    = (mtdCharType*)aStack[0].value;
    sMessage   = (mtdCharType*)aStack[1].value;
    sAlgorithm = (mtdCharType*)aStack[2].value;

    if( (sAlgorithm->length == 4) &&
        (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                         (const SChar*)"SHA1",
                         4 ) == 0) )
    {
        IDE_TEST( idsSHA1::digest( sDigest->value,
                                   sMessage->value,
                                   sMessage->length )
                  != IDE_SUCCESS );
        sDigest->length = 40;
    }
    else if( (sAlgorithm->length == 5) &&
             (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                              (const SChar*)"SHA-1",
                              5 ) == 0) )
    {
        IDE_TEST( idsSHA1::digest( sDigest->value,
                                   sMessage->value,
                                   sMessage->length )
                  != IDE_SUCCESS );
        sDigest->length = 40;
    }
    else if( (sAlgorithm->length == 6) &&
             (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                              (const SChar*)"SHA256",
                              6 ) == 0) )
    {
        (void) idsSHA256::digest( sDigest->value,
                                  sMessage->value,
                                  sMessage->length );
        sDigest->length = 64;
    }
    else if( (sAlgorithm->length == 7) &&
             (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                              (const SChar*)"SHA-256",
                              7 ) == 0) )
    {
        (void) idsSHA256::digest( sDigest->value,
                                  sMessage->value,
                                  sMessage->length );
        sDigest->length = 64;
    }
    else if( (sAlgorithm->length == 6) &&
             (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                              (const SChar*)"SHA512",
                              6 ) == 0) )
    {
        (void) idsSHA512::digest( sDigest->value,
                                  sMessage->value,
                                  sMessage->length );
        sDigest->length = 128;
    }
    else if( (sAlgorithm->length == 7) &&
             (idlOS::strncmp( (const SChar*)sAlgorithm->value,
                              (const SChar*)"SHA-512",
                              7 ) == 0) )
    {
        (void) idsSHA512::digest( sDigest->value,
                                  sMessage->value,
                                  sMessage->length );
        sDigest->length = 128;
    }
    else if( (sAlgorithm->length == 3) &&
             (idlOS::strncmp( (const SChar *)sAlgorithm->value,
                              (const SChar*)"MD5",
                              3 ) == 0) )
    {
        IDE_RAISE( NOT_SUPPORTED_YET );
    }
    else if( (sAlgorithm->length == 3) &&
             (idlOS::strncmp( (const SChar *)sAlgorithm->value,
                              (const SChar*)"MD2",
                              3 ) == 0) )
    {
        IDE_RAISE( NOT_SUPPORTED_YET );
    }
    else
    {
        IDE_RAISE( INVALID_DIGEST_ALGORITHM );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( NOT_SUPPORTED_YET );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_SUPPORTED_YET));
    };
    IDE_EXCEPTION( INVALID_DIGEST_ALGORITHM );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_DIGEST_ALGORITHM));
    };
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
