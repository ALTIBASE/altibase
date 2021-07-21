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
 


/**
 * @mainpage Introduction to ID layer
 *
 * @section ID
 * - Independent layer
 * - Help Altibase developers do not care platform dependency
 * - So many useful libraries common for all Altibase modules
 * - made from ida, idc, ide, idf, idk, idl, idm, idn, idp, ids, idt, idu, idv, idw class
 *
 * @sectioin idCore
 * - wrapper class for Alticore
 * - include many APIs of Alticore, which are used in Altibase
 *
 */


/**
 * @defgroup ID Altibase InDependent layer
 *
 *
 */

/**
 * @defgroup idCore
 *
 * wrapper interfaces for Alticore APIs
 *
 * - idCore.h
 */

 
/**
 * @defgroup chk
 * @ingroup ID
 *
 * applications to check platform environment
 *
 * - checkConnect.cpp
 * - checkIPC.cpp
 * - checkIPC2.cpp
 * - checkCond.cpp
 * - checkEnv.cpp
 * - checkType.cpp
 */


/**
 * @defgroup ida
 * @ingroup ID
 *
 * 
 *
 * - ida.h
 */

/**
 * @defgroup idc
 * @ingroup ID
 *
 * 
 *
 * - 
 */

/**
 * @defgroup idd
 * @ingroup ID
 *
 * 
 *
 * - iddTypes.h
 */
   

/**
 * @defgroup ide
 * @ingroup ID
 *
 * error handling and callstack printing
 *
 * - ide.h
 * - ideCallback.h
 * - ideErrorMgr.h
 * - ideMsgLog.h
 */

/**
 * @defgroup idf
 * @ingroup ID
 *
 * 
 *
 * - idf.h
 * - idfMemory.h
 */

/**
 * @defgroup idk
 * @ingroup ID
 *
 * 
 *
 * - idkAtomic.h
 */

/**
 * @defgroup idl
 * @ingroup ID
 *
 * 
 * - idl.h
 * - idl.i
 */

/**
 * @defgroup idm
 * @ingroup ID
 *
 * 
 *
 * - idm.h
 * - idmDef.h
 */

/**
 * @defgroup idn
 * @ingroup ID
 *
 * 
 *
 * - idn.h
 * - idnAscii.h
 * - idnBig5.h
 * - idnCaseConvMap.h
 * - idnCharSet.h
 * - idnChosung.h
 * - idnConv.h
 * - idnCp949.h
 * - idnEnglish.h
 * - idnEucjp.h
 * - idnEuckr.h
 * - idnGb2312.h
 * - idnJisx0201.h
 * - idnJisx0208.h
 * - idnJisx0212.h
 * - idnKorean.h
 * - idnKsc5601.h
 * - idnReplaceCharMap.h
 * - idnSjis.h
 * - idnUhc1.h
 * - idnUhc2.h
 * - idnUtf8.h
 */

/**
 * @defgroup idp
 * @ingroup ID
 *
 * 
 *
 * - idp.h
 * - idpBase.h
 * - idpSInt.h
 * - idpSLong.h
 * - idpString.h
 * - idpUInt.h
 * - idpULong.h
 */

/**
 * @defgroup ids
 * @ingroup ID
 *
 * 
 *
 * - idsCrypt.h
 * - idsDES.h
 * - idsGPKI.h
 * - idsRC4.h
 * - idsSHA1.h
 */

/**
 * @defgroup idt
 * @ingroup ID
 *
 * 
 *
 * - idtBaseThread.h
 */

/**
 * @defgroup idu
 * @ingroup ID
 *
 * idu class
 *
 * - idu.h
 * - iduAIOQueue.h
 * - iduArgument.h
 * - iduBridge.h
 * - iduBridgeTime.h
 * - iduCheckLicense.h
 * - iduCompression.h
 * - iduCond.h
 * - iduFXStack.h
 * - iduFatalCallback.h
 * - iduFile.h
 * - iduFileAIO.h
 * - iduFileStream.h
 * - iduFixedTable.h
 * - iduFixedTableDef.h
 * - iduGrowingMemoryHandle.h
 * - iduHash.h
 * - iduHashUtil.h
 * - iduHeap.h
 * - iduHeapSort.h
 * - iduLatch.h
 * - iduLatchObj.h
 * - iduLimitManager.h
 * - iduList.h
 * - iduMemList.h
 * - iduMemMgr.h
 * - iduMemPool.h
 * - iduMemPool2.h
 * - iduMemPoolMgr.h
 * - iduMemory.h
 * - iduMemoryHandle.h
 * - iduMutex.h
 * - iduMutexEntry.h
 * - iduMutexMgr.h
 * - iduMutexObj.h
 * - iduMutexRes.h
 * - iduName.h
 * - iduOIDMemory.h
 * - iduPriorityQueue.h
 * - iduProperty.h
 * - iduPtrList.h
 * - iduQueue.h
 * - iduReusedMemoryHandle.h
 * - iduRevision.h
 * - iduRunTimeInfo.h
 * - iduSema.h
 * - iduSessionEvent.h
 * - iduStack.h
 * - iduStackMgr.h
 * - iduString.h
 * - iduStringHash.h
 * - iduSync.h
 * - iduTable.h
 * - iduVarMemList.h
 * - iduVarString.h
 * - iduVersion.h
 * - iduVersionDef.h
 */

/**
 * @defgroup idv
 * @ingroup ID
 *
 * 
 *
 * - idv.h
 * - idvProfile.h
 * - idvProfileDef.h
 * - idvTime.h
 * - idvType.h
 */

/**
 * @defgroup idw
 * @ingroup ID
 *
 * 
 *
 * - idwService.h
 */


