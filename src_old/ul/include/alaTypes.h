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
 
#ifndef _O_ALA_TYPES_H_
#define _O_ALA_TYPES_H_ 1

/*----------------------------------------------------------------*
 * Primitive Type Definition
 *----------------------------------------------------------------*/

#ifndef _O_IDTYPES_H_
#define _O_IDTYPES_H_ 1

#ifndef HAVE_LONG_LONG
#define HAVE_LONG_LONG 1
#endif

#ifndef SIZEOF_LONG
# if defined(__alpha) || defined(__sparcv9) || defined(__amd64) || defined(__LP64__) || defined(_LP64) || defined(__64BIT__)
# define SIZEOF_LONG        8
#else
# define SIZEOF_LONG        4
#endif
#endif

typedef signed char         SChar;      /* Signed   Char       8-bits */
typedef unsigned char       UChar;      /* Unsigned Char       8-bits */
typedef signed short int    SShort;     /* Signed   Small Int 16-bits */
typedef unsigned short      UShort;     /* Unsigned Small Int 16-bits */



#if (SIZEOF_LONG == 8)
typedef int                 SInt;       /* Signed   Integer   32-bits */
typedef unsigned int        UInt;       /* Unsigned Integer   32-bits */
#else
typedef long                SInt;       /* Signed   Integer   32-bits */
typedef unsigned long       UInt;       /* Unsigned Integer   32-bits */
#endif

#if (SIZEOF_LONG == 8)
typedef long                SLong;      /* Signed   Big Int   64-bits */
typedef unsigned long       ULong;      /* Unsigned Big Int   64-bits */
#else
# ifdef HAVE_LONG_LONG
typedef long long           SLong;      /* Signed   Big Int   64-bits */
typedef unsigned long long  ULong;      /* Unsigned Big Int   64-bits */
# else
struct __SLONG_STRUCT
{
    int             hiword;
    unsigned int    loword;
};
struct __ULONG_STRUCT
{
    unsigned int    hiword;
    unsigned int    loword;
};
typedef struct __SLONG_STRUCT   SLong;  /* Signed   Big Int   64-bits */
typedef struct __ULONG_STRUCT   ULong;  /* Unsigned Big Int   64-bits */


# endif
#endif

#endif /* _O_IDTYPES_H_ */

typedef unsigned char       ALA_BOOL;

#define ALA_TRUE   ((ALA_BOOL)1)
#define ALA_FALSE  ((ALA_BOOL)0)

typedef enum
{
    ALA_FAILURE = -1,
    ALA_SUCCESS =  0
} ALA_RC;

/*----------------------------------------------------------------*
 * ALTIBASE LOG ANALYSIS API
 *----------------------------------------------------------------*/

#define ALA_ERROR_MSG_LEN           (2048)
#define ALA_NAME_LEN                (40 + 1)
#define ALA_IP_LEN                  (39 + 1)
#define ALA_SOCKET_FILENAME_LEN     (250 + 1)


/* Handle */
typedef void * ALA_Handle;


/* Error Manager */
typedef struct ALA_ErrorMgr
{
    UInt   mErrorCode;      /* CODE  */
    SChar  mErrorState[6];  /* STATE */
    SChar  mErrorMessage[ALA_ERROR_MSG_LEN + 256];
} ALA_ErrorMgr;

typedef enum
{
    ALA_ERROR_FATAL = 0,    /* Need to Destroy */
    ALA_ERROR_ABORT,        /* Need to Handshake */
    ALA_ERROR_INFO          /* Information */
} ALA_ErrorLevel;


