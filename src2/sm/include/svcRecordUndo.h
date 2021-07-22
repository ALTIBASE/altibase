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
 
#ifndef _O_SVC_RECORDUNDO_H_
#define _O_SVC_RECORDUNDO_H_ 1

#include <smDef.h>
#include <svrLogMgr.h>

/* 최대 8바이트까지 physical logging을 할 수 있다. */
typedef struct svcPhysical8Log
{
    svrUndoFunc mUndo;
    SChar     * mDestination;
    UChar       mLength;
    SChar       mData[8];
} svcPhysical8Log;

class svcRecordUndo
{
  public:
    static IDE_RC initializeStatic();

    static IDE_RC logInsert(svrLogEnv  *aLogEnv,
                            void       *aTransPtr,
                            smOID       aTableOID,
                            scSpaceID   aSpaceID,
                            SChar      *aFixedRow);

    static IDE_RC logUpdate(svrLogEnv  *aLogEnv,
                            void       *aTransPtr,
                            smOID       aTableOID,
                            scSpaceID   aSpaceID,
                            SChar      *aOldFixedRow,
                            SChar      *aNewFixedRow,
                            ULong      aBeforeNext);

    static IDE_RC logUpdateInplace( svrLogEnv           *aEnv,
                                    void                *aTrans,
                                    scSpaceID            aSpaceID,
                                    smOID                aTableOID,
                                    SChar               *aFixedRowPtr,
                                    const smiColumnList *aColumnList,
                                    ULong                aModifyIdxBit );

    static IDE_RC logDelete(svrLogEnv *aLogEnv,
                            void      *aTransPtr,
                            smOID      aTableOID,
                            SChar     *aRowPtr,
                            ULong      aNextVersion);

    inline static IDE_RC logPhysical8( svrLogEnv * aEnv,
                                       SChar     * aRow,
                                       UInt        aLength );

    inline static IDE_RC undoPhysical8( svrLogEnv * /*aLogEnv*/,
                                        svrLog    * aPhyLog,
                                        svrLSN     /*aSubLSN*/ );
};

/******************************************************************************
 * Description:
 *     physical log를 기록한다.
 ******************************************************************************/
inline IDE_RC svcRecordUndo::logPhysical8( svrLogEnv  * aEnv,
                                           SChar      * aRow,
                                           UInt         aLength )
{
    svcPhysical8Log sPhyLog;

    IDE_ASSERT( aLength <= 8 );

    sPhyLog.mUndo           = svcRecordUndo::undoPhysical8;
    sPhyLog.mDestination    = aRow;
    sPhyLog.mLength         = aLength;
    idlOS::memcpy( sPhyLog.mData,
                   aRow,
                   aLength );

    return ( svrLogMgr::writeLog( aEnv,
                                  (svrLog *)&sPhyLog,
                                  ID_SIZEOF( sPhyLog ) ) );
}

/******************************************************************************
 * Description:
 *    physical log로 undo를 수행한다. 데이터의 길이는 8이하이다.
 ******************************************************************************/
inline IDE_RC svcRecordUndo::undoPhysical8( svrLogEnv * /*aLogEnv*/,
                                            svrLog    * aPhyLog,
                                            svrLSN     /*aSubLSN*/ )
{
    svcPhysical8Log * sPhyLog = (svcPhysical8Log *)aPhyLog;

    IDE_ASSERT( sPhyLog->mLength <= 8 );

    idlOS::memcpy( sPhyLog->mDestination,
                   sPhyLog->mData,
                   sPhyLog->mLength );

    return IDE_SUCCESS;
}

#endif /* _O_SVC_RECORDUNDO_H_ */
