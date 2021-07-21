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
 * $Id: qsxAvl.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     PROJ-1075 AVL-Tree
 *
 **********************************************************************/

#ifndef _O_QSX_AVL_H_
#define _O_QSX_AVL_H_ 1

#include <qc.h>
#include <iduMemListOld.h>
#include <qsParseTree.h>
#include <qsxDef.h>
#include <qsxEnv.h>
#include <qmcCursor.h>

#define QSX_AVL_HEIGHT_LIMIT   (36)
#define QSX_AVL_EXTEND_COUNT   (32)
#define QSX_AVL_FREE_COUNT     (5)

typedef enum
{
    AVL_LEFT = 0,
    AVL_RIGHT = 1
} qsxAvlDirection;

typedef enum
{
    AVL_CURRENT = 0,
    AVL_NEXT = 1
} qsxAvlStartPos;

class qsxAvl
{
public:
    // avlTree의 초기화
    static void initAvlTree( qsxAvlTree    * aAvlTree,
                             mtcColumn     * aKeyColumn,
                             mtcColumn     * aDataColumn,
                             idvSQL        * aStatistics );

    // avlTree의 삭제
    static IDE_RC deleteAll( qsxAvlTree * aAvlTree );

    // key에 해당하는 노드 삭제
    static IDE_RC deleteKey( qsxAvlTree * aAvlTree,
                             mtcColumn  * aKeyCol,
                             void       * aKey,
                             idBool     * aDeleted );

    // key값에 해당하는 row가 존재하면 해당 row의 data를 update하고,
    // 존재하지 않는다면 새로 만든다.
    // 최종적으로 해당 row의 pointer를 반환한다.
    static IDE_RC insert( qsxAvlTree  * aAvlTree,
                          mtcColumn   * aKeyCol,
                          void        * aKey,
                          void       ** aRowPtr );
    
    // key값을 이용한 search
    static IDE_RC search( qsxAvlTree * aAvlTree,
                          mtcColumn  * aKeyCol,
                          void       * aKey,
                          void      ** aRowPtr,
                          idBool     * aFound );

    static IDE_RC searchMinMax( qsxAvlTree * aAvlTree,
                                SInt         aDir,
                                void      ** aRowPtr,
                                idBool     * aFound );


    // 해당 key의 next row를 가져옴
    static IDE_RC searchNext( qsxAvlTree * aAvlTree,
                              mtcColumn  * aKeyCol,
                              void       * aKey,
                              SInt         aDir,
                              SInt         aStart,
                              void      ** aRowPtr,
                              idBool     * aFound );

    //for deleteRange
    static IDE_RC searchNext4DR( qsxAvlTree * aAvlTree,
                                 mtcColumn  * aKeyCol,
                                 void       * aKey,
                                 SInt         aDir,
                                 SInt         aStart,
                                 void      ** aRowPtr,
                                 idBool     * aFound );

    // BUG-41311
    static IDE_RC searchNth( qsxAvlTree * aAvlTree,
                             UInt         aIndex,
                             void      ** aRowPtr,
                             idBool     * aFound );
    
    // aKeyMin <= node->key <= aKeyMax
    // 를 만족하는 모든 row를 삭제.
    static IDE_RC deleteRange( qsxAvlTree * aAvlTree,
                               mtcColumn  * aKeyMinCol,
                               void       * aKeyMin,
                               mtcColumn  * aKeyMaxCol,
                               void       * aKeyMax,
                               SInt       * aCount );

private:
    
    static IDE_RC _move( qsxAvlNode ** aPath,
                         SInt        * aTop,
                         SInt          aDir,
                         void       ** aRowPtr,
                         idBool      * aFound );

    //for deleteRange
    static IDE_RC _move4DR( qsxAvlNode ** aPath,
                            SInt        * aTop,
                            SInt          aDir,
                            void       ** aRowPtr,
                            idBool      * aFound,
                            SInt          aKeySize );
 
    static void _rotate_single( qsxAvlNode ** aNode,
                                SInt          aDir );

    static void _rotate_double( qsxAvlNode ** aNode,
                                SInt          aDir );

    static void _adjust_balance( qsxAvlNode ** aNode,
                                 SInt          aDir,
                                 SInt          aBalance );
    
    static void _insert_balance( qsxAvlNode ** aNode,
                                 SInt          aDir );

    static void _remove_balance( qsxAvlNode ** aNode,
                                 SInt          aDir,
                                 idBool      * aDone );

    static void _getRowSizeAndDataOffset( mtcColumn * aKeyColumn,
                                          mtcColumn * aDataColumn,
                                          SInt      * aRowSize,
                                          SInt      * aDataOffset );

    static IDE_RC _makeNode( qsxAvlTree  * aAvlTree,
                             mtcColumn   * aKeyCol,
                             void        * aKey,
                             qsxAvlNode ** aNode );

    static IDE_RC _deleteNode( qsxAvlTree  * aAvlTree,
                               qsxAvlNode  * aNode );

    static void _copyNodeRow( qsxAvlTree * aAvlTree,
                              qsxAvlNode * aDestNode,
                              qsxAvlNode * aSrcNode );
};

#endif /* _O_QSX_AVL_H_ */
