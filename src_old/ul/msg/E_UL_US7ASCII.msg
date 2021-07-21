#
# UL 에러
#

# 에러코드 영역 번호
# 0 => id
# 1 => sm
# 2 => mt
# 3 => qp
# 4 => mm
# 5 => ul
# 6 => rp
# 7 => cm
# 9 => ut
# 10 => st
# 11 => lk
# 12 => dk
# 14 => sd

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.
# Last Number = 550

SECTION = 5

0, 00000, ulERR_IGNORE_NO_ERROR = Success
# *Cause: This is not an error.
# *Action: You can ignore this error message.

1, HY000, ulERR_ABORT_idnSetDefaultFactoryError = No idn Default Factory found.
# *Cause: No character set was found.
# *Action: Change the NLS_USE property.

2, IH, ulERR_ABORT_INVALID_HANDLE = Invalid Handle.
# *Cause: A handle is invalid.
# *Action: Check if the handle is SQL_NULL_HANDLE or NULL.

# 9, HY000, ulERR_ABORT_Invalid_Passwd_SYSDBA_Error = SYSDBA Password File Open Error.
# *Cause: Can't find a password file for the sysdba user.
# *Action: Check for the existence of a password file or create it.

# 10, 28000, ulERR_ABORT_INCORRECT_PASSWD = Invalid authorization specification. A password has not been provided, or is simply NULL.
# *Cause: Incorrect password provided.
# *Action: Use the correct password for the user.

# 11, HY000, ulERR_ABORT_Marshal_findInfo_Error = Protocol Error (Can't Find ID: <0%d>).
# *Cause: Protocol sequence broken, or corrupted protocol data used.
# *Action: Check protocol versions on both the client and the server.

12, HY001, ulERR_FATAL_MEMORY_ALLOC_ERROR = Memory allocation error. <0%s>.
# *Cause: Memory space has been exhausted.
# *Action: Reduce the amount of memory being used by the client application.

# 13, HY000, ulERR_ABORT_Channel_Initailize_Error = Communication Channel init error.
# *Cause: Internal Error
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 14, CIDLE, ulERR_ABORT_Connected_Idle_Instance_Error = Connected to idle instance.
# *Cause: This is not an error, just notification of connection to an idle instance.
# *Action: No action is necessary.

15, 22008, ulERR_ABORT_DateTime_Field_Overflow_Error = Datetime field overflow. <0%s>.
# *Cause: Invalid datetime data was used.
# *Action: Check the data type range.

16, 22007, ulERR_ABORT_Invalid_Datatime_Format_Error = Invalid datetime format.
# *Cause: The datetime data format is invalid.
# *Action: Check the datetime format.

17, 01S02, ulERR_IGNORE_OPTION_VALUE_CHANGED = Option value changed. <0%s>.
# *Cause:  The set option value has been changed.
# *Action: Check the option value.

18, HYC00, ulERR_ABORT_Ignored_Option_Error = This option is ignored.
# *Cause: An unsupported property was used.
# *Action: Check the property value.

19, HYC00, ulERR_ABORT_OPTIONAL_FEATURE_NOT_IMPLEMENTED = This optional feature is not implemented.
# *Cause: An unsupported property was used.
# *Action: Check the property value.

20, HY010, ulERR_ABORT_FUNCTION_SEQUENCE_ERR = Function sequence error.
# *Cause: The function call sequence is out of order.
# *Action: Refer to the state transition table.

21, HY024, ulERR_ABORT_INVALID_ATTRIBUTE_VALUE = Invalid attribute value.
# *Cause: Invalid attribute value.
# *Action: Check if the value you provided is valid for specified attribute.

22, HY092, ulERR_ABORT_INVALID_ATTR_OPTION = Invalid attribute/option identifier : <0%d>.
# *Cause: The attribute or option identifier was invalid.
# *Action: Check the attribute ID or option value.

# 23, 22018, ulERR_ABORT_Invalid_BinaryType_Value_Error = Invalid value <0%s> for cast specification.
# *Cause: The character value was invalid for the cast specification.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

24, HY003, ulERR_ABORT_INVALID_APP_BUFFER_TYPE = Invalid application buffer type : <0%d>.
# *Cause: The application buffer type was invalid.
# *Action: Check the host variable type in the application.

25, 07009, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX = Invalid descriptor index : <0%d>.
# *Cause: The value specified for the ParameterNumber argument was less than 1, or the value specified for ColumnNumber was greater than the number of columns in the result set.
# *Action: Check the ParameterNumber argument.

26, HY105, ulERR_ABORT_INVALID_PARAM_TYPE = Invalid parameter type : <0%d>.
# *Cause: The value specified for the InputOutputType argument was invalid.
# *Action: Check the InputOutputType argument.

27, 07006, ulERR_ABORT_RESTRICTED_DATATYPE_VIOLATION = Restricted data type attribute violation. SQL_C_TYPE <0%d> and SQL_TYPE <1%d> cannot be interconverted.
# *Cause: The data type identified by the ValueType argument cannot be converted to the data type identified by the ParameterType argument.
# *Action: Check the ValueType and ParameterType attributes.

28, HY009, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER = Invalid use of null pointer.
# *Cause: Invalid use of null pointer.
# *Action: Check the value pointer and value length.

29, HY090, ulERR_ABORT_INVALID_BUFFER_LEN = Invalid string or buffer length : <0%d>.
# *Cause: The value in BufferLength was less than 0 or greater than the property size.
# *Action: Check the host variable.

# 30, HY104, ulERR_ABORT_Invalid_Precision_Error = Invalid precision.
# *Cause: The value specified for the argument ColumnSize was outside the range.
# *Action: Check the argument ColumnSize.

# 31, 07002, ulERR_ABORT_Invalid_Null_Point_Field_Error = Invalid use of null pointer[<0%d> field].
# *Cause:
# *Action:

32, HY090, ulERR_ABORT_Invalid_StrLen_IndPtr_Error = Invalid string length.
# *Cause: The value in BufferLength was less than 0 or greater than the property value.
# *Action: Check the host variable.

# 33, HY000, ulERR_ABORT_Mutex_Error = Mutex error.
# *Cause: Failed to invoke the mutex_() system function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

########################
# MT not used
########################
# 34, HY001, ulERR_ABORT_Mtc_InitailizeClient_Error = mtc::initializeForClient() function call error
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 35, HY001, ulERR_ABORT_Mtl_moduleByName_Error = mtl::moduleByName() function call error
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 36, HY001, ulERR_ABORT_Mtd_ModuleById_Error = mtd::moduleById() function call error
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 37, 08001, ulERR_ABORT_Invaild_ALTIBASE_HOME_Error = The ALTIBASE_HOME environment must be set.
# *Cause: The ALTIBASE_HOME environment has not been set.
# *Action: Check the ALTIBASE_HOME environment variable.

