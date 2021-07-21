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

cmpOpMap gCmpOpDBMap[] =
{
    {"CMP_OP_DB_Message"                    },
    {"CMP_OP_DB_ErrorResult"                },
    {"CMP_OP_DB_ErrorInfo"                  },
    {"CMP_OP_DB_ErrorInfoResult"            },
    {"CMP_OP_DB_Connect"                    },
    {"CMP_OP_DB_ConnectResult"              },
    {"CMP_OP_DB_Disconnect"                 },
    {"CMP_OP_DB_DisconnectResult"           },
    {"CMP_OP_DB_PropertyGet"                },
    {"CMP_OP_DB_PropertyGetResult"          },
    {"CMP_OP_DB_PropertySet"                },
    {"CMP_OP_DB_PropertySetResult"          },
    {"CMP_OP_DB_Prepare"                    },
    {"CMP_OP_DB_PrepareResult"              },
    {"CMP_OP_DB_PlanGet"                    },
    {"CMP_OP_DB_PlanGetResult"              },
    {"CMP_OP_DB_ColumnInfoGet"              },
    {"CMP_OP_DB_ColumnInfoGetResult"        },
    {"CMP_OP_DB_ColumnInfoGetListResult"    },
    {"CMP_OP_DB_ColumnInfoSet"              },
    {"CMP_OP_DB_ColumnInfoSetResult"        },
    {"CMP_OP_DB_ParamInfoGet"               },
    {"CMP_OP_DB_ParamInfoGetResult"         },
    {"CMP_OP_DB_ParamInfoSet"               },
    {"CMP_OP_DB_ParamInfoSetResult"         },
    {"CMP_OP_DB_ParamInfoSetList"           },
    {"CMP_OP_DB_ParamInfoSetListResult"     },
    {"CMP_OP_DB_ParamDataIn"                },
    {"CMP_OP_DB_ParamDataOut"               },
    {"CMP_OP_DB_ParamDataOutList"           },
    {"CMP_OP_DB_ParamDataInList"            },
    {"CMP_OP_DB_ParamDataInListResult"      },
    {"CMP_OP_DB_Execute"                    },
    {"CMP_OP_DB_ExecuteResult"              },
    {"CMP_OP_DB_FetchMove"                  },
    {"CMP_OP_DB_FetchMoveResult"            },
    {"CMP_OP_DB_Fetch"                      },
    {"CMP_OP_DB_FetchBeginResult"           },
    {"CMP_OP_DB_FetchResult"                },
    {"CMP_OP_DB_FetchListResult"            },
    {"CMP_OP_DB_FetchEndResult"             },
    {"CMP_OP_DB_Free"                       },
    {"CMP_OP_DB_FreeResult"                 },
    {"CMP_OP_DB_Cancel"                     },
    {"CMP_OP_DB_CancelResult"               },
    {"CMP_OP_DB_Transaction"                },
    {"CMP_OP_DB_TransactionResult"          },
    {"CMP_OP_DB_LobGetSize"                 },
    {"CMP_OP_DB_LobGetSizeResult"           },
    {"CMP_OP_DB_LobGet"                     },
    {"CMP_OP_DB_LobGetResult"               },
    {"CMP_OP_DB_LobPutBegin"                },
    {"CMP_OP_DB_LobPutBeginResult"          },
    {"CMP_OP_DB_LobPut"                     },
    {"CMP_OP_DB_LobPutEnd"                  },
    {"CMP_OP_DB_LobPutEndResult"            },
    {"CMP_OP_DB_LobFree"                    },
    {"CMP_OP_DB_LobFreeResult"              },
    {"CMP_OP_DB_LobFreeAll"                 },
    {"CMP_OP_DB_LobFreeAllResult"           },
    {"CMP_OP_DB_XaOperation"                },
    {"CMP_OP_DB_XaXid"                      },
    {"CMP_OP_DB_XaResult"                   },
    {"CMP_OP_DB_XaTransaction"              },
    {"CMP_OP_DB_LobGetBytePosCharLen"       },
    {"CMP_OP_DB_LobGetBytePosCharLenResult" },
    {"CMP_OP_DB_LobGetCharPosCharLen"       },
    {"CMP_OP_DB_LobBytePos"                 },
    {"CMP_OP_DB_LobBytePosResult"           },
    {"CMP_OP_DB_LobCharLength"              },
    {"CMP_OP_DB_LobCharLengthResult"        },
    {"CMP_OP_DB_ParamDataInResult"          }, // bug-28259 for ipc
    {"CMP_OP_DB_Handshake"                  }, // proj_2160 cm_type
    {"CMP_OP_DB_HandshakeResult"            },
    /* PROJ-2177 User Interface - Cancel */
    {"CMP_OP_DB_PrepareByCID"               },
    {"CMP_OP_DB_CancelByCID"                },
    /* PROJ-2047 Strengthening LOB - Added Interfaces */
    {"CMP_OP_DB_LobTrim"                    },
    {"CMP_OP_DB_LobTrimResult"              },
    /* BUG-38496  Notify users when their password expiry date is approaching */
    {"CMP_OP_DB_ConnectEx"                  },
    {"CMP_OP_DB_ConnectExResult"            },
    /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
    {"CMP_OP_DB_FetchV2"                    },
    /* BUG-41793 Keep a compatibility among tags */
    {"CMP_OP_DB_PropertySetV2"              },
    /* PROJ-2616 */
    {"CMP_OP_DB_IPCDALastOpEnded"           },
    /* BUG-44572 */
    {"CMP_OP_DB_ParamDataInListV2"          },
    {"CMP_OP_DB_ParamDataInListV2Result"    },
    {"CMP_OP_DB_ExecuteV2"                  },
    {"CMP_OP_DB_ExecuteV2Result"            },
    /* PROJ-2598 altibase sharding */
    {"CMP_OP_DB_ShardAnalyze"               },
    {"CMP_OP_DB_ShardAnalyzeResult"         },
    {"CMP_OP_DB_ShardNodeUpdateList"        },
    {"CMP_OP_DB_ShardNodeUpdateListResult"  },
    {"CMP_OP_DB_ShardNodeGetList"           },
    {"CMP_OP_DB_ShardNodeGetListResult"     },
    {"CMP_OP_DB_ShardHandshake"             },
    {"CMP_OP_DB_ShardHandshakeResult"       },
    {"CMP_OP_DB_ShardTransaction"           },
    {"CMP_OP_DB_ShardTransactionResult"     },
    {"CMP_OP_DB_ShardPrepare"               },
    {"CMP_OP_DB_ShardPrepareResult"         },
    {"CMP_OP_DB_ShardEndPendingTx"          },
    {"CMP_OP_DB_ShardEndPendingTxResult"    },
    /* BUG-46785 Shard statement partial rollback */
    {"CMP_OP_DB_SetSavepoint"               },
    {"CMP_OP_DB_SetSavepointResult"         },
    {"CMP_OP_DB_RollbackToSavepoint"        },
    {"CMP_OP_DB_RollbackToSavepointResult"  },
    {"CMP_OP_DB_ShardStmtPartialRollback"   },
    {"CMP_OP_DB_ShardStmtPartialRollbackResult"},
    {"CMP_OP_DB_ShardNodeReport"            },
    {"CMP_OP_DB_ShardNodeReportResult"      },
    /* PROJ-2733-Protocol 프로토콜 이름 V3로 통일 */
    {"CMP_OP_DB_ErrorV3Result"              },
    {"CMP_OP_DB_ConnectV3"                  },
    {"CMP_OP_DB_ConnectV3Result"            },
    {"CMP_OP_DB_PropertySetV3"              },
    {"CMP_OP_DB_PropertySetV3Result"        },
    {"CMP_OP_DB_ParamDataInListV3"          },
    {"CMP_OP_DB_ParamDataInListV3Result"    },
    {"CMP_OP_DB_ExecuteV3"                  },
    {"CMP_OP_DB_ExecuteV3Result"            },
    {"CMP_OP_DB_ShardTransactionV3"         },
    {"CMP_OP_DB_ShardTransactionV3Result"   },
    {"CMP_OP_DB_ShardPrepareV3"             },
    {"CMP_OP_DB_ShardPrepareV3Result"       },
    {"CMP_OP_DB_ShardEndPendingTxV3"        },
    {"CMP_OP_DB_ShardEndPendingTxV3Result"  },
    /* PROJ-2728 Sharding LOB */
    {"CMP_OP_DB_LobGetSizeV3"               },
    {"CMP_OP_DB_LobGetSizeV3Result"         },
    {"CMP_OP_DB_FetchV3"                    },
    {"CMP_OP_DB_ShardRebuildNotiV3"         },
    /* BUG-48775 */
    {"CMP_OP_DB_PrepareV3"                  },
    {"CMP_OP_DB_PrepareByCIDV3"             },
    {"CMP_OP_DB_PrepareV3Result"            },
    {"CMP_OP_DB_MAX"                        }
};

