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
 * $Id: mtl.h 32306 2009-04-24 02:39:40Z mhjeong $
 **********************************************************************/

#ifndef _O_MTCL_H_
# define _O_MTCL_H_ 1

# include <acp.h>
# include <acl.h>
# include <aciTypes.h>
# include <mtcc.h>
# include <mtccDef.h>
# include <mtclCollate.h>

#define MTL_NAME_LEN_MAX          (20)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mtlU16Char
{
    acp_uint8_t value1;
    acp_uint8_t value2;
} mtlU16Char;

ACI_RC mtlInitialize( acp_char_t  * aDefaultNls, acp_bool_t aIsClient );

ACI_RC mtlFinalize( void );

ACI_RC mtlModuleByName( const mtlModule ** aModule,
                        const void       * aName,
                        acp_uint32_t       aLength );

ACI_RC mtlModuleById( const mtlModule ** aModule,
                      acp_uint32_t       aId );

const mtlModule* mtlDefaultModule( void );

void mtlMakeNameInFunc( const mtlModule * aMtlModule,
                        acp_char_t      * aDstName,
                        acp_char_t      * aSrcName,
                        acp_sint32_t      aSrcLen );

ACI_RC mtlMakeNameInSQL( const mtlModule * aMtlModule,
                         acp_char_t      * aDstName,
                         acp_char_t      * aSrcName,
                         acp_sint32_t      aSrcLen );

void mtlMakeQuotedName( acp_char_t   * aDstName,
                        acp_char_t   * aSrcName,
                        acp_sint32_t   aSrcLen );

acp_bool_t mtlIsQuotedName( acp_char_t   * aSrcName,
                            acp_sint32_t   aSrcLen );

// PROJ-1579 NCHAR
aciConvCharSetList mtlGetIdnCharSet( const mtlModule  * aCharSet );

void mtlGetUTF16UpperStr( mtlU16Char * aDest,
                          mtlU16Char * aSrc );

void mtlGetUTF16LowerStr( mtlU16Char * aDest,
                          mtlU16Char * aSrc );

acp_uint8_t mtlGetOneCharSize( acp_uint8_t     * aOneCharPtr,
                               acp_uint8_t     * aFence,
                               const mtlModule * aLanguage );

// BUG-45608 JDBC 4Byte char length
mtlNCRet mtlnextCharClobForClient( acp_uint8_t ** aSource,
                                   acp_uint8_t  * aFence );

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

#ifdef __cplusplus
}
#endif

#endif /* _O_MTCL_H_ */
