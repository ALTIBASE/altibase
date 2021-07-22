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
 

#ifndef _O_DKN_LINK_H_
#define _O_DKN_LINK_H_ 1

typedef struct dknLink dknLink;

IDE_RC dknLinkCreate ( dknLink ** aLink );
void dknLinkDestroy ( dknLink * aLink );

IDE_RC dknLinkConnect( dknLink  * aLink,
                       SChar    * aAddress,
                       UShort     aPortNumber,
                       UInt       aTimeoutSec );
IDE_RC dknLinkDisconnect( dknLink * aLink, idBool aReuseFlag );

IDE_RC dknLinkCheck( dknLink * aLink, idBool * aConnectedFlag );
    
IDE_RC dknLinkCheckAndFlush( dknLink * aLink, UInt aLen );

IDE_RC dknLinkWriteOneByteNumber( dknLink * aLink, void * aSource );
IDE_RC dknLinkWriteTwoByteNumber( dknLink * aLink, void * aSource );
IDE_RC dknLinkWriteFourByteNumber( dknLink * aLink, void * aSource );
IDE_RC dknLinkWriteEightByteNumber( dknLink * aLink, void * aSource );
IDE_RC dknLinkWrite( dknLink * aLink, void * aSource, UInt aLength );

IDE_RC dknLinkReadOneByteNumber( dknLink * aLink, void * aDestination );
IDE_RC dknLinkReadTwoByteNumber( dknLink * aLink, void * aDestination );
IDE_RC dknLinkReadFourByteNumber( dknLink * aLink, void * aDestination );
IDE_RC dknLinkReadEightByteNumber( dknLink * aLink, void * aDestination );
IDE_RC dknLinkRead( dknLink * aLink, void * aDestination, UInt aLength );

IDE_RC dknLinkSend( dknLink * aLink );
IDE_RC dknLinkRecv( dknLink * aLink, ULong aTimeoutSec, idBool * aTimeoutFlag );

typedef struct dknPacketInternal dknPacketInternal;

typedef struct dknPacket {
    
    iduListNode mNode;

    dknPacketInternal * mInternal;
    
} dknPacket;

IDE_RC dknPacketCreate( dknPacket ** aPacket, UInt aDataSize );
void dknPacketDestroy( dknPacket * aPacket );

UInt dknPacketGetMaxDataLength( void );

IDE_RC dknPacketWriteOneByteNumber( dknPacket * aPacket, void * aSource );
IDE_RC dknPacketWriteTwoByteNumber( dknPacket * aPacket, void * aSource );
IDE_RC dknPacketWriteFourByteNumber( dknPacket * aPacket, void * aSource );
IDE_RC dknPacketWriteEightByteNumber( dknPacket * aPacket, void * aSource );
IDE_RC dknPacketWrite( dknPacket    * aPacket, 
                       void         * aSource, 
                       UInt           aLength );

UInt dknPacketGetCapacity( dknPacket * aPacket );
UInt dknPacketGetPayloadLength( dknPacket * aPacket );
                         
IDE_RC dknPacketSkip( dknPacket * aPacket, UInt aLength );
void dknPacketRewind( dknPacket * aPacket );

IDE_RC dknLinkSendPacket( dknLink * aLink, dknPacket * aPacket );

#endif /* _O_DKN_LINK_H_ */
