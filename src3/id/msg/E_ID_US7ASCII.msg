# $Id: E_ID_US7ASCII.msg 29483 2008-11-21 03:27:17Z sungminee $
#
# ID 에러 메시지 화일 (used by genErrMsg)
#

# 에러코드 영역 번호  # BUGBUG : 나중에 순서를 id 순으로 바꿔야 할 것임
# 0 => id
# 1 => sm
# 2 => mt
# 3 => qp
# 4 => mm
# 5 => rp2
# 6 => rp
# 7 => cm
# 8 => ul
# 9 => ut
# 10 => st
# 11 => lk
# 12 => dk

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.
# Last Number = 238

SECTION = 0

0, idERR_IGNORE_NoError = No ID module error
# *Cause: This is not an error.
# *Action: You may safely ignore this message.

1, idERR_ABORT_idnCharacterSetNotFound = No character set was found.
# *Cause: No character set was found.
# *Action: Specify a valid value for the NLS_USE property.

2, idERR_ABORT_idnInvalidCharacter = Invalid character in use
# *Cause: An invalid character is being used.
# *Action: Verify that every character in the input string is a valid character.

3, idERR_ABORT_idnReachEnd = Unexpected end of string
# *Cause: Unexpected end of string.
# *Action: Verify that the form of the string is legal.

4, idERR_ABORT_idnNotFound = Unable to find a string to search
# *Cause: The system failed to find a string to search.
# *Action: Please send a bug report to the vendor.

5, idERR_ABORT_idnTooLarge = The input string is too long.
# *Cause: Internal server error. The input string is too long.
# *Action: Please send a bug report to the vendor.

6, idERR_ABORT_idnLikeEscape = Invalid use of escape characters in LIKE predicate.
# *Cause: The server has detected the invalid use of escape characters in a LIKE predicate.
# *Action: Verify that the use of escape characters in the LIKE predicate is syntactically correct.

7, idERR_ABORT_idaNullValue = A NULL value is not allowed for this data type.
# *Cause: This data type cannot have a NULL value.
# *Action: Verify that there is no NULL value in the field.

8, idERR_ABORT_idaInvalidNumeric = Unable to cast the string to the INTEGER type.
# *Cause: It is impossible to cast the string to the INTEGER type.
# *Action: Change the string appropriately.

9, idERR_ABORT_idaInvalidNibble = Unable to cast the string to the NIBBLE type.
# *Cause: It is impossible to cast the string to the NIBBLE type.
# *Action: Change the string appropriately.

10, idERR_ABORT_idaInvalidBytes = Unable to cast the string to the BYTE type.
# *Cause: It is impossible to cast the string to the BYTE type.
# *Action: Change the string appropriately.

11, idERR_ABORT_idaOverflow = Value exceeds range supported by type
# *Cause: The value is out of the range supported by the type.
# *Action: Change the input value so that it falls within the range of values supported by the given type.

12, idERR_ABORT_idaDivideByZero = Division by zero
# *Cause: An attempt is being made to divide a value by zero.
# *Action: Verify that the value is not being divided by zero.

13, idERR_ABORT_idaLengthOutbound = Length exceeded
# *Cause: The data type length has been exceeded.
# *Action: Verify that the length of the data type is correct.

14, idERR_ABORT_idaPrecisionOutbound = Precision exceeded
# *Cause: The precision of the data type has been exceeded.
# *Action: Verify that the precision of the data type is correct.

15, idERR_ABORT_idaScaleOutbound = Scale exceeded
# *Cause: The scale of the data type has been exceeded.
# *Action: Verify that the scale of the data type is correct.

16, idERR_ABORT_idaLargerThanPrecision = Precision exceeded
# *Cause: The precision of the data type has been exceeded.
# *Action: Verify that the precision of the data type is correct.

17, idERR_ABORT_idaInvalidParameter = Invalid Argument
# *Cause: Invalid argument.
# *Action: Verify that the arguments are valid in context.

18, idERR_FATAL_idc_SHM_ATTACH = Unable to attach shared memory
# *Cause: Failed to invoke the shmat() function.
# *Action: Verify that you have permission to access shared memory. Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

19, idERR_FATAL_idc_SHM_CTL = Unable to delete shared memory
# *Cause: Failed to invoke the shmctl() function.
# *Action: Verify that you have permission to access shared memory. Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

20, idERR_FATAL_idc_SEM_CTL = Unable to delete a semaphore
# *Cause: Failed to invoke the semctl() function.
# *Action: Verify that you have permission to access the semaphore. Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

21, idERR_FATAL_idc_SEM_OP = Unable to execute operations on a semaphore
# *Cause: Failed to invoke the semop() function.
# *Action: Check the error number from altibase_boot.log and verify that the semaphore group exists.

22, idERR_IGNORE_idc_CHANNEL_OPEN_FAILED = Unable to open a communication channel because none has been created
# *Cause: Failed to open a communication channel because no communication channel has been created yet.
# *Action: Check kernel properties.

23, idERR_FATAL_idc_CHANNEL_NOT_CREATED = Unable to delete the communication channel
# *Cause: The system tried to delete an invalid communication channel.
# *Action: Please send a bug report to the vendor.

24, idERR_FATAL_idc_INVALID_CHANNEL_TYPE = Unsupported communication channel.
# *Cause: The system tried to use an unsupported communication channel.
# *Action: Please send a bug report to the vendor.

25, idERR_FATAL_idc_MUTEX_LOCK = Failed to invoke the mutex_lock() system function
# *Cause: Failed to invoke the mutex_lock() system function.
# *Action: Please send a bug report to the vendor.

26, idERR_FATAL_idc_MUTEX_UNLOCK = Failed to invoke the mutex_unlock() system function
# *Cause: Failed to invoke the mutex_unlock() system function.
# *Action: Please send a bug report to the vendor.

27, idERR_FATAL_idc_CLOSE_FAILED = Unable to close the communication channel
# *Cause: Failed to invoke the close() system function.
# *Action: Please send a bug report to the vendor.

28, idERR_FATAL_idc_SOCKET_SHUTDOWN_FAILED = Unable to shut down the communication channel
# *Cause: Failed to invoke the shutdown() system function.
# *Action: Please send a bug report to the vendor.

29, idERR_FATAL_idc_INET_SOCKET_CREATE_FAILED = Unable to create an INET socket
# *Cause: Failed to invoke the socket() function on an INET socket.
# *Action: Please send a bug report to the vendor.

30, idERR_FATAL_idc_UNIX_SOCKET_CREATE_FAILED = Unable to create a UNIX domain socket
# *Cause: Failed to invoke the socket() function on a UNIX domain socket
# *Action: Please send a bug report to the vendor.

31, idERR_FATAL_idc_SVC_INET_BIND_ERROR = Unable to bind the INET socket.(<0%d>)
# *Cause: Failed to invoke the bind() function on the INET socket because the port was already in use by another process.
# *Action: Verify that the port is not being used by another process. If it is, close that process or select a different port.

32, idERR_FATAL_idc_SVC_UNIX_BIND_ERROR = Unable to bind the UNIX domain socket
# *Cause: Failed to invoke the bind() function on a UNIX domain socket. The port was in use by another process.
# *Action: Verify that the port is not being used by another process. If it is, close that process or select a different port.

33, idERR_FATAL_idc_SVC_INET_LISTEN_ERROR = Failed to invoke the listen() system function
# *Cause: Failed to invoke the listen() function on the INET socket
# *Action: Please send a bug report to the vendor.

