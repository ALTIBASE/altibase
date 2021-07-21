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
 * $Id: mtl.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_MTL_H_
# define _O_MTL_H_ 1

# include <mtc.h>
# include <mtcDef.h>
# include <mtlCollate.h>

#define MTL_NAME_LEN_MAX          (20)

typedef struct mtlU16Char
{
    UChar value1;
    UChar value2;
} mtlU16Char;

class mtl {
private:
    static const mtlModule* modules[];
    static const mtlModule* modulesForClient[];

    static const mtlModule* defModule;

public:
    // PROJ-1579 NCHAR
    static const mtlModule* mDBCharSet;
    static const mtlModule* mNationalCharSet;

public:
    static IDE_RC initialize( SChar  * aDefaultNls, idBool aIsClient );

    static IDE_RC finalize( void );

    static const mtlModule* defaultModule( void );

    static IDE_RC moduleByName( const mtlModule** aModule,
                                const void*       aName,
                                UInt              aLength );

    static IDE_RC moduleById( const mtlModule** aModule,
                              UInt              aId );

    static void makeNameInFunc( SChar * aDstName,
                                SChar * aSrcName,
                                SInt    aSrcLen );

    static void makeNameInSQL( SChar * aDstName,
                               SChar * aSrcName,
                               SInt    aSrcLen );
    
    static void makePasswordInSQL( SChar * aDstName,
                                   SChar * aSrcName,
                                   SInt    aSrcLen );
    
    static void makeQuotedName( SChar * aDstName,
                                SChar * aSrcName,
                                SInt    aSrcLen );

    // PROJ-1579 NCHAR
    static idnCharSetList getIdnCharSet( const mtlModule * aCharSet );

    static void getUTF16UpperStr( mtlU16Char * aDest,
                                  mtlU16Char * aSrc );
    
    static void getUTF16LowerStr( mtlU16Char * aDest,
                                  mtlU16Char * aSrc );

    static UChar getOneCharSize( UChar           * aOneCharPtr,
                                 UChar           * aFence,
                                 const mtlModule * aLanguage );

    static UInt getSafeCutSize( UChar           *aPtr,
                                UInt             aLen,
                                UChar           *aFence,
                                const mtlModule *aModule,
                                idBool           aValidOnly = ID_FALSE );

    static mtlNCRet nextCharClobForClient( UChar ** aSource,
                                           UChar  * aFence );
};

typedef enum
{
    MTL_PC_IDX = 0, // '%'
    MTL_UB_IDX,     // '_'
    MTL_SP_IDX,     // ' '
    MTL_TB_IDX,     // '\t'
    MTL_NL_IDX,     // '\n'
    MTL_QT_IDX,     // '\''
    MTL_BS_IDX,     // '\\'
    MTL_MAX_IDX
} mtlSpecialCharType;

#endif /* _O_MTL_H_ */
