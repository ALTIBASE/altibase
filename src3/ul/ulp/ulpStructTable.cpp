/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ulpStructTable.h>


ulpStructTable::ulpStructTable()
{

}


ulpStructTable::~ulpStructTable()
{

}

void ulpStructTable::ulpInit()
{
    SInt sI;
    for (sI = 0; sI < MAX_SYMTABLE_ELEMENTS + 1; sI++)
    {
        mStructTable[sI] = NULL;
    }

    for (sI = 0; sI < MAX_SCOPE_DEPTH + 1; sI++)
    {
        mStructScopeT[sI] = NULL;
    }

    mCnt         = 0;
    mHash        = ulpHashFunc;
    mSize        = MAX_SYMTABLE_ELEMENTS;  // 마지막 bucket 제외
}

void ulpStructTable::ulpFinalize()
{
    SInt sI;
    ulpStructTNode *sNode;
    ulpStructTNode *sPNode;

    for (sI = 0; sI < MAX_SYMTABLE_ELEMENTS + 1; sI++)
    {
        sNode = mStructTable[sI];
        while (sNode != NULL)
        {
            sPNode  = sNode;
            sNode   = sNode->mNext;
            if ( sPNode -> mChild != NULL )
            {
                // delete field hash table
                sPNode->mChild->ulpFinalize();
                delete sPNode->mChild;
            }
            // free struct node
            idlOS::free(sPNode);
            sPNode = NULL;
        }
    }
}

// struct table에 새로운 tag를 저장한다.
ulpStructTNode *ulpStructTable::ulpStructAdd ( SChar *aTag, SInt aScope )
{
    SInt            sIndex;
    ulpStructTNode *sStNode;
    ulpStructTNode *sStNode2;

    // 이미 같은 struct tag가 같은 scope상에 존재한는지 확인한다.
    IDE_TEST_RAISE( ulpStructLookup( aTag, aScope ) != NULL,
                    ERR_STRUCT_AREADY_EXIST );

    // alloc struct node
    /* BUG-32413 APRE memory allocation failure should be fixed */
    sStNode = (ulpStructTNode *) idlOS::malloc( ID_SIZEOF(ulpStructTNode) );
    IDE_TEST_RAISE( sStNode == NULL, ERR_MEMORY_ALLOC );
    idlOS::memset(sStNode, 0, ID_SIZEOF(ulpStructTNode) );

    idlOS::strncpy( sStNode->mName, aTag, MAX_HOSTVAR_NAME_SIZE-1 );

    sStNode->mScope = aScope;

    sIndex = (*mHash)( (UChar *)sStNode->mName ) % ( mSize );

    // bucket list의 제일 앞에다 추가함.
    sStNode2 = mStructTable[sIndex];

    mStructTable[sIndex] = sStNode;

    // double linked-list
    sStNode->mPrev = NULL; // bucket-list의 첫node이기 때문.
    sStNode->mNext = sStNode2;
    if( sStNode2 != NULL )
    {
        sStNode2->mPrev = sStNode;
    }

    // for field hash table
    sStNode->mChild = new ulpSymTable;
    sStNode->mChild->ulpInit();
    // for del node
    sStNode->mIndex = sIndex;

    IDE_TEST_RAISE( sStNode->mChild == NULL, ERR_MEMORY_ALLOC );

    // add to struct scope linked list 제일 앞에다 추가함.
    sStNode2 = mStructScopeT[aScope];
    mStructScopeT[aScope] = sStNode;
    sStNode->mSLink = sStNode2;

    mCnt++;

    return sStNode;

    IDE_EXCEPTION ( ERR_STRUCT_AREADY_EXIST );
    {
    }
    IDE_EXCEPTION ( ERR_MEMORY_ALLOC );
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Memory_Alloc_Error );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        IDE_ASSERT(0);
    }
    IDE_EXCEPTION_END;

    return NULL;
}


