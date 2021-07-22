# $id: E_MAIN_US7ASCII.msg,v 1.4 2000/03/23 03:01:56 sjkim Exp $
#
# MAIN 에러 메시지 화일 (used by genErrMsg)
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

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.

# Last Number = 265

SECTION = 4

0,mmERR_IGNORE_NO_ERROR              = No main module error
# *Cause: This is not an error.
# *Action: Ignore this error message.

# 1,mmERR_ABORT_MEMORY_ERROR          = Not enough memory
# *Cause: Not enough memory
# *Action: Verify that the system has enough main memory and restart the system.

15,mmERR_FATAL_LOAD_MSB_ERROR        = Unable to load the message file <0%s>
# *Cause: The system failed to find a "msb" file or open the file.
# *Action: Please make sure that the "msb" file is in the "msg" directory.

19,mmERR_FATAL_MAXHANDLE_ERROR       = Unable to get the maximum number of file handles
# *Cause: It is impossible to retrieve the maximum number of file descriptors allowed for this system.
# *Action: Please send a bug report to the vendor.

# 21,mmERR_FATAL_SIGNAL_SET_ERROR      = Unable to install a signal handler.
# *Cause: It is impossible to set the signal handler.
# *Action: Please send a bug report to the vendor.

# 24,mmERR_FATAL_NO_ADMIN_CONNECT_ERROR = No connection request from SYSDBA while starting the server.
# *Cause: The SYSDBA did not request a connection while starting the server.
# *Action: Please send a bug report to the vendor.

27,mmERR_FATAL_SETLIMIT_ERROR        = Failed to invoke the setlimit() system function
# *Cause: An error occurred while invoking the setlimit() function.
# *Action: Please send a bug report to the vendor.

28,mmERR_FATAL_GETLIMIT_ERROR        =  Failed to invoke the getlimit() system function
# *Cause: An error occurred while invoking the getlimit() function.
# *Action: Please send a bug report to the vendor.

40,mmERR_FATAL_FLOCK_INIT =  Failed to invoke the flock_init() system function [<0%s>]
# *Cause:
# - The flock_init() system call failed.
# *Action: Please send a bug report to the vendor.

91,mmERR_FATAL_FLOCK_DESTROY =  Failed to invoke the flock_destroy() system function [<0%s>]
# *Cause:
# - The flock_destroy() system call failed.
# *Action: Please send a bug report to the vendor.

41,mmERR_FATAL_FLOCK_TRYWRLOCK =  Failed to invoke the flock_trywrlock() system function
# *Cause:
# - The flock_trywrlock() system call failed.
# *Action: Please send a bug report to the vendor.

93,mmERR_FATAL_FLOCK_WRLOCK =  Failed to invoke the flock_wrlock() system function
# *Cause:
# - The flock_wrlock() system call failed.
# *Action: Please send a bug report to the vendor.

42,mmERR_FATAL_FLOCK_UNLOCK =  Failed to invoke the flock_unlock() system function
# *Cause:
# - The flock_unlock() system call failed.
# *Action: Please send a bug report to the vendor.

### -- connect
# 43, mmERR_ABORT_PROTOCOL_VER_MISMATCH_ERROR  = Protocol version incompatibility between the client and the server.
# *Cause: The client protocol version is different from the server protocol version.
# *Action: Verify the client protocol and use a client that is compatible with the server protocol.

44,mmERR_ABORT_IDN_MISMATCH_ERROR = Incompatible NLS between the client(<0%s>) and the server(<1%s>).
# *Cause: The NLS of the client is different from the one supported by the server.
# *Action: Use the same NLS as the server NLS.

45,mmERR_ABORT_NO_USER_ERROR                = Invalid user
# *Cause: The user is not registered in the database.
# *Action: Use a user who is registered in the database.

46,mmERR_ABORT_INVALID_PASSWORD_ERROR       = Invalid password
# *Cause: The password is incorrect.
# *Action: Enter the correct password for the user.

# 47,mmERR_ABORT_INVALID_CONN_ENV_ERROR       = Invalid client interface (Currently supported interfaces are JDBC, ODBC and CLI)
# *Cause: The client interface is not supported yet.
# *Action: Use one of the supported client interfaces.

# 48,mmERR_FATAL_INTERNAL_ERROR  =  Internal server error
# *Cause: The system encountered an internal server error.
# *Action: Please send a bug report to the vendor.

# 49,mmERR_ABORT_TIMEOUT_ERROR   = Data transfer timed out.
# *Cause: No data transfer detected for some time.
# *Action: Increase the value of the CM_MAX_TIMEOUT property or set the value to 0 (No timeout).

# 50,mmERR_ABORT_LARGE_ERROR     = Communication packet too large
# *Cause: Packet size for communication exceeded the value specified by the CM_BUF_SIZE property.
# *Action: Adjust the value of the CM_BUF_SIZE property appropriately.

51,mmERR_ABORT_INVALID_ERROR   = Invalid communication protocol
# *Cause: Code based on an invalid protocol has been found.
# *Action: Verify the version of library that is used by the client.

# 52,mmERR_ABORT_INVALID_STATE_ERROR  = Unsupported communication protocol
# *Cause: Code based on an unsupported protocol has been found.
# *Action: Please send a bug report to the vendor

53,mmERR_ABORT_ALREADY_CONNECT_ERROR = Already connected to a session
# *Cause: A new connection has been requested to an already connected session.
# *Action: Verify the client code.

# 56,mmERR_ABORT_INVALID_STATEMENT_ID = The value of the statement ID exceeds the maximum value.
# *Cause: The ID number for a SQL statement exceeds the maximum value.
# *Action: Please send a bug report to the vendor.

57,mmERR_ABORT_INSUFFICIENT_QUERY_ERROR =  SQL statement too short
# *Cause: The message from the client did not appear to contain any SQL text.
# *Action: Verify the SQL statement from the client. Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

58,mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR =  Invalid statement processing request
# *Cause: The request was invalid for the current state of the statement.
# *Action: Check the client application.

60,mmERR_ABORT_INVALID_FETCH_ERROR = Invalid SQL statement data fetch request
# *Cause: An invalid SQL statement fetch request has been detected.
# *Action: Verify the SQL statement from the client.

# 61,mmERR_ABORT_LARGE_FETCH_BUFFER_ERROR =  Too much fetched data
# *Cause: The size of a row exceeds the size of the communication buffer.
# *Action: Increase the CM_BUF_SIZE property value so that it is larger than the maximum record size and restart the database.

63,mmERR_ABORT_TOO_MANY_STATEMENT = There are too many allocated statements.
# *Cause: Too many statements are allocated on this system.
# *Action: Remove unused statements or increase system resources.

### -- admin & dispatcher
64,mmERR_ABORT_ADMIN_MODE_ONLY = Unable to provide service while the server starts up.
# *Cause: A request for service was made to the server while it was starting up.
# *Action: Terminate all the clients while the server starts up.

65,mmERR_ABORT_ADMIN_ALREADY_RUNNING = Another SYSDBA session is already running.
# *Cause: A SYSDBA session is already running.
# *Action: Terminate the SYSDBA session process and execute it again.

### MMC related code below
67,mmERR_ABORT_MMC_ACCESS_MODE = Invalid access privileges
# *Cause: You do not have access privileges for the table.
# *Action: Verify that you have access privileges for the table.

# 68,mmERR_ABORT_MMC_NOT_DEFINED_STATEMENT = Internal server error
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

71,mmERR_ABORT_MMC_TRANSACTION_ALREADY_ACTIVE = The transaction is already active.
# *Cause: The transaction is already active.
# *Action: Wait until the transaction is complete.

72,mmERR_ABORT_MMC_CANT_SET_TRANSACTION_IN_AUTOCOMMIT_MODE = The transaction is already in auto-commit mode.
# *Cause: The transaction is already in auto-commit mode.
# *Action: Check that the transaction is in auto-commit mode.