34, idERR_FATAL_idc_SVC_UNIX_LISTEN_ERROR = Unable to invoke the listen() function on the UNIX domain socket
# *Cause: Failed to invoke the listen() function on the UNIX domain socket
# *Action: Please send a bug report to the vendor.

35, idERR_FATAL_idc_FD_NONBLOCK_FAILED = Unable to set the socket to non-blocking mode.
# *Cause: Failed to invoke the fcntl() function to set non-blocking mode on a file descriptor.
# *Action: Please send a bug report to the vendor.

36, idERR_FATAL_idc_SYS_ACCEPT_FAILED = Unable to accept the socket
# *Cause: Failed to invoke the accept() system function
# *Action: Please send a bug report to the vendor.

37, idERR_ABORT_idc_MARSHAL_BUFFER_FULL = Insufficient communication buffer
# *Cause: The communication buffer is not large enough.
# *Action: Verify that the communication buffer size specified in the property file is greater than 64kB.

38, idERR_ABORT_idc_SOCKET_CLOSED = Socket communication error
# *Cause: The socket of the client has already been closed.
# *Action: Debug the client program.

39, idERR_ABORT_idc_ERR_FULL_IPC_CHANNEL = No remaining IPC channels (MAX=<0%d>, USED=<1%d>, BUFSIZE=<2%d>)
# *Cause: All IPC channels have been allocated to clients.
# *Action: Use the INET/UNIX Domain channel or create more IPC channels.

40, idERR_ABORT_idc_SEM_INIT_OP = Unable to invoke the semop() function as described by system semaphore properties.
# *Cause: The semaphore might have been deleted.
# *Action: Verify that the semaphore exists.

41, idERR_FATAL_idc_SYS_SELECT_FAILED = Failed to invoke the select() system function
# *Cause: Failed to invoke the select() system function.
# *Action: Please send a bug report to the vendor.

42, idERR_FATAL_WRITELINE_ERROR = Unable to communicate with the db admin
# *Cause: Failed to invoke the timed_writeline() system function.
# *Action: Please send a bug report to the vendor.

43, idERR_FATAL_SIGMASK_ERROR = Unable to mask the signal
# *Cause: Failed to invoke the pthread_sigmask() system function.
# *Action: Please send a bug report to the vendor.

44, idERR_FATAL_FILE_OPEN = Unable to open a file [<0%s>]
# *Cause: The path of the file is invalid or the storage manager does not have permission to open the file.
# *Action: Verify that the path and file name are correct and that you have read/write permission for the file.

45, idERR_FATAL_THR_NOT_CREATED_BUT_USED = No thread object was used.
# *Cause: Internal Bug
# *Action: Please send a bug report to the vendor.

46, idERR_ABORT_THR_CREATE_FAILED = Failed to create a thread object.
# *Cause: Internal Bug
# *Action: Please send a bug report to the vendor.

47, idERR_FATAL_THR_NOT_STARTED = Thread object did not start after <0%u> seconds.
# *Cause: Internal Bug
# *Action: Please send a bug report to the vendor.

48, idERR_ABORT_idc_MARSHAL_FIND_FAIL = Unable to find the specified unique marshal id.
# *Cause: Internal Bug
# *Action: Please send a bug report to the vendor.

# iduMemory
49, idERR_ABORT_IDU_MEMORY_INVALID_STATUS = An error has been detected in the memory manager.
# *Cause: Internal memory manager error.
# *Action: Set the MEMORY_ALLOCATOR_USE_PRIVATE property to 0 and restart the server.  For further information about this property, refer to the General Reference.

50, idERR_FATAL_IDU_MEMORY_ALLOCATION = Memory allocation failed
# *Cause: Insufficient memory.
# *Action: Verify that the system has enough available memory.

51,idERR_FATAL_ThrMutexInit = Failed to invoke the mutex_init() system function
# *Cause: The system failed to initialize a mutex.
# *Action: Please send a bug report to the vendor.

52,idERR_FATAL_ThrMutexDestroy = Failed to invoke the mutex_destroy() system function
# *Cause: The system failed to destroy a mutex.
# *Action: Please send a bug report to the vendor.

53, idERR_FATAL_ThrMutexLock = Failed to invoke the mutex_lock() system function
# *Cause: The system failed to lock a mutex.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

54, idERR_FATAL_ThrMutexUnlock = Failed to invoke the mutex_unlock() system function
# *Cause: The system failed to unlock a mutex.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

55,idERR_FATAL_ThrCondInit = Failed to invoke the cond_init() system function
# *Cause: The system failed to invoke the cond_init() function.
# *Action: Please send a bug report to the vendor.

56,idERR_FATAL_ThrCondSignal = Failed to invoke the cond_signal() system function
# *Cause: The system failed to invoke the cond_signal() function.
# *Action:  Please send a bug report to the vendor.

57,idERR_FATAL_ThrMutexTrylock = Failed to invoke the mutex_trylock() system function
# *Cause: Another thread has already locked the mutex.
# *Action: Please send a bug report to the vendor.

58,idERR_FATAL_ThrCondDestroy =	Failed to invoke the cond_destroy() system function
# *Cause: The system failed to remove a condition variable.
# *Action: Please send a bug report to the vendor.

59, idERR_FATAL_LongFileName = Filename too long
# *Cause: The file name is too long.
# *Action: Shorten the file name.

60, idERR_ABORT_SysCreat = Unable to invoke the create() function on [<0%s>]
# *Cause: The system failed to create the file.
# *Action: Verify that the storage manager has sufficient permission for the directory in which the file is created.

61, idERR_FATAL_SysOpen = Unable to invoke the open() function on [<0%s>]
# *Cause: The system failed to open the file.
# *Action: Verify that the file exists and that the operating system has not already opened the maximum possible number of files.

62,idERR_FATAL_SysClose = Unable to invoke the close() function on [<0%s>]
# *Cause: The system failed to close the file.
# *Action: Please send a bug report to the vendor.

63, idERR_FATAL_SysRead = Unable to invoke the read() function on [<0%s>]
# *Cause: The system failed to read the file because the disk is physically damaged or a bad sector has occurred.
# *Action: Execute the fsck command to troubleshoot and fix any filesystem errors.

64, idERR_ABORT_SysWrite = Unable to invoke the write() function on [<0%s>]
# *Cause: The system failed to write data to the file due to a physical disk failure or a full disk.
# *Action: Execute the fsck command to troubleshoot and fix any filesystem errors.

65, idERR_FATAL_Sysfstat = Unable to invoke the fstat() function on [<0%s>]
# *Cause: The system failed to call the fstat() function.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

66,idERR_FATAL_SyncError = Failed to sync a file for [<0%s>].
# *Cause: Failed to invoke the sync() function.
# *Action: Please send a bug report to the vendor.

67, idERR_ABORT_Session_Closed = The session has been closed by the server
# *Cause: The session has been closed by the server. This could be the result of a session timeout.
# *Action: Check the altibase_boot.log file or other log files. Check the properties related to session timeout.

68, idERR_ABORT_Query_Timeout = Client's query exceeded the execution time limit.
# *Cause: The query ran for too long.
# *Action: Check any properties related to session timeouts, and increase if necessary. Check the status of the system if the properties are set appropriately.

69,idERR_FATAL_idm_Sort_Table_Shortage = Sort table shortage
# *Cause: Too many children in idmModule
# *Action: Please send a bug report to the vendor.

70,idERR_FATAL_idm_Invalid_idmModule = Invalid idmModule
# *Cause: An invalid idmModule exists.
# *Action: Please send a bug report to the vendor.

