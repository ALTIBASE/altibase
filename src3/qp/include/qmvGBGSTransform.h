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
 * $Id: qmvGBGSTransform.h 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#ifndef _Q_QMV_GBGS_TRANSFORM_H_
#define _Q_QMV_GBGS_TRANSFORM_H_ 1

#define QMV_GBGS_TRANSFORM_MAX_TARGET_COUNT 10

typedef struct qmvGBGSQuerySetList
{
    qmsQuerySet         * querySet;
    qmvGBGSQuerySetList * next;

} qmvGBGSQuerySetList;

class qmvGBGSTransform
{
    
public :
    
    static IDE_RC doTransform( qcStatement * aStatement,
                               qmsQuerySet * aQuerySet );

    static IDE_RC removeNullGroupElements( qmsSFWGH * aSFWGH );

    static IDE_RC makeNullOrPassNode( qcStatement      * aStatement,
                                      qtcNode          * aExpression,
                                      qmsConcatElement * aGroup,
                                      idBool             aMakePassNode );

private :
    
    static IDE_RC modifyFrom( qcStatement           * aStatement,
                              qmsQuerySet           * aQuerySet,
                              qmvGBGSQuerySetList   * aQuerySetList );

    static IDE_RC getPosForPartialQuerySet( qmsQuerySet * aQuerySet,
                                            SInt        * aStartOffset,
                                            SInt        * aSize );

    static IDE_RC getPosForOrderBy( qmsSortColumns * aOrderBy,
                                    SInt           * aStartOffset,
                                    SInt           * aSize );

    static IDE_RC modifyList( qcStatement        * aStatement,
                              qmsSFWGH           * aSFWGH,
                              qmsConcatElement  ** aPrev,
                              qmsConcatElement   * aElement,
                              qmsConcatElement   * aNext,
                              qmsGroupByType       aType );

    static IDE_RC modifyRollupCube( qcStatement       * aStatement,
                                    qmsSFWGH          * aSFWGH,
                                    qmsConcatElement ** aPrev,
                                    qmsConcatElement  * aElement,
                                    qmsConcatElement  * aNext );

    static IDE_RC modifyGroupingSets( qcStatement      * aStatement,
                                      qmsSFWGH         * aSFWGH,
                                      qmsConcatElement * aPrev,
                                      qmsConcatElement * aElement,
                                      qmsConcatElement * aNext,
                                      SInt               aNullCheck );

    static IDE_RC modifyOrgSFWGH( qcStatement * aStatement,
                                  qmsSFWGH    * aSFWGH );

    static IDE_RC modifyGroupBy( qcStatement         * aStatement,
                                 qmvGBGSQuerySetList * aQuerySetList );

    static IDE_RC makeInlineView( qcStatement * aStatement,
                                  qmsSFWGH    * aParentSFWGH,
                                  qmsQuerySet * aChildQuerySet );
 
    static IDE_RC unionQuerySets( qcStatement          * aStatement,
                                  qmvGBGSQuerySetList  * aQuerySetList,
                                  qmsQuerySet         ** aRet );

    static IDE_RC copyPartialQuerySet( qcStatement          * aStatement,
                                       qmsConcatElement     * aGroupingSets,
                                       qmsQuerySet          * aSrcQuerySet,
                                       qmvGBGSQuerySetList ** aDstQuerySetList );

    static IDE_RC searchGroupingSets( qmsQuerySet       * aQuerySet,
                                      qmsConcatElement ** aGroupingSets,
                                      qmsConcatElement ** aPrevGroup );

    static IDE_RC modifyOrderBy( qcStatement         * aStatement,
                                 qmvGBGSQuerySetList * aQuerySetList );

    static IDE_RC setNoMerge( qmsFrom * aFrom );

    static IDE_RC makeNullNode( qcStatement * aStatement,
                                qtcNode     * aExpression );

};

#endif /* _Q_QMV_GBGS_TRANSFORM_H_ */