38, HY010, ulERR_ABORT_INVALID_CONN_TYPE_ERROR = Invalid connection type : (<0%d>).
# *Cause: Invalid connection type.
# *Action: Check the value of the CONNTYPE keyword.

39, 01004, ulERR_IGNORE_RIGHT_TRUNCATED = String data right-truncated.
# *Cause: The buffer was not large enough to return the entire string.
# *Action: Check the buffer size.

# 40, HY000, ulERR_ABORT_Invalid_Port_Number_Error = Invalid Port Number.
# *Cause: PORT_NO is null.
# *Action: Check the value of the PORT_NO keyword.

# 41, HY000, ulERR_ABORT_Invalid_Host_Name_Error = Invalid Host name.
# *Cause: PWD is null.
# *Action: Check the value of the PWD keyword.

# 42, 01S06, ulERR_ABORT_ATTEMPT_FETCH_BEFOR_RETURN_ERR = A fetch attempt was made before the result set returned the first rowset.
# *Cause: A fetch attempt was made before the result set returned the first rowset.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 43, 01S00, ulERR_ABORT_Connection_String_Null_Pointer_Error = A connection string cannot be a null pointer.
# *Cause: The connection string is null.
# *Action: Set the connection string appropriately.

# 44, 01S00, ulERR_ABORT_Invaild_Connection_String_Attribute_Error = Invalid connection string attribute.
# *Cause: Invalid connection string attribute.
# *Action: Check the connection string attribute.

# 45, HY000, ulERR_ABORT_Invalid_Communication_Buffer_Error = Cannot get communication buffer value from server.
# *Cause: Cannot get communication buffer value from server.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

46, 24000, ulERR_ABORT_INVALID_CURSOR_STATE = Invalid cursor state.
# *Cause: A cursor was opened on the StatementHandle, and an invalid function was called.
# *Action: Check the function sequence.

# 47, HY000, ulERR_ABORT_Unable_DirectExecute_Array_Error = Unable to execute SQLExecDirect using array-binding.
# *Cause: Array binding is not used when executing SQLExecDirect.
# *Action: Check array binding when SQLExecDirect executes.

# 48, HY000, ulERR_ABORT_Exceeding_Statement_Number_Error = Statement ID overflow.
# *Cause: The statement number was greater than 1024.
# *Action: Use a statement number less than 1024.

# 49, 07006, ulERR_ABORT_Invaild_Converted_SQLBindCol_Error = The data value could not be converted to the data type specified by TargetType in SQLBindCol.
# *Cause: The TargetType in SQLBindCol is invalid.
# *Action: Check the TargetType attribute.

50, 08001, ulERR_FATAL_FAIL_TO_ESTABLISH_CONNECTION = Client unable to establish connection. (<0%s>)
# *Cause: The driver was unable to establish a connection with the data source.
# *Action: Check the connection information.

# 51, HY000, ulERR_ABORT_Invalid_Communication_Buffer_Data_Error = Invalid communication buffer data.
# *Cause: The system encountered an internal server error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

52, HY004, ulERR_ABORT_INVALID_SQL_TYPE = Invalid SQL data type : <0%d>.
# *Cause: The value specified for the InfoType argument was not valid for the ODBC version supported by the driver.
# *Action: Check the InfoType argument.

53, 08002, ulERR_ABORT_CONNECTION_NAME_IN_USE = Connection name in use.
# *Cause: The specified ConnectionHandle has already been used to establish a connection, and the connection is still open.
# *Action: Check the ConnectionHandle argument.

54, 08003, ulERR_ABORT_NO_CONNECTION = Connection does not exist.
# *Cause: The connection specified in the ConnectionHandle argument is not open.
# *Action: Check the ConnectionHandle argument.

# 56, HY000, ulERR_ABORT_Commnunication_Buffer_Full_Error = Communication buffer full; decrease array size.
# *Cause: The communication buffer requirement exceeds the CM_BUF_SIZE property value.
# *Action: Check and increase the value of CM_BUF_SIZE.

# 57, HY000, ulERR_ABORT_Invalid_Protocol_Error = Internal Error : Invalid Protocol. (<0%s>)
# *Cause: Invalid Protocol.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

58, HY012, ulERR_ABORT_INVALID_TRANSACTION_OPCODE = Invalid transaction operation code.
# *Cause: The value specified for the CompletionType argument was neither SQL_COMMIT nor SQL_ROLLBACK.
# *Action: Check the CompletionType argument.

# 59, HY000, ulERR_FATAL_Communication_Buffer_Insufficient_Error = The communication buffer is too small.
# *Cause: The communication buffer requirement exceeds CM_BUF_SIZE.
# *Action: Check and increase the CM_BUF_SIZE value.

# 60, HY000, ulERR_ABORT_Conn_Pool_Overflow_Error = ConnectionPool Entry Overflow
# *Cause: A connection pool overflow has occurred.
# *Action: ?

# 61, HYC00, ulERR_ABORT_Wrong_Dynamic_Cursor_Error = FORWARD ONLY dynamic cursor incorrectly used.
# *Cause:
# *Action:

# 62, HY000, ulERR_ABORT_Unable_Select_OR_OutBindPSM_Array_Error = Unable to execute select_query or out_bind_procedure.
# *Cause: You can use BATCH_ON mode as not 'OUT', 'INOUT' but 'IN' with SQLBindParameter.
# *Action: Use batch mode and inoutType in SQLBindParameter.

64, HY106, ulERR_ABORT_WRONG_FETCH_TYPE = Fetch type out of range.
# *Cause: Invalid FetchOrientation.
# *Action: Check the FetchOrientation value.

65, 22002, ulERR_ABORT_INDICATOR_REQUIRED_BUT_NOT_SUPPLIED_ERR = Indicator variable required but not supplied.
# *Cause: NULL data was bound to an output parameter for which StrLen_or_IndPtr, set in SQLBindParameter, was a null pointer.
# *Action: Set the StrLen_or_IndPtr argument in SQLBindParameter.

# 66, HY000, ulERR_ABORT_Unable_Batch_Processing_Array_Error = Unable to execute batch processing using array binding.
# *Cause: Unable to execute batch processing using array binding.
# *Action: Check batch mode and array binding.

67, 08S01, ulERR_ABORT_COMMUNICATION_LINK_FAILURE = Communication link failure. <0%s>
# *Cause: Communication link failure.
# *Action: Check the detailed error message following this error message.