80,mmERR_ABORT_NOT_APPLICABLE = Operation not applicable
# *Cause: Operation not applicable.
# *Action: Specify a valid parameter

# 81,mmERR_FATAL_THREAD_JOIN =  Failed to invoke the thr_join() system function
# *Cause: The system failed to join threads.
# *Action: Please send a bug report to the vendor.

82,mmERR_FATAL_THREAD_CONDITION_INIT =  Failed to invoke the cond_init() system function
# *Cause: The system failed to invoke cond_init().
# *Action: Please send a bug report to the vendor.

83,mmERR_FATAL_THREAD_CONDITION_SIGNAL =  Failed to invoke the cond_signal() system function
# *Cause: The system failed to invoke cond_signal().
# *Action: Please send a bug report to the vendor.

84,mmERR_FATAL_THREAD_CONDITION_DESTROY =  Failed to invoke the cond_destroy() system function
# *Cause: Use of invalid condition value
# *Action: Please send a bug report to the vendor.

# 86,mmERR_ABORT_CLIENT_CLOSED = Unable to send data to client.
# *Cause: Client socket already closed.
# *Action:  Please send a bug report to the vendor.

87,mmERR_ABORT_MMC_CANT_LOCK_TABLE_IN_AUTOCOMMIT_MODE = Unable to execute LOCK TABLE statement while the system is in auto-commit mode.
# *Cause: The system tried to execute a LOCK TABLE statement while the system was in auto-commit mode.
# *Action: When the system is in auto-commit mode, Remove the LOCK TABLE statement.

89,mmERR_ABORT_NO_AVAILABLE_TASK = Task pool overflow. Check properties.
# *Cause: There are no more available tasks for service.
# *Action: Increase the value of the MAX_CLIENT property and restart the database.

90,mmERR_ABORT_OTHER_STATEMENT_REMAINS = Several statements still open
# *Cause: There are a few opened statements.
# *Action: Close all statements and retry the statement.

94,mmERR_ABORT_BIND_COLUMN_COUNT_MISMATCH = Mismatched bind column count
# *Cause: The number of host variables and the number of data items to bind to them are different.
# *Action: Check client applications.

95,mmERR_ABORT_INVAILD_STACKCOUNT = Invalid stack count
# *Cause: The stack count is too big or too small.
# *Action: Set the stack count to be a positive integer less than 65536.

96,mmERR_FATAL_OSFileSizeLimit_ERROR = The maximum file size of the OS is less than that specified in the property. Decrease the file size property.
# *Cause: The maximum file size of the OS is less than that specified in the property.
# *Action: Please decrease the file size property.

97,mmERR_ABORT_TOO_LONG_IDENTIFIER_NAME = The length of the identifier is too long
# *Cause: Identifier too long.
# *Action: Give the identifier a shorter name.

# 100,mmERR_ABORT_ARRAYBINDING_NOT_ALLOWED_SELECT = Unable to execute SELECT using array-binding
# *Cause: The user has executed a SELECT query with array-binding.
# *Action: Do not execute a select query using array-binding.

# 101,mmERR_ABORT_ARRAY_COUNT_MISMATCH = Mismatched bind array count
# *Cause: The number of elements on an array host variable and the number of data items to bind to them are different.
# *Action: Check client applications.

# 102,mmERR_ABORT_PROTOCOL_ORDER_MISMATCH = Mismatched communication protocol sequence
# *Cause: Protocol Sequence is invalid.
# *Action: Please send a bug report to the vendor.

# 103,mmERR_ABORT_BATCH_COUNT_MISMATCH = Mismatched batch execute statement count
# *Cause: Protocol Sequence is invalid.
# *Action: Please send a bug report to the vendor.

104,mmERR_FATAL_THREAD_CONDITION_WAIT =  Failed to invoke the cond_wait() system function
# *Cause: The system failed to invoke the cond_wait() function.
# *Action: Please send a bug report to the vendor.

106,mmERR_FATAL_THREAD_CONDITION_BROADCAST =  Failed to invoke the cond_broadcast() system function
# *Cause: The system failed to invoke the cond_broadcast() function.
# *Action: Please send a bug report to the vendor.

# 107,mmERR_ABORT_STATISTICS_INDEX_MISMATCH = Unable to match query-statistics index
# *Cause: The query statistics index does not match the accumulated information structure.
# *Action: Retry with an appropriate index.

108,mmERR_FATAL_SOCKET_CREATE_FAILED = Unable to create socket
# *Cause: Unable to create a socket.
# *Action: Too many files are open. Please change the system parameter.

109,mmERR_FATAL_SOCKET_BIND_FAILED = Unable to bind socket
# *Cause: Either the port is already bound or it is impossible to bind the port due to a system limitation.
# *Action: Please use another port.

110,mmERR_ABORT_ADMIN_MODE_ERROR = If ADMIN_MODE is activated, only the users SYS and SYSTEM_ can connect to the database with SYSDBA privileges. Verify that you have SYSDBA privileges.
# *Cause: When ADMIN_MODE is on, only the SYS and SYSTEM_ users can connect to DB using the SYSDBA option.
# *Action: Check ADMIN_MODE property.

# 111,mmERR_ABORT_MMI_DUPLICATED_LIBRARY_NAME = Duplicate library name <0%s>
# *Cause: The library name to be created already exists.
# *Action: Use another library name.

# 112,mmERR_ABORT_MMI_NOT_EXIST_LIBRARY = Library not found <0%s>
# *Cause: The specified library was not found.
# *Action: Verify that the library exists.

# 113,mmERR_ABORT_MMI_NOT_EXIST_MODULE = Module not found <0%s>
# *Cause: The specified module was not found.
# *Action: Verify that the module exists.

# 114,mmERR_ABORT_MMI_DUPLICATED_MODULE_NAME = Duplicate module names <0%s>
# *Cause: A module with the same name as the module to be created already exists.
# *Action: Use another module name.

# 115,mmERR_ABORT_MMI_DUPLICATED_UNIQUE_NAME = Duplicate unique module name <0%s>
# *Cause: A module with that unique name already exists.
# *Action: Use another unique module name.

# 116,mmERR_ABORT_MMI_NOT_EXIST_UNIQUEMODULE = Unique module name not found <0%s>
# *Cause: The specified unique module name was not found.
# *Action: Verify that the unique module name exists.

# 117,mmERR_ABORT_MMI_ALREADY_STOP_MODULE = Unable to stop a module that has already been stopped.
# *Cause: The specified module has already been stopped.
# *Action: Verify that the module is running before attempting to stop it.

118,mmERR_ABORT_MMI_NOT_IMPLEMENTED = Not supported yet
# *Cause: Not supported yet.
# *Action: Not supported yet.

# 119,mmERR_ABORT_MMT_INVALID_PACKET = Invalid packet
# *Cause: An invalid network packet has been detected.
# *Action: Check the network status.

# 121,mmERR_ABORT_SEA_INVALID_ARG_VALUE = Argument value too long
# *Cause: The length of the argument value must be shorter than 256.
# *Action: Check the argument value.

122,mmERR_ABORT_STARTUP_PHASE_ERROR = Unable to start up in the specified phase in the current state.
# *Cause: It is impossible to start up in the specified phase in the current startup state.
# *Action: Check the current startup phase.

123,mmERR_ABORT_SHUTDOWN_MODE_ERROR = Unable to shut down Altibase using the specified mode in the current state.
# *Cause: It is impossible to shut down the server using the specified mode in the current state.
# *Action: Check the current startup phase.

124,mmERR_ABORT_INSUFFICIENT_PRIV = Insufficient privileges. The user has to connect as SYSDBA.
# *Cause: The user has to connect as SYSDBA.
# *Action: Check your privileges.

