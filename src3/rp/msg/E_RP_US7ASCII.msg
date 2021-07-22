# $Id: E_RP_US7ASCII.msg 26799 2008-07-01 00:55:17Z lswhh $
#
# RP 에러 메시지 화일 (used by genErrMsg)
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
# Last Number = 438

SECTION = 6

#0,rpERR_ABORT_RP_RECEIVER_NOT_FOUND_TUPLE = The received record was not found in the database.
# *Cause:
#   - Unable to find the received record in the database.
# *Action:
#   - Check the altibase_rp.log file.

#1,rpERR_ABORT_RP_RECEIVER_CONFLICT = A conflict occurred while executing the received statement.
# *Cause:
#   - A conflict has been occurred while executing the received statement.
# *Action:
#   - Check the altibase_rp.log file.

2,rpERR_FATAL_RP_MEMORY_ALLOCATION = Unable to allocate memory
# *Cause:
#   - The system failed to allocate sufficient memory.
# *Action:
#   - Verify that sufficient memory is available.

3,rpERR_ABORT_RP_READ_SOCKET = Unable to read from a socket
# *Cause:
#   - The connection to replication has been disconnected or failed to receive data within the specified time.
# *Action:
#   - Increase the replication-related timeout or check the local and remote altibase_rp.log file to find out why the connection is broken.

4,rpERR_ABORT_RP_WRITE_SOCKET = Unable to write to a socket
# *Cause:
#   - The system failed to write on socket since connection for replication was closed.
# *Action:
#   - Check the local and remote altibase_rp.log file to find out why the connection is broken.

#5,rpERR_FATAL_RP_CREATE_TEMPINFO = Internal error
# *Cause:
#   - Internal error
# *Action:
#   - Please send a bug report to the vendor.

6,rpERR_FATAL_RP_SENDER_INVALID_OPTION = Internal error
# *Cause:
#   - An invalid replication option was set for the replication Sender
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

#7,rpERR_ABORT_RP_META_NOT_INITIALIZED = Unable to initialize meta information
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

8,rpERR_ABORT_RP_META_NO_SUCH_DATA = Unable to find meta information.
# *Cause:
#   - Failed to find information from the replication metadata either because the database crashed or an error occurred.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

9,rpERR_ABORT_RP_JOIN_THREAD = Unable to join the sender thread
# *Cause:
#   - The system failed to join the sender thread.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

10,rpERR_ABORT_RP_ALREADY_STARTED = The sender has already been started.
# *Cause:
#   - The sender has already been started.
# *Action:
#   - Check whether the sender has already been started.

11,rpERR_ABORT_RP_MAXIMUM_THREADS_REACHED = Out of replication threads ( replication thread count > <0%d>)
# *Cause:
#   - The number of replications currently running on the system has exceeded the maximum limit.
# *Action:
#   - Check the maximum number of replication threads.

12,rpERR_ABORT_RP_RECEIVER_NOT_FOUND = The receiver does not exist.
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

13,rpERR_ABORT_RP_SENDER_HANDSHAKE = [Sender] Failed to handshake with the peer server (<0%s>)
# *Cause:
#   - Network or server error.
#   - The replication definition might contain an error.
# *Action:
#   - Check the network.
#   - Verify that the replication definition is the same on the corresponding server.

14,rpERR_ABORT_RP_PEER_WAKEUP = Failed to wake up peer sender
# *Cause:
#   - When the server started, it attempted to connect to the remote database, but failed.
# *Action:
#   - Start the remote database.

#15,rpERR_IGNORE_RP_SENDER_STOP_OLD = [Sender] Stop sender thread <0%s>:<1%d> at [<2%ld>], Restart SN[<3%ld>] 
# *Cause:
#   - The sender thread was stopped either normally or abnormally due to a failure in the network.
# *Action:
#   - This error is not attributable to Altibase.

16,rpERR_ABORT_RP_SENDER_START = [Sender] Failed to start the sender thread
# *Cause:
#   - The communication or handshaking failed.
# *Action:
#   - Check the network, the server, and the replication definition.

17,rpERR_ABORT_RP_SENDER_SYNCSTART = [Sender] Failed to start sync
# *Cause:
#   - The communication or handshaking failed.
# *Action:
#   - Check the network, the server, and the replication definition.

18,rpERR_ABORT_RP_SENDER_CONNECT_PEER = [Sender] Failed to connect to the peer server
# Cause: 
#   - 1. Peer server is not running.
#   - 2. IP address or Port number is invalid.
#   - 3. Network is not working.
# *Action:
#   - 1. Confirm peer server is running. 
#   - 2. Verify both IP address and port number are correctly entered.
#   - 3. Ensure that network is working normally.

19,rpERR_ABORT_RP_SENDER_DO_REPLICATION = [Sender] Replication failed
# *Cause:
#   - The replication Sender failed during replication.
# *Action:
#   - Check the error number from the altibase_rp.log file and take appropriate action.

20,rpERR_ABORT_RP_SENDER_MAKE_XLOG_IN_LOGFILE = [Sender] Failed to make XLOG in a log file at SN[<0%lu>]
# *Cause:
#   - The replication Sender failed to convert the log of the corresponding SN in the log file to an XLog.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

21,rpERR_ABORT_RP_SENDER_CHECK_LOG = [Sender] Failed to check a physical log at SN[<0%lu>]
# *Cause:
#   - The replication Sender cannot read the log of the corresponding SN.
# *Action:
#   - The replication Sender that sends logs to the Altibase Log Analyzer must specify the SN to another position.

#22,rpERR_ABORT_RP_SENDER_MAKE_XLOG = [Sender] Failed to make an xlog
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#23,rpERR_ABORT_RP_SENDER_ADD_BEGIN_XLOG = [Sender] Failed to add the begin XLOG
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#24,rpERR_ABORT_RP_SENDER_ADD_ABORT_XLOG = [Sender] Failed to add the abort XLOG
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#25,rpERR_ABORT_RP_SENDER_ADD_COMMIT_XLOG = [Sender] Failed to add the commit XLOG
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#26,rpERR_ABORT_RP_SENDER_ADD_INSERT_XLOG = [Sender] Failed to add the insert XLOG
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#27,rpERR_ABORT_RP_SENDER_ADD_DELETE_XLOG = [Sender] Failed to add the delete XLOG
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#28,rpERR_ABORT_RP_SENDER_ADD_UPDATE_XLOG = [Sender] Failed to add the update XLOG
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

29,rpERR_ABORT_RP_SENDER_SYNC_TABLE = [Sender] Failed to sync a table
# *Cause:
#   - The replication Sender failed to synchronize the table.
# *Action:
#   - Check the network status or the status of the corresponding server, and take appropriate action.

#30,rpERR_ABORT_RP_SENDER_ADD_SYNC_XLOG = [Sender] Failed to add the SYNC XLOG
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#31,rpERR_ABORT_RP_SENDER_SEND_BUFFER = [Sender] Failed to send replication buffer
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#32,rpERR_ABORT_RP_SENDER_UPDATE_XLSN = [Sender] Failed to update XLSN
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

33,rpERR_ABORT_RP_SENDER_ADD_INSERT_SYNC_XLOG = [Sender] Failed to add the insert SYNC XLOG
# *Cause:
#   - The replication Sender failed to send data to the corresponding server.
# *Action:
#   - Please send a bug report to the vendor.

34,rpERR_ABORT_RP_SENDER_SLEEP = [Sender] Sender Sleep : <0%d> seconds
# *Cause:
#   - The sender is in sleep.
# *Action:
#   - This message is for information only.

35,rpERR_ABORT_RP_REPLICATION_DISABLED = Replication is disabled
# *Cause:
#   - The replication feature of Altibase could not be used.
#   - Replication was not initialized on startup because the port is not set correctly.
# *Action:
#   - Set the port correctly.

36,rpERR_IGNORE_RP_SENDER_HASH_INIT = [Sender] Failed to initialize the hash table
# *Cause:
#   - The replication feature of Altibase could not be used.
#   - Replication was not initialized on startup because the port is not set correctly.
# *Action:
#   - Set the port correctly.

37,rpERR_ABORT_RP_REPLICATION_DENY = Replication denied (<0%s>)
# *Cause:
#   - Replication access was denied because the corresponding server has not yet started.
# *Action:
#   - Restart replication after a brief delay.
#   - If replication has already started, wait for the corresponding server to start.

38,rpERR_ABORT_RP_REPLICATION_NOK = Unable to process the replication (<0%s>)
# *Cause:
#   - Replication was not possible because some conditions were not satisfied.
# *Action:
#   - Try again later. 

39,rpERR_ABORT_RP_REPLICATION_NOT_STARTED = Replication did not start.
# *Cause:
#   - The replication did not start.
# *Action:
#   - Verify that replication was started.

40,rpERR_ABORT_RP_REPLICATION_SELF_REPLICATION = A case of self-replication has been detected. (Peer=<0%s>:<1%u>)
# *Cause:
#   - The IP address and port for replication are the same as those of the local server.
# *Action:
#   - Check the IP address and port for replication.

41,rpERR_ABORT_RP_ENTRY_EXIST = [Sender] Entry already exists in insert()
# *Cause:
#   - The same transaction table slot is being used.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

#42,rpERR_ABORT_BIND = Bind failure
# *Cause:
#   - Network error
# *Action:
#   - Check the network.

#43,rpERR_ABORT_SPACE_INIT = [Receiver] Failed to initialize error space
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

44,rpERR_ABORT_START_RECEIVER = [Receiver] Failed to start the receiver
# *Cause:
#   - The replication Receiver has failed to start either because the versions are different or there is a network error.
# *Action:
#   - Check the replication version between the two nodes, or check the network.

45,rpERR_ABORT_LISTEN = [Receiver] Failed to listen to a replication socket (Port No:<0%d>)
# Cause: 
#   - REPLICATION_PORT_NO port is occupied by other application or not yet released. 
# *Action:
#   - Use other port number or terminate the application using the REPLICATION_PORT_NO port.

#46,rpERR_ABORT_SOCKET_CREATE = [Receiver] Failed to create a replication socket.
# *Cause:
#   - Insufficient memory
# *Action:
#   - Increase the amount of available memory in the system.

#47,rpERR_ABORT_IDEALLOCERRORSPACE = [Receiver] Fatal error in ideAllocErrorSpace() function
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

48,rpERR_ABORT_META_BUILD = [Receiver] Failed to build meta information
# *Cause:
#   - The system failed to create metadata.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

49,rpERR_ABORT_META_MISMATCH = [Receiver] Meta information does not match
# *Cause:
#   - The receiver's meta information does not match the sender's meta information.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

50,rpERR_ABORT_SEND_ACK = [Receiver] Failed to send ACK 
# *Cause:
#   - Failed to send a handshake ACK to the replication Sender.
# *Action:
#   - Check the network.

51,rpERR_ABORT_TX_BEGIN = [Receiver] Transaction begin failure
# *Cause:
#   - The replication Receiver failed to start an internal transaction.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

52,rpERR_ABORT_TX_COMMIT = [Receiver] Transaction commit failure
# *Cause:
#   - The replication Receiver failed to commit an internal transaction.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

53,rpERR_ABORT_UPDATE_CONFLICT = [Receiver] An update conflict occurred.
# *Cause:
#   - An update conflict occurred while the replication was in process.
# *Action:
#   - You may safely ignore this message if you have restarted replication.

#54,rpERR_ABORT_NOT_FOUND_DEL = [Receiver] err_not found in deleteXlog()
# *Cause:
#   - The record to be deleted was not found.
# *Action:
#   - Check the altibase_rp.log file.

55,rpERR_ABORT_NOT_EXIST_TABLE_DEL = [Receiver] err_not_exist_table in deleteXlog()
# *Cause:
#   - The table does not exist.
# *Action:
#   - Check the altibase_rp.log file.

56,rpERR_ABORT_NOT_EXIST_TABLE_INS = [Receiver] err_not_exist_table in insertXlog()
# *Cause:
#   - The table does not exist.
# *Action:
#   - Check the altibase_rp.log file.

57,rpERR_ABORT_NOT_EXIST_TABLE_UPT = [Receiver] err_not_exist_table in updateXlog()
# *Cause:
#   - The table does not exist.
# *Action:
#   - Check the altibase_rp.log file.

#58,rpERR_ABORT_NOT_FOUND_UPT = [Receiver] err_not_found in updateXlog()
# *Cause:
#   - The table does not exist.
# *Action:
#   - Check the altibase_rp.log file.

#59,rpERR_ABORT_NULL_POINTER_ABORT = [Receiver] err_null_pointer in abort()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#60,rpERR_ABORT_NULL_POINTER_BEGIN = [Receiver] err_null_pointer in begin()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#61,rpERR_ABORT_NULL_POINTER_COMMIT = [Receiver] err_null_pointer in commit()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#62,rpERR_ABORT_NULL_POINTER_DEL = [Receiver] err_null_pointer in deleteXLog
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#63,rpERR_ABORT_NULL_POINTER_INS = [Receiver] err_null_pointer in insertXlog()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#64,rpERR_ABORT_NULL_POINTER_UPT = [Receiver] err_null_pointer in updateXlog()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#65,rpERR_ABORT_TX_BEGIN2 = [Receiver] err_tx in begin()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

66,rpERR_ABORT_TX_COMMIT2 = [Receiver] err_tx in commit()
# *Cause:
#   - The replication Receiver failed to commit a replicated transaction.
# *Action:
#   - Check the error code from the altibase_rp.log file and take appropriate action.
#   - If the maximum number of rows has been set for creating tables, commit can fail.

67,rpERR_ABORT_TX_ERROR = [Receiver] err_tx in err_tx()
# *Cause:
#   - The replication Receiver failed to roll back a committed transaction.
# *Action:
#   - Check the error code from the altibase_rp.log file and take appropriate action.

#68,rpERR_ABORT_ERROR_RECVXLOG = [Receiver] Protocol error in recvXLog()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#69,rpERR_ABORT_ERROR_SENDACK = [Receiver] error in sendACK()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

70,rpERR_ABORT_PROTOCOL_RUN = [Receiver] protocol error in run()
# *Cause:
#   - The replication Receiver received an unexpected protocol from the replication Sender.
# *Action:
#   - Check the replication protocol version between the two nodes.
#   - If the versions are different, change it to the same version.
#   - If the versions are the same, check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

71,rpERR_ABORT_ERROR_RECVXLOG2 = [Receiver] <0%s> receiver has error in recvXlog()
# *Cause:
#   - Unable to read data from the network connection.
# *Action:
#   - Check the network connection.

72,rpERR_ABORT_RECVXLOG_RUN = [Receiver] <0%s> receiver has recvXLog error in run()
# *Cause:
#   - Unable to read data from the network connection.
# *Action:
#   - Check the network connection.

#73,rpERR_ABORT_SENDACK_RECVXLOG = [Receiver] sendACK error in recvXLog()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

74,rpERR_IGNORE_RECEIVER_START = [Receiver] started ...
# *Cause:
#   - The receiver thread has already been started.
# *Action:
#   - You can safely ignore this message.

75,rpERR_ABORT_END_THREAD = [Receiver] <0%s> receiver is ended (by thr_exit)
# *Cause:
#   - The receiver thread has been terminated.
# *Action:
#   - You may safely ignore this message.

76,rpERR_IGNORE_START_SENDER = [Sender] Replication <0%s> Start... at [<1%ld>]
# *Cause:
#    - The replication started up correctly.
# *Action:
#    - N/A

77,rpERR_ABORT_CLOSE_LOG_FILE = [Sender] Failed to close log file logfile<0%d>.log
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

78,rpERR_ABORT_GET_ACK = [Sender] Failed to get ACK
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

79,rpERR_ABORT_RP_SENDER_INIT = [Sender] Failed to initialize
# *Cause:
#   - The replication Sender failed to initialize.
# *Action:
#   - Restart the replication Sender. 
#   - If the replication Sender repeatedly fails to start, check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).


80,rpERR_ABORT_OPEN_LOG_FILE = [Sender] Failed to open log file logfile<0%d>.log
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#81,rpERR_ABORT_INVALID_XLSN = [Sender] Invalid XLSN[<0%d>,<1%d>] > current[<2%d>,<3%d>] occurred
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#82,rpERR_ABORT_INVALID_LOG = [Sender] Invalid log
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

83,rpERR_ABORT_INVALID_PARAM = [Sender] Invalid parameter
# *Cause:
#   - Internal server error
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

