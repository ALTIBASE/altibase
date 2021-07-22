#
# ALA 에러
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
# Last Number = 74

SECTION = 5

0, 00000, ulaERR_IGNORE_LOG_MGR_INITIALIZE = Failed to initialize log manager
# *Cause:
#   - Failed to initialize the Log Manager.
# *Action:
#   - Verify that the file path is correct and that you have permission for the file. Check disk space.

1, 00000, ulaERR_IGNORE_LOG_FILE_OPEN = Failed to open log file
# *Cause:
#   - Failed to open the log file.
# *Action:
#   - Check the maximum number of files that can be open.
#   - Verify that the file path is correct and that you have permission for the file.

2, 00000, ulaERR_IGNORE_LOG_FILE_CLOSE = Failed to close log file
# *Cause:
#   - Failed to close the log file.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

3, 00000, ulaERR_IGNORE_LOG_MGR_DESTROY = Failed to detroy log manager
# *Cause:
#   - Failed to destroy the Log Manager.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

4, 00000, ulaERR_IGNORE_LOG_MGR_LOCK = Failed to lock log manager
# *Cause:
#   - Internal API error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

5, 00000, ulaERR_IGNORE_LOG_MGR_UNLOCK = Failed to unlock log manager
# *Cause:
#   - Internal API error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

6, 00000, ulaERR_ABORT_MEMORY_ALLOC = Memory allocation failure
# *Cause:
#   - Insufficient memory.
# *Action:
#   - Check the amount of free memory.

7, 00000, ulaERR_IGNORE_GEMOETRY_ENDIAN = Endian conversion of geometry type failed [<0%s>, <1%u>]
# *Cause:
#   - This is an internal API error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

8, 00000, ulaERR_FATAL_TX_ALREADY_BEGIN = Transaction has already begun [<0%u>:<1%u>]
# *Cause:
#   - Transaction already exists in the Transaction ID's slot.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

9, 00000, ulaERR_IGNORE_NOT_ACTIVE_TX = Inactive transaction [<0%u>]
# *Cause:
#   - Transaction is not active.
# *Action:
#   - Check Altibase Log Analysis API.

10, 00000, ulaERR_FATAL_MUTEX_INITIALIZE = Failed to initialize mutex [<0%s>]
# *Cause:
#   - Failed to initialize a mutex.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

11, 00000, ulaERR_FATAL_MUTEX_DESTROY = Failed to destroy mutex [<0%s>]
# *Cause:
#   - Failed to destroy a mutex.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

12, 00000, ulaERR_FATAL_MUTEX_LOCK = Failed to lock mutex [<0%s>]
# *Cause:
#   - Failed to lock a mutex.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

13, 00000, ulaERR_FATAL_MUTEX_UNLOCK = Failed to unlock mutex [<0%s>]
# *Cause:
#   - Failed to unlock a mutex.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

14, 00000, ulaERR_IGNORE_LINKEDLIST_NOT_EMPTY = Linked list is not empty.
# *Cause:
#   - Internal API error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

15, 00000, ulaERR_IGNORE_PARAMETER_NULL = Null parameter [<0%s>]
# *Cause:
#   - Input or output parameter is null.
# *Action:
#   - Check Altibase Log Analysis API.

16, 00000, ulaERR_IGNORE_INVALID_ROLE = Invalid Role [<0%d>]
# *Cause:
#   - Altibase Log Analysis API supports only XLog Sender.
# *Action:
#   - Check XLog Sender DDL.

17, 00000, ulaERR_IGNORE_INVALID_REPLICATION_FLAGS = Invalid replication flags [<0%u>]
# *Cause:
#   - Unsupported protocol.
# *Action:
#   - You may safely ignore this message.

18, 00000, ulaERR_ABORT_META_NOT_EXIST = The meta information does not exist.
# *Cause:
#   - The meta information does not exist.
# *Action:
#   - Check the Altibase Log Analysis API.

19, 00000, ulaERR_ABORT_NET_INITIALIZE_CONTEXT = Failed to initialize network context [<0%s>]
# *Cause:
#   - Internal API error.
# *Action:
#   - Please send a bug report to the vendor.

20, 00000, ulaERR_IGNORE_NET_TIMEOUT = Network timeout [<0%s>]
# *Cause:
#   - A network timeout occurred.
# *Action:
#   - Check the Altibase Log Analysis API.

