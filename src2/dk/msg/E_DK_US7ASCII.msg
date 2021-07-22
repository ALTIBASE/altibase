# $Id$
#
# DK 에러 메시지 화일 (used by genErrMsg)
#

# 에러코드 영역 번호
# 0 => id
# 1 => sm 
# 2 => mt 
# 3 => qp
# 4 => mm 
# 5 => ul
# 6 => rp 
# 9 => ut
# 10 => st
# 11 => lk
# 12 => dk

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.
# Last Number = 109

SECTION = 12

0,dkERR_ABORT_NotSupportOperation = Not supported operation
# *Cause:  The current operation isn't supported yet.
# *Action: Verify that the current operation is valid.

1,dkERR_ABORT_MEMORY_ALLOCATION = Insufficient memory for Database Link
# *Cause:
#   - The memory manager could not allocate sufficient memory for the query processor.
# *Action:
#   - Verify that the system has sufficient memory.

2,dkERR_ABORT_DK_FUNCTION_NOT_ALLOWED = This function is executable only in an execute procedure/function statement
# *Cause:
#   - The function was called in DML.
# *Action:
#   - Use this function only in an EXECUTE statement.

3,dkERR_ABORT_DK_MISMATCHED_COLUMN_TYPE = Type mismatched column type and return type
# *Cause:
#   - The column type and return type do not match.
# *Action:
#   - Check the column specification.

4,dkERR_ABORT_DK_UNSUPPORTED_COLUMN_TYPE = Not supported column type of remote database
# *Cause:
#   - The column type is unsupported.
# *Action:
#   - Check the column specification of database link.

5,dkERR_ABORT_DK_PARSING_DBLINK_CONF_FAILED = Cannot parse dblink.conf - <0%s>
# *Cause:
#   - The dblink.conf file could not be parsed.
# *Action:
#   - Check the dblink.conf file.

6,dkERR_ABORT_DK_OPEN_DBLINK_CONF_FAILED = Cannot open dblink.conf
# *Cause:
#   - The dblink.conf file could not be opened.
# *Action:
#   - Check the dblink.conf file.

7,dkERR_ABORT_DK_NO_HOME_DIRECTORY = The 'ALTIBASE_HOME' environment must be set
# *Cause:
#   - The environment variable 'ALTIBASE_HOME' is not set.
# *Action:
#   - Check the environment variable 'ALTIBASE_HOME'.

8,dkERR_ABORT_DKM_SESSION_INIT_FAILED = Failed to initialize a linker data session (Session Id:<0%d>)
# *Cause:
#   - The number of linker data sessions is full.
# *Action:
#   - Check the number of sessions from performance view.

9,dkERR_ABORT_DKM_GTX_PREPARE_PHASE_FAILED = Remote atomic transaction in prepare phase failed
# *Cause:
#   - There is a network problem or protocol version inconsistency between Altibase and the AltiLinker process.
# *Action:
#   - Check the network or product version.

10,dkERR_ABORT_DKM_GTX_COMMIT_PHASE_FAILED = Remote atomic transaction in commit phase failed
# *Cause:
#   - There is a network problem or protocol version inconsistency between Altibase and the AltiLinker process.
# *Action:
#   - Check the number of sessions from performance views.

11,dkERR_ABORT_DK_INVALID_TX_LEVEL = Invalid atomic transaction level value for remote transaction
# *Cause:
#   - An invalid atomic transaction level is set.
# *Action:
#   - Check this transaction information from performance view.

12,dkERR_ABORT_DKM_GTX_ROLLBACK_IMPOSSIBLE = Cannot rollback the remote transaction
# *Cause:
#   - The remote server currently has the autocommit mode set to ‘On’.
# *Action:
#   - Check the atomic transaction level and the auto-commit mode of the remote server.

13,dkERR_ABORT_DKT_ROLLBACK_FAILED = Failed to rollback the remote transaction
# *Cause:
#   - There is a network problem or protocol version inconsistency between Altibase and the AltiLinker process.
# *Action:
#   - Check the network or product version.

14,dkERR_ABORT_DKT_GLOBAL_COORDINATOR_NOT_EXIST = The global coordinator does not exist
# *Cause:
#   - A global transaction can not be handled from an invalid transaction ID.
# *Action:
#   - Check the transaction information using performance views.
 
