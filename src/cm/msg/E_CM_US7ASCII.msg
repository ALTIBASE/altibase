# CM 에러 메시지 화일 (used by genErrMsg)

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
# Last Number = 196

SECTION = 7

0, cmERR_IGNORE_NoError = No CM module error
# *Cause: This is not an error.
# *Action: You can safely ignore this error message.

1, cmERR_ABORT_INVALID_MODULE = Invalid module
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 2, cmERR_ABORT_INVALID_MODULE_VERSION = Invalid module version
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

3, cmERR_ABORT_INVALID_SESSION = Invalid session
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

4, cmERR_ABORT_INVALID_OPERATION = Invalid operation
# *Cause: Client version is higher than Server version or internal error occurs while interpreting protocol.
# *Action: Make sure Client version is same or lower than Server version or contact Altibase's Support Center (http://support.altibase.com).

5, cmERR_ABORT_INVALID_LINK_TYPE = Invalid link type
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

6, cmERR_ABORT_INVALID_LINK_IMPL = Invalid link impl
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 7, cmERR_ABORT_INVALID_DISPATCHER_IMPL = Invalid dispatcher impl
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

8, cmERR_ABORT_UNSUPPORTED_LINK_IMPL = Unsupported link impl
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

9, cmERR_ABORT_UNSUPPORTED_DISPATCHER_IMPL = Unsupported dispatcher impl
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

10, cmERR_ABORT_SESSION_NOT_ADDED = Session not added
# *Cause: Internal error.
# *Action: Verify that the session has been added.

11, cmERR_ABORT_SESSION_LIMIT_REACH = Session limit reached
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

12, cmERR_ABORT_SESSION_DOES_NOT_EXIST = Session does not exist
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

13, cmERR_ABORT_SOCKET_ALREADY_OPENED = Socket already opened
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

14, cmERR_ABORT_SOCKET_OPEN_ERROR = Unable to create a socket, errno=<0%u>
# *Cause: Failed to invoke the socket() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

15, cmERR_ABORT_BIND_ERROR = Unable to bind the socket, errno=<0%u>
# *Cause: Failed to invoke the bind() system function on the socket.
# *Action: Check the error number and take the appropriate action.

16, cmERR_ABORT_LISTEN_ERROR = Failed to invoke the listen() system function
# *Cause: Failed to invoke the listen() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

17, cmERR_ABORT_LINK_LIMIT_REACH = Link limit reached
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

18, cmERR_ABORT_LINK_ALREADY_IN_DISPATCHING = Link already in dispatch
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

19, cmERR_ABORT_TIMED_OUT = Timed out
# *Cause: A network timeout occurred
# *Action: Check the network.

20, cmERR_ABORT_POLL_ERROR = Failed to invoke the poll() system function
# *Cause: Failed to invoke the poll() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

21, cmERR_ABORT_SELECT_ERROR = Failed to invoke the select() system function, socket_no=<0%d>
# *Cause: Failed to invoke the select() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

22, cmERR_ABORT_ACCEPT_ERROR = Failed to invoke the accept() system function
# *Cause: Failed to invoke the accept() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

23, cmERR_ABORT_CONNECT_ERROR = Failed to invoke the connect() system function, errno=<0%u>
# *Cause: Failed to invoke the connect() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

24, cmERR_ABORT_RECV_ERROR = Failed to invoke the read() system function
# *Cause: Failed to invoke the read() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

25, cmERR_ABORT_SEND_ERROR = Failed to invoke the write() system function
# *Cause: Failed to invoke the write() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

26, cmERR_ABORT_CONNECTION_CLOSED = Connection closed
# *Cause: The connection has been closed due to network failure or abnormal client termination.
# *Action: Check the network or client status.

27, cmERR_ABORT_UNIX_PATH_TRUNCATED = Unix domain socket path truncated
# *Cause: Unix domain socket path truncated
# *Action: The Altibase socket path is too long. Make it shorter.

28, cmERR_ABORT_LINK_ALREADY_REGISTERED = Link already registered to Session
# *Cause: Link already registered to session
# *Action: Ensure that a session has been initialized and ensure that no link is already registered.

29, cmERR_ABORT_PROTOCOL_HEADER_ERROR = Protocol header error.(<0%s>)
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

30, cmERR_ABORT_UNKNOWN_MODULE = Unknown module
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 31, cmERR_ABORT_UNKNOWN_MODULE_VERSION = Unknown module version
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 32, cmERR_ABORT_UNKNOWN_OPERATION = Unknown operation
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

