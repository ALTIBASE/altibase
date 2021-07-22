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
 
#include <acpStr.h>


/*
 * constant string objects
 */
typedef struct myGlobal
{
    acp_sint32_t mInt;
    acp_str_t    mStr;
} myGlobal;

static myGlobal gMyGlobal[] =
{
    {
        1,
        ACP_STR_CONST("A")
    },
    {
        2,
        ACP_STR_CONST("B")
    },
    {
        0,
        ACP_STR_CONST(NULL)
    }
};


/*
 * string objects in structure
 */
typedef struct myObj
{
    acp_sint32_t mInt;

    ACP_STR_DECLARE_CONST(mConstStr);
    ACP_STR_DECLARE_STATIC(mStaticStr, 100);
    ACP_STR_DECLARE_DYNAMIC(mDynamicStr);
} myObj;

void myObjInit(myObj *aObj)
{
    aObj->mNumber = 0;

    ACP_STR_INIT_CONST(aObj->mConstStr);
    ACP_STR_INIT_STATIC(aObj->mStaticStr);
    ACP_STR_INIT_DYNAMIC(aObj->mDynamicStr, 50, 20);
}

void myObjFinal(myObj *aObj)
{
    ACP_STR_FINAL(aObj->mConstStr);   /* optional                             */
    ACP_STR_FINAL(aObj->mStaticStr);  /* optional                             */
    ACP_STR_FINAL(aObj->mDynamicStr); /* mandatory; if not exist, memory leak */
}


/*
 * string objects in stack
 */
void myFunc(void)
{
    acp_sint32_t sInt;

    ACP_STR_DECLARE_CONST(sConstStr);
    ACP_STR_DECLARE_STATIC(sStaticStr, 100);
    ACP_STR_DECLARE_DYNAMIC(sDynamicStr);

    ACP_STR_INIT_CONST(sConstStr);
    ACP_STR_INIT_STATIC(sStaticStr);
    ACP_STR_INIT_DYNAMIC(sDynamicStr, 50, 20);

    /*
     * do something
     */

    ACP_STR_FINAL(sConstStr);   /* optional                             */
    ACP_STR_FINAL(sStaticStr);  /* optional                             */
    ACP_STR_FINAL(sDynamicStr); /* mandatory; if not exist, memory leak */
}
