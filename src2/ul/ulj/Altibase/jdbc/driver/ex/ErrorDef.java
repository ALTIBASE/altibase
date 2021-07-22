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

package Altibase.jdbc.driver.ex;

import javax.transaction.xa.XAException;

public class ErrorDef
{
    // #region 서버에서 받는 에러 코드

    /* 서버에서 받는 에러 코드도 여기 놓아두자.
     * 이 값은 mmErrorCode.ih 등에 있는걸 그대로 가져온거다. */
    public static final int       IGNORE_NO_ERROR                                   = 0x42000;
    public static final int       IGNORE_NO_COLUMN                                  = 0x420A4;
    public static final int       IGNORE_NO_CURSOR                                  = 0x420A7;
    public static final int       IGNORE_NO_PARAMETER                               = 0x420A8;
    public static final int       QUERY_TIMEOUT                                     = 0x01044;
    public static final int       QUERY_CANCELED_BY_CLIENT                          = 0x0106A;
    public static final int       INVALID_LOB_START_OFFSET                          = 0x110CD;
    public static final int       INVALID_DATA_TYPE_LENGTH                          = 0x21069;
    public static final int       TABLE_NOT_FOUND                                   = 0x31031;
    public static final int       PROCEDURE_OR_FUNCTION_NOT_FOUND                   = 0x31129;
    public static final int       SEQUENCE_NOT_FOUND                                = 0x31013;
    public static final int       COLUMN_NOT_FOUND                                  = 0x31058;
    // BUG-32092 Failover should be performed although ADMIN_MODE=1
    public static final int       ADMIN_MODE_ERROR                                  = 0x4106E;
    // PROJ-1789 Updatable Scrollable Cursor
    public static final int       CANNOT_SELECT_PROWID                              = 0x31384;
    public static final int       PROWID_NOT_SUPPORTED                              = 0x31385;
    public static final int       INVALID_STATEMENT_PROCESSING_REQUEST              = 0x4103A;
    public static final int       DB_NOT_FOUND                                      = 0x410D8;
    public static final int       MATERIALIZED_VIEW_NOT_FOUND                       = 0x31373;
    public static final int       SYNONYM_NOT_FOUND                                 = 0x31207;
    public static final int       UNDEFINED_USER_NAME                               = 0x3103C;
    public static final int       SQL_SYNTAX_ERROR                                  = 0x31001;
    public static final int       FETCH_OUT_OF_SEQUENCE                             = 0x410D2;
    public static final int       FAILURE_TO_FIND_STATEMENT                         = 0x41098;
    public static final int       INVALID_LOB_RANGE                                 = 0x4109E;
    public static final int       SHARD_META_NUMBER_INVALID                         = 0x410FA;

    // BUG-46790 serverside failover 관련 에러코드
    public static final int       SHARD_LIBRARY_LINK_FAILURE                        = 0xE1132;
    public static final int       SHARD_LIBRARY_FAILOVER_SUCCESS                    = 0xE1139;
    public static final int       SHARD_LIBRARY_FAILOVER_IS_NOT_AVAILABLE           = 0xE113B;

    // BUG-48762 TASK-7218 Handling Multiple Errors - 반영
    // BUG-48921 [R2-FUNC2] sd, ul 의 Multiple Error 코드 단일화 
    public static final int       SHARD_MULTIPLE_ERRORS                             = 0xE113F;

    // BUG-38496 Notify users when their password expiry date is approaching.
    public static final int       PASSWORD_GRACE_PERIOD                             = 0x420E1;

    /* BUG-41908 Add processing the error 'mmERR_IGNORE_UNSUPPORTED_PROPERTY' in JDBC
     * Server에서 지원하지 않는 Property에 대한 요청이 왔을 경우 Server는 해당 에러를 리턴한다. */
    public static final int       UNSUPPORTED_PROPERTY                              = 0x420DB;

    // PROJ-2733 For stmt retry
    public static final int       STATEMENT_TOO_OLD                                 = 0x111B7;

    // #endregion

    // #region JDBC 전용 에러 코드

    /* JDBC에서의 에러 코드는 0x51A01 ~ 0x51AFF를 사용해야 한다. */

    private static final int      FIRST_ERROR_CODE                                  = 0x51A01;

