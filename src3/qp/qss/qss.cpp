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
 * $Id: qss.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <qss.h>

/* Set texts for qcSharedPlan */
IDE_RC qss::setStmtTexts( qcStatement    * aStatement,
                          SChar          * aPlainText,
                          SInt             aPlainTextLen )
{
    /* 적합성 검사 */
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPlainText != NULL );
    IDE_DASSERT( aPlainTextLen != 0 );

    /* set encryptedText */
    aStatement->myPlan->encryptedText    = aStatement->myPlan->stmtText;
    aStatement->myPlan->encryptedTextLen = aStatement->myPlan->stmtTextLen;

    /* set plain text to myPlan->stmtText */
    aStatement->myPlan->stmtText = NULL;
    aStatement->myPlan->stmtTextLen = 0;

    IDU_FIT_POINT( "qss::setStmtTexts::alloc", qpERR_ABORT_MEMORY_ALLOCATION );
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  aPlainTextLen + 1,
                  (void **)&aStatement->myPlan->stmtText )
              != IDE_SUCCESS );

    idlOS::memcpy( aStatement->myPlan->stmtText,
                   aPlainText,
                   aPlainTextLen );

    aStatement->myPlan->stmtText[aPlainTextLen] = '\0';
    aStatement->myPlan->stmtTextLen = aPlainTextLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* getPlainText */
IDE_RC qss::doDecryption( qcStatement    * aStatement,
                          qcNamePosition   aBody )
{
    qcNamePosition    sEncryptedBodyPos;
    SChar           * sDecryptedBody    = NULL;
    SInt              sDecryptedBodyLen = 0;
    SInt              sState = 0;

    /* 적합성 검사 */
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( QC_IS_NULL_NAME( aBody ) == ID_FALSE );

    /* sEncryptedBodyPos 아래와 같이 설정하는 이유는 다음과 같다.
 
       encrypted text가 아래와 같은 예라고 하자.
       => create or replace procedure proc1 wrapped
          'NTM=
          NTY=
          AAhjcmVhdGUgb3IgcmVwbGFjZSBwcm9jZWCRTY5NkMwQzA5RTMzMTVD
          ';
       /

       aBody의 position은 
       'NTM=
       NTY=
       AAhjcmVhdGUgb3IgcmVwbGFjZSBwcm9jZWCRTY5NkMwQzA5RTMzMTVD
       '
       이다.

       그러나, decrpytion 해야 하는 부분은 ' '를 제외한
       NTM=
       NTY=
       AAhjcmVhdGUgb3IgcmVwbGFjZSBwcm9jZWCRTY5NkMwQzA5RTMzMTVD
       부분인 것이다. 

       따라서, decryption 해야 할 text를 offset은 '를 다음 부터이므로 +1을 해줘야 하며,
       전체 text size에서 ' '를 제외한 길이여야 하므로, -2를 해줘야 한다. */
    sEncryptedBodyPos.stmtText = aBody.stmtText;
    sEncryptedBodyPos.offset   = aBody.offset + 1;
    sEncryptedBodyPos.size     = aBody.size - 2;

    /* Decryption and get plain text */
    IDE_TEST( idsAltiWrap::decryption( (SChar*)sEncryptedBodyPos.stmtText + sEncryptedBodyPos.offset, 
                                       sEncryptedBodyPos.size,
                                       &sDecryptedBody,
                                       &sDecryptedBodyLen )
              != IDE_SUCCESS );
    sState = 1;

    /* set texts to qcSharedPlan */
    IDE_TEST( setStmtTexts( aStatement,
                            sDecryptedBody,
                            sDecryptedBodyLen )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( idsAltiWrap::freeResultMem( sDecryptedBody ) != IDE_SUCCESS );

    IDE_DASSERT( aStatement->myPlan->encryptedText != NULL );
    IDE_DASSERT( aStatement->myPlan->encryptedTextLen != 0 );
    IDE_DASSERT( aStatement->myPlan->stmtText != NULL );
    IDE_DASSERT( aStatement->myPlan->stmtTextLen != 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( (sState == 1) &&
         (sDecryptedBody != NULL) )
    {
        (void )idsAltiWrap::freeResultMem( sDecryptedBody );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}