125,mmERR_ABORT_CREATE_DB_ERROR = Failure to CREATE DATABASE
# *Cause: The system failed to create a database.
# *Action: Check the error message.

126,mmERR_ABORT_DROP_DB_ERROR = Failure to DROP DATABASE
# *Cause: The system failed to drop a database.
# *Action: Check the error message.

127,mmERR_ABORT_ADMIN_MODE_PRIV = Failure changing to ADMIN_MODE. Only SYS and SYSTEM_ users can change the current mode to ADMIN_MODE.
# *Cause: Only the SYS and SYSTEM_ users can change the current mode to ADMIN_MODE.
# *Action: Check the connected user.

128,mmERR_ABORT_INVALID_SESSION_ID = Invalid Session ID <0%d>
# *Cause: It is impossible to find the session ID. Use a valid session ID.
# *Action: Verify that a session with the specified ID exists.

# 129,mmERR_FATAL_INTERNAL_SERVER_ERROR_ARG =  Internal server error (<0%s>)
# *Cause:
# - The System encountered an internal server error.
# *Action
# - Please send a bug report to the vendor.

130,mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG = Internal server error (<0%s>)
# *Cause: The system encountered an internal server error. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

131,mmERR_ABORT_NOT_UPDATE_PROPERTY = A specified property does not exist or cannot be updated.
# *Cause: A specified property does not exist or cannot be updated.
# *Action: Check the property.

# 132,mmERR_FATAL_MEMORY_ERROR_ARG2 =  Not enough memory. <0%s> ( requested size : <1%d> )
# *Cause:
# - Not enough memory.
# *Action:
# - Please make sure that the system has enough system memory.

133,mmERR_FATAL_PASSWORD_FILE_ERROR = Password file open error
# *Cause: Password file open error.
# *Action: Plase check your environment.

# 134,mmERR_FATAL_CRYPT_NOT_SUPPORTED =  Unsupported encryption scheme.
# *Cause:
# - Internal server error
# *Action:
# - Please send a bug report to the vendor.

135,mmERR_ABORT_ADMIN_NOT_ACCEPTED = Failed to connect through a non-SYSDBA channel
# *Cause: The user has to connect through a SYSDBA channel.
# *Action: Change the connection channel type.

136,mmERR_ABORT_SERVER_PHASE_MISMATCHES_QUERY_TYPE = Unable to execute the query in the current start-up phase
# *Cause: This query cannot be executed while the server is in the current state.
# *Action: After proceeding to a suitable startup phase, try again.

137,mmERR_ABORT_MMC_DATE_FORMAT_LENGTH_EXCEED = The length of the value of the DEFAULT_DATE_FORMAT property equals or exceeds the maximum length ( <0%d> ).
# *Cause: DEFAULT_DATE_FORMAT string is too long.
# *Action: Execute the ALTER SESSION statement with a valid DEFAULT_DATE_FORMAT string.

138,mmERR_ABORT_QUEUE_NOT_FOUND = Queue not found
# *Cause: The queue is not in the queue manager. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

# 139,mmERR_ABORT_DATABASE_ALREADY_EXIST = Database already exists (<0%s>)
# *Cause: The database already exists.
# *Action: Drop the database first.

140,mmERR_ABORT_GETRLIMIT_ERROR =  Failed to invoke the getrlimit() system function
# *Cause: An error occurred while invoking the getrlimit() function. This is an OS (system call error or library function) error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

141,mmERR_ABORT_OSFileSizeLimit_ERROR = The maximum file size of the OS is less than that specified in the property. Decrease the file size property.
# *Cause: The maximum file size of the OS is less than that specified in the property.
# *Action: Decrease the file size property.

142,mmERR_ABORT_DATABASE_NAME_LENGTH_ERROR = Invalid Database Name Length. Valid Database Name Length = 1 ~ <0%d>.
# *Cause: The database name length is invalid.
# *Action: Use a database name with a valid length.

143,mmERR_ABORT_PAGE_RANGE_ERROR = Invalid createdb initial size: <0%lu> bytes. Valid size range = <1%lu> ~ <2%lu> bytes.
# *Cause: The initial DB size specified for database creation is invalid.
# *Action: Specify an initial DB size within the valid range.

144,mmERR_ABORT_DATABASE_NAME_ERROR = Invalid Database Name. Check the properties and retry.
# *Cause: Invalid database name.
# *Action: Check the properties and retry.

145,mmERR_ABORT_SHM_ALREADY_EXIST_ERROR = Shared Memory Already Exist. Remove Shared Memory First.
# *Cause: Shared memory already exists.
# *Action: Remove shared memory first.

# 150, mmERR_ABORT_INVALID_DISPATCHER_IMPL = Invalid dispatcher implementation
# *Cause: Invalid dispatcher implementation
# *Action: Please send a bug report to the vendor.

152, mmERR_ABORT_STATEMENT_NOT_FOUND = Failure to find statement
# *Cause: It is impossible to find the statement. Use a valid statement.
# *Action: Verify that the statement is valid.

153, mmERR_ABORT_TOO_MANY_SESSION = There are too many sessions
# *Cause: Too many sessions are connected to this system.
# *Action: Disconnect unused sessions or increase the MAX_CLIENT property.

154, mmERR_ABORT_SESSION_NOT_SPECIFIED = Session is not specified in protocol
# *Cause: The session is not specified in the protocol. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

155, mmERR_ABORT_SESSION_NOT_CONNECTED = Session is not connected
# *Cause: The session is not connected. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

156, mmERR_ABORT_INVALID_SESSION_PROPERTY = Invalid session property
# *Cause: Invalid session property.
# *Action: Use a valid session property.

157, mmERR_ABORT_INVALID_EXECUTE_OPTION = Invalid execute option
# *Cause: Invalid execute option. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

158, mmERR_ABORT_INVALID_LOB_RANGE = Invalid lob range
# *Cause: Invalid lob range.
# *Action: Check the lob size and use a valid lob range.

159, mmERR_ABORT_INVALID_DATA_CONVERSION = Invalid data conversion
# *Cause: Invalid data conversion. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

160, mmERR_ABORT_INVALID_BIND_PARAMETER_NUMBER = Invalid bind parameter number
# *Cause: Invalid number of binding parameters.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

161, mmERR_ABORT_ADD_TASK_TIMED_OUT = Add task timed out
# *Cause: A timeout occurred while attempting to add a task. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

162, mmERR_ABORT_NUMERIC_PRECISION_OVERFLOW = Numeric precision overflow
# *Cause: Numeric precision overflow
# *Action: Numeric precision overflow

163, mmERR_ABORT_NUMERIC_SCALE_OVERFLOW = Numeric scale overflow
# *Cause: Numeric scale overflow
# *Action: Numeric scale overflow

164, mmERR_IGNORE_NO_COLUMN =  No column
# *Cause: There was no column in the specified statement.
# *Action: Ignore this error message.

165, mmERR_ABORT_INVALID_STATEMENT_FREE_MODE = Invalid statement free mode
# *Cause: The client set an invalid free mode for the statement. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

166, mmERR_ABORT_UNSUPPORTED_FETCHMOVE = Unsupported fetch move operation
# *Cause: Unsupported fetch move operation. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

167, mmERR_IGNORE_NO_CURSOR =  No cursor
# *Cause: There was no cursor in the specified statement.
# *Action: Ignore this error message.

210, mmERR_ABORT_FETCH_OUT_OF_SEQ = Fetch out of sequence.
# *Cause: A fetch was attempted using a cursor that is no longer valid.
# *Action: Do not execute the COMMIT or ROLLBACK statement in a fetch loop.

168, mmERR_IGNORE_NO_PARAMETER =  No parameter
# *Cause: There was no parameter in the specified statement.
# *Action: Ignore this error message.

169, mmERR_ABORT_INVALID_BIND_COLUMN_NUMBER = Invalid bind column number
# *Cause: Invalid bind column number. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

