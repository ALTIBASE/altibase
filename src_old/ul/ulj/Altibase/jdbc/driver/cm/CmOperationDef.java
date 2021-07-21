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

abstract public class CmOperationDef
{
    static final byte             DB_OP_MESSAGE                          = 0;
    static final byte             DB_OP_ERROR_RESULT                     = 1;
    static final byte             DB_OP_DISCONNECT                       = 6;
    static final byte             DB_OP_DISCONNECT_RESULT                = 7;
    static final byte             DB_OP_GET_PROPERTY                     = 8;
    static final byte             DB_OP_GET_PROPERTY_RESULT              = 9;
    static final byte             DB_OP_SET_PROPERTY_RESULT              = 11;
    static final byte             DB_OP_PREPARE                          = 12;  // 사용하지 않음. 대신 OPERATION_PREPARE_BY_CID 사용.
    static final byte             DB_OP_PREPARE_RESULT                   = 13;
    static final byte             DB_OP_GET_PLAN                         = 14;
    static final byte             DB_OP_GET_PLAN_RESULT                  = 15;
    static final byte             DB_OP_GET_COLUMN_INFO                  = 16;
    static final byte             DB_OP_GET_COLUMN_INFO_RESULT           = 17;
    static final byte             DB_OP_GET_COLUMN_INFO_LIST_RESULT      = 18;
    static final byte             DB_OP_GET_PARAM_INFO                   = 21;
    static final byte             DB_OP_GET_PARAM_INFO_RESULT            = 22;
    static final byte             DB_OP_SET_PARAM_INFO_LIST              = 25;
    static final byte             DB_OP_SET_PARAM_INFO_LIST_RESULT       = 26;
    static final byte             DB_OP_PARAM_DATA_IN                    = 27;
    static final byte             DB_OP_PARAM_DATA_OUT_LIST              = 29;
    static final byte             DB_OP_FETCH_MOVE                       = 34;
    static final byte             DB_OP_FETCH_MOVE_RESULT                = 35;
    static final byte             DB_OP_FETCH_BEGIN_RESULT               = 37;
    static final byte             DB_OP_FETCH_RESULT                     = 38;
    static final byte             DB_OP_FETCH_END_RESULT                 = 40;
    static final byte             DB_OP_FREE                             = 41;
    static final byte             DB_OP_FREE_RESULT                      = 42;
    static final byte             DB_OP_CANCEL                           = 43;  // 사용하지 않음. 대신 OPERATION_CANCEL_BY_CID 사용.
    static final byte             DB_OP_CANCEL_RESULT                    = 44;
    static final byte             DB_OP_TRANSACTION_RESULT               = 46;
    static final byte             DB_OP_LOB_GET_SIZE                     = 47;
    static final byte             DB_OP_LOB_GET_SIZE_RESULT              = 48;
    static final byte             DB_OP_LOB_GET                          = 49;
    static final byte             DB_OP_LOB_GET_RESULT                   = 50;
    static final byte             DB_OP_LOB_PUT_BEGIN                    = 51;
    static final byte             DB_OP_LOB_PUT_BEGIN_RESULT             = 52;
    static final byte             DB_OP_LOB_PUT                          = 53;
    static final byte             DB_OP_LOB_PUT_END                      = 54;
    static final byte             DB_OP_LOB_PUT_END_RESULT               = 55;
    static final byte             DB_OP_LOB_FREE                         = 56;
    static final byte             DB_OP_LOB_FREE_RESULT                  = 57;
    static final byte             DB_OP_LOB_FREE_ALL                     = 58;
    static final byte             DB_OP_LOB_FREE_ALL_RESULT              = 59;
    static final byte             DB_OP_XA_OPERATION                     = 60;
    static final byte             DB_OP_XA_XID_RESULT                    = 61;
    static final byte             DB_OP_XA_RESULT                        = 62;
    static final byte             DB_OP_XA_TRANSACTION                   = 63;
    static final byte             DB_OP_LOB_GET_BYTE_POS_CHAR_LEN        = 64;
    static final byte             DB_OP_LOB_GET_BYTE_POS_CHAR_LEN_RESULT = 65;
    static final byte             DB_OP_LOB_GET_CHAR_POS_CHAR_LEN        = 66;
    static final byte             DB_OP_LOB_BYTE_POS                     = 67;
    static final byte             DB_OP_LOB_BYTE_POS_RESULT              = 68;
    static final byte             DB_OP_LOB_CHAR_LENGTH                  = 69;
    static final byte             DB_OP_LOB_CHAR_LENGTH_RESULT           = 70;
    static final byte             DB_OP_PARAM_DATA_IN_RESULT             = 71;
    static final byte             DB_OP_HANDSHAKE                        = 72;
    static final byte             DB_OP_HANDSHAKE_RESULT                 = 73;
    static final byte             DB_OP_PREPARE_BY_CID                   = 74;
    static final byte             DB_OP_CANCEL_BY_CID                    = 75;
    static final byte             DB_OP_LOB_TRIM                         = 76;
    static final byte             DB_OP_LOB_TRIM_RESULT                  = 77;
    static final byte             DB_OP_CONNECT_EX                       = 78;  // BUG-38496
    static final byte             DB_OP_CONNECT_EX_RESULT                = 79;  // BUG-38496
    static final byte             DB_OP_FETCH_V2                         = 80;  // BUG-39463
    static final byte             DB_OP_SET_PROPERTY_V2                  = 81;  // BUG-41793
    static final byte             DB_OP_PARAM_DATA_IN_LIST_V2_RESULT     = 84;  // BUG-44572
    static final byte             DB_OP_SHARD_ANALYZE                    = 87;
    static final byte             DB_OP_SHARD_ANALYZE_RESULT             = 88;
    static final byte             DB_OP_SHARD_NODE_UPDATE_LIST           = 89;
    static final byte             DB_OP_SHARD_NODE_UPDATE_LIST_RESULT    = 90;
    static final byte             DB_OP_SHARD_NODE_GET_LIST              = 91;
    static final byte             DB_OP_SHARD_NODE_GET_LIST_RESULT       = 92;
    static final byte             DB_OP_SHARD_HANDSHAKE                  = 93;
    static final byte             DB_OP_SHARD_HANDSHAKE_RESULT           = 94;
    static final byte             DB_OP_SHARD_TRANSACTION_RESULT         = 96;