#84,rpERR_ABORT_INVALID_XTYPE = [Sender] Invalid xlog type
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

85,rpERR_ABORT_MEMORY_ALLOC = Memory allocation failure [Function=<0%s>, Variable=<1%s>]
# *Cause:
#   - The system failed to allocate memory.
# *Action:
#   - Check the status of the memory available for allocation and retry.

86,rpERR_ABORT_ALREADY_FINAL = [Receiver] to be ended (already finalized by manager)
# *Cause:
#   - The replication Receiver terminated abnormally. 
#   - This error mainly occurs due to a network error or a rollback of a long-running transaction.
# *Action:
#   - Check the error number from the altibase_rp.log file and take appropriate action.
#   - If the replication Receiver has rolled back a long-running transaction, increase the value of replication_receive_timeout on both nodes.

87,rpERR_ABORT_PROTOCOL_ERROR = [Receiver] rp_x_b protocol error in run()
# *Cause:
#   - The receiver thread tried to start a transaction that has already been started.
# *Action:
#   - N/A

#88,rpERR_ABORT_ABORT_IN_FINAL = [Receiver] smiTrans abort error in final()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#89,rpERR_ABORT_INSERT_ERROR_IN_RUN = [Receiver] insert error in run()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

90,rpERR_ABORT_BEGIN_ERROR_IN_RUN = [Receiver] begin error in run()
# *Cause:
#   - The replication Receiver failed to add a new transaction.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

91,rpERR_ABORT_ABORT_ERROR_IN_RUN = [Receiver] abort error in run()
# *Cause:
#   - The replication Receiver failed to roll back a transaction.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

92,rpERR_ABORT_COMMIT_ERROR_IN_RUN = [Receiver] commit error in run()
# *Cause:
#   - The replication Receiver failed to commit a replicated transaction.
# *Action:
#   - Check the error number from the altibase_rp.log file and take appropriate action.

#93,rpERR_ABORT_ABORT_IN_DESTROY = [Receiver] smiTrans abort error in destroy()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#94,rpERR_ABORT_REMOVE_IN_DESTROY = [Receiver] hash remove error in destroy()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#95,rpERR_ABORT_DESTROY_IN_DESTROY = [Receiver] hash destroy error in destroy()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#96,rpERR_ABORT_RP_FIND_IN_HASH_INSERT = [Receiver] hash find error in insert()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#97,rpERR_ABORT_RP_FIND_IN_HASH_REMOVE = [Receiver] hash find error in remove()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#98,rpERR_ABORT_FIND_ERROR_IN_RUN = [Receiver] hash find error in run()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#99,rpERR_ABORT_INVALID_TYPE_IN_CONVERT = [Receiver] invalid type in convertValue()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#100,rpERR_ABORT_BUFFER_OVERRUN_IN_CONVERT = [Receiver] buffer overrun in convertValue()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#101,rpERR_IGNORE_SENDER_RESTART = [Sender] restart from logfile <0%d>.log
# *Cause:
#   - The sender thread was restarted because of the action related to the property REPLICATION_MAX_LOGFILE specified by the user.
# *Action:
#   - N/A

#102,rpERR_FATAL_RP_MANAGER_INTERNAL_ARG = Fatal internal server replication manager error. (<0%s>)
# *Cause:
#   - Program Error
# *Action
#   - Please send bug a report to the vendor.

#103,rpERR_FATAL_RP_RECEIVER_INTERNAL_ARG = Fatal internal server replication receiver error. (<0%s>)
# *Cause:
#   - Program Error
# *Action
#   - Please send bug a report to the vendor.

104,rpERR_IGNORE_RP_INIT_SOCK = Unable to initialize a socket
# *Cause:
#   - Not enough memory.
#   - Unable to connect to a remote server.
# *Action
#   - Check whether the remote server is running and online.

105,rpERR_ABORT_RP_INTERNAL_ARG = Internal server error in replication module (<0%s>).
# *Cause:
#   - Program error.
# *Action
#   - Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

#106,rpERR_ABORT_RP_SENDER_ADD_SAVEPOINT_XLOG = [Sender] Failed to add savepoint XLOG (<0%s>).
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

107, rpERR_ABORT_RP_SENDER_SEND_ERROR = [Sender] Failed to send XLog to peer network
# *Cause:
#   - The peer network has a problem.
# *Action:
#   - The system will retry automatically.

108, rpERR_ABORT_RP_SENDER_RECV_ERROR = [Sender] Failed to receive XLog from peer network
# *Cause:
#   - The peer network has a problem or is offline.
# *Action:
#   - The system will retry automatically.

109,rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN = [Receiver] smiTrans set(savepoint) error in run()
# *Cause:
#   - The replication Receiver failed to create a savepoint.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

110,rpERR_ABORT_RP_ABORT_SAVEPOINT_ERROR_IN_RUN = [Receiver] smiTrans abort(savepoint) error in run()
# *Cause:
#   - The replication Receiver failed to roll back to a savepoint.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

111,rpERR_ABORT_RP_NOT_DROP_ONE_TABLE = You cannot drop only one replicated table.
# *Cause:
#   - You cannot drop a table when only one replicated table is remaining.
# *Action:
#   - Drop the replication object, or insert another table and drop the table.

112, rpERR_IGNORE_RP_PROTOCOL_DIFF = Different replication protocols
# *Cause:
#   - The replication protocol is different from that on the peer server.
# *Action:
#   - Check the replication protocol version, endian, and 32/64 bit.

113, rpERR_FATAL_ThrCondInit = Failed to invoke the cond_init() system function
# *Cause: The system failed to invoke the cond_init() function.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

#114, rpERR_FATAL_ThrCondSignal = Failed to invoke the cond_signal() system function, Error number: (<0%d>).
# *Cause: The system failed to invoke the cond_signal() function.
# *Action: 
#   - Refer to the error number for the exact cause of failure. Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

#115, rpERR_FATAL_ThrCondDestroy = Failed to invoke the cond_destroy() system function
# *Cause: Invalid condition value
# *Action: Please send a bug report to the vendor.

#116,  rpERR_FATAL_ThrJoin = Failed to invoke the pthread_join() system function
# *Cause: The system failed to join a thread.
# *Action: Please send a bug report to the vendor.

117,rpERR_ABORT_TIMEOUT_EXCEED = Timeout exceed.
# *Cause :
#   - A timeout occurred while the replication Sender and Receiver were communicating.
# *Action :
#   - Set a larger value for the REPLICATION_RECEIVE_TIMEOUT property. 

118,rpERR_ABORT_INVALID_XSN = [Sender] Invalid XSN[<0%ld>] > Current[<1%ld>] occurred
# *Cause:
#   - The restart SN value of the replication Sender is invalid.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

119,rpERR_ABORT_RP_SENDER_UPDATE_XSN = [Sender] Failed to update XSN
# *Cause:
#   - The replication Sender failed to update the restart SN value.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

120,rpERR_ABORT_RP_TRANSACTION_TABLE_IN_INSERT = [Receiver] replication transaction table error during insert() operation
# *Cause:
#   - The replication Receiver failed to insert data into the transaction table.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

121,rpERR_ABORT_RP_FIND_IN_TRANSTABLE_REMOVE = [Receiver] find error in transaction table during remove() operation
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

122,rpERR_ABORT_NOT_HAVE_HOST = Invalid Host [<0%s>, <1%d>]
# *Cause :
#   - Host Information (IP, Port) doesn't belong to this replication.
# *Action :
#   - Attempt this operation again using the correct host information.

123,rpERR_ABORT_COLUMN_COUNT_MISMATCH = <0%s> column count mismatch of <1%s> table. ( column count of received xlog is <2%d> and column count of meta is <3%d>)
# *Cause :
#   -  The sender and receiver table column counts do not match.
# *Action :
#   - Reorganize replicated tables and recreate the replication after checking both the sender and receiver table meta information.

124,rpERR_ABORT_COLUMN_ID_MISMATCH = <0%s> <1%d>'th column ID mismatch of <2%s> table. ( column ID of received xlog is <3%d> and  column ID of meta is <4%d>)
# *Cause :
#   - The sender and receiver table column IDs do not match.
# *Action :
#   - Reorganize replicated tables and recreate the replication after checking both the sender and receiver table meta information.

#125,rpERR_ABORT_RP_TID_NOT_EXIST_IN_HASH_REMOVE = [Receiver] transaction ID does not exist in hash remove().
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#126,rpERR_ABORT_INVALID_XTYPE_APPEND_XLOG = [Sender] Invalid xlog type [<0%u>] in appendXLog()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#127,rpERR_ABORT_INVALID_XTYPE_ANALYZE_XLOG = [Receiver] Invalid xlog type [<0%u>] in analyzeXLog()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#128,rpERR_ABORT_INVALID_XTYPE_APPEND_HEADER = [Sender] Invalid xlog type [<0%u>] in appendHeader()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

#129,rpERR_ABORT_INVALID_XTYPE_ANALYZE_HEADER = [Receiver] Invalid xlog type [<0%u>] in analyzeHeader()
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

130,rpERR_ABORT_INDEX_COUNT_IS_NOT_AVAILABLE = [Meta] Index Count isn't available [<0%u>]
# *Cause:
#   - Index count mismatch
# *Action:
#   - Check the number of table indexes.

131,rpERR_ABORT_ADD_SESSION = [Network] Add Session operation failed
# *Cause:
#   - cmiAddSession() returned an error
# *Action:
#   - Please send a bug report to the vendor.

132,rpERR_ABORT_SET_LINK = [Network] Set link to session failed
# *Cause:
#   - The session already has another link.
# *Action:
#   - Please send a bug report to the vendor.

133,rpERR_ABORT_FREE_LINK = [Network] Free link operation failed
# *Cause:
#   - Failed to free network memory resources.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

134,rpERR_ABORT_FREE_DISPATCHER = [Network] Free dispatcher operation failed
# *Cause:
#   - Failed to free network dispatcher resources.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

135,rpERR_ABORT_SHUTDOWN_LINK = [Network] Shutdown link operation failed
# *Cause:
#   - Failed to shut down the network because the corresponding party has terminated first.
# *Action:
#   - You may safely ignore this message.

136,rpERR_ABORT_CLOSE_LINK = [Network] Close link operation failed
# *Cause:
#   - Failed to close a network socket.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

137,rpERR_ABORT_ALLOC_LINK = [Network] Alloc link operation failed
# *Cause:
#   - Failed to allocate network resources.
# *Action:
#   - Check the state of system resources. If they are insufficient, take appropriate action.

138,rpERR_ABORT_CONNECT_FAIL = [Network] Connection failed
# *Cause: 
#   - The system failed to invoke connect() function.
# *Action:
#   - Verify that the corresponding server is alive, or check the network.

139,rpERR_ABORT_SEND_FAIL = [Network] Send operation failed
# *Cause:
#   - The replication Sender failed to send data to the corresponding server.
# *Action:
#   - Verify that the corresponding server is alive, or check the network status.

140,rpERR_ABORT_OPEN_LOB_CURSOR = [SmExec] A failure occurred while opening LOB Cursor
# *Cause:
#   - The replication Receiver tried to open a LOB cursor to apply LOB data, but failed.
# *Action:
#   - Check whether the LOB data of the replication target table is the same and take appropriate action.

141,rpERR_ABORT_CLOSE_LOB_CURSOR = [SmExec] A failure occurred while closing the LOB Cursor
# *Cause:
#   - The replication Receiver tried to close a LOB cursor to finish applying LOB data, but failed.
# *Action:
#   - Check whether the LOB data of the replication target table is the same and take appropriate action.

142,rpERR_ABORT_LOB_PREPARE_WRITE = [SmExec] A failure occurred while preparing for LOB writing
# *Cause:
#   - The replication Receiver tried to prepare to write the LOB cursor to finish applying LOB data, but failed.
# *Action:
#   - Check whether the LOB data of the replication target table is the same and take appropriate action.

143,rpERR_ABORT_LOB_FINISH_WRITE = [SmExec] A failure occurred while finishing LOB writing
# *Cause:
#   - The replication Receiver tried to finish writing to the LOB cursor to finish applying LOB data, but failed.
# *Action:
#   - Check whether the LOB data of the replication target table is the same and take appropriate action.

144,rpERR_ABORT_LOB_PARTIAL_WRITE = [SmExec] A failure occurred while writing the LOB piece
# *Cause:
#   - The replication Receiver tried to partially write to the LOB cursor to finish applying LOB data, but failed.
# *Action:
#   - Check whether the LOB data of the replication target table is the same and take appropriate action.

145,rpERR_ABORT_LOB_GET_LENGTH = [SmExec] A failure occurred while getting the LOB piece length
# *Cause:
#   - The replication Receiver tried to get the length of the LOB cursor to finish applying LOB data, but failed.
# *Action:
#   - Check whether the LOB data of the replication target table is the same and take appropriate action.

146,rpERR_ABORT_LOB_READ = [SmExec] A failure occurred while reading an LOB piece
# *Cause:
#   - The replication Receiver tried to read the LOB cursor to finish applying LOB data, but failed.
# *Action:
#   - Check whether the LOB data of the replication target table is the same and take appropriate action.

147,rpERR_ABORT_NOT_FOUND_LOCATOR = [SmExec] Locator [<0%lu>] not found
# *Cause:
#   - A LOB locator was not found.
# *Action:
#   - Please send a bug report to the vendor.

148,rpERR_ABORT_RP_SENDER_NOT_FOUND_TABLE = The given table isn't found in the meta.
# *Cause:
#   - Unable to find the table in the meta.
# *Action:
#   - Check the altibase_rp.log file and replication definition.

149,rpERR_ABORT_RP_RECEIVER_NOT_FOUND_TABLE = The given table isn't found in the meta.
# *Cause:
#   - Unable to find the table in the meta.
# *Action:
#   - Check the altibase_rp.log file and replication definition.

150,rpERR_ABORT_RP_RECEIVER_NOT_FOUND_COLUMN = The given column isn't found in the meta information.
# *Cause:
#   - Unable to find the column in the meta information.
# *Action:
#   - Check the altibase_rp.log file and replication definition.

151,rpERR_ABORT_GET_MODULE = The module couldn't be found on the server using the module ID.
# *Cause:
#   - Unable to find the module pointer on the server.
# *Action:
#   - Verify the altibase_rp.log file and replication definition.

152,rpERR_ABORT_SENDER_INVALID_XLOG_TYPE = [Sender] Invalid xlog type [<0%u>] in sender analysis
# *Cause:
#   - The XSN read and created by the replication Sender is invalid. 
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

153,rpERR_ABORT_RECEIVER_INVALID_XLOG_TYPE = [Receiver] Invalid xlog type [<0%u>] in receiver analysis
# *Cause:
#   - The replication Receiver received an invalid XLog.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

154,rpERR_ABORT_ITEM_NOT_EXIST = [Meta] Replication meta item doesn't exist [Replication Name:<0%s>]
# *Cause:
#   - The replication metadata item does not exist due to a database error
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

155,rpERR_IGNORE_RP_META_DIFF = Replication meta is different
# *Cause:
#   - The replication meta is different from that on the peer server.
# *Action:
#   - Check the Altibase version and the replication protocol version.

156,rpERR_ABORT_CONVERT_ENDIAN = Endian conversion operation failed
# *Cause:
#   - A server with different Endians failed to convert the Endians during replication.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

#157,rpERR_ABORT_APPLY_DB = [Receiver] DML operation failed.
# *Cause:
#   - Internal server error.
# *Action:
#   - Please send a bug report to the vendor.

158,rpERR_ABORT_NOT_EXIST_TABLE = [Receiver] The specified table doesn't exist.
# *Cause:
#   - The table does not exist.
# *Action:
#   - Check the altibase_rp.log file.

159,rpERR_ABORT_RP_ALREADY_STARTED_RECEIVER = The receiver has already been started.
# *Cause:
#   - The receiver has already been started.
# *Action:
#   - Check whether the receiver has already been started.

160,rpERR_ABORT_GIVEUP_SENDER_STOP = [Sender] Stopping <0%s> sender thread (sender read = SN[<1%ld>], current = SN[<2%ld>], distance from checkpoint = <3%ld>)
# *Cause:
#   - The system tried to restart the sender thread with the current log file as specified by the REPLICATION_MAX_LOGFILE property.
# *Action:
#   - Check the status of the replication object, then synchronize the replication data and then restart replication.

