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

#include <cmAll.h>

cmpOpMap gCmpOpRPMap[] =
{
    {"CMP_OP_RP_Version"            } ,
    {"CMP_OP_RP_MetaRepl"           } ,
    {"CMP_OP_RP_MetaReplTbl"        } ,
    {"CMP_OP_RP_MetaReplCol"        } ,
    {"CMP_OP_RP_MetaReplIdx"        } ,
    {"CMP_OP_RP_MetaReplIdxCol"     } ,
    {"CMP_OP_RP_HandshakeAck"       } ,
    {"CMP_OP_RP_TrBegin"            } ,
    {"CMP_OP_RP_TrCommit"           } ,
    {"CMP_OP_RP_TrAbort"            } ,
    {"CMP_OP_RP_SPSet"              } ,
    {"CMP_OP_RP_SPAbort"            } ,
    {"CMP_OP_RP_StmtBegin"          } ,
    {"CMP_OP_RP_StmtEnd"            } ,
    {"CMP_OP_RP_CursorOpen"         } ,
    {"CMP_OP_RP_CursorClose"        } ,
    {"CMP_OP_RP_Insert"             } ,
    {"CMP_OP_RP_Update"             } ,
    {"CMP_OP_RP_Delete"             } ,
    {"CMP_OP_RP_UIntID"             } ,
    {"CMP_OP_RP_Value"              } ,
    {"CMP_OP_RP_Stop"               } ,
    {"CMP_OP_RP_KeepAlive"          } ,
    {"CMP_OP_RP_Flush"              } ,
    {"CMP_OP_RP_FlushAck"           } ,
    {"CMP_OP_RP_Ack"                } ,
    {"CMP_OP_RP_LobCursorOpen"      } ,
    {"CMP_OP_RP_LobCursorClose"     } ,
    {"CMP_OP_RP_LobPrepare4Write"   } ,
    {"CMP_OP_RP_LobPartialWrite"    } ,
    {"CMP_OP_RP_LobFinish2Write"    } ,
    {"CMP_OP_RP_TxAck"              } ,
    {"CMP_OP_RP_RequestAck"         } ,
    {"CMP_OP_RP_Handshake"          } ,
    {"CMP_OP_RP_SyncPKBegin"        } ,
    {"CMP_OP_RP_SyncPK"             } ,
    {"CMP_OP_RP_SyncPKEnd"          } ,
    {"CMP_OP_RP_FailbackEnd"        } ,
    {"CMP_OP_RP_SyncTableNumber"    } ,
    {"CMP_OP_RP_SyncStart"          } ,
    {"CMP_OP_RP_SyncEnd"            } ,
    {"CMP_OP_RP_LobTrim"            } ,
    {"CMP_OP_RP_MetaReplCheck"      } ,
    {"CMP_OP_RP_MetaDictTableCount" } ,
    {"CMP_OP_RP_AckonDML"           } ,
    {"CMP_OP_RP_AckEager"           } ,
    {"CMP_OP_RP_DDLSyncInfo"        } ,
    {"CMP_OP_RP_DDLSyncMsg"         } ,
    {"CMP_OP_RP_DDLSyncMsgAck"      } ,
    {"CMP_OP_RP_DDLSyncCancel"      } ,
    {"CMP_OP_RP_DDLReplicateHandshake"      }, /* BUG-46252 Partition Merge / Split / Replace DDL asynchronization support */
    {"CMP_OP_RP_DDLReplicateQueryStatement" },
    {"CMP_OP_RP_DDLReplicateExecute"},
    {"CMP_OP_RP_DDLReplicateAck"    },
    {"CMP_OP_RP_MetaPartitionCount" },
    {"CMP_OP_RP_MetaInitialize    " },
    {"CMP_OP_RP_TemporarySyncInfo"  },
    {"CMP_OP_RP_TemporarySyncItem"  },
    {"CMP_OP_RP_TemporarySyncHandshakeAck"  },
    {"CMP_OP_RP_MetaReplTblCondition"       },
    {"CMP_OP_RP_MetaReplTblConditionResult" },
    {"CMP_OP_RP_Truncate"           },
    {"CMP_OP_RP_TruncateAck"        },
    {"CMP_OP_RP_XA_START_REQ"       },  /* PROJ-2747 Global Tx Consistent */
    {"CMP_OP_RP_XA_PREPARE_REQ"     },
    {"CMP_OP_RP_XA_PREPARE"         },
    {"CMP_OP_RP_XA_COMMIT"          },
    {"CMP_OP_RP_XA_END"             },
    {"CMP_OP_RP_MAX_VER1"           }
};


