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
 * $Id$ sdlDataTypes.c
 **********************************************************************/

#include <idl.h>
#include <mtdTypeDef.h>
#include <sdlDataTypes.h>
#include <sdSql.h>

sdlDataTypeMap gSdlDataTypeTable[ULN_MTYPE_MAX] =
{
    { ULN_MTYPE_NULL,         MTD_NULL_ID,         SDL_TYPE( NULL )        , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //01
    { ULN_MTYPE_CHAR,         MTD_CHAR_ID,         SDL_TYPE( CHAR )        , SQL_C_CHAR       , SQL_CHAR        }, //02
    { ULN_MTYPE_VARCHAR,      MTD_VARCHAR_ID,      SDL_TYPE( VARCHAR )     , SQL_C_CHAR       , SQL_VARCHAR     }, //03
    { ULN_MTYPE_NUMBER,       MTD_NUMBER_ID,       SDL_TYPE( NUMBER )      , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //04
    { ULN_MTYPE_NUMERIC,      MTD_NUMERIC_ID,      SDL_TYPE( NUMERIC )     , SQL_C_CHAR       , SQL_NUMERIC     }, //05
    { ULN_MTYPE_BIT,          MTD_BIT_ID,          SDL_TYPE( BIT )         , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //06
    { ULN_MTYPE_SMALLINT,     MTD_SMALLINT_ID,     SDL_TYPE( SMALLINT )    , SQL_C_SHORT      , SQL_SMALLINT    }, //07
    { ULN_MTYPE_INTEGER,      MTD_INTEGER_ID,      SDL_TYPE( INTEGER )     , SQL_C_LONG       , SQL_INTEGER     }, //08
    { ULN_MTYPE_BIGINT,       MTD_BIGINT_ID,       SDL_TYPE( BIGINT )      , SQL_C_SBIGINT    , SQL_BIGINT      }, //09
    { ULN_MTYPE_REAL,         MTD_REAL_ID,         SDL_TYPE( REAL )        , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //10
    { ULN_MTYPE_FLOAT,        MTD_FLOAT_ID,        SDL_TYPE( FLOAT )       , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //11
    { ULN_MTYPE_DOUBLE,       MTD_DOUBLE_ID,       SDL_TYPE( DOUBLE )      , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //12
    { ULN_MTYPE_BINARY,       MTD_BINARY_ID,       SDL_TYPE( BINARY )      , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //13
    { ULN_MTYPE_VARBIT,       MTD_VARBIT_ID,       SDL_TYPE( VARBIT )      , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //14
    { ULN_MTYPE_NIBBLE,       MTD_NIBBLE_ID,       SDL_TYPE( NIBBLE )      , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //15
    { ULN_MTYPE_BYTE,         MTD_BYTE_ID,         SDL_TYPE( BYTE )        , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //16
    { ULN_MTYPE_VARBYTE,      MTD_VARBYTE_ID,      SDL_TYPE( VARBYTE )     , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //17
    { ULN_MTYPE_TIMESTAMP,    MTD_DATE_ID,         SDL_TYPE( TIMESTAMP )   , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //18
    { ULN_MTYPE_DATE,         MTD_DATE_ID,         SDL_TYPE( DATE )        , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //19
    { ULN_MTYPE_TIME,         MTD_DATE_ID,         SDL_TYPE( TIME )        , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //20
    { ULN_MTYPE_INTERVAL,     MTD_INTERVAL_ID,     SDL_TYPE( INTERVAL )    , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //21
    { ULN_MTYPE_BLOB,         MTD_BLOB_LOCATOR_ID, SDL_TYPE( BLOB )        , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //22
    { ULN_MTYPE_CLOB,         MTD_CLOB_LOCATOR_ID, SDL_TYPE( CLOB )        , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //23
    { ULN_MTYPE_BLOB_LOCATOR, MTD_BLOB_LOCATOR_ID, SDL_TYPE( BLOB_LOCATOR ), SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //24
    { ULN_MTYPE_CLOB_LOCATOR, MTD_CLOB_LOCATOR_ID, SDL_TYPE( CLOB_LOCATOR ), SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //25
    { ULN_MTYPE_GEOMETRY,     MTD_GEOMETRY_ID,     SDL_TYPE( GEOMETRY )    , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //26
    { ULN_MTYPE_NCHAR,        MTD_NCHAR_ID,        SDL_TYPE( NCHAR )       , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}, //27
    { ULN_MTYPE_NVARCHAR,     MTD_NVARCHAR_ID,     SDL_TYPE( NVARCHAR )    , SQL_UNKNOWN_TYPE , SQL_UNKNOWN_TYPE}  //28
};

UShort mtdType_to_SDLMType(UInt aMTDType)
{
    UShort sCnt  = 0;
    UShort sType = SDL_MTYPE_MAX;

    for ( sCnt = 0 ; sCnt < ULN_MTYPE_MAX ; sCnt++ )
    {
        if ( gSdlDataTypeTable[sCnt].mMTD_Type == aMTDType )
        {
            sType = gSdlDataTypeTable[sCnt].mSDL_MTYPE;
            break;
        }
    }

    return sType;
}

UInt mtype_to_MTDType(UShort aMType)
{
    UInt sType = MTD_UNDEF_ID;
    if ( aMType < ULN_MTYPE_MAX )
    {
        sType = gSdlDataTypeTable[aMType].mMTD_Type;
    }
    else
    {
        /* Do Nothing. */
    }
    return sType;
}

UInt sdlMType_to_MTDType(UShort aSdlMType)
{
    UShort sUlnType = SDL_MTYPE_MAX;

    ULN_TYPE( sUlnType, aSdlMType );

    return mtype_to_MTDType( sUlnType );
}

UShort mtdType_to_SQLCType(UInt aMTDType)
{
    UShort sCnt  = 0;
    UShort sType = SDL_MTYPE_MAX;
    
    for ( sCnt = 0 ; sCnt < ULN_MTYPE_MAX ; sCnt++ )
    {
        if ( gSdlDataTypeTable[sCnt].mMTD_Type == aMTDType )
        {
            sType = gSdlDataTypeTable[sCnt].mSQL_CType;
            break;
        }
    }
    
    return sType;
}

UShort mtdType_to_SQLType(UInt aMTDType)
{
    UShort sCnt  = 0;
    UShort sType = SDL_MTYPE_MAX;
    
    for ( sCnt = 0 ; sCnt < ULN_MTYPE_MAX ; sCnt++ )
    {
        if ( gSdlDataTypeTable[sCnt].mMTD_Type == aMTDType )
        {
            sType = gSdlDataTypeTable[sCnt].mSQL_Type;
            break;
        }
    }
    
    return sType;
}