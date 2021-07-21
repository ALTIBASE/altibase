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

#include <qci.h>
#include <mmErrorCode.h>
#include <mmuAccessList.h>
#include <mmuProperty.h>

iduLatch  mmuAccessList::mLatch;
idBool    mmuAccessList::mInitialized = ID_FALSE;

/* BUG-48515 
 * 기존 access_list와 limit size가 설정된 access_list를 분리. */
mmuIPACLInfo mmuAccessList::mIPACLInfo[MM_IP_ACL_MAX_COUNT];
mmuIPACLInfo mmuAccessList::mIPACLInfoWithLimit[MM_IP_ACL_MAX_COUNT];

UInt   mmuAccessList::mIPACLCount;
UInt   mmuAccessList::mIPACLCountWithLimit;

IDE_RC mmuAccessList::initialize()
{
    IDE_TEST( mLatch.initialize(
                    (SChar *)"MMU_ACCESS_LIST_LATCH",
                    IDU_LATCH_TYPE_NATIVE )
              != IDE_SUCCESS );

    mInitialized = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmuAccessList::finalize()
{
    mInitialized = ID_FALSE;

    IDE_TEST( mLatch.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mmuAccessList::clear()
{
    lock();

    mIPACLCount = 0;

    mIPACLCountWithLimit = 0;

    unlock();
}

IDE_RC mmuAccessList::add( idBool            aIPACLPermit,
                           struct in6_addr * aIPACLAddr,
                           SChar           * aIPACLAddrStr,
                           UInt              aIPACLAddrFamily,
                           UInt              aIPACLMask,
                           UInt              aIPACLLimitSize )
{
    idBool  sLocked = ID_FALSE;

    lock();
    sLocked = ID_TRUE;

    IDE_TEST_RAISE( (mIPACLCount + mIPACLCountWithLimit) >= MM_IP_ACL_MAX_COUNT,
                    ERR_ABORT_EXCEEDED_ACCESS_LIST_COUNT_LIMIT );

    if ( aIPACLLimitSize > 0 )
    {
        mIPACLInfoWithLimit[mIPACLCountWithLimit].mPermit = aIPACLPermit;
        mIPACLInfoWithLimit[mIPACLCountWithLimit].mAddr = *aIPACLAddr;
        idlOS::snprintf( mIPACLInfoWithLimit[mIPACLCountWithLimit].mAddrStr, MM_IP_ACL_MAX_ADDR_STR, "%s", aIPACLAddrStr);
        mIPACLInfoWithLimit[mIPACLCountWithLimit].mAddrFamily = aIPACLAddrFamily;
        mIPACLInfoWithLimit[mIPACLCountWithLimit].mMask = aIPACLMask;
        mIPACLInfoWithLimit[mIPACLCountWithLimit].mLimitCount = aIPACLLimitSize;
        mIPACLInfoWithLimit[mIPACLCountWithLimit].mCurConnCount = 0;
        mIPACLCountWithLimit++;
    }
    else
    {
        mIPACLInfo[mIPACLCount].mPermit = aIPACLPermit;
        mIPACLInfo[mIPACLCount].mAddr   = *aIPACLAddr;
        idlOS::snprintf( mIPACLInfo[mIPACLCount].mAddrStr, MM_IP_ACL_MAX_ADDR_STR, "%s", aIPACLAddrStr);
        mIPACLInfo[mIPACLCount].mAddrFamily = aIPACLAddrFamily;
        mIPACLInfo[mIPACLCount].mMask = aIPACLMask;
        mIPACLInfo[mIPACLCount].mLimitCount = aIPACLLimitSize;
        mIPACLInfo[mIPACLCount].mCurConnCount = 0;
        mIPACLCount++;
    }
    sLocked = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_EXCEEDED_ACCESS_LIST_COUNT_LIMIT );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_EXCEEDED_ACCESS_LIST_COUNT_LIMIT ) );
    }
    IDE_EXCEPTION_END;

    if (sLocked == ID_TRUE)
    {
        unlock();
    }
    
    return IDE_FAILURE;
}

/* proj-1538 ipv6 */
static idBool bitsIsSame( void* aSrc1, void* aSrc2, UInt aBitLen )
{
    /* ex) aBitLen: 20 => (8 * 2 sByteLen) + 4 sBitsLeft */
    SInt  sByteLen = aBitLen / 8;
    SInt  sBitsLeft = aBitLen % 8;
    UChar sLastByte1 = 0;
    UChar sLastByte2 = 0;
    UChar sBitMask = 0;
    SInt  sBit = 0;

    /* compare in bytes */
    if (sByteLen > 0)
    {
        IDE_TEST_RAISE(idlOS::memcmp( aSrc1, aSrc2, sByteLen ) != 0,
                       LABEL_BIT_DIFF);
    }
    else
    {
        /* Nothing To Do */
    }

    /* compare the last byte in bits */
    sLastByte1 = *( ((SChar*)aSrc1) + sByteLen);
    sLastByte2 = *( ((SChar*)aSrc2) + sByteLen);
    for (sBit = 7; (sBitsLeft > 0) && (sBit >= 0); sBit--)
    {
        sBitMask = 0x01 << sBit;
        IDE_TEST_RAISE(
            (sLastByte1 & sBitMask) != (sLastByte2 & sBitMask),
            LABEL_BIT_DIFF);
        sBitsLeft--;
    }

    return ID_TRUE;

LABEL_BIT_DIFF:
    return ID_FALSE;
}

