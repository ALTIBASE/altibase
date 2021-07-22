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

#ifndef _ULP_IFSTACKMGR_H_
#define _ULP_IFSTACKMGR_H_ 1

#include <idl.h>
#include <ulpMacro.h>

/* macro if 조건문처리를 위한 자료구조 */
typedef struct ulpPPifstack
{
    ulpPPiftype   mType;
    ulpPPifresult mVal;
    /* BUG-28162 : SESC_DECLARE 부활  */
    idBool        mSescDEC;
} ulpPPifstack;


class ulpPPifstackMgr
{
    private:

        ulpPPifstack mIfstack[MAX_MACRO_IF_STACK_SIZE];

        SInt         mIndex;

    public:

        ulpPPifstackMgr();

        SInt ulpIfgetIndex();

        idBool ulpIfCheckGrammar( ulpPPiftype aIftype );

        /* BUG-28162 : SESC_DECLARE 부활  */
        idBool ulpIfpop4endif();

        IDE_RC ulpIfpush( ulpPPiftype aIftype, ulpPPifresult aVal );

        ulpPPifresult ulpPrevIfStatus ( void );

        /* BUG-27961 : preprocessor의 중첩 #if처리시 #endif 다음소스 무조건 출력하는 버그  */
        // preprocessor가 #endif 를 마친후 다음에오는 코드들을
        // 파일로 출력을 해야하는지 그냥 skip해야하는지를 결정해주는 함수.
        idBool ulpIfneedSkip4Endif(void);
};

#endif
