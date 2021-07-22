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

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import com.altibase.altilinker.util.TraceLogger;

/**
 * JAVA communication module component class to communicate with ALTIBASE server 
 */
public class JCommModule
{
    /**
     * Session id generator of packet
     */
    protected static class SessionIdGenerator
    {
        private boolean[] mSessionIdArray = new boolean[0xffff];

        /**
         * Generate session id of packet
         * 
         * @return      Session id of packet
         */
        public int generate()
        {
            int sSessionId = 0;

            int i;
            for (i = 0; i < mSessionIdArray.length; ++i)
            {
                if (mSessionIdArray[i] == false)
                {
                    mSessionIdArray[i] = true;
                    
                    sSessionId = i + 1;
                    
                    break;
                }
            }
            
            return sSessionId;
        }
        
        /**
         * Release the generated session id 
         * 
         * @param aSessionId    Session id to release
         */
        public void release(int aSessionId)
        {
            if (aSessionId <= 0 || aSessionId > mSessionIdArray.length)
            {
                return;
            }
            
            mSessionIdArray[aSessionId - 1] = false;
        }
    }
    
    /**
     * Client class 
     */
    protected static class Client
    {
        public Socket       mSocket            = null;

        // for read
        public ByteBuffer   mReadBuffer        = null;
        public PacketHeader mReadPacketHeader  = null;

        // for write
        public PacketHeader mWritePacketHeader = null;
        
        /**
         * Constructor
         */
        public Client()
        {
            mReadBuffer        = ByteBuffer.allocateDirect(MaxCmBlockSize);
            mReadPacketHeader  = new PacketHeader();
            
            mWritePacketHeader = new PacketHeader();
        }
        
        /**
         * Close for this object
         */
        public void close()
        {            
            mReadBuffer        = null;
            mReadPacketHeader  = null;
            
            mWritePacketHeader = null;
        }
    }

    private static final long   DefaultWaitTimeForNewClient = 1000; // 1 sec
    private static final long   DefaultSelectTimeout = 100; // 100ms
    private static final int    MaxCmBlockSize       = 32 * 1024; // 32KB

    static private TraceLogger  mTraceLogger         = TraceLogger.getInstance();
    
    private JCommModuleObserver mObserver            = null;
    private int                 mListenPort          = 0;
    private long                mSelectTimeout       = DefaultSelectTimeout;

    private boolean             mStarted             = false;
    private boolean             mListened            = false;
    private Selector            mSelector            = null;
    private ServerSocketChannel mServerSocketChannel = null;
    private ServerSocket        mServerSocket        = null;
    private Listener            mListener            = null;
    private HashMap             mClientMap           = null;
    
    private SessionIdGenerator  mSessionIdGenerator  = null;

    /**
     * Constructor
     */
    public JCommModule()
    {
        mTraceLogger.logInfo("JCommModule component created.");
    }
    
    /**
     * Close for this component
     */
    public void close()
    {
        stopListen();
        
        mTraceLogger.logInfo("JCommModule component closed.");
    }
    
    /**
     * Get TCP listen port
     * 
     * @return TCP listen port 
     */
    public int getListenPort()
    {
        return mListenPort;
    }

    /**
     * Set observer object to receive event of JCM
     * 
     * @param aObserver Observer object
     */
    public void setObserver(JCommModuleObserver aObserver)
    {
        mObserver = aObserver;
    }

    /**
     * Set select timeout for socket event polling
     * 
     * @param aTimeout  Select timeout for socket event polling
     */
    public void setSelectTimeout(long aTimeout)
    {
        long sTimeout = aTimeout;
        if (sTimeout <= 0)
        {
            sTimeout = DefaultSelectTimeout;
        }

        mSelectTimeout = sTimeout;
    }
    
