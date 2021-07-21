# $Id$
#
# SM Error Message File (used by genErrMsg)
#

# 에러코드 영역 번호
# 0 => sm
# 1 => qp
# 2 => cm
# 3 => main
# 4 => id
# 5 => rp

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.
# Last Number = 442

SECTION = 1

0,smERR_ABORT_already_created      = The shared memory region is already in use.
# *Cause: The shared memory region is already in use.
# *Action: Change the shared memory key value to a different value.

1,smERR_ABORT_NoMore_SHM_Page      = Insufficient memory for database
# *Cause: The system failed to increase the amount of memory because the database was created in a shared memory region.
# *Action: Shut down the database and verify that the system has sufficient memory using shmutil.

2, smERR_FATAL_FileDelete =	    Unable to delete <0%s> file
# *Cause: The system failed to unlink the file.
# *Action: Terminate unnecessary processes.

3, smERR_ABORT_FileDelete =	    Unable to delete <0%s> file
# *Cause: The system failed to unlink the file.
# *Action: Terminate unnecessary processes.

#4, smERR_FATAL_NoMoreMemory =      Insufficient memory
# *Cause: The system failed to allocate sufficient memory.
# *Action: Terminate unnecessary processes.

#5, smERR_FATAL_MmapFail =         Failed to invoke the mmap() system function
# *Cause: Insufficient system resources.
# *Action: Verify that there are enough system resources. Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

6, smERR_FATAL_MunmapFail =       Failed to invoke the unmap() system function
# *Cause: Insufficient system resources.
# *Action: Verify that there are enough system resources. Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

7, smERR_IGNORE_SyncFail =        Failed to invoke the msync() system function
# *Cause: The system failed to synchronize a file with mapped memory.
# *Action: Ignore this error message.

8, smERR_ABORT_SysShmGet = 			Failed to invoke the shmget() system function
# *Cause: The system failed to allocate a new shared memory region due to the lack of resources on the system.
# *Action: Adjust parameters in order to obtain sufficient shared memory resources.

9, smERR_ABORT_SysShmAt = 			    Failed to invoke the shmat() system function
# *Cause: The system failed to attach a shared memory region to a process.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

10, smERR_FATAL_SysShmDt = 			    Failed to invoke the shmdt() system function
# *Cause: The system failed to detach a shared memory region from a process.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

11, smERR_FATAL_SysShmCtl = 			Failed to invoke the shmctl() system function
# *Cause: The system failed to remove shared memory.
# *Action: Change kernel parameters related to shared memory.

12, smERR_FATAL_Systhrjoin =			Failed to invoke the thr_join() system function
# *Cause: The system failed to join threads.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

13,smERR_FATAL_ThrMutexInit =			Unable to initialize a mutex.
# *Cause: The system failed to initialize a mutex.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

14,smERR_FATAL_ThrMutexDestroy =		Failed to invoke the mutex_destroy() system function
# *Cause: The system failed to remove a mutex.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

15,smERR_FATAL_ThrMutexLock =			Failed to invoke the mutex_lock() system function
# *Cause: The system failed to lock a mutex.
# *Action : Check the error number from the trace log and contact Altibase’s SupportCenter (http://support.altibase.com).

16,smERR_FATAL_ThrMutexUnlock =	 	Failed to invoke the mutex_unlock() system function
# *Cause: The system failed to unlock a mutex.
# *Action : Check the error number from the trace log and contact Altibase’s SupportCenter (http://support.altibase.com).

17,smERR_FATAL_ThrCondInit =		 Failed to invoke the cond_init() system function
# *Cause: The system failed to initialize a condition variable.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

18,smERR_FATAL_ThrCondSignal =	 	Failed to invoke the cond_signal() system function
# *Cause: The system failed to signal using a condition variable.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com)

19,smERR_FATAL_ThrCondDestroy =		Failed to invoke the cond_destroy() system function
# *Cause: The system failed to remove a condition variable.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

20,smERR_FATAL_ThrCondWait =		Failed to invoke the cond_wait() system function
# *Cause: Invalid condition value or invalid mutex.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

#21,smERR_ABORT_commitTSS=  Unable to set commit SCN into TSS slot or unable to add TSS slot to the committed TSS list.
# *Cause: There is a problem with the TSS segment.
# *Action: No action is needed because the current transaction has been rolled back.

22,smERR_ABORT_NotFoundSavepoint =             Savepoint not found
# *Cause: There is no such savepoint in the transaction.
# *Action: Verify that the use of the savepoint is correct.

#23,smERR_FATAL_NoMoreMem =			Insufficient memory
# *Cause: Insufficient memory
# *Action: Terminate unnecessary processes or increase the amount of memory.

24,smERR_ABORT_BACKUP_DISK_INVALID =      The version of data file for backup is not compatible with the version of storage manager. Backup DB => [ Version ID = <0%s>,  Bit = <1%d>, Endian = <2%s> LogSize = <3%lu>  Transaction Table Size = <4%d> ]  Server=>[ Version ID = <5%s>,  Bit = <6%d>, Endian = <7%s> LogSize = <8%lu> Transaction Table Size = <9%d> ]
# *Cause: Database data files are not backwards compatible.
# *Action: Import or export the database, or use a previous version of the storage manager that is compatible with the data file.

25,smERR_FATAL_SysRead = 			Failed to invoke the read() system function
# *Cause: The system failed to read from the file.
# *Action: Check the file system.

#26,smERR_FATAL_Sysfstat = 			Failed to invoke the fstat() system function
# *Cause: The system failed to invoke the fstat() function.
# *Action: Check the file system.

#27,smERR_ABORT_InsufficientBufferPool = Buffer pool not sufficient to get a free page.
# *Cause: The system cannot get a free page.
# *Action: Increase the buffer pool size.

28,smERR_FATAL_PageCorrupted = A page is corrupt. ( Current Space ID : <0%d>, Current Page ID : <1%d>, The corresponding page has been dumped to "$ALTIBASE_HOME/trc/altibase_dump.log". )
# *Cause: The page was not completely written.
# *Action: Recover the tablespace that contains the corrupt page using backup and recovery utilities.

#29,smERR_ABORT_ExceedBufferPoolMaxSize = The buffer pool size exceeds the maximum size. ( Buffer Pool Max Size : <0%d>, Wanted Buffer Pool Size : <1%d>, Current Buffer Size : <2%d> )
# *Cause: The buffer pool size exceeds the maximum size.
# *Action: Set the buffer pool size so that it is less than or equal to the maximum buffer pool size.

#30,smERR_FATAL_DWBufferHdrPageCorrupted = The DW buffer header has not been created yet, or its page is corrupt. ( DW Buffer Space ID : <0%d>, DW Buffer Hdr Page ID : <1%d> )
# *Cause: DW buffer manager initialization failed because the DW buffer header was corrupt.
# *Action: The DW buffer header page must be recovered.

31,smERR_ABORT_InvalidAutoExtFileSize = The MAXSIZE of the data file cannot be less than its current size. ( Request Max Size : <0%lu> pages, Current Size : <1%lu> pages )
# *Cause: The MAXSIZE of the data file is less than the current size of the data file.
# *Action: Set the MAXSIZE of the data file appropriately.

32,smERR_ABORT_InitExceedMaxFileSize = The INITSIZE of the data file exceeds the maximum file size. ( Request Init Size : <0%lu> pages, Maximum File Size : <1%lu> pages )
# *Cause: The INITSIZE of the data file exceeds the maximum file size.
# *Action: Set the INITSIZE of the data file correctly.

#33,smERR_ABORT_CurrExceedMaxFileSize = The CURRSIZE of the data file exceeds the maximum file size. ( Request Cur Size : <0%lu> pages, Maximum File Size : <1%lu> pages )
# *Cause: The CURRSIZE of the data file exceeds the maximum file size.
# *Action: Please send a bug report to the vendor.

34,smERR_ABORT_MaxExceedMaxFileSize = The MAXSIZE of the data file exceeds the maximum file size. ( Request Max Size : <0%lu> pages, Maximum File Size : <1%lu> pages )
# *Cause: The MAXSIZE of the data file exceeds the maximum file size.
# *Action: Set the MAXSIZE of the data file correctly.

35,smERR_ABORT_InvalidFilePathABS = The data file path is not an absolute path.
# *Cause: The data file path must be an absolute path.
# *Action: Check the environment variable ALTIBASE_HOME for this account.

36,smERR_ABORT_InvalidFilePathKeyWord = The data file path contains special keywords or reserved system keywords.
# *Cause: The data file path contains special keywords or reserved system keywords.
# *Action: Verify that the data file path does not contain special keywords or reserved system keywords.

37,smERR_ABORT_AlreadyExistFile = The data file already exists ( File Name : <0%s> ).
# *Cause: The data file already exists.
# *Action: Add the 'REUSE' clause to the CREATE statement, or remove the existing datafile.

#38,smERR_ABORT_CannotShrinkFile = The data file size cannot be decreased.
# *Cause: The data file size cannot be decreased.
# *Action: Please set the desired size of the data file again.

39,smERR_ABORT_NotExistFile = The data file does not exist.
# *Cause: The data file does not exist.
# *Action: Verify that the data file exists.

40,smERR_ABORT_NoReadPermFile = The path[ <0%s> ] does not pass the test for read permission.
# *Cause: The path does not pass the test for read permission.
# *Action: Grant read permission for the path.

41,smERR_ABORT_NoWritePermFile = The path[ <0%s> ] does not pass the test for write permission.
# *Cause: The path does not pass the test for write permission.
# *Action: Grant write permission for the path.

42,smERR_ABORT_NotFoundTableSpaceNodeByName = Tablespace node not found ( Name : <0%s> )
# *Cause: The tablespace was not found.
# *Action: Verify that the desired tablespace exists.

43,smERR_ABORT_NotFoundTableSpaceNode = Tablespace node not found ( ID : <0%d> )
# *Cause: The tablespace was not found.
# *Action: Verify that the desired tablespace exists.

44,smERR_ABORT_MustBeDataFileOnlineMode = The data file node must be in online mode (FID:<0%d>).
# *Cause: The data file node must be in online mode.
# *Action: Change the data file node to online mode and try again.

#45,smERR_ABORT_MustBeTableSpaceOnlineMode = The tablespace node must be in online mode (Name:<0%s>).
# *Cause: The tablespace node must be in online mode.
# *Action: Change the tablespace node to online mode and try again.

#46,smERR_ABORT_MustBeTableSpaceOfflineMode = The tablespace node must be in offline mode (Name:<0%s>).
# *Cause: The tablespace node must be in offline mode.
# *Action: Change the tablespace node to offline mode and try again.

#47,smERR_ABORT_InvalidShrinkFileSize = The used part of the data file cannot be truncated (<0%d> pages).
# *Cause: The utilized portion of a datafile cannot be truncated.
# *Action: Resize the datafile to a size that would not truncate the datafile.

48,smERR_ABORT_InvalidExtendFileSize = The data file cannot be increased in size because the requested size is bigger than the maximum size. ( Request Size : <0%lu> pages, Maximum File size : <1%lu> pages )
# *Cause: The datafile cannot be resized to a value that exceeds its maximum size.
# *Action: Resize the datafile to a size that does not exceed its maximum limit.

49,smERR_ABORT_NotEnoughTableSpaceID = Unable to create tablespace. (current greatest tablespace ID : <0%d>)
# *Cause: The number of tablespaces has reached the maximum.
# *Action: Use an existing tablespace or rebuild the database.

50,smERR_ABORT_AlreadySetAutoExtendMode = The attribute of AUTOEXTEND mode has already been set. (FID :<0%d>)
# *Cause: The AUTOEXTEND mode has already been set.
# *Action: No action is necessary.

51,smERR_ABORT_forbiddenOpWhileBackup =  <0%s> cannot operate while a tablespace is in the process of being backed up.
# *Cause: A tablespace is currently being backed up.
# *Action: Wait until the tablespace backup process finishes.

52,smERR_ABORT_NotFoundDataFileNode = Data file node not found ( <0%s> )
# *Cause: The data file node was not found.
# *Action: Verify that the data file exists.

53,smERR_ABORT_NotEnoughFreeSpace = The tablespace does not have enough free space ( TBS Name :<0%s>, Type : <1%d>, Used Page Limit : <2%d> ).
# *Cause: The tablespace does not have enough free space.
# *Action: Add a new data file.

54,smERR_ABORT_CannotRemoveDataFileNode = The data file is in use.
# *Cause: The data file is in use.
# *Action: No action is necessary.

55,smERR_ABORT_CannotDropTableSpace= Unable to drop system-related tablespaces.
# *Cause: SYSTEM, UNDO, and SYSTEM TEMP tablespaces cannot be dropped.
# *Action: Verify that the tablespace is not a system tablespace.

#56,smERR_ABORT_CannotOfflineTableSpace=Unable to change the tablespace to offline mode.
# *Cause: SYSTEM, UNDO or SYSTEM TEMP tablespaces cannot be offlined.
# *Action: Verify that the tablespace is not a system tablespace

57,smERR_ABORT_InvalidLogAnchorFile = The log anchor file does not exist or is not valid.
# *Cause: The loganchor file does not exist or is invalid.
# *Action: Specify the loganchor file directory for the LOGANCHOR_DIR property.

#60,smERR_ABORT_PhaseOfTableSpaceOffline= A tablespace can be taken offline only during the control phase.
# *Cause: A tablespace can be taken offline only during the control phase.
# *Action: Shut down and start up in the control phase.

#61,smERR_ABORT_CannotOfflineDataFile= A data file can be taken offline only during the control phase.
# *Cause: A data file can be taken offline only during the control phase.
# *Action: Shut down and start up in the control phase.

62,smERR_ABORT_MediaRecoDataFile= <0%s> is only allowed during the control phase.
# *Cause: The user tried to execute ALTER DATABASE CREATE data file, or tried to rename a file, during the service phase.
# *Action: Shut down and start up in the control phase.

#63, smERR_ABORT_InvalidDiskLogRec = The log record for a disk table is invalid. ( LSN {<0%d>,<1%d>} )
# *Cause: The log record for a disk table is invalid.
# *Action: Please send a bug report to the vendor.

#64, smERR_FATAL_InvalidUpdate = Unable to update a temporary variable column
# *Cause: It is impossible to update a temporary variable column.
# *Action: Please send a bug report to the vendor.

65,smERR_ABORT_Aborted =		 A deadlock situation has been detected.
# *Cause: A deadlock victim transaction has been stopped and terminated due to the deadlock resolution scheme.
# *Action: The transaction was rolled back. Re-execute the transaction.

#66,smERR_FATAL_ReadLogRec =		 Unable to read a log record
# *Cause: Unable to read a log record
# *Action: Verify that the log file was not lost.

67,smERR_FATAL_WrongLogFileSize = 	The size of the log file is wrong <0%s>.
# *Cause: The file system has a problem.
# *Action: Check the file system.

68,smERR_ABORT_SequenceReachMaxValue =  Sequence upper bound exceeded
# *Cause: Sequence upper bound exceeded.
# *Action: Reinitialize the sequence.

69,smERR_ABORT_SequenceReachMinValue =  Sequence lower bound exceeded
# *Cause: Sequence lower bound exceeded.
# *Action: Reinitialize the sequence.

70,smERR_ABORT_SequenceNotInitialized = The sequence has not been initialized yet.
# *Cause: An attempt was made to use a sequence that has not been initialized yet.
# *Action: Reinitialize the sequence first.

71,smERR_ABORT_NotDir =					Not a directory
# *Cause: The specified path is not a directory.
# *Action: Specify a valid directory name.

72,smERR_FATAL_TooLongDBName = Database filename too long ( A database file name must be shorter than <0%d> characters. )
# *Cause: The database file name is too long.
# *Action: Use a database file name whose length does not exceed the limit.

73,smERR_ABORT_TooManyPage =           Too many pages were allocated ( Maximum Number of Pages=<0%ld> ).
# *Cause: The number of pages has exceeded the maximum limit.
# *Action: The system failed to increase the size of the database. Refer to the SQL manual.

74,smERR_ABORT_smiTraverseNotApplicable = Unable to traverse
# *Cause: Failed to next-traverse a cursor because the cursor was not used in the proper order.
# *Action: Traverse the cursor in the correct order.

75,smERR_FATAL_Shm_No_Permission = No permission for the shared memory database
# *Cause: A shared memory region has already been created with the same key, but with a different UID.
# *Action: Remove the shared memory region, or create a shared memory region with another key.

76,smERR_RETRY_Already_Modified = A record has already been updated.
# *Cause: Another transaction has already updated the same record.
# *Action: No action is necessary.

77,smERR_ABORT_smiUpdateStatementExist = An update statement already exists.
# *Cause: An UPDATE statement already exists.
# *Action: Do not use the savepoint operation when an UPDATE statement exists on one of the parent statements.

78,smERR_ABORT_smiStatementExist = One or more statements exist.
# *Cause: One or more statements exist.
# *Action: A transaction operation can be used only when no statements exist.

79,smERR_ABORT_smiUpdateOverflow = Too many update cursors on a transaction
# *Cause: The number of update cursors on a transaction exceeds the limitation.
# *Action: Roll back the work of the transaction.

80,smERR_FATAL_smnNotSupportedIndex = The index type is not supported.
# *Cause: The index type is not supported.
# *Action: Refer to the list of supported index types.

81,smERR_ABORT_smiCursorOpened = The cursor has already been opened.
# *Cause: The cursor has already been opened.
# *Action: Open another cursor, or do not open the same cursor twice.

82,smERR_FATAL_smiCursorNotOpened = The cursor has not been opened yet.
# *Cause: The cursor has not been opened yet.
# *Action: Open the cursor before using it.

83,smERR_ABORT_smiWriteNotApplicable = The function is not allowed to be used in read-only mode.
# *Cause: This function cannot be used in read-only mode.
# *Action: Verify that the function can be used in read-only mode.

84,smERR_FATAL_smiNoSelectedRow = No rows were selected.
# *Cause: No rows were selected.
# *Action: Select a row to update or delete.

85,smERR_REBUILD_smiTableModified = The table structure has been modified.
# *Cause: The table structure has been modified.
# *Action: Build the query again.

#86,smERR_ABORT_smiTableNotLoaded = The table is not in main memory.
# *Cause: The table is not in main memory.
# *Action: Load the table into main memory.

87,smERR_FATAL_Uncommitted_Row_Found = An uncommitted row was found.
# *Cause: An uncommitted row was found.
# *Action : Check the error number from the trace log and contact Altibase’s SupportCenter (http://support.altibase.com).

88,smERR_ABORT_smnUniqueViolation = The row already exists in a unique index.
# *Cause: A duplicate key was inserted or updated.
# *Action: Do not insert or update a duplicate key, or drop the unique constraint.

89,smERR_ABORT_smnNotFoundByIndexName = Index type name not found
# *Cause: It is impossible to find the index type name
# *Action: Select an appropriate index.

90,smERR_ABORT_smcFixedPageSizeError = The fixed record size exceeds the size of one page.
# *Cause: A table could not be created because the record size exceeds the size of one page.
# *Action: Create a table for which the record size is less than the size of one page.

91,smERR_ABORT_smcVarColumnSizeError = A variable column size exceeds the maximum size for a variable column.
# *Cause: A variable column that is larger than a page cannot be created.
# *Action: Reduce the size of the variable column so that it is less than the size of one page.

92, smERR_ABORT_Too_Long_Var_Data = A variable data size exceeds the maximum size of a variable item.
# *Cause: A variable data size exceeds the maximum size of a variable item.
# *Action: Change the variable data's length so that it does not exceed 32KB.

93, smERR_ABORT_smiCantBeginUpdateStatement = Unable to begin a new update statement.
# *Cause: Either the statement is read-only, or more than one UPDATE child statement has been requested.
# *Action: Verify that the request to begin a new UPDATE statement is valid

94, smERR_FATAL_smiChildStatementExist = One or more child statements exists.
# *Cause: A cursor cannot be opened when a child statement exists.
# *Action: Do not open a cursor when a child statement exists.