15,dkERR_FATAL_ThrMutexInit = Unable to initialize a mutex
# *Cause:
#   - The system failed to initialize a mutex.
# *Action:
#   - Check the error number in the trace log and contact your operating system engineer.

16,dkERR_ABORT_DKM_INVALID_DBLINK = Unable to find a database link
# *Cause:
#   - A database link is removed or the wrong database link name is used.
# *Action:
#   - Verify that the correct database link name is being used through the use of performance views.

17,dkERR_ABORT_DKM_INVALID_REMOTE_STMT = Unable to find a remote statemtent
# *Cause:
#   - The remote statement has finished processing.
# *Action:
#   - Check the remote statement information using performance views.

18,dkERR_ABORT_DKT_INVALID_REMOTE_TX = Unable to find a remote transaction
# *Cause:
#   - The remote transaction has finished processing.
# *Action:
#   - Check the remote transaction information using performance views.

19,dkERR_ABORT_DKM_COMMIT_FAILED = Failed to commit a remote transaction
# *Cause:
#   - A network problem occurred.
# *Action:
#   - Check the transaction or session information using performance views, and consider using 'commit force'.

20,dkERR_ABORT_DKM_ROLLBACK_IMPOSSIBLE = Unable to rollback remote transaction 
# *Cause:
#   - The autocommit mode is not set correctly, or the atomic transaction level is set to 'remote statement execution level.'
# *Action:
#   - Check the auto-commit mode or atomic transaction level.

21,dkERR_ABORT_DKM_DBLINK_ALREADY_EXIST = Database link already exists
# *Cause:
#   - A database link already exists.
# *Action:
#   - Check the database link information using performance views.

22,dkERR_ABORT_DKM_INVALID_USER_MODE = Invalid user mode
# *Cause:
#   - An input value for user mode is invalid.
# *Action:
#   - Check if the input value for the user mode is set to 'PUBLIC' or 'PRIVATE'.

23,dkERR_ABORT_DKS_ALLOC_LINK = [Network] Failed to allocate a communication link
# *Cause:
#   - Failed to create a llink object due to insufficient memory.
# *Action:
#   - Check the error number in the trace log and take appropriate action.

24,dkERR_ABORT_DKS_ALLOC_CM_BLOCK = [Network] Failed to allocate a communication block
# *Cause:
#   - Failed to allocate a memory buffer for network communication.
# *Action:
#   - Check the error number in the trace log and take appropriate action.

25,dkERR_ABORT_DKS_SHUTDOWN_LINK = [Network] Failed to shutdown a communication link
# *Cause:
#   - The other party closed the socket before the network was shut down.
# *Action:
#   - You may safely ignore this error.

26,dkERR_ABORT_DKS_CLOSE_LINK = [Network] Failed to close a communication link
# *Cause:
#   - The network socket was closed.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

27,dkERR_ABORT_DKS_FREE_CM_BLOCK = [Network] Failed to free a communication read or write block
# *Cause:
#   - An allocated memory buffer was freed.
# *Action:
#   - Check the error number and take the appropriate action.

28,dkERR_ABORT_DKS_FREE_LINK = [Network] Failed to free a communication link
# *Cause:
#   - A created link object was freed.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

29,dkERR_ABORT_DKO_DBLINK_IS_BEING_USED = Database link already being used
# *Cause:
#   - An attempt was made to shut down the database link while it was being used by another session.
# *Action:
#   - Check the performance view for dynamic database link information.

30,dkERR_ABORT_DKO_DBLINK_ALREADY_EXIST = Database link name already exists
# *Cause:
#   - There was an attempt to create a database link with a name that already exists.
# *Action:
#   - Check the meta table and verify that the database link name does not conflict with an existing object.

31,dkERR_ABORT_DKA_ACCESS_LINKER_EXEC_FILE = Unable to get the Altilinker process execution file.
# *Cause:
#   - An AltiLinker process execution file does not exist or the wrong file path is used.
# *Action:
#   - Can not find or access 'ALTIBASE_HOME/bin/altilinker.jar' file. 

32,dkERR_ABORT_DKA_FORK_EXEC_LINKER_PROCESS = Unable to execute Altilinker process.
# *Cause:
#   - Failed to start the AltiLinker process.
# *Action:
#   - Check whether JAVA_HOME is specified, or whether you have the execute privilege, or whether there are too many symbolic links.
#   - If none of the above are wrong, check the error number in the trace log and take appropriate action.

