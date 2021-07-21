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
 * $Id: mtx.cpp 84870 2019-02-08 07:06:26Z andrew.shin $
 **********************************************************************/

#include <mtx.h>

extern mtxModule mtxRid;
extern mtxModule mtxColumn;
extern mtxModule mtxAnd;
extern mtxModule mtxOr;
extern mtxModule mtxNot;
extern mtxModule mtxLnnvl;

IDE_RC mtx::calculateEmpty( mtxEntry ** /* aEntry */ )
{
    IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_APPLICABLE ) );

    return IDE_FAILURE;
}

IDE_RC mtx::calculateNA( mtxEntry ** /* aEntry */ )
{
    IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_APPLICABLE ) );

    return IDE_FAILURE;
}

IDE_RC mtx::generateSerialExecute( mtcTemplate          * aTemplate,
                                   mtxSerialFilterInfo  * aInfo,
                                   mtxSerialExecuteData * aData,
                                   UInt                   aTable )
{
    mtxSerialFilterInfo * sInfo       = NULL;
    mtxSerialFilterInfo * sInfoFense  = NULL;
    mtxEntry            * sFirst      = NULL;
    mtxEntry            * sEntryFense = NULL;
    mtxEntry            * sEntry      = NULL;
    void                * sAddress1   = NULL;
    void                * sAddress2   = NULL;
    void                * sAddress3   = NULL;

    IDE_TEST( ( aInfo == NULL ) ||
              ( aData == NULL ) );

    sInfo       = aInfo;
    sInfoFense  = aInfo + aData->mEntrySetCount;
    sEntry      = aData->mEntry;
    sFirst      = aData->mEntry;
    sEntryFense = aData->mEntry + aData->mEntryTotalSize;

    IDE_TEST_CONT( aData->mTable != NULL,
                   SKIP_BUILD );

    while ( sInfo < sInfoFense )
    {
        IDE_TEST_RAISE( ( sEntry + sInfo->mHeader.mSize ) > sEntryFense,
                        INVALID_ENTRY_ERROR )

        IDE_TEST_RAISE( sEntry != ( sFirst + sInfo->mOffset ),
                        INVALID_ENTRY_ERROR );

        switch( sInfo->mHeader.mType )
        {
            case MTX_ENTRY_TYPE_VALUE:
                sAddress1 = MTX_GET_RETURN_ADDR( aTemplate, sInfo->mNode );

                IDE_TEST_RAISE( sAddress1 == NULL,
                                NULL_POINTER );

                /* control, return */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_RETN( sEntry, sAddress1 );

                break;

            case MTX_ENTRY_TYPE_RID:
                sAddress1 = NULL;
                sAddress2 = MTX_GET_TUPLE_ADDR( aTemplate, sInfo->mNode );

                IDE_TEST_RAISE( sAddress2 == NULL,
                                NULL_POINTER );

                /* control, return, tuple */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_RETN( sEntry, sAddress1 );
                MTX_SET_ENTRY_ADDR( sEntry, sAddress2 );

                break;

            case MTX_ENTRY_TYPE_COLUMN:
                sAddress1 = NULL;
                sAddress2 = MTX_GET_COLUNM_ADDR( aTemplate, sInfo->mNode );
                sAddress3 = MTX_GET_TUPLE_ADDR( aTemplate, sInfo->mNode );

                IDE_TEST_RAISE( ( sAddress2 == NULL ) ||
                                ( sAddress3 == NULL ),
                                NULL_POINTER );

                /* control, return, column, tuple */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_RETN( sEntry, sAddress1 );
                MTX_SET_ENTRY_ADDR( sEntry, sAddress2 );
                MTX_SET_ENTRY_ADDR( sEntry, sAddress3 );

                break;

            case MTX_ENTRY_TYPE_FUNCTION:
                sAddress1 = MTX_GET_RETURN_ADDR( aTemplate, sInfo->mNode );

                IDE_TEST_RAISE( ( sAddress1       == NULL ) ||
                                ( sInfo->mExecute == NULL ),
                                NULL_POINTER );

                IDE_TEST_RAISE( ( sInfo->mLeft->mOffset  >= aData->mEntryTotalSize ) ||
                                ( sInfo->mRight->mOffset >= aData->mEntryTotalSize ),
                                INVALID_ENTRY_ERROR );

                /* control, return, argument 1, argument 2, function */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_RETN( sEntry, sAddress1 );
                MTX_SET_ENTRY_ADDR( sEntry, sFirst + sInfo->mLeft->mOffset );
                MTX_SET_ENTRY_ADDR( sEntry, sFirst + sInfo->mRight->mOffset );
                MTX_SET_ENTRY_EXEC( sEntry, sInfo->mExecute );

                break;

            case MTX_ENTRY_TYPE_SINGLE:
            case MTX_ENTRY_TYPE_CONVERT:
                sAddress1 = MTX_GET_RETURN_ADDR( aTemplate, sInfo->mNode );

                IDE_TEST_RAISE( ( sAddress1       == NULL ) ||
                                ( sInfo->mExecute == NULL ),
                                NULL_POINTER );

                IDE_TEST_RAISE( sInfo->mLeft->mOffset >= aData->mEntryTotalSize,
                                INVALID_ENTRY_ERROR );

                /* control, return, argument 1, function */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_RETN( sEntry, sAddress1 );
                MTX_SET_ENTRY_ADDR( sEntry, sFirst + sInfo->mLeft->mOffset );
                MTX_SET_ENTRY_EXEC( sEntry, sInfo->mExecute );

                break;

            case MTX_ENTRY_TYPE_CONVERT_CHAR:
                sAddress1 = MTX_GET_RETURN_ADDR( aTemplate, sInfo->mNode );
                sAddress2 = MTX_GET_MTC_COLUMN( aTemplate, sInfo->mNode );

                IDE_TEST_RAISE( ( sAddress1       == NULL ) ||
                                ( sAddress2       == NULL ) ||
                                ( sInfo->mExecute == NULL ),
                                NULL_POINTER );

                IDE_TEST_RAISE( sInfo->mLeft->mOffset >= aData->mEntryTotalSize,
                                INVALID_ENTRY_ERROR );

                /* control, return, argument 1, column( for precision ), function */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_RETN( sEntry, sAddress1 );
                MTX_SET_ENTRY_ADDR( sEntry, sFirst + sInfo->mLeft->mOffset );
                MTX_SET_ENTRY_ADDR( sEntry, sAddress2 );
                MTX_SET_ENTRY_EXEC( sEntry, sInfo->mExecute );

                break;

            case MTX_ENTRY_TYPE_CONVERT_DATE:
                sAddress1 = MTX_GET_RETURN_ADDR( aTemplate, sInfo->mNode );
                sAddress2 = (void *)aTemplate->dateFormat;
                sAddress3 = (void *)&( aTemplate->dateFormatRef );

                IDE_TEST_RAISE( ( sAddress1       == NULL ) ||
                                ( sAddress2       == NULL ) ||
                                ( sAddress3       == NULL ) ||
                                ( sInfo->mExecute == NULL ),
                                NULL_POINTER );

                IDE_TEST_RAISE( sInfo->mLeft->mOffset >= aData->mEntryTotalSize,
                                INVALID_ENTRY_ERROR );

                /* control, return, argument 1, date_format, boolean, function */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_RETN( sEntry, sAddress1 );
                MTX_SET_ENTRY_ADDR( sEntry, sFirst + sInfo->mLeft->mOffset );
                MTX_SET_ENTRY_ADDR( sEntry, sAddress2 );
                MTX_SET_ENTRY_ADDR( sEntry, sAddress3 );
                MTX_SET_ENTRY_EXEC( sEntry, sInfo->mExecute );

                break;

            case MTX_ENTRY_TYPE_CHECK:
                IDE_TEST_RAISE( sInfo->mLeft->mOffset >= aData->mEntryTotalSize,
                                INVALID_ENTRY_ERROR );

                /* control, argument 1 */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_ADDR( sEntry, sFirst + sInfo->mLeft->mOffset );

                break;

            case MTX_ENTRY_TYPE_AND:
            case MTX_ENTRY_TYPE_OR:
                sAddress1 = MTX_GET_RETURN_ADDR( aTemplate, sInfo->mNode );

                IDE_TEST_RAISE( sAddress1 == NULL,
                                NULL_POINTER );

                IDE_TEST_RAISE( ( sInfo->mLeft->mOffset  >= aData->mEntryTotalSize ) ||
                                ( sInfo->mRight->mOffset >= aData->mEntryTotalSize ),
                                INVALID_ENTRY_ERROR );

                /* control, return, argument 1, argument 2 */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_RETN( sEntry, sAddress1 );
                MTX_SET_ENTRY_ADDR( sEntry, sFirst + sInfo->mLeft->mOffset );
                MTX_SET_ENTRY_ADDR( sEntry, sFirst + sInfo->mRight->mOffset );

                break;

            case MTX_ENTRY_TYPE_AND_SINGLE:
            case MTX_ENTRY_TYPE_OR_SINGLE:
            case MTX_ENTRY_TYPE_NOT:
            case MTX_ENTRY_TYPE_LNNVL:
                sAddress1 = MTX_GET_RETURN_ADDR( aTemplate, sInfo->mNode );

                IDE_TEST_RAISE( sAddress1 == NULL,
                                NULL_POINTER );

                IDE_TEST_RAISE( sInfo->mLeft->mOffset >= aData->mEntryTotalSize,
                                INVALID_ENTRY_ERROR );

                /* control, return, argument 1 */
                MTX_SET_ENTRY_HEAD( sEntry, sInfo->mHeader );
                MTX_SET_ENTRY_RETN( sEntry, sAddress1 );
                MTX_SET_ENTRY_ADDR( sEntry, sFirst + sInfo->mLeft->mOffset );

                break;

            default:
                IDE_RAISE( INVALID_ENTRY_TYPE );

                break;
        }

        sInfo++;
    }

    aData->mTable = aTemplate->rows + aTable;

    IDE_EXCEPTION_CONT( SKIP_BUILD );

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ERROR )
    {
        ideLog::log( IDE_QP_0,
                     "Serial Execute Failure: %s: %s",
                     "mtx::generateSerialExecute",
                     "access an invalid entry" );
    }
    IDE_EXCEPTION( NULL_POINTER )
    {
        ideLog::log( IDE_QP_0,
                     "Serial Execute Failure: %s: %s",
                     "mtx::generateSerialExecute",
                     "null pointer exception were occurred" );
    }
    IDE_EXCEPTION( INVALID_ENTRY_TYPE )
    {
        ideLog::log( IDE_QP_0,
                     "Serial Execute Failure: %s: %s",
                     "mtx::generateSerialExecute",
                     "invalid entry type" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtx::doSerialFilterExecute( idBool       * aResult,
                                   const void   * aRow,
                                   void         *, /* aDirectKey */
                                   UInt          , /* aDirectKeyPartialSize */
                                   const scGRID   aRid,
                                   void         * aData )
{
    mtxSerialExecuteData * sData = (mtxSerialExecuteData*)aData;
    mtxEntry             * sArgument[4] = { NULL, NULL, NULL, NULL };
    mtxEntry             * sEntry;
    mtxEntry             * sFense;
    mtxEntry             * sHeader;
    UShort                 sIndex = 1;

    IDE_DASSERT( ( sData->mTable  != NULL ) ||
                 ( sData->mEntry  != NULL ) );

    sData->mTable->row = (void*)aRow;
    sData->mTable->rid = aRid;
    sData->mTable->modify++;

    sEntry = sData->mEntry;
    sFense = sData->mEntry + sData->mEntryTotalSize;

    while ( sIndex <= sData->mEntrySetCount )
    {
        IDE_TEST_RAISE( sEntry + sEntry->mHeader.mSize > sFense,
                        INVALID_ENTRY_ERROR );

        MTX_INITAILZE_ENTRY_FLAG( sEntry );

        switch ( sEntry->mHeader.mType )
        {
            case MTX_ENTRY_TYPE_VALUE:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;

                break;

            case MTX_ENTRY_TYPE_RID:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = sEntry++;

                IDE_TEST( mtxRid.mCommon( sArgument )
                          != IDE_SUCCESS );

                break;

            case MTX_ENTRY_TYPE_COLUMN:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = sEntry++;
                sArgument[2] = sEntry++;

                IDE_TEST( mtxColumn.mCommon( sArgument )
                          != IDE_SUCCESS );

                break;

            case MTX_ENTRY_TYPE_FUNCTION:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;
                sArgument[2] = (mtxEntry *)(sEntry++)->mAddress;

                IDE_TEST( (sEntry++)->mExecute( sArgument )
                          != IDE_SUCCESS );

                break;

            case MTX_ENTRY_TYPE_SINGLE:
            case MTX_ENTRY_TYPE_CONVERT:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;

                IDE_TEST( (sEntry++)->mExecute( sArgument )
                          != IDE_SUCCESS );

                break;

            case MTX_ENTRY_TYPE_CONVERT_CHAR:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;
                sArgument[2] = sEntry++;

                IDE_TEST( (sEntry++)->mExecute( sArgument )
                          != IDE_SUCCESS );

                break;

            case MTX_ENTRY_TYPE_CONVERT_DATE:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;
                sArgument[2] = sEntry++;
                sArgument[3] = sEntry++;

                IDE_TEST( (sEntry++)->mExecute( sArgument )
                          != IDE_SUCCESS );

                break;

            case MTX_ENTRY_TYPE_CHECK:
                sHeader      = sEntry++;
                sArgument[3] = (mtxEntry *)(sEntry++)->mAddress;
                sArgument[0] = sArgument[3] + 1;

                if ( MTX_IS_NOT_CALCULATED( sArgument[3]->mHeader.mFlag ) )
                {
                    /* Nothing to do */
                }
                else
                {
                    sEntry = sArgument[3] + sArgument[3]->mHeader.mSize;
                    sIndex = sArgument[3]->mHeader.mId;
                }

                break;

            case MTX_ENTRY_TYPE_AND:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;
                sArgument[2] = (mtxEntry *)(sEntry++)->mAddress;
                sArgument[3] = sHeader;

                IDE_TEST( mtxAnd.mCommon( sArgument )
                          != IDE_SUCCESS );

                sEntry = sArgument[3] + sArgument[3]->mHeader.mSize;
                sIndex = sArgument[3]->mHeader.mId;

                break;

            case MTX_ENTRY_TYPE_OR:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;
                sArgument[2] = (mtxEntry *)(sEntry++)->mAddress;
                sArgument[3] = sHeader;

                IDE_TEST( mtxOr.mCommon( sArgument )
                          != IDE_SUCCESS );

                sEntry = sArgument[3] + sArgument[3]->mHeader.mSize;
                sIndex = sArgument[3]->mHeader.mId;

                break;

            case MTX_ENTRY_TYPE_AND_SINGLE:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;

                IDE_TEST( mtxAnd.mCommon( sArgument )
                          != IDE_SUCCESS );

                break;

            case MTX_ENTRY_TYPE_OR_SINGLE:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;

                IDE_TEST( mtxOr.mCommon( sArgument )
                          != IDE_SUCCESS );

                break;

            case MTX_ENTRY_TYPE_NOT:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;

                IDE_TEST( mtxNot.mCommon( sArgument )
                          != IDE_SUCCESS );

                break;

            case MTX_ENTRY_TYPE_LNNVL:
                sHeader      = sEntry++;
                sArgument[0] = sEntry++;
                sArgument[1] = (mtxEntry *)(sEntry++)->mAddress;

                IDE_TEST( mtxLnnvl.mCommon( sArgument )
                          != IDE_SUCCESS );

                break;

            default:
                IDE_RAISE( INVALID_ENTRY_TYPE );

                break;
        }

        MTX_SET_ENTRY_FLAG( sHeader, MTX_ENTRY_FLAG_CALCULATE );

        sIndex++;
    }

    IDE_TEST_RAISE( sArgument[0] == NULL,
                    INVALID_ENTRY_ARRAY );

    if ( *(mtdBooleanType *)sArgument[0]->mAddress == MTD_BOOLEAN_TRUE )
    {
        *aResult = ID_TRUE;
    }
    else
    {
        *aResult = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_ENTRY_ARRAY )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtx::doSerialExecute",
                                  "invalid entry array" ) );
    }
    IDE_EXCEPTION( INVALID_ENTRY_ERROR )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtx::doSerialExecute",
                                  "access an invalid entry " ) );
    }
    IDE_EXCEPTION( INVALID_ENTRY_TYPE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtx::doSerialExecute",
                                  "invalid entry type" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
