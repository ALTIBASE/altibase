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
 * $Id: qsxArray.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <qc.h>
#include <qsParseTree.h>
#include <qsxArray.h>
#include <qsxUtil.h>

IDE_RC qsxArray::initArray( qcStatement  *  aQcStmt,
                            qsxArrayInfo ** aArrayInfo,
                            qcmColumn    *  aColumn,
                            idvSQL       *  aStatistics )
{
/***********************************************************************
 *
 * Description : PROJ-1075 array 초기화
 *
 * Implementation :
 *          array 변수에 필요한 다음 정보들을 세팅한다.
 *          (1) avlTree에서 필요한 정보
 *              memory - iduMemListOld
 *              column - keyColumn, dataColumn
 *              statistics - 세션 이벤트 감지용
 *
 ***********************************************************************/    

    qsxArrayInfo * sArrayInfo = NULL;
    UInt           sStage = 0;

    IDE_DASSERT( *aArrayInfo == NULL );

    /*
     * PROJ-1904 Extend UDT
     * 1) aArrayInfo 할당
     * 2) avl tree 초기화
     * 3) memory manager 할당
     */
    IDE_TEST( qsxUtil::allocArrayInfo( aQcStmt,
                                       &sArrayInfo )
              != IDE_SUCCESS );
    sStage = 1;

    qsxAvl::initAvlTree( &sArrayInfo->avlTree,
                         aColumn->basicInfo,       // index column
                         aColumn->next->basicInfo, // data column
                         aStatistics );

    IDE_TEST( qsxUtil::allocArrayMemMgr( aQcStmt,
                                         sArrayInfo )
              != IDE_SUCCESS );
    sStage = 2;

    sArrayInfo->row = NULL;

    *aArrayInfo = sArrayInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 2:
            // Nothing to do.
            /* fall through */
        case 1:
            {
                (void)qsxArray::finalizeArray( &sArrayInfo );
            }
            /* fall through */
        default:
            break;
    }

    *aArrayInfo = NULL;

    return IDE_FAILURE;    
}

/***********************************************************************
 *
 * Description : PROJ-1094 Extend UDT
 *               Array type 변수를 정리 한다.
 *
 * Implementation : 
 *   * Array type 변수의 각 column의 type에 맞게 동작한다.
 *     (1) AA type        : truncate table & free qsxArrayInfo
 *     (2) Record type    : Nothing to do.
 *     (3) primitive type : Nothing to do.
 *
 ***********************************************************************/
IDE_RC qsxArray::finalizeArray( qsxArrayInfo ** aArrayInfo )
{
    qsxArrayInfo * sArrayInfo = NULL;
    UInt           sStage = 0;

    IDE_DASSERT( aArrayInfo != NULL );

    sArrayInfo = *aArrayInfo;

    if ( sArrayInfo != NULL )
    {
        sArrayInfo->avlTree.refCount--;

        if ( sArrayInfo->avlTree.refCount == 0 )
        {
            IDE_TEST( truncateArray( sArrayInfo )
                      != IDE_SUCCESS );
            sStage = 1;

            IDE_TEST( qsxUtil::freeArrayInfo( sArrayInfo )
                      != IDE_SUCCESS );
            sStage = 2;
        }
        else
        {
            IDE_DASSERT( sArrayInfo->avlTree.refCount > 0 );
        }
    }
    else
    {
        // Nothing to do.
    }

    *aArrayInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 2:
            /* fall through */
        case 1:
            (void)qsxUtil::freeArrayInfo( sArrayInfo );
            break;
        default:
            break;
    }

    *aArrayInfo = NULL;

    return IDE_FAILURE;
}

