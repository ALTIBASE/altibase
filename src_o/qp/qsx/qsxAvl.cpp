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
 * $Id: qsxAvl.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <qsxAvl.h>
#include <qsxUtil.h>
#include <qsxArray.h>

#define CMPDIR( sCmp ) ( ( sCmp < 0 ) ? AVL_RIGHT : AVL_LEFT )
#define REVERSEDIR( sDir ) ( ( sDir == AVL_LEFT ) ? AVL_RIGHT : AVL_LEFT )

/***********************************************************************
 * PUBLIC METHODS
 **********************************************************************/

void qsxAvl::initAvlTree( qsxAvlTree    * aAvlTree,
                          mtcColumn     * aKeyColumn,
                          mtcColumn     * aDataColumn,
                          idvSQL        * aStatistics )
{
/***********************************************************************
 *
 * Description : PROJ-1075 avl-tree의 초기화
 *
 * Implementation :
 *         (1) memory관리자를 초기화.
 *         (1.1) 할당할 chunk의 크기 및 dataoffset을 계산한다.
 *         (1.2) memory관리자를 chunk크기로 할당받도록 초기화
 *         (2) 컬럼 정보 및 timeout처리를 위한 정보를 연결
 *
 ***********************************************************************/

    SInt sRowSize;
    SInt sDataOffset;
    
    _getRowSizeAndDataOffset( aKeyColumn, aDataColumn, &sRowSize, &sDataOffset );

    aAvlTree->keyCol     = aKeyColumn;
    aAvlTree->dataCol    = aDataColumn;
    aAvlTree->dataOffset = sDataOffset;
    aAvlTree->rowSize    = sRowSize;
    aAvlTree->root       = NULL;
    aAvlTree->nodeCount  = 0;
    aAvlTree->refCount   = 1;
    aAvlTree->statistics = aStatistics;
    aAvlTree->compare    =
        aKeyColumn->module->keyCompare[MTD_COMPARE_MTDVAL_MTDVAL]
                                      [MTD_COMPARE_ASCENDING];
}


