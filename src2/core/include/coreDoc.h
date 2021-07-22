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
 
/*****************************************************************************
 * $Id: coreDoc.h 5469 2009-04-29 08:17:25Z sjkim $
 ****************************************************************************/

#if !defined(_O_CORE_DOC_H_)
#define _O_CORE_DOC_H_

/*
 * This file is for documenting Altibase Core
 */


/**
 * @mainpage Altibase Core Library Project
 *
 * 
 */


/**
 * @defgroup Core Altibase Core
 *
 * Altibase Core consists of following sub modules
 * - ACP: Porting Layer
 * - ACL: Foundation Library
 * - ACE: Exception Handling
 * - ACT: Unit Testing Framework
 *
 *
 * <table border="1" cellpadding="1" cellspacing="0">
 * <tr><th>Standard</th><th>Core</th></tr>
 * <tr><td>abort</td><td>acpProcAbort()</td></tr>
 * <tr><td>accept</td><td>acpSockAccept()</td></tr>
 * <tr><td>bind</td><td>acpSockBind()</td></tr>
 * <tr><td>calloc</td><td>acpMemCalloc()</td></tr>
 * <tr><td>close</td><td>acpFileClose() acpSockClose()</td></tr>
 * <tr><td>closedir</td><td>acpDirClose()</td></tr>
 * <tr><td>connect</td><td>acpSockConnect()</td></tr>
 * <tr><td>dlclose</td><td>acpDlClose()</td></tr>
 * <tr><td>dlerror</td><td>acpDlError()</td></tr>
 * <tr><td>dlopen</td><td>acpDlOpen()</td></tr>
 * <tr><td>dlsym</td><td>acpDlSym()</td></tr>
 * <tr><td>dup</td><td>acpFileDup()</td></tr>
 * <tr><td>execv</td><td>acpProcExec()</td></tr>
 * <tr><td>exit</td><td>acpProcExit()</td></tr>
 * <tr><td>fclose</td><td>acpStdClose()</td></tr>
 * <tr><td>fflush</td><td>acpStdFlush()</td></tr>
 * <tr><td>fgetc</td><td>acpStdGetChar()</td></tr>
 * <tr><td>fgets</td><td>acpStdGetString()</td></tr>
 * <tr><td>flock</td><td>acpFileLock() acpFileTryLock() acpFileUnlock()</td></tr>
 * <tr><td>fopen</td><td>acpStdOpen()</td></tr>
 * <tr><td>fork</td><td>acpProcForkExec()</td></tr>
 * <tr><td>free</td><td>acpMemFree()</td></tr>
 * <tr><td>fstat</td><td>acpFileStatAtPath()</td></tr>
 * <tr><td>ftell</td><td>acpStdTell()</td></tr>
 * <tr><td>ftruncate</td><td>acpFileTruncate()</td></tr>
 * <tr><td>getaddrinfo</td><td>acpInetGetHostByName()</td></tr>
 * <tr><td>getc</td><td>acpStdGetChar()</td></tr>
 * <tr><td>getchar</td><td>acpStdGetChar()</td></tr>
 * <tr><td>getenv</td><td>acpEnvGet()</td></tr>
 * <tr><td>gethostbyaddr</td><td>acpInetGetHostByAddr()</td></tr>
 * <tr><td>gethostbyname</td><td>acpInetGetHostByName()</td></tr>
 * <tr><td>gethostname</td><td>acpSysGetHostName()</td></tr>
 * <tr><td>getnameinfo</td><td>acpInetGetHostByAddr()</td></tr>
 * <tr><td>getopt</td><td>acpOptGet()</td></tr>
 * <tr><td>getpeername</td><td>acpSockGetPeerName()</td></tr>
 * <tr><td>getpid</td><td>acpProcGetSelfID()</td></tr>
 * <tr><td>getrlimit</td><td>acpSysGetHandleLimit() acpSysSetHandleLimit()</td></tr>
 * <tr><td>getsockname</td><td>acpSockGetName()</td></tr>
 * <tr><td>getsockopt</td><td>acpSockGetOpt()</td></tr>
 * <tr><td>gets</td><td>acpStdGetString()</td></tr>
 * <tr><td>gettimeofday</td><td>acpTimeNow()</td></tr>
 * <tr><td>gmtime</td><td>acpTimeGetGmTime()</td></tr>
 * <tr><td>htonl</td><td>#ACP_TON_BYTE4</td></tr>
 * <tr><td>htons</td><td>#ACP_TON_BYTE2</td></tr>
 * <tr><td>inet_addr</td><td>acpInetStrToAddr()</td></tr>
 * <tr><td>inet_aton</td><td>acpInetStrToAddr()</td></tr>
 * <tr><td>inet_ntoa</td><td>acpInetAddrToStr()</td></tr>
 * <tr><td>inet_ntop</td><td>acpInetAddrToStr()</td></tr>
 * <tr><td>inet_pton</td><td>acpInetStrToAddr()</td></tr>
 * <tr><td>isGraph</td><td>acpCharIsGraph()</td></tr>
 * <tr><td>isalnum</td><td>acpCharIsAlnum()</td></tr>
 * <tr><td>isalpha</td><td>acpCharIsAlpha()</td></tr>
 * <tr><td>isascii</td><td>acpCharIsAscii()</td></tr>
 * <tr><td>iscntrl</td><td>acpCharIsCntrl()</td></tr>
 * <tr><td>isdigit</td><td>acpCharIsDigit()</td></tr>
 * <tr><td>islower</td><td>acpCharIsLower()</td></tr>
 * <tr><td>isprint</td><td>acpCharIsPrint()</td></tr>
 * <tr><td>ispunct</td><td>acpCharIsPunct()</td></tr>
 * <tr><td>isspace</td><td>acpCharIsSpace()</td></tr>
 * <tr><td>isupper</td><td>acpCharIsUpper()</td></tr>
 * <tr><td>isxdigit</td><td>acpCharIsXdigit()</td></tr>
 * <tr><td>kill</td><td>acpProcKill()</td></tr>
 * <tr><td>listen</td><td>acpSockListen()</td></tr>
 * <tr><td>localtime</td><td>acpTimeGetLocalTime()</td></tr>
 * <tr><td>lseek</td><td>acpFileSeek()</td></tr>
 * <tr><td>malloc</td><td>acpMemAlloc()</td></tr>
 * <tr><td>memcmp</td><td>acpMemCmp()</td></tr>
 * <tr><td>memcpy</td><td>acpMemCpy()</td></tr>
 * <tr><td>memmove</td><td>acpMemMove()</td></tr>
 * <tr><td>memset</td><td>acpMemSet()</td></tr>
 * <tr><td>mmap</td><td>acpMmapAttach()</td></tr>
 * <tr><td>msync</td><td>acpMmapSync()</td></tr>
 * <tr><td>munmap</td><td>acpMmapDetach()</td></tr>
 * <tr><td>ntohl</td><td>#ACP_TOH_BYTE4</td></tr>
 * <tr><td>ntohs</td><td>#ACP_TOH_BYTE2</td></tr>
 * <tr><td>open</td><td>acpFileOpen()</td></tr>
 * <tr><td>opendir</td><td>acpDirOpen()</td></tr>
 * <tr><td>poll</td><td>acpPollDispatch()</td></tr>
 * <tr><td>printf</td><td>acpPrintf()</td></tr>
 * <tr><td>pthread_attr_destroy</td><td>acpThrAttrDestroy()</td></tr>
 * <tr><td>pthread_attr_init</td><td>acpThrAttrCreate()</td></tr>
 * <tr><td>pthread_attr_setdetachstate</td><td>acpThrAttrSetDetach()</td></tr>
 * <tr><td>pthread_attr_setscope</td><td>acpThrAttrSetBound()</td></tr>
 * <tr><td>pthread_attr_setstacksize</td><td>acpThrAttrSetStackSize()</td></tr>
 * <tr><td>pthread_cond_broadcast</td><td>acpThrCondBroadcast()</td></tr>
 * <tr><td>pthread_cond_destroy</td><td>acpThrCondDestroy()</td></tr>
 * <tr><td>pthread_cond_init</td><td>acpThrCondCreate()</td></tr>
 * <tr><td>pthread_cond_signal</td><td>acpThrCondSignal()</td></tr>
 * <tr><td>pthread_cond_timedwait</td><td>acpThrCondTimedWait()</td></tr>
 * <tr><td>pthread_cond_wait</td><td>acpThrCondWait()</td></tr>
 * <tr><td>pthread_create</td><td>acpThrCreate()</td></tr>
 * <tr><td>pthread_detach</td><td>acpThrDetach()</td></tr>
 * <tr><td>pthread_exit</td><td>acpThrExit()</td></tr>
 * <tr><td>pthread_join</td><td>acpThrJoin()</td></tr>
 * <tr><td>pthread_mutex_destroy</td><td>acpThrMutexDestroy()</td></tr>
 * <tr><td>pthread_mutex_init</td><td>acpThrMutexCreate()</td></tr>
 * <tr><td>pthread_mutex_lock</td><td>acpThrMutexLock()</td></tr>
 * <tr><td>pthread_mutex_trylock</td><td>acpThrMutexTryLock()</td></tr>
 * <tr><td>pthread_mutex_unlock</td><td>acpThrMutexUnlock()</td></tr>
 * <tr><td>pthread_once</td><td>acpThrOnce()</td></tr>
 * <tr><td>pthread_rwlock_destroy</td><td>acpThrRwlockDestroy()</td></tr>
 * <tr><td>pthread_rwlock_init</td><td>acpThrRwlockCreate()</td></tr>
 * <tr><td>pthread_rwlock_rdlock</td><td>acpThrRwlockLockRead()</td></tr>
 * <tr><td>pthread_rwlock_tryrdlock</td><td>acpThrRwlockTryLockRead()</td></tr>
 * <tr><td>pthread_rwlock_trywrlock</td><td>acpThrRwlockTryLockWrite()</td></tr>
 * <tr><td>pthread_rwlock_unlock</td><td>acpThrRwlockUnlock()</td></tr>
 * <tr><td>pthread_rwlock_wrlock</td><td>acpThrRwlockLockWrite()</td></tr>
 * <tr><td>pthread_self</td><td>acpThrGetSelfID()</td></tr>
 * <tr><td>pthread_sigmask</td><td>acpSignalBlockAll() acpSignalBlockDefault() acpSignalBlock() acpSignalUnblock()</td></tr>
 * <tr><td>pthread_yield</td><td>acpThrYield()</td></tr>
 * <tr><td>putenv</td><td>acpEnvSet()</td></tr>
 * <tr><td>rand</td><td>acpRand()</td></tr>
 * <tr><td>read</td><td>acpFileRead()</td></tr>
 * <tr><td>readdir</td><td>acpDirRead()</td></tr>
 * <tr><td>realloc</td><td>acpMemRealloc()</td></tr>
 * <tr><td>recv</td><td>acpSockRecv()</td></tr>
 * <tr><td>recvfrom</td><td>acpSockRecvFrom()</td></tr>
 * <tr><td>remove</td><td>acpFileRemove()</td></tr>
 * <tr><td>rename</td><td>acpFileRename()</td></tr>
 * <tr><td>rewinddir</td><td>acpDirRewind()</td></tr>
 * <tr><td>rmdir</td><td>acpFileRemove()</td></tr>
 * <tr><td>sched_yield</td><td>acpThrYield()</td></tr>
 * <tr><td>select</td><td>acpPollDispatch()</td></tr>
 * <tr><td>send</td><td>acpSockSend()</td></tr>
 * <tr><td>sendto</td><td>acpSockSendTo()</td></tr>
 * <tr><td>setenv</td><td>acpEnvSet()</td></tr>
 * <tr><td>setsockopt</td><td>acpSockSetOpt()</td></tr>
 * <tr><td>shmat</td><td>acpShmAttach()</td></tr>
 * <tr><td>shmctl</td><td>acpShmCreate() acpShmDestroy()</td></tr>
 * <tr><td>shmdt</td><td>acpShmDetach()</td></tr>
 * <tr><td>shutdown</td><td>acpSockShutdown()</td></tr>
 * <tr><td>sigaction</td><td>acpSignalSetExceptionHandler()</td></tr>
 * <tr><td>sigprocmask</td><td>acpSignalBlockAll() acpSignalBlockDefault() acpSignalBlock() acpSignalUnblock()</td></tr>
 * <tr><td>snprintf</td><td>acpSnprintf() acpStrCpyFormat() acpStrCatFormat()</td></tr>
 * <tr><td>socket</td><td>acpSockOpen()</td></tr>
 * <tr><td>srand</td><td>acpRandSeed()</td></tr>
 * <tr><td>stat</td><td>acpFileStat()</td></tr>
 * <tr><td>strcat</td><td>acpStrCat() acpStrCatBuffer() acpStrCatCString()</td></tr>
 * <tr><td>strchr</td><td>acpStrFindChar() acpStrFindCharSet()</td></tr>
 * <tr><td>strcmp</td><td>acpStrCmp() acpStrCmpCString() acpStrEqual()</td></tr>
 * <tr><td>strcpy</td><td>acpStrCpy() acpStrCpyBuffer() acpStrCpyCString()</td></tr>
 * <tr><td>strerror</td><td>acpErrorString()</td></tr>
 * <tr><td>strlen</td><td>acpStrGetLength()</td></tr>
 * <tr><td>strstr</td><td>acpStrFindString() acpStrFindCString()</td></tr>
 * <tr><td>time</td><td>acpTimeNow()</td></tr>
 * <tr><td>times</td><td>acpSysGetCPUTimes()</td></tr>
 * <tr><td>tolower</td><td>acpCharToLower()</td></tr>
 * <tr><td>toupper</td><td>acpCharToUpper()</td></tr>
 * <tr><td>truncate</td><td>acpFileTruncateAtPath()</td></tr>
 * <tr><td>unlink</td><td>acpFileRemove()</td></tr>
 * <tr><td>wait</td><td>acpProcWait()</td></tr>
 * <tr><td>write</td><td>acpFileWrite()</td></tr>
 * </table>
 */