IDE_RC cmpArgInitializeRPMetaRepl(cmpProtocol *aProtocol)
{
    cmpArgRPMetaRepl *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, MetaRepl);

    IDE_TEST(cmtVariableInitialize(&sArg->mRepName) != IDE_SUCCESS);

    /* PROJ-1915 */
    IDE_TEST(cmtVariableInitialize(&sArg->mOSInfo) != IDE_SUCCESS);

    IDE_TEST(cmtVariableInitialize(&sArg->mDBCharSet) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mNationalCharSet) != IDE_SUCCESS);

    IDE_TEST(cmtVariableInitialize(&sArg->mServerID) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mRemoteFaultDetectTime) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeRPMetaRepl(cmpProtocol *aProtocol)
{
    cmpArgRPMetaRepl *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, MetaRepl);

    IDE_TEST(cmtVariableFinalize(&sArg->mRepName) != IDE_SUCCESS);

    /* PROJ-1915 */
    IDE_TEST(cmtVariableFinalize(&sArg->mOSInfo) != IDE_SUCCESS);

    IDE_TEST(cmtVariableFinalize(&sArg->mDBCharSet) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mNationalCharSet) != IDE_SUCCESS);

    IDE_TEST(cmtVariableFinalize(&sArg->mServerID) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mRemoteFaultDetectTime) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeRPMetaReplTbl(cmpProtocol *aProtocol)
{
    cmpArgRPMetaReplTbl *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, MetaReplTbl);

    IDE_TEST(cmtVariableInitialize(&sArg->mRepName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mLocalUserName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mLocalTableName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mLocalPartName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mRemoteUserName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mRemoteTableName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mRemotePartName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mPartCondMinValues) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mPartCondMaxValues) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mConditionStr) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeRPMetaReplTbl(cmpProtocol *aProtocol)
{
    cmpArgRPMetaReplTbl *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, MetaReplTbl);

    IDE_TEST(cmtVariableFinalize(&sArg->mRepName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mLocalUserName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mLocalTableName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mLocalPartName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mRemoteUserName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mRemoteTableName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mRemotePartName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mPartCondMinValues) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mPartCondMaxValues) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mConditionStr) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeRPMetaReplCol(cmpProtocol *aProtocol)
{
    cmpArgRPMetaReplCol *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, MetaReplCol);

    IDE_TEST(cmtVariableInitialize(&sArg->mColumnName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mPolicyName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mPolicyCode) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mECCPolicyName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mECCPolicyCode) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeRPMetaReplCol(cmpProtocol *aProtocol)
{
    cmpArgRPMetaReplCol *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, MetaReplCol);

    IDE_TEST(cmtVariableFinalize(&sArg->mColumnName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mPolicyName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mPolicyCode) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mECCPolicyName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mECCPolicyCode) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeRPMetaReplIdx(cmpProtocol *aProtocol)
{
    cmpArgRPMetaReplIdx *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, MetaReplIdx);

    IDE_TEST(cmtVariableInitialize(&sArg->mIndexName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeRPMetaReplIdx(cmpProtocol *aProtocol)
{
    cmpArgRPMetaReplIdx *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, MetaReplIdx);

    IDE_TEST(cmtVariableFinalize(&sArg->mIndexName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeRPHandshakeAck(cmpProtocol *aProtocol)
{
    cmpArgRPHandshakeAck *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, HandshakeAck);

    IDE_TEST(cmtVariableInitialize(&sArg->mMsg) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeRPHandshakeAck(cmpProtocol *aProtocol)
{
    cmpArgRPHandshakeAck *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, HandshakeAck);

    IDE_TEST(cmtVariableFinalize(&sArg->mMsg) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeRPSPSet(cmpProtocol *aProtocol)
{
    cmpArgRPSPSet *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, SPSet);

    IDE_TEST(cmtVariableInitialize(&sArg->mSPName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeRPSPSet(cmpProtocol *aProtocol)
{
    cmpArgRPSPSet *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, SPSet);

    IDE_TEST(cmtVariableFinalize(&sArg->mSPName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeRPSPAbort(cmpProtocol *aProtocol)
{
    cmpArgRPSPAbort *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, SPAbort);

    IDE_TEST(cmtVariableInitialize(&sArg->mSPName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeRPSPAbort(cmpProtocol *aProtocol)
{
    cmpArgRPSPAbort *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, SPAbort);

    IDE_TEST(cmtVariableFinalize(&sArg->mSPName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeRPValue(cmpProtocol *aProtocol)
{
    cmpArgRPValue *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, Value);

    IDE_TEST(cmtVariableInitialize(&sArg->mValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeRPValue(cmpProtocol *aProtocol)
{
    cmpArgRPValue *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, Value);

    IDE_TEST(cmtVariableFinalize(&sArg->mValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeRPPartialWrite(cmpProtocol *aProtocol)
{
    cmpArgRPLobPartialWrite *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, LobPartialWrite);

    IDE_TEST(cmtVariableInitialize(&sArg->mPieceValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeRPPartialWrite(cmpProtocol *aProtocol)
{
    cmpArgRPLobPartialWrite *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, RP, LobPartialWrite);

    IDE_TEST(cmtVariableFinalize(&sArg->mPieceValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


cmpArgFunction gCmpArgInitializeFunctionRP[CMP_OP_RP_MAX] =
{
    cmpArgNULL,                       /* CMP_OP_RP_Version          */
    cmpArgInitializeRPMetaRepl,       /* CMP_OP_RP_MetaRepl         */
    cmpArgInitializeRPMetaReplTbl,    /* CMP_OP_RP_MetaReplTbl      */
    cmpArgInitializeRPMetaReplCol,    /* CMP_OP_RP_MetaReplCol      */
    cmpArgInitializeRPMetaReplIdx,    /* CMP_OP_RP_MetaReplIdx      */
    cmpArgNULL,                       /* CMP_OP_RP_MetaReplIdxCol   */
    cmpArgInitializeRPHandshakeAck,   /* CMP_OP_RP_HandshakeAck     */
    cmpArgNULL,                       /* CMP_OP_RP_TrBegin          */
    cmpArgNULL,                       /* CMP_OP_RP_TrCommit         */
    cmpArgNULL,                       /* CMP_OP_RP_TrAbort          */
    cmpArgInitializeRPSPSet,          /* CMP_OP_RP_SPSet            */
    cmpArgInitializeRPSPAbort,        /* CMP_OP_RP_SPAbort          */
    cmpArgNULL,                       /* CMP_OP_RP_StmtBegin        */
    cmpArgNULL,                       /* CMP_OP_RP_StmtEnd          */
    cmpArgNULL,                       /* CMP_OP_RP_CursorOpen       */
    cmpArgNULL,                       /* CMP_OP_RP_CursorClose      */
    cmpArgNULL,                       /* CMP_OP_RP_Insert           */
    cmpArgNULL,                       /* CMP_OP_RP_Update           */
    cmpArgNULL,                       /* CMP_OP_RP_Delete           */
    cmpArgNULL,                       /* CMP_OP_RP_CID              */
    cmpArgInitializeRPValue,          /* CMP_OP_RP_Value            */
    cmpArgNULL,                       /* CMP_OP_RP_Stop             */
    cmpArgNULL,                       /* CMP_OP_RP_KeepAlive        */
    cmpArgNULL,                       /* CMP_OP_RP_Flush            */
    cmpArgNULL,                       /* CMP_OP_RP_FlushAck         */
    cmpArgNULL,                       /* CMP_OP_RP_Ack              */
    cmpArgNULL,                       /* CMP_OP_RP_LobCursorOpen    */
    cmpArgNULL,                       /* CMP_OP_RP_LobCursorClose   */
    cmpArgNULL,                       /* CMP_OP_RP_LobPrepare4Write */
    cmpArgInitializeRPPartialWrite,   /* CMP_OP_RP_LobPartialWrite  */
    cmpArgNULL,                       /* CMP_OP_RP_LobFinish2Write  */
    cmpArgNULL,                       /* CMP_OP_RP_TxAck            */
    cmpArgNULL,                       /* CMP_OP_RP_RequestAck       */
    cmpArgNULL,                       /* CMP_OP_RP_Handshake        */
    cmpArgNULL,                       /* CMP_OP_RP_SyncPKBegin      */
    cmpArgNULL,                       /* CMP_OP_RP_SyncPK           */
    cmpArgNULL,                       /* CMP_OP_RP_SyncPKEnd        */
    cmpArgNULL,                       /* CMP_OP_RP_FailbackEnd      */
    cmpArgNULL,                       /* CMP_OP_RP_SyncTableNumber  */
    cmpArgNULL,                       /* CMP_OP_RP_SyncStart        */
    cmpArgNULL,                       /* CMP_OP_RP_SyncEnd          */
    cmpArgNULL,                       /* CMP_OP_RP_LobTrim          */
    cmpArgNULL,                       /* CMP_OP_RP_MetaReplCheck    */
    cmpArgNULL,                       /* CMP_OP_RP_MetaDictTableCount*/
    cmpArgNULL,                       /* CMP_OP_RP_AckonDML         */
    cmpArgNULL,                       /* CMP_OP_RP_AckEager         */
    cmpArgNULL,                       /* CMP_OP_RP_DDLSyncInfo      */
    cmpArgNULL,                       /* CMP_OP_RP_DDLSyncMsg       */
    cmpArgNULL,                       /* CMP_OP_RP_DDLSyncMsgAck    */
    cmpArgNULL,                       /* CMP_OP_RP_DDLSyncCancel    */
    cmpArgNULL,                       /* CMP_OP_RP_DDLReplicateHandshake      */ 
    cmpArgNULL,                       /* CMP_OP_RP_DDLReplicateQueryStatement */
    cmpArgNULL,                       /* CMP_OP_RP_DDLReplicateExecute        */
    cmpArgNULL,                       /* CMP_OP_RP_DDLReplicateAck  */
    cmpArgNULL,                       /* CMP_OP_RP_MetaPartitionCount */
    cmpArgNULL,                       /* CMP_OP_RP_MetaInitialize */
    cmpArgNULL,                       /* CMP_OP_RP_TemporarySyncInfo */
    cmpArgNULL,                       /* CMP_OP_RP_TemporarySyncItem */
    cmpArgNULL,                       /* CMP_OP_RP_TemporarySyncHandshakeAck */
    cmpArgNULL,                       /* CMP_OP_RP_MetaReplTblCondition */
    cmpArgNULL,                       /* CMP_OP_RP_MetaReplTblConditionResult */
    cmpArgNULL,                       /* CMP_OP_RP_Truncate */
    cmpArgNULL,                       /* CMP_OP_RP_TruncateAck */
    cmpArgNULL,                       /* CMP_OP_RP_XA_START_REQ */
    cmpArgNULL,                       /* CMP_OP_RP_XA_PREPARE_REQ */
    cmpArgNULL,                       /* CMP_OP_RP_XA_PREPARE */
    cmpArgNULL,                       /* CMP_OP_RP_XA_COMMIT */
    cmpArgNULL                        /* CMP_OP_RP_XA_END */
};

cmpArgFunction gCmpArgFinalizeFunctionRP[CMP_OP_RP_MAX] =
{
    cmpArgNULL,                       /* CMP_OP_RP_Version          */
    cmpArgFinalizeRPMetaRepl,         /* CMP_OP_RP_MetaRepl         */
    cmpArgFinalizeRPMetaReplTbl,      /* CMP_OP_RP_MetaReplTbl      */
    cmpArgFinalizeRPMetaReplCol,      /* CMP_OP_RP_MetaReplCol      */
    cmpArgFinalizeRPMetaReplIdx,      /* CMP_OP_RP_MetaReplIdx      */
    cmpArgNULL,                       /* CMP_OP_RP_MetaReplIdxCol   */
    cmpArgFinalizeRPHandshakeAck,     /* CMP_OP_RP_HandshakeAck     */
    cmpArgNULL,                       /* CMP_OP_RP_TrBegin          */
    cmpArgNULL,                       /* CMP_OP_RP_TrCommit         */
    cmpArgNULL,                       /* CMP_OP_RP_TrAbort          */
    cmpArgFinalizeRPSPSet,            /* CMP_OP_RP_SPSet            */
    cmpArgFinalizeRPSPAbort,          /* CMP_OP_RP_SPAbort          */
    cmpArgNULL,                       /* CMP_OP_RP_StmtBegin        */
    cmpArgNULL,                       /* CMP_OP_RP_StmtEnd          */
    cmpArgNULL,                       /* CMP_OP_RP_CursorOpen       */
    cmpArgNULL,                       /* CMP_OP_RP_CursorClose      */
    cmpArgNULL,                       /* CMP_OP_RP_Insert           */
    cmpArgNULL,                       /* CMP_OP_RP_Update           */
    cmpArgNULL,                       /* CMP_OP_RP_Delete           */
    cmpArgNULL,                       /* CMP_OP_RP_CID              */
    cmpArgFinalizeRPValue,            /* CMP_OP_RP_Value            */
    cmpArgNULL,                       /* CMP_OP_RP_Stop             */
    cmpArgNULL,                       /* CMP_OP_RP_KeepAlive        */
    cmpArgNULL,                       /* CMP_OP_RP_Flush            */
    cmpArgNULL,                       /* CMP_OP_RP_FlushAck         */
    cmpArgNULL,                       /* CMP_OP_RP_Ack              */
    cmpArgNULL,                       /* CMP_OP_RP_LobCursorOpen    */
    cmpArgNULL,                       /* CMP_OP_RP_LobCursorClose   */
    cmpArgNULL,                       /* CMP_OP_RP_LobPrepare4Write */
    cmpArgFinalizeRPPartialWrite,     /* CMP_OP_RP_LobPartialWrite  */
    cmpArgNULL,                       /* CMP_OP_RP_LobFinish2Write  */
    cmpArgNULL,                       /* CMP_OP_RP_TxAck            */
    cmpArgNULL,                       /* CMP_OP_RP_RequestAck       */
    cmpArgNULL,                       /* CMP_OP_RP_Handshake        */
    cmpArgNULL,                       /* CMP_OP_RP_SyncPKBegin      */
    cmpArgNULL,                       /* CMP_OP_RP_SyncPK           */
    cmpArgNULL,                       /* CMP_OP_RP_SyncPKEnd        */
    cmpArgNULL,                       /* CMP_OP_RP_FailbackEnd      */
    cmpArgNULL,                       /* CMP_OP_RP_SyncTableNumber  */
    cmpArgNULL,                       /* CMP_OP_RP_SyncStart        */
    cmpArgNULL,                       /* CMP_OP_RP_SyncEnd          */
    cmpArgNULL,                       /* CMP_OP_RP_LobTrim          */
    cmpArgNULL,                       /* CMP_OP_RP_MetaReplCheck    */
    cmpArgNULL,                       /* CMP_OP_RP_MetaDictTableCount*/
    cmpArgNULL,                       /* CMP_OP_RP_AckonDML         */
    cmpArgNULL,                       /* CMP_OP_RP_AckEager         */
    cmpArgNULL,                       /* CMP_OP_RP_DDLSyncInfo      */
    cmpArgNULL,                       /* CMP_OP_RP_DDLSyncMsg       */
    cmpArgNULL,                       /* CMP_OP_RP_DDLSyncMsgAck    */
    cmpArgNULL,                       /* CMP_OP_RP_DDLSyncCancel    */
    cmpArgNULL,                       /* CMP_OP_RP_DDLReplicateHandshake      */ 
    cmpArgNULL,                       /* CMP_OP_RP_DDLReplicateQueryStatement */
    cmpArgNULL,                       /* CMP_OP_RP_DDLReplicateExecute        */
    cmpArgNULL,                       /* CMP_OP_RP_DDLReplicateAck  */
    cmpArgNULL,                       /* CMP_OP_RP_MetaPartitionCount */
    cmpArgNULL,                       /* CMP_OP_RP_MetaInitialize */
    cmpArgNULL,                       /* CMP_OP_RP_TemporarySyncInfo */
    cmpArgNULL,                       /* CMP_OP_RP_TemporarySyncItem */
    cmpArgNULL,                       /* CMP_OP_RP_TemporarySyncHandshakeAck */
    cmpArgNULL,                       /* CMP_OP_RP_MetaReplTblCondition */
    cmpArgNULL,                       /* CMP_OP_RP_MetaReplTblConditionResult */
    cmpArgNULL,                       /* CMP_OP_RP_Truncate */
    cmpArgNULL,                       /* CMP_OP_RP_TruncateAck */
    cmpArgNULL,                       /* CMP_OP_RP_XA_START_REQ */
    cmpArgNULL,                       /* CMP_OP_RP_XA_PREPARE_REQ */
    cmpArgNULL,                       /* CMP_OP_RP_XA_PREPARE */
    cmpArgNULL,                       /* CMP_OP_RP_XA_COMMIT */
    cmpArgNULL                        /* CMP_OP_RP_XA_END */
};