170, mmERR_ABORT_INVALID_XA_OPERATION = Invalid XA operation
# *Cause: Invalid XA operation. This is an internal programming exception.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

171, mmERR_ABORT_NUMERIC_CONVERSION_OVERFLOW = Numeric conversion overflow
# *Cause: An overflow occurred while converting the numeric data type.
# *Action: Use the correct numeric precision.

172, mmERR_IGNORE_UNABLE_TO_MAKE_LISTENER = Unable to make listener
# *Cause: Unable to make listener.
# *Action: Ignore this error message.

173, mmERR_ABORT_DATA_CONVERSION_OVERFLOW = Data conversion overflow
# *Cause: Data conversion overflow.
# *Action: The data cannot be stored in the database as this data type.

174, mmERR_ABORT_INVALID_PRECISION = Invalid precision or length
# *Cause: Invalid precision or length.
# *Action: Specify a suitable precision or length.

175, mmERR_ABORT_PRECISION_MISMATCH = Precision mismatch
# *Cause: Precision mismatch.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

176, mmERR_ABORT_UNSUPPORTED_NETWORK_PROTOCOL = Unsupported network protocol
# *Cause: Unsupported network protocol.
# *Action: Use a supported network protocol.

177, mmERR_ABORT_INVALID_SESSION_STATE = Invalid request in current session state
# *Cause: The request was invalid for the current session state.
# *Action: Check the client application.

178, mmERR_ABORT_TASK_ALLOC_FAILED = Task allocation failed
# *Cause: Task memory allocation failed.
# *Action: Verify that enough memory is available.

# 179, mmERR_ABORT_INVALID_COMMIT_WRITE_WAIT = Invalid COMMIT_WRITE_WAIT
# *Cause: Can't have COMMIT_WRITE_NOWAIT in parallel logging.
# *Action: Check COMMIT_WRITE_WAIT mode.

# 180, mmERR_ABORT_USRDN_ISNULL = User DN (Distinguished Name) is null.
# *Cause: The DN (Distinguished Name) in the user certificate is null.
# *Action: Check the user certificate.

# 181, mmERR_ABORT_SVRDN_ISNULL = Server DN (Distinguished Name) is null.
# *Cause: The DN (Distinguished Name) in the server certificate is null.
# *Action: Check the server certificate.

182, mmERR_ABORT_DNMismatched = The DNs (Distinguished Names) in the Server and User Certificates are different.("<0%s>" != "<1%s>")
# *Cause: The DNs (Distinguished Names) in the server and user certificates are different.
# *Action: Check both the server and client certificates.

183, mmERR_ABORT_INVALID_DATA_SIZE = Invalid size of data to bind to a host variable [ Param ID = <0%d>, Data Type = <1%s>, Data Size = <2%d>, Declared Size of Host Variable = <3%d> ]
# *Cause: The size of the data and the size of the host variable differ.
# *Action: Check the size of the data to be bound to the host variable.

184, mmERR_ABORT_INVALID_SCALE_SIZE = Invalid scale of data to bind to a host variable [ Scale Size = <0%d> , Declared Scale of Host Variable = <1%d> ]
# *Cause: The scale of the data and the scale of the host variable differ.
# *Action: Check the scale of the data to be bound to the host variable.

185, mmERR_FATAL_ThrCondWait =  Failed to invoke the cond_wait() system function.
# *Cause: Invalid condition value or invalid mutex.
# *Action: Please send a bug report to the vendor.

186, mmERR_ABORT_NO_ALERTS_REGISTERED = There are no alerts registered.
# *Cause: There are no alerts registered.
# *Action: Register alerts.

188, mmERR_ABORT_NOT_EXIST_XID = A specified XID does not exist.
# *Cause: A specified XID does not exist.
# *Action: Check the XID.

189, mmERR_ABORT_INVALID_XA_SESSION = Do after xa_open.
# *Cause: This session is not an XA session.
# *Action: Check the XID.

190, mmERR_ABORT_NOT_ASSOCIATED_BRANCH = A specified XID is not associated.
# *Cause: A specified XID is not associated.
# *Action: Check the XID.

191, mmERR_ABORT_INVALID_XA_STATE = State transition error (curr:<0%s>).
# *Cause: The function routine was invoked in an improper context.
# *Action: Check the function routine.

192, mmERR_ABORT_XID_INUSE = The specified XID is in use.
# *Cause: The specified XID is in use.
# *Action: Check the XID.

193, mmERR_ABORT_ALREADY_EXIST_XID = The specified XID already exists.
# *Cause: The specified XID already exists.
# *Action: Check the XID.

194, mmERR_ABORT_INVALID_XID = Invalid XID.
# *Cause: The specified XID does not exist.
# *Action: Check the XID.

195, mmERR_ABORT_NOT_ALLOWED_DCL = A DCL SQL command is not allowed in an xa session.
# *Cause: A DCL SQL statement is not allowed in a global transaction.
# *Action: Do not use the DCL SQL command.

196, mmERR_ABORT_NOT_ALLOWED_DDL = A DDL SQL command is not allowed in an xa session.
# *Cause: The Altibase XA application cannot execute any DDL SQL statements.
# *Action: Do not use the DDL SQL command.

197, mmERR_ABORT_SERVER_ALREADY_STARTED = The server is already started.
# *Cause: The server is already started.
# *Action: Stop the server.

198, mmERR_ABORT_ATOMIC_EXECUTE_ERROR = Atomic Execute Error
# *Cause: Atomic execute error.
# *Action: Check the data or the statement.

199,mmERR_ABORT_DATABASE_NAME_ASCII_ERROR = Invalid Character Set for Database Name.
# *Cause: Invalid database name character set.
# *Action: Use an ASCII character set for the database name.

# 200,mmERR_ABORT_INSUFFICIENT_PRIV_SYS = Insufficient privileges. The user has to connect as a SYS user.
# *Cause: The user has to connect as a SYS user.
# *Action: Check the connected user.

201,mmERR_ABORT_REMOTE_SYSDBA_NOT_ALLOWED = Remote access as SYSDBA not allowed
# *Cause: The REMOTE_SYSDBA_ENABLE property is set to 0.
# *Action: Set the REMOTE_SYSDBA_ENABLE property to 1.

# 202,mmERR_ABORT_LinkerExistAlready = A linker process is already running
# *Cause: A linker process is already running.
# *Action: Check the status of the linker process.

203,mmERR_ABORT_MMC_NOT_SUPPORT_REPL_MODE = This replication mode is not supported at the session level.
# *Cause: The replication modes (EAGER, ACKED, LAZY) do not pertain to sessions.
# *Action: No action is necessary.

204,mmERR_ABORT_NullSourceData = "NULL source data were found when converting from MT source data to CM data.";
# *Cause: Conversion failed because the source data received as an argument was NULL. This is an internal error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

205,mmERR_ABORT_InvalidMantissaLength = "The mantissa length ( <0%d> ) of the source data exceeds the maximum mantissa length.";
# *Cause: Conversion failed because the data type value was invalid.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

206,mmERR_ABORT_InvalidArrayBinds = "Array binding is not permitted with SELECT statements.";
# *Cause: An attempt was made to bind an array when executing a SELECT statement.
# *Action: Do not attempt to bind an array when executing a SELECT statement.

207,mmERR_ABORT_TOO_MANY_STATEMENTS_IN_THE_SESSION = There are too many statements in the session.
# *Cause: Too many statements have been allocated to this session.
# *Action: Change the value of the MAX_STATEMENTS_PER_SESSION property or remove unused statements.

208,mmERR_ABORT_STATEMENT_NUMBER_EXCEEDS_INPUT_VALUE = The number of allocated statements exceeds the input maximum value.
# *Cause: The number of allocated statements exceeds the input maximum value.
# *Action: Enter a larger maximum value.