21, 00000, ulaERR_ABORT_NET_TIMEOUT = Network timeout [<0%s>]
# *Cause:
#   - A network timeout occurred.
# *Action:
#   - Check the Altibase Log Analysis API.

22, 00000, ulaERR_ABORT_NET_READ = Network read failure [<0%s>, <1%u>]
# *Cause:
#   - Network or Altibase Log Analysis API problem.
# *Action:
#   - Check the network and environment variables.

23, 00000, ulaERR_ABORT_NET_EXIT = Already exited network session [<0%s>]
# *Cause:
#   - The network exit flag has already been set.
# *Action:
#   - Check the Altibase Log Analysis API.

24, 00000, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL = Unexpected network protocol [<0%s>]
# *Cause:
#   - Invalid protocol.
# *Action:
#   - Check the replication protocol version.

25, 00000, ulaERR_ABORT_NET_FINALIZE_PROTOCOL = Failed to finalize network protocol [<0%s>]
# *Cause:
#   - Internal API error.
# *Action:
#   - Please send a bug report to the vendor.

26, 00000, ulaERR_ABORT_NET_FINALIZE_CONTEXT = Failed to finalize network context [<0%s>]
# *Cause:
#   - Internal API error.
# *Action:
#   - Please send a bug report to the vendor.

27, 00000, ulaERR_ABORT_NET_WRITE = Network write failure [<0%s>, <1%u>]
# *Cause:
#   - Network or Altibase Log Analysis API problem.
# *Action:
#   - Check the network and environment variables.

28, 00000, ulaERR_ABORT_NET_FLUSH = Network flush failure [<0%s>, <1%u>]
# *Cause:
#   - Network or Altibase Log Analysis API problem.
# *Action:
#   - Check the network and environment variables.

29, 00000, ulaERR_IGNORE_PARAMETER_INVALID = Invalid parameter [<0%s>]
# *Cause:
#   - A parameter is invalid.
# *Action:
#   - Check Altibase Log Analysis API.

30, 00000, ulaERR_ABORT_MEM_POOL_ALLOC = Memory allocation failure in pool
# *Cause:
#   - The memory pool for XLogs is exhausted.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

31, 00000, ulaERR_ABORT_MEM_POOL_FREE = Memory free failure in pool
# *Cause:
#   - Internal API error.
# *Action:
#   - Please send a bug report to the vendor.

32, 00000, ulaERR_ABORT_MEM_POOL_INITIALIZE = Failed to initialize memory pool
# *Cause:
#   - Failed to allocate a memory pool for XLogs.
# *Action:
#   - Verify that there is enough available memory.

33, 00000, ulaERR_ABORT_MEM_POOL_DESTROY = Failed to destroy memory pool
# *Cause:
#   - Internal API error.
# *Action:
#   - Please send a bug report to the vendor.

34, 00000, ulaERR_ABORT_LINK_SHUTDOWN = Failed to shut down link
# *Cause:
#   - Failed to shut down network.
# *Action:
#   - You may safely ignore this message.

35, 00000, ulaERR_ABORT_LINK_FREE = Failed to free link
# *Cause:
#   - Failed to free network memory.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

36, 00000, ulaERR_ABORT_PROTOCOL_DIFF = Different protocol versions
# *Cause:
#   - The replication protocol version is different from that of XLog Sender.
# *Action:
#   - Check the version of the XLog Sender protocol.

37, 00000, ulaERR_IGNORE_SOCKET_TYPE_NONE = Socket type not selected
# *Cause:
#   - The socket type has not been selected.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

38, 00000, ulaERR_IGNORE_SOCKET_TYPE_NOT_SUPPORT = Socket type not supported
# *Cause:
#   - The socket type is not supported.
# *Action:
#   - Check the Altibase Log Analysis API.

39, 00000, ulaERR_ABORT_LINK_ALLOC = Failed to allocate link
# *Cause:
#   - Failed to allocate network resources.
# *Action:
#   - Verify that there are enough available system resources. Take appropriate action if necessary.

40, 00000, ulaERR_ABORT_LINK_LISTEN = Failed to listen for link
# *Cause:
#   - Failed to listen to a connection on a socket.
# *Action:
#   - Check the port status.

