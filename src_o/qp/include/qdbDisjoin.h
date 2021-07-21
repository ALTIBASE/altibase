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
#ifndef  _O_QDB_DISJOIN_H_
#define  _O_QDB_DISJOIN_H_ 1

#include <qc.h>
#include <qdParseTree.h>

class qdbDisjoin
{
public:
    static IDE_RC validateDisjoinTable( qcStatement * aStatement );
    
    static IDE_RC executeDisjoinTable( qcStatement * aStatement );

    static IDE_RC copyConstraintSpec( qcStatement     * aStatement,
                                      UInt              aOldTableID,
                                      UInt              aNewTableID );

    static IDE_RC modifyColumnID( qcStatement     * aStatement,
                                  qcmColumn       * aColumn,
                                  UInt              aColCount,
                                  UInt              aNewColumnID,
                                  void            * aHandle );  /* table or partition */

private:
    static IDE_RC copyTableSpec( qcStatement     * aStatement,
                                 qdDisjoinTable  * aDisjoin,
                                 SChar           * aTBSName,
                                 UInt              aOldTableID );

    static IDE_RC copyColumnSpec( qcStatement     * aStatement,
                                  UInt              aUserID,
                                  UInt              aOldTableID,
                                  UInt              aNewTableID,
                                  UInt              aOldPartID,
                                  UInt              aOldColumnID,
                                  UInt              aNewColumnID,
                                  UInt              aFlag );

    static IDE_RC checkPartitionExistByName( qcStatement          * aStatement,
                                             qcmPartitionInfoList * aPartInfoList,
                                             qdDisjoinTable       * aDisjoin );
};
#endif
