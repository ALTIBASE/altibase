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
 * $Id: qdtDrop.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef  _O_QDT_DROP_H_
#define  _O_QDT_DROP_H_  1

#include <iduMemory.h>
#include <qcmTableSpace.h>
#include <qc.h>
#include <qdParseTree.h>


class qdtDrop
{
public:
    // validation
    static IDE_RC validate( qcStatement * aStatement );

    // execution
    static IDE_RC execute( qcStatement * aStatement );

private:
    static IDE_RC executeDropTableInTBS( qcStatement          * aStatement,
                                         qcmTableInfoList     * aTableInfoList,
                                         qcmPartitionInfoList * aPartInfoList,
                                         qdIndexTableList     * aIndexTables);

    static IDE_RC executeDropIndexInTBS( qcStatement          * aStatement,
                                         qcmIndexInfoList     * aIndexInfoList,
                                         qcmPartitionInfoList * aPartInfoList);

    /* PROJ-2211 Materialized View */
    static IDE_RC executeDropMViewOfRelated(
                        qcStatement       * aStatement,
                        qcmTableInfoList  * aTableInfoList,
                        qcmTableInfoList ** aMViewOfRelatedInfoList );
};


#endif // _O_QDT_DROP_H_
