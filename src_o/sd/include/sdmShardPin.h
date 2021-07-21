/** 
 *  Copyright (c) 1999~2018, Altibase Corp. and/or its affiliates. All rights reserved.
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
 

/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _O_SDM_SHARD_PIN_H_
#define _O_SDM_SHARD_PIN_H_ 1

#include <sdi.h>


/**************************************************************
                       BASIC DEFINITIONS
 **************************************************************/
typedef enum {
    SDM_SHARD_PIN_VER_INVALID = 0,
    SDM_SHARD_PIN_VER_1       = 1,
} sdmShardPinVersion;

typedef enum {
    SDM_SHARD_PIN_RESEREVED = 0,
} sdmShardPinReserved;

/**************************************************************
                  STRUCTURE DEFINITION
 **************************************************************/
typedef struct sdmShardNodeInfo
{
    /* Meta Node Info for shard pin */
    sdiShardNodeId           mShardNodeId;
} sdmShardNodeInfo;

typedef struct sdmShardPinInfo
{
    UChar                   mVersion;
    UChar                   mReserved;
    sdmShardNodeInfo        mShardNodeInfo;
    UInt                    mSeq;
} sdmShardPinInfo;


/**************************************************************
                  CLASS DEFINITION
 **************************************************************/
class sdmShardPinMgr
{
  public:
    static void         initialize();
    static void         finalize();
    static void         loadShardPinInfo();
    static void         updateShardPinInfoOnRuntime( sdiLocalMetaInfo *aMetaNodeInfo );
    static sdiShardPin  getNewShardPin();
    static void         shardPinToString( SChar *aDst, UInt aLen, sdiShardPin aShardPin );

  private:
    static inline idBool      isSupport();
    static inline UChar       getShardPinVersion();
    static inline void        initShardPinInfo( sdmShardPinInfo *aShardPinInfo );
    static inline void        initShardNodeID( sdmShardNodeInfo *aMetaNodeInfo );
    static inline sdiShardPin toShardPin( sdmShardPinInfo *aShardPinInfo );
    static inline void        toShardPinInfo( sdmShardPinInfo *aShardPinInfo, sdiShardPin aShardPin );
    static inline void        copyShardPinInfo( sdmShardPinInfo *aDst, sdmShardPinInfo *aSrc );
    static inline void        copyShardNodeID( sdmShardNodeInfo *aDst, sdiLocalMetaInfo *aSrc );
    static inline idBool      isValidShardPinInfo( sdmShardPinInfo *aShardPinInfo );


  private:
    static sdmShardPinInfo mShardPinInfo;
};


#endif /* _O_SDM_SHARD_PIN_H_ */
