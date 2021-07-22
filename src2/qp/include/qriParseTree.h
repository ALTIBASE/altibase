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
 

/***********************************************************************
 * $Id: qrParseTree.h 48038 2011-07-12 08:48:06Z hssong $
 **********************************************************************/

#ifndef _O_QRI_PARSE_TREE_H_
#define _O_QRI_PARSE_TREE_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <rp.h>

typedef qcNamePosition  qciNamePosition;
typedef qcNamePosList   qciNamePosList;
typedef qcParseTree     qciParseTree;

typedef struct qriParseTree
{
    qciParseTree          common;
    // members for only REPLICATION statement
    qciNamePosition       replName;
    struct qriReplHost  * hosts;
    SInt                  conflictResolution; // QRX_CONFLICT_RESOLUTION
    struct qriReplItem  * replItems;
    SInt                  replMode;
    idBool                replModeSelected;   // BUG-17616
    SInt                  parallelFactor;
    RP_SENDER_TYPE        startType;
    idBool                isImmediateStop;
    SInt                  startOption;        // BUG-18714
    ULong                 startSN;
    SInt                  role;
    //replication create시 설정된 옵션 리스트를 flag로 표시 0x00000001(recovery)
    //alter replication set시 설정된 옵션을 flag로 표시
    /* PROJ-1915 */
    struct qriReplOptions* replOptions;

    // To Fix PR-10590
    // Replication Flush Option
    rpFlushOption         flushOption;
    
    // BUG-21761
    // N타입을 U타입으로 변형시킬 때 사용
    qciNamePosList       * ncharList;

    // DDL 과  RP 의 lock 순서를 보장 해주기 위해 사용
    void                 * lockTable;
    
} qriParseTree;

#define QR_PARSE_TREE_INIT( _dst_ )                     \
{                                                       \
    SET_EMPTY_POSITION(_dst_->replName);                \
    _dst_->common.stmtKind = QCI_STMT_NON_SCHEMA_DDL;   \
    _dst_->hosts           = NULL;                      \
    _dst_->replOptions     = NULL;                      \
    _dst_->replItems       = NULL;                      \
    _dst_->parallelFactor  = 1;                         \
    _dst_->startType       = RP_NORMAL;                 \
    _dst_->isImmediateStop = ID_FALSE;                  \
    _dst_->startOption     = RP_START_OPTION_NONE;      \
    _dst_->replMode        = RP_LAZY_MODE;              \
    _dst_->ncharList       = NULL;                      \
    _dst_->role            = RP_ROLE_REPLICATION;       \
}

typedef struct qriSessionParseTree
{
    qciParseTree         common;
    UInt                 replModeFlag;
} qriSessionParseTree;

typedef struct qriReplHost
{
    qciNamePosition         hostIp;
    UInt                    portNumber;
    struct qriReplConnOpt * connOpt;

    struct qriReplHost    * next;
} qriReplHost;

typedef struct qriReplConnOpt
{
    RP_SOCKET_TYPE          connType;
    rpIBLatency             ibLatency;
} qriReplConnOpt;

/* PROJ-1915 : replication reption 확장  for off-line replication */
typedef struct qriReplDirPath
{
    qciNamePosition         path;
    struct qriReplDirPath * next;
} qriReplDirPath;

typedef struct qriReplApplierBuffer
{
    ULong size;
    UChar type;
} qriReplApplierBuffer;

typedef struct qriReplOptions
{
    UInt                    optionsFlag;
    qriReplDirPath        * logDirPath;
    ULong                   parallelReceiverApplyCount;
    qriReplApplierBuffer  * applierBuffer;
    qciNamePosition         peerReplName;
    struct qriReplOptions * next;
} qriReplOptions;

#define QR_REPL_OPTIONS_INIT( _dst_ )                       \
{                                                           \
    (_dst_)->optionsFlag                = 0;                \
    (_dst_)->logDirPath                 = NULL;             \
    (_dst_)->parallelReceiverApplyCount = ID_ULONG(0);      \
    (_dst_)->applierBuffer              = NULL;             \
    SET_EMPTY_POSITION( (_dst_)->peerReplName );            \
    (_dst_)->next                       = NULL;             \
}

// BUG-29115
typedef struct qriReplStartOption
{
    UInt                   startOption;
    ULong                  startSN;
} qriReplStartOption;

typedef struct qriReplItem
{
    qciNamePosition      localUserName;
    qciNamePosition      localTableName;
    qciNamePosition      localPartitionName;
    qciNamePosition      remoteUserName;
    qciNamePosition      remoteTableName;
    qciNamePosition      remotePartitionName;
    // validation information
    UInt                 localUserID;
    qciNamePosList     * ncharLiteralPos; // PROJ-1579
    qriReplItem        * next;
    rpReplicationUnit    replication_unit; // PROJ-2336
} qriReplItem;

#endif /* _O_QRI_PARSE_TREE_H_ */
 
