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
 
package com.altibase.altilinker.adlp;

import java.net.Socket;
import java.nio.ByteBuffer;
import java.text.MessageFormat;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;

import com.altibase.altilinker.adlp.op.OpId;
import com.altibase.altilinker.adlp.type.ResultCode;
import com.altibase.altilinker.jcm.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.util.TraceLogger;

/**
 * Protocol manager component class to analyze ADLP protocol
 */
public class ProtocolManager implements JCommModuleObserver
{
    /**
     * Linker session class for communication related data structures by linker session  
     */
    protected static class LinkerSession
    {
        public int          mSessionId                    = -1;
        public Socket       mSocket                       = null;

        // for read
        public Operation    mOperationForReassembly       = null;
        public ByteBuffer   mOpPayloadForReassembly       = null;
        public int          mOpPayloadLengthForReassembly = 0;
        public byte[]       mFragmentedOpPayload          = null;
        
        // for write
        public CommonHeader mWriteCommonHeader            = null;
        public ByteBuffer   mWriteCmBlock                 = null; // with packet header
        public ByteBuffer   mOpPayload                    = null;
        public byte[]       mFragmentOpPayload            = null;
       
        private long        mSeqNo                        = 0;
        
        /**
         * Constructor
         */
        public LinkerSession()
        {
            mWriteCommonHeader = new CommonHeader();
            mWriteCmBlock      = ByteBuffer.allocateDirect(MaxCmBlockSize);
            mOpPayload         = ByteBuffer.allocateDirect(MaxOpPayloadSize);
            mFragmentOpPayload = new byte[MaxOpPayloadSize];
        }

        /**
         * Ensure operation payload buffer for fragmentation
         * 
         * @param aResultOperation          Result operation
         */
        public void reallocOpPaylodBuffer(ResultOperation aResultOperation)
        {
            int sNeedOpPayloadSize = aResultOperation.getOpPayloadSize();
            
            if (mOpPayload.capacity() < sNeedOpPayloadSize)
            {
                int sNewAllocSize = 
                        ((sNeedOpPayloadSize / MaxOpPayloadSize) + 1) * MaxOpPayloadSize;
                
                mOpPayload = ByteBuffer.allocateDirect(sNewAllocSize);
            }
        }

        /**
         * Reset cursor of operation payload buffer for write
         */
        public void resetCursorOfOpPayloadForWrite()
        {
            int sMaxSize = mOpPayload.capacity();
            
            mOpPayload.position(0);
            mOpPayload.limit(sMaxSize);
        }
        
        /**
         * Reset cursor of operation payload buffer for read
         */
        public void resetCursorOfOpPayloadForRead()
        {
            int sOpPayloadSize = mOpPayload.position();
            
            mOpPayload.position(0);
            mOpPayload.limit(sOpPayloadSize);
        }
        
        
        /**
         * Set network disconnected
         */
        public void setDisconnected()
        {
            mSocket = null;

//            mOperationForReassembly = null;
//            if (mOpPayloadForReassembly != null)
//                mOpPayloadForReassembly.clear();
//            mOpPayloadLengthForReassembly = 0;
//            
//            mWriteCmBlock.clear();
//            mOpPayload.clear();
        }

        /**
         * Set network connected
         * 
         * @param aSocket       The accepted socket instance 
         */
        public void setConnected(Socket aSocket)
        {
            mSocket = aSocket;
        }
        
        /**
         * Close for this instance
         */
        public void close()
        {
            setDisconnected();
            
            mSessionId           = -1;

            mFragmentedOpPayload = null;
            
            mFragmentOpPayload   = null;
        }
        
        /**
         * Whether network connected or disconnected
         * 
         * @return      true, if network connected. false, if else.
         */
        public boolean isConnected()
        {
            return (mSocket != null) ? true : false;
        }
        
        /**
         * Whether network connection equals or not
         * 
         * @param aSocket       Comparison target socket
         * @return              true, if equals. false, if not.
         */
        public boolean equalConnection(Socket aSocket)
        {
            if (aSocket == null)
            {
                return false;
            }
            
            if (mSocket == null)
            {
                return false;
            }
            
            return mSocket.equals(aSocket);            
        }
        
        /**
         * BUG-39201
         * Set SeqNo
         * 
         * @param aSeqNo
         */
        public void setSeqNo( long aSeqNo )
        {
            mSeqNo = aSeqNo;
        }
        
        /**
         * BUG-39201
         * get SeqNo
         * 
         * @return SeqNo
         */
        public long getSeqNo()
        {
            return mSeqNo;
        }
    }
    
    private static final short ProtocolMajorVersion   = 1;  
    private static final short ProtocolMinorVersion   = 0;  
    private static final int   MaxCmBlockSize         = JCommModule.getMaxCmBlockSize();
    private static final int   PacketHeaderSize       = JCommModule.getPacketHeaderSize();
    private static final int   MaxOpPayloadSize       = MaxCmBlockSize - (PacketHeaderSize + CommonHeader.HeaderSize); 