    static final byte             DB_OP_SHARD_STMT_PARTIAL_ROLLBACK      = 105;
    static final byte             DB_OP_SHARD_STMT_PARTIAL_ROLLBACK_RESULT = 106;
    static final byte             DB_OP_SHARD_NODE_REPORT                = 107;
    static final byte             DB_OP_SHARD_NODE_REPORT_RESULT         = 108;
    static final byte             DB_OP_ERROR_V3_RESULT                  = 109;  
    static final byte             DB_OP_CONNECT_V3                       = 110;  
    static final byte             DB_OP_CONNECT_V3_RESULT                = 111;  
    static final byte             DB_OP_SET_PROPERTY_V3_RESULT           = 113;  
    static final byte             DB_OP_PARAM_DATA_IN_LIST_V3_RESULT     = 115;  
    static final byte             DB_OP_SHARD_TRANSACTION_V3_RESULT      = 119;  
    static final byte             DB_OP_SHARD_PREPARE_V3                 = 120;  
    static final byte             DB_OP_SHARD_PREPARE_V3_RESULT          = 121;  
    static final byte             DB_OP_SHARD_END_PENDING_TX_V3          = 122;  
    static final byte             DB_OP_SHARD_END_PENDING_TX_V3_RESULT   = 123;  
    static final byte             DB_OP_LOB_GET_SIZE_V3                  = 124;
    static final byte             DB_OP_LOB_GET_SIZE_V3_RESULT           = 125;
    static final byte             DB_OP_FETCH_V3                         = 126;  
    static final byte             DB_OP_SHARD_REBUILD_NOTI_V3            = 127;  
    static final byte             DB_OP_PREPARE_V3                       = (byte)128;  // 사용하지 않음. 대신 OPERATION_PREPARE_BY_CID_V3 사용.
    static final byte             DB_OP_PREPARE_BY_CID_V3                = (byte)129;
    static final byte             DB_OP_PREPARE_V3_RESULT                = (byte)130;

    static final byte             DB_OP_COUNT                            = (byte)131;  // the number of the operations

