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
 * $Id: sdptbFT.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 ***********************************************************************/

# ifndef _O_SDPTB_FT_H_
# define _O_SDPTB_FT_H_ 1

#include <sdptb.h>
#include <sdpst.h>

class sdptbFT {
public:
    static IDE_RC buildRecord4FreeExtOfTBS( void                * aHeader,
                                            void                * aDumpObj,
                                            iduFixedTableMemory * aMemory );

    static IDE_RC buildRecord4FreeExtOfGG( void                * aHeader,
                                           iduFixedTableMemory * aMemory,
                                           sdptbSpaceCache     * aCache,
                                           sdFileID              aFID );

    static IDE_RC buildRecord4FreeExtOfLG( void                * aHeader,
                                           iduFixedTableMemory * aMemory,
                                           sdptbSpaceCache     * aSpaceCache,
                                           sdptbGGHdr          * aGGPtr,
                                           scPageID              aLGHdrPID );
};

#endif // _O_SDPTB_FT_H_
