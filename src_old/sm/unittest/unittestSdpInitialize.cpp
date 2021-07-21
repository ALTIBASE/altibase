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
 *  Unittest for initializing in SM module.
 *
 ******************************************************************************/

#include <act.h>
#include <ide.h>
#include <sdpPhyPage.h>
#include <sdnbDef.h>

#define UNITTEST_FREE_SIZE       (6000)
#define UNITTEST_BEGIN_OFFSET    (1000)
#define UNITTEST_END_OFFSET      (7000)

void initPageValues( sdpPhyPageHdr * aPageHdr )
{
    aPageHdr->mTotalFreeSize         = UNITTEST_FREE_SIZE;
    aPageHdr->mAvailableFreeSize     = UNITTEST_FREE_SIZE;
    aPageHdr->mSizeOfCTL             = 0;
    aPageHdr->mLogicalHdrSize        = 0;
    aPageHdr->mFreeSpaceBeginOffset  = UNITTEST_BEGIN_OFFSET;
    aPageHdr->mFreeSpaceEndOffset    = UNITTEST_END_OFFSET;
}

void testInitLogicalHdr( sdpPhyPageHdr * aPageHdr,
                         UInt            aSize )
{
    initPageValues( aPageHdr );

    sdpPhyPage::initLogicalHdr( aPageHdr,
                                aSize );

    ACT_CHECK( aPageHdr->mAvailableFreeSize == UNITTEST_FREE_SIZE - aSize );

    ACT_CHECK( aPageHdr->mTotalFreeSize == UNITTEST_FREE_SIZE - aSize );

    ACT_CHECK( aPageHdr->mFreeSpaceBeginOffset == 
               UNITTEST_BEGIN_OFFSET + aSize );

    ACT_CHECK( aPageHdr->mLogicalHdrSize == aSize );
}

void testInitCTL( sdpPhyPageHdr * aPageHdr,
                  UInt            aSize,
                  UChar        ** aHdrStartPtr )
{
    initPageValues( aPageHdr );

    sdpPhyPage::initCTL( aPageHdr,
                         aSize,
                         aHdrStartPtr );

    ACT_CHECK( aPageHdr->mSizeOfCTL == aSize );

    ACT_CHECK( aPageHdr->mFreeSpaceBeginOffset == 
               UNITTEST_BEGIN_OFFSET + aSize );

    ACT_CHECK( aPageHdr->mAvailableFreeSize == 
               UNITTEST_END_OFFSET - (UNITTEST_BEGIN_OFFSET + aSize) );

    ACT_CHECK( aPageHdr->mTotalFreeSize == aPageHdr->mAvailableFreeSize );
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    UChar               sDummyMemory[SD_PAGE_SIZE * 2];
    UChar             * sHdrStartPtr;
    sdpPhyPageHdr     * sPageHdr;

    sPageHdr = (sdpPhyPageHdr *)sdpPhyPage::getPageStartPtr( 
                                                &sDummyMemory[SD_PAGE_SIZE] );

    ACT_TEST_BEGIN();

    testInitLogicalHdr( (sdpPhyPageHdr *)sPageHdr,
                        ID_SIZEOF(sdnbNodeHdr) );

    testInitCTL( (sdpPhyPageHdr *)sPageHdr,
                 ID_SIZEOF(sdpCTL),
                 &sHdrStartPtr );

    ACT_TEST_END();

    return ACP_RC_SUCCESS;
}