    // BUG-46513 shardjdbc 관련클래스에서 참조하기 위해 아래 속성들을 public으로 변경
    public static final byte      DB_OP_SET_PROPERTY                     = 10;
    public static final byte      DB_OP_TRANSACTION                      = 45;
    public static final byte      DB_OP_PARAM_DATA_IN_LIST_V2            = 83;  // BUG-44572
    public static final byte      DB_OP_EXECUTE_V2                       = 85;  // BUG-44572
    public static final byte      DB_OP_EXECUTE_V2_RESULT                = 86;  // BUG-44572
    public static final byte      DB_OP_SHARD_TRANSACTION                = 95;

    public static final byte      DB_OP_SET_PROPERTY_V3                  = 112;
    public static final byte      DB_OP_PARAM_DATA_IN_LIST_V3            = 114;  
    public static final byte      DB_OP_EXECUTE_V3                       = 116;  
    public static final byte      DB_OP_EXECUTE_V3_RESULT                = 117;  
    public static final byte      DB_OP_SHARD_TRANSACTION_V3             = 118;

    private static final String[] DB_OP_NAMES = {
        "DB_OP_MESSAGE",
        "DB_OP_ERROR_RESULT",
        "DB_OP_GET_ERROR_INFO",
        "DB_OP_GET_ERROR_INFO_RESULT",
        "DB_OP_CONNECT",
        "DB_OP_CONNECT_RESULT",
        "DB_OP_DISCONNECT",
        "DB_OP_DISCONNECT_RESULT",
        "DB_OP_GET_PROPERTY",
        "DB_OP_GET_PROPERTY_RESULT",
        "DB_OP_SET_PROPERTY",
        "DB_OP_SET_PROPERTY_RESULT",
        "DB_OP_PREPARE",
        "DB_OP_PREPARE_RESULT",
        "DB_OP_GET_PLAN",
        "DB_OP_GET_PLAN_RESULT",
        "DB_OP_GET_COLUMN_INFO",
        "DB_OP_GET_COLUMN_INFO_RESULT",
        "DB_OP_GET_COLUMN_INFO_LIST_RESULT",
        "DB_OP_SET_COLUMN_INFO",
        "DB_OP_SET_COLUMN_INFO_RESULT",
        "DB_OP_GET_PARAM_INFO",
        "DB_OP_GET_PARAM_INFO_RESULT",
        "DB_OP_SET_PARAM_INFO",
        "DB_OP_SET_PARAM_INFO_RESULT",
        "DB_OP_SET_PARAM_INFO_LIST",
        "DB_OP_SET_PARAM_INFO_LIST_RESULT",
        "DB_OP_PARAM_DATA_IN",
        "DB_OP_PARAM_DATA_OUT",
        "DB_OP_PARAM_DATA_OUT_LIST",
        "DB_OP_PARAM_DATA_IN_LIST",
        "DB_OP_PARAM_DATA_IN_LIST_RESULT",
        "DB_OP_EXECUTE",
        "DB_OP_EXECUTE_RESULT",
        "DB_OP_FETCH_MOVE",
        "DB_OP_FETCH_MOVE_RESULT",
        "DB_OP_FETCH",
        "DB_OP_FETCH_BEGIN_RESULT",
        "DB_OP_FETCH_RESULT",
        "DB_OP_FETCH_LIST_RESULT",
        "DB_OP_FETCH_END_RESULT",
        "DB_OP_FREE",
        "DB_OP_FREE_RESULT",
        "DB_OP_CANCEL",
        "DB_OP_CANCEL_RESULT",
        "DB_OP_TRANSACTION",
        "DB_OP_TRANSACTION_RESULT",
        "DB_OP_LOB_GET_SIZE",
        "DB_OP_LOB_GET_SIZE_RESULT",
        "DB_OP_LOB_GET",
        "DB_OP_LOB_GET_RESULT",
        "DB_OP_LOB_PUT_BEGIN",
        "DB_OP_LOB_PUT_BEGIN_RESULT",
        "DB_OP_LOB_PUT",
        "DB_OP_LOB_PUT_END",
        "DB_OP_LOB_PUT_END_RESULT",
        "DB_OP_LOB_FREE",
        "DB_OP_LOB_FREE_RESULT",
        "DB_OP_LOB_FREE_ALL",
        "DB_OP_LOB_FREE_ALL_RESULT",
        "DB_OP_XA_OPERATION",
        "DB_OP_XA_XID_RESULT",
        "DB_OP_XA_RESULT",
        "DB_OP_XA_TRANSACTION",
        "DB_OP_LOB_GET_BYTE_POS_CHAR_LEN",
        "DB_OP_LOB_GET_BYTE_POS_CHAR_LEN_RESULT",
        "DB_OP_LOB_GET_CHAR_POS_CHAR_LEN",
        "DB_OP_LOB_BYTE_POS",
        "DB_OP_LOB_BYTE_POS_RESULT",
        "DB_OP_LOB_CHAR_LENGTH",
        "DB_OP_LOB_CHAR_LENGTH_RESULT",
        "DB_OP_PARAM_DATA_IN_RESULT",
        "DB_OP_HANDSHAKE",
        "DB_OP_HANDSHAKE_RESULT",
        "DB_OP_PREPARE_BY_CID",
        "DB_OP_CANCEL_BY_CID",
        "DB_OP_LOB_TRIM",
        "DB_OP_LOB_TRIM_RESULT", 
        "DB_OP_CONNECT_EX",                              // BUG-38496
        "DB_OP_CONNECT_EX_RESULT",                       // BUG-38496
        "DB_OP_FETCH_V2",                                // BUG-39463
        "DB_OP_SET_PROPERTY_V2",                         // BUG-41793
        "DB_OP_IPCDA_LAST_OP_ENDED",                     // PROJ-2616
        "DB_OP_PARAM_DATA_IN_LIST_V2",                   // BUG-44572
        "DB_OP_PARAM_DATA_IN_LIST_V2_RESULT",            // BUG-44572
        "DB_OP_EXECUTE_V2",                              // BUG-44572
        "DB_OP_EXECUTE_V2_RESULT",                       // BUG-44572
        "DB_OP_SHARD_ANALYZE",
        "DB_OP_SHARD_ANALYZE_RESULT",
        "DB_OP_SHARD_NODE_UPDATE_LIST",
        "DB_OP_SHARD_NODE_UPDATE_LIST_RESULT",
        "DB_OP_SHARD_NODE_GET_LIST",
        "DB_OP_SHARD_NODE_GET_LIST_RESULT",
        "DB_OP_SHARD_HANDSHAKE",
        "DB_OP_SHARD_HANDSHAKE_RESULT",
        "DB_OP_SHARD_TRANSACTION",
        "DB_OP_SHARD_TRANSACTION_RESULT",
        "DB_OP_SHARD_PREPARE",
        "DB_OP_SHARD_PREPARE_RESULT",
        "DB_OP_SHARD_ENDPENDING_TX",
        "DB_OP_SHARD_ENDPENDING_TX_RESULT",
        "DB_OP_SET_SAVEPOINT",
        "DB_OP_SET_SAVEPOINT_RESULT",
        "DB_OP_ROLLBACK_TO_SAVEPOINT",
        "DB_OP_ROLLBACK_TO_SAVEPOINT_RESULT",
        "DB_OP_SHARD_STMT_PARTIAL_ROLLBACK",
        "DB_OP_SHARD_STMT_PARTIAL_ROLLBACK_RESULT",
        "DB_OP_SHARD_NODE_REPORT",
        "DB_OP_SHARD_NODE_REPORT_RESULT",
        "DB_OP_ERROR_V3_RESULT",
        "DB_OP_CONNECT_V3",
        "DB_OP_CONNECT_V3_RESULT",
        "DB_OP_SET_PROPERTY_V3",
        "DB_OP_SET_PROPERTY_V3_RESULT",
        "DB_OP_PARAM_DATA_IN_LIST_V3",
        "DB_OP_PARAM_DATA_IN_LIST_V3_RESULT",
        "DB_OP_EXECUTE_V3",
        "DB_OP_EXECUTE_V3_RESULT",
        "DB_OP_SHARD_TRANSACTION_V3",
        "DB_OP_SHARD_TRANSACTION_V3_RESULT",
        "DB_OP_SHARD_PREPARE_V3",
        "DB_OP_SHARD_PREPARE_V3_RESULT",
        "DB_OP_SHARD_END_PENDING_TX_V3",
        "DB_OP_SHARD_END_PENDING_TX_V3_RESULT",
        "DB_OP_LOB_GET_SIZE_V3",
        "DB_OP_LOB_GET_SIZE_V3_RESULT",
        "DB_OP_FETCH_V3",
        "DB_OP_SHARD_REBUILD_NOTI_V3",
        "DB_OP_PREPARE_V3",
        "DB_OP_PREPARE_BY_CID_V3",
        "DB_OP_PREPARE_V3_RESULT"
    };

