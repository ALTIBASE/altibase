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
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#if !defined( _O_IDU_SHM_CHUNK_MGR_H_ )
#define _O_IDU_SHM_CHUNK_MGR_H_ 1

#include <idl.h>
#include <ide.h>
#include <iduMemListOld.h>

class iduShmChunkMgr
{
public:
    static IDE_RC initialize( ULong aMaxSegCnt );
    static IDE_RC destroy();

    static IDE_RC createSysChunk( UInt         aSize,
                                  iduSCH    ** aNewSCH );

    static IDE_RC allocSHMChunk( UInt          aSize,
                                 iduShmType    aShmType,
                                 iduSCH     ** aNewSCH );

    static IDE_RC freeShmChunk( UInt aSegID );

    /* BUG-40895 Cannot destroy the shared memory segments which is created as other user */
    static IDE_RC changeSHMChunkOwner( PDL_HANDLE aShmID,
                                       uid_t      aOwnerUID );

    static IDE_RC attachAndGetSSegHdr( iduShmSSegment * aSSegHdr );

    static IDE_RC attachAndGetSSegHdr( iduShmSSegment * aSSegHdr,
                                       key_t            aShmKey );

    static IDE_RC attachChunk( key_t      aShmKey4SSeg,
                               UInt       aSize,
                               iduSCH   * aNewSCH );

    static IDE_RC attachChunk( key_t      aShmKey4SSeg,
                               UInt       aSize,
                               iduSCH  ** aNewSCH );

    static IDE_RC detachChunk( UInt aSegID );
    static IDE_RC detachChunk( SChar * aChunkPtr );

    static IDE_RC removeChunck( UInt aSegID );
    static IDE_RC removeChunck( SChar     * aChunkPtr,
                                PDL_HANDLE  aShmID );

    static void registerShmChunk( UInt aIndex, iduSCH * aSCHPtr );

    static void unregisterShmChunk( UInt aIndex );

    static IDE_RC checkExist( key_t     aShmKey,
                              idBool&   aExist );

private:
    static IDE_RC createSHMChunkFromOS( key_t           aShmKey,
                                        ULong           aSize,
                                        PDL_HANDLE    * aShmID,
                                        void         ** aNewChunkPtr );
private:
    static iduSCH  ** mArrSCH;
    static iduMemListOld mSCHPool;
};

#endif /* _O_IDU_SHM_CHUNK_MGR_H_ */
