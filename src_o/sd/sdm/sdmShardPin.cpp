#include <sdi.h>
#include <sdm.h>
#include <sdmShardPin.h>

sdmShardPinInfo sdmShardPinMgr::mShardPinInfo;

idBool sdmShardPinMgr::isSupport()
{
    return ( ( sdi::isShardEnable() == ID_TRUE ) ? ID_TRUE : ID_FALSE );
}

UChar sdmShardPinMgr::getShardPinVersion()
{
    return SDM_SHARD_PIN_VER_1;
}

void sdmShardPinMgr::initialize()
{
    initShardPinInfo( &mShardPinInfo );

    /* Current time of millisecond */
    mShardPinInfo.mSeq = (UInt)( acpTimeNow() / 1000 );
}

void sdmShardPinMgr::finalize()
{
    /* Nothing to do */
}

void sdmShardPinMgr::initShardPinInfo( sdmShardPinInfo *aShardPinInfo )
{
    aShardPinInfo->mVersion         = (UChar)SDM_SHARD_PIN_VER_INVALID;
    aShardPinInfo->mReserved        = (UChar)SDM_SHARD_PIN_RESEREVED;
    aShardPinInfo->mSeq             = 0;

    initShardNodeID( &aShardPinInfo->mShardNodeInfo );
}

void sdmShardPinMgr::initShardNodeID( sdmShardNodeInfo *aMetaNodeInfo )
{
    aMetaNodeInfo->mShardNodeId      = SDM_DEFAULT_SHARD_NODE_ID;
}

void sdmShardPinMgr::loadShardPinInfo()
{
    sdiLocalMetaInfo sLocalMetaInfo;

    IDE_TEST_CONT( isSupport()             == ID_FALSE, NO_SHARD_ENABLE );
    IDE_TEST_CONT( sdi::checkMetaCreated() == ID_FALSE, NO_SHARD_ENABLE );

    if ( sdm::getLocalMetaInfo( &sLocalMetaInfo ) == IDE_SUCCESS )
    {
        if ( sLocalMetaInfo.mShardNodeId != SDM_DEFAULT_SHARD_NODE_ID )
        {
            copyShardNodeID( &mShardPinInfo.mShardNodeInfo, &sLocalMetaInfo );
        }
        else
        {
            initShardNodeID( &mShardPinInfo.mShardNodeInfo );
        }
    }
    else
    {
        initShardNodeID( &mShardPinInfo.mShardNodeInfo );
    }

    mShardPinInfo.mVersion = getShardPinVersion();

    IDE_EXCEPTION_CONT( NO_SHARD_ENABLE );
}

sdiShardPin sdmShardPinMgr::toShardPin( sdmShardPinInfo *aShardPinInfo )
{
    sdiShardPin sShardPin;

    sShardPin =   ( ( (sdiShardPin)aShardPinInfo->mVersion  & 0xff       ) << SDI_OFFSET_VERSION      )    /* << 7 byte */
                | ( ( (sdiShardPin)aShardPinInfo->mReserved & 0xff       ) << SDI_OFFSET_RESERVED     )    /* << 6 byte */
                | ( ( (sdiShardPin)aShardPinInfo->mShardNodeInfo.mShardNodeId
                                                            & 0xffff     ) << SDI_OFFSET_SHARD_NODE_ID )    /* << 4 byte */
                | ( ( (sdiShardPin)aShardPinInfo->mSeq      & 0xffffffff ) << SDI_OFFSET_SEQEUNCE     );   /* << 0 byte */

    return sShardPin;
}

void sdmShardPinMgr::updateShardPinInfoOnRuntime( sdiLocalMetaInfo *aLocalMetaInfo )
{
    copyShardNodeID( &mShardPinInfo.mShardNodeInfo, aLocalMetaInfo );
}

sdiShardPin sdmShardPinMgr::getNewShardPin()
{
    sdmShardPinInfo sShardPinInfo;
    sdiShardPin     sRetShardPin = SDI_SHARD_PIN_INVALID;

    copyShardPinInfo( &sShardPinInfo, &mShardPinInfo );

    if ( isValidShardPinInfo( &sShardPinInfo ) == ID_TRUE )
    {
        sShardPinInfo.mSeq = acpAtomicInc32( &mShardPinInfo.mSeq );

        sRetShardPin = toShardPin( &sShardPinInfo );
    }

    return sRetShardPin;
}

void sdmShardPinMgr::shardPinToString( SChar *aDst, UInt aLen, sdiShardPin aShardPin )
{
    idlOS::snprintf( aDst,
                     aLen,
                     SDI_SHARD_PIN_FORMAT_STR,
                     SDI_SHARD_PIN_FORMAT_ARG( aShardPin ) );
}

void sdmShardPinMgr::copyShardPinInfo( sdmShardPinInfo *aDst, sdmShardPinInfo *aSrc )
{
    *aDst = *aSrc;
}

void sdmShardPinMgr::copyShardNodeID( sdmShardNodeInfo *aDst, sdiLocalMetaInfo *aSrc )
{
    aDst->mShardNodeId = aSrc->mShardNodeId;
}

idBool sdmShardPinMgr::isValidShardPinInfo( sdmShardPinInfo *aShardPinInfo )
{
    return ( ( aShardPinInfo->mVersion != SDM_SHARD_PIN_VER_INVALID ) ? ID_TRUE : ID_FALSE );
}
