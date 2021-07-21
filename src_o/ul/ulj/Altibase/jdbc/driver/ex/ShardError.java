/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.ex;

import Altibase.jdbc.driver.AltibaseConnection;
import Altibase.jdbc.driver.cm.*;
import Altibase.jdbc.driver.sharding.core.*;
import Altibase.jdbc.driver.util.StringUtils;

import java.math.BigInteger;
import java.sql.Connection;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

/**
 * Sharding 관련된 에러 메세지를 파싱하여 SMN, Failover Align 과 같은 작업들을 처리한다.
 */
public class ShardError
{
    private static final String DATA_NODE_SMN_PREFIX                = "Data Node SMN=";
    private static final String SHARD_DISCONNECT_PREFIX             = "Disconnect=N";
    private static final String EXECUTE_RESULT_PREFIX               = "ExecuteV2Result";   // ErrorResult의 ErrMsg. 프로토콜 버전별로 스트링을 달리 하지 않고 대표 스트링을 사용 

    // BUG-46790 에러메세지로부터 node id, error code를 바로 가져오기 위한 정규표현식 선언
    private static final String SHARD_FOOT_PRINT_REGULAR_EXPRESSION = ".*?(\\[<<ERC-(\\w*) NODE-ID:(\\d*)>>])";

    /**
     * 서버로부터 넘어온 SMN Invalid 에러를 처리한다.
     * @param aErrorResult 서버에서 받은 에러/경고 정보
     * @param aExceptionList SQLException 리스트
     * @param aWarningList SQLWarning 리스트
     * @param aErrorCode 에러코드
     * @param aErrorMsg 에러메세지
     * @param aSQLState SQLState
     */
    static void processInvalidSMNError(CmErrorResult aErrorResult, List<SQLException> aExceptionList,
                                       List<SQLWarning> aWarningList,
                                       int aErrorCode, String aErrorMsg, String aSQLState)
    {
        // BUG-46513 에러메세지를 파싱해 SMN값과 NeedToDisconnect값을 얻어온다.
        setShardMetaNumberOfDataNode(aErrorResult, aErrorMsg);
        boolean sNeedToDisconnect = setShardDisconnectValue(aErrorResult, aErrorMsg);

        switch (aErrorResult.getErrorOp())
        {
            case CmOperationDef.DB_OP_SET_PROPERTY_V3 :
                if (sNeedToDisconnect)
                {
                    aExceptionList.add(new SQLException(aErrorMsg, aSQLState, aErrorCode));
                }
                else
                {
                    aWarningList.add(new SQLWarning(aErrorMsg, aSQLState, aErrorCode));
                }
                break;
            case CmOperationDef.DB_OP_EXECUTE_V3:
            case CmOperationDef.DB_OP_PARAM_DATA_IN_LIST_V3:
                aWarningList.add(new SQLWarning(aErrorMsg, aSQLState, aErrorCode));
                parseExecuteResults(aErrorResult.getProtocolContext(), aErrorMsg);
                break;
            case CmOperationDef.DB_OP_TRANSACTION:
            case CmOperationDef.DB_OP_SHARD_TRANSACTION_V3:
                aWarningList.add(new SQLWarning(aErrorMsg, aSQLState, aErrorCode));
                break;
            default:
                break;
        }
    }

    /**
     * 에러메세지를 파싱하여 얻은 SMN값을 node 커넥션 및 meta 커넥션에 반영한다.
     * @param aErrorResult 서버에서 받은 에러/경고 정보
     * @param aErrorMsg 에러메세지
     */
    private static void setShardMetaNumberOfDataNode(CmErrorResult aErrorResult, String aErrorMsg)
    {
        long sSMNOfDataNode = getSMNOfDataNodeFromErrorMsg(aErrorMsg);
        aErrorResult.setSMNOfDataNode(sSMNOfDataNode);
    }