// PROJ-1697 Performance view
ULong gDBProtocolStat[CMP_OP_DB_MAX];

IDE_RC cmpArgInitializeDBMessage(cmpProtocol *aProtocol)
{
    cmpArgDBMessageA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Message);

    IDE_TEST(cmtVariableInitialize(&sArg->mMessage) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBMessage(cmpProtocol *aProtocol)
{
    cmpArgDBMessageA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Message);

    IDE_TEST(cmtVariableFinalize(&sArg->mMessage) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBErrorResult(cmpProtocol *aProtocol)
{
    cmpArgDBErrorResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorResult);

    IDE_TEST(cmtVariableInitialize(&sArg->mErrorMessage) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBErrorResult(cmpProtocol *aProtocol)
{
    cmpArgDBErrorResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorResult);

    IDE_TEST(cmtVariableFinalize(&sArg->mErrorMessage) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBErrorInfoResult(cmpProtocol *aProtocol)
{
    cmpArgDBErrorInfoResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorInfoResult);

    IDE_TEST(cmtVariableInitialize(&sArg->mErrorMessage) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBErrorInfoResult(cmpProtocol *aProtocol)
{
    cmpArgDBErrorInfoResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorInfoResult);

    IDE_TEST(cmtVariableFinalize(&sArg->mErrorMessage) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBConnect(cmpProtocol *aProtocol)
{
    cmpArgDBConnectA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Connect);

    IDE_TEST(cmtVariableInitialize(&sArg->mDatabaseName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mUserName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mPassword) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBConnect(cmpProtocol *aProtocol)
{
    cmpArgDBConnectA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Connect);

    IDE_TEST(cmtVariableFinalize(&sArg->mDatabaseName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mUserName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mPassword) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBPropertyGetResult(cmpProtocol *aProtocol)
{
    cmpArgDBPropertyGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertyGetResult);

    IDE_TEST(cmtAnyInitialize(&sArg->mValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBPropertyGetResult(cmpProtocol *aProtocol)
{
    cmpArgDBPropertyGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertyGetResult);

    IDE_TEST(cmtAnyFinalize(&sArg->mValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBPropertySet(cmpProtocol *aProtocol)
{
    cmpArgDBPropertySetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertySet);

    IDE_TEST(cmtAnyInitialize(&sArg->mValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBPropertySet(cmpProtocol *aProtocol)
{
    cmpArgDBPropertySetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertySet);

    IDE_TEST(cmtAnyFinalize(&sArg->mValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBPrepare(cmpProtocol *aProtocol)
{
    cmpArgDBPrepareA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Prepare);

    IDE_TEST(cmtVariableInitialize(&sArg->mStatementString) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBPrepare(cmpProtocol *aProtocol)
{
    cmpArgDBPrepareA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Prepare);

    IDE_TEST(cmtVariableFinalize(&sArg->mStatementString) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// bug-25109 support ColAttribute baseTableName
IDE_RC cmpArgInitializeDBPrepareResult(cmpProtocol *aProtocol)
{
    cmpArgDBPrepareResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PrepareResult);

    IDE_TEST(cmtVariableInitialize(&sArg->mBaseTableOwnerName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mBaseTableName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mTableName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBPrepareResult(cmpProtocol *aProtocol)
{
    cmpArgDBPrepareResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PrepareResult);


    IDE_TEST(cmtVariableFinalize(&sArg->mBaseTableOwnerName) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mBaseTableName) != IDE_SUCCESS);
    // mTableName not used and must not be finalized
    //IDE_TEST(cmtVariableFinalize(&sArg->mTableName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBPlanGetResult(cmpProtocol *aProtocol)
{
    cmpArgDBPlanGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PlanGetResult);

    IDE_TEST(cmtVariableInitialize(&sArg->mPlan) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBPlanGetResult(cmpProtocol *aProtocol)
{
    cmpArgDBPlanGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PlanGetResult);

    IDE_TEST(cmtVariableFinalize(&sArg->mPlan) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBColumnInfoGetResult(cmpProtocol *aProtocol)
{
    cmpArgDBColumnInfoGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGetResult);

    IDE_TEST(cmtVariableInitialize(&sArg->mDisplayName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBColumnInfoGetResult(cmpProtocol *aProtocol)
{
    cmpArgDBColumnInfoGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGetResult);

    IDE_TEST(cmtVariableFinalize(&sArg->mDisplayName) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBColumnInfoGetListResult(cmpProtocol *aProtocol)
{
    cmpArgDBColumnInfoGetListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGetListResult);

    IDE_TEST(cmtCollectionInitialize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBColumnInfoGetListResult(cmpProtocol *aProtocol)
{
    cmpArgDBColumnInfoGetListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGetListResult);

    IDE_TEST(cmtCollectionFinalize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBParamInfoSetList(cmpProtocol *aProtocol)
{
    cmpArgDBParamInfoSetListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoSetList);

    IDE_TEST(cmtCollectionInitialize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBParamInfoSetList(cmpProtocol *aProtocol)
{
    cmpArgDBParamInfoSetListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoSetList);

    IDE_TEST(cmtCollectionInitialize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBParamDataIn(cmpProtocol *aProtocol)
{
    cmpArgDBParamDataInA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataIn);

    IDE_TEST(cmtAnyInitialize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBParamDataIn(cmpProtocol *aProtocol)
{
    cmpArgDBParamDataInA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataIn);

    IDE_TEST(cmtAnyFinalize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBParamDataOut(cmpProtocol *aProtocol)
{
    cmpArgDBParamDataOutA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataOut);

    IDE_TEST(cmtAnyInitialize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBParamDataOut(cmpProtocol *aProtocol)
{
    cmpArgDBParamDataOutA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataOut);

    IDE_TEST(cmtAnyFinalize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBParamDataOutList(cmpProtocol *aProtocol)
{
    cmpArgDBParamDataOutListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataOutList);

    IDE_TEST(cmtCollectionInitialize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBParamDataOutList(cmpProtocol *aProtocol)
{
    cmpArgDBParamDataOutListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataOutList);

    IDE_TEST(cmtCollectionFinalize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBParamDataInList(cmpProtocol *aProtocol)
{
    cmpArgDBParamDataInListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataInList);

    IDE_TEST(cmtCollectionInitialize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBParamDataInList(cmpProtocol *aProtocol)
{
    cmpArgDBParamDataInListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataInList);

    IDE_TEST(cmtCollectionFinalize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBFetchResult(cmpProtocol *aProtocol)
{
    cmpArgDBFetchResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchResult);

    IDE_TEST(cmtAnyInitialize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBFetchResult(cmpProtocol *aProtocol)
{
    cmpArgDBFetchResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchResult);

    IDE_TEST(cmtAnyFinalize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBFetchListResult(cmpProtocol *aProtocol)
{
    cmpArgDBFetchListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchListResult);

    IDE_TEST(cmtCollectionInitialize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBFetchListResult(cmpProtocol *aProtocol)
{
    cmpArgDBFetchListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchListResult);

    IDE_TEST(cmtCollectionFinalize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBLobGetResult(cmpProtocol *aProtocol)
{
    cmpArgDBLobGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetResult);

    IDE_TEST(cmtVariableInitialize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBLobGetResult(cmpProtocol *aProtocol)
{
    cmpArgDBLobGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetResult);

    IDE_TEST(cmtVariableFinalize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBLobGetBytePosCharLenResult(cmpProtocol *aProtocol)
{
    cmpArgDBLobGetBytePosCharLenResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetBytePosCharLenResult);

    IDE_TEST(cmtVariableInitialize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBLobGetBytePosCharLenResult(cmpProtocol *aProtocol)
{
    cmpArgDBLobGetBytePosCharLenResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetBytePosCharLenResult);

    IDE_TEST(cmtVariableFinalize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBLobPut(cmpProtocol *aProtocol)
{
    cmpArgDBLobPutA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPut);

    IDE_TEST(cmtVariableInitialize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBLobPut(cmpProtocol *aProtocol)
{
    cmpArgDBLobPutA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPut);

    IDE_TEST(cmtVariableFinalize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* To fix BUG-20513 */
IDE_RC cmpArgInitializeDBLobFreeAll(cmpProtocol *aProtocol)
{
    cmpArgDBLobFreeAllA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobFreeAll);

    IDE_TEST(cmtCollectionInitialize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBLobFreeAll(cmpProtocol *aProtocol)
{
    cmpArgDBLobFreeAllA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobFreeAll);

    IDE_TEST(cmtCollectionFinalize(&sArg->mListData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* PROJ-1573 XA */
IDE_RC cmpArgInitializeDBXaXid(cmpProtocol *aProtocol)
{
    cmpArgDBXaXidA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaXid);

    IDE_TEST(cmtVariableInitialize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBXaXid(cmpProtocol *aProtocol)
{
    cmpArgDBXaXidA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaXid);

    IDE_TEST(cmtVariableFinalize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeDBXaTransaction(cmpProtocol *aProtocol)
{
    cmpArgDBXaTransactionA5 *sArg
        = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaTransaction);
    IDE_TEST(cmtVariableInitialize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeDBXaTransaction(cmpProtocol *aProtocol)
{
    cmpArgDBXaTransactionA5 *sArg
        = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaTransaction);

    IDE_TEST(cmtVariableFinalize(&sArg->mData) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

cmpArgFunction gCmpArgInitializeFunctionDB[CMP_OP_DB_MAX_A5] =
{
    cmpArgInitializeDBMessage,                     /* CMP_OP_DB_Message                     */
    cmpArgInitializeDBErrorResult,                 /* CMP_OP_DB_ErrorResult                 */
    cmpArgNULL,                                    /* CMP_OP_DB_ErrorInfo                   */
    cmpArgInitializeDBErrorInfoResult,             /* CMP_OP_DB_ErrorInfoResult             */
    cmpArgInitializeDBConnect,                     /* CMP_OP_DB_Connect                     */
    cmpArgNULL,                                    /* CMP_OP_DB_ConnectResult               */
    cmpArgNULL,                                    /* CMP_OP_DB_Disconnect                  */
    cmpArgNULL,                                    /* CMP_OP_DB_DisconnectResult            */
    cmpArgNULL,                                    /* CMP_OP_DB_PropertyGet                 */
    cmpArgInitializeDBPropertyGetResult,           /* CMP_OP_DB_PropertyGetResult           */
    cmpArgInitializeDBPropertySet,                 /* CMP_OP_DB_PropertySet                 */
    cmpArgNULL,                                    /* CMP_OP_DB_PropertySetResult           */
    cmpArgInitializeDBPrepare,                     /* CMP_OP_DB_Prepare                     */
    cmpArgInitializeDBPrepareResult,               /* CMP_OP_DB_PrepareResult               */
    cmpArgNULL,                                    /* CMP_OP_DB_PlanGet                     */
    cmpArgInitializeDBPlanGetResult,               /* CMP_OP_DB_PlanGetResult               */
    cmpArgNULL,                                    /* CMP_OP_DB_ColumnInfoGet               */
    cmpArgInitializeDBColumnInfoGetResult,         /* CMP_OP_DB_ColumnInfoGetResult         */
    cmpArgInitializeDBColumnInfoGetListResult,     /* CMP_OP_DB_ColumnInfoGetListResult     */
    cmpArgNULL,                                    /* CMP_OP_DB_ColumnInfoSet               */
    cmpArgNULL,                                    /* CMP_OP_DB_ColumnInfoSetResult         */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoGet                */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoGetResult          */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoSet                */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoSetResult          */
    cmpArgInitializeDBParamInfoSetList,            /* CMP_OP_DB_ParamInfoSetList            */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoSetListResult      */
    cmpArgInitializeDBParamDataIn,                 /* CMP_OP_DB_ParamDataIn                 */
    cmpArgInitializeDBParamDataOut,                /* CMP_OP_DB_ParamDataOut                */
    cmpArgInitializeDBParamDataOutList,            /* CMP_OP_DB_ParamDataOutList            */
    cmpArgInitializeDBParamDataInList,             /* CMP_OP_DB_ParamDataInList             */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamDataInListResult       */
    cmpArgNULL,                                    /* CMP_OP_DB_Execute                     */
    cmpArgNULL,                                    /* CMP_OP_DB_ExecuteResult               */
    cmpArgNULL,                                    /* CMP_OP_DB_FetchMove                   */
    cmpArgNULL,                                    /* CMP_OP_DB_FetchMoveResult             */
    cmpArgNULL,                                    /* CMP_OP_DB_Fetch                       */
    cmpArgNULL,                                    /* CMP_OP_DB_FetchBeginResult            */
    cmpArgInitializeDBFetchResult,                 /* CMP_OP_DB_FetchResult                 */
    cmpArgInitializeDBFetchListResult,             /* CMP_OP_DB_FetchListResult             */
    cmpArgNULL,                                    /* CMP_OP_DB_FetchEndResult              */
    cmpArgNULL,                                    /* CMP_OP_DB_Free                        */
    cmpArgNULL,                                    /* CMP_OP_DB_FreeResult                  */
    cmpArgNULL,                                    /* CMP_OP_DB_Cancel                      */
    cmpArgNULL,                                    /* CMP_OP_DB_CancelResult                */
    cmpArgNULL,                                    /* CMP_OP_DB_Transaction                 */
    cmpArgNULL,                                    /* CMP_OP_DB_TransactionResult           */
    cmpArgNULL,                                    /* CMP_OP_DB_LobGetSize                  */
    cmpArgNULL,                                    /* CMP_OP_DB_LobGetSizeResult            */
    cmpArgNULL,                                    /* CMP_OP_DB_LobGet                      */
    cmpArgInitializeDBLobGetResult,                /* CMP_OP_DB_LobGetResult                */
    cmpArgNULL,                                    /* CMP_OP_DB_LobPutBegin                 */
    cmpArgNULL,                                    /* CMP_OP_DB_LobPutBeginResult           */
    cmpArgInitializeDBLobPut,                      /* CMP_OP_DB_LobPut                      */
    cmpArgNULL,                                    /* CMP_OP_DB_LobPutEnd                   */
    cmpArgNULL,                                    /* CMP_OP_DB_LobPutEndResult             */
    cmpArgNULL,                                    /* CMP_OP_DB_LobFree                     */
    cmpArgNULL,                                    /* CMP_OP_DB_LobFreeResult               */
    cmpArgInitializeDBLobFreeAll,                  /* CMP_OP_DB_LobFreeAll                  */
    cmpArgNULL,                                    /* CMP_OP_DB_LobFreeAllResult            */
    cmpArgNULL,                                    /* CMP_OP_DB_XaOperation                 */
    cmpArgInitializeDBXaXid,                       /* CMP_OP_DB_XaXid                       */
    cmpArgNULL,                                    /* CMP_OP_DB_XaResult                    */
    cmpArgInitializeDBXaTransaction,               /* CMP_OP_DB_XaTransaction               */
    cmpArgNULL,                                    /* CMP_OP_DB_LobGetBytePosCharLen        */
    cmpArgInitializeDBLobGetBytePosCharLenResult,  /* CMP_OP_DB_LobGetBytePosCharLenResult  */
    cmpArgNULL,                                    /* CMP_OP_DB_LobGetCharPosCharLen        */
    cmpArgNULL,                                    /* CMP_OP_DB_LobBytePos                  */
    cmpArgNULL,                                    /* CMP_OP_DB_LobBytePosResult            */
    cmpArgNULL,                                    /* CMP_OP_DB_LobCharLength               */
    cmpArgNULL,                                    /* CMP_OP_DB_LobCharLengthResult         */
    cmpArgNULL                                     /* CMP_OP_DB_ParamDataInResult           */
    /* BUG-43080 Warning - Do not add protocols for A7 here anymore */
};

cmpArgFunction gCmpArgFinalizeFunctionDB[CMP_OP_DB_MAX_A5] =
{
    cmpArgFinalizeDBMessage,                       /* CMP_OP_DB_Message                     */
    cmpArgFinalizeDBErrorResult,                   /* CMP_OP_DB_ErrorResult                 */
    cmpArgNULL,                                    /* CMP_OP_DB_ErrorInfo                   */
    cmpArgFinalizeDBErrorInfoResult,               /* CMP_OP_DB_ErrorInfoResult             */
    cmpArgFinalizeDBConnect,                       /* CMP_OP_DB_Connect                     */
    cmpArgNULL,                                    /* CMP_OP_DB_ConnectResult               */
    cmpArgNULL,                                    /* CMP_OP_DB_Disconnect                  */
    cmpArgNULL,                                    /* CMP_OP_DB_DisconnectResult            */
    cmpArgNULL,                                    /* CMP_OP_DB_PropertyGet                 */
    cmpArgFinalizeDBPropertyGetResult,             /* CMP_OP_DB_PropertyGetResult           */
    cmpArgFinalizeDBPropertySet,                   /* CMP_OP_DB_PropertySet                 */
    cmpArgNULL,                                    /* CMP_OP_DB_PropertySetResult           */
    cmpArgFinalizeDBPrepare,                       /* CMP_OP_DB_Prepare                     */
    cmpArgFinalizeDBPrepareResult,                 /* CMP_OP_DB_PrepareResult               */
    cmpArgNULL,                                    /* CMP_OP_DB_PlanGet                     */
    cmpArgFinalizeDBPlanGetResult,                 /* CMP_OP_DB_PlanGetResult               */
    cmpArgNULL,                                    /* CMP_OP_DB_ColumnInfoGet               */
    cmpArgFinalizeDBColumnInfoGetResult,           /* CMP_OP_DB_ColumnInfoGetResult         */
    cmpArgFinalizeDBColumnInfoGetListResult,       /* CMP_OP_DB_ColumnInfoGetListResult     */
    cmpArgNULL,                                    /* CMP_OP_DB_ColumnInfoSet               */
    cmpArgNULL,                                    /* CMP_OP_DB_ColumnInfoSetResult         */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoGet                */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoGetResult          */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoSet                */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoSetResult          */
    cmpArgFinalizeDBParamInfoSetList,              /* CMP_OP_DB_ParamInfoSetList            */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamInfoSetListResult      */
    cmpArgFinalizeDBParamDataIn,                   /* CMP_OP_DB_ParamDataIn                 */
    cmpArgFinalizeDBParamDataOut,                  /* CMP_OP_DB_ParamDataOut                */
    cmpArgFinalizeDBParamDataOutList,              /* CMP_OP_DB_ParamDataOutList            */
    cmpArgFinalizeDBParamDataInList,               /* CMP_OP_DB_ParamDataInList             */
    cmpArgNULL,                                    /* CMP_OP_DB_ParamDataInListResult       */
    cmpArgNULL,                                    /* CMP_OP_DB_Execute                     */
    cmpArgNULL,                                    /* CMP_OP_DB_ExecuteResult               */
    cmpArgNULL,                                    /* CMP_OP_DB_FetchMove                   */
    cmpArgNULL,                                    /* CMP_OP_DB_FetchMoveResult             */
    cmpArgNULL,                                    /* CMP_OP_DB_Fetch                       */
    cmpArgNULL,                                    /* CMP_OP_DB_FetchBeginResult            */
    cmpArgFinalizeDBFetchResult,                   /* CMP_OP_DB_FetchResult                 */
    cmpArgFinalizeDBFetchListResult,               /* CMP_OP_DB_FetchListResult             */
    cmpArgNULL,                                    /* CMP_OP_DB_FetchEndResult              */
    cmpArgNULL,                                    /* CMP_OP_DB_Free                        */
    cmpArgNULL,                                    /* CMP_OP_DB_FreeResult                  */
    cmpArgNULL,                                    /* CMP_OP_DB_Cancel                      */
    cmpArgNULL,                                    /* CMP_OP_DB_CancelResult                */
    cmpArgNULL,                                    /* CMP_OP_DB_Transaction                 */
    cmpArgNULL,                                    /* CMP_OP_DB_TransactionResult           */
    cmpArgNULL,                                    /* CMP_OP_DB_LobGetSize                  */
    cmpArgNULL,                                    /* CMP_OP_DB_LobGetSizeResult            */
    cmpArgNULL,                                    /* CMP_OP_DB_LobGet                      */
    cmpArgFinalizeDBLobGetResult,                  /* CMP_OP_DB_LobGetResult                */
    cmpArgNULL,                                    /* CMP_OP_DB_LobPutBegin                 */
    cmpArgNULL,                                    /* CMP_OP_DB_LobPutBeginResult           */
    cmpArgFinalizeDBLobPut,                        /* CMP_OP_DB_LobPut                      */
    cmpArgNULL,                                    /* CMP_OP_DB_LobPutEnd                   */
    cmpArgNULL,                                    /* CMP_OP_DB_LobPutEndResult             */
    cmpArgNULL,                                    /* CMP_OP_DB_LobFree                     */
    cmpArgNULL,                                    /* CMP_OP_DB_LobFreeResult               */
    cmpArgFinalizeDBLobFreeAll,                    /* CMP_OP_DB_LobFreeAll                  */
    cmpArgNULL,                                    /* CMP_OP_DB_LobFreeAllResult            */
    cmpArgNULL,                                    /* CMP_OP_DB_XaOperation                 */
    cmpArgFinalizeDBXaXid,                         /* CMP_OP_DB_XaXid                       */
    cmpArgNULL,                                    /* CMP_OP_DB_XaResult                    */
    cmpArgFinalizeDBXaTransaction,                 /* CMP_OP_DB_XaTransaction               */    
    cmpArgNULL,                                    /* CMP_OP_DB_LobGetBytePosCharLen        */
    cmpArgFinalizeDBLobGetBytePosCharLenResult,    /* CMP_OP_DB_LobGetBytePosCharLenResult  */
    cmpArgNULL,                                    /* CMP_OP_DB_LobGetCharPosCharLen        */
    cmpArgNULL,                                    /* CMP_OP_DB_LobBytePos                  */
    cmpArgNULL,                                    /* CMP_OP_DB_LobBytePosResult            */
    cmpArgNULL,                                    /* CMP_OP_DB_LobCharLength               */
    cmpArgNULL,                                    /* CMP_OP_DB_LobCharLengthResult         */
    cmpArgNULL                                     /* CMP_OP_DB_ParamDataInResult           */
    /* BUG-43080 Warning - Do not add protocols for A7 here anymore */
};