41, 00000, ulaERR_ABORT_LINK_WAIT = Failed to wait for link
# *Cause:
#   - Failed while waiting for a link.
# *Action:
#   - Check the network status.

42, 00000, ulaERR_ABORT_LINK_ACCEPT = Failed to accept link
# *Cause:
#   - Failed to accept a connection on a socket.
# *Action:
#   - Check the network status.

43, 00000, ulaERR_ABORT_LINK_SET = Failed to set link for session
# *Cause:
#   - cmiSetLinkForSession() failed.
# *Action:
#   - Please send a bug report to the vendor.

44, 00000, ulaERR_ABORT_SESSION_ADD = Failed to add session
# *Cause:
#   - cmiAddSession() failed.
# *Action:
#   - Please send a bug report to the vendor.

45, 00000, ulaERR_IGNORE_XLOG_SENDER_NAME_DIFF = Different XLog Sender name [<0%s>:<1%s>]
# *Cause:
#   - The local XLog sender name is different from the remote name.
# *Action:
#   - Check the XLog Sender DDL and Log Analysis API.

46, 00000, ulaERR_IGNORE_SOCKET_ALREADY_INITIALIZE = Socket already initialized
# *Cause:
#   - Socket can be initialized only once.
# *Action:
#   - Check the Altibase Log Analysis API.

47, 00000, ulaERR_IGNORE_SOCKET_NOT_SUPPORT_API = Socket does not support API
# *Cause:
#   - The socket does not support the API.
# *Action:
#   - Check the Altibase Log Analysis API.

48, 00000, ulaERR_IGNORE_AUTH_INFO_ONE = One piece of authentication information is needed
# *Cause:
#   - The TCP socket type needs at least one piece of authentication information.
# *Action:
#   - Check the Altibase Log Analysis API.

49, 00000, ulaERR_IGNORE_AUTH_INFO_MAX = Amount of authentication information already reached maximum
# *Cause:
#   - The amount of authentication information has already reached the maximum.
# *Action:
#   - Check the Altibase Log Analysis API.

50, 00000, ulaERR_IGNORE_AUTH_FAIL = Authentication failure
# *Cause:
#   - Peer authentication information do not exist.
# *Action:
#   - Check the Altibase Log Analysis API.

51, 00000, ulaERR_IGNORE_XLOG_POOL_EMPTY = XLog pool is empty.
# *Cause:
#   - XLog pool is empty.
# *Action:
#   - Check the Altibase Log Analysis API.

52, 00000, ulaERR_IGNORE_API_INITIALIZE = Global initialization failed
# *Cause:
#   - ALA_InitializeAPI() failed.
# *Action:
#   - Please send a bug report to the vendor.

53, 00000, ulaERR_IGNORE_API_DESTROY = Global termination failed
# *Cause:
#   - Invalid ALA_Handle.
# *Action:
#   - Check the ALA_Handle.

54, 00000, ulaERR_IGNORE_MTD_MODULE_GET = Failed to get MTD Module [<0%s>, <1%u>]
# *Cause:
#   - This is an internal error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

55, 00000, ulaERR_IGNORE_MTD_ENCODE = Failed to encode MTD Module [<0%s>]
# *Cause:
#   - This is an internal error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

56, 00000, ulaERR_IGNORE_CMT_INITIALIZE = Failed to initialize CMT
# *Cause:
#   - cmtAnyInitialize() failed.
# *Action:
#   - Please send a bug report to the vendor.

57, 00000, ulaERR_IGNORE_CMT_FINALIZE = Failed to finalize CMT
# *Cause:
#   - This is an internal API error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

58, 00000, ulaERR_IGNORE_DIAG_HEADER_CREATE = Failed to create diagnostic header
# *Cause:
#   - This is an internal API error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

59, 00000, ulaERR_IGNORE_DIAG_HEADER_DESTROY = Failed to destroy diagnostic header
# *Cause:
#   - This is an internal API error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

60, 00000, ulaERR_IGNORE_MT_TO_CMT_CONVERT = Failed to convert MT value to CMT value
# *Cause:
#   - This is an internal API error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

61, 00000, ulaERR_IGNORE_CMT_TO_COLUMN_COPY = Failed to copy CMT value to ulnColumn value
# *Cause:
#   - ulnDataWritePacketToCache() failed.
# *Action:
#   - Please send a bug report to the vendor.