68, 22003, ulERR_ABORT_Data_Value_Overflow_Error = <0%s> value out of range.
# *Cause: The value is out of range.
# *Action: Check the value.

# 69, 22003, ulERR_ABORT_Invalid_Numeric_Format_Error = The value is not in a numeric format.
# *Cause:
# *Action:

70, 01S07, ulERR_IGNORE_FRACTIONAL_TRUNCATION = Fractional truncation.
# *Cause: The data returned for an input/output or output parameter was truncated.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

71, HY013, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR = Memory management error <0%s>
# *Cause: Memory has been corrupted.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 72, HY000, ulERR_ABORT_Library_Load_Error = Library load error: <0%s>
# *Cause: The library could not be loaded.
# *Action: Check your system environment.

# 73, HY015, ulERR_ABORT_No_Cursor_Name_Available_Error = No cursor name available.
# *Cause: No cursor name available.
# *Action:

# 74, 34000, ulERR_ABORT_Invalid_Cursor_Name_Error = Invalid cursor name.
# *Cause: Invalid cursor name.
# *Action:

75, HY109, ulERR_ABORT_Invalid_Cursor_Position_Error = Invalid cursor position.
# *Cause: The cursor position is within the range of an invalid rowset. Either the row has been deleted or the rowset has not been fetched.
# *Action: Check that the cursor value is within the range of a fetched rowset.

76, HY107, ulERR_ABORT_ROW_VALUE_OUT_OF_RANGE_ERR = Row value out of range.
# *Cause: Row value out of range.
# *Action: Verify that the row value is correct.

#####################################
# uln 을 새로 만들면서 추가한 에러들.
#####################################

77, HYT01, ulERR_ABORT_CONNECTION_TIME_OUT = Connection timeout.
# *Cause: Connection timeout
# *Action: Increase CONNECTION_TIMEOUT value or tune the query.

78, HY013, ulERR_ABORT_CM_CALLBACK_NOT_EXIST = Communication module callback function does not exist.
# *Cause: The communication module version and CLI version are not compatible.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

79, 08S01, ulERR_ABORT_CM_GENERAL_ERROR = Communication link failure. (<0%s>)
# *Cause: Communication module error.
# *Action: Check the error message and take appropriate action.

# 80, HY000, ulERR_ABORT_BINDINFO_BUILD_ERR = Bindinfo build fail. <0%s>
# *Cause: Failed to bind parameters.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

81, HY000, ulERR_ABORT_PARAMETER_NOT_BOUND = Some parameters were not bound.
# *Cause: Some parameters were not bound.
# *Action: Check whether all the parameters were bound.

82, HY000, ulERR_ABORT_UNHANDLED_MT_TYPE = An unhandled server data type (<0%d>) has been received.
# *Cause: Received an invalid server data type.
# *Action: Check whether the server data type is supported.

# 83, HY000, ulERR_ABORT_BINDTYPE_MISMATCH = The CMT type of the received data does not match the user bound type. CMT type id : <0%d>, user specified type id : <1%d>.
# *Cause: 
# *Action:

# 84, HY000, ulERR_ABORT_UNKNOWN_CMT_TYPE = An unknown CMT type (<0%d>) has been received.
# *Cause: 
# *Action:

# 85, HY000, ulERR_ABORT_TYPES_NOT_COMPATIBLE = The user buffer type and SQL type are not compatible.
# *Cause:
# *Action:

86, HY017, ulERR_ABORT_INVALID_USE_OF_IMP_DESC = Invalid use of an automatically allocated descriptor handle.
# *Cause: An attempt was made to manipulate a descriptor handle allocated by the driver.
# *Action: Check the coding, and check that the proper syntax was used.

87, 07005, ulERR_ABORT_STMT_HAVE_NO_RESULT_SET = The prepared statement is not a cursor specification.
# *Cause: Since the statement associated with the statement handle did not return a result set, there was no column to describe.
# *Action: Check that the executed query returns a result set.

# 88, 01000, ulERR_IGNORE_NUM_OF_AFFETCTED_ROW_CNT_TOO_BIG = The number of affected rows exceeds the range that can be represented by a signed integer.
# *Cause: 
# *Action:

89, 25000, ulERR_ABORT_INVALID_TRANSACTION_STATE = Invalid transaction state.
# *Cause: Failed to execute the command.
# *Action: Check the transaction status.

# 90, HY090, ulERR_ABORT_INVALID_BUFFER_LEN_BOOKMARK = Invalid string or buffer length. Column number is 0 and BufferLength is not 4.
# *Cause:
# *Action:

94, HY011, ulERR_ABORT_ATTRIBUTE_CANNOT_BE_SET_NOW = Attribute cannot be set now.
# *Cause: Failed to set the attribute.
# *Action: Check the function call sequence.

95, HY095, ulERR_ABORT_FUNCTION_TYPE_OUT_OF_RANGE = Function type out of range.
# *Cause: Invalid function type.
# *Action: Check the function type.

# 96, IM010, ulERR_ABORT_DATASOURCE_NAME_TOO_LONG = Data source name too long.
# *Cause:
# *Action:

# 97, HY024, ulERR_ABORT_TCP_PORT_OUT_OF_RANGE = Invalid attribute value : TCP port number is out of range.
# *Cause:
# *Action:

98, HY024, ulERR_ABORT_CONNTYPE_NOT_SUPPORTED = Invalid attribute Value : CONNTYPE <0%d> is currently not supported.
# *Cause: Invalid CONNTYPE.
# *Action: Check CONNTYPE.

99, 01S00, ulERR_IGNORE_CONNECTION_STR_IGNORED = Connection string pair [<0%s> = {<1%s>};] ignored.
# *Cause: The connection string keyword is invalid or has no value.
# *Action: Check the keyword or value.

# 100, 01S00, ulERR_IGNORE_URL_TOKEN_IGNORED = URL token pair [<0%s> = {<1%s>};] ignored.
# *Cause:
# *Action:

# 101, 01S02, ulERR_IGNORE_TCP_HOSTNAME_NOT_SET = TCP hostname was not set; connecting to localhost.
# *Cause:
# *Action:

# 102, 01S02, ulERR_IGNORE_PORT_NO_NOT_SET = PORT_NO is not specified in the connection string. Using <0%d> from the ALTIBASE_PORT_NO environment variable instead.
# *Cause:
# *Action:

103, HY000, ulERR_ABORT_PORT_NO_ALTIBASE_PORT_NO_NOT_SET = Neither PORT_NO in the connection string nor the ALTIBASE_PORT_NO environment variable has been set.
# *Cause: A port has not been set.
# *Action: Set the port with a connection string or the ALTIBASE_PORT_NO environment variable.

