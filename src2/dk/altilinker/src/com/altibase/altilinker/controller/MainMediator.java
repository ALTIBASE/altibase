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
 
package com.altibase.altilinker.controller;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.type.Properties;

/**
 * Main mediator interface
 */
public interface MainMediator
{
    /**
     * Set product info event from lower components
     * 
     * @param aProductInfo      Product info
     * @param aDBCharSet        Database character set of ALTIBASE server
     */
    public void onSetProductInfo(String aProductInfo, int aDBCharSet);
    
    /**
     * Load properties event for lower components 
     * 
     * @param aProperties       dblink.conf properties
     */
    public void onLoadProperties(Properties aProperties);
    
    /**
     * Get ADLP protocol version event from lower components
     * 
     * @return  ADLP protocol version
     */
    public int onGetProtocolVersion();
    
    /**
     * Created linker control session event from lower components
     */
    public void onCreatedLinkerCtrlSession();
    
    /**
     * Created linker data session event from lower components
     * 
     * @param aSessionId        Linker data session ID
     */
    public void onCreatedLinkerDataSession(int aSessionId);
     
    /**
     * Destroy linker session event from lower components
     * 
     * @param aSessionId        Linker session Id
     * @param aForce            true, if remote statement is alive and stop force.
     */
    public boolean onDestroyLinkerSession(int aSessionId, boolean aForce);
    
    /**
     * Get linker status event from lower components
     * 
     * @return Linker status
     */
    public int onGetLinkerStatus();
    
    /**
     * Prepare to shutdown ALTILINKER event from lower components
     * 
     * @return true, if prepare to shutdown. false, if fail.
     */
    public boolean onPrepareLinkerShutdown();
    
    /**
     * Shutdown ALTILINKER event from lower components
     */
    public void onDoLinkerShutdown();
    
    /**
     * Disconnected linker session from lower components
     * 
     * @param aSessionId        Linker session ID
     */
    public void onDisconnectedLinkerSession(int aSessionId);
    
    /**
     * Operation send event from lower components
     * 
     * @param aOperation        Operation to send
     * 
     * @return true, if sending is success. false, if fail.
     */
    public boolean onSendOperation(Operation aOperation);
    
    /**
     * Received operation event from lower components
     * 
     * @param aOperation        Received operation
     */
    public boolean onReceivedOperation(Operation aOperation);
    
    /**
     * Get Thread Dump Log Information
    * @return true, if prepare to shutdown. false, if fail.
     */
    public boolean onGetThreadDumpLog();  
    
    /**
     *  BUG-39001
     *  Un-assign to a thread for linker session
     *  
     *  @param aLinkerSessionId  Linker session ID
     */
    public void onUnassignToThreadPool( int aSessionId );
    
    /**
     *  BUG-39001
     *  Drop Task from TaskThread
     *  
     *  @param aLinkerSessionId  Linker session ID
     */
    public void onDropTask( int aLinkerSessionID );

}
