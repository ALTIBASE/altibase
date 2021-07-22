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
 * $id$
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <mt.h>
#include <qci.h>

#include <mtdTypes.h>

#include <dkDef.h>
#include <dkErrorCode.h>

/*
 *
 */ 
IDE_RC dkifUtilConvertMtdCharToCString( mtdCharType * aValue,
                                        SChar ** aCString )
{
    SChar * sCString = NULL;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       aValue->length + 1,
                                       (void **)&sCString,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERROR_MEMORY_ALLOC );

    (void)idlOS::memcpy( sCString, (const char *)aValue->value,
                          aValue->length );

    sCString[ aValue->length ] = '\0';

    *aCString = sCString;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sCString != NULL )
    {
        (void)iduMemMgr::free( sCString );
        sCString = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkifUtilFreeCString( SChar * aCString )
{
    IDE_TEST( iduMemMgr::free( aCString ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkifUtilCopyDblinkName( mtdCharType * aValue,
                               SChar * aDblinkName )
{
    IDE_TEST_RAISE( aValue->length > DK_NAME_LEN, WRONG_DBLINK_NAME );

    (void)idlOS::memcpy( aDblinkName, (const char *)aValue->value,
                         aValue->length );

    aDblinkName[ aValue->length ] = '\0';
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( WRONG_DBLINK_NAME )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKI_WRONG_LINK_NAME ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Check mtcNode's flag and count of argument is corrent.
 */
IDE_RC dkifUtilCheckNodeFlag( ULong aFlag, UInt aArgumentCount )
{
    IDE_TEST_RAISE( ( aFlag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERROR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aFlag & MTC_NODE_ARGUMENT_COUNT_MASK ) != aArgumentCount,
                    ERROR_INVALID_FUNCTION_ARGUMENT );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERROR_NOT_AGGREGATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );
    }
    IDE_EXCEPTION( ERROR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkifUtilCheckNullColumn( mtcStack * aStack, UInt aIndex )
{
    IDE_TEST_RAISE( aStack[aIndex].column->module->isNull(
                        aStack[aIndex].column,
                        aStack[aIndex].value )
                    != ID_FALSE, ERROR_ARGUMENT_NOT_APPLICABLE );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERROR_ARGUMENT_NOT_APPLICABLE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
static IDE_RC dkifUtilGetNthArgument( mtcNode * aNode,
                                      SInt aArgumentNumber,
                                      mtcNode ** aArgument )
{
    SInt i = 0;
    mtcNode * sCurrentArgument = NULL;
    
    sCurrentArgument = aNode->arguments;
    
    for ( i = 1;
          ( sCurrentArgument != NULL ) && ( i < aArgumentNumber );
          i++ )
    {
        sCurrentArgument = sCurrentArgument->next;
    }

    IDE_TEST_RAISE( sCurrentArgument == NULL,
                    ERROR_WRONG_ARGUMENT_NUMBER );

    *aArgument = sCurrentArgument;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_WRONG_ARGUMENT_NUMBER )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );        
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
 *
 */
static IDE_RC dkifUtilGetConstantIntegerValue( mtcTemplate * aTemplate,
                                               mtcNode * aNode,
                                               SInt * aValue )
{
    ULong sFlag = 0;
    mtcTuple * sTuple = NULL;
    mtcId * sType = NULL;
    smiColumn * sColumn = NULL;
    mtdSmallintType * sSmallIntValue = NULL;
    mtdIntegerType * sIntegerValue = NULL;    
    
    sFlag = ( aTemplate->rows[ aNode->table ].lflag & MTC_TUPLE_TYPE_MASK );
    IDE_TEST_RAISE( sFlag != MTC_TUPLE_TYPE_CONSTANT,
                    ERROR_NOT_CONSTANT_VALUE );

    sTuple = &(aTemplate->rows[ aNode->table ]);
    sColumn = &(sTuple->columns[ aNode->column ].column);
    sType = &(sTuple->columns[ aNode->column ].type);

    switch ( sType->dataTypeId )
    {
        case MTD_SMALLINT_ID:
            sSmallIntValue = (mtdSmallintType *)((SChar *)sTuple->row +
                                                 sColumn->offset);
            *aValue = *sSmallIntValue;
            break;
            
        case MTD_INTEGER_ID:
            sIntegerValue = (mtdIntegerType *)((SChar *)sTuple->row +
                                               sColumn->offset);
            *aValue = *sIntegerValue;
            break;
            
        default:
            IDE_RAISE( ERROR_NOT_PROPER_TYPE );
            break;
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NOT_CONSTANT_VALUE )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_ARGUMENT_IS_NOT_CONSTANT ) );
    }
    IDE_EXCEPTION( ERROR_NOT_PROPER_TYPE )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_ARGUMENT_IS_NOT_PROPER_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
 *
 */
IDE_RC dkifUtilGetIntegerValueFromNthArgument( mtcTemplate * aTemplate,
                                               mtcNode * aNode,
                                               SInt aArgumentNumber,
                                               SInt * aValue )
{
    mtcNode * sArgument = NULL;

    IDE_TEST( dkifUtilGetNthArgument( aNode, aArgumentNumber, &sArgument )
              != IDE_SUCCESS );

    IDE_TEST( dkifUtilGetConstantIntegerValue( aTemplate,
                                               sArgument,
                                               aValue )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