// no tag struct node는 hash table 마지막 bucket에 추가된다.
ulpStructTNode *ulpStructTable::ulpNoTagStructAdd ( void )
{
    SInt            sIndex;
    ulpStructTNode *sStNode;
    ulpStructTNode *sStNode2;

    // alloc struct node
    /* BUG-32413 APRE memory allocation failure should be fixed */
    sStNode = (ulpStructTNode *) idlOS::malloc( ID_SIZEOF(ulpStructTNode) );
    IDE_TEST_RAISE( sStNode == NULL, ERR_MEMORY_ALLOC );
    idlOS::memset(sStNode, 0, ID_SIZEOF(ulpStructTNode) );


    sIndex = mSize;

    // bucket list의 제일 앞에다 추가함.
    sStNode2 = mStructTable[sIndex];

    mStructTable[sIndex] = sStNode;

    // double linked-list
    sStNode->mPrev = NULL; // bucket-list의 첫node이기 때문.
    sStNode->mNext = sStNode2;
    if( sStNode2 != NULL )
    {
        sStNode2->mPrev = sStNode;
    }

    // for field hash table
    sStNode->mChild = new ulpSymTable;
    sStNode->mChild->ulpInit();

    IDE_TEST_RAISE( sStNode->mChild == NULL, ERR_MEMORY_ALLOC );

    return sStNode;

    IDE_EXCEPTION ( ERR_MEMORY_ALLOC );
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Memory_Alloc_Error );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        IDE_ASSERT(0);
    }
    IDE_EXCEPTION_END;

    return NULL;
}

// 특정 이름을 갖는 tag를 struct table에서 scope를 줄여가면서 검색한다.
ulpStructTNode *ulpStructTable::ulpStructLookupAll( SChar *aTag, SInt aScope )
{
    SInt sIndex;
    ulpStructTNode *sStNode;

    sIndex  = (*mHash)( (UChar *)aTag ) % ( mSize );
    sStNode = mStructTable[sIndex];

    while ( sStNode != NULL )
    {
        if ( (!idlOS::strcmp( aTag, sStNode->mName ))
             && (sStNode->mScope <= aScope) ) // is OK? need more think.
        {
            break;
        }
        sStNode = sStNode->mNext;
    }

    return sStNode;
}


// 특정 이름을 갖는 tag를 특정 scope에서 존재하는지 struct table을 검색한다.
ulpStructTNode *ulpStructTable::ulpStructLookup( SChar *aTag, SInt aScope )
{
    SInt sIndex;
    ulpStructTNode *sStNode;

    sIndex  = (*mHash)( (UChar *)aTag ) % ( mSize );
    sStNode = mStructTable[sIndex];

    while ( sStNode != NULL )
    {
        if ( (!idlOS::strcmp( aTag, sStNode->mName ))
              && (sStNode->mScope == aScope) )
        {
            break;
        }
        sStNode = sStNode->mNext;
    }

    return sStNode;
}


// 같은 scope상의 모든 struct 정보를 struct table에서 제거한다.
void ulpStructTable::ulpStructDelScope( SInt aScope )
{
    ulpStructTNode *sStNode;
    ulpStructTNode *sNStNode;

    sStNode = mStructScopeT[aScope];

    while (  sStNode != NULL )
    {
        sNStNode = sStNode -> mSLink;
        // update double linked-list
        if( sStNode -> mPrev == NULL ) //bucket-list 처음 node일경우
        {
            mStructTable[sStNode->mIndex] = sStNode->mNext;
            if ( sStNode->mNext != NULL )
            {
                sStNode -> mNext -> mPrev = NULL; 
            }
        }
        else
        {
            sStNode -> mPrev -> mNext = sStNode -> mNext;
            if ( sStNode->mNext != NULL )
            {
                sStNode -> mNext -> mPrev = sStNode -> mPrev;
            }
        }

        if ( sStNode -> mChild )
        {
            sStNode -> mChild -> ulpFinalize();
            delete sStNode -> mChild;
            sStNode -> mChild = NULL;
        }
        idlOS::free( sStNode );
        mCnt--;

        sStNode  = sNStNode;
    }

    mStructScopeT[aScope] = NULL;
}


