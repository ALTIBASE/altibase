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

#include <ulpIfStackMgr.h>


ulpPPifstackMgr::ulpPPifstackMgr()
{
    mIndex = IFSTACKINITINDEX;
}


SInt ulpPPifstackMgr::ulpIfgetIndex()
{
    return mIndex;
}


// #elif, #endif, #else 조건 문에 대한 사용 문법 검사.
idBool ulpPPifstackMgr::ulpIfCheckGrammar( ulpPPiftype aIftype )
{

    switch( aIftype )
    {
        case PP_ELIF:
            if( (mIfstack[mIndex].mType == PP_ELSE)
                 || (mIndex == IFSTACKINITINDEX) )
            {
                return ID_FALSE;
            }
            break;
        case PP_ENDIF:
        case PP_ELSE:
            if( mIndex == IFSTACKINITINDEX )
            {
                return ID_FALSE;
            }
            break;
        default:
            break;
    }

    return ID_TRUE;
}


// #endif 를 만났을경우, #if, #ifdef 혹은 #ifndef 까지 pop 한다.
idBool ulpPPifstackMgr::ulpIfpop4endif()
{
    idBool sBreak;
    idBool sSescDEC;

    sBreak   = ID_FALSE;
    sSescDEC = ID_FALSE;

    while ( mIndex > IFSTACKINITINDEX )
    {
        switch( mIfstack[ mIndex ].mType )
        {
            case PP_IF:
                //idlOS::printf("#pop n:%d, t:%d, s:%d\n", 
                //              mIndex, mIfstack[ mIndex ].mType, mIfstack[ mIndex ].mSkip);
                mIndex--;
                sBreak = ID_TRUE;
                break;
            case PP_IFDEF:
                //idlOS::printf("#pop n:%d, t:%d, s:%d\n", 
                //              mIndex, mIfstack[ mIndex ].mType, mIfstack[ mIndex ].mSkip);
                /* BUG-28162 : SESC_DECLARE 부활  */
                sSescDEC = mIfstack[ mIndex ].mSescDEC;
                mIndex--;
                sBreak = ID_TRUE;
                break;
            case PP_IFNDEF:
                //idlOS::printf("#pop n:%d, t:%d, s:%d\n", 
                //              mIndex, mIfstack[ mIndex ].mType, mIfstack[ mIndex ].mSkip);
                mIndex--;
                sBreak = ID_TRUE;
                break;
            default:
                //idlOS::printf("#pop n:%d, t:%d, s:%d\n", 
                //              mIndex, mIfstack[ mIndex ].mType, mIfstack[ mIndex ].mSkip);
                mIndex--;
                break;
        }
        if ( sBreak == ID_TRUE )
        {
            break;
        }
    }

    return sSescDEC;
}


IDE_RC ulpPPifstackMgr::ulpIfpush( ulpPPiftype aIftype, ulpPPifresult aVal )
{
    if( mIndex >= MAX_MACRO_IF_STACK_SIZE )
    {
        // error report
        return IDE_FAILURE;
    }
    mIndex++;
    mIfstack[ mIndex ].mType = aIftype;
    /* BUG-28162 : SESC_DECLARE 부활  */
    if( aVal != PP_IF_SESC_DEC )
    {
        mIfstack[ mIndex ].mVal     = aVal;
        mIfstack[ mIndex ].mSescDEC = ID_FALSE;
    }
    else
    {
        // #ifdef SESC_DECLARE 가오면 무조건 참이다.
        mIfstack[ mIndex ].mVal     = PP_IF_TRUE;
        mIfstack[ mIndex ].mSescDEC = ID_TRUE;
    }

    return IDE_SUCCESS;
}


// 해당 macro 조건문을 그냥 skip해도 되는지 검사하기위해 이전상태값을 리턴해준다.
ulpPPifresult ulpPPifstackMgr::ulpPrevIfStatus()
{
    if( mIndex > IFSTACKINITINDEX )
    {
        return mIfstack[ mIndex ].mVal;
    }
    else
    {
        return PP_IF_TRUE;
    }
}

/* BUG-27961 : preprocessor의 중첩 #if처리시 #endif 다음소스 무조건 출력하는 버그  */
// preprocessor가 #endif 를 마친후 다음에오는 코드들을
// 파일로 출력을 해야하는지 그냥 skip해야하는지를 결정해줌.
idBool ulpPPifstackMgr::ulpIfneedSkip4Endif()
{
    idBool sRes;

    if( mIndex < 0 )
    {
        // 아래의 경우에 해당됨.
        // #if ...
        // ...
        // #endif
        // ... 여기 부터는 파일로 출력 해야함. <<< 이 조건임.
        sRes = ID_FALSE;
    }
    else
    {
        if( (mIndex == 0) && (mIfstack[ mIndex ].mVal == PP_IF_TRUE) )
        {
            // 아래의 경우에 해당됨.
            // #if 1
            // ...
            //   #if ...
            //   ...
            //   #endif
            // ... 여기 부터는 파일로 출력 해야함. <<< 이 조건임.
            // #endif
            sRes = ID_FALSE;
        }
        else
        {
            if( (mIndex > 0) &&
                (mIfstack[ mIndex ].mVal     == PP_IF_TRUE) &&
                (mIfstack[ mIndex - 1 ].mVal == PP_IF_FALSE) )
            {
                // 아래의 경우에 해당됨.
                // #if 0
                // ...
                // #else
                // ...
                //   #if ...
                //   ...
                //   #endif
                // ... 여기 부터는 파일로 출력 해야함. <<< 이 조건임.
                // #endif
                sRes = ID_FALSE;
            }
            else
            {
                if( (mIndex > 0) &&
                    (mIfstack[ mIndex ].mVal     == PP_IF_TRUE) &&
                    (mIfstack[ mIndex - 1 ].mVal == PP_IF_TRUE) )
                {
                    // 아래의 경우에 해당됨.
                    //#if 1
                    // #if 1
                    // ...
                    //   #if ...
                    //   ...
                    //   #endif
                    // ... 여기 부터는 파일로 출력 해야함. <<< 이 조건임.
                    // #endif
                    //#endif
                    sRes = ID_FALSE;
                }
                else
                {
                    // 나머지 경우에는 무시됨.
                    sRes = ID_TRUE;
                }
            }
        }
    }

    return sRes;
}

