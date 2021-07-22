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
 * $Id$
 **********************************************************************/

#ifndef _O_SDN_MANAGER_H_
#define _O_SDN_MANAGER_H_ 1

#include <smc.h>
#include <smnDef.h>

class sdnManager
{
public:
    static IDE_RC lockRow( smiCursorProperties * aProperties,
                           void                * aTrans,
                           smSCN               * aViewSCN,
                           smSCN               * aInfiniteSCN,
                           scSpaceID             aSpaceID,
                           sdRID                 aRowRID,
                           idBool                aForbiddenToRetry );

    static IDE_RC makeFetchColumnList( smcTableHeader      * aTableHeader, 
                                       smiFetchColumnList ** aFetchColumnList,
                                       UInt                * aMaxRowSize );

    static IDE_RC destFetchColumnList( smiFetchColumnList * aFetchColumnList );
};

#endif /* _O_SDN_MANAGER_H_ */