33,dkERR_ABORT_DKS_LINKER_SESSION_CONNECT = [Network] Failed to connect with Altilinker process. 
# *Cause:
#   - cmiConnectWithoutData() returned an error.
# *Action:
#   - Check the network connection or verify that the AltiLinker process running.

34,dkERR_ABORT_ADLP_PROTOCOL = ADLP protocol failed to work
# *Cause:
#   - ADLP protocol error.
# *Action:
#   - Check the version of the AltiLinker process and Altibase server or check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

35,dkERR_ABORT_DKP_TIMEOUT = [Network] A timeout occurred
# *Cause:
#   - Network failure.
# *Action:
#   - Check network connection or check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

36,dkERR_ABORT_DKP_ANALYZE_ADLP_HEADER = [Network] Failed to analyze an ADLP header
# *Cause:
#   - Network failure or wrong protocol version
# *Action:
#   - Check network connection or check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

37,dkERR_ABORT_DKP_CMI_SEND = [Network] Failed to send a packet
# *Cause:
#   - cmiSend() returned an error
# *Action:
#   - Check the network connection, whether the AltiLinker process is running or check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

38,dkERR_ABORT_DKM_ALTILINKER_DISABLED = Failed to execute Altilinker process
# *Cause:
#   - The user tried to start the AltiLinker process but it was disabled.
# *Action:
#   - Check the property file 'dblink.conf'.

39,dkERR_ABORT_DKM_ALTILINKER_ALREAY_RUNNING = Altilinker process already running
# *Cause:
#   - The AltiLinker process is already running.
# *Action:
#   - Check if an AltiLinker process is already running.

40,dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING = No running Altilinker process
# *Cause:
#   - The AltiLinker process is not running.
# *Action:
#   - Check if an AltiLinker process is running.

41,dkERR_ABORT_DTK_REMOTE_SERVER_ERROR = Remote server error occurred ( Error code: <0%d>, SQL state: <1%s>, Description: <2%s> )
# *Cause:
#   - There was a remote server error caused by a syntax error in the remote statement
# *Action:
#   - Check the syntax of the remote statement and check the remote server error logs.

42,dkERR_ABORT_DKT_INVALID_JDBC_DRIVER = Invalid JDBC driver
# *Cause:
#   - The JDBC driver either does not exist or is invalid.
# *Action:
#   - Check the directory and the JDBC drivers.

43,dkERR_ABORT_DKT_INVALID_STMT_TYPE = Invalid remote statement type
# *Cause:
#   - There is a misapplication of the procedure function.
# *Action:
#   - Check the syntax of the remote statement.

44,dkERR_ABORT_DKM_GET_ERROR_INFO = Failed to get remote statement execution error information
# *Cause:
#   - This error was either not caused by the remote server, or was simply not executed yet.
# *Action:
#   - Verify that the remote statement executed and check the remote server's log files.

45,dkERR_FATAL_DK_FAILED = A fatal error occurred!! This error is very critical. 
# *Cause:
#   - Undefined error 
# *Action:
#   - Please send a bug report to the vendor.

46,dkERR_ABORT_DKM_REMOTE_STMT_IS_NOT_FAILED = No error for the remote statement
# *Cause:
#   - The function was called successfully, so there is no remote_statement_id to pass to the REMOTE_GET_ERROR_INFO function.
# *Action:
#   - Check the result of the remote statement execution.

47,dkERR_ABORT_DKP_INVALID_REMOTE_STMT = Invalid remote statement id detected in protocol operation
# *Cause:
#   - Altibase received the query results of a terminated session from AltiLinker.
# *Action:
#   - Forcefully kill AltiLinker, then execute the ALTER DATABASE LINKER STOP FORCE command, and then execute the ALTER DATABASE LINKER START command.

48,dkERR_ABORT_DKT_GLOBAL_TX_NOT_PREPARED = Global transaction not prepared to commit 
# *Cause:
#   - Network error or internal malfunction.
# *Action:
#   - Check the network and check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

49,dkERR_ABORT_DKM_DBLINK_DISABLED = dblink disabled 
# *Cause:
#   - Dblink disabled.
# *Action:
#   - Check the altibase.properties file and set the correct value.