62, 00000, ulaERR_IGNORE_COLUMN_TO_ODBC_CONVERT = Failed to convert ulnColumn value to ODBC C value
# *Cause:
#   - Failed to convert the column passed as an argument to a datatype supported by ODBC.
# *Action:
#   - Refer to the log file specified for ALA_EnableLogging and pass a data type supported by ODBC.

63, 00000, ulaERR_ABORT_TABLE_NOT_FOUND = Table Not Found [<0%s>, <1%lu>]
# *Cause:
#   - Failed to retrieve table information from the XLog Collector.
# *Action:
#   - Use the XLog Collector that has information of the table or check the application logic.

64, 00000, ulaERR_ABORT_COLUMN_NOT_FOUND = Column Not Found [<0%s>, <1%u>]
# *Cause:
#   - Failed to retrieve the column from the XLog Collector.
# *Action:
#   - Use the XLog Collector that has information of the table column or check the application logic.

65, 00000, ulaERR_ABORT_NOT_INIT_TXTABLE = Transaction table not initialized
# *Cause:
#   - The transaction table was not initialized.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

66, 00000, ulaERR_ABORT_NO_ENV_VARIABLE = Environment variable <0%s> is not set
# *Cause:
#   - Environment variable cannot be found.
# *Action:
#   - Set the environment variable.

67, 00000, ulaERR_ABORT_NullSourceData = Found that source data was NULL, when converting MT source data to CM data
# *Cause: The data source contains an Altibase Value that is NULL.
# *Action: Check that the application has properly passed the Altibase Value when using the API.

68, 00000, ulaERR_ABORT_InvalidMantissaLength = The mantissa length( <%d> ) of source data exceeds the limit of mantissa length
# *Cause: The data source contains invalid Numeric data.
# *Action: Check whether the data source has been passed properly.

69, 00000, ulaERR_ABORT_INVALID_DATA_CONVERSION = Invalid data conversion
# *Cause: Invalid data source.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

70, 00000, ulaERR_ABORT_NUMERIC_CONVERSION_OVERFLOW = Numeric conversion overflow
# *Cause: An overflow occurred while converting to the numeric data type because it exceeded the range.
# *Action: Check the data type and whether the value is within range.

71, 00000, ulaERR_ABORT_MEM_ALLOC_GET_INSTANCE = Failed to create an instance of the memory allocator [<0%s>]
# *Cause:
#   - Failed to instantiate the memory allocator object.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

72, 00000, ulaERR_ABORT_MEM_ALLOC_SET_ATTR = Failed to set memory allocator attributes [<0%s>]
# *Cause:
#   - Failed to set attributes on the memory allocator object.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

73, 00000, ulaERR_ABORT_ALLOC_CM_BLOCK = Failed to allocate communication block [<0%s>]
# *Cause:
#   - Failed to allocate memory.
# *Action:
#   - Verify that there is enough available memory.

74, 00000, ulaERR_ABORT_FREE_CM_BLOCK = Failed to free communication block [<0%s>]
# *Cause:
#   - Failed to free memory.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

75, 00000, ulaERR_ABORT_INSUFFICIENT_XLOG_POOL = ALA XLog Collector cannot receive allocable XLog because the XLog in XLog Pool is all consumed. 
# *Cause:
#   - There is no XLog that is available for use in the XLog Pool. 
# *Action:
#   - In order to receive the allocable XLog in ALA Application, restart ALA sender after raising
#   the values of ALA_XLOG_POOL_SIZE. Refer to Adapter for Oracle User’s Manual for more
#   information. 

INTERNAL_TRACE_MESSAGE_BEGIN

ULA_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
                 "signed long =<%ld>\n unsigned long = <%lu>\n"
                 "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>\n";

## API
ULA_TRC_E_ERROR_MGR_NULL = "[API] Error manager is null.";
ULA_TRC_E_INVALID_ACTION = "[API] Error action[<%d>] is invalid. ";

## XLog Collector
ULA_TRC_I_XC_CREATE    = "[XLog Collector] [<%s>] Created.";
ULA_TRC_I_XC_HANDSHAKE = "[XLog Collector] [<%s>] Handshaked.";
ULA_TRC_I_XC_DESTROY   = "[XLog Collector] [<%s>] Destroyed.";


INTERNAL_TRACE_MESSAGE_END