/**
 * @defgroup CoreAtomic Atomic Operation
 * @ingroup Core
 *
 * Altibase Core Atomic Operations
 *
 * - acpAtomic.h atomic operations for 32/64 bit integer
 * - acpSpinLock.h spin lock
 * - acpSpinWait.h spin wait a condition
 */

/**
 * @defgroup CoreChar Character & String
 * @ingroup Core
 *
 * Altibase Core Character & String Support
 *
 * - acpChar.h character operations (islower, tolower, ...)
 * - acpPrintf.h format printing
 * - acpStr.h string
 * - acpCStr.h Const C-Style Strings
 * - aclReadLine.h string
 */

/**
 * @defgroup CoreCollection Collection
 * @ingroup Core
 *
 * Altibase Core Collections
 *
 * - aclHash.h chained hash table
 * - acpList.h doubly circularly linked list
 * - aclQueue.h FIFO(first-in first-out) queue
 */

/**
 * @defgroup CoreDl Dynamic Loading
 * @ingroup Core
 *
 * Altibase Core Dynamic Loading Library Support
 *
 * - acpDl.h dynamic libraries
 */

/**
 * @defgroup CoreEnv Environment & Option
 * @ingroup Core
 *
 * Altibase Core Environment & Option Support
 *
 * - acpEnv.h environment variables
 * - acpOpt.h program argument parser
 * - aclConf.h configuration file parser
 */