50,dkERR_ABORT_TRAVERSE_NOT_APPLICABLE = Impossible to traverse
# *Cause: 
#   - The traverse is impossible.
# *Action: 
#   - Verify that the traverse is valid.

51,dkERR_ABORT_FUNCTION_NOT_AVAILABLE = The function is not available
# *Cause: 
#   - The function doesn't have to be called.
# *Action: 
#   - Please make sure that the function is valid.

52,dkERR_ABORT_META_IS_DIFFERENT = Remote table's meta has changed
# *Cause:
#   - The AltiLinker process is not running.
# *Action:
#   - Check if an AltiLinker process is running.

53,dkERR_REBUILD_META_IS_DIFFERENT = Remote table's meta has changed
# *Cause:
#   - The AltiLinker process is not running.
# *Action:
#   - Check if an AltiLinker process is running.

54,dkERR_ABORT_DKO_DBLINK_NOT_EXIST = A database link does not exist
# *Cause:
#   - The specified database link does not exist.
# *Action:
#   - Verify that the correct database link was specified, or use performance views to check for available database links.

55,dkERR_ABORT_ARGUMENT_IS_NOT_CONSTANT = The argument is not a constant value
# *Cause:
#   - The argument is not a constant value.
# *Action:
#   - Check the function's arguments.

56,dkERR_ABORT_ARGUMENT_IS_NOT_PROPER_TYPE = The argument is not proper type
# *Cause:
#   - The argument is not of the smallint or integer types.
# *Action:
#   - Check the function's arguments.

57,dkERR_ABORT_DKM_INVALID_TARGET = Unable to find a valid target
# *Cause:
#   - A database link property file is not set correctly.
# *Action:
#   - Check the JDBC driver or the URL of the target remote server in the database link property file.

58,dkERR_ABORT_DKS_REMOTE_SESSION_IS_BUSY = Unable to close session, a remote node session is busy
# *Cause:
#   - There is a currently executing remote statement.
# *Action:
#   - Check performance views to find the remote transaction or statement, and then close the session after commiting or rolling back the remote transaction.

59,dkERR_ABORT_DKM_DATA_SESSION_NOT_EXIST = No linker data session exists
# *Cause:
#   - There is a currently executing remote statement.
# *Action:
#   - Check performance views to find the remote transaction or statement, and then close the session after commiting or rolling back the remote transaction.

60,dkERR_ABORT_DKS_SESSION_NOT_EXIST = Session not exist
# *Cause:
#   - Tried to connect using a session that does not exist
# *Action:
#   - Please check a performance view for session information and close session

61,dkERR_ABORT_DKM_INVALID_COLUMN = Unable to find column meta
# *Cause:
#   - Wrong column index used or this column does not exist.
# *Action:
#   - Check the 'REMOTE_GET_COLUMN_VALUE' function in the procedure.

62,dkERR_ABORT_DKT_REMOTE_SERVER_DISCONNECT = [Network] Unable to access a remote server
# *Cause:
#   - An incorrect user ID or password was specified for the target remote server, or the connection was disconnected.
# *Action:
#   - Check the TARGET item in the 'dblink.conf' configuration file or check the connection with the target remote server.

63,dkERR_ABORT_DKM_COLUMN_VALUE_NULL = No column value
# *Cause:
#   - REMOTE_GET_COLUMN_VALUE_ function is used against a column which has no value.
# *Action:
#   - Check the procedure.

#64,dkERR_ABORT_DKT_GLOBAL_COORDINATOR_IS_ACTIVE = Active global coordinator exists
# *Cause:
#   - Tried to destroy global coordinator which is running
# *Action:
#   - Please send a bug report to the vendor.

65,dkERR_ABORT_DKD_REMOTE_QUERY_EMPTY = Empty remote query
# *Cause:
#   - The remote query is empty.
# *Action:
#   - Check the remote query.

66,dkERR_ABORT_DKD_NO_COLUMN = No column
# *Cause:
#   - There is no column.
# *Action:
#   - Check column count and information.

67,dkERR_ABORT_DKT_SAVEPOINT_ALREADY_EXIST = Same savepoint name already exists
# *Cause:
#   - The savepoint name already exists.
# *Action:
#   - Use a different name for the savepoint.

68,dkERR_ABORT_DKS_DATA_SESSION_ALREADY_EXIST = A linker data session already exists
# *Cause:
#   - Tried to create a linker data session that already exists.
# *Action:
#   - Check the performance view for linker data session information.

