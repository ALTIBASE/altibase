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
 * $Id: mtcf.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_MTCF_H_
# define _O_MTCF_H_ 1

# include <mtccDef.h>
# include <mtcdTypes.h>
#include <mtclTerritory.h>

#define MTF_MEMPOOL_ELEMENT_CNT 4

ACP_EXTERN_C_BEGIN

    /*
    IDE_RC makeFormatInfo( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  UChar*       aFormat,
                                  UInt         aFormatLen,
                                  mtcCallBack* aCallBack );

    IDE_RC mtfTo_charCalculateNumberFor2Args( mtcNode*     aNode,
                                                mtcStack*    aStack,
                                                SInt         aRemain,
                                                void*        aInfo,
                                                mtcTemplate* aTemplate );

    IDE_RC mtfTo_charCalculateDateFor2Args( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate );
                                                   */

ACI_RC mtfToCharInterface_checkFormat( acp_uint8_t * aFmt,
                                       acp_uint32_t  aLength,
                                       acp_uint8_t * aToken );

ACI_RC mtfToCharInterface_mtfTo_char( mtcStack    * aStack,
                                      acp_uint8_t * aToken,
                                      mtlCurrency * aCurrency );

ACP_EXTERN_C_END

#endif /* _O_MTCF_H_ */
 