209,mmERR_ABORT_TOO_LONG_PASSWORD_ERROR = Password too long
# *Cause: The input password exceeded the maximum allowable length.
# *Action: Enter a shorter password.

211,mmERR_ABORT_MMC_TIMEZONE_LENGTH_EXCEED = TIME_ZONE string too long
# *Cause: The length of the TIME_ZONE property exceeds the maximum length.
# *Action: Execute the ALTER SESSION statement with a valid TIME_ZONE string.

212,mmERR_ABORT_EXCEEDS_DEDICATED_THREAD_MAX_COUNT = The number of dedicated threads exceeds the maximum limit
# *Cause: The number of dedicated threads exceeds the maximum limit.
# *Action: Change the value of the DEDICATED_THREAD_MAX_COUNT property.

213,mmERR_ABORT_FAIL_TO_ESTABLISH_CONNECTION = Client unable to establish connection. (<0%s>)
# *Cause: An attempt to connect as a normal user failed.
# *Action: Try to connect as sysdba.

214,mmERR_ABORT_SHUTDOWN_NOT_SUPPORTED_VIA_IPC = Not Supported via IPC.
# *Cause: Executing the command via IPC is not supported.
# *Action: Try connecting via UNIXDOMAIN. 'SHUTDOWN EXIT' can be executed via IPC.

# 215,mmERR_ABORT_CPU_AFFINITY_GETCONF_ERROR = Failed to get CPU affinity configuration
# *Cause: Failed to get the CPU affinity configuration
# *Action: Check your platform.

216,mmERR_ABORT_DBMS_NOT_FOUND = DB not found : <0%s>
# *Cause: The specified database does not exist.
# *Action: Verify that the specified database exists.

# 217,mmERR_FATAL_STARTUP_WITHOUT_DR_ENABLE_ERROR = If you want startup standby server, you have to set DR_ENABLE property.
# *Cause: startup without DR_ENABLE.
# *Action: Please set DR_ENABLE and reboot.

# 218,mmERR_ABORT_STARTUP_WITHOUT_DR_SERVER_LIST_ERROR = If you want startup standby server, you have to input dr server list.
# *Cause: Startup without a DR server list.
# *Action: Input a DR server list and retry.

219,mmERR_IGNORE_UNSUPPORTED_PROPERTY =  Unsupported property : [<0%d>].
# *Cause: Unsupported property was used.
# *Action: Upgrade the server to the latest version.

# 220,mmERR_ABORT_NEED_FULL_SYNC = Invalid failover server count. Re-sync the active server with failover server(s) then restart the active server.
# *Cause: Failed server activation due to synchronization errors between active server(s) and failover server(s) due to two or more failovers occurring before the active server is restarted.
# *Action: Re-synchronize the active server with failover server(s) then restart the active server.

221,mmERR_ABORT_EXCEEDS_MAX_SESSION =  The number of generable sessions is set at a larger value than the maximum value of MAX_CLIENT.
# *Cause: The number of generable sessions is set at a larger value than the maximum value of MAX_CLIENT.
# *Action: Decrease the value of the JOB_THREAD_COUNT or the MAX_CLIENT property.

222,mmERR_IGNORE_ONLY_SYSDBA_CANDOTHAT = Only sysdba can do that.
# *Cause: Tried to change a property that only sysdba can change.
# *Action: Login as sysdba and try again.

223,mmERR_IGNORE_THREAD_COUNT_UNCHANGED = Maximum number of threads cannot be changed to smaller value.
# *Cause: Tried to set the number of threads to a value lower than the current value.
# *Action: Do not attempt to set the threads to a value lower than the current value.

#224,mmERR_ABORT_DR_STANDBY_NO_META_PHASE = The Disaster Recovery standby server cannot start up in the META or SERVICE phase.
# *Cause: The disaster recovery standby server cannot start up in the META or SERVICE phase.
# *Action: Check whether or not the server is a DR standby server.

225,mmERR_IGNORE_PASSWORD_GRACE_PERIOD = The password will expire within <0%d> day(s).
# *Cause: The password expiration date is approaching.
# *Action: Change your password.

226,mmERR_FATAL_THREAD_CREATE_FAILED = Unable to create thread
# *Cause: The system failed to create threads.
# *Action:  Please send a bug report to the vendor.

227,mmERR_ABORT_DISABLED_TCP_USER = The user cannot connect using TCP. 
# *Cause: This user cannot connect using TCP.
# *Action: Please connect using SSL/UNIX domain/IPC. JAVA users can change the connection method in the application (for iSQL, change the value of the iSQL_CONNECTION environment variable).

228, mmERR_ABORT_GET_IPV6_INFO = Failed to get IPV6 infomation. 
# *Cause: Failed to get IPv6 information. 
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

229, mmERR_FATAL_MUTEX_INIT = Unable to initialize a mutex.
# *Cause: The system failed to initialize a mutex.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

230, mmERR_FATAL_LATCH_INIT = Unable to initialize a latch.
# *Cause: The system failed to initialize a latch.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

231,mmERR_ABORT_VALUE_LENGTH_EXCEED = The length of the value set for <0%s> exceeds the limit ( <1%d> ).
# *Cause: Input value is too long.
# *Action: Verify that the length of the input value does not exceed the limit.

232,mmERR_ABORT_InvalidProcedure = The ARRAY INSERT statement cannot be used with a SELECT statement that returns a ResultSet in stored procedures
# *Cause: The stored procedure contains both the ARRAY INSERT statement and a SELECT statement that returns a ResultSet.
# *Action: Verify that the stored procedure does not contain both the ARRAY INSERT statement and a SELECT statement that returns a ResultSet.

233, mmERR_ABORT_IP_ACL_DENIED = Connection is not permitted by the ACCESS_LIST: <0%s> ( IP : <1%s> )
# *Cause: ACCESS_LIST is set DENY.
# *Action: Check and modify to PERMIT on the ACCESS_LIST.

234, mmERR_ABORT_INVALID_DATA_TYPE = Invalid type of data to bind to a host variable [ Param ID = <0%d>, Data Type = <1%d> ]
# *Cause: The data type to be bound is not valid.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

235, mmERR_ABORT_NO_SESSION_TO_CLOSE = The session to close cannot be found.
# *Cause: No session to close exists anymore.
# *Action: Ascertain ID, user, and status of the session in V$SESSION. 

236, mmERR_ABORT_CANNOT_CLOSE_SELF_SESSION = The self-accessed session cannot be closed.
# *Cause: Closing the session is impossible since it is currently being accessed.
# *Action: Re-close the session after identifying the session ID.

237, mmERR_ABORT_SESSION_CLOSE_NOT_PERMITTED = Failed to close the session(s). Permission denied.
# *Cause: The authority for closing a session is the responsibility of SYS user.
# *Action: Login as SYS user then close the session.

238, mmERR_ABORT_INVALID_ACCESS_LIST_FILE = Failed to open or close the access list file : <0%s> - <1%s>.
# *Cause: The ACCESS_LIST_FILE property is led to an invalid file.
# *Action:
# - 1. Check to verify whether the file ACCESS_LIST_FILE property leads exists or not.
# - 2. Check the error number in the trace log file.

239, mmERR_ABORT_EXCEEDED_ACCESS_LIST_COUNT_LIMIT = The access list file has items exceeding the maximum limit.
# *Cause: The number of items in the access list file exceed the maximum limit which is 1024.
# *Action: Reduce the number of access list items to be less than the maximum limit.

240, mmERR_ABORT_RELOAD_ACL_NOT_PERMITTED = Failed to reload the access list.
# *Cause:The ACCESS_LIST_FILE property is omitted or disabled.
# *Action: Enable the ACCESS_LIST_FILE property and reboot the DBMS.