    /**
     * Start network service
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean start()
    {
        // check already to start        
        if (mStarted == true)
        {
            return true;
        }
        
        boolean sSuccess = false;

        mTraceLogger.logDebug(
                "Select timeout is {0}.",
                new Object[] { new Long(mSelectTimeout) } );

        try
        {
            // open selector for socket event polling
            mSelector = Selector.open();

            mClientMap = new HashMap();
            
            // create listener and start listen thread
            mListener = new Listener();
            
            Thread sThread = new Thread(mListener);
            sThread.setDaemon(true);
            sThread.start();
            
            mTraceLogger.logDebug("Listener thread started.");

            mStarted = true;

            sSuccess = true;

            mTraceLogger.logDebug("JCommModule started.");
        }
        catch (IOException e)
        {
            mTraceLogger.log(e);

            if (mSelector != null)
            {
                try
                {
                    mSelector.close();
                }
                catch (IOException e1)
                {
                    mTraceLogger.log(e1);
                }
            }
            
            mSelector  = null;
            mClientMap = null;
            mListener  = null;
        }

        return sSuccess;
    }
    
    /**
     * Start TCP listening
     * 
     * @param aPort             Listen port
     * @param aBindAddress      Bind address
     * @return                  true, if success. false, if fail.
     */
    public boolean startListen(int aPort, String aBindAddress)
    {
        // check to stop
        if (mStarted == false)
        {
            return false;
        }
        
        // check already to listen
        if (mListened == true)
        {
            return true;
        }
        
        boolean sSuccess = false;

        try
        {
            mSessionIdGenerator = new SessionIdGenerator();
            
            // if bind address is null or empty,
            // then that bind loop-back interface(localhost).
            String sBindAddress = aBindAddress;
            if (sBindAddress == null || sBindAddress.length() == 0)
            {
                sBindAddress = "127.0.0.1";
            }
            
            InetSocketAddress sInetSocketAddress =
                    new InetSocketAddress(sBindAddress, aPort);

            mTraceLogger.logInfo("Bind address = " + sInetSocketAddress);

            // open server socket channel and set non-blocking socket option
            mServerSocketChannel = ServerSocketChannel.open();
            mServerSocket = mServerSocketChannel.socket();
            
            // TODO
            mServerSocketChannel.configureBlocking(false);

            // set SO_REUSEADDR socket option
            mServerSocket.setReuseAddress(true);
            
            // bind ethernet interface
            mServerSocket.bind(sInetSocketAddress);

            // register server socket to selector for listening
            mServerSocketChannel.register(mSelector, SelectionKey.OP_ACCEPT);

            mListenPort = aPort;
            
            mListened = true;
            
            sSuccess = true;

            mTraceLogger.logDebug("TCP listening started.");
        }
        catch (IOException e)
        {
            mTraceLogger.log(e);
            
            if (mServerSocketChannel != null)
            {
                try
                {
                    mServerSocketChannel.close();
                }
                catch (IOException e1)
                {
                    mTraceLogger.log(e1);
                }
            }
            
            if (mServerSocket != null)
            {
                try
                {
                    mServerSocket.close();
                }
                catch (IOException e1)
                {
                    mTraceLogger.log(e1);
                }
            }
            
            mServerSocketChannel = null;
            mServerSocket        = null;
            mSessionIdGenerator  = null;
        }

        return sSuccess;
    }
    
    /**
     * Stop TCP listening
     */
    public void stopListen()
    {
        if (mListened == false)
        {
            return;
        }
        
        try
        {
            // unregister for selector
            SelectionKey sSelectionKey = mServerSocketChannel.keyFor(mSelector);
            if (sSelectionKey != null)
            {
                sSelectionKey.cancel();
            }
            
            mServerSocketChannel.close();
            mServerSocket.close();
        }
        catch (IOException e)
        {
            mTraceLogger.log(e);
        }

        mTraceLogger.logDebug("TCP listening stopped.");
        
        mServerSocketChannel = null;
        mServerSocket        = null;
        mSessionIdGenerator  = null;
        
        mListened = false;
    }
    
    /**
     * Stop network service
     */
    public void stop()
    {
        // check to start
        if (mStarted == true)
        {
            return;
        }
        
        // check to listen
        if (mListened == true)
        {
            stopListen();
        }
        
        try
        {
            mTraceLogger.logDebug("Wait to stop listener thread.");

            // stop listen thread
            if (mListener != null)
            {
                mListener.stop();
                
                while (mListener.isStopped() == false)
                {
                    Thread.sleep(100);
                }
            }

            mTraceLogger.logDebug("Listener thread stopped.");
            
            // disconnect all of clients
            disconnectAllClients();

            mSelector.close();
        }
        catch (IOException e)
        {
            mTraceLogger.log(e);
        }
        catch (InterruptedException e)
        {
            mTraceLogger.log(e);
        }

        mTraceLogger.logDebug("JCommModule stopped.");
        
        mSelector  = null;
        mClientMap = null;
        mListener  = null;
        
        mStarted = false;
    }

    /**
     * TCP listener class
     */
    private class Listener implements Runnable
    {
        private volatile boolean mStop    = false;
        private volatile boolean mStopped = false;

