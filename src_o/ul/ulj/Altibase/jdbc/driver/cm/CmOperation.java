/*
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

package Altibase.jdbc.driver.cm;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.CoderResult;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import Altibase.jdbc.driver.AltibaseVersion;
import Altibase.jdbc.driver.AltibaseXid;
import Altibase.jdbc.driver.LobConst;
import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.ColumnFactory;
import Altibase.jdbc.driver.datatype.ColumnInfo;
import Altibase.jdbc.driver.datatype.ListBufferHandle;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.ex.ShardFailoverIsNotAvailableException;
import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.sharding.core.ShardVersion;
import Altibase.jdbc.driver.sharding.core.GlobalTransactionLevel;
import Altibase.jdbc.driver.util.AltibaseProperties;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class CmOperation extends CmOperationDef
{
    private static final byte HANDSHAKE_MODULE_ID = 1;
    private static final byte HANDSHAKE_FLAG      = 0; // This flag value has been reserved for the future. This value is arbitrary now

    private static Logger mLogger = null;

    static
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_DEFAULT);
        }
    }

    static void writeHandshake(CmChannel aChannel) throws SQLException
    {
        aChannel.checkWritable(6);
        aChannel.writeOp(DB_OP_HANDSHAKE);
        aChannel.writeByte(HANDSHAKE_MODULE_ID);
        aChannel.writeByte(AltibaseVersion.CM_MAJOR_VERSION);
        aChannel.writeByte(AltibaseVersion.CM_MINOR_VERSION);
        aChannel.writeByte(AltibaseVersion.CM_PATCH_VERSION);
        aChannel.writeByte(DB_OP_COUNT); // TASK-7220 고성능 분산공유트랜잭션 정합성 
    }

    static void writeShardHandshake(CmChannel aChannel) throws SQLException
    {
        aChannel.checkWritable(5);
        aChannel.writeOp(DB_OP_SHARD_HANDSHAKE);
        aChannel.writeByte(ShardVersion.SHARD_MAJOR_VERSION);
        aChannel.writeByte(ShardVersion.SHARD_MINOR_VERSION);
        aChannel.writeByte(ShardVersion.SHARD_PATCH_VERSION);
        aChannel.writeByte((byte)0);    // flag
    }

    static void readHandshake(CmChannel aChannel, CmHandshakeResult aResult) throws SQLException
    {
        byte sOp = aChannel.readOp();
        
        if (sOp == DB_OP_HANDSHAKE_RESULT)
        {
            // 서버로부터 오는 base version, module id, module version 모두 특별히 사용할 데가 없다.
            aResult.setResult(aChannel.readByte(), aChannel.readByte(), aChannel.readByte(), aChannel.readByte(), aChannel.readByte());
        }
        else if (sOp == DB_OP_ERROR_RESULT || sOp == DB_OP_ERROR_V3_RESULT)
        {
            aChannel.readByte();     // skip ErrOpID
            aChannel.readInt();      // skip ErrIndex
            int sErrCode = aChannel.readInt();
            String sErrMsg = aChannel.readStringForErrorResult();
            if (sOp == DB_OP_ERROR_V3_RESULT)
            {
                aChannel.readLong(); // skip SCN
            }
            aResult.setError(sErrCode, sErrMsg);
        }
        else
        {
            Error.throwInternalError(ErrorDef.INVALID_OPERATION_PROTOCOL, String.valueOf(DB_OP_HANDSHAKE));
        }
    }

    /**
     * 프로토콜을 통해 DB ShardHandshake(93) 결과를 저장한다.
     * @param aChannel CmChannel객체
     * @param aShardHandshakeResult shard handshake 컨텍스트 객체
     * @throws SQLException shard handshake프로토콜을 전송받는 중 에러가 발생한 경우
     */
    static void readShardHandshake(CmChannel aChannel,
                                   CmShardHandshakeResult aShardHandshakeResult) throws SQLException
    {
        byte sOp = aChannel.readOp();
        if (sOp == DB_OP_SHARD_HANDSHAKE_RESULT)
        {
            byte sMajorVersion = aChannel.readByte();
            byte sMinorVersion = aChannel.readByte();
            byte sPatchVersion = aChannel.readByte();
            byte sFlag         = aChannel.readByte();
            /*
                PROJ-2690 서버로부터 shard handshake의 결과로 major version, minor version, patch version, flag가 넘어오지만
                특별히 사용하는 곳은 없다.
             */
            aShardHandshakeResult.setResult(sMajorVersion, sMinorVersion, sPatchVersion, sFlag);
        }
        else if (sOp == DB_OP_ERROR_V3_RESULT)
        {
            aChannel.readByte();   // skip ErrOpID
            aChannel.readInt();    // skip ErrIndex
            int sErrCode = aChannel.readInt();
            String sErrMsg = aChannel.readStringForErrorResult();
            aChannel.readLong();   // skip SCN
            aShardHandshakeResult.setError(sErrCode, sErrMsg);
        }
        else
        {
            /* Invalid protocol seq error 처리 */
            Error.throwInternalError(ErrorDef.INVALID_OPERATION_PROTOCOL, String.valueOf(DB_OP_SHARD_HANDSHAKE));
        }
    }

    public static void readProtocolResult(CmProtocolContext aContext) throws SQLException
    {
        CmChannel sChannel = aContext.channel();
        while (true)
        {
            byte sResultOp = sChannel.readOp();
            if (sResultOp == CmChannel.NO_OP)
            {
                break;
            }

            switch (sResultOp)
            {
                case DB_OP_MESSAGE:
                    printMessage(sChannel);
                    break;
                case DB_OP_ERROR_RESULT:
                case DB_OP_ERROR_V3_RESULT:
                    readErrorV3Result(sChannel, aContext, sResultOp);
                    break;
                case DB_OP_CONNECT_V3_RESULT:
                    // BUG-38496 Notify users when their password expiry date is approaching.
                    readConnectV3Result(sChannel, (CmConnectExResult)aContext.getCmResult(sResultOp), aContext);
                    break;
                case DB_OP_DISCONNECT_RESULT:
                    // nothing to read
                    break;
                case DB_OP_GET_PROPERTY_RESULT:
                    readGetPropertyResult(sChannel, (CmGetPropertyResult)aContext.getCmResult(sResultOp));
                    break;
                case DB_OP_SET_PROPERTY_V3_RESULT:
                    // TASK-7220 고성능 분산공유트랜잭션 정합성
                    readSetPropertyV3Result(sChannel, (CmSetPropertyResult)aContext.getCmResult(sResultOp), aContext);
                    break;
                case DB_OP_PREPARE_V3_RESULT:
                    readPrepareResult(sChannel, (CmPrepareResult)aContext.getCmResult(sResultOp));
                    // BUG-48431 prepare가 성공한 후 deferred request 리스트를 clear한다.
                    CmProtocolContextDirExec aPrepContext = (CmProtocolContextDirExec)aContext;
                    if (aPrepContext.getDeferredRequests().size() > 0)
                    {
                        aPrepContext.clearDeferredRequests();
                    }
                    break;
                case DB_OP_GET_PLAN_RESULT:
                    readGetPlanResult(sChannel, (CmGetPlanResult)aContext.getCmResult(sResultOp));
                    break;
                case DB_OP_GET_COLUMN_INFO_RESULT:
                    readGetColumnInfoResult(sChannel, (CmGetColumnInfoResult)aContext.getCmResult(sResultOp), sChannel.getByteLengthPerChar());
                    break;
                case DB_OP_GET_COLUMN_INFO_LIST_RESULT:
                    readGetColumnInfoListResult(sChannel, (CmGetColumnInfoResult)aContext.getCmResult(CmGetColumnInfoResult.MY_OP), sChannel.getByteLengthPerChar());
                    break;
                case DB_OP_GET_PARAM_INFO_RESULT:
                    readGetBindParamInfoResult(sChannel, (CmGetBindParamInfoResult)aContext.getCmResult(sResultOp), sChannel.getByteLengthPerChar());
                    break;
                case DB_OP_SET_PARAM_INFO_LIST_RESULT:
                    // nothing to read
                    break;
                case DB_OP_PARAM_DATA_IN_RESULT:
                    // nothing to read
                    break;
                case DB_OP_PARAM_DATA_OUT_LIST:
                    readBindParamDataOutListResult(sChannel, (CmBindParamDataOutResult)aContext.getCmResult(sResultOp));
                    break;
                case DB_OP_PARAM_DATA_IN_LIST_V3_RESULT:
                    readBindParamDataInListV3Result(sChannel, aContext);
                    break;
                case DB_OP_EXECUTE_V3_RESULT:
                    readExecuteV3Result(sChannel, aContext);
                    break;
                case DB_OP_FETCH_BEGIN_RESULT:
                    readFetchBeginResult(sChannel, (CmGetColumnInfoResult)aContext.getCmResult(CmGetColumnInfoResult.MY_OP), (CmFetchResult)aContext.getCmResult(CmFetchResult.MY_OP));
                    break;
                case DB_OP_FETCH_RESULT:
                    readFetchResult(sChannel, (CmFetchResult)aContext.getCmResult(sResultOp));
                    break;
                case DB_OP_FETCH_END_RESULT:
                    readFetchEndResult(sChannel, (CmGetColumnInfoResult)aContext.getCmResult(CmGetColumnInfoResult.MY_OP), (CmFetchResult)aContext.getCmResult(CmFetchResult.MY_OP));
                    break;
                case DB_OP_FREE_RESULT:
                    // nothing to read
                    break;
                case DB_OP_CANCEL_RESULT:
                    // nothing to read
                    break;
                case DB_OP_TRANSACTION_RESULT:
                    // nothing to read
                    break;
                case DB_OP_SHARD_TRANSACTION_V3_RESULT:
                    readShardTransactionResult(sChannel, aContext);
                    break;
                case DB_OP_LOB_GET_SIZE_V3_RESULT:
                    readLobGetSizeResult(sChannel, (CmBlobGetResult)aContext.getCmResult(CmBlobGetResult.MY_OP));
                    break;
                case DB_OP_LOB_GET_RESULT:
                    readLobGetResult(sChannel, aContext);
                    break;
                case DB_OP_LOB_PUT_BEGIN_RESULT:
                    // nothing to read
                    break;
                case DB_OP_LOB_PUT_END_RESULT:
                    // nothing to read
                    break;
                case DB_OP_LOB_FREE_RESULT:
                    // nothing to read
                    break;
                case DB_OP_LOB_FREE_ALL_RESULT:
                    // nothing to read
                    break;
                case DB_OP_XA_XID_RESULT:
                    readXidResult(sChannel, (CmXidResult)aContext.getCmResult(sResultOp));
                    break;
                case DB_OP_XA_RESULT:
                    readXaResult(sChannel, (CmXAResult)aContext.getCmResult(sResultOp));
                    break;
                case DB_OP_LOB_GET_BYTE_POS_CHAR_LEN_RESULT:
                    readClobGetResult(sChannel, aContext);
                    break;
                case DB_OP_LOB_BYTE_POS_RESULT:
                    readLobBytePosResult(sChannel, (CmClobGetResult)aContext.getCmResult(CmClobGetResult.MY_OP));
                    break;
                case DB_OP_LOB_CHAR_LENGTH_RESULT:
                    readLobCharLengthResult(sChannel, (CmClobGetResult)aContext.getCmResult(CmClobGetResult.MY_OP));
                    break;
                case DB_OP_LOB_TRIM_RESULT:
                    break;

                // #region 안쓰는 프로토콜

                // 예약해 둔 것일 뿐, 아직 안쓰는 프로토콜
                case DB_OP_FETCH_MOVE_RESULT:

                default:
                    Error.throwInternalError(ErrorDef.INVALID_OPERATION_PROTOCOL, String.valueOf(sResultOp));
                    break;

                // #endregion
            }
        }
    }

    private static void printMessage(CmChannel aChannel) throws SQLException
    {
        // BUG-45237 서버메시지를 SQLWarning으로 생성하는 대신 CmChannel에 등록된 콜백에 위임한다.
        aChannel.readAndPrintServerMessage();
    }

    public static void readErrorV3Result(CmChannel aChannel, CmProtocolContext aContext, byte aResultOp) throws SQLException
    {
        long sSCN;
        CmErrorResult sError = new CmErrorResult();
        sError.readFrom(aChannel, aResultOp);
        if (aContext instanceof CmProtocolContextDirExec)
        {
            CmExecutionResult sExecResult = (CmExecutionResult)aContext.getCmResult(CmExecutionResult.MY_OP);

            if (sExecResult.isBatchMode())
            {
                sError.setBatchError(true);
            }

            switch (sError.getErrorOp())
            {
                case DB_OP_PREPARE_BY_CID_V3:
                    // prepare 에러는 Error Index가 설정되지 않으므로(항상 0), 현재 RowNumber 값으로 초기
                    sError.initErrorIndex(sExecResult.getUpdatedRowCountArraySize());
                case DB_OP_PARAM_DATA_IN_LIST_V3:
                case DB_OP_EXECUTE_V3:
                    sExecResult.setUpdatedRowCount(Statement.EXECUTE_FAILED);
                    break;
                default:
                    break;
            }
        }

        sSCN = sError.getSCN();
        if (sSCN > 0)
        {
            aContext.updateSCN(sSCN);
        }

        aContext.addError(sError);
    }

    static void writeConnectEx(CmChannel aChannel, String aDBName, String aUserName, String aPassword, short aMode) throws SQLException
    {
        int[] sLimit = new int[3];
        ByteBuffer sBuf = prepareToWriteStringForConnect(aChannel, new String[] { aDBName, aUserName, aPassword }, sLimit);

        aChannel.checkWritable(9 + sBuf.remaining());
        aChannel.writeOp(DB_OP_CONNECT_V3);
        for (int sSLimit : sLimit)
        {
            sBuf.limit(sSLimit);
            aChannel.writeShort((short)sBuf.remaining());
            aChannel.writeBytes(sBuf);
        }
        aChannel.writeShort(aMode);
    }

    private static ByteBuffer prepareToWriteStringForConnect(CmChannel aChannel, String[] aStrs, int[] sLimit) throws SQLException
    {
        int sBufLen = aStrs[0].length();
        for (int i = 1; i < aStrs.length; i++)
        {
            sBufLen += aStrs[i].length();
        }
        sBufLen *= aChannel.getByteLengthPerChar();
        ByteBuffer sBuf = ByteBuffer.allocate(sBufLen);
        for (int i = 0; i < aStrs.length; i++)
        {
            CoderResult sEncResult = aChannel.encodeString(aStrs[i], sBuf);
            if (sEncResult == CoderResult.OVERFLOW)
            {
                Error.throwInternalError(ErrorDef.INTERNAL_BUFFER_OVERFLOW);
            }
            sLimit[i] = sBuf.position();
        }
        sBuf.flip();
        return sBuf;
    }

    // BUG-38496 Notify users when their password expiration date is approaching.
    private static void readConnectV3Result(CmChannel aChannel, CmConnectExResult aResult, CmProtocolContext aContext) throws SQLException
    {
        long sSCN;  

        aResult.setSessionID(aChannel.readInt());
        aResult.setResultCode(aChannel.readInt());
        aResult.setResultInfo(aChannel.readInt());
        sSCN = aChannel.readLong();
        aResult.setSCN(sSCN);  // TASK-7220 고성능 분산공유트랜잭션 정합성 
        if (Error.toClientSideErrorCode(aResult.getResultCode()) != ErrorDef.IGNORE_NO_ERROR)
        {
            CmErrorResult sError = new CmErrorResult(aResult.getResultOp(), aResult.getResultInfo(), aResult.getResultCode(), null, false);
            aContext.addError(sError);
        }
        
        if (sSCN > 0)
        {
            aContext.updateSCN(sSCN);
        }
    }

    static void writeDisconnect(CmChannel aChannel) throws SQLException
    {
        aChannel.checkWritable(2);
        aChannel.writeOp(DB_OP_DISCONNECT);
        aChannel.writeByte((byte)0); // option은 사용하지 않음
    }

    static void writeCommit(CmChannel aChannel) throws SQLException
    {
        aChannel.checkWritable(2);
        aChannel.writeOp(DB_OP_TRANSACTION);
        aChannel.writeByte(TRANSACTION_OP_COMMIT);
    }

    /**
     * ClientAutoCommit이 On인 경우에만 버퍼에 커밋프로토콜을 저장한다.
     * @param aChannel Socket통신을 위한 CmChannel객체
     * @param aClientSideAutoCommit 클라이언트사이드오토커밋여부
     * @throws SQLException 커밋프로토콜 전송이 실패한 경우
     */
    static void writeClientCommit(CmChannel aChannel, boolean aClientSideAutoCommit) throws SQLException
    {
        if (aClientSideAutoCommit)
        {
            CmOperation.writeCommit(aChannel);
        }
    }

    static void writeRollback(CmChannel aChannel) throws SQLException
    {
        aChannel.checkWritable(2);
        aChannel.writeOp(DB_OP_TRANSACTION);
        aChannel.writeByte(TRANSACTION_OP_ROLLBACK);
    }

    /* PROJ-2733 */
    static void writeShardTransaction(CmChannel aChannel, boolean aIsCommit) throws SQLException
    {
        short sTouchedNodeCount = 0;
        aChannel.checkWritable(2 + 2 + sTouchedNodeCount * 4);
        aChannel.writeOp(CmOperation.DB_OP_SHARD_TRANSACTION_V3);
        if (aIsCommit)
        {
            aChannel.writeByte((byte)1);  // commit
        }
        else
        {
            aChannel.writeByte((byte)2);  // rollback
        }
        aChannel.writeShort(sTouchedNodeCount);
    }

    private static void readShardTransactionResult(CmChannel aChannel, CmProtocolContext aContext) throws SQLException
    {
        long sSCN = aChannel.readLong();
        if (sSCN > 0)
        {
            aContext.updateSCN(sSCN);
        }
    }

    static void writePrepare(CmChannel aChannel, int aCID, String aSql, byte aExecMode, byte aHoldMode, byte aKeySetDrivenMode, boolean aNliteralReplace) throws SQLException
    {
        // BUG-45156 encoding시 예외가 발생할수도 있기 때문에 버퍼에 write하기 전에 encoding 먼저 수행한다.
        int sWriteStringMode = aNliteralReplace ? WRITE_STRING_MODE_NLITERAL : WRITE_STRING_MODE_DB;
        int sBytesLen = aChannel.prepareToWriteString(aSql, sWriteStringMode);

        aChannel.checkWritable(27);
        aChannel.writeOp(DB_OP_PREPARE_BY_CID_V3);
        aChannel.writeInt(aCID); // client에서 만든 session statement id
        aChannel.writeByte((byte)(aExecMode | aHoldMode | aKeySetDrivenMode));
        // BUG-48775 Reserved 17 bytes
        aChannel.writeLong(0);  
        aChannel.writeLong(0);
        aChannel.writeByte((byte)0);
        aChannel.writeInt(sBytesLen);
        aChannel.writePreparedString();
    }

    private static void readPrepareResult(CmChannel aChannel, CmPrepareResult aResult) throws SQLException
    {
        aResult.setStatementId(aChannel.readInt());
        aResult.setStatementType(aChannel.readInt());
        aResult.setParameterCount(aChannel.readShort());
        aResult.setResultSetCount(aChannel.readShort());
        aChannel.readLong();        // BUG-48775 Reserved 8 bytes
        aResult.setPrepared(true);  // BUG-42424 서버로부터 값을 전달받은 후 flag를 enable해준다.
    }

    static void writeExecuteV3(CmChannel aChannel, int aStatementId, int aRowNumber, byte aMode, CmProtocolContext aContext) throws SQLException
    {
        aChannel.checkWritable(41);
        aChannel.writeOp(DB_OP_EXECUTE_V3);
        aChannel.writeInt(aStatementId);
        aChannel.writeInt(aRowNumber);
        aChannel.writeByte(aMode);

        // TASK-7220 고성능 분산공유트랜잭션 정합성
        aChannel.writeLong(aContext.getDistTxInfo().getSCN());
        aChannel.writeLong(aContext.getDistTxInfo().getTxFirstStmtSCN());
        aChannel.writeLong(aContext.getDistTxInfo().getTxFirstStmtTime());
        aChannel.writeByte(aContext.getDistTxInfo().getDistLevel());
        aChannel.writeInt(0);  /* TASK-7219 Non-shard DML */
        aChannel.writeShort((short)0);  /* BUG-48315 FIXME */
    }

    private static void readExecuteV3Result(CmChannel aChannel, CmProtocolContext aContext) throws SQLException
    {
        CmExecutionResult sExecResult = (CmExecutionResult)aContext.getCmResult(CmExecutionResult.MY_OP);
        sExecResult.setStatementId(aChannel.readInt());
        int sRowNumber = aChannel.readInt();
        int sResultSetCount = aChannel.readUnsignedShort();
        long sUpdatedRowCount = aChannel.readLong();
        aChannel.readLong(); // unused. fetched row count
        long sSCN = aChannel.readLong();  
        sExecResult.setSessionPropID(aChannel.readShort());   // For "alter session set global_transaction_level ~ " 
        int sSessionPropValueLen = aChannel.readInt();  
        // BUGBUG : alter session문이 아닐때도 0 이상의 값이 넘어옴. 확인 필요.
        if (sSessionPropValueLen > 0)
        {
            sExecResult.setSessionPropValueStr(aChannel.readString(sSessionPropValueLen));
        }
        
        if (sExecResult.isBatchMode() && sResultSetCount > 0)
        {
            // batch 작업안에 fetch 구문이 있으면 예외를 올려야 한다.
            CmErrorResult sError = new CmErrorResult(DB_OP_EXECUTE_V3,
                                                     sRowNumber,
                                                     Error.toServerSideErrorCode(ErrorDef.INVALID_BATCH_OPERATION_WITH_SELECT, 0),
                                                     ErrorDef.getErrorMessage(ErrorDef.INVALID_BATCH_OPERATION_WITH_SELECT),
                                                     true);
            aContext.addError(sError);
            sUpdatedRowCount = Statement.EXECUTE_FAILED; // overwrite
        }
        sExecResult.setRowNumber(sRowNumber);
        sExecResult.setResultSetCount(sResultSetCount);
        sExecResult.setUpdatedRowCount(sUpdatedRowCount);
        
        if (sSCN > 0)
        {
            aContext.updateSCN(sSCN);
        }
    }

    /* BUG-39463 Add new fetch protocol that can request over 65535 rows.*/
    static void writeFetchV3(CmChannel aChannel, int aStatementId, short aResultSetId, int aFetchCount) throws SQLException
    {
        aChannel.checkWritable(23);
        aChannel.writeOp(DB_OP_FETCH_V3);
        aChannel.writeInt(aStatementId);
        aChannel.writeShort(aResultSetId);
        aChannel.writeInt(aFetchCount);
        aChannel.writeShort((short)1);  // fetch할 시작 컬럼 위치
        aChannel.writeShort((short)0);  // fetch할 끝 컬럼 위치: 0이면 컬럼 개수만큼
        aChannel.writeLong(0);          // Reserved
    }

    private static void readFetchBeginResult(CmChannel aChannel, CmGetColumnInfoResult aGetColResult, CmFetchResult aFetchResult) throws SQLException
    {
        aFetchResult.setStatementId(aChannel.readInt());
        aFetchResult.setResultSetId(aChannel.readShort());
        beginFetch(aGetColResult, aFetchResult);
    }

    private static void beginFetch(CmGetColumnInfoResult aGetColResult, CmFetchResult aFetchResult)
    {
        List<Column> sColumns = aGetColResult.getColumns();
        if (sColumns == null)
        {
            return;
        }

        // BUG-47460 maxfieldsize 값이 있을때만 setMaxBinayLength를 호출한다.
        int sMaxFieldSize = aFetchResult.getMaxFieldSize();
        if (sMaxFieldSize > 0)
        {
            for (Column sColumn : sColumns)
            {
                sColumn.setMaxBinaryLength(sMaxFieldSize);
            }
        }
        aFetchResult.fetchBegin(sColumns);
    }

    private static void readFetchResult(CmChannel aChannel, CmFetchResult aResult) throws SQLException
    {
        long sRowSize = aChannel.readUnsignedInt();

        // BUG-43807 Rowhandle에 저장하는 부분을 따로 하지 않고 한꺼번에 수행한다.
        aResult.increaseStoreCursor();
        List<Column> sColumns = aResult.getColumns();
        for (Column sColumn : sColumns)
        {
            sColumn.readFrom(aChannel, aResult);
        }
        aResult.updateFetchStat(sRowSize);
    }

    private static void readFetchEndResult(CmChannel aChannel, CmGetColumnInfoResult aGetColResult, CmFetchResult aResult) throws SQLException
    {
        aResult.setStatementId(aChannel.readInt());
        aResult.setResultSetId(aChannel.readShort());
        if (!aResult.isBegun())
        {
            // fetch 결과가 없으면 (no rows이면) fetch begin없이 바로 fetch end가 온다.
            // 이때 이 작업을 해줘야 한다.
            beginFetch(aGetColResult, aResult);
        }
        aResult.fetchEnd();
    }

    static void writeGetColumn(CmChannel aChannel, int aStatementId, short aResultSetId) throws SQLException
    {
        aChannel.checkWritable(9);
        aChannel.writeOp(DB_OP_GET_COLUMN_INFO);
        aChannel.writeInt(aStatementId);
        aChannel.writeShort(aResultSetId);
        aChannel.writeShort((short) 0);
    }

    private static void readGetColumnInfoListResult(CmChannel aChannel, CmGetColumnInfoResult aResult, int aMaxBytesPerChar) throws SQLException
    {
        aResult.setStatementId(aChannel.readInt());
        aChannel.readShort(); // ResultSet ID는 사용하지 않음

        short sColCount = aChannel.readShort();
        List<Column> sColumnsList = new ArrayList<Column>(sColCount);
        for (int i = 0; i < sColCount; i++)
        {
            int sDataType = aChannel.readInt();
            int sLanguage = aChannel.readInt();
            byte sArguments = aChannel.readByte();
            int sPrecision = aChannel.readInt();
            int sScale = aChannel.readInt();
            byte sFlag = aChannel.readByte();

            String sDisplayName = aChannel.readStringForColumnInfo();
            String sColumnName = aChannel.readStringForColumnInfo();
            String sBaseColumnName = aChannel.readStringForColumnInfo();
            String sTableName = aChannel.readStringForColumnInfo();
            String sBaseTableName = aChannel.readStringForColumnInfo();
            String sSchemaName = aChannel.readStringForColumnInfo();
            String sCatalogName = aChannel.readStringForColumnInfo();

            boolean sNullable = (sFlag & COLUMN_INFO_FLAG_NULLABLE) > 0;
            boolean sUpdatable = (sFlag & COLUMN_INFO_FLAG_UPDATABLE) > 0;

            ColumnInfo sColumnInfo = new ColumnInfo();
            sColumnInfo.setColumnInfo(sDataType,
                                      sLanguage,
                                      sArguments,
                                      sPrecision,
                                      sScale,
                                      ColumnInfo.IN_OUT_TARGET_TYPE_TARGET,
                                      sNullable,
                                      sUpdatable,
                                      sCatalogName,
                                      sTableName,
                                      sBaseTableName,
                                      sColumnName,
                                      sDisplayName,
                                      sBaseColumnName,
                                      sSchemaName,
                                      aMaxBytesPerChar);
            Column sColumn = aChannel.getColumnFactory().getInstance(sColumnInfo.getDataType());
            sColumn.setColumnIndex(i); // BUG-43807 DynamicArray를 바로 찾기위해 Column객체에 index를 할당한다.
            sColumnsList.add(sColumn);
            if (sColumnsList.get(i) == null)
            {
                Error.throwInternalError(ErrorDef.INVALID_OPERATION_PROTOCOL, String.valueOf(aResult.getResultOp()));
            }
            sColumnsList.get(i).setColumnInfo(sColumnInfo);
        }
        aResult.setColumns(sColumnsList);
    }

    private static void readGetColumnInfoResult(CmChannel aChannel, CmGetColumnInfoResult aResult, int aMaxBytesPerChar) throws SQLException
    {
        aResult.setStatementId(aChannel.readInt());
        aChannel.readShort(); // ResultSet ID는 사용하지 않음
        aChannel.readShort(); // BUG-42987 column index는 사용하지 않음

        int sDataType = aChannel.readInt();
        int sLanguage = aChannel.readInt();
        byte sArguments = aChannel.readByte();
        int sPrecision = aChannel.readInt();
        int sScale = aChannel.readInt();
        byte sFlag = aChannel.readByte();
        boolean sNullable = (sFlag & COLUMN_INFO_FLAG_NULLABLE) > 0;
        boolean sUpdatable = (sFlag & COLUMN_INFO_FLAG_UPDATABLE) > 0;
        String sDisplayName = aChannel.readStringForColumnInfo();
        String sColumnName = aChannel.readStringForColumnInfo();
        String sBaseColumnName = aChannel.readStringForColumnInfo();
        String sTableName = aChannel.readStringForColumnInfo();
        String sBaseTableName = aChannel.readStringForColumnInfo();
        String sSchemaName = aChannel.readStringForColumnInfo();
        String sCatalogName = aChannel.readStringForColumnInfo();

        ColumnInfo sColumnInfo = new ColumnInfo();
        sColumnInfo.setColumnInfo(sDataType,
                sLanguage,
                sArguments,
                sPrecision,
                sScale,
                ColumnInfo.IN_OUT_TARGET_TYPE_TARGET,
                sNullable,
                sUpdatable,
                sCatalogName,
                sTableName,
                sBaseTableName,
                sColumnName,
                sDisplayName,
                sBaseColumnName,
                sSchemaName,
                aMaxBytesPerChar);


        Column sColumn = aChannel.getColumnFactory().getInstance(sColumnInfo.getDataType());
        sColumn.setColumnInfo(sColumnInfo);

        aResult.addColumn(sColumn);
    }

    static void writeGetBindParamInfo(CmChannel aChannel, int aStatementId) throws SQLException
    {
        aChannel.checkWritable(7);
        aChannel.writeOp(DB_OP_GET_PARAM_INFO);
        aChannel.writeInt(aStatementId);
        aChannel.writeShort((short)0); // parameter index: 항상 모든 컬럼 정보를 요청하기 때문에 0을 준다.
    }

    private static void readGetBindParamInfoResult(CmChannel aChannel, CmGetBindParamInfoResult aResult, int aMaxBytesPerChar) throws SQLException
    {
        aResult.setStatementId(aChannel.readInt());
        int sParamIdx = aChannel.readShort();    // BUG-42424 parameter index를 맵의 키로 사용한다.
        int sDataType = aChannel.readInt();
        int sLanguage = aChannel.readInt();
        byte sArguments = aChannel.readByte();
        int sPrecision = aChannel.readInt();
        int sScale = aChannel.readInt();
        aChannel.readByte(); // 서버에서 주는 in-out type은 버린다. 의미없는 정보이다.
        byte sInOutType = ColumnInfo.IN_OUT_TARGET_TYPE_NONE;
        byte sNullable = aChannel.readByte();

        ColumnInfo sColumnInfo = new ColumnInfo();
        sColumnInfo.setColumnInfo(sDataType,
                                  sLanguage,
                                  sArguments,
                                  sPrecision,
                                  sScale,
                                  sInOutType,
                                  (sNullable & COLUMN_INFO_FLAG_NULLABLE) > 0,
                                  false,
                                  null,
                                  null,
                                  null,
                                  null,
                                  null,
                                  null,
                                  null,
                                  aMaxBytesPerChar);

        aResult.addColumnInfo(sParamIdx, sColumnInfo);
    }

    static void writeSetBindParamInfoList(CmChannel aChannel, int aStatementId, List<Column> aParams) throws SQLException
    {
        if (CmOperation.isChangedBindInfo(aParams))
        {
            CmOperation.unChangeBindInfo(aParams);
        }
        else
        {
            // BUG-38947 바인드파라메터정보에 변화가 없으면 메타정보를 서버로 전송하지 않고 바로 리턴한다.
            return;
        }

        aChannel.checkWritable(7 + (20 * aParams.size()));
        aChannel.writeOp(DB_OP_SET_PARAM_INFO_LIST);
        aChannel.writeInt(aStatementId);
        aChannel.writeShort((short) aParams.size());
        for (int i = 0; i < aParams.size(); i++)
        {
            ColumnInfo sParamInfo = aParams.get(i).getColumnInfo();
            aChannel.writeShort((short)(i + 1));
            aChannel.writeInt(sParamInfo.getDataType());
            aChannel.writeInt(sParamInfo.getLanguage());
            aChannel.writeByte(sParamInfo.getArguments());
            aChannel.writeInt(sParamInfo.getPrecision()); // char, varchar를 위해 charset이 고려된 precision을 서버로 보낸다.
            aChannel.writeInt(sParamInfo.getScale());
            aChannel.writeByte(sParamInfo.getInOutTargetType());
        }
    }

    /**
     * 파라메터정보가 변경되었는지 체크한다..
     * @param aColumns 바인드된 파라메터의 컬럼정보 배열
     * @return 변경된 파라메터가 있을 경우 true, 없을 경우엔 false
     */
    private static boolean isChangedBindInfo(List<Column> aColumns)
    {
        for (Column sColumn : aColumns)
        {
            if (sColumn.getColumnInfo().isChanged())
            {
                return true;
            }
        }
        return false;
    }

    /**
     * 변경된 파라메터가 있을 경우 파라메터의 상태를 unchanged로 바꿔준다.
     * @param aColumns 바인드된 파라메터의 컬럼정보 배열
     */
    private static void unChangeBindInfo(List<Column> aColumns)
    {
        for (Column sColumn : aColumns)
        {
            ColumnInfo sColumnInfo = sColumn.getColumnInfo();
            if (sColumnInfo.isChanged())
            {
                // BUG-38947 중복해서 파라메터정보를 서버로 보내는 것을 막기위해 ColumnInfo 객체의 mChange변수를 false로
                // 변경해준다. mChange변수는 Column객체가 생성될 때 또는 setXXX메소드로 파라메터 메타정보가 변경될 때
                // true로 셋팅된다.
                sColumnInfo.unchange();
            }
        }
    }

    static void writeBindParamDataIn(CmChannel aChannel, int aStatementId, List<Column> aColumns) throws SQLException
    {
        for (int i = 0; i < aColumns.size(); i++)
        {
            if (aColumns.get(i).getColumnInfo().hasInType())
            {
                aChannel.checkWritable(11);
                aChannel.writeOp(DB_OP_PARAM_DATA_IN);
                aChannel.writeInt(aStatementId);
                aChannel.writeShort((short)(i + 1));
                int sDataSize = aColumns.get(i).prepareToWrite(aChannel);
                aChannel.writeInt(sDataSize);
                int sActSize = aColumns.get(i).writeTo(aChannel);
                if (sActSize != sDataSize)
                {
                    Error.throwInternalError(ErrorDef.INCONSISTANCE_DATA_LENGTH,
                                             String.valueOf(sDataSize),
                                             String.valueOf(sActSize));
                }
            }
        }
    }

    /**
     * ListBufferHandle 의 Buffer 에 담긴  Data 를 channel에 쓴다.
     *
     * @param aChannel Data를 쓸 channel
     * @param aStatementId Statement ID
     * @param aBufferHandle  List protocol 의 data buffer 를 handle 하는 객체
     * @param aIsAtomic Atomic Operation 사용 여부
     * @param aIsArray Array mode 적용 여부
     * @throws SQLException channel에 쓰지 못한 경우
     */
    static void writeBindParamDataInListV3(CmChannel aChannel, int aStatementId,
                                           ListBufferHandle aBufferHandle,
                                           boolean aIsAtomic,
                                           boolean aIsArray,
                                           CmProtocolContext aContext) throws SQLException
    {
        aChannel.checkWritable(53);
        aChannel.writeOp(DB_OP_PARAM_DATA_IN_LIST_V3);
        aChannel.writeInt(aStatementId);
        // List Protocol 에서 FromRowNumber 는 항상 1
        aChannel.writeInt(1);
        aChannel.writeInt(aBufferHandle.size());

        // BUG-46443 batch모드일때만 array 모드로 실행한다.
        if (aIsArray)
        {
            aChannel.writeByte(aIsAtomic ? CmOperation.EXECUTION_MODE_ATOMIC : CmOperation.EXECUTION_MODE_ARRAY);
        }
        else
        {
            aChannel.writeByte(CmOperation.EXECUTION_MODE_NORMAL);
        }

        // TASK-7220 고성능 분산공유트랜잭션 정합성
        aChannel.writeLong(aContext.getDistTxInfo().getSCN());
        aChannel.writeLong(aContext.getDistTxInfo().getTxFirstStmtSCN());
        aChannel.writeLong(aContext.getDistTxInfo().getTxFirstStmtTime());
        aChannel.writeByte(aContext.getDistTxInfo().getDistLevel());
        aChannel.writeInt(0);  /* TASK-7219 Non-shard DML */
        aChannel.writeShort((short)0);  /* BUG-48489 FIXME */

        aChannel.writeLong(aBufferHandle.getBufferPosition());
        aBufferHandle.flipBuffer();
        aChannel.writeBytes(aBufferHandle.getBuffer());
    }

    private static void readBindParamDataInListV3Result(CmChannel aChannel, CmProtocolContext aContext) throws SQLException
    {
        long sSCN;
        CmExecutionResult sExecResult = (CmExecutionResult)aContext.getCmResult(CmExecutionResult.MY_OP);
        sExecResult.setStatementId(aChannel.readInt());
        int sFromRowNumber  = aChannel.readInt();
        int sToRowNumber     = aChannel.readInt();
        aChannel.readUnsignedShort(); // Select 에서만 유효, resultset count
        long sUpdatedRowCount = aChannel.readLong();
        aChannel.readLong(); // unused. fetched row count
        sSCN = aChannel.readLong(); // SCN. TASK-7220 고성능 분산공유트랜잭션 정합성 

        for (int i = sFromRowNumber; i <= sToRowNumber; i++)
        {
            sExecResult.setUpdatedRowCount(sUpdatedRowCount);
        }
        
        if (sSCN > 0)
        {
            aContext.updateSCN(sSCN);
        }
    }

    private static void readBindParamDataOutListResult(CmChannel aChannel, CmBindParamDataOutResult aResult) throws SQLException
    {
        aResult.setStatementId(aChannel.readInt());
        aChannel.readInt(); // BUG-43807 rownumber는 사용되지 않기 때문에 skip한다.
        aChannel.readUnsignedInt(); // skip RowSize
        for (Column sColumn : aResult.getBindParams())
        {
            if (sColumn.getColumnInfo().getInOutTargetType() != ColumnInfo.IN_OUT_TARGET_TYPE_IN)
            {
                sColumn.readParamsFrom(aChannel);
            }
        }
        aResult.storeLobDataAtBatch();
    }

    static void writeGetProperty(CmChannel aChannel, short aPropertyKey) throws SQLException
    {
        aChannel.checkWritable(3);
        aChannel.writeOp(DB_OP_GET_PROPERTY);
        aChannel.writeShort(aPropertyKey);
    }

    private static void readGetPropertyResult(CmChannel aChannel, CmGetPropertyResult aResult) throws SQLException
    {
        short sPropertyKey = aChannel.readShort();
        int sLength;
        String sStrValue;
        Column sValueContainer = null;
        byte sByteValue;
        int sIntValue;

        switch (sPropertyKey)
        {
            case (AltibaseProperties.PROP_CODE_NLS):
            case (AltibaseProperties.PROP_CODE_DATE_FORMAT):
            case (AltibaseProperties.PROP_CODE_SERVER_PACKAGE_VERSION):
            case (AltibaseProperties.PROP_CODE_NLS_CHARACTERSET):
            case (AltibaseProperties.PROP_CODE_NLS_NCHAR_CHARACTERSET):
                sLength = aChannel.readInt();
                sStrValue = aChannel.readDecodedString(sLength, 0);
                sValueContainer = aChannel.getColumnFactory().createVarcharColumn();
                sValueContainer.setValue(sStrValue);
                break;
            case (AltibaseProperties.PROP_CODE_AUTOCOMMIT):
            case (AltibaseProperties.PROP_CODE_ENDIAN):
                sByteValue = aChannel.readByte();
                sValueContainer = ColumnFactory.createBooleanColumn();
                sValueContainer.setValue((int)sByteValue);
                break;
            case (AltibaseProperties.PROP_CODE_EXPLAIN_PLAN):
                sByteValue = aChannel.readByte();
                sValueContainer = ColumnFactory.createTinyIntColumn();
                sValueContainer.setValue(sByteValue);
                break;
            case (AltibaseProperties.PROP_CODE_ISOLATION_LEVEL): /* BUG-39817 */
            case (AltibaseProperties.PROP_CODE_OPTIMIZER_MODE):
            case (AltibaseProperties.PROP_CODE_HEADER_DISPLAY_MODE):
            case (AltibaseProperties.PROP_CODE_STACK_SIZE):
            case (AltibaseProperties.PROP_CODE_IDLE_TIMEOUT):
            case (AltibaseProperties.PROP_CODE_QUERY_TIMEOUT):
            case (AltibaseProperties.PROP_CODE_FETCH_TIMEOUT):
            case (AltibaseProperties.PROP_CODE_UTRANS_TIMEOUT):
            case (AltibaseProperties.PROP_CODE_NORMALFORM_MAXIMUM):
            case (AltibaseProperties.PROP_CODE_NLS_NCHAR_LITERAL_REPLACE):
            case (AltibaseProperties.PROP_CODE_DDL_TIMEOUT):
            case (AltibaseProperties.PROP_CODE_LOB_CACHE_THRESHOLD):
            case (AltibaseProperties.PROP_CODE_INDOUBT_FETCH_TIMEOUT):
                sIntValue = aChannel.readInt();
                sValueContainer = ColumnFactory.createIntegerColumn();
                sValueContainer.setValue(sIntValue);
                break;
            case (AltibaseProperties.PROP_CODE_GLOBAL_TRANSACTION_LEVEL):
            case (AltibaseProperties.PROP_CODE_SHARD_STATEMENT_RETRY):
            case (AltibaseProperties.PROP_CODE_INDOUBT_FETCH_METHOD):
                sByteValue = aChannel.readByte();
                sValueContainer = ColumnFactory.createIntegerColumn();
                sValueContainer.setValue((int)sByteValue);
                break;
            default:
                Error.throwSQLException(ErrorDef.INVALID_PROPERTY_ID, String.valueOf(sPropertyKey));
                break;
        }

        aResult.addProperty(sPropertyKey, sValueContainer);
    }

    static void writeSetPropertyV3(CmChannel aChannel, short aPropertyKey, Column aValue) throws SQLException
    {
        // BUG-41793 Keep a compatibility among tags
        //
        // Replaced   : CMP_OP_DB_PropertySetV2(1) | PropetyID(2) | ValueLen(4) | Value(?)
        // Deprecated : CMP_OP_DB_PropertySet  (1) | PropetyID(2) | Value(?)
        //
        // Deprecated writeSetProperty() since CM 7.1.3
        int sValueLen = aValue.prepareToWrite(aChannel);

        aChannel.checkWritable(7 + sValueLen);
        aChannel.writeOp(DB_OP_SET_PROPERTY_V3);
        aChannel.writeShort(aPropertyKey);
        aChannel.writeInt(sValueLen);
        aValue.writeTo(aChannel);
    }

    private static void readSetPropertyV3Result(CmChannel aChannel, CmSetPropertyResult aResult, CmProtocolContext aContext) throws SQLException
    {
        byte sGTxLevel;
        long sSCN;
        
        int sPropertyID = aChannel.readShort();
        aResult.setPropertyID(sPropertyID);
        if (sPropertyID == AltibaseProperties.PROP_CODE_GLOBAL_TRANSACTION_LEVEL)
        {
            sGTxLevel = aChannel.readByte();
            sSCN = aChannel.readLong();
            aResult.setGTxLevel(sGTxLevel);
            aResult.setSCN(sSCN);
            if (sGTxLevel == GlobalTransactionLevel.GCTX.getValue())
            {
                if (sSCN > 0)
                {
                    aContext.updateSCN(sSCN);
                }
            }
            else
            {
                aContext.initDistTxInfo();
                // tc에서 검증을 위해 initDistTxInfo 호출 후 AltibaseConnection.setDistTxInfoForVerify()를 호출해야 하는데.. 여기서는 생략하기로 함.
                // 왜냐하면 여기에서는 현재 커넥션 인스턴스를 알 수가 없음.
                // 현재 기준으로, 본 함수는 connection 시점에만 호출되고 tc에서 connect 후에 distTxInfo 검사를 하지 않으므로 생략해도 무방..
            }
        }
    }

    static void writeCloseCursor(CmChannel aChannel, int aStmtID, short aResultSetID) throws SQLException
    {
        aChannel.checkWritable(8);
        aChannel.writeOp(DB_OP_FREE);
        aChannel.writeInt(aStmtID);
        aChannel.writeShort(aResultSetID);
        aChannel.writeByte(FREE_MODE_CLOSE);
    }

    static void writeDropStatement(CmChannel aChannel, int aStmtID) throws SQLException
    {
        aChannel.checkWritable(8);
        aChannel.writeOp(DB_OP_FREE);
        aChannel.writeInt(aStmtID);
        aChannel.writeShort(FREE_ALL_RESULTSET);
        aChannel.writeByte(FREE_MODE_DROP);
    }

    static void writeCancelStatement(CmChannel aChannel, int aCID) throws SQLException
    {
        aChannel.checkWritable(5);
        aChannel.writeOp(DB_OP_CANCEL_BY_CID);
        aChannel.writeInt(aCID);
    }

    static void writeLobGet(CmChannel aChannel, long aLocatorId, long aOffset, long aSize) throws SQLException
    {
        aChannel.checkWritable(17);
        aChannel.writeOp(DB_OP_LOB_GET);
        aChannel.writeLong(aLocatorId);
        aChannel.writeUnsignedInt(aOffset);
        aChannel.writeUnsignedInt(aSize);
    }

    private static void readLobGetResult(CmChannel aChannel, CmProtocolContext aContext) throws SQLException
    {
        long sLocatorId = aChannel.readLong();
        int sOffset = aChannel.readInt();
        long sByteLength = aChannel.readUnsignedInt();
        if (aContext instanceof CmProtocolContextLob)
        {
            CmBlobGetResult sResult = (CmBlobGetResult)aContext.getCmResult(CmBlobGetResult.MY_OP);
            sResult.setLocatorId(sLocatorId);
            sResult.setOffset(sOffset);
            sResult.setLobLength(sByteLength);
            CmProtocolContextLob sLobContext = (CmProtocolContextLob)aContext;
            aChannel.readBytes(sLobContext.getBlobData(), sLobContext.getDstOffset(), (int)sResult.getLobLength());
        }
        else
        {
            aChannel.skip((int)sByteLength);
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mLogger.log(Level.INFO, "LOB skipped: locator id = "+ sLocatorId
                            +", offset = "+ sOffset
                            +", byte len = "+ sByteLength);
            }
        }
    }

    static void writeLobGetSize(CmChannel aChannel, long aLocatorId) throws SQLException
    {
        aChannel.checkWritable(9);
        aChannel.writeOp(DB_OP_LOB_GET_SIZE_V3);
        aChannel.writeLong(aLocatorId);
    }

    private static void readLobGetSizeResult(CmChannel aChannel, CmBlobGetResult aResult) throws SQLException
    {
        aResult.setLocatorId(aChannel.readLong());
        aResult.setLobLength(aChannel.readInt());
        aChannel.readByte();  // IsNullLob.  TASK-7220 고성능 분산공유트랜잭션 정합성 
    }

    static void writeLobPutBegin(CmChannel aChannel, long aLocatorId, long aOffset) throws SQLException
    {
        aChannel.checkWritable(17);
        aChannel.writeOp(DB_OP_LOB_PUT_BEGIN);
        aChannel.writeLong(aLocatorId);
        aChannel.writeUnsignedInt(aOffset);
        aChannel.writeUnsignedInt(LobConst.LOB_BUFFER_SIZE);
    }

    /**
     * LobPut 프로토콜을 쓴다.
     * 
     * @param aContext 프로토콜 컨텍스트
     * @param aSource 프로토콜로 쓸 데이타
     * @return 프로토콜에 쓴 데이타 사이즈(byte 단위)
     * @throws SQLException 프로토콜을 쓰는데 실패했을 경우
     * @throws IOException 데이타를 읽는데 실패했을 경우
     */
    static long writeLobPut(CmProtocolContextLob aContext, ReadableCharChannel aSource) throws SQLException, IOException
    {
        long sTotalWrited = 0;
        while (true)
        {
            int sWrited = aContext.channel().writeBytes4Clob(aSource, aContext.locatorId(), aContext.isCopyMode());
            if (sWrited == -1)
            {
                break;
            }
            sTotalWrited += sWrited;
        }
        return sTotalWrited;
    }

    static void writeLobPut(CmChannel aChannel, long aLocatorId, byte[] aSource, int aSourceOffset, int aLengthPerOp) throws SQLException
    {
        while(aLengthPerOp > 0)
        {
            int sWritableSize = aChannel.checkWritable4Lob(aLengthPerOp);
            aChannel.writeOp(DB_OP_LOB_PUT);
            aChannel.writeLong(aLocatorId);
            aChannel.writeUnsignedInt(sWritableSize);
            aChannel.writeBytes(aSource, aSourceOffset, sWritableSize);

            aSourceOffset += sWritableSize;
            aLengthPerOp -= sWritableSize;
        }
    }
    
    static void writeLobPutEnd(CmChannel aChannel, long aLocatorId) throws SQLException
    {
        aChannel.checkWritable(9);
        aChannel.writeOp(DB_OP_LOB_PUT_END);
        aChannel.writeLong(aLocatorId);
    }

    static void writeLobTruncate(CmChannel aChannel, long aLocatorId, int aOffset) throws SQLException
    {
        aChannel.checkWritable(13);
        aChannel.writeOp(DB_OP_LOB_TRIM);
        aChannel.writeLong(aLocatorId);
        aChannel.writeUnsignedInt(aOffset);
    }

    static void writeLobGetBytePosCharLen(CmChannel aChannel, long aLocatorId, long aByteOffset, long aCharLength) throws SQLException
    {
        aChannel.checkWritable(17);
        aChannel.writeOp(DB_OP_LOB_GET_BYTE_POS_CHAR_LEN);
        aChannel.writeLong(aLocatorId);
        aChannel.writeUnsignedInt(aByteOffset);
        aChannel.writeUnsignedInt(aCharLength);
    }
    
    static void writeLobGetCharPosCharLen(CmChannel aChannel, long aLocatorId, long aCharOffset, long aCharLength) throws SQLException
    {
        aChannel.checkWritable(17);
        aChannel.writeOp(DB_OP_LOB_GET_CHAR_POS_CHAR_LEN);
        aChannel.writeLong(aLocatorId);
        aChannel.writeUnsignedInt(aCharOffset);
        aChannel.writeUnsignedInt(aCharLength);
    }
    
    private static void readClobGetResult(CmChannel aChannel, CmProtocolContext aContext) throws SQLException
    {
        long sLocatorId = aChannel.readLong();
        long sOffset = aChannel.readUnsignedInt();
        long sCharLength = aChannel.readUnsignedInt();
        long sByteLength = aChannel.readUnsignedInt();
        if (aContext instanceof CmProtocolContextLob)
        {
            CmClobGetResult sResult = (CmClobGetResult)aContext.getCmResult(CmClobGetResult.MY_OP);
            if (sLocatorId == sResult.getLocatorId())
            {
                if (sResult.getOffset() == sOffset) // first data
                {
                    sResult.setCharLength(sCharLength);
                    sResult.setByteLength(sByteLength);
                }
                else // following data
                {
                    sResult.addCharLength(sCharLength);
                    sResult.addByteLength(sByteLength);
                }
                CmProtocolContextLob sLobContext = (CmProtocolContextLob)aContext;
                sCharLength = aChannel.readCharArrayTo(sLobContext.getClobData(), sLobContext.getDstOffset(), (int)sByteLength, (int)sCharLength);
                sLobContext.setDstOffset((int)(sLobContext.getDstOffset() + sCharLength));
            }
            else
            {
                aChannel.skip((int)sByteLength);
                Error.throwInternalError(ErrorDef.INTERNAL_ASSERTION);
            }
        }
        else
        {
            aChannel.skip((int)sByteLength);
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mLogger.log(Level.INFO, "CLOB skipped: locator id = "+ sLocatorId
                            +", offset = "+ sOffset
                            +", char len = "+ sCharLength
                            +", byte len = "+ sByteLength);
            }
        }
    }
    
    static void writeLobBytePos(CmChannel aChannel, long aLocatorId, int aCharLength) throws SQLException
    {
        aChannel.checkWritable(13);
        aChannel.writeOp(DB_OP_LOB_BYTE_POS);
        aChannel.writeLong(aLocatorId);
        aChannel.writeInt(aCharLength);
    }
    
    private static void readLobBytePosResult(CmChannel aChannel, CmClobGetResult aResult) throws SQLException
    {
        aResult.setLocatorId(aChannel.readLong());
        aResult.setOffset(aChannel.readInt());
    }
    
    static void writeLobCharLength(CmChannel aChannel, long aLocatorId) throws SQLException
    {
        aChannel.checkWritable(9);
        aChannel.writeOp(DB_OP_LOB_CHAR_LENGTH);
        aChannel.writeLong(aLocatorId);
    }
    
    private static void readLobCharLengthResult(CmChannel aChannel, CmClobGetResult aResult) throws SQLException
    {
        aResult.setLocatorId(aChannel.readLong());
        aResult.setCharLength(aChannel.readInt());
    }
    
    static void writeLobFree(CmChannel aChannel, long aLocatorId) throws SQLException
    {
        aChannel.checkWritable(9);
        aChannel.writeOp(DB_OP_LOB_FREE);
        aChannel.writeLong(aLocatorId);
    }

    static void writeXaOperation(CmChannel aChannel, byte aOp, int aRmId, long aOpFlag, long aArgument) throws SQLException
    {
        aChannel.checkWritable(22);
        aChannel.writeOp(DB_OP_XA_OPERATION);
        aChannel.writeByte(aOp);
        aChannel.writeInt(aRmId);
        aChannel.writeLong(aOpFlag);
        aChannel.writeLong(aArgument);
    }
    
    static void writeXaTransaction(CmChannel aChannel, byte aOp, int aRmId, long aOpFlag, long aArgument, long aFormatId, byte[] aGlobalTransactionId, byte[] aBranchQualifier) throws SQLException
    {
        aChannel.checkWritable(46 + AltibaseXid.MAXDATASIZE);
        aChannel.writeOp(DB_OP_XA_TRANSACTION);
        aChannel.writeByte(aOp);
        aChannel.writeInt(aRmId);
        aChannel.writeLong(aOpFlag);
        aChannel.writeLong(aArgument);
        aChannel.writeLong(aFormatId);
        aChannel.writeLong(aGlobalTransactionId.length);
        aChannel.writeLong(aBranchQualifier.length);
        aChannel.writeBytes(aGlobalTransactionId);
        aChannel.writeBytes(aBranchQualifier);
        // Xid Data가 128B가 되도록 padding
        int sPadLen = AltibaseXid.MAXDATASIZE - (aGlobalTransactionId.length + aBranchQualifier.length);
        for (int i = 0; i < sPadLen; i++)
        {
            aChannel.writeByte((byte) 0);
        }
    }
    
    private static void readXidResult(CmChannel aChannel, CmXidResult aResult) throws SQLException
    {
        long sFormatId = aChannel.readLong();
        int sGlobalTransIdLen = (int) aChannel.readLong();
        int sBranchQualifierLen = (int) aChannel.readLong();
        byte[] sGlobalTransId = new byte[sGlobalTransIdLen];
        byte[] sBranchQualifier = new byte[sBranchQualifierLen];
        aChannel.readBytes(sGlobalTransId);
        aChannel.readBytes(sBranchQualifier);
        // Xid Data는 항상 128B를 채워서 보내게 되어있다. 그러므로, 남은건 skip 해야한다.
        aChannel.skip(AltibaseXid.MAXDATASIZE - (sGlobalTransIdLen + sBranchQualifierLen));

        aResult.addXid(new AltibaseXid(sFormatId, sGlobalTransId, sBranchQualifier));
    }

    private static void readXaResult(CmChannel aChannel, CmXAResult aResult) throws SQLException
    {
        aChannel.readByte(); // XA op
        aResult.setResultValue(aChannel.readInt());
    }

    /**
     * Get Plan operation을 쓴다.
     *
     * @param aChannel operation을 쓸 channel 
     * @param aStmtID  Statement ID
     *
     * @throws SQLException channel에 쓰지 못한 경우
     */
    static void writeGetPlan(CmChannel aChannel, int aStmtID) throws SQLException
    {
        aChannel.checkWritable(5);
        aChannel.writeOp(DB_OP_GET_PLAN);
        aChannel.writeInt(aStmtID);
    }

    /**
     * Get Plan operation 결과를 읽는다. 
     *
     * @param aChannel 결과를 읽을 channel
     * @param aResult  결과를 담을 객체
     *
     * @throws SQLException 결과를 읽을 수 없는 경우
     */
    private static void readGetPlanResult(CmChannel aChannel, CmGetPlanResult aResult) throws SQLException
    {
        aResult.setStatementId(aChannel.readInt());
        int sPlanTextSize = aChannel.readInt();
        aResult.setPlanText(aChannel.readDecodedString(sPlanTextSize, 0));
    }

    public static void throwShardFailoverIsNotAvailableException(String aNodeName,
                                                                 String aIpAddress,
                                                                 int aPort) throws ShardFailoverIsNotAvailableException
    {
        int sErrorCode = ErrorDef.SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE;
        String sErrorMsg = ErrorDef.getErrorMessage(ErrorDef.SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE);
        // BUG-46790 shard failover 관련 exception 정보에 ip, port 추가
        ShardFailoverIsNotAvailableException sFailoverException =
                new ShardFailoverIsNotAvailableException(sErrorMsg, sErrorCode, aNodeName,
                                                         aIpAddress, aPort);
        shard_log(Level.SEVERE, "(THROW SHARD FAILOVER EXCEPTION) ", sFailoverException);

        throw sFailoverException;
    }
}
