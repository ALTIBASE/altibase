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
 * $Id: qdbComment.h 00000 2009-09-17 21:42:30Z junokun $
 **********************************************************************/
#ifndef  _O_QDB_COMMENT_H_
#define  _O_QDB_COMMENT_H_  1

class qdbComment
{
public:
    static IDE_RC validateCommentTable(qcStatement * aStatement);
    
    static IDE_RC validateCommentColumn(qcStatement * aStatement);
    
    static IDE_RC executeCommentTable(qcStatement * aStatement);
    
    static IDE_RC executeCommentColumn(qcStatement * aStatement);

    static IDE_RC updateCommentTable(  qcStatement  * aStatement,
                                       SChar        * aUserName,
                                       SChar        * aOldTableName,
                                       SChar        * aNewTableName);

    static IDE_RC updateCommentColumn( qcStatement  * aStatement,
                                       SChar        * aUserName,
                                       SChar        * aTableName,
                                       SChar        * aOldColumnName,
                                       SChar        * aNewColumnName);

    static IDE_RC deleteCommentTable(  qcStatement  * aStatement,
                                       SChar        * aUserName,
                                       SChar        * aTableName);

    static IDE_RC deleteCommentColumn( qcStatement  * aStatement,
                                       SChar        * aUserName,
                                       SChar        * aTableName,
                                       SChar        * aColumnName);

    static IDE_RC existCommentTable(  qcStatement   * aStatement,
                                      SChar         * aUserName,
                                      SChar         * aTableName,
                                      idBool        * aExist);

    static IDE_RC existCommentColumn( qcStatement   * aStatement,
                                      SChar         * aUserName,
                                      SChar         * aTableName,
                                      SChar         * aColumnName,
                                      idBool        * aExist);

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC copyCommentsMeta( qcStatement  * aStatement,
                                    qcmTableInfo * aSourceTableInfo,
                                    qcmTableInfo * aTargetTableInfo );

};

#endif /* _O_QDB_COMMENT_H_ */