    private static TraceLogger      mTraceLogger      = TraceLogger.getInstance();
    private static OperationFactory mOperationFactory = OperationFactory.getInstance();
    
    private MainMediator mMainMediator     = null;
    private int          mSelectTimeout    = 0;
    private JCommModule  mJCommModule      = null;
    private HashMap      mLinkerSessionMap = null;

    private CommonHeader mReadCommonHeader = null;
    
    /**
     * Constructor
     * 
     * @param aMainMediator     MainMediator instance for pushing events of this instance
     */
    public ProtocolManager(MainMediator aMainMediator)
    {
        mTraceLogger.logTrace("ProtocolManager component created.");

        mMainMediator = aMainMediator;
    }
    
    /**
     * Close for this instance
     */
    public void close()
    {
        stopListen();
        
        mTraceLogger.logTrace("ProtocolManager component closed.");
    }
    
    /**
     * Get major version of ADLP protocol
     * 
     * @return  Major version of ADLP protocol
     */
    public static short getProtocolMajorVersion()
    {
        return ProtocolMajorVersion;
    }
    
    /**
     * Get minor version of ADLP protocol
     * 
     * @return  Minor version of ADLP protocol
     */
    public static short getProtocolMinorVersion()
    {
        return ProtocolMinorVersion;
    }

    /**
     * Get ADLP protocol version
     * 
     * @return  ADLP protocol version
     */
    public static int getProtocolVersion()
    {
        return (ProtocolMajorVersion << 16) + ProtocolMinorVersion;
    }

    /**
     * Get ADLP protocol version
     * 
     * @return  ADLP protocol version
     */
    public static String getProtocolVersionString()
    {
        Object[] sArgs = {
                new Integer(ProtocolMajorVersion),
                new Integer(ProtocolMinorVersion),
        };
        
        String sProtocolVersion = MessageFormat.format("{0}.{1}", sArgs);

        return sProtocolVersion;
    }
    
    /**
     * Get maximum CM block size
     * 
     * @return  Maximum CM block size
     */
    public static int getMaxCmBlockSize()
    {
        return MaxCmBlockSize;
    }
    
    /**
     * Get packet header size for a CM block
     * 
     * @return  Packet header size for a CM block
     */
    public static int getPacketHeaderSize()
    {
        return PacketHeaderSize;
    }
    
    /**
     * Get common header size for a operation
     * 
     * @return  Common header size for a operation
     */
    public static int getCommonHeaderSize()
    {
        return CommonHeader.HeaderSize;
    }
    
    /**
     * Get maximum operation payload size
     * 
     * @return  Maximum operation payload size
     */
    public static int getMaxOpPayloadSize()
    {
        return MaxOpPayloadSize;
    }

    /**
     * Set timeout for socket event polling
     * 
     * @param aTimeout  Timeout socket event polling
     */
    public void setSelectTimeout(int aTimeout)
    {
        mSelectTimeout = aTimeout;
    }
    
    /**
     * Start network service
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean start()
    {
        if (mJCommModule != null)
        {
            return true;
        }

        if (mMainMediator == null)
        {
            mTraceLogger.logError("MainMediator is null");
            
            return false;
        }

        mLinkerSessionMap = new HashMap();
        mReadCommonHeader = new CommonHeader();
        
        mJCommModule = new JCommModule();
        mJCommModule.setObserver(this);
        mJCommModule.setSelectTimeout(mSelectTimeout);
        
        if (mJCommModule.start() == false)
        {
            mJCommModule.close();
            mJCommModule = null;
            
            mLinkerSessionMap = null;
            mReadCommonHeader = null;
            
            return false;
        }
        
        mTraceLogger.logDebug("ProtocolManager started");
        
        return true;
    }
    
    /**
     * Start TCP listening for server side
     * 
     * @param aPort             TCP listen port
     * @param aBindAddress      Bind address
     * @return                  true, if success. false, if fail.
     */
    public boolean startListen(int aPort, String aBindAddress)
    {
        if (mJCommModule == null)
        {
            return false;
        }
        
        if (mMainMediator == null)
        {
            mTraceLogger.logError("MainMediator is null");
            
            return false;
        }

        boolean sSuccess = false;
        
        sSuccess = mJCommModule.startListen(aPort, aBindAddress);
        
        return sSuccess;
    }
    
    /**
     * Stop TCP listening
     */
    public void stopListen()
    {
        if (mJCommModule != null)
        {
            mJCommModule.stopListen();
        }
    }

    /**
     * Stop network service
     * 
     * @return  true, if success. false, if fail.
     */
    public void stop()
    {        
        if (mJCommModule != null)
        {
            mJCommModule.stop();
            mJCommModule.close();
            mJCommModule = null;
        }

        synchronized (this)
        {
            mLinkerSessionMap = null;
        }
        
        mReadCommonHeader = null;

        mTraceLogger.logDebug("ProtocolManager stopped");
    }

