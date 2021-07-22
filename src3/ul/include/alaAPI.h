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
 
#ifndef _O_ALA_API_H_
#define _O_ALA_API_H_ 1

#include <alaTypes.h>

#ifdef __cplusplus
    extern "C" {
#endif

/* Error Handling API */
ALA_RC ALA_ClearErrorMgr(ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_GetErrorCode(const ALA_ErrorMgr * aErrorMgr,
                              UInt         * aOutErrorCode);

ALA_RC ALA_GetErrorLevel(const ALA_ErrorMgr   * aErrorMgr,
                               ALA_ErrorLevel * aOutErrorLevel);

ALA_RC ALA_GetErrorMessage(const ALA_ErrorMgr  * aErrorMgr,
                           const SChar        ** aOutErrorMessage);


/* Logging API */
ALA_RC ALA_EnableLogging(const SChar        * aLogDirectory,
                         const SChar        * aLogFileName,
                               UInt           aFileSize,
                               UInt           aMaxFileNumber,
                               ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_DisableLogging(ALA_ErrorMgr * aOutErrorMgr);


/* Preparation API */
ALA_RC ALA_InitializeAPI(ALA_BOOL       aUseAltibaseODBCDriver,
                         ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_CreateXLogCollector(const SChar        * aXLogSenderName,
                               const SChar        * aSocketInfo,
                                     SInt           aXLogPoolSize,
                                     ALA_BOOL       aUseCommittedTxBuffer,
                                     UInt           aACKPerXLogCount,
                                     ALA_Handle   * aOutHandle,
                                     ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_AddAuthInfo(      ALA_Handle     aHandle,
                       const SChar        * aAuthInfo,
                             ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_RemoveAuthInfo(      ALA_Handle     aHandle,
                          const SChar        * aAuthInfo,
                                ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_SetHandshakeTimeout(ALA_Handle     aHandle,
                               UInt           aSecond,
                               ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_SetReceiveXLogTimeout(ALA_Handle     aHandle,
                                 UInt           aSecond,
                                 ALA_ErrorMgr * aOutErrorMgr);


/* Control API */
ALA_RC ALA_Handshake(ALA_Handle     aHandle,
                     ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_ReceiveXLog(ALA_Handle     aHandle,
                       ALA_BOOL     * aOutInsertXLogInQueue,
                       ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_GetXLog(      ALA_Handle      aHandle,
                   const ALA_XLog     ** aOutXLog,
                         ALA_ErrorMgr  * aOutErrorMgr);

ALA_RC ALA_SendACK(ALA_Handle     aHandle,
                   ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_FreeXLog(ALA_Handle     aHandle,
                    ALA_XLog     * aXLog,
                    ALA_ErrorMgr * aOutErrorMgr);


/* Finish API */
ALA_RC ALA_DestroyXLogCollector(ALA_Handle     aHandle,
                                ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_DestroyAPI(ALA_BOOL       aUseAltibaseODBCDriver,
                      ALA_ErrorMgr * aOutErrorMgr);


/* XLog API */
ALA_RC ALA_GetXLogHeader(const ALA_XLog        * aXLog,
                         const ALA_XLogHeader ** aOutXLogHeader,
                               ALA_ErrorMgr    * aOutErrorMgr);

ALA_RC ALA_GetXLogPrimaryKey(const ALA_XLog            * aXLog,
                             const ALA_XLogPrimaryKey ** aOutXLogPrimaryKey,
                                   ALA_ErrorMgr        * aOutErrorMgr);

ALA_RC ALA_GetXLogColumn(const ALA_XLog        * aXLog,
                         const ALA_XLogColumn ** aOutXLogColumn,
                               ALA_ErrorMgr    * aOutErrorMgr);

ALA_RC ALA_GetXLogSavepoint(const ALA_XLog           * aXLog,
                            const ALA_XLogSavepoint ** aOutXLogSavepoint,
                                  ALA_ErrorMgr       * aOutErrorMgr);

ALA_RC ALA_GetXLogLOB(const ALA_XLog     * aXLog,
                      const ALA_XLogLOB ** aOutXLogLOB,
                            ALA_ErrorMgr * aOutErrorMgr);


/* Meta Information API */
ALA_RC ALA_GetProtocolVersion(const ALA_ProtocolVersion * aOutProtocolVersion,
                                    ALA_ErrorMgr        * aOutErrorMgr);

ALA_RC ALA_GetReplicationInfo(      ALA_Handle         aHandle,
                              const ALA_Replication ** aOutReplication,
                                    ALA_ErrorMgr     * aOutErrorMgr);

ALA_RC ALA_GetTableInfo(      ALA_Handle      aHandle,
                              ULong           aTableOID,
                        const ALA_Table    ** aOutTable,
                              ALA_ErrorMgr  * aOutErrorMgr);

ALA_RC ALA_GetTableInfoByName(      ALA_Handle      aHandle,
                              const SChar         * aFromUserName,
                              const SChar         * aFromTableName,
                              const ALA_Table    ** aOutTable,
                                    ALA_ErrorMgr  * aOutErrorMgr);

ALA_RC ALA_GetColumnInfo(const ALA_Table     * aTable,
                               UInt            aColumnID,
                         const ALA_Column   ** aOutColumn,
                               ALA_ErrorMgr  * aOutErrorMgr);

ALA_RC ALA_GetIndexInfo(const ALA_Table     * aTable,
                              UInt            aIndexID,
                        const ALA_Index    ** aOutIndex,
                              ALA_ErrorMgr  * aOutErrorMgr);

ALA_RC ALA_IsHiddenColumn( const ALA_Column    * aColumn,
                                 ALA_BOOL      * aIsHiddenColumn,
                                 ALA_ErrorMgr  * aOutErrorMgr );

/* Monitoring */
ALA_RC ALA_GetXLogCollectorStatus(ALA_Handle                aHandle,
                                  ALA_XLogCollectorStatus * aOutXLogCollectorStatus,
                                  ALA_ErrorMgr            * aOutErrorMgr);


/* Data Type Conversion API */
ALA_RC ALA_GetAltibaseText(ALA_Column   * aColumn,
                           ALA_Value    * aValue,
                           UInt           aBufferSize,
                           SChar        * aOutBuffer,
                           ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_IsNullValue( ALA_Column   * aColumn,
                        ALA_Value    * aValue,
                        ALA_BOOL     * aOutIsNull,
                        ALA_ErrorMgr * aOutErrorMgr );

ALA_RC ALA_GetAltibaseSQL(ALA_Table    * aTable,
                          ALA_XLog     * aXLog,
                          UInt           aBufferSize,
                          SChar        * aOutBuffer,
                          ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_GetODBCCValue(ALA_Column   * aColumn,
                         ALA_Value    * aAltibaseValue,
                         SInt           aODBCCTypeID,
                         UInt           aODBCCValueBufferSize,
                         void         * aOutODBCCValueBuffer,
                         ALA_BOOL     * aOutIsNull,
                         UInt         * aOutODBCCValueSize,
                         ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_GetInternalNumericInfo(ALA_Column   * aColumn,
                                  ALA_Value    * aAltibaseValue,
                                  SInt         * aOutSign,
                                  SInt         * aOutExponent,
                                  ALA_ErrorMgr * aOutErrorMgr);

ALA_RC ALA_SetXLogPoolSize(ALA_Handle     aHandle, 
                           SInt           aXLogPoolSize,
                           ALA_ErrorMgr  *aOutErrorMgr);

#ifdef __cplusplus
    }
#endif

#endif /* _O_ALA_API_H_ */