95, smERR_ABORT_smiUpdateSameTable = No more than one update cursor can be used on a table.
# *Cause: Only one update cursor can be used on a table.
# *Action: Do not use more than one update cursor on a table.

96,smERR_ABORT_Already_Locked = A lock has already been acquired by another transaction.
# *Cause: A lock has already been acquired by another transaction.
# *Action: This situation occurs when a user executes a DDL, and a locked item has already been modified by another transaction.

97,smERR_ABORT_Column_Mismatch = Unable to alter the table because the information does not correspond to the definition of the column.
# *Cause: Table information (e.g. a constraint) cannot be modified because it is not in the specified column.
# *Action: Try again with the correct column and modification information.

98,smERR_ABORT_Index_Not_Found = The index was not found on the database.
# *Cause: The index was not found in the database.
# *Action: Verify that the index is correct.

99,smERR_FATAL_Overflow_DB_Size = The database size is larger than specified in the property. (Specified Page Limit=<0%lu> : Allocated Page Size=<1%vu>)
# *Cause: The database page count is too low.
# *Action: Increase the MEM_MAX_DB_SIZE property value.

100,smERR_ABORT_Table_Not_Found = The table was not found in the database.
# *Cause: The specified table was not found in the database.
# *Action: Verify that the specified table exists in the database.

101,smERR_ABORT_Maximum_Index_Count = The number of indices in the table exceeds the limit(<0%d>).
# *Cause: The number of indices in the table exceeds the limit.
# *Action: Do not create more than the allowed number of indices for a table.

102,smERR_ABORT_Invalid_ID_Value = Invalid column ID
# *Cause: The column IDs in the column list do not increment sequentially.
# *Action: Verify that the column ID is correct and create the table again.

103,smERR_ABORT_Maximum_Column_count = Too many columns in a table
# *Cause : Too many columns were specified in the CREATE/ALTER TABLE statement.
# *Action: Reduce the number of columns to 1024 or less.

104,smERR_FATAL_smnColumnNotFound = Column not found
# *Cause: Unable to find the column.
# *Action: Verify that the column you are looking for is valid.

105,smERR_ABORT_INTERNAL_ARG = Internal server error in the storage manager (<0%s>)
# *Cause: Internal server error.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

106,smERR_FATAL_Shm_Link_Not_Exist = A shared memory link has been disconnected. Please remove it and try again.
# *Cause: One or more shared memory links are not connected.
# *Action: Remove the shared memory regions and try again.

107,smERR_FATAL_Shm_Link_Invalid_State = Invalid shared memory link found. Remove it and try again.(key = <0%d>)
# *Cause: The system failed to link a shared memory region.
# *Action: Remove all of the shared memory regions, and try again.

108,smERR_FATAL_Shm_Link_Invalid_Version = Invalid shared memory link version. Remove it and try again (key = <0%d>).
# *Cause: One or more shared memory links has an invalid version.
# *Action: Remove all shared memory regions, and try again.

109,smERR_ABORT_No_More_Shm_Key = Unable to find an available shared memory key.
# *Cause: No more shared memory keys are available.
# *Action: Remove all of the shared memory regions and try again.

110,smERR_FATAL_smiCantExecuteDDL = Unable to execute a DDL.
# *Cause: Either another statement already exists or a cursor is already open.
# *Action: Verify that no other statement currently exists. Ensure that cursors are opened only once.

111,smERR_FATAL_smiCantOpenUpdateCursor = Unable to open an update cursor.
# *Cause: A read-only statement is not allowed to open an update cursor.
# *Action: Verify that an update cursor is not opened in a read-only statement.

112,smERR_FATAL_smiCantRestartUpdateCursor = Unable to restart an update cursor.
# *Cause: An update cursor cannot be restarted.
# *Action: Verify that no attempt to restart an update cursor is being made.

113, smERR_ABORT_smiNotRootStatement = Statement must be the root of the statement.
# *Cause: The statement is not the root of the statement.
# *Action: Abort the transaction.

114, smERR_ABORT_smiInvalidCursorOpen = Invalid cursor open
# *Cause: Invalid cursor open
# *Action: Abort the transaction.

#115, smERR_ABORT_BackupFileOpen =          Unable to open a backup file [<0%s>].
# *Cause: The path of the file is invalid or the storage manager does not have permission to open the file.
# *Action: Check the backup target directory and access permissions for the file.

116, smERR_ABORT_InvalidBackupFile =       [<0%s>] is an invalid backup file for the table.
# *Cause: The table backup file is invalid.
# *Action: Check the database version or backup file.

117, smERR_ABORT_smcExceedLockTimeWait = The transaction has exceeded the lock timeout specified by the user.
# *Cause: The transaction failed to lock the object.
# *Action: Increase the transaction's lock timeout value or check whether the data has a transaction with a long-term lock.

118, smERR_ABORT_NoExistFile =     File not found (<0%s>)
# *Cause: The db file or log file does not exist.
# *Action: Verify that the path and filename are correct.

119, smERR_ABORT_NoExistPath = Directory path not found (<0%s>).
# *Cause: The directory path does not exist.
# *Action: Create the directory path or specify a valid directory.

#120, smERR_FATAL_DiffDbName = The DB name on the disk [<0%s>] does not match that in the property file.
# *Cause: The DB name on the disk differs from that specified in the property value.
# *Action: Correct the DB name in the property file to match the one on the disk.

121, smERR_ABORT_BackupWrite = Disk full.
# *Cause: The disk is full.
# *Action: Provide additional disk space.

122, smERR_FATAL_smiCantEndStatement_too_many = Unable to end the statement because it has child statements (The number of child statements is [<0%d>].)
# *Cause: The statement has child statements.
# *Action: End all of the child statements.

123, smERR_FATAL_smiCantEndStatement_not_closed = Unable to end the statement because it has open cursors.
# *Cause: The statement has open cursors.
# *Action: Close all of the open cursors.

124, smERR_ABORT_droped_Sequence = The sequence has already been dropped.
# *Cause: The sequence has already been dropped.
# *Action: Check whether the sequence exists.

#125, smERR_ABORT_BackupFileOperation = The file I/O operation failed during online backup.
# *Cause: The file cannot be created, opened, or closed.
# *Action: Check file permissions and access mode and whether the file exists.

#126, smERR_ABORT_OnlineBackup = Online backup failure
# *Cause: Online backup failed.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

127, smERR_FATAL_ShmDB_Signature_Mismatch = The DB signature in the shared memory region and the backup database file are different. (Backup DB : <0%s>, Shared DB : <1%s>)
# *Cause: The shared memory region and the backup database file have different timestamps.
# *Action: Remove all of the shared memory regions and try again.

#128, smERR_ABORT_CHECKPOINT_DISABLED = Checkpoint is disabled.
# *Cause: The value of the CHECKPOINT_ENABLED property is 0.
# *Action: Before a checkpoint is executed, set the value of the CHECKPOINT_ENABLED property to 1.

#129, smERR_ABORT_INVALID_LOGGING_LEVEL_VALUE = LOGGING_LEVEL must be from 0 to 2.
# *Cause: LOGGING_LEVEL is not set to a value between 0 and 2.
# *Action: Verify that the value of LOGGING_LEVEL is between 0 and 2 inclusive.

130, smERR_FATAL_DISABLED_ABORT_IN_LOGGING_LEVEL_0 = ABORT is disabled.
# *Cause: There is no log when LOGGING_LEVEL is set to 0.
# *Action: Check the value of the LOGGING_LEVEL and whether a transaction was aborted.

#131, smERR_ABORT_PARALLEL_INDEX_BUILD = The system failed to build an index.
# *Cause: The system failed to build an index.
# *Action: Check the atibase _boot.log file.

132, smERR_ABORT_DISABLED_INDEX = The Index is disabled.
# *Cause: The Index with the table is disabled.
# *Action: Enable the index associated with the table.

133, smERR_FATAL_CannotOpenDir = The system failed to open the directory. ( Directory Name : <0%s> )
# *Cause: The system failed to invoke the opendir() system function.
# *Action: Check the altibase_error.log file for details.

134, smERR_FATAL_CannotReadDir = The system failed to read the directory. ( Directory Name : <0%s> )
# *Cause: The system failed to invoke the readdir_r() system function.
# *Action: Check the altibase_error.log file for details.

135,smERR_ABORT_ExceedMaxRows = The record count has reached the maximum limit.
# *Cause: The number of records has reached or exceeded the maximum limit.
# *Action: Increase the values of MAXROWS for the table.

136,smERR_ABORT_INVALID_ROW_SCN  = Invalid row SCN
# *Cause: Invalid row SCN
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

137,smERR_ABORT_INVALID_COLUMN_SIZE  = Invalid column size
# *Cause: The size of the column is invalid.
# *Action: Check the altibase_error.log file for details.

138,smERR_FATAL_MEMBASE_INVALID = The database is inconsistent.
# *Cause: The database is inconsistent.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

#139, smERR_ABORT_ArchiveLog = Archive log file backup failure
# *Cause: The archive log file backup has failed.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

140,smERR_ABORT_Invalid_MaxRows  = Invalid MAXROWS value
# *Cause: The values of MAXROWS is less than the number of records.
# *Action: Increase the values of MAXROWS.

141,smERR_FATAL_NotFoundDataFile = The data file containing page [<0%d>] does not exist. ( Tablespace - ID : <1%d>, Type : <2%d> )
# *Cause: The data file does not exist.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

142,smERR_ABORT_NotFoundDataFileNodeByID = Data file node not found (ID : <0%d>)
# *Cause: The data file node does not exist.
# *Action: Check the data file.

143,smERR_ABORT_CanStartARCH= Archive thread cannot be started in NO ARCHIVE mode
# *Cause: The user tried to start an archive thread in NO ARCHIVE mode.
# *Action: Shut down, start up to the control phase and then execute the ALTER DATABASE ARCHIVELOG statement.

144,smERR_ABORT_BackupDatafile = The system failed to back up <0%s> <1%s> file.
# *Cause: An error occurred while copying a memory region or a data file of a disk tablespace.
# *Action: Check the disk.

145,smERR_ABORT_DontNeedBackupTempTBS= TEMP tablespace backup not required.
# *Cause: The user tried to back up the TEMP tablespace.
# *Action: Do not back up the TEMP tablespace. It is unnecessary because the TEMP tablespace is initialized whenever the server starts.

#146,smERR_ABORT_BackupLogAnchor= The system failed to back up the log anchor file.
# *Cause: Invalid backup path or not enough disk space.
# *Action: Try to back up the loganchor file using a valid backup directory.

147,smERR_ABORT_WrongStartupPhase= Wrong startup phase. Please proceed to the correct startup phase.
# *Cause: Wrong startup phase. Proceed to the correct startup phase.
# *Action: Proceed to the correct startup phase.

148,smERR_ABORT_ErrArchiveLogMode= Execution impossible in NO ARCHIVE mode.
# *Cause: Restart recovery cannot be performed in noarchivelog mode.
# *Action: To execute restart recovery, switch to archivelog mode.

149,smERR_ABORT_NeedMediaRecovery= Please start up in the CONTROL phase and then execute a complete media recovery.
# *Cause: The data file does not exist or is not valid.
# *Action: Run a complete media recovery in the CONTROL phase.

#150,smERR_ABORT_NotFoundRecvDataFile= Invalid file recovery [<0%s>]
# *Cause: Invalid file recovery.
# *Action: Remove the file from the list of files to recover.

152,smERR_ABORT_BackupLogMode= <0%s> cannot be executed in no archive log mode
# *Cause: Restart recovery cannot be performed in noarchivelog mode.
# *Action: To execute restart recovery, switch to archivelog mode.

153,smERR_ABORT_UseFileInOtherTBS= The file name [<0%s>] is in use by another tablespace.
# *Cause: The file name is in use by another tablespace.
# *Action: Select another name for the destination file.

154,smERR_ABORT_SelfCopy= Unable to copy the file because the source and destination are the same.
# *Cause: The source and the destination path and filename are the same.
# *Action: Choose another destination path.

155,smERR_ABORT_NotSupport= <0%s> is not supported yet.
# *Cause: An attempt was made to execute an unsupported command.
# *Action: No action is necessary.

156, smERR_FATAL_INVALID_SHARED_MEMORY_DATABASE =  When the TRANSACTION_DURABILITY_LEVEL value is not 3, a shared memory database cannot be used. Set the SHM_DB_KEY property to 0.
# *Cause: An attempt was made to use a shared memory database when the TRANSACTION_DURABILITY_LEVEL value was not 3.
# *Action: Please set the TRANSACTION_DURABILITY_LEVEL value to 3, or set the SHM_DB_KEY value to 0.

# deprecated error msg.
#157,smERR_ABORT_WrongAddExtNumValue = The number of extents that were appended to the tablespace (<0%d>) is greater than the number of extents that were appended to the segment.
# *Cause: ADD_EXTENT_NUM_FROM_TBS_TO_SEG * 2 > ADD_EXTENT_NUM_FROM_SYSTEM_TO_TBS
# *Action: Keep the following rule : ADD_EXTENT_NUM_FROM_TBS_TO_SEG * 2 <= ADD_EXTENT_NUM_FROM_SYSTEM_TO_TBSDataFile.

#160,smERR_ABORT_WrongBufferSize = The initial size of the buffer pool is greater than its maximum size.
# *Cause: BUFFER_POOL_SIZE > BUFFER_POOL_MAX_SIZE.
# *Action: Decrease the size of the buffer pool or increase the maximum size of the buffer pool.

161,smERR_ABORT_InvalidFileHdr = Invalid data file header {SID:<0%d> - FID:<1%d>}
# *Cause: Invalid data file header
# *Action: Copy a valid data file to [MEM_DB_DIR].

162,smERR_ABORT_NeedResetLogs= Incomplete media recovery. Resetlogs required.
# *Cause: The media recovery attempt was incomplete because the resetlogs are needed.
# *Action: Start up in the meta phase using the RESETLOGS option.

164,smERR_ABORT_BACKUP_GOING= Backup in progress. Please wait until the current backup is completed.
# *Cause: It is impossible to switch logfiles while a backup is in progress.
# *Action: Wait until the current backup process is completed.

165,smERR_ABORT_NotBeginBackup= Tablespace : <0%d> BACKUP is not in progress.
# *Cause: The tablespace backup has not yet begun.
# *Action: Execute the following statement first: ALTER TABLESPACE tablespacename BEGIN BACKUP.

166,smERR_ABORT_NoActiveBeginBackup = The backup process is not currently underway.
# *Cause: The backup process is not currently underway.
# *Action: Execute the following statement first: ALTER TABLESPACE tablespacename BEGIN BACKUP.

#167,smERR_ABORT_NotExistLogFile= Logfile <0%s> nonexistent.
# *Cause: The logfile does not exist.
# *Action: Check the logfile path and filename.

168,smERR_ABORT_NoExecutePermFile = No execution permission for the path [ <0%s> ].
# *Cause: File execution is not permitted for this path.
# *Action: Add file execution permission for this path.

169,smERR_ABORT_AlreadyBeginBackup= The tablespace <0%d> backup is in the BEGIN state.
# *Cause: The tablespace backup has already begun.
# *Action: Alter the previously completed backup of the tablespace.

170,smERR_ABORT_AlreadyExistTableSpaceName = Duplicate tablespace names [ <0%s> ].
# *Cause: Duplicate tablespace names.
# *Action: Check the tablespace names.

172,smERR_ABORT_TableLockUse = X or S table lock is not allowed while the TABLE_LOCK_ENABLE property is 0.
# *Cause: The user tried to execute a DDL command when the TABLE_LOCK_ENABLE property was 0.
# *Action: Change the TABLE_LOCK_ENABLE value to 1.

173,smERR_ABORT_ActiveTransExits = The alter system TABLE_LOCK_ENABLE can't be executed when an active transaction exists.
# *Cause: A transaction is still active.
# *Action: End all active transactions and try again.

#174,smERR_ABORT_smiCantRestartFixedTableCursor = Unable to restart a fixed table cursor
# *Cause: A fixed table cursor cannot be restarted.
# *Action: Do not attempt to restart a fixed table cursor.

175,smERR_ABORT_OSFileSizeLimit_ERROR = The maximum file size of the OS is less than the database file size that was specified in the CREATE DATABASE statement ( <0%lu> MB ).
# *Cause: The maximum file size of the OS is less than that specified in the property.
# *Action: Increase the maximum file size of the OS.

176, smERR_ABORT_GETLIMIT_ERROR =         Failed to invoke the getrlimit() system function
# *Cause: The system failed to map the log file.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

177,smERR_ABORT_CantAllocTrans = Unable to allocate a nested transaction object
# *Cause: There is no free transaction object available.
# *Action: Wait until there is room for a free transaction object.

178,smERR_ABORT_InvalidDB = Database information is invalid.
# *Cause: Database information is invalid.
# *Action: Check the error number from altibase_sm.log and contact Altibase's Support Center (http://support.altibase.com).