/* proj-1538 ipv6
 * there are 3 cases which we have to handle.
 *       client addr     acl addr   server mode
 * 1.    ipv4            ipv4       ipv4
 * 2.    v4mapped ipv6   ipv4       ipv6-dual
 * 3.    ipv6            ipv6       ipv6-dual
 */
/**
 * ACL을 검사해 접속을 허용할지 여부를 확인한다.
 *
 * @param aLink    ACL을 검사할 cmiLink
 * @param aAllowed 접속을 허용할지 여부
 * @param aIPACL   ACL 정보.
 *                 aAllowed가 ID_FALSE 일때 해당 규칙 정보를 담는다.
 *                 aAllowed가 ID_TRUE 일 때는 의미 없음.
 *
 * @return 에러가 발생했으면 IDE_FAILURE, 아니면 IDE_SUCCESS
 */
/* latch는 밖에서 잡았다. */
IDE_RC mmuAccessList::checkIPACL( struct sockaddr_storage  * aAddr,
                                  SChar                    * aAddrStr,
                                  idBool                   * aAllowed )
{
    struct sockaddr*         sAddrCommon = NULL ;
    struct sockaddr_in*      sAddr4 = NULL;
    struct sockaddr_in6*     sAddr6 = NULL;
    UInt                     sAddr4Dst   = 0;
    UInt                     sAddr4Entry = 0;
    UInt*                    sUIntPtr    = NULL;
    UInt                     i = 0;
    idBool                   sIsIPv6Client = ID_FALSE;
    SChar                   *sIPACL = NULL;

    *aAllowed = ID_TRUE; /* default: all clients are allowed */

    /* if no real-entry exist, then the 1st entry is dummy */
    IDE_TEST_CONT( (mIPACLCount + mIPACLCountWithLimit) == 0, LABEL_EMPTY_LIST);

    /* bug-30541: ipv6 code review bug.
     * use sockaddr.sa_family instead of sockaddr_storage.ss_family.
     * because the name is different on AIX 5.3 tl1 as __ss_family
     */
    sAddrCommon = (struct sockaddr*)aAddr;
    /* client is ipv4. it means that server mode is ipv4 only */
    if (sAddrCommon->sa_family == AF_INET)
    {
        sAddr4 = (struct sockaddr_in*)aAddr;
        sAddr4Dst = *((UInt*)&sAddr4->sin_addr);
        sIsIPv6Client = ID_FALSE;
    }
    /* client addr is ipv6 or v4mapped-ipv6 */
    else
    {
        sAddr6 = (struct sockaddr_in6*)aAddr;
        /* if v4mapped-ipv6, we compare it with ipv4s in acl list */
        if (idlOS::in6_is_addr_v4mapped(&(sAddr6->sin6_addr)))
        {
            /* sin6_addr: 16bytes => 4 UInts.
             * ex) ::ffff:127.0.0.1 => extract 127.0.0.1 */
            sUIntPtr = (UInt*)&(sAddr6->sin6_addr);
            sAddr4Dst = *(sUIntPtr + 3);
            sIsIPv6Client = ID_FALSE;
        }
        else
        {
            sIsIPv6Client = ID_TRUE;
        }
    }

    /* fix BUG-28834 IP Access Control List 잘못되었습니다 */
    /* IF BITXOR (BITAND(IP_Packet, mask) , BITAND(address,mask)) */
    /* if ipv4 or v4mapped-ipv6, then sIsIPv6Client is false */
    if (sIsIPv6Client == ID_FALSE)
    {
        /* BUG-48515 일반 access_list와 limit size 설정된 list를 따로 chcek한다. */
        for ( i = 0; i < mIPACLCount; i++ )
        {
            if (mIPACLInfo[i].mAddrFamily == AF_INET)
            {
                sAddr4Entry = *((UInt*)&mIPACLInfo[i].mAddr);

                if ( ((sAddr4Dst & mIPACLInfo[i].mMask) ^ (sAddr4Entry & mIPACLInfo[i].mMask)) == 0 )
                {
                    if ( mIPACLInfo[i].mPermit == ID_TRUE)
                    {
                        *aAllowed = ID_TRUE;
                        sIPACL = NULL;
                        break;
                    }
                    else
                    {
                        *aAllowed = ID_FALSE;
                        /* 별도로 기록된 address string을 반환 */
                         sIPACL = mIPACLInfo[i].mAddrStr;
                    }
                }
            }
        }

        /* BUG-48515 limit size list 체크 */
        for ( i = 0; i < mIPACLCountWithLimit; i++ )
        {
            if (mIPACLInfoWithLimit[i].mAddrFamily == AF_INET)
            {
                sAddr4Entry = *((UInt*)&mIPACLInfoWithLimit[i].mAddr);

                if ( ((sAddr4Dst & mIPACLInfoWithLimit[i].mMask) ^ (sAddr4Entry & mIPACLInfoWithLimit[i].mMask)) == 0 )
                {
                    if ( mIPACLInfoWithLimit[i].mPermit == ID_TRUE)
                    {
                        if ( mIPACLInfoWithLimit[i].mLimitCount > 0 && ((mIPACLInfoWithLimit[i].mCurConnCount + 1) > mIPACLInfoWithLimit[i].mLimitCount) )
                        {
                            *aAllowed = ID_FALSE;
                            sIPACL = mIPACLInfoWithLimit[i].mAddrStr;
                            IDE_CONT(ConnectionLimitError);
                        }

                        mIPACLInfoWithLimit[i].mCurConnCount++;
                        *aAllowed = ID_TRUE;
                        sIPACL = NULL;
                    }
                }
            }
        }


    }
    /* client: ipv6 addr. it is compared to only ipv6 addrs of list */
    else
    {
        for (i = 0; i < mIPACLCount; i++ )
        {
            if (mIPACLInfo[i].mAddrFamily == AF_INET6)
            {
                if (bitsIsSame(&sAddr6->sin6_addr,
                               &(mIPACLInfo[i].mAddr),
                               mIPACLInfo[i].mMask) == ID_TRUE)
                {
                    if ( mIPACLInfo[i].mPermit == ID_TRUE)
                    {
                        *aAllowed = ID_TRUE;
                        sIPACL = NULL;
                        break;
                    }
                    else
                    {
                        *aAllowed = ID_FALSE;
                        /* 별도로 기록된 address string을 반환 */
                         sIPACL = mIPACLInfo[i].mAddrStr;
                    }
                }
            }
        }

        /* BUG-48515 limit size list 체크 */
        for (i = 0; i < mIPACLCountWithLimit; i++ )
        {
            if (mIPACLInfoWithLimit[i].mAddrFamily == AF_INET6)
            {
                if (bitsIsSame(&sAddr6->sin6_addr,
                               &(mIPACLInfoWithLimit[i].mAddr),
                               mIPACLInfoWithLimit[i].mMask) == ID_TRUE)
                {
                    if ( mIPACLInfoWithLimit[i].mPermit == ID_TRUE)
                    {
                        IDE_TEST_RAISE( mIPACLInfoWithLimit[i].mLimitCount > 0 && ((mIPACLInfoWithLimit[i].mCurConnCount + 1) > mIPACLInfoWithLimit[i].mLimitCount), 
                                        ConnectionLimitError );

                       *aAllowed = ID_FALSE;
                        sIPACL = mIPACLInfoWithLimit[i].mAddrStr;
                        IDE_CONT(ConnectionLimitError);
                    }
                    else
                    {
                        mIPACLInfoWithLimit[i].mCurConnCount++;
                        *aAllowed = ID_TRUE;
                        sIPACL = NULL;
                    }
                }
            }
        }
    }

    IDE_TEST_RAISE(*aAllowed == ID_FALSE, ConnectionDenied);

    IDE_EXCEPTION_CONT(LABEL_EMPTY_LIST);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ConnectionDenied)
    {
        /* BUG-46787 */
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IP_ACL_DENIED, sIPACL, aAddrStr));
    }
    /* BUG-48515 */
    IDE_EXCEPTION(ConnectionLimitError)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IP_ACL_CONNECT_OVER, aAddrStr, 
                                                                 mIPACLInfoWithLimit[i].mAddrStr, 
                                                                 mIPACLInfoWithLimit[i].mLimitCount, 
                                                                 mIPACLInfoWithLimit[i].mCurConnCount));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-48515 
 * access_list limit size가 설정되었을 경우 현재 접속된 session count를 차감 */