104, HY024, ulERR_ABORT_ALTIBASE_HOME_NOT_SET = The ALTIBASE_HOME environment variable must be set before trying to connect to the Altibase server through a UNIX domain.
# *Cause: The ALTIBASE_HOME environment variable has not been set.
# *Action: Refer to the manual and set the ALTIBASE_HOME environment variable.

# 105, 01S00, ulERR_IGNORE_PORT_NO_IGNORED = The port number was ignored because the connection type is a Unix domain.
# *Cause:
# *Action:

# 106, 01S02, ulERR_IGNORE_SHM_KEY_NOT_SET = An option value has been changed. The SysV shared memory key has not been set. Using the default value <0%d>.
# *Cause:
# *Action:

# 107, 01S02, ulERR_IGNORE_CONNTYPE_NOT_SET = An option value has been changed. The connection type has not been set. Using the default connection type (TCP).
# *Cause:
# *Action:

108, HY096, ulERR_ABORT_INVALID_INFO_TYPE = Invalid information type. SQLGetInfo does not know the InfoType <0%d>.
# *Cause: Invalid InfoType.
# *Action: Verify that InfoType is valid.

109, HY091, ulERR_ABORT_READONLY_DESCRIPTOR_FIELD = Invalid descriptor field identifier : <0%d>. The descriptor field is read-only.
# *Cause: An attempt was made to change a read-only descriptor field.
# *Action: Check the coding, and check that the proper syntax was used.

110, HY024, ulERR_ABORT_INVALID_ALTIBASE_PORT_NO = Connection string does not have PORT_NO, and environment variable ALTIBASE_PORT_NO does not have a valid value : <0%s>.
# *Cause: The connection type is TCP, but the connection string does not specify a PORT_NO. Further, the environment variable ALTIBASE_PORT_NO is not valid.
# *Action: Set the ALTIBASE_PORT_NO environment variable correctly. Or specify PORT_NO in the connection string.

111, 01S06, ulERR_IGNORE_FETCH_BEFORE_RESULTSET = A fetch attempt made before the result set returned the first rowset.
# *Cause: A fetch attempt made before the result set returned the first rowset.
# *Action: You may safely ignore this error.

112, HY000, ulERR_ABORT_UNHANDLED_MT_TYPE_BINDINFO = <0%s> : There is missing case in Bindinfo build table, where CTYPE = <1%d>, MTYPE = <2%d>.
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

113, 07006, ulERR_ABORT_RESTRICTED_DATATYPE_VIOLATION_BOOKMARK = Restricted data type attribute violation. The ColumnNumber argument was 0, but the TargetType argument was not SQL_C_BOOKMARK or SQL_C_VARBOOKMARK.
# *Cause: The ColumnNumber argument was 0, but the TargetType argument was not SQL_C_BOOKMARK or SQL_C_VARBOOKMARK.
# *Action: Check the TargetType argument value.

114, 22003, ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE = Numeric value out of range.
# *Cause: The numeric value is out of range.
# *Action: Check the numeric value.

115, HY091, ulERR_ABORT_INVALID_DESC_FIELD_IDENTIFIER = Invalid descriptor field identifier : <0%d>.
# *Cause: An invalid descriptor field ID was used.
# *Action: Check the descriptor field ID.

# 116, HY013, ulERR_ABORT_INVALID_NIBBLE_PRECISION = Memory management error. The precision of the received nibble is too big.
# *Cause:
# *Action:

117, 22018, ulERR_ABORT_INVALID_NUMERIC_LITERAL = Invalid character value for cast specification.
# *Cause: A data value that cannot be converted has been input.
# *Action: Check the data type and input value.

118, 07006, ulERR_ABORT_INVALID_CONVERSION = Restricted data type attribute violation. SQL_TYPE cannot be converted to SQL_C_TYPE as requested.
# *Cuuse: SQL_TYPE cannot be converted to SQL_C_TYPE as requested.
# *Action: Refer to the CLI User's manual to convert to a supported type.

119, 22003, ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE_MINUS = Numeric value out of range. The bound data type is unsigned but the fetched data is less than zero.
# *Cause: The numeric value is less than zero.
# *Action: Check the numeric value.

521, 22003, ulERR_ABORT_NUMERIC_PRECISION_OUT_OF_RANGE = Numeric value out of range. The value exceeded the numeric precision range of SQL_NUMERIC_STRUCT.
# *Cause: The value exceeded the numeric precision range of SQL_NUMERIC_STRUCT.
# *Action: Change the value or increase the precision.

120, 22003, ulERR_ABORT_NUMERIC_SCALE_OUT_OF_RANGE = Numeric value out of range. The value exceeded the numeric scale range of SQL_NUMERIC_STRUCT.
# *Cause: The value exceeded the numeric scale range of SQL_NUMERIC_STRUCT.
# *Action: Use a value within the numeric scale range of SQL_NUMERIC_STRUCT.

# 121, 22007, ulERR_ABORT_INVALID_DATE_TIME_FORMAT = Invalid datetime format.
# *Cause:
# *Action:

122, HY109, ulERR_ABORT_INVALID_CURSOR_POSITION_GD = Invalid cursor position. The cursor position was AFTER END or BEFORE START.
# *Cuase: The cursor position was AFTER END or BEFORE START.
# *Action: Use SQLFetch or SQLFetchScroll to move to a valid cursor position.

123, 22003, ulERR_ABORT_BIT_VALUE_OUT_OF_RANGE = Numeric value out of range. SQL_C_BIT can only have the value 0 or 1.
# *Cuase: Invalid BIT value.
# *Action: Enter either 0 or 1.

124, 22003, ulERR_ABORT_BINARY_SIZE_NOT_MATCH = Numeric value out of range. SQL_C_BINARY data length does not match the size of the SQL data type.
# *Cuase: SQL_C_BINARY data length does not match the size of the SQL data type.
# *Action: Use another data type.

125, 07006, ulERR_ABORT_CANNOT_INSERT_INTERVAL = Restricted data type attribute violation. The interval type is only for fetch operations. Interval type columns cannot be created, nor can values be inserted into interval type columns?.
# *Cuase: The interval type was used.
# *Action: Use another data type.

# 127, 28000, ulERR_ABORT_INCORRECT_USERNAME = Invalid authorization specification. A username has not been provided, or is simply NULL.
# *Cause: Invalid user name.
# *Action: Provide the correct user name.

# 128, 28000, ulERR_ABORT_INCORRECT_DATABASENAME = Invalid authorization specification. Databasename has not been provided, or is simply NULL.
# *Cause: Invalid database name.
# *Action: Specify the correct database name.