71, idERR_ABORT_idm_Id_Not_Found = ID not found.
# *Cause: The ID was not found.
# *Action: Verify that the ID is valid.

72, idERR_ABORT_idm_Unable_To_Get_Attribute = Unable to get idm attributes
# *Cause: Unable to get idm attributes.
# *Action: Verify the id and attributes.

73, idERR_ABORT_idm_Unable_To_Set_Attribute = Unable to set idm attributes
# *Cause: Unable to set idm attributes.
# *Action: Verify the id and attributes.

74,idERR_FATAL_SysDirectIO = Unable to invoke the directio() function on [<0%s>].
# *Cause: The system failed to invoke the directio() function on the file
# *Action: Please send a bug report to the vendor.

75, idERR_ABORT_idp_NameNotFound = Unable to find the property name [<0%s>].
# *Cause: The specified property name is not registered in the system.
# *Action: Check the property name.

76, idERR_ABORT_idp_RangeOverflow = Property value overflow [<0%s>]
# *Cause: The specified property value exceeded the allowable range.
# *Action: Refer to the General Reference, and enter a property value that falls within the range.

77, idERR_ABORT_idp_NoSuchEntry = The entry [<1%d>] of the property [<0%s>] does not exist.
# *Cause: The specified property entry does not exist.
# *Action: Check whether the property entry exists.

78, idERR_ABORT_idp_ReadOnlyEntry = The property [<0%s>] is read-only.
# *Cause: The specified property entry cannot be modified.
# *Action: Refer to the specification of the property.

79,idERR_ABORT_idp_NotReadOnly   = The property [<0%s>] is not read-only
# *Cause: Internal Error
# *Action: Please send a bug report to the vendor.

80, idERR_ABORT_idu_NotFoundLicenseFile = License File does not exist.
# *Cause: License file does not exist.
# *Action: Install a valid license.

81, idERR_ABORT_IDU_MEMORY_ALLOCATION = Memory [<0%s>] failed.
# *Cause: Insufficient memory.
# *Action: Verify that the system has enough memory.

82, idERR_ABORT_SysOpen = Unable to invoke the open() function on [<0%s>]
# *Cause: The system failed to open the file.
# *Action: Verify that the file exists and that the operating system has not already opened the maximum possible number of files.

83,idERR_ABORT_SyncError = Failed to sync a file for [<0%s>].
# *Cause: Failed to invoke the sync() function.
# *Action: Please send a bug report to the vendor.

84,idERR_ABORT_ASYNC_IO_FAILED = AIO operation failed [<0%d>].
# *Cause:  - The asynchronous I/O operation timed out.
# *Action: - Please check the files on which operation was to occur.

85,idERR_ABORT_ASYNC_IO_READ_FAILED = AIO read operation failed [<0%d>].
# *Cause:  - The asynchronous I/O read operation failed.
# *Action: - Please check the files to be read.

86,idERR_ABORT_ASYNC_IO_WRITE_FAILED = AIO write operation failed [<0%d>].
# *Cause:   - The asynchronous I/O write operation failed.
# *Action: - Please check the files which could not be written to.

87,idERR_ABORT_ASYNC_IO_RETURN_FAILED = AIO return operation failed [<0%d>].
# *Cause:   - The asynchronous I/O aio_return() operation failed.
# *Action: - Please check the files on which operation was to occur.

88, idERR_ABORT_DISK_SPACE_EXHAUSTED = Failed to create, extend or sync a file (Name : <0%s>, Start Offset : <1%d>, Write Size : <2%d> )
# *Cause: The disk space or the user's disk quota has been exhausted.
# *Action: Increase the disk space or the user's quota for the log file, memory db file, or disk tablespace datafile.

89, idERR_ABORT_EXCEED_FILE_SIZE_LIMIT = Failed to increase size of file ( Name : <0%s>, Start Offset : <1%d>, Write Size : <2%d> )
# *Cause: An attempt to increase the data file size exceeded the file size limit of the process or the maximum file size allowed by the operating system.
# *Action: Change the system file size limit.

90, idERR_ABORT_EXCEED_OPEN_FILE_LIMIT = Failed to create file( Name : <0%s>, Start Offset : <1%d>, Write size : <2%d> )
# *Cause: The limit on the total number of open files for the system or the maximum number of file descriptors has been reached.
# *Action: Close open files that are not in use or change system properties.

91, idERR_ABORT_IDU_FILE_INVALID_PATH = Invalid directory path.
# *Cause: A corresponding directory object does not exist.
# *Action: Correct the directory object parameter or create a corresponding directory object with the CREATE DIRECTORY command.

92, idERR_ABORT_IDU_FILE_INVALID_FILEHANDLE = Invalid file handle.
# *Cause: A file handle was specified for which no corresponding open file exists.
# *Action: Verify that the file handle is a value returned from a call to FOPEN.

93, idERR_ABORT_IDU_FILE_INVALID_OPERATION = Invalid file operation.
# *Cause: An attempt was made to read from a file or directory that does not exist, or file or directory access was denied by the operating system.
# *Action: Verify the file and directory access privileges on the file system, and, if reading, verify that the file exists.

94, idERR_ABORT_IDU_FILE_DELETE_FAILED = File remove operation failed.
# *Cause: An attempt to delete a file was refused by the operating system.
# *Action: Verify that the file exists and that delete privileges have been granted for the directory and the file.

95, idERR_ABORT_IDU_FILE_RENAME_FAILED = File rename operation failed.
# *Cause: A file rename attempt was refused by the operating system, either because the source or destination directory does not exist or is inaccessible, the source file isn't accessible, or a file with that name already exists.
# *Action: Verify that the source file, source directory, and destination directory exist and are accessible, and that no file with the desired name already exists.

96, idERR_ABORT_IDU_FILE_NO_DATA_FOUND = No data found in file.
# *Cause: Cannot read any data from the file.
# *Action: Check whether the file is empty or the position has reached end-of-file.

97, idERR_ABORT_IDU_FILE_READ_ERROR = File read error.
# *Cause: An attempt to read from a file failed.
# *Action: Verify that the file exists, that it is accessible, and that it is open in read mode.

98, idERR_ABORT_IDU_FILE_WRITE_ERROR = File write error.
# *Cause: Failed to write to a file.
# *Action: Verify that the file exists, that it is accessible, and that it is open in write or append mode.

99, idERR_ABORT_MESSAGE_OVERFLOW = Message too long.
# *Cause: The message to be processed is too long.
# *Action: Make the message length shorter than 2^61.

100,idERR_FATAL_ThrCondWait = Failed to invoke the cond_wait() system function
# *Cause: The system failed to invoke the cond_wait() function.
# *Action:  Please send a bug report to the vendor.

101, idERR_FATAL_idc_UNIX_PATH_TRUNCATED = Unix domain socket path truncated.
# *Cause: A Unix domain socket path has been truncated.
# *Action: The Altibase socket path is too long. Shorten it.

102, idERR_ABORT_idp_Value_Convert_Error = The property [<0%s>] value [<1%s>] cannot be converted.
# *Cause: The data format is invalid.
# *Action: Enter a valid data format.

103, idERR_ABORT_MAX_MEM_SIZE_EXCEED = The memory size allocated for the statement has exceeded the maximum limit ( Name : <0%s>, Wanted Memory Size : <1%lu>, Max size : <2%lu> ).
# *Cause: The memory size allocated for the statement has exceeded the maximum limit.
# *Action: Increase the XXXXXXX_STMT_MEMORY_MAXIMUM property value.