IDE_RC mmuAccessList::checkIPACLWithSessDecre( struct sockaddr_storage  * aAddr )
{
    struct sockaddr*         sAddrCommon = NULL ;
    struct sockaddr_in*      sAddr4 = NULL;
    struct sockaddr_in6*     sAddr6 = NULL;
    UInt                     sAddr4Dst   = 0;
    UInt                     sAddr4Entry = 0;
    UInt*                    sUIntPtr    = NULL;
    UInt                     i = 0;
    idBool                   sIsIPv6Client = ID_FALSE;

    /* bug-30541: ipv6 code review bug.
     * use sockaddr.sa_family instead of sockaddr_storage.ss_family.
     * because the name is different on AIX 5.3 tl1 as __ss_family
     */
    sAddrCommon = (struct sockaddr*)aAddr;
    /* client is ipv4. it means that server mode is ipv4 only */
    if (sAddrCommon->sa_family == AF_INET)
    {
        sAddr4 = (struct sockaddr_in*)aAddr;
        sAddr4Dst = *((UInt*)&sAddr4->sin_addr);
        sIsIPv6Client = ID_FALSE;
    }
    /* client addr is ipv6 or v4mapped-ipv6 */
    else
    {
        sAddr6 = (struct sockaddr_in6*)aAddr;
        /* if v4mapped-ipv6, we compare it with ipv4s in acl list */
        if (idlOS::in6_is_addr_v4mapped(&(sAddr6->sin6_addr)))
        {
            /* sin6_addr: 16bytes => 4 UInts.
             * ex) ::ffff:127.0.0.1 => extract 127.0.0.1 */
            sUIntPtr = (UInt*)&(sAddr6->sin6_addr);
            sAddr4Dst = *(sUIntPtr + 3);
            sIsIPv6Client = ID_FALSE;
        }
        else
        {
            sIsIPv6Client = ID_TRUE;
        }
    }

    /* fix BUG-28834 IP Access Control List 잘못되었습니다 */
    /* IF BITXOR (BITAND(IP_Packet, mask) , BITAND(address,mask)) */
    /* if ipv4 or v4mapped-ipv6, then sIsIPv6Client is false */
    if (sIsIPv6Client == ID_FALSE)
    {
        for ( i = 0; i < mIPACLCountWithLimit; i++ )
        {
            if (mIPACLInfoWithLimit[i].mAddrFamily == AF_INET)
            {
                sAddr4Entry = *((UInt*)&mIPACLInfoWithLimit[i].mAddr);

                if ( ((sAddr4Dst & mIPACLInfoWithLimit[i].mMask) ^ (sAddr4Entry & mIPACLInfoWithLimit[i].mMask)) == 0 )
                {
                    if ( mIPACLInfoWithLimit[i].mPermit == ID_TRUE)
                    {
                        mIPACLInfoWithLimit[i].mCurConnCount--;
                    }
                }
            }
        }
    }
    /* client: ipv6 addr. it is compared to only ipv6 addrs of list */
    else
    {
        for (i = 0; i < mIPACLCountWithLimit; i++ )
        {
            if (mIPACLInfoWithLimit[i].mAddrFamily == AF_INET6)
            {
                if (bitsIsSame(&sAddr6->sin6_addr,
                               &(mIPACLInfoWithLimit[i].mAddr),
                               mIPACLInfoWithLimit[i].mMask) == ID_TRUE)
                {
                    if ( mIPACLInfoWithLimit[i].mPermit == ID_TRUE)
                    {
                        mIPACLInfoWithLimit[i].mCurConnCount--;
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;
}

/* BUG-48515 
 * A ~ B 가 동일한 ip인지 체크하는 함수 */
IDE_RC mmuAccessList::equalsIPACL( struct sockaddr_storage *aSessionAddr,
                                   struct in6_addr         *aIPACLAddr,
                                   UInt                     aIPACLFamily,
                                   UInt                     aIPACLMask )
{
    struct sockaddr      *sSessionAddr     = NULL;
    struct sockaddr_in   *sSessionAddrIn4  = NULL;
    struct sockaddr_in6  *sSessionAddrIn6  = NULL;

    UInt                  sSessionAddr4Dst   = 0;
    idBool                sIsIPv6Client      = ID_FALSE;

    sSessionAddr = (struct sockaddr*)aSessionAddr;

    if (sSessionAddr->sa_family == AF_INET)
    {
        sSessionAddrIn4 = (struct sockaddr_in*)aSessionAddr;
        sSessionAddr4Dst = *((UInt*)&sSessionAddrIn4->sin_addr);
        sIsIPv6Client = ID_FALSE;
    }
    else
    {
        sSessionAddrIn6 = (struct sockaddr_in6*)aSessionAddr;
        if (idlOS::in6_is_addr_v4mapped(&(sSessionAddrIn6->sin6_addr)))
        {
            sSessionAddr4Dst = *((UInt*)&(sSessionAddrIn6->sin6_addr) + 3);
            sIsIPv6Client = ID_FALSE;
        }
        else
        {
            sIsIPv6Client = ID_TRUE;
        }
    }

    if (sIsIPv6Client == ID_FALSE)
    {
        IDE_TEST( aIPACLFamily != AF_INET);
        IDE_TEST( ((sSessionAddr4Dst & aIPACLMask) ^ ((*((UInt*)aIPACLAddr)) & aIPACLMask)) != 0 );
    }
    else
    {
        IDE_TEST( aIPACLFamily != AF_INET6 );
        IDE_TEST( bitsIsSame(&sSessionAddrIn6->sin6_addr, aIPACLAddr, aIPACLMask) != ID_TRUE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmuAccessList::disconnect(mmcTask *aTask)
{
    struct sockaddr_storage  sAddr;

    cmiLink *sLinkPeer = NULL;
    idBool   sIsRemoteIP = ID_FALSE;
    idBool   sLocked = ID_FALSE;

    lock();
    sLocked = ID_TRUE;
    if ( mIPACLCountWithLimit > 0 )
    {
        sLinkPeer = aTask->getLink();
        if ( (sLinkPeer->mImpl == CMI_LINK_IMPL_TCP) ||
             (sLinkPeer->mImpl == CMI_LINK_IMPL_SSL) ||
             (sLinkPeer->mImpl == CMI_LINK_IMPL_IB) )
        {
            (void) cmiCheckRemoteAccess(sLinkPeer, &sIsRemoteIP);
            if (sIsRemoteIP == ID_TRUE)
            {
                idlOS::memset(&sAddr, 0x00, ID_SIZEOF(sAddr));
                IDE_TEST(cmiGetLinkInfo(sLinkPeer, (SChar *)&sAddr, ID_SIZEOF(sAddr),
                                        CMI_LINK_INFO_REMOTE_SOCKADDR)
                            != IDE_SUCCESS);

                (void) mmuAccessList::checkIPACLWithSessDecre(&sAddr);
            }
        }
    }
    sLocked = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLocked == ID_TRUE )
    {
        unlock();
    }

    return IDE_FAILURE;
}

/* idp::readSPFile()을 참조하여 구현 */

/* ACCESS_LIST_METHOD가 1인 경우는 0일때에 비해
 * access control value를 엄격하게 검사한다.
 */
IDE_RC mmuAccessList::loadAccessList()
{
    SChar            sLineBuf[IDP_MAX_PROP_LINE_SIZE];
    SChar            sMsg[IDP_MAX_PROP_LINE_SIZE];
    FILE            *sFP = NULL;
    idBool           sOpened = ID_FALSE;
    struct in6_addr  sDummyAddr;
    SChar           *sTk = NULL;
    UInt             sLen = 0;
    UInt             sLine = 0;

    mmuIPACLInfo     sIPACLInfo[MM_IP_ACL_MAX_COUNT];
    mmuIPACLInfo     sIPACLInfoWithLimit[MM_IP_ACL_MAX_COUNT];

    UInt             sIndex  = 0;
    UInt             sIndexWithLimit = 0;
    UInt             i = 0;
    UInt             j = 0;

    idBool           sPermit = ID_FALSE;
    struct in6_addr  sAddr;
    UInt             sAddrFamily;
    SChar            sAddrStr[MM_IP_ACL_MAX_ADDR_STR];
    UInt             sMask;
    UInt             sLimitCount;

    UInt             sCount = 0;

    idBool           sIsRemoteIP = ID_FALSE;
    iduList         *sTaskList = NULL;
    iduListNode     *sIterator = NULL;
    iduListNode     *sNextNode = NULL;
    mmcTask         *sTask     = NULL;
    cmiLink         *sLink     = NULL;
    struct sockaddr_storage  sSessionSockAddr;

    /* ACCESS_LIST_FILE 프로퍼티에 지정된 이름이 NULL - error */
    IDE_TEST_RAISE( mmuProperty::mIPACLFile[0] == '\0',
                    ERR_ABORT_RELOAD_ACL_NOT_PERMITTED );

    /* open ACCESS LIST FILE */
    sFP = idlOS::fopen( mmuProperty::mIPACLFile, "r" );

    /* open error */
    IDE_TEST_RAISE( sFP == NULL,
                    ERR_ABORT_ACCESS_LIST_FILE_OPEN_ERROR );
    sOpened = ID_TRUE;

    while ( idlOS::idlOS_feof( sFP ) == 0 )
    {
        idlOS::memset( sLineBuf, 0, IDP_MAX_PROP_LINE_SIZE );

        if ( idlOS::fgets( sLineBuf, IDP_MAX_PROP_LINE_SIZE, sFP ) == NULL )
        {
            /* EOF */
            break;
        }
        else
        {
            sLine++;
        }

        /* remove white space */
        idp::eraseWhiteSpace( sLineBuf );

        if ( (sLineBuf[0] == '#') || (sLineBuf[0] == 0) )
        {
            /* comment */
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST_RAISE( (sIndex + sIndexWithLimit) >= MM_IP_ACL_MAX_COUNT,
                        ERR_ABORT_EXCEEDED_ACCESS_LIST_COUNT_LIMIT );

        /* default setting; initialize */
        sPermit = ID_TRUE;
        idlOS::memset(&sAddr, 0x00, ID_SIZEOF(struct in6_addr));
        sAddrFamily = AF_UNSPEC;
        sAddrStr[0] = '\0';
        sMask = 0;
        sLimitCount = 0;

        idlOS::memset( &sDummyAddr, 0x00, ID_SIZEOF(struct in6_addr) );

        idlOS::snprintf( sMsg,
                         IDP_MAX_PROP_LINE_SIZE,
                         "%s",
                         sLineBuf );

        /* ACCESS_LIST string에 대한 validation 필요 */
        /* ascii 확인 */
        sLen = idlOS::strlen( sLineBuf );
        for ( j = 0; j < sLen; j++ )
        {
            IDE_TEST_RAISE( ((sLineBuf[j]) & (~0x7F)) != 0,
                            ERR_ABORT_INVALID_ACCESS_LIST_VALUE );
        }

        /* 길이 확인 */
        IDE_TEST_RAISE( sLen > IDP_MAX_PROP_STRING_LEN,
                        ERR_ABORT_INVALID_ACCESS_LIST_VALUE );

        /* First Value, "Permit" or "Deny" */
        sTk = idlOS::strtok( sLineBuf, " \t," );
        /* if no entry in the file, sTk: NULL,  mIPACLCount: 1. */
        IDE_TEST_RAISE( sTk == NULL,
                        ERR_ABORT_INVALID_ACCESS_LIST_VALUE );
        if (idlOS::strcasecmp(sTk, "DENY") == 0)
        {
            sPermit = ID_FALSE;
        }
        else if (idlOS::strcasecmp(sTk, "PERMIT") == 0)
        {
            sPermit = ID_TRUE;
        }
        else
        {
            IDE_RAISE( ERR_ABORT_INVALID_ACCESS_LIST_VALUE );
        }

        /* Second Value, IP Address */
        sTk = idlOS::strtok(NULL, " \t,");
        IDE_TEST_RAISE( sTk == NULL,
                        ERR_ABORT_INVALID_ACCESS_LIST_VALUE );
        /* ipv6 addr includes colons */
        if (idlOS::strchr(sTk, ':'))
        {
            /* inet_pton returns a negative value or 0 if error */
            IDE_TEST_RAISE( idlOS::inet_pton(AF_INET6, sTk, &sAddr) <= 0,
                            ERR_ABORT_INVALID_ACCESS_LIST_VALUE );
            sAddrFamily = AF_INET6;
            idlOS::snprintf( sAddrStr,
                             MM_IP_ACL_MAX_ADDR_STR,
                             "%s",
                             sTk );
        }
        /* ipv4 addr */
        else
        {
            /* inet_pton returns a negative value or 0 if error */
            IDE_TEST_RAISE( idlOS::inet_pton(AF_INET, sTk, &sAddr) <= 0,
                            ERR_ABORT_INVALID_ACCESS_LIST_VALUE );
            sAddrFamily = AF_INET;
            idlOS::snprintf( sAddrStr,
                             MM_IP_ACL_MAX_ADDR_STR,
                             "%s",
                             sTk );
        }

        /* Third Value, IP mask
         * ipv4: d.d.d.d mask form
         * ipv6: prefix bit mask length by which to compare */
        sTk = idlOS::strtok(NULL, " \t,");
        IDE_TEST_RAISE( sTk == NULL,
                        ERR_ABORT_INVALID_ACCESS_LIST_VALUE );
        if (sAddrFamily == AF_INET)
        {
            /* inet_pton returns a negative value or 0 if error */
            IDE_TEST_RAISE( idlOS::inet_pton( AF_INET, sTk, &sDummyAddr ) <= 0,
                            ERR_ABORT_INVALID_ACCESS_LIST_VALUE );

            sMask = idlOS::inet_addr(sTk);
        }
        else
        {
            sMask = idlOS::atoi(sTk);
            /* max ipv6 addr bits: 128 */
            IDE_TEST_RAISE( sMask > 128, 
                            ERR_ABORT_INVALID_ACCESS_LIST_VALUE );
        }

        sTk = idlOS::strtok(NULL, " \t,");
        if ( sTk != NULL && sPermit == ID_TRUE )
        {
            /* BUG-48515 ip 최대 허용 size */
            sLimitCount = idlOS::atoi(sTk);
        }
        else
        {
            /* BUG-48515 ip 최대 허용 size가 셋팅하지 않았으므로 0으로 셋팅 */
            sLimitCount = 0;
        }

        if ( sLimitCount > 0 )
        {
            sIPACLInfoWithLimit[sIndexWithLimit].mPermit = sPermit;
            sIPACLInfoWithLimit[sIndexWithLimit].mAddr = sAddr;
            sIPACLInfoWithLimit[sIndexWithLimit].mAddrFamily = sAddrFamily;
            idlOS::snprintf( sIPACLInfoWithLimit[sIndexWithLimit].mAddrStr, MM_IP_ACL_MAX_ADDR_STR, "%s", sAddrStr);
            sIPACLInfoWithLimit[sIndexWithLimit].mMask = sMask;
            sIPACLInfoWithLimit[sIndexWithLimit].mLimitCount = sLimitCount;
            sIPACLInfoWithLimit[sIndexWithLimit].mCurConnCount = 0;
            sIndexWithLimit++;
        }
        else
        {
            sIPACLInfo[sIndex].mPermit = sPermit;
            sIPACLInfo[sIndex].mAddr = sAddr;
            sIPACLInfo[sIndex].mAddrFamily = sAddrFamily;
            idlOS::snprintf( sIPACLInfo[sIndex].mAddrStr, MM_IP_ACL_MAX_ADDR_STR, "%s", sAddrStr);
            sIPACLInfo[sIndex].mMask = sMask;
            sIPACLInfo[sIndex].mLimitCount = 0;
            sIPACLInfo[sIndex].mCurConnCount = 0;
            sIndex++;
        }

        ideLog::log( IDE_SERVER_0, "[ACCESS LIST] LOADED : %s", sMsg );
    }

    sOpened = ID_FALSE;
    IDE_TEST_RAISE( idlOS::fclose( sFP ) != 0,
                    ERR_ABORT_ACCESS_LIST_FILE_CLOSE_ERROR );

    /* BUG-48515 
     * 현재 Session count를 동기화 한다. 
     * limit size를 초과하였더라도 session을 close하지는 않는다 */
    if ( mmtSessionManager::isRun() == ID_TRUE )
    {
        for ( i = 0; i < sIndexWithLimit; i++ )
        {
            mmtSessionManager::lockRead();

            sTaskList = mmtSessionManager::getTaskList();

            IDU_LIST_ITERATE_SAFE( sTaskList, sIterator, sNextNode )
            {
                sTask = (mmcTask *)sIterator->mObj;
                sLink = sTask->getLink();

                if( sLink == NULL)
                {
                    continue;
                }

                if ( (sLink->mImpl == CMI_LINK_IMPL_TCP) ||
                     (sLink->mImpl == CMI_LINK_IMPL_SSL) ||
                     (sLink->mImpl == CMI_LINK_IMPL_IB) )
                {
                    (void) cmiCheckRemoteAccess(sLink, &sIsRemoteIP);
                    if (sIsRemoteIP == ID_TRUE)
                    {
                        idlOS::memset(&sSessionSockAddr, 0x00, ID_SIZEOF(sSessionSockAddr));
                        if ( cmiGetLinkInfo(sLink, (SChar*)&sSessionSockAddr, ID_SIZEOF(sSessionSockAddr), CMI_LINK_INFO_REMOTE_SOCKADDR) != IDE_SUCCESS )
                        {
                            /* BUG-48515 sock addr을 얻어 오지 못하면 무시한다. */
                            continue;
                        }

                        if( equalsIPACL(&sSessionSockAddr, &sIPACLInfoWithLimit[i].mAddr, sIPACLInfoWithLimit[i].mAddrFamily, sIPACLInfoWithLimit[i].mMask) == IDE_SUCCESS )
                        {
                            sIPACLInfoWithLimit[i].mCurConnCount++;
                        }
                    }
                }
            }

            mmtSessionManager::unlock();
        }
    }

    /* 복제 */
    lock();

    for ( i = 0; i < sIndex; i++ )
    {
        mIPACLInfo[i].mPermit      = sIPACLInfo[i].mPermit;
        mIPACLInfo[i].mAddr        = sIPACLInfo[i].mAddr;
        mIPACLInfo[i].mAddrFamily  = sIPACLInfo[i].mAddrFamily;
        idlOS::snprintf( mIPACLInfo[i].mAddrStr, MM_IP_ACL_MAX_ADDR_STR, "%s", sIPACLInfo[i].mAddrStr);
        mIPACLInfo[i].mMask        = sIPACLInfo[i].mMask;
        mIPACLInfo[i].mLimitCount   = sIPACLInfo[i].mLimitCount;
        mIPACLInfo[i].mCurConnCount = sIPACLInfo[i].mCurConnCount;
    }

    for ( i = 0; i < sIndexWithLimit; i++ )
    {
        mIPACLInfoWithLimit[i].mPermit      = sIPACLInfoWithLimit[i].mPermit;
        mIPACLInfoWithLimit[i].mAddr        = sIPACLInfoWithLimit[i].mAddr;
        mIPACLInfoWithLimit[i].mAddrFamily  = sIPACLInfoWithLimit[i].mAddrFamily;
        idlOS::snprintf( mIPACLInfoWithLimit[i].mAddrStr, MM_IP_ACL_MAX_ADDR_STR, "%s", sIPACLInfoWithLimit[i].mAddrStr);
        mIPACLInfoWithLimit[i].mMask        = sIPACLInfoWithLimit[i].mMask;
        mIPACLInfoWithLimit[i].mLimitCount   = sIPACLInfoWithLimit[i].mLimitCount;
        mIPACLInfoWithLimit[i].mCurConnCount = sIPACLInfoWithLimit[i].mCurConnCount;
    }
    mIPACLCount = sIndex;
    mIPACLCountWithLimit = sIndexWithLimit;

    unlock();

    /* log */
    ideLog::log( IDE_SERVER_0, "[ACCESS LIST] %"ID_INT32_FMT" LOADED\n", sCount );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_RELOAD_ACL_NOT_PERMITTED );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_RELOAD_ACL_NOT_PERMITTED ) );
    }
    IDE_EXCEPTION( ERR_ABORT_ACCESS_LIST_FILE_OPEN_ERROR );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INVALID_ACCESS_LIST_FILE,
                                  "open file error",
                                  mmuProperty::mIPACLFile ) );
    }
    IDE_EXCEPTION( ERR_ABORT_ACCESS_LIST_FILE_CLOSE_ERROR );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INVALID_ACCESS_LIST_FILE,
                                  "close file error",
                                  mmuProperty::mIPACLFile ) );
    }
    IDE_EXCEPTION( ERR_ABORT_EXCEEDED_ACCESS_LIST_COUNT_LIMIT );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_EXCEEDED_ACCESS_LIST_COUNT_LIMIT ) );
        ideLog::log( IDE_SERVER_0, "[ACCESS LIST] EXCEEDED MAXIMUM LIMIT" );
    }
    IDE_EXCEPTION( ERR_ABORT_INVALID_ACCESS_LIST_VALUE );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INVALID_ACCESS_LIST_VALUE,
                                  sLine,
                                  sMsg ) );

        /* startup시 error mgr가 로딩되지 않은 상태일 수 있어 추가 기록한다. */
        ideLog::log( IDE_SERVER_0,
                     "[ACCESS LIST] value is not acceptable : "
                     "(Line %"ID_INT32_FMT") %s",
                     sLine,
                     sMsg );
    }
    IDE_EXCEPTION_END;

    if ( sOpened == ID_TRUE )
    {
        idlOS::fclose( sFP );
    }
    else
    {
        /* Nothing To Do */
    }

    return IDE_FAILURE;
}