    /**
     * Send a operation
     * 
     * @param aOperation        Operation to send
     * @return                  true, if send successfully. false, if fail to send.
     */
    public boolean sendOperation(Operation aOperation)
    {
        ResultOperation sResultOperation     = (ResultOperation)aOperation;
        int             sSessionId           = aOperation.getSessionId();
        LinkerSession   sLinkerSession       = null;
        Socket          sLinkerSessionSocket = null;
        boolean         sFragmentable        = false;

        // get linker session
        sLinkerSession = getLinkerSession(sSessionId);
        
        if (sLinkerSession == null)
        {
            mTraceLogger.logError(
                    "Unknown linker session (Linker Session Id = {0}, Opration Id = {1})",
                    new Object[] {
                            new Integer(sSessionId),
                            new Integer(aOperation.getSessionId()) });
            
            return false;
        }
        
        /*
         * BUG-39201
         * check Old Result Operation
         */
        if ( sLinkerSession.getSeqNo() != aOperation.getSeqNo() )
        {
            mTraceLogger.logInfo( "The result was discarded. " +
            		               "( Linker Session ID = {0}, Operation ID = {1}, Seq No(Link Session : {2}, Operation : {3} ) ",
            		               new Object [] {
                                       new Integer( sSessionId ),
                                       new Integer( aOperation.getOperationId() ),
                                       new Long( sLinkerSession.getSeqNo() ),
                                       new Long( aOperation.getSeqNo() ) } );            
            
            return false;
        }
        else
        {
            /* do nothing */
        }

        // get socket to send a operation
        sLinkerSessionSocket = sLinkerSession.mSocket;
        
        if (sLinkerSessionSocket == null)
        {
            mTraceLogger.logWarning(
                    "Network disconnected for linker session (Linker Session Id = {0}, Opration Id = {1})",
                    new Object[] {
                            new Integer(sSessionId),
                            new Integer(aOperation.getSessionId()) });
            
            return false;
        }
        
        sFragmentable = sResultOperation.isFragmentable();

        // ensure operation payload buffer
        if (sFragmentable == true)
        {
            sLinkerSession.reallocOpPaylodBuffer(sResultOperation);
        }

        // reset cursor of operation payload buffer for write
        sLinkerSession.resetCursorOfOpPayloadForWrite();

        // write operation payload and fill common header
        if (sResultOperation.writeOperation(sLinkerSession.mWriteCommonHeader,
                                            sLinkerSession.mOpPayload) == false)
        {
            mTraceLogger.logFatal(
                    "ProtocolManager could not write operation. (Linker Session Id = {0}, Operation Id = {1})",
                    new Object[] {
                            new Integer(aOperation.getSessionId()),
                            new Integer(aOperation.getOperationId()) });
            
            System.exit(-1);
            
            return false;
        }

        // reset cursor of operation payload buffer for read
        sLinkerSession.resetCursorOfOpPayloadForRead();
        
        // send CM block(s)
        boolean sSuccess = false;
        
        if (sFragmentable == true)
        {
            // fragment and send CM block(s)
            sSuccess = sendFragmentableOperation(sLinkerSession,
                                                 sLinkerSessionSocket,
                                                 sResultOperation);
        }
        else
        {
            // send a CM block
            sSuccess = sendNormalOperation(sLinkerSession,
                                           sLinkerSessionSocket);
        }

        if (sSuccess == true)
        {
            if (mTraceLogger.isLoggable(TraceLogger.LogLevelDebug) == true)
            {
                mTraceLogger.logDebug(
                        "Operation sent successfully. (Linker Session Id = {0}, Operation Id = {1}, Socket = {2})",
                        new Object[] {
                                new Integer(aOperation.getSessionId()),
                                new Integer(aOperation.getOperationId()),
                                sLinkerSessionSocket.toString() } );
            }
        }
        else
        {            
            mTraceLogger.logError(
                    "Operation failed to send. (Linker Session Id = {0}, Operation Id = {1}, Socket = {2})",
                    new Object[] {
                            new Integer(aOperation.getSessionId()),
                            new Integer(aOperation.getOperationId()),
                            sLinkerSessionSocket.toString() } );
        }

        return sSuccess;
    }