    /**
     * 에러메세지를 파싱하여 SMN값을 구한다.
     * @param aErrorMsg 에러메세지
     * @return SMN값
     */
    static long getSMNOfDataNodeFromErrorMsg(String aErrorMsg)
    {
        if (StringUtils.isEmpty(aErrorMsg)) return 0;

        int sSMNIndex = aErrorMsg.indexOf(DATA_NODE_SMN_PREFIX);
        if (sSMNIndex < 0)  return 0;

        String sSMNRemainStr = aErrorMsg.substring(sSMNIndex + DATA_NODE_SMN_PREFIX.length());
        StringBuilder sSb = new StringBuilder();
        for (char sEach : sSMNRemainStr.toCharArray())
        {
            if (Character.isDigit(sEach))
            {
                sSb.append(sEach);
            }
            else
            {
                break;
            }
        }

        return Long.parseLong(sSb.toString());
    }

    /**
     * SMN Invalid 에러가 발생했을 때 에러메세지를 파싱하여 executeV2의 결과를 저장한다.<br>
     * ExecuteV2Result전문 예 : ExecuteV2Result 65536 1 1 18446744073709551615 0 0
     * @param aContext 프로토콜 컨텍스트 객체
     * @param aErrorMsg 에러메세지
     */
    static void parseExecuteResults(CmProtocolContext aContext, String aErrorMsg)
    {
        CmExecutionResult sExecResult = (CmExecutionResult)aContext.getCmResult(CmOperation.DB_OP_EXECUTE_V3_RESULT);
        int sIndex = aErrorMsg.indexOf(EXECUTE_RESULT_PREFIX);
        sIndex += EXECUTE_RESULT_PREFIX.length() + 1;
        String sRemain = aErrorMsg.substring(sIndex);
        String[] sValues = sRemain.split(" ");

        sExecResult.setStatementId(Integer.parseInt(sValues[0]));
        sExecResult.setRowNumber(Integer.parseInt(sValues[1]));
        sExecResult.setResultSetCount(Integer.parseInt(sValues[2]));
        String sUpdatedRowCount = sValues[3];
        // BUG-46513 updatedRowCount는 unsigned long 형태의 text로 넘어 오기 때문에 BigInteger로 처리한다.
        BigInteger sBigInt = new BigInteger(sUpdatedRowCount);
        sExecResult.setUpdatedRowCount(sBigInt.longValue());
    }

    /**
     * 에러메세지를 파싱하여 Disconnect값을 반영한다.
     * @param aErrorResult 서버에서 받은 에러/경고 정보
     * @param aErrorMsg 에러 메세지
     */
    private static boolean setShardDisconnectValue(CmErrorResult aErrorResult, String aErrorMsg)
    {
        boolean sNeedToDisconnect = getShardDiconnectFromErrorMsg(aErrorMsg);
        aErrorResult.setNeedToDisconnect(sNeedToDisconnect);

        return sNeedToDisconnect;
    }

    /**
     * 에러메세지를 파싱하여 ShardDisconnect 값을 구한다.
     * @param aErrorMsg 에러메세지
     * @return needToDisconnect여부 <br>
     *         false  : 이전 SMN을 가진 Connection을 허용(default) <br>
     *         true   : 이전 SMN을 가진 Connection을 허용하지 않음
     */
    static boolean getShardDiconnectFromErrorMsg(String aErrorMsg)
    {
        if (StringUtils.isEmpty(aErrorMsg)) return false;

        boolean sIsDisconnect = true;
        if (aErrorMsg.contains(SHARD_DISCONNECT_PREFIX))
        {
            sIsDisconnect = false;
        }

        return sIsDisconnect;
    }