129, HY024, ulERR_ABORT_STRING_LENGTH_TOO_LONG = Invalid attribute value. The string is too long to be used as an attribute value.
# *Cause: Invalid attribute value.
# *Action: Check the length of the provided string.

# 130, HY024, ulERR_ABORT_ATTRIBUTE_VALUE_OUT_OF_RANGE = Invalid attribute value. The provided attribute value is too big or too small.
# *Cause: Invalid attribute value.
# *Action: Check the value for the attribute and specify an appropriate value.

131, 22023, ulERR_ABORT_INCONSISTENT_NIBBLE_PRECISION_AND_BUFFER_SIZE = Invalid parameter value. The SQL_NIBBLE data structure precision field value exceeds the specified buffer size.
# *Cause: Invalid nibble structure precision.
# *Action: Specify an appropriate precision value for the nibble.

132, 22023, ulERR_ABORT_INCONSISTENT_BIT_PRECISION_AND_BUFFER_SIZE = Invalid parameter value. The SQL_BIT data structure precision field value exceeds the specified buffer size.
# *Cause: Invalid SQL_BIT structure precision.
# *Action: Specify an appropriate precision value for the SQL_BIT.

133, HY016, ulERR_ABORT_CANNOT_MODIFY_IRD = Cannot modify an implementation row descriptor.
# *Cause: An attempt was made to modify an implementation row descriptor.
# *Action: Check the coding, and check that the proper syntax was used.

134, HY000, ulERR_ABORT_FETCH_OFFSET_OUT_OF_RANGE = Fetch offset is too big or too small.
# *Cause: A scrollable cursor was used outside the FetchOffset range.
# *Action: When using a scrollable cursor, use it within the FetchOffset range.

135, 01000, ulERR_IGNORE_VALUE_TOO_BIG = The value is too big to fit in the parameter variable.
# *Cause: The rowcount value exceeds the out parameter maximum value.
# *Action: You may safely ignore this error.

136, HY024, ulERR_ABORT_INVALID_ARRAY_SIZE = Invalid array size. Please decrease the array size.
# *Cause: Invalid array size.
# *Action: Decrease the array size.

137, HY000, ulERR_ABORT_CONVERT_CHARSET_FAILED = Failed to convert charset : source charset id = <0%d>, target charset id = <1%d>, source index = <2%d>, target index = <3%d>
# *Cause: Failed to encode the string.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

138, 08001, ulERR_ABORT_ADMIN_ALREADY_RUNNING = Another SYSDBA session is already running.
# *Cause: A SYSDBA session is already running.
# *Action: Terminate the SYSDBA session process and execute it again.

139, HY000, ulERR_ABORT_INCOMPATIBLE_PROPERTY = Incompatible property : [<0%s>]. Upgrade server.
# *Cause: Incompatible property was used.
# *Action: Upgrade the server to the latest version.

140, 01000, ulERR_IGNORE_PASSWORD_GRACE_PERIOD = The password will expire within <0%d> day(s). 
# *Cause: The password expiration date is approaching. 
# # *Auction: Change your password.

141, HY090, ulERR_ABORT_INVALID_CONNECTION_STR_FORM = Invalid connection string format: <0%d> : [<1%c>]
# *Cause: The connection string format is invalid.
# *Action: Check the connection string. If it has exceeded the maximum allowed length. a question mark is output.

###############################################
# 2012.01.18
# proj_2160 cm_type removal
###############################################
151, 22007, ulERR_ABORT_INVALID_DATE_STRING = Invalid string: [<0%s>] for date format: [<1%s>]
# *Cause: The input data does not match the date format.
# *Action: Ensure that the input value matches the date format.

152, 22018, ulERR_ABORT_INVALID_NUMBER_STRING = Invalid string for number: [<0%.*s>]
# *Cause: The input string cannot be converted to a number.
# *Action: Check the input string.

153, HY010, ulERR_ABORT_LOCK_SEQUENCE_ERR = Lock sequence error.
# *Cause: invalid Lock call sequence.
# *Action: Try disconnect and reconnect 

#########################
# SQLGetData / SQLPutData
#########################

# 280, HYC00, ulERR_ABORT_GD_MULTIPLE_ROW_NOT_SUPPORT = This optional feature is not implemented. The driver does not support use of SQLGetData with multiple rows.
# *Cause:
# *Action:

# 281, 07009, ulERR_ABORT_GD_COLUMN_NUMBER_OUT_OF_ORDER = Invalid column number. The column number specified in the current call was less than or equal to the column number specified in the preceding call.
# *Cause:
# *Action:

# 282, 07009, ulERR_ABORT_GD_COLUMN_NUMBER_OUT_OF_BOUND = Invalid column number. The number of the specified column was greater than or equal to the number of the highest bound column.
# *Cause:
# *Action:

# 283, 07009, ulERR_ABORT_GD_COLUMN_ALREADY_BOUND = Invalid column number. The specified column has been bound. In order to use SQLGetData() on any column, please do not bind the column.
# *Cause:
# *Action:

284, HY011, ulERR_ABORT_PD_ATTRIBUTE_CANNOT_BE_SET_NOW = This attribute cannot be set now. You can not call SQLPutData() with StrLen_or_Ind set to SQL_NULL_DATA after one or more previous calls to SQLPutData() for the same parameter have been returned.
# *Cause: SQL_NULL_DATA was used for SQLPutData().
# *Action: Enter data other than SQL_NULL_DATA for SQLPutData().

285, HY000, ulERR_ABORT_PD_DATA_TOO_BIG = The SQL_CHAR or SQL_VARCHAR data type cannot be bigger than 32KB.
# *Cause: The SQL_CHAR or SQL_VARCHARSQL_CHAR data type cannot accept a value larger than 32KB.
# *Action: Enter a value smaller than 32KB for the SQL_CHAR or SQL_VARCHARSQL_CHAR data type. If the value is larger than 32KB, divide the value and input several times.

# 286, HY000, ulERR_ABORT_PD_DATA_AT_EXEC_OUTBINDING = DATA_AT_EXEC parameter should not be designated as an output parameter.
# *Cause:
# *Action:

287, HY019, ulERR_ABORT_PD_FIXED_DATA_NOT_ALLOWD_IN_PIECES = Non-character and non-binary data sent in pieces.
# *Cause: An attempt was made to send non-variable data (non-CHAR, BINARY data) in pieces.
# *Action: Send non-variable data in one piece.

#########################
# LOB
#########################

300, HY000, ulERR_ABORT_LOB_AUTOCOMMIT_MODE_ERR = The connection is in autocommit mode. One cannot operate on LOB data in autocommit mode.
# *Cause: An attempt to execute a LOB operation was made while the connection is in autocommit mode.
# *Action: Turn off autocommit mode.

