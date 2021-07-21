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
 

/******************************************************************************
 * $ID$
 *
 *  Description :
 *  Unittest for CompareLSN in SM module.
 *
 ******************************************************************************/

#include <act.h>
#include <smrCompareLSN.h>

void testTrueCond( struct smLSN * aLSN1,
                   struct smLSN * aLSN2)
{
    ACT_CHECK( smrCompareLSN::isGT(aLSN1, aLSN2) == ACP_TRUE );

    ACT_CHECK( smrCompareLSN::isGTE(aLSN1, aLSN2) == ACP_TRUE );
}

void testFalseCond( struct smLSN * aLSN1,
                    struct smLSN * aLSN2)
{
    ACT_CHECK( smrCompareLSN::isGT(aLSN1, aLSN2) == ACP_FALSE );

    ACT_CHECK( smrCompareLSN::isGTE(aLSN1, aLSN2) == ACP_FALSE );

    ACT_CHECK( smrCompareLSN::isEQ(aLSN1, aLSN2) == ACP_FALSE );

    ACT_CHECK( smrCompareLSN::isZero(aLSN1) == ACP_FALSE );

    ACT_CHECK( smrCompareLSN::isZero(aLSN2) == ACP_FALSE );
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    struct smLSN    sLSN1;
    struct smLSN    sLSN2;

    sLSN1.mFileNo   = 65;
    sLSN1.mOffset   = 65;

    sLSN2.mFileNo   = 1;
    sLSN2.mOffset   = 1;


    ACT_TEST_BEGIN();

    testTrueCond( &sLSN1,
                  &sLSN2 );

    testFalseCond( &sLSN2,
                   &sLSN1 );

    ACT_TEST_END();

    return ACP_RC_SUCCESS;
}