        /**
         * Thread run method
         */
        public void run()
        {
            long sWaitTimeForNewClient = DefaultWaitTimeForNewClient;

            while (mStop == false)
            {
                try
                {
                    // because of system call reduction and performance
                    if (mSelector.keys().size() == 0)
                    {
                        Thread.sleep(mSelectTimeout);
                        continue;
                    }
                    
                    // select server and client socket for timeout
                    mSelector.select(mSelectTimeout);

                    Iterator sIterator;
                    SelectionKey sSelectionKey;
                    
                    sIterator = mSelector.selectedKeys().iterator();
                    while (sIterator.hasNext() == true)
                    {
                        sSelectionKey = (SelectionKey)sIterator.next();
                        
                        if (sSelectionKey.isAcceptable() == true)
                        {
                            // accept for client socket
                            if ( accept(sSelectionKey) == false )
                            {
                                Thread.sleep( sWaitTimeForNewClient );
                            }
                        }
                        else if (sSelectionKey.isReadable() == true)
                        {
                            if (read(sSelectionKey) < 0)
                            {
                                // if return value of read function is smaller than 0,
                                // then this socket is disconnected.
                                close(sSelectionKey);
                            }
                        }
                        else if (sSelectionKey.isWritable() == true)
                        {
                            // nothing to do
                        }
                        else
                        {
                            close(sSelectionKey);
                        }

                        sIterator.remove();
                    }
                }
                catch (Exception e)
                {
                    mTraceLogger.log(e);
                }
            }
            
            mStopped = true;
        }
        
        /**
         * Stop thread
         */
        public void stop()
        {
            mStop = true;
        }
        
        /**
         * Check thread stop state
         * 
         * @return      true, if stopped. false, if running.
         */
        public boolean isStopped()
        {
            return mStopped;
        }
    }

    /**
     * Accept client socket
     * 
     * @param aSelectionKey     Selection key
     * @return                  true, if accept. false, if decline.
     */
    private boolean accept(SelectionKey aSelectionKey)
    {
        ServerSocketChannel sServerSocketChannel = null;
        SocketChannel       sSocketChannel       = null;
        Socket              sSocket              = null;
        int                 sSessionId           = 0;
        int                 sClientCount         = 0;
        boolean             sIsAccepted          = false;
        boolean             sIsGeneratedID       = false;
        
        sServerSocketChannel = (ServerSocketChannel)aSelectionKey.channel();
        
        try
        {
            sSocketChannel = sServerSocketChannel.accept();
            if ( sSocketChannel == null )
            {
                return false;
            }
            
            sIsAccepted = true;
            
            mTraceLogger.logDebug(
                    "New accepted client socket. (Socket = {0})",
                    new Object[] { sSocketChannel.toString() } );
            
            // generate session ID of CM packet header
            sSessionId = mSessionIdGenerator.generate();
            if ( sSessionId == 0 )
            {
                // if session ID is 0, then acceptable client is full.
                
                mTraceLogger.logError(
                        "The acceptable client socket is full. This socket will disconnect. (Socket = {0})",
                        new Object[] { sSocketChannel.toString() } );
                
                throw new Exception( "Can not generate ID" );
            }
            sIsGeneratedID = true;
            
            // set non-blocking socket option
            sSocketChannel.configureBlocking(false);
            
            // register new client socket to selector 
            sSocketChannel.register(mSelector, SelectionKey.OP_READ);
            
            Client sClient = new Client();

            sSocket = sSocketChannel.socket();
            
            sClient.mSocket = sSocket;
            sClient.mWritePacketHeader.setSessionId(sSessionId);
            
            synchronized (this)
            {
                mClientMap.put(sSocket, sClient);

                sClientCount = mClientMap.size();

            }
            
            if ( mObserver != null )
            {
                mObserver.onNewClient( this, sSocket );
            }
            
            mTraceLogger.logDebug(
                    "All of connected client sockets is {0}.",
                    new Object[] { new Integer(sClientCount) } );
        }
        catch (Exception e)
        {
            mTraceLogger.log(e);
            
            if ( sIsGeneratedID == true )
            {
                mSessionIdGenerator.release( sSessionId );
            }
            
            if ( sIsAccepted == true )
            {
                try
                {
                    // if socket channel close, then this will unregister to selector
                    sSocketChannel.close(); 
                }
                catch( Exception aCloseException )
                {
                    mTraceLogger.log( aCloseException );
                }
            }
            else
            {
                mTraceLogger.logDebug(
                      "Client socket did not accept from upper module. (Socket = {0})",
                      new Object[] { sSocketChannel.toString() } );
            }
            
            return false;
        }
        
        return true;
    }