// print struct table for debug
void ulpStructTable::ulpPrintStructT()
{
    SInt   sI, sJ;
    SInt   sCnt;
    SInt   sMLineCnt;
    SInt   sIDcnt;
    idBool sIsIDEnd;    // id 길이가 15자를 넘을 경우 다음 라인에 출력하기 위해 사용됨.
    idBool sIsFirst;
    ulpStructTNode *sNode;

    /*BUG-28414*/
    sMLineCnt = 1;
    sIDcnt    = 0;
    sCnt = 1;
    sIsIDEnd   = ID_FALSE;
    sIsFirst   = ID_TRUE;
    const SChar sLineB[90] = // + 3 + 49 + 5 + 13 + 10 +
            "+---+-------------------------------------------------+-----+-------------+----------+\n";

    idlOS::printf( "\n\n[[ STRUCT_DEF TABLE (total num.:%d) ]]\n", mCnt );
    idlOS::printf( "%s", sLineB );
    idlOS::printf( "|%-3s|%-49s|%-5s|%-13s|          |\n",
                   "No.","STRUCT NAME","Scope","Num. of Attr." );

    for ( sI = 0 ; sI < MAX_SYMTABLE_ELEMENTS + 1; sI++)
    {
        sNode = mStructTable[sI];
        while (sNode != NULL)
        {
            if( sIsFirst == ID_TRUE )
            {
                sMLineCnt = 1;
                idlOS::printf( "%s", sLineB );
                idlOS::printf( "|%-3d|", sCnt++ );
            }
            else
            {
                idlOS::printf( "|   |" ); // 3 spaces
            }

            if( sIsIDEnd != ID_TRUE )
            {
                for( (sIsFirst == ID_TRUE )?sIDcnt = 0 : sIDcnt=sIDcnt
                     ; sIDcnt < 49 * sMLineCnt ; sIDcnt++ )
                {
                    if( sNode->mName[sIDcnt] == '\0' )
                    {
                        sIsIDEnd = ID_TRUE;
                        for( sJ = 49 * sMLineCnt - sIDcnt ; sJ > 0 ; sJ-- )
                        {
                            idlOS::printf(" ");
                        }
                        break;
                    }
                    else
                    {
                        idlOS::printf( "%c", sNode->mName[sIDcnt] );
                    }
                }
            }
            else
            {
                for( sJ = 0 ; sJ < 49 ; sJ++ )
                {
                    idlOS::printf(" ");
                }
            }

            idlOS::printf( "|" );

            if ( sIsFirst )
            {
                idlOS::printf( "%-5d|", sNode->mScope );
            }
            else
            {
                idlOS::printf( "     |" ); // 5 spaces
            }

            if ( sIsFirst )
            {
                if ( sNode->mChild == NULL )
                {
                    idlOS::printf( "%-13d|", 0 );
                }
                else
                {
                    idlOS::printf( "%-13d|", sNode->mChild->mCnt );
                }
            }
            else
            {
                idlOS::printf( "             |" ); // 13 spaces
            }

            idlOS::printf( "          |\n" );

            if ( sIsIDEnd == ID_FALSE )
            {
                sMLineCnt++;
                sIsFirst = ID_FALSE;
                continue;
            }

            // structure field 정보 출력
            if ( sNode->mChild != NULL )
            {
                sNode->mChild->ulpPrintSymT( -1 );
            }

            sNode    = sNode->mNext;
            sIsFirst = ID_TRUE;
            sIsIDEnd = ID_FALSE;
        }
    }

    idlOS::printf( "%s\n", sLineB );
    idlOS::fflush(NULL);
}
