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
 * $Id: 
 **********************************************************************/
 
#ifndef _O_RPS_SQL_EXECUTOR_H_
#define _O_RPS_SQL_EXECUTOR_H_ 1

#include <idl.h>

#include <rpdQueue.h>
#include <rpdMeta.h>

#include <qci.h>
#include <smiStatement.h>

class rpsSQLExecutor
{
public:
    static IDE_RC executeSQL( smiStatement   * aSmiStatement,
                              rpdMetaItem    * aRemoteMetaItem,
                              rpdMetaItem    * aLocalMetaItem,
                              rpdXLog        * aXLog,
                              SChar          * aSQLBuffer,
                              UInt             aSQLBufferLength,
                              idBool           aCompareBeforeImage,
                              SLong          * sRowCount );

    static IDE_RC hardRebuild( idvSQL                  * aStatistics,
                               qciStatement            * aQciStatement,
                               smiStatement            * aParentSmiStatement,
                               smiStatement            * aSmiStatement,
                               idBool                  * aIsBegun,
                               qciSQLPlanCacheContext  * aPlanCacheContext,
                               SChar                   * aSQLBuffer,
                               UInt                      aSQLBufferLength,
                               UInt                      aStmtFlag );
private:
    static IDE_RC rebuild( qciStatement            * aQciStatement,
                           smiStatement            * aParentSmiStatement,
                           smiStatement            * aSmiStatement,
                           idBool                  * aIsBegun,
                           qciSQLPlanCacheContext  * aPlanCacheContext,
                           SChar                   * aSQLBuffer,
                           UInt                      aSQLBufferLength );

    static IDE_RC retry( qciStatement      * aQciStatement,
                         smiStatement      * aParentSmiStatement,
                         smiStatement      * aSmiStatement,
                         idBool            * aIsBegun );

    static IDE_RC reprepare( qciStatement              * aQciStatement,
                             smiStatement              * aSmiStatement,
                             qciSQLPlanCacheContext    * aPlanCacheContext,
                             SChar                     * aSQLBuffer,
                             UInt                        aSQLBufferLength );

    static IDE_RC prepare( qciStatement              * aQciStatement,
                           smiStatement              * aSmiStatement,
                           qciSQLPlanCacheContext    * aPlanCacheContext,
                           SChar                     * aSQLBuffer,
                           UInt                        aSQLBufferLength );

    static IDE_RC setBindParamInfo( qciStatement       * aQciStatement,
                                    mtcColumn          * aColumn,
                                    UInt                 aId,
                                    UInt                 aLength );

    static IDE_RC setBindParamInfoArray( qciStatement      * aQciStatement,
                                         rpdMetaItem       * aMetaItem,
                                         UInt              * aCIDArray,
                                         smiValue          * aValueArray,
                                         UInt                aColumnCount,
                                         rpdXLog           * aXLog,
                                         idBool              aUpdateWere,
                                         UInt                aStartId,
                                         UInt              * aEndId );

    static IDE_RC setBindParamDataArray( qciStatement       * aQciStatement,
                                         rpdMetaItem        * aRemoteMetaItem,
                                         rpdMetaItem        * aLocalMetaItem,
                                         UInt               * aCIDArray,
                                         smiValue           * aValueArray,
                                         UInt                 aColumnCount,
                                         rpdXLog            * aXLog,
                                         idBool               aUpdateWere,
                                         UInt                 aStartId,
                                         UInt               * aEndId );

    static IDE_RC addtionalBindParamInfoforUpdate( qciStatement     * aQciStatement,
                                                   rpdMetaItem      * aRemoteMetaItem,
                                                   rpdMetaItem      * aLocalMetaItem,
                                                   rpdXLog          * aXLog,
                                                   UInt               aId,
                                                   idBool             aCompareBeforeImage );

    static IDE_RC addtionalBindParamDataforUpdate( qciStatement     * aQciStatement,
                                                   rpdMetaItem      * aRemoteMetaItem,
                                                   rpdMetaItem      * aLocalMetaItem,
                                                   rpdXLog          * aXLog,
                                                   UInt               aId,
                                                   idBool             aCompareBeforeImage );

    static IDE_RC bind( qciStatement     * aQciStatement,
                        rpdMetaItem      * aRemoteMetaItem,
                        rpdMetaItem      * aLocalMetaItem,
                        rpdXLog          * aXLog,
                        idBool             aCompareBeforeImage );

    static IDE_RC execute( qciStatement            * aQciStatement,
                           smiStatement            * aParentSmiStatement,
                           qciSQLPlanCacheContext  * aPlanCacheContext,
                           SChar                   * aSQLBuffer,
                           UInt                      aSQLBufferLength,
                           SLong                   * aRowCount );
};

#endif /* _O_RPS_SQL_EXECUTOR_H_ */
