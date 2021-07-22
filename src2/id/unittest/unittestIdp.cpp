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
#include <ide.h>
#include <idu.h>

#include <idpUInt.h>
#include <idpSInt.h>
#include <idpULong.h>
#include <idpSLong.h>
#include <idpString.h>

#include <idp.h>


int main()
{
    ACT_TEST_BEGIN();

    idpUInt sTestUInt("testUInt", IDP_ATTR_SL_ALL, 0, ID_UINT_MAX, 0);
    idpSInt sTestSInt("testSInt", IDP_ATTR_SL_ALL, 0, ID_SINT_MAX, 0);
    idpULong sTestULong("testULong", IDP_ATTR_SL_ALL, 0, ID_ULONG_MAX, 0);
    idpSLong sTestSLong("testSLong", IDP_ATTR_SL_ALL, 0, ID_SLONG_MAX, 0);
    
    SInt sRet;
    UInt *sUIval;
    SInt *sSIval;
    ULong *sULval;
    SLong *sSLval;
    
    char sBufNormalMega[] = "123M";
    char sBufNormalKilo[] = "123K";
    char sBufNormalGiga[] = "1g";
    
    char sBufHugeMega[] = "4096M";
    char sBufHugeKilo[] = "4194304K";
    char sBufHugeGiga[] = "4g";

    char sBufZero[] = "0";
    char sBufMinusOne[] = "-1";
    char sBufWrong[] = "99999999999999999999999999999999999999999";
    char sBufWrongGiga[] = "9999999999999999999999999999999999999G";
    char sBufWrongMega[] = "9999999999999999999999999999999999999M";
    char sBufWrongKilo[] = "9999999999999999999999999999999999999K";
    char sBufWrongSym[] = "123;";
    char sBufWrongChar[] = "123a";
    char sBufWrongNum[] = "12 34";
    

    char sBufUIntMax[] = "4294967295"; // ID_UINT_MAX
    char sBufUIntOver[] = "4294967296"; // ID_UINT_MAX+1
    char sBufSIntMax[] = "2147483647"; // ID_SINT_MAX
    char sBufSIntOver[] = "2147483648"; // ID_SINT_MAX+1
    char sBufSIntMin[] = "-2147483648"; // ID_SINT_MIN
    char sBufSIntUnder[] = "-2147483649"; // ID_SINT_MIN-1

    char sBufULongMax[] = "18446744073709551615"; // ID_ULONG_MAX
    char sBufULongOver[] = "18446744073709551616"; // ID_ULONG_MAX+1
    char sBufSLongMax[] = "9223372036854775807"; // ID_SLONG_MAX
    char sBufSLongOver[] = "9223372036854775808"; // ID_SLONG_MAX+1
    char sBufSLongMin[] = "-9223372036854775808"; // ID_SLONG_MIN
    char sBufSLongUnder[] = "-9223372036854775809"; // ID_SLONG_MIN-1
    
    
    // idp and iduMutexMgr must be initlaized at the first of program
    ACT_ASSERT(idp::initialize(NULL, NULL) == IDE_SUCCESS);

    // SInt

    sRet = sTestSInt.convertFromString(sBufNormalMega, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSIval == 123*1024*1024);
    sRet = sTestSInt.convertFromString(sBufNormalKilo, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSIval == 123*1024);
    sRet = sTestSInt.convertFromString(sBufNormalGiga, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSIval == 1024*1024*1024);
    
    sRet = sTestSInt.convertFromString(sBufHugeMega, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufHugeKilo, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufHugeGiga, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    
    sRet = sTestSInt.convertFromString(sBufZero, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSIval == 0);
    sRet = sTestSInt.convertFromString(sBufMinusOne, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSIval == -1);
    
    sRet = sTestSInt.convertFromString(sBufWrong, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufWrongGiga, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufWrongMega, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufWrongKilo, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufWrongSym, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufWrongChar, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufWrongNum, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);

    sRet = sTestSInt.convertFromString(sBufSIntMax, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSIval == (SInt)(0x7fffffff));
    sRet = sTestSInt.convertFromString(sBufSIntOver, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufSIntMin, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSIval == -2147483648);
    sRet = sTestSInt.convertFromString(sBufSIntUnder, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSInt.convertFromString(sBufSLongMax, (void **)&sSIval);
    ACT_CHECK(sRet == IDE_FAILURE);


    // UInt
    sRet = sTestUInt.convertFromString(sBufNormalMega, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sUIval == 123*1024*1024);
    sRet = sTestUInt.convertFromString(sBufNormalKilo, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sUIval == 123*1024);
    sRet = sTestUInt.convertFromString(sBufNormalGiga, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sUIval == 1024*1024*1024);

    sRet = sTestUInt.convertFromString(sBufHugeMega, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestUInt.convertFromString(sBufHugeKilo, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestUInt.convertFromString(sBufHugeGiga, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    
    sRet = sTestUInt.convertFromString(sBufZero, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sUIval == 0);
    sRet = sTestUInt.convertFromString(sBufMinusOne, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    
    sRet = sTestUInt.convertFromString(sBufWrong, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestUInt.convertFromString(sBufWrongGiga, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestUInt.convertFromString(sBufWrongMega, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestUInt.convertFromString(sBufWrongKilo, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestUInt.convertFromString(sBufWrongSym, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestUInt.convertFromString(sBufWrongChar, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestUInt.convertFromString(sBufWrongNum, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    
    sRet = sTestUInt.convertFromString(sBufUIntMax, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sUIval == ID_UINT_MAX);
    sRet = sTestUInt.convertFromString(sBufUIntOver, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestUInt.convertFromString(sBufULongMax, (void **)&sUIval);
    ACT_CHECK(sRet == IDE_FAILURE);
    

    // ULong
    sRet = sTestULong.convertFromString(sBufNormalMega, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == 123*1024*1024);
    sRet = sTestULong.convertFromString(sBufNormalKilo, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == 123*1024);
    sRet = sTestULong.convertFromString(sBufNormalGiga, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == 1024*1024*1024);

    sRet = sTestULong.convertFromString(sBufHugeMega, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == (ULong)4096*1024*1024);
    sRet = sTestULong.convertFromString(sBufHugeKilo, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == (ULong)4194304*1024);
    sRet = sTestULong.convertFromString(sBufHugeGiga, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == (ULong)4*1024*1024*1024);
    
    sRet = sTestULong.convertFromString(sBufZero, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == 0);
    sRet = sTestULong.convertFromString(sBufMinusOne, (void **)&sULval);
    ACT_CHECK(sRet == IDE_FAILURE);
    
    sRet = sTestULong.convertFromString(sBufWrong, (void **)&sULval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestULong.convertFromString(sBufWrongGiga, (void **)&sULval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestULong.convertFromString(sBufWrongMega, (void **)&sULval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestULong.convertFromString(sBufWrongKilo, (void **)&sULval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestULong.convertFromString(sBufWrongSym, (void **)&sULval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestULong.convertFromString(sBufWrongChar, (void **)&sULval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestULong.convertFromString(sBufWrongNum, (void **)&sULval);
    ACT_CHECK(sRet == IDE_FAILURE);
    
    sRet = sTestULong.convertFromString(sBufULongMax, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == ID_ULONG_MAX);
    sRet = sTestULong.convertFromString(sBufULongOver, (void **)&sULval);
    ACT_CHECK(sRet == IDE_FAILURE);

    // SLong
    sRet = sTestSLong.convertFromString(sBufNormalMega, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSLval == 123*1024*1024);
    sRet = sTestSLong.convertFromString(sBufNormalKilo, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSLval == 123*1024);
    sRet = sTestSLong.convertFromString(sBufNormalGiga, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSLval == 1024*1024*1024);

    sRet = sTestSLong.convertFromString(sBufHugeMega, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == (SLong)4096*1024*1024);
    sRet = sTestSLong.convertFromString(sBufHugeKilo, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == (SLong)4194304*1024);
    sRet = sTestSLong.convertFromString(sBufHugeGiga, (void **)&sULval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sULval == (SLong)4*1024*1024*1024);
    
    sRet = sTestSLong.convertFromString(sBufZero, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSLval == 0);
    sRet = sTestSLong.convertFromString(sBufMinusOne, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSLval == -1);
    
    sRet = sTestSLong.convertFromString(sBufWrong, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSLong.convertFromString(sBufWrongGiga, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSLong.convertFromString(sBufWrongMega, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSLong.convertFromString(sBufWrongKilo, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSLong.convertFromString(sBufWrongSym, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSLong.convertFromString(sBufWrongChar, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSLong.convertFromString(sBufWrongNum, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_FAILURE);
    
    sRet = sTestSLong.convertFromString(sBufSLongMax, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSLval == (SLong)(0x7fffffffffffffff));
    sRet = sTestSLong.convertFromString(sBufSLongOver, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_FAILURE);
    sRet = sTestSLong.convertFromString(sBufSLongMin, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_SUCCESS && *sSLval == (SLong)0x8000000000000000);
    sRet = sTestSLong.convertFromString(sBufSLongUnder, (void **)&sSLval);
    ACT_CHECK(sRet == IDE_FAILURE);

    
    ACT_TEST_END();

    return 0;
}