/**
 * @defgroup CoreError Error & Exception
 * @ingroup Core
 *
 * Altibase Core Error & Exception Handling
 *
 * - acpCallstack.h callstack
 * - acpError.h core error codes
 * - aceError.h product error code
 * - aceException.h exception handling
 * - aclContext.h context
 * - aclLog.h message logging
 */

/**
 * @defgroup CoreFile File & Directory
 * @ingroup Core
 *
 * Altibase Core File Management
 *
 * - acpDir.h directory entries
 * - acpFile.h low-level file operations
 * - acpMmap.h memory-mapped file
 * - acpPrintf.h format printing (to file stream)
 * - acpStd.h standard buffered file stream
 */

/**
 * @defgroup CoreInit Initialization
 * @ingroup Core
 *
 * Altibase Core Initializations
 *
 * - acpInit.h ACP initialization
 */

/**
 * @defgroup CoreMath Mathematics
 * @ingroup Core
 *
 * Altibase Core Mathematics
 *
 * - acpBit.h bit operations
 * - acpRand.h pseudo random generator
 */

/**
 * @defgroup CoreMem Memory
 * @ingroup Core
 *
 * Altibase Core Memory Management
 *
 * - acpAlign.h alignment calculation
 * - acpMem.h memory allocations and basic operations
 * - acpMemBarrier.h memory barriers
 * - acpMmap.h memory-mapped file
 * - aclMemArea.h stack like dynamic memory allocator
 * - aclMemPool.h dynamic memory allocator for fixed size memory blocks
 */