    public static final int       COMMUNICATION_ERROR                               = 0x51A01;
    public static final int       INVALID_DATA_CONVERSION                           = 0x51A02;
    public static final int       INVALID_TYPE_CONVERSION                           = 0x51A03;
    public static final int       INVALID_PACKET_HEADER_VERSION                     = 0x51A04;
//  public static final int       INVALID_PACKET_NEXT_HEADER_TYPE                   = 0x51A05;
    public static final int       INVALID_PACKET_SEQUENCE_NUMBER                    = 0x51A06;
//  public static final int       INVALID_PACKET_SERIAL_NUMBER                      = 0x51A07;
//  public static final int       INVALID_PACKET_MODULE_ID                          = 0x51A08;
//  public static final int       INVALID_PACKET_MODULE_VERSION                     = 0x51A09;
    public static final int       INVALID_OPERATION_PROTOCOL                        = 0x51A0A;
    public static final int       DYNAMIC_ARRAY_OVERFLOW                            = 0x51A0B;
    public static final int       DYNAMIC_ARRAY_UNDERFLOW                           = 0x51A0C;
    public static final int       INTERNAL_ASSERTION                                = 0x51A0D;
    public static final int       INVALID_CONNECTION_URL                            = 0x51A0E;
    public static final int       NO_RESULTSET                                      = 0x51A0F;
    public static final int       MULTIPLE_RESULTSET_RETURNED                       = 0x51A10;
    public static final int       INVALID_COLUMN_NAME                               = 0x51A11;
    public static final int       NO_DB_NAME_SPECIFIED                              = 0x51A12;
    public static final int       NO_USER_ID_SPECIFIED                              = 0x51A13;
    public static final int       NO_PASSWORD_SPECIFIED                             = 0x51A14;
    public static final int       NO_TABLE_NAME_SPECIFIED                           = 0x51A71;
//  public static final int       HANDSHAKE_FAIL                                    = 0x51A15;
    public static final int       CURSOR_AT_BEFORE_FIRST                            = 0x51A16;
    public static final int       CLOSED_CONNECTION                                 = 0x51A17;
    public static final int       TXI_LEVEL_CANNOT_BE_MODIFIED_FOR_AUTOCOMMIT       = 0x51A18;
    public static final int       INVALID_ARGUMENT                                  = 0x51A19;
    public static final int       CANNOT_RENAME_DB_NAME                             = 0x51A1A;
    public static final int       READONLY_CONNECTION_NOT_SUPPORTED                 = 0x51A1B;
    public static final int       INVALID_PROPERTY_VALUE                            = 0x51A1C;
    public static final int       CLOSED_STATEMENT                                  = 0x51A1D;
    public static final int       NO_BATCH_JOB                                      = 0x51A1E;
    public static final int       SOME_BATCH_JOB                                    = 0x51A1F;
    public static final int       SOME_RESULTSET_RETURNED                           = 0x51A20;
    public static final int       NOT_SUPPORTED_OPERATION_ON_FORWARD_ONLY           = 0x51A21;
    public static final int       NOT_SUPPORTED_OPERATION_ON_READ_ONLY              = 0x51A22;
    public static final int       UNSUPPORTED_FEATURE                               = 0x51A23;
    public static final int       WAS_NULL_CALLED_BEFORE_CALLING_GETXXX             = 0x51A24;
    public static final int       TOO_LARGE_FETCH_SIZE                              = 0x51A25;
    public static final int       CLOSED_RESULTSET                                  = 0x51A26;
    public static final int       RESULTSET_CREATED_BY_INTERNAL_STATEMENT           = 0x51A27;
    public static final int       INVALID_COLUMN_INDEX                              = 0x51A28;
    public static final int       CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL     = 0x51A29;
    public static final int       NOT_SUPPORTED_OPERATION_ON_SCROLL_INSENSITIVE     = 0x51A2A;
    public static final int       INVALID_BIT_CHARACTER                             = 0x51A2B;
    public static final int       NULL_SQL_STRING                                   = 0x51A2C;
//  public static final int       CANNOT_EXECUTE_THE_QUERY_DURING_BATCH_EXECUTION   = 0x51A2D;
    public static final int       CANNOT_BIND_THE_DATA_TYPE_DURING_ADDING_BATCH_JOB = 0x51A2E;
    public static final int       NEED_MORE_PARAMETER                               = 0x51A2F;
//  public static final int       NOT_IN_TYPE_PARAMETER                             = 0x51A30;
    public static final int       NOT_OUT_TYPE_PARAMETER                            = 0x51A31;
    public static final int       CURSOR_AT_AFTER_LAST                              = 0x51A32;
    public static final int       STREAM_ALREADY_CLOSED                             = 0x51A33;
    public static final int       CURSOR_AT_INSERTING_ROW                           = 0x51A34;
    public static final int       CURSOR_NOT_AT_INSERTING_ROW                       = 0x51A35;
    public static final int       XA_OPEN_FAIL                                      = 0x51A37;
    public static final int       XA_CLOSE_FAIL                                     = 0x51A38;
    public static final int       RESPONSE_TIMEOUT                                  = 0x51A39;
    public static final int       UNSUPPORTED_TYPE_CONVERSION                       = 0x51A3A;
    public static final int       FAILOVER_SUCCESS                                  = 0x51A3B;
    public static final int       INVALID_FORMAT_OF_ALTERNATE_SERVERS               = 0x51A3C;
    public static final int       UNKNOWN_HOST                                      = 0x51A3D;
    public static final int       CLIENT_UNABLE_ESTABLISH_CONNECTION                = 0x51A3E;
    public static final int       XA_RECOVER_FAIL                                   = 0x51A3F;
//  public static final int       INSUFFICIENT_BUFFER_SIZE                          = 0x51A40;
//  public static final int       INVALID_PACKET_SIZE                               = 0x51A41;
    public static final int       THREAD_INTERRUPTED                                = 0x51A42;
    public static final int       EXCEED_PRIMITIVE_DATA_SIZE                        = 0x51A43;
    public static final int       EXPLAIN_PLAN_TURNED_OFF                           = 0x51A44;
    public static final int       STATEMENT_IS_NOT_PREPARED                         = 0x51A45;
    public static final int       STMT_ID_MISMATCH                                  = 0x51A46;
    public static final int       PACKET_TWISTED                                    = 0x51A47;
    public static final int       INVALID_QUERY_STRING                              = 0x51A48;
    public static final int       DOES_NOT_MATCH_COLUMN_LIST                        = 0x51A49;
    public static final int       OPTION_VALUE_CHANGED                              = 0x51A50;
    public static final int       CURSOR_OPERATION_CONFLICT                         = 0x51A51;
    public static final int       INVALID_BIND_COLUMN                               = 0x51A52;
    public static final int       INVALID_BATCH_OPERATION_WITH_SELECT               = 0x51A53;
    public static final int       TRANSACTION_ALREADY_ACTIVE                        = 0x51A54;
    public static final int       BATCH_UPDATE_EXCEPTION_OCCURRED                   = 0x51A55;
    public static final int       TOO_MANY_BATCH_JOBS                               = 0x51A56;
    public static final int       TOO_MANY_STATEMENTS                               = 0x51A57;
    public static final int       IOEXCEPTION_FROM_INPUTSTREAM                      = 0x51A58;
    public static final int       INVALID_METHOD_INVOCATION                         = 0x51A59;
    public static final int       INCONSISTANCE_DATA_LENGTH                         = 0x51A5A;
    public static final int       STATEMENT_NOT_YET_EXECUTED                        = 0x51A5B;
    public static final int       CANNOT_SET_SAVEPOINT_AT_AUTO_COMMIT_MODE          = 0x51A5C;
    public static final int       INVALID_SAVEPOINT_NAME                            = 0x51A5D;
    public static final int       INVALID_SAVEPOINT                                 = 0x51A5E;
    public static final int       NOT_SUPPORTED_OPERATION_ON_NAMED_SAVEPOINT        = 0x51A5F;
    public static final int       NOT_SUPPORTED_OPERATION_ON_UNNAMED_SAVEPOINT      = 0x51A60;
    public static final int       NO_CURRENT_ROW                                    = 0x51A61;
    public static final int       NO_AVAILABLE_DATASOURCE                           = 0x51A62;
    public static final int       EMPTY_RESULTSET                                   = 0x51A63;
    public static final int       INVALID_PROPERTY_ID                               = 0x51A64;
    public static final int       INVALID_STATE                                     = 0x51A65;
    public static final int       MORE_DATA_NEEDED                                  = 0x51A66;
    public static final int       INVALID_TYPE                                      = 0x51A67;
    public static final int       INTERNAL_BUFFER_OVERFLOW                          = 0x51A68;
    public static final int       INTERNAL_DATA_INTEGRITY_BROKEN                    = 0x51A69;
    public static final int       ILLEGAL_ACCESS                                    = 0x51A70;
    public static final int       ALREADY_CONVERTED                                 = 0x51A73;
    public static final int       INVALID_HEX_STRING_LENGTH                         = 0x51A74;
    public static final int       INVALID_HEX_STRING_ELEMENT                        = 0x51A75;
    public static final int       INVALID_NIBBLE_ARRAY_ELEMENT                      = 0x51A76;
    public static final int       COLUMN_READER_INITIALIZATION_SKIPPED              = 0x51A77;
    public static final int       VALUE_LENGTH_EXCEEDS                              = 0x51A78;
    public static final int       PASSWORD_EXPIRATION_DATE_IS_COMING                = 0x51A79;

