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
 * $Id: mtvUndef2All.cpp 47933 2011-11-09 02:01:37Z junokun $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvUndef2Bigint;
extern mtvModule mtvUndef2Bit;
extern mtvModule mtvUndef2Blob;
extern mtvModule mtvUndef2Boolean;
extern mtvModule mtvUndef2Byte;
extern mtvModule mtvUndef2Char;
extern mtvModule mtvUndef2Clob;
extern mtvModule mtvUndef2Date;
extern mtvModule mtvUndef2Double;
extern mtvModule mtvUndef2Echar;
extern mtvModule mtvUndef2Evarchar;
extern mtvModule mtvUndef2Float;
extern mtvModule mtvUndef2Integer;
extern mtvModule mtvUndef2Interval;
extern mtvModule mtvUndef2Nchar;
extern mtvModule mtvUndef2Nibble;
extern mtvModule mtvUndef2Null;
extern mtvModule mtvUndef2Numeric;
extern mtvModule mtvUndef2Nvarchar;
extern mtvModule mtvUndef2Real;
extern mtvModule mtvUndef2Smallint;
extern mtvModule mtvUndef2Varbit;
extern mtvModule mtvUndef2Varchar;
extern mtvModule mtvUndef2List;
extern mtvModule mtvUndef2Number;
extern mtvModule mtvUndef2File;
extern mtvModule mtvUndef2BlobLocator;
extern mtvModule mtvUndef2ClobLocator;
extern mtvModule mtvUndef2Binary;

extern mtdModule mtdUndef;
extern mtdModule mtdBigint;
extern mtdModule mtdBit;
extern mtdModule mtdBlob;
extern mtdModule mtdBoolean;
extern mtdModule mtdByte;
extern mtdModule mtdChar;
extern mtdModule mtdClob;
extern mtdModule mtdDate;
extern mtdModule mtdDouble;
extern mtdModule mtdEchar;
extern mtdModule mtdEvarchar;
extern mtdModule mtdFloat;
extern mtdModule mtdInteger;
extern mtdModule mtdInterval;
extern mtdModule mtdNchar;
extern mtdModule mtdNibble;
extern mtdModule mtdNull;
extern mtdModule mtdNumeric;
extern mtdModule mtdNvarchar;
extern mtdModule mtdReal;
extern mtdModule mtdSmallint;
extern mtdModule mtdVarbit;
extern mtdModule mtdVarbyte;
extern mtdModule mtdVarchar;
extern mtdModule mtdList;
extern mtdModule mtdNumber;
extern mtdModule mtsFile;
extern mtdModule mtdBlobLocator;
extern mtdModule mtdClobLocator;
extern mtdModule mtdBinary;
extern mtdModule mtsConnect;

/*
static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );
*/