104, idERR_ABORT_QUEUE_EMPTY = Queue empty
# *Cause: Queue empty
# *Action: Internal error.

105, idERR_ABORT_QUEUE_FULL = Queue full
# *Cause: Queue full
# *Action: Internal error.

106, idERR_ABORT_Query_Canceled = Query canceled by client.
# *Cause: The query has been canceled by the client.
# *Action: No action is necessary.

107, idERR_ABORT_Session_Disconnected = The session has been disconnected by the client
# *Cause: The session has been disconnected by the client.
# *Action: No action is necessary.

108, idERR_ABORT_Invalid_Profile_State = The query profile has already been activated. Turn it off first.
# *Cause: The query profile has already been activated.
# *Action: Query profiling is already enabled so no further action is required. However, if the value of the QUERY_PROF_FLAG property must be changed, first set the value to 0 and then set it to the desired value.

109, idERR_FATAL_FILE_CLOSE = Unable to close a file
# *Cause: An invalid File Handle was specified.
# *Action: Please send a bug report to the vendor.

110, idERR_IGNORE_VAR_STRING_APPEND_FORMAT_TOO_LONG = VarString appendFormat too long
# *Cause: VarString appendFormat too long.
# *Action: Internal error.

111, idERR_ABORT_BY_RECOVERY_TEST = Recovery Test Abort Error
# *Cause: The recovery test resulted in an abort error.
# *Action: Internal error.

112, idERR_FATAL_DirectIO_Invalid_Argument = Invalid argument for direct I/O write or read operation on file <0%s> (arguments: offset:<1%lu> buffer:<2%lu> size:<3%lu>)
# *Cause: An argument for the direct I/O write or read operation was invalid.
# *Action: Verify that the arguments for the direct I/O read or write operation are valid.

113, idERR_FATAL_WrongDirectIOPageSize = The page size for Direct I/O should be exactly 512, 1024, 2048, 4096, or 8192.
# *Cause: The DIRECT_IO_PAGE_SIZE property is not set to an acceptable value.
# *Action: Set the DIRECT_IO_PAGE_SIZE property to one of 512, 1024(1K), 2048(2K), 4096(4K), or 8192(8K).

114, idERR_FATAL_SysSeek = Unable to invoke the seek() function on [<0%s>]
# *Cause: The system failed to change the access position in the file because the disk is physically damaged or a bad sector has occurred.
# *Action: Execute the fsck command to troubleshoot and fix any filesystem errors.

115, idERR_ABORT_GPKI_API_Init = GPKI_API_Init error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

116, idERR_ABORT_GPKI_API_GetVersion = GPKI_API_GetVersion error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

117, idERR_ABORT_GPKI_API_Finish = GPKI_API_Finish error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

118, idERR_ABORT_GPKI_STORAGE_ReadCert = GPKI_STORAGE_ReadCert error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

119, idERR_ABORT_GPKI_STORAGE_ReadPriKey = GPKI_STORAGE_ReadPriKey error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

120, idERR_ABORT_GPKI_PRIKEY_CheckKeyPair = GPKI_PRIKEY_CheckKeyPair error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

121, idERR_ABORT_GPKI_CRYPT_GenRandom = GPKI_CRYPT_GenRandom error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

122, idERR_ABORT_GPKI_VersionMismatched = Version mismatch error
# *Cause: The GPKI module version is different from that of the client.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

123, idERR_ABORT_GPKI_BINSTR_SetData = GPKI_BINSTR_SetData error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

124, idERR_ABORT_GPKI_CMS_MakeSignedAndEnvData = GPKI_CMS_MakeSignedAndEnvData error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

125, idERR_ABORT_GPKI_CRYPT_GetKeyAndIV = GPKI_CRYPT_GetKeyAndIV error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

126, idERR_ABORT_GPKI_CMS_ProcessSignedAndEnvData = GPKI_CMS_ProcessSignedAndEnvData error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

127, idERR_ABORT_GPKI_CERT_Verify = GPKI_CERT_Verify error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

128, idERR_ABORT_GPKI_CERT_Load = GPKI_CERT_Load error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

129, idERR_ABORT_GPKI_CERT_GetSubjectName = GPKI_CERT_GetSubjectName error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

130, idERR_ABORT_GPKI_CERT_Unload = GPKI_CERT_Unload error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

131, idERR_ABORT_GPKI_ValueMismatched = Value mismatch error
# *Cause: The random value is different from that of the client.
# *Action: Check the GPKI module version.

132, idERR_ABORT_GPKI_CRYPT_Encrypt = GPKI_CRYPT_Encrypt error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

133,idERR_ABORT_GPKI_CypherTextTooLong = CypherTextTooLong error
# *Cause: The cyphertext is too long.
# *Action: Please send a bug report to the vendor.

134, idERR_ABORT_GPKI_CRYPT_Decrypt = GPKI_CRYPT_Decrypt error: [<0%d>]
# *Cause: The GPKI module failed to call the function.
# *Action: Check the error number from the altibase_boot.log file and refer to the GPKI Manual.

135,idERR_ABORT_GPKI_PlainTextTooLong = PlainTextTooLong error
# *Cause: The plaintext is too long.
# *Action: Please send a bug report to the vendor.

136, idERR_ABORT_GPKI_dlopen = dlopen error [<0%s>]
# *Cause: dlopen error.
# *Action: Check the shared library.

137, idERR_ABORT_GPKI_dlsym = dlsym error
# *Cause: dlsym error.
# *Action: Check the shared library.

138, idERR_ABORT_GPKI_dlclose = dlclose error
# *Cause: dlclose error
# *Action: Check the shared library.

139, idERR_ABORT_CannotShrinkFile = The data file size cannot be shrunk.
# *Cause: The data file size cannot be shrunk.
# *Action: Change the desired data file size setting.

140, idERR_FATAL_MmapFail = Failed to invoke the mmap() system function
# *Cause: The system failed to map the log file.
# *Action: Please send a bug report to the vendor.

141, idERR_ABORT_idu_InvalidLicense = License invalid or expired.
# *Cause: The license is invalid or has expired.
# *Action: Install a valid license.

142, idERR_ABORT_idu_UNLOAD_ERROR = [<0%s>] module unload error.
# *Cause: Dlclose error.
# *Action: Check the error number from the altibase_boot.log file and take appropriate action.

143, idERR_ABORT_MODULE_IS_NOT_LOADED = [<0%s>] module not loaded.
# *Cause: A module is not loaded.
# *Action: Check the error number from the altibase_boot.log file and take appropriate action.

144, idERR_ABORT_FINALIZE_ERROR = [<0%s>] : cannot finalize module.
# *Cause: Failed to finalize.
# *Action: Check the error number from the altibase_boot.log file and take appropriate action.

145, idERR_ABORT_CannotOpenDir = The system failed to open the directory.
# *Cause: The system failed to open the directory.
# *Action: Check the error number from the altibase_boot.log file and take appropriate action.

146, idERR_ABORT_INVALID_CHARACTER = character conversion error
# *Cause: The source data abnormally terminated in the middle of a multibyte character.
# *Action: Use a complete multibyte character.

147, idERR_ABORT_PARTIAL_MULTIBYTE_CHARACTER = partial multibyte character
# *Cause: A partial multibyte character was found at the end of the input.
# *Action: Use a complete multibyte character.

148,idERR_ABORT_BUFFER_SIZE_TOO_SMALL = buffer too small to hold the converted characters
# *Cause: The buffer is not large enough to hold the value.
# *Action: Please send a bug report to the vendor.

