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
 
#ifndef _O_ULA_META_H_
#define _O_ULA_META_H_ 1

#include <cmAllClient.h>
#include <ulaErrorMgr.h>
#include <mtcl.h>
#define ULA_ROLE_ANALYSIS   (1)
#define ULA_ROLE_ANALYSIS_PROPAGATION   (4)

/* Replication Handshake Flags */
// 1번 비트 : Endian bit : 0 - Big Endian, 1 - Little Endian
#define ULA_LITTLE_ENDIAN                   (0x00000001)
#define ULA_BIG_ENDIAN                      (0x00000000)
#define ULA_ENDIAN_MASK                     (0x00000001)

// 3번 비트 : Wakeup Peer Sender
#define ULA_WAKEUP_PEER_SENDER_FLAG_SET     (0x00000004)
#define ULA_WAKEUP_PEER_SENDER_FLAG_UNSET   (0x00000000)
#define ULA_WAKEUP_PEER_SENDER_MASK         (0x00000004)

#define ULA_MAX_KEY_COLUMN_COUNT            32


/* PROJ-1090 Function-based Index
 *  Hidden Column인지 여부: QCM_COLUMN_HIDDEN_COLUMN_MASK 참조
 */
#define ULN_QPFLAG_HIDDEN_COLUMN_MASK            (0x00002000)
#define ULN_QPFLAG_HIDDEN_COLUMN_FALSE           (0x00000000)
#define ULN_QPFLAG_HIDDEN_COLUMN_TRUE            (0x00002000)

typedef enum
{
    ULA_META_DICTTABLECOUNT  = 0,
    ULA_META_PARTITIONCOUNT  = 1,
    ULA_META_XSN             = 2,
    ULA_META_SRID            = 3,
    ULA_META_COMPRESSTYPE    = 4,
    ULA_META_MAX
} ULA_PROTOCOL_OP_CODE;

typedef struct ulaTable ulaTable;
typedef struct ulaIndex ulaIndex;

typedef struct ulaProtocolVersion
{
    acp_uint16_t    mMajor;                         /* Major Version */
    acp_uint16_t    mMinor;                         /* Minor Version */
    acp_uint16_t    mFix;                           /* Fix Version */
} ulaProtocolVersion;

typedef struct ulaReplication
{
    acp_char_t    mXLogSenderName[ULA_NAME_LEN];  /* XLog Sender Name */
    acp_uint32_t  mTableCount;                    /* Table Count */
    ulaTable     *mTableArray;                    /* Table Array */
    acp_char_t    mDBCharSet[ULA_NAME_LEN];       /* DB Charter Set */
    acp_char_t    mDBNCharSet[ULA_NAME_LEN];      /* DB National Charter Set */
    acp_uint64_t  mSenderVersion;                 /* Sender Version */
    cmiCompressType mCompressType;                /* Compress Type */
    
} ulaReplication;

typedef struct ulaReplTempInfo
{
    acp_sint32_t mRole;              /* ROLE (Internal only) */
    acp_uint32_t mFlags;             /* Replication Flags (Internal only) */
    acp_uint32_t mTransTableSize;    /* Transaction Table Size */
} ulaReplTempInfo;

typedef struct ulaColumn
{
    acp_uint32_t    mColumnID;                      /* Column ID */
    acp_char_t      mColumnName[ULA_NAME_LEN];      /* Column Name */
    acp_uint32_t    mDataType;                      /* Column Data Type */
    acp_uint32_t    mLanguageID;                    /* Column Language ID */
    acp_uint32_t    mSize;                          /* Column Size */
    acp_sint32_t    mPrecision;                     /* Column Precision */
    acp_sint32_t    mScale;                         /* Column Scale */
    acp_bool_t      mNotNull;                       /* Column Not Null? */
    acp_bool_t      mEncrypt;                       /* Column Encrypt? */
    acp_uint32_t    mQPFlag;                        /* Column Hidden? */
} ulaColumn;

typedef struct ulaCheck
{
    acp_char_t      mName[ULA_NAME_LEN];
    acp_uint32_t    mConstraintID;
    acp_uint32_t    mConstraintColumn[ULA_MAX_KEY_COLUMN_COUNT];
    acp_char_t     *mCheckCondition;
    acp_uint32_t    mConstraintColumnCount;
} ulaCheck;