301, HY000, ulERR_ABORT_LOB_FILE_WRITE_ERR = Failed to write LOB data to the file.
# *Cause: Failed to write LOB data to the file.
# *Action: Check whether the file path is valid and whether a file can be written.

302, HY000, ulERR_ABORT_LOB_FILE_READ_ERR = Failed to read from a file.
# *Cause: Failed to read the file.
# *Action: Check whether the file path is valid and whether the file can be read.

303, HY000, ulERR_ABORT_FILE_OPEN = Failed to open file : <0%s>.
# *Cause: Failed to open the file.
# *Action: Check whether the file path is valid and whether the file can be read.

304, HY000, ulERR_ABORT_FILE_EXIST = The file <0%s> already exists.
# *Cause: The file already exists.
# *Action: Delete the file, or use the SQL_FILE_OVERWRITE or SQL_FILE_APPEND option.

305, HY000, ulERR_ABORT_FILE_CLOSE = Failed to close file : <0%s>.
# *Cause: Failed to close the file.
# *Action: Check the operating system error number.

306, HY000, ulERR_ABORT_GET_FILE_SIZE = Failed to get the file size: <0%s>.
# *Cause: Failed to retrieve file information due to an operating system error.
# *Action: Check the operating system error number.

307, HY000, ulERR_ABORT_UNKNOWN_FILE_OPTION = Unknown file option <0%d>.
# *Cause: An invalid file option was used.
# *Action: Use a valid file options (SQL_FILE_CREATE, SQL_FILE_OVERWRITE, SQL_FILE_APPEND, SQL_FILE_READ)

# 308, 01004, ulERR_IGNORE_RIGHT_TRUNCATED_LOB = String data truncated; right portion removed. More LOB data to come.
# *Cause: 
# *Action:

# 309, HY000, ulERR_ABORT_LOB_NOT_ALLOWD = A LOB cannot be used with a scrollable cursor or the prefetch option.
# *Cause: A Lob cannot be used with a scrollable cursor or when the prefetch option is on. However, binding a variable directly to a LOB locator is acceptable.
# *Action: Refrain from using a LOB with a scrollable cursor. Alternatively, handle the LOB using a LOB locator.

# 310, HY013, ulERR_FATAL_LOB_LOCATOR_NOT_EXIST = Memory management error. A LOB locator could not be found. Possible memory corruption.
# *Cause: The state of the ulnLob structure was ULN_LOB_ST_INITIAL while attempting to open or close a LOB locator.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

311, HY013, ulERR_FATAL_LOB_NOT_OPENED = Memory management error. An attempt was made to execute an operation on a LOB locator that was not open. Possible memory corruption in <0%s>.
# *Cause: The state of the ulnLob structure was not ULN_LOB_ST_OPENED while attempting to execute an operation on the ulnLob.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

312, HY000, ulERR_ABORT_INVALID_FILE_OPTION = Invalid fileoption <0%d>.
# *Cause: The specified file option is invalid. For instance, you may have specified SQL_FILE_CREATE while you intended to send the contents of a file to the Altibase server.
# *Action: Use a valid file options (SQL_FILE_CREATE, SQL_FILE_OVERWRITE, SQL_FILE_APPEND, SQL_FILE_READ).

313, HY013, ulERR_FATAL_LOB_INVALID_STATE = Memory management error. The LOB function should not be called in this state. Possible memory corruption in <0%s>.
# *Cuase: Failed to call the LOB function using a handle.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

314, HY000, ulERR_ABORT_FILE_SIZE_TOO_BIG = General error. The size of the provided file <0%s> is too big. The maximum file size is 4,294,967,295 bytes (4GB-1byte).
# *Cuase: The file is too big.
# *Action: Check the file size and use a smaller file.

# 315, 01000, ulERR_IGNORE_LOB_RETRIEVED_SIZE_TOO_BIG = The size of the LOB exceeds the range that can be represented by a 32-bit signed integer.
# *Cause: The LOB data size exceeds the range that can be represented by a 32 bit signed integer.
# *Action: Type cast the variable to a 32-bit unsigned integer.

316, HY003, ulERR_ABORT_INVALID_APP_BUFFER_TYPE_LOB = Invalid application buffer type : type id <0%d> cannot be used as a LOB source buffer.
# *Cause: An invalid buffer type was used.
# *Action: Check the buffer type.

# 317, 01000, ulERR_IGNORE_LOB_SIZE_OVER_2G = The size of the LOB data is excessive.

318, S1090, ulERR_ABORT_VALIDATE_INVALID_LENGTH = Invalid data value
# *Cause: The value of the data exceeds the logical value scope.
# *Action: Check the data value.

319, HY000, ulERR_ABORT_INVALID_CHARACTER = Invalid character use.
# *Cause: An invalid character is being used.
# *Action: Verify that every character in the input string is a valid character.

320, HY000, ulERR_ABORT_INVALID_LOB_RANGE = Invalid lob range
# *Cause: Invalid lob range
# *Action: Check the lob size and use a valid lob range.

#########################
# Failover
#########################

400, HY000, ulERR_ABORT_FAILOVER_SUCCESS = Failover success (ERR-<0%x> [<1%s>] <2%s>)
# *Cause: Session failover success
# *Action: Re-execute application logic.

401, HY024, ulERR_ABORT_INVALID_ALTERNATE_SERVER_HOST = The IP/Host value used in AlternateServers connection attribute is invalid: <0%s>
# *Cause:  The IP/Host value used in the AlternateServers connection attribute is invalid.
# *Action: Make sure the IP/Host value used in the AlternateServers connection attribute is valid.

402, HY024, ulERR_ABORT_GETADDRINFO_ERROR = The call to getaddrinfo() failed. The host name or service may be unknown.
# *Cause: The host name or service may be unknown.
# *Action: Check the host name.

403, HY024, ulERR_ABORT_CONNECT_INVALIDARG = Invalid connect() argument.
# *Cause: An argument for the connect() function was invalid.
# *Action: Check the IP and host name.

404, HY024, ulERR_ABORT_INVALID_ALTERNATE_SERVER_FORMAT= The value of AlternateServers connection attribute is invalid: <0%s>
# *Cause:  The value of AlternateServers connection attribute does not comply with the specified format.
# *Action: Make sure the value of AlternateServers connection attribute complies with the specified format.

405, HY024, ulERR_ABORT_INVALID_ALTERNATE_SERVER_PORT = The port values used in AlternateServers connection attribute are invalid: <0%s>
# *Cause:  The port values used in the AlternateServers connection attribute are not numbers or out of available range.
# *Action: Make sure the port values used in the AlternateServers connection attribute are valid.

