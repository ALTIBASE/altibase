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

#ifndef _O_MMD_MANAGER_H_
#define _O_MMD_MANAGER_H_ 1

#include <idl.h>
#include <qcmXA.h>
#include <mmcDef.h>
#include <mmdDef.h>

class mmdManager
{
private:
    static idBool mInitFlag;

public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    static void   checkXaTimeout();
    //fix BUG-27218 XA Load Heurisitc Transaction함수 내용을 명확히 해야 한다.
    static IDE_RC loadHeuristicTrans( idvSQL                        *aStatistics,   /* PROJ-2446 */ 
                                      mmdXaLoadHeuristicXidFlag      aLoadHeuristicXidFlag, 
                                      ID_XID                       **aHeuristicXids, 
                                      SInt                          *aHeuristicXidsCnt );
    /* BUG-18981 */
    static IDE_RC insertHeuristicTrans( idvSQL          *aStatistics,   /* PROJ-2446 */
                                        ID_XID          *aXid, 
                                        qcmXaStatusType  aStatus );
    static IDE_RC removeHeuristicTrans( idvSQL  *aStatistics,   /* PROJ-2446 */
                                        ID_XID  *aXid );

    static IDE_RC showTransactions();
    static IDE_RC rollbackTransaction(smTID aTid);
    static IDE_RC commitTransaction(smTID aTid);
};


#endif
