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
#ifndef  _O_QDB_JOIN_H_
#define  _O_QDB_JOIN_H_  1

#include <qc.h>
#include <qdParseTree.h>

class qdbJoin
{
public:
    static IDE_RC validateJoinTable( qcStatement * aStatement );
    
    static IDE_RC executeJoinTable( qcStatement * aStatement );

private:
    static IDE_RC validateAndCompareTables( qcStatement   * aStatement );

    static IDE_RC createTablePartitionSpec( qcStatement   * aStatement,
                                            SChar         * aPartMinVal,
                                            SChar         * aPartMaxVal,
                                            SChar         * aNewPartName,
                                            UInt            aOldTableID,
                                            UInt            aNewTableID,
                                            UInt            aNewPartID );

    static IDE_RC createPartLobSpec( qcStatement   * aStatement,
                                     UInt            aColumnCount,
                                     qcmColumn     * aOldColumn,
                                     qcmColumn     * aNewColumn,
                                     UInt            aNewTableID,
                                     UInt            aNewPartID );
};

#endif