    /* PROJ-2474 SSL 관련 에러코드 */
    public static final int       UNSUPPORTED_KEYSTORE_ALGORITHM                    = 0x51A7A;
    public static final int       CAN_NOT_CREATE_KEYSTORE_INSTANCE                  = 0x51A7B;
    public static final int       CAN_NOT_LOAD_KEYSTORE                             = 0x51A7C;
    public static final int       INVALID_KEYSTORE_URL                              = 0x51A7D;
    public static final int       KEY_MANAGEMENT_EXCEPTION_OCCURRED                 = 0x51A7E;
    public static final int       CAN_NOT_RETREIVE_KEY_FROM_KEYSTORE                = 0x51A7F;
    public static final int       CAN_NOT_OPEN_KEYSTORE                             = 0x51A80;
    public static final int       DEFAULT_ALGORITHM_DEFINITION_INVALID              = 0x51A81;

    /* BUG-41908 Add processing the error 'mmERR_IGNORE_UNSUPPORTED_PROPERTY' in JDBC
      * 반드시 필요한 Property를 Server에서 지원하지 못할 경우 해당 에러를 리턴한다. 또한 접속도 해제한다. */
    public static final int       NOT_SUPPORTED_MANDATORY_PROPERTY                  = 0x51A82;

    public static final int       OPENED_CONNECTION                                 = 0x51A83;