    public static final short     CONNECT_MODE_NORMAL                    = 0;
    public static final short     CONNECT_MODE_SYSDBA                    = 1;

    static final byte             TRANSACTION_OP_COMMIT                  = 1;
    static final byte             TRANSACTION_OP_ROLLBACK                = 2;

    static final byte             PREPARE_MODE_EXEC_MASK                 = 0x01;
    static final byte             PREPARE_MODE_EXEC_PREPARE              = 0x00;
    static final byte             PREPARE_MODE_EXEC_DIRECT               = 0x01;
    static final byte             PREPARE_MODE_HOLD_MASK                 = 0x02;
    static final byte             PREPARE_MODE_HOLD_ON                   = 0x00;
    static final byte             PREPARE_MODE_HOLD_OFF                  = 0x02;
    static final byte             PREPARE_MODE_KEYSET_MASK               = 0x04;
    static final byte             PREPARE_MODE_KEYSET_OFF                = 0x00;
    static final byte             PREPARE_MODE_KEYSET_ON                 = 0x04;

    static final byte             COLUMN_INFO_FLAG_NULLABLE              = 0x01;
    static final byte             COLUMN_INFO_FLAG_UPDATABLE             = 0x02;

    static final byte             EXECUTION_MODE_NORMAL                  = 1;
    static final byte             EXECUTION_MODE_ARRAY                   = 2;
    static final byte             EXECUTION_MODE_BEGIN_ARRAY             = 3;
    static final byte             EXECUTION_MODE_END_ARRAY               = 4;
    static final byte             EXECUTION_MODE_ATOMIC                  = 5;
    static final byte             EXECUTION_MODE_BEGIN_ATOMIC            = 6;
    static final byte             EXECUTION_MODE_END_ATOMIC              = 7;

