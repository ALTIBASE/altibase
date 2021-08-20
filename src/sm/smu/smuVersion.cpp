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
 

/*****************************************************************************
 * $Id: smuVersion.cpp 91457 2021-08-11 23:46:14Z emlee $
 ****************************************************************************/

#include <iduCheckLicense.h>
#include <idu.h>
#include <smuVersion.h>
/* ------------------------------------------------------------------------------
 *
 *           VERSION을 아래의 매크로에서 수정하기 바람.
 *
 *  -> example)  버젼이 9.2.123일 경우
 *
 *               #define SM_ALTIBASE_MAJOR_VERSION    (9)
 *               #define SM_ALTIBASE_MINOR_VERSION    (2)
 *               #define SM_ALTIBASE_PATCH_LEVEL    (123)
 *
 * -----------------------------------------------------------------------------*/

#define MKHEX2(a)       ((unsigned int)(((a / 10) * 16) + (a % 10)))
#define MKHEX4(a)       ((unsigned int)((((a % 10000) / 1000) * 16 * 16 * 16) + (( (a % 1000) / 100) * 16 * 16) + (( (a % 100) / 10) * 16) + (a % 10)))

#define SM_VERSION_STRING(a,b,c)  #a"."#b"."#c


/* ------------------------------------------------
 *  For Altibase - 4
 * ----------------------------------------------*/
#define SM_MAJOR_VERSION   (7)
#define SM_MINOR_VERSION   (3)
#define SM_PATCH_LEVEL     (0)

const SChar *smVersionString = SM_VERSION_STRING(7, 3, 0);

#if defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5)
UInt __SM_MAJOR_VERSION__  = MKHEX2(SM_MAJOR_VERSION) << 24;
UInt __SM_MINOR_VERSION__  = MKHEX2(SM_MINOR_VERSION) << 16;
UInt __SM_PATCH_LEVEL__    = MKHEX4(SM_PATCH_LEVEL);
#else
const UInt __SM_MAJOR_VERSION__ = MKHEX2(SM_MAJOR_VERSION) << 24;
const UInt __SM_MINOR_VERSION__ = MKHEX2(SM_MINOR_VERSION) << 16;
const UInt __SM_PATCH_LEVEL__   = MKHEX4(SM_PATCH_LEVEL);
#endif

const UInt   smVersionID = __SM_MAJOR_VERSION__ + __SM_MINOR_VERSION__ + __SM_PATCH_LEVEL__;

/* ------------------------------------------------
 *  Production DB Unique String Creation
 *  PR-4314 :
 *  Composed by
 *  1. Host Unique ID
 *  2. TimeStamp
 *  3. Random Number
 * ----------------------------------------------*/

void smuMakeUniqueDBString(SChar *aUnique)
{
    PDL_Time_Value sTime;
    SChar          sUniqueString[IDU_SYSTEM_INFO_LENGTH];
    SChar          sHostID[IDU_SYSTEM_INFO_LENGTH];
    UInt           sRandomValue;

    idlOS::srand(idlOS::time());

    idlOS::memset(sUniqueString, 0, IDU_SYSTEM_INFO_LENGTH);
    idlOS::memset(sHostID,       0, IDU_SYSTEM_INFO_LENGTH);

    iduCheckLicense::getHostUniqueString(sHostID, ID_SIZEOF(sHostID));

    sTime = idlOS::gettimeofday();

    sRandomValue = (UInt)idlOS::rand();

    idlOS::snprintf(sUniqueString, IDU_SYSTEM_INFO_LENGTH,
                    "%s"
                    "-%08"ID_XINT32_FMT /* sec  */
                    ":%08"ID_XINT32_FMT /* usec */
                    "-%08"ID_XINT32_FMT,
                    sHostID,
                    (UInt)sTime.sec(),
                    (UInt)sTime.usec(),
                    sRandomValue);
    idlOS::strncpy(aUnique, sUniqueString, (IDU_SYSTEM_INFO_LENGTH - 1));
//    fprintf(stderr, "unique=[%s]\n", sUniqueString);
}