    // PROJ-2681
    public static final int       UNKNOWN_CONNTYPE                                  = 0x51A84;

    /* PROJ-2690 sharding관련 에러코드 */
    public static final int       SHARD_INVALID_TEST_MARK                           = 0x51A85;
    public static final int       SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE              = 0x51A86;
    public static final int       SHARD_NODE_NOT_FOUNDED                            = 0x51A87;
    // BUG-47314 샤드키 파라메터가 셋팅되지 않았을때 발생하는 에러
    public static final int       SHARD_BIND_PARAMETER_MISSING                      = 0x51A88;
    public static final int       SHARD_MULTINODE_TRANSACTION_REQUIRED              = 0x51A89;
    public static final int       SHARD_INVALID_NODE_TOUCH                          = 0x51A8A;
    public static final int       SHARD_SERVERSIDE_SINGLE_NODE_TOUCH_ERROR          = 0x51A8B;
    public static final int       SHARD_JDBC_METHOD_INVOKE_ERROR                    = 0x51A8C;
    public static final int       SHARD_SPLIT_METHOD_NOT_SUPPORTED                  = 0x51A8D;
    public static final int       SHARD_RANGE_NOT_FOUNDED                           = 0x51A8E;
    public static final int       SHARD_NO_NODES                                    = 0x51A8F;
    public static final int       SHARD_SMN_OPERATION_FAILED                        = 0x51A90;
    public static final int       SHARD_PIN_CHANGED                                 = 0x51A91;
    public static final int       SHARD_MULTIPLE_LOB_OPERATION_NOT_SUPPORTED        = 0x51A92;
    // BUG-47228 shard value encoding 실패시 발생하는 에러
    public static final int       SHARD_CHARSET_ENCODE_ERROR                        = 0x51A93;
    public static final int       EXECUTOR_CANNOT_BE_NULL                           = 0x51A94;
    public static final int       TYPE_PARAMETER_CANNOT_BE_NULL                     = 0x51A95;
    public static final int       TYPE_CONVERSION_NOT_SUPPORTED                     = 0x51A96;
    public static final int       CANNOT_BE_UNWRAPPED                               = 0x51A97;
    public static final int       SHARD_NEED_ROLLBACK                               = 0x51A98;
    public static final int       SHARD_INTERNAL_ERROR                              = 0x51A99;
    private static final int      LAST_ERROR_CODE                                   = 0x51A99;

    // #endregion

    private static final String[] ERROR_MAP_MESSAGE  = new String[LAST_ERROR_CODE - FIRST_ERROR_CODE + 1];
    private static final String[] ERROR_MAP_SQLSTATE = new String[LAST_ERROR_CODE - FIRST_ERROR_CODE + 1];