/* XLog */
typedef enum
{
    XLOG_TYPE_COMMIT            = 2,    /* Transaction Commit */
    XLOG_TYPE_ABORT             = 3,    /* Transaction Rollback */
    XLOG_TYPE_INSERT            = 4,    /* DML: Insert */
    XLOG_TYPE_UPDATE            = 5,    /* DML: Update */
    XLOG_TYPE_DELETE            = 6,    /* DML: Delete */
    XLOG_TYPE_SP_SET            = 8,    /* Savepoint Set */
    XLOG_TYPE_SP_ABORT          = 9,    /* Abort to savepoint */
    XLOG_TYPE_LOB_CURSOR_OPEN   = 14,   /* LOB Cursor open */
    XLOG_TYPE_LOB_CURSOR_CLOSE  = 15,   /* LOB Cursor close */
    XLOG_TYPE_LOB_PREPARE4WRITE = 16,   /* LOB Prepare for write */
    XLOG_TYPE_LOB_PARTIAL_WRITE = 17,   /* LOB Partial write */
    XLOG_TYPE_LOB_FINISH2WRITE  = 18,   /* LOB Finish to write */
    XLOG_TYPE_KEEP_ALIVE        = 19,   /* Keep Alive */
    XLOG_TYPE_REPL_STOP         = 21,   /* Replication Stop */
    XLOG_TYPE_LOB_TRIM          = 35,   /* LOB Trim */
    XLOG_TYPE_CHANGE_META       = 25    /* Meta change by DDL */
} ALA_XLogType;

typedef UInt   ALA_TID;     /* Transaction ID */
typedef ULong  ALA_SN;      /* Log Record SN */
typedef struct ALA_Value    /* Altibase Internal Data */
{
    UInt         length;        /* Length of value */
    const void * value;
} ALA_Value;

typedef struct ALA_XLogHeader       /* XLog Header */
{
    ALA_XLogType mType;         /* XLog Type */
    ALA_TID      mTID;          /* Transaction ID */
    ALA_SN       mSN;           /* SN */
    ALA_SN       mSyncSN;       /* Reserved */
    ALA_SN       mRestartSN;    /* Used internally */
    ULong        mTableOID;     /* Table OID */
} ALA_XLogHeader;

typedef struct ALA_XLogPrimaryKey   /* Primary Key */
{
    UInt         mPKColCnt;     /* Primary Key Column Count */
    ALA_Value   *mPKColArray;   /* Priamry Key Column Value Array */
} ALA_XLogPrimaryKey;

typedef struct ALA_XLogColumn       /* Column */
{
    UInt         mColCnt;       /* Column Count */
    UInt        *mCIDArray;     /* Column ID Array */
    ALA_Value   *mBColArray;    /* Before Image Column Value Array (Only for Update) */
    ALA_Value   *mAColArray;    /* After Image Column Value Array */
} ALA_XLogColumn;

typedef struct ALA_XLogSavepoint    /* Savepoint */
{
    UInt         mSPNameLen;    /* Savepoint Name Length */
    SChar       *mSPName;       /* Savepoint Name */
} ALA_XLogSavepoint;

typedef struct ALA_XLogLOB          /* LOB (LOB Column does not have Before Image) */
{
    ULong        mLobLocator;   /* LOB Locator of Altibase */
    UInt         mLobColumnID;
    UInt         mLobOffset;
    UInt         mLobOldSize;
    UInt         mLobNewSize;
    UInt         mLobPieceLen;
    UChar       *mLobPiece;
} ALA_XLogLOB;

typedef struct ALA_XLog             /* XLog */
{
    ALA_XLogHeader      mHeader;
    ALA_XLogPrimaryKey  mPrimaryKey;
    ALA_XLogColumn      mColumn;
    ALA_XLogSavepoint   mSavepoint;
    ALA_XLogLOB         mLOB;

    /* Used internally */
    struct ALA_XLog    *mPrev;
    struct ALA_XLog    *mNext;
} ALA_XLog;


/* Meta Information */
typedef struct ALA_ProtocolVersion
{
    UShort      mMajor;             /* Major Version */
    UShort      mMinor;             /* Minor Version */
    UShort      mFix;               /* Fix Version */
} ALA_ProtocolVersion;