149, idERR_ABORT_DATA_LOSS_IN_CONVERSION = Character data loss in conversion from NCHAR to CHAR
# *Cause: When character set conversion happens between CHAR and NCHAR either implicitly or explicitly, some characters are lost due to unmapped characters in the destination character set.
# *Action: Verify that all characters can be mapped to the destination character set, or set NLS_NCHAR_CONV_EXCP to false.

150, idERR_ABORT_FILE_OPEN = Unable to open a file [<0%s>]
# *Cause: The path or filename is invalid, or the storage manager does not have permission to open the file.
# *Action: Verify that the path and filename are correct and that you have read/write permission for the file.

151, idERR_ABORT_FILE_CLOSE = Unable to close file
# *Cause: An invalid file handle was specified.
# *Action: Please send a bug report to the vendor.

152, idERR_ABORT_idp_Property_NotFound = Unable to find the property [SID=<0%s>, Name=<%1s>].
# *Cause: The specified property is not registered in the system.
# *Action: Verify that the specified property is registered in the system.

153, idERR_ABORT_idp_Value_Accept_Error = The property [<0%s>] value [<1%s>] is not acceptable.
# *Cause: The data format is invalid.
# *Action: Enter a valid data format.

154, idERR_ABORT_idp_Initialize_Error = Property initialization failed.
# *Cause: A property is set to an invalid value.
# *Action: Identify and correct the mis-configured property.

155, idERR_ABORT_SysSeek = Unable to invoke the seek() function on [<0%s>]
# *Cause: The system failed to write data to the file due to a physical disk failure or a full disk.
# *Action: Execute the fsck command to troubleshoot and fix any filesystem errors.

156, idERR_ABORT_ART = Error generated by Automatic Recovery Test(ART)
# *Cause: Automatic recovery test generated an abort error.
# *Action: You may safely ignore this error.

157, idERR_ABORT_InsufficientMemory = Insufficient memory
# *Cause: Insufficient memory.
# *Action: Verify that the system has enough available memory.

158, idERR_ABORT_InternalServerError = Internal server error.
# *Cause: An unexpected internal server error has occurred.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

159, idERR_ABORT_IDX_LIBRARY_NOT_FOUND = Library file for external procedure/function not found : <0%s><1%s><2%s>
# *Cause: The library file for the external procedure/function was not found.
# *Action: Verify that the specified library file exists in the library path.

160, idERR_ABORT_IDX_FUNCTION_NOT_FOUND = Function for external procedure/function not found
# *Cause: A user-defined function for the external procedure/function was not found in the library.
# *Action: Verify that the specified function exists in the library file.

161, idERR_ABORT_IDX_ENTRY_FUNCTION_NOT_FOUND = Entry function for external procedure/function not found
# *Cause: An entry function for the external procedure/function was not found in the library.
# *Action: Add the entry function to the library file.

162, idERR_ABORT_IDX_INVALID_PROPERTY_MANIPULATION = Invalid manipulation on a parameter property value of a parameter
# *Cause: There was an invalid manipulation of a parameter’s property value.
# *Action: Check the function in the library.

163, idERR_ABORT_IDX_AGENT_PROCESS_FAILURE = Failed to start an agent process
# *Cause: Failed to create or start an agent.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

164, idERR_ABORT_IDX_CALL_PROCEDURE_FAILURE = Failed to call external procedure/function
# *Cause: Failed to call the specified external procedure/function.
# *Action: Check the external procedure definition.

165, idERR_ABORT_IDX_AGENT_CONNECTION_FAILURE = Failed to connect to the agent : <0%s>
# *Cause: Failed to connect to the agent.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

166, idERR_ABORT_IDX_AGENT_CONNECTION_LOST = Connection to the agent has been lost
# *Cause: Connection to the agent has been lost.
# *Action: Check the trace logs of the agent.

167, idERR_IGNORE_THREAD_CREATEFAIL = Thread object cannot be created.
# *Cause: A thread object cannot be created.
# *Action: Check kernel properties.

168, idERR_IGNORE_THREADCOUNT_EXCEEDED = The number of Threads has reached to maximum count : <0%d>
# *Cause: Thread count has reached the limit.
# *Action: Increase the MAX_THREAD_COUNT property.

169, idERR_ABORT_DOUBLEFREE = Tried to free a memory block already freed
# *Cause: Thread count has reached the limit.
# *Action: Increase the MAX_THREAD_COUNT property.

170, idERR_ABORT_DISK_OR_DEVICE_BUSY = Disk or device busy ( Name : <0%s>, Start Offset : <1%d>, Write Size : <2%d> )
# *Cause: The disk space or the user's disk quota has been exhausted.
# *Action: Increase the disk space or the user's quota for the log file, memory db file, or disk tablespace datafile.

171,idERR_IGNORE_THREAD_NOTSTARTED = Joining a thread that is not started
# *Cause: Trying to join a thread that is not started.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

172,idERR_IGNORE_THREAD_UNBOUND = Joining a detached thread
# *Cause: Trying to join a thread that is not joinable
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

173,idERR_FATAL_THREAD_JOINERROR = System error while joining
# *Cause: The system failed to join a mutex.
# *Action: Check the error number from the altibase_boot.log file and take appropriate action.

174,idERR_ABORT_already_created = The shared memory region is already in use.
# *Cause: The shared memory region is already in use.
# *Action: Please change the shared memory key value to a different value.

175,idERR_ABORT_SysShmGet = Failed to invoke the shmget() system function
# *Cause: The system failed to allocate a new shared memory region due to the lack of resources on the system.
# *Action: Please adjust parameters in order to obtain sufficient shared memory resources.

176,idERR_FATAL_SysShmDt = Failed to invoke the shmdt() system function
# *Cause: The system failed to detach a shared memory region from a process.
# *Action: Please send a bug report to the vendor.

177,idERR_FATAL_Shm_No_Permission = No permission for the shared memory database
# *Cause:  A shared memory region has already been created with the same key but a different UID.
# *Action: Please remove the shared memory region, or create a shared memory region with another key.

178,idERR_ABORT_No_More_Shm_Key = Unable to find an available shared memory key.
# *Cause:  No more shared memory keys are available.
# *Action: Please remove all of the shared memory regions and try again.

179,idERR_ABORT_NoMore_SHM_Page = Insufficient memory for database
# *Cause: The system failed to increase the amount of memory because the database was created in a shared memory region.
# *Action: Please shut down the database and make sure that the system has sufficient memory using xdbshmutil.

180,idERR_ABORT_Shm_Max_Size = A shared memory size consumed excceds the size than expected.
# *Cause:  The shared memory size property value is less than the size which the process needs.
# *Action: Please increase the size of the shared memory property.

181,idERR_FATAL_PROCESS_CHECK_ALIVE = Unable to check if a process is alive.
# *Cause: Failed to invoke the semctl() function
# *Action: Please verify that you have permission to access the semaphore and send a bug report to the vendor.
#  Server Internal Message

182,idERR_FATAL_SEM_DELETE = Unable to delete the semaphore.
# *Cause: Failed to invoke the semctl() function
# *Action: Please verify that you have permission to access the semaphore and send a bug report to the vendor.

183,idERR_FATAL_SEM_GET = Unable to create or get a semaphore.
# *Cause: Failed to invoke the semop() function
# *Action: Please verify the error number and make sure that the semaphore group exists.

184,idERR_FATAL_Systhrjoin = Failed to invoke the thr_join() system function
# *Cause: The system failed to join threads.
# *Action: Please send a bug report to the vendor.

