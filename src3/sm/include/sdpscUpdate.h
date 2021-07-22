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
 * $Id: sdpscUpdate.h 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#ifndef _O_SDPSC_UPDATE_H_
#define _O_SDPSC_UPDATE_H_ 1

#include <sdpscDef.h>

class sdpscUpdate
{

public:

    static IDE_RC redo_SDPSC_INIT_SEGHDR( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx*         aMtx );

    static IDE_RC redo_SDPSC_INIT_EXTDIR( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx*         aMtx );

    static IDE_RC redo_SDPSC_ADD_EXTDESC_TO_EXTDIR( SChar       * aData,
                                                    UInt          aLength,
                                                    UChar       * aPagePtr,
                                                    sdrRedoInfo * /*aRedoInfo*/,
                                                    sdrMtx*         aMtx );

};

#endif // _O_SDPSC_UPDATE_H_