iduFixedTableColDesc gAccessListColDesc[] =
{
    {
        (SChar *)"ID",
        offsetof(mmuAccessListInfo, mNumber),
        IDU_FT_SIZEOF(mmuAccessListInfo, mNumber),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OPERATION",
        offsetof(mmuAccessListInfo, mOp),
        IDU_FT_SIZEOF(mmuAccessListInfo, mOp),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ADDRESS",
        offsetof(mmuAccessListInfo, mAddress),
        MM_IP_ACL_MAX_ADDR_STR,
        IDU_FT_TYPE_VARCHAR,
        mmuAccessList::convertToChar,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"MASK",
        offsetof(mmuAccessListInfo, mMask),
        MM_IP_ACL_MAX_MASK_STR,
        IDU_FT_TYPE_VARCHAR,
        mmuAccessList::convertToChar,
        0, 0, NULL // for internal use
    },
        {
        (SChar *)"LIMIT",
        offsetof(mmuAccessListInfo, mLimitCount),
        IDU_FT_SIZEOF(mmuAccessListInfo, mLimitCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
        {
        (SChar *)"CONNECTED",
        offsetof(mmuAccessListInfo, mCurConnCount),
        IDU_FT_SIZEOF(mmuAccessListInfo, mCurConnCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    },
};

iduFixedTableDesc gAccessListDesc =
{
    (SChar *)"X$ACCESS_LIST",
    mmuAccessList::buildAccessListRecord,
    gAccessListColDesc,
    IDU_STARTUP_INIT,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* BUG-48515 */
IDE_RC mmuAccessList::buildAccessListRecordAdd( void                * aHeader,
                                                iduFixedTableMemory * aMemory, 
                                                mmuIPACLInfo        * aIPAClInfo,
                                                mmuAccessListInfo   * aACLInfo )
{
    struct in_addr         sAddr;
    
    /* Permit/Deny */
    if ( aIPAClInfo->mPermit == ID_TRUE )
    {
        aACLInfo->mOp = 1;
    }
    else
    {
        aACLInfo->mOp = 0;
    }

    /* Address & Mask */
    if ( aIPAClInfo->mAddrFamily == AF_INET )
    {
        /* AF_INET; IPv4 */
        /* address */
        idlOS::inet_ntop( AF_INET,
                            (void*)(&aIPAClInfo->mAddr),
                            aACLInfo->mAddress,
                            MM_IP_ACL_MAX_ADDR_STR );

        /* mask */
        sAddr.s_addr = aIPAClInfo->mMask;
        idlOS::inet_ntop( AF_INET,
                            (void*)&sAddr,
                            aACLInfo->mMask,
                            MM_IP_ACL_MAX_MASK_STR );
    }
    else
    {
        /* IPv6 */
        /* address */
        idlOS::inet_ntop( AF_INET6,
                            (void*)(&aIPAClInfo->mAddr),
                            aACLInfo->mAddress,
                            MM_IP_ACL_MAX_ADDR_STR );

        /* mask */
        idlOS::snprintf( aACLInfo->mMask,
                            MM_IP_ACL_MAX_MASK_STR,
                            "%"ID_UINT32_FMT"",
                            aIPAClInfo->mMask );
    }

    aACLInfo->mLimitCount = aIPAClInfo->mLimitCount;
    aACLInfo->mCurConnCount = aIPAClInfo->mCurConnCount;

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)(aACLInfo) )
                != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmuAccessList::buildAccessListRecord( idvSQL              * /* aStatistics */,
                                             void                * aHeader,
                                             void                * /* aDumpObj */,
                                             iduFixedTableMemory * aMemory )
{
    mmuAccessListInfo      sACLInfo;
    idBool                 sLocked = ID_FALSE;
    UInt                   i = 0;
    UInt                   sNumber = 1;

    lockRead();
    sLocked = ID_TRUE;

    /* BUG-48515 */
    for ( i = 0; i < mIPACLCount; i++ )
    {
        sACLInfo.mNumber = sNumber;
        IDE_TEST( buildAccessListRecordAdd(aHeader,
                                           aMemory,
                                           &mIPACLInfo[i],
                                           &sACLInfo) != IDE_SUCCESS );
        sNumber++;
    }

    for ( i = 0; i < mIPACLCountWithLimit; i++ )
    {
        sACLInfo.mNumber = sNumber;
        IDE_TEST( buildAccessListRecordAdd(aHeader,
                                           aMemory,
                                           &mIPACLInfoWithLimit[i],
                                           &sACLInfo) != IDE_SUCCESS );
        sNumber++;
    }

    sLocked = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sLocked == ID_TRUE)
    {
        unlock();
    }

    return IDE_FAILURE;
}

UInt mmuAccessList::convertToChar( void   * /* aBaseObj */,
                                   void   * aMember,
                                   UChar  * aBuf,
                                   UInt     aBufSize )
{
    UInt sSize = 0;

    if ( aMember != NULL )
    {
        sSize = idlOS::strlen( (SChar *)aMember );

        if ( sSize > aBufSize )
        {
            sSize = aBufSize;
        }
        else
        {
            /* Nothing To Do */
        }

        idlOS::memcpy( (void*)aBuf, aMember, sSize );
    }
    else
    {
        /* Nothing To Do */
    }

    return sSize;
}