33, cmERR_ABORT_INVALID_PROTOCOL_SEQUENCE = Invalid protocol sequence
# *Cause: Invalid protocol sequence
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

34, cmERR_ABORT_INVALID_XA_XID_DATA_SIZE = Invalid XA XID data size
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

35, cmERR_ABORT_MARSHAL_ERROR = Protocol marshal error
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

36, cmERR_ABORT_CALLBACK_DOES_NOT_EXIST = Callback does not exist
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 37, cmERR_ABORT_CALLBACK_ERROR = Callback error
# *Cause: Callback error. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

38, cmERR_ABORT_INCOMPLETE_VARIABLE = Incomplete variable
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

39, cmERR_ABORT_VARIABLE_NOT_EMPTY = Variable not empty
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

40, cmERR_ABORT_VARIABLE_PIECE_RANGE_MISMATCH = Variable piece range mismatch
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

41, cmERR_ABORT_VARIABLE_SEQUENCE_SIZE_MISMATCH = Variable sequence size mismatch
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

42, cmERR_ABORT_INVALID_VARIABLE_TYPE = Invalid variable type
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

43, cmERR_ABORT_INVALID_DATATYPE = Invalid data type
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 44, cmERR_ABORT_PROTOCOL_ERROR = Protocol error
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 45, cmERR_ABORT_SHM_ALREADY_CREATED = Shared memory for IPC already allocated
# *Cause: Shared memory for IPC is already allocated. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

46, cmERR_ABORT_NUMERIC_SIZE_OVERFLOW = Numeric data size overflow
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

47, cmERR_ABORT_SOCKET_SET_BLOCKING_FAILED = Unable to set the socket to blocking mode
# *Cause: Failed to invoke the fcntl() function for setting a socket to blocking mode. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

48, cmERR_ABORT_SOCKET_SET_NONBLOCKING_FAILED = Unable to set the socket to non-blocking mode
# *Cause: Failed to invoke the fcntl() function for setting a socket to non-blocking mode. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

49, cmERR_RETRY_SOCKET_OPERATION_WOULD_BLOCK = Socket operation would be blocked
# *Cause: A socket is not ready for this operation
# *Action: You may safely ignore this message.

50, cmERR_ABORT_SOCKET_SHUTDOWN_FAILED = Unable to shut down the socket
# *Cause: Failed to invoke the shutdown() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

51, cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY = Unsupported link info key
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

52, cmERR_ABORT_GETSOCKNAME_ERROR = Failed to invoke the getsockname() system function
# *Cause: Failed to invoke the getsockname() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 53, cmERR_ABORT_INET_NTOP_ERROR = Failed to invoke the inet_ntop() system function
# *Cause: Failed to invoke the inet_ntop() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

54, cmERR_ABORT_SHARED_POOL_NOT_EXIST = Shared pool does not exist
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

55, cmERR_ABORT_STRING_OUTPUT_ERROR = String output error
# *Cause: Failed to invoke a string output function, such as printf(). This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

56, cmERR_IGNORE_STRING_TRUNCATED = String truncated
# *Cause: Output string truncated.
# *Action: Ignore this error message.

# 57, cmERR_IGNORE_END_OF_PROTOCOL_SEQUENCE = End of protocol sequence
# *Cause: End of protocol sequence reached.
# *Action: Ignore this error message.

58, cmERR_ABORT_VARIABLE_PIECE_TOO_LARGE = Variable piece too large
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

59, cmERR_ABORT_VARIABLE_SIZE_MISMATCH = Variable size mismatch
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

60, cmERR_ABORT_VARIABLE_RANGE_MISMATCH = Variable range mismatch
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 61, cmERR_ABORT_GETHOSTBYNAME_R_ERROR = Failed to invoke the gethostbyname_r() system function
# *Cause: Failed to invoke the gethostbyname_r() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 62, cmERR_ABORT_INET_NTOA_ERROR = Failed to invoke the inet_ntoa() system function
# *Cause: Failed to invoke the inet_ntoa() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

118, cmERR_FATAL_CMN_SHM_ATTACH = Unable to attach the shared memory
# *Cause: Failed to invoke the shmat() function
# *Action: Ensure that you have permission to access the shared memory. Contact Altibase's Support Center (http://support.altibase.com).

119, cmERR_FATAL_CMN_SHM_CTL = Unable to delete the shared memory
# *Cause: Failed to invoke the shmctl() function
# *Action: Ensure that you have permission to access the shared memory. Contact Altibase's Support Center (http://support.altibase.com).