69,dkERR_ABORT_DKM_NOT_SUPPORT_GTX_LEVEL = Not supported global transaction level
# *Cause:
#   - The global transaction level was set to an unsupported value.
# *Action:
#   - Check the performance view or the altibase.properties file.

70,dkERR_ABORT_DKM_NOT_SUPPORT_AUTOCOMMIT_MODE = Not supported autocommit mode
# *Cause:
#   - The autocommit mode was set to an unsupported value.
# *Action:
#   - Check the performance view or the altibase.properties file.

71,dkERR_ABORT_DKT_REMOTE_STMT_TYPE = Invalid remote statement type
# *Cause:
#   - An invalid remote statement type is being used.
# *Action:
#   - Verify that the correct remote statement type is being used, or refer to the Database Link User’s Manual.

72,dkERR_ABORT_DKD_REMOTE_TABLE_COLUMN_IS_NO_NULL = Received null value from non null column of remote table
# *Cause:
#   - The specified column in the remote table has a NOT NULL constraint, but a NULL value was received.
# *Action:
#   - Check whether changes have been made to the remote table's schema.

73,dkERR_ABORT_DKA_PREPARE_LINKER_SHUTDOWN_FAILED = Failed to prepare AltiLinker process shutdown, AltiLinker maybe busy now
# *Cause:
#   - The AltiLinker process is currently too busy to shut down.
# *Action:
#   - Check the the AltiLinker status using performance views, retry this command after the AltiLinker process is finished, or use the 'FORCE' option.

74,dkERR_ABORT_DKT_REMOTE_NODE_SESSION_COUNT_EXCESS = Cannot create remote node session any more
# *Cause:
#   - Too many remote node sessions exist.
# *Action:
#   - Increase the 'ALTILINKER_REMOTE_NODE_SESSION_COUNT' property or check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

75,dkERR_ABORT_DKM_INVALID_USER = Failed to drop a database link by invalid user
# *Cause:
#   - A user who is not the owner of a database link tried to drop it.
# *Action:
#   - Check the ownership of the database link.

76,dkERR_ABORT_DKM_START_ALTILINKER_PROCESS = Unable to fork AltiLinker process
# *Cause:
#   - Another process is using same port number or wrong property file etc.
# *Action:
#   - Verify that the correct port number or property file path is being used.

77,dkERR_ABORT_DKM_CREATE_CTRL_SESSION_FAILED = Failed to create linker control session
# *Cause:
#   - Another process is using same port number or wrong property file etc.
# *Action:
#   - Verify that the correct port number or property file path is being used.

78,dkERR_ABORT_DKM_DBLINK_PROPERTIES_LOAD_FAILED = Failed to load dblink.conf
# *Cause:
#   - The dblink property file specified is incorrect or does not exist.
# *Action:
#   - Check the formatting and contents of the dblink property file and restart the AltiLinker process.

79,dkERR_ABORT_DKM_INVALID_TRANSACTION = Invalid remote transaction
# *Cause:
#   - Couldn't find the global transaction or remote transaction
# *Action:
#   - Check the right statement id or transaction id.

80,dkERR_ABORT_DKT_SAVEPOINT_NOT_EXIST = Savepoint not exist
# *Cause:
#   - The savepoint does not exist or no savepoint name entered.
# *Action:
#   - Verify that the savepoint name is correct.

81,dkERR_ABORT_DKP_INVALID_SESSION_ID = Invalid session id
# *Cause:
#   - An invalid session ID was passed.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

82,dkERR_ABORT_DKA_DO_LINKER_SHUTDOWN_FAILED = [Network] Failed to shutdown AltiLinker process
# *Cause:
#   - Network failure
# *Action:
#   - Verify that the AltiLinker process running. If it is, check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

83,dkERR_ABORT_DKM_INVALID_REMOTE_STMT_TYPE = Invalid remote statement type
# *Cause:
#   - Tried to execute invalid remote statement type
# *Action:
#   - Check the remote statement type or string.

84,dkERR_ABORT_DKM_NEXT_ROW_FAILED = Failed to get next row
# *Cause:
#   - The remote statement is already freed or the wrong statement ID was used
# *Action:
#   - Verify the remote statement ID and that the remote statement is active.