    /**
     * Send a normal (not-fragementable) operation
     * 
     * @param aLinkerSession            Linker session
     * @param aLinkerSessionSocket      Socket of linker session
     * @return                          true, if success. false, if fail.
     */
    private boolean sendNormalOperation(LinkerSession aLinkerSession,
                                        Socket        aLinkerSessionSocket)
    {
        boolean sSuccess       = false;
        int     sOpPayloadSize = aLinkerSession.mOpPayload.limit();
    
        aLinkerSession.mOpPayload.get(aLinkerSession.mFragmentOpPayload,
                                      0,
                                      sOpPayloadSize);

        // reset to write common header and operation payload
        aLinkerSession.mWriteCmBlock.limit(aLinkerSession.mWriteCmBlock.capacity());
        aLinkerSession.mWriteCmBlock.position(PacketHeaderSize);

        // set data length to common header and write common header
        aLinkerSession.mWriteCommonHeader.setDataLength(sOpPayloadSize);
        aLinkerSession.mWriteCommonHeader.write(aLinkerSession.mWriteCmBlock);

        // write fragment operation payload
        aLinkerSession.mWriteCmBlock.put(aLinkerSession.mFragmentOpPayload,
                                         0,
                                         sOpPayloadSize);

        // reset to read in JCM 
        aLinkerSession.mWriteCmBlock.limit(aLinkerSession.mWriteCmBlock.position());
        aLinkerSession.mWriteCmBlock.position(0);
        
        // send cm block
        if (mJCommModule.sendCmBlock(aLinkerSessionSocket,
                                     aLinkerSession.mWriteCmBlock) == true)
        {
            sSuccess = true;
        }

        return sSuccess;
    }
    
    /**
     * Send a fragmentable operation
     * 
     * @param aLinkerSession            Linker session
     * @param aLinkerSessionSocket      Socket of linker session
     * @param aResultOperation          Result operation
     * @return                          true, if success. false, if fail.
     */
    private boolean sendFragmentableOperation(LinkerSession   aLinkerSession,
                                              Socket          aLinkerSessionSocket,
                                              ResultOperation aResultOperation)
    {
        boolean    sSuccess             = false;
        int        sRemainedPayloadSize = aLinkerSession.mOpPayload.limit();
        int        sFragmentPayloadSize = 0;
        int        sSentPayloadSize     = 0;
        LinkedList sFragmentPosHint     = null;
        
        // get fragment position hint, if fragmentable operation
        sFragmentPosHint = aResultOperation.getFragmentPosHint();
        
        do
        {
            if (sRemainedPayloadSize > MaxOpPayloadSize)
            {
                sFragmentPayloadSize =
                        getFragmentPayloadSize(sFragmentPosHint,
                                               sSentPayloadSize,
                                               MaxOpPayloadSize);
            }
            else
            {
                sFragmentPayloadSize = sRemainedPayloadSize;
            }
            
            aLinkerSession.mOpPayload.get(aLinkerSession.mFragmentOpPayload,
                                          0,
                                          sFragmentPayloadSize);

            // reset to write common header and operation payload
            aLinkerSession.mWriteCmBlock.limit(aLinkerSession.mWriteCmBlock.capacity());
            aLinkerSession.mWriteCmBlock.position(PacketHeaderSize);

            // set data length to common header and write common header
            aLinkerSession.mWriteCommonHeader.setDataLength(sFragmentPayloadSize);
            aLinkerSession.mWriteCommonHeader.write(aLinkerSession.mWriteCmBlock);

            // write fragment operation payload
            aLinkerSession.mWriteCmBlock.put(aLinkerSession.mFragmentOpPayload,
                                             0,
                                             sFragmentPayloadSize);

            // send cm block
            aLinkerSession.mWriteCmBlock.limit(aLinkerSession.mWriteCmBlock.position());
            aLinkerSession.mWriteCmBlock.position(0);
            
            if (mJCommModule.sendCmBlock(aLinkerSessionSocket,
                                         aLinkerSession.mWriteCmBlock) == false)
            {
                sSuccess = false;
                break;
            }

            sSentPayloadSize     += sFragmentPayloadSize;
            sRemainedPayloadSize -= sFragmentPayloadSize;
            
            sSuccess = true;
            
        } while (sRemainedPayloadSize > 0);

        return sSuccess;
    }
    
    /**
     * Get payload size to be fragmented by fragment position hint
     * 
     * @param aFragmentPosHint  Fragment position hint list
     * @param aSentPayloadSize  Sent payload size
     * @param aMaxPayloadSize   Maximum payload size
     * @return                  Payload size to be fragmented
     */
    static private int getFragmentPayloadSize(LinkedList aFragmentPosHint,
                                              int        aSentPayloadSize,
                                              final int  aMaxPayloadSize)
    {
        if (aFragmentPosHint == null)
        {
            return aMaxPayloadSize;
        }

        if (aFragmentPosHint.size() == 0)
        {
            return aMaxPayloadSize;
        }

        int      sFragmentPayloadSize = 0;
        int      sBeginPayloadPos     = aSentPayloadSize;
        int      sMaxPayloadPos       = sBeginPayloadPos + aMaxPayloadSize;
        Iterator sPreviousIterator    = null;
        Iterator sIterator;
        Integer  sPos;

        sIterator = aFragmentPosHint.iterator();
        while (sIterator.hasNext() == true)
        {
            sPreviousIterator = sIterator;
            
            sPos = (Integer)sIterator.next();

            if (sPos.intValue() <= sMaxPayloadPos)
            {
                sFragmentPayloadSize = sPos.intValue() - sBeginPayloadPos;

                sPreviousIterator.remove();
            }
            else
            {
                break;
            }
        }
        
        if (sFragmentPayloadSize == 0)
        {
            sFragmentPayloadSize = aMaxPayloadSize;
        }

        return sFragmentPayloadSize;
    }
    