120, cmERR_FATAL_CMN_SEM_CTL = Unable to delete the semaphore
# *Cause: Failed to invoke the semctl() function
# *Action: Verify that you have permission to access the semaphore. Contact Altibase's Support Center (http://support.altibase.com).

121, cmERR_FATAL_CMN_SEM_OP  = Unable to execute operations on a semaphore
# *Cause: Failed to invoke the semop() function
# *Action: Check the error number and verify that the semaphore group exists.

# 122, cmERR_IGNORE_CMN_CHANNEL_OPEN_FAILED = Unable to open a communication channel because none has been created
# *Cause: Failed to open a communication channel because no communication channel has been created yet
# *Action: This error does not originate with Altibase.

# 123, cmERR_FATAL_CMN_CHANNEL_NOT_CREATED = Unable to delete the communication channel
# *Cause: The system tried to delete an invalid communication channel.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 124, cmERR_FATAL_CMN_INVALID_CHANNEL_TYPE = Unsupported communication channel.
# *Cause: The system tried to use an unsupported communication channel.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

125, cmERR_FATAL_CMN_MUTEX_LOCK = Failed to invoke the mutex_lock() system function
# *Cause: Failed to invoke the mutex_lock() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

126, cmERR_FATAL_CMN_MUTEX_UNLOCK = Failed to invoke the mutex_unlock() system function
# *Cause: Failed to invoke the mutex_unlock() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 127, cmERR_FATAL_CMN_CLOSE_FAILED = Unable to close the communication channel
# *Cause: Failed to invoke the close() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 130, cmERR_FATAL_CMN_UNIX_SOCKET_CREATE_FAILED = Unable to create a UNIX domain socket
# *Cause: Failed to invoke the socket() function on a UNIX domain socket. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 132, cmERR_FATAL_CMN_SVC_UNIX_BIND_ERROR = Unable to bind the UNIX domain socket
# *Cause: Failed to invoke the bind() function on a UNIX domain socket. The port was in use by another process.
# *Action: Check whether the port is in use by another process.

# 134, cmERR_FATAL_CMN_SVC_UNIX_LISTEN_ERROR = Unable to invoke the listen() function on the UNIX domain socket
# *Cause: Failed to invoke the listen() function on the UNIX domain socket. This is a system call error.
# *Action: Check whether the port is in use by another process.

# 135, cmERR_FATAL_CMN_FD_NONBLOCK_FAILED = Unable to set the socket to non-blocking mode.
# *Cause: Failed to invoke the fcntl() function to set non-blocking mode on a file descriptor. This is a system call error.
# *Action: Check whether the port is in use by another process.

# 137, cmERR_ABORT_CMN_MARSHAL_BUFFER_FULL = Insufficient communication buffer.
# *Cause: The communication buffer is insufficient.
# *Action: Verify that the size of the communication buffer specified in the property file is valid. ( e.g., > 64KB )

138, cmERR_ABORT_CMN_SOCKET_CLOSED = Socket communication error
# *Cause: The socket on the client has already been closed.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

139, cmERR_ABORT_CMN_ERR_FULL_IPC_CHANNEL = No more IPC channels
# *Cause: All IPC channels have been allocated to clients.
# *Action: Use a TCP/UNIX Domain channel or create more IPC channels.

140, cmERR_ABORT_CMN_SEM_INIT_OP  = Unable to invoke the semop() function as described in the system semaphore properties.
# *Cause: Failed to invoke the semop() system function
# *Action: Check the system semaphore properties.

# 141, cmERR_FATAL_CMN_SYS_SELECT_FAILED = Failed to invoke the select() system function
# *Cause: Failed to invoke the select() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 142, cmERR_FATAL_WRITELINE_ERROR = Unable to communicate with the db admin
# *Cause: Failed to invoke the timed_writeline() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 148, cmERR_ABORT_CMN_MARSHAL_FIND_FAIL = Unable to find the specified unique marshal id.
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

150, cmERR_ABORT_GETADDRINFO_ERROR = Failed to invoke the getaddrinfo() system function: <0%s>
# *Cause: Failed to invoke the getaddrinfo() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

151, cmERR_ABORT_GETNAMEINFO_ERROR = Failed to invoke the getnameinfo() system function
# *Cause: Failed to invoke the getnameinfo() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

152, cmERR_ABORT_SETSOCKOPT_ERROR = Failed to invoke the setsockopt() system function: <0%s>
# *Cause: Failed to invoke the setsockopt() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