    static
    {
        register(COMMUNICATION_ERROR                               , "08S01" , "Communication link failure: %s" );
        register(INVALID_DATA_CONVERSION                           , "42001" , "Data conversion failure: Cannot convert the value <%s> to the SQL %s data type." );
        register(INVALID_TYPE_CONVERSION                           , "42001" , "Type mismatch: Cannot convert the column data type from SQL %s to %s." );
        register(INVALID_PACKET_HEADER_VERSION                     , "08P01" , "Invalid packet header version: <%s> expected, but <%s>" );
        register(INVALID_PACKET_SEQUENCE_NUMBER                    , "08P03" , "Invalid packet sequence number: <%s> expected, but <%s>" );
        register(INVALID_OPERATION_PROTOCOL                        , "08P07" , "Invalid operation protocol: %s" );
        register(DYNAMIC_ARRAY_OVERFLOW                            , "JID01" , "Dynamic array cursor overflow, chunk index=%s" );
        register(DYNAMIC_ARRAY_UNDERFLOW                           , "JID02" , "Dynamic array cursor underflow, chunk index=%s" );
        register(INTERNAL_ASSERTION                                , "JI000" , "Internal assertion" );
        register(INVALID_CONNECTION_URL                            , "08U01" , "Invalid ALTIBASE connection URL: %s" );
        register(NO_RESULTSET                                      , "02001" , "No results were returned by the query: %s" );
        register(MULTIPLE_RESULTSET_RETURNED                       , "0100D" , "The statement returned multiple ResultSets: %s" );
        register(INVALID_COLUMN_NAME                               , "3F000" , "Invalid column name: %s" );
        register(NO_DB_NAME_SPECIFIED                              , "01S00" , "No database name specified." );
        register(NO_USER_ID_SPECIFIED                              , "01S00" , "No user id specified." );
        register(NO_PASSWORD_SPECIFIED                             , "01S00" , "No password specified." );
        register(NO_TABLE_NAME_SPECIFIED                           , "01S00" , "No table name specified." );
        register(CURSOR_AT_BEFORE_FIRST                            , "HY109" , "Invalid cursor position: Cursor is at before the start" );
        register(CLOSED_CONNECTION                                 , "08006" , "Connection already closed." );
        register(OPENED_CONNECTION                                 , "08006" , "Connection is already open." );
        register(TXI_LEVEL_CANNOT_BE_MODIFIED_FOR_AUTOCOMMIT       , "HY011" , "The transaction isolation level cannot be modified for autocommit mode." );
        register(INVALID_ARGUMENT                                  , "22023" , "Invalid argument: %s: <%s> expected, but <%s>" );
        register(CANNOT_RENAME_DB_NAME                             , "0AC01" , "DB name cannot be renamed." );
        register(READONLY_CONNECTION_NOT_SUPPORTED                 , "0AC02" , "setReadOnly: Read-only connections are not supported." );
        register(INVALID_PROPERTY_VALUE                            , "01S00" , "Invalid property value: <%s> expected, but <%s>" );
        register(CLOSED_STATEMENT                                  , "01C01" , "Statement already closed." );
        register(NO_BATCH_JOB                                      , "01B01" , "executeBatch: no batch job to execute." ); // oracle은 예외를 발생하지 않음?
        register(SOME_BATCH_JOB                                    , "01B02" , "%s: Cannot execute the method because some batch jobs were added." );
        register(SOME_RESULTSET_RETURNED                           , "07R01" , "The given SQL statement returns some ResultSet" );
        register(NOT_SUPPORTED_OPERATION_ON_FORWARD_ONLY           , "0AT01" , "Not supported operation on the forward-only ResultSet." );
        register(NOT_SUPPORTED_OPERATION_ON_READ_ONLY              , "0AT02" , "Not supported operation on the read-only ResultSet." );
        register(UNSUPPORTED_FEATURE                               , "0A000" , "Unsupported feature: %s" );
        register(WAS_NULL_CALLED_BEFORE_CALLING_GETXXX             , "01R01" , "wasNull() was called without a preceding call to get a column." );
        register(TOO_LARGE_FETCH_SIZE                              , "01R02" , "Option value changed: Using maximum fetch size %s for %s exceeding the limit." );
        register(CLOSED_RESULTSET                                  , "01C02" , "ResultSet already closed." );
        register(RESULTSET_CREATED_BY_INTERNAL_STATEMENT           , "JID03" , "ResultSet was created by an internal SQL statement" );
        register(INVALID_COLUMN_INDEX                              , "42S22" , "Invalid column index: <%s> expected, but <%s>" );
        register(CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL     , "0AV01" , "%s: Cannot call the PrepareStatement method using an SQL query string" );
        register(NOT_SUPPORTED_OPERATION_ON_SCROLL_INSENSITIVE     , "0AV01" , "Not supported operation on the scroll-insensitive ResultSet." );
        register(INVALID_BIT_CHARACTER                             , "01V01" , "Invalid bit character: <'0' | '1'> expected, but <%s>" );
        register(NULL_SQL_STRING                                   , "22004" , "The SQL statement to be executed is empty or null." );
        register(CANNOT_BIND_THE_DATA_TYPE_DURING_ADDING_BATCH_JOB , "01B04" , "Cannot change binding data types once they were set; which was %s, but now %s." );
        register(NEED_MORE_PARAMETER                               , "22P01" , "Parameter not set at index: %s" );
        register(NOT_OUT_TYPE_PARAMETER                            , "22P03" , "Not registered OUT type parameter at index: %s" );
        register(CURSOR_AT_AFTER_LAST                              , "HY109" , "Invalid cursor position: Cursor is at after the last." );
        register(STREAM_ALREADY_CLOSED                             , "01C03" , "Stream already closed." );
        register(CURSOR_AT_INSERTING_ROW                           , "HY109" , "Invalid cursor position: Cursor is at inserting row." );
        register(CURSOR_NOT_AT_INSERTING_ROW                       , "HY109" , "Invalid cursor position: Cursor is not at inserting row." );
        register(XA_OPEN_FAIL                                      , "XAF01" , "XA open failed: %s" );
        register(XA_CLOSE_FAIL                                     , "XAF02" , "XA close failed: %s" );
        register(XA_RECOVER_FAIL                                   , "XAF03" , "XA recover failed: %s" );
        register(RESPONSE_TIMEOUT                                  , "HYT00" , "Response timeout : %s sec." );
        register(UNSUPPORTED_TYPE_CONVERSION                       , "42001" , "Unsupported type conversion: Cannot convert %s to SQL %s" );
        register(FAILOVER_SUCCESS                                  , "08F01" , "Failover success (%s: %s)" );
        register(INVALID_FORMAT_OF_ALTERNATE_SERVERS               , "08F02" , "Invalid value format for alternateservers propery: %s" );
        register(UNKNOWN_HOST                                      , "08H01" , "Unknown host: %s" );
        register(UNKNOWN_CONNTYPE                                  , "08H02" , "Unknown conntype: %s");
        register(CLIENT_UNABLE_ESTABLISH_CONNECTION                , "08001" , "Communication link failure: Cannot establish connection to server" );
        register(THREAD_INTERRUPTED                                , "JID04" , "Connection thread interrupted." );
        register(EXCEED_PRIMITIVE_DATA_SIZE                        , "JID05" , "The maximum buffer length was exceeded." );
        register(EXPLAIN_PLAN_TURNED_OFF                           , "EPS01" , "EXPLAIN PLAN is set to OFF");
        register(STATEMENT_IS_NOT_PREPARED                         , "HY007" , "The associated statement has not been prepared.");
        register(STMT_ID_MISMATCH                                  , "22V01" , "Invalid statement id: <%s> expected, but <%s>");
        register(PACKET_TWISTED                                    , "JIP01" , "Unexpected packet received.");
        register(INVALID_QUERY_STRING                              , "07Q01" , "Invalid query string");
        register(DOES_NOT_MATCH_COLUMN_LIST                        , "21S01" , "The INSERT value list does not match the column list.");
        register(OPTION_VALUE_CHANGED                              , "01S02" , "Option value changed: %s");
        register(CURSOR_OPERATION_CONFLICT                         , "01001" , "Cursor operation conflict.");
        register(INVALID_BIND_COLUMN                               , "22P04" , "There is no column that needs a bind parameter.");
        register(INVALID_BATCH_OPERATION_WITH_SELECT               , "01B05" , "Cannot fetch during batch update.");
        register(TRANSACTION_ALREADY_ACTIVE                        , "25002" , "Transaction already active.");
        register(BATCH_UPDATE_EXCEPTION_OCCURRED                   , "01B00" , "Batch update exception occurred: %s");
        register(TOO_MANY_BATCH_JOBS                               , "01B31" , "There are too many added batch jobs.");
        register(TOO_MANY_STATEMENTS                               , "HY000" , "There are too many allocated statements.");
        register(IOEXCEPTION_FROM_INPUTSTREAM                      , "22S01" , "IOException occured from InputStream: %s");
        register(INVALID_METHOD_INVOCATION                         , "JII01" , "Invalid method invocation.");
        register(INCONSISTANCE_DATA_LENGTH                         , "22L01" , "Inconsistance data length: <%s> expected, but <%s>");
        register(STATEMENT_NOT_YET_EXECUTED                        , "07S01" , "Statement has not been executed yet.");
        register(CANNOT_SET_SAVEPOINT_AT_AUTO_COMMIT_MODE          , "3BS01" , "Cannot set savepoint for auto-commit mode." );
        register(INVALID_SAVEPOINT_NAME                            , "3BV01" , "Invalid savepoint name: %s");
        register(INVALID_SAVEPOINT                                 , "3BV02" , "Invalid savepoint");
        register(NOT_SUPPORTED_OPERATION_ON_NAMED_SAVEPOINT        , "3BN01" , "Not supported operation on named savepoint.");
        register(NOT_SUPPORTED_OPERATION_ON_UNNAMED_SAVEPOINT      , "3BN02" , "Not supported operation on un-named savepoint.");
        register(NO_CURRENT_ROW                                    , "HY109" , "Invalid cursor state: No current row.");
        register(NO_AVAILABLE_DATASOURCE                           , "08D01" , "No available data source configurations.");
        register(EMPTY_RESULTSET                                   , "HYR01" , "Empty ResultSet");
        register(INVALID_PROPERTY_ID                               , "08P08" , "Invalid property id: %s");
        register(INVALID_STATE                                     , "JI001" , "Invalid state: <%s> expected, but <%s>" );
        register(MORE_DATA_NEEDED                                  , "JI002" , "Invalid packet: Expecting more data packets, but last packet received." );
        register(INVALID_TYPE                                      , "JI003" , "Invalid type: <%s> expected, but <%s>" );
        register(INTERNAL_BUFFER_OVERFLOW                          , "JI004" , "Buffer overflow" );
        register(INTERNAL_DATA_INTEGRITY_BROKEN                    , "JI005" , "Integrity of internal data has been broken" );
        register(ILLEGAL_ACCESS                                    , "JI006" , "Illegal access" );
        register(INVALID_HEX_STRING_LENGTH                         , "JI007" , "Length of the hex string must be multiple of 2" );
        register(INVALID_HEX_STRING_ELEMENT                        , "JI008" , "A character out of range in the hex string: index=%s, value=%s" );
        register(INVALID_NIBBLE_ARRAY_ELEMENT                      , "JI009" , "An element out of range in the nibble array: index=%s, value=%s" );
        register(COLUMN_READER_INITIALIZATION_SKIPPED              , "JI000" , "Desiginated ColumnReader has not been initialized" );
        register(VALUE_LENGTH_EXCEEDS                              , "22P05" , "Value length(%s) exceeds the maximum size(%s)" );
        register(PASSWORD_EXPIRATION_DATE_IS_COMING                , "01000" , "The password will expire within %s day(s).");
        register(UNSUPPORTED_KEYSTORE_ALGORITHM                    , "08000" , "Unsupported keystore algorithm : %s");
        register(CAN_NOT_CREATE_KEYSTORE_INSTANCE                  , "08000" , "Cannot create keystore instance: %s");
        register(CAN_NOT_LOAD_KEYSTORE                             , "08000" , "Unable to load %s type keystore from %s due to certificate problems");
        register(INVALID_KEYSTORE_URL                              , "08000" , "Invalid keystore url");
        register(KEY_MANAGEMENT_EXCEPTION_OCCURRED                 , "08000" , "Key management failure : %s");
        register(CAN_NOT_RETREIVE_KEY_FROM_KEYSTORE                , "08000" , "Cannot recover keys from client keystore");
        register(CAN_NOT_OPEN_KEYSTORE                             , "08000" , "Cannot open keystore from given url");
        register(DEFAULT_ALGORITHM_DEFINITION_INVALID              , "08000" , "Invalid default algorithm definitions for TrustManager and/or KeyManager");
        register(NOT_SUPPORTED_MANDATORY_PROPERTY                  , "08M01" , "Mandatory properties that are supported for the client version are not supported for the server version." );
        register(SHARD_INVALID_TEST_MARK                           , "HY000" , "Invalid test mark" );
        register(SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE              , "HY000" , "Failover is not available." );
        register(SHARD_NODE_NOT_FOUNDED                            , "HY000" , "No shard node founded by shard key value." );
        register(SHARD_BIND_PARAMETER_MISSING                      , "HY000" , "Shard parameter is not set at index: %s" );
        register(SHARD_MULTINODE_TRANSACTION_REQUIRED              , "HY000" , "Multi-node transaction required." );
        register(SHARD_INVALID_NODE_TOUCH                          , "HY000" , "Invalid node touch." );
        register(SHARD_SERVERSIDE_SINGLE_NODE_TOUCH_ERROR          , "HY000" , "Server-side query can not be performed in single-node transaction" );
        register(SHARD_JDBC_METHOD_INVOKE_ERROR                    , "HY000" , "shard jdbc method invoke error : %s" );
        register(SHARD_SPLIT_METHOD_NOT_SUPPORTED                  , "HY000" , "shard split method type %s not supported." );
        register(SHARD_RANGE_NOT_FOUNDED                           , "HY000" , "No shard range was founded" );
        register(SHARD_NO_NODES                                    , "HY000" , "No shard nodes" );
        register(SHARD_SMN_OPERATION_FAILED                        , "HY000" , "%s of client-side sharding failed. : %s" );
        register(SHARD_PIN_CHANGED                                 , "HY000" , "Shard pin was changed." );
        register(SHARD_MULTIPLE_LOB_OPERATION_NOT_SUPPORTED        , "HY000" , "Shard multiple lob operation is not supported." );
        register(SHARD_CHARSET_ENCODE_ERROR                        , "HY000" , "Cannot encode '%s' to %s charset." );
        register(EXECUTOR_CANNOT_BE_NULL                           , "HY000" , "Executor cannot be null." );
        register(TYPE_PARAMETER_CANNOT_BE_NULL                     , "22P06" , "Type parameter cannot be null." );
        register(TYPE_CONVERSION_NOT_SUPPORTED                     , "22P06" , "Conversion not supported for type %s" );
        register(CANNOT_BE_UNWRAPPED                               , "22023" , "%s cannot be unwrapped as %s" );
        register(SHARD_NEED_ROLLBACK                               , "HYC00" , "Failed to execute the statement, because the global transaction has been terminated." );
        register(SHARD_INTERNAL_ERROR                              , "HY000" , "Shard internal error: %s" );
    }