185,idERR_FATAL_SYSTEM_PROCESS_ALREADY_STARTED = [0%s] Process has been already started.
# *Cause: System process which is one of Deamon or WServer has been started before this operation.
# *Action: Check the other system process is alive or started.

186,idERR_FATAL_UNDO_THREAD_BY_WATCH_DOG = The undo operation of a thread in a dead process is failed.
# *Cause: There is an unexpected error while doing an undo operation of a thread.
# *Action: Please send a bug report to the vendor.

187,idERR_FATAL_WATCHDOG_THEAD_ABNORMAL_EXIT = The watchdog thread faced an unexpected error.
# *Cause: There is an unexpected error in the watchdog thread.
# *Action: Please send a bug report to the vendor.

188,idERR_FATAL_UNCOMPATIBLE_SHARED_MEMORY = The version of the shared memory is not compatible with the version of the shared memory manager. Shared Memory Version => [ Version ID = <0%s> ] Shared Memory Manager=>[ Version ID = <1%s> ]
# *Cause: The shared memory created before is no compatitle with the current shared memory manager.
# *Action: Please remove shared memory created by Altibase and restart Altibase.

189,idERR_FATAL_UNDO_PROCESS_BY_WATCH_DOG = The undo operation for a dead process is failed.
# *Cause: There is an unexpected error in the watchdog undo thread for a dead process.
# *Action: Please send a bug report to the vendor.

190,idERR_FATAL_ATTACH_SHARED_MEMORY_MGR = The attach operation for a shared memory is failed.
# *Cause: There is an unexpected error while doing a attach operation of a shared memory.
# *Action: Please send a bug report to the vendor.

191,idERR_FATAL_INVALID_SHARED_MEMORY_BLOCK_SIZE = The shared memory block size[0%u] is invalid.
# *Cause: The shared memory block size is invalid.
# *Action: Please send a bug report to the vendor.

192,idERR_ABORT_REQUEST_PROCESS_FAILURE = The process to deal with a request asked by the current process is dead.
# *Cause: The process to deal with a request is dead.
# *Action: Please send a bug report to the vendor.

193,idERR_ABORT_LATCH_TIMEOUT = Timeout happens while trying to do latch.
# *Cause: There is a thread or process acquiring a latch for long time.
# *Action: Check which a process or thread get a latch for a long time.

194,idERR_FATAL_INVALID_SHM_SEGCOUNT = The max number of shared memory segments is invalid.
# *Cause: The maximum of the segment count is 0.
# *Action: Check SHM_MAX_SIZE and SHM_CHUNK_SIZE since the number of the max shared memory segments is calculated by SHM_MAX_SIZE / SHM_CHUNK_SIZE.

195,idERR_FATAL_INVALID_ELEMENT_SIZE = The number of elements in a chunk of a mempool is invalid.
# *Cause: The number of elements in a chunk of a mempool is 0.
# *Action: Please send a bug report to the vendor.

196,idERR_IGNORE_FAIL_OPEN_ISQL_PTY_FILE = can not to open a file that includes the path of pty.
# *Cause: Failed to can not to open a file that includes the path of xdbisql pty.
# *Action: This error does not originate with Altibase.

197,idERR_FATAL_REGISTER_NEW_PROCESS_DISABLED = A process can not access a database in a shared memory.
# *Cause: Database is a invalid state to be accessed by a process.
# *Action: Check the system process is shutting down or the state of a shared memory area.

198,idERR_IGNORE_FAIL_COMMIT_TRANS_DUE_TO_MAX_ROW = Transaction faces a error while committing.
# *Cause: The table has more rows than the max row limitation.
# *Action: Check the table's max rows.

199,idERR_FATAL_REMOVE_SYSTEM_SHARED_MEMORY_SEGMENT = The remove operation of a system shared memory segment is failed.
# *Cause: There is a process to attach a system shared memory segment.
# *Action: Check if there is a process to attach a system shared memory segment.

200,idERR_FATAL_Invalid_SHM_DB_KEY = When the SHM_DB_KEY value is 0, a shared memory database cannot be used. Set the SHM_DB_KEY property.
# *Cause:  SHM_DB_KEY value is 0.
# *Action: Please set the SHM_DB_KEY property.

201,idERR_ABORT_Invalid_Process_ID = The process ID is invalid.
# *Cause:  There is no process in XDB processes.
# *Action: Please check if the process ID is valid.

202,idERR_ABORT_Invalid_Thread_ID = The thread ID is invalid.
# *Cause:  There is no thread in XDB processes.
# *Action: Please check if the thread ID is valid.

203,idERR_FATAL_NOT_OWNER_OF_SHARED_MEMORY = Try to access a shared memory chunk which is not owned: SHMKEY is <0%d>.
# *Cause:  Try to access a shared memory chunk which is not owned.
# *Action: Please check if there is already created shared memory associated with SHM_DB_KEY.

204,idERR_FATAL_CREATE_SYSTEM_SEGMENT = Cannot create the system segment.
# *Cause:  Unavailable system resource.
# *Action: Please check your system.

205,idERR_ABORT_SET_LATCH_STACK_OVERFLOW = Set latch stack overflow happens while trying to lock table.
# *Cause: There are too many table locks acquired in a transaction.
# *Action: Reduce a table count referenced in a transaction (Maximum table count in a transaction is 128 in LOCK_MGR_TYPE=1).

206,idERR_ABORT_IDU_MEMORY_DEALLOCATION = Memory deallocation failed.
# *Cause:  - System call failed.
# *Action: - Please send a bug report to the vendor.

207, idERR_ABORT_IDX_AGENT_CONNECTION_LOST_WHILE_RECV = Connection to the agent has been lost while receiving (step : <0%d>)
# *Cause: Connection to the agent has been lost while receiving.
# *Action: Check the trace logs of the agent.

208, idERR_ABORT_IDX_AGENT_CONNECTION_LOST_WHILE_SEND = Connection to the agent has been lost while sending (step : <0%d>)
# *Cause: Connection to the agent has been lost while sending.
# *Action: Check the trace logs of the agent.

209, idERR_ABORT_IDX_AGENT_CONNECTION_LOST_BY_PEER = Connection to the agent has been lost by peer (step : <0%d>, Read Count : <1%d>, Buffer Size : <2%d>)
# *Cause: Connection to the agent has been lost by peer.
# *Action: Check the trace logs of the agent.

210, idERR_FATAL_CHANGE_OWNER_OF_SHARED_MEMORY_SEGMENT = Cannot change the owner of the shared memory segment.
# *Cause: Failed to invoke the shmctl() function.
# *Action: Verify that you have permission to access shared memory. Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

211,idERR_ABORT_IDU_SHM_INCOMPATIBLE_CONFIG = Incompatible changes to the shared memory configuration have been detected.  Previously => [ SHM Chunk Size = <0%u>, SHM Max Chunk Count = <1%lu> ]  Now => [ SHM Chunk Size = <2%u>, SHM Max Chunk Count = <3%lu> ]
# *Cause: Some of the shared memory configuration have been changed and the changes require the database to be reloaded into the shared memory.
# *Action: Clear the shared memory by issuing xdbshmutil -e command and start the server again.

212, idERR_ABORT_INVALID_ENCRYPTED_TEXT = Malformed or corrupted wrapped unit.
# *Cause: The format of the wrapped unit being compiled is not understood by the compiler. This may be because the unit was edited or modified after it was wrapped.
# *Action: Rewrap the unit.

213, idERR_ABORT_PERSSYS_CREATE_FAILED = Could not create persistent system chunk.
# *Cause: System chunk of persistent memory manager could not be created.
# *Action: Cleanup shared memory and try again, or change system chunk key.