153, cmERR_ABORT_CONNECT_INVALIDARG = Invalid argument supplied for connect()
# *Cause: Invalid argument supplied for connect().
# *Action: Check the IP address and hostname.

# 154, cmERR_ABORT_CMN_PIPE_INIT_OP = Failed to invoke ConnectNamedPipe() system function
# *Cause: Failed to invoke the ConnectNamedPipe() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

# 155, cmERR_ABORT_CONNECTION_CLOSED_BY_PIPE_ERROR = Connection closed by pipe error
# *Cause: The connection has been closed due to a pipe error. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

156, cmERR_ABORT_SHMGET_ERROR = Failed to get shared memory ID
# *Cause: Failed to get shared memory ID with the key
# *Action: Check your platform.

157, cmERR_ABORT_COMPRESS_DATA_ERROR = Failed to compress data in a packet
# *Cause: Failed to compress packet data.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

158, cmERR_ABORT_DECOMPRESS_DATA_ERROR = Failed to decompress data in a packet
# *Cause: Failed to decompress packet data.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

159, cmERR_ABORT_UNSUPPORTED_NETWORK_PROTOCOL = Unsupported network protocol
# *Cause: Unsupported network protocol
# *Action: Check the protocol version of the client application and use an appropriate library. 

160, cmERR_ABORT_INVALID_CERTIFICATE = Failed to load a certificate. SSL error: <0%s>
# *Cause: Failed to load a valid certificate from the file the user specified in the altibase.properties file.
# *Action: Check whether or not the certificate and its location are valid. 

161, cmERR_ABORT_INVALID_PRIVATE_KEY = Failed to load a private key. SSL error: <0%s>
# *Cause: Failed to load a valid private key from the file the user specified in the altibase.properties file.
# *Action: Check whether or not the private key and its location are valid.

162, cmERR_ABORT_PRIVATE_KEY_VERIFICATION = Failed to verify the private key. SSL error: <0%s>
# *Cause: Private key does not match the certificate loaded into SSL context. 
# *Action: Check whether or not the private key matches the certificate. 

163, cmERR_ABORT_SSL_HANDSHAKE = SSL handshake failed. SSL error: <0%s>
# *Cause: SSL handshake failed. 
# *Action: Refer to the altibase_boot.log file for further information about the failure.

164, cmERR_ABORT_SSL_READ = SSL read failed. SSL error: <0%s>
# *Cause: SSL read failed. 
# *Action: Refer to the altibase_boot.log file for further information about the failure.

165, cmERR_ABORT_SSL_WRITE = SSL write failed. SSL error: <0%s>
# *Cause: SSL write failed. 
# *Action: Refer to the altobase_boot.log file for further information about the failure.

166, cmERR_ABORT_SSL_SHUTDOWN = SSL shutdown failed. SSL error: <0%s>
# *Cause: SSL shutdown failed. 
# *Action: Refer to the altobase_boot.log file for further information about the failure.

167, cmERR_ABORT_INVALID_VERIFY_LOCATION = Failed to load trusted certificates from the specified location(s). SSL error: <0%s>
# *Cause: Failed to load trusted certificates from the specified location(s).
# *Action: Check whether the CA and/or CAPath properties are valid. 

168, cmERR_ABORT_INVALID_CA_LIST_FILE = Failed to load trusted certificates from the CA file. SSL error: <0%s>
# *Cause: Failed to load trusted certificates from the CA file.
# *Action: Check whether or not the CA file is valid.

169, cmERR_ABORT_SSL_CONNECT = SSL connect failed.
# *Cause: SSL connect failed. 
# *Action: Refer to the altibase_boot.log file for further information about the failure.

170, cmERR_ABORT_VERIFY_PEER_CERITIFICATE = Failed to verify the peer certificate. SSL error: <0%s>
# *Cause: Failed to verify the peer certificate.
# *Action: Check whether or not the peer has a valid certificate.

171, cmERR_ABORT_SSL_OPERATION = SSL error: <0%s>
# *Cause: SSL operation failed.
# *Action: Refer to the altibase_boot.log file for further information about the failure.

172, cmERR_ABORT_DLOPEN = Failed to open the <0%s> library. Error: <1%s>
# *Cause: Failed to open the library. 
# *Action: Check whether or not the library has been installed and set properly. 

173, cmERR_ABORT_DLSYM = Failed to load functions from the <0%s> library. Error: <1%s>
# *cause: Failed to load functions from the library.
# *Action: Check whether or not the library has been installed properly. 
                                                                                   