struct ulaTable
{
    acp_uint64_t   mTableOID;                    /* Table OID */
    acp_char_t     mFromUserName[ULA_NAME_LEN];  /* (From) User Name */
    acp_char_t     mFromTableName[ULA_NAME_LEN]; /* (From) Table Name */
    acp_char_t     mToUserName[ULA_NAME_LEN];    /* (To) User Name */
    acp_char_t     mToTableName[ULA_NAME_LEN];   /* (To) Table Name */
    acp_uint32_t   mPKIndexID;                   /* Index ID of Primary Key */
    acp_uint32_t   mPKColumnCount;               /* Primary Key Column Count */
    ulaColumn    **mPKColumnArray;               /* Primary Key Column Array */
    acp_uint32_t   mColumnCount;                 /* Column Count */
    ulaColumn     *mColumnArray;                 /* Column Array */
    acp_uint32_t   mIndexCount;                  /* Index Count */
    ulaIndex      *mIndexArray;                  /* Index Array */
    acp_uint32_t   mCheckCount;                  /* check constraint count */
    ulaCheck      *mChecks;                      /* check constraint */
};

struct ulaIndex
{
    acp_uint32_t    mIndexID;                       /* Index ID */
    acp_char_t      mIndexName[ULA_NAME_LEN];       /* Index Name */
    acp_bool_t      mUnique;                        /* Index Unique? */
    acp_uint32_t    mColumnCount;                   /* Index Column Count */
    acp_uint32_t   *mColumnIDArray;                 /* Index Column ID Array */
};

typedef struct ulaVersion
{
    acp_uint64_t       mVersion;
} ulaVersion;

/*
 * -----------------------------------------------------------------------------
 *  ulaMeta structure
 * -----------------------------------------------------------------------------
 */
typedef struct ulaMeta
{
    ulaReplication   mReplication;
    ulaTable       **mItemOrderByTableOID;
    ulaTable       **mItemOrderByTableName;

    acp_bool_t       mEndianDiff;
} ulaMeta;


/*
 * -----------------------------------------------------------------------------
 *  ulaMeta interfaces
 * -----------------------------------------------------------------------------
 */
void ulaMetaInitialize(ulaMeta *aMeta);
void ulaMetaDestroy(ulaMeta *aMeta);

acp_bool_t ulaMetaIsEndianDiff(ulaMeta *aMeta);

ACI_RC ulaMetaGetProtocolVersion(ulaProtocolVersion *aOutProtocolVersion,
                                 ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaMetaSendMeta( cmiProtocolContext * aProtocolContext,
                        acp_char_t         * aRepName,
                        acp_uint64_t         aRemoteVersion,
                        acp_uint32_t         aFlag,
                        ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaMetaRecvMeta( ulaMeta            * aMeta,
                        cmiProtocolContext * aProtocolContext,
                        acp_uint32_t         aTimeoutSec,
                        ulaVersion           aOutReplVersion,
                        acp_char_t         * aXLogSenderName,
                        acp_uint32_t       * aOutTransTableSize,
                        ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaMetaSortMeta(ulaMeta *aMeta, ulaErrorMgr *aOutErrorMgr);

ACI_RC ulaMetaGetReplicationInfo(ulaMeta         *aMeta,
                                 ulaReplication **aOutReplication,
                                 ulaErrorMgr     *aOutErrorMgr);

ACI_RC ulaMetaGetTableInfo(ulaMeta       *aMeta,
                           acp_uint64_t   aTableOID,
                           ulaTable     **aOutTable,
                           ulaErrorMgr   *aOutErrorMgr);

ACI_RC ulaMetaGetTableInfoByName(ulaMeta           *aMeta,
                                 const acp_char_t  *aFromUserName,
                                 const acp_char_t  *aFromTableName,
                                 ulaTable         **aOutTable,
                                 ulaErrorMgr       *aOutErrorMgr);

ACI_RC ulaMetaGetColumnInfo(ulaTable      *aTable,
                            acp_uint32_t   aColumnID,
                            ulaColumn    **aOutColumn,
                            ulaErrorMgr   *aOutErrorMgr);

ACI_RC ulaMetaGetIndexInfo(ulaTable      *aTable,
                           acp_uint32_t   aIndexID,
                           ulaIndex     **aOutIndex,
                           ulaErrorMgr   *aOutErrorMgr);

acp_bool_t ulaMetaIsHiddenColumn( ulaColumn  * aColumn );

acp_bool_t needToProcessProtocolOperation( ULA_PROTOCOL_OP_CODE aOpCode,
                                           acp_uint64_t         aVersion );
#endif /* _O_ULA_META_H_ */
