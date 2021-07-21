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
 

/*******************************************************************************
 * $Id: sdpDPathInfoMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 ******************************************************************************/

#ifndef _O_SDP_DPATH_INFO_MGR_H_
#define _O_SDP_DPATH_INFO_MGR_H_ 1

#include <smDef.h>
#include <sdpDef.h>

class sdpDPathInfoMgr
{
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static IDE_RC initDPathInfo( sdpDPathInfo *aDPathInfo );
    static IDE_RC destDPathInfo( sdpDPathInfo *aDPathInfo );

    static IDE_RC createDPathSegInfo( idvSQL              * aStatistics,
                                      void                * aTrans,
                                      smOID                 aTableOID,
                                      sdpDPathInfo        * aDPathInfo, 
                                      sdpDPathSegInfo    ** aDPathSegInfo);

    static IDE_RC destDPathSegInfo( sdpDPathInfo    * aDPathInfo,
                                    sdpDPathSegInfo * aDPathSegInfo,
                                    idBool            aMoveLastFlag );


    static IDE_RC destAllDPathSegInfo( sdpDPathInfo  * aDPathInfo );

    static IDE_RC mergeAllSegOfDPathInfo( idvSQL       * aStatistics,
                                          void         * aTrans,
                                          sdpDPathInfo * aDPathInfo );

    static sdpDPathSegInfo* findLastDPathSegInfo( sdpDPathInfo  * aDPathInfo,
                                                  smOID           aTableOID );

    static IDE_RC dumpDPathInfo( sdpDPathInfo *aDPathInfo );
    static IDE_RC dumpDPathSegInfo( sdpDPathSegInfo *aDPathSegInfo );

private:
    static iduMemPool   mSegInfoPool;
};

#endif /* _O_SDP_DPATH_INFO_MGR_H_ */