214, idERR_ABORT_PERSSYS_MAP_FAILED = Could not map persistent system chunk.
# *Cause: System chunk of persistent memory manager could not be mapped.
# *Action: Cleanup shared memory and try again, or change system chunk key.

215, idERR_ABORT_PERS_SHM_FULL = Amount of shared memory reached system limit.
# *Cause: Amount of shared memory reached system limit.
# *Action: Cleanup shared memory, or modify kernel parameters to increase shared memory limit.

216, idERR_ABORT_PERS_CLEANUP_FAILED = Could not cleanup shared memory chunks.
# *Cause: Shared memory cleanup failed.
# *Action: Remove persistent system chunk manually.

217, idERR_ABORT_PERS_ATTACH_FAILED = Could not attach shared memory chunk <0%d>(<1%d>,<2%d>) : <3%d>
# *Cause: Shared memory attach data chunk failed.
# *Action: Remove persistent system chunk and try again.

218, idERR_ABORT_PERS_INVALID_CHUNK = Chunk <0%d> is invalid.
# *Cause: Shared memory attach data chunk failed.
# *Action: Remove persistent system chunk and try again.

219, idERR_ABORT_PERS_INVALID_ADDRESS = Could not access [<0%d>][<1%x>], which is not in [<2%d>][<3%x>]
# *Cause: Invalid access of memory
# *Action: Remove persistent system chunk and try again.

220, idERR_ABORT_PERSSYS_NOT_MINE = System chunk was created by other user.
# *Cause: Other user has created a chunk with SHM_PERS_KEY
# *Action: Change the property SHM_PERS_KEY and try again.

221, idERR_ABORT_FILENAME_TOO_LONG = The filename is too long [<0%s>]
# *Cause: The filename is too long.
# *Action: Shorten the filename.

222, idERR_ABORT_FILE_ALREADY_EXIST = The file already exists [<0%s>]
# *Cause: The file already exists.
# *Action: Try another filename.

223, idERR_ABORT_INVALID_ACCESS = Access denied [<0%s>]
# *Cause: The file cannot be accessed.
# *Action: Verify the permission of the file, or the parent directory.

224, idERR_ABORT_NO_SUCH_FILE = No such file exists [<0%s>]
# *Cause: The file does not exist.
# *Action: Verify that the path and the filename are correct.

225, idERR_ABORT_WRONG_Profile_Flag = The value of QUERY_PROF_FLAG property exceeds the maximum value(<0%d>).
# *Cause: The current value of QUERY_PROF_FLAG property exceeds the maximum value 63.
# *Action: Modify the value of QUERY_PROF_FLAG property to be less than or equal to 63.

226, idERR_IGNORE_NO_SUCH_CPUINFO = Unable to retrieve CPU information (errno:<0%d>). 
# *Cause: CPU affinity configuration is not found.
# *Action: Verify the error number in the trace log and contact Altibase Support Center (http://support.altibase.com). 

227, idERR_IGNORE_EXCEED_NUMA_NODE = The NUMA node number [<0%d>] exceeds the maximum value [<1%d>].    
# *Cause: The NUMA node number exceeds the maximum value.
# *Action: Verify the NUMA node number and valid range in the trace log and contact Altibase Support Center. (http://support.altibase.com). 

228, idERR_IGNORE_EXCEED_CPUNO = The CPU number [<0%d>]/[<1%d>] exceeded the supported maximum value [<2%d>]/[<3%d>].
# *Cause: The CPU number exceeds the maximum value. 
# *Action: Verify the CPU number and valid range in the trace log and contact Altibase Support Center. (http://support.altibase.com). 

229, idERR_IGNORE_INVALID_CPUNO = Invalid CPU number [<0%d>].  The Valid range is 0 to <1%d>.
# *Cause: The CPU number is invalid.
# *Action: Verify the CPU number and valid range in the trace log and contact Altibase Support Center (http://support.altibase.com).

230, idERR_IGNORE_INVALID_NUMA_NODE_NO = Invalid NUMA node number [<0%d>]. The Valid range is 0 to <1%d>.
# *Cause: The NUMA node number is invalid.
# *Action: Verify the NUMA node number and valid range in the trace log and contact Altibase Support Center (http://support.altibase.com).

231, idERR_IGNORE_BINDTHREAD_FAILED = Unable to set CPU affinity (errno:<0%d>). 
# *Cause: The system function failed to bind cpusets on a thread. 
# *Action: Verify the error number in the trace log and contact Altibase Support Center (http://support.altibase.com). 

232, idERR_IGNORE_BIND_PROCESS_FAILED = Failed to set CPU affinity on a process (errno:<0%d>). 
# *Cause: The system function failed to bind cpusets on a process. 
# *Action: Verify the error number in the trace log and contact Altibase Support Center (http://support.altibase.com). 

233, idERR_IGNORE_UNBINDTHREAD_FAILED = Failed to unset CPU affinity on a thread.
# *Cause: The CPU number is invalid.
# *Action: This message can be disregarded. 

234, idERR_IGNORE_UNBIND_PROCESS_FAILED = Failed to unset CPU affinity on a process.
# *Cause: The CPU number is invalid. 
# *Action: This message can be disregarded.

235, idERR_ABORT_NOT_SUPPORT_FALLOCATE = The operation is not supported by the filesystem(or kernel) (<0%s>)
# *Cause: The filesystem(or kernel) containing the file does not support this operation.
# *Action: Set the LOG_CREATE_METHOD property value to 0 and restart the server.
 
236, idERR_ABORT_Sysfallocate = Unable to invoke the fallocate() function on [<0%s>].
# *Cause: The system failed to call the fallocate() function.
# *Action: Set the LOG_CREATE_METHOD property value to 0 and restart the server.
#
237, idERR_ABORT_InternalServerErrorWithString = Internal server error ( <0%s> ).
# *Cause: An unexpected internal server error has occurred.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).


238, idERR_ABORT_IDU_MEMORY_INVALID_SET_STATUS = An error has been detected in the memory manager. [<0%s>]
# *Cause: Internal memory manager error.
# *Action: Set the MEMORY_ALLOCATOR_USE_PRIVATE property to 0 and restart the server. Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

###############################################################################
#  Server Internal Message
###############################################################################

INTERNAL_TRACE_MESSAGE_BEGIN

ID_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
            "signed long =<%ld>\n unsigned long = <%lu>\n"
            "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>";

ID_TRC_MSB_NOT_FOUND = "<%s> MSB File Not Found \n";

ID_TRC_MSB_VERSION_INVALID = "Invalid version found in this msb file: <%s> ";

ID_TRC_PROPERTY_STATIC_INIT_ERROR = "Prorpty Read Error in initializeStaticModule()"
                                    " : i = <%u>";

ID_TRC_MEMORY_THRESHOLD_OVERFLOW = "[ID_DEBUG] Required memory "
                                   "over threshold [<%ld>] !!";

ID_TRC_PROPERTY_TYPE_INVALID = "[WARNING] Property[<%s>] Value[<%s>] was invalid."
                               " Default value was used.";

ID_TRC_AIO_FAILED = "AIO Error Occurred!!(errno=<%u>)";

ID_TRC_ASSERT_BEGIN = "IDE_ASSERT(<%s>)[<%s>:<%u>] \n"
                      "** STACK DUMP BEGIN **\n";
ID_TRC_ASSERT_END   = "** STACK DUMP END **n";

ID_TRC_CREATE_FAILED = "WARNING : create error number : <%u>, "
                      "file name : <%s>\n";

