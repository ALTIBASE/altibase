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
 * $Id: qcmXA.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QCM_XA_H_
#define _O_QCM_XA_H_ 1

#include    <smiDef.h>
#include    <qc.h>
#include    <qcm.h>
#include    <qtc.h>

#define QCM_XID_FIELD_STRING_SIZE ( 128 )
#define QCM_XID_FIELD_STRING_SIZE_STR "128"


enum qcmXaStatusType
{
    QCM_XA_ROLLBACKED,
    QCM_XA_COMMITTED
};

typedef struct qcmXaHeuristicTrans
{
    vULong formatId;
    SChar  globalTxId      [QCM_XID_FIELD_STRING_SIZE + 1];
    SChar  branchQualifier [QCM_XID_FIELD_STRING_SIZE + 1];
    SInt   status;             // qcms_xa_status_type
} qcmXaHeuristicTrans;

class qcmXA
{
public:
    static IDE_RC insert(
        smiStatement    * aSmiStmt,
        /* BUG-18981 */
        ID_XID          * aXid,
        SInt              aStatus );

    static IDE_RC remove(
        smiStatement    * aSmiStmt,
        ID_XID          * aXid,
        idBool          * aIsRemoved );


    static IDE_RC select(
        smiStatement            * aSmiStmt,
        ID_XID                  * aXid,
        idBool                  * aIsFound,
        qcmXaHeuristicTrans     & aQcmXaHeuristicTrans);

    static IDE_RC selectAll(
        smiStatement           * aSmiStmt,
        SInt*                    aNumHeuristicTrans,
        qcmXaHeuristicTrans    * aHeuristicTrans,
        SInt                     aMaxHeuristicTrans );

    static void XidToString(
        ID_XID          * aXid,
        vULong          * aFormatId,
        SChar           * aGlobalTxId,
        SChar           * aBranchQualifier );

    static void StringToXid(
        vULong            aFormatId,
        SChar           * aGlobalTxId,
        SChar           * aBranchQualifier,
        ID_XID          * aXid );

private :
    static IDE_RC insert(
        smiStatement         * aSmiStmt,
        qcmXaHeuristicTrans  & aQcmXaHeuristicTrans);

    static IDE_RC remove(
        smiStatement    * aSmiStmt,
        vULong            aFormatId,
        SChar           * aGlobalTxId,
        SChar           * aBranchQualifier,
        idBool          * aIsRemoved );

    static IDE_RC select(
        smiStatement         * aSmiStmt,
        vULong                 aFormatId,
        SChar                * aGlobalTxId,
        SChar                * aBranchQualifier,
        idBool               * aIsFound,
        qcmXaHeuristicTrans  & aQcmXaHeuristicTrans);

    static IDE_RC  setMember( idvSQL              * aStatistics,
                              const void          * aRow,
                              qcmXaHeuristicTrans * aXaHeuristicTrans );
};

#endif /* _O_QCM_XA_H_ */
