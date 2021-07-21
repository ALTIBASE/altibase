/***********************************************************************
 * Copyright 1999-2013, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idxLocalSock.h 69798 2015-03-19 23:55:11Z khkwak $
 **********************************************************************/

/***********************************************************************
 * Description :
 *
 *   These interfaces are for local socket communication through a
 *   single (socket) file. In WINDOWS, they use NamedPipe interfaces.
 *   Otherwise, they use domain socket interfaces.
 *
 **********************************************************************/
#ifndef _O_IDX_LOCALSOCK_H_
#define  _O_IDX_LOCALSOCK_H_  1

#define IDX_LOCALSOCK_BACKLOG      5 /* commonly used */
#define IDX_LOCALSOCK_RECV_WAITSEC 1 /* recv() waits up to 1 sec for next stream */

#include <idTypes.h>
#include <idl.h>
#include <iduMemory.h>

#if defined(ALTI_CFG_OS_WINDOWS)
#include <windows.h>
#endif

/* PDL_HANDLE == PDL_SOCKET */
#if defined(ALTI_CFG_OS_WINDOWS)
    typedef struct idxPipeHandle
    {
        PDL_HANDLE   mHandle;   // pipe handle
        LPOVERLAPPED mOverlap;  // overlapped I/O info. (ptr)
    } idxPipeHandle;
    typedef idxPipeHandle IDX_LOCALSOCK;
    #define IDX_IS_CONNECTED(sock) ((sock.mHandle >= 0) ? ID_TRUE : ID_FALSE)
    #define IDX_SET_INVALID(sock)  (sock.mHandle = PDL_INVALID_HANDLE)
#else
    typedef PDL_HANDLE IDX_LOCALSOCK;
    #define IDX_IS_CONNECTED(sock) ((sock >= 0) ? ID_TRUE : ID_FALSE)
    #define IDX_SET_INVALID(sock)  (sock = PDL_INVALID_HANDLE)
#endif /* ALTI_CFG_OS_WINDOWS */

class idxLocalSock
{
public:
    static SChar*  mHomePath;

    // initialize class
    static void   initializeStatic();

    // finalize class
    static void   destroyStatic();
    
    // BUG-40195 check if agent process is available
    static IDE_RC ping( UInt            aPID,
                        IDX_LOCALSOCK * aSocket,
                        idBool        * aIsAlive );

    static void   setPath( SChar         * aBuffer,
                           SChar         * aSockName,
                           idBool          aNeedUniCode );
     
    // BUG-37957 initialize socket
    static IDE_RC initializeSocket( IDX_LOCALSOCK        * aSocket,
                                    iduMemoryClientIndex   aIndex );
    static IDE_RC initializeSocket( IDX_LOCALSOCK * aSocket,
                                    iduMemory     * aMemory );

    // BUG-37957 finalize socket
    static void finalizeSocket( IDX_LOCALSOCK * aSocket );

    /* communication interfaces */
    static IDE_RC socket( IDX_LOCALSOCK * aSock );

    static IDE_RC bind( IDX_LOCALSOCK * aSock,
                        SChar         * aPath,
                        iduMemory     * aMemory );

    static IDE_RC listen( IDX_LOCALSOCK aSock );
    
    static IDE_RC accept( IDX_LOCALSOCK   aServSock,
                          IDX_LOCALSOCK * aClntSock,
                          PDL_Time_Value  aWaitTime,
                          idBool        * aIsTimeout );

    static IDE_RC connect( IDX_LOCALSOCK * aSock,
                           SChar         * aPath,
                           iduMemory     * aMemory,
                           idBool        * aIsRefused );

    static IDE_RC select( IDX_LOCALSOCK    aSock,
                          PDL_Time_Value * aWaitTime,
                          idBool         * aIsTimeout );

    static IDE_RC send( IDX_LOCALSOCK   aSock,
                        void          * aBuffer,
                        UInt            aBufferSize );
                        
    static IDE_RC sendTimedWait( IDX_LOCALSOCK   aSock,
                                 void          * aBuffer,
                                 UInt            aBufferSize,
                                 PDL_Time_Value  aWaitTime,
                                 idBool        * aIsTimeout );

    static IDE_RC recv( IDX_LOCALSOCK   aSock,
                        void          * aBuffer,
                        UInt            aBufferSize,
                        UInt          * aReceivedSize );
                        
    static IDE_RC recvTimedWait( IDX_LOCALSOCK   aSock,
                                 void          * aBuffer,
                                 UInt            aBufferSize,
                                 PDL_Time_Value  aWaitTime,
                                 UInt          * aReceivedSize,
                                 idBool        * aIsTimeout );
                        
    static IDE_RC setBlockMode( IDX_LOCALSOCK aSock );

    static IDE_RC setNonBlockMode( IDX_LOCALSOCK aSock );

    static IDE_RC close( IDX_LOCALSOCK * aSock );
    
    static IDE_RC disconnect( IDX_LOCALSOCK * aSock );
};

#endif /* _O_IDX_LOCALSOCK_H_ */
