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
 * $Id: qmoCheckViewColumnRef.h 89448 2020-12-07 04:24:14Z cory.chae $
 *
 * Description :
 *     PROJ-2469 Optimize View Materialization
 *
 **********************************************************************/

#include <ide.h>
#include <qc.h>
#include <qmsParseTree.h>

#ifndef _O_QMO_CHECK_VIEW_COLUMN_REF_H_
#define _O_QMO_CHECK_VIEW_COLUMN_REF_H_ 1

class qmoCheckViewColumnRef
{    
public :

    static IDE_RC checkViewColumnRef( qcStatement      * aStatement,
                                      qmsColumnRefList * aParentColumnRef,
                                      idBool             aAllColumnUsed );

    /* TASK-7219 */
    static IDE_RC checkUselessViewTarget( qmsTarget        * aTarget,
                                          qmsColumnRefList * aParentColumnRef,
                                          qmsSortColumns   * aOrderBy );

private :    
    
    static IDE_RC checkQuerySet( qmsQuerySet      * sQuerySet,
                                 qmsColumnRefList * aParentColumnRef,
                                 qmsSortColumns   * aOrderBy,
                                 idBool             aAllColumnUsed );
    
    static IDE_RC checkFromTree( qmsFrom          * aFrom,
                                 qmsColumnRefList * aParentColumnRef,
                                 qmsSortColumns   * aOrderBy,
                                 idBool             aAllColumnUsed );

    static IDE_RC checkUselessViewColumnRef( qmsTableRef      * aTableRef,
                                             qmsColumnRefList * aParentColumnRef,
                                             qmsSortColumns   * aOrderBy );
    /* BUG-48090 */
    static IDE_RC checkWithViewFlagFromQuerySet( qmsQuerySet  * aQuerySet,
                                                 idBool       * aIsWithView );
    static IDE_RC checkWithViewFlagFromFromTree( qmsFrom      * aFrom,
                                                 idBool       * aIsWithView );
};
    
#endif /* _O_QMO_CHECK_VIEW_COLUMN_REF_H_ */