161,rpERR_ABORT_GIVEUP_SENDER_RESET = [Sender] Resetting <0%s> sender XSN (sender XSN = SN[<1%ld>], current = SN[<2%ld>], distance from checkpoint = <3%ld>)
# *Cause:
#   - The system tried to change the current log file as specified by the REPLICATION_MAX_LOGFILE property.
# *Action:
#   - Check the status of the replication object, then synchronize the replication data and then restart replication.

162,rpERR_FATAL_ThrCondBroadcast = Failed to invoke a system function, cond_broadcast(), errno is (<0%d>).
# *Cause:
#   - The system failed to invoke the cond_broadcast() function.
# *Action:
#   - Please send a bug report to the vendor.

163,rpERR_ABORT_ROLE_NOT_SUPPORT_RECEIVER = [Receiver] The receiver is not supported in this role.
# *Cause:
#   - The replication receiver is not supported in this role.
# *Action:
#   - Check the role of the replication.

164,rpERR_ABORT_RECEIVER_UNEXPECTED_PROTOCOL = [Receiver] Unexpected replication protocol [<0%u>]
# *Cause:
#   - The replication Receiver received an undefined protocol for replication. 
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

165,rpERR_ABORT_RECEIVER_TX_NOT_BEGIN = [Receiver] Transaction has not begun. Received XLog [Type:<0%d>, TID:<1%u>, SN:<2%ld>]
# *Cause:
#   - The replication Receiver received the XLog of a transaction that has not yet started.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

166,rpERR_ABORT_LOGBUFFER_ALLOC = Replication log buffer memory allocation failed.
# *Cause:
#   - The system failed to allocate memory.
# *Action:
#   - Verify that sufficient memory is available.

167,rpERR_ABORT_RP_SENDER_UPDATE_INVALID_MAX_SN = [Sender] Failed to update INVALID MAX SN
# *Cause:
#   - Failed to update replication metadata (INVALID MAX SN) due to a database error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

168,rpERR_ABORT_RECOVERY_INFO_EXIST = Recovery information already exists.
# *Cause: 
#   - Adding a table into or deleting a table from the replication is prohibited if the recovery option is enabled.
# *Action:
#   - Disable the recovery option or create another replication and try again.

169,rpERR_ABORT_RECOVERY_ALREADY_STARTED = The recovery sender has already been started.
# *Cause:
#   - The recovery sender has already been started.
# *Action:
#   - Wait for complete recovery or cancel the recovery.

170,rpERR_ABORT_RP_MAXIMUM_RECOVERYITEM_REACHED = Irreplicable recovery information
# *Cause:
#   - The amount of available recovery information on the system exceeds the maximum limit.
# *Action: Retry after
#   - 1. Increase value of the REPLICATION_MAX_COUNT property.
#   - 2. Remove the replication which enables other RECOVERY option. 
#   - 3. Disable RECOVERY option.

#171,rpERR_ABORT_GIVEUP_RECEIVER_STOP = [Receiver] Stopping <0%s> receiver thread (recovery info min sn = SN[<1%ld>], distance from checkpoint = <2%ld>)
# *Cause:
#   - The system tried to restart the receiver thread according to the REPLICATION_RECOVERY_MAX_LOGFILE property.
# *Action:
#   - N/A

172,rpERR_ABORT_GIVEUP_RECOVERY_DESTROY = [Receiver] Destroy <0%s> recovery information (recovery info min sn = SN[<1%ld>], distance from checkpoint = <2%ld>)
# *Cause:
#   - The number of logs that the replication Receiver stored for replication recovery has exceeded REPLICATION_MAX_LOGFILE.
#   - Recovery data and log files necessary for replication recovery have been dropped. 
# *Action:
#   - To use the replication recovery option, increase the value of the REPLICATION_MAX_LOGFILE property.

173,rpERR_ABORT_RP_RECOVERY_REQUEST = Failed to send <0%s> recovery request
# *Cause:
#   - Failed to request replication recovery either because the corresponding server has not started or there is a network error
# *Action:
#   - Replication recovery is impossible if the corresponding server has been terminated.
#   - To disable this option, set REPLICATION_RECOVERY_MAX_TIME to 0, and restart. 
#   - If a network error has occurred, increase REPLICATION_RECOVERY_REQUEST_TIMEOUT, and restart replication.
#   - If connection fails, check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

#174,rpERR_ABORT_ARRANGE_XLOG = XLog arrangement failed.
# *Cause:
#   - Internal server error.
# *Action:
#   - Please send a bug report to the vendor.

#175,rpERR_ABORT_CONVERT_SIZE = Size conversion operation failed.
# *Cause:
#   - Internal server error.
# *Action:
#   - Please send a bug report to the vendor.

176,rpERR_ABORT_MISMATCH_OLD_ITEMS_COUNT = Old items count mismatch (<0%d>:<1%d>)
# *Cause:
#   - The number of rows selected from SYS_REPL_OLD_ITEMS_ is invalid due to a database error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

#177,rpERR_ABORT_INVALID_CONDITION_NODE = Failed to compare conditions
# *Cause:
#   - Failed to compare conditions.
# *Action:
#   - Check the condition node.

178,rpERR_ABORT_CONDITION_COLUMN_IS_NOT_FOUND = Condition column not found.
# *Cause:
#   - The condition column could not be found.
# *Action:
#   - Check the existence of a condition column.

179,rpERR_ABORT_ART= Error generated by Automatic Recovery Test(ART)
# *Cause:  Ignore.
# *Action: Ignore.

180,rpERR_FATAL_ThrMutexInit = Unable to initialize a mutex.
# *Cause: The system failed to initialize a mutex.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

181,rpERR_FATAL_ThrMutexDestroy = Failed to invoke the mutex_destroy() system function
# *Cause: The system failed to remove a mutex.
# *Action: Please send a bug report to the vendor.

182,rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE = Table not found in the meta [SN=<0%ld>, Table OID=<1%ld>]
# *Cause:
#   - Failed to find the table with the table OID from replication Receiver metadata due to a database error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

183,rpERR_ABORT_RECEIVER_NOT_FOUND_PK_COLUMN = PK Column not found in the meta [SN=<0%ld>, Table OID=<1%ld>, PK Column Seq=<2%d>]
# *Cause:
#   - Failed to find the primary key column from metadata due to a database error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

184,rpERR_ABORT_RECEIVER_NOT_FOUND_COLUMN = Column not found in the meta [SN=<0%ld>, Table OID=<1%ld>, Column ID=<2%d>]
# *Cause:
#   - Failed to find the column from metadata due to a database error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

185,rpERR_ABORT_RP_NOT_DROP_ONE_HOST = You cannot drop only one host.
# Cause: 
# - Replication cannot be dropped when only one host is remaining.
# *Action:
#  - Drop the replication object, or insert another host and drop the host.

186,rpERR_ABORT_RP_ABORT_PSM_SVP =  [Receiver] smiTrans abort PSM savepoint error in run()
# *Cause:
#   - The replication receiver failed to abort to the savepoint.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

187,rpERR_ABORT_MISMATCH_OFFLINE_LOG_LFGCOUNT = Offline log LFG counts do not match.
# *Cause:
#   - The sender and receiver offline log LFG counts do not match.
# *Action:
#   - Check the LFG counts.

188,rpERR_ABORT_MISMATCH_OFFLINE_LOG_OSVERSION = Offline log system OS versions do not match.
# *Cause:
#   - The sender and receiver offline log system OS versions do not match.
# *Action:
#   - Confirm that the operating systems of Active and Standby server are the same or unset OFFLINE option.

189,rpERR_ABORT_MISMATCH_OFFLINE_LOG_SMVERSION = Offline log SM versions do not match.
# *Cause:
#   - The sender and receiver offline log SM versions do not match.
# *Action:
#   - Confirm that the database versions of Active and Standby server are the same or unset OFFLINE option.

190,rpERR_ABORT_MISMATCH_OFFLINE_LOG_COMPILEBIT = Offline log compile bits do not match.
# *Cause:
#   - The sender and receiver offline log compile bits do not match.
# *Action:
#   - Confirm that the compile bit versions of Active and Standby server are the same or unset OFFLINE option.

191,rpERR_ABORT_SENDER_NOT_FOUND_COLUMN = Column not found in the meta.
# *Cause:
#   - Failed to find the column from metadata due to a database error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

192,rpERR_ABORT_MISMATCH_OFFLINE_LOG_FILESIZE = Offline log file size does not match.
# *Cause:
#   - The sender and receiver offline log file sizes do not match.
# *Action:
#   - Check the log file size.

193,rpERR_ABORT_INVALID_CM_VARIABLE_SIZE = Invalid CM variable size [<0%s>]
# *Cause:
#   - Internal communication logic error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

194,rpERR_ABORT_RP_NOT_FOUND_OFFMETA = The meta information for the offline sender cannot be found in memory.
# *Cause:
#   - The remote sender has not been started since the local server was started.
# *Action:
#   - Start the replication in the remote server.

195,rpERR_ABORT_INVALID_OFFLINE_RESTARTSN = [Offline Sender] Failed to get valid XSN
# *Cause:
#   - The replication stopped before XSN update.
# *Action:
#   - In this case, you cannot use the offline sender. 

196,rpERR_ABORT_NOT_EXIST_REPL_ITEM = Replication items not found.
# *Cause:
#   - When the ALTER REPLICATION DROP TABLE statement is executed, the table is not in the replication.
#   - When the ALTER REPLICATION REP START statement is executed, the table on the sender (Active) side is not in the receiver (standby).
# *Action:
#   - Ensure that the replication table exists on both sides or that the table is included in the replication.

197, rpERR_ABORT_TX_ALLOC_TIMEOUT = [Receiver] Transaction allocation exceeds timeout.
# *Cause:
#   - Replication failed to begin a transaction for data replication because of no available transaction.
# *Action:
#   - Check if lots of concurrent transactions are entered. 
#   - Check if there are many sessions that have not ended transaction.
#   - Increase TRANSACTION_TABLE_SIZE to a multiple of 2 and restart the system.

198, rpERR_ABORT_INVALID_CONT_LOG = Invalid log exists before current analyzing log [SN:<0%lu>].
# *Cause: The previously analyzed log has an invalid continue flag.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

199, rpERR_ABORT_RP_OFFLINE_SENDER_ABNORMALLY_EXIT = Offline sender exits without finishing some jobs.
# *Cause: An offline replication Sender has exited without finishing some jobs.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

200, rpERR_ABORT_RP_WRONG_OPERATION_TYPE = Invalid Operation type [<0%u>] of replication protocol.
# *Cause: The replication protocol's operation type is invalid.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

201, rpERR_ABORT_RP_OVERFLOW = Size of log record is grater than size of replication log buffer.
# *Cause: 
#   - Replication is delayed or replication log buffer size is too small. 
# *Action:
#   - Increase REPLICATION_LOG_BUFFER_SIZE and restart the system.
#   - Find and resolve cause of the delay.

202, rpERR_IGNORE_RP_NO_SPACE = There is not available space for replication log buffer.
# *Cause: 
#   - Replication is delayed or replication log buffer size is too small. 
# *Action:
#   - Increase REPLICATION_LOG_BUFFER_SIZE and restart the system.
#   - Find and resolve cause of the delay.

203, rpERR_ABORT_REPLICATION_NAME_MISMATCH = The replication name does not match [<0%s>:<1%s>].
# *Cause: The replication name does not match.
# *Action: Check the replication's name.

204, rpERR_ABORT_CONFLICT_RESOLUTION = The conflict resolution of the replicated table does not match [<0%d>:<1%d>].
# *Cause: The master/slave conflict resolution scheme for this replication object is not allowed.
# *Action: Check the conflict resolution scheme of the replication object.

205, rpERR_ABORT_REPLICATION_ITEM_COUNT_MISMATCH = The replication's item count does not match [<0%d>:<1%d>].
# *Cause: The replication item count mismatch.
# *Action: Check the replication items.

206, rpERR_ABORT_ROLE_MISMATCH = The replication's role does not match [<0%d>:<1%d>].
# *Cause: The replication role mismatch.
# *Action: Check the replication role.

207, rpERR_ABORT_TRANSACTION_TABLE_SIZE_MISMATCH = The transaction table size of the replication does not match [<0%u>:<1%u>].
# *Cause: Replication transaction table size mismatch.
# *Action: Check the transaction table size.

208, rpERR_ABORT_OPTION_MISMATCH = The replication's option does not match [<0%d>:<1%d>].
# *Cause: The replication's option mismatch.
# *Action: Check the replication's option.

209, rpERR_ABORT_CHARACTER_SET_MISMATCH = The character set of the database does not match. (DB=[<0%s>:<1%s>], National=[<2%s>:<3%s>]).
# *Cause: Database character set mismatch.
# *Action: Check the character set of the database.

210, rpERR_ABORT_PRIMARY_KEY_COUNT_MISMATCH = The primary key column count of the replicated table does not match [<0%s>(<1%d>):<2%s>(<3%d>)].
# *Cause: The primary key column count of the replicated table mismatch.
# *Action: Check the primary key of the replicated table.

211, rpERR_ABORT_USER_NAME_MISMATCH = The user name of the replicated table's owner does not match [<0%s>(<1%s>):<2%s>(<3%s>)].
# *Cause: Replicated table's owner user name mismatch.
# *Action: Check the user name of the replicated table's owner.

212, rpERR_ABORT_TABLE_NAME_MISMATCH = The replicated table name does not match [<0%s>:<1%s>].
# *Cause: Replicated table name mismatch.
# *Action: Check the replicated table name.

213, rpERR_ABORT_PARTITION_NAME_MISMATCH = The partition name of the replicated table does not match [<0%s>:<1%s>].
# *Cause: Replicated table partition name mismatch.
# *Action: Check the partition name of the replicated table.

214, rpERR_ABORT_PARTITION_METHOD_MISMATCH = The partition method of the replicated table does not match [<0%s>(<1%d>):<2%s>(<3%d>)].
# *Cause: Replicated table partition method mismatch.
# *Action: Check the partition method of the replicated table.

215, rpERR_ABORT_PARTITION_ORDER_MISMATCH = The partition order of the replicated table does not match [<0%s>(<1%d>):<2%s>(<3%d>)].
# *Cause: Replicated table partition order mismatch.
# *Action: Check the partition order of the replicated table.

216, rpERR_ABORT_UNIQUE_INDEX_COUNT_MISMATCH = The unique index count of the replicated table does not match [<0%s>:<1%s>].
# *Cause: Replicated table unique index count mismatch.
# *Action: Check the unique index count of the replicated table.

217, rpERR_ABORT_NON_REPLICATION_COLUMN_NOT_NULL = The non-replicated column of the replicated table has NOT NULL constraint. [<0%s>:<1%s>(<2%s>)].
# *Cause: The replicated table's non-replicated column has a NOT NULL constraint.
# *Action: Check the non-replicated column of the replicated table.

218, rpERR_ABORT_COLUMN_TYPE_MISMATCH = The column type of the replicated table does not match. [<0%s>.<1%s>(<2%u>):<3%s>.<4%s>(<5%u>)].
# *Cause: Replicated table column type mismatch.
# *Action: Check the column type of the replicated table.

219, rpERR_ABORT_COLUMN_SIZE_MISMATCH = The column size of the replicated table does not match. [<0%s>(Name:<1%s>, Size:<2%d>, Precision:<3%d>, Scale:<4%d>):<5%s>(Name:<6%s>, Size:<7%d>, Precision: <8%d>, Scale: <9%d>)].
# *Cause: Replicated table column size mismatch.
# *Action: Check the column size of the replicated table.

220, rpERR_ABORT_ENCRYPT_PRECISION_MISMATCH = The encrypt precision of the replicated table's column does not match. [<0%s>(Name:<1%s>, Precision:<2%d>):<3%s>(Name:<4%s>, Precision: <5%d>)].
# *Cause: Replicated table's column encryption precision mismatch.
# *Action: Check the the encrypt precision of the replicated table's column.

221, rpERR_ABORT_COLUMN_CHARACTER_SET_MISMATCH = The column character set of the replicated table does not match. [<0%s>.<1%s>(<2%u>):<3%s>.<4%s>(<5%u>)].
# *Cause: Replicated table's column character set mismatch.
# *Action: Check the column character set of the replicated table.

