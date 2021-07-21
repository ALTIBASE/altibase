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
 
package com.altibase.altilinker.jcm;

import java.net.Socket;
import java.nio.ByteBuffer;

import com.altibase.altilinker.jcm.PacketHeader;

/**
 * JAVA communication module observer interface
 */
public interface JCommModuleObserver
{
    /**
     * New connected client event
     * 
     * @param aJcmMoudle        JCommModule object
     * @param aSocket           New connected client socket
     * @return                  true, if new client socket accepts. false, if decline.
     */
    public boolean onNewClient(JCommModule aJcmMoudle, Socket aSocket);
    
    /**
     * Disconnection event for client socket
     * 
     * @param aJcmModule        JCommModule object
     * @param aSocket           Disconnected client socket
     */
    public void onClientDisconnected(JCommModule aJcmModule, Socket aSocket);
    
    /**
     * A received CM block
     * 
     * @param aJcmModule        JCommModule object
     * @param aSocket           Client socket
     * @param aPacketHeader     Packet header which is analyzed aCmBlock
     * @param aCmBlock          A CM block
     */
    public void onReceivedPacket(JCommModule  aJcmModule,
                                 Socket       aSocket,
                                 PacketHeader aPacketHeader,
                                 ByteBuffer   aCmBlock);
}