    /**
     * New client event of JCommModule class instance
     */
    public boolean onNewClient(JCommModule aJcmMoudle, Socket aSocket)
    {
        mTraceLogger.logDebug(
                "New client connected. (Socket = {0})",
                new Object[] { aSocket.toString() } );
        
        return true;
    }

    /**
     * Client disconnected event of JCommModule class instance
     * 
     * Clear network connection related information of linker session
     */
    public void onClientDisconnected(JCommModule aJcmModule, Socket aSocket)
    {
        mTraceLogger.logDebug(
                "Client disconnected. (Socket = {0})", new Object[] { aSocket.toString() } );
        
        LinkerSession sLinkerSession = null;
        
        synchronized (this)
        {
            Iterator sIterator = mLinkerSessionMap.entrySet().iterator();
            while (sIterator.hasNext() == true)
            {
                Map.Entry sEntry = (Map.Entry)sIterator.next();

                LinkerSession sLinkerSessionForIterator = (LinkerSession)sEntry.getValue();

                if (sLinkerSessionForIterator.mSocket == aSocket)
                {
                    sLinkerSession = sLinkerSessionForIterator;
                    break;
                }
            }
        }
        
        if (sLinkerSession != null)
        {
            // clear network connection related information
            sLinkerSession.setDisconnected();
            
            if (mMainMediator != null)
            {
                mMainMediator.onDisconnectedLinkerSession(
                        sLinkerSession.mSessionId);
            }
            
            sLinkerSession = null;
        }
        else
        {
            // not connected or already disconnected linker session
        }
    }
    
    /**
     * Whether connected or not linker session
     *  
     * @param aSessionId        Linker session ID
     * @return                  true, if connected. false, if disconnected.
     */
    public boolean isConnectedLinkerSession(int aSessionId)
    {
        LinkerSession sLinkerSession = null;
        
        synchronized (this)
        {
            Integer sKey = new Integer(aSessionId);

            sLinkerSession = (LinkerSession)mLinkerSessionMap.remove(sKey);
        }
        
        if (sLinkerSession == null)
            return false;
        
        return sLinkerSession.isConnected();
    }

    /**
     * Destroy linker session 
     * 
     * @param aSessionId        Linker session ID
     */
    public void destroyLinkerSession(int aSessionId)
    {
        LinkerSession sLinkerSession = null;
        
        synchronized (this)
        {
            Integer sKey = new Integer(aSessionId);

            sLinkerSession = (LinkerSession)mLinkerSessionMap.remove(sKey);
        }
        
        if (sLinkerSession != null)
        {
            sLinkerSession.close();
            sLinkerSession = null;
        }
    }
    