174, cmERR_ABORT_GETSOCKOPT_ERROR = Failed to invoke the getsockopt() system function: <0%s>
# *Cause: Failed to invoke the getsockopt() system function. This is a system call error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).
                                                                                   
175, cmERR_ABORT_SHARD_VERSION_MISMATCH = Mismatched Shard version.
# *Cause: The shard versions of server and client do not correspond.
# *Action: Use the identical shard and client versions.

176, cmERR_ABORT_SYSTEM_CALL_ERROR = Failed to invoke the <0%s> system function.
# *Cause: Failed to invoke the system function. This is a system call error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

#############################
# IB
#############################

177, cmERR_ABORT_IB_RSOCKET_ALREADY_OPENED = IB rsocket already opened
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

178, cmERR_ABORT_IB_RSOCKET_OPEN_ERROR = Unable to create a IB rsocket, errno=<0%u>
# *Cause: Failed to invoke the rsocket() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

179, cmERR_ABORT_IB_RBIND_ERROR = Unable to bind the IB rsocket, errno=<0%u>
# *Cause: Failed to invoke the rbind() function on the IB rsocket.
# *Action: Check the error number and take the appropriate action.

180, cmERR_ABORT_IB_RLISTEN_ERROR = Failed to invoke the rlisten() function
# *Cause: Failed to invoke the rlisten() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

181, cmERR_ABORT_IB_RPOLL_ERROR = Failed to invoke the rpoll() function
# *Cause: Failed to invoke the rpoll() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

182, cmERR_ABORT_IB_RSELECT_ERROR = Failed to invoke the rselect() function
# *Cause: Failed to invoke the rselect() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

183, cmERR_ABORT_IB_RACCEPT_ERROR = Failed to invoke the raccept() function
# *Cause: Failed to invoke the raccept() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

184, cmERR_ABORT_IB_RCONNECT_INVALIDARG = Invalid argument supplied for rconnect()
# *Cause: Invalid argument supplied for rconnect().
# *Action: Check the IP address and hostname.

185, cmERR_ABORT_IB_RCONNECT_ERROR = Failed to invoke the rconnect() function, errno=<0%u>
# *Cause: Failed to invoke the rconnect() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

186, cmERR_ABORT_IB_RRECV_ERROR = Failed to invoke the rread() function
# *Cause: Failed to invoke the rread() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

187, cmERR_ABORT_IB_RSEND_ERROR = Failed to invoke the rwrite() function
# *Cause: Failed to invoke the rwrite() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

188, cmERR_ABORT_IB_RSOCKET_SHUTDOWN_FAILED = Unable to shut down the socket
# *Cause: Failed to invoke the rshutdown() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

189, cmERR_ABORT_IB_RSETSOCKOPT_ERROR = Failed to invoke the rsetsockopt() function: <0%s>
# *Cause: Failed to invoke the rsetsockopt() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

190, cmERR_ABORT_IB_RGETSOCKOPT_ERROR = Failed to invoke the rgetsockopt() function: <0%s>
# *Cause: Failed to invoke the rgetsockopt() function.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

191, cmERR_ABORT_IB_RSOCKET_SET_BLOCKING_FAILED = Unable to set the IB rsocket to blocking mode
# *Cause: Failed to invoke the rfcntl() function for setting a socket to blocking mode.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

192, cmERR_ABORT_IB_RSOCKET_SET_NONBLOCKING_FAILED = Unable to set the IB rsocket to non-blocking mode
# *Cause: Failed to invoke the rfcntl() function for setting a socket to non-blocking mode.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

193, cmERR_RETRY_IB_RSOCKET_OPERATION_WOULD_BLOCK = IB rsocket operation would be blocked
# *Cause: A IB rsocket is not ready for this operation
# *Action: You may safely ignore this message.

194, cmERR_ABORT_IB_RPOLL_INVALID_DIRECTION = Invalid direction: <0%d>
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

195, cmERR_ABORT_COMPRESS_DATA_ERROR_WITH_TYPE = Failed to compress data in a packet. (compress type : <0%d>)
# *Cause: Failed to compress packet data.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

196, cmERR_ABORT_DECOMPRESS_DATA_ERROR_WITH_TYPE = Failed to decompress data in a packet. (decompress type : <0%d>)
# *Cause: Failed to decompress packet data.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).


###############################################################################
#  Server Internal Message
###############################################################################