222, rpERR_ABORT_COLUMN_CONSTRAINT_MISMATCH = NOT NULL constraints of the replicated table's column does not match. [<0%s>.<1%s>(<2%d>):<3%s>.<4%s>(<5%d>)].
# *Cause:  Replicated table's NOT NULL constraints mismatch.
# *Action: Check the column's constraint of the replicated table.

223, rpERR_ABORT_POLICY_MISMATCH = The encrypt policy name of the replicated table's column does not match. [<0%s>(Name:<1%s>, Policy:<2%s>):<3%s>(Name:<4%s>, Policy:<5%s>)].
# *Cause: Replicated table's column encryption policy name mismatch.
# *Action: Check the encrpyt policy name of the replicated table's column.

224, rpERR_ABORT_POLICY_CODE_MISMATCH = The encrypt policy code of the replicated table's column does not match. [<0%s>(Name:<1%s>, Policy:<2%s>):<3%s>(Name:<4%s>, Policy:<5%s>)].
# *Cause: Replicated table's column encryption policy code mismatch.
# *Action: Check the encryption policy name of the replicated table's column.

225, rpERR_ABORT_ECC_POLICY_MISMATCH = The encrypted comparision code policy name of the replicated table's column does not match. [<0%s>(Name:<1%s>, ECC Policy:<2%s>):<3%s>(Name:<4%s>, ECC Policy:<5%s>)].
# *Cause: The replicated table's column encryption comparison code policy name mismatch.
# *Action: Check the encrypted comparison code policy name of the replicated table's column.

226, rpERR_ABORT_ECC_POLICY_CODE_MISMATCH = The encrypted comparison policy code of the replicated table's column does not match. [<0%s>(Name:<1%s>, ECC Policy:<2%s>):<3%s>(Name:<4%s>, ECC Policy:<5%s>)].
# *Cause: Replicated table's column encryption comparison code mismatch.
# *Action: Check the encrypted comparison code of the replicated table's column.

227, rpERR_ABORT_INDEX_NAME_MISMATCH = The index name of the replicated table does not match. [<0%s>(Name:<1%s>):<2%s>(Name:<3%s>)].
# *Cause: Replicated table index name mismatch.
# *Action: Check the index name of the replicated table.

228, rpERR_ABORT_INDEX_TYPE_MISMATCH = The index type of the replicated table does not match. [<0%s>(Name:<1%s>, Type:<2%u>):<3%s>(Name:<4%s>, Type:<5%u>)].
# *Cause: Replicated table index name mismatch.
# *Action: Check the index name of the replicated table.

229, rpERR_ABORT_INDEX_UNIQUENESS_MISMATCH = The index uniqueness of the replicated table does not match. [<0%s>(Name:<1%s>, Uniqueness:<2%d>):<3%s>(Name:<4%s>, Uniqueness:<5%d>)].
# *Cause: Replicated table index uniqueness mismatch.
# *Action: Check the index uniqueness of the replicated table.

230, rpERR_ABORT_LOCAL_INDEX_UNIQUENESS_MISMATCH = The local index uniqueness of the replicated table does not match. [<0%s>(Name:<1%s>, Uniqueness:<2%d>):<3%s>(Name:<4%s>, Uniqueness:<5%d>)].
# *Cause: Replicated table local index uniqueness mismatch.
# *Action: Check the local index uniqueness of the replicated table.

231, rpERR_ABORT_INDEX_COLUMN_COUNT_MISMATCH = The index column count of the replicated table does not match. [<0%s>(Name:<1%s>, Count:<2%u>):<3%s>(Name:<4%s>, Count:<5%u>)].
# *Cause: Replicated table index column count mismatch.
# *Action: Check the index column count of the replicated table.

232, rpERR_ABORT_INDEX_COLUMN_ID_MISMATCH = The index column ID of the replicated table does not match. [<0%s>(Name:<1%s>, ID:<2%u>):<3%s>(Name:<4%s>, ID:<5%u>)].
# *Cause: Replicated table index column ID mismatch.
# *Action: Check the index column ID of the replicated table.

233, rpERR_ABORT_INDEX_COLUMN_ORDER_MISMATCH = The index column order of the replicated table does not match. [<0%s>(Name:<1%s>, ID:<2%d>, Flag:<3%d>):<4%s>(Name:<5%s>, ID:<6%d>, Flag:<7%d>)].
# *Cause: Replicated table index column order mismatch.
# *Action: Check the index column order of the replicated table.

234, rpERR_ABORT_MIXED_UNIQUE_INDEX_COLUMN = The unique index of the replicated table is composed of replication column and non-replication column [<0%s>(<1%s>)].
# *Cause: The unique index of the replicated table is composed of a replication column and a non-replication column.
# *Action: Check the unique index of the replicated table.

235, rpERR_IGNORE_EXIT_FLAG_SET = Exit flag is set.
# *Cause: The exit flag has been set.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

#236,rpERR_IGNORE_RP_CHILD_SENDER_STOP = [Sender] Stop parallel child sender thread [name: <0%s>, child id: <1%s>] at [<2%ld>], Restart SN[<3%ld>] 
# *Cause:
#   - The parallel child sender thread has stopped, either normally or abnormally, due to a network issue.
# *Action:
#   - This is not an error of Altibase.

237,rpERR_ABORT_CANCEL_COMMIT_BY_REPL = Transaction's commit was canceled by replication conflict.
# *Cause: 
#   - Replication conflict occurred in the standby host.
# *Action: 
#   - Execute rollback and check your application update logic.

238,rpERR_IGNORE_NOT_ACITVE_TX = The transaction does not exist in transaction table of the sender.
# *Cause: 
#   - The Sender of the replication is not alone. Another Sender of the replication is processing the transaction.
# *Action: 
#   - This is not an error of Altibase.

239,rpERR_ABORT_ABNORMAL_TYPE = [<0%s>:<1%u>] sender have abnormal type.
# *Cause:
#   - The replication Sender type is set to an invalid value.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

240,rpERR_IGNORE_RP_SENDER_STOP = [Sender] Stop sender thread <0%s>:<1%u> at [<2%ld>], Restart SN[<3%ld>] 
# *Cause:
#   - This error occurs when the replication Sender stops.
# *Action:
#   - You may safely ignore this message.

241,rpERR_ABORT_RP_SENDER_UPDATE_REMOTE_FAULT_DETECT_TIME = [Sender] Failed to update REMOTE_FAULT_DETECT_TIME
# *Cause:
#   - Failed to update the REMOTE_FAULT_DETECT_TIME column in the SYS_REPLICATIONS_ meta table due to a database error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

242,rpERR_ABORT_SENDER_APPLY_ABNORMAL_EXIT = [Sender] The apply thread exits abnormally.
# *Cause:
#   - While the replication Sender was terminating, a related thread terminated abnormally due to a network error. 
# *Action:
#   - Some data may have been unsent. Restart the replication Sender and then stop it, to terminate it normally.

243,rpERR_ABORT_FAILBACK_SENDER_NOT_EXIST = [Receiver] Failback sender does not exist.
# *Cause:
#   - Failback sender does not exist.
# *Action:
#   - Check the status of the replication sender.

244,rpERR_ABORT_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED = [Receiver] The queue for failback is full.
# *Cause:
#   - The queue for failback is full.
# *Action:
#   - Check status of the replication sender.

245,rpERR_ABORT_FAILBACK_INCREMENTAL_SYNC_MISMATCH = The REPLICATION_FAILBACK_INCREMENTAL_SYNC property mismatch. [<0%u>:<1%u>]
# *Cause:
#   - The REPLICATION_FAILBACK_INCREMENTAL_SYNC property mismatch.
# *Action:
#   - Check the REPLICATION_FAILBACK_INCREMENTAL_SYNC property.

246,rpERR_ABORT_RP_LOG_BUFFER_PROPERTY_CONFLICT = The replication log buffer cannot be used when the REPLICATION_SYNC_LOG property is 1.
# *Cause:
#   - The REPLICATION_LOG_BUFFER_SIZE property is greater than 0 and the REPLICATION_SYNC_LOG property is 1.
# *Action:
#   - Check the REPLICATION_LOG_BUFFER_SIZE and REPLICATION_SYNC_LOG properties.

247,rpERR_ABORT_NOT_FOUND_RECORD = [Receiver] Unable to find record in <0%s> function 
# *Cause:
#   - The record does not exist.
# *Action:
#   - Find the conflicted record in altibase_rp.log or altibase_rp_conflict.log file and match the data or find and resolve the cause of the conflict (such as app logic).
#   - In case that there is no primary key information for the log about conflict in altibase_rp.log or altibase_rp_conflict.log file, set the third bit of RP_MSGLOG_FLAG or the RP_CONFLICT_MSGLOG_FLAG to output the primary key information into the log. 
#   - It is possible to match the data using tools such as AltiComp if conflict has already occurred.

248,rpERR_ABORT_RP_SENDER_ADD_XLOG = [Sender] Failed to add a XLog [TID:<0%u>, SN:<1%lu>, Type:<2%d>, Log Type:<3%d>, Change Log Type:<4%d>]
# *Cause:
#   - The replication Sender failed to create an XLog from a log.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

249,rpERR_IGNORE_TIMESTAMP_INSERT_CONFLICT = [Receiver] An insert conflict occurred during timestamp conflict resolution.
# *Cause:
#   - The timestamp value of an XLog is older than an existing one.
# *Action:
#   - This behavior is normal and can be expected from time to time.

250,rpERR_IGNORE_TIMESTAMP_UPDATE_CONFLICT = [Receiver] An update conflict occurred during timestamp conflict resolution.
# *Cause:
#   - The timestamp value of an XLog is older than an existing one.
# *Action:
#   - This behavior is normal and can be expected from time to time.

251,rpERR_ABORT_UNEXPECTED_HANDSHAKE_ACK = Unexpected replication handshake ACK [Result:<0%u>]
# *Cause:
#   - The replication Sender received an unexpected ACK while handshaking the replication Receiver.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

252,rpERR_ABORT_HANDSHAKE_DISCONNECT = Disconnection during handshaking [<0%s>]
# *Cause:
#   - A network error has occurred.
# *Action:
#   - Check the network status and ensure that all systems are connected to the network.

253,rpERR_ABORT_SENDER_INFO_NOT_EXIST = No Sender information for replication <0%s>.
# *Cause:
#   - There is no information about the replication Sender.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

254,rpERR_ABORT_RPC_REPLICATION_ALREADY_STARTED = Replication has already started.
# *Cause:
#   - Replication is currently underway.
# *Action:
#   - Stop the current replication to start again.

255,rpERR_ABORT_RPC_NOT_SUPPORT_REPLICATION_DDL = This replication DDL is no longer supported.
# *Cause:
#   - An attempt to use a deprecated DDL statement was made.
# *Action:
#   - Refer to the Replication User's Manual.

256,rpERR_ABORT_RPC_DUPLICATE_REPLICATION = Duplicate replication names. The replication name already exists in the database.
# *Cause:
#   - The replication name already exists in the database.
#   - The IP address and the port number must be unique.
# *Action:
#   - Use another replication name.
#   - Consult the meta tables in order to find replications that use the same IP address and port number.

257,rpERR_ABORT_RPC_DUPLICATE_REPL_OPTION = Duplicate option names.
# *Cause:
#   - Duplicate option names exist.
# *Action:
#   - Check option names for duplicates.

258,rpERR_ABORT_RPC_MAX_REPLICATION_COUNT = No more replications may be created. A database cannot have more than the maximum number of replications.
# *Cause:
#   - A database cannot have more than the maximum number of replications.
# *Action:
#   - Decrease the number of replications or increase the value of REPLICATION_MAX_COUNT property.

259,rpERR_ABORT_RPC_ROLE_NOT_SUPPORT_UNIX_DOMAIN = UNIX domain socket is not supported in this role.
# *Cause:
#   - This role does not support UNIX_DOMAIN in the 'WITH' clause.
# *Action:
#   - Check the role of the replication.

260,rpERR_ABORT_RPC_ROLE_SUPPORT_ONE_SOCKET_TYPE = One socket type is supported in this role.
# *Cause:
#   - This role supports one socket type.
# *Action:
#   - Check the role of the replication.

261,rpERR_ABORT_RPC_ROLE_SUPPORT_ONE_UNIX_DOMAIN = Only one UNIX domain socket is supported in this role.
# *Cause:
#   - This role supports one UNIX domain socket.
# *Action:
#   - Check the role of the replication.

262,rpERR_ABORT_RPC_ROLE_NOT_SUPPORT_REPL_RECOVERY = Recovery option not supported in this role.
# *Cause:
#   - This role does not support the recovery option.
# *Action:
#   - Check the options or the role of the replication.

263,rpERR_ABORT_RPC_NOT_SUPPORT_REPL_OFFLINE_AND_RECOVERY = Offline option is not supported with recovery option.
# *Cause:
#   - Offline option is not supported with recovery option.
# *Action:
#   - Check the replication options.

264,rpERR_ABORT_RPC_NOT_SUPPORT_REPL_OFFLINE_AND_EAGER = The offline option is not supported with eager mode.
# *Cause:
#   - Simultaneous use of eager mode and the offline option is not supported.
# *Action:
#   - Change the option or mode of the replication.

265,rpERR_ABORT_RPC_ROLE_NOT_SUPPORT_REPL_OFFLINE = Offline option not supported in this role.
# *Cause:
#   - This role does not support the offline option.
# *Action:
#   - Check the options or the role of the replication.

266,rpERR_ABORT_RPC_ROLE_NOT_SUPPORT_DEFAULT_REPL_MODE = The default replication mode is not supported in this role.
# *Cause:
#   - This role does not support the default replication mode.
# *Action:
#   - Check the role of the replication.

267,rpERR_ABORT_RPC_INVALID_HOST_IP_PORT = The host IP address or port number is invalid.
# *Cause:
#   - Invalid host IP address or port number.
# *Action:
#   - Check the host IP address and port number.

268,rpERR_ABORT_RPC_ALREADY_EXIST_REPL_HOST = Replication hosts already exist.
# *Cause:
#   - Replication hosts already exist.
# *Action:
#   - Check the meta tables.

269,rpERR_ABORT_RPC_REPLICATED_OBJECT_TYPE = The replicated object type must be a table. <0%s>
# *Cause:
#   - Only a table may be replicated.
# *Action:
#   - Check the object type.

270,rpERR_ABORT_RPC_NOT_EXIST_USER = User not found
# *Cause:
#   - The user is not in the meta database.
# *Action:
#   - Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

271,rpERR_ABORT_RPC_NOT_EXISTS_USER = User not found : <0%s>
# *Cause:
#   - The name of the specified user was not found in the database.
# *Action:
#   - Verify that the user exists in the database.

272,rpERR_ABORT_RPC_NOT_EXIST_TABLE = Table not found
# *Cause:
#   - The table is not in the meta database.
# *Action:
#   - Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

273,rpERR_ABORT_RPC_NOT_EXISTS_TABLE = Table not found : <0%s>
# *Cause:
#   - The name of the specified table was not found in the database.
# *Action:
#   - Verify that the table exists in the database.
#   - Verify that there are no typos in the table name.

274,rpERR_ABORT_RPC_REPLICATE_TABLE_WITH_REFERENCE = Replication is not allowed on tables that have referential constraints. (<0%s>.<1%s>)
# *Cause:
#   - The user tried to replicate a table that has referential constraints.
# *Action:
#   - Verify that the table to be replicated does not have any referential constraints.

275,rpERR_ABORT_RPC_NOT_EXISTS_PRIMARY_KEY = A replicated table must have a primary key. (<0%s>.<1%s>)
# *Cause:
#   - No primary key was found in the table to be replicated.
#   - For replication, please specify a table having a primary key.
# *Action:
#   - Create a primary key for the table to be replicated.

276,rpERR_ABORT_RPC_REPL_RECOVERY_COUNT = A table can recover only one replication.
# *Cause:
#   - Table recovery already supported by another replication.
# *Action:
#   - Check the replication recovery count.

277,rpERR_ABORT_RPC_NOT_APPLICABLE_POLICY = This column policy is not compatible with replication. [<0%s>]
# *Cause:
#   - This column policy is not compatible with replication.
# *Action:
#   - Check the column policy.

278,rpERR_ABORT_RPC_CANNOT_USE_VOLATILE_TABLE = Replication not allowed on volatile tables.
# *Cause:
#   - A volatile table cannot be replicated.
# *Action:
#   - Verify that the table to be replicated is not a volatile table.