    /**
     * shard sd 모듈 에러메세지를 파징하여 에러코드 및 nodeID값을 errorResult객체에 셋팅한다.
     * @param aExceptionList SQLException이 저장되는 ArrayList
     * @param aErrorResult 서버에서 받은 에러/경고 정보
     * @param aErrorCode 에러코드
     * @param aErrorMsg footprint [<<ERC-NNNNN NODE-ID:NNN>>] 가 포함된 에러메세지
     * @param aSQLState SQLState
     */
    static void parseShardingError(List<SQLException> aExceptionList, CmErrorResult aErrorResult, int aErrorCode,
                                   String aErrorMsg, String aSQLState)
    {
        // BUG-46790 정규표현식 매칭할때 개행문자를 포함하기 위해 Patter.DOTALL을 셋팅해 준다.
        Pattern sFootPrintPattern = Pattern.compile(SHARD_FOOT_PRINT_REGULAR_EXPRESSION, Pattern.DOTALL);
        Matcher sMatcher = sFootPrintPattern.matcher(aErrorMsg);
        boolean sMatchFounded = false;
        boolean sIsServerSideNormalErrorExist = false;
        SQLException sCause = new SQLException(aErrorMsg, aSQLState, aErrorCode);
        while (sMatcher.find())
        {
            sMatchFounded = true;
            String sFootPrint = sMatcher.group(1);
            int sNativeErrorCode = Integer.parseInt(sMatcher.group(2), 16);
            int sNodeId = Integer.parseInt(sMatcher.group(3));
            shard_log("[FAILOVER FOOT_PRINT ERROR_CODE NODE_ID] : {0}, {1}, {2} ",
                      new Object[] {sFootPrint, sNativeErrorCode, sNodeId} );
            if (isServerSideFailoverErrorCode(sNativeErrorCode))
            {
                switch (sNativeErrorCode)
                {
                    case ErrorDef.SHARD_LIBRARY_LINK_FAILURE :
                        FailoverAlignInfo sAlignInfo = new FailoverAlignInfo(true, aSQLState, sNativeErrorCode,
                                                                             aErrorMsg, sNodeId);
                        aErrorResult.addFailoverAlignInfo(sAlignInfo);
                        break;
                    case ErrorDef.SHARD_LIBRARY_FAILOVER_SUCCESS :
                        ShardFailOverSuccessException sFOException = new ShardFailOverSuccessException(
                                aErrorMsg, ErrorDef.FAILOVER_SUCCESS);
                        sFOException.initCause(sCause);
                        aExceptionList.add(sFOException);
                        break;
                    case ErrorDef.SHARD_LIBRARY_FAILOVER_IS_NOT_AVAILABLE :
                        ShardFailoverIsNotAvailableException sFailoverNotAvailableEx =
                                new ShardFailoverIsNotAvailableException(aErrorMsg,
                                                                         ErrorDef.SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE );
                        sFailoverNotAvailableEx.initCause(sCause);
                        aExceptionList.add(sFailoverNotAvailableEx);
                        break;
                }
            }
            else
            {
                // BUG-46790 서버사이드 failover 예외는 아니지만 foot print가 포함된 경우 SQLException 하나로 처리한다.
                sIsServerSideNormalErrorExist = true;
            }
        }
        /* BUG-46790 에러메세지에 failover foot print가 없거나 failover가 아닌 다른 sd 에러가 발생한 경우 일반
           SQLException으로 처리한다.  */
        if (!sMatchFounded || sIsServerSideNormalErrorExist)
        {
            aExceptionList.add(sCause);
        }
    }