    /**
     * Packet receive event of JCommModule class instance
     * Re-assemble one or multiple operation(s) from one or multiple CM block(s)
     * 
     * Operation type through CM block.
     *   1) Multiple operation per a CM block
     *   2) Multiple CM block for one operation (fragmentation)
     * 
     * @param aJcmMoudle        JCommModule
     * @param aSocket           Client socket
     * @param aPacketHeader     Packet header of aCmBlock
     * @param aCmBlock          A received CM block
     */
    public void onReceivedPacket(JCommModule  aJcmModule,
                                 Socket       aSocket,
                                 PacketHeader aPacketHeader,
                                 ByteBuffer   aCmBlock)
    {
        LinkerSession sLinkerSession = null;

        do
        {
            // read common header (10bytes) from a CM block
            mReadCommonHeader.read(aCmBlock);

            if (mTraceLogger.isLoggable(TraceLogger.LogLevelDebug) == true)
            {
                mTraceLogger.logDebug(
                        "Protocol Manager read common header (10bytes) a CM block. (Linker Session Id = {0}, Operation Id = {1}, Socket = {2})",
                        new Object[] {
                                new Integer(mReadCommonHeader.getSessionId()),
                                new Integer(mReadCommonHeader.getOperationId()),
                                aSocket.toString() } );
            }
            
            int sOpPayloadPosition = aCmBlock.position();
            int sOpPayloadLength   = mReadCommonHeader.mDataLength;
            
            // find or new linker session
            sLinkerSession = findAndNewLinkerSession(mReadCommonHeader, aSocket);
            
            // unknown session
            if (sLinkerSession == null)
            {
                mTraceLogger.logError(
                        "Unknown linker session. (Linker Session Id = {0}, Operation Id = {1})",
                        new Object[] {
                                new Integer(mReadCommonHeader.mSessionId),
                                new Integer(mReadCommonHeader.mOperationId) });

                sendUnknownSession(aSocket, mReadCommonHeader);

                aCmBlock.position(aCmBlock.position() + mReadCommonHeader.mDataLength);

                continue;
            }
            else
            {
                /*
                 * BUG-39201
                 * set SeqNo 
                 */
                sLinkerSession.setSeqNo( aPacketHeader.getSeqNo() );
                
                if (sLinkerSession.isConnected() == true)
                {
                    if (sLinkerSession.equalConnection(aSocket) == false)
                    {
                        sendAlreadyCreatedSession(aSocket, mReadCommonHeader);

                        aCmBlock.position(aCmBlock.position() + mReadCommonHeader.mDataLength);
                        
                        continue;
                    }
                    else
                    {
                        // equaled socket
                    }
                }
                else
                {
                    // already disconnected
                    
                    sLinkerSession.setConnected(aSocket);
                }
            }

            Operation sOperation            = null;
            boolean   sReassembledOperation = false;

            // check to reassemble operation
            if (sLinkerSession.mOperationForReassembly != null)
            {
                if (sLinkerSession.mOperationForReassembly.getOperationId()
                                              == mReadCommonHeader.mOperationId)
                {
                    sOperation = sLinkerSession.mOperationForReassembly;
                    sReassembledOperation = true;
                }
                else
                {
                    // reset, because it is not continuous fragmented operation.
                    sLinkerSession.mOperationForReassembly = null;
                    sLinkerSession.mOpPayloadForReassembly.clear();
                }
            }

            // allocate new operation, if no reassembly operation
            if (sOperation == null)
            {
                sOperation =
                        mOperationFactory.newOperation(
                                mReadCommonHeader.mOperationId);
                
                if (sOperation == null)
                {
                    mTraceLogger.logError(
                            "Unknown operation. (Linker Session Id = {0}, Operation Id = {1})",
                            new Object[] {
                                    new Integer(mReadCommonHeader.mSessionId),
                                    new Integer(mReadCommonHeader.mOperationId) });

                    sendUnknownOperation(mReadCommonHeader);
    
                    aCmBlock.position(aCmBlock.position() + mReadCommonHeader.mDataLength);
    
                    continue;
                }
            }
            
            RequestOperation sRequestOperation = (RequestOperation)sOperation;
            ByteBuffer       sOpPayload        = aCmBlock;

            // check to potential fragmentation operation
            if (sRequestOperation.isFragmentable() == true)
            {
                boolean sFragmentedOperation =
                        sRequestOperation.isFragmented(mReadCommonHeader);

                // reassembled operations or fragmented operation
                if (sReassembledOperation == true || sFragmentedOperation == true)
                {
                    // reassembe for multiple fragmented operation
                    reassembleOperation(sLinkerSession,
                                        aPacketHeader,
                                        aCmBlock);

                    // check end of fragmentation
                    if (sFragmentedOperation == true)
                    {
                        sLinkerSession.mOperationForReassembly = sRequestOperation;
                        
                        continue;
                    }
                    
                    // end of fragmentation
                    mReadCommonHeader.mDataLength =
                            sLinkerSession.mOpPayloadForReassembly.position();
                    
                    sOpPayload = sLinkerSession.mOpPayloadForReassembly;
                    
                    sOpPayload.position(0);
                }
            }

            // read and analyze operation payload
            boolean sSuccessOperation = 
                    sRequestOperation.readOperation(mReadCommonHeader,
                                                    sOpPayload);
            
            if (sSuccessOperation == false)
            {
                // send result operation for request operation that is consisted of wrong packet
                sendWrongPacketOperation(sRequestOperation);
            }
            else
            {
                // validate request operation value(s)
                if (sRequestOperation.validate() == false)
                {
                    // send result operation for request operation that is consisted of out of range value(s)
                    sendOutOfRangeOperation(sRequestOperation);
                    
                    sSuccessOperation = false;
                }
            }

            // clear reassemble related variables
            if (sReassembledOperation == true)
            {
                sLinkerSession.mOperationForReassembly = null;
                sLinkerSession.mOpPayloadForReassembly.clear();
                sLinkerSession.mOpPayloadLengthForReassembly = 0;
            }

            // move explicitly buffer position
            aCmBlock.position(sOpPayloadPosition + sOpPayloadLength);
            
            if (sSuccessOperation == false)
            {
                continue;
            }
            
            if (mMainMediator != null)
            {
                /*
                 * BUG-39201
                 * Set SeqNo
                 */
                sOperation.setSeqNo( aPacketHeader.getSeqNo() );
                
                if (mMainMediator.onReceivedOperation(sOperation) == false)
                {
                    // nothing to do
                }
                
                sOperation.close();
            }
            else
            {
                sOperation.close();
            }
            
        } while ((PacketHeader.HeaderSize + aPacketHeader.getPayloadLength()) > aCmBlock.position());
    }

    private LinkerSession getLinkerSession(int aSessionId)
    {
        LinkerSession sLinkerSession;
        
        synchronized (this)
        {
            sLinkerSession = 
                    (LinkerSession)mLinkerSessionMap.get(
                            new Integer(aSessionId));
        }
        
        return sLinkerSession;
    }
    