INTERNAL_TRACE_MESSAGE_BEGIN

CM_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
                "signed long =<%ld>\n unsigned long = <%lu>\n"
                "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>";

CM_TRC_SOCKET_KEEPALIVE_FAIL = "Failed to set SO_KEEPALIVE for socket (errno=<%u>) -> ignored\n";

CM_TRC_SOCKET_REUSEADDR_FAIL = "Failed to set SOL_REUSEADDR for socket (errno=<%u>) -> ignored\n";

CM_TRC_SOCKET_SET_BLOCKING_MODE_FAIL = "Failed to set Blocking Mode for socket (errno=<%u>)";

CM_TRC_TCP_NODELAY_FAIL = "Failed to set TCP_NODELAY for TCP socket (errno=<%u>) -> ignored\n";

CM_TRC_SOCKET_SET_OPTION_FAIL = "Failed to set <%s> for socket (errno=<%u>) -> ignored\n";

CM_TRC_TCP_BIND_FAIL = "Failed to bind TCP socket to the port <%s> (errno=<%u>)";

CM_TRC_IB_INITIALIZED = "IB is initialized successfully.";

CM_TRC_IB_RSOCKET_KEEPALIVE_FAIL = "Failed to set SO_KEEPALIVE for IB rsocket (errno=<%u>) -> ignored\n";

CM_TRC_IB_RSOCKET_REUSEADDR_FAIL = "Failed to set SOL_REUSEADDR for IB rsocket (errno=<%u>) -> ignored\n";

CM_TRC_IB_RSOCKET_SET_BLOCKING_MODE_FAIL = "Failed to set Blocking Mode for IB rsocket (errno=<%u>)";

CM_TRC_IB_TCP_NODELAY_FAIL = "Failed to set TCP_NODELAY for IB rsocket (errno=<%u>) -> ignored\n";

CM_TRC_IB_RSOCKET_SET_OPTION_FAIL = "Failed to set <%s> for IB rsocket (errno=<%u>) -> ignored\n";

CM_TRC_IB_RBIND_FAIL = "Failed to bind IB rsocket to the port <%s> (errno=<%u>)";

CM_TRC_IPC_SHM_REMOVE_FAIL = "Failed to remove shm";

CM_TRC_IPC_FILE_READ_ERROR = "Can't read IPC-File -> ignored\n";

CM_TRC_IPC_NO_SEMID = "Old Sema-Id Doesn't exit -> ignored\n";

CM_TRC_IPC_NO_SEMKEY = "Old Sema-Key Doesn't exit -> ignored\n";

CM_TRC_IPC_FILE_OPEN_ERROR = "Can't Open IPC-File -> ignored\n";

CM_TRC_NO_SHM_MEM = "32-bit Memory Size Overflow = <%lu>\n";

CM_TRC_IPC_SHMGET_ERROR = "shmget() error (errno=<%u>d)\n";

CM_TRC_NO_SEM_MEM = "Not Enough Semaphore Memory (errno=<%u>)\n";

CM_TRC_IPC_SEMGET_ERROR = "semget() error : errno=<%u>\n";

CM_TRC_IPC_SEMRESET_ERROR = "Semaphore reset error (errno=<%d>)\n";

CM_TRC_IPC_SEMINIT_ERROR = "Server semaphore initialization error (errno=<%d>)\n";

CM_TRC_IPC_CLIENT_SEMINIT_ERROR = "Client semaphore initialization error (errno=<%d>)\n";

CM_TRC_IPC_CLIENT_PIPEINIT_ERROR = "Client pipe initialization error (errno=<%d>)\n";

CM_TRC_MEM_ALLOC_ERROR = "Memory alloction error (errno=<%d>) (requested size : <%lu>)\n";

CM_TRC_DUMP_CTX           = "# CmSeqNo (Ctx) = <%u>\n\n";

CM_TRC_DUMP_PACKET_HEADER = "# HeaderSign    = <%u>\n"
                            "# PayloadLength = <%u>\n"
                            "# CmSeqNo       = <%u>\n"
                            "# DataSize      = <%u>\n";

CM_TRC_DUMP_PACKET_PAYLOAD = "# BLOCK PAYLOAD FROM <%u> TO <%u>";

CM_TRC_COMPRESS_TYPE_ERROR = "Invalid send compress type : <%d>\n";

CM_TRC_DECOMPRESS_TYPE_ERROR = "Invalid receive decompress type : <%d>\n";

INTERNAL_TRACE_MESSAGE_END