241, mmERR_ABORT_INVALID_ACCESS_LIST_VALUE = The value of ACCESS_LIST is not acceptable : (Line <0%d>) <1%s>
#*Cause: The data in the access list file is not compatible with the specified ACCESS_LIST format.
#*Action: Modify the data to comply with the ACCESS_LIST format.

242,mmERR_ABORT_INVALID_SNAPSHOT_SCN = This is a snapshot SCN which cannot execute SELECT.
# *Cause: SCN cannot execute SELECT through a snapshot if SCN has been changed # by a DDL occurred on a relevant table or iLoader is executed by creating a new # table after starting the snapshot.
# *Action: Re-start the snapshot(ALTER DATABASE BEGIN SNAPSHOT;) after terminating the snapshot currently running(ALTER DATABASE END SNAPSHOT;).

243,mmERR_ABORT_INVALID_BEGIN_SNAPSHOT = Snapshot cannot be started since an iLoader session exists.
# *Cause: An iLoader session exists. Snapshot is unable to run since it might affect the existing iLoader session.
# *Action: Terminate the current iLoader session, and then re-start to start the snapshot.

244,mmERR_ABORT_INVALID_SNAPSHOT_THRESHOLD = Snapshot cannot be working since the database exceeds maximum threshold value.
# *Cause: Snapshot cannot be able to run since the capacity of memory or disk undo space has exceeded the maximum threshold value.
# *Action: Adjust the property value of SNAPSHOT_MEM_THRESHOLD, SNAPSHOT_DISK_UNDO_THRESHOLD after checking the amount of memory use and undo space in V$SNAPSHOT to secure space for the maximum threshold value.

245,mmERR_ABORT_BEGIN_SNAPSHOT = Snapshot has already been being started.
# *Cause: There should be no snapshot being executed in order to start the snapshot.
# *Action: Re-start (ALTER DATABASE BEGIN SNAPSHOT;)the snapshot after terminating(ALTER DATABASE END SNAPSHOT;) the snapshot which is currently running.

246,mmERR_ABORT_END_SNAPSHOT = Snapshot has been ended.
# *Cause: Snapshot has already been ended.
# *Action: Retry after executing the statement ALTER DATABASE END SNAPSHOT.

247,mmERR_ABORT_INVALID_WORKING_SNAPSHOT = Snapshot does not work.
# *Cause: Snapshot is not running. The snapshot should be started in order for it to work.
# *Action: Retry after executing the snapshot(ALTER DATABASE BEGIN SNAPSHOT;).

248,mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG = The message size exceeds the maximum IPCDA buffer size(<0%d>).
# *Cause: The size of message to send exceeds the maximum size of IPCDA buffer.
# *Action: Use another connection type.

249,mmERR_ABORT_IPCDA_UNSUPPORTED_QUEUE = Unsupported IPCDA for queue.
# *Cause: The queue(enqueue/dequeue) is not supported in IPCDA.
# *Action: Set another communication protocols. Altibase supports the following communication besides IPCDA ; TCP/IP, Unix Domain Socket, IPC using Shared Memory, SSL/TLS.

#250,mmERR_ABORT_SESSION_WITH_INVALID_SMN = The shard meta number(SMN) is invalid.: [ Session SMN=<0%lu>, Data Node SMN=<1%lu>, Disconnect=<2%s>, Protocol=<3%s> ]
# *Cause: The shard meta number(SMN) of the data node is greater than the SMN of the session.
# *Action: Reconnect to the meta node.

251,mmERR_ABORT_PLAN_CACHE_INVALID_SQL_TEXT_ID = SQLTextID must be a number of at least five digits. (SQLTextID = <0%s>)
# *Cause: SQLTextID does not match the format that you can enter.
# *Action: SQLTextID must be a number of at least five digits.

252,mmERR_ABORT_PLAN_CACHE_NOT_FOUND_SQL_TEXT_ID = SQLTextID not found. (SQLTextID = <0%s>)
# *Cause: The SQLTextID does not exist.
# *Action: Check whether SQLTextID exists in V$SQL_PLAN_CACHE_SQLTEXT.

253,mmERR_ABORT_SHARED_TRANSACTION_STATE_INVALID = The state of shared transaction is invalid. (State = <0%s>, Action = <1%s>)
# *Cause: This is an internal error related to unexpected state of shared transaction.
# *Action: Please contact Altibase's Support Center (http://support.altibase.com).

254,mmERR_ABORT_SHARD_ADMIN_MODE_ERROR = If SHARD_ADMIN_MODE is activated, only the users SYS and SYSTEM_ can connect to the database.
# *Cause: When SHARD_ADMIN_MODE is on, only the SYS and SYSTEM_ users can connect to DB.
# *Action: Connect to the database as SYS or SYSTEM_.

255, mmERR_ABORT_ALTER_SESSION_NOT_ALLOW = ALTER SESSION statement is not allowed to execute on a specific session.
# *Cause:  ALTER SESSION statement is not allowed to execute on a specific session.
# *Action: Check the statement.

256, mmERR_ABORT_GCTX_NOT_PERMIT = The client library version does not support the Global Consistent Transaction level.
# *Cause: The client library version does not support the Global Consistent Transaction level.
# *Action: Upgrade the client library version which supports the Global Consistent Transaction level.

257, mmERR_ABORT_GLOBAL_TRANSACTIONS_ARE_OPEN = Global transactions are not finished to run.
# *Cause: Failed to shutdown Server, because there are running global transactions.
# *Action: Terminate the running global transactions, or execute 'SHUTDOWN ABORT'. 

258, mmERR_ABORT_CANNOT_CHANGE_AUTOCOMMIT_IN_SHARD_ENV = Unable to change to auto-commit mode in shard environment.
# *Cause: Unable to change to auto-commit mode in shard environment.
# *Action: Do not attempt to change to auto-commit mode in shard environment.

259, mmERR_ABORT_CONNECT_ERROR_IN_SHARD_ENV= In shard environment, a client set to autocommit cannot connect to the database.
# *Cause: In shard environment, a client set to autocommit cannot connect to the database.
# *Action: Set to non-autocommit mode.  

260, mmERR_RETRY_SHARD_INTERNAL_STATEMENT_IS_TOO_OLD = The records required for consistent reading are out of date.
# *Cause: The records required for consistent reading are out of date.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

261, mmERR_REBUILD_SHARD_INTERNAL_SHARD_META_OUT_OF_DATE = Shard meta information of session is out of date.
# *Cause: Shard meta information has been changed.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

262, mmERR_ABORT_NO_SHARD_META_CHANGE_TO_REBUILD = No shard meta information changes to perform shard rebuild. (<0%s>)
# *Cause: There are no more shard meta information changes to perform shard rebuild.
# *Action: Check the shard configuration information.

263, mmERR_ABORT_COMMIT_ERROR = In shard environment, commit failed.
# *Cause: This is an internal error.
# *Action: Please contact Altibase's Support Center (http://support.altibase.com).

264, mmERR_ABORT_ROLLBACK_ERROR = In shard environment, rollback failed.
# *Cause: This is an internal error.
# *Action: Please contact Altibase's Support Center (http://support.altibase.com).

265, mmERR_ABORT_IP_ACL_CONNECT_OVER = New connection try exceeds ACL limit: ( IP : <0%s>, ACL: <1%s>, Limit : <2%d>, Connected : <3%d> )
# *Cause: New connection try aborted because it exceeds access control list (ACL) limit.
# *Action: Change the limit value of the ACCESS_LIST.

INTERNAL_TRACE_MESSAGE_BEGIN

MM_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
                "signed long =<%ld>\n unsigned long = <%lu>\n"
                "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>\n";

MM_TRC_MAX_HANDLE_EXCEED_FD_SETSIZE = "[FATAL] MaxHandle(<%d>) exceeds FD_SETSIZE(<%d>)\n";