    private LinkerSession findAndNewLinkerSession(CommonHeader aCommonHeader, Socket aSocket)
    {
        LinkerSession sLinkerSession;
        
        synchronized (this)
        {
            Integer sKey = new Integer(aCommonHeader.mSessionId);

            sLinkerSession = (LinkerSession)mLinkerSessionMap.get(sKey);
            if (sLinkerSession == null)
            {
                if (aCommonHeader.mOperationId == OpId.CreateLinkerCtrlSession ||
                    aCommonHeader.mOperationId == OpId.CreateLinkerDataSession)
                {
                    mTraceLogger.logDebug(
                            "New linker session. (Linker Session Id = {0}, Socket = {1})",
                            new Object[] {
                                  new Integer(aCommonHeader.mSessionId),
                                  aSocket.toString() });
                    
                    sLinkerSession = new LinkerSession();
                    sLinkerSession.mSessionId = aCommonHeader.mSessionId;
                    sLinkerSession.mSocket    = aSocket;

                    mLinkerSessionMap.put(sKey, sLinkerSession);
                }
                else
                {
                    mTraceLogger.logWarning(
                            "Operation can execute after creating linker session. (Linker Session Id = {0}, Socket = {1})",
                            new Object[] {
                                    new Integer(aCommonHeader.mSessionId),
                                    aSocket.toString() });
                }
            }
        }
        
        return sLinkerSession;
    }

    /**
     * Reassemble operation
     * 
     * @param
     * @return  true, if complete reassembly. false, if need more operation.
     */
    private void reassembleOperation(LinkerSession aLinkerSession,
                                     PacketHeader  aPacketHeader,
                                     ByteBuffer    aCmBlock)
    {
        // ensure buffer for reassembly operations
        if (aLinkerSession.mOpPayloadForReassembly == null)
        {
            aLinkerSession.mOpPayloadLengthForReassembly = mReadCommonHeader.mDataLength;

            aLinkerSession.mOpPayloadForReassembly =
                    ByteBuffer.allocateDirect(MaxCmBlockSize * 2);
        }
        else
        {
            if (aLinkerSession.mOpPayloadForReassembly.remaining()
                                        < mReadCommonHeader.mDataLength)
            {
                ByteBuffer sOldBuffer = aLinkerSession.mOpPayloadForReassembly;
                
                sOldBuffer.limit(sOldBuffer.position());
                sOldBuffer.position(0);
                
                int sNewAllocSize = sOldBuffer.capacity() + (MaxCmBlockSize * 2);
                
                aLinkerSession.mOpPayloadForReassembly =
                        ByteBuffer.allocateDirect(sNewAllocSize);
                
                aLinkerSession.mOpPayloadForReassembly.put(sOldBuffer);
            }
        }

        // ensure temporary buffer for one fragmented operation payload
        if (aLinkerSession.mFragmentedOpPayload == null)
        {
            aLinkerSession.mFragmentedOpPayload = new byte[MaxCmBlockSize];
        }

        // copy one fragemented operation payload to reassembly operation payload
        int sFragmentedOpPayloadLength =
                aPacketHeader.getPayloadLength() - CommonHeader.HeaderSize;
        
        aCmBlock.get(aLinkerSession.mFragmentedOpPayload,
                     0,
                     sFragmentedOpPayloadLength);
        
        aLinkerSession.mOpPayloadForReassembly.put(aLinkerSession.mFragmentedOpPayload,
                                                   0,
                                                   sFragmentedOpPayloadLength);
    }
    
    /**
     * Send unknown session operation
     * 
     * @param aSocket           Socket instance to send
     * @param aReadCommonHeader The read common header
     */
    protected void sendUnknownSession(Socket aSocket, CommonHeader aReadCommonHeader)
    {
        ResultOperation sResultOperation =
                (ResultOperation)mOperationFactory.newResultOperation(
                        aReadCommonHeader.mOperationId);
        
        if (sResultOperation == null)
        {
            mTraceLogger.logFatal(
                            "Result operation could not allocate. (Linker Session Id = {0}, Request Operation Id = {1})",
                            new Object[] {
                                    new Integer(aReadCommonHeader.getSessionId()),
                                    new Integer(aReadCommonHeader.getOperationId())});

            System.exit(-1);
            
            return;
        }

        ByteBuffer   sWriteCmBlock      = ByteBuffer.allocateDirect(PacketHeaderSize + CommonHeader.HeaderSize);
        CommonHeader sWriteCommonHeader = new CommonHeader();
        
        sWriteCmBlock.limit(sWriteCmBlock.capacity());
        sWriteCmBlock.position(PacketHeaderSize);

        // write common header from unknown operation
        sResultOperation.setSessionId(aReadCommonHeader.mSessionId);
        sResultOperation.setResultCode(ResultCode.UnknownSession);
        sResultOperation.writeOperation(sWriteCommonHeader, null);
        
        // write common header to CM block
        sWriteCommonHeader.setDataLength(0);
        sWriteCommonHeader.write(sWriteCmBlock);

        sWriteCmBlock.limit(sWriteCmBlock.position());
        sWriteCmBlock.position(0);
        
        if (mJCommModule.sendCmBlock(aSocket, sWriteCmBlock) == false)
        {
            mTraceLogger.logError(
                    "Failed to send unknown operation. (Socket = {0})",
                    new Object[] { aSocket.toString() } );
        }
        
        sResultOperation.close();
    }
    