    /**
     * Read from socket buffer
     * 
     * @param aSelectionKey     Selection key
     * @return                  Received bytes
     */
    private int read(SelectionKey aSelectionKey)
    {
        int sReadBytes = 0;

        boolean sNeedMoreData = false;
        int     sNextReadPosition = 0;
        int     sLimit;

        SocketChannel sSocketChannel = (SocketChannel)aSelectionKey.channel();
        Socket sSocket = sSocketChannel.socket();
        Client sClient = null;

        synchronized (this)
        {
            sClient = (Client)mClientMap.get(sSocket);
        }
        
        if (sClient == null)
        {
            mTraceLogger.logError(
                    "Unknown socket. (Socket = {0})",
                    new Object[] { sSocket } );
            
            return -1;
        }
        
        try
        {
            // read from socket buffer
            sReadBytes = sSocketChannel.read(sClient.mReadBuffer);
            if (sReadBytes > 0)
            {
                mTraceLogger.logDebug(
                        "JCommModule recveived data from socket buffer. (Bytes = {0}, Socket = {1})",
                        new Object[] { new Integer(sReadBytes), sSocket.toString()} );
                
                // limit = current position and position = 0
                sClient.mReadBuffer.flip();
                
                sLimit = sClient.mReadBuffer.limit(); 
                
                do
                {
                    if (sClient.mReadBuffer.limit() > PacketHeader.HeaderSize)
                    {
                        // read packet header (16bytes)
                        if (sClient.mReadPacketHeader.read(sClient.mReadBuffer) == true)
                        {
                            int sPayloadLength = sClient.mReadPacketHeader.getPayloadLength();
                            int sPacketLength  = sClient.mReadPacketHeader.getPacketLength();
                            
                            if (sClient.mReadBuffer.remaining() >= sPayloadLength)
                            {
                                // received one or more CM block
                                if (mObserver != null)
                                {
                                    mObserver.onReceivedPacket(
                                            this,
                                            sSocket,
                                            sClient.mReadPacketHeader,
                                            sClient.mReadBuffer);
                                }
                                else
                                {
                                    // nothing to do
                                }

                                // move explicitly buffer position of end of one packet
                                sClient.mReadBuffer.position(sPacketLength);
                                sLimit -= sPacketLength;

                                // truncate read one packet (packet header + packet payload)
                                sClient.mReadBuffer.compact();

                                // reset buffer position for next packet
                                // (position = 0 and limit = last position for filled buffer) 
                                sClient.mReadBuffer.position(0);
                                sClient.mReadBuffer.limit(sLimit);
                            }
                            else
                            {
                                sNeedMoreData = true;
                                break;
                            }
                        }
                        else
                        {
                            // Because this module cannot analyze CM header from network stream,
                            // this socket will disconnect. 
                            mTraceLogger.logError(
                                    "JCommMoudule could not analyze CM header from network stream. (Socket = {0})",
                                    new Object[] { sSocket.toString() } );

                            sReadBytes = -1;
                            break;
                        }
                    }
                    else
                    {
                        sNeedMoreData = true;
                        break;
                    }
                } while (true);
                
                if (sNeedMoreData == true)
                {
                    sNextReadPosition = sClient.mReadBuffer.limit();
                    
                    sClient.mReadBuffer.limit(MaxCmBlockSize);
                    sClient.mReadBuffer.position(sNextReadPosition);
                }
            }
        }
        catch ( Exception e )
        {
            sReadBytes = -1; /* for processing error */
            mTraceLogger.log(e);
        }

        return sReadBytes;
    }

    /**
     * Close the socket
     * 
     * @param aSelectionKey     Selection key
     */
    private void close(SelectionKey aSelectionKey)
    {
        SocketChannel sSocketChannel = null;
        Socket        sSocket        = null;
        int           sSessionID     = 0;
        Client        sClient        = null;
                
        try
        {
            sSocketChannel = (SocketChannel)aSelectionKey.channel();

            sSocket = sSocketChannel.socket();

            if ( sSocket != null )
            {
                synchronized (this)
                {
                    sClient = (Client)mClientMap.remove(sSocket);
                }
                
                if ( sClient != null )
                {
                    sSessionID = sClient.mWritePacketHeader.getSessionId();
                    
                    if ( mSessionIdGenerator != null )
                    {
                        mSessionIdGenerator.release( sSessionID );
                    }
                }
                
                if (mObserver != null)
                {
                    mObserver.onClientDisconnected(this, sSocket);
                }
                
                mTraceLogger.logDebug(
                        "Client socket close or disconnet. (Socket = {0})",
                        new Object[] { sSocket.toString() } );
            }
            
            // if socket channel close, then this will unregister to selector
            sSocketChannel.close();
        }
        catch (Exception e)
        {
            mTraceLogger.log(e);
        }
    }