/**
 * @defgroup CoreCompression Compress & Decompress
 * @ingroup Core
 *
 * Altibase Core Compression 
 *
 * - aclCompression.h RealTime Compress & Decompress Library 
 */


/**
 * @defgroup CoreThr Multithreading
 * @ingroup Core
 *
 * Altibase Core Multithreading Support
 *
 * - acpSignal.h signal handling
 * - acpSleep.h thread sleep
 * - acpSpinLock.h synchronization spin lock
 * - acpSpinWait.h spin wait a condition
 * - acpThr.h thread attributes, creating, joing, detaching, yielding, once
 * - acpThrBarrier.h thread barrier
 * - acpThrCond.h condition variables
 * - acpThrMutex.h synchronization mutex
 * - acpThrRwlock.h synchronization read-write lock
 */

/**
 * @defgroup CoreNet Networking
 * @ingroup Core
 *
 * Altibase Core Networking Support
 *
 * - acpInet.h internet address conversions
 * - acpPoll.h multiplexing
 * - acpSock.h socket
 */

/**
 * @defgroup CoreProc Process
 * @ingroup Core
 *
 * Altibase Core Process Management
 *
 * - acpProc.h process management
 * - acpProcMutex.h process synchronization mutex
 * - acpSignal.h signal handling
 * - acpSleep.h thread sleep
 */