static IDE_RC mtvEstimateBigint( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateBit( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateBlob( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateBoolean( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateByte( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateChar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateClob( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateDate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateDouble( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateEchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateEvarchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateFloat( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateInteger( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateInterval( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateNchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateNibble( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateNull( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateNumeric( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateNvarchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateReal( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateSmallint( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateVarbit( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateVarbyte( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateVarchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

static IDE_RC mtvEstimateList( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               mtcCallBack* aCallBack );

static IDE_RC mtvEstimateNumber( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

static IDE_RC mtvEstimateFile( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               mtcCallBack* aCallBack );

// BUG-40854
static IDE_RC mtvEstimateConnect( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

static IDE_RC mtvEstimateBlobLocator( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

static IDE_RC mtvEstimateClobLocator( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

static IDE_RC mtvEstimateBinary( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Undef2All( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

mtvModule mtvUndef2Bigint = {
    &mtdBigint,
    &mtdUndef,
    0,
    mtvEstimateBigint
};

mtvModule mtvUndef2Bit = {
    &mtdBit,
    &mtdUndef,
    0,
    mtvEstimateBit
};

mtvModule mtvUndef2Blob = {
    &mtdBlob,
    &mtdUndef,
    0,
    mtvEstimateBlob
};

mtvModule mtvUndef2Boolean = {
    &mtdBoolean,
    &mtdUndef,
    0,
    mtvEstimateBoolean
};

mtvModule mtvUndef2Byte = {
    &mtdByte,
    &mtdUndef,
    0,
    mtvEstimateByte
};

mtvModule mtvUndef2Char = {
    &mtdChar,
    &mtdUndef,
    0,
    mtvEstimateChar
};

mtvModule mtvUndef2Clob = {
    &mtdClob,
    &mtdUndef,
    0,
    mtvEstimateClob
};

mtvModule mtvUndef2Date = {
    &mtdDate,
    &mtdUndef,
    0,
    mtvEstimateDate
};

mtvModule mtvUndef2Double = {
    &mtdDouble,
    &mtdUndef,
    0,
    mtvEstimateDouble
};

mtvModule mtvUndef2Echar = {
    &mtdEchar,
    &mtdUndef,
    0,
    mtvEstimateEchar
};

mtvModule mtvUndef2Evarchar = {
    &mtdEvarchar,
    &mtdUndef,
    0,
    mtvEstimateEvarchar
};

mtvModule mtvUndef2Float = {
    &mtdFloat,
    &mtdUndef,
    0,
    mtvEstimateFloat
};

mtvModule mtvUndef2Integer = {
    &mtdInteger,
    &mtdUndef,
    0,
    mtvEstimateInteger
};

mtvModule mtvUndef2Interval = {
    &mtdInterval,
    &mtdUndef,
    0,
    mtvEstimateInterval
};

mtvModule mtvUndef2Nchar = {
    &mtdNchar,
    &mtdUndef,
    0,
    mtvEstimateNchar
};

mtvModule mtvUndef2Nibble = {
    &mtdNibble,
    &mtdUndef,
    0,
    mtvEstimateNibble
};

mtvModule mtvUndef2Null = {
    &mtdNull,
    &mtdUndef,
    0,
    mtvEstimateNull
};

mtvModule mtvUndef2Numeric = {
    &mtdNumeric,
    &mtdUndef,
    0,
    mtvEstimateNumeric
};

mtvModule mtvUndef2Nvarchar = {
    &mtdNvarchar,
    &mtdUndef,
    0,
    mtvEstimateNvarchar
};

mtvModule mtvUndef2Real = {
    &mtdReal,
    &mtdUndef,
    0,
    mtvEstimateReal
};

mtvModule mtvUndef2Smallint = {
    &mtdSmallint,
    &mtdUndef,
    0,
    mtvEstimateSmallint
};

mtvModule mtvUndef2Varbit = {
    &mtdVarbit,
    &mtdUndef,
    0,
    mtvEstimateVarbit
};

mtvModule mtvUndef2Varbyte = {
    &mtdVarbyte,
    &mtdUndef,
    0,
    mtvEstimateVarbyte
};

mtvModule mtvUndef2Varchar = {
    &mtdVarchar,
    &mtdUndef,
    0,
    mtvEstimateVarchar
};

mtvModule mtvUndef2List = {
    &mtdList,
    &mtdUndef,
    0,
    mtvEstimateList
};

mtvModule mtvUndef2Number = {
    &mtdNumber,
    &mtdUndef,
    0,
    mtvEstimateNumber
};

mtvModule mtvUndef2File = {
    &mtsFile,
    &mtdUndef,
    0,
    mtvEstimateFile
};

// BUG-40854
mtvModule mtvUndef2Connect = {
    &mtsConnect,
    &mtdUndef,
    0,
    mtvEstimateConnect
};

mtvModule mtvUndef2BlobLocator = {
    &mtdBlobLocator,
    &mtdUndef,
    0,
    mtvEstimateBlobLocator
};

mtvModule mtvUndef2ClobLocator = {
    &mtdClobLocator,
    &mtdUndef,
    0,
    mtvEstimateClobLocator
};

mtvModule mtvUndef2Binary = {
    &mtdBinary,
    &mtdUndef,
    0,
    mtvEstimateBinary
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,          // initialize
    mtf::calculateNA,          // aggregate
    mtf::calculateNA,
    mtf::calculateNA,          // finalize
    mtvCalculate_Undef2All,      // calculate
    NULL,                      // additional info for calculate (NA)
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};
/*
static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBlob,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
*/

static IDE_RC mtvEstimateBigint( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateBit( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBit,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateBlob( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBlob,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateBoolean( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateByte( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdByte,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateChar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdChar,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateClob( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdClob,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateDate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDate,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateDouble( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateEchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdEchar,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateEvarchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdEvarchar,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateFloat( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateInteger( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateInterval( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInterval,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateNchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNchar,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateNibble( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNibble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateNull( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNull,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateNumeric( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNumeric,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateNvarchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNvarchar,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateReal( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdReal,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateSmallint( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdSmallint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateVarbit( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarbit,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateVarbyte( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarbyte,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateVarchar( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarchar,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateList( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt,
                               mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdList,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateNumber( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt,
                                 mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNumber,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateFile( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt,
                               mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtsFile,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-40854
static IDE_RC mtvEstimateConnect( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt,
                                  mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtsConnect,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateBlobLocator( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt,
                                      mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBlobLocator,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateClobLocator( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt,
                                      mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdClobLocator,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtvEstimateBinary( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt,
                                 mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBinary,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Undef2All( mtcNode*,
                             mtcStack*,
                             SInt,
                             void*,
                             mtcTemplate* )
{
    // undef 타입은 실제 conversion 이 실행되면 안되므로 ASSERT 처리한다.
    IDE_ASSERT(0);

    return IDE_SUCCESS;
}