    static final short            EXECUTION_ARRAY_INDEX_NONE             = 0;

    static final short            FREE_ALL_RESULTSET                     = (short)0xFFFF;
    static final byte             FREE_MODE_CLOSE                        = 0;
    static final byte             FREE_MODE_DROP                         = 1;

    static final byte             XA_OP_OPEN                             = 1;
    static final byte             XA_OP_CLOSE                            = 2;
    static final byte             XA_OP_START                            = 3;
    static final byte             XA_OP_END                              = 4;
    static final byte             XA_OP_PREPARE                          = 5;
    static final byte             XA_OP_COMMIT                           = 6;
    static final byte             XA_OP_ROLLBACK                         = 7;
    static final byte             XA_OP_FORGET                           = 8;
    static final byte             XA_OP_RECOVER                          = 9;

    public static final int       WRITE_STRING_MODE_DB                   = 1;
    public static final int       WRITE_STRING_MODE_NCHAR                = 2;
    public static final int       WRITE_STRING_MODE_NLITERAL             = 3;
    
    public static final int       SHARD_NODE_REPORT_TYPE_CONNECTION         = 1;
    public static final int       SHARD_NODE_REPORT_TYPE_TRANSACTION_BROKEN = 2;

    protected CmOperationDef()
    {
    }

    /**
     * aOp에 해당하는 Operation 이름을 얻는다.
     * 
     * @param aOp Operation Code
     * @return Operation 이름
     */
    public static String getOperationName(int aOp)
    {
        return getOperationName(aOp, false);
    }

    /**
     * aOp에 해당하는 Operation 이름을 얻는다.
     * <p>
     * aWithID를 true로 주면 'OPERATION_NAME(99)'와 같이 ID값이 포함된 형태로 준다.
     * 
     * @param aOp Operation Code
     * @param aWithID ID값이 포함된 형태로 얻을것인지 여부
     * @return Operation 이름
     */
    public static String getOperationName(int aOp, boolean aWithID)
    {
        String sName;
        if (aOp < 0 || DB_OP_NAMES.length < aOp)
        {
            sName = "INVALID_OPERATION";
        }
        else
        {
            sName = DB_OP_NAMES[aOp];
        }
        if (aWithID == true)
        {
            sName += "(" + aOp + ")";
        }
        return sName;
    }
}