/**
 * @defgroup CorePset Processor Set
 * @ingroup Core
 *
 * Altibase Core Processor Set Management
 *
 * - acpPset.h processor management
 */

/**
 * @defgroup CoreShm Shared Memory
 * @ingroup Core
 *
 * Altibase Core Shared Memory Management
 *
 * - acpShm.h shared memory
 */

/**
 * @defgroup CoreSort Sorting Algorithms
 * @ingroup Core
 *
 * Altibase Core Sorting Algorithms
 * 
 * - aclSort.h Sorting Function Definitions
 */

/**
 * @defgroup CoreSem System Semaphore
 * @ingroup Core
 *
 * Altibase Core System Semaphore Management
 *
 * - acpSem.h system semaphore
 */

/**
 * @defgroup CoreSys System Information
 * @ingroup Core
 *
 * Altibase Core System Information
 *
 * - acpEnv.h environment variables
 * - acpSys.h system information
 * - acpTime.h time
 */

/**
 * @defgroup CoreType Type
 * @ingroup Core
 *
 * Altibase Core Types and Operators
 *
 * - acpAlign.h alignment calculation
 * - acpByteOrder.h byte order conversions
 * - acpChar.h character operations (islower, tolower, ...)
 * - acpTypes.h basic types
 */

/**
 * @defgroup CoreUnit Unit Testing
 * @ingroup Core
 *
 * Altibase Core Unit Testing Framework
 *
 * - actBarrier.h execution barrier
 * - actTest.h testing
 */


#endif