typedef struct ALA_Column
{
    UInt         mColumnID;                     /* Column ID */
    SChar        mColumnName[ALA_NAME_LEN];     /* Column Name */
    UInt         mDataType;                     /* Column Data Type */
    UInt         mLanguageID;                   /* Column Language ID */
    UInt         mSize;                         /* Column Size */
    SInt         mPrecision;                    /* Column Precision */
    SInt         mScale;                        /* Column Scale */
    ALA_BOOL     mNotNull;                      /* Column Not Null? */
    ALA_BOOL     mEncrypt;                      /* Column Encrypt? */
    UInt         mFlags;                        /* Column flags */
} ALA_Column;

typedef struct ALA_Index
{
    UInt         mIndexID;                      /* Index ID */
    SChar        mIndexName[ALA_NAME_LEN];      /* Index Name */
    ALA_BOOL     mUnique;                       /* Index Unique? */
    UInt         mColumnCount;                  /* Index Column Count */
    UInt        *mColumnIDArray;                /* Index Column ID Array */
} ALA_Index;

#define ALA_MAX_KEY_COLUMN_COUNT                (32)

typedef struct ALA_Check
{
    SChar        mName[ALA_NAME_LEN];           /* Check Name */
    UInt         mConstraintID;                 /* Constranit ID */
    UInt         mConstraintColumn[ALA_MAX_KEY_COLUMN_COUNT];   /* Constranit ID */
    SChar       *mCheckCondition;
    UInt         mConstraintColumnCount;
} ALA_Check;

typedef struct ALA_Table
{
    ULong        mTableOID;                     /* Table OID */
    SChar        mFromUserName[ALA_NAME_LEN];   /* (From) User Name */
    SChar        mFromTableName[ALA_NAME_LEN];  /* (From) Table Name */
    SChar        mToUserName[ALA_NAME_LEN];     /* (To) User Name */
    SChar        mToTableName[ALA_NAME_LEN];    /* (To) Table Name */
    UInt         mPKIndexID;                    /* Index ID of Primary Key */
    UInt         mPKColumnCount;                /* Primary Key Column Count */
    ALA_Column **mPKColumnArray;                /* Primary Key Column Array */
    UInt         mColumnCount;                  /* Column Count */
    ALA_Column  *mColumnArray;                  /* Column Array */
    UInt         mIndexCount;                   /* Index Count */
    ALA_Index   *mIndexArray;                   /* Index Array */
    UInt         mCheckCount;                   /* Check Constraint Count */
    ALA_Check   *mChecks;                       /* Check Constraint */
} ALA_Table;

typedef struct ALA_Replication
{
    SChar        mXLogSenderName[ALA_NAME_LEN]; /* XLog Sender Name */
    UInt         mTableCount;                   /* Table Count */
    ALA_Table   *mTableArray;                   /* Table Array */
    SChar        mDBCharSet[ALA_NAME_LEN];      /* DB Charter Set */
    SChar        mDBNCharSet[ALA_NAME_LEN];     /* DB National Charter Set */
    ULong        mSenderVersion;                /* Sender Version */    
} ALA_Replication;


/* Monitoring */
typedef struct ALA_XLogCollectorStatus
{
    SChar       mMyIP[ALA_IP_LEN];                     /* [TCP] XLog Collector IP */
    SInt        mMyPort;                               /* [TCP] XLog Collector Port */
    SChar       mPeerIP[ALA_IP_LEN];                   /* [TCP] XLog Sender IP */
    SInt        mPeerPort;                             /* [TCP] XLog Sender Port */
    SChar       mSocketFile[ALA_SOCKET_FILENAME_LEN];  /* [UNIX] Socket File Name */
    UInt        mXLogCountInPool;                      /* XLog Count in XLog Pool */
    ALA_SN      mLastArrivedSN;                        /* Last Arrived SN */
    ALA_SN      mLastProcessedSN;                      /* Last Processed SN */
    ALA_BOOL    mNetworkValid;                         /* Network is valid? */
} ALA_XLogCollectorStatus;

#endif /* _O_ALA_TYPES_H_ */