85,dkERR_ABORT_DKM_INVALID_COLUMN_INDEX = Invalid column index
# *Cause:
#   - The wrong column index number was used.
# *Action:
#   - Check the 'REMOTE_GET_COLUMN_VALUE' function in the procedure.

86,dkERR_ABORT_DKI_WRONG_LINK_NAME = Invalid database link name
# *Cause:
#   - The specified database link name is invalid.
# *Action:
#   - Check the database link name.

87,dkERR_ABORT_ALTILINKER_DISCONNECTED = [FAILURE] Altilinker process disconnected
# *Cause:
#   - The AltiLinker process was killed or disconnected.
# *Action:
#   - Verify that the AltiLinker process is active and that it is currently connected.

88,dkERR_ABORT_DKM_SET_SAVEPOINT = Failed to set a savepoint to remote server 
# *Cause:
#   - The remote node session is running in autocommit mode, or the remote server does not support changes to the autocommit mode.
# *Action:
#   - Verify that the autocommit mode of the remote node session is set to 'OFF'.

89,dkERR_ABORT_DKI_STATEMENT_IS_NULL = There is no internal statement information.
# *Cause:
#   - Internal statement is missing.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

90,dkERR_ABORT_DKM_LINKER_DUMP_ERROR = Failed to dump altilinker information.
# *Cause:
#   - The AltiLinker process is running abnormally.
# *Action:
#   - Verify the AltiLinker process status and the "altibase_lk.log" file.

91,dkERR_ABORT_DKD_INVALID_BUFFER_SIZE = The buffer size is not large enough to fetch the remote query results.
# *Cause:
#   - The remote server record size is greater than the allocated dblink buffer size.
# *Action:
#   - Check the DBLINK_DATA_BUFFER_BLOCK_SIZE, DBLINK_DATA_BUFFER_BLOCK_COUNT, DBLINK_DATA_BUFFER_ALLOC_RATIO or DBLINK_REMOTE_TABLE_BUFFER_SIZE property.

92,dkERR_ABORT_DKD_INTERNAL_BUFFER_FULL = Insufficient memory for the Database Link.
# *Cause:
#   - Internal buffer for database link is full.
# *Action:
#   - Please check whether the query result is too large.

93,dkERR_ABORT_DKM_DBLINK_ALTILINKER_STATUS = DBLINK_ALTILINKER_STATUS is only available in the DAEMON process.
# *Cause:
#   - DBLINK_ALTILINKER_STATUS is visible at DAEMON process.
# *Action:
#   - Please query with the SYSDBA privilege.

94,dkERR_ABORT_DKN_GET_ADDR_INFO_ERROR = An error occurred while receiving address information over the network.
# *Cause:
#   - There is error while getting address info.
# *Action:
#   - Please check network status.

95,dkERR_ABORT_DKN_OPEN_SOCKET_ERROR = A network error occurred.
# *Cause:
#   - An error occurred while opening the socket.
# *Action:
#   - Please check network status.

96,dkERR_ABORT_DKN_SELECT_SOCKET_ERROR = A network error occurred.
# *Cause:
#   - An error occurred while selecting the socket.
# *Action:
#   - Please check network status.

97,dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER = An error occurred while sending data over the network.
# *Cause:
#   - The data does not adhere to ADLP.
# *Action:
#   - Please send a bug report to the vendor.

98,dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_READ_BUFFER = An error occurred while receiving data over the network.
# *Cause:
#   - The data does not adhere to ADLP.
# *Action:
#   - Please send a bug report to the vendor.

99,dkERR_ABORT_DKN_SEND_SOCKET_ERROR = An error occurred while sending data over the network.
# *Cause:
#   - An error occurred while sending data to the socket.
# *Action:
#   - Please check network status.

100,dkERR_ABORT_DKN_RECV_SOCKET_ERROR = An error occurred while receiving data over the network.
# *Cause:
#   - An error occurred while receiving data from the socket.
# *Action:
#   - Please check network status.

101,dkERR_ABORT_DKN_WRONG_HEADER_SIGN = The ADLP protocol header is wrong.
# *Cause:
#   - The communication protocol header is invalid.
# *Action:
#   - Please check the network or product version.

102,dkERR_ABORT_DKT_INVALID_COLUMN_INDEX = Invalid column index
# *Cause:
#   - The wrong column index number was used.
# *Action:
#   - Check the 'REMOTE_GET_RESULT_BATCH' function in the procedure.