279,rpERR_ABORT_RPC_CANNOT_USE_TEMPORARY_TABLE = Temporary tables cannot be replicated.
# *Cause:
#   - An attempt was made to add a temporary table to a replication object.
# *Action:
#   - Verify that none of the tables to be replicated are temporary tables.

280,rpERR_ABORT_RPC_NOT_SUPPORT_MAX_ROWS_AND_EAGER = Replication in eager mode is not supported with the MAX ROWS table option.
# *Cause:
#   - A table with the MAX_ROWS option specified cannot be replicated in eager mode.
# *Action:
#   - Check the replication mode.

281,rpERR_ABORT_RPC_NOT_ALLOW_ADD_TABLE = An ADD TABLE statement is not allowed in recovery support replication.
# *Cause:
#   - An ADD TABLE statement is not allowed in a recovery support replication.
# *Action:
#   - Check the replication options.

282,rpERR_ABORT_RPC_NOT_EXIST_REPL_ITEM = No replication items found.
# *Cause:
#   - No replication items were found.
# *Action:
#   - Check the meta tables.

283,rpERR_ABORT_RPC_NOT_EXIST_REPLICATION = Undefined replication name. The replication name was not found in the database.
# *Cause:
#   - The replication name was not found in the database.
#   - The name is incorrect or the replication has not been created yet.
# *Action:
#   - Verify that the replication name is typed correctly.
#   - Consult meta tables to see if the same replication name exists in the database.

284,rpERR_ABORT_RPC_ALREADY_RECOVERY_SET = Recovery option already set.
# *Cause:
#   - The replication recovery option has already been set.
# *Action:
#   - Check the replication options.

285,rpERR_ABORT_RPC_ALREADY_RECOVERY_UNSET = The recovery option was already unset.
# *Cause:
#   - The replication recovery option was already unset.
# *Action:
#   - Check the replication options.

286,rpERR_ABORT_RPC_NOT_ALLOW_DROP_TABLE = An DROP TABLE statement is not allowed in recovery support replication.
# *Cause:
#   - A DROP TABLE statement is not allowed in recovery support replication.
# *Action:
#   - Check the replication options.

287,rpERR_ABORT_RPC_NOT_EXIST_REPL_HOST = Replication hosts not found.
# *Cause:
#   - No replication hosts were found.
# *Action:
#   - Check the meta tables.

288,rpERR_ABORT_RPC_NOT_SUPPORT_AT_SN_CLAUSE = Replication cannot start from a specific SN unless it is used with the Log Analyzer.
# *Cause:
#   - The AT SN clause was specified, but the role of the replication object is not "Log Analyzer".
# *Action:
#   - Either specify the FOR ANALYSIS clause in the CREATE REPLICATION statement, or do not use the AT SN clause in the ALTER REPLICATION statement.

289,rpERR_ABORT_RPC_NOT_SUPPORT_RETRY_AND_EAGER = The retry option is not supported when replication is running in eager mode.
# *Cause:
#   - The retry option is not supported in eager mode.
# *Action:
#   - Either remove the retry option or change the replication mode.

290,rpERR_ABORT_RPC_ROLE_NOT_SUPPORT_SYNC = Replication SYNC is not supported in this role.
# *Cause:
#   - This role does not support SYNC.
# *Action:
#   - Check the role of the replication.

291,rpERR_ABORT_RPC_ALREADY_OFFLINE_SET = Offline option already set.
# *Cause:
#   - The replication offline option was already set.
# *Action:
#   - Check the replication options.

292,rpERR_ABORT_RPC_ALREADY_OFFLINE_UNSET = Offline option already unset.
# *Cause:
#   - The replication offline option was already unset.
# *Action:
#   - Check the replication options.

293,rpERR_FATAL_RPD_REPL_META_CRASH = A replication meta table crashed.
# *Cause:
#   - A replication meta table crashed.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

294,rpERR_ABORT_RPD_REPL_NOT_FOUND = Replication not found
# *Cause:
#   - The specified replication has not been created yet.
# *Action:
#   - Create the replication first.
#   - Verify that the replication name is valid.

295,rpERR_ABORT_RPD_INTERNAL_ARG = Internal server error in meta module (<0%s>).
# *Cause:
#   - Failed to execute DML on replication metadata due to a database error.
# *Action
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

296,rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OLD_ITEMS = Internal server error. There are too many old item metas for replication. (<0%s>)
# *Cause:
#   - A larger number of rows than those existing were returned from SYS_REPL_OLD_ITEMS_. This is an internal server error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

297,rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OLD_ITEMS = Internal server error. There are too few old item metas for replication. (<0%s>)
# *Cause:
#   - A smaller number of rows than those existing were returned from SYS_REPL_OLD_ITEMS_. This is an internal server error.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

298,rpERR_ABORT_RPD_NOT_EXIST_REPL_OFFLINE_DIR_PATH = Replication offline log dir not found.
# *Cause:
#   - No replication offline log directory path was found.
# *Action:
#   - Check the meta tables.

299,rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OFFLINE_DIR_PATH = Internal server error. Too many offline log dir paths for replication.
# *Cause:
#   - Too many directories were specified for offline replication.
# *Action:
#   - Specify only one directory.

300,rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OFFLINE_DIR_PATH = Internal server error. Too few offline log dir paths for replication.
# *Cause:
#   - A directory has not been specified for offline replication.
# *Action:
#   - Specify a directory.

301,rpERR_ABORT_RPD_MAX_REPLICATION = Internal server error.
# *Cause:
#   - This is an internal error related to metadata of the replication object.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

302,rpERR_ABORT_RPD_TOO_MANY_REPLICATION_HOSTS = Internal server error. There are too many hosts for replication.
# *Cause:
#   - This is an internal error related to metadata of replication hosts.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

303,rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_HOSTS = Internal server error. There are too few hosts for replication.
# *Cause:
#   - This is an internal error related to metadata of replication hosts.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

304,rpERR_ABORT_RPD_TOO_MANY_REPLICATION_ITEMS = Internal server error. There are too many tables for replication.
# *Cause:
#   - This is an internal error related to SYS_REPL_ITEMS_.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

305,rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_ITEMS = Internal server error. There are too few tables for replication.
# *Cause:
#   - This is an internal error related to SYS_REPL_ITEMS_.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

306,rpERR_ABORT_RPD_MAXIMUM_OBJECT_NUMBER_EXCEED = The number of elements <0%s> exceeds the maximum limit (<1%ld>).
# *Cause:
#   - The specified number of elements exceeds the maximum limit.
# *Action:
#   - Reduce the specified number of elements.

307,rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OLD_COLUMNS = Internal server error. There are too many old column metas for replication. (<0%s>, <1%lu>)
# *Cause:
#   - This is an internal error related to SYS_REPL_ITEMS_.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

308,rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OLD_COLUMNS = Internal server error. There are too few old column metas for replication. (<0%s>, <1%lu>)
# *Cause:
#   - This is an internal error related to SYS_REPL_ITEMS_.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

309,rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OLD_INDICES = Internal server error. There are too many old index metas for replication. (<0%s>, <1%lu>)
# *Cause:
#   - This is an internal error related to SYS_REPL_ITEMS_.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

310,rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OLD_INDICES = Internal server error. There are too few old index metas for replication. (<0%s>, <1%lu>)
# *Cause:
#   - This is an internal error related to SYS_REPL_ITEMS_.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

311,rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OLD_INDEX_COLS = Internal server error. There are too many old index column metas for replication. (<0%s>, <1%lu>, <2%u>)
# *Cause:
#   - This is an internal error related to SYS_REPL_ITEMS_.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

312,rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OLD_INDEX_COLS = Internal server error. There are too few old index column metas for replication. (<0%s>, <1%lu>, <2%u>)
# *Cause:
#   - This is an internal error related to SYS_REPL_ITEMS_.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

313,rpERR_ABORT_RPD_ALREADY_SUPPORT_RECOVERY = The table recovery is already supported by another replication <0%s>
# *Cause:
#   - The recovery of this table is already supported by an existing replication.
# *Action:
#   - Check the replication item.

314,rpERR_ABORT_NOT_EXIST_SYNC_TABLE = A table to be synchronized does not exist.
# *Cause:
#   - The table is not a replication target object.
# *Action:
#   - Check whether the table is a replication target object.

315,rpERR_ABORT_SEND_SYNC_TABLES = Failed to send information about a table to synchronize.
# *Cause:
#   - A network error has occurred.
# *Action:
#   - Check the network status and ensure that all systems are connected to the network.

316,rpERR_ABORT_INVALID_SYNC_TABLE_NUMBER = The number of tables to be synchronized is invalid.
# *Cause:
#   - The number of tables to be synchronized is invalid.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

317,rpERR_ABORT_REBUILD_INDEX = Failed to rebuild indexes after synchronization operation.
# *Cause:
#   - The table may be locked, or meta information may be corrupted.
# *Action:
#   - Wait for the lock to be released and then try again.

318,rpERR_ABORT_RP_SYNC = Synchronization failed. Indexes on the remote server may have been inconsistent.
# *Cause:
#   - Check the log files on the remote server to determine the cause of failure.
# *Action:
#   - Identify and rectify the cause of synchronization failure. 
#   - Additionally, check whether indexes on the remote server have been inconsistent, and rebuild the indexes if necessary.

319,rpERR_ABORT_SEND_SYNC_START = Failed to send signal starting replication synchronization.
# *Cause:
#   - A network error has most likely occurred.
# *Action:
#   - Check the network status and ensure that all systems are connected to the network.

320,rpERR_ABORT_LOCK_TABLE = Failed to lock table for replication synchronization.
# *Cause:
#   - A lock conflict occurred.
# *Action:
#   - Wait for the lock to be released or manually release the lock.

321,rpERR_ABORT_SEND_SYNC_TABLES_NUMBER = Failed to send information about the number of tables to synchronize.
# *Cause:
#   - A network error has most likely occurred.
# *Action:
#   - Check the network status and ensure that all systems are connected to the network.

322,rpERR_ABORT_ALLOC_CM_BLOCK = [Network] Allocate communication block failed
# *Cause:
#   - Failed to allocate network resources.
# *Action:
#   - Check the state of system resources. If they are insufficient, take appropriate action.

323,rpERR_ABORT_FREE_CM_BLOCK = [Network] Free communication block failed
# *Cause:
#   - Failed to free network memory resources.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

324,rpERR_ABORT_LOB_TRIM = [SmExec] A failure occurred while trimming LOB type data
# *Cause:
#   - Failed to trim LOB data.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

325,rpERR_ABORT_RPC_CANNOT_REPLICATE_TABLE_WITH_COMPRESSED_COLUMN = Replication is not allowed on tables that have compressed column.
# *Cause:
#   - A table with a compressed column cannot be replicated.
# *Action:
#   - Verify that the table to be replicated does not have any compressed columns.

326,rpERR_ABORT_INVALID_PACKET_TYPE = Invalid communication packet type
# *Cause:
#   - An invalid communication packet type has been detected.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

327,rpERR_ABORT_CHECK_NAME_NOT_FOUND = Check constraint name of the replicated table does not exist. [<0%s>(Name: <1%s>)]
# *Cause:
#   - The check constraint name of the replicated table does not match.
# *Action:
#   - Check the check constraint name.

328,rpERR_ABORT_CHECK_CONDITION_MISMATCH = Check constraint condition string of the replicated table mismatch. [<0%s>(Condition String: <1%s>):<2%s>(Condition String: <3%s>)]
# *Cause:
#   - The check constraint condition string for both tables does not match.
# *Action:
#   - Check the condition string in check constraint.

329,rpERR_ABORT_CHECK_COLUMN_NOT_REPL = Check constraint is composed of replication column and non-replication column. [<0%s>(<1%s>)]
# *Cause:
#   - The check constraint column composition does not match.
# *Action:
#   - Check the column in check constraint.

