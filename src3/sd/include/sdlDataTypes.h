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
 * $Id$ sdlDataTypes.h
 **********************************************************************/

#ifndef _O_SDL_DATA_TYPES_H_
#define _O_SDL_DATA_TYPES_H_

typedef enum ulnMTypeID
{
    ULN_MTYPE_NULL = 0,     //00
    ULN_MTYPE_CHAR,         //01
    ULN_MTYPE_VARCHAR,      //02
    ULN_MTYPE_NUMBER,       //03
    ULN_MTYPE_NUMERIC,      //04
    ULN_MTYPE_BIT,          //05
    ULN_MTYPE_SMALLINT,     //06
    ULN_MTYPE_INTEGER,      //07
    ULN_MTYPE_BIGINT,       //08
    ULN_MTYPE_REAL,         //09
    ULN_MTYPE_FLOAT,        //10
    ULN_MTYPE_DOUBLE,       //11
    ULN_MTYPE_BINARY,       //12
    ULN_MTYPE_VARBIT,       //13
    ULN_MTYPE_NIBBLE,       //14
    ULN_MTYPE_BYTE,         //15
    ULN_MTYPE_VARBYTE,      //16
    ULN_MTYPE_TIMESTAMP,    //17
    ULN_MTYPE_DATE,         //18
    ULN_MTYPE_TIME,         //19
    ULN_MTYPE_INTERVAL,     //20
    ULN_MTYPE_BLOB,         //21
    ULN_MTYPE_CLOB,         //22
    ULN_MTYPE_BLOB_LOCATOR, //23
    ULN_MTYPE_CLOB_LOCATOR, //24
    ULN_MTYPE_GEOMETRY,     //25
    ULN_MTYPE_NCHAR,        //26
    ULN_MTYPE_NVARCHAR,     //27
    ULN_MTYPE_MAX           //28
} ulnMTypeID;

#define SDL_ULN_MTYPE_DIFF  30000
#define SDL_MTYPE_MAX       SDL_ULN_MTYPE_DIFF + ULN_MTYPE_MAX

#define SDL_TYPE( aType ) ( ULN_MTYPE_##aType + SDL_ULN_MTYPE_DIFF )
#define ULN_TYPE( aType, aSdlType ) {\
            if ( aSdlType >= SDL_TYPE( MAX ) )\
            {\
                aType = ULN_MTYPE_MAX;\
            }\
            else\
            {\
                aType = aSdlType - SDL_ULN_MTYPE_DIFF;\
            }\
        }

typedef struct sdlDataTypeMap
{
    ulnMTypeID mULN_MTYPE;
    UInt       mMTD_Type;
    UShort     mSDL_MTYPE;
    UShort     mSQL_CType;
    UShort     mSQL_Type;
}sdlDataTypeMap;

UInt   sdlMType_to_MTDType( UShort aSdlMType);
UInt   mtype_to_MTDType(    UShort aMType   );
UShort mtdType_to_SDLMType( UInt   aMTDType );
UShort mtdType_to_SQLCType( UInt aMTDType );
UShort mtdType_to_SQLType( UInt aMTDType );

#endif /* _O_SDL_DATA_TYPES_H_ */