    /**
     * Shard 관련 에러(SMN에러 or Align에러)를 처리한다.
     * @param aErrorResult 에러 결과 객체
     * @throws SQLException align data node 수행중 에러가 발생한 경우
     */
    public static void processShardError(AltibaseShardingConnection aMetaConn,
                                         CmErrorResult aErrorResult) throws SQLException
    {
        if (aMetaConn == null)
        {
            return;
        }
        List<SQLException> sFailoverExceptionList = new ArrayList<SQLException>();
        // BUG-46513 SMN Invalid 오류가 발생한 경우 SMN값과 needToDisconnect값을 셋팅한다.
        while (aErrorResult != null)
        {
            if (aErrorResult.isInvalidSMNError())
            {
                long sSMN = aErrorResult.getSMNOfDataNode();
                aMetaConn.setShardMetaNumberOfDataNode(sSMN);

                boolean sIsNeedToDisconnect = aErrorResult.isNeedToDisconnect();
                aMetaConn.setNeedToDisconnect(sIsNeedToDisconnect);
            }

            // BUG-46790 serverside failover에 따른 align이 필요한지 체크한다.
            AltibaseShardingFailover sShardFailover = aMetaConn.getShardFailover();
            for (FailoverAlignInfo sAlignInfo : aErrorResult.getFailoverAlignInfoList())
            {
                ShardNodeConfig sNodeConfig = aMetaConn.getShardNodeConfig();
                DataNode sNode = sNodeConfig.getNode(sAlignInfo.getNodeId());

                Connection sNodeConn = aMetaConn.getCachedConnections().get(sNode);
                if (sNodeConn == null)
                {
                   /* BUG-46790 아직 노드커넥션이 없는 경우에는 수동으로 alternate server와의 연결을 수립하고
                      서버에 notify를 보낸다. */
                    // BUGBUG : getNodeConnection 실패에 대한 처리가 없음.
                    AltibaseConnection sAlternateConn = (AltibaseConnection)aMetaConn.getNodeConnection(sNode, true);
                    sShardFailover.notifyFailover(sAlternateConn);
                    sFailoverExceptionList.add(new ShardFailOverSuccessException(sAlignInfo.getMessageText(),
                                                                                 ErrorDef.FAILOVER_SUCCESS,
                                                                                 sNode.getNodeName(),
                                                                                 sNode.getServerIp(),
                                                                                 sNode.getPortNo()));
                }
                else
                {
                    try
                    {
                        // BUGBUG : disconnection이 없는 것으로 보임... /nok 설계문서 내용과 다름.
                        sShardFailover.alignDataNodeConnection((AltibaseConnection)sNodeConn, sAlignInfo);
                    }
                    catch (ShardJdbcException aEx)
                    {
                        sFailoverExceptionList.add(aEx);
                    }
                }
            }
            aErrorResult = aErrorResult.getNextError();
        }

        throwSQLExceptionIfExists(sFailoverExceptionList);
    }

    private static boolean isServerSideFailoverErrorCode(int aNativeErrorCode)
    {
        switch (aNativeErrorCode)
        {
            case ErrorDef.SHARD_LIBRARY_LINK_FAILURE :
            case ErrorDef.SHARD_LIBRARY_FAILOVER_SUCCESS :
            case ErrorDef.SHARD_LIBRARY_FAILOVER_IS_NOT_AVAILABLE:
                return true;
        }

        return false;
    }

    public static void throwSQLExceptionIfExists(List<SQLException> aExceptionList) throws SQLException
    {
        if (aExceptionList.size() > 0)
        {
            SQLException sException = null;

            if (aExceptionList.size() > 1)
            {
                sException = makeMultiErrors(aExceptionList);
            }

            for (SQLException sEach : aExceptionList)
            {
                if (sException == null)
                {
                    sException = sEach;
                }
                else
                {
                    sException.setNextException(sEach);
                }
            }
            shard_log(Level.SEVERE, "(NODE EXECUTION EXCEPTION) ", sException);

            throw sException;
        }
    }

    static SQLException makeMultiErrors(List<SQLException> aExceptionList)
    {
        boolean isFirst = true;
        boolean isAllSame = true;
        int sErrorCode = 0;
        String sSqlState = "";
        StringBuilder sSb = new StringBuilder();
        
        for (SQLException sEach : aExceptionList)
        {
             if (isFirst)
             {
                 sErrorCode = sEach.getErrorCode();
                 sSqlState = sEach.getSQLState();
             }
             else
             {
                 if (isAllSame && (sErrorCode != sEach.getErrorCode()))
                 {
                     sErrorCode = ErrorDef.SHARD_MULTIPLE_ERRORS;
                     sSqlState = SQLStateMap.getSQLState(sErrorCode);
                     isAllSame = false;
                 }
                 sSb.append("\n");
             }
             
             sSb.append(sEach.getMessage());
             isFirst = false;
        }

        SQLException sException = new SQLException(sSb.toString(), sSqlState, sErrorCode);

        return sException;
    }
}