    private static void register(int aErrorCode, String aSQLState, String aMessageForm)
    {
        int sIndex = aErrorCode - FIRST_ERROR_CODE;
        ERROR_MAP_MESSAGE[sIndex] = aMessageForm;
        ERROR_MAP_SQLSTATE[sIndex] = aSQLState;
    }

    public static String getErrorMessage(int aErrorCode)
    {
        if (aErrorCode < FIRST_ERROR_CODE || aErrorCode > LAST_ERROR_CODE)
        {
            return null;
        }
        return ERROR_MAP_MESSAGE[aErrorCode - FIRST_ERROR_CODE];
    }

    public static String getErrorState(int aErrorCode)
    {
        if (aErrorCode < FIRST_ERROR_CODE || aErrorCode > LAST_ERROR_CODE)
        {
            // 정의된 JDBC 에러가 아니면 08000을 준다.
            return "08000";
        }
        return ERROR_MAP_SQLSTATE[aErrorCode - FIRST_ERROR_CODE];
    }

    private static final String[] XA_ERROR_MESSAGE =
    {
        /*  X : prefix       */ "Altibase XA Error: ",
        /* -2 : XAER_ASYNC   */ "There is an asynchronous operation already outstanding.",
        /* -3 : XAER_RMERR   */ "A resource manager error has occurred in the transaction branch.",
        /* -4 : XAER_NOTA    */ "The XID is not valid.",
        /* -5 : XAER_INVAL   */ "Invalid arguments were given.",
        /* -6 : XAER_PROTO   */ "Routine was invoked in an improper context.",
        /* -7 : XAER_RMFAIL  */ "Resource manager is unavailable.",
        /* -8 : XAER_DUPID   */ "The XID already exists.",
        /* -9 : XAER_OUTSIDE */ "The resource manager is doing work outside a global transaction.",
    };

    static String getXAErrorMessageWith(String aErrorMessage)
    {
        return ErrorDef.XA_ERROR_MESSAGE[0] + aErrorMessage;
    }

    public static String getXAErrorMessage(int aErrorCode)
    {
        if (aErrorCode < XAException.XAER_OUTSIDE || XAException.XAER_ASYNC < aErrorCode)
        {
            return null;
        }

        return getXAErrorMessageWith(XA_ERROR_MESSAGE[-aErrorCode - 1]);
    }
}