MM_TRC_SMALL_SIZE_OF_FD_SET = "[FATAL] Size of fd_set(<%d>) is smaller than (FD_SETSIZE / 8)(<%d>)\n";

MM_TRC_MAX_HANDLE_EXCEED_MAX_CLIENT = "[WARNING] Max Clients > Max Handles => Check Property...";

MM_TRC_GET_MAX_HANDLE_FAILED = "WARNING!!! Error getting the maximum number of handles";

MM_TRC_ENV_NOT_DEFINED = "WARNING : [<%s>] = Not Defined!!!";

MM_TRC_ENV_DEFINED = "DEFINED : [<%s>] = [<%s>]";

MM_TRC_DATABASE_FILE_DISK_FULL = "Database File Disk Partition Full";

MM_TRC_LOG_FILE_DISK_FULL = "Log File Disk Partition Full";

MM_TRC_SYSTEM_ABNORMAL_SHUTDOWN = "Abnormal System Shutdown!!";

MM_TRC_SYSTEM_UPTIME = "SYSTEM UPTIME => <%s>";

MM_TRC_DATABASE_PID = "ALTIBASE PROCESS ID => <%lu>";

MM_TRC_STARTUP = "[[[[[[ ALTIBASE STARTUP ]]]]]]";

MM_TRC_STARTUP_BOOT = "[[[[[[ ALTIBASE STARTUP - Boot ]]]]]]";

MM_TRC_SHUTDOWN = "[[[[[[ ALTIBASE SHUTDOWN ]]]]]]";

MM_TRC_SHUTDOWN_BOOT = "[[[[[[ ALTIBASE SHUTDOWN - Boot ]]]]]]";

MM_TRC_SHUTDOWN_NORMAL = "ALTER DATABASE SHUTDOWN NORMAL";

MM_TRC_SHUTDOWN_IMMEDIATE = "ALTER DATABASE SHUTDOWN IMMEDIATE";

MM_TRC_SHUTDOWN_EXIT = "ALTER DATABASE SHUTDOWN EXIT";

MM_TRC_DATABASE_INFO = "\n"
                       "DB Info (Page Size     = <%lu>)\n"
                       "        (Page Count    = <%lu>)\n"
                       "        (Total DB Size = <%lu>)\n"
                       "        (DB File Size  = <%lu>)\n";

MM_TRC_CREATE_DATABASE_BEGIN = "BEGIN: Database Creation...";

MM_TRC_CREATE_DATABASE_SUCCESS = "SUCCESS: Database Creation Completed...";

MM_TRC_CREATE_QP_META_BEGIN = "BEGIN: QP Meta Information Creation...";

MM_TRC_CREATE_QP_META_SUCCESS = "SUCCESS: QP Meta Information Creation Completed...";

MM_TRC_DATABASE_WRITING_BEGIN = "BEGIN: DB Writing (Page Count = <%lu>)";

MM_TRC_DATABASE_WRITING_SUCCESS = "SUCCESS: DB Writing Completed. All Done.";

MM_TRC_GETRLIMIT_FAILED = "ERROR: getrlimit() function call error";

MM_TRC_OS_FILE_SIZE_LIMIT = "ERROR: The maximum file size of the OS is less than that of property\n"
                            "Decrease the (db/log) file size property.";

MM_TRC_DATABASE_ALREADY_EXIST = "ERROR: [<%s>] Already Exists.";

MM_TRC_DATABASE_NAME_LENGTH_ERROR = "ERROR: Invalid Database Name Length\n"
                                    "Usage: Database Name Range = 1 ~ <%d>";

MM_TRC_DATABASE_NAME_ASCII_ERROR =  "ERROR: Invalid Database Name Character Set\n"
                                    "Usage: Database Name Character Set = ASCII";

MM_TRC_DATABASE_PAGE_RANGE_ERROR = "ERROR: Invalid initial size for createdb: <%lu> bytes. Valid size range = <%lu> ~ <%lu> bytes";

MM_TRC_DATABASE_NAME_ERROR = "ERROR: Invalid database name. Check properties and retry.";

MM_TRC_SMI_INIT_FAILED = "ERROR: smiInit() error occurred.";

MM_TRC_SMI_SHUTDOWN_FAILED = "ERROR: smiShutdown() error occurred.";

MM_TRC_DATABASE_CREATE_FAILED = "ERROR: Database creation failed !!!";

MM_TRC_SHM_ALREADY_EXIST = "ERROR: Can't create database.\n"
                           "Shared memory already exists.\n"
                           "Remove shared memory first.";

MM_TRC_WARNING = "[WARNING(errno=<%u>)] [<%s>:<%u> = <%s>]\n";

MM_TRC_SIGNAL1 = "[Abnormal Termination] : Signal[<%d>] Occurred\n\n";

MM_TRC_SIGNAL2 = "[Abnormal Termination] : Signal[<%d>:<%s>] Occurred\n\n";

MM_TRC_ASSERT = "[ASSERT] ERROR LINE => [<%s>:<%d>]\n";

MM_TRC_FATAL = "[FATAL] ERROR LINE => [<%s>:<%d>] MSG[<%s>]\n";

MM_TRC_FATAL_TERMINATED = "[========= FATAL Terminated ==========]\n\n";

MM_TRC_CURRENT_IDE_STACK = "====== CURRENT IDE ERROR STACK =====\n";

MM_TRC_CURRENT_SESSION_INFO_BEGIN = "============ Current Session Info Start =============\n\n";

MM_TRC_CURRENT_SESSION_INFO_END = "============ Current Session Info End ===============\n\n";

MM_TRC_CURRENT_SESSION = "--- Current Session ---\n";

MM_TRC_SESSION_OVERVIEW = "--- Session Overview ---\n"
                          "  Next SessionID        = [<%u>]\n"
                          "  Max Session Count     = [<%u>]\n"
                          "  Current Session Count = [<%u>]\n"
                          "  Service Thread Count  = [<%u>]\n";

MM_TRC_SESSION_MANAGER_SHUTDOWN_FAILED = "Session manager shutdown immediate failed. Task remaining...";

MM_SESSION_MANAGER_SHUTDOWN_TASK_REMAIN= "\nWhen shutting down ALTIBASE HDB, it is aborted by SHUTDOWN_IMMEDIATE_TIMEOUT property. Check the error message from altibase_boot.log.\n";

MM_TRC_SERVER_NOT_ABORTED_BY_SERVICE_SESSION = "The server must not be aborted by a service session.\n";

MM_TRC_TASK_COUNT_EXCEEDED = "The number of tasks has exceeded that specified in the MAX_CLIENT system property.";

MM_TRC_LOGIN_TIMEOUT = "Login timeout detected [<%s>]. Closing connection...";

MM_TRC_IDLE_TIMEOUT = "[Notify : Idle Timeout] "
                      "Session Closed by Server : Session ID = <%u>\n"
                      "    CLIENT_INFO           => <%s>(PID : <%lu>)\n"
                      "    Time Limit            => <%u>\n"
                      "    Running Time          => <%u>\n";

MM_TRC_QUERY_TIMEOUT = "[Notify : Query Timeout] "
                       "Query Canceled by Server : Session ID = <%u>\n"
                       "    CLIENT_INFO           => <%s>(PID : <%lu>)\n"
                       "    Time Limit            => <%u>\n"
                       "    Running Time          => <%u>\n"
                       "    Caused by Query       => <%s>\n";

MM_TRC_DDL_TIMEOUT = "[Notify : DDL Timeout] "
                       "DDL Query Canceled by Server : Session ID = <%u>\n"
                       "    CLIENT_INFO           => <%s>(PID : <%lu>)\n"
                       "    Time Limit            => <%u>\n"
                       "    Running Time          => <%u>\n"
                       "    Caused by DDL Query   => <%s>\n";

