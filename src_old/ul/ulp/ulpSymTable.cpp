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

#include <ulpSymTable.h>


ulpSymTable::ulpSymTable()
{

}

ulpSymTable::~ulpSymTable()
{

}

void ulpSymTable::ulpInit()
{
    SInt sI;
    for (sI = 0; sI < MAX_SYMTABLE_ELEMENTS; sI++)
    {
        mSymbolTable[sI] = NULL;
    }
    mCnt         = 0;
    mHash        = ulpHashFunc;
    mSize        = MAX_SYMTABLE_ELEMENTS;
    mInOrderList = NULL;
}

void ulpSymTable::ulpFinalize()
{
    SInt sI;
    ulpSymTNode *sNode;
    ulpSymTNode *sPNode;

    for (sI = 0; sI < MAX_SYMTABLE_ELEMENTS; sI++)
    {
        sNode = mSymbolTable[sI];
        while (sNode != NULL)
        {
            sPNode  = sNode;
            sNode   = sNode->mNext;
            idlOS::free(sPNode);
            sPNode = NULL;
        }
    }
}

// host variable를 m_SymbolTable에 저장한다.
ulpSymTNode *ulpSymTable::ulpSymAdd ( ulpSymTElement *aSym )
{
    SInt            sIndex;
    ulpSymTNode    *sSymNode = NULL;
    ulpSymTNode    *sSymNode2 = NULL;
    ulpSymTNode    *sSymNode3 = NULL;

    /* BUG-32413 APRE memory allocation failure should be fixed */
    sSymNode = (ulpSymTNode *) idlOS::malloc( ID_SIZEOF(ulpSymTNode) );
    IDE_TEST_RAISE( sSymNode == NULL, ERR_MEMORY_ALLOC );
    idlOS::memset(sSymNode, 0, ID_SIZEOF(ulpSymTNode) );

    idlOS::memcpy( &(sSymNode->mElement), aSym, ID_SIZEOF(ulpSymTElement) );

    sIndex = (*mHash)( (UChar *)sSymNode->mElement.mName ) % ( mSize );

    // bucket list의 제일 앞에다 추가함.
    sSymNode2 = mSymbolTable[sIndex];

    mSymbolTable[sIndex] = sSymNode;

    sSymNode->mNext = sSymNode2;

    // mInOrderList 에 추가함.
    for ( sSymNode2 = mInOrderList, sSymNode3 = NULL
          ; sSymNode2 != NULL
          ; sSymNode2 = sSymNode2->mInOrderNext  )
    {
        sSymNode3 = sSymNode2;
    }

    if ( sSymNode3 == NULL)
    {
        mInOrderList = sSymNode;
    }

    else
    {
        sSymNode3->mInOrderNext = sSymNode;
    }

    mCnt++;

    return sSymNode;

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

// 특정 이름을 갖는 변수를 symbol table에서 검색한다.
ulpSymTElement *ulpSymTable::ulpSymLookup( SChar *aName )
{
    SInt sIndex;
    ulpSymTNode *sSymNode;

    sIndex = (*mHash)( (UChar *)aName ) % ( mSize );
    sSymNode = mSymbolTable[sIndex];

    while ( ( sSymNode != NULL ) &&
            idlOS::strcmp( aName, sSymNode->mElement.mName )  )
    {
        sSymNode = sSymNode->mNext;
    }

    if( sSymNode == NULL )
    {
        return NULL;
    }
    else
    {
        return &(sSymNode->mElement);
    }
}

// 특정 이름을 갖는 변수를 symbol table에서 제거한다.
void ulpSymTable::ulpSymDelete( SChar *aName )
{
    SInt sIndex;
    ulpSymTNode *sSymNode;
    ulpSymTNode *sPSymNode;

    sPSymNode = NULL;
    sIndex = (*mHash)( (UChar *)aName ) % ( mSize );
    sSymNode = mSymbolTable[sIndex];

    while (  sSymNode != NULL )
    {
        if ( !idlOS::strcmp( aName, sSymNode->mElement.mName ) )
        {
            if( sPSymNode == NULL )
            {
                mSymbolTable[sIndex] = sSymNode->mNext;
            }
            else
            {
                sPSymNode -> mNext = sSymNode->mNext;
            }
            idlOS::free(sSymNode);
            mCnt--;
            break;
        }
        sPSymNode = sSymNode;
        sSymNode = sSymNode -> mNext;
    }
}


// print symbol table for debug
void ulpSymTable::ulpPrintSymT( SInt aScopeD )
{
    SInt   sI, sJ, sK;
    SInt   sCnt;
    SInt   sMLineCnt;
    SInt   sIDcnt;
    SInt   sSIDcnt;
    SInt   sArrSizecnt;
    idBool sIsIDEnd;      // id 길이가 15자를 넘을 경우 다음 라인에 출력하기 위해 사용됨.
    idBool sIsSIDEnd;     // struct id 길이가 10자를 넘을 경우 사용됨.
    idBool sIsArrSizeEnd; // array size 길이가 10자를 넘을 경우 사용됨.
    idBool sIsFirst;
    SChar  sTmpStr[11];
    SChar  sArrSize[100]; // array size sting을 저장하기 위한 임시버퍼.
    ulpSymTNode *sNode;

    sCnt = 1;
    /*BUG-28414*/
    sMLineCnt  = 1;
    sIDcnt     = 0;
    sSIDcnt    = 0;
    sArrSizecnt= 0;
    sIsIDEnd   = ID_FALSE;
    sIsSIDEnd  = ID_FALSE;
    sIsArrSizeEnd = ID_FALSE;
    sIsFirst   = ID_TRUE;
    const SChar sLineB1[10] = // + 3 + (5) + 15 + 10 + 7 + 10 + 5 + 6 + 10 + 6 + (total len=86)
                              "+---";
    const SChar sLineB2[10] = "+-----"; // for scope depth info.
    const SChar sLineB3[80] = "+---------------+----------+-------+-----+----------+------+----------+------+\n";

    if (aScopeD == -1)
    {
        idlOS::printf("+---+-------------------------------------------------+-----+-------------+----------+\n");
        idlOS::printf( "    |%-3s|%-15s|%-10s|%-7s|%-5s|%-10s|%-6s|%-10s|%-6s|\n",
                       "No.","ID","TYPE","Typedef","Array","ArraySize","Struct","S_name","Signed" );
    }

    for ( sI = 0 ; sI < MAX_SYMTABLE_ELEMENTS; sI++)
    {
        sNode = mSymbolTable[sI];
        while (sNode != NULL)
        {
            if( sNode->mElement.mIsstruct != ID_TRUE )
            {
                sIsSIDEnd  = ID_TRUE;
            }
            else
            {
                sIsSIDEnd  = ID_FALSE;
            }

            if( sIsFirst == ID_TRUE )
            {
                sMLineCnt  = 1;
                if (aScopeD == -1)
                {   // structure 일경우
                    idlOS::printf( "    " );  //4 spaces
                }
                idlOS::printf( "%s", sLineB1 );
                if (aScopeD != -1)
                {
                    idlOS::printf( "%s", sLineB2 );
                }
                idlOS::printf( "%s", sLineB3 );
                if( aScopeD != -1 )
                {
                    idlOS::printf( "|%-3d|", sCnt++ );
                }
                else
                {
                    idlOS::printf( "    |%-3d|", sCnt++ );
                }

                sArrSize[0] = '\0';
                if( sNode->mElement.mArraySize2[0] != '\0' )
                {
                    idlOS::snprintf( sArrSize, 100, "%s, %s",
                                     sNode->mElement.mArraySize,
                                     sNode->mElement.mArraySize2);
                }
                else
                {
                    idlOS::snprintf( sArrSize, 100, "%s",
                                     sNode->mElement.mArraySize);
                }
            }
            else
            {
                if (aScopeD == -1)
                {   // structure 일경우
                    idlOS::printf( "    " );  //4 spaces
                }
                idlOS::printf( "|   |" ); // 3 spaces
            }

            if( aScopeD != -1 )
            {
                if( (sIsFirst == ID_TRUE))
                {
                    idlOS::printf( "%-5d|", aScopeD );
                }
                else
                {
                    idlOS::printf( "     |" ); // 5 spaces
                }
            }

            if( sIsIDEnd != ID_TRUE )
            {
                for( (sIsFirst == ID_TRUE )?sIDcnt = 0 : sIDcnt=sIDcnt
                     ; sIDcnt < 15 * sMLineCnt ; sIDcnt++ )
                {
                    if( sNode->mElement.mName[sIDcnt] == '\0' )
                    {
                        sIsIDEnd = ID_TRUE;
                        for( sJ = 15 * sMLineCnt - sIDcnt ; sJ > 0 ; sJ-- )
                        {
                            idlOS::printf(" ");
                        }
                        break;
                    }
                    else
                    {
                        idlOS::printf( "%c", sNode->mElement.mName[sIDcnt] );
                    }
                }
            }
            else
            {
                for( sJ = 0 ; sJ < 15 ; sJ++ )
                {
                    idlOS::printf(" ");
                }
            }

            idlOS::printf( "|" );

            if( sIsFirst == ID_TRUE )
            {
                if( sNode->mElement.mIsstruct == ID_TRUE )
                {
                    idlOS::strcpy(sTmpStr, "struct");
                }
                else
                {
                    switch(sNode->mElement.mType)
                    {
                        case H_UNKNOWN :
                            idlOS::strcpy(sTmpStr, "unknown");
                            break;
                        case H_NUMERIC :
                            idlOS::strcpy(sTmpStr, "numeric");
                            break;
                        /* BUG-45933 */
                        case H_NUMERIC_STRUCT :
                            idlOS::strcpy(sTmpStr, "numeric_struct");
                            break;
                        case H_INTEGER :
                            idlOS::strcpy(sTmpStr, "integer");
                            break;
                        case H_INT     :
                            idlOS::strcpy(sTmpStr, "int");
                            break;
                        case H_LONG    :
                            idlOS::strcpy(sTmpStr, "long");
                            break;
                        case H_LONGLONG    :
                            idlOS::strcpy(sTmpStr, "longlong");
                            break;
                        case H_SHORT   :
                            idlOS::strcpy(sTmpStr, "short");
                            break;
                        case H_CHAR    :
                            idlOS::strcpy(sTmpStr, "char");
                            break;
                        case H_VARCHAR :
                            idlOS::strcpy(sTmpStr, "varchar");
                            break;
                        case H_NCHAR    :
                            idlOS::strcpy(sTmpStr, "nchar");
                            break;
                        case H_NVARCHAR :
                            idlOS::strcpy(sTmpStr, "nvarchar");
                            break;
                        case H_BINARY  :
                            idlOS::strcpy(sTmpStr, "binary");
                            break;
                        case H_BINARY2  :  /* BUG-46418 */
                            idlOS::strcpy(sTmpStr, "binary2");
                            break;
                        case H_BIT     :
                            idlOS::strcpy(sTmpStr, "bit");
                            break;
                        case H_BYTES   :
                            idlOS::strcpy(sTmpStr, "byte");
                            break;
                        case H_VARBYTE :
                            idlOS::strcpy(sTmpStr, "varbyte");
                            break;
                        case H_NIBBLE  :
                            idlOS::strcpy(sTmpStr, "nibble");
                            break;
                        case H_FLOAT   :
                            idlOS::strcpy(sTmpStr, "float");
                            break;
                        case H_DOUBLE  :
                            idlOS::strcpy(sTmpStr, "double");
                            break;
                        case H_DATE    :
                            idlOS::strcpy(sTmpStr, "date");
                            break;
                        case H_TIME    :
                            idlOS::strcpy(sTmpStr, "time");
                            break;
                        case H_TIMESTAMP :
                            idlOS::strcpy(sTmpStr, "timestamp");
                            break;
                        case H_CLOB    :
                            idlOS::strcpy(sTmpStr, "clob");
                            break;
                        case H_BLOB    :
                            idlOS::strcpy(sTmpStr, "blob");
                            break;
                        case H_BLOBLOCATOR :
                            idlOS::strcpy(sTmpStr, "blob_loc");
                            break;
                        case H_CLOBLOCATOR :
                            idlOS::strcpy(sTmpStr, "clob_loc");
                            break;
                        case H_BLOB_FILE :
                            idlOS::strcpy(sTmpStr, "bloc_file");
                            break;
                        case H_CLOB_FILE :
                            idlOS::strcpy(sTmpStr, "clob_file");
                            break;
                        case H_USER_DEFINED :
                            idlOS::strcpy(sTmpStr, "user_def");
                            break;
                        case H_FAILOVERCB :
                            idlOS::strcpy(sTmpStr, "failovercb");
                            break;
                        default:
                            break;
                    }
                }
                if ( sNode->mElement.mPointer != 0 )
                {
                    for( sJ = sNode->mElement.mPointer ; sJ > 0 ; sJ-- )
                    {
                        idlOS::strcat(sTmpStr, "*");
                    }
                }

                idlOS::printf("%s", sTmpStr);

                for( sJ = 10 - idlOS::strlen(sTmpStr) ; sJ > 0 ; sJ-- )
                {
                    idlOS::printf(" ");
                }
                idlOS::printf("|");
            }
            else
            {
                idlOS::printf( "          |" ); // 10 spaces
            }

            if( sIsFirst == ID_TRUE )
            {
                idlOS::printf( "%-7c|", (sNode->mElement.mIsTypedef == ID_TRUE)?'Y':'N' );
            }
            else
            {
                idlOS::printf( "       |" ); // 7 spaces
            }

            if( sIsFirst == ID_TRUE )
            {
                idlOS::printf( "%-5c|", (sNode->mElement.mIsarray == ID_TRUE)?'Y':'N' );
            }
            else
            {
                idlOS::printf( "     |" ); // 5 spaces
            }

            if( sIsArrSizeEnd != ID_TRUE )
            {
                for( (sIsFirst == ID_TRUE )?sArrSizecnt = 0 : sArrSizecnt=sArrSizecnt
                      ; sArrSizecnt < 10 * sMLineCnt ; sArrSizecnt++ )
                {
                    if( sArrSize[sArrSizecnt] == '\0' )
                    {
                        sIsArrSizeEnd = ID_TRUE;
                        for( sK = 10 * sMLineCnt - sArrSizecnt ; sK > 0 ; sK-- )
                        {
                            idlOS::printf(" ");
                        }
                        break;
                    }
                    else
                    {
                        idlOS::printf( "%c", sArrSize[sArrSizecnt] );
                    }
                }
            }
            else
            {
                for( sK = 0 ; sK < 10 ; sK++ )
                {
                    idlOS::printf(" ");
                }
            }

            idlOS::printf( "|" );

            if( sIsFirst == ID_TRUE )
            {
                idlOS::printf( "%-6c|", (sNode->mElement.mIsstruct == ID_TRUE)?'Y':'N' );
            }
            else
            {
                idlOS::printf( "      |" ); // 6 spaces
            }

            if( sIsSIDEnd != ID_TRUE )
            {
                for( (sIsFirst == ID_TRUE )?sSIDcnt = 0 : sSIDcnt=sSIDcnt
                      ; sSIDcnt < 10 * sMLineCnt ; sSIDcnt++ )
                {
                    if( sNode->mElement.mStructName[sSIDcnt] == '\0' )
                    {
                        sIsSIDEnd = ID_TRUE;
                        for( sJ = 10 * sMLineCnt - sSIDcnt ; sJ > 0 ; sJ-- )
                        {
                            idlOS::printf(" ");
                        }
                        break;
                    }
                    else
                    {
                        idlOS::printf( "%c", sNode->mElement.mStructName[sSIDcnt] );
                    }
                }
            }
            else
            {
                idlOS::printf( "          " ); // 10 spaces
            }

            if( sIsFirst == ID_TRUE )
            {
                idlOS::printf( "|%-6c|\n", (sNode->mElement.mIssign == ID_TRUE)?'Y':'N' );
            }
            else
            {
                idlOS::printf( "|      |\n" ); // 6 spaces
            }

            if ( ( sIsIDEnd == ID_FALSE ) || ( sIsSIDEnd == ID_FALSE )
                 || ( sIsArrSizeEnd == ID_FALSE ))
            {
                sMLineCnt++;
                sIsFirst = ID_FALSE;
                continue;
            }

            sNode     = sNode->mNext;
            sIsFirst  = ID_TRUE;
            sIsIDEnd  = ID_FALSE;
            sIsArrSizeEnd = ID_FALSE;
        }
    }
    idlOS::fflush(NULL);
}

