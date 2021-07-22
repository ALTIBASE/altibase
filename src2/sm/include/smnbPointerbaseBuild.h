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
 * $Id: smnbPointerbaseBuild.h 14768 2006-01-03 00:57:49Z unclee $
 **********************************************************************/

#ifndef _O_SMNB_POINTER_BASE_BUILD_H_
# define _O_SMNB_POINTER_BASE_BUILD_H_ 1

# include <idtBaseThread.h>
# include <iduStackMgr.h>
# include <smnDef.h>
# include <smnbDef.h>
# include <smp.h>
# include <smnbModule.h>

class smnbPointerbaseBuild : public idtBaseThread
{
public:
    smnbPointerbaseBuild();
    virtual ~smnbPointerbaseBuild();

    /* Index Build Main */
    static IDE_RC buildIndex( void            * aTrans,
                              idvSQL          * aStatistics,
                              smcTableHeader  * aTable,
                              smnIndexHeader  * aIndex,
                              UInt              aThreadCnt,
                              idBool            aIsPersistent,
                              idBool            aIsNeedValidation,
                              smnGetPageFunc    aGetPageFunc,
                              smnGetRowFunc     aGetRowFunc,
                              SChar           * aNullPtr );

    /* 쓰레드 초기화 */
    IDE_RC initialize( void        * aTrans,
                       idvSQL      * aStatistics,
                       smnpJobItem * aJobInfo,
                       SChar       * aNullPtr );

    static IDE_RC removePrepareNodeList( smnIndexHeader    * aIndex );

    IDE_RC insertRowToLeaf( smnIndexHeader  * aIndex,
                            SChar           * aRow);
    

    IDE_RC prepareQuickSort( smcTableHeader * aTable,
                             smnIndexHeader * aIndex,
                             smnpJobItem    * aJobInfo );

    IDE_RC quickSort( void            * aTrans,
                      smnIndexHeader  * aIndex,
                      SChar           * aNullPtr,
                      iduStackMgr     * aStack,
                      smnpJobItem     * aJobInfo);

    IDE_RC extractRow( idvSQL           * aStatistics,
                       smcTableHeader   * aTable,
                       smnIndexHeader   * aIndex,
                       idBool             aIsNeedValidation,
                       UInt             * aRowCount,
                       smnGetPageFunc     aGetPageFunc,
                       smnGetRowFunc      aGetRowFunc );

     IDE_RC finalize( smnIndexHeader * aIndex /*,
                                                      smnpJobItem    * aJobInfo */);

    IDE_RC partition( void*             aTrans,
                      smnbHeader*       aIndexHeader,
                      smnbStatistic*    aIndexStat,
                      SChar*            aNull,
                      smnbLNode**       aArrNode,
                      idBool            aIsUnique,
                      smnbLNode*        aLeftNode,
                      UInt              aLeftPos,
                      smnbLNode*        aRightNode,
                      UInt              aRightPos,
                      smnbLNode**       aOutNode,
                      UInt*             aOutPos);

    IDE_RC makeTree( smnbHeader  * aIndexHeader,
                     smnbLNode   * aFstLNode,
                     smnbNode   ** sRootNode);
    
    IDE_RC duplicate( smnIndexHeader * aSrcIndex,
                      smnIndexHeader * aDestIndex);

    SChar * getPivot( smnbHeader      * aIndexHeader,
                      smnbStatistic   * aIndexStat,
                      smnbLNode      ** aArrNode,
                      smnbLNode       * aLeftNode,
                      UInt              aLeftPos,
                      smnbLNode       * aRightNode,
                      UInt              aRightPos );
 
    SChar * getPivot4Large( smnbHeader      * aIndexHeader,
                            smnbStatistic   * aIndexStat,
                            smnbLNode      ** aArrNode,
                            smnbLNode       * aLeftNode,
                            UInt              aLeftPos,
                            smnbLNode       * aRightNode,
                            UInt              aRightPos );

    IDE_RC destroy();       /* 쓰레드 해제 */

    void run();
 
  
public:
    idvSQL             * mStatistics;
    SChar              * mNullPtr;
    UInt                 mError;
    smnpJobItem        * mJobInfo;
    iduStackMgr          mStack;
    void               * mTrans;
};

#endif /* _O_SMNB_POINTER_BASE_BUILD_H_ */