406, HY024, ulERR_ABORT_ALTERNATE_SERVER_NOT_SET = The AlternateServers is not set.
# *Cause: The AlternateServers is not set.
# *Action: Set up the value of AlternateServers connection.

#########################
# ACS Error Code
# ID, N/A, Error Code
#########################

# 500, 00000, ulERR_IGNORE_ACS_NO_ERROR = Success
# *Cause: This is not an error.
# *Action: You can safely ignore this error message.

501, 00000, ulERR_ABORT_ACS_INSUFFICIENT_BUFFER_SIZE = Insufficient buffer size ( <0%d> ). Recommended size is more than ( <1%d> ).
# *Cause: There was insufficient buffer size for the geometry object.
# *Action: Increase the buffer size.

502, 00000, ulERR_ABORT_ACS_INVALID_BYTE_ORDER = Invalid byte order
# *Cause: The byte order is invalid.
# *Action: Set the byte order information correctly.

503, 00000, ulERR_ABORT_ACS_INVALID_DATA_TYPE = Invalid geometry sub-type
# *Cause: The geometry sub-type is invalid.
# *Action: Set the geometry sub-type information correctly.

504, 00000, ulERR_ABORT_ACS_INVALID_PARAMETER_RANGE = Invalid parameter value.
# *Cause: A parameter value was invalid.
# *Action: Check the parameter value.

505, 00000, ulERR_ABORT_ACS_INVALID_GEOMETRY = Invalid geometry value.
# *Cause: A geometry value was invalid.
# *Action: Check the geometry value.

506, 00000, ulERR_ABORT_ACS_INVALID_GEOMETRY_BYTEORDER = Invalid geometry byte order.
# *Cause: The geometry byte order was invalid.
# *Action: Check the geometry byte order and consider calling ACSAdjustByteOrder.

507, 00000, ulERR_ABORT_ACS_NOT_APPLICABLE_GEOMETRY_TYPE = Inapplicable geometry type
# *Cause: The geometry type is not applicable.
# *Action: Check the geometry type.

508, 00000, ulERR_ABORT_ACS_INVALID_WKB = Error parsing well-known-binary
# *Cause:  The specified well-known-binary is not correct.
# *Action: Check the well-known-binary.

#########################
# Altibase C Interface
#########################

509, HY000, ulERR_ABORT_UNSUPPORTED_FUNCTION = Unsupported function.
# *Cause:  This function is not supported.
# *Action: Do not use this function.

510, HY000, ulERR_ABORT_INVALID_USE_OF_BIND_TYPE = Invalid use of bind type: <0%d>.
# *Cause:  The specified bind type has not been defined.
# *Action: Check the list of defined bind types and try again.

511, HY000, ulERR_ABORT_INVALID_USE_OF_FIELD_TYPE = Invalid use of field type: <0%d>.
# *Cause:  The specified field type has not been defined.
# *Action: Check the field type.

512, HY000, ulERR_ABORT_TOO_MANY_STATEMENT = There are too many allocated statements.
# *Cause:  Too many statements are allocated.
# *Action: Remove unused statements.

522, HY000, ulERR_ABORT_INVALID_INDICATOR = Invalid indicator value.
# *Cause: The indicator value was invalid. 
# *Action: Initialize the indicator value.

523, HY000, ulERR_ABORT_FAIL_OBTAIN_ERROR_INFORMATION = failed to obtain error information.
# *Cause: Failed to obtain error information.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).


#########################
# Updatable Scrollable Cursor
#########################

513, HY111, ulERR_ABORT_INVALID_BOOKMARK_VALUE = Invalid bookmark value
# *Cause:  FetchOrientation is SQL_FETCH_BOOKMARK, but the SQL_ATTR_FETCH_BOOKMARK_PTR value is NULL or invalid.
# *Action: Check your application.

514, 02502, ulERR_IGNORE_HOLE_DETECTED = Delete or update hole detected
# *Cause:  A row in the result table is missing or changed.
# *Action: Check the indicator variable and handle the hole accordingly.

515, 24519, ulERR_IGNORE_HOLE_DETECTED_BUT_NO_INDICATOR = Hole detected but no indicator variable provided
# *Cause:  A hole was found in the result table, but no row status indicator variable was provided.
# *Action: Set row-status indicator variables.

516, 07006, ulERR_ABORT_RESTRICTED_DATATYPE_VIOLATION_BOOKMARK_TYPE = Restricted data type attribute violation
# *Cause:  Column 0 is bound with SQL_C_BOOKMARK when SQL_ATTR_USE_BOOKMARKS value is SQL_UB_VARIABLE, or is bound with SQL_C_VARBOOKMARK when SQL_ATTR_USE_BOOKMARKS value is not SQL_UB_VARIABLE.
# *Action: Set SQL_ATTR_USE_BOOKMARKS to SQL_UB_VARIABLE when binding Column 0 with SQL_C_VARBOOKMARK, or to SQL_UB_FIXED otherwise.

517, HY003, ulERR_ABORT_PROGRAM_TYPE_OUT_OF_RANGE = Data type in program out of range
# *Cause:  ColumnNumber argument is 0 and TargetType does not match bookmark setting.
# *Action: Use SQL_C_VARBOOKMARK for a variable bookmark, or SQL_C_BOOKMARK for a static bookmark.

518, 01S01, ulERR_IGNORE_ERROR_IN_ROW = Error(s) in row(s)
# *Cause:  One or more rows could not be processed.
# *Action: Check the error information.

519, 01001, ulERR_ABORT_CURSOR_OP_CONFLICT = Cursor operation conflict
# *Cause:  The target rows for the UPDATE or DELETE operation could not be found, or have already been updated or deleted.
# *Action: Refetch before executing UPDATE or DELETE, and do not execute UPDATE or DELETE on deleted rows.

520, 21S02, ulERR_ABORT_DOES_NOT_MATCH_COLUMN_LIST = Type and number of columns in derived table does not match column list
# *Cause:  The column on which to perform the operation could not be found.
# *Action: Check if the column is unbound, read-only, or if the length/indicator value is SQL_COLUMN_IGNORE.

#############################
# SSL
#############################
524, HY000, ulERR_ABORT_SSL_OPERATION_FAILURE = SSL operation failure. <0%s>
# *Cause: SSL operation failure. 
# *Action: Check the detailed error and take appropriate action. 

525, HY000, ulERR_ABORT_SSL_LIBRARY_ERROR = Failed to load the OpenSSL library - <0%s>
# *Cause: Failed to open the library or load functions from the library. 
# *Action: Check whether or not the library has been installed and set properly. 

