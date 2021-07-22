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
 
#include <idl.h>
#include <act.h>
#include <acp.h>

/*
 * idlOS::strToUInt unittests
 */
void unittestIdlOsStrToUInt()
{
    ACT_CHECK(idlOS::strToUInt((UChar *)"1234", 4) == (UInt)1234);

    ACT_CHECK(idlOS::strToUInt((UChar *)"12345678", 8) == (UInt)12345678);
}

/*
 * idlOS::strCaselessMatch unittests
 */
void unittestIdlOsStrCaselessMatch()
{
    UChar sBuf1[] = "ABCDEFGHIJ";
    UChar sBuf2[] = "aBcDeFgHiJ";
    UChar sBuf3[] = "ABCDefgh";

    ACT_CHECK(idlOS::strCaselessMatch(sBuf1, sBuf2) != -1);

    ACT_CHECK(idlOS::strCaselessMatch(sBuf2, sBuf3) == -1);
}

SInt main()
{
    unittestIdlOsStrToUInt();

    unittestIdlOsStrCaselessMatch();

    return 0;
}
