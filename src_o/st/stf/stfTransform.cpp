/***********************************************************************
 * Copyright 1999-2020, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: stfTransform.cpp 88007 2020-07-10 00:26:34Z cory.chae $
 **********************************************************************/

#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <stdTypes.h>
#include <stdUtils.h>
#include <ste.h>
#include <stfBasic.h>
#include <stfProj4.h>

#if defined(ST_ENABLE_PROJ4_LIBRARY)

extern mtdModule stdGeometry;
extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;

extern mtfModule stfTransform;

static mtcName stfTransformFunctionName[1] = {
    { NULL, 12, (void*)"ST_TRANSFORM" }
};

static IDE_RC stfTransformEstimate( mtcNode     * aNode,
                                    mtcTemplate * aTemplate,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    mtcCallBack * aCallBack );

mtfModule stfTransform = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    stfTransformFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfTransformEstimate
};

IDE_RC stfTransformCalculateGI( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

IDE_RC stfTransformCalculateGV( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );


IDE_RC stfTransformCalculateGVV( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC stfTransformCalculateGVI( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

static const mtcExecute stfExecuteGI = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfTransformCalculateGI,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute stfExecuteGV = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfTransformCalculateGV,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute stfExecuteGVV = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfTransformCalculateGVV,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute stfExecuteGVI = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfTransformCalculateGVI,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC stfTransformEstimate( mtcNode     * aNode,
                             mtcTemplate * aTemplate,
                             mtcStack    * aStack,
                             SInt       /* aRemain */,
                             mtcCallBack * aCallBack )
{
    const mtdModule  * sModules[3];
    UInt               sModuleId          = 0;
    UInt               sArgumentCount     = 0;
    qcTemplate       * sTemplate = NULL;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) == MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    sArgumentCount = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    IDE_TEST_RAISE( ( sArgumentCount < 2 ) ||
                    ( sArgumentCount > 3 ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    if ( sArgumentCount == 2 )
    {
        sModuleId = aStack[2].column->module->id;

        switch ( sModuleId )
        {
            case MTD_SMALLINT_ID :
            case MTD_INTEGER_ID :
            case MTD_BIGINT_ID :
                sModules[0] = &stdGeometry;
                sModules[1] = &mtdInteger;

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );

                aTemplate->rows[aNode->table].execute[aNode->column] = stfExecuteGI;

                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 &stdGeometry,
                                                 1,
                                                 aStack[1].column->precision,
                                                 0 )
                          != IDE_SUCCESS );
                break;

            case MTD_CHAR_ID :
            case MTD_VARCHAR_ID :
            case MTD_NCHAR_ID :
            case MTD_NVARCHAR_ID :
                sModules[0] = &stdGeometry;
                sModules[1] = &mtdVarchar;

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );

                aTemplate->rows[aNode->table].execute[aNode->column] = stfExecuteGV;

                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 &stdGeometry,
                                                 1,
                                                 aStack[1].column->precision,
                                                 0 )
                          != IDE_SUCCESS );
                break;
            
            default :
                IDE_RAISE( ERR_OBJECT_TYPE_NOT_APPLICABLE );
        }

    }
    else if ( sArgumentCount == 3 )
    {
        sModuleId = aStack[3].column->module->id;

        switch ( sModuleId )
        {
            case MTD_SMALLINT_ID : 
            case MTD_INTEGER_ID :
            case MTD_BIGINT_ID :
                sModules[0] = &stdGeometry;
                sModules[1] = &mtdVarchar;
                sModules[2] = &mtdInteger;

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );

                aTemplate->rows[aNode->table].execute[aNode->column] = stfExecuteGVI;

                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 &stdGeometry,
                                                 1,
                                                 aStack[1].column->precision,
                                                 0 )
                          != IDE_SUCCESS );
                break;

            case MTD_CHAR_ID :
            case MTD_VARCHAR_ID :
            case MTD_NCHAR_ID :
            case MTD_NVARCHAR_ID :
                sModules[0] = &stdGeometry;
                sModules[1] = &mtdVarchar;
                sModules[2] = &mtdVarchar;

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );

                aTemplate->rows[aNode->table].execute[aNode->column] = stfExecuteGVV;

                IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                                 &stdGeometry,
                                                 1,
                                                 aStack[1].column->precision,
                                                 0 )
                          != IDE_SUCCESS );
                break;

             default :
                IDE_RAISE( ERR_OBJECT_TYPE_NOT_APPLICABLE );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-42639 수정 사항 대응 코드
     * Cursor를 열어야하므로 테이블 종류와 무관하게 Transaction을 생성하도록 설정한다.
     */
    sTemplate = (qcTemplate *)aTemplate;
    sTemplate->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
    sTemplate->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_NOT_AGGREGATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION( ERR_OBJECT_TYPE_NOT_APPLICABLE )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfTransformCalculateGI( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate )
{
    mtdCharType             sFromMtdCharType;
    mtdCharType           * sFromProj4Text     = &sFromMtdCharType;
    SInt                    sFromSRIDSInt;
    mtdCharType             sToMtdCharType;
    mtdCharType           * sToProj4Text       = &sToMtdCharType;
    mtdIntegerType        * sToSRIDInteger     = NULL;
    qcTemplate            * sTemplate          = NULL;
    stdGeometryHeader     * sFromGeometry      = NULL;
    stdGeometryHeader     * sToGeometry        = NULL;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sFromGeometry = (stdGeometryHeader *)aStack[1].value;

    if ( ( stdGeometry.isNull( NULL, sFromGeometry ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else 
    {
        if ( stdUtils::isEmpty( sFromGeometry ) == ID_TRUE )
        {
            sToGeometry = (stdGeometryHeader *)aStack[0].value;
                
            stdUtils::copyGeometry( sToGeometry, sFromGeometry );       
        }
        else
        {
            sTemplate = (qcTemplate *)aTemplate;
            
            sFromSRIDSInt = stfBasic::getSRID( sFromGeometry );

            IDE_TEST( stfBasic::getProj4TextFromSRID( sTemplate->stmt, 
                                                      &sFromSRIDSInt, 
                                                      &sFromProj4Text )
                      != IDE_SUCCESS );


            sToSRIDInteger = (mtdIntegerType *)aStack[2].value;

            IDE_TEST( stfBasic::getProj4TextFromSRID( sTemplate->stmt, 
                                                      sToSRIDInteger, 
                                                      &sToProj4Text )
                      != IDE_SUCCESS );

            if ( sFromSRIDSInt == *sToSRIDInteger )
            {
                sToGeometry = (stdGeometryHeader *)aStack[0].value;
                
                stdUtils::copyGeometry( sToGeometry, sFromGeometry );
            }
            else
            {
                IDE_TEST( stfProj4::doTransform( aStack, 
                                                 sFromProj4Text, 
                                                 sToProj4Text )
                          != IDE_SUCCESS );

                if ( stdUtils::isExtendedType( sFromGeometry->mType ) == ID_TRUE )
                {
                    sToGeometry = (stdGeometryHeader *)aStack[0].value;
                    
                    IDE_TEST( stfBasic::setSRID( sToGeometry,
                                                 aStack[0].column->precision,
                                                 *sToSRIDInteger )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfTransformCalculateGV( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate )
{
    mtdCharType             sFromMtdCharType;
    mtdCharType           * sFromProj4Text     = &sFromMtdCharType;
    SInt                    sFromSRIDSInt;
    mtdCharType           * sToProj4Text       = NULL;
    qcTemplate            * sTemplate          = NULL;
    stdGeometryHeader     * sFromGeometry      = NULL;
    stdGeometryHeader     * sToGeometry        = NULL;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sFromGeometry = (stdGeometryHeader *)aStack[1].value;

    if ( ( stdGeometry.isNull( NULL, sFromGeometry ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else 
    {
        if ( stdUtils::isEmpty( sFromGeometry ) == ID_TRUE )
        {
            sToGeometry = (stdGeometryHeader *)aStack[0].value;
                
            stdUtils::copyGeometry( sToGeometry, sFromGeometry );       
        }
        else
        {
            sTemplate = (qcTemplate *)aTemplate;

            sFromSRIDSInt = stfBasic::getSRID( sFromGeometry );

            IDE_TEST( stfBasic::getProj4TextFromSRID( sTemplate->stmt, 
                                                      &sFromSRIDSInt, 
                                                      &sFromProj4Text )
                      != IDE_SUCCESS );

            sToProj4Text = (mtdCharType *)aStack[2].value;

            IDE_TEST( stfProj4::doTransform( aStack, 
                                             sFromProj4Text, 
                                             sToProj4Text )
                      != IDE_SUCCESS );
                      
            if ( stdUtils::isExtendedType( sFromGeometry->mType ) == ID_TRUE )
            {
                sToGeometry = (stdGeometryHeader *)aStack[0].value;
                
                IDE_TEST( stfBasic::setSRID( sToGeometry,
                                             aStack[0].column->precision,
                                             ST_SRID_UNDEFINED )
                          != IDE_SUCCESS );
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


IDE_RC stfTransformCalculateGVV( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate )
{
    mtdCharType           * sFromProj4Text     = NULL;
    mtdCharType           * sToProj4Text       = NULL;
    stdGeometryHeader     * sFromGeometry      = NULL;
    stdGeometryHeader     * sToGeometry        = NULL;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sFromGeometry = (stdGeometryHeader *)aStack[1].value;

    if ( ( stdGeometry.isNull( NULL, sFromGeometry ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) ||
         ( aStack[3].column->module->isNull( aStack[3].column,
                                             aStack[3].value ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else 
    {
        if ( stdUtils::isEmpty( sFromGeometry ) == ID_TRUE )
        {
            sToGeometry = (stdGeometryHeader *)aStack[0].value;
                
            stdUtils::copyGeometry( sToGeometry, sFromGeometry );       
        }
        else
        {
            sFromProj4Text = (mtdCharType *)aStack[2].value;
            sToProj4Text = (mtdCharType *)aStack[3].value;

            IDE_TEST( stfProj4::doTransform( aStack,
                                             sFromProj4Text, 
                                             sToProj4Text )
                      != IDE_SUCCESS );

            if ( stdUtils::isExtendedType( sFromGeometry->mType ) == ID_TRUE )
            {
                sToGeometry = (stdGeometryHeader *)aStack[0].value;
                
                IDE_TEST( stfBasic::setSRID( sToGeometry,
                                             aStack[0].column->precision,
                                             ST_SRID_UNDEFINED )
                          != IDE_SUCCESS );
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

IDE_RC stfTransformCalculateGVI( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate )
{
    mtdCharType           * sFromProj4Text     = NULL;
    mtdCharType             sToMtdCharType;
    mtdCharType           * sToProj4Text       = &sToMtdCharType;
    mtdIntegerType        * sToSRIDInteger     = NULL;
    qcTemplate            * sTemplate          = NULL;
    stdGeometryHeader     * sFromGeometry      = NULL;
    stdGeometryHeader     * sToGeometry        = NULL;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sFromGeometry = (stdGeometryHeader *)aStack[1].value;

    if ( ( stdGeometry.isNull( NULL, sFromGeometry ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) ||
         ( aStack[3].column->module->isNull( aStack[3].column,
                                             aStack[3].value ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else 
    {
        if ( stdUtils::isEmpty( sFromGeometry ) == ID_TRUE )
        {
            sToGeometry = (stdGeometryHeader *)aStack[0].value;
                
            stdUtils::copyGeometry( sToGeometry, sFromGeometry );       
        }
        else
        {
            sTemplate = (qcTemplate *)aTemplate;
            sFromProj4Text = (mtdCharType *)aStack[2].value;
            sToSRIDInteger = (mtdIntegerType *)aStack[3].value;

            IDE_TEST( stfBasic::getProj4TextFromSRID( sTemplate->stmt, 
                                                      sToSRIDInteger, 
                                                      &sToProj4Text )
                      != IDE_SUCCESS );

            IDE_TEST( stfProj4::doTransform( aStack,
                                             sFromProj4Text, 
                                             sToProj4Text )
                      != IDE_SUCCESS );

            if ( stdUtils::isExtendedType( sFromGeometry->mType ) == ID_TRUE )
            {
                sToGeometry = (stdGeometryHeader *)aStack[0].value;
                
                IDE_TEST( stfBasic::setSRID( sToGeometry,
                                             aStack[0].column->precision,
                                             *sToSRIDInteger )
                          != IDE_SUCCESS );
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

#endif /* ST_ENABLE_PROJ4_LIBRARY */