IDE_RC qsxArray::truncateArray( qsxArrayInfo * aArrayInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1075 array element들을 모두 삭제.
 *
 * Implementation :
 *          array elements는 모두 avlTree에서 관리하기 때문에
 *          avlTree의 node들을 모두 삭제.
 *
 ***********************************************************************/    
    IDE_TEST( qsxAvl::deleteAll( &aArrayInfo->avlTree )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxArray::searchKey( qcStatement  * aQcStmt,
                            qsxArrayInfo * aArrayInfo,
                            mtcColumn    * aKeyCol,
                            void         * aKey,
                            idBool         aInsert,
                            idBool       * aFound )
{
/***********************************************************************
 *
 * Description : PROJ-1075 key에 해당하는 값을 search.
 *
 * Implementation :
 *         (1) aInsert가 true인 경우
 *             search해서 없으면 insert
 *         (2) aInsert가 false인 경우
 *             search해서 없으면 error
 *
 ***********************************************************************/

    void      * sRowPtr  = NULL;
    void      * sDataPtr = NULL;
    mtcColumn * sColumn  = NULL;

    *aFound = ID_FALSE;

    IDE_TEST( qsxAvl::search( &aArrayInfo->avlTree,
                              aKeyCol,
                              aKey,
                              &sRowPtr,
                              aFound )
              != IDE_SUCCESS );

    if( *aFound == ID_FALSE )
    {
        if( aInsert == ID_TRUE )
        {
            IDE_TEST( qsxAvl::insert( &aArrayInfo->avlTree,
                                      aKeyCol,
                                      aKey,
                                      &sRowPtr )
                          != IDE_SUCCESS );
        
            *aFound = ID_TRUE;

            sDataPtr = (SChar*)sRowPtr + aArrayInfo->avlTree.dataOffset;
            aArrayInfo->row = sDataPtr;

            sColumn = aArrayInfo->avlTree.dataCol;

            // PROJ-1904 Extend UDT
            if ( sColumn->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) 
            {
                *(qsxArrayInfo**)sDataPtr = NULL;
                IDE_TEST( qsxArray::initArray( aQcStmt,
                                               (qsxArrayInfo**)sDataPtr,
                                               (qcmColumn*)(((qtcModule*)sColumn->module)->typeInfo->columns), 
                                               QC_STATISTICS( aQcStmt ) )
                          != IDE_SUCCESS );
            }
            else if ( (sColumn->module->id == MTD_ROWTYPE_ID) ||
                      (sColumn->module->id == MTD_RECORDTYPE_ID) )
            {
                IDE_TEST( qsxUtil::initRecordVar( aQcStmt,
                                                  sColumn,
                                                  sDataPtr,
                                                  ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                // fix BUG-32758
                sColumn->module->null( sColumn,
                                       aArrayInfo->row );
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        sDataPtr = (SChar*)sRowPtr + aArrayInfo->avlTree.dataOffset;
        aArrayInfo->row = sDataPtr;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsxArray::deleteOneElement( qsxArrayInfo * aArrayInfo,
                                   mtcColumn    * aKeyCol,
                                   void         * aKey,
                                   idBool       * aDeleted )
{
/***********************************************************************
 *
 * Description : PROJ-1075 element 한개를 삭제
 *
 * Implementation :
 *           qsxAvl::deleteKey함수 호출.
 *
 ***********************************************************************/    

    IDE_TEST( qsxAvl::deleteKey( &aArrayInfo->avlTree,
                                 aKeyCol,
                                 aKey,
                                 aDeleted )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxArray::deleteElementsRange( qsxArrayInfo * aArrayInfo,
                                      mtcColumn    * aKeyMinCol,
                                      void         * aKeyMin,
                                      mtcColumn    * aKeyMaxCol,
                                      void         * aKeyMax,
                                      SInt         * aCount )
{
/***********************************************************************
 *
 * Description : PROJ-1075 range에 속한 element를 삭제
 *
 * Implementation :
 *           qsxAvl::deleteRange함수 호출.
 *
 ***********************************************************************/

    IDE_TEST( qsxAvl::deleteRange( &aArrayInfo->avlTree,
                                   aKeyMinCol,
                                   aKeyMin,
                                   aKeyMaxCol,
                                   aKeyMax,
                                   aCount )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxArray::searchNext( qsxArrayInfo * aArrayInfo,
                             mtcColumn    * aKeyCol,
                             void         * aKey,
                             void        ** aNextKey,
                             idBool       * aFound )
{
/***********************************************************************
 *
 * Description : PROJ-1075 해당 key의 다음 key를 가져옴.
 *
 * Implementation :
 *           qsxAvl::searchNext 호출.
 *           next를 가져와야 하므로 AVL_RIGHT
 *           자기자신을 포함하지 않으므로 AVL_NEXT
 *
 ***********************************************************************/

    void * sRowPtr;
    
    IDE_TEST( qsxAvl::searchNext( &aArrayInfo->avlTree,
                                  aKeyCol,
                                  aKey,
                                  AVL_RIGHT,
                                  AVL_NEXT,
                                  &sRowPtr,
                                  aFound )
              != IDE_SUCCESS );

    if( *aFound == ID_TRUE )
    {    
        *aNextKey = sRowPtr;
    }
    else
    {
        *aNextKey = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxArray::searchPrior( qsxArrayInfo * aArrayInfo,
                              mtcColumn    * aKeyCol,
                              void         * aKey,
                              void        ** aPriorKey,
                              idBool       * aFound )
{
/***********************************************************************
 *
 * Description : PROJ-1075 해당 key의 이전 key를 가져옴.
 *
 * Implementation :
 *           qsxAvl::searchNext 호출.
 *           prior를 가져와야 하므로 AVL_LEFT
 *           자기자신을 포함하지 않으므로 AVL_NEXT
 *
 ***********************************************************************/

    void * sRowPtr;
    
    IDE_TEST( qsxAvl::searchNext( &aArrayInfo->avlTree,
                                  aKeyCol,
                                  aKey,
                                  AVL_LEFT,
                                  AVL_NEXT,
                                  &sRowPtr,
                                  aFound )
              != IDE_SUCCESS );

    if( *aFound == ID_TRUE )
    {
        *aPriorKey = sRowPtr;
    }
    else
    {
        *aPriorKey = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxArray::searchFirst( qsxArrayInfo * aArrayInfo,
                              void        ** aFirstKey,
                              idBool       * aFound )
{
/***********************************************************************
 *
 * Description : PROJ-1075 min key를 가져옴.
 *
 * Implementation :
 *          qsxAvl::searchMinMax함수 호출.
 *          min이므로 AVL_LEFT
 *
 ***********************************************************************/

    void * sRowPtr;
    
    IDE_TEST( qsxAvl::searchMinMax( &aArrayInfo->avlTree,
                                    AVL_LEFT,
                                    &sRowPtr,
                                    aFound )
              != IDE_SUCCESS );

    if( *aFound == ID_TRUE )
    {
        *aFirstKey = sRowPtr;
    }
    else
    {
        *aFirstKey = NULL;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxArray::searchLast( qsxArrayInfo * aArrayInfo,
                             void        ** aLastKey,
                             idBool       * aFound )
{
/***********************************************************************
 *
 * Description : PROJ-1075 max key를 가져옴.
 *
 * Implementation :
 *          qsxAvl::searchMinMax함수 호출.
 *          max이므로 AVL_RIGHT
 *
 ***********************************************************************/

    void * sRowPtr;
    
    IDE_TEST( qsxAvl::searchMinMax( &aArrayInfo->avlTree,
                                    AVL_RIGHT,
                                    &sRowPtr,
                                    aFound )
              != IDE_SUCCESS );

    if( *aFound == ID_TRUE )
    {
        *aLastKey = sRowPtr;
    }
    else
    {
        *aLastKey = NULL;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxArray::searchNth( qsxArrayInfo * aArrayInfo,
                            UInt           aIndex,
                            void        ** aKey,
                            void        ** aData,
                            idBool       * aFound )
{
/***********************************************************************
 *
 * Description : BUG-41311 table function
 *
 * Implementation :
 *
 ***********************************************************************/

    void  * sRowPtr;
    
    IDE_TEST( qsxAvl::searchNth( &aArrayInfo->avlTree,
                                 aIndex,
                                 &sRowPtr,
                                 aFound )
              != IDE_SUCCESS );

    if( *aFound == ID_TRUE )
    {
        *aKey  = sRowPtr;
        *aData = (SChar*)sRowPtr + aArrayInfo->avlTree.dataOffset;
    }
    else
    {
        *aKey  = NULL;
        *aData = NULL;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsxArray::exists( qsxArrayInfo * aArrayInfo,
                         mtcColumn    * aKeyCol,
                         void         * aKey,
                         idBool       * aExists )
{
/***********************************************************************
 *
 * Description : PROJ-1075 해당 key를 가진 element가 존재하는지 검사.
 *
 * Implementation :
 *
 ***********************************************************************/

    void * sRowPtr;
    
    IDE_TEST( qsxAvl::search( &aArrayInfo->avlTree,
                              aKeyCol,
                              aKey,
                              &sRowPtr,
                              aExists )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

SInt qsxArray::getElementsCount( qsxArrayInfo * aArrayInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1075 count를 가져옴.
 *
 * Implementation :
 *
 ***********************************************************************/

    return aArrayInfo->avlTree.nodeCount;
}

IDE_RC qsxArray::assign( mtcTemplate  * aDstTemplate,
                         qsxArrayInfo * aDstArrayInfo,
                         qsxArrayInfo * aSrcArrayInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1075 array assign
 *
 * Implementation :
 *        (1) aDstArrayInfo를 truncate한다.
 *        (2) aSrcArrayInfo에서 하나씩 가져와서 aDstArrayInfo에 삽입.
 *
 ***********************************************************************/

    void        * sRowPtr    = NULL;
    void        * sDstRowPtr = NULL;
    mtcColumn   * sKeyCol    = NULL;
    void        * sKeyPtr    = NULL;
    mtcColumn   * sDataCol   = NULL;
    void        * sDataPtr   = NULL;

    idBool        sFound;
    UInt          sActualSize = 0;
    qcmColumn   * sInternalColumn = NULL;
    qsxAvlTree  * sSrcTree = NULL;
    qsxAvlTree  * sDstTree = NULL;
    qcStatement * sQcStmt  = NULL;
    iduMemory   * sMemory  = NULL;
    
    sSrcTree = &aSrcArrayInfo->avlTree;
    sDstTree = &aDstArrayInfo->avlTree;

    // 동일한 tree면 copy 하지 않는다.
    IDE_TEST_CONT( sSrcTree == sDstTree, SKIP_ASSIGN );

    IDE_TEST( truncateArray( aDstArrayInfo )
              != IDE_SUCCESS );

    sKeyCol  = sSrcTree->keyCol;
    sDataCol = sSrcTree->dataCol;

    IDE_DASSERT( sKeyCol  != NULL );
    IDE_DASSERT( sDataCol != NULL );
    
    IDE_TEST( qsxAvl::searchMinMax( &aSrcArrayInfo->avlTree,
                                    AVL_LEFT,
                                    &sRowPtr,
                                    &sFound )
              != IDE_SUCCESS );

    IDE_DASSERT( sDstTree->dataCol->module == sDataCol->module );

    if ( sDataCol->module->id == MTD_ASSOCIATIVE_ARRAY_ID )
    {
        sQcStmt = ((qcTemplate*)aDstTemplate)->stmt;
        sInternalColumn = (qcmColumn*)(((qtcModule*)sDataCol->module)->typeInfo->columns);
    }
    else if ( (sDataCol->module->id == MTD_ROWTYPE_ID) ||
              (sDataCol->module->id == MTD_RECORDTYPE_ID) )
    {
        sQcStmt = ((qcTemplate*)aDstTemplate)->stmt;
        sMemory = QC_QMX_MEM( sQcStmt );
    }
    else
    {
        // Nothing to do.
    }

    while( sFound != ID_FALSE )
    {
        sKeyPtr  = sRowPtr;
        sDataPtr = ((SChar*)sRowPtr + sSrcTree->dataOffset);

        IDE_TEST( qsxAvl::insert( sDstTree,
                                  sKeyCol,
                                  sKeyPtr,
                                  &sDstRowPtr )
                  != IDE_SUCCESS );

        sDstRowPtr = (SChar*)sDstRowPtr + sDstTree->dataOffset;

        if ( sDataCol->module->id == MTD_ASSOCIATIVE_ARRAY_ID )
        {
            *((qsxArrayInfo**)sDstRowPtr) = NULL;
            IDE_TEST( qsxArray::initArray( sQcStmt,
                                           (qsxArrayInfo**)sDstRowPtr,
                                           sInternalColumn,
                                           QC_STATISTICS( sQcStmt ) )
                      != IDE_SUCCESS );

            IDE_TEST( qsxArray::assign( aDstTemplate,
                                        *((qsxArrayInfo**)sDstRowPtr),
                                        *((qsxArrayInfo**)sDataPtr) )
                      != IDE_SUCCESS );
        }
        else if ( (sDataCol->module->id == MTD_ROWTYPE_ID) ||
                  (sDataCol->module->id == MTD_RECORDTYPE_ID) )
        {
            IDE_TEST( qsxUtil::initRecordVar(  sQcStmt,
                                               sDataCol,
                                               sDstRowPtr,
                                               ID_FALSE )
                      != IDE_SUCCESS );

            IDE_TEST( qsxUtil::assignRowValue( sMemory,
                                               sDataCol,
                                               sDataPtr,
                                               sDataCol,
                                               sDstRowPtr,
                                               aDstTemplate,
                                               ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            sActualSize = sDataCol->module->actualSize( sDataCol,
                                                        sDataPtr );
            idlOS::memcpy( sDstRowPtr, sDataPtr, sActualSize );
        }

        IDE_TEST( qsxAvl::searchNext( &aSrcArrayInfo->avlTree,
                                      sKeyCol,
                                      sKeyPtr,
                                      AVL_RIGHT,
                                      AVL_NEXT,
                                      &sRowPtr,
                                      &sFound )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_ASSIGN );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