526, 08S01, ulERR_ABORT_SSL_LINK_FAILURE = SSL link failure. <0%s>
# *Cause: SSL link failure.
# *Action: Check the detailed error code and take appropriate action. 

527, HY000, ulERR_IGNORE_CANNOT_BE_PREFETCHED_ASYNC = Synchronous prefetch is executed since the asynchronous prefetch failed.
# *Cause: Other statement is performing asynchronous prefetch.
# *Action: Retry asynchronous prefetch after closing the cursor of statement.

541, HY024, ulERR_ABORT_INVALID_ALTIBASE_SSL_PORT_NO = Connection string does not have PORT_NO, and environment variable ALTIBASE_SSL_PORT_NO does not have a valid value : <0%s>.
# *Cause: The connection type is SSL, but the connection string does not specify a PORT_NO. Further, the environment variable ALTIBASE_SSL_PORT_NO is not valid.
# *Action: Set the ALTIBASE_SSL_PORT_NO environment variable correctly. Or specify PORT_NO in the connection string.

542, HY000, ulERR_ABORT_PORT_NO_ALTIBASE_SSL_PORT_NO_NOT_SET = Neither PORT_NO in the connection string nor the ALTIBASE_SSL_PORT_NO environment variable has been set.
# *Cause: A port has not been set.
# *Action: Set the port with a connection string or the ALTIBASE_SSL_PORT_NO environment variable.

#############################
# IPCDA
#############################
528, HY000, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG = Exceeding message size for maximum size of IPCDA buffer(<0%d>).
# *Cause: The message to convey exceeds the maximum size of IPCDA buffer.
# *Action: Use another connection type.

529, HYC00, ulERR_ABORT_IPCDA_UNSUPPORTED_FUNCTION = Unsupported function for IPCDA. 
# *Cause: The function is not supported in IPCDA.
# *Action: Use another connection type.

530, HYC00, ulERR_ABORT_IPCDA_UNSUPPORTED_SQL_DATA_TYPE = Unsupported SQL data type(<0%d>) for IPCDA.
# *Cause: Unsupported SQL data type in IPCDA was used.
# *Action: Use another connection type.

531, HYC00, ulERR_ABORT_IPCDA_UNSUPPORTED_C_DATA_TYPE = Unsupported C data type(<0%d>) for IPCDA.
# *Cause: Unsupported C data type in IPCDA was used.
# *Action: Use another connection type.

#############################
# For Shard
#############################
532, HY000, ulERR_ABORT_SHARD_ERROR = The <0%s> of client-side sharding failed due to the following reason: <1%s>
# *Cause: The operation of client-side sharding is invalid.
# *Action: Take an appropriate measure for the error.

533, HY000, ulERR_ABORT_SHARD_CLI_ERROR = The <0%s> of client-side sharding failed.: <1%s>
# *Cause: The operation of shard data node failed.
# *Action: Take an appropriate measure for the error.

534, HY000, ulERR_ABORT_SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE = Failover is not available.
# *Cause: Failed to connect to shard library or coordinates.
# *Action: Check the shard node status or network.

535, HY000, ulERR_ABORT_SHARD_NODE_INVALID_TOUCH = This is an invalid transaction performing on multiple nodes.(The previous access node[<0%s>,<1%s>:<2%d>], the current access node[<3%s>,<4%s>:<5%d>])
# *Cause: Each transaction is performed only on a single shard data node.
# *Action: Complete the transaction being currently executed.

536, HY000, ulERR_ABORT_SHARD_VERSION_MISMATCH = Mismatched Shard version. 
# *Cause: The shard versions of server and client do not correspond. 
# *Action: Use the identical shard and client versions.

537, HY000, ulERR_ABORT_SHARD_DATA_NODE_CONNECTION_FAILURE = <0%s>
# *Cause: An error occurred during the data node connection.
# *Action: Take appropriate measures after verifying the data node and error number.

543, 01000, ulERR_ABORT_SHARD_META_CHANGED = The shard meta information was changed.
# *Cause: The shard meta information was changed.
# *Action: Re-execute application logic.

544, HY000, ulERR_ABORT_SHARD_INTERNAL_ERROR = The shard internal server error (<0%s>).
# *Cause: Unexepected error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

545, HYC00, ulERR_ABORT_SHARD_UNSUPPORTED_FUNCTION = The <0%s> is not supported in shard client library. 
# *Cause: This function is not supported in shard client library.
# *Action: Do not use this function.

546, HYC00, ulERR_ABORT_FEATURE_NOT_IMPLEMENTED_IN_SHARD = This feature is not implemented.
# *Cause: An unsupported attribute was used in shard client library.
# *Action: Check the attribute.

547, HYC00, ulERR_ABORT_NEED_ROLLBACK = Failed to execute the statement, because the global transaction has been terminated.
# *Cause: After failing to commit in global transaction level, rollback is necessary to execute a statement to a remote server.
# *Action: Execute rollback and retry the statement.

548, IH, ulERR_ABORT_INVALID_LOCATOR_HANDLE = Invalid Locator Handle.
# *Cause: A locator handle is invalid.
# *Action: Check if the locator is 0.

549, HY000, ulERR_ABORT_SHARD_MULTIPLE_ERRORS = Multiple errors occurred on each of shard nodes.
# *Cause: Multiple errors occurred on each of shard nodes.
# *Action: Check the error messages on each shard node.

550, HY000, ulERR_ABORT_FAILED_TO_PROPAGATE_SHARD_META_NUMBER = Failed to propagate shard meta number.
# *Cause: Failed to propagate shard meta number.
# *Action: Check the shard node status or network.

#############################
# IB
#############################

538, HY024, ulERR_ABORT_INVALID_ALTIBASE_IB_PORT_NO = Connection string does not have PORT_NO, and environment variable ALTIBASE_IB_PORT_NO does not have a valid value : <0%s>.
# *Cause: The connection type is IB, but the connection string does not specify a PORT_NO. Further, the environment variable ALTIBASE_IB_PORT_NO is not valid.
# *Action: Set the ALTIBASE_IB_PORT_NO environment variable correctly. Or specify PORT_NO in the connection string.

539, HY000, ulERR_ABORT_PORT_NO_ALTIBASE_IB_PORT_NO_NOT_SET = Neither PORT_NO in the connection string nor the ALTIBASE_IB_PORT_NO environment variable has been set.
# *Cause: A port has not been set.
# *Action: Set the port with a connection string or the ALTIBASE_IB_PORT_NO environment variable.

540, HY024, ulERR_ABORT_IB_RCONNECT_INVALIDARG = Invalid rconnect() argument.
# *Cause: An argument for the rconnect() function was invalid.
# *Action: Check the IP and host name.