MM_TRC_FETCH_TIMEOUT = "[Notify : Fetch Timeout] "
                       "Session Closed by Server : Session ID = <%u>\n"
                       "    CLIENT_INFO           => <%s>(PID : <%lu>)\n"
                       "    Time Limit            => <%u>\n"
                       "    Running Time          => <%u>\n"
                       "    Caused by Query       => <%s>\n"
                       "    Transaction ID        => <%u>\n";

MM_TRC_UTRANS_TIMEOUT = "[Notify : UTrans Timeout] "
                        "Session Closed by Server : Session ID = <%u>\n"
                        "    CLIENT_INFO           => <%s>(PID : <%lu>)\n"
                        "    Time Limit            => <%u>\n"
                        "    Running Time          => <%u>\n"
                        "    Last Query            => <%s>\n"
                        "    Caused by Transaction => <%u>\n";

MM_TRC_SYSDBA_DISCONNECTED = "[WARNING] Sysdba disconnected.. but proceed";

MM_TRC_THREAD_MANAGER_ADD_TASK_FAILED = "Thread manager failed to add task";

MM_TRC_THREAD_MANAGER_START_THREAD_FAILED = "Thread manager failed to start new service thread";

MM_TRC_DISPATCHER_HAS_TOO_MANY_LINKS = "Too many links in dispatcher thread";

MM_TRC_DISPATCHER_ADD_LINK_FAILED = "Dispatcher thread failed to add link";

MM_TRC_DISPATCHER_REMOVE_LINK_FAILED = "Dispatcher thread failed to remove link";

MM_TRC_DISPATCHER_SELECT_FAILED = "Dispatcher thread failed to select";

MM_TRC_DISPATCHER_CALLBACK_FAILED = "Dispatcher callback failed";

MM_TRC_SERVICE_THREAD_ALLOC_FAILED = "Service thread terminated due to insufficient memory";

MM_TRC_FLUSH_PROTOCOL_FAILED = "Flush protocol failed. Close connection...";

MM_TRC_PROTOCOL_PROCESSING_FAILED = "Protocol processing failed. Close connection...";

MM_TRC_SESSION_FREE_FAILED = "Session free failed. There may be a memory leak...";

MM_TRC_TASK_FREE_FAILED = "Task free failed. There may be a memory leak...";

MM_TRC_TASK_STATE_ERROR = "Task state transition error. Close connection...";

MM_TRC_XA_INVALID_PROPERTY = "Invalid Property Value ( <%s> = <%d> )";

MM_TRC_SERVER_NOT_STARTED = "Server is not started.";

MM_TRC_INVALID_PHASE = "Unable to startup in the specified phase in the current state.";

MM_TRC_INVALID_OPTION = "Server option is invalid.";

MM_TRC_THREAD_INITFAIL = "Initializing thread container failed.";

MM_TRC_UL_INITIALIZED = "The client library is already initialized.";

MM_TRC_ABORT_SESSION_DISCONNECTED = "ERR-<%d>(errno=<%d>) <%s>\n"
                                    "    Session ID  : <%d>\n"
                                    "    Client Info : <%s>\n";

MM_TRC_ABORT_SHUTDOWN_NOT_SUPPORTED_VIA_IPC = "Not Supported via IPC.";

MM_TRC_SET_UNSUPPORTED_PROPERTY = "[WARNING] Unsupported property was used for set : [<%d>]";
MM_TRC_GET_UNSUPPORTED_PROPERTY = "[WARNING] Unsupported property was used for get : [<%d>]";

MM_TRC_EXTPROC_MEM_PROBLEM = "There is a problem with memory allocation.";
MM_TRC_EXTPROC_CONN_INIT_FAILED = "Failed to initialize connection.";
MM_TRC_EXTPROC_CONN_OPEN_FAILED = "Failed to open connection.";
MM_TRC_EXTPROC_CONN_WAIT_FAILED = "Failed to wait connection.";
MM_TRC_EXTPROC_DISCONN_FAILED = "Failed to disconnect.";
MM_TRC_EXTPROC_CONN_TASK_TIMEOUT = "<%s> is cancelled (time-out).";
MM_TRC_EXTPROC_MSG_WAIT_FAILED = "Failed to wait message.";
MM_TRC_EXTPROC_MSG_SEND_FAILED = "Failed to send result message.";
MM_TRC_EXTPROC_MSG_RECV_FAILED = "Failed to receive message.";
MM_TRC_EXTPROC_MSG_PACK_FAILED = "Failed to pack result message.";
MM_TRC_EXTPROC_MSG_UNPACK_FAILED = "Failed to unpack message.";
MM_TRC_EXTPROC_MSG_PARAM_SET_FAILED = "An error occurred when pointer to a parameter is set.";

MM_TRC_THREADCOUNT_EXPANDED = "Maximum number of threads expanded from <%d> to <%d>.";

# PROJ-2473 SNMP
MM_TRC_SNMP_DISABLED = "SNMP has been disabled.";

MM_TRC_SNMP_QUERY_TIMEOUT = "[Notify : Query Timeout] "
                            "Query Canceled by Server : Session ID = <%u>";

MM_TRC_SNMP_FETCH_TIMEOUT = "[Notify : Fetch Timeout] "
                            "Session Closed by Server : Session ID = <%u>";

MM_TRC_SNMP_UTRANS_TIMEOUT = "[Notify : UTrans Timeout] "
                             "Session Closed by Server : Session ID = <%u>";

MM_TRC_SNMP_SESSION_FAILURE_COUNT = "[Notify : Session Failure] "
                                    "Session Failed Continuously : Session ID = <%u>, Count = <%u>";

MM_TRC_SNMP_CHECK_BOOT_LOG = "Please check altibase_boot.log";

MM_TRC_SNMP_PROPERTIES = "SNMP: PORT_NO      = <%u>\n"
                         "      TRAP_PORT_NO = <%u>\n"
                         "      RECV_TIMEOUT = <%u> ms\n"
                         "      SEND_TIMEOUT = <%u> ms\n"
                         "      TRC_FLAG     = <%u>";

MM_TRC_SNMP_LISTENER_START = "  [MM] Listener started : UDP on port <%u> [IPV4] for SNMP";

MM_TRC_SNMP_INVALID_PROTOCOL = "SNMP: INVALID PROTOCOL (Length:<%d>) <%s> from <%s>";

MM_TRC_SNMP_DUMP_PROTOCOL = "SNMP: PROTOCOL <%s> <%s> (Length:<%d>) from <%s>";

MM_TRC_SNMP_ERR = "SNMP: ERROR - <%s>";

MM_TRC_INVALID_DATA_TYPE = "Invalid type of data to bind to a host variable\n"
                           "[ Param ID  = <%d>\n"
                           "  Data Type = <%d>\n"
                           "  Language  = <%d>\n"
                           "  Arguments = <%d>\n"
                           "  Precision = <%d>\n"
                           "  Scale     = <%d>\n"
                           "  InoutType = <%d>\n"
                           "  Data Size = <%d> ]";

MM_TRC_CM_SHUTDOWN_FAILURE = "shutdown() FAILURE in termiate()";

MM_TRC_SYSDBA_SESSION_CLOSED = "SYSDBA session [<%u>] being killed by SYS session [<%u>]";

MM_MSG_SESSION_CLOSED = "<%d> session(s) closed.\n";

MM_MSG_RLIMIT_HEADER = "Resource Limits   : Soft, Hard\n------------------------------\n";

MM_TRC_DDL_SYNC_TIMEOUT = "[Notify : DDL Sync Timeout] "
                          "DDL Query Canceled by Server : Session ID = <%u>\n"
                          "    CLIENT_INFO           => <%s>(PID : <%lu>)\n"
                          "    Time Limit            => <%u>\n"
                          "    Running Time          => <%u>\n"
                          "    Caused by DDL Query   => <%s>\n";

INTERNAL_TRACE_MESSAGE_END
