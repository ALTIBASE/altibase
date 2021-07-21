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
 * $Id: 
 **********************************************************************/

#ifndef _O_RPD_CONVERT_SQL_H_
#define _O_RPD_CONVERT_SQL_H_ 1

#include <rpdMeta.h>
#include <rpdQueue.h>

class rpdConvertSQL
{
public:
    static void        getColumnListClause( rpdMetaItem  * aMetaItemForSource,
                                            rpdMetaItem  * aMetaItemForTarget,
                                            UInt           aCIDCount,
                                            UInt         * aCIDArrayForSource,
                                            UInt         * aCIDArrayForTarget,
                                            smiValue     * aSmiValueArray,
                                            idBool         aNeedColumnName,
                                            idBool         aIsWhereClause,
                                            idBool         aNeedRPad,
                                            SChar        * aDelimeter,
                                            SChar        * aValueClause,
                                            UInt           aValueClauseLength,
                                            idBool       * aIsValid );

    static IDE_RC       getColumnClause( rpdColumn    * aColumnForSource,
                                         rpdColumn    * aColumnForTarget,
                                         smiValue     * aSmiValue,
                                         idBool         aNeedColumnName,
                                         idBool         aIsWhereClause,
                                         idBool         aNeedRPad,
                                         SChar        * aColumnValue,
                                         UInt           aColumnValueLength );

    static IDE_RC       getInsertSQL( rpdMetaItem * aMetaItemForSource,
                                      rpdMetaItem * aMetaItemForTarget,
                                      rpdXLog     * aXLog,
                                      SChar       * aQueryString,
                                      UInt          aQueryStringLength );

    static IDE_RC       getUpdateSQL( rpdMetaItem * aMetaItemForSource,
                                      rpdMetaItem * aMetaItemForTarget,
                                      rpdXLog     * aXLog,
                                      idBool        aNeedBeforeImage,
                                      SChar       * aQueryString,
                                      UInt          aQueryStringLength );

    static IDE_RC       isNullValue( mtcColumn           * aColumn,
                                     smiValue            * aSmiValue,
                                     idBool              * aIsNullValue );

private:
    static void         getColumnValueCondition( UInt        aTargetDataTypeID,
                                                 UInt        aSourceDataTypeID,
                                                 idBool    * aNeedSingleQuotation,
                                                 idBool    * aNeedRPad,
                                                 idBool    * aNeedRTrim,
                                                 idBool    * aNeedColumnType,
                                                 idBool    * aIsUTF16 );

    static void         getTimestampCondition( UInt        aColumnID,
                                               rpdXLog   * aXLog,
                                               rpdColumn * aColumnArray,
                                               SChar     * aCondition,
                                               UInt        aConditionLength );

    static void         addSingleQuotation( SChar    * aValue,
                                            UInt       aValueLength );

    static void         rTrim( idBool       aIsUTF16,
                               SChar      * aString );

    static IDE_RC       rPadForUTF16String( UInt       aColumnLength,
                                            SChar    * aString,
                                            UInt       aStringLength );

    static IDE_RC       convertNumericType( mtcColumn    * aColumn,
                                            smiValue     * aSmiValue,
                                            SChar        * aValue,
                                            UInt           aValueLength );

    static IDE_RC       convertCharType( mtcColumn   * aColumn,
                                         smiValue    * aSmiValue,
                                         SChar       * aValue,
                                         UInt          aValueLength );
    static IDE_RC       convertDateType( mtcColumn    * aColumn,
                                         smiValue     * aSmiValue,
                                         SChar        * aValue,
                                         UInt           aValueLength );

    static IDE_RC       convertBitType( smiValue     * aSmiValue,
                                        UChar        * aValue,
                                        UInt           aValueLength );

    static IDE_RC       convertNCharType( smiValue    * aSmiValue,
                                          UChar       * aResult,
                                          UInt          aResultLength );

    static IDE_RC       convertByteType( smiValue    * aSmiValue,
                                         SChar       * aValue,
                                         UInt          aValueLength );

    static IDE_RC       convertNibbleType( smiValue      * aSmiValue,
                                           SChar         * aValue,
                                           UInt            aValueLength );

    static IDE_RC       charSetConvert( const mtlModule  * aSrcCharSet,
                                        const mtlModule  * aDestCharSet,
                                        mtdNcharType     * aSource,
                                        UChar            * aResult,
                                        UInt             * aResultLen );

    static idBool       isSupportDataType( UInt   aDataTypeId );

    static IDE_RC       convertValue( mtcColumn   * aColumn,
                                      smiValue    * aSmiValue,
                                      SChar       * aValue,
                                      UInt          aValueLength );

    static IDE_RC       getColumnNameClause( rpdMetaItem  * aMetaItemForSource,
                                             rpdMetaItem  * aMetaItemForTarget,
                                             rpdXLog      * aXLog,
                                             SChar        * aColumnName,
                                             UInt           aColumnNameLength );

    static SChar        hexToString( UChar    aHex );

    inline static UInt  getColumnIndex( UInt    aColumnID )
    {
        return ( aColumnID &= SMI_COLUMN_ID_MASK );
    };
};
#endif