103,dkERR_ABORT_DKT_INVALID_REMOTE_STMT_TYPE = Invalid remote statement type
# *Cause:
#   - Tried to execute invalid remote statement type
# *Action:
#   - Check the remote statement type or string.

104,dkERR_ABORT_XA_APPLY_FAIL = Fail notifier application. Result type = <0%u>, Global tx id = <1%lu>.
# *Cause:
#   - There's network error or remote server failure.
# *Action:
#   - Please check network and remote server's status.

105,dkERR_ABORT_DKM_CAN_NOT_CHANGE_GTX_LEVEL = It can not change transaction level while global transaction is running.
# *Cause:
#   - There's running global transaction.
# *Action:
#   - Please try again after finish global transaction.

106,dkERR_ABORT_DK_INTERNAL_ERROR = Internal error occurs. (<0%s>).
# *Cause:
#   - internal error.
# *Action:
#   - Please send a bug report to the vendor.

107,dkERR_ABORT_DKM_CANNOT_SET_SAVEPOINT = It cannot set a savepoint to remote server in global transaction level 2.
# *Cause:
#   - Global transaction level is 2.
# *Action:
#   - Please check global transaction level.

108,dkERR_ABORT_DKA_JAVA_EXEC_ERR = Java binary which can execute the Altilinker process cannot be found [ <0%s>: <1%s> ].
# *Cause:
#   - Either java has not been installed or JAVA_HOME has been incorrectly configured, otherwise, the system privilege does not exist.
# *Action:
#   - The Java binary with the system privilege is required to execute the process.
#   - To do that, install JRE on a machine where AltiLinker would be operating, and then configure the installation path of JRE in JAVA_HOME environment variables or verify the system privilege if it has already been installed.

109,dkERR_ABORT_DKA_JAVA_HOME_ERR = altilinker cannot be executed since the environment variables of JAVA_HOME has not been configured yet. 
# *Cause:
#   - The Java_HOME environment variables are not configured.
# *Action:
#   - Accurately configure the installation path in which JRE is installed in the JAVA_HOME environment variables.

110,dkERR_ABORT_DKT_ALTILINKER_XAEXCEPTION = XAException occurred.
# *Cause:
#   - XAException occured from Altilinker process.
# *Action:
#   - Check XAException message in altibase_lk file and take appropriate action.

111,dkERR_ABORT_DKM_NOT_EXECUTE_DML = Failed to execute a DML because the global transaction has been terminated.
# *Cause:
#   - After failed to commit in global transaction level 2, do not execute a DML statement to a remote server.
# *Action:
#   - Execute rollback and try again.

INTERNAL_TRACE_MESSAGE_BEGIN

DK_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
            "signed long =<%ld>\n unsigned long = <%lu>\n"
            "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>\n";

DK_TRC_I_PRE_PROCESS = "[DK] Pre Process phase\n";
DK_TRC_I_PROCESS = "[DK] Process phase\n";
DK_TRC_I_CONTROL = "[DK] Control phase\n";
DK_TRC_I_META = "[DK] Meta phase\n";
DK_TRC_I_SERVICE = "[DK] Service phase\n";
DK_TRC_I_SHUTDOWN = "[DK] Shutdown phase\n";
DK_TRC_T_COORDINATOR_XA_RESULT_CODE = "[DK] XA<%s> first failure :"
            "result type = <%u>, Global tx id = <%lu>, xid = <%s>, result code =<%u>\n"
            "all failed count = <%u> failure branch transaction remained...\n";
DK_TRC_T_NOTIFIER_XA_APPLY_RESULT_CODE = "[DK] notifier apply result :"
            "result type = <%u>, Global tx id = <%lu>, xid = <%s>, result code =<%d>\n";

DK_TRC_T_NOTIFIER_THREAD = "[DK] Unable to start the notifier thread. Check the error number in the trace log and take appropriate action.";
DK_TRC_T_NOT_EXIST_DTX_BRANCH_INFO = "[DK] It can not find distributed branch transaction info";
DK_TRC_T_NOT_EXIST_DTX_INFO = "[DK] It can not find distributed transaction info.";
DK_TRC_T_UNKNOWN_DTX_TYPE = "[DK] Unknown distributed transaction type occurs";

INTERNAL_TRACE_MESSAGE_END