    /**
     * Send unknown operation
     * 
     * @param aReadCommonHeader The read common header
     */
    protected void sendUnknownOperation(CommonHeader aReadCommonHeader)
    {
        Operation sUnknownOperation =
                mOperationFactory.newOperation(OpId.UknownOperation);
        
        if (sUnknownOperation == null)
        {
            mTraceLogger.logFatal(
                    "Unknown operation could not allocate. (Linker Session Id = {0}, Operation Id = {1})",
                    new Object[] {
                            new Integer(aReadCommonHeader.getSessionId()),
                            new Integer(aReadCommonHeader.getOperationId())});

            System.exit(-1);
            
            return;
        }

        sUnknownOperation.setSessionId(aReadCommonHeader.mSessionId);
        
        sendOperation(sUnknownOperation);

        sUnknownOperation.close();
    }
    
    /**
     * Send wrong packet operation
     * 
     * @param aRequestOperation The request operation
     */
    protected void sendWrongPacketOperation(RequestOperation aRequestOperation)
    {
        // wrong packet
        Operation sWrongPacketOperation = aRequestOperation.newResultOperation();
        if (sWrongPacketOperation == null)
        {
            mTraceLogger.logFatal(
                    "Result operation could not allocate. (Linker Session Id = {0}, Request Operation Id = {1})",
                    new Object[] {
                            new Integer(aRequestOperation.getSessionId()),
                            new Integer(aRequestOperation.getOperationId()) });
            
            System.exit(-1);
            
            return;
        }

        sWrongPacketOperation.setResultCode(ResultCode.WrongPacket);
        sWrongPacketOperation.setSessionId(aRequestOperation.getSessionId());
        
        sendOperation(sWrongPacketOperation);
        
        sWrongPacketOperation.close();
    }
    
    /**
     * Send out of range operation
     * 
     * @param aRequestOperation The request operation
     */
    protected void sendOutOfRangeOperation(RequestOperation aRequestOperation)
    {
        // invalidated operation
        Operation sResultOperation = aRequestOperation.newResultOperation();
        if (sResultOperation == null)
        {
            mTraceLogger.logFatal(
                    "Result operation could not allocate. (Linker Session Id = {0}, Request Operation Id = {1})",
                    new Object[] {
                            new Integer(aRequestOperation.getSessionId()),
                            new Integer(aRequestOperation.getOperationId()) });
            
            System.exit(-1);
            
            return;
        }

        sResultOperation.setResultCode(ResultCode.OutOfRange);
        sResultOperation.setSessionId(aRequestOperation.getSessionId());
        sResultOperation.setSeqNo( aRequestOperation.getSeqNo() );
        
        sendOperation(sResultOperation);
        
        sResultOperation.close();
    }
    
    protected void sendAlreadyCreatedSession(Socket aSocket, CommonHeader aReadCommonHeader)
    {
        ResultOperation sResultOperation =
                (ResultOperation)mOperationFactory.newOperation(OpId.CreateLinkerDataSessionResult);
        
        if (sResultOperation == null)
        {
            mTraceLogger.logFatal(
                    "Result operation could not allocate. (Linker Session Id = {0}, Request Operation Id = {1})",
                    new Object[] {
                            new Integer(aReadCommonHeader.getSessionId()),
                            new Integer(aReadCommonHeader.getOperationId()) });
            
            System.exit(-1);
            
            return;
        }

        ByteBuffer   sWriteCmBlock      = ByteBuffer.allocateDirect(PacketHeaderSize + CommonHeader.HeaderSize);
        CommonHeader sWriteCommonHeader = new CommonHeader();
        
        sWriteCmBlock.limit(sWriteCmBlock.capacity());
        sWriteCmBlock.position(PacketHeaderSize);

        // write common header from unknown operation
        sResultOperation.setSessionId(aReadCommonHeader.mSessionId);
        sResultOperation.setResultCode(ResultCode.AlreadyCreatedSession);
        sResultOperation.writeOperation(sWriteCommonHeader, null);
        
        // write common header to CM block
        sWriteCommonHeader.setDataLength(0);
        sWriteCommonHeader.write(sWriteCmBlock);

        sWriteCmBlock.limit(sWriteCmBlock.position());
        sWriteCmBlock.position(0);
        
        if (mJCommModule.sendCmBlock(aSocket, sWriteCmBlock) == false)
        {
            mTraceLogger.logError(
                    "Failed to send unknown operation. (Socket = {0})",
                    new Object[] { aSocket.toString() } );
        }
        
        sResultOperation.close();
    }
}
