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
 * $Id: svnReq.h 73590 2015-11-25 07:53:19Z donghyun $
 **********************************************************************/


#ifndef _O_SVN_REQ_H_
#define _O_SVN_REQ_H_  1

#include <idl.h> /* for win32 porting */
#include <smiAPI.h>
#include <smxAPI.h>

class svnReqFunc
{
    public:

        /* smi */
        static IDE_RC beginTableStat( smcTableHeader   * aHeader,
                                      SFloat             aPercentage,
                                      idBool             aDynamicMode,
                                      void            ** aTableArgument )
        {
            return smiStatistics::beginTableStat( aHeader,
                                                  aPercentage,
                                                  aDynamicMode,
                                                  aTableArgument );
        };
        static IDE_RC analyzeRow4Stat( smcTableHeader * aHeader,
                                       void           * aTableArgument,
                                       void           * aTotalTableArg,
                                       UChar          * aRow )
        {
            return smiStatistics::analyzeRow4Stat( aHeader,
                                                   aTableArgument,
                                                   aTotalTableArg,
                                                   aRow );
        };
        static IDE_RC setTableStat( smcTableHeader * aHeader,
                                    void           * aTrans,
                                    void           * aTableArgument,
                                    smiAllStat     * aAllStats,
                                    idBool           aDynamicMode )
        {
            return smiStatistics::setTableStat( aHeader,
                                                aTrans,
                                                aTableArgument,
                                                aAllStats,
                                                aDynamicMode );
        };
        static IDE_RC endTableStat( smcTableHeader       * aHeader,
                                    void                 * aTableArgument,
                                    idBool                 aDynamicMode )
        {
            return smiStatistics::endTableStat( aHeader,
                                                aTableArgument,
                                                aDynamicMode );
        };
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };
};

#define smLayerCallback    svnReqFunc

#endif