    /**
     * Disconnect client
     * 
     * @param aSocket   Client socket to disconnect
     */
    public void disconnectClient(Socket aSocket)
    {
        synchronized (this)
        {
            mClientMap.remove(aSocket);
        }
        
        mTraceLogger.logDebug(
                "Client socket disconnet. (Socket = {0})",
                new Object[] { aSocket.toString() } );
        
        SocketChannel sSocketChannel = aSocket.getChannel();
        
        SelectionKey sSelectionKey = sSocketChannel.keyFor(mSelector);
        sSelectionKey.cancel();
        
        try
        {
            sSocketChannel.close();
            aSocket.close();
        }
        catch (IOException e)
        {
            mTraceLogger.log(e);
        }
    }

    /**
     * Disconnect all of clients
     */
    public void disconnectAllClients()
    {
        mTraceLogger.logDebug("All of client sockets disconnet.");
        
        HashMap sClientMap = null;
        
        synchronized (this)
        {
            sClientMap = mClientMap;
            mClientMap = new HashMap();
        }
        
        if (sClientMap != null)
        {
            Iterator sIterator = sClientMap.entrySet().iterator();
            while (sIterator.hasNext() == true)
            {
                Map.Entry sEntry = (Map.Entry)sIterator.next();
    
                Socket sKey = (Socket)sEntry.getKey();
    
                SocketChannel sSocketChannel = sKey.getChannel();
                
                SelectionKey sSelectionKey = sSocketChannel.keyFor(mSelector);
                sSelectionKey.cancel();
                
                try
                {
                    sSocketChannel.close();
                    sKey.close();
                }
                catch (IOException e)
                {
                    mTraceLogger.log(e);
                }
                
                sIterator.remove();
            }
            
            sClientMap.clear();
        }
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
     * Get packet header size
     * 
     * @return  Packet header size
     */
    public static int getPacketHeaderSize()
    {
        return PacketHeader.HeaderSize;
    }

    /**
     * Get the connected client count
     * 
     * @return  The connected client count
     */
    public int getConnectedClientCount()
    {
        synchronized (this)
        {
            if (mClientMap == null)
            {
                return 0;
            }
            
            return mClientMap.size();
        }
    }
    
    /**
     * Send a CM block
     * 
     * @param aSocket   Socket
     * @param aCmBlock  A CM block to send
     * @return          true, if success. false, if fail.
     */
    public boolean sendCmBlock(Socket aSocket, ByteBuffer aCmBlock)
    {
        long sWrittenBytes = 0;

        // find client socket from client socket map by aSocket key
        Client sClient;

        synchronized (this)
        {
            sClient = (Client)mClientMap.get(aSocket);
        }
        
        if (sClient == null)
        {
            mTraceLogger.logError(
                    "Unkown client socket. (Socket = {0})",
                    new Object[] { aSocket.toString() } );
            
            return false;
        }

        // composite packet header
        int sPayloadLength = aCmBlock.limit() - PacketHeader.HeaderSize;

        aCmBlock.position(0);
        sClient.mWritePacketHeader.setPayloadLength(sPayloadLength);
        sClient.mWritePacketHeader.write(aCmBlock);
        aCmBlock.position(0);

        int sWriteBytes = PacketHeader.HeaderSize + sPayloadLength;
        
        // write CM block until sending completely
        try
        {
            SocketChannel sSocketChannel = aSocket.getChannel();

            int sRetriedCount = 0;

            do
            {
                sWrittenBytes += sSocketChannel.write(aCmBlock);
                
                if (sWrittenBytes >= sWriteBytes)
                {
                    break;
                }

                /*
                 * BUG-41257
                 * 
                 * When sSocketChannel.write() returns 0, aCmBlock's position
                 * is not recovered.
                 */
                aCmBlock.position(new Long(sWrittenBytes).intValue());
                
                Thread.sleep(100);
                ++sRetriedCount;
                
            } while (true);
            
            if (mTraceLogger.isLoggable(TraceLogger.LogLevelDebug) == true)
            {
                mTraceLogger.logDebug(
                        "JCommModule sent a CM block. (Bytes = {0}, Retry Count = {1}, Socket = {2})",
                        new Object[] { new Integer(sWriteBytes), new Integer(sRetriedCount), aSocket.toString() });
            }
        }
        catch (IOException e)
        {
            mTraceLogger.log(e);
        }
        catch (InterruptedException e)
        {
            mTraceLogger.log(e);
        }

        return (sWrittenBytes >= sWriteBytes) ? true : false;
    }
}