ID_TRC_WRITE_FAILED = "WARNING : write error number : <%u>, "
                        "file name : <%s>, "
                        "file handle : <%u>, "
                        "intended size : <%u>, "
                        "written size : <%u>\n";

ID_TRC_SERVER_ABNORMAL_SHUTDOWN = "Abnormal System Shutdown!!\n"
                                  "Error: \n\n"
                                  "File Size Increase Failed...\n"
                                  "Check Disk Space !!\n"
                                  "<%s>\n";
ID_TRC_FILE_EXTEND_FAILED = "File Size Increase Failed. : "
                            "The disk space has been exhausted.\n"
                            "Error : <%s>\n";

ID_TRC_FILE_SYNC_FAILED = "WARNING : sync error number : <%u>, "
                          "file name : <%s>, "
                          "file handle : <%u>\n";

ID_TRC_LATCH_FAILED = "Odd Situation...!! w=<%ld> r=<%ld>\n";

ID_TRC_MEMLIST_MISMATCH_FREE_SLOT_COUNT =  "Free Count Mismatch Free:<%u>, Max:<%u> \n";
ID_TRC_MEMLIST_WRONG_FREE_SLOT_COUNT  =  "Wrong Free Slot Count  Free:<%u>, Max:<%u> \n";

ID_TRC_MEMLIST_MISMATCH_ALLOC_SIZE =  "Free Size Mismatch. real size:<%lu>, free size:<%lu> \n";

ID_TRC_MEMLIST_WRONG_FULL_CHUNK_COUNT  = "WARNING !!! , Full Count is not zero --> <%d> \n";
ID_TRC_MEMLIST_WRONG_PARTIAL_CHUNK_COUNT  = "WARNING !!! , Partial Count is not zero --> <%d> \n";

ID_TRC_MEMMGR_LEAK_TITLE = "List of Memory Blocks to Be Released\n";
ID_TRC_MEMMGR_LEAK_HEAD  = "-----------------------------------\n"
                           " NAME                             | SIZE             | COUNT\n";
ID_TRC_MEMMGR_LEAK_BODY  = " <%s> | <%lu> | <%lu>\n";
ID_TRC_MEMMGR_LEAK_TAIL  = "------------------------------------ SUM | <%lu> | <%lu>\n";
ID_TRC_MEMMGR_NO_LEAK    = "All memory blocks have been freed!\n";

ID_TRC_MEMMGR_ALLOC_FAILED = "[Warning] Memory allocation failed. Size:"
                             "<%lu> Timeout: <%lu>\n";

ID_TRC_MEMMGR_REQUEST_ACK =  "[Information] Memory allocation request"
                             " Module: <%s>"
                             " Size: <%lu>"
                             " Timeout: <%lu>\n";

ID_TRC_MEMMGR_EXCEED_HEAPMEM_MAXSIZE = "[Warning] Memory allocation failed."
                             "(memory usage exceeds __HEAP_MEM_MAX_SIZE)\n";

ID_TRC_MUTEXMGR_LEAK_HEAD = "List of Mutexes to Be Released\n";
ID_TRC_MUTEXMGR_LEAK_BODY = " <%u> : <%s>\n";
ID_TRC_MUTEXMGR_LEAK_TAIL_LEAK = " <%d> mutex(es) remained\n";
ID_TRC_MUTEXMGR_LEAK_TAIL_CLEAN = " No mutex remained.\n";

ID_TRC_LATCH_LEAK_HEAD = "List of Latches to Be Released\n";
ID_TRC_LATCH_LEAK_BODY = " <%d> : <%s>\n";
ID_TRC_LATCH_LEAK_TAIL_LEAK = " <%d> latch(es) remained\n";
ID_TRC_LATCH_LEAK_TAIL_CLEAN = " No latch remained.\n";

ID_TRC_HEAP_INSPECTION_ACK = "INSPECTION_LARGE_HEAP_THRESHOLD has changed"
                            " [<%d>] --> [<%d>] !!\n";

ID_TRC_INVALID_STAT_SEQ =  "[FATAL] Invalid StatName Sequence..<%d> \n";

ID_TRC_INVALID_WAIT_SEQ =  "[FATAL] Invalid WaitName Sequence..<%d> \n";

ID_TRC_MRECOVERY_FILE_MMAP_ERROR = "Failed to invoke the mmap() system function. (errno:<%u>): Retrying...\n";

ID_TRC_INVALID_LATCH_PAGE_IDX = "[Warning] Invalid Latch Page Type IDX: <%u>\n";
ID_TRC_CALLOC_FAIL="[Warning] calloc fail !!";
ID_TRC_MALLOC_FAIL="[Warning] malloc fail !!";
ID_TRC_CALLOC_RETRY_FAIL="[Warning] calloc retry-fail !!";

ID_TRC_TIMER_RUNNING_LEVEL_EXCEPTION = "[Warning] Because a TIMER_RUNNING_LEVEL of 3 is not supported on this platform, "
                                       "it has been set to the default(=<%d>) for this platform.\n";

ID_TRC_MEMMGR_ALLOC_REALTIME_FAILED = "[Warning] Memory allocation via allocator(type=<%lu> "
                                      "addr=<%lu> failed. Size:<%lu> Timeout:<%lu>\n";

ID_TRC_MEMMGR_ALLOC_PRIVATE_FAILED = "[Warning] Memory allocation via private allocator(type=<%lu> "
                                      "addr=<%lu>) failed. Size:<%lu> Timeout:<%lu>\n";

ID_TRC_MEMMGR_INVALID_ALLOC_INDEX = "[Warning] Invalid allocator index <%u> (must be less than <%u>) "
                                    "in block at <%lu>\n";

ID_TRC_MEMMGR_INVALID_ALLOC_PRIVATE = "[Warning] Memory deallocation of addr=<%lu> via private allocator"
                                      "(addr=<%lu>) failed\n";

ID_TRC_MEMMGR_INVALID_ALLOC = "[Warning] Memory deallocation of addr=<%lu> via allocator(addr=<%ld>) failed\n";
                                    
ID_TRC_MEMMGR_FREE_FAILED = "[Warning] Freeing memory at <%lu> failed.\n";

ID_TRC_MEMMGR_ALLOCATOR_LEAK_HEAD = "List of Memory Allocators to Be Released\n"
                          "----------------------------------- SIZE | COUNT\n";
ID_TRC_MEMMGR_ALLOCATOR_LEAK_BODY = "<%lu> | <%lu>\n";
ID_TRC_MEMMGR_ALLOCATOR_LEAK_TAIL = "------------------------------------ SUM | <%lu> | <%lu>\n";

# PROJ-1685
ID_TRC_EXTPROC_PROC_CREATED = "Agent process for external procedure has been created. (PID : <%d>)\n";
ID_TRC_EXTPROC_PROC_RECREATED = "Agent process for external procedure has been re-created. (PID : <%d>)\n";
ID_TRC_EXTPROC_PROC_TERMINATED = "Agent process for external procedure has been terminated. (PID : <%d>)\n";

# PROJ-2473 SNMP
ID_TRC_SNMP_TRAP_MSG = "[SNMP_TRAP]\n"
                       "Address  = <%s>\n"
                       "Level    = <%u>\n"
                       "Code     = <%u>\n"
                       "Message  = <%s>\n"
                       "MoreInfo = <%s>";

# BUG-44270
ID_TRC_MEMMGR_RETRY_ALLOC = "[Warning] Memory <%s> failed, retry.\n";

INTERNAL_TRACE_MESSAGE_END
 