179, smERR_ABORT_DIFFERENT_DB_FREE_PAGE_LIST_COUNT = Different Page List Count. The value of the PAGE_LIST_GROUP_COUNT property is different from the one used when createdb was executed. ( current PAGE_LIST_GROUP_COUNT:<0%d>, createdb's PAGE_LIST_GROUP_COUNT <1%d> )
# *Cause: The value of the PAGE_LIST_GROUP_COUNT property is different from the one used when createdb was executed.
# *Action: Set the PAGE_LIST_GROUP_COUNT value to exactly the same value specified when the database was created.

180, smERR_ABORT_DIFFERENT_DB_EXPAND_CHUNK_PAGE_COUNT = Different Expand Chunk Page Count. The value of the EXPAND_CHUNK_PAGE_COUNT property is different from the one used when createdb was executed. ( current EXPAND_CHUNK_PAGE_COUNT:<0%d>, createdb EXPAND_CHUNK_PAGE_COUNT <1%d> )
# *Cause: The value of the EXPAND_CHUNK_PAGE_COUNT property is different from the one used when createdb was executed.
# *Action: Set the EXPAND_CHUNK_PAGE_COUNT to exactly the same value specified when the database was created.

181, smERR_ABORT_TOO_MANY_PER_LIST_PAGE_COUNT_ERROR = Too many pages in per list page count. ( Please change property values to meet the condition EXPAND_CHUNK_PAGE_COUNT(<0%lu>) is greater than or equal to 2 * PER_LIST_DIST_PAGE_COUNT(<1%lu>) * PAGE_LIST_GROUP_COUNT(<2%lu>). )
# *Cause: An expand chunk must have enough pages to be distributed to all database free page list at least twice. The user has specified property values that does not satisfy this condition, EXPAND_CHUNK_PAGE_COUNT <= 2 * PER_LIST_DIST_PAGE_COUNT * PAGE_LIST_GROUP_COUNT.
# *Action: Change EXPAND_CHUNK_PAGE_COUNT or PER_LIST_DIST_PAGE_COUNT to meet the condition EXPAND_CHUNK_PAGE_COUNT <= 2 * PER_LIST_DIST_PAGE_COUNT * PAGE_LIST_GROUP_COUNT.

#182,smERR_ABORT_InvalidLFGCount= Log file group count of loganchor is not equal to Log file group count in altibase.properties.
# *Cause: The log file group count in altibase.properties was changed after the db was created.
# *Action: Verify that the log file group count of altibase.properties is correct.

183,smERR_ABORT_InvalidUseResetLog= There is no need to use resetlog.
# *Cause: There is no need to use resetlog.
# *Action: Do not use resetlog when not necessary.

#184,smERR_ABORT_Invalid_LFGCntAndPageListCnt= LFG Count(<0%d>) and Page List Count(<1%d>) are invalid.
# *Cause: LFG Count and Page List Count are invalid.
# *Action: Check the property file to verify that the LFG Count and Page List Count values are correct.

#185,smERR_FATAL_MISMATCHED_LFGID_IN_LOGFILE=Mismatched Log File Group ID. The log file(<0%s>) was originally in LFG#<1%d>, but was moved to LFG#<2%d>.
# *Cause: The log file has been moved to another Log File Group.
# *Action: Check the property file to determine whether the order of the LOG_DIR properties has been changed.

186,smERR_FATAL_MISMATCHED_FILENO_IN_LOGFILE=Mismatched log file NO. The number of the log file(<0%s>) was originally #<1%d>, which is different from the one indicated by its name, #<2%d>.
# *Cause: The log file has been renamed.
# *Action: Check if the logfile has been renamed, and rename it to its original name.

188,smERR_ABORT_WaitLogFileOpen = Unable to open log file<%s>.
# *Cause: The system failed to open the log file.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

#189,smERR_ABORT_WrongLogDirCount = The number of LOG_DIR (<0%d>) is not equal to LOG_FILE_GROUP_COUNT(<1%d>).
# *Cause: The number of LOG_DIR is different from LOG_FILE_GROUP_COUNT.
# *Action: Check LOG_FILE_GROUP_COUNT and the number of LOG_DIR.

#190,smERR_ABORT_WrongArchiveDirCount = The number of ARCHIVE_DIR (<0%d>) is not equal to LOG_FILE_GROUP_COUNT(<1%d>).
# *Cause: The number of ARCHIVE_DIR is different from LOG_FILE_GROUP_COUNT.
# *Action: Check LOG_FILE_GROUP_COUNT and the number of ARCHIVE_DIR.

#191,smERR_ABORT_DisableLFG_InMMap = This Log Buffer Type <0%d> is not a supported Log File Group.
# *Cause: The Memory Mapped log buffer type is not a supported Log File Group.
# *Action: Check LOG_BUFFER_TYPE and LOG_FILE_GROUP_COUNT.

#192,smERR_ABORT_WrongDirectIOPageSize= The page size for Direct I/O must be exactly equal to one of 512, 1024, 2048, 4096, or 8192.
# *Cause: The DIRECT_IO_PAGE_SIZE is none of 512, 1024(1K), 2048(2K), 4096(4K), or 8192(8K).
# *Action: Use only 512, 1024(1K), 2048(2K), 4096(4K), or 8192(8K) for the DIRECT_IO_PAGE_SIZE property value.

193,smERR_ABORT_NotFoundDataFile = The data file containing page [<0%u>] does not exist. ( Tablespace - ID : <1%d>, Type : <2%d> )
# *Cause: The data file does not exist.
# *Action: Verify that the data file exists.

194,smERR_ABORT_ART= Error generated by Automatic Recovery Test(ART)
# *Cause: This error was generated by Automatic Recovery Test(ART).
# *Action: You may safely ignore this error.

195,smERR_FATAL_INVALID_MEM_MAX_DB_SIZE = MEM_MAX_DB_SIZE(<0%d>) is less than EXPAND_CHUNK_PAGE_COUNT(<1%d>) * PAGE_SIZE.
# *Cause: The MEM_MAX_DB_SIZE property value is invalid.
# *Action: Use a larger value for the MEM_MAX_DB_SIZE property.

196,smERR_ABORT_CannotSpanTransByLobLocator =  LobLocator cannot span the transaction <0%d>.
# *Cause: The transaction pointed to by Loblocator has ended.
# *Action: No action is necessary.

197,smERR_ABORT_LobCursorClosed = LobCursor  already closed <0%d> ,<1%d>.
# *Cause: An attempt was made to use a lob cursor that was already closed.
# *Action: No action is necessary.

198,smERR_ABORT_CanNotModifyLob = Cannot modify a lob using a read-only LobCursor.
# *Cause: An attempt was made to modify a lob using a read-only lob cursor.
# *Action: No action is necessary.

199,smERR_ABORT_MissMatchedLobTransID = While copying a lob, mismatched source lob <0%d> and destination lob <1%d> transaction ID were found.
# *Cause: An attempt was made to modify a lob using a read-only lob cursor.
# *Action: No action is necessary.

200,smERR_ABORT_overflowLobCursorID = Lob cursor ID overflow
# *Cause: Too many lob cursors have been opened.
# *Action: Close the lob cursor.

201,smERR_ABORT_PrePareWriteProtocol= Lob write preparation protocol error
# *Cause: Before writing was completed, another attempt to write to the lob was made.
# *Action: No action is necessary.

202,smERR_ABORT_FinishWriteProtocol= Lob write finish protocol error
# *Cause: The lob could not be written to before writing was completed.
# *Action: No action is necessary.

203,smERR_ABORT_RangeError= The lob operation range (<0%d> ~ <1%d>) is not in the target range (<2%d> ~ <3%d>)
# *Cause: Lob write range error.
# *Action: No action is necessary.

204,smERR_ABORT_LobCursorTooOld= The lob cursor is too old.
# *Cause: Because another statement updated the same row, this lob cursor became too old.
# *Action: Reopen the lob cursor.

205,smERR_ABORT_InvalidLobStartOffset= Lob operation start offset [<0%d>] > current lob length [<1%d>]
# *Cause: While preparing for writing, the operation start offset was found to be greater than the current length of the lob.
# *Action: No action is necessary.

#206,smERR_ABORT_InValidOldAnNewSize=  Lob operation old size and new size are both equal to zero
# *Cause: While preparing for writing, it was found that old size = 0 and new size = 0.
# *Action: Check the size of the LOB data that needs to be updated.

#207,smERR_ABORT_DmlInterrupt = A lob api function and a DML <0%s> statement were executed on the same row by the same transaction.
# *Cause: A lob api function and a DELETE or UPDATE statement were executed on the same row by the same transaction.
# *Action: Do not combine lob API functions with DML statements.

208,smERR_ABORT_MaxLobErrorSize =  The lob size is bigger than the maximum lob size
# *Cause: The lob size is bigger than the maximum lob size.
# *Action: Ensure that the lob size is correct.

209,smERR_ABORT_INVALIDE_LOB_CURSOR_MODE= If the table cursor is read-only, the lob cursor must also be read-only.
# *Cause: The LOB cursor must be read-only if the table cursor is read-only.
# *Action: Verify that the LOB cursor is read-only.

#210,smERR_ABORT_CANCEL_COMMIT_BY_REPL = The transaction was not committed due to a replication conflict.
# *Cause: A replication conflict has occurred in the standby host.
# *Action: Execute rollback and check your application update logic.

#211,smERR_FATAL_InvalidChkptPathCount = The number of checkpoint paths in the log anchor file is invalid.
# *Cause: The number of checkpoint paths in the log anchor file is invalid.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

212,smERR_FATAL_TooLongTBSName = Tablespace file name too long. (A tablespace file name must be shorter than <0%d> characters.)
# *Cause: The tablespace file name is too long.
# *Action: Verify that the tablespace file name does not exceed the limit.

213,smERR_FATAL_Shm_Link_Invalid_TBSID = Invalid shared memory linkage tablespace ID. Please remove it and try again (Tablespace ID on Shared Memory Chunk= <0%d>, Expected Tablespace ID on Shared Memory Chunk= <1%d>).
# *Cause: The tablespace ID and the tablespace ID stored in the tablespace header in shared memory are different.
# *Action: Remove all shared memory regions and restart the server

214,smERR_FATAL_INVALID_SHARED_MEMORY_DATABASE_TRIAL_TO_DIFFERENT_RESTORE_MODE = Invalid Shared Memory. An attempt was made to restore a tablespace in a different mode. Please remove all shared memory regions and restart Altibase.
# *Cause: A shared memory region is invalid.
# *Action: Remove all shared memory regions and try again.

215,smERR_ABORT_INVALID_STARTUP_PHASE_NOT_CONTROL= <0%s> is only allowed during the control phase.
# *Cause: The user tried to execute a statement which may only be executed in the CONTROL phase.
# *Action: Shut down, start up to the CONTROL phase and try again.


216,smERR_ABORT_CPATH_NOT_EXIST= The checkpoint path '<0%s>' does not exist.
# *Cause: The user tried to use a checkpoint path that does not exist.
# *Action: Verify that the checkpoint path exists.

217,smERR_ABORT_CPATH_NO_READ_PERMISSION= The checkpoint path '<0%s>' does not have READ permission.
# *Cause: The user tried to use a checkpoint path that does not have READ permission.
# *Action: Verify that the checkpoint path has READ permission.

218,smERR_ABORT_CPATH_NO_WRITE_PERMISSION= The checkpoint path '<0%s>' does not have WRITE permission.
# *Cause: The user tried to use a checkpoint path that does not have WRITE permission.
# *Action: Verify that the checkpoint path has WRITE permission.

219,smERR_ABORT_CPATH_NO_EXEC_PERMISSION= The checkpoint path '<0%s>' does not have EXECUTE permission.
# *Cause: The user tried to use a checkpoint path that does not have EXECUTE permission.
# *Action: Verify that the checkpoint path has EXECUTE permission.

220,smERR_ABORT_CPATH_NOT_A_DIRECTORY= The checkpoint path '<0%s>' is not a directory.
# *Cause: The user tried to use a checkpoint path that is not a directory.
# *Action: Verify that the checkpoint path is a directory.

221,smERR_ABORT_CPATH_NODE_NOT_EXIST= The checkpoint path node '<0%s>' does not exist.
# *Cause: The user tried to rename or drop a checkpoint path that does not exist.
# *Action: Verify that the checkpoint path exists.

222,smERR_ABORT_UNABLE_TO_DROP_LAST_CPATH= A tablespace needs at least one checkpoint path. Unable to drop the checkpoint path '<0%s>'.
# *Cause: The user tried to drop the only checkpoint path in the tablespace.
# *Action: Rename the checkpoint path instead of dropping it.

223,smERR_ABORT_CPATH_ALREADY_EXISTS= The checkpoint path node '<0%s>' already exists.
# *Cause: The user tried to add a checkpoint path that already exists.
# *Action: Do not try to add a checkpoint path that already exists.

224,smERR_ABORT_ALTER_TBS_AUTOEXTEND_ALREADY_SET = The attribute of AUTOEXTEND mode has already been set.
# *Cause: The AUTOEXTEND mode has already been set.
# *Action: No action is necessary.

225,smERR_ABORT_ALTER_TBS_NEXTSIZE_NOT_ALIGNED_TO_CHUNK_SIZE = The NEXT attribute must be a multiple of the expand chunk size ( <0%lu>K )
# *Cause: The NEXT attribute must be a multiple of the expand chunk size. The expand chunk size is calculated by EXPAND_CHUNK_PAGE_COUNT * PAGE_SIZE
# *Action: Verify that the NEXT size is aligned to EXPAND_CHUNK_PAGE_COUNT * PAGE_SIZE.

226,smERR_ABORT_ALTER_TBS_MAXSIZE_LESSTHAN_CURRENT_SIZE = The MAXSIZE attribute must be greater than or equal to the current size of the tablespace ( <0%lu>K )
# *Cause: The MAXSIZE attribute is less than the current size of the tablespace.
# *Action: Verify that the MAXSIZE attribute is greater than or equal to the current size of the tablespace.

227,smERR_ABORT_ALTER_TBS_AT_DROPPED_TBS = Unable to alter a dropped tablespace
# *Cause: The tablespace was dropped.
# *Action: Verify that the tablespace exists.

228,smERR_ABORT_ALTER_TBS_AT_OFFLINE_TBS = Unable to alter an offline tablespace
# *Cause: The tablespace is offline.
# *Action: Verify that the tablespace is online before attempting to alter it.

229,smERR_ABORT_INVALID_CIMAGE_HEADER = Invalid Checkpoint Image Header {SID:<0%d> - PPID:<1%d> - FID:<2%d>}
# *Cause: Invalid Checkpoint Image header.
# *Action: Copy a valid Checkpoint Image to [MEM_DB_DIR].

230,smERR_ABORT_DefaultDBFileSizeNotAlignedToChunkSize = The value of the DEFAULT_MEM_DB_FILE_SIZE property must be a multiple of the expand chunk size ( EXPAND_CHUNK_PAGE_COUNT * PAGE_SIZE(32K) = <0%lu>K )
# *Cause: The value of DEFAULT_MEM_DB_FILE_SIZE property is not a multiple of the expand chunk size.
# *Action: Verify that the value of DEFAULT_MEM_DB_FILE_SIZE property is a multiple of the expand chunk size.

231,smERR_ABORT_CANNOT_ALTER_STATUS_OF_SYSTEM_TABLESPACE= Unable to change the system tablespace status to OFFLINE or ONLINE.
# *Cause: It is impossible to alter the status of a system tablespace whose type is SYSTEM, UNDO, or SYSTEM TEMP.
# *Action: Verify that the tablespace is not a system tablespace.

232,smERR_ABORT_CANNOT_ALTER_AUTOEXTEND_DICTIONARY_TABLESPACE= Unable to alter the AUTOEXTEND mode of the dictionary tablespace.
# *Cause: It is impossible to alter the AUTOEXTEND mode of the dictionary tablespace.
# *Action: Verify that the tablespace is not the dictionary tablespace.

233,smERR_ABORT_ALTER_TBS_ONOFF_ALLOWED_ONLY_AT_META_SERVICE_PHASE= ALTER TABLESPACE ONLINE/OFFLINE execution is allowed only during the META/SERVICE phase.
# *Cause: It is impossible to alter the status of a tablespace during the PROCESS or CONTROL phases.
# *Action: Attempt this action during the META or SERVICE startup phases.

234,smERR_ABORT_SplitSizeNotAlignedToChunkSize = The split size of the memory checkpoint image file must be a multiple of the expand chunk size ( EXPAND_CHUNK_PAGE_COUNT * PAGE_SIZE(32K) = <0%lu>K )
# *Cause: The split size of the memory checkpoint image file is not a multiple of the expand chunk size.
# *Action: Verify that the split size of the memory checkpoint image file is a multiple of the expand chunk size.

235,smERR_ABORT_INVALID_CIMAGE_FILESPEC_FORMAT = Invalid Checkpoint Image Filespec Format '<0%s>'.
# *Cause: Invalid checkpoint image filespec format.
# *Action: Check checkpoint image filespec format.

236,smERR_ABORT_INPUT_UNSTABLE_CIMAGE = The checkpoint image '<0%s>' is not stable.
# *Cause: The user tried to input a checkpoint image that is not stable.
# *Action: Check the loganchor file and input a stable checkpoint image.

237,smERR_ABORT_ERROR_MEDIA_RECOVERY_TYPE= Please execute incomplete media recovery in the CONTROL phase, or execute a restart recovery.
# *Cause: Either media recovery is complete or unnecessary.
# *Action: Restart normally.

238,smERR_ABORT_TBSInitSizeNotAlignedToChunkSize = The initial size of the tablespace must be a multiple of the expand chunk size ( EXPAND_CHUNK_PAGE_COUNT * PAGE_SIZE(32K) = <0%lu>K )
# *Cause: The initial size of the tablespace is not a multiple of the expand chunk size.
# *Action: Verify that the initial size of the tablespace is a multiple of the expand chunk size.

239,smERR_ABORT_UNABLE_TO_EXTEND_CHUNK_WHEN_AUTO_EXTEND_OFF = Unable to extend the tablespace(<0%s>) when AUTOEXTEND mode is OFF
# *Cause: The user tried to extend the tablespace when the AUTOEXTEND mode of the tablespace was OFF.
# *Action: Use the ALTER TABLESPACE AUTOEXTEND ON statement to set AUTOEXTEND mode to ON.

240,smERR_ABORT_UNABLE_TO_EXTEND_CHUNK_MORE_THAN_MEM_MAX_DB_SIZE = Unable to extend the tablespace (<0%s>) because the database would be larger than MEM_MAX_DB_SIZE(<1%lu>K).
# *Cause: The user tried to extend the tablespace, but the size of the database would be larger than MEM_MAX_DB_SIZE.
# *Action: Enlarge MEM_MAX_DB_SIZE or drop another tablespace.

241,smERR_ABORT_UNABLE_TO_EXTEND_CHUNK_MORE_THAN_TBS_MAXSIZE = Unable to extend the tablespace (<0%s>) because the current size of the tablespace (<1%lu>K) would be larger than MAXSIZE (<2%lu>K) of the tablespace.
# *Cause: The user tried to extend the tablespace, but the size of the tablespace would exceed MAXSIZE for the tablespace.
# *Action: Use the ALTER TABLESPACE AUTOEXTEND ON MAXSIZE statement to change the MAXSIZE for the tablespace.

#242,smERR_ABORT_OSFileSizeLimit_ERROR_SUSPENDED = The maximum file size of the OS (<0%lu>K) is smaller than the file size (<1%lu>K) specified by <2%s>.
# *Cause: The maximum file size for the OS is smaller than the size specified by the user.
# *Action: Decrease the file size or increase the OS file size limit.

243,smERR_ABORT_PAGE_RANGE_ERROR = Invalid Page Range. Valid Page Range = 1 ~ <0%d>.
# *Cause: The page range is invalid.
# *Action: Specify a valid page range.

244,smERR_ABORT_TABLESPACE_IS_ALREADY_ONLINE = The tablespace is already in ONLINE mode
# *Cause: The user tried to execute “ALTER TABLESPACE ONLINE” on a tablespace that is already in ONLINE mode.
# *Action: Verify that the table space is not in ONLINE mode before attempting this action.

245,smERR_ABORT_TABLESPACE_IS_ALREADY_OFFLINE=The tablespace is already in OFFLINE mode
# *Cause: The user tried to execute “ALTER TABLESPACE OFFLINE” on a tablespace that is already in OFFLINE mode.
# *Action: Verify that the table space is not in OFFLINE mode before attempting this action.

246,smERR_ABORT_DUMP_EMPTY_OBJECT = Empty dump object.
# *Cause: You did not specify a dump object for the dump table.
# *Action: Verify that you have set a dump object for the dump table.

#247,smERR_ABORT_CannotOnlineTableSpace=Unable to change the tablespace to online mode.
# *Cause: It is impossible to change the tablespace to online mode because its type is SYSTEM, UNDO or SYSTEM_TEMP.
# *Action: Verify that the tablespace is not a system tablespace.

248,smERR_ABORT_CannotDiscardTableSpace=Unable to change the tablespace to discard mode.
# *Cause: It is impossible to change the tablespace to discard mode because its type is SYSTEM, UNDO or SYSTEM_TEMP.
# *Action: Verify that the tablespace is not a system tablespace.

249,smERR_ABORT_MEDIA_RECOVERY_IS_NOT_SUPPORT_SHARED_MEMORY=Media Recovery is not supported by the shared memory version.
# *Cause: ALTER DATABASE RECOVER DATABASE is not supported for shared memory versions.
# *Action: Verify that the SHM_DB_KEY property is set to zero.

250,smERR_ABORT_CannotAllocLogBufferMemory = Cannot allocate memory for the volatile log buffer.
# *Cause: Insufficient memory.
# *Action: Retry this statement in autocommit mode.

251,smERR_ABORT_UNABLE_TO_USE_OFFLINE_TBS=Unable to use an offline tablespace. (NAME=<0%s>)
# *Cause: The user tried to use an offline tablespace.
# *Action: Execute the ALTER TABLESPACE ONLINE statement and try again.

252,smERR_ABORT_UNABLE_TO_USE_DISCARDED_TBS=Unable to use a discarded tablespace. (NAME=<0%s>)
# *Cause: The user tried to use a discarded tablespace.
# *Action: Execute the DROP TABLESPACE statement and recreate the tablespace.

253,smERR_ABORT_TBS_ALREADY_DISCARDED=The tablespace has already been discarded.
# *Cause: The user tried to discard a tablespace that has already been discarded.
# *Action: Execute the DROP TABLESPACE statement and recreate the tablespace.

254,smERR_ABORT_AUTOEXT_ON_UNALLOWED_FOR_USED_UP_FILE = The autoextend attribute of a data file that has been used up cannot be switched on.
# *Cause: The user tried to turn on the autoextend attribute for a data file that has been used up.
# *Action: Try again with a current or unused data file.

255,smERR_ABORT_StmtMaxDepthLevel = The statement depth has reached the maximum level.
# *Cause: The depth of the statement is greater than 255.
# *Action: Investigate the reason for the statement depth.

256,smERR_ABORT_UNABLE_TO_EXTEND_CHUNK_MORE_THAN_VOLATILE_MAX_DB_SIZE = Unable to extend the tablespace (<0%s>) because the database would be larger than VOLATILE_MAX_DB_SIZE (<1%lu>K).
# *Cause: The user tried to extend the tablespace, but the size of the database would be larger than VOLATILE_MAX_DB_SIZE.
# *Action: Increase the VOLATILE_MAX_DB_SIZE value or drop another tablespace.

257,smERR_ABORT_UNABLE_TO_BACKUP_FOR_VOLATILE_TABLESPACE = Unable to back up a volatile tablespace.
# *Cause: The user tried to back up a volatile tablespace.
# *Action: It is not necessary to back up volatile tablespaces.

258,smERR_ABORT_UNABLE_TO_ALTER_ONLINE_CUZ_MEM_MAX_DB_SIZE = Unable to change the tablespace status (NAME=<0%s>, SIZE=<1%lu>K) to ONLINE because the database would be larger than MEM_MAX_DB_SIZE (<2%lu>K). ( Current Database Size = <3%lu>K )
# *Cause: The user tried to bring the tablespace online, but the size of the database would be larger than MEM_MAX_DB_SIZE.
# *Action: Increase the MEM_MAX_DB_SIZE value or bring another tablespace offline, and try again.

259,smERR_ABORT_UNABLE_TO_CREATE_CUZ_MEM_MAX_DB_SIZE = Unable to create the tablespace (NAME=<0%s>, SIZE=<1%lu>K) because the database would be larger than MEM_MAX_DB_SIZE (<2%lu>K). ( Current Database Size = <3%lu>K )
# *Cause: The user tried to create a tablespace, but the size of the database would be larger than MEM_MAX_DB_SIZE.
# *Action: Increase the MEM_MAX_DB_SIZE value or bring another tablespace offline.

260,smERR_ABORT_FileNameIsNullString = The length of the filename is zero.
# *Cause: The filename is an empty string.
# *Action: Enter a valid filename for the file to be created.

261,smERR_ABORT_InvalidExtendFileSizeOSLimit = The data file cannot be extended because the requested size is bigger than the OS file limit size. ( Request Size : <0%lu> pages, OS File Limit Size : <1%lu> pages )
# *Cause: The resize (extend) value is too big.
# *Action: Use a suitable value for the resize action.

#262,smERR_ABORT_InvalidDatafileToResize = Cannot resize a datafile that has already been filled with data.
# *Cause: The datafile to be resized is already filled with data.
# *Action: Attempt the resize operation on another datafile.

263,smERR_ABORT_UNABLE_TO_CREATE_CUZ_VOL_MAX_DB_SIZE = Unable to create the tablespace (NAME=<0%s>, SIZE=<1%lu>K) because the database would be larger than VOLATILE_MAX_DB_SIZE(<2%lu>K). ( Current Total Volatile Tablespace Size = <3%lu>K )
# *Cause: The user tried to create the tablespace, but the total size of the volatile tablespace would be larger than VOLATILE_MAX_DB_SIZE.
# *Action: Increase the VOLATILE_MAX_DB_SIZE value or drop another volatile tablespace.

264,smERR_ABORT_LogFileSizeNotAlignedToDirectIOPageSize = The size of the logfile is not aligned to DIRECT_IO_PAGE_SIZE.
# *Cause: The size of the logfile is not an exact multiple of DIRECT_IO_PAGE_SIZE.
# *Action: Verify that the size of the logfile is an exact multiple of DIRECT_IO_PAGE_SIZE.

265,smERR_ABORT_MAX_AGER_COUNT_LT_MIN_AGER_COUNT = MAX_LOGICAL_AGER_COUNT property is less than MIN_LOGICAL_AGER_COUNT property. ( MAX_LOGICAL_AGER_COUNT=<0%d>, MIN_LOGICAL_AGER_COUNT=<1%d> )
# *Cause: The user has specified a maximum number of logical agers that is less than the minimum number of logical agers.
# *Action: Verify that the MAX_LOGICAL_AGER_COUNT value is greater than or equal to the MIN_LOGICAL_AGER_COUNT value.


266,smERR_ABORT_AGER_COUNT_OUT_OF_MAX_COUNT = The LOGICAL_AGER_COUNT property value is greater than the MAX_LOGICAL_AGER_COUNT property value. ( LOGICAL_AGER_COUNT=<0%d>, MAX_LOGICAL_AGER_COUNT=<1%d> )
# *Cause: The LOGICAL_AGER_COUNT property value is greater than the MAX_LOGICAL_AGER_COUNT property value.
# *Action: Verify the LOGICAL_AGER_COUNT property value is less than or equal to the MAX_LOGICAL_AGER_COUNT property value.

267,smERR_ABORT_AGER_COUNT_OUT_OF_MIN_COUNT = The LOGICAL_AGER_COUNT property value is less than the MIN_LOGICAL_AGER_COUNT property value. ( LOGICAL_AGER_COUNT=<0%d>, MIN_LOGICAL_AGER_COUNT=<1%d> )
# *Cause: The LOGICAL_AGER_COUNT property value is less than the MIN_LOGICAL_AGER_COUNT property value.
# *Action: Verify the LOGICAL_AGER_COUNT property value is greater than or equal to the MIN_LOGICAL_AGER_COUNT property value.

268, smERR_FATAL_ALLOC_NEW_EXPAND_CHUNK = Fatal error during alloc new expand chunk
# *Cause: Insufficient available memory.
# *Action: Verify that there is enough available memory and disk space.

269, smERR_ABORT_NOT_NULL_VIOLATION = Unable to add NOT NULL constraint to the specified column because it already contains null values
# *Cause: The NOT NULL constraint cannot be added to the specified column because it already contains null values.
# *Action: Remove all null values from the column prior to adding the NOT NULL constraint.

#270,smERR_ABORT_DISK_AGER_COUNT_OUT_OF_MAX_COUNT = The DISK_GC_THREAD_COUNT or DISK_DELETE_THREAD_COUNT property value is greater than the MAX_DISK_AGER_THREAD_COUNT property value. ( DISK_GC_THREAD_COUNT or DISK_DELETE_THREAD_COUNT=<0%d>, MAX_DISK_AGER_THREAD_COUNT=<1%d> )
# *Cause: The DISK_GC_THREAD_COUNT or DISK_GC_THREAD_COUNT property value is greater than the MAX_DISK_AGER_THREAD_COUNT property value.
# *Action: Verify the DISK_GC_THREAD_COUNT or DISK_DELETE_THREAD_COUNT property value is less than or equal to the MAX_DISK_AGER_THREAD_COUNT property value.

#271,smERR_ABORT_DISK_AGER_COUNT_OUT_OF_MIN_COUNT = The DISK_GC_THREAD_COUNT or DISK_DELETE_THREAD_COUNT property value is less than the MIN_DISK_AGER_THREAD_COUNT property value. ( DISK_GC_THREAD_COUNT or DISK_DELETE_THREAD_COUNT=<0%d>, MIN_DISK_AGER_THREAD_COUNT=<1%d> )
# *Cause: The DISK_GC_THREAD_COUNT or DISK_DELETE_THREAD_COUNT property value is less than the MIN_DISK_AGER_THREAD_COUNT property value.
# *Action: Verify the DISK_GC_THREAD_COUNT or DISK_DELETE_THREAD property value is greater than or equal to the MIN_DISK_AGER_THREAD_COUNT property value.

272, smERR_ABORT_INCONSISTENT_INDEX = The index is inconsistent
# *Cause: The index is inconsistent.
# *Action: Check index consistency and rebuild the index.

273,smERR_REBUILD_smiTBSModified = The tablespace structure has been modified
# *Cause: The tablespace has been dropped.
# *Action: Build the query again.

274,smERR_ABORT_Not_Support_MMap = The OS does not support mmap.
# *Cause: The OS does not support mmap.
# *Action: Check LOG_BUFFER_TYPE.

#275,smERR_ABORT_Invalid_Datafile_Header = The data file '<0%s>' has an invalid header. : DATABASE SID=<1%u>, FID=<2%u>, RedoLSN=control[<3%u>, <4%u>, <5%u>], [<6%u>, <7%u>, <8%u>], CreateLSN=control[<9%u>, <10%u>, <11%u>], [<12%u>, <13%u>, <14%u>], DBVer=<15%u>, CtrVer=<16%u>, FileVer=<17%u>
# *Cause: The data file creation LSN does not match one of the log anchor files.
# *Action: Verify the data file.

276,smERR_ABORT_INVALID_DUMP_OBJECT = Invalid dump object
# *Cause: The object is not valid for a dump table.
# *Action: Verify that a valid dump object has been used for the dump table.

277, smERR_ABORT_TBS_ATTR_FLAG_ALREADY_SET = The tablespace attribute has already been set to the given value.
# *Cause: The LOG COMPRESS attribute already corresponds to the given value.
# *Action: No action is necessary.

278, smERR_ABORT_TABLE_ATTR_FLAG_ALREADY_SET = The table attribute has already been set to the given value.
# *Cause: The attribute is already the same as the given value.
# *Action: No action is necessary.

279, smERR_ABORT_UNABLE_TO_COMPRESS_VOLATILE_TBS_LOG = Log compression is not supported for volatile tablespaces.
# *Cause: The user tried to compress a volatile tablespace log.
# *Action: No action is necessary.

280,smERR_ABORT_TOO_MANY_UPDATE_LOG = The update log size '<0%lu>' is bigger than TRX_UPDATE_MAX_LOGSIZE '<1%lu>'
# *Cause: The update log size '<%lu>' is bigger than TRX_UPDATE_MAX_LOGSIZE '<%lu>'
# *Action: TRX_UPDATE_MAX_LOGSIZE is too small.

#281,smERR_ABORT_WrongLogBufferTypeCount = The number of LOG_BUFFER_TYPE (<0%d>) properties is not equal to the value of LOG_FILE_GROUP_COUNT(<1%d>).
# *Cause: The number of LOG_BUFFER_TYPE properties is different from the LOG_FILE_GROUP_COUNT value.
# *Action: Check the LOG_FILE_GROUP_COUNT value and the number of LOG_BUFFER_TYPE properties.

282,smERR_ABORT_Invalid_DataFile_Create_LSN = The create LSN (<0%u>, <1%u>, <2%u>) of the data file (<3%s>) is bigger than the restart redo LSN (<4%u>, <5%u>, <6%u>).
# *Cause: The data file is invalid.
# *Action: Verify that the data file was backed up correctly.

#283,smERR_ABORT_NotEnough_DPath_Ins_Temp_Queue = DIRECT_PATH_INSERT_TEMP_QUEUE_SIZE(='<0%d>') property must be greater than Insert Row Size ( = '<1%d>').
# *Cause: Direct-Path INSERT Temporary Queue too small.
# *Action: Increase the DIRECT_PATH_INSERT_TEMP_QUEUE_SIZE value.

#284,smERR_ABORT_CHILD_TX_COMMIT_FAIL = Child Transaction was not committed.
# *Cause: Child Transaction was not committed.
# *Action: Child Transaction was not committed.

#285,smERR_ABORT_PARALLEL_DEGREE_TOO_HIGH = Parallel Degree is less than or equal to (CPU*2).
# *Cause: The parallel degree has exceeded the maximum size (= CPU * 2).
# *Action: Set the parallel degree so that it is less than or equal to (CPU * 2).

#286,smERR_ABORT_Cannot_Access_Already_Modifying_Object_In_Parallel = Cannot read/modify an object after modifying it in parallel.
# *Cause: Cannot read/modify an object after modifying it in parallel.
# *Action: Repeat this action after commit.

287,smERR_ABORT_PageCorrupted = A page is corrupt. ( Current Space ID : <0%d>, Current Page ID : <1%d>, The corresponding page has been dumped to "$ALTIBASE_HOME/trc/altibase_dump.log". )
# *Cause: The page was not written completely.
# *Action: Recover the tablespace that contains the corrupt page with backup and recovery utilities.

288,smERR_ABORT_INCONSISTENT_TABLE = The table is inconsistent.
# *Cause: The table is inconsistent.
# *Action: Recreate the table.

289,smERR_ABORT_CANNOT_ADD_DataFile = It is impossible to add any more data files.
# *Cause: The number of files has reached the limit.
# *Action: Refrain from attempting this action.

290,smERR_ABORT_CANT_SHRINK_BELOW_HWM = It is impossible to shrink this file. ( Request Size : <0%lu> pages, Used File Size : <1%lu> pages )
# *Cause: An attempt was made to shrink the file below HWM.
# *Action: Refrain from attempting this action.

291,smERR_ABORT_NOT_ENOUGH_SPACE = The tablespace does not have enough free space ( TBS Name :<0%s> ).
# *Cause: The tablespace does not have enough free space.
# *Action: Add a new data file.

292,smERR_ABORT_FILE_IS_TOO_SMALL = Not even one extent can be created. ( Request Init Size : <0%lu> pages, Minimum File Size : <1%lu> pages )
# *Cause: The file is too small for even one extent.
# *Action: Retry with a greater size.

#293,smERR_ABORT_FILE_SIZE_IS_TOO_BIG = File too big
# *Cause: The file size exceeds the maximum.
# *Action: Refrain from attempting this action.

294,smERR_ABORT_SegmentExceedMaxExtents= Max no. of extents reached for segment ( TBSID : <0%u>, SEGHDR_FID : <1%u>, SEGHDR_FPID : <2%u>, CurrExtCnt : <3%u>, MaxExtCnt : <4%u> ).
# *Cause: The maximum number of extents has been reached for this segment.
# *Action: Increase the MAXEXTENTS value for this segment.

295,smERR_ABORT_NotExistSegment=The segment does not exist or is not in a valid state.
# *Cause: The segment does not exist or is not in a valid state.
# *Action: Verify that the segment object is valid.

296,smERR_ABORT_SHRINK_SIZE_IS_TOO_SMALL = The requested size of the data file is less than the minimum file size. ( Request Size : <0%lu> pages, Minimum File Size : <1%lu> pages )
# *Cause: The file is too small for even one extent.
# *Action: Increase the file size.

297,smERR_ABORT_TOO_MANY_DATA_FILE = The Tablespace has too many data files.
# *Cause: An attempt was made to create the Tablespace with too many data files.
# *Action: Reduce the number of data files in the Tablespace to 1024 or less.

#298,smERR_FATAL_BCB_HASH_RESIZE = Error occurred during BCB Hash Table resize.
# *Cause:  Unexpected internal error.
# *Action: Please send a bug report to the vendor.

#299,smERR_ABORT_DWFILE_INVALID_MAGIC_NUMBER = Invalid magic number in the double write file <0%s>.
# *Cause: The first 4-byte number of the dw file are invalid.
# *Action: Check the dw file.

#300,smERR_ABORT_DWFILE_INVALID_SM_VERSION_NUMBER = Invalid SM version number in the double write file <0%s>.
# *Cause: The second 4-byte number of the dw file is invalid.
# *Action: Check the dw file.

301,smERR_ABORT_smiInvalidFlusherID = The flusher ID [<0%d>] must be less than <1%d>.
# *Cause: The flusher ID is invalid.
# *Action: Check the flusher count.

302,smERR_ABORT_sdbFlusherNotStarted = The flusher [<0%d>] was not started.
# *Cause: The flusher was not started.
# *Action: Check the flusher status.

303,smERR_ABORT_sdbFlusherRunning = The flusher [<0%d>] is already running.
# *Cause: The flusher cannot be started because it is already running.
# *Action: Check the flusher status.

304,smERR_ABORT_ILLEGAL_REQ_BUFFER_SIZE = BUFFER_AREA_SIZE has to be larger than current BUFFER_AREA_SIZE.
# *Cause: The current version of Altibase doesn't support decreasing the buffer area size.
# *Action: If you want to decrease the buffer area size, first shut down Altibase and set a suitable value for the property. Then start Altibase again.

#305,smERR_ABORT_BCB_RESOURCE_DEADLOCK = There are no available BCBs.
# *Cause: All BCBs are fixed.
# *Action: Unfix all of your BCBs and try again.

306,smERR_ABORT_INVALID_BUFFER_EXPAND_SIZE = |BUFFER_AREA_SIZE - current BUFFER_AREA_SIZE| must be larger than BUFFER_AREA_CHUNK_SIZE.
# *Cause: |BUFFER_AREA_SIZE - current BUFFER_AREA_SIZE| is smaller than BUFFER_AREA_CHUNK_SIZE.
# *Action: Enter another value for BUFFER_AREA_SIZE.

#307,smERR_FATAL_INVALID_LSN_OFFSET = Invalid LSN Offset (LFG ID=<0%u>, File No=<1%u>, Offset=<2%u>)
# *Cause: LSN Offset is invalid.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

308,smERR_ABORT_Not_Support_function = Altibase doesn't support this function
# *Cause: Altibase doesn't support this function.
# *Action: Check the manual.

309,smERR_ABORT_AlreadyExistLogFile = The Log file already exists.
# *Cause: The Log file already exists.
# *Action: Confirm that you have executed destroydb.

310,smERR_ABORT_AlreadyExistLogAnchorFile = The LogAnchor file already exists.
# *Cause: The LogAnchor file already exists.
# *Action: Confirm that you have executed destroydb.

311,smERR_ABORT_UseFileInTheTBS= The file name [<0%s>] is in use by the tablespace [<1%s>].
# *Cause: The file name is in use by a tablespace.
# *Action: Choose another destination file name.

312,smERR_ABORT_BUFFER_MANAGER_BUSY = The buffer area size cannot be changed because of transactions.
# *Cause: There are currently executing transactions.
# *Action: Try again when the system is not busy.

313,smERR_ABORT_CannotCreateSegInUndoTBS = Cannot create segments in an undo tablespace.
# *Cause: Cannot create a table or index in an undo tablespace.
# *Action: Use another tablespace.

314,smERR_FATAL_ErrNeedMoreLog = There are insufficient logfiles, or invalid logfiles at '<0%s>'.
# *Cause: There are insufficient or invalid logfiles.
# *Action: Check the logfiles.

315,smERR_ABORT_Datafile_Header_Read_Failure = File header not read from database file '<0%s>'.
# *Cause: The DBFile was not found, or the DBFileHdr value in the DBFile could not be read.
# *Action: Check the DBFile.

316,smERR_ABORT_Datafile_Header_Write_Failure = File header of database file '<0%s>' not written.
# *Cause: The DBFile was not found, or the DBFileHdr was not written in the DBFile.
# *Action: Check the DBFile.

317,smERR_ABORT_NotFoundDataFileByPath = Data file not found  '<0%s>'.
# *Cause: Data file not found.
# *Action: Check the data file.

318,smERR_ABORT_EXTENT_SIZE_IS_TOO_SMALL = An extent size for an auto segment space managed tablespace must have at least 5 blocks.
# *Cause: The user tried to create a tablespace with an invalid (too small) extent size.
# *Action: Create a tablespace with a valid (greater than PAGESIZE * 5) extent size.

#319, smERR_ABORT_smiInvalidLobCursorOpen = Invalid lob cursor open
# *Cause: An invalid LOB cursor is open.
# *Action: Abort the transaction.

320, smERR_ABORT_LogSizeExceedLogFileSize= The size of a log record exceeds the logfile size ( logfile size <0%d> bytes, log record size <1%d> bytes ).
# *Cause: The size of a log record exceeds the logfile size.
# *Action: Change the property to a suitable value and recreate the database.

321,smERR_ABORT_Invalid_Mtx_LatchStack_Size = The mini transaction's latchstack size is not 1.
# *Cause:  Internal mtx misuse.
# *Action: Send a bug report to the vendor.

#322,smERR_ABORT_CantSupportAutoSegmentManagementYet= Automatic segment management is an experimental feature.
# *Cause: This is an experimental feature.
# *Action: Create a disk tablespace using MANUAL segment management.

#323,smERR_ABORT_sdcTooLargeColumnError = The temp record column is too large (<0%d> is larger than 65535).
# *Cause: A temp record column cannot be larger than 65535.
# *Action: Cannot use queries based on the temp table.

#324,smERR_ABORT_RefuseLobPartialWrite = LOB Partial Write not supported yet.
# *Cause: LOB Partial Write is not supported yet.
# *Action: No action is necessary.

325,smERR_ABORT_AllFlushersStopped = You cannot run the DCL command because all flushers are stopped.
# *Cause: All flushers are stopped.
# *Action: Start some flushers.

326,smERR_ABORT_DW_FILE_NOT_FOUND = There is no double write file.
# *Cause: The DOUBLE_WRITE_DIRECTORY property has been modified.
# *Action: Check your properties.

327,smERR_ABORT_INVALID_LOGFILE = Invalid logfile ( File Name : <0%s> ).
# *Cause: The log file is invalid.
# *Action: Check the logfile.

#328,smERR_ABORT_NOT_FOUND_LOG = Cannot find the log record (SN >= <0%lu>) that is needed in the logfile (<1%s>).
# *Cause: The log file is invalid.
# *Action: Check the logfile.

329,smERR_ABORT_WrongVerifyDiskIndexCount = The number of __SM_VERIFY_DISK_INDEX_NAME(<0%d>) is not __SM_VERIFY_DISK_INDEX_COUNT(<1%d>).
# *Cause: The number of __SM_VERIFY_DISK_INDEX_NAME is different from __SM_VERIFY_DISK_INDEX_COUNT.
# *Action: Check __SM_VERIFY_DISK_INDEX_COUNT and the number of __SM_VERIFY_DISK_INDEX_NAME.

330,smERR_ABORT_TX_ALLOC = Failed to allocate a transaction.
# *Cause: Allocating the transaction took too long.
# *Action: Check the transaction table status.

331,smERR_IGNORE_CAN_NOT_USE_THIS_LOCATOR = You can't use this LobLocator.
# *Cause: A NULL LobLocator cannot be used.
# *Action: Refrain from attempting this action.

332,smERR_ABORT_DROP_CPATH_NOT_YET_MOVED_CIMG_IN_CPATH= The checkpoint image '<0%s>' has not yet been moved from the checkpoint path '<1%s>'.
# *Cause: An attempt was made to drop a checkpoint path that contains a checkpoint image.
# *Action: Move the checkpoint image to another checkpoint path.

333,smERR_ABORT_DML_AFTER_INSERT_APPEND = This DML operation is not permitted.
# *Cause: An attempt was made to execute DML operations during Direct-Path INSERT.
# *Action: Execute DML operations after Direct-Path INSERT is Terminated.

334,smERR_ABORT_NOT_FOUND_LOGFILE = No log files could be found in the '<0%s>' directory.
# *Cause: The specified directory did not contain any log files.
# *Action: Check the specified directory.

335,smERR_ABORT_EXIST_ACTIVE_TRANS_IN_RECOV = Recovery failure. Active transactions exist.
# *Cause: All transactions that were commenced during the startup phase must be completed before the recovery phase.
# *Action: Check for and end all active transactions during the control phase.

336,smERR_ABORT_InitSizeExceedMaxSize = The INITSIZE of the data file exceeds the MAXSIZE of the data file.
# *Cause: The INITSIZE of the data file exceeds the MAXSIZE of the data file.
# *Action: Check the INITSIZE and MAXSIZE of the data file.

337,smERR_ABORT_InitSizePropExceedMaxSizeProp = The value of the <0%s> property exceeds the value of the <1%s> property.
# *Cause: The value of the INITSIZE property exceeds the value of the MAXSIZE property.
# *Action: Check the properties pertaining to the size of the data file.

338,smERR_ABORT_MaxSizePropExceedOSLimit = The value of the <0%s> property exceeds the OS file size limit. ( Max Size Property : <1%lu>, OS file size limit : <2%lu> )
# *Cause: The MAXSIZE property of the data file exceeds the OS file size limit.
# *Action: Set the MAXSIZE of the data file appropriately.

339,smERR_ABORT_InitSizeExceedOSLimit = The INITSIZE of the data file exceeds the OS file size limit. ( Request Init Size : <0%lu> pages, OS file size limit : <1%lu> pages )
# *Cause: The MAXSIZE of the data file exceeds the OS file size limit.
# *Action: Set the MAXSIZE of the data file appropriately.

340,smERR_ABORT_MaxSizeExceedOSLimit = The MAXSIZE of the data file exceeds the OS file size limit. ( Request Max Size : <0%lu> pages, OS file size limit : <1%lu> pages )
# *Cause: The MAXSIZE of the data file exceeds the OS file size limit.
# *Action: Set the MAXSIZE of the data file appropriately.

341,smERR_ABORT_InvalidFileSizeOnLogAnchor = The file size reported for the file <0%s> is invalid. ( Init Size : <1%lu> pages, Current Size : <2%lu> pages, Max Size : <3%lu> pages, Maximum File Size <4%lu> pages )
# *Cause: Invalid information about the size of a data file is stored in a loganchor file.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

342,smERR_ABORT_InvalidExtendFileSizeMaxSize = The requested size of the data file exceeds the MAXSIZE for the data file. ( Requested Size : <0%lu> pages, Max Size : <1%lu> pages )
# *Cause: The specified file size value is too big.
# *Action: Set the value for the file size appropriately.

343,smERR_ABORT_WRONG_ENTRY_ID = __MANUAL_BINDING_TRANSACTION_SEGMENT_BY_ENTRY_ID '<0%d>' exceeds the total entry count '<1%d>'.
# *Cause: The entry ID specified in __MANUAL_BINDING_TRANSACTION_SEGMENT_BY_ENTRY_ID is too high.
# *Action: Set the value of __MANUAL_BINDING_TRANSACTION_SEGMENT_BY_ENTRY_ID property to a value less than the total entry count.

344,smERR_ABORT_NOT_AVAILABLE_ENTRY = __MANUAL_BINDING_TRANSACTION_SEGMENT_BY_ENTRY_ID '<0%d>' has already been bound in another transaction.
# *Cause: The entry ID specified in __MANUAL_BINDING_TRANSACTION_SEGMENT_BY_ENTRY_ID has already been bound in another transaction.
# *Action: Retry after a moment, or set the __MANUAL_BINDING_TRANSACTION_SEGMENT_BY_ENTRY_ID property to another value and try again.

345, smERR_ABORT_DATA_PORT_INTERNAL_ERROR = Internal import/export error.
# *Cause: Unexpected internal error.
# *Action: Notify Altibase technical support staff.

346, smERR_ABORT_CORRUPTED_BLOCK = A block is corrupt. ( BlockID  <0%d> ).
# *Cause: A block appears to be corrupt.
# *Action: Check the DataPort file.

347, smERR_ABORT_VERSION_NOT_SUPPORTED = The DataPort file version is not supported. ( <0%d> > <1%d>  ).
# *Cause: The version of the DataPort file is too high.
# *Action: Check the version of the database and the DataPort file.

348, smERR_ABORT_CANT_OPEN_FILE = Cannot find the file. ( '<0%s>' ).
# *Cause: The DataPort file does not exist.
# *Action: Check the DataPort file name and try again.

349, smERR_ABORT_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE = The column-chaining threshold is greater than the maximum size(=BlockSize/2)( ColumnChainingThreshold:<0%d>, BlockSize:<1%d> ).
# *Cause: The column-chaining threshold is greater than the maximum size(=BlockSize/2).
# *Action: Set the column-chaining threshold to a value less than (BlockSize/2).

350, smERR_ABORT_CORRUPTED_HEADER = The DataPort file header is corrupt.
# *Cause: The Dataport file header appears to be corrupt.
# *Action: Check the DataPort file.

351,smERR_ABORT_CheckpointPathIsNullString = The length of the checkpoint path is zero.
# *Cause: The value specified for the checkpoint path is a null string.
# *Action: Provide a valid checkpoint path to be created.

352,smERR_ABORT_InvalidCheckpointPathABS = The checkpoint path is not an absolute path.
# *Cause: The checkpoint path is not an absolute path.
# *Action: Check the ALTIBASE_HOME environment variable for this account.

353,smERR_ABORT_InvalidCheckpointPathKeyWord = The checkpoint path contains special keywords or system reserved keywords.
# *Cause: The checkpoint path contains special keywords or system reserved keywords.
# *Action: Set the checkpoint path appropriately.

354,smERR_ABORT_TooLongCheckpointPath = The checkpoint path is too long <0%s> <1%s>.
# *Cause: The checkpoint path is too long.
# *Action: Specify a checkpoint path that is within the allowable length for a checkpoint path.

355,smERR_ABORT_TooLongFilePath = The total length of the file path and name is too long <0%s> <1%s>.
# *Cause: The total length of the file path and name is too long.
# *Action: Specify a file path and/or filename such that the total length of the file path and name is within the allowable length.

356,smERR_ABORT_AlreadyExistDBFiles = The data file(s) already exists.
# *Cause: The data file(s) already exists.
# *Action: Ensure that you have executed destroydb and try again.

357,smERR_ABORT_INTERNAL = The storage manager experienced an internal server error.
# *Cause: Internal server error.
# *Action: Check the error number from the trace log and contact Altibase’s Support Cente (http://support.altibase.com).

358,smERR_ABORT_TRANSACTION_TABLE_SIZE_IS_NOT_POWER_OF_TWO = TRANSACTION_TABLE_SIZE ['<0%d>'] is not a power of two.
# *Cause: The value of the TRANSACTION_TABLE_SIZE property is not a power of two(=2^n).
# *Action: Check the altibase.properties file and set the value of TRANSACTION_TABLE_SIZE to a power of two (2^n).

359,smERR_ABORT_DB_FILE_SIZE_EXCEEDS_LIMIT = The size of the DB file(<0%s>) exceeds the size specified in the MEM_MAX_DB_SIZE property.
# *Cause: The MEM_MAX_DB_SIZE property is set to a value that is less than the current size of the DB file.
# *Action: Check the altibase.properties file and increase the value of the MEM_MAX_DB_SIZE property

360,smERR_ABORT_LOG_FILE_MISSING=Non-continuous log file numbers. ( <0%s> ).
# *Cause: Either the server failed to remove an old log file, or a current log file has been lost for some unknown reason.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

361, smERR_ABORT_FAILURE_DURABILITY_AT_STARTUP = Restart recovery aborted to protect database durability.
# *Cause: A log file has been lost. If restart recovery continues, DB durability and DB objects may be broken.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

362, smERR_ABORT_FAILURE_DRDB_WAL_AT_STARTUP = Restart recovery aborted due to Write-Ahead-Logging failure.
# *Cause: The server cannot find a log file that is necessary for restoring a DRDB object. If restart recovery continues, DB consistency may be broken.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

363, smERR_ABORT_FAILURE_MRDB_WAL_AT_STARTUP = Restart recovery aborted due to Write-Ahead-Logging failure.
# *Cause: The server cannot find a log file that is necessary for restoring an MRDB object. If restart recovery continues, DB consistency may be broken.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

364, smERR_ABORT_INCONSISTENT_DB = Access blocked to prevent DB inconsistency from worsening.
# *Cause: A SQL statement attempted to access a portion of the DB that is already inconsistent.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

365, smERR_ABORT_INCONSISTENT_PAGE = A page is inconsistent. ( Current Space ID : <0%d>, Current Page ID : <1%d> )
# *Cause: The page is not consistent.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

366, smERR_ABORT_ERR_INCONSISTENT_DB_AND_LOG_BUFFER_TYPE = Could not perform emergency startup due to current LOG_BUFFER_TYPE setting.
# *Cause: The current log buffer type setting (=0) forcefully records all logs, including emergency logs. However, emergency logs must not be recorded.
# *Action: Set the value of the LOG_BUFFER_TYPE property to 1.

367, smERR_ABORT_LOGFILE_TOO_BIG_WITH_DIRECT_IO = The logfile is too big. ( <0%d> > <1%d> )
# *Cause: The size of the logfile exceeds the DIRECT I/O limitation.
# *Action: Verify that the logfile is smaller than the DIRECT I/O limitation, or set the value of the LOG_IO_TYPE property to 0.

#368,smERR_ABORT_smiCantRestartRemoteTableCursor = Unable to restart a remote table cursor
# *Cause: An attempt was made to restart a remote table cursor.
# *Action: Do not attempt to restart a remote table cursor.

369, smERR_ABORT_InvalidChangeTrackingFile = The change-tracking file is not valid. ( File Name :<0%s> )
# *Cause: The file was corrupted by an abnormal shutdown or some other unexpected activity.
# *Action: Disable and re-enable the change-tracking manger.

370, smERR_ABORT_ChangeTrackingState = Unexpected change-tracking manager state.
# *Cause: The change-tracking manager is already enabled or disabled.
# *Action: Check the state of the change-tracking manager.

371, smERR_ABORT_ErrUntilTag = Can`t perform media recovery at point of backup tag. ( Backup Tag Name :<0%s> )
# *Cause: An attempt was made to restore the database using an invalid backup tag.
# *Action: Restore the database using the correct backup tag.

372, smERR_ABORT_InvalidBackupInfoFile = The backup information file is not valid. ( File Name :<0%s> )
# *Cause: The file was corrupted by an abnormal shutdown or some other unexpected activity.
# *Action: Restore the backup information file from a recent backup.

373, smERR_ABORT_InvalidRestoreTime = Can`t restore database to specified point of time
# *Cause: No backup file predating the specified time exists.
# *Action: Restore the database to a more recent point in time.

374, smERR_ABORT_NotDefinedIncrementalBackupPath = No incremental backup path defined
# *Cause: No incremental backup path has been defined.
# *Action: Specify an incremental backup directory.

375, smERR_ABORT_AlreadyExistIncrementalBackupPath = Incremental backup path already exists (Dir Name :<0%s> )
# *Cause: The incremental backup path already exists.
# *Action: Change the incremental backup directory, or wait and try incremental backup again.

376, smERR_ABORT_BackupInfoState = Unexpected Backup Information Manager state
# *Cause: The Backup Information Manager is already enabled or disabled.
# *Action: Check the state of the Backup Information Manager.

377, smERR_ABORT_AlreadyExistPath = Directory already exists (Dir Name :<0%s> ).
# *Cause: The specified directory already exists.
# *Action: Delete or rename the existing directory, or specify a different directory.

378, smERR_ABORT_ThereIsNoDatabaseIncrementalBackup = There is no incremental database backup.
# *Cause: An incremental database backup has not been performed.
# *Action: Perform an incremental database backup before restoring the database.

379, smERR_ABORT_ThereIsNoIncrementalBackup = There is no incremental backup.
# *Cause: An incremental backup has not been performed.
# *Action: Perform an incremental backup before restoring the database.

380, smERR_ABORT_FailToCreateDirectory = Failed to create directory (Dir Name :<0%s> )
# *Cause: The specified directory could not be created.
# *Action: Check the directory path or permission.

#381,smERR_FATAL_NOT_AVAILABLE_533_DBFILE = Cannot use version 5.3.3 data files.
# *Cause: An attempt was made to use a version 5.3.3. data file with 5.3.3 version of the altibase binary.
# *Action: Please use version 5.3.3 of the altibase binary with this data file.

#382,smERR_ABORT_WrongArchiveMultiplexCount = ARCHIVE_MULTIPLEX_DIR(<0%d>) can only be set if LOG_FILE_GROUP_COUNT(<1%d>) is 1
# *Cause: One or more directories have been specified for the ARCHIVE_MULTIPLEX_DIR property, but the value of LOG_FILE_GROUP_COUNT is greater than 1.
# *Action: Set LOG_FILE_GROUP_COUNT to 1, or do not set ARCHIVE_MULTIPLEX_DIR.

#383,smERR_ABORT_WrongLogMultiplexCount = LOG_MULTIPLEX_DIR(<0%d>) can only be set if LOG_FILE_GROUP_COUNT(<1%d>) is 1
# *Cause: One or more directories have been specified for the LOG_MULTIPLEX_DIR property, but the value of LOG_FILE_GROUP_COUNT is greater than 1.
# *Action: Set LOG_FILE_GROUP_COUNT to 1, or do not set LOG_MULTIPLEX_DIR.

384,smERR_ABORT_DuplicateMultiplexDirPath = A multiplex directory path (<0%s>) has been specified more than once
# *Cause: Duplicate multiplex directory path settings exist in LOG_MULTIPLEX_DIR and/or ARCHIVE_MULTIPLEX_DIR.
# *Action: Check the LOG_MULTIPLEX_DIR and ARCHIVE_MULTIPLEX_DIR property settings and remove duplicate paths.

385,smERR_ABORT_WrongLogMultiplexDirCount = Number of LOG_MULTIPLEX_DIR(<0%d>) directories not equal to LOG_MULTIPLEX_COUNT(<1%d>)
# *Cause: The number of directories specified using the LOG_MULTIPLEX_DIR property is different from the value of LOG_MULTIPLEX_COUNT.
# *Action: Set the value of LOG_MULTIPLEX_COUNT to the number of directories specified using LOG_MULTIPLEX_DIR.

386,smERR_ABORT_WrongArchMultiplexDirCount = Number of ARCH_MULTIPLEX_DIR(<0%d>) directories not equal to ARCH_MULTIPLEX_COUNT(<1%d>)
# *Cause: The number of directories specified using the ARCH_MULTIPLEX_DIR property is different from the value of ARCH_MULTIPLEX_COUNT.
# *Action: Set the value of ARCH_MULTIPLEX_COUNT to the number of directories specified using ARCH_MULTIPLEX_DIR.

#387, smERR_ABORT_NOT_ENOUGH_NEXTENTSIZE = Insufficient page descriptor area in the temp table.
# *Cause: There is not enough room in the temporary table for the page descriptor information.
# *Action: Increase the value of the TEMP_MAX_PAGE_COUNT property.

388, smERR_ABORT_NOT_ENOUGH_WORKAREA = Insufficient free space in work area
# *Cause: There is not enough free space in the work area.
# *Action: Increase the value of the TOTAL_WA_SIZE property.

389, smERR_ABORT_INVALID_SORTAREASIZE = Insufficient sort area space
# *Cause: Cannot manage the page descriptor area due to the decrease in the sort area size.
# *Action: Increase the value of the SORT_AREA_SIZE property.

390, smERR_ABORT_INVALID_HASHAREASIZE = Insufficient hash area space
# *Cause: Cannot manage the page descriptor area due to the decrease in the hash area size.
# *Action: Increase the value of the HASH_AREA_SIZE property.

391,smERR_ABORT_TEMP_FLUSHER_STOPPED = Cannot sort or hash because all temp flushers are stopped.
# *Cause: The temporary flushers have stopped due to some unexpected problem.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

392,smERR_ABORT_ALL_INDEX_DISABLED = All Indexes are disabled.
# *Cause: The requested action could not be performed because all of the indexes for the table are disabled.
# *Action: Enable the index(es) and try again.

393,smERR_ABORT_PathIsNullString = The length of the path is zero.
# *Cause: The path is an empty string.
# *Action: Provide a valid path to be created.

394,smERR_ABORT_TablespaceLockUse = X or S tablespace lock is not allowed while the TABLESPACE_LOCK_ENABLE property is 0.
# *Cause: The user tried to execute a DDL command when the TABLESPACE_LOCK_ENABLE property was 0.
# *Action: Change the TABLESPACE_LOCK_ENABLE value to 1.

395,smERR_RETRY_Row_Retry = A record has already been updated.
# *Cause: Another transaction has already updated the same record.
# *Action: No action is necessary.

396,smERR_ABORT_sdsFlusherNotStarted = The Secondary flusher [<0%d>] was not started.
# *Cause: The flusher was not started.
# *Action: Check the secondary flusher status.

397,smERR_ABORT_sdsFlusherRunning = The Secondary flusher [<0%d>] is already running.
# *Cause: The flusher cannot be started because it is already running.
# *Action: Check the secondary flusher status.

398,smERR_ABORT_AllSecondaryFlushersStopped = You cannot run the DCL command because all secondary flushers are stopped.
# *Cause: All secondary flushers are stopped.
# *Action: Start some secondary flushers.

399,smERR_ABORT_PageMovedownStopped = Failed to movedown pages.
# *Cause: Cannot movedown page to the secondary buffer.
# *Action: Check the secondary buffer.

400,smERR_ABORT_PageMoveUpStopped = Failed to moveup a page.
# *Cause: Cannot moveup page to the secondary buffer.
# *Action: Check the secondary buffer.

401,smERR_FATAL_PageFlushStopped = Failed to flush pages. [err:%d]
# *Cause: The secondary flusher has stopped due to an unexpected problem.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

402,smERR_FATAL_PageReadStopped = Failed to read a page.
# *Cause: Cannot read page from the secondary buffer.
# *Action: Check the secondary buffer path.

403,smERR_ABORT_InvalidSecondaryBufferHdr = Invalid Secondary Buffer File file header
# *Cause: Invalid Secondary Buffer File header
# *Action: Copy a valid file to [SECONDARY_BUFFER_FILE_DIRECTORY]

404,smERR_ABORT_invalid_secondary_buffer_propperty = Invalid Secondary Buffer property. size=<0%d>, PATH = <1%s>
# *Cause: The SECONDARY_BUFFER_XXXX property value is invalid.
# *Action: Check the environment variable SECONDARY_BUFFER_XXXX.

405,smERR_ABORT_service_secondary_buffer_in_recv = Recovery failure. Secondary Buffer is serving.
# *Cause: Secondary Buffer service cannot be served before the recovery phase.
# *Action: Check the environment variable SECONDARY_BUFFER_XXXX.

406,smERR_ABORT_CannotCreateSecondaryBuffer = Unable to create Secondary Buffer.
# *Cause: A secondary file could not be created.
# *Action: Check the environment variable SECONDARY_BUFFER_XXXX.

#407,smERR_ABORT_Cannot_Execute_With_DisasterRecovery = <0%s> cannot be executed with Disaster Recovery.
# *Cause: Disaster Recovery is enabled
# *Action: Shut down the server and disable Disaster Recovery using the DR_ENABLE property.

#408,smERR_ABORT_Cannot_Set_Sync_Mode_Without_Fast_Unlock_Log_Alloc_Mutex = Cannot set DR mode to sync without Fast Unlock Log Alloc Mutex.
# *Cause: DR mode is set to sync without Fast Unlock Log Alloc Mutex.
# *Action: Set DR mode to sync with Fast unlock Log Alloc Mutex.

409,smERR_ABORT_Cannot_Perform_Level1_Backup = Cannot perform level 1 backup.
# *Cause: Level 0 backup does not exist.
# *Action: Perform a level 0 backup prior to executing a level 1 backup.

410,smERR_ABORT_UnableToExecuteAlterTable = Unable to execute ALTER TABLE on [ TableOID: <0%lu> ].
# *Cause: Another transaction is performing an ALTER TABLE statement on the table.
# *Action: Check whether another transaction is performing an ALTER TABLE statement on the table.

411,smERR_ABORT_TablespaceDoesNotExist = Tablespace[ID: %d] does not exist.
# *Cause: Tablespace which is written in the SCT_UPDATE_DRDB_CREATE_DBF log does not exist in the database.
# *Action: Restore with a valid backup file.

#412,smERR_ABORT_RemoteCallNotAvailable = The remote call is unavailable.
# *Cause: The cluster has not been configured.
# *Action: Configure the cluster first.

#413,smERR_ABORT_VOLATILE_TBS_WITH_DR = Disaster Recovery does not support volatile tablespaces.
# *Cause: Disaster Recovery does not support volatile tablespaces.
# *Action: Disable Disaster Recovery or drop the volatile tablespace.

414,smERR_ABORT_SDMOpenFailed = Unable to open SDM device. 
# *Cause: Device does not support Smart SSD features or the device path may be incorrectly specified.
# *Action: Check whether the device specified in the configuration file supports Smart SSD features and is correct.

#415,smERR_ABORT_INVALID_LOG = Invalid Log (LFG:<0%d>,FileNo:<1%d>,Offset:<2%d>)
# *Cause: Invalid log.
# *Action: Check the logfile.

416,smERR_ABORT_Maximum_Column_count_in_temptable = Too many columns in a temp table
# *Cause: Too many columns in a temptable
# *Action: Cannot use queries based on the disk temp table.

417,smERR_ABORT_TooManySlotIndiskTempTable = Too many slot in disk temp table.
# *Cause: Cannot manage the page descriptor area bacause value of the properties pertaining to the size of the disk temp table is incompatible.
# *Action: Increase the value of the SORT_AREA_SIZE/HASH_AREA_SIZE property or decrease the value of the TEMP_MAX_PAGE_COUNT

418,smERR_ABORT_NO_CALLBACK = The database link is not initialized.
# *Cause: No callback functions are registered for the remote table.
# *Action: Please check that the database link is enabled.

419,smERR_ABORT_Invalid_slot  = Invalid slot access.
# *Cause: Invalid slot access.
#*Action: Refrain from attempting this action.

420,smERR_ABORT_TooSmallDirectKeySize = Failed to create a direct key index because the column is larger than the available space in the index.
# *Cause: The column on which the direct key index is to be created is bigger than the maximum size set for the direct key index.
# *Action: Set a larger value for the MAXSIZE option of the direct key index.

421,smERR_ABORT_NonDirectKeyOption = The DIRECTKEY option is only supported for memory B-Tree indexes.
# *Cause: The DIRECTKEY option is only supported for memory B-Tree indexes.
# *Action: Create a memory B-Tree index or do not use the DIRECTKEY option for this index.

422,smERR_ABORT_InvalidDirectKeyMaxSize = The value of the MAXSIZE option exceeds the maximum limit.
# *Cause: The maximum size of the direct key index exceeds the limit.
# *Action: Check the maximum size of the direct key index and change it so that it is between 8 bytes and one-third of the memory B-Tree node.

423,smERR_ABORT_InvalidDataTypeInDirectKey = Direct key indexes do not support this data type.
# *Cause: The direct key index was used for an unsupported data type.
# *Action: Create direct key indexes only on CHAR, VARCHAR, NCHAR, and NVARCHAR columns.

424,smERR_ABORT_NoCompressionInDirectKey = Direct key indexes do not support compressed columns.
# *Cause: An attempt was made to create a direct key index on a compressed column.
# *Action: Either create a memory B-Tree index or do not use the DIRECTKEY option for this index.

425,smERR_ABORT_NoDirectKeyOnPartTable = Direct key indexes do not support partitioned tables.
# *Cause: An attempt was made to create a direct key index on a partitioned table.
# *Action: Either create a memory B-Tree index or do not use the DIRECTKEY option for this index.

426,smERR_ABORT_ReorgFail = A memory index is failed to reorganizate.
# *Cause: The system failed to lock a mutex.
# *Action: Retry the reorganization.

427,smERR_ABORT_TOO_MANY_TRAVERSAL = It takes too long to retrieve the index. (TableOID: <0%lu>, IndexID: <1%u>)
# *Cause: An index is corrupt.
# *Action: Drop and rebuild the index.

428,smERR_ABORT_LogFileSizeIsZero = OS return Log file size is zero. ( <0%s> ).
# *Cause: The log file prepare thread has been failed during a creation of the log file or OS error. 
# *Action: Check the log files size. If the size of log file exists zero, remove its below.

429,smERR_ABORT_InvalidDatafileHeader = The data file '<0%s>' has an invalid header. : DATABASE SID=<1%u>, FID=<2%u>, RedoLSN=control[<3%u>, <4%u>], [<5%u>, <6%u>], CreateLSN=control[<7%u>, <8%u>], [<9%u>, <10%u>], DBVer=<11%u>, CtrVer=<12%u>, FileVer=<13%u>
# *Cause: The data file creation LSN does not match one of the log anchor files.
# *Action: Verify the data file.

430,smERR_ABORT_InvalidDataFileCreateLSN = The create LSN (<0%u>, <1%u>) of the data file (<2%s>) is bigger than the restart redo LSN (<3%u>, <4%u>).
# *Cause: The data file is invalid.
# *Action: Verify that the data file was backed up correctly.

431,smERR_FATAL_InvalidLSNOffset = Invalid LSN Offset (File No=<0%u>, Offset=<1%u>)
# *Cause: LSN Offset is invalid.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center(http://support.altibase.com).

432,smERR_ABORT_NotFoundLog = Cannot find the log record (LSN >= <0%u>,<1%u> ) that is needed in the logfile (<2%s>).
# *Cause: The log file is invalid.
# *Action: Check the logfile.

433,smERR_ABORT_InvalidLog = Invalid Log (FileNo:<0%d>,Offset:<1%d>)
# *Cause: Invalid log.
# *Action: Check the logfile.

434,smERR_ABORT_InvalidBCB = The BCB does not exist or is not valid. ( SpaceID:<0%u>, PageID:<1%u> )
# *Cause: The BCB does not exist or is not valid.
# *Action: Please shut down and restart to rebuild BCB List.

435,smERR_ABORT_smnUniqueViolationInReplTrans = Unique violation caused conflict resolution in replication.
# *Cause: Unique violation caused conflict resolution on replication.
# *Action: Check the conflict message written on $ALTIBASE_HOME/trc/altibase_rp.log.

436, smERR_ABORT_NOT_SUPPORT_FALLOCATE = The operation is not supported by the filesystem(or kernel).
# *Cause: The filesystem(or kernel) containing the file does not support this operation.
# *Action: Set the LOG_CREATE_METHOD property value to 0 and restart the server.

437, smERR_ABORT_ERR_LOG_CONSISTENCY = Incomplete media recovery aborted due to log consistency failure. ( <0%s> ).
# *Cause: A log file has been lost. If Incomplete media recovery continues, data may be lost.
# *Action: Copy a valid log file to [LOG_DIR] or move log files that are not needed for recovery to another directory.

438, smERR_ABORT_Distributed_Aborted = A distributed deadlock situation has been detected. ( <0%s> )
# *Cause: A deadlock victim transaction has been stopped and terminated due to the deadlock resolution scheme.
# *Action: Re-execute the transaction.

439,smERR_ABORT_StatementTooOld = The records required for consistent reading are out of date. ( <0%s> )
# *Cause: The records required for consistent reading are out of date.
# *Action: Re-execute the transaction.

440,smERR_ABORT_INVALID_SCN = Invalid SCN(System Commit Number) : <0%lu>.
# *Cause: Internal error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

#441,smERR_ABORT_NOT_SUPPORT_DISK = Altibase sharding does not support statements on disk table spaces for global consistent transactions.
# *Cause: Altibase sharding does not support statements on disk table spaces for global consistent transactions.
# *Action: Use memory tablespace.

442,smERR_ABORT_INDOUBT_FETCH_TIMEOUT = The transaction has exceeded the in-doubt fetch timeout specified by the user due to a prepared transaction. (XID:<0%s>).
# *Cause: The statement attempts to access a resource locked by a global consistent transaction in a prepare state.
# *Action: Execute commit or abort command to finalize the global consistent transaction.

#
#  Server Internal Message
#

INTERNAL_TRACE_MESSAGE_BEGIN

SM_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
            "signed long =<%ld>\n unsigned long = <%lu>\n"
            "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>\n";

SM_TRC_DPAGE_WARNNING1 SM_TRC_STATE = "sState: <%u>\n";

#SM_TRC_DAGER_WARNNING = "Index PID=<%u>: Tablespace(<%u>) has been removed; Skip index-segment free\n";

#SM_TRC_DAGER_NEXTTSS = "next tail tss rid= <%u>, <%u>, <%u>\n";

#SM_TRC_DAGER_TAILTSS = "tail tss rid= <%u>, <%u>, <%u>\n";

#SM_TRC_DAGER_CALC_AGING_JOB_COUNT = "Total AgingJob Count At Startup = <%u> (  completed [<%u>], incomplete [<%u>] )\n";

#SM_TRC_BUFFER_WARNNING = "The buffer flush thread was aborted and restarted : "
#                         "because of insufficient memory or disk capacity or other problems\n";

SM_TRC_BUFFER_CORRUPTED_PAGE = "Recovered corrupted page [TBSID:<%u>, PID:<%u>].\n";

#SM_TRC_BUFFER_WARNNING2 = "Buffer pool reached the maximum : "
#                          "may be need to increase the buffer pool size\n";

#SM_TRC_DEBUG_INVALID_ROLLBACK_RID = "Invalid rollback RID: <%u>,<%u>\n";

#SM_TRC_DEBUG_CURRENT_ROW_RID = "Current row RID: <%u>,<%u>,<%u>\n";

#SM_TRC_DEBUG_INVALID_UPDATE_COL_TYPE_LIST = "Invalid update column type list: <%u>,<%u>\n";

SM_TRC_DISK_FILE_CREATE = "[FILE-CREATE] <%s> \n";

SM_TRC_DISK_FILE_EXTEND = "[FILE-EXTEND] [<%lu> --> <%lu>] <%s> \n";

SM_TRC_DISK_FILE_TRUNCATE = "[FILE-TRUNCATE] [<%lu> --> <%lu>] <%s> \n";

SM_TRC_DISK_FILE_EXTENDMODE = "[FILE-ATTR-EXTENDMODE] [<%u> --> <%u>] <%s> \n";

#SM_TRC_DISK_FILE_MAXSIZE = "[FILE-ATTR-MAXSIZE] [<%lu> --> <%lu>] <%s> \n";

#SM_TRC_DISK_FILE_NEXTSIZE = "[FILE-ATTR-NEXTSIZE] [<%lu> --> <%lu>] <%s> \n";

SM_TRC_DISK_FILE_CURRSIZE = "[FILE-ATTR-CURRSIZE] [<%lu> --> <%lu>] <%s> \n";

SM_TRC_DISK_FILE_INITSIZE = "[FILE-ATTR-INITSIZE] [<%lu> --> <%lu>] <%s> \n";

SM_TRC_DISK_FILE_RENAME = "[FILE-ATTR-RENAME] [<%s> --> <%s>] \n";

SM_TRC_DISK_FILE_BACKUP_ERROR = "CompleteFileBackup  error, TBSID(<%u>), FILE_ID(<%u>) in sddDiskMgr::abortBackupTableSpace \n";

SM_TRC_DISK_FILE_SYNC_ERROR = "Sync error.... while sddDiskMgr::updateDataFileState \n";

#SM_TRC_DPAGE_PAGE_SIZE_INVALID = "Ext dir page unit size is invalid : \n"
#                                 "  Ext Dir Page's Unit Size = <%u>, Calc Unit Size = <%u>\n";

#SM_TRC_DPAGE_PAGE_DESC_COUNT_INVALID = "Ext dir page desc count is invalid : \n"
#                                       "  Ext Dir Page's Desc Count = <%u>, Calc Desc Count = <%u>\n";

#SM_TRC_DPAGE_PAGE_DESC_FREE_INVALID = "Ext dir page free limit is invalid : \n"
#                                      "  Ext Dir Page's Free Limit = <%u>, Calc Free Limit = <%u>\n";

#SM_TRC_DPAGE_EXT_DIR_STATE_INVALID = "Ext desc state is invalid : Ext dir state = <%u>\n";

#SM_TRC_DPAGE_EXT_DESC_LIST_INVALID = "Ext desc list is invalid : pred RID = <%u>, next RID = <%u>\n";

#SM_TRC_DPAGE_EXT_DESC_STATE_INVALID = "Ext desc state is invalid (there are free pages) : \n"
#                                      "  Ext desc state = <%u>, Free page index = <%u>\n";

SM_TRC_DPAGE_WARNNING1 = "[SegType: <%u>] [PID:<%u>] The tablespace (ID:<%u>) has been removed; Skip segment initialization\n";

SM_TRC_DPAGE_WARNNING2 = "[PID:<%u>] The tablespace (ID:<%u>) has been removed; Skip action of freeing table-segments\n";

SM_TRC_DPAGE_WARNNING3 = "[PID:<%u>] The tablespace (ID:<%u>) has been removed; Skip action of freeing index-segments\n";

SM_TRC_DPAGE_WARNNING4 = "[PID:<%u>] The tablespace (ID:<%u>) has been offline; Skip action of freeing table-segments\n";

SM_TRC_DPAGE_WARNNING5 = "[PID:<%u>] The tablespace (ID:<%u>) has been offline; Skip action of freeing index-segments\n";

SM_TRC_DPAGE_PHY_ERROR1 = "The page space id is invalid: request space id=<%u>, read page space id=<%u> from DB\n";

SM_TRC_DPAGE_PHY_ERROR2 = "The page id is invalid: request page id=<%u>, read page id=<%u> from DB\n";

SM_TRC_DPAGE_PHY_ERROR3 = "The total free size of the page is invalid: page id=<%u>:<%u>, page size=<%u>, total free size=<%u>, free offset=<%u>\n";

SM_TRC_DPAGE_PHY_ERROR4 = "The page free offset is invalid: page id=<%u>:<%u>, page size=<%u>, total free size=<%u>, free offset=<%u>\n";

SM_TRC_DPAGE_PHY_ERROR5 = "The page type is invalid: page id=<%u>:<%u>, page type=<%u>\n";

#SM_TRC_DPAGE_SEGMENT_ERROR = "Seg dir page state is invalid: original state=<%u>, current state=<%u>\n";

#SM_TRC_DPAGE_SEGMENT_ERROR2 = "Segment desc type is invalid : <%u>\n";

#SM_TRC_DPAGE_SEGMENT_ERROR3 = "Segment desc state is invalid : <%u>\n";

#SM_TRC_DPAGE_SEGMENT_ERROR4 = "Segment desc state must be a used state : state=<%u>\n";

#SM_TRC_DPAGE_SEGMENT_ERROR5 = "Ext full list count of segment desc is invalid : list count=<%u>, real count=<%u>\n";

#SM_TRC_DPAGE_META_ERROR = "Page list of table meta page is invalid : page list head pid=<%u>, used page limit=<%u>\n";

SM_TRC_DPAGE_TABLESPACE_ERROR = "0th page has to be meta type : <%u>\n";

#SM_TRC_DPAGE_TABLESPACE_ERROR2 = "0th page ext free list is invalid : page list count=<%u>, real list count=<%u>\n";

SM_TRC_MTXSTACK_SIZE_OVERFLOW1 = "sdrMtxStack size overflow; Dumping mutex latch items...\n";

SM_TRC_MTXSTACK_SIZE_OVERFLOW2 = "[<%u>] <%u>:<%u>, SDR_MTX_PAGE_X\n";

SM_TRC_MTXSTACK_SIZE_OVERFLOW3 = "[<%u>] <%u>:<%u>, SDR_MTX_PAGE_S\n";

SM_TRC_MTXSTACK_SIZE_OVERFLOW4 = "[<%u>] <%u>:<%u>, SDR_MTX_PAGE_NO\n";

SM_TRC_MTXSTACK_SIZE_OVERFLOW5 = "[<%u>] (LATCH_OBJ, SDR_MTX_LATCH_X)\n";

SM_TRC_MTXSTACK_SIZE_OVERFLOW6 = "[<%u>] (LATCH_OBJ, SDR_MTX_LATCH_S)\n";

SM_TRC_MTXSTACK_SIZE_OVERFLOW7 = "[<%u>] (LATCH_OBJ, SDR_MTX_MUTEX_X)\n";

SM_TRC_DRECOVER_WARNNING1 = "Tablespace (ID:<%u>) has been removed; Skip redo\n";

#SM_TRC_DRECOVER_IGNORE_CORRUPTED_PAGE = "Ignore corrupt page : TBS=<%s>, PageID=<%u>\n";

SM_TRC_DRECOVER_ADD_CORRUPTED_PAGE = "Detect and add corrupt page to hash : TBS=<%s>, PageID=<%u>\n";

SM_TRC_DRECOVER_DEL_CORRUPTED_PAGE = "Del corrupt page from hash : TBS=<%s>, PageID=<%u>\n";

SM_TRC_DRECOVER_DEL_CORRUPTED_PAGE_NOTALLOC = "Del corrupt page from hash because of unallocated page : TBS=<%s>, PageID=<%u>\n";

SM_TRC_DRECOVER_PAGE_IS_CORRUPTED = "Page is corrupted : TBS=<%s>, PageID=<%u>\n";

SM_TRC_DRECOVER_GROUPHDR_IS_CORRUPTED = "Group Hdr Page is corrupted : TBS=<%s> GHPID=<%u> PageID=<%u>\n";

SM_TRC_DRECOVER_OVERWRITE_CORRUPTED_PAGE = "Overwrite corrupt page to recover : TBS=<%s>, PageID=<%u>, LogType=<%u>\n";

SM_TRC_DRECOVER_REDO_LOG_HASHING_FATAL1 = "Invalid page id in hashing redo log: <%s>, spid=<%u>,<%u>\n";

SM_TRC_DRECOVER_REDO_LOG_HASHING_FATAL2 = "Page range = <%u> ~ <%u>\n";

SM_TRC_DRECOVER_ADD_DATA_FILE1 = "Add recovery data file: <%s>, SFID=<%u>,<%u>\n";

SM_TRC_DRECOVER_ADD_DATA_FILE2 = "Page range = <%u> ~ <%u>\n";

SM_TRC_DRECOVER_INVALID_LOG_OFFSET = "Log Offset(<%u>) to be read is larger than Max Log Offset(<%u>).\n"
                                     "Property LOG_FILE_SIZE : <%u>\n";

SM_TRC_DRECOVER_NEED_MORE_LOGFILE = "Recovery failure, need more logfile\n"
                                    "Latest Redo LSN [<%u>,<%u>] - DBFile <%s> need Redo Log [<%u>,<%u>]\n";

SM_TRC_MAGER_WARNNING = "The delete thread is aborted and restarted : "
                        "because of insufficient memory or disk capacity or other problems\n";

SM_TRC_MAGER_WARNNING2 = "The logical ager is aborted and restarted : "
                         "because of insufficient memory or disk capacity or other problems\n";

SM_TRC_MRECORD_INVALID_COL_SIZE = "Table OID:<%lu>, smcTable::<%u>, CO:<%u>, SZ:<%u>\n";

SM_TRC_MRECORD_FILE_UNLINK = "[FILE-UNLINK] <%s> \n";

SM_TRC_INTERFACE_ARCHIVE_THREAD_START = "[ALTER SYSTEM] Archive Thread Start...\n";

SM_TRC_INTERFACE_ARCHIVE_THREAD_SUCCESS = "[SUCCESS]\n";

SM_TRC_INTERFACE_ARCHIVE_THREAD_FAILURE = "[FAILURE]\n";

SM_TRC_INTERFACE_ARCHIVE_THREAD_SHUTDOWN = "[ALTER SYSTEM] Archive Thread Shutdown...\n";

#SM_TRC_INTERFACE_LOG_FATAL1 = "[ERR UPT AFTER] LSN(<%u>,<%u>), SIZE=<%u>, CCNT=<%u>, CID=<%u>, CLEN=<%u>\n";

SM_TRC_INTERFACE_FILE_CREATE = "[FILE-CREATE] <%s> \n";

SM_TRC_INTERFACE_FILE_UNLINK_ERROR = "[FILE-UNLINK] <%s> unlink Error(errno=<%u>) \n";

SM_TRC_INTERFACE_WAITFOR_FATAL1 = "Abnormal system shutdown because of insufficient disk space!!\n";

SM_TRC_INTERFACE_WAITFOR_FATAL2 = "Internal DB File write error : Check Disk Space\n";

SM_TRC_MEMORY_DUMP_MEMBASE1 = "Membase ---##############################\n";

SM_TRC_MEMORY_DUMP_MEMBASE2 = "DBName: <%s>\n";

SM_TRC_MEMORY_DUMP_MEMBASE3 = "mProductSignature: <%s>\n";

SM_TRC_MEMORY_DUMP_MEMBASE4 = "mDBFileSignature: <%s>\n";

SM_TRC_MEMORY_DUMP_MEMBASE5 = "mVersionID: <%u>\n";

SM_TRC_MEMORY_DUMP_MEMBASE6 = "mCompileBit: <%u>\n";

SM_TRC_MEMORY_DUMP_MEMBASE7 = "mBigEndian: <%u>\n";

SM_TRC_MEMORY_DUMP_MEMBASE8 = "mLogSize: <%lu>\n";

SM_TRC_MEMORY_DUMP_MEMBASE9 = "mDBFilePageCount: <%lu>\n";

SM_TRC_MEMORY_DUMP_MEMBASE10 = "mTransactionTBLSize: <%u>\n";

SM_TRC_MEMORY_DUMP_MEMBASE11 = "mDBCharSet: <%s>\n";

SM_TRC_MEMORY_DUMP_MEMBASE12 = "mNationalCharSet: <%s>\n";

SM_TRC_MEMORY_DUMP_MEMBASE13 = "Backup Membase ---##############################\n";

SM_TRC_MEMORY_LOADING_DATAFILE_FATAL = "Error loading DataFile.\n";

SM_TRC_MEMORY_LOGANCHOR_RECREATE = "[Warning] : loganchor file does not exist => recreate loganchor files\n";

SM_TRC_MEMORY_PCH_ARRAY_NULL1 = "PCH Array is NULL. Page ID : <%lu>\n";

SM_TRC_MEMORY_PCH_ARRAY_NULL2 = "Maximum Number of Pages in DB : <%lu>\n";

SM_TRC_MEMORY_PCH_ARRAY_NULL3 = "Current Allocated Pages in DB : <%lu>\n";

SM_TRC_MEMORY_PCH_ARRAY_NULL4 = "The page#<%lu> is a free page\n";

SM_TRC_MEMORY_PCH_ARRAY_NULL5 = "The page#<%lu> is allocated to a table\n";

SM_TRC_MEMORY_PCH_ARRAY_NULL6 = "Failed to check whether the page is a free page\n";

SM_TRC_MEMORY_SCN_SYNC_INTERVAL_FATAL = "Invalid System SCN Sync Interval: <%lu>\n";

SM_TRC_MEMORY_INVALID_SCN = "Invalid System SCN: <%lu>, Update SCN: <%lu>\n";

SM_TRC_MEMORY_FILE_CREATE = "[FILE-CREATE] <%s> \n";

SM_TRC_MEMORY_AIO_FATAL = "AIO Failed: errno=<%u>\n";

SM_TRC_MEMORY_DB_SIGNATURE_FATAL1 = "DB Signature on SHM  : <%s>\n";

SM_TRC_MEMORY_DB_SIGNATURE_FATAL2 = "DB Signature on DISK : <%s>\n";

SM_TRC_MEMORY_INVALID_PID_IN_PAGEHEAD1 = "Invalid PageID in Page Header\n";
SM_TRC_MEMORY_INVALID_PID_IN_PAGEHEAD2 = "Expected : <%lu>\n";
SM_TRC_MEMORY_INVALID_PID_IN_PAGEHEAD3 = "Page ID in Page Header : <%lu>\n";


SM_TRC_MINDEX_FILE_CREATE = "[FILE-CREATE] <%s> \n";

SM_TRC_MINDEX_INDEX_TYPE_FATAL = "Invalid Index Type(<%u>)\n";

SM_TRC_MINDEX_INDEX_INFO = "TRANS ID:<%u> File:<%s>, Line<%u>: CUR PID:<%lu>, HEAD:<%lu> TAIL:<%lu>\n";

SM_TRC_DINDEX_INDEX_SEG_FREE = "Index segment is free!! Skip table (<%lu>), Index(<%lu>)\n";

#SM_TRC_DINDEX_DUMP_LEFT_MOST = "Dump <%u> 's leftmost child <%u>\n";

#SM_TRC_DINDEX_DUMP_RIGHT = "Dump <%u> 's <%u>th child to the right <%u> \n";

#SM_TRC_DINDEX_DUMP_NODE1 = "=== leaf node (<%u>) dump begin ==\n";

#SM_TRC_DINDEX_DUMP_NODE2 = "=== internal node (<%u>) dump begin ==\n";

#SM_TRC_DINDEX_DUMP_NODE3 = "<%s>  <%s>  <%u> \n";

#SM_TRC_DINDEX_DUMP_NODE4 = "=== node dump (<%u>) end ==\n";

#SM_TRC_DINDEX_TEMP_HASH_CREATE_FATAL = "Temp hash table create Error = space id <%u>\n";

SM_TRC_MPAGE_ALLOC_SLOT_FATAL1 = "== smpFixedPageList::allocSlotForTempTableHdr ==\n";

SM_TRC_MPAGE_ALLOC_SLOT_FATAL2 = "ERROR : sCurFreeSlotHeader->mUsedFlag != SMP_FREE_SLOT";

SM_TRC_MPAGE_ALLOC_SLOT_FATAL3 = "Page ID: <%lu>, Slot OID: <%lu>\n";

#SM_TRC_MPAGE_ALLOC_SLOT_FATAL4 = "== smpFixedPageList::allocSlot ==\n";

#SM_TRC_MPAGE_FREE_SLOT_FATAL1 = "== smpFixedPageList::setFreeSlot ==\n";

#SM_TRC_MPAGE_FREE_SLOT_FATAL2 = "ERROR : sCurSlotHeader->mUsedFlag != SMP_USED_SLOT\n";

SM_TRC_MPAGE_INVALID_SCN = "RID:<%vu>>, Row SCN:<%lu>, System SCN:<%lu>\n";

SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST1 = "Invalid Free Slot List Detected. Page #<%lu>\n";

SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST2 = "Free Slot Count on Page ==> <%lu>\n";

SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST3 = "Free Slot Count on List ==> <%lu>\n";

SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST4 = "Free Slot Head on Page ==> <%vx>\n";

SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST5 = "Free Slot Tail on Page ==> <%vx>\n";

SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST6 = "Free Slot Tail on List ==> <%vx>\n";

#SM_TRC_MPAGE_WARNNING = "Undo Buffer pool reached the maximum : "
#                        "may be need to increase the Undo buffer pool size\n";

SM_TRC_MPAGE_DUMP_VAR_COL_HEAD = "Slot Next OID: <%lu>, Slot Length: <%u>, Slot Flag: <%u>\n";

SM_TRC_MPAGE_REMOVE_VAR_PAGE_FATAL1 = "== smpVarPageList::removeSlotFromFreeSlotList ==";

SM_TRC_MPAGE_REMOVE_VAR_PAGE_FATAL2 = "ERROR : mUsedFlag != SMP_FREE_SLOT";

SM_TRC_MRECOVERY_ARCH_WARNING = "Log archive thread not initialized, so never performed archive log\n";

SM_TRC_MRECOVERY_ARCH_REBUILD_LOGFILE = "Rebuild archive logfile list (Total Count <%u>)...[SUCCESS]\n";

SM_TRC_MRECOVERY_ARCH_WARNING2 = "Log archive directory does not exist, so never performed archive log\n";

SM_TRC_MRECOVERY_ARCH_WARNING3 = "The archive thread was aborted and restarted : errno (<%u>) "
                                 "because of insufficient memory or disk capacity or other problems\n";

SM_TRC_MRECOVERY_ARCH_FATAL1 = "Insufficient disk space, cannot make logfile\n";

SM_TRC_MRECOVERY_ARCH_FATAL2 = "Archive Log Error: Check Disk Space\n";

SM_TRC_MRECOVERY_ARCH_ABORT1 = "Disk Space was insufficient during archive log backup. Check Space!!\n"
                               "Skip archiveing logfile(<%s>)";

SM_TRC_MRECOVERY_ARCH_ABORT2 = "Archive LogFile backup has failed!!\n";

SM_TRC_MRECOVERY_ARCH_WAITING = "Waiting logfile<%u> to archive\n";

SM_TRC_MRECOVERY_BACKUP_ABORT = "Log anchor file backup has failed!!\n";

SM_TRC_MRECOVERY_BACKUP_START = "Starting TableSpace-Level Backup\n";

SM_TRC_MRECOVERY_BACKUP_END = "TableSpace-Level Backup Completed [SUCCESS]\n";

SM_TRC_MRECOVERY_BACKUP_FAIL = "TableSpace-Level Backup Incomplete [FAILURE]\n";

SM_TRC_MRECOVERY_BACKUP_INFO1 = "MEMORY TABLESPACE DATAFILE <%s> BACKUP TO <%s>\n";

SM_TRC_MRECOVERY_BACKUP_ABORT1 = "Disk Space is insufficient during online backup writing. Check Space!!\n";

SM_TRC_MRECOVERY_BACKUP_ABORT2 = "Memory tablespace datafile <%s> cp file fail!\n";

SM_TRC_MRECOVERY_BACKUP_ABORT3 = "Memory internal file backup <%s> cp fail\n";

SM_TRC_MRECOVERY_BACKUP_ABORT4 = "During backup, session killed ... so backup aborted !! \n";

SM_TRC_MRECOVERY_BACKUP_INFO2 = "DISK TABLESPACE <%s> DATABASE <%s> BACKUP TO <%s>\n";

SM_TRC_MRECOVERY_BACKUP_ABORT5 = "Disk tablespace datafile <%s> cp file fail!\n";

SM_TRC_MRECOVERY_BACKUP_START2 = "Starting Database-level Backup\n";

SM_TRC_MRECOVERY_BACKUP_END2 = "Database-Level Backup Completed [SUCCESS]\n";

SM_TRC_MRECOVERY_BACKUP_FAIL2 = "Database-Level Backup Incomplete [FAILURE]\n";

SM_TRC_MRECOVERY_BACKUP_SWITCH_START = "Alter system switch log begin ...\n";

SM_TRC_MRECOVERY_BACKUP_SWITCH_END = "Alter system switch log completed...\n";

SM_TRC_MRECOVERY_BACKUP_SWITCH_FAIL = "Alter system switch log failure !!!  \n";

SM_TRC_MRECOVERY_BACKUP_TABLESPACE_BEGIN = "Alter tablespace tbsid (<%u>) begin backup\n";

SM_TRC_MRECOVERY_BACKUP_TABLESPACE_END = "Alter tablespace tbsid (<%u>) end backup\n";

SM_TRC_MRECOVERY_BACKUP_DBFILE_MISMATCH = "The memory tablespace file (<%u>,<%u>) exists in memory but not in disk\n";

SM_TRC_MRECOVERY_CHKP_DISABLE_THREAD = "Disable checkpoint thread...\n";

SM_TRC_MRECOVERY_CHKP_SLEEP_THREAD =  "Sleep checkpoint thread ( next time : <%u>-<%u>-<%u> <%u>:<%u>:<%u> )\n";

SM_TRC_MRECOVERY_CHKP_WARNING = "The checkpoint thread was aborted and restarted : "
                                "because of insufficient memory or disk capacity or other problems.\n";

#SM_TRC_MRECOVERY_CHKP_AIO_INFO = "  =>> CHECK_AIO=<%u>\n";

#SM_TRC_MRECOVERY_CHKP_DUMP_BEGIN = "######### Dirty Page List Begin #########\n";

#SM_TRC_MRECOVERY_CHKP_DUMP_END = "########## Dirty Page List End #########\n";

SM_TRC_MRECOVERY_DIRTY_PAGE_LIST_FATAL1 = "Insufficient disk space, cannot write page\n";

SM_TRC_MRECOVERY_DIRTY_PAGE_LIST_WARNNING = "DB page write error: Check Disk Space\n";

#SM_TRC_MRECOVERY_LFGMGR_LFGID = "The Original Redo LSN : [<%u>,<%u>]\n";

#SM_TRC_MRECOVERY_LFGMGR_BACKWARD_SCANNING =  "*********************************************************\n"
#                                             "Starting Backward File Scanning...\n";

#SM_TRC_MRECOVERY_LFGMGR_LFGID2 = "LFGID=<%u>\n";

#SM_TRC_MRECOVERY_LFGMGR_BACKWARD_SCANNING2 = "Staring Backward Scanning From FileNo #<%u>\n";

#SM_TRC_MRECOVERY_LFGMGR_SKIP_FORWARD_SCANNING = "No more files found. Will skip forward scan. \n"
#                                                "The new redo LSN is [<%u>,<%u>]\n";

#SM_TRC_MRECOVERY_LFGMGR_EMPTY_LOGFILE = "An empty LogFile detected at [<%u>,<%u>]\n"
#                                        "The record will be ignored and backward scan will continue.\n";

SM_TRC_MRECOVERY_LFTHREAD_SYNC_LOGFILE_LIST = "======== sync Log File List =============\n";

SM_TRC_MRECOVERY_LFTHREAD_SYNC_LOGFILE_NO = "logfile = <%u>\n";

SM_TRC_MRECOVERY_LFTHREAD_INSUFFICIENT_MEMORY_WARNNING = "The log file thread is aborted and restarted : "
                                         "because of insufficient memory or disk capacity or other problems\n";

SM_TRC_MRECOVERY_LFTHREAD_TOTAL_SYNCED_LOGFILE_COUNT = "Logfile sync thread total synced <%u> logfile(s) at one time.\n";

SM_TRC_MRECOVERY_LOGANC_BACKUP_START =  "Starting LogAnchor Backup\n";

SM_TRC_MRECOVERY_LOGANC_BACKUP_TO = "<%s> BACKUP TO <%s> \n";

SM_TRC_MRECOVERY_LOGANC_BACKUP_END = "LogAnchor Backup Completed [SUCCESS]\n";

SM_TRC_MRECOVERY_LOGANC_BACKUP_FAIL = "LogAnchor Backup Incomplete [FAILURE]\n";

SM_TRC_MRECOVERY_LOGANC_BACKUP_ABORT = "While backup, session killed ... so backup aborted !! \n";

#SM_TRC_MRECOVERY_LOGFILE_MMAP_ERROR = "Failed to invoke the mmap() system function. (errno:<%u>): But retry...\n";

SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG1 = "Recovery failure...\n";

SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG2 = "Log type mismatch : head=<%u>, tail=<%u>\n";

SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG3 = "Retry to read log file...\n";

SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG4 = "Retrying...fails!\n";

SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG5 = "Log type mismatch too: head=<%u>, tail=<%u>\n";

SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG6 = "Retrying...success!\n";

SM_TRC_MRECOVERY_LOGFILE_INVALID_MAGIC = "A invalid magic number is found [FILE_NO=<%u>, OFFSET=<%u>]: log head magic=<%u>, calculated magic=<%u>\n";

#SM_TRC_MRECOVERY_LOGFILE_APPEND_ERROR = "Invalid log size and offset.\n"
#"Log size to be written          = <%u>\n"
#"Log buffer offset to be written = <%u>\n"
#"Current log buffer offset       = <%u>\n";
 
#SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG_SIZE = "Invalid log size and offset.\n"
#"Log size to be written    = <%u>\n"
#"Log buffer free size      = <%u>\n"
#"Current log buffer offset = <%u>\n";

SM_TRC_MRECOVERY_LOGFILEGROUP_INVALID_LOGTYPE = "Log Type:[H:<%u>,T:<%u>], LSN:[<%u>,<%u>]\n";

SM_TRC_MRECOVERY_LOGFILEMGR_INSUFFICIENT_MEMORY = "The log file manager thread was aborted and restarted : "
                        "because of insufficient memory or disk capacity or other problems\n";

SM_TRC_MRECOVERY_LOGFILEMGR_CANNOT_REMOVE_LOGFILE = "Can't Remove <%s> In Checkpoint(errno=<%u>)\n";

SM_TRC_MRECOVERY_RECOVERYMGR_REDOALL_START = "Starting redoall-pass[<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_SKIP_REDO_LSN = "Skip redo LSN[<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_REDOALL_SUCCESS = "Success redoall-pass[<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_UNDOALL_START = "Starting undoall-pass\n";

SM_TRC_MRECOVERY_RECOVERYMGR_UNDO_TRANS_ID = "undo transaction id [<%u>] [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_UNDOALL_SUCCESS = "undoall-pass success\n";

SM_TRC_MRECOVERY_RECOVERYMGR_UPDATE_ARCH_MODE = "[ALTER DATABASE] <%s>";

SM_TRC_MRECOVERY_RECOVERYMGR_UPDATE_ARCH_MODE_SUCCESS = "[SUCCESS]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_UPDATE_ARCH_MODE_FAILURE = "[FAILURE]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_DRDB_START = "[DRDB-CHECKPOINT-BEGIN]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_DRDB_SUMMARY = "[DRDB-CHECKPOINT-summary] oldest LSN [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_DRDB_END = "[DRDB-CHECKPOINT-END]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_STEP0 = "[CHECKPOINT-step0] Dirty pages from DiskBufferPool\n";

#SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_STEP0_1 = "[Request Synced LSN=<%u>,<%u>]\n";
#SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_STEP0_2 = "[FLU-DirtyPageCount=<%u>]\n";

#SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_STEP1 = "[CHECKPOINT-step1] Synchronize Disk TableSpaces with LogAnchor\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP1 = "[CHECKPOINT BY SYSTEM]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP2 = "[CHECKPOINT BY LOGFILE SWITCH COUNT(<%u>)]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP3 = "[CHECKPOINT BY TIME(<%u> sec)]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP4 = "[CHECKPOINT BY USER]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP5 = "[CHECKPOINT-BEGIN]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP6 = "[CHECKPOINT-step2] Write BeginChkpt Log [<%u>,<%u>]\n"
                                     "Active Tx Recovery LSN [<%u>,<%u>]\n"
                                     "Disk Buffer Oldest LSN [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP7 = "[CHECKPOINT-step5] Write End_Chkpt Log [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP8 = "[CHECKPOINT-summary] "
                "BeginChkptLSN=[<%u>,<%u>], EndChkptLSN=[<%u>,<%u>], DiskRecLSN=[<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP9 = "Minimum LSN = [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP10 = "[CHECKPOINT-END]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP11 = "[CHECKPOINT-step6] Sync Log File\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP12 = "[CHECKPOINT-step7] Check LogFiles That Is Not Needed\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP13 = "Replication MinLSN [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP14 = "[CHECKPOINT-step7] Skip (Check LogFiles that are not Needed)\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP15 = "[CHECKPOINT-step8] Update and Flush Log Anchor\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP16 = "[CHECKPOINT-step9] Remove Online Log File\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP17 = "Remove Online Log File[<%u> ~ <%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP18 = "[CHECKPOINT-step9] Remove Online Log File[None]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP19 = "[CHECKPOINT-step9] Remove Online Log File Skip\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP20 = "[CHECKPOINT-FAILURE]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_REBUILD_ARCHLOG_LIST = "Rebuild archivelog list (total count <%u>)...SUCCESS\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP21 = "[CHECKPOINT-skip step0,1] Because some tablespace already performed backup.\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP22 = "[CHECKPOINT-step3] Flush Dirty Page(s)\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP23 = "[PRE-DirtyPageCount=<%lu>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_BEGIN_SYNCLSN="    + Begin Sync For All-LFG - Request LSN [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_END_SYNCLSN="    + End Sync For All-LFG\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP24 = "[NEW-DirtyPageCount=<%lu>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP25 = "[DUP-DirtyPageCount=<%lu>]\n";

SM_TRC_MRECOVERY_CHKP_FLUSH_SYNC_BEGIN = "    + Begin Bulk DB Sync (Prop.<%u>) - Write <%u> pages.\n";

SM_TRC_MRECOVERY_CHKP_FLUSH_SYNC_END = "    + End Bulk DB Sync.\n\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP26 = "[FLU-DirtyPageCount=<%lu>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP27 = "[REM-DirtyPageCount=<%lu>]\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP28 = "[CHECKPOINT-step4] sync Database File\n";

SM_TRC_MRECOVERY_RECOVERYMGR_CHKP29 = "[BUFFERMGR=[ <%u>, <%u> ]\n";

SM_TRC_MRECOVERY_CHECK_DB_SID_FID = "[CHECK DATABASE SID=<%u>, FID=<%u>]\n";

SM_TRC_MRECOVERY_CHECK_DISK_RLSN = "RedoLSN=control[<%u>,<%u>], [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_CHECK_DISK_CLSN = "CreateLSN=control[<%u>,<%u>], [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_CHECK_MEMORY_RLSN = "RedoLSN=control[<%u>,<%u>], [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_CHECK_MEMORY_CLSN = "CreateLSN=control[<%u>,<%u>], [<%u>,<%u>]\n";

SM_TRC_MRECOVERY_CHECK_DB_SUCCESS = "[CHECK DATABASE-SUCCESS]\n";

SM_TRC_MRECOVERY_CHECK_NEED_MEDIARECOV = "[CHECK DATABASE-NEED MEDIA RECOVERY]\n";

SM_TRC_MRECOVERY_CHECK_DB_FAILURE = "[CHECK DATABASE-FAILURE]\n";

SM_TRC_MRECOVERY_CHECK_DB_SID_PPID_FID = "[CHECK DATABASE SID=<%u>, PPID=<%u>, FID=<%u>]\n";

SM_TRC_MRECOVER_INVALID_DECOMP_LOG_LSN = "Log decompress failed - invalid log LSN:[<%u>,<%u>]\n";

SM_TRC_MRECOVER_INVALID_DECOMP_LOG_HEAD = "Log decompress failed - invalid log head info\n"
"    Flag:<%d>, Type:<%d>, Magic:<%d>, Size:<%u>, \n    PrevUndoLSN:[<%u>,<%u>], TransID:<%u>, ReplSPNum:<%d>\n";

SM_TRC_INVALID_LOGANCHOR_ATTRTYPE = "Invalid Log anchor buffer attr type <%u>\n";

#SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_MEM_TBS  = "Invalid anchor offset in Memory TBS Node [<%u>] - anchor <%u> node <%u>\n";

#SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_DISK_TBS = "Invalid anchor offset in Disk TBS Node [<%u>] - anchor <%u> node <%u>\n";

SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_VOL_TBS  = "Invalid anchor offset in Volatile TBS Node [<%u>] - anchor <%u> node <%u>\n";

SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_DBFILE   = "Invalid anchor offset in DBFile Node [<%u>/<%u>] - anchor <%u> node <%u>\n";

SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_CHKPT_PATH = "Invalid anchor offset in Chkpt Path Node [<%u>] - anchor <%u> node <%u>\n";

SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_CHKPT_IMG  = "Invalid anchor offset in Chkpt Image Node [<%u>] - anchor <%u> node <%u>\n";

SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_SECONDARY_BUFFER = "Invalid anchor offset in Secondary Buffer - anchor <%u> node <%u>\n";

SM_TRC_ANCHOR_CHECK_INVALID_OFFSET  = "Invalid anchor offset - anchor <%u> node <%u>\n";

#SM_TRC_ANCHOR_CHECK_CHKPT_PATH_ATTR = "ChkptPathAttrByNode   SpaceID <%u> AttrType <%u> Path <%s>\n"
#                                      "ChkptPathAttrByAnchor SpaceID <%u> AttrType <%u> Path <%s>";

SM_TRC_ANCHOR_CHECK_CHKPT_PATH_ATTR_INVALID = "ChkptPathAttr not found SpaceID <%u> AttrType <%u> Path <%s>\n"
                                              " aAnchorOffset <%u> mWriteOffset <%u> aAttrSize <%u>";

SM_TRC_ANCHOR_CHECK_CHKPT_PATH_ATTR_NOT_FOUND = "ChkptPathNode List SpaceID <%u> AttrType <%u> Path <%s>";

SM_TRC_ANCHOR_CHECK_CHKPT_IMAGE_ATTR_NOT_FOUND = "ChkptImageNode Not Found SpaceID <%u> AttrType <%u>\n"
                                                 " PingPongCnt <%u> FileNum <%u> CreateDBFileOnDisk[<%u>/<%u>]\n"
                                                 " aAnchorOffset <%u> mWriteOffset <%u> aAttrSize <%u>";

SM_TRC_ANCHOR_CHECK_DISK_TBS_ATTR = "<%s> Disk Attribute\n"
                                    " AttrType <%u> ID <%u> Name <%s> Len <%u> Type <%u> State <%u> flag <%u>\n"
                                    " NewFileID <%u> Extent <%u>  ExtPageCnt <%u>\n";

SM_TRC_ANCHOR_CHECK_MEM_TBS_ATTR = "<%s> Memory Attribute\n"
                                   " AttrType <%u> ID <%u> Name <%s> Len <%u> Type <%u> State <%u> flag <%u>\n"
                                   " ShmKey <%u> CurrentDB <%u> ChkptPathCount <%u> SplitFilePageCount <%u>\n"
                                   " AutoExtend <%u> MaxPageCount <%u> NextPageCount <%u> InitPageCount <%u>\n";

SM_TRC_ANCHOR_CHECK_VOL_TBS_ATTR = "<%s> Volatile Attribute\n"
                                   " AttrType <%u> ID <%u> Name <%s> Len <%u> Type <%u> State <%u> flag <%u>\n"
                                   " AutoExtend <%u> MaxPageCount <%u> NextPageCount <%u> InitPageCount <%u>\n";

SM_TRC_ANCHOR_CHECK_DBFILE_ATTR_BY_NODE   = "DBFile By Node   AttrType <%u> ID <%u> FileID <%u> Name <%s> Len <%u> AutoExt <%u>\n"
                               " State <%u> Max <%u> Next <%u> Curr <%u> Init <%u> CreateMode <%u> CreateLSN[<%u>/<%u>]";
SM_TRC_ANCHOR_CHECK_DBFILE_ATTR_BY_ANCHOR = "DBFile By Anchor AttrType <%u> ID <%u> FileID <%u> Name <%s> Len <%u> AutoExt <%u>\n"
                               " State <%u> Max <%u> Next <%u> Curr <%u> Init <%u> CreateMode <%u> CreateLSN[<%u>/<%u>]";

SM_TRC_ANCHOR_CHECK_CHKPT_IMG_ATTR = "ChkptImage Node   SpaceID <%u> AttrType <%u> FileNum <%u> CreateDBFileOnDisk[<%u>/<%u>]\n"
                                     "ChkptImage Anchor SpaceID <%u> AttrType <%u> FileNum <%u> CreateDBFileOnDisk[<%u>/<%u>]";

SM_TRC_ANCHOR_CHECK_CRT_LSN_IN_CHKPT_IMG_ATTR = "Chkpt Image Node SpaceID <%u> ArrMemCreateLSN[<%u>/<%u>]\n"
                                                "Chkpt Image Attr SpaceID <%u> ArrMemCreateLSN[<%u>/<%u>]";

SM_TRC_ANCHOR_CHECK_SBFILE_ATTR_BY_NODE   = "Secondary Buffer By Node   AttrType <%u> Name <%s> Len <%u> Cnt <%u> State <%u> CreateLSN[<%u>/<%u>]";
SM_TRC_ANCHOR_CHECK_SBFILE_ATTR_BY_ANCHOR = "Secondary Buffer By Anchor AttrType <%u> Name <%s> Len <%u> Cnt <%u> State <%u> CreateLSN[<%u>/<%u>]";





#SM_TRC_UTIL_RECOVERY_TEST = "Recovery Test Disabled..\n";

SM_TRC_TRANS_DBFILE_UNLINK_ERROR = "DataFile <%s> unlink Error(errno=<%u>)\n";

SM_TRC_TRANS_DBFILE_UNLINK = "[FILE-UNLINK] <%s> \n";

SM_TRC_DPAGE_VERIFY_TBS_SUCCESS = "[SUCCESS] verify tablespace (ID:<%u>)\n";

SM_TRC_DPAGE_VERIFY_TBS_FAILURE = "[FAILURE] verify tablespace (ID:<%u>)\n";

#SM_TRC_MEMORY_PAGE_MEMORY_NOT_NULL1 = "Page memory has been allocated, but the page has not been allocated yet. Page ID : <%lu>\n";

#SM_TRC_MEMORY_PAGE_MEMORY_NOT_NULL2 = "Maximum Number of Pages on DB : <%lu>\n";

#SM_TRC_MEMORY_PAGE_MEMORY_NOT_NULL3 = "Current Allocated Pages on DB : <%lu>\n";
SM_TRC_MEMORY_CHECKPOINT_IMAGE_NOT_REMOVED = "Failed to remove checkpoint image file, <%s>\n";

#SM_TRC_WARNING_VERIFY_SDP_STRUCT1 = "[VERIFY: Invalid extent state]: In used ext list, invalid ext[<%lu>] state=<%lu>";

#SM_TRC_WARNING_VERIFY_SDP_STRUCT2 = "[VERIFY: Invalid page state]: ext=ins free, ins=<%lu>, free=<%lu>";

#SM_TRC_WARNING_VERIFY_SDP_STRUCT3 = "[VERIFY: Invalid page state]: ext=ins no free, ins=<%lu>";

#SM_TRC_WARNING_VERIFY_SDP_STRUCT4 = "[VERIFY: Invalid page state]: ext=no ins free, free=<%lu>";

#SM_TRC_WARNING_VERIFY_SDP_STRUCT5 = "[VERIFY: Invalid page state]: ext=free, page[<%lu>]=<%lu>";

#SM_TRC_WARNING_VERIFY_SDP_STRUCT6 = "[VERIFY: Invalid page state]: ext=full, page[<%lu>]=<%lu>";

#SM_TRC_WARNING_VERIFY_SDP_STRUCT7 = "[VERIFY: Invalid page state]: ext=ins no free, page[<%lu>]=free";

#SM_TRC_WARNING_VERIFY_SDP_STRUCT8 = "[VERIFY: Invalid page state]: ext=no ins free, page[<%lu>]=ins";

SM_TRC_MRECOVERY_LFTHREAD_INFO_FOR_SYNC_LOGFILE = "Logfile sync thread synced  [ first: <%u> ~ last: <%u>, count:<%u> ] logfile(s).\n";

SM_TRC_BUFFER_FLUSH_BUFFER_POOL= "[ALTER SYSTEM] Flush Buffer Pool.\n";
#SM_TRC_BUFFER_FLUSH_BUFFER_POOL_SUCCESS= "[ALTER SYSTEM] Flush Buffer Pool Succeeded. [ flush page count : <%u> ]\n";
SM_TRC_BUFFER_FLUSH_BUFFER_POOL_FAILURE= "[ALTER SYSTEM] Flush Buffer Pool Failed.\n";

SM_TRC_PAGE_PID_INVALID = "SpaceID:<%u> Page: <%lu> is invalid because
it is bigger than Max PID: <%lu>\n";

SM_TRC_BUFFER_POOL_FIXEDBCB_DURING_INVALIDATE = "Fixed BCB is targeted during invalidateDirtyOfBCB. Filter <%u>";

#SM_TRC_BUFFER_POOL_FIXEDBCB_DURING_MAKEFREEBCB = "Fixed BCB is targeted during makeFreeBCB";

SM_TRC_BUFFER_MGR_RESIZE = "BUFFER MANAGER RESIZE SUCCESS: askedSize [<%lu>], newSize[<%lu>]";

SM_TRC_BUFFER_POOL_CONFLICT_CREATEPAGE = "BUFFER POOL CONFLICT CREATEPAGE : Type [<%lu>]";

SM_TRC_BUFFER_POOL_BCB_INFO = "BCB Info..\n"
"mID <%u>\n"
"mState <%u>\n"
"mFrame <%lx>\n"
"mSpaceID <%u>\n"
"mPageID <%u>\n"
"mPageType <%u>\n"
"mRecoveryLSN <%u>,<%u>\n"
"mBCBListType <%u>\n"
"mBCBListNo <%u>\n"
"mCPListNo <%u>\n"
"mTouchCnt <%u>\n"
"mFixCnt <%u>\n"
"mReadyToRead <%u>\n"
"mPageReadError <%u>\n"
"mWriteCount <%u>\n";

SM_TRC_BUFFER_FLUSH_SECONDARY_BUFFER= "[ALTER SYSTEM] Flush Secondary Buffer.\n";
#SM_TRC_BUFFER_FLUSH_SECONDARY_BUFFER_SUCCESS= "[ALTER SYSTEM] Flush Secondary Buffer Succeeded. [ flush page count : <%u> ]\n";
SM_TRC_BUFFER_FLUSH_SECONDARY_BUFFER_FAILURE= "[ALTER SYSTEM] Flush Secondary Buffer Failed.\n";

#SM_TRC_SECONDARY_BUFFER_BCB_INFO = "SECONDARY BCB Info..\n"
#"mID <%u>\n"
#"mState <%u>\n"
#"mSpaceID <%u>\n"
#"mPageID <%u>\n"
#"mRecoveryLSN <%u>,<%u>\n";

SM_TRC_SDP_INVALID_PAGE_INFO = "Invalid Page Info..\n"
"mSpaceID <%u>\n"
"mPageID <%u>\n"
"mType <%u>";

#SM_TRC_SDPSC_INVALID_CIRCULAR_LIST = "[CMS] Mismatch Extent Count Of ExtDir(<%u>)\n"
#"TotExtCnt: (<%u>), MaxExtCnt: (<%u>)\n";

SM_TRC_TRANS_DISABLE_MIN_VIEWSCN_BUILDER = "Disable MinViewSCN Builder ...\n";

SM_TRC_TRANS_EXIST_ACTIVE_TRANS = "Transaction ID <%u> is active. ( Status: <%u> )\n";

SM_TRC_INDEX_INTEGRITY_CHECK_THE_INDEX_IS_NOT_CONSISTENT = "INDEX_INTEGRITY_CHECK - The index is not consistent. \n";

SM_TRC_INDEX_INTEGRITY_CHECK_KEY_ORDER = "INDEX_INTEGRITY_CHECK KeyOrder -"
"isLeaf[<%c>] CompareRetVal[<%d>] NodePID[<%d>] SlotSeq[<%d>] Row[<%d>,<%d>]\n";

SM_TRC_INDEX_INTEGRITY_CHECK_SMO_NUMBER ="INDEX_INTEGRITY_CHECK SMO Number-"
"PID[<%d>] Index SMO[<%d>], Node SMO[<%d>]\n";

SM_TRC_INDEX_INTEGRITY_CHECK_IS_NOT_LEAF_NODE="INDEX_INTEGRITY_CHECK This node is Not a Leaf -"
"Invalid Height. PID(<%d>), Height(<%d>)\n";

SM_TRC_INDEX_INTEGRITY_CHECK_BROKEN_LEAF_PAGE_LINK ="INDEX_INTEGRITY_CHECK Broken Leaf Page Link -"
"Prev PID(<%d>), Prev->Next(<%d>), Next PID(<%d>), Next->Prev(<%d>)\n";

SM_TRC_INDEX_INTEGRITY_CHECK_INVALID_PAGE_TYPE ="INDEX_INTEGRITY_CHECK Invalid Page Type -"
"PID(<%d>), Type(<%d>)\n";

SM_TRC_INDEX_INTEGRITY_CHECK_INVALID_SLOT_COUNT ="INDEX_INTEGRITY_CHECK Invalid Slot Count -"
"PID(<%d>), Slot Count(0)\n";

SM_TRC_EMERECO_DRDB_WAL_VIOLATION ="DRDB WAL protocol violation : "
"DB UpdateLSN=[<%d>,<%d>] > RedoLSN=[<%d>,<%d>]\n";

SM_TRC_EMERECO_MRDB_WAL_VIOLATION ="MRDB WAL protocol violation : "
"CheckpointLSN=[<%u>,<%u>] > RedoLSN=[<%u>,<%u>]\n";

SM_TRC_EMERECO_FAILED_SET_INCONSISTENCY_FLAG ="Failed to set object inconsistency flag";

SM_TRC_WAIT_SELF_WARNING ="Transaction is waiting for itself [<%d>]";

SM_TRC_INVALID_BCB = "The BCB does not exist or is not valid. ( SpaceID:(<%u>), PageID:(<%u>), ListNode(<%lx>), BCB(<%lx>) )\n";

SM_TRC_INVALID_BCB_LIST ="Invalid BCB List : ListNode(<%lx>), Prev(<%lx>), Next(<%lx>), data(<%lx>)\n";


INTERNAL_TRACE_MESSAGE_END