330,rpERR_ABORT_FUNC_BASED_INDEX_EXPRESSION_MISMATCH = Function-based index expression string of the replicated table mismatch. [<0%s>(Name:<1%s>, Expression String:<2%s>:<3%s>(Name:<4%s>, Expression String:<5%s>)]
# *Cause:
#   - The function-based index expression string is not the same as the corresponding table.
# *Action:
#   - Check the expression string in the function-based index.

331,rpERR_ABORT_FUNC_BASED_INDEX_COUNT_MISMATCH = Function-based index count of the replicated table does not equal. [<0%s>:<1%s>]
# *Cause :
#   - The function-based index count of the replicated table does not match.
# *Action :
#   - Check the function-based index.

332,rpERR_ABORT_COLUMN_HIDDEN_MISMATCH = Hidden columns mismatch.[<0%s>.<1%s>(<2%u>):<3%s>.<4%s>(<5%u>)]
# *Cause: 
#   - The hidden column names and the regular column names are identical for the replication target tables.
# *Action: 
#   - Check whether the hidden column names and the regular column names are identical.

333,rpERR_ABORT_FUNC_BASED_INDEX_COLUMN_NOT_REPL = Function-based index is composed of replication column and non-replication column [<%0s>(<%1s>)]
# *Cause:
#   - The function based index is composed of a replicated column and a non-replicated column.
# *Action:
#   - Check the composition of the function based index.

334,rpERR_ABORT_COMPATIBILITY_BETWEEN_TABLE_AND_PARTITION = The replication units mismatch: <0%s>
# *Cause:
#   - The replication unit (table or partition) to be dropped or synced is not the same as the existing replication unit.
# *Action:
#   - Check whether the specified replication unit exists in the partition_unit column of the SYS_REPL_IMTES_ meta table.

335,rpERR_ABORT_RPC_NOT_EXISTS_PARTITION = Partition not found : <0%s>
# *Cause :
#   - The name of the specified partition was not found in the specified table.
# *Action:
#   - Verify that the partition exists in the database.
#   - Verify that there are no typos in the partition name.

336,rpERR_ABORT_RPC_EXISTS_OFFLINE_OPTION_COMPRESSED_COLUMN = Offline replication cannot be performed on a compressed table. [<0%s>.<1%s>]
# *Cause :
#   - Offline replication cannot be performed on a compressed table.
# *Action:
#   - Check whether the specified table is compressed or not.

337,rpERR_ABORT_RPC_NOT_SUPPORT_OFFLINE_REPLICATE_TABLE_WITH_COMPRESSED_COLUMN = Compressed tables do not support offline replication.
# *Cause :
#   - Compressed tables do not support offline replication.
# *Action:
#   - Check whether the specified table is compressed or not.

338,rpERR_ABORT_RP_SYNC_WITH_INDEX_CAUTION = Replication synchronization failed. Check whether the index on the remote server is consistent.
# *Cause:
#   - Check logs on the local and remote servers for the cause of failure.
# *Action:
#   - Check whether the index on the remote server is inconsistent and rebuild it, if necessary.

339,rpERR_ABORT_RP_SYNC_WITHOUT_INDEX_CAUTION = Replication synchronization failed.
# *Cause:
#   - Check logs on the local and remote servers for the cause of failure.
# *Action:
#   - Check the cause for synchronization failure and fix it.

340,rpERR_ABORT_INSERT_GLOBAL_INDEX_TABLE = Failed to insert the record to the global index table.
# *Cause: 
#   - Inserting data into a partition and a global index failed while duplicating a partition with a global index.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

341,rpERR_ABORT_UPDATE_GLOBAL_INDEX_TABLE = Failed to update the record to the global index table.
# *Cause:
#   - Updating data on a partition and a global index failed while duplicating a partition with a global index.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

342,rpERR_ABORT_DELETE_GLOBAL_INDEX_TABLE = Failed to delete the record from the global index table.
# *Cause:
#   - Deleting data from a partition and a global index failed while duplicating a partition with a global index.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

343,rpERR_ABORT_RPD_OVERFLOW_COUNT_REPL_OFFLINE_DIR_PATH = Exceeds the maximum number of log directory paths allowed to be specified. 
# *Cause:
#   - Only one log directory path can be specified with the replication offline option.
# *Action:
#   - Check the log directory path of the Active server.

344,rpERR_ABORT_DATA_STILL_REMAIN_IN_BUFFER = If receive buffer has been processed, no data should remain in the receive buffer.
# *Cause:
#   - Already processed data remains in the replication receive buffer.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

345,rpERR_ABORT_REPLICATION_MODE_MISMATCH = The replication mode does not match. [<0%u>:<1%u>]
# *Cause:
#   - The mode of the replicated table mismatch.
# *Action:
#   - Please check the mode of the replicated table.

346,rpERR_ABORT_PARTITION_CONDITION_MISMATCH = The partition condition of the replicated table does not match. [<0%s>:<1%s>]
# *Cause:
#   - The partition condition of the replicated table mismatch.
# *Action:
#   - Please check the partition condition of the replicated table.

347,rpERR_ABORT_NOT_SUPPORT_FEATURE_WITH_V6_PROTOCOL = This feature is not supported with __REPLICATION_USE_V6_PROTOCOL property. (<0%s>)
# *Cause:
#   - When using A5 protocol, this feature is not surpported.
# *Action:
#   - Check the __REPLICATION_USE_V6_PROTOCOL property or table schemes.

348,rpERR_ABORT_RPC_NOT_SUPPORT_REPL_GAPLESS_AND_EAGER = The gap control option is not supported with eager mode.
# *Cause:
#   - Simultaneous use of eager mode and the gap control option is not supported.
# *Action:
#   - Change the option or mode of the replication.

349,rpERR_ABORT_RPC_ALREADY_GAPLESS_SET = The gapless option already set.
# *Cause:
#   - The replication gapless option has already been set.
# *Action:
#   - Check the replication options.

350,rpERR_ABORT_RPC_ALREADY_GAPLESS_UNSET = The gapless option was already unset.
# *Cause:
#   - The replication gapless option was already unset.
# *Action:
#   - Check the replication options.

351,rpERR_ABORT_RPC_NOT_SUPPORT_REPL_PARALLEL_OPTION_AND_EAGER = The parallel option is not supported with eager mode.
# *Cause:
#   - Simultaneous use of eager mode and the parallel option is not supported.
# *Action:
#   - Change the option or mode of the replication.

352,rpERR_ABORT_RPC_NOT_PROPER_VALUE_FOR_PARALLEL_COUNT = Invalid the parallel count  value. Valid value range = 0 ~ 512
# *Cause:
#   - The value of parallel count is invalid.
# *Action:
#   - Spcify the value of parallel count whthin valid range.

353,rpERR_ABORT_RPC_ALREADY_PARALLEL_SET = The parallel option already set.
# *Cause:
#   - The replication parallel option has already been set.
# *Action:
#   - Check the replication options.

354,rpERR_ABORT_RPC_ALREADY_PARALLEL_UNSET = The parallel option was already unset.
# *Cause:
#   - The replication parallel option was already unset.
# *Action:
#   - Check the replication options.

355,rpERR_ABORT_RPC_ALREADY_GROUPING_SET = The grouping option already set.
# *Cause:
#   - The grouping option has already been set.
# *Action:
#   - Check the replication options.

356,rpERR_ABORT_RPC_ALREADY_GROUPING_UNSET = The grouping option was already unset.
# *Cause:
#   - The grouping option was already unset.
# *Action:
#   - Check the replication options.

357,rpERR_ABORT_RPC_NOT_SUPPORT_REPL_GROUPING_AND_EAGER = The grouping option is not supported with eager mode.
# *Cause:
#   - Simultaneous use of eager mode and the grouping option is not supported.
# *Action:
#   - Change the option or mode of the replication.

358,rpERR_ABORT_AHEAD_ANALYZER_TIMED_WAIT_ERROR = Internal server error. ( system call ( timedwait ) got error )
# *Cause:
#   - cond_timedwait() returned false while the Ahead Analyzer for grouping transaction was waiting after finishing the analysis in the replication with the grouping option enabled.
# *Action:
#   - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

359,rpERR_FATAL_ThrLatchInit = Unable to initialize a latch.
# *Cause: The system failed to initialize a latch.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

360,rpERR_ABORT_TRANSACTION_NOT_ACTIVE = Transaction is not active.
# *Cause: A transaction is not active.
# *Action:  Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com). 

361,rpERR_ABORT_SOCKET_OPEN_ERROR = Socket Can not be openned. 
# *Cause:
#   - An error occurred while opening the socket.
# *Action:
#   - Please check system resources

362,rpERR_ABORT_SOCKET_SET_BLOCK_MODE = Socket can not be set as block mode.
# *Cause:
#   - An error occurred while setting a block mode to socket.
# *Action:
#   - Please check network status.

363,rpERR_ABORT_SOCKET_GET_OPTION = Socket can not be gotten option value.
# *Cause:
#   - An error occurred while getting socket option.
# *Action:
#   - Please check system resource (file descriptor) or network resources.

364,rpERR_ABORT_POLL_REMOVE_SOCK = Socket can not be removed from the poll.
# *Cause:
#   - An error occurred while removing socket from the poll.
# *Action:
#   - Please check system (file descriptor) or network resources.

365,rpERR_ABORT_POLL_DISPATCH_TIMEOUT = There is no network event in time.
# *Cause:
#   - There is no network I/O in time.
# *Action:
#   - Please check network interface controller.

366,rpERR_ABORT_POLL_DISPATCH_ERROR = Fail to invoke the dispatcher system function.
# *Cause:
#   - This is a system call error.
# *Action:
#   - Please check system resources (file descriptor or memory).

367,rpERR_ABORT_POLL_CREATE_ERROR = Dispatcher can not be created.
# *Cause:
#   - An error occurred while creating dispatcher was being created.
# *Action:
#   - Please check system resources (file descriptor or memory).

368,rpERR_ABORT_SOCK_GET_ADDR_INFO_ERROR = Fail to invoke getAddrInfo system function. (IP = <0%s>, PORT = <1%d>)
# *Cause:
#   - IP address and port number are wrong, or There is not enough memory.
# *Action:
#   - Please check IP address and port Number, or system resources(memory). 

369,rpERR_ABORT_REPLICATION_HAS_RECOVERY_OPTION_IN_SPLIT_PARTITION = Replication option is set to recovery.(Replcation Name:<0%s>, User:<1%s>, Table:<2%s>, Partition:<3%s>)
# *Cause:
#   - The partition split cannot be performed in recovery option.
# *Action:
#   - Check the replication options

370,rpERR_ABORT_REPLICATION_HAS_RECOVERY_OPTION_IN_MERGE_PARTITION = Replication option is set to recovery.(Replcation Name:<0%s>, User:<1%s>, Table:<2%s>, Partition:<3%s>, Partition:<4%s>)
# *Cause:
#   - The partition merge cannot be performed in recovery option.
# *Action:
#   - Check the replication options

371,rpERR_ABORT_TABLE_PARTITIONS_ARE_NOT_SAME_REPLICATION = Replicated partitions are not included same replication.(User:<0%s>, Table:<1%s>, Partition:<2%s>, Partition:<3%s>)
# *Cause:
#   - The partition merge cannot be performed in other replicated partition.
# *Action:
#   - Check the partition name of the replicated table.

372,rpERR_ABORT_PARTITION_NAME_DIFF = The partition name of local table must be same with remote one.( Replication name: <0%s> Local (User:<1%s>, Table:<2%s>, Partition:<3%s>), Remote (User:<4%s>, Table:<5%s>, Partition:<6%s>) )
# *Cause:
#   - The partition name of local table is not same with remote one.
# *Action:
#   - Modify the partition name of local table.

373,rpERR_ABORT_TABLE_NAME_DIFF = The name of local table must be same with remote one.(Replication name: <0%s> Local (User:<1%s>, Table:<2%s>, Partition:<3%s>), Remote (User:<4%s>, Table:<5%s>, Partition:<6%s>) )
# *Cause:
#   - The name of local table must be same with remote one.
# *Action:
#   - Verify that the name of local table is same with remote table.

374,rpERR_ABORT_REPLICATION_HAS_RECOVERY_OPTION_IN_DROP_PARTITION = Replication option is set to recovery.(Replcation Name:<0%s>, User:<1%s>, Table:<2%s>, Partition:<3%s>)
# *Cause:
#   - Cannot perform a DDL(DROP PARTITION) in recovery option.
# *Action:
#   - Remove the recovery option. 

375,rpERR_ABORT_POLL_ADD_SOCK = A Socket cannot be added to the poll.
# *Cause:
#   - An error occurred while adding a socket to the poll.
# *Action:
#   - Check the system or network resources; in particular, verify the file descriptor.

376,rpERR_ABORT_RPC_NOT_PROPER_VALUE = Invalid <0%s> value : <1%s>. Valid value range = <2%lu> ~ <3%lu> <4%s>
# *Cause:
#   - The value is out of range.
# *Action:
#   - Spcify the value whthin valid range.

#377,rpERR_ABORT_REPLICATION_MODE_MISMATCH = The replication mode does not match. [<0%u>:<1%u>]
# *Cause:
#   - The mode of the replicated table mismatch.
# *Action:
#   - Please check the mode of the replicated table.

#378,rpERR_ABORT_PARTITION_CONDITION_MISMATCH = The partition condition of the replicated table does not match. [<0%s>:<1%s>]
# *Cause:
#   - The partition condition of the replicated table mismatch.
# *Action:
#   - Please check the partition condition of the replicated table.

379,rpERR_ABORT_RPC_NOT_SUPPORT_REPL_LOCAL_AND_EAGER = It is unable to use eager mode when local replication option used.
# *Cause:
#   - Eager mode does not support local replication option.
# *Action:
#   - Change replication mode or remove LOCAL option.

380,rpERR_ABORT_SELF_REPLICATION_TABLE = Self-replication has been detected due to duplicate table name. [<0%s>.<1%s>.<2%s>:<3%s>.<4%s>.<5%s>]
# *Cause:
#   - The same name is used in the source and target tables.
# *Action:
#   - Set a different name for the source and target tables.

381,rpERR_ABORT_SELF_REPLICATION_NAME = Self-replication has been detected due to duplicate replication name. [<0%s>]
# *Cause:
#   - Same replication name used for itself and peer replicaiton.
# *Action:
#   - Use different peer replication name from its replication name.

382,rpERR_ABORT_RP_NOT_SUPPORT_CONVERT_APPLY_DATA_TYPE = The table(<0%s>.<1%s>:<2%s>.<3%s>) containing <4%s> does not support sql apply feature.
# *Cause:
#   - The table containing specific data type does not support sql apply feature.
# *Action:
#   - Please drop specific data type from the table.

383,rpERR_ABORT_REPLICATION_DDL_ENABLE_LEVEL = Cannot execute this DDL on a replicated table when the system property REPLICATION_DDL_ENABLE_LEVEL is <0%u>.
# *Cause: 
#   - System property REPLICATION_DDL_ENABLE_LEVEL is not appropriate for this DDL. 
# *Action: 
#   - Check the system property REPLICATION_DDL_ENABLE_LEVEL.

384,rpERR_ABORT_RPC_DDL_NOT_SUPPORTED_REPLICATED_TABLE = This DDL is not supported on replicated table.(Cause : <0%s>)
# *Cause: 
#   - This DDL is not supported on replicated table.
# *Action: 
#   - Identify the cause and take appropriate action.

385, rpERR_ABORT_RP_ALREADY_DDL_SYNC = Can not perform a DDL synchronization because it is currently in progress (Replication name : <0%s>).
# *Cause:
#   - Can not perform a DDL synchronization because it is currently in progress. 
# *Action:
#   - Retry after finishing the DDL synchronization.

386, rpERR_ABORT_RP_DDL_SYNC_NOT_SUPPORT_EAGER = Can not execute a DDL synchronization on a table using eager mode replication. 
# *Cause:
#   - Can not execute a DDL synchronization for a table using eager mode replication.
# *Action:
#   - Change the replication mode in eager mode replication or remove the table in eager mode replication.

387, rpERR_ABORT_RP_DDL_SYNC_NOT_SUPPORT_GLOBAL_NON_PARTITION_INDEX = A DDL synchronization is not support on a table with global non-partitioned index.
# *Cause:
#   - A DDL synchronization is not support on a table with global non-partitioned index. 
# *Action:
#   - Verify that the table with a global non-paritited index.

388, rpERR_ABORT_RP_DDL_SYNC_DISABLE = Can not perform the DDL synchronization when the value of the REPLICATION_DDL_SYNC property is 0.
# *Cause:
#   - Can not perform the DDL synchronization when the value of the REPLICATION_DDL_SYNC property is 0.
# *Action:
#   - Verify and change the value of the REPLICATION_DDL_SYNC property is 1.

389, rpERR_ABORT_RP_DDL_SYNC_ERROR_AFTER_COMMIT_MSG = A DDL synchronization is failed after sending commit message. The table schema of the remote server may have been inconsistent.
# *Cause:
#   - A DDL synchronization is failed after sending commit message.
# *Action:
#   - Verify that the replication actually commits 

390, rpERR_ABORT_RP_DDL_SYNC_RECEIVE_ERROR = A DDL synchronization receives the error message from the remote server (<0%s>).
# *Cause:
#   - Receive the error message from the remote server.
# *Action:
#   - Perform a DDL synchronization after troubleshooting the remote server.

391, rpERR_ABORT_RP_SENDER_NOT_RUNNING = The sender of replication <0%s> is not running.
# *Cause:
#   - The sender of replication is not running. 
# *Action:
#   - Restart replication or wait for the replication sender to run.

392, rpERR_ABORT_RP_DDL_SYNC_SQL_LENGTH_ERROR = The length of the SQL query exceeds the maximum limit (<0%u>).
# *Cause:
#   - The length of the SQL query exceeds the maximum limit. 
# *Action:
#   - Refer to the Replication Manaual for the maximum possible length allowed for the DDL synchronization query.

393,rpERR_ABORT_PARENT_AND_CHILD_CONNECT_DIFFERENT = Failed to connected by the parallel senders. The parallel senders get different IP address from DNS server.
# *Cause: 
#   - The parallel senders must be connected by the same IP.
# *Action: 
#   - Verify the DNS server or restart the replication sender. (When you restart the replication, the parallel senders have been connected by the same IP.)

394,rpERR_ABORT_RPC_ROLE_NOT_SUPPORT_IB = InfiniBand(IB) can not be used with 'FOR ANALYSIS' syntax. 
# *Cause:
#   - InfiniBand(IB) can not be used with 'FOR ANALYSIS' syntax. 
# *Action:
#   - Check the role of the replication. Do not use the InfiniBand(IB).

395,rpERR_ABORT_RPC_NOT_DEFAULT_SRID = Only the default SRID can be replicated.
# *Cause: 
#   - The SRID of the geometry column is not the default.
# *Action: 
#   - Check the SRID of the geometry column.

396,rpERR_ABORT_RP_DDL_SYNC_WITH_PROPAGATION = The propagation option(FOR PROPAGABLE LOGGING/ FOR PROPAGATION) does not support DDL synchronization.
# *Cause:
#   - The propagation option(FOR PROPAGABLE LOGGING/ FOR PROPAGATION) does not support DDL synchronization.
# *Action:
#   - Remove the propagation option.

397,rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR = The HeartBeat thread(HBT) detects peer server or network error.
# *Cause:
#   - 1. Peer server is not running.
#   - 2. Network is not working.
# *Action:
#   - 1. Confirm peer server is running.
#   - 2. Ensure that network is working normally.

398,rpERR_ABORT_SEND_TIMEOUT_EXCEED = Timeout exceed while the replication Sender and Receiver were communicating. 
# *Cause : 
#   - A timeout occurred while the replication Sender and Receiver were communicating.
# *Action : 
#   - Set a larger value for the REPLICATION_SENDER_SEND_TIMEOUT property. 
#   - Check the peer server or network connection.

399,rpERR_ABORT_FAULT_TOLERATED = Failed to work because an internal exception occurred from an OS.[Contact Altibase's Support Center]
# *Cause: An internal exception occurred from an OS.
# *Action: Check the error number from the trace log for more detailed information. And contact Altibase's Support Center (http://support.altibase.com).

400,rpERR_ABORT_PARTITION_COUNT_MISMATCH = The partition count of the replicated table does not match [<0%s>(<1%d>):<2%s>(<3%d>)].
# *Cause:
#   - Replicated table partition count mismatch.
# *Action: 
#   - Check the partition count of the replicated table.

401,rpERR_ABORT_SYNC_FAILED_BY_BULK_INSERT = Replication synchronization failed by replication bulk insert.
# *Cause: 
#   - Bulk insert error has been occurred while executing the received record.
# *Action: 
#   - Check the altibase_rp.log file.

402, rpERR_ABORT_SRID_MISMATCH = The geometry SRID of the replicated table's column does not match. [<0%s>(Name:<1%s>, SRID:<2%d>):<3%s>(Name:<4%s>, SRID: <5%d>)].
# *Cause: 
#   - Replicated table's column geometry SRID mismatch.
# *Action: 
#   - Check the geometry SRID of the replicated table's column.

403, rpERR_ABORT_NOT_SUPPORT_SRID_REMOTE = The geometry SRID of <0%s> table's column is not supported on the remote server;
# *Cause: 
#   - The remote server is a version that does not support geometry SRID.
# *Action: 
#   - Check the error number from the trace log for more detailed information. And contact Altibase's Support Center (http://support.altibase.com).

404,rpERR_ABORT_RPC_NOT_SUPPORT_REPL_CONDITIONAL_START_AND_EAGER = SYNC CONDITIONAL and START CONDITIONAL clauses are not supported with replication eager mode. 
# *Cause:
#   - It's unable to execute the statement, becuase SYNC CONDITIONAL and START CONDITIONAL clauses are not supported with replication eager mode.
# *Action:
#   - Check the replication mode.

405, rpERR_ABORT_DDL_SYNC_WITH_LOCK_UNTIL_NEXT_DDL = You cannot execute DDL for the replicated table after performing LOCK.. UNTIL NEXT DDL on REPLICATION_DDL_SYNC enable mode.
# *Cause: 
#   - You cannot use the property REPLICATION_DDL_SYNC enable and LOCK.. UNTIL NEX DDL statement on the replicated table.
# *Action: 
#   - hange the property REPLICATION_DDL_SYNC to 0(disable) or close a transaction.

406, rpERR_ABORT_RPD_TOO_LONG_MUTEX_NAME = A mutex name is too long. (<0%s>)
# *Cause:
# - The Internal mutex name is over 64 characters.
# *Action:
# - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

407, rpERR_ABORT_RPD_TOO_LONG_FILE_PATH = A xlogfile full-path is too long. (<0%s>)
# *Cause:
# - A xlogfile full-path is over 1024 characters.
# *Action:
# - Modify XLOGFILE_DIR property to short full-path.

408, rpERR_ABORT_RPD_FAILURE_UNLINK_FILE = Failed to delete xlogfile. [path=(<0%s>), errcode=<1%d>]
# *Cause:
# - The system failed to remove a file, because the file is busy.
# *Action:
# - Terminate unnecessary processes. 

409, rpERR_ABORT_RPD_CREATER_THREAD_NOT_ALIVE = <0%s> xlogfile creater thread has beed terminated.
# *Cause:
# - The xlogfile creater thread has been terminated.
# *Action:
# - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

410, rpERR_ABORT_RPD_FLUSHER_THREAD_NOT_ALIVE = <0%s> xlogfile flusher thread has been terminated.
# *Cause:
# - The xlogfile flusher thread has been terminated.
# *Action:
# - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

411, rpERR_ABORT_RPN_UNABLE_READ_CONTEXT = Invalid read context.
# *Cause:
# - Internal error.
# *Action:
# - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

412, rpERR_ABORT_RPX_FAIL_WRITE_XLOG_IN_XLOGFILE = Failed to write xlog into xlogfile.
# *Cause:
# - Internal error.
# *Action:
# - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

413, rpERR_ABORT_RPC_CONSISTENT_MODE_MUST_HAVE_PARALLEL = syntax error : PARALLEL APPLIER Option is missing on CREATE CONSISTENT REPLICATION statement.
# *Cause:
# - Consistent replication should be made with parallel applier option.
# *Action:
# - Check the syntax of CREATE CONSISTENT REPLICATION statement.

414, rpERR_ABORT_RPC_NOT_COMPATIBLE_ROLE_OPTION_IN_CONSISTENT_MODE = Unable to use the role option in consistent replication.
# *Cause:
# - Cannot create consistent replication with role option, except for parallel applier option.
# *Action:
# - Check the constraint for consistent replication.

415, rpERR_ABORT_RPC_CONSISTENT_DO_NOT_HAVE_ANY_OTHER_OPTIONS = Unable to use the option in consistent replication.
# *Cause:
# - Unable to use any option in consistent replication, except for parallel applier option.
# *Action:
# - Check the constraint for consistent replication.

416, rpERR_ABORT_RPC_FLUSH_XLOGFILE_STATEMENT_CAN_ONLY_RUN_IN_CONSISTENT = ALTER REPLICATION ... FLUSH FROM XLOGFILE statement can be executed only in a consistent mode replication.
# *Cause:
# - This statement can be executed only in a consistent mode replication.
# *Action:
# - Check if the replication mode is consistent or not.

417, rpERR_ABORT_RPC_FAILED_FLUSH_XLOGFILE = Failed to flush xlog files.
# *Cause:
# - Internal error.
# *Action:
# - Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

418, rpERR_ABORT_RPC_CANNOT_FIND_REMOTE_META = Cannot find the replication remote metadata.
# *Cause:
# - Xlogfile recovery receiver must have a replication remote metadata.
#  *Action:
# - Check if the replication has started after Altibase server startup.
# - A replication remote metadata is generated when the replication starts after Altibase server startup.
 
419, rpERR_ABORT_RPC_RECEIVER_IS_WORKING_ON_IT = Xlogfile recovery cannot run while another receiver is working on synchronization or recovery.
# *Cause:
# - Another receiver with same replication name is processing on synchronization or recovery.
#  *Action:
# - Check whether another receiver with same replication name is processing on synchronization or recovery.

420, rpERR_ABORT_RP_CANNOT_FIND_XLOGFILE = Unable to find a xlogfile related with the replication name. (<0%s>)
# *Cause:
# - If the replication has been started once in consistent mode, the xlogfile should exist as "xlogfile_repName_fileno". But the file cannot be found.
#  *Action:
# - Check the replication's name and verify that the xlogfile exists in XLOGFILE_DIR.

421, rpERR_ABORT_RPC_NOT_SUPPORT_FAILBACK_THIS_MODE = Failback is not supported in this replication mode.
# *Cause:
#   - Failback is supported only in the consistent mode.
# *Action:
#   - Verify whether the replication mode is the consistent mode.

422, rpERR_ABORT_RPC_NOT_SUPPORT_RETRY_AND_CONSISTENT = The retry option is not supported when a replication is running in the consistent mode.
# *Cause:
#   - The retry option is not supported in the consistent mode.
# *Action:
#   - Either remove the retry option or change the replication mode.

423, rpERR_IGNORE_RPX_END_OF_XLOGFILES = Reach end of xlogfiles
# *Cause:
#  - Reached end of xlogfiles when consistent receiver is in the read only mode. (not apply mode) 
# *Action:
#  - You can safely ignore this message.

424, rpERR_REBUILD_RPD_MISS_MATCH_LOCK_TABLE_AND_META = The replication meta information has been changed.
# *Cause:
# - The replication meta information has been changed.
# *Action:
# - You may safely ignore this message.

425, rpERR_ABORT_RP_RECEIVER_INITIALIZE_FAIL = The receiver(%s) has not started, because the replication meta information has been changed.
# *Cause:
# - The receiver has not started, because the replication meta has been changed.
# *Action:
#  - Restart the replication sender.

426, rpERR_ABORT_RP_XLOG_FILE_FAILBACK_MASTER_SENDER_INITIALIZE_FAIL = Failed to perform a failback, because the replication meta information has been changed.
# *Cause:
# - Failed to perform a failback, because the replication meta information has been changed.
# *Action:
# - Retry 'ALTER REPLICATION repl_name FAILBACK';

427, rpERR_ABORT_RPC_FAILOVER_ONLY_SUPPORT_SHARD_SYSTEM = Failed to perform a replication failover, because SHARD_ENABLE property is not 1.
# *Cause:
#   - ALTER REPLICATION ... FAILOVER statement can be executed when SHARD_ENABLE property is 1.
# *Action:
#   - Check if SHARD_ENABLE property is 1.
 
428, rpERR_ABORT_RPC_FAILOVER_ONLY_SUPPORT_CONSISTENT_MODE = Failed to perform a replication failover, because the replication mode is not a consistent mode.
# *Cause:
#   - ALTER REPLICATION ... FAILOVER statement can be executed only in consistent mode.
# *Action:
#   - Check if the replication mode is the consistent mode.
 
429, rpERR_ABORT_OFFLINE_OPTION_IS_NOT_SET = Offline option is not set.
# *Cause:
#   - Replication is created without the offline option.
# *Action: 
#   - Check the options of the replication.
 
430, rpERR_ABORT_OFFLINE_META_ROLE_NOT_SUPPORT = Offline meta is supported only ANALYSIS. 
# *Cause:
#   - This role does not support offline meta.
# *Action: 
#   - Check the role of the replication.
 
431, rpERR_ABORT_RPC_ROLE_NOT_SUPPORT_REPL_META_LOGGING = Meta Logging option is supported only ANALYSIS.
# *Cause:
#   - This role does not support the Meta Logging option.
# *Action:
#   - Check the options or the role of the replication.
 
432, rpERR_ABORT_RP_REMOTE_META_EXIST = BUILD OFFLINE META is already run.
# *Cause:
#   - Remote meta already exists.
# *Action:
#   - run 'ALTER REPLICATION replication_name RESET OFFLINE META;'
 
433, rpERR_ABORT_RP_REMOTE_META_FILE_IS_CORRUPTED = Remote meta file is corrupt. (Meta End SN : <0%ld>, Restart SN : <1%ld>)
# *Cause:
#   - The 'replication_name_META_NEW.bin' file is corrupt. Restart SN is more than Meta End SN of 'replication_name_META_OLD.bin' file.
# *Action:
#   - Run 'ALTER REPLICATION replication_name BUILD OFFLINE META AT SN(sn)' with a value less than Meta End SN.

434, rpERR_ABORT_RP_ERR_REMOTE_META_NOT_BUILD_OFFLINE_META = Remote meta is not from building offline meta.
# *Cause:
#   - Remote meta is not from building offline meta. 
# *Action:
#   - Check the role of the replication.
 
435, rpERR_ABORT_ERR_NO_VALID_METAFILE = There is no valid meta file.
# *Cause: 
#   - There is no meta file. Or it's not valid.
# *Action: 
#   - 'BUILD OFFLINE META' can't proceed anymore. Contact Altibase's Support Center (http://support.altibase.com).
 
436, rpERR_ABORT_ERR_NO_VALID_SNFILE = There is no valid sn file.
# *Cause: 
#   - There is no sn file. Or it's not valid.
# *Action: 
#   - 'BUILD OFFLINE META' can't proceed anymore. Contact Altibase's Support Center (http://support.altibase.com).
 
437, rpERR_ABORT_FAIL_TO_CREATE_DIRECTORY = Failed to create meta directory (Dir Name :<0%s> )
# *Cause:
#   - The specified directory could not be created.
# *Action:
#   - Check the directory path or permission.
 
438, rpERR_ABORT_RP_OFFLINE_ALA_SENDER_PROCESS_DDL = Offline ALA Sender read DDL log.
# *Cause:
#   - Offline ALA Sender read DDL log.
# *Action:
#   - After executing DDL on the target DB, Offline Adapter should be executed again.

INTERNAL_TRACE_MESSAGE_BEGIN

RP_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
            "signed long =<%ld>\n unsigned long = <%lu>\n"
            "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>\n";

## rpcManager
RP_TRC_E_ERR_DUMMY_SOCKET_SETNONBLOCK      = "[Manager] Dummy setNonBlock() error";
RP_TRC_E_ERR_DUMMY_SOCKET_SEND             = "[Manager] Dummy send_nn() error";
RP_TRC_E_ERR_DUMMY_SOCKET_CLOSE            = "[Manager] Dummy closesocket() error";
RP_TRC_E_ERR_RUN_HANDLE_READY              = "[Manager] handle_ready() error in run()";
RP_TRC_E_ERR_RUN_ACCEPT                    = "[Manager] accept() error in run()";
RP_TRC_E_ERR_RUN_SOCKET_CLOSE              = "[Manager] closesocket() error in run()";
RP_TRC_E_ERR_WAKEUP_SOCKET_SETNONBLOCK     = "[Manager] Wakeup setNonBlock() error";
RP_TRC_E_ERR_WAKEUP_SOCKET_CLOSE_NOT_READY = "[Manager] Wakeup closesocket() error"
                                             " when peer server is not ready";
RP_TRC_E_ERR_WAKEUP_SOCKET_CLOSE           = "[Manager] Wakeup closesocket() error";
RP_TRC_E_ERR_GIVEUP_SENDER                 = "[Manager] Replication <%s> Sender Give-up error.\n"
                                             "sender XSN = SN[<%ld>], current = SN[<%ld>], "
                                             "distance from checkpoint = <%ld>, active = <%d>";
RP_TRC_E_ERR_GIVEUP_RECOVERY               = "[Manager] Replication <%s> Recovery Give-up error.\n"
                                             "Min For Recovery XSN = SN[<%ld>], current = SN[<%ld>], "
                                             "distance from checkpoint = <%ld>";
RP_TRC_E_NTC_GIVEUP_RECOVERY               = "[Manager] Replication Recovery Give-up";
RP_TRC_E_NTC_GIVEUP_FAILBACK               = "[Manager] Replication Failback Give-up";
RP_TRC_C_FORCE_PARALLEL_APPLIER            = "[Manager] Receiver Applier [Count : <%d>] is set by force";
PR_TRC_C_SPLIT_PARTITION                   = "[Manager] Replication Name: <%s> Table: <%s>.<%s> Source Partition: <%s> is split Target Partition: <%s> and Target Partition: <%s>";
PR_TRC_C_MERGE_PARTITION                   = "[Manager] Replication Name: <%s> Table: <%s>.<%s> Source Partition: <%s> and Source Partition: <%s> are merged Target Partition: <%s>"; 
PR_TRC_C_DROP_PARTITION                    = "[Manager] Replication Name: <%s> Table: <%s>.<%s> Partition: <%s> is dropped";
RP_TRC_C_FAILOVER_START                    = "[Manager] Replication <%s> Failover start";
RP_TRC_C_FAILOVER_END                      = "[Manager] Replication <%s> Failover End";

## rpcHBT
RP_TRC_HBT_ERR_RUN_COND_TIMEDWAIT     = "[HBT] cond_timedwait() error in run()";
RP_TRC_HBT_ERR_SOCKET_SHUTDOWN        = "[HBT] shutdown error.";
RP_TRC_HBT_ERR_UNREG_SOCKET_CLOSE     = "[HBT] closesocket() error in unregistHost(). But, proceed...";
RP_TRC_HBT_ERR_READY_SOCKET_CLOSE     = "[HBT] closesocket() error in readyDetection(). But, proceed...";
RP_TRC_HBT_ERR_PRECD_SOCKET_CLOSE     = "[HBT] closesocket() error in proceedDetection(). But, proceed...";
RP_TRC_HBT_ERR_PRECD_GET_SOCKOPT      = "[HBT] getsockopt() get error number <%u>";
RP_TRC_HBT_ERR_PRECD_SELECT           = "[HBT] select() error in proceedDetection()";
RP_TRC_HBT_NTC_REMAINED_HOST          = "[HBT] Host threads still remain. But, proceed...";
RP_TRC_HBT_NTC_DETECT_FAULT_PRECD     = "[HBT] == Network Fault Detection Proceeding ==";
RP_TRC_HBT_ERR_PRECD_INTERNAL_CONNECT = "[HBT] Found an error code in kernel using "
                                        "getsockopt() (errno=<%u>)";
#RP_TRC_HBT_NTC_HOST_STATUS           = "[HBT] Host status info.\n"
#                                       "      [<%s>:<%u>] Ref=<%u> Mode=<%u> Handle=<%u> WaterMark=<%u> mFault=<%s>";
RP_TRC_HBT_NTC_HOST_STATUS            = "[HBT] Host status info.\n"
                                        "      [<%s>:<%u>] Ref=<%u> Mode=<%u> Last Error=<%u> mFault=<%s>";
RP_TRC_HBT_NTC_CONNECTED              = "[HBT] Peer Server Connection Succeeds.\n"
                                        "      [<%s>:<%u>] Ref=<%u> Mode=<%u>";
RP_TRC_HBT_ERR_FAULT_DETECTED         = "[HBT] Peer Server Connection Fails.\n"
                                        "      [<%s>:<%u>] Ref=<%u> Mode=<%u> Fail Count=<%u>";
RP_TRC_HBT_ERR_SOCKET_OPEN_FAIL       = "[HBT] Socket Open Fail. This time is skipped(Not Fault)\n"
                                        "      [<%s>:<%u>] Ref=<%u> Mode=<%u> Fail Count=<%u>";



## rpxPJChild
RP_TRC_PJC_ERR_RUN                    = "[PJChild] Error in run()";
RP_TRC_PJC_ERR_CON_SOCKET_SETNONBLOCK = "[PJChild] setNonBlock() error";
RP_TRC_PJC_ERR_DISCON_SOCKET_SHUTDOWN = "[PJChild] shutdown() error";
RP_TRC_PJC_ERR_DISCON_SOCKET_CLOSE    = "[PJChild] closesocket() error";
RP_TRC_PJC_NTC_MSG                    = "[PJChild] <%s>";

## rpxPJMgr
RP_TRC_PJM_ERR_TUPLE_READ_ROW  = "[PJMgr] readRow() error in getTupleNumber()";
RP_TRC_PJM_ERR_ONE_CHILD_START = "[PJMgr] start() of child thread[<%d>] error in run()";
RP_TRC_PJM_ERR_ALL_CHILD_START = "[PJMgr] start() of all child threads error in run()";
RP_TRC_PJM_ERR_CHILD_FAIL      = "[PJMgr] child thread[<%d>] failure in run()";

## rpxSender
RP_TRC_S_ERR_SLP_COND_TIMEDWAIT     = "[Sender] cond_timedwait() error in sleepForNextConnect()";
RP_TRC_S_ERR_NLU_NOT_INIT_META      = "[Sender] Meta is not initialized in getNextLastUsedHostNo()";
RP_TRC_S_NTC_NLU_HOST_NO            = "[Sender] getNextLastUsedHostNo: from <%s>:<%u> to <%s>:<%u>";
RP_TRC_S_SENDER_START               = "[Sender] Replication <%s>:<%u> Start... at [<%ld>]";
RP_TRC_S_SENDER_STOP                = "[Sender] Stop sender thread <%s>:<%u> at [<%ld>], Restart SN[<%ld>]";
RP_TRC_S_SENDER_APPLY_THRJOIN       = "[Sender] SenderApply thread join error. errno [<%d>]";
RP_TRC_S_RECOVERY_SENDER_START      = "[Recovery Sender] Replication <%s>:<%u> Start... at [<%ld>]";
RP_TRC_S_OFFLINE_SENDER_START       = "[Offline Sender] Replication <%s>:<%u> Start... at [<%ld>]";
RP_TRC_S_SET_SWITCH_ARCHIVE_LOG     = "[ArchiveALA] Success to set switch to archive log : mXSN=[<%ld>], file#=[<%u>]";
RP_TRC_S_ERR_SET_SWITCH_ARCHIVE_LOG = "[ArchiveALA] Fail to set switch to archive log : mXSN=[<%ld>], file#=[<%u>]";
RP_TRC_S_INIT_REDO_LOG_MGR          = "[ArchiveALA] Initialize redo log manager";
RP_TRC_S_ERR_INIT_REDO_LOG_MGR      = "[ArchiveALA] Fail to initialize redo log manager";
RP_TRC_S_INIT_ARCHIVE_LOG_MGR       = "[ArchiveALA] Initialize archive log manager";
RP_TRC_S_ERR_INIT_ARCHIVE_LOG_MGR   = "[ArchiveALA] Fail to initialize archive log manager";
RP_TRC_S_INIT_OK_LOG_MGR            = "[ArchiveALA] Initialize OK : LastSN=[<%ld>]";
RP_TRC_S_ERR_EAGER_ABNORMAL_STOP    = "[Sender] Eager Replication <%s> Abnormally Stopped ...";
RP_TRC_S_NTC_FAILBACK_NOMRAL_BEGIN  = "[Sender] Replication <%s> Failback Normal Begin... at [<%ld>]";
RP_TRC_S_NTC_FAILBACK_MASTER_BEGIN  = "[Sender] Replication <%s> Failback Master Begin... at [<%ld>]";
RP_TRC_S_NTC_FAILBACK_SLAVE_BEGIN   = "[Sender] Replication <%s> Failback Slave Begin... at [<%ld>]";
RP_TRC_S_NTC_FAILBACK_SUCCEED       = "[Sender] Replication <%s> Failback Succeeded... at [<%ld>]";
RP_TRC_S_NTC_FAILBACK_FAIL          = "[Sender] Replication <%s> Failback Failed... at [<%ld>]";
RP_TRC_S_GET_IP_BY_HOSTNAME         = "[Sender] Get ip address(<%s>) from hostname(<%s>)";

## rpxSenderApply
RP_TRC_SA_UPDATE_XSN_TRANS_INIT     = "[SenderApply] Trans init() error in updateXSN()";
RP_TRC_SA_UPDATE_XSN_TRANS_BEGIN    = "[SenderApply] Trans begin() error in updateXSN()";
RP_TRC_SA_UPDATE_XSN_TRANS_COMMIT   = "[SenderApply] Trans commit() error in updateXSN()";
RP_TRC_SA_UPDATE_XSN_TRANS_DESTROY  = "[SenderApply] Trans destroy() error in updateXSN()";
RP_TRC_SA_UPDATE_XSN_UPDATE_XSN     = "[SenderApply] rpcExecutor updateXSN()  error in updateXSN()";
RP_TRC_SA_SENDER_APPLY_START        = "[SenderApply] Replication <%s> Start";
RP_TRC_SA_SENDER_APPLY_END          = "[SenderApply] Replication <%s> Normal End";
RP_TRC_SA_SENDER_APPLY_GOT_ERROR    = "[SenderApply] Replication <%s> Got Error";

## rpxSenderHandshake
RP_TRC_SH_ERR_REL_DISCONNECT         = "[SenderHandshake] disconnectPeer() error in releaseHandshake()";
RP_TRC_SH_ERR_CON_SOCKET_SETNONBLOCK = "[SenderHandshake] setNonBlock() error";
RP_TRC_SH_ERR_CON_SOCKET_CLOSE       = "[SenderHandshake] closesocket() error in connectPeer()";
RP_TRC_SH_ERR_DISCON_SOCKET_CLOSE    = "[SenderHandshake] closesocket() error in disconnectPeer()";
RP_TRC_SH_NTC_HANDSHAKE_XLOG         = "[SenderHandshake] Handshake without network reconnection";
RP_TRC_SH_NTC_NETWORK_CLOSE          = "[SenderHandshake] Normal network closing";

## rpxSenderXLog
RP_TRC_SX_NTC_TABLE_META_LOG       = "[SenderXLog] Table Meta Log [TableOID:<%ld>-><%ld>, "
                                     "User Name:<%s>, Table Name:<%s>, Partition Name:<%s>]";
RP_TRC_SX_REACH_END_ARCHIVE_LOG    = "[ArchiveALA] Reach end of archive log : mXSN=[<%ld>], file#=[<%u>]";
RP_TRC_SX_REPLICATE_REDO_LOG_NO    = "[ArchiveALA] Replicate redo log : mXSN=[<%ld>], file#=[<%u>]";
RP_TRC_SX_REPLICATE_ARCHIVE_LOG_NO = "[ArchiveALA] Replicate archive log : mXSN=[<%ld>], file#=[<%u>]";

## rpxSenderSyncParallel
RP_TRC_SSP_ERR_SYNC                 = "[SenderSyncParallel] Error in syncParallel()";
RP_TRC_SSP_ERR_ADD_JOB              = "[SenderSyncParallel] Error in addJob()";
RP_TRC_SSP_ERR_ALLOC_JOB            = "[SenderSyncParallel] Error in allocJob()";
RP_TRC_SSP_ERR_DESTOY_JOB           = "[SenderSyncParallel] Error in destroyJob()";
RP_TRC_SSP_ERR_ALLOC_SCN_LOCK_TABLE = "[SenderSyncParallel] lockTable() error in allocSCN()";
RP_TRC_SSP_ERR_ALLOC_SCN            = "[SenderSyncParallel] Error in allocSCN()";
RP_TRC_SSP_ERR_DESTOY_SCN           = "[SenderSyncParallel] Error in destroySCN()";

## rpxReceiver
RP_TRC_R_ERR_SHUT_RA_BYE           = "[Receiver] ReceiverApply.byeNow() error in shutdown()";
RP_TRC_R_ERR_FINAL_SOCKET_SHUTDOWN = "[Receiver] shutdown() error in finalize()";
RP_TRC_R_ERR_FINAL_SOCKET_CLOSE    = "[Receiver] closesocket() error in finalize()";
RP_TRC_R_ERR_FINAL_RA_SHUTDOWN     = "[Receiver] ReceiverApply.shutdown() error in finalize()";
RP_TRC_R_ERR_FINAL_THREAD_JOIN     = "[Receiver] thr_join() error in finalize()";
RP_TRC_R_RECEIVER_START            = "[Receiver] Replication <%s> Started ...";
RP_TRC_R_RECEIVER_STOP             = "[Receiver] Replication <%s> Stopped ...";
RP_TRC_R_RECO_RECEIVER_START       = "[Recovery Receiver] Replication <%s> Started ...";
RP_TRC_R_ERR_EAGER_ABNORMAL_STOP   = "[Receiver] Eager Replication <%s> Abnormally Stopped ...";
RP_TRC_R_STOP_MSG_ARRIVED          = "[Receiver] Replication Stop Message is arrived";
RP_TRC_R_NORMAL_STOP               = "[Receiver] Normal Stop";
RP_TRC_R_LAST_RECEIVED_SN          = "[Receiver] RepName:<%s> is received at [Last Received SN:<%lu>]";
RP_TRC_R_LAST_PROCESSED_SN         = "[Receiver] RepName:<%s> is processed at [Last Processed SN:<%lu>]";
RP_TRC_R_PEER_IP_PORT_NAME         = "[Receiver] Peer IP : <%s>. Peer Port : <%d>. RepName : <%s> ";
RP_TRC_R_CONSISTENT_RECO_RECEIVER_START = "[Consistent recovery Xlogfile Receiver] Replication <%s> Started ...";
RP_TRC_R_CONSISTENT_FAILOVER_RECEIVER_START = "[Consistent Failover Receiver] Replication <%s> Started ...";

## rpxReceiverApply
RP_TRC_RA_ERR_META           = "[ReceiverApply] <%s>.<%s> does not exist.";
RP_TRC_RA_CONFLICT_COMMIT    = "[ReceiverApply] Commit conflict";
RP_TRC_RA_CONFLICT_ABORT     = "[ReceiverApply] Abort conflict";
RP_TRC_RA_ERR_LOB_CLOSE      = "[ReceiverApply] Lob Cursor Close Error";
RP_TRC_RA_NTC_REPL_STOP_XLOG = "[ReceiverApply] REPLICATION STOP XLog arrived!";
RP_TRC_RA_NTC_HANDSHAKE_XLOG = "[ReceiverApply] Handshake without network reconnection";
RP_TRC_RA_NTC_SYNC_PK_BEGIN  = "[ReceiverApply] Incremental Sync Primary Key Begin";
RP_TRC_RA_NTC_SYNC_PK        = "[ReceiverApply] Incremental Sync Primary Key";
RP_TRC_RA_NTC_SYNC_PK_END    = "[ReceiverApply] Incremental Sync Primary Key End";
RP_TRC_RA_NTC_FAILBACK_END   = "[ReceiverApply] Failback End";
RP_TRC_RA_ERR_INVALID_XLOG   = "[ReceiverApply] Invalid XLog : Type[<%d>], TID[<%u>], SN[<%ld>], Table OID[<%ld>]";

## rpxReceiverHandshake
RP_TRC_RH_ERR_CHK_SOCKET_RECV = "[ReceiverHandshake] HBT message is received.";
RP_TRC_RH_NTC_CHK_CONNECTED   = "[ReceiverHandshake] Connected from <%s>:<%u>";

## rpdLogBufferMgr
RP_TRC_LB_NO_SPACE       = "[RPLogBufferMgr] Unable to write copy of transaction log into the replication log buffer.";
RP_TRC_LB_OVERFLOW_RPBUF = "[RPLogBufferMgr] The transaction log size is greater than replication log buffer size.";

## rpdTransTbl
RP_TRC_TT_DUPLICATED_BEGIN    = "Entry already exists in transaction table "
                                "[existing begin SN in transaction table:<%lu>, "
                                "analyzing begin SN:<%lu>, analyzing TID:<%u>, trasaction table size:<%d>]";

## rpdSenderInfo
RP_TRC_SI_REPLICATION_GAP_IS_OVER_MAX = "replication gap exceeds REPLICATION_SERVICE_WAIT_MAX_LIMIT";
RP_TRC_SI_REPLICATION_GAP_IS_UNDER_MAX = "replication gap goes down REPLICATION_SERVICE_WAIT_MAX_LIMIT";

## rpdCatalog
RP_TRC_D_ERR_UPDATE_RECEVIER_APPLY_COUNT = "[rpdCatalog::updateReceiverApplyCount] [UPDATE SYS_REPLICATIONS_ = <%ld>]";

## rpnMessenger
RP_TRC_M_LINK_IS_NON_BLOCKING = "[Messenger] Socket is in non-blocking mode";
RP_TRC_M_LINK_IS_BLOCKING = "[Messenger] Socket is in blocking mode";

## rpcSQLManager
RP_TRC_SQLMANAGER_FAILURE = "[SQLExecutor] Local Command related to replication was failed. [Type: <%d>]";

## rpxAhaedAnalyzer
RP_TRC_X_AHEAD_ANALYZER_IS_START = "[AheadAnalyzer] Thead is started";
RP_TRC_X_AHEAD_ANALYZER_IS_END = "[AheadAnalyzer] Thead is ended";
RP_TRC_X_AHEAD_ANALYZER_ANALYZE_IS_START = "[AheadAnalyzer] Analyzing is started";
RP_TRC_X_AHEAD_ANALYZER_ANALYZE_IS_STOP = "[AheadAnalyzer] Analyzing is stopped";
RP_TRC_X_AHEAD_ANALYZER_START_FILENO = "[AheadAnalyzer] File No(<%d>) is started to analyze";
RP_TRC_X_AHEAD_ANALYZER_SKIP_FILENO = "[AheadAnalyzer] File No(<%d>) is skipped";
RP_TRC_X_AHEAD_ANALYZER_TRY_STOP_ANALYZE = "[AheadAnalyzer] Sender tries to stop to anlzye";
RP_TRC_X_AHEAD_ANALYZER_SET_FILENO = "[AheadAnalyzer] AheadAnalyzer Read FileNo <%d> Sender Read FileNo <%d>";
RP_TRC_X_AHEAD_ANALYZER_CAN_NOT_ANALYZE = "[ReceiverApplier] Can not analyze, it could get error";

## rpxReceiverApplier
RP_TRC_X_APPLIER_IS_START = "[ReceiverApplier] [RepName:<%s>, Index:<%d>] is started";
RP_TRC_X_APPLIER_TIMEWAIT_ERROR = "[ReceiverApplier] Got Error timedwait()";
RP_TRC_X_APPLIER_IS_STOP = "[ReceiverApplier] [RepName:<%s>, Index:<%d>] is stopped";
RP_TRC_X_APPLIER_GOT_FAILURE = "[ReceiverApplier] [RepName:<%s>, Index:<%d>] got failure";
RP_TRC_X_APPLIER_REMAINED_QUEUE_INFO = "[ReceiverApplier] RepName:<%s> Index:<%d> Remained Queue : <%d>";
RP_TRC_X_APPLIER_PROCESSED_XLOG_COUNT = "[ReceiverApplier] RepName:<%s> Index:<%d> processed <%d> XLogs";
RP_TRC_X_APPLIER_LAST_PROCESSED_SN    = "[ReceiverApplier] RepName:<%s> Index:<%d> is processed at [Last Processed SN:<%lu>]";

## rpdMeta
RP_TRC_R_CONVERT_APPLY_SQL_MODE        = "Table( OID : <%ld>, Name : <%s>.<%s>.<%s> ) in replication( Name : <%s> ) is convert apply sql mode";
RP_TRC_R_CONVERT_APPLY_SKIP_MODE        = "Table( OID : <%ld>, Name : <%s>.<%s>.<%s> ) in replication( Name : <%s> ) is convert apply skip mode";

INTERNAL_TRACE_MESSAGE_END