IDE_RC qsxAvl::deleteAll( qsxAvlTree * aAvlTree )
{
/***********************************************************************
 *
 * Description : PROJ-1075 avl-tree안의 모든 node 삭제.
 *
 * Implementation :
 *         (1) tree root 의 left부터 rotation을 하여
 *             left child node가 없는 root로 변형.
 *         (2) root의 child가 왼쪽에 없다면
 *             root를 지움.
 *
 *       10
 *     /    \
 *    8     12
 *     \   /  \
 *      9 11  13
 * 
 * deleteAll.
 * 
 * currNode : 10
 * saveNode : 8
 * 
 * rotate for empty-left-child root 8.
 * 
 *      8
 *       \
 *       10
 *      /  \
 *     9   12
 *        /  \
 *       11  13
 * 
 * currNode : 8
 * saveNode : 10
 * 
 * delete currNode 8.
 * 
 *       10
 *      /  \
 *     9   12
 *        /  \
 *       11  13
 * 
 * currNode : 10
 * saveNode : 9
 * 
 * rotate for empty-left-child root 9.
 * 
 *      9
 *       \
 *       10
 *         \
 *         12
 *        /  \
 *       11  13
 * 
 * currNode : 9
 * saveNode : 10
 * 
 * delete currNode 9.
 * 
 *       10
 *         \
 *         12
 *        /  \
 *       11  13
 * 
 * currNode : 10
 * 
 * 10 is empty-left-child root.
 * delete currNode 10.
 * 
 *         12
 *        /  \
 *       11  13
 * 
 * currNode : 12
 * saveNode : 11
 * 
 * rotate for empty-left-child root 11.
 * 
 *       11
 *         \
 *         12
 *           \
 *           13
 * 
 * currNode : 11
 * saveNode : 12
 * 
 * delete currNode 11.
 * 
 *         12
 *           \
 *           13
 * 
 * currNode : 12
 * 
 * 12 is empty-left-child root.
 * delete currNode 12.
 * 
 *           13
 * 
 * currNode : 13
 * 
 * 13 is empty-left-child root
 * delete currNode 13.
 * 
 * no data found. finished.
 *
 ***********************************************************************/

    qsxAvlNode * sCurrNode;
    qsxAvlNode * sSaveNode;

    sCurrNode = aAvlTree->root;

    while( sCurrNode != NULL )
    {
        if( sCurrNode->link[AVL_LEFT] == NULL )
        {
            sSaveNode = sCurrNode->link[AVL_RIGHT];

            IDE_TEST( _deleteNode( aAvlTree,
                                   sCurrNode )
                      != IDE_SUCCESS );
            aAvlTree->nodeCount--;
        }
        else
        {
            sSaveNode = sCurrNode->link[AVL_LEFT];
            sCurrNode->link[AVL_LEFT] = sSaveNode->link[AVL_RIGHT];
            sSaveNode->link[AVL_RIGHT] = sCurrNode;
        }

        sCurrNode = sSaveNode;
    }

    aAvlTree->root = NULL;
    
    IDE_DASSERT( aAvlTree->nodeCount == 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxAvl::search( qsxAvlTree * aAvlTree,
                       mtcColumn  * aKeyCol,
                       void       * aKey,
                       void      ** aRowPtr,
                       idBool     * aFound )
{
/***********************************************************************
 *
 * Description : PROJ-1075 key search
 *
 * Implementation :
 *         (1) root부터 시작.
 *         (2) 만약 key값을 compare해서 같으면 break하고 rowPtr을 세팅.
 *         (3) 만약 작거나 크면 link를 따라감.
 *             sCmp값이 0보다 작으면 ( left child ),
 *             sCmp값이 0보다 크면 ( right child )
 *
 ***********************************************************************/
#define IDE_FN "qsxAvl::search"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode  * sCurrNode = aAvlTree->root;
    SInt          sCmp;
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    *aFound = ID_FALSE;
        
    while ( sCurrNode != NULL )
    {
        IDE_TEST( iduCheckSessionEvent( aAvlTree->statistics )
                  != IDE_SUCCESS );

        sValueInfo1.column = aAvlTree->keyCol;
        sValueInfo1.value  = sCurrNode->row;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = aKeyCol;
        sValueInfo2.value  = aKey;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;
        
        sCmp = aAvlTree->compare( &sValueInfo1, &sValueInfo2 );
        
        if ( sCmp == 0 )
            break;
        
        sCurrNode = sCurrNode->link[CMPDIR(sCmp)];
    }

    if( sCurrNode == NULL )
    {
        *aRowPtr = NULL;
        *aFound = ID_FALSE;
    }
    else
    {
        *aRowPtr = sCurrNode->row;
        *aFound = ID_TRUE;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxAvl::insert( qsxAvlTree * aAvlTree,
                       mtcColumn  * aKeyCol,
                       void       * aKey,
                       void      ** aRowPtr )
{
/***********************************************************************
 *
 * Description : PROJ-1075 insert
 *
 * Implementation :
 *         (1) root의 parent부터 시작.
 *         (2) 해당 키값이 삽입될 위치를 검색
 *         (2.1) 검색 path를 따라가면서 balance 재조정이 필요한 node 검색
 *         (3) new node 를 insert
 *         (4) balance 값을 다시 세팅
 *         (5) balance가 2이상이면 rotation을 통해 balance를 재조정
 *         (6) balance 조정 이후 balancing node의 parent의
 *             balance 조정 이전 node가 있던 자리에 balance 조정 이후의 node연결
 *
 * Ex) 4 insert. (..) : balance
 * 
 *    5(-1)
 *   /    \
 * 2(1)   7(0)
 *    \
 *    3(0)
 * 
 * 
 * phase (1),(2)
 * parent of balancing node ->   5(-1)
 *                              /    \
 *         balancing node -> 2(1)   7(0)
 *                              \
 *                              3(0)
 *                                 \
 *                                  * insert position
 * 
 * phase (3)
 *    5(-1)
 *   /    \
 * 2(1)   7(0)
 *    \
 *    3(0)
 *       \
 *       4(0)
 * 
 * phase (4)  * : need rotation
 *    5(-1)                5(-1)
 *   /    \               /    \
 * 2(1+1) 7(0)     =>  *2(2)   7(0)
 *    \                    \
 *    3(0+1)               3(1)
 *       \                    \
 *       4(0)                 4(0)
 * 
 * phase (5) single-left rotation
 *     5(-1)                  5(-1)
 *    /    \                 /    \
 * *2(2)   7(0)    =>  savepos     7(0)
 *     \
 *     3(1)              3(0) <- new balanced node
 *        \              /  \
 *        4(0)        2(0) 4(0)
 * 
 * phase (6) link new balanced node
 *       5(-1)
 *      /    \
 *    3(0)   7(0)
 *    /  \
 *  2(0) 4(0)
 *
 ***********************************************************************/
#define IDE_FN "qsxAvl::insert"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode   sHeadNode;       // parent node of root.
    qsxAvlNode * sCurrBalNode;    // balancing node
    qsxAvlNode * sParentBalNode;  // balancing node의 parent
    qsxAvlNode * sCurrIterNode;   // for iteration
    qsxAvlNode * sNextIterNode;   // for iteration
    qsxAvlNode * sNewNode;        // for new node
    qsxAvlNode * sSavedBalNode;   // before balancing node
    SInt         sDir;            // node direction
    SInt         sCmp;            // compare result
    mtdValueInfo sValueInfo1;
    mtdValueInfo sValueInfo2;
    
    // 아무것도 없는 경우
    if ( aAvlTree->root == NULL )
    {
        // root에 그냥 붙임
        IDE_TEST( _makeNode( aAvlTree,
                             aKeyCol,
                             aKey,
                             &aAvlTree->root )
                  != IDE_SUCCESS );
        aAvlTree->nodeCount++;
        *aRowPtr = aAvlTree->root->row;
    }
    else
    {
        // (1)
        // root가 balance 조정 이후 바뀔 때를 고려하여
        // root의 parent부터 시작.
        sParentBalNode = &sHeadNode;
        sParentBalNode->link[AVL_RIGHT] = aAvlTree->root;

        // (2)
        // insert될 자리를 search하면서
        // balancing node, balancing node의 parent,
        // node가 삽입될 위치를 세팅한다.
        for ( sCurrIterNode = sParentBalNode->link[AVL_RIGHT],
                  sCurrBalNode = sCurrIterNode;
              ;
              sCurrIterNode = sNextIterNode )
        {
            IDE_TEST( iduCheckSessionEvent( aAvlTree->statistics )
                      != IDE_SUCCESS );

            sValueInfo1.column = aAvlTree->keyCol;
            sValueInfo1.value  = sCurrIterNode->row;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aAvlTree->keyCol;
            sValueInfo2.value  = aKey;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            sCmp = aAvlTree->compare( &sValueInfo1,
                                      &sValueInfo2 );
            
            IDE_TEST_RAISE( sCmp == 0, err_dup_key );
            
            sDir = CMPDIR( sCmp );
            sNextIterNode = sCurrIterNode->link[sDir];

            // 더이상 찾을 곳이 없다면 그자리가 삽입될 자리.
            // sCurrIterNode->link[sDir] 에 연결 되어야 함.
            if ( sNextIterNode == NULL )
                break;

            // (2.1)
            // 삽입될 node의 parent중
            // balancing node, balancing node의 parent저장
            if ( sNextIterNode->balance != 0 ) {
                sParentBalNode = sCurrIterNode;
                sCurrBalNode = sNextIterNode;
            }
        }

        // (3)
        // 삽입될 노드를 생성
        IDE_TEST( _makeNode( aAvlTree,
                             aKeyCol,
                             aKey,
                             &sNewNode )
                  != IDE_SUCCESS );

        // 삽입될 노드의 data를 output argument에 세팅
        *aRowPtr = sNewNode->row;

        // 원래 연결될 자리에 연결
        sCurrIterNode->link[sDir] = sNewNode;
        aAvlTree->nodeCount++;

        // (4)
        // balance값을 조정함
        // balance 조정 노드를 위에서 선정하였으므로
        // 거기서부터 시작하여 삽입된 노드까지 따라가면서
        // top-down방식으로 balance증감
        for ( sCurrIterNode = sCurrBalNode;
              sCurrIterNode != sNewNode;
              sCurrIterNode = sCurrIterNode->link[sDir] )
        {
            IDE_TEST( iduCheckSessionEvent( aAvlTree->statistics )
                      != IDE_SUCCESS );
            
            sValueInfo1.column = aAvlTree->keyCol;
            sValueInfo1.value  = sCurrIterNode->row;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aAvlTree->keyCol;
            sValueInfo2.value  = aKey;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            
            sCmp = aAvlTree->compare( &sValueInfo1,
                                      &sValueInfo2 );
            
            sDir = CMPDIR(sCmp);
            sCurrIterNode->balance += ( ( sDir == AVL_LEFT ) ? -1 : 1 );
        }

        // balance조정 전의 balancing node를 백업
        sSavedBalNode = sCurrBalNode;

        // (5)
        // balance가 1이상인 경우 재조정이 필요함
        // rotation을 통해 balance를 재조정
        if ( abs( sCurrBalNode->balance ) > 1 )
        {
            sValueInfo1.column = aAvlTree->keyCol;
            sValueInfo1.value  = sCurrBalNode->row;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aAvlTree->keyCol;
            sValueInfo2.value  = aKey;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            
            sCmp = aAvlTree->compare( &sValueInfo1,
                                      &sValueInfo2 );
            
            sDir = CMPDIR(sCmp);
            _insert_balance( &sCurrBalNode, sDir );
        }

        // (6)
        // balance조정 이후 balance 조정 노드의 parent노드에 다시 붙임
        if ( sSavedBalNode == sHeadNode.link[AVL_RIGHT] )
        {
            // balance 조정된 이후 root가 바뀐 경우
            aAvlTree->root = sCurrBalNode;
        }
        else
        {
            // basic case.
            // balance 조정 이전 노드가 어디 붙어있었는지 찾은 다음
            // balance 조정 이후의 노드를 그자리에 다시 붙임
            sDir = ( sSavedBalNode == sParentBalNode->link[AVL_RIGHT] ) ? AVL_RIGHT : AVL_LEFT;        
            sParentBalNode->link[sDir] = sCurrBalNode;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_dup_key);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_DUP_VAL_ON_INDEX));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsxAvl::deleteKey( qsxAvlTree * aAvlTree,
                          mtcColumn  * aKeyCol,
                          void       * aKey,
                          idBool     * aDeleted )
{
/***********************************************************************
 *
 * Description : PROJ-1075 delete
 *
 * Implementation :
 *         (1) root부터 시작.
 *         (2) 해당 키값이 삭제될 위치를 검색
 *         (2.1) 검색 path를 저장. path direction도 함께 저장
 *         (3) node 삭제
 *         (3.1) node의 child가 없거나 하나인 경우
 *               child를 지울 node의 parent에 연결하고 node삭제
 *         (3.2) node의 child가 둘다 있는 경우
 *               해당 node다음으로 큰 node를 찾아서
 *               데이터를 copy한 다음 연결관계 조정.
 *               해당 node를 지우는 것이 아니라 해당 node다음으로
 *               큰 node를 삭제함.(delete by copy algorithm)
 *         (4) balance재조정
 *             저장해 두었던 search path를 따라가면서
 *             balance조정. 만약 balance가 2이상이라면 rotation으로 재조정.
 *             +-1인경우 조정할 필요가 없음
 *             0인 경우 조정이 완료된 경우거나 아무것도 할 필요가 없는 경우임.
 *
 * Ex) 2 delete. (..) : balance
 * 
 * phase (1)
 * 
 * start=> 6(-1)
 *      /        \
 *    2(1)       8(0)
 *   /   \      /   \
 * 1(0) 4(1)  7(0)  9(0)
 *         \
 *         5(0)
 * 
 * 
 * phase (2)
 * 
 * path    : | 6 |   |
 * pathDir : | L |   |
 * top     :       ^
 * 
 *         6(-1)
 *       /       \
 *   *2(1)       8(0)
 *   /   \      /   \
 * 1(0) 4(1)  7(0)  9(0)
 *         \
 *         5(0)
 * * : current node
 * 
 * 
 * phase (3) -> (3.2) current have 2 childs
 * 
 * path    : | 6 | 2 |   |
 * pathDir : | L | R |   |
 * top     :           ^
 * 
 *         6(-1)
 *       /       \
 *   *2(1)       8(0)
 *   /   \      /   \
 * 1(0) #4(1) 7(0)  9(0)
 *         \
 *         5(0)
 * * : current node
 * # : heir node
 * 
 * 
 * current node := heir node (row copy)
 * 
 *         6(-1)
 *       /       \
 *   *4(1)       8(0)
 *   /   \      /   \
 * 1(0) #4(1) 7(0)  9(0)
 *         \
 *         5(0)
 * * : current node
 * # : heir node
 * 
 * 
 * link heir node->link[AVL_RIGHT]
 * 
 *         6(-1)
 *       /       \
 *   *4(1)       8(0)
 *   /   \      /   \
 * 1(0) +5(0) 7(0)  9(0)
 *         
 *      #4(1)  
 *         \
 *        +5(0)
 * * : current node
 * # : heir node
 * + : new linked node(heir node->link[AVL_RIGHT])
 * 
 * delete heir node.
 * 
 *         6(-1)
 *       /       \
 *   *4(1)       8(0)
 *   /   \      /   \
 * 1(0) +5(0) 7(0)  9(0)
 * 
 * 
 * phase (4) re-balance
 * 
 * path    : | 6 | 4 |   |
 * pathDir : | L | R |   |
 * top     :       ^      if pathDir[top] is R then balance - 1
 * 
 *         6(-1)
 *       /       \
 *   *4(1-1)     8(0)
 *   /   \      /   \
 * 1(0) +5(0) 7(0)  9(0)
 * 
 * path    : | 6 | 4 |   |
 * pathDir : | L | R |   |
 * top     :   ^          if pathDir[top] is L then balance + 1
 * 
 *        6(-1+1)
 *       /       \
 *   *4(1-1)     8(0)
 *   /   \      /   \
 * 1(0) +5(0) 7(0)  9(0)
 * 
 * finish! 
 * 
 *         6(0)
 *      /       \
 *   *4(0)      8(0)
 *   /   \      /   \
 * 1(0) +5(0) 7(0)  9(0)
 * 
 ***********************************************************************/
#define IDE_FN "qsxAvl::deleteKey"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sCurrNode;
    qsxAvlNode * sHeirNode;
    qsxAvlNode * sPath[QSX_AVL_HEIGHT_LIMIT];
    
    SInt         sPathDir[QSX_AVL_HEIGHT_LIMIT];
    SInt         sCmp;
    SInt         sDir;
    SInt         sTop = 0;
    idBool       sDone = ID_FALSE;
    mtdValueInfo sValueInfo1;
    mtdValueInfo sValueInfo2;
      

    *aDeleted = ID_FALSE;
    
    if ( aAvlTree->root != NULL )
    {
        // (1) root부터 시작
        sCurrNode = aAvlTree->root;

        // (2)
        // delete될 node를 검색하면서
        // path를 저장.
        while(1)
        {
            if ( sCurrNode == NULL )
            {
                IDE_CONT(key_not_found);
            }
            else
            {
                sValueInfo1.column = aAvlTree->keyCol;
                sValueInfo1.value  = sCurrNode->row;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = aKeyCol;
                sValueInfo2.value  = aKey;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;
        
                sCmp = aAvlTree->compare( &sValueInfo1,
                                          &sValueInfo2 );
                
                if ( sCmp == 0 )
                {
                    break;
                }
                else
                {
                    /* Push direction and node onto stack */
                    sPathDir[sTop] = CMPDIR(sCmp);
                    sPath[sTop++] = sCurrNode;
                    
                    sCurrNode = sCurrNode->link[sPathDir[sTop - 1]];
                }
            }
        }

        // (3) node를 삭제.
        if ( sCurrNode->link[AVL_LEFT] == NULL || sCurrNode->link[AVL_RIGHT] == NULL )
        {
            // (3.1) child가 아예 없거나 하나가 있는 경우
            sDir = ( sCurrNode->link[AVL_LEFT] == NULL ) ? AVL_RIGHT : AVL_LEFT;

            // 삭제될 node의 child node를 삭제될 node의 parent에 연결
            if ( sTop != 0 )
            {
                // basic case
                sPath[sTop - 1]->link[sPathDir[sTop - 1]] = sCurrNode->link[sDir];
            }
            else
            {
                // root가 삭제된 경우
                aAvlTree->root = sCurrNode->link[sDir];
            }

            aAvlTree->nodeCount--;

            // 해당 node를 삭제
            IDE_TEST( _deleteNode( aAvlTree,
                                   sCurrNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // (3.2) child가 두개 붙어 있는 경우.
            /* Find the inorder successor */
            sHeirNode = sCurrNode->link[AVL_RIGHT];
      
            /* Save this path too */
            sPathDir[sTop] = 1;
            sPath[sTop++] = sCurrNode;

            while ( sHeirNode->link[AVL_LEFT] != NULL )
            {
                sPathDir[sTop] = AVL_LEFT;
                sPath[sTop++] = sHeirNode;
                sHeirNode = sHeirNode->link[AVL_LEFT];
            }

            // row copy
            _copyNodeRow( aAvlTree, sCurrNode, sHeirNode );
            
            // node 연결
            sPath[sTop - 1]->link[ ( ( sPath[sTop - 1] == sCurrNode ) ? AVL_RIGHT : AVL_LEFT ) ]  =
                sHeirNode->link[AVL_RIGHT];

            aAvlTree->nodeCount--;

            // 해당 node를 삭제
            IDE_TEST( _deleteNode( aAvlTree,
                                   sHeirNode )
                      != IDE_SUCCESS );
        }

        // (4) balance 조정.
        while ( ( --sTop >= 0 ) &&
                ( sDone == ID_FALSE ) )
        {
            sPath[sTop]->balance += ( ( sPathDir[sTop] != AVL_LEFT ) ? -1 : 1 );

            // balance값이 -1또는 +1 이면 그냥 종료.
            // balance가 2이상이면 rotation을 통해 balance 조정
            if ( abs( sPath[sTop]->balance ) == 1 )
            {    
                break;
            }
            else if ( abs ( sPath[sTop]->balance ) > 1 )
            {
                _remove_balance( &sPath[sTop], sPathDir[sTop], &sDone );
                
                // parent조정
                if ( sTop != 0 )
                {
                    // basic case
                    sPath[sTop - 1]->link[sPathDir[sTop - 1]] = sPath[sTop];
                }
                else
                {
                    // root인 경우
                    aAvlTree->root = sPath[0];
                }
            }
            else
            {
                // Nothing to do.
                // balance가 0인 경우는 아무런 조정이 없었거나,
                // 조정후 0이 된 경우임.
            }
        }

        *aDeleted = ID_TRUE;
    }
    else
    {
        IDE_CONT(key_not_found);
    }

    IDE_EXCEPTION_CONT(key_not_found);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxAvl::searchMinMax( qsxAvlTree * aAvlTree,
                             SInt         aDir,
                             void      ** aRowPtr,
                             idBool     * aFound )
{
/***********************************************************************
 *
 * Description : PROJ-1075 min, max 검색
 *
 * Implementation :
 *          (1) direction이 0이면 min, direction이 1이면 max
 *          (2) 한쪽 방향으로 node의 next가 null일 때까지 따라감
 *          (3) 만약 root가 없다면 에러.
 *
 ***********************************************************************/
#define IDE_FN "qsxAvl::searchMinMax"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sCurrNode;

    sCurrNode = aAvlTree->root;
    *aFound   = ID_FALSE;

    if( sCurrNode != NULL )
    {
        while( sCurrNode->link[aDir] != NULL )
        {
            IDE_TEST( iduCheckSessionEvent( aAvlTree->statistics )
                      != IDE_SUCCESS );
        
            sCurrNode = sCurrNode->link[aDir];
        }

        *aRowPtr = sCurrNode->row;
        *aFound  = ID_TRUE;
    }
    else
    {
        *aRowPtr = NULL;
        *aFound  = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxAvl::searchNext( qsxAvlTree * aAvlTree,
                           mtcColumn  * aKeyCol,
                           void       * aKey,
                           SInt         aDir,
                           SInt         aStart,
                           void      ** aRowPtr,
                           idBool     * aFound )
{
/***********************************************************************
 *
 * Description : PROJ-1075 next, priv 검색
 *
 * Implementation :
 *          (1) 일반 search랑 같은 logic이지만 next, priv를 위해 stack을 사용
 *          (2) 해당 key를 찾으면 sFound를 TRUE로 세팅하고 멈춤.
 *          (3) 만약 current부터 시작이고 key를 찾았다면
 *              해당 위치의 row를 반환.
 *          (4) 만약 current부터 시작이 아니라면 다음 위치의 row반환.
 *
 ***********************************************************************/
#define IDE_FN "qsxAvl::searchNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sCurrNode;
    qsxAvlNode * sPath[QSX_AVL_HEIGHT_LIMIT];
    SInt         sCmp;
    SInt         sDir = 0;
    SInt         sTop = 0;
    mtdValueInfo sValueInfo1;
    mtdValueInfo sValueInfo2;
      
    
    *aFound = ID_FALSE;

    if ( aAvlTree->root != NULL )
    {
        // (1) root부터 시작
        sCurrNode = aAvlTree->root;

        while ( sCurrNode != NULL )
        {
            sPath[sTop++] = sCurrNode;
            
            IDE_TEST( iduCheckSessionEvent( aAvlTree->statistics )
                      != IDE_SUCCESS );

            sValueInfo1.column = aAvlTree->keyCol;
            sValueInfo1.value  = sCurrNode->row;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aKeyCol;
            sValueInfo2.value  = aKey;
            sValueInfo2.flag   = MTD_OFFSET_USELESS; 
            
            sCmp = aAvlTree->compare ( &sValueInfo1,
                                       &sValueInfo2 );
        
            if ( sCmp == 0 )
            {
                *aFound = ID_TRUE;
                break;
            }
            else
            {
                sDir = CMPDIR(sCmp);

                // sCurrNode->key < aKey 면 right로
                // sCurrNode->key > aKey 면 left로
                sCurrNode = sCurrNode->link[sDir];
            }
        }

        if( *aFound == ID_TRUE && aStart == AVL_CURRENT )
        {
            // key가 같은 node를 찾았고, 시작시점이 current이면
            // 찾은 node의 rowPtr을 넘겨주면 된다.
            *aRowPtr = sCurrNode->row;
        }
        else
        {
            if( *aFound == ID_FALSE && sDir != aDir )
            {
                // key가 같은 node를 못찾았고,
                // 이동할 방향과 마지막 search방향이 다르면
                // 마지막 path에 있는 node로 설정.
                *aRowPtr = sPath[--sTop]->row;
                *aFound = ID_TRUE;
            }
            else
            {
                // key가 같은 node를 찾았거나,
                // 이동할 방향과 마지막 search방향이 같다면
                // next로 이동.
                IDE_TEST( _move( sPath,
                                 &sTop,
                                 aDir,
                                 aRowPtr,
                                 aFound )
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        *aRowPtr = NULL;
        *aFound = ID_FALSE;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxAvl::searchNext4DR( qsxAvlTree * aAvlTree,
                              mtcColumn  * aKeyCol,
                              void       * aKey,
                              SInt         aDir,
                              SInt         aStart,
                              void      ** aRowPtr,
                              idBool     * aFound )
{
#define IDE_FN "qsxAvl::searchNext4DR"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sCurrNode;
    qsxAvlNode * sPath[QSX_AVL_HEIGHT_LIMIT];
    SInt         sCmp;
    SInt         sDir = 0;
    SInt         sTop = 0;
    mtdValueInfo sValueInfo1;
    mtdValueInfo sValueInfo2;
      
    
    *aFound = ID_FALSE;

    if ( aAvlTree->root != NULL )
    {
        // (1) root부터 시작
        sCurrNode = aAvlTree->root;

        while ( sCurrNode != NULL )
        {
            sPath[sTop++] = sCurrNode;
            
            IDE_TEST( iduCheckSessionEvent( aAvlTree->statistics )
                      != IDE_SUCCESS );

            sValueInfo1.column = aAvlTree->keyCol;
            sValueInfo1.value  = sCurrNode->row;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aKeyCol;
            sValueInfo2.value  = aKey;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
        
            sCmp = aAvlTree->compare ( &sValueInfo1,
                                       &sValueInfo2 );
        
            if ( sCmp == 0 )
            {
                *aFound = ID_TRUE;
                break;
            }
            else
            {
                sDir = CMPDIR(sCmp);

                // sCurrNode->key < aKey 면 right로
                // sCurrNode->key > aKey 면 left로
                sCurrNode = sCurrNode->link[sDir];
            }
        }

        if( *aFound == ID_TRUE && aStart == AVL_CURRENT )
        {
            // key가 같은 node를 찾았고, 시작시점이 current이면
            // 찾은 node의 rowPtr을 넘겨주면 된다.
            idlOS::memcpy( *aRowPtr,
                           sCurrNode->row,
                           aAvlTree->dataOffset );
        }
        else
        {
            if( *aFound == ID_FALSE && sDir != aDir )
            {
                // key가 같은 node를 못찾았고,
                // 이동할 방향과 마지막 search방향이 다르면
                // 마지막 path에 있는 node로 설정.
                idlOS::memcpy( *aRowPtr,
                               sPath[--sTop]->row,
                               aAvlTree->dataOffset );
                *aFound = ID_TRUE;
            }
            else
            {
                // key가 같은 node를 찾았거나,
                // 이동할 방향과 마지막 search방향이 같다면
                // next로 이동.
                IDE_TEST( _move4DR( sPath,
                                    &sTop,
                                    aDir,
                                    aRowPtr,
                                    aFound,
                                    aAvlTree->dataOffset )
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        *aFound = ID_FALSE;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxAvl::searchNth( qsxAvlTree * aAvlTree,
                          UInt         aIndex,
                          void      ** aRowPtr,
                          idBool     * aFound )
{
/***********************************************************************
 *
 * Description : BUG-41311 table function
 *
 * Implementation :
 *     - aIndex는 0부터 시작한다.
 *     - key의 순서대로 n번째 element를 찾는다.
 *
 ***********************************************************************/
#define IDE_FN "qsxAvl::searchNth"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sCurrNode;
    qsxAvlNode * sPath[QSX_AVL_HEIGHT_LIMIT];
    SInt         sTop = 0;
    void       * sRowPtr = NULL;
    idBool       sFound = ID_FALSE;
    UInt         i = 0;

    sCurrNode = aAvlTree->root;

    // inorder traverse
    while ( ( sTop > 0 ) || ( sCurrNode != NULL ) )
    {
        if ( sCurrNode != NULL )
        {
            sPath[sTop++] = sCurrNode;
            
            sCurrNode = sCurrNode->link[AVL_LEFT];
        }
        else
        {
            sCurrNode = sPath[--sTop];
            
            if ( i == aIndex )
            {
                sRowPtr = sCurrNode->row;
                sFound = ID_TRUE;
                break;
            }
            else
            {
                i++;
            }
            
            sCurrNode = sCurrNode->link[AVL_RIGHT];
        }
    }
    
    *aRowPtr = sRowPtr;
    *aFound = sFound;
    
    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qsxAvl::deleteRange( qsxAvlTree * aAvlTree,
                            mtcColumn  * aKeyMinCol,
                            void       * aKeyMin,
                            mtcColumn  * aKeyMaxCol,
                            void       * aKeyMax,
                            SInt       * aCount )
{
/***********************************************************************
 *
 * Description : PROJ-1075 next, priv 검색
 *
 * Implementation :
 *          (1) min, max에 근접하거나 같은(GE, LE) start, end key를 찾아놓음
 *          (2) min의 start부터 next로 따라가면서 하나씩 삭제
 *          (3) end key까지 왔으면 중단.
 *
 ***********************************************************************/
#define IDE_FN "qsxAvl::deleteRange"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    void * sStartKey = NULL;
    void * sEndKey   = NULL;
    void * sPrevKey  = NULL;
    void * sCurrKey  = NULL;
    idBool sDeleted;
    idBool sFound;
    SInt   sCmp;
    mtdValueInfo sValueInfo1;
    mtdValueInfo sValueInfo2;
      
  
    //fix BUG-18164 
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSN,
                                 aAvlTree->dataOffset,
                                 (void **)&sStartKey ) != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSN,
                                 aAvlTree->dataOffset,
                                 (void **)&sEndKey ) != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSN,
                                 aAvlTree->dataOffset,
                                 (void **)&sPrevKey ) != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSN,
                                 aAvlTree->dataOffset,
                                 (void **)&sCurrKey ) != IDE_SUCCESS );
 
    *aCount = 0;

    sValueInfo1.column = aKeyMinCol;
    sValueInfo1.value  = aKeyMin;
    sValueInfo1.flag   = MTD_OFFSET_USELESS;

    sValueInfo2.column = aKeyMaxCol;
    sValueInfo2.value  = aKeyMax;
    sValueInfo2.flag   = MTD_OFFSET_USELESS;

    sCmp = aAvlTree->compare ( &sValueInfo1,
                               &sValueInfo2 );
    
    IDE_TEST_CONT( sCmp > 0, invalid_range );
    
    // min의 start
    IDE_TEST( searchNext4DR( aAvlTree,
                             aKeyMinCol,
                             aKeyMin,
                             AVL_RIGHT,
                             AVL_CURRENT,
                             &sStartKey,
                             &sFound )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sFound == ID_FALSE, invalid_range );
    
    // max의 start
    IDE_TEST( searchNext4DR( aAvlTree,
                             aKeyMaxCol,
                             aKeyMax,
                             AVL_LEFT,
                             AVL_CURRENT,
                             &sEndKey,
                             &sFound )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sFound == ID_FALSE, invalid_range );
    
    //fix BUG-18164 
    idlOS::memcpy( sPrevKey,
                   sStartKey,
                   aAvlTree->dataOffset );

    while( 1 )
    {
        sValueInfo1.column = aAvlTree->keyCol;
        sValueInfo1.value  = sPrevKey;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = aAvlTree->keyCol;
        sValueInfo2.value  = sEndKey;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;
    
        sCmp = aAvlTree->compare ( &sValueInfo1,
                                   &sValueInfo2 );
        if( sCmp == 0 )
        {
            IDE_TEST( deleteKey( aAvlTree,
                                 aAvlTree->keyCol,
                                 sPrevKey,
                                 &sDeleted )
                      != IDE_SUCCESS );

            // key를 search해서 지우는 것이므로 반드시 지워져야 함.
            IDE_DASSERT( sDeleted == ID_TRUE );
            
            (*aCount)++;
            
            break;
        }
        else
        {
            IDE_TEST( searchNext4DR( aAvlTree,
                                     aAvlTree->keyCol,
                                     sPrevKey,
                                     AVL_RIGHT,
                                     AVL_NEXT,
                                     &sCurrKey,
                                     &sFound )
                      != IDE_SUCCESS );

            IDE_TEST( deleteKey( aAvlTree,
                                 aAvlTree->keyCol,
                                 sPrevKey,
                                 &sDeleted )
                      != IDE_SUCCESS );
            // key를 search해서 지우는 것이므로 반드시 지워져야 함.
            IDE_DASSERT( sDeleted == ID_TRUE );
            
            (*aCount)++;

            if( sFound == ID_FALSE )
            {
                break;
            }
            else
            {
                //fix BUG-18164 
                idlOS::memcpy( sPrevKey,
                               sCurrKey,
                               aAvlTree->dataOffset );
            }
        }
    }

    IDE_EXCEPTION_CONT( invalid_range );

    if( sStartKey != NULL )
    {
        IDE_TEST( iduMemMgr::free( sStartKey ) != IDE_SUCCESS );
        sStartKey = NULL;
    }

    if( sEndKey != NULL )
    {
        IDE_TEST( iduMemMgr::free( sEndKey ) != IDE_SUCCESS );
        sEndKey = NULL;
    }

    if( sPrevKey != NULL )
    {
        IDE_TEST( iduMemMgr::free( sPrevKey ) != IDE_SUCCESS );
        sPrevKey = NULL;
    }

    if( sCurrKey != NULL )
    {
        IDE_TEST( iduMemMgr::free( sCurrKey ) != IDE_SUCCESS );
        sCurrKey = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sStartKey != NULL )
    {
        (void)iduMemMgr::free( sStartKey );
        sStartKey = NULL;
    }

    if( sEndKey != NULL )
    {
        (void)iduMemMgr::free( sEndKey );
        sEndKey = NULL;
    }

    if( sPrevKey != NULL )
    {
        (void)iduMemMgr::free( sPrevKey );
        sPrevKey = NULL;
    }

    if( sCurrKey != NULL )
    {
        (void)iduMemMgr::free( sCurrKey );
        sCurrKey = NULL;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

/***********************************************************************
 * PRIVATE METHODS
 **********************************************************************/

IDE_RC qsxAvl::_move( qsxAvlNode ** aPath,
                      SInt        * aTop,
                      SInt          aDir,
                      void       ** aRowPtr,
                      idBool      * aFound )
{
/***********************************************************************
 *
 * Description : PROJ-1075 현재 위치에서 next, priv 이동
 *
 * Implementation :
 *          (1) 아래로 내려가는 경우는 해당 node의 direction에 node가 있는 경우
 *          (2) 위로 올라가는 경우는 해당 node의 direction이 NULL인 경우
 *
 *       10
 *     /    \
 *    8     12
 *     \   /  \
 *      9 11  13
 * 
 * case (1) next of 10, direction : AVL_RIGHT
 * 
 * path : | 10 |
 * top  :         ^
 * 
 * 10(--top)->link[1] is 12.
 * path : | 10 |
 * top  :    ^
 * 
 * find link[0] of 12. result is 11.
 * find link[0] of 11. result is null.
 * break.
 * 
 * next node is 11.
 *
 *
 * case (2) next of 9, direction : AVL_RIGHT
 * 
 * path : | 10 | 8 | 9 |
 * top  :            ^
 * 
 * 9 is last, 8(--top) is current.
 * last(9) is link[1] of 8.
 * 
 * path : | 10 | 8 | 9 |
 * top  :        ^
 * 
 * 
 * 8 is last, 10(--top) is current.
 * last(8) is link[0] of 10.
 * break.
 * 
 * path : | 10 | 8 | 9 |
 * top  :   ^
 * 
 * next node is 10.
 *
 ***********************************************************************/
#define IDE_FN "qsxAvl::_move"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sCurrNode;
    qsxAvlNode * sLastNode;
    
    sCurrNode = aPath[--(*aTop)]->link[aDir];
    *aFound   = ID_FALSE;
    
    if ( sCurrNode != NULL )
    {
        while ( sCurrNode->link[REVERSEDIR(aDir)] != NULL )
        {
            sCurrNode = sCurrNode->link[REVERSEDIR(aDir)];
            aPath[(*aTop)++] = sCurrNode;
        }
    }
    else
    {
        /* Move to the next branch */        
        do
        {
            if ( *aTop == 0 )
            {
                sCurrNode = NULL;
                break;
            }
            
            sLastNode = aPath[*aTop];

            sCurrNode = aPath[--(*aTop)];
        } while ( sLastNode == aPath[*aTop]->link[aDir] );
    }

    if( sCurrNode != NULL )
    {
        *aRowPtr = sCurrNode->row;
        *aFound  = ID_TRUE;
    }
    else
    {
        *aRowPtr = NULL;
        *aFound  = ID_FALSE;
    }
    
    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qsxAvl::_move4DR( qsxAvlNode ** aPath,
                         SInt        * aTop,
                         SInt          aDir,
                         void       ** aRowPtr,
                         idBool      * aFound,
                         SInt          aKeySize )
{
#define IDE_FN "qsxAvl::_move4DR"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sCurrNode;
    qsxAvlNode * sLastNode;
    
    sCurrNode = aPath[--(*aTop)]->link[aDir];
    *aFound   = ID_FALSE;
    
    if ( sCurrNode != NULL )
    {
        while ( sCurrNode->link[REVERSEDIR(aDir)] != NULL )
        {
            sCurrNode = sCurrNode->link[REVERSEDIR(aDir)];
            aPath[(*aTop)++] = sCurrNode;
        }
    }
    else
    {
        /* Move to the next branch */        
        do
        {
            if ( *aTop == 0 )
            {
                sCurrNode = NULL;
                break;
            }
            
            sLastNode = aPath[*aTop];

            sCurrNode = aPath[--(*aTop)];
        } while ( sLastNode == aPath[*aTop]->link[aDir] );
    }

    if( sCurrNode != NULL )
    {
        idlOS::memcpy( *aRowPtr,
                       sCurrNode->row,
                       aKeySize );
        *aFound  = ID_TRUE;
    }
    else
    {
        *aFound  = ID_FALSE;
    }
    
    return IDE_SUCCESS;

#undef IDE_FN
}


void qsxAvl::_rotate_single( qsxAvlNode ** aNode, SInt aDir )
{
/***********************************************************************
 *
 * Description : PROJ-1075 node의 single rotate
 *
 * Implementation :
 *          (1) 3개 노드 중 가운데 노드를 parent로 올라오게 회전.
 *          (2) 한쪽 방향으로 치우친 경우. L->L, R->R
 *
 * 1. left rotation. direction is 0.
 * 
 *   1
 *  / \             2
 * A   2     =>    / \
 *    / \         1   3
 *   B   3       / \
 *              A   B
 *
 * 2. right rotation. direction is 1.
 * 
 *     3
 *    / \           2
 *   2   B  =>     / \
 *  / \           1   3
 * 1   A             / \
 *                  A   B
 ***********************************************************************/
#define IDE_FN "qsxAvl::_rotate_single"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sSaveNode;

    SInt sReverseDir;

    sReverseDir = REVERSEDIR(aDir);
        
    sSaveNode = (*aNode)->link[sReverseDir];

    (*aNode)->link[sReverseDir] = sSaveNode->link[aDir];

    sSaveNode->link[aDir] = *aNode;

    *aNode = sSaveNode;

    return;

#undef IDE_FN
}

void qsxAvl::_rotate_double(qsxAvlNode ** aNode, SInt aDir )
{
/***********************************************************************
 *
 * Description : PROJ-1075 node의 double rotate
 *
 * Implementation :
 *          (1) 3개 노드 중 가운데 노드를 parent로 올라오게 두번 회전.
 *          (2) 한쪽 방향으로 치우친 것이 아니라 L->R, R->L의 형태.
 *
 * 1. right-left rotation
 *
 *   1                 1
 *  / \               / \                   2
 * A   3       =>    A   2       =>       /   \
 *    / \               / \              1     3
 *   2   D             B   3            / \   / \
 *  / \                   / \          A   B C   D
 * B   C                 C   D
 *
 * 2. left-right rotation
 *
 *     3                 3
 *    / \               / \                 2
 *   1   D     =>      2   D       =>     /   \
 *  / \               / \                1     3
 * A   2             1   C              / \   / \
 *    / \           / \                A   B C   D
 *   B   C         A   B
 *
 ***********************************************************************/
#define IDE_FN "qsxAvl::_rotate_double"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sSaveNode;
    SInt sReverseDir;

    sReverseDir = REVERSEDIR(aDir);

    sSaveNode = (*aNode)->link[sReverseDir]->link[aDir];

    (*aNode)->link[sReverseDir]->link[aDir]
                        = sSaveNode->link[sReverseDir];

    sSaveNode->link[sReverseDir] = (*aNode)->link[sReverseDir];

    (*aNode)->link[sReverseDir] = sSaveNode;

    sSaveNode = (*aNode)->link[sReverseDir];

    (*aNode)->link[sReverseDir] = sSaveNode->link[aDir];

    sSaveNode->link[aDir] = *aNode;

    *aNode = sSaveNode;

    return;
    
#undef IDE_FN
}

void qsxAvl::_adjust_balance( qsxAvlNode ** aNode, SInt aDir, SInt sBalance )
{
/***********************************************************************
 *
 * Description : PROJ-1075 double rotation을 하기 전에 balance를 조정한다.
 *
 * Implementation :
 *
 ***********************************************************************/
#define IDE_FN "qsxAvl::_adjust_balance"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sNode;
    qsxAvlNode * sChildNode;
    qsxAvlNode * sGrandChildNode;

    sNode = *aNode;

    sChildNode = sNode->link[aDir];
    sGrandChildNode = sChildNode->link[REVERSEDIR(aDir)];

    if( sGrandChildNode->balance == 0 )
    {
        sNode->balance = 0;
        sChildNode->balance = 0;
    }
    else if ( sGrandChildNode->balance == sBalance )
    {
        sNode->balance = -sBalance;
        sChildNode->balance = 0;
    }
    else // if ( sGrandChildNode->balance == -sBalance )
    {
        sNode->balance = 0;
        sChildNode->balance = sBalance;
    }

    sGrandChildNode->balance = 0;

    return;

#undef IDE_FN
}

void qsxAvl::_insert_balance( qsxAvlNode ** aNode, SInt aDir )
{

#define IDE_FN "qsxAvl::_insert_balance"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sChildNode;
    SInt         sBalance;
    
    sChildNode = (*aNode)->link[aDir];
    sBalance = ( aDir == AVL_LEFT ) ? -1 : 1;
    
    if( sChildNode->balance == sBalance )
    {
        (*aNode)->balance = 0;
        sChildNode->balance = 0;
        _rotate_single( aNode, REVERSEDIR(aDir) );
    }
    else // if( sChildNode->balance == -sBalance )
    {
        _adjust_balance( aNode, aDir, sBalance );
        _rotate_double( aNode, REVERSEDIR(aDir) );
    }

    return;

#undef IDE_FN
}

void qsxAvl::_remove_balance( qsxAvlNode ** aNode, SInt aDir, idBool * aDone )
{

#define IDE_FN "qsxAvl::_remove_balance"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsxAvlNode * sChildNode;
    SInt         sBalance;

    sChildNode = (*aNode)->link[REVERSEDIR(aDir)];
    sBalance = ( aDir == AVL_LEFT ) ? -1 : 1;

    if( sChildNode->balance == -sBalance )
    {
        (*aNode)->balance = 0;
        sChildNode->balance = 0;
        _rotate_single( aNode, aDir );
    }
    else if( sChildNode->balance == sBalance )
    {
        _adjust_balance( aNode, REVERSEDIR(aDir), -sBalance );
    }
    else // if( sChildNode->balance == 0 )
    {
        (*aNode)->balance = -sBalance;
        sChildNode->balance = sBalance;
        _rotate_single( aNode, aDir );
        *aDone = ID_TRUE;
    }
    
    return;

#undef IDE_FN
}

void qsxAvl::_getRowSizeAndDataOffset( mtcColumn * aKeyColumn,
                                       mtcColumn * aDataColumn,
                                       SInt      * aRowSize,
                                       SInt      * aDataOffset )
{

#define IDE_FN "qsxAvl::getRowSizeAndDataOffset"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    *aDataOffset = idlOS::align8(aKeyColumn->column.size);

    *aRowSize = *aDataOffset + aDataColumn->column.size;

    return;

#undef IDE_FN
}

IDE_RC qsxAvl::_makeNode( qsxAvlTree  * aAvlTree,
                          mtcColumn   * aKeyCol,
                          void        * aKey,
                          qsxAvlNode ** aNode )
{

#define IDE_FN "qsxAvl::_makeNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    void * sCanonizedValue;
    UInt   sActualSize;

    // 적합성 검사. key는 null이 올 수 없음.
    IDE_DASSERT( aKey != NULL );

    // 적합성 검사. key의 module은 반드시 동일해야 함.
    IDE_DASSERT( aAvlTree->keyCol->module == aKeyCol->module );

    // varchar, integer뿐이 올 수 없으므로 canonize할때 alloc받진 않는다.
    // 추후 다른 type을 지원하게 되면 바꾸어야 함.
    if ( ( aAvlTree->keyCol->module->flag & MTD_CANON_MASK )
         == MTD_CANON_NEED )
    {
        IDE_TEST( aAvlTree->keyCol->module->canonize(
                      aAvlTree->keyCol,
                      &sCanonizedValue,           // canonized value
                      NULL,
                      aKeyCol,
                      aKey,                     // original value
                      NULL,
                      NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        sCanonizedValue = aKey;
    }

    IDE_TEST( aAvlTree->memory->alloc( (void**)aNode )
              != IDE_SUCCESS );
    
    sActualSize = aAvlTree->keyCol->module->actualSize(
        aKeyCol,
        sCanonizedValue );
    
    idlOS::memcpy( (*aNode)->row,
                   sCanonizedValue,
                   sActualSize );

    (*aNode)->balance = 0;
    (*aNode)->reserved = 0;
    (*aNode)->link[AVL_LEFT] = NULL;
    (*aNode)->link[AVL_RIGHT] = NULL;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxAvl::_deleteNode( qsxAvlTree  * aAvlTree,
                            qsxAvlNode  * aNode )
{

#define IDE_FN "qsxAvl::_deleteNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    void          * sDataPtr   = NULL;
    qsxArrayInfo ** sArrayInfo = NULL;

    sDataPtr = (SChar*)aNode->row + aAvlTree->dataOffset;

    // PROJ-1904 Extend UDT
    if ( aAvlTree->dataCol->type.dataTypeId == MTD_ASSOCIATIVE_ARRAY_ID )
    {
        sArrayInfo = (qsxArrayInfo**)sDataPtr;

        if ( *sArrayInfo != NULL )
        {
            IDE_TEST( qsxArray::finalizeArray( sArrayInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else if ( (aAvlTree->dataCol->type.dataTypeId == MTD_ROWTYPE_ID) ||
              (aAvlTree->dataCol->type.dataTypeId == MTD_RECORDTYPE_ID) )
    {
        IDE_TEST( qsxUtil::finalizeRecordVar( aAvlTree->dataCol,
                                              sDataPtr)
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( aAvlTree->memory->memfree( aNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

void qsxAvl::_copyNodeRow( qsxAvlTree * aAvlTree,
                           qsxAvlNode * aDestNode,
                           qsxAvlNode * aSrcNode )
{

#define IDE_FN "qsxAvl::_copyNodeRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    idlOS::memcpy( aDestNode->row, aSrcNode->row, aAvlTree->rowSize );
    
    return;

#undef IDE_FN
}
