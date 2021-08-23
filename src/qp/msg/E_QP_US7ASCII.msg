# $Id: E_QP_US7ASCII.msg 32106 2009-04-13 01:53:28Z kimmkeun $
#
# QCU Error Message File (used by genErrMsg)
#

# 에러코드 영역 번호
# 0 => sm
# 1 => qp
# 2 => cm
# 3 => main

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.

# Last Number = 1194

SECTION = 3

###############################################################################
#########                 MEMORY ERROR CODE                           #########
###############################################################################
470,qpERR_ABORT_MEMORY_ALLOCATION = Insufficient memory for Query Processor
# *Cause: The memory manager could not allocate sufficient memory for the query processor.
# *Action: Verify that the system has sufficient memory.

###############################################################################
#########                 THREAD ERROR CODE                           #########
###############################################################################

987,qpERR_FATAL_THREAD_ATTR_CREATE = Failed to invoke the thread_attr_init() system function
# *Cause: The system failed to create a thread attribute.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

988,qpERR_FATAL_THREAD_ATTR_SET_BOUND = Failed to invoke the thread_attr_setscope() system function
# *Cause: The system failed to bind a thread attribute.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

989,qpERR_FATAL_THREAD_ATTR_DESTROY = Failed to invoke the thread_attr_destroy() system function
# *Cause: The system failed to destroy a thread attribute.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

990,qpERR_ABORT_THREAD_CREATE = Failed to invoke the thread_create() system function
# *Cause: The system failed to create a thread.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

991,qpERR_FATAL_THREAD_JOIN = Failed to invoke the thread_join() system function
# *Cause: The system failed to join a thread.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

992,qpERR_FATAL_MUTEX_INIT = Unable to initialize a mutex.
# *Cause: The system failed to initialize a mutex.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

993,qpERR_FATAL_MUTEX_DESTROY = Failed to invoke the mutex_destroy() system function
# *Cause: The system failed to destroy a mutex.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

994,qpERR_FATAL_MUTEX_LOCK = Failed to invoke the mutex_lock() system function
# *Cause: The system failed to lock a mutex.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

995,qpERR_FATAL_MUTEX_UNLOCK = Failed to invoke the mutex_unlock() system function
# *Cause: The system failed to unlock a mutex.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

996,qpERR_FATAL_COND_INIT = Failed to invoke the cond_init() system function
# *Cause: The system failed to initialize a condition variable.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

997,qpERR_FATAL_COND_DESTROY = Failed to invoke the cond_destroy() system function
# *Cause: The system failed to destroy a condition variable.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

998,qpERR_FATAL_COND_SIGNAL = Failed to invoke the cond_signal() system function
# *Cause: The system failed to signal using a condition variable.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

999,qpERR_FATAL_COND_WAIT = Failed to invoke the cond_wait() system function
# *Cause: Invalid condition value or invalid mutex.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

1000,qpERR_ABORT_BIND_PROCESS = Failed to invoke the bind_process() system function
# *Cause: System failed to bind cpusets to a process.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

1061,qpERR_FATAL_MUTEX_TRYLOCK = Failed to invoke the mutex_trylock() system function
# *Cause: The system failed to lock a mutex.
# *Action: Check the call stack from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

1109,qpERR_ABORT_FAULT_TOLERATED = Failed to work because an internal exception occurred from an OS.[Contact Altibase's Support Center]
# *Cause: An internal exception occurred from an OS.
# *Action: Check the error number from the trace log for more detailed information. And contact Altibase's Support Center (http://support.altibase.com).

###############################################################################
#########                 QMC ERROR CODE                              #########
###############################################################################
792,qpERR_ABORT_QMC_UNEXPECTED_ERROR = Unexpected errors may have occurred: <0%s>: <1%s>
# *Cause: Unexpected errors may have occurred.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).


###############################################################################
#########                 QCP ERROR CODE                              #########
###############################################################################

1,qpERR_ABORT_QCP_SYNTAX = SQL syntax error <0%s>
# *Cause: The unusable reserved words or delimiter inapplicable was used. Or the reserved word cannot be used.
# *Action: Refer to SQL Reference. Or verify the reserved words listed in SQL Reference.

3,qpERR_ABORT_QCP_NOT_SUPPORTED_SYNTAX = Unsupported syntax
# *Cause: The syntax is currently not supported.
# *Action: Refer to the SQL manual.

4,qpERR_ABORT_QCP_CONFLICT_NULL_CONSTRAINT = Duplicate or conflicting NULL and/or NOT NULL constraints
# *Cause: Duplicate or conflicting NULL and/or NOT NULL constraints.
# *Action: Remove one of the duplicate or conflicting constraints.

5,qpERR_ABORT_QCP_NO_HAVE_DATATYPE_IN_CRT_TBL = A column in a CREATE TABLE or ALTER TABLE ADD COLUMN statement must have a data type.
# *Cause: A column in a CREATE TABLE or ALTER TABLE ADD COLUMN statement must have a data type.
# *Action: Specify a data type for the column.

6,qpERR_ABORT_QCP_HAVE_DATATYPE_IN_CRT_TBL_AS_SELECT = A column in a CREATE TABLE AS SELECT statement must not have a data type.
# *Cause: A column in a CREATE TABLE AS SELECT statement must not have a data type.
# *Action: Do not specify data types in the column definition clause.

7,qpERR_ABORT_QCP_DUPLICATE_COLUMN_NAME = Duplicate column name <0%s>
# *Cause: Duplicate column name
# *Action: Verify that no duplicate column names are specified.

564,qpERR_ABORT_QCP_DUPLICATE_CONSTRAINT_NAME = Duplicate constraint name <0%s>
# *Cause: Duplicate constraint name
# *Action: Verify that no duplicate constraint names are specified.

8,qpERR_ABORT_QCP_HAVE_NO_COLUMN = This statement must have at least one column.
# *Cause: A CREATE TABLE or ALTER TABLE ADD COLUMN statement must have at least one column.
# *Action: Specify more than one column.

9,qpERR_ABORT_QCP_LIMIT_VALUE = The START or COUNT value in a SELECT LIMIT clause must be greater than 0.
# *Cause: The START or COUNT value in a SELECT LIMIT clause must be greater than 0.
# *Action: Specify a positive integer for the START or COUNT value.

395,qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW = The length of the name exceeds the maximum limit. <0%s>
# *Cause: The length of the name exceeds the maximum limit.
# *Action: Refer to the SQL Reference for the maximum possible length allowed for the object's name. Enter a name with a valid length.

338,qpERR_ABORT_QSX_ROWTYPE_ON_RETURN_NOT_ALLOWED_ERROR_SQLTEXT = ROWTYPE is not allowed for a RETURN clause. <0%s>
# *Cause: The RETURN type of a function is ROWTYPE.
# *Action Do not use ROWTYPE as a RETURN clause.

390,qpERR_ABORT_QCP_FUNC_NAME_NOT_MATCHED = Mismatched function name <0%s>
# *Cause: The function name is mismatched.
# *Action: Match the function names or remove the function name after the END keyword.

391,qpERR_ABORT_QCP_FUNC_TOO_MANY_ARGU = A procedure or function has too many parameters.
# *Cause: A procedure or function has too many parameters.
# *Action: Refer to the Stored Procedures Manual for the maximum possible number of parameters allowed for the procedure or function. Define a valid number of parameters.

516,qpERR_ABORT_QCP_INVALID_INSERT_LIMIT_OPTION = Invalid values for INSERT LOW LIMIT and INSERT HIGH LIMIT options.
# *Cause: The value for the INSERT HIGH LIMIT option must be greater than the one for the INSERT LOW LIMIT option.
# *Action: Check the INSERT LOW LIMIT and INSERT HIGH LIMIT options.

540,qpERR_ABORT_QCP_INVALID_LOGGING_OPTION = Only one LOGGING or NOLOGGING clause may be specified.
# *Cause: Duplicate LOGGING options.
# *Action: Check the LOGGING options.

541,qpERR_ABORT_QCP_INVALID_PARALLEL_OPTION = Only one PARALLEL or NOPARALLEL clause may be specified.
# *Cause: Duplicate PARALLEL options.
# *Action: Check the PARALLEL options.

542,qpERR_ABORT_QCP_INVALID_TABLESPACE_OPTION = Duplicate tablespace name clause.
# *Cause: Duplicate TABLE SPACE NAME options.
# *Action: Check the TABLESPACE options.

578,qpERR_ABORT_QCP_INVALID_BUFFER_OPTION = Only one BUFFER or NOBUFFER clause may be specified.
# *Cause: Duplicate BUFFER options.
# *Action: Check the BUFFER options.

577,qpERR_ABORT_QCP_INVALID_SYSDBA_OPTION = Invalid SYSDBA option.
# *Cause: Invalid SYSDBA option.
# *Action: Usage: CONNECT username/password [AS SYSDBA].

667,qpERR_ABORT_QCP_DUPLICATE_SYNC_TABLE = Duplicate table names.
# *Cause: Duplicate table names exist.
# *Action: Check table names for duplicates.

688,qpERR_ABORT_QCP_DUPLICATE_REPL_OPTION = Duplicate option names.
# *Cause: Duplicate option names exist.
# *Action: Check option names for duplicates.

689,qpERR_ABORT_QCP_NOT_SUPPORTED_REPL_OPTION = Unsupported replication option name <0%s>
# *Cause: The option is currently not supported.
# *Action: Refer to the Replication Manual for information about supported options. Select an available option.

714,qpERR_ABORT_QCP_INVALID_FUNCTION = Functions or procedures other than TO_DATE function not allowed.
# *Cause: Invalid function call.
# *Action: Check the function name.

733,qpERR_ABORT_QCP_INVALID_DATABASE_CHARSET = A database character set must be specified.
# *Cause: Database character set is not specified.
# *Action: Specify a database character set.

734,qpERR_ABORT_QCP_INVALID_NATIONAL_CHARSET = A national character set must be specified.
# *Cause: A national character set is not specified.
# *Action: Specify a national character set.

904,qpERR_ABORT_QCP_COLUMN_CHECK_CONSTRAINT_REFERENCE_OTHER_COLUMN = A column check constraint cannot reference other columns.
# *Cause: A column check constraint references other columns.
# *Action: Check and revise the column check constraint.

905,qpERR_ABORT_QCP_SET_USER_NAME_OR_TABLE_NAME_TO_CONSTRAINT_COLUMN = Cannot specify user name or table name in column constraint <0%s>
# *Cause: A user name or table name was specified in a column constraint definition.
# *Action: Remove the user name or table name from the column constraint definition.

922,qpERR_ABORT_QCP_CANNOT_SPECIFY_USER_NAME_OR_TABLE_NAME = Cannot specify user name or table name in function-based index column <0%s>
# *Cause: A user name or table name was specified in the function-based index column definition.
# *Action: Remove the user name or table name from the function-based index column definition.

927,qpERR_ABORT_QCP_REQUIRE_OWNER_NAME_IN_DEFAULT_EXPR = Name of stored function owner is required in function-based index definition. <0%s>
# *Cause: The name of the stored function's owner is required in function-based index definition.
# *Action: Specify the name of the stored function's owner in the function-based index definition.

928,qpERR_ABORT_QCP_REQUIRE_OWNER_NAME_IN_CHECK_EXPR = Name of stored function owner is required in check constraint expression. <0%s>
# *Cause: The name of the stored function's owner is required in check constraint expression.
# *Action: Specify the name of the stored function's owner in the check constraint expression.

1032,qpERR_ABORT_QCP_DUPLICATE_LATERAL_KEYWORD = The LATERAL keyword cannot come after CROSS APPLY or OUTER APPLY.
# *Cause: The LATERAL keyword comes after CROSS APPLY or OUTER APPLY.
# *Action: Do not write the LATERAL keyword after CROSS APPLY or OUTER APPLY.

###############################################################################
#########                 QMS ERROR CODE                              #########
###############################################################################

514,qpERR_ABORT_INVALID_LIMIT_VALUE_TYPE = A limit value host variable was bound to an invalid type.
# *Cause: The host variable type for the limit value must be one of INTEGER, BIGINT, or SMALLINT.
# *Action: Bind the limit value host variable to one of the integer types.

###############################################################################
#########                 QCM ERROR CODE                              #########
###############################################################################
13,qpERR_ABORT_QCM_INITDB = Unable to initialize the meta database
# *Cause: The system failed to initialize the meta database due to a MEM_DIC tablespace error.
# *Action: Recover the MEM_DIC tablespace or the database, or re-create the database.

14,qpERR_ABORT_QCM_META_ALEADY_EXIST = The meta database already exists.
# *Cause: The meta database already exists.
# *Action: Verify that the database has been properly dropped.

16,qpERR_ABORT_QCM_NOT_EXIST_USER = User not found
# *Cause: The user is not in the meta database.
# *Action: Verify that you have entered a valid user name and that the user exists in the database.

17,qpERR_ABORT_QCM_NOT_EXIST_TABLE = Table not found
# *Cause: The table is not in the meta database.
# *Action: Verify that you have entered a valid table name and that the table exists in the database.

18,qpERR_ABORT_QCM_NOT_EXIST_COLUMN = Column not found
# *Cause: The column is not in the meta database.
# *Action: Verify that you have entered a valid column name and use the DESC command to check that the column exists in the table.

500,qpERR_ABORT_QCM_NOT_EXIST_COLUMN_NAME = Invalid column name <0%s>
# *Cause: The column name does not exist.
# *Action: Specify a valid column name.

19,qpERR_ABORT_QCM_NOT_EXIST_SEQUENCE = Sequence not found
# *Cause: The specified sequence name was not found in the database.
# *Action: Verify that the sequence exists.

20,qpERR_ABORT_QCM_NOT_EXISTS_INDEX = Index not found
# *Cause: The specified index name was not found in the database.
# *Action: Verify that the index exists. Look up meta tables to verify that the index name is correct.

21,qpERR_ABORT_QCM_META_CRASH = The meta table crashed.
# *Cause: The meta table crashed.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

22,qpERR_FATAL_QCM_REPL_META_CRASH = A replication meta table crashed.
# *Cause:
#   - A replication meta table crashed.
# *Action:
#   - Please send a bug report to the vendor.

23,qpERR_ABORT_QCM_REPL_NOT_FOUND = Replication not found
# *Cause: The specified replication has not been created yet.
# *Action: Create the replication first. Verify that the replication name is valid.

24,qpERR_ABORT_QCM_INTERNAL_ARG = Internal server error in meta module (<0%s>).
# *Cause: Program error
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

25,qpERR_ABORT_QCM_NOT_ENOUGH_REPLICATION_ITEMS = Internal server error. There are too few tables for replication.
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

26,qpERR_ABORT_QCM_TOO_MANY_REPLICATION_ITEMS = Internal server error. There are too many tables for replication.
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

27,qpERR_ABORT_QCM_MAX_REPLICATION = Internal server error.
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

402,qpERR_ABORT_QCM_LOCK_FAIL = The table is locked by other transactions.
# *Cause: Cannot obtain a lock on the specified table because it is already locked by other transactions.
# *Action: Check other transactions.

422,qpERR_ABORT_QCM_NOT_ENOUGH_REPLICATION_HOSTS = Internal server error. There are too few hosts for replication.
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

423,qpERR_ABORT_QCM_TOO_MANY_REPLICATION_HOSTS = Internal server error. There are too many hosts for replication.
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

201,qpERR_ABORT_RESET_SENDER  =  [REPL manager] Resetting <0%s> sender XSN (sender XSN = SN<1%ld>,current = SN<2%ld>)
# *Cause:
#   - The system tried to reset the current log file specified by the REPLICATION_MAX_LOGFILE property.
# *Action:
#   - N/A

202,qpERR_ABORT_STOP_SENDER  =  [REPL manager] Stopping <0%s> sender thread (sender read = SN<1%ld>,current = SN<2%ld>,SN = <3%ld>)
# *Cause:
#   - The system tried to restart the sender thread with the current log file specified by the REPLICATION_MAX_LOGFILE property.
# *Action:
#   - N/A

522,qpERR_ABORT_DROP_LAST_TABLE = You cannot drop the last table.
# *Cause: You cannot drop the last table from a replication object.
# *Action: Check the META information.

523,qpERR_ABORT_DROP_LAST_HOST = You cannot drop the last host.
# *Cause: You cannot drop the last host from a replication object.
# *Action: Check the META information.

586,qpERR_ABORT_QCM_INVALID_DUMP_TABLE = Invalid dump table
# *Cause: The table is not a dump table.
# *Action: Use a dump table.

587,qpERR_ABORT_QCM_INVALID_DUMP_OBJECT = Invalid dump object
# *Cause: The object is not valid for the dump table.
# *Action: Use a valid dump object for the dump table.

588,qpERR_ABORT_QCM_TOO_MANY_DUMP_OBJECT = Too many dump objects
# *Cause: You are using too many dump objects.
# *Action: Use a suitable number of dump objects for the dump table.

603,qpERR_ABORT_QCM_NOT_EXIST_TABLE_PARTITION = The table partition does not exist. <0%s>
# *Cause: The table partition does not exist.
# *Action: Check the table partition name.

604,qpERR_ABORT_QCM_TOO_MANY_LIST_PARTITION_CONDITION_VALUE = Too many partition condition values for the list partitioned table
# *Cause: There are too many partition condition values for the list partitioned table.
# *Action: Check the total number of partition condition values.

690,qpERR_ABORT_QCM_ALREADY_SUPPORT_RECOVERY = The table recovery is already supported by another replication <0%s>
# *Cause: The recovery of replication item is already supported.
# *Action: Check the replication item.

697,qpERR_ABORT_QCM_MAXIMUM_OBJECT_NUMBER_EXCEED = The number of elements <0%s> exceeds the maximum limit (<1%ld>).
# *Cause: The specified number of elements exceeds the maximum limit.
# *Action: Reduce the specified number of elements.

700,qpERR_ABORT_QCM_NOT_ENOUGH_REPLICATION_OLD_ITEMS = Internal server error. There are too few old item metas for replication. (<0%s>)
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

701,qpERR_ABORT_QCM_TOO_MANY_REPLICATION_OLD_ITEMS = Internal server error. There are too many old item metas for replication. (<0%s>)
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

702,qpERR_ABORT_QCM_NOT_ENOUGH_REPLICATION_OLD_COLUMNS = Internal server error. There are too few old column metas for replication. (<0%s>, <1%lu>)
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

703,qpERR_ABORT_QCM_TOO_MANY_REPLICATION_OLD_COLUMNS = Internal server error. There are too many old column metas for replication. (<0%s>, <1%lu>)
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

704,qpERR_ABORT_QCM_NOT_ENOUGH_REPLICATION_OLD_INDICES = Internal server error. There are too few old index metas for replication. (<0%s>, <1%lu>)
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

705,qpERR_ABORT_QCM_TOO_MANY_REPLICATION_OLD_INDICES = Internal server error. There are too many old index metas for replication. (<0%s>, <1%lu>)
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

706,qpERR_ABORT_QCM_NOT_ENOUGH_REPLICATION_OLD_INDEX_COLS = Internal server error. There are too few old index column metas for replication. (<0%s>, <1%lu>, <2%u>)
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

707,qpERR_ABORT_QCM_TOO_MANY_REPLICATION_OLD_INDEX_COLS = Internal server error. There are too many old index column metas for replication. (<0%s>, <1%lu>, <2%u>)
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

715,qpERR_ABORT_QCM_INVALID_REPL_CONDITION_MTCNAME = There is an invalid operator or operand in a replication condition.
# *Cause: The length of mtcName cannot be over 8.
# *Action: Check the replication conditions.

716,qpERR_ABORT_QCM_INVALID_REPL_CONDITION = Invalid replication condition.
# *Cause: The condition's top node has only an argument node.
# *Action: Check the replication conditions.

717,qpERR_ABORT_QCM_INVALID_REPL_CONDITION_DATATYPE = Data type not permitted in replication condition.
# *Cause: The data type is not permitted.
# *Action: Check the data type of the operand for the replication condition.

718,qpERR_ABORT_QCM_INVALID_REPL_CONDITION_OPERAND = If one operand is a column, the other operand has to be a value.
# *Cause: A pair of operands must always consist of a column and a value.
# *Action: Check the operands for the replication condition.

719,qpERR_ABORT_QCM_DIFF_REPL_CONDITION_OPERANDS = A Pair of operands has to have the same data type.
# *Cause: The data types of a pair of operands are different from each other.
# *Action: Check the data types of the operands for the replication condition.

720,qpERR_ABORT_QCM_COLUMN_VALUE_MISMATCH = Invalid update column list.
# *Cause: The number of values and the number of columns in an update column list are not the same.
# *Action: Check the update column list.

721,qpERR_ABORT_QCM_INVALID_REPL_CONDITION_VALUE = Sysdate is not allowed as a replication condition value.
# *Cause: Sysdate was specified as a replication condition value.
# *Action: Do not use SYSDATE as a replication condition value.

756,qpERR_ABORT_QCM_TOO_MANY_REPLICATION_OFFLINE_DIR_PATH = Internal server error. Too many offline log dir paths for replication.
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

757,qpERR_ABORT_QCM_NOT_ENOUGH_REPLICATION_OFFLINE_DIR_PATH = Internal server error. Too few offline log dir paths for replication.
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

758,qpERR_ABORT_QRC_NOT_EXIST_REPL_OFFLINE_DIR_PATH = Replication offline log dir not found.
# *Cause: No replication offline log dir path was found.
# *Action: Check the meta tables.

759,qpERR_ABORT_QRC_ROLE_NOT_SUPPORT_REPL_OFFLINE = Offline option not supported in this role.
# *Cause: This role does not support the offline option.
# *Action: Check the options or the role of the replication.

760,qpERR_ABORT_QCP_MAX_PATH_LENGTH_OVERFLOW = The length of the path exceeds the maximum limit. <0%s>
# *Cause: The length of the path exceeds the maximum limit.
# *Action: Refer to the SQL Reference for the maximum possible length allowed for the path. Enter a path with a valid length.

761,qpERR_ABORT_QRC_NOT_SUPPORT_REPL_OFFLINE_AND_RECOVERY = Offline option is not supported with recovery option.
# *Cause: The offline option is not supported with the recovery option.
# *Action: Check the replication options.

762,qpERR_ABORT_QRC_ALREADY_OFFLINE_SET = Offline option already set.
# *Cause: The replication offline option was already set.
# *Action: Check the replication options.

763,qpERR_ABORT_QRC_ALREADY_OFFLINE_UNSET = Offline option already unset.
# *Cause: The replication offline option was already unset.
# *Action: Check the replication options.

872,qpERR_ABORT_QCM_REUSE_COUNT = The password cannot be reused.
# *Cause: The password has been reused more than the allowable number of times.
# *Action: Enter a new password.
    
873,qpERR_ABORT_QCM_USER_DEFINED_ERROR = user defined error: <0%s>
# *Cause: A user-defined callback function returned an error.
# *Action: Respond to the error raised by the user-defined callback function.
 
874,qpERR_ABORT_QCM_MESSAGE_TOO_LONG = Callback function message too long
# *Cause: The error message returned by a callback function was too long.
# *Action: Reduce the length of the callback function error message.
 
875,qpERR_ABORT_QCM_VERIFY_FUNCTION_SELECT_ERROR = Invalid user-defined function 'PASSWORD_VERIFY_FUNCTION'.
# *Cause: A user-defined function is invalid, or was not called properly.
# *Action: Check the validity of the user-defined function and verify that it was called correctly.
    
883,qpERR_ABORT_QCM_NOT_EXIST_MVIEW = Materialized view not found
# *Cause: The materialized view is not in the meta database.
# *Action: Verify that you have entered a valid materialized view name and that the materialized view exists in the database.

972,qpERR_ABORT_QCM_JOB_QUEUE_OVERFLOW = The job thread queue overflows.
# *Cause: The size of the thread queue is too small for the number of jobs.
# *Action: Enlarge the size of the JOB_THREAD_QUEUE_SIZE property.

1028,qpERR_ABORT_QCM_NOT_EXIST_USER_OR_ROLE = User or role not found.
# *Cause: The specified role or user does not exist.
# *Action: Check whether the role or user exists.

1029,qpERR_ABORT_QCM_USER_TO_ROLES_MAX_COUNT = Unable to grant any more roles to the user.
# *Cause: The number of roles granted to the user has reached the maximum limit.
# *Action: Do not grant any more roles to the user, or revoke a role first.

###############################################################################
#########                 QCS ERROR CODE                              #########
###############################################################################

768,qpERR_ABORT_DLOPEN = dlopen error: [<0%s>]
# *Cause: dlopen error
# *Action: Check the shared library.

769,qpERR_ABORT_DLSYM = dlsym error
# *Cause: dlsym error
# *Action: Check the shared library.

770,qpERR_ABORT_DLCLOSE = dlclose error: [<0%s>]
# *Cause: dlclose error
# *Action: Check the shared library.

771,qpERR_ABORT_FUNC_CALL = Function call error: [<0%s> : <1%s>]
# *Cause: Function call error
# *Action: Check the shared library.

772,qpERR_ABORT_INVALID_MODULE = Invalid security module.
# *Cause: Invalid security module.
# *Action: Check the security module.

773,qpERR_ABORT_FAIL_TO_VERIFY_MODULE = Failed to verify security module: [<0%s>]
# *Cause: Failed to verify the security module.
# *Action: Check the security module.

774,qpERR_ABORT_NOT_EXIST_POLICY = Policy does not exist: [<0%s>]
# *Cause: The policy does not exist.
# *Action: Check the security module.

775,qpERR_ABORT_INVALID_POLICY = Policy is not valid: [<0%s>]
# *Cause: The policy is not valid.
# *Action: Check the security module.

776,qpERR_ABORT_NOT_EXIST_ECC_POLICY = ECC policy does not exist: [<0%s>]
# *Cause: The ECC policy does not exist.
# *Action: Check the security module.

777,qpERR_ABORT_INVALID_ECC_POLICY = ECC policy is not valid: [<0%s>]
# *Cause: The ECC policy is not valid.
# *Action: Check the security module.

781,qpERR_ABORT_FAIL_TO_OPEN_SECURITY_MODULE = Failed to open security module.
# *Cause: Failed to open the security module.
# *Action: Check the SECURITY_MODULE property.

782,qpERR_ABORT_SECURITY_MODULE_ALREADY_STARTED = Security has already been started.
# *Cause: Security has already been started.
# *Action: Stop SECURITY_MODULE to start again.

783,qpERR_ABORT_MISMATCH_SECURITY_MODULE = Mismatched security module [<0%s>], [<1%s>], [<2%s>]
# *Cause: Mismatched security module.
# *Action: Check the security module.

784,qpERR_ABORT_EXIST_ENCRYPTED_COLUMN = Encrypted column exists.
# *Cause: An encrypted column exists.
# *Action: Check the encrypted column.

785,qpERR_ABORT_INVALID_SECURITY_MODULE_VERSION = Security module version is not valid.
# *Cause: Security module version is not valid.
# *Action: Check the security module.

786,qpERR_ABORT_POLICY_BLOCK_SIZE_TOO_BIG = Policy block size is too big: [<0%s>]
# *Cause: The policy block size is too big.
# *Action: Check the security module.

###############################################################################
#########                 QDB ERROR CODE                              #########
###############################################################################
32,qpERR_ABORT_QDB_DDL_WITH_REPLICATED_TBL = You cannot execute DDL on a replicated table.
# *Cause: You cannot execute DDL on a replicated table.
# *Action: Verify that the specified table is not replicated.

985,qpERR_ABORT_QDB_DDL_WITH_REPLICATED_PART = You cannot execute DDL on a replicated partition.
# *Cause: You cannot execute DDL on a replicated partition.
# *Action: Verify that the specified partition is not replicated.

1141,qpERR_ABORT_QDB_DDL_SYNC_WITH_LOCK_UNTIL_NEXT_DDL = You cannot execute DDL for the replicated table after performing LOCK.. UNTIL NEXT DDL on REPLICATION_DDL_SYNC enable mode.
# *Cause: You cannot use the property REPLICATION_DDL_SYNC enable and LOCK.. UNTIL NEX DDL statement on the replicated table.
# *Action: Change the property REPLICATION_DDL_SYNC to 0(disable) or close a transaction.

34,qpERR_ABORT_QDB_EXIST_OBJECT_NAME = The name is already used by an existing object.
# *Cause: An attempt was made to create a database object (such as a table, view or sequence) that already exists. A user's database objects must have distinct names.
# *Action: Enter a unique name for the database object or modify or drop the existing object so it can be reused.

35,qpERR_ABORT_QDB_DUPLICATE_COLUMN = Duplicate column name : <0%s>
# *Cause: Two or more identical column names were declared for a table.
# *Action: Change one of the duplicate column names in the table.

451,qpERR_ABORT_QDB_DUPLICATE_TIMESTAMP_COLUMN = A table can have only one TIMESTAMP column. <0%s>
# *Cause: A table can have only one TIMESTAMP column.
# *Action: Specify only one timestamp column in the table description.

452,qpERR_ABORT_QDB_CANNOT_SET_TIMESTAMP_DEFAULT = No default value can be specified for a TIMESTAMP column. <0%s>
# *Cause: No default value can be specified for a TIMESTAMP column.
# *Action: Remove the default clause from the TIMESTAMP column.

36,qpERR_ABORT_QDB_NOT_EXISTS_ALIAS = No alias is specified. The SELECT statement has a compound expression without an alias.
# *Cause: The SELECT statement has a compound expression without an alias name, which is used as a column name for the new table.
# *Action: Specify an alias to the compound expression in the target list of the SELECT statement.

37,qpERR_ABORT_QDB_MISMATCH_COL_COUNT = The number of columns in the list and the number of values in the list do not match.
# *Cause: When the user executes a CREATE TABLE AS SELECT statement, the number of defined columns must be equivalent to the number of specified expressions in the target list of the SELECT statement.
# *Action: Verify that the number of columns is equivalent to the number of expressions in the target list of the SELECT statement.

38,qpERR_ABORT_QDB_INVALID_COLUMN_COUNT = Too many or too few columns in a table
# *Cause: The number of columns in a table is either larger than the limit or zero as a result of adding or dropping a column.
# *Action: If the number of columns becomes zero, drop the table. If the number of columns exceeds the limit, create a new table for the column.

39,qpERR_ABORT_QDB_NOT_EXISTS_DEFAULT = Unable to find the DEFAULT specification to drop.
# *Cause: Could not find a DEFAULT specification to drop.
# *Action: Consult the meta tables to see if a DEFAULT value has been specified for the column.

40,qpERR_ABORT_QDB_CREATE_DISABLE_DATA_TYPE = Unable to create a column with the specified data type.
# *Cause: Unable to create a column with the specified data type
# *Action: Verify that it is permissible to create the specified data type.

41,qpERR_ABORT_QDB_META_CRASH = The meta database crashed.
# *Cause: The meta database crashed.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

42,qpERR_ABORT_QDB_REFERENTIAL_CONSTRAINT_EXIST = A foreign key constraint depends on the table or column.
# *Cause: There is a foreign key constraint that depends on the table or column.
# *Action: Drop the related foreign key constraints first, and then retry this statement.

43,qpERR_ABORT_QDB_NOT_EXIST_NOT_NULL = Unable to alter the column to a nullable column because no NOT NULL constraint is specified for the column.
# *Cause: There is no NULL constraint to drop.
# *Action: Check the column specification.

44,qpERR_ABORT_QDB_NOT_ALLOWED_DROP_NOT_NULL = Unable to drop a NOT NULL constraint because a primary key constraint exists.
# *Cause: A primary key constraint exists for this column.
# *Action: Check the column specification.

45,qpERR_ABORT_QDB_EXIST_NOT_NULL = Unable to add a NOT NULL constraint to the specified column because it already has either a NOT NULL or PRIMARY KEY constraint.
# *Cause: The column already has a NOT NULL constraint.
# *Action: Check the column specification.

47,qpERR_ABORT_QDB_NOTNULL_HAS_NULL = Unable to add a NOT NULL constraint to the specified column because it already contains null values
# *Cause: One or more records in the specified column have null values.
# *Action: Check the column specification.

386,qpERR_ABORT_QDB_DUPLICATE_COLUMN_NAME = Duplicate column name
# *Cause: The same column name exists in the table specification.
# *Action: Check the table specification.

387,qpERR_ABORT_QDB_NOT_ALLOWED_SUBQUERY = A subquery is not allowed here. <0%s>
# *Cause: A subquery is not allowed here.
# *Action: Remove the subquery.

431,qpERR_ABORT_QDB_DDL_ON_CLOSED_TABLE = This table is not in memory. <0%s>
# *Cause: This table is not in memory.
# *Action: Execute ALTER TABLE OPEN.

511,qpERR_ABORT_QDB_HIGHLIMIT_LESS_LOWLIMIT = INSERT HIGH LIMIT cannot be less than or equal to INSERT LOW LIMIT.
# *Cause: INSERT HIGH LIMIT cannot be less than or equal to INSERT LOW LIMIT.
# *Action: Check INSERT HIGH LIMIT and INSERT LOW LIMIT

563,qpERR_ABORT_QDB_FIXED_PAGE_SIZE_ERROR = The fixed record size exceeds the page size.
# *Cause: A table cannot be created because the size of a record exceeds the page size.
# *Action: Create a table whose record size is less than a page size.

566,qpERR_ABORT_QDB_IN_ROW_SIZE_ERROR = IN ROW size exceeds the maximum limit.
# *Cause: A column cannot be created because the IN ROW size exceeds the maximum limit.
# *Action: Create a column whose IN ROW size is less than the maximum limit.

579,qpERR_ABORT_QDB_MISMATCHED_LOB_TYPE_COLUMN = Mismatched LOB type column <0%s>
# *Cause: A LOB type column mismatch has occurred.
# *Action: Check the column specification.

580,qpERR_ABORT_QDB_NOT_FOUND_LOB_TYPE_COLUMN = LOB type column not found.
# *Cause: The specified LOB type column was not found in the table.
# *Action: Check the LOB type column specification.

599,qpERR_ABORT_QDB_LOB_VIOLATION_ON_VOLATILE_TABLE = A volatile table cannot have a LOB column.
# *Cause: You tried to create or alter a table with a LOB column.
# *Action: Retry this action without the LOB column specification.

605,qpERR_ABORT_QDB_DUPLICATE_PARTITION_NAME = Duplicate partition name <0%s>
# *Cause: Duplicate partition name
# *Action: Verify that no duplicate partition names have been specified.

606,qpERR_ABORT_QDB_TOO_MANY_PARTITION_CONDITION_LIST = A partition condition list contains too many elements
# *Cause: Too many partition condition elements
# *Action: Verify that the number of partition condition elements is less than or equal to the number of partition key columns.

607,qpERR_ABORT_QDB_DUPLICATE_PARTITION_KEY_COLUMN_NAME = Duplicate partition key column name <0%s>
# *Cause: Duplicate partition key column name
# *Action: Verify that no duplicate partition key column names are specified.

608,qpERR_ABORT_QDB_INVALID_PARTITION_KEY_COLUMN_TYPE = Invalid partition key column type <0%s>
# *Cause: Invalid partition key column type
# *Action: Check the partition key column type.

609,qpERR_ABORT_QDB_INVALID_PARTITION_KEY_COLUMN_NAME = Invalid partition key column name <0%s>
# *Cause: A column which dose not belong to the partitioned table is used as a partition key.
# *Action: Check the name of partition key column.

610,qpERR_ABORT_QDB_MAX_PARTITION_KEY_COLUMN_COUNT = Too many partition key columns
# *Cause: Only one partition key is allowed in a list partition.
# *Action: Check the number of partition key columns.

611,qpERR_ABORT_QDB_INVALID_DEFAULT_PARTITION_COUNT = The number of default partitions must be one in a range or list partitioned table
# *Cause: Invalid default partition count in a range or list partitioned table.
# *Action: Check the number of default partitions.

612,qpERR_ABORT_QDB_NO_CREATE_PARTITION_IN_MEMORY_TBS = Unable to create a partitioned table or partition in the memory tablespace.
# *Cause: Unable to create a partitioned table or partition in the memory tablespace
# *Action: Check the specified tablespace.

613,qpERR_ABORT_QDB_INVALID_PART_COND_VALUE = Invalid partition key condition value : <0%s>
# *Cause: Partition key condition value is invalid.
# *Action: Check the partition key condition value.

614,qpERR_ABORT_QDB_DUPLICATE_PART_COND_VALUE = Duplicate partition key condition value : <0%s>
# *Cause: Duplicate partition key condition value.
# *Action: Check the partition key condition value.

615,qpERR_ABORT_QDB_INVALID_DEFAULT_PART_COUNT_IN_HASH_PART_TABLE = Hash partitioned table cannot have default partition
# *Cause: Invalid default partition count in hash partitioned table
# *Action: Check the number of default partitions.

616,qpERR_ABORT_QDB_CANNOT_ALTER_TABLE_PARTITION_ON_NONE_PART_TABLE= Unable to alter table partition on non-partitioned table
# *Cause:
#   - Unable to alter table partition on non-partitioned table
# *Action:
#   - Please check the table specification.

617,qpERR_ABORT_QDB_INVALID_PART_TABLE_METHOD_FOR_SPLIT_PARTITION_AT = Split partition AT must be used on range partitioned table
# *Cause: The table is not a range partitioned table
# *Action: Check the partitioning method.

618,qpERR_ABORT_QDB_INVALID_PART_TABLE_METHOD_FOR_SPLIT_PARTITION_VALUES = Split partition VALUES must be used in list partitioned table
# *Cause: The table is not a list partitioned table.
# *Action: Check the partitioning method.

619,qpERR_ABORT_QDB_TOO_MANY_SPLIT_CONDITION_VALUE = Too many split condition values
# *Cause: The number of split condition values should be less than or equal to the number of partition key columns.
# *Action: Check the split condition values.

620,qpERR_ABORT_QDB_CANNOT_SPLIT_HASH_PART_TABLE = Cannot split partition in hash partitioned table
# *Cause: Unable to split a partition in a hash partitioned table.
# *Action: Check the partitioning method.

621,qpERR_ABORT_QDB_SPLIT_CONDITION_VALUE_ON_RANGE_PARTITION = Split condition value must be between PARTITION_MIN_VALUE and PARTITION_MAX_VALUE of the partition to be split
# *Cause: Split condition value is less than PARTITION_MIN_VALUE or more than PARTITION_MAX_VALUE.
# *Action: Check the split condition value.

622,qpERR_ABORT_QDB_NOT_EXIST_SPLIT_CONDITION_VALUE_ON_LIST_PARTITION = Split condition value does not exist for the partition to be split
# *Cause: The split condition value does not exist.
# *Action: Check the split condition values.

623,qpERR_ABORT_QDB_ALREADY_EXIST_SPLIT_CONDITION_VALUE_ON_LIST_PARTITION = Split condition value already exists in another partition
# *Cause: The split condition value already exists in another partition.
# *Action: Check the split condition values.

624,qpERR_ABORT_QDB_ALREADY_EXIST_PARTITION_NAME = The partition name is already used by the partitioned table <0%s>
# *Cause: The partition name is already used by the partitioned table.
# *Action: Check the partition name.

625,qpERR_ABORT_QDB_DROP_PART_KEY = Cannot drop partitioning key
# *Cause:
#   - Unable to drop a column that is related to the partitioning key.
# *Action:
#   - No Action.

626,qpERR_ABORT_QDB_MODIFY_MAXROWS_OF_PARTITION = Cannot modify maxrows of the partitioned table
# *Cause:
#   - Unable to modify the maxrows value for the partitioned table.
# *Action:
#   - No Action.

627,qpERR_ABORT_QDB_CANNOT_MERGE_HASH_PART_TABLE = Cannot merge hash partitioned table
# *Cause: - Unable to merge a hash partitioned table.
# *Action: Check the partitioning method.

628,qpERR_ABORT_QDB_CANNOT_MERGE_NOT_ADJ_PARTITIONS_IN_RANGE_PART_TABLE = Partitions being merged are not adjacent
# *Cause: Nonadjacent partitions cannot be merged.
# *Action: Check the position of the partitions being merged.

629,qpERR_ABORT_QDB_CANNOT_DROP_PARTITION_ON_HASH_PART_TABLE = Cannot drop partition from hash partitioned table
# *Cause: Unable to drop a partition from a hash partitioned table.
# *Action: Check the partitioning method.

630,qpERR_ABORT_QDB_CANNOT_DROP_THE_ONLY_PARTITION = Cannot drop the only partition from a partitioned table
# *Cause: Cannot drop the only partition from a partitioned table.
# *Action: Check the total partition count of the partitioned table.

631,qpERR_ABORT_QDB_CANNOT_DROP_THE_DEFAULT_PARTITION = Cannot drop the default partition
# *Cause: Cannot drop the default partition.
# *Action: Check the partition value type.

632,qpERR_ABORT_QDB_CANNOT_ADD_PARTITION_ON_NONE_HASH_PART_TABLE = Cannot add a partition to a non-hash partitioned table
# *Cause: Unable to add a partition to a non-hash partitioned table.
# *Action: Check the partitioning method.

633,qpERR_ABORT_QDB_CANNOT_COALESCE_PARTITION_ON_NONE_HASH_PART_TABLE = Cannot coalesce partitions in non-hash partitioned table
# *Cause: Unable to coalesce partitions in non-hash partitioned table.
# *Action: Check the partitioning method.

634,qpERR_ABORT_QDB_CANNOT_COALESCE_THE_ONLY_PARTITION = Cannot coalesce the only partition of a partitioned table
# *Cause: Cannot coalesce the only partition of a partitioned table.
# *Action: Check the total partition count of a partitioned table.

635,qpERR_ABORT_QDB_TOO_LONG_PARTITION_CONDITION_VALUE = The length of a partition condition value is too long
# *Cause: Partition condition value too long.
# *Action: Check the length of all partition condition values.

636,qpERR_ABORT_DUP_SPLIT_COND_VALUE_ON_LIST_PARTITION = Duplicate split condition value
# *Cause: Duplicate split condition value
# *Action: Check the split condition values for duplicates.

637,qpERR_ABORT_QDB_NOT_EXIST_PARTITIONED_INDEX = The partitioned index was not found in the partitioned table : <0%s>
# *Cause: Partitioned index not found.
# *Action: Check the partitioned index name.

638,qpERR_ABORT_QDB_DUP_PARTITIONED_INDEX_NAME = Duplicate partitioned index name : <0%s>
# *Cause: Duplicate partitioned index name.
# *Action: Check all partitioned index names for duplicates.

679,qpERR_ABORT_QDB_DUPLICATE_TABLE_ATTRIBUTE = Duplicate table attribute.  <0%s>
# *Cause: The user tried to specify a duplicate attribute name in the list of table attributes.
# *Action: Check the list of table attributes.

680,qpERR_ABORT_QDB_UNABLE_TO_COMPRESS_VOLATILE_TBS_LOG = Log compression not supported for volatile tablespace.
# *Cause: The user tried to compress a volatile tablespace log.
# *Action: No action is necessary.

683,qpERR_ABORT_QDB_NOT_ALLOWED_PROC_IN_DEFAULT = Procedure or function not allowed here. <0%s>
# *Cause: A procedure or function was specified in an inappropriate place in a statement.
# *Action: Verify that the procedure or function name is correct and in the correct place, or remove it.

708,qpERR_ABORT_QDB_REPLICATION_DDL_DISABLED = Cannot execute this DDL on a replicated table when the system property REPLICATION_DDL_ENABLE is 0.
# *Cause: System property REPLICATION_DDL_ENABLE is 0.
# *Action: Check the system property REPLICATION_DDL_ENABLE.

709,qpERR_ABORT_QDB_CANNOT_DDL_WITH_RECOVERY = Cannot execute this DDL with replication recovery.
# *Cause: Replication recovery is set.
# *Action: Check replication recovery.

710,qpERR_ABORT_QDB_CANNOT_WRITE_REPL_INFO = Cannot execute this DDL on a replicated table when the session property REPLICATION is NONE.
# *Cause: Session property REPLICATION is set to NONE.
# *Action: Check the session property REPLICATION.

726,qpERR_ABORT_QDB_ADD_COL_UNIQUE_KEY_WITH_REPLICATED_TBL = It is forbidden to add a unique key column to a replicated table.
# *Cause: A unique key column cannot be added to a replicated table.
# *Action: Drop the replication.

728,qpERR_ABORT_QDB_ADD_COL_NOT_NULL_WITH_REPLICATED_TBL = It is forbidden to add a not null column to a replicated table.
# *Cause: A NOT NULL column cannot be added to a replicated table.
# *Action: Drop the replication.

729,qpERR_ABORT_QDB_DROP_COL_NOT_NULL_WITH_REPLICATED_TBL = It is forbidden to drop a not null column from a replicated table.
# *Cause: A NOT NULL column cannot be dropped from a replicated table.
# *Action: Drop the replication.

731,qpERR_ABORT_QDB_DROP_COL_HAS_REPL_CONDITION = It is forbidden to drop a column for which a replication condition exists.
# *Cause: A column with a replication condition cannot be dropped.
# *Action: Remove the replication condition.

744,qpERR_ABORT_QDB_INVALID_INIT_TRANS = The INITRANS value must be between <0%d> and <1%d>
# *Cause: Invalid INITRANS value.
# *Action: Check the INITRANS value.

745,qpERR_ABORT_QDB_INVALID_MAX_TRANS = The MAXTRANS value must be between <0%d> and <1%d>
# *Cause: Invalid MAXTRANS value.
# *Action: Check the MAXTRANS value.

746,qpERR_ABORT_QDB_MAXTRANS_LESS_INITTRANS = MAXTRANS cannot be less than INITRANS.
# *Cause: The MAXTRANS value cannot be less than the INITRANS value.
# *Action: Check the INITRANS and MAXTRANS values.

749,qpERR_ABORT_QDB_INVALID_MODIFICATION = Invalid modification of column <0%s>
# *Cause: 
#  - 1. Modifying the data type on the columns with a type of char, blob, clob, nibble, byte, timestamp, or geometry is not allowed.
#  - 2. Modifying data type of columns into blob, clob, timestamp, or geometry is not allowed.
#  - 3. The columns with a type of char, varchar, echar, evarchar cannot be encrypted using the ENCRYPT clause.
#  - 4. The data type of a column cannot be modified; however, the data type length can be increased if the column refers to a foreign key.
#  - 5. The data type of a column cannot be modified; however, the data type can be increased if the column is used as a partition key.
# *Action: Check modification of the column.

750,qpERR_ABORT_QDB_INVALID_LENGTH = Invalid length for the data type <0%s>
# *Cause: The data type length was invalid.
# *Action: Check the length of the data type.

751,qpERR_ABORT_QDB_INVALID_NULL_VALUE = Unable to add NOT NULL constraint to the specified column because it already contains null values <0%s>
# *Cause: One or more records in the specified column has a null value.
# *Action: Check the column specification.

764,qpERR_ABORT_QDB_COMMENT_TOO_LONG = Comment string is too long.
# *Cause: The comment string is too long. It must not exceed 4000 bytes.
# *Action: Check the length of the comment string.

765,qpERR_ABORT_QDB_COMMENT_NOT_SUPPORT_SEQUENCE = COMMENT ON statements are not supported for sequences.<0%s>
# *Cause: COMMENT ON statements are not supported for sequences.
# *Action: Check the object type.

766,qpERR_ABORT_QDB_COMMENT_NOT_SUPPORT_PSM = COMMENT ON statements are not supported for PSMs. <0%s>
# *Cause: COMMENT ON statements are not supported for PSMs.
# *Action: Check the object type.

767,qpERR_ABORT_QDB_COMMENT_NOT_SUPPORT_OBJ = COMMENT ON statements are not supported for this Object. <0%s>
# *Cause: COMMENT ON statements are only supported for user tables and views.
# *Action: Check the object type.

794,qpERR_ABORT_QDB_NOT_ALLOWED_ENCRYPTED_COLUMN_ACCESS = Encrypted column access is not permitted due to the security module.
# *Cause: Encrypted column access is not permitted due to the security module.
# *Action: Check the security module.

799,qpERR_ABORT_QDB_ALREADY_EXIST_INDEX_PARTITION_NAME = The index name for a new partition must differ from those of existing index partitions. <0%s>
# *Cause: Index names of new partitions must differ from existing index partition names.
# *Action: Ensure that the new index partition name is different from the name of any existing partition (including the one being renamed) of a given table or index.

800,qpERR_ABORT_QDB_INVALID_INDEX_PARTITION_NAME = The specified index partition name did not match the name of an existing index partition. <0%s>
# *Cause: The specified index partition name did not match an existing index partition.
# *Action: Ensure that the index partition name exists.

801,qpERR_ABORT_QDB_DROP_MULTI_COLUMN_CONSTRAINT_EXIST = It is forbidden to drop a column that has multi-column constraints.
# *Cause: A column with multi column constraints cannot be dropped.
# *Action: Drop all multi-column constraints related to the dropped column first, and then retry this statement.

826,qpERR_ABORT_QDB_INVALID_TBS_FOR_PCTFREE_PCTUSED = PCTFREE and PCTUSED can only be used with disk tablespaces.
# *Cause: PCTFREE and PCTUSED can only be used with disk tablespaces.
# *Action: Check the tablespace type.

827,qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME = Invalid object name : <0%s>
# *Cause: Object names cannot start with reserved words.
# *Action: Change the object name.

862,qpERR_ABORT_QDB_INVALID_TABLE_OPTION = Invalid table option <0%s>
# *Cause: A table option was missing or invalid.
# *Action: Check the table options.

863,qpERR_ABORT_QDB_CANNOT_CREATE_TEMPORARY_TABLE_IN_NONVOLATILE_TBS = Temporary tables cannot be created in non-volatile tablespaces.
# *Cause: An attempt was made to create a temporary table in a tablespace other than a volatile tablespace.
# *Action: Create the temporary table in a volatile tablespace.

864,qpERR_ABORT_QDB_NOT_SUPPORTED_TEMPORARY_TABLE_FEATURE = Unsupported temporary table feature
# *Cause: One of the table features specified in the temporary table creation statement is not supported.
# *Action: Avoid specifying unsupported features when creating temporary tables.

867,qpERR_ABORT_QDB_TEMPORARY_TABLE_DDL_DISABLE = Cannot execute DDL when a temporary table is in use.
# *Cause: The DDL could not be executed because there are one or more temporary tables based on the table.
# *Action: Truncate all temporary tables based on the table and try again.

906,qpERR_ABORT_QDB_ADD_COL_CHECK_WITH_REPLICATED_TBL = It is forbidden to add a column having check constraint to a replicated table.
# *Cause: A column with a check constraint cannot be added to a replicated table.
# *Action: Drop the replication.

907,qpERR_ABORT_QDB_DROP_COL_CHECK_WITH_REPLICATED_TBL = It is forbidden to drop a column having check constraint from a replicated table.
# *Cause: A column with a check constraint cannot be dropped from a replicated table.
# *Action: Drop the replication.

908,qpERR_ABORT_QDB_USE_SEQUENCE_IN_CHECK_CONSTRAINT = A sequence cannot be used as a CHECK constraint in a CREATE or ALTER TABLE statement. <0%s>
# *Cause: A sequence cannot be used as a CHECK constraint in a CREATE or ALTER TABLE statement.
# *Action: Remove the sequences.

909,qpERR_ABORT_QDB_USE_VARIABLE_IN_CHECK_CONSTRAINT = A variable cannot be used as a CHECK constraint in a CREATE or ALTER TABLE statement. <0%s>
# *Cause: A variable cannot be used as a CHECK constraint in a CREATE or ALTER TABLE statement.
# *Action: Remove the variables.

910,qpERR_ABORT_QDB_NOT_ALLOWED_CHECK_CONSTRAINT = A check constraint is not allowed here. <0%s>
# *Cause: A check constraint cannot be used here.
# *Action: Remove the check constraints.

911,qpERR_ABORT_QDB_NOT_ALLOWED_COLUMN_HAVING_CHECK_CONSTRAINT = A column having check constraint is not allowed here. <0%s>
# *Cause: A column with a check constraint cannot be used here.
# *Action: Remove the check constraints.

921,qpERR_ABORT_QDB_INVALID_DEFAULT_EXPRESSION = Invalid default expression : <0%s>
# *Cause: The default expression is incorrect.
# *Action: Check the default expression.

925,qpERR_ABORT_QDB_CANNOT_USE_HIDDEN_COLUMN = Cannot use columns generated by a function-based index <0%s>
# *Cause: Cannot use columns generated by a function-based index
# *Action: Remove the columns.

926,qpERR_ABORT_QDB_CANNOT_ALTER_HIDDEN_COLUMN = Cannot alter hidden columns <0%s>
# *Cause: Cannot alter hidden columns
# *Action: Check the column type.

950,qpERR_ABORT_QDB_COMPRESSION_NOT_SUPPORTED_DATATYPE = Unsupported data type on compression column. <0%s>
# *Cause: Dictionary based compression columns do not support some types
# *Action: Check the column type.

951,qpERR_ABORT_QDB_COMPRESSION_NOT_SUPPORTED_TABLESPACE = Unsupported tablespace type on compression column.
# *Cause: Dictionary based compression columns support only memory tablespaces.
# *Action: Check the tablespace type.

952,qpERR_ABORT_QDB_CANNOT_DDL_DML_DICTIONARY_TABLE = Cannot execute DDL/DML on dictionary table.
# *Cause: DDL/DML cannot be executed on a dictionary table.
# *Action: Check the specified table.

953,qpERR_ABORT_QDB_CANNOT_CREATE_COMPRESSION_COLUMN_REPLICATION = Cannot create compression column on the replicated table <0%s>
# *Cause: Compression columns do not support replication.
# *Action: Drop the compression column.

954,qpERR_ABORT_QDB_CANNOT_ALTER_COMPRESSION_COLUMN = Cannot alter compression column <0%s>
# *Cause: Cannot alter compression columns.
# *Action: Check the column type.

979,qpERR_ABORT_QDB_NO_CREATE_PARTITION_IN_COMPOSITE_TBS = Partitions of a partitioned table cannot be stored in both memory and disk tablespaces.
# *Cause: Partitions of a partitioned table cannot be stored in both memory and disk tablespaces.
# *Action: Check whether the tablespace types are the same.

977,qpERR_ABORT_QDB_UNABLE_TO_COMPRESS_MEM_PARTITION = Columnar compression is unsupported for memory partitioned tables.
# *Cause: Columnar compression is unsupported for memory partitioned tables.
# *Action: Check whether the table is a memory partitioned table.
    
980,qpERR_ABORT_QDB_CANNOT_ALTER_REORGANIZE_COMPRESSION_COLUMN = ALTER REORGANIZE statement cannot be executed on uncompressed columns. <0%s>
# *Cause: The 'ALTER REORGANIZE' statement cannot executed on uncompressed columns.
# *Action: Check the column.Check whether the specified column is compressed or not.

1006,qpERR_ABORT_QDB_ADD_COL_COMPRESSION_COL_WITH_REPLICATED_TBL = A compressed column cannot be added to a replication target table.
# *Cause: A compressed column cannot be added to a replication target table.
# *Action: Do not add a compressed column to a replication target table.

1007,qpERR_ABORT_QDB_CANNOT_ALTER_REORGANIZE_COMPRESSION_COL_WITH_REPLICATED_TBL = A compressed column of a replication target table cannot be reorganized.
# *Cause: A compressed column of a replication target table cannot be reorganized.
# *Action: Check whether the specified column is compressed or not.

1008,qpERR_ABORT_QDB_DROP_COMPRESSED_COLUMN_WITH_REPLICATE_TABLE = A compressed column cannot be dropped from a replication target table.
# *Cause: A compressed column cannot be dropped from a replication target table.
# *Action: Check whether the specified column is compressed or not.

1009,qpERR_ABORT_QDB_CANNOT_TRUNCATE_COMPRESSED_TABLE = A compressed table which is a replication target table cannot be truncated.
# *Cause: A compressed table which is a replication target table cannot be truncated.
# *Action: Check whether the specified table is compressed or not.

1035,qpERR_ABORT_QDB_UNABLE_TO_COMPRESS_DISK_PARTITION = Columnar compression is unsupported for disk partitioned tables.
# *Cause: Columnar compression is unsupported for disk partitioned tables.
# *Action: Check whether the table is a disk partitioned table.

1072,qpERR_ABORT_QDB_DUPLICATE_TABLE_NAME = Duplicate table name <0%s>
# *Cause: Duplicate table name, or the name is already used by an existing object.
# *Action: Verify that no duplicate table names have been specified, or enter a unique table name. 

1073,qpERR_ABORT_QDB_CANNOT_JOIN_DISJOIN_HASH_OR_NON_PART_TBL = Cannot conjoin/disjoin a hash partitioned table or a non-partitioned table.
# *Cause: You can conjoin/disjoin tables only with non-hash partitioned tables.
# *Action: Verify that the target tables are range/list partitioned tables.

1074,qpERR_ABORT_QDB_CANNOT_JOIN_PARTITIONED_TABLE = Unable to conjoin partitioned tables : <0%s>
# *Cause: Only partitioned tables can be conjoined.
# *Action: Verify that the target table is not a partitioned table.

1075,qpERR_ABORT_QDB_CANNOT_JOIN_DISJOIN_TABLE_SPEC = Unable to conjoin/disjoin tables with primary keys, unique keys, foreign keys, indexes, and triggers. : <0%s>
# *Cause: Only partitions without private keys, unique keys, foreign keys, indexes, nor triggers can be exchanged.
# *Action: Verify that the table does not have any constraints, indexes, or triggers.

1076,qpERR_ABORT_QDB_JOIN_DISJOIN_NOT_READ_WRITE = Not READ/WRITE : <0%s>
# *Cause: A read only or read append table cannot be conjoined/disjoined
# *Action: Check the access option of the table.

1077,qpERR_ABORT_QDB_JOIN_DIFFERENT_TABLE_SPEC = Unidentical table/column spec : <0%s>
# *Cause: Unidentical table/column specification.
# *Action: Verify that the specifications(number of constraints, number/names/order/type/scale/precision of columns, etc.) are identical.

1078,qpERR_ABORT_QDB_JOIN_DIFFERENT_CONSTRAINT_SPEC = Unidentical constraint specification : <0%s>
# *Cause: The table's constraint specification unidentical.
# *Action: Verify that the constraint specifications(type, scale, precision, etc.) are identical.

1079,qpERR_ABORT_QDB_CANNOT_JOIN_DISJOIN_COLUMN_SPEC = Unable to conjoin/disjoin hidden/encrypted/compressed columns : <0%s>
# *Cause: Only partitions without any hidden/encrypted/compressed columns can be exchanged.
# *Action: Verify that there are no hidden/encrypted/compressed columns.

1080,qpERR_ABORT_QDB_DISJOIN_NOT_ALL_PARTITION = All partitions should be listed in the DISJOIN table statement.
# *Cause: One or more partitions are missing from the SQL statement.
# *Action: Verify that every partition is in the SQL statement.

1082,qpERR_ABORT_QDB_CANNOT_SUPPORT_ON_HYBRID_PARTITIONED_TABLE = It is unsupported for the hybrid partitioned tables.
# *Cause: It is unsupported for hybrid partitioned tables.
# *Action: Check whether the table is a hybrid disk partitioned table.

1083,qpERR_ABORT_QDB_CANNOT_ALTER_PARTITION_ON_SAME_TABLESPACE = Alter partition cannot be move rows on same tablespaces.
# *Cause: Alter partition cannot be move rows on same tablespaces.
# *Action: Check whether the tablespace of a target partition is a same tablespace.

1084,qpERR_ABORT_QDB_MINEXTENTS_LESS_INITEXTENTS = MINEXTENTS cannot be less than INITEXTENTS.
# *Cause: The MINEXTENTS value cannot be less than the INITEXTENTS value.
# *Action: Check the INIEXTENTS and MINEXTENTS values.

1085,qpERR_ABORT_QDB_MINEXTENTS_LESS_MAXEXTENTS = MINEXTENTS cannot be less than MAXEXTENTS.
# *Cause: The MINEXTENTS value cannot be less than the MAXEXTENTS value.
# *Action: Check the MAXEXTENTS and MINEXTENTS values.

1099,qpERR_ABORT_QDB_CANNOT_ALTER_TABLESPACE_ON_SAME_TABLESPACE = Alter tablespace cannot be move rows on same tablespaces.
# *Cause: The user tried to move rows by executing a MOVE statement on the same tablespace. Alter tablespace cannot be move rows on the same tablespaces.
# *Action: Do not execute a MOVE statement on the rows in the same tablespace. Change the tablespace of target table.

1100,qpERR_ABORT_QDB_CANNOT_ALTER_TABLESPACE_ON_NONE_PART_INDEX = Cannot alter the tablespace for a partitioned table with a non-partitioned index.
# *Cause: The tablespace cannot be altered for a partitioned table with a non-partitioned index.
# *Action: Remove the non-partitioned index.

1101,qpERR_ABORT_QDB_MEMORY_USAGE_THRESHOLD = Need more memory resource. Progress : <0%u>%, Threshold : <1%u>%, Estimated Page Count : <2%lu>.
# *Cause: There is not enough free space in the memory to execute this DDL statements.
# *Action: Free up space of memory.

1102,qpERR_ABORT_QDB_TBS_USAGE_THRESHOLD = Need more free space of tablespace. Tablespace Name: <0%s>, Progress : <1%u>%, Threshold : <2%u>%, Estimated Page Count : <3%lu>.
# *Cause: The tablespace does not have enough free space to execute DDL.
# *Action: Free up space of tablespace.

1103,qpERR_ABORT_QDB_NOT_EXIST_INDEX = The index was not found in the table : <0%s>
# *Cause: Index not found.
# *Action: Check the index name.

1112,qpERR_ABORT_QDB_CANNOT_ALTER_TABLESPACE_TEMPORARY_TABLE = A temporary table cannot modify the tablespace.
# *Cause: ALTER TABLESPACE cannot be executed since a temporary table is executed within the same tablespace.
# *Action: Do not execute the syntax of ALTER TABLESPACE on the temporary table.

1115,qpERR_ABORT_QDB_USE_ONLY_DISK_TABLE_PARTITION_IN_DISK_EDITION = Memory/volatile tables or partitions cannot be used in disk edition.
# *Cause: Only disk tables and partitions are allowed to use in the disk edition.
# *Action: Use disk tables or disk partitions, otherwise purchase the standard/enterprise edition.

1116,qpERR_ABORT_QDB_SET_ONLY_DISK_TO_DEFAULT_TABLESPACE_IN_DISK_EDITION = Memory tablelspace or volatile tablespace cannot be specified as the default tablespace in the disk edition.
# *Cause: The default tablespace can be specified only with disk tablespace in the disk edition.
# *Action: Specify disk tablespace as the default tablespace or purchase the standard/enterprise edition.

1118,qpERR_ABORT_QDB_USE_SYNONYM_IN_CHECK_CONSTRAINT = A synonym is not allowed to be a CHECK constraint in a CREATE or ALTER TABLE statement. <0%s>
# *Cause: The synonym was used as a CHECK constraint in a CREATE or ALTER TABLE statement.
# *Action: Remove the synonyms used in the statement.

1120,qpERR_ABORT_QDB_DDL_NOT_SUPPORTED_REPLICATED_TABLE = This DDL is not supported on replicated table.(Cause : <0%s>)
# *Cause: This DDL is not supported on replicated table.
# *Action: Identify the cause and take appropriate action.

1121,qpERR_ABORT_QDB_COPY_SWAP_SUPPORT_NORMAL_USER_TABLE = Invalid table type. <0%s>
# *Cause: Copying or swapping DDL supports a normal user table.
# *Action: Attempt this action on a normal user table.

1122,qpERR_ABORT_QDB_SELF_SWAP_DENIED = Unable to swap the same table.
# *Cause: The source table and the target table are the same.
# *Action: Change the source table name or the target table name.

1123,qpERR_ABORT_QDB_SWAP_NOT_SUPPORT_REPLICATION_WITH_COMPRESSED_COLUMN = Invalid replication with compressed columns. <0%s>
# *Cause: Swapping DDL does not support a replication with compressed columns.
# *Action: Remove the compressed columns.

1124,qpERR_ABORT_QDB_REPL_PARTITION_METHOD_MISMATCH = The partition method of the replicated table does not match.
# *Cause: Replicated table partition method mismatch.
# *Action: Check the partition method of the replicated table.

1125,qpERR_ABORT_QDB_REPL_PARTITION_CONDITION_MISMATCH = The partition condition of the replicated table does not match. [<0%s>:<1%s>]
# *Cause: Replicated table partition condition mismatch.
# *Action: Check the partition condition of the replicated table.

1126,qpERR_ABORT_QDB_REPL_PARTITION_ORDER_MISMATCH = The partition order of the replicated table does not match. [<0%s>:<1%s>]
# *Cause: Replicated table partition order mismatch.
# *Action: Check the partition order of the replicated table.

1127,qpERR_ABORT_QDB_REPL_TABLE_PARTITION_TYPE_MISMATCH = The partition type of the replicated table does not match.
# *Cause: Replicated table partition type mismatch.
# *Action: Check the partition type of the replicated table.

1128,qpERR_ABORT_QDB_REPL_PARTITION_NAME_MISMATCH = The partition name of the replicated table does not match. [<0%s>]
# *Cause: Replicated table partition name mismatch.
# *Action: Check the partition name of the replicated table.

1129,qpERR_ABORT_QDB_USING_TARGET_NAMES_PREFIX_IS_TOO_LONG = USING PREFIX clause is too long.
# *Cause: A generated object name is too long or short.
# *Action: Give the prefix a shorter name.

1130,qpERR_ABORT_QDB_SWAP_TABLES_EXIST_IN_SAME_REPLICATION = The source table and the target table exist in same replication. <0%s>
# *Cause: Swapping DDL does not support the source table and the target table in same replication.
# *Action: Remove the source table or the target table from the replication.

1131,qpERR_ABORT_QDB_COPY_SWAP_DIFFERENT_TABLE_OWNER = The owners of the source table and the target table are different.
# *Cause: Copying and swapping DDL does not support different table owners.
# *Action: Check the owners of the source table and the target table.

1132,qpERR_ABORT_QDB_DROP_COL_UNIQUE_INDEX_WITH_REPLICATED_TBL = A column contains an unique index that can not be dropped from a replicated table.
# *Cause: A column contains an unique index that can not be dropped from a replicated table.
# *Action: Drop column again after dropping the replication.

1134,qpERR_ABORT_QDB_CANNOT_ALTER_REPLACE_PARTITON_NONE_PART = ALTER TABLE REPLACE statement is not allowed on a non partitioned table.
# *Cause: The user tried to execute ALTER TABLE REPLACE statement, but does not support different schema in the partitioned table. 
# *Action: Check the partitioning method of the table.

1135,qpERR_ABORT_QDB_REPLACE_DIFFERENT_PARTITION = The schema is different between the source partition table and the destination partition table.
# *Cause: The user tried to execute a replace DDL command, but does not support different schema in the partitioned table.
# *Action: Verify that the source and target tables have the same schema.

1139,qpERR_ABORT_QDB_INVALID_SRID = Cannot change the Spatial Reference ID(SRID).
# *Cause: There is an invalid Spatial Reference ID(SRID) because the specific row has been SRID.
# *Action: Check that a valid Spatial Reference ID(SRID) is being used.

1180,qpERR_ABORT_QDB_NO_SHARD_TABLE = Unable to change the usable state for the none shard table.
# *Cause: It is possible to change the usable state only for the shard table.
# *Action: Check if the table is a shard table.

1184,qpERR_ABORT_QDB_BOUND_HIGHER_THAN_LAST_PARTITION = Partition bound must higher than that of the last partition
# *Cause: Partition bound specified in DDL did not collate higher than that of the table's last partition.
# *Action: Check bound of table's last partition.

1185,qpERR_ABORT_QDB_TOO_MANY_ADD_CONDITION_VALUE = Too many add condition values
# *Cause: The number of add condition values should be less than or equal to the number of partition key columns.
# *Action: Check the add condition values.

1186,qpERR_ABORT_QDB_CANNOT_ADD_PARTITION_ON_NONE_RANGE_PART_TABLE = Cannot add a partition to a non-range partitioned table.
# *Cause: Unable to add a partition to a non-range partitioned table.
# *Action: Check the partitioning method or check syntax of the statement.

###############################################################################
#########                 QCV ERROR CODE                              #########
###############################################################################
48,qpERR_ABORT_QCV_NO_GRANT = You do not have permission to execute the SQL statement.
# *Cause: You do not have permission to execute the SQL statement. (The SYS user can access an arbitrary object in any SQL statement.)
# *Action: Verify that the user has enough permission to execute the SQL statement.

393,qpERR_ABORT_QCV_NOT_EXISTS_USER = User not found : <0%s>
# *Cause: The name of the specified user was not found in the database.
# *Action: Verify that the user exists in the database.

49,qpERR_ABORT_QCV_NOT_EXISTS_TABLE = Table or view was not found : <0%s>
# *Cause: The name of the specified object was not found in the database.
# *Action: Verify that the object exists in the database. Verify that there is no typo in the object name.

50,qpERR_ABORT_QCV_NOT_ALLOWED_HOSTVAR = Invalid use of host variables
# *Cause: A host variable was misused.
# *Action: Verify the position of every host variable.

51,qpERR_ABORT_QCV_UNSUPPORTED = Unsupported function <0%s>
# *Cause:
#   - The function is not supported yet.
# *Action:
#   - No Action

409,qpERR_ABORT_QCV_DML_ON_SEQUENCE = Unable to execute DML statements on a sequence. <0%s>
# *Cause: It is impossible to execute DML statements on a sequence.
# *Action: Specify a table name on which to execute the DML statements.

639,qpERR_ABORT_QCV_NOT_PARTITIONED_TABLE = Table is not partitioned : <0%s>
# *Cause: The table is not partitioned.
# *Action: Verify that the specified table is partitioned.

###############################################################################
#########                 QDS ERROR CODE                              #########
###############################################################################
53,qpERR_ABORT_QDS_DUPLICATE_SEQUENCE = Duplicate sequence name
# *Cause: The sequence name already exists in the database.
# *Action: Create a new sequence with a different name.

54,qpERR_ABORT_QDS_START_LESS_MIN = START value cannot be less than MIN value.
# *Cause: The START value cannot be less than the MIN value.
# *Action: Check the START and MIN values.

55,qpERR_ABORT_QDS_START_MORE_MAX = START value cannot be greater than MAX value.
# *Cause: The START value cannot be greater than the MAX value.
# *Action: Check the START and MAX values.

411,qpERR_ABORT_QDS_CURR_LESS_MIN = CURRENT value cannot be less than MIN value.
# *Cause: The CURRENT value cannot be less than the MIN value.
# *Action: Check the CURRENT and MIN values.

412,qpERR_ABORT_QDS_CURR_MORE_MAX = CURRENT value cannot be greater than MAX value.
# *Cause: The CURRENT value cannot be greater than the MAX value.
# *Action: Check the CURRENT and MAX values.

56,qpERR_ABORT_QDS_MIN_VALUE = MIN value must be less than MAX value.
# *Cause: The MIN value must be less than the MAX value.
# *Action: Check the MIN and MAX values.

57,qpERR_ABORT_QDS_CACHE_VALUE = CACHE value must be greater than 1.
# *Cause: The CACHE value must be greater than 1.
# *Action: Check the CACHE value.

58,qpERR_ABORT_QDS_INCREMENT_VALUE = INCREMENT value must be a non-zero integer.
# *Cause: The INCREMENT value must be a non-zero integer.
# *Action: Check the INCREMENT value.

59,qpERR_ABORT_QDS_INVALID_INCREMENT_VALUE = INCREMENT value must be less than the difference between MAX and MIN values.
# *Cause: The INCREMENT value must be less than the difference between the MAX and MIN values.
# *Action: Check the INCREMENT, MAX, and MIN values.

796,qpERR_ABORT_QDS_OVERFLOW_MAXVALUE = MAXVALUE is greater than internal sequence maximum value (<0%s>).
# *Cause: The MAXVALUE specified for a query was greater than the maximum value allowed for the server.
# *Action: Change the value so that it is less than the maximum and repeat the query.

797,qpERR_ABORT_QDS_OVERFLOW_MINVALUE = MINVALUE is less than internal sequence minimum value (<0%s>).
# *Cause: The MINVALUE specified for a query was less than the minimum value allowed for the server.
# *Action: Change the value so that it is greater than the minimum and repeat the query.

981,qpERR_ABORT_QDS_CANNOT_ALTER_SEQ_TABLE = Unable to alter the sequence with sync table option
# *Cause: Unable to alter the sequence with the sync table option.
# *Action: Change the sync table option of the sequence.

1155,qpERR_ABORT_QDS_CANNOT_ALTER_SHARDED_SEQUENCE = Scale of sharded sequence cannot be changed.
# *Cause: Scale of sharded sequence cannot be changed.
# *Action: Check scale of sharded sequence.

###############################################################################
#########                 QDR ERROR CODE                              #########
###############################################################################
60,qpERR_ABORT_QDR_NOT_EXISTS_USER = Undefined user name. The user specified as the owner of a table or an object was not found in the database. <0%s>
# *Cause: The user specified as the owner of a table or an object was not found in the database.
# *Action: Verify that a user name corresponding to the owner of an object is registered in the database.

61,qpERR_ABORT_QDR_DUPLICATE_USER = Duplicate user or role names. The user name already exists in the database.
# *Cause: The user or role name to be created already exists in the database.
# *Action: Use another user or role name.
    
876,qpERR_ABORT_QDR_INVALID_RESOURCE_LIMIT = Internal resource limit exceeded
# *Cause: A specified value exceeded an internal server limitation.
# *Action: Specify a lower value.
    
877,qpERR_ABORT_QDR_PASSWORD_POLICY_SYSDBA = Only SYS user can change password policy
# *Cause: Only the SYS user can change password options.
# *Action: Log in as the SYS user and try again.
                         
878,qpERR_ABORT_QDR_NOT_ALLOWED_LOCK = Only SYS user can lock users
# *Cause: Only the SYS user can explicitly lock other users.
# *Action: Log in as the SYS user and try again.
                                                                                             
879,qpERR_ABORT_QDR_COUNT_LIMIT = Internal count limit exceeded
# *Cause: A specified count value exceeded the internal server limitation of 1000.
# *Action: Specify a count limit less than 1000.

###############################################################################
#########                 QDD ERROR CODE                              #########
###############################################################################
63,qpERR_ABORT_QDD_DDL_WITH_REPLICATED_TBL = No DDL statement may be executed on a replicated table.
# *Cause: An attempt was made to execute a DDL statement on a replicated table.
# *Action: Verify that the specified table is not replicated.

64,qpERR_ABORT_QDD_NO_DROP_SYS_USER = It is forbidden to drop the SYS user.
# *Cause: You cannot drop the SYS user.
# *Action: Verify that the specified user name is not SYS.

65,qpERR_ABORT_QDD_EXIST_OWN_OBJECT = The user cannot be dropped because the user still owns one or more objects.
# *Cause: The user to be dropped owns at least one object.
# *Action: Drop every object owned by the user being dropped.

414,qpERR_ABORT_QDD_DROP_SYSTEM_INDEX = The primary key or unique key constraint cannot be dropped using the DROP INDEX statement. Use the ALTER TABLE statement.
# *Cause: The primary key or unique key constraint cannot be dropped using the DROP INDEX statement.
# *Action: Use the ALTER TABLE statement.

421,qpERR_ABORT_QDD_NO_DROP_META_TABLE = It is forbidden to execute ALTER, DROP or TRUNCATE TABLE statements on a meta table.
# *Cause: You cannot alter, drop or truncate meta tables.
# *Action: Check the table name.

640,qpERR_ABORT_QDD_PART_TABLE_BELONGS_TO_ANOTHER_USER = Partitioned table belongs to another user
# *Cause: The partitioned table belongs to another user.
# *Action: Drop the partitioned table first, and then retry this statement.

641,qpERR_ABORT_QDD_PART_INDEX_BELONGS_TO_ANOTHER_USER = Partitioned index belongs to another user
# *Cause: The partitioned index belongs to another user.
# *Action: Drop the partitioned index first, and then retry this statement.

711,qpERR_ABORT_QDD_DROP_COL_PRIMARY_KEY_WITH_REPLICATED_TBL = It is forbidden to drop a primary key column from a replicated table.
# *Cause: A primary key column cannot be dropped from a replicated table.
# *Action: Drop the replication.

727,qpERR_ABORT_QDD_DROP_COL_UNIQUE_KEY_WITH_REPLICATED_TBL = It is forbidden to drop a unique key column from a replicated table.
# *Cause: A unique key cannot be dropped from a replicated table.
# *Action: Drop the replication.

730,qpERR_ABORT_QDD_INVALID_STORAGE_OPTION_VALUE = invalid <0%s> storage option value.
# *Cause: Invalid STORAGE clause option value.
# *Action: Specify a valid option value.

870,qpERR_ABORT_QDD_DROP_USER_DISABLE_BECAUSE_TEMP_TABLE = Could not drop user because temporary table in use.
# *Cause: The user could not be dropped because there are one or more temporary tables based on tables in the user schema.
# *Action: Truncate all temporary tables based on tables in the user schema and try again.

884,qpERR_ABORT_QDD_NOT_EXIST_MVIEW = Materialized view not found : <0%s>
# *Cause: The name of the specified materialized view was not found in the database.
# *Action: Verify that the materialized view exists in the database. Verify that the name of the materialized view was entered correctly.

###############################################################################
#########                 QDN ERROR CODE                              #########
###############################################################################
66,qpERR_ABORT_QDN_NOT_EXISTS_CONSTRAINT = Constraint not found.
# *Cause: The specified constraint name was not found in the database.
# *Action: Verify that the constraint name exists in the SYS_CONSTRAINTS_ meta table.

67,qpERR_ABORT_QDN_NOT_EXISTS_UNIQUE_KEY = UNIQUE KEY constraint not found in the table.
# *Cause: The UNIQUE KEY constraint was not found in the table.
# *Action: Check the UNIQUE KEY constraint in the meta tables.

68,qpERR_ABORT_QDN_NOT_EXISTS_PRIMARY_KEY = PRIMARY KEY constraint not found in the table.
# *Cause: The PRIMARY KEY constraint was not found in the table.
# *Action: Check the PRIMARY KEY constraint in the meta tables.

69,qpERR_ABORT_QDN_DUPLICATE_PRIMARY_KEY = A primary key already exists in the table.
# *Cause: The table already has a primary key. A table can have only one primary key.
# *Action: If a primary key already exists, drop the key before creating a new one.

70,qpERR_ABORT_QDN_DUPLICATE_CONSTRAINT = Duplicate constraint name : <0%s>
# *Cause: The constraint name already exists in the database.
# *Action: If a constraint with the same name already exists, create a constraint with a different name.

71,qpERR_ABORT_QDN_MAX_KEY_COLUMN_COUNT = Too many key columns
# *Cause: Too many key columns.
# *Action: Reduce the number of columns in the index.

73,qpERR_ABORT_QDN_REFERENCED_CONSTRAINT_NOT_FOUND = Unable to find referenced constraint
# *Cause: There is no primary key or unique constraint in the referenced column list
# *Action: Verify that a referenced constraint exists in the reference column list.

74,qpERR_ABORT_QDN_ADD_COL_NO_DEFAULT_NOTNULL = Unable to add NOT NULL constraint to a column without a DEFAULT value
# *Cause: It is impossible to add a NOT NULL constraint to a column for which a DEFAULT value has not been specified.
# *Action: Remove the NOT NULL constraint from the column specification, or specify a DEFAULT value for the column.

75,qpERR_ABORT_QDN_DUPLICATE_CONSTRAINT_SPEC = Unable to create or add a constraint for the column because it already has the same constraint.
# *Cause: There is at least one similar constraint for the column.
# *Action: Check the constraint definition.

400,qpERR_ABORT_QDN_NOT_COMPATIBLE_TYPE = Incompatible data types
# *Cause: The data types are incompatible.
# *Action: Check the data types.

401,qpERR_FATAL_QDN_NOT_FOUND_FOREIGNKEY = Foreign key not found
# *Cause: Foreign key not found
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

453,qpERR_ABORT_QDN_CANNOT_DROP_TIMESTAMP = Unable to drop a TIMESTAMP constraint.
# *Cause: It is impossible to drop a TIMESTAMP constraint.
# *Action: Execute the ALTER TABLE DROP COLUMN statement.

560,qpERR_ABORT_QDN_SELF_REFERENCE_NOT_SUPPORTED = Self referential constraints are not supported yet.
# *Cause: Self referential constraints are not supported.
# *Action: Check the SQL statement.

568,qpERR_ABORT_QDN_MISMATCHED_REFERENCING_COLUMN_COUNT = Number of referencing columns must match number of referenced columns.
# *Cause: The number of referencing columns that you listed did not match the primary key or unique key.
# *Action: Check the referencing columns that you listed.

642,qpERR_ABORT_QDN_NOT_EXISTS_LOCAL_UNIQUE_KEY = LOCAL UNIQUE KEY constraint not found in the table.
# *Cause: The LOCAL UNIQUE KEY constraint was not found in the table.
# *Action: Check the LOCAL UNIQUE KEY constraint in the meta tables.

657,qpERR_ABORT_QDN_CANNOT_CREATE_LOCAL_UNIQUE_KEY_CONSTR_ON_NON_PART_TABLE = Cannot create a local unique key constraint for a non-partitioned table.
# *Cause: A local unique key constraint cannot be created on a non partitioned table.
# *Action: Check the partitioning method of the table.

712,qpERR_ABORT_QDN_ADD_COL_FOREIGN_KEY_WITH_REPLICATED_TBL = It is forbidden to add a foreign key column to a replicated table.
# *Cause: A foreign key column cannot be added to a replicated table.
# *Action: Drop the replication.

803,qpERR_ABORT_QDN_NOT_SUPPORTED_CONSTR_STATE = This kind of constraint does not permit this constraint state. <0%s>
# *Cause: A constraint does not allow this constraint state.
# *Action: Specify a constraint state that is permissible for this constraint.

865,qpERR_ABORT_QDN_CANNOT_CREATE_FOREIGN_KEY_ON_TEMPORARY_TABLE = Cannot create a referential constraint for a temporary table.
# *Cause: A foreign key was specified in a temporary table creation statement.
# *Action: Do not attempt to specify a foreign key when creating a temporary table.

912,qpERR_ABORT_QDN_NOT_SUPPORT_LOB_COLUMN_IN_CHECK_CONSTRAINT = Unsupported use of LOB column in a CHECK constraint <0%s>
# *Cause: One or more LOB columns were referenced in a CHECK constraint.
# *Action: Remove the LOB column(s) from the CHECK constraint.

913,qpERR_ABORT_QDN_INVALID_CHECK_CONSTRAINT_EXPRESSION = Invalid check constraint <0%s> expression
# *Cause: The check constraint expression is incorrect.
# *Action: Check the check constraint expression.

914,qpERR_ABORT_QDN_VIOLATE_CHECK_CONSTRAINT = Check constraint <0%s> violated
# *Cause: Some rows do not conform to the check constraint's requirements.
# *Action: Check all rows that are related to the check constraint.

1019,qpERR_ABORT_QDN_NOT_SUPPORT_CONSTRAINT_IN_COMPRESSED_COLUMN = Unable to create or add the constraint for the compressed column. <0%s>
# *Cause: The user tried to create a primary key, a unique key or a timestamp constraint on the compressed column.
# *Action: Do not create a primary key, a unique key or a timestamp constraint on the compressed column.

1045,qpERR_ABORT_QDN_NOT_ALLOW_MEM_TBS_PK_UK_OF_GLOBAL_INDEX = A primary key or unique key constraint can be defined for a non-partitioned index on a disk partitioned table.
# *Cause: A primary key or unique key constraint can be defined for a non-partitioned index on a disk partitioned table.
# *Action: Do define a primary key or unique key constraint for a non-partitioned index on a disk partitioned table.

###############################################################################
#########                 QDX ERROR CODE                              #########
###############################################################################
76,qpERR_ABORT_QDX_DUPLICATE_INDEX = Duplicate index name
# *Cause: The specified index name already exists in the database.
# *Action: Create a new index with a different name.

77,qpERR_ABORT_QDX_CRT_IDX_ON_REPLICATED_TBL = It is forbidden to create an index in a replicated table.
# *Cause: An index cannot be created on a replicated table.
# *Action: Drop the replication.

79,qpERR_ABORT_QDX_INVALID_KEY_FIELD_COUNT = Too many columns for a composite index.
# *Cause: It is impossible to create an index having more columns than the maximum number of key columns in a composite index.
# *Action: Check the number of key columns in the composite index.

81,qpERR_ABORT_QDX_DUPLICATE_INDEX_COLS = Duplicate key columns in an index
# *Cause: Duplicate columns exist in the description of an index.
# *Action: Create a new index with a different description.

572,qpERR_ABORT_QDX_INVALID_INDEX_COLS = Invalid key columns in an index
# *Cause: Invalid key columns in an index.It is not permitted to create an unique or composite index if the type of a column is geometry.
# *Action: Check the key columns.

567,qpERR_ABORT_QDX_CANNOT_CREATE_INDEX_DATATYPE = cannot create index in the data type.
# *Cause: The user tried to create an index in the specified data type.
# *Action: Do not create an index in the specified data type.

643,qpERR_ABORT_QDX_NOT_ALLOWED_PRIMARY_AND_UNIQUE_KEY_OF_NONE_PREFIXED_INDEX = Unable to create a primary key or a unique key constraint in the local non-prefixed index.
# Cause: The user tried to create a primary key or a unique key constraint in the local prefixed index.
# *Action: Do not create a primary key or a unique key constraint in the local non-prefixed index.

644,qpERR_ABORT_QDX_CANNOT_SPECIFY_TBS_OF_LOCAL_PARTITIONED_INDEX = cannot specify tablespace of partitioned index. <0%s>
# *Cause: The tablespace cannot be specified for a partitioned index.
# *Action: Check the position of the tablespace.

645,qpERR_ABORT_QDX_INVALID_INDEX_PARTITION_COUNT = Too many index partitions.
# *Cause: Too many index partitionsYou can create only the number of partitions less than or equal to that of the table partitions.
# *Action: Check the number of index partitions.

646,qpERR_ABORT_QDX_CANNOT_CREATE_PART_INDEX_ON_NONE_PART_TABLE = cannot create a partitioned index in a non-partitioned table.
# *Cause: Partitioned indexes cannot be created on non partitioned tables.
# *Action: Check the partitioning method of the table.

647,qpERR_ABORT_QDX_CANNOT_ALTER_INDEX_REBUILD_ON_NONE_PART_INDEX = cannot alter the index rebuild for a non-partitioned index.
# *Cause: The index rebuild cannot be altered for a non partitioned table.
# *Action: Check the partitioning method of the table.

648,qpERR_ABORT_QDX_NOT_EXIST_INDEX_PARTITION = The index partition does not exist.
# *Cause: The index partition does not exist.
# *Action: Check the name of the index partition.

649,qpERR_ABORT_QDX_CANNOT_CREATE_NONE_PART_INDEX_ON_PART_TABLE = cannot create a non-partitioned index in a partitioned table.
# *Cause: A non partitioned index cannot be created on a partitioned table.
# *Action: Check the partitioning method of the table.

666,qpERR_ABORT_QDX_NON_DISK_INDEX_LOGGING_OPTION = LOGGING and NOLOGGING options are only allowed for a disk B-Tree index.
# *Cause: The LOGGING or NOLOGGING option was used for a non disk B-Tree index.
# *Action: Create an index without using the LOGGING or NOLOGGING options.

713,qpERR_ABORT_QDX_NON_MEMORY_INDEX_PERSISTENT_OPTION = The PERSISTENT option is only allowed for a memory B-Tree index.
# *Cause: The PERSISTENT option was used for a non memory B-Tree index.
# *Action: Create an index without the PERSISTENT option.

868,qpERR_ABORT_QDX_TEMPORARY_INDEX_NOT_ALLOW_TBS_NAME = Cannot specify tablespace when creating a temporary table index
# *Cause: An attempt was made to specify a tablespace when creating a temporary table index.
# *Action: Do not attempt to specify a tablespace when creating a temporary table index.

920,qpERR_ABORT_QDX_CANNOT_SPECIFY_INDEX_PARTITION = Cannot specify local index partition for non-partitioned index
# *Cause: Cannot specify a local index partition for non-partitioned index.
# *Action: Remove the local index partition reference.

923,qpERR_ABORT_QDX_NOT_SUPPORT_LOB_COLUMN = LOB column is not supported for a function-based index. <0%s>
# *Cause: LOB columns are not supported for a function-based index.
# *Action: Remove any LOB column expression.

924,qpERR_ABORT_QDX_NOT_SUPPORT_ENCRYPTED_COLUMN = Encrypted column is not supported for a function-based index. <0%s>
# *Cause: Encrypted columns are not supported for a function-based index.
# *Action: Remove any encrypted column expression.

976,qpERR_ABORT_QDX_CANNOT_CREATE_NONE_PART_INDEX_ON_MEM_PART_TABLE = A non-partitioned index can be created on a disk partitioned table.
# *Cause: A non-partitioned index can be created on a disk partitioned table.
# *Action: Check whether the table is a disk partitioned table.

1041,qpERR_ABORT_QDX_CREATE_HIDDEN_COLUMN_WITH_REPLICATED_TBL = Unable to create a function-based index on a replicated table.
# *Cause: Cannot create a function-based index on a replicated table.
# *Action: Drop the replication object.

1042,qpERR_ABORT_QDX_DROP_HIDDEN_COLUMN_WITH_REPLICATED_TBL = Unable to drop a function-based index from a replicated table.
# *Cause: Cannot drop a function-based index from a replicated table.
# *Action: Drop the replication object.

1049,qpERR_ABORT_QDX_CANNOT_CREATE_PARTITIONED_INDEX = Cannot create a partitioned index.
# *Cause: The partitioned index does not have a partition key column.
# *Action: Set a partition key on the index key column, or create a localunique index

1108,qpERR_ABORT_QDX_NON_MEMORY_BTREE_INDEX_REORGANIZATION_OPTION = The reorganization option is only allowed for a memory B-Tree index.
# *Cause: The reorganization option was used for a non memory B-Tree index.
# *Action: Remove the reorganization option from a non memory B-Tree index. 

###############################################################################
#########                 QMV ERROR CODE                              #########
###############################################################################
83,qpERR_ABORT_QMV_NOT_ENOUGH_INSERT_VALUES = Not enough insert values
# *Cause: The number of columns is not equal to the number of values.
# *Action: Verify that the same number of columns and values are specified.

84,qpERR_ABORT_QMV_TOO_MANY_INSERT_VALUES = Too many insert values
# *Cause: The number of columns is not equal to the number of values.
# *Action: Verify that the same number of columns and values are specified.

85,qpERR_ABORT_QMV_NOT_ALLOW_PRIMARY_KEY_UPDATE = The primary key of a table cannot be updated when the table is used for replication.
# *Cause: The user tried to update the primary key of a table that was used for replication.
# *Action: Verify that the table is not used for replication.

86,qpERR_ABORT_QMV_NOT_NULL_CONSTRAINT = Unable to insert (or update) NULL into a NOT NULL column.
# *Cause: It is impossible to insert or update a NULL value into a NOT NULL column.
# *Action: Drop the NOT NULL constraint or verify that the values to be inserted or updated are not NULL values.

87,qpERR_ABORT_QMV_DML_ON_CLOSED_TABLE = It is impossible to access the object. <0%s>
# *Cause: It is impossible to access the object.
# *Action: Verify that the server status is in the service phase.

88,qpERR_ABORT_QMV_NOT_EXISTS_COLUMN = Column not found <0%s>
# *Cause: The specified column was not found in the table.
# *Action: Verify that the column name is correct.

89,qpERR_ABORT_QMV_COLUMN_AMBIGUOUS_DEF = Ambiguous column name: <0%s>
# *Cause: Two or more tables to be selected have columns with the same name.
# *Action: Specify column names explicitly, such as 'table_name.column_name'.

90,qpERR_ABORT_QMV_NOT_EXISTS_ALIAS = The specified alias name was not found in the target list. <0%s>
# *Cause: The specified alias name was not found in the target list.
# *Action: Check the alias names in the target list.

91,qpERR_ABORT_QMV_DUPLICATE_ALIAS = Duplicate alias names were found in the target list. <0%s>
# *Cause: Duplicate alias names were found in the target list.
# *Action: Check the alias names in the target list.

92,qpERR_ABORT_QMV_NO_GROUP_EXPRESSION = Not a group expression in the target list
# *Cause: If the SELECT statement includes a GROUP BY clause, only group expressions can be used in the target list.
# *Action: Verify only the expressions in the GROUP BY clause are used in the SELECT, WHERE, and ORDER BY clauses.

93,qpERR_ABORT_QMV_NO_SELECTED_EXPRESSION = Only the expressions in the target list can appear in the ORDER BY clause.
# *Cause: One or more expressions in the ORDER BY clause are not in the target list.
# *Action: Verify that the target list has every expression used in the ORDER BY clause.

94,qpERR_ABORT_QMV_INVALID_POSITION_IN_ORDERBY = Invalid position value in ORDER BY clause.
# *Cause: The position value is greater than the number of expressions in the target list.
# *Action: Verify that the position value is less than or equal to the number of expressions in the target list.

95,qpERR_ABORT_QMV_INVALID_ORDERBY_EXP_WITH_SET = Invalid expression in ORDER BY clause. <0%s>
# *Cause: An invalid expression was found in the ORDER BY clause.
# *Action: Check the ORDER BY clause.

96,qpERR_ABORT_QMV_CANNOT_USE_AGG = Invalid use of aggregate function. An aggregate function such as COUNT, MIN, MAX, SUM or AVG was used in a clause in which it is not allowed.
# *Cause: An aggregate function takes another aggregate function as an argument. An aggregate function such as COUNT, MIN, MAX, SUM or AVG is used on a clause where it is forbidden.
# *Action: Verify the positions of aggregate functions in the SQL statement. Verify that no aggregate function is used in a SQL statement that has no GROUP BY clause.

97,qpERR_ABORT_QCV_NOT_ALLOWED_AGGREGATION = An aggregate function is not allowed here. <0%s>
# *Cause: An aggregate function cannot be used here.
# *Action: Check all aggregate functions.

98,qpERR_ABORT_QMV_MISMATCH_TARGET_COUNT = Mismatched number of expressions in the target lists of SELECT statements for a SET query
# *Cause: The number of expressions in target lists of SELECT statements for a SET differ from one another.
# *Action: Count the number of expressions in each target list of SELECT statements in the SET query.

99,qpERR_ABORT_QMV_INVALID_TABLE_NAME = Invalid table name <0%s>
# *Cause: A table name that is not listed in the FROM clause was used.
# *Action: Check the FROM clause.

100,qpERR_ABORT_QMV_GROUP_ON_FORUPDATE = A GROUP BY or HAVING clause is not allowed in a SELECT FOR UPDATE statement.
# *Cause: Violation of SELECT FOR UPDATE restrictions.
# *Action: Do not use a GROUP BY or HAVING clause.

101,qpERR_ABORT_QMV_AGGREGATE_ON_FORUPDATE = Aggregate functions are not allowed in SELECT FOR UPDATE statements.
# *Cause: Violation of SELECT FOR UPDATE restrictions.
# *Action: Do not use aggregate functions.

102,qpERR_ABORT_QMV_DISTINCT_SETFUNCTION_ON_FORUPDATE = DISTINCT, UNION, UNION ALL, INTERSECT and MINUS are not allowed in SELECT FOR UPDATE statements.
# *Cause: Violation of SELECT FOR UPDATE restrictions.
# *Action: Do not use DISTINCT or set functions.

565,qpERR_ABORT_QMV_JOIN_ON_FORUPDATE = JOIN is not allowed in SELECT FOR UPDATE statements.
# *Cause: Violation of SELECT FOR UPDATE restrictions.
# *Action: Do not use the JOIN operation.

103,qpERR_ABORT_QMV_USE_SEQUENCE_WITH_ORDER_BY = A sequence cannot be used in a SELECT statement with an ORDER BY clause. <0%s>
# *Cause: A sequence cannot be used in a SELECT statement having an ORDER BY clause.
# *Action: Remove the sequence or the ORDER BY clause.

104,qpERR_ABORT_QMV_USE_SEQUENCE_WITH_GROUP_BY = A sequence cannot be used in a SELECT statement that has a GROUP BY or HAVING clause or aggregate functions. <0%s>
# *Cause: A sequence cannot be used in a SELECT statement having a GROUP BY or HAVING clause or aggregate functions.
# *Action: Remove the aggregate functions, the GROUP BY or HAVING clause, or the sequence.

105,qpERR_ABORT_QMV_USE_SEQUENCE_WITH_DISTINCT = A sequence cannot be used in a SELECT statement that has a DISTINCT clause. <0%s>
# *Cause: A sequence cannot be used in a SELECT statement having a DISTINCT clause.
# *Action: Remove the sequences or the DISTINCT option.

106,qpERR_ABORT_QMV_USE_SEQUENCE_WITH_SET = A sequence cannot be used in a SELECT statement that has a UNION, UNION ALL, INTERSECT, or MINUS operation. <0%s>
# *Cause: A sequence cannot be used in SELECT statements that contain UNION, UNION ALL, INTERSECT, or MINUS operations.
# *Action: Remove the sequences or SET operation.

107,qpERR_ABORT_QMV_USE_SEQUENCE_IN_VIEW = A sequence cannot be used in a view. <0%s>
# *Cause: A sequence cannot be used in a view.
# *Action: Remove the sequences.

108,qpERR_ABORT_QMV_USE_SEQUENCE_IN_SUBQUERY = A sequence cannot be used in a subquery. <0%s>
# *Cause: A sequence cannot be used in a subquery.
# *Action: Remove the sequences.

109,qpERR_ABORT_QMV_USE_SEQUENCE_IN_WHERE = A sequence cannot be used in a WHERE clause. <0%s>
# *Cause: A sequence cannot be used in a WHERE clause.
# *Action: Remove the sequences.

110,qpERR_ABORT_QMV_USE_SEQUENCE_IN_DEFAULT = A sequence cannot be used as a DEFAULT constraint in a CREATE or ALTER TABLE statement. <0%s>
# *Cause: A sequence cannot be used as a DEFAULT constraint in a CREATE or ALTER TABLE statement.
# *Action: Remove the sequences.

111,qpERR_ABORT_QMV_NESTED_AGGR_WITHOUT_GROUP = Unable to use a nested aggregate function without a GROUP BY clause.
# *Cause: A nested aggregate function cannot be used without a GROUP by clause.
# *Action: Use a GROUP BY clause with the nested aggregate function.

112,qpERR_ABORT_QMV_TOO_DEEPLY_NESTED_AGGR = Aggregate function nested too deeply
# *Cause: Aggregate function nested too deeply.
# *Action: Nest an aggregate function to only one layer.

1036,qpERR_ABORT_QMV_NOT_SUPPORT_NESTED_AGGR = Unsupported use of nested aggregate function with GROUPING SETS clause.
# *Cause: A nested aggregate function was used with the GROUPING SETS clause.
# *Action: Use the GROUPING SETS clause without the nested aggregate function.

113,qpERR_ABORT_QMV_TYPE_MISMATCH_IN_SET = Type mismatch error in SET.
# *Cause: One or more of the target types are incompatible with one another.
# *Action: Verify that the type of every target in SET is correct.

114,qpERR_ABORT_QMV_DUP_COLUMN_IN_SET = Duplicate columns specified in a SET clause.
# *Cause: One column is specified more than once in a SET clause.
# *Action: Verify that the target column name appears only once in a SET clause.

403,qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_TARGET = Inapplicable data type in target list <0%s>
# *Cause: Inapplicable data type in target list.
# *Action: Remove targets that are of the BOOLEAN or LIST data types.

416,qpERR_ABORT_QMV_HIERARCHICAL_WITH_JOIN = A hierarchical query cannot have a join operation <0%s>.
# *Cause: A hierarchical query cannot have a join operation.
# *Action: Use the hierarchical query only on a base table.

417,qpERR_ABORT_QMV_HIERARCHICAL_WITH_VIEW = Hierarchical queries are not allowed for created views or inline views <0%s>.
# *Cause: A hierarchical query can be used only on a physical table.
# *Action: Use a hierarchical query only on a physical table. Remove all inline views from hierarchical queries.

418,qpERR_ABORT_QMV_SUBQ_IN_CONNECTBY = A CONNECT BY clause cannot include a subquery <0%s>.
# *Cause: A CONNECT BY clause cannot include a subquery.
# *Action: Remove the subquery from the CONNECT BY clause.

419,qpERR_ABORT_QMV_PRIOR_WITHOUT_CONNECTBY = PRIOR cannot be specified without a CONNECT BY clause <0%s>.
# *Cause: PRIOR cannot be specified without a CONNECT BY clause.
# *Action: Specify a CONNECT BY clause.

469,qpERR_ABORT_QMV_NOT_PREDICATE = Not a predicate <0%s>.
# *Cause: A predicate was not specified.
# *Action: Specify a predicate.

513,qpERR_ABORT_PSM_NOT_IN_SERVICE_PHASE = A stored function or procedure can be invoked only in the SERVICE phase <0%s>.
# *Cause: A stored function or procedure can be invoked only in the SERVICE phase.
# *Action: Do not use stored functions or procedures in phases other than the SERVICE phase.

517,qpERR_ABORT_QMV_MOVE_SAME_TABLE = It is forbidden to execute a MOVE statement on the same table.
# *Cause: The user tried to execute a MOVE statement on the same table.
# *Action: Do not execute a MOVE statement on the same table.

569,qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_ORDER_BY = Inapplicable data type in ORDER BY clause <0%s>
# *Cause: An inapplicable data type was specified in an ORDER BY clause.
# *Action: Do not use BOOLEAN or LIST type data for ORDER BY clauses.

570,qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_GROUP_BY = Inapplicable data type in GROUP BY clause <0%s>
# *Cause: An inapplicable data type was specified in a GROUP BY clause.
# *Action: Do not use BOOLEAN or LIST type data for GROUP BY clauses.

571,qpERR_ABORT_QMV_NOT_ALLOWED_HOSTVAR = Invalid use of host variables <0%s>
# *Cause: A host variable was misused.
# *Action: Set the specified data type using a CAST operator.

582,qpERR_ABORT_QMV_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE = Incomparable data types (GEOMETRY,LOB) cannot be used in ORDER BY, GROUP BY, HAVING and CONNECT BY clauses.
# *Cause: Incomparable data types (GEOMETRY,LOB) cannot be used in ORDER BY, GROUP BY, HAVING and CONNECT BY clauses.
# *Action: Do not use incomparable data types (GEOMETRY,LOB) in GROUP BY and CONNECT BY clauses.

573,qpERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_SET = An incomparable data type (GEOMETRY,LOB) cannot be used in a SELECT statement that contains a UNION, INTERSECT, or MINUS operation. <0%s>
# *Cause: An incomparable data type (GEOMETRY,LOB) cannot be used in a SELECT statement that contains a UNION, INTERSECT, or MINUS operation.
# *Action: Do not use incomparable data types (GEOMETRY,LOB) in SET operations.

574,qpERR_ABORT_QMV_USE_INCOMPARABLE_DATA_TYPE_WITH_DISTINCT = An incomparable data type (GEOMETRY,LOB) cannot be used in a SELECT statement that has a DISTINCT clause.
# *Cause: An incomparable data type (GEOMETRY,LOB) cannot be used in a SELECT statement having a DISTINCT clause.
# *Action: Do not use incomparable data types (GEOMETRY,LOB) with the DISTINCT option.

590,qpERR_ABORT_QMV_USE_SEQUENCE_IN_ON_JOIN_CONDITION = A sequence cannot be used in an ON JOIN CONDITION clause. <0%s>
# *Cause: A sequence cannot be used in an ON JOIN CONDITION clause.
# *Action: Remove the sequences.

650,qpERR_ABORT_QMV_INVALID_PARTITION_KEY_INSERT = Inserted partition key is not mapped to any partition.
# *Cause: An attempt was made to insert a record into a Range or Composite Range object having a concatenated partition key that is beyond the concatenated partition bound list of the last partition -OR- An attempt was made to insert a record into a List object having a partition key that did not match the literal values specified for any of the partitions.
# *Action: Do not insert the key. Alternatively, add a partition capable of accepting the key, or add values matching the key to a partition specification.

662,qpERR_ABORT_QMV_USE_ROWNUM_IN_DML = ROWNUM pseudo column not supported in DML statements. <0%s>
# *Cause: The ROWNUM pseudo column is not supported in DML statements.
# *Action: Remove the ROWNUM.

664,qpERR_ABORT_QMV_USE_ROWNUM_IN_START_WITH = ROWNUM pseudo column not supported in START WITH clauses. <0%s>
# *Cause: The ROWNUM pseudo column is not supported in START WITH clauses.
# *Action: Remove the ROWNUM.

665,qpERR_ABORT_QMV_USE_ROWNUM_IN_CONNECT_BY = ROWNUM pseudo column not supported in CONNECT BY clauses. <0%s>
# *Cause: The ROWNUM pseudo column is not supported in CONNECT BY clauses.
# *Action: Remove the ROWNUM.

684,qpERR_ABORT_QMV_NOT_ALLOWED_DATABASE_OBJECTS = Table, View or Sequence reference not allowed in this context. <0%s>
# *Cause: A reference to a database table, view, or sequence was found in an inappropriate context. Such references can appear only in SQL statements.
# *Action: Remove or relocate the illegal reference.

698,qpERR_ABORT_QMV_COLUMN_NOT_ALLOWED_HERE = Column not allowed here: <0%s>
# *Cause: A column name was used in an expression in which it is not permitted, such as in the VALUES clause of an INSERT or ENQUEUE statement.
# *Action: Check the syntax of the statement and use column names only where appropriate.

828,qpERR_ABORT_REACHED_MAX_SET_OP_RECURSION_DEPTH = Too many consecutive set operations.
# *Cause: The query could not be validated because there were too many consecutive set operations (union, intersect, minus, ... ).
# *Action: Try to group set operations using parentheses.

833,qpERR_ABORT_MISMATCHED_VALUE_TYPE = The column type and the value type are mismatched.
# *Cause: The column type and the value type are mismatched.
# *Action: Check the column specification.

834,qpERR_ABORT_QMV_NOT_PIVOT_AGGREGATION = Non-aggregate functions used in pivot operation
# *Cause: Something other than an aggregate function was specified in the PIVOT clause.
# *Action: Ensure that only aggregate functions are specified in the PIVOT clause.

835,qpERR_ABORT_QMV_AMBIGOUS_PIVOT_COLUMN = Duplicate column names
# *Cause: The pivot operation resulted in multiple columns having the same name.
# *Action: Specify aliases in the PIVOT clause to prevent the generation of duplicate column names.

836,qpERR_ABORT_QMV_INVALID_PIVOT_FOR_IN_COUNT = Mismatched number of PIVOT FOR elements and PIVOT IN subelements
# *Cause: The number of elements in the PIVOT FOR clause is not the same as the number of subelements in each element in the PIVOT IN clause.
# *Action: Ensure that the number of PIVOT FOR elements is the same as the number of PIVOT IN subelements.

1069,qpERR_ABORT_QMV_INVALID_UNPIVOT_ARGUMENT = Invalid number of arguments for the UNPIVOT clause
# *Cause: The number of arguments for the UNPIVOT clause is invalid.
# *Action: Verify that the correct number of arguments have been input for the UNPIVOT clause.

1070,qpERR_ABORT_QMV_INVALID_UNPIVOT_COLUMN = Unable to find the specified column name in the unpivoted table. <0%s>
# *Cause: The specified column name was not found in the unpivoted table.
# *Action: Verify that the column name is correct.

837,qpERR_ABORT_QMV_CANNOT_USE_DEFAULT_IN_VIEW = The view creation attempt failed because a default column value was specified.
# *Cause: An attempt was made to specify a default column value when creating or replacing a view.
# *Action: Do not use the DEFAULT clause in view creation statements.

851,qpERR_ABORT_QMV_NOT_APPLICABLE_OPERATOR_IN_TARGET = Invalid operator in SELECT list. <0%s>
# *Cause: The SELECT list contains an operator that cannot be used in the SELECT list.
# *Action: Remove operators such as the outer join operator from the SELECT list.

852,qpERR_ABORT_QMV_OUTER_JOINED_TO_SUBQUERY = A column cannot be outer-joined to a subquery. <0%s>
# *Cause: A column was outer-joined to a subquery.
# *Action: Rewrite the query to avoid outer-joining a column to a subquery.

853,qpERR_ABORT_QMV_NOT_ALLOWED_IN_OPERAND_OR = Outer join operator not allowed in OR or IN condition. <0%s>
# *Cause: The outer join operator was used in an operand of an OR or IN condition.
# *Action: Avoid using the outer join operator in OR and IN conditions.

854,qpERR_ABORT_QMV_NOT_ALLOWED_WITH_ANSI_JOIN = The outer join operator cannot be used with ANSI joins. <0%s>
# *Cause: The outer join operator was used with an ANSI join.
# *Action: Avoid using the outer join operator with ANSI joins.

855,qpERR_ABORT_QMV_TOO_MANY_REFERENCE_OUTER_JOINED_TABLE = A predicate may reference only one outer-joined table. <0%s>
# *Cause: The outer join operator was used with more than one table in a predicate.
# *Action: Use the outer join operator with only one table in a predicate.

856,qpERR_ABORT_QMV_NOT_ALLOW_JOIN_OPER_WITH_NON_COLUMN = The outer join operator can only be used with columns. <0%s>
# *Cause: An attempt was made to use the outer join operator with something other than a column.
# *Action: Verify that the outer join operator is used only with columns.

857,qpERR_ABORT_QMV_NOT_ALLOW_OUTER_JOIN_EACH_OTHER = A table cannot be outer-joined to itself. <0%s>
# *Cause: The outer join operator was used in a WHERE clause in which a table is compared with itself.
# *Action: Avoid using the outer join operator in a predicate in which a table is compared with itself.

858,qpERR_ABORT_QMV_INVALID_OPERATION_WITH_LISTS = The outer join operator can't be used in a list. <0%s>
# *Cause: The outer join operator was used in a list that was compared with a list in an IN(ANY,ALL) condition.
# *Action: When comparing a list that includes an outer join with an IN(ANY,ALL) condition, the IN(ANY,ALL) condition can contain only one member.

859,qpERR_ABORT_QMV_NOT_ALLOW_OUTER_JOIN_INAPPROPRIATE_TABLE = Invalid table used with outer join operator in subquery. <0%s>
# *Cause: The outer join operator was used in a subquery predicate with a main query select list table.
# *Action: Avoid using the outer join operator in a subquery predicate with a main query select list table.

860,qpERR_ABORT_QMV_MAYBE_OUTER_JOINED_TO_AT_MOST_ONE_OTHER_TABLE = A table can be outer joined to at most one other table. <0%s>
# *Cause: A table is outer joined to more than one other table.
# *Action: Avoid outer joining a table to more than one other table.

861,qpERR_ABORT_QMV_OUTER_JOIN_OPERATOR_NOT_ALLOW_NOCNF = The outer join operator can only be used with CNF (Conjunctive Normal Form).
# *Cause: The outer join operator was not used with CNF (Conjunctive Normal Form).
# *Action: Change the NORMALFORM_MAXIMUM property to a larger value using the ALTER SESSION command.

843,qpERR_ABORT_QMV_NOT_SUPPORT_SUBQUERY = Unsupported use of subquery in RETURNING INTO clause
#*Cause: A subquery was used in a RETURNING INTO clause.
# *Action: Remove the subquery from the RETURNING INTO clause.

844,qpERR_ABORT_QMV_NOT_SUPPORT_LOB_COLUMN = Unsupported use of LOB column in RETURNING INTO clause
# *Cause: One or more LOB columns were referenced in a RETURNING INTO clause.
# *Action: Remove the LOB column(s) from the RETURNING INTO clause.

845,qpERR_ABORT_QMV_NOT_RETURN_ALLOWED_HOSTVAR = Invalid use of host variable
# *Cause: One or more host variables were referenced after the RETURNING keyword.
# *Action: Use host variables only after the INTO keyword.

846,qpERR_ABORT_QMV_NOT_SAME_COLUMN_RETURN_INTO = Mismatched number of RETURNING expressions and host variables
# *Cause: The number of expressions after the RETURNING keyword did not match the number of host variables.
# *Action: Ensure that the number of expressions after the RETURNING keyword matches the number of host variables.

847,qpERR_ABORT_QMV_SEQUENCE_NOT_ALLOWED = Invalid use of sequence
# *Cause: One or more sequences were referenced in a RETURNING INTO clause.
# *Action: Remove the sequence(s) from the RETURNING INTO clause.

885,qpERR_ABORT_QMV_NO_GRANT_DML_PRIV_OF_MVIEW_TABLE = Cannot perform INSERT, DELETE, or UPDATE on materialized views
# *Cause: An attempt was made to execute an INSERT, DELETE, or UPDATE statement on a materialized view.
# *Action: Check the name and type of the table on which to execute the statement.

888,qpERR_ABORT_QMV_CONNECT_BY_NOT_ALLOW_NOCNF = The CONNECT BY clause can only be used with CNF (Conjunctive Normal Form).
# *Cause: The connect by clause was not used with CNF (Conjunctive Normal Form).
# *Action: Change the NORMALFORM_MAXIMUM property to a larger value using the ALTER SESSION command.

889,qpERR_ABORT_QMV_USE_ISLEAF_IN_START_WITH = CONNECT_BY_ISLEAF pseudocolumn not supported in START WITH clauses <0%s>
# *Cause: An attempt was made to use the CONNECT_BY_ISLEAF pseudocolumn in a START WITH clause.
# *Action: Remove the CONNECT_BY_ISLEAF pseudocolumn from the START WITH clause.

890,qpERR_ABORT_QMV_USE_ISLEAF_IN_CONNECT_BY = CONNECT_BY_ISLEAF pseudocolumn not supported in CONNECT BY clauses <0%s>
# *Cause: An attempt was made to use the CONNECT_BY_ISLEAF pseudocolumn within a CONNECT BY clause.
# *Action: Remove the CONNECT_BY_ISLEAF pseudocolumn from the CONNECT BY clause.

891,qpERR_ABORT_QMV_ISLEAF_NEED_CONNECT_BY = CONNECT_BY_ISLEAF pseudocolumn used without CONNECT BY clauses
# *Cause: An attempt was made to use the CONNECT_BY_ISLEAF pseudocolumn without a CONNECT BY clause.
# *Action: Use a CONNECT BY clause or remove the CONNECT_BY_ISLEAF pseudocolumn.

892,qpERR_ABORT_QMV_CONNECT_BY_ROOT_NEED_CONNECT_BY = CONNECT_BY_ROOT used without CONNECT BY clause
# *Cause: An attempt was made to use the CONNECT_BY_ROOT operator without a CONNECT BY clause.
# *Action: Use a CONNECT BY clause or remove the CONNECT_BY_ROOT operator.

893,qpERR_ABORT_QMV_NOT_ALLOW_CONNECT_BY_ROOT = CONNECT_BY_ROOT not supported in START WITH or CONNECT BY clause
# *Cause: The CONNECT_BY_ROOT operator was used in a START WITH or CONNECT BY clause.
# *Action: Remove the CONNECT_BY_ROOT operator from the START WITH or CONNECT BY clause.

894,qpERR_ABORT_QMV_NOT_ALLOW_SYS_CONNECT_BY_PATH = SYS_CONNECT_BY_PATH not supported in START WITH or CONNECT BY clause
# *Cause: The SYS_CONNECT_BY_PATH function was used in a START WITH or CONNECT BY clause.
# *Action: Remove the SYS_CONNECT_BY_PATH function from the START WITH or CONNECT BY clause.

895,qpERR_ABORT_QMV_SYS_CONNECT_BY_PATH_NEED_CONNECT_BY = SYS_CONNECT_BY_PATH used without CONNECT BY clauses
# *Cause: An attempt was made to use the SYS_CONNECT_BY_PATH function without a CONNECT BY clause.
# *Action: Use a CONNECT BY clause or remove the SYS_CONNECT_BY_PATH function.

896,qpERR_ABORT_UNSUPPORTED_PSEUDO_COLUMN_IN_ORDER_SIBLINGS_BY = Unsupported use of pseudocolumn in ORDER SIBLINGS BY clause <0%s>
# *Cause: An unsupported pseudocolumn was specified in an ORDER SIBLINGS BY clause.
# *Action: Specify only table column names in the ORDER SIBLINGS BY clause.

897,qpERR_ABORT_QMV_NOT_ALLOW_ORDER_SIBLINGS_BY = ORDER SIBLINGS BY clause not allowed here
# *Cause: An attempt was made to use the ORDER SIBLINGS BY clause in an unsupported manner.
# *Action: Remove the ORDER SIBLINGS BY clause from the query.

898,qpERR_ABORT_QMV_NOT_ALLOW_PRIOR_LOB = PRIOR not supported with LOB type columns
# *Cause: A LOB type column was specified after the PRIOR operator.
# *Action: Specify a non-LOB type column after the PRIOR operator.

900,qpERR_ABORT_QMV_CANNOT_SELECT_PROWID = Cannot select _PROWID from a view or multiple tables. <0%s>
# *Cause: Cannot select _PROWID from a view or multiple tables.
# *Action: Use a single table in FROM clause.

901,qpERR_ABORT_QMV_PROWID_NOT_SUPPORTED = _PROWID is not supported.
# *Cause: PROWID is not supported.
# *Action: Do not use _PROWID.

903,qpERR_ABORT_QMV_MERGE_REFERENCED_COLUMNS_CANNOT_BE_UPDATED = Cannot update columns in the ON Clause. <0%s>
# *Cause: An attempt was made to update one or more columns referenced in the ON Clause of a MERGE statement.
# *Action: Remove the columns referenced in the ON Clause from the update column list.

915,qpERR_ABORT_QMV_NEED_GROUP_BY = GROUPING function used without GROUP BY ROLLUP or GROUP BY CUBE or GROUP BY GROUPING SETS clause.
# *Cause: An attempt was made to use the GROUPING function without the GROUP BY ROLLUP or GROUP BY CUBE or GROUP BY GROUPING SETS clause.
# *Action: Add the GROUP BY ROLLUP or GROUP BY CUBE or GROUP BY GROUPING SETS or remove the GROUPING function clause from the statement.

916,qpERR_ABORT_QMV_NOT_SUPPORT_COMPLICATE_GROUP_EXT = GROUP BY ROLLUP or GROUP BY CUBE or GROUP BY GROUPING SETS clause used multiple times.
# *Cause: An attempt was made to use the GROUP BY ROLLUP and/or GROUP BY CUBE and/or GROUP BY GROUPING SETS clause more than once.
# *Action: Use the GROUP BY ROLLUP and/or GROUP BY CUBE and/or GROUP BY GROUPING SETS clause only once.

917,qpERR_ABORT_QMV_NOT_SUPPORT_GROUPING_SETS = GROUPING SETS expression not supported
# *Cause: An attempt was made to use the GROUPING SETS expression
# *Action: Remove the GROUPING SETS expression from the statement.

918,qpERR_ABORT_QMV_MAX_CUBE_COUNT = Too many expressions used in CUBE expression
# *Cause: Too many expressions or columns were specified in the CUBE expression.
# *Action: Use a maximum of 15 expressions in the CUBE expression.

919,qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_RETURN = Inapplicable data type in the RETURNING expression <0%s>
# *Cause: Inapplicable data type in the RETURNING expression.
# *Action: Remove the column from the RETURNING expression.

929,qpERR_ABORT_QMV_WINDOW_INVALID_AGGREGATION = Invalid window aggregation group for window frame
# *Cause: The window aggregation group specified in the window frame is invalid.
# *Action: Use a valid window aggregation group in the window frame.

930,qpERR_ABORT_QMV_NOT_WINDOW_CLAUSE_FUNC = Not supported aggregation function for window frame <0%s>
# *Cause: The aggregation function specified does not support window frames.
# *Action: Choose the right function for window frames.
932,qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE = Cannot modify a column for a non key-preserved table
# *Cause:
#   - Only key-preserved tables allow INSERT, UPDATE and DELETTE operations.
# *Action:
#   - Please alter the base table to key-preserved table.

933,qpERR_ABORT_QMV_NOT_ONE_BASE_TABLE = Cannot modify multiple tables through a join view
# *Cause: UPDATE, DELETE, INSERT operations on a join view is allowed only if there is one base table.
# *Action: Check if the target of the UPDATE, DELETE or INSERT operation is a table.

934,qpERR_ABORT_QMV_TABLE_NOT_FOUND = Table not found
# *Cause: Cannot find the table for UPDATE, DELETE, or INSERT operations.
# *Action: Check the base table before UPDATE, DELETE or INSERT operations.

942,qpERR_ABORT_QMV_NOT_DML_READ_ONLY_VIEW = Cannot perform a DML operation on a read-only view.
# *Cause: If the view was created with the read-only option, then any DML operation cannot be performed on it.
# *Action: Alter the read option of the view with the ALTER VIEW statement.

978,qpERR_ABORT_QDX_NOT_ALLOWED_MEM_TBS_PRIMARY_AND_UNIQUE_KEY_OF_NONE_PREFIXED_INDEX = A primary key or a unique key constraint cannot be defined for a local non-prefixed index on a memory partitioned table.
# *Cause: A primary key or a unique key constraint cannot be defined for a local non-prefixed index on a memory partitioned table.
# *Action: Do not define a primary key or a unique key constraint for a local non-prefixed index on a memory partitioned table.

1034,qpERR_ABORT_QMV_FIXED_TABLE_WITH_LATERAL_VIEW = Cannot use a fixed table in a lateral view.
# *Cause: A fixed table is used in a lateral view.
# *Action: Do not use a fixed table in a lateral view.

1037,qpERR_ABORT_QMV_INVALID_DEFAULT_VALUE = The sequence in the default value does not exist.
# *Cause: The sequence which is used as the default value does not exist.
# *Action: Create the sequence again or alter the default value.

1053,qpERR_ABORT_QMV_NOT_APPLICABLE_TABLE_FUNCTION = An unsupported data type was specified for the table function <0%s>
# *Cause: An argument of the table function clause must be of the associative array type, record type, or list type.
# *Action: Verify that the data type of the table function is of the associative array type, record type, or list type.

1089,qpERR_ABORT_QMV_ORDER_RECURSIVE_VIEW =  recursive WITH Clause needs an initialization branch.
# *Cause: Rewrite the recursive WITH query to have a branch in a UNION ALL operation with no references to itself.
# *Action: A WITH Clause query referred to itself (recursive) but did not have a branch in a UNION ALL with no references to itself.

1090,qpERR_ABORT_QMV_DUPLICATE_RECURSIVE_VIEW =  recursive query name referenced more than once in recursive branch of recursive WITH clause element.
# *Cause: The recursive component of the UNION ALL in a recursive WITH clause element referenced the recursive query name more than once. Only one reference to the recursive query name is allowed in the recursive branch of a recursive WITH clause element.
# *Action: Rewrite the query using only one reference to the recursive query name in the recursive branch of the recursive WITH clause element.

1091,qpERR_ABORT_QMV_NON_EXIST_UNION_ALL_RECURSIVE_VIEW = recursive WITH clause must use a UNION ALL operation.
# *Cause: A WITH clause query referred to itself (recursive) but did not use a UNION ALL operation in its definition query.
# *Action: Rewrite the recursive WITH clause query to use a UNION ALL operation.

1092,qpERR_ABORT_QMV_MULTI_SET_RECURSIVE_VIEW = UNION ALL operation in recursive WITH clause must have only two branches.
# *Cause: A WITH clause query referred to itself (recursive) but used a UNION ALL operation with more than two branches, witch is not currently supported.
# *Action: Rewrite the recursive WITH clause query to use only two branches in the UNION ALL operation.

1093,qpERR_ABORT_QMV_OPERATION_NOT_ALLOWED_RECURSIVE_VIEW = <0%s> is not allowed in recursive WITH statements.
# *Cause: Rewrite the query without the unsupported operation.
# *Action: The recursive component of the UNION ALL in a recursive WITH clause element used an operation that was currently not allowed. The following should not be used in the recursive branch of the UNION ALL operation: GROUP BY, DISTINCT, grouping sets, CONNECT BY, HAVING, aggregate functions.

1094,qpERR_ABORT_QMV_NO_COLUMN_ALIAS_RECURSIVE_VIEW = recursive WITH clause must have column alias list.
# *Cause: A WITH Clause query referred to itself (recursive) but did not have a column alias list specified for it. 
# *Action: Add a column alias list for the WITH clause query name.

1095,qpERR_ABORT_QMV_NOT_ALLOWED_JOIN_RECURSIVE_VIEW = JOIN is not allowed in recursive WITH statements.
# *Cause: Violation of recursive WITH restrictions: 
# *Action: Do not use the JOIN operation.

1096,qpERR_ABORT_QMV_ITSELF_DIRECTLY_RECURSIVE_VIEW = recursive WITH clause must reference itself directly in one of the UNION ALL branches.
# *Cause:  A WITH clause query referred to itself (recursive) indirectly, such as through a subquery or view.
# *Action: Rewrite the recursive WITH clause query to refer to itself directly in the FROM clause of one of the UNION ALL branches.

1097,qpERR_ABORT_QMV_DUPLICATION_QUERY_NAME_RECURSIVE_VIEW = Duplicate recursive WITH names.
# *Cause: The recursive WITH name already exists in the query.
# *Action: Use another recursive WITH name.

1105,qpERR_ABORT_QMV_CANNOT_LOCK_TABLE_UNTIL_NEXT_DDL_IN_NON_EXCLUSIVE_MODE = Unable to execute a LOCK TABLE statement with UNTIL NEXT DDL option in non-exclusive mode.
# *Cause: A LOCK TABLE statement with UNTIL NEXT DDL option cannot be executed in non-exclusive mode.
# *Action: Retry a LOCK TABLE statement in exclusive mode.

1110,qpERR_ABORT_QMV_CANNOT_USE_ORDER_BY_CLAUSE = An ORDER BY clause is not allowed in the function. <0%s>
# *Cause: An ORDER BY clause is not allowed in the function.
# *Action: Remove the ORDER BY clause from the statement.

1133,qpERR_ABORT_QCV_NOT_ALLOWED_ANALYTIC = An analytic function is not allowed here. <0%s>
# *Cause: An analytic function cannot be used here.
# *Action: Check all anaytic functions.

1181,qpERR_ABORT_QMV_TABLE_WRITE_DENIED = Unable to write to the clone table <0%s>.: <1%s>
# *Cause: Write operation can be only executed on a clone table when the transaction is executed with the GLOBAL_CONSISTENT_TRANSACTION in a joined shard node.
# *Action: Check the global transaction level and the status of the shard node.

1182,qpERR_ABORT_QMV_BAK_TABLE_WRITE_DENIED = Unable to write to backup tables.
# *Cause: Backup tables cannot be inserted, deleted, or updated.
# *Action: Check the name and type of the table.

1187,qpERR_ABORT_QMV_CLONE_TABLE_WRITE_DENIED = Unable to write to the clone table in PSM when executed locally. [<0%s>]
# *Cause: A clone table cannot be written in PSM when executed locally.
# *Action: A clone table should be written in PSM when executed globally.

##############################################################################
#########                 QMX ERROR CODE                              #########
###############################################################################
115,qpERR_ABORT_QMX_SEQ_NOT_DEFINE_IN_SESSION = The sequence is not defined in this session. <0%s>
# *Cause: The sequence is not defined in this session.
# *Action: Define the sequence first.

116,qpERR_ABORT_QMX_NOT_NULL_CONSTRAINT = Unable to insert (or update) NULL into NOT NULL column.<0%s><1%s>
# *Cause: The user tried to insert (or update) NULL into a column with a NOT NULL constraint.
# *Action: Drop the NOT NULL constraint or Verify that the value to be inserted or used for update is not a NULL value.

118,qpERR_ABORT_QMX_CHILD_EXIST = Unable to modify records that have child records. Check the referential constraints. <0%s>
# *Cause: It is impossible to modify records that have child records. Check the referential constraints.
# *Action: Check referential constraints that are related to the table.

119,qpERR_ABORT_QMX_NOT_FOUND_PARENT_ROW = The parent record was not found for <0%s>.
# *Cause: The parent record was not found.
# *Action: Check the referential constraints that are related to the table.

454,qpERR_ABORT_QMX_CANNOT_SET_TIMESTAMP = Unable to set the TIMESTAMP value.
# *Cause:
#   - The system failed to set the TIMESTAMP value.
# *Action:
#   - Please send a bug report to the vendor.

510,qpERR_REBUILD_QMX_TOO_OLD_PLANTREE = The plan tree is too old
# *Cause: The plan tree is too old.
# *Action: No action is necessary. The plan tree will be automatically recompiled.

982,qpERR_ABORT_QMX_TABLE_PARTITION_ACCESS_DENIED = Unable to update table or partition <0%s>
# *Cause : Update operation is not allowed on the table or partition.
# *Action : Check the access mode of the table or partition.

1047,qpERR_ABORT_QDR_NOT_SYS_USER_DISABLE_TCP = Only the SYS user can enable or disable TCP.
# *Cause: A normal user cannot enable or disable TCP.
# *Action: Re-login as SYS.

1048, qpERR_ABORT_QDD_CANNOT_DROP_NOT_ENOUGH_RECYCLEBIN_SPACE = Unable to drop the table because the recycle bin has insufficient <0%s> storage capacity.
# *Cause:
#   - The recycle bin has either insufficient memory, or disk, or memory and disk storage capacity.
# *Action:
#   - Empty the recycle bin or increase RECYCLEBIN_MEM_MAX_SIZE, or RECYCLEBIN_DISK_MAX_SIZE, or both RECYCLEBIN_MEM_MAX_SIZE and RECYCLEBIN_DISK_MAX_SIZE.

1106,qpERR_ABORT_QMX_CANNOT_LOCK_TABLE_UNTIL_NEXT_DDL_WITH_DML = Unable to execute a LOCK TABLE statement with UNTIL NEXT DDL option in the transaction that modified data.
# *Cause: A LOCK TABLE  statement with UNTIL NEXT DDL option cannot be executed in the transaction that modified data.
# *Action: Execute the COMMIT or ROLLBACK statement, and retry a LOCK TABLE statement.

1107,qpERR_ABORT_QMX_CANNOT_LOCK_TABLE_UNTIL_NEXT_DDL_MORE_THAN_ONCE = Cannot execute a LOCK TABLE statement with UNTIL NEXT DDL option twice or more.
# *Cause: A LOCK TABLE statement with UNTIL NEXT DDL option cannot be executed twice or more.
# *Action: Execute the COMMIT or ROLLBACK statement, and retry a LOCK TABLE statement.

1183,qpERR_ABORT_QMX_TABLE_PARTITION_UNUSABLE = Unable to access to the table or the partition <0%s>, which of state is unusable.
# *Cause : The table or the partition is unusable state.
# *Action : Check if the state of tables or partitions is usable.

###############################################################################
#########                 QMN ERROR CODE                              #########
###############################################################################
398,qpERR_FATAL_QMN_HASH_INSERT = Hash Insertion Failure
# *Cause:
#   - Internal server error
# *Action:
#   - Please send a bug report to the vendor.

420,qpERR_ABORT_QMN_HIER_LOOP = Loop in hierarchical query detected.
# *Cause: A loop in a hierarchical query has been detected.
# *Action: Check the records.

1020,qpERR_ABORT_QMN_INVALID_TEMPORARY_TABLE = Cannot use the temporary table as it has been changed.
# *Cause: The query cannot be executed as the temporary table has become invalid.
# *Action: Truncate the temporary table and then retry.

1038,qpERR_ABORT_QMN_MODIFY_UNABLE_RECORD = Failed to modify the base table.
# *Cause: The value in the base table to be updated or deleted is already updated or deleted, or does not exist.
# *Action: Check the create join view query and result set.

1098,qpERR_ABORT_QMN_RECURSION_LEVEL_MAXIMUM_RECURSIVE_VIEW = maximum level of recursion reached while executing recursive WITH query.
# *Cause:  A recursive WITH query reached the maximum level of recursion specified and was stopped.
# *Action: Increase the maximum level of recursion.

###############################################################################
#########                 QMO ERROR CODE                              #########
###############################################################################
130,qpERR_FATAL_QMO_INVALID_MEMORY_AREA = Invalid memory area access.
# *Cause: Invalid memory area access
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

562,qpERR_ABORT_QMO_INVALID_MEMORY_AREA = Invalid memory area access.
# *Cause: Invalid memory area access
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

589,qpERR_ABORT_QMO_EMPTY_DUMP_OBJECT = Empty dump object.
# *Cause: You did not specify a dump object for the dump table.
# *Action: Specify a dump object for the dump table.

1033,qpERR_ABORT_QMO_NOT_ALLOWED_LVIEW_LEFT_FULL_JOIN_WITH_REF = Cannot perform a RIGHT or FULL outer join between a lateral view containing a left correlation and a left object.
# *Cause: A RIGHT or FULL outer join cannot be performed between a lateral view containing a left correlation and a left object.
# *Action: Use another join type or change the lateral view to an inline view.

1039,qpERR_ABORT_QMO_NOT_ALLOWED_OUTER_COLUMN_IN_SUBQUERY_WITH_CONNECT_BY_FUNC = A CONNECT BY function cannot take a subquery containing an outer column as an argument. <0%s>
# *Cause: A subquery containing an outer column is taken as an argument.
# *Action: Remove the outer column from the subquery.

1040,qpERR_ABORT_QMO_NOT_ALLOWED_OUTER_JOIN_OPER = Unable to execute the outer join operator.
# *Cause: Unable to execute the outer join operator.
# *Action: Check the OUTER_JOIN_OPERATOR_TRANSFORM_ENABLE property value.

1119,qpERR_ABORT_QMO_NOT_ALLOWED_LOB_FILTER = Lob filter is not supported.
# *Cause: The lob filter was used within a hierarchy query in the SELECT FOR UPDATE statement.
# *Action: Remove the lob filter from the query.

###############################################################################
#########                 QMR ERROR CODE                            #########
###############################################################################

944,qpERR_ABORT_QMR_NO_CALLBACK = No callback for remote table is registered.
# *Cause: No callback for the remote table is registered.
# *Action: Verify that the database link is enabled.

945,qpERR_ABORT_QMR_NO_DATABASE_LINK_NAME = Database link name is NULL.
# *Cause: Database link name is NULL.
# *Action: Verify that the database link name is not null.

###############################################################################
#########                 QRC ERROR CODE                              #########
###############################################################################
132,qpERR_ABORT_QRC_NO_GRANT = The user has no permission to execute the SQL statement.
# *Cause: The user does not have sufficient permissions to execute the SQL statement.
# *Action: Verify that the user has sufficient permission to execute the SQL statement.

133,qpERR_ABORT_QRC_DUPLICATE_REPLICATION = Duplicate replication names. The replication name already exists in the database.
# *Cause: The replication name already exists in the database. The IP address and the port number must be unique.
# *Action: Use another replication name. Consult the meta tables in order to find replications that use the same IP address and port number.

134,qpERR_ABORT_QRC_NOT_EXIST_REPLICATION = Undefined replication name. The replication name was not found in the database.
# *Cause: The replication name was not found in the database. The name is incorrect or the replication has not been created yet.
# *Action: Verify that the replication name is typed correctly. Consult meta tables to see if the same replication name exists in the database.

135,qpERR_ABORT_QRC_MAX_REPLICATION_COUNT = No more replications may be created. A database cannot have more than the maximum number of replications.
# *Cause: A database cannot have more than the maximum number of replications.
# *Action: Check whether the system already has the maximum number of replications.

136,qpERR_ABORT_QRC_NOT_EXISTS_PRIMARY_KEY = A replicated table must have a primary key. (<0%s>.<1%s>)
# *Cause: No primary key was found in the table to be replicated. For replication, specify a table having a primary key.
# *Action: Create a primary key for the table to be replicated.

137,qpERR_ABORT_QRC_REPLICATION_ALREADY_STARTED = Replication has already started.
# *Cause: Replication is currently underway.
# *Action: Stop the current replication to start again.

138,qpERR_ABORT_QRC_CANNOT_USE_META = Replication is not allowed on meta tables.
# *Cause: It is impossible to replicate a meta table.
# *Action: Verify that the table to be replicated is not a meta table.

139,qpERR_ABORT_QRC_REPLICATE_TABLE_WITH_REFERENCE = Replication is not allowed on tables that have referential constraints. (<0%s>.<1%s>)
# *Cause: The user tried to replicate a table that has referential constraints.
# *Action: Verify that the table to be replicated does not have any referential constraints.

443,qpERR_ABORT_QRC_REPLICATED_OBJECT_TYPE = The replicated object type must be a table. <0%s>
# *Cause: Only a table may be replicated.
# *Action: Check the object type.

406,qpERR_ABORT_QRC_NOT_EXIST_REPL_ITEM = No replication items found.
# *Cause: No replication items were found.
# *Action: Check the meta tables.

653,qpERR_ABORT_QRC_ROLE_NOT_SUPPORT_SYNC = Replication SYNC is not supported in this role.
# *Cause: This role does not support SYNC.
# *Action: Check the role of the replication.

654,qpERR_ABORT_QRC_ROLE_NOT_SUPPORT_UNIX_DOMAIN = UNIX domain socket is not supported in this role.
# *Cause: This role does not support UNIX_DOMAIN in the 'WITH' clause.
# *Action: Check the role of the replication.

655,qpERR_ABORT_QRC_ROLE_SUPPORT_ONE_SOCKET_TYPE = One socket type is supported in this role.
# *Cause: This role supports one socket type.
# *Action: Check the role of the replication.

656,qpERR_ABORT_QRC_ROLE_SUPPORT_ONE_UNIX_DOMAIN = Only one UNIX domain socket is supported in this role.
# *Cause: This role supports one UNIX domain socket.
# *Action: Check the role of the replication.

658,qpERR_ABORT_QRC_ROLE_NOT_SUPPORT_DEFAULT_REPL_MODE = The default replication mode is not supported in this role.
# *Cause: This role does not support the default replication mode.
# *Action: Check the role of the replication.

659,qpERR_ABORT_QCI_NotPermittedUser = Unauthorized user.
# *Cause: The user does not have appropriate privileges.
# *Action: Check the user’s privileges.

660,qpERR_ABORT_QCM_DNBufferOverflow = The OwnerDN length has exceeded its range (overflow).
# *Cause: OwnerDN length overflow.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

675,qpERR_ABORT_QRC_INVALID_HOST_IP_PORT = The host IP address or port number is invalid.
# *Cause: Invalid host IP address or port number.
# *Action: Check the host IP address and port number.

677,qpERR_ABORT_QRC_ALREADY_EXIST_REPL_HOST = Replication hosts already exist.
# *Cause: Replication hosts already exist.
# *Action: Check the meta tables.

678,qpERR_ABORT_QRC_NOT_EXIST_REPL_HOST = Replication hosts not found.
# *Cause: No replication hosts were found.
# *Action: Check the meta tables.

691,qpERR_ABORT_QRC_REPL_RECOVERY_COUNT = A table can recover only one replication.
# *Cause: Table recovery already supported by another replication.
# *Action: Check the replication recovery count.

692,qpERR_ABORT_QRC_ROLE_NOT_SUPPORT_REPL_RECOVERY = Recovery option not supported in this role.
# *Cause: This role does not support the recovery option.
# *Action: Check the options or the role of the replication.

693,qpERR_ABORT_QRC_ALREADY_RECOVERY_SET = Recovery option already set.
# *Cause: The replication recovery option has already been set.
# *Action: Check the replication options.

694,qpERR_ABORT_QRC_ALREADY_RECOVERY_UNSET = The recovery option was already unset.
# *Cause: The replication recovery option was already unset.
# *Action: Check the replication options.

695,qpERR_ABORT_QRC_NOT_ALLOW_ADD_TABLE = An ADD TABLE statement is not allowed in recovery support replication.
# *Cause: An ADD TABLE statement is not allowed in a recovery support replication.
# *Action: Check the replication options.

696,qpERR_ABORT_QRC_NOT_ALLOW_DROP_TABLE = An DROP TABLE statement is not allowed in recovery support replication.
# *Cause: A DROP TABLE statement is not allowed in recovery support replication.
# *Action: Check the replication options.

722,qpERR_ABORT_QRC_REPL_MAKE_CONDITION = The attempt to make replication conditions failed.
# *Cause: The WHERE clause is invalid.
# *Action: Check the WHERE clause.

738,qpERR_ABORT_QRC_TOO_LONG_REPL_CONDITION_VALUE = Replication condition value too long
# *Cause: A replication condition value is too long.
# *Action: Check the length of replication condition values.

787,qpERR_ABORT_QRC_NOT_APPLICABLE_POLICY = This column policy is not compatible with replication. [<0%s>]
# *Cause: This column policy is not compatible with replication.
# *Action: Check the column policy.

791,qpERR_ABORT_QRC_CANNOT_USE_VOLATILE_TABLE = Replication not allowed on volatile tables.
# *Cause: A volatile table cannot be replicated.
# *Action: Verify that the table to be replicated is not a volatile table.

804,qpERR_ABORT_QDB_REPLICATION_DDL_EAGER_MODE = Cannot execute DDL statements on a replicated table while replication is running in eager mode.
# *Cause: DDL statements cannot be executed on a replicated table running in eager mode.
# *Action: Check the replication mode.

805,qpERR_ABORT_QRC_SELECTION_AND_EAGER = The attempt to create a replication failed because conditions and eager mode were both specified.
# *Cause: A replication object with conditions cannot run in eager mode.
# *Action: Change the replication mode or remove the conditions.

806,qpERR_ABORT_QRC_NOT_SUPPORT_REPL_OFFLINE_AND_EAGER = The offline option is not supported with eager mode.
# *Cause: Simultaneous use of eager mode and the offline option is not supported.
# *Action: Change the option or mode of the replication.

807,qpERR_ABORT_QRC_NOT_SUPPORT_REPLICATION_DDL = This replication DDL is no longer supported.
# *Cause: An attempt to use a deprecated DDL statement was made.
# *Action: This DDL statement cannot be run on a replicated table.

808,qpERR_ABORT_QRC_NOT_SUPPORT_MAX_ROWS_AND_EAGER = Replication in eager mode is not supported with the MAX ROWS table option.
# *Cause: An attempt was made to use replication in eager mode on a table for which the MAX ROWS option was specified.
# *Action: Check mode of the replication.

829,qpERR_ABORT_QRC_NOT_SUPPORT_RETRY_AND_EAGER = The retry option is not supported when replication is running in eager mode.
# *Cause: The retry option is not supported in eager mode.
# *Action: Either remove the retry option or change the replication mode.

866,qpERR_ABORT_QRC_CANNOT_USE_TEMPORARY_TABLE = Temporary tables cannot be replicated.
# *Cause: An attempt was made to add a temporary table to a replication object.
# *Action: Verify that none of the tables to be replicated are temporary tables.

899,qpERR_ABORT_QRC_NOT_SUPPORT_AT_SN_CLAUSE = Replication cannot start from a specific SN unless it is used with the Log Analyzer.
# *Cause: The AT SN clause was specified, but the role of the replication object is not "Log Analyzer".
# *Action: Either specify the FOR ANALYSIS clause in the CREATE REPLICATION statement, or do not use the AT SN clause in the ALTER REPLICATION statement.

###############################################################################
#########                 QTC ERROR CODE                              #########
###############################################################################
285,qpERR_ABORT_QTC_TUPLE_SHORTAGE = There are too many DML statements in the stored procedure, or the SQL query is too long.
# *Cause: The internal tuple set has been exhausted.
# *Action: The query or stored procedure is too long. Split the stored procedure or query.

286,qpERR_ABORT_QTC_INVALID_AGGREGATION = Invalid aggregation
# *Cause: Invalid use of aggregation.
# *Action: Change the SQL statement.

287,qpERR_ABORT_QTC_INVALID_ARGUMENT_TYPE = Invalid argument type. <0%s>
# *Cause: Invalid argument type
# *Action: Check the argument type.

288,qpERR_ABORT_QTC_NO_HOSTVAR_ALLOWED = No host variable is allowed here.
# *Cause: No host variable is allowed here.
# *Action: Check the argument.

2,qpERR_ABORT_QTC_MULTIPLE_ROWS = A single-row subquery has returned more than one row.
# *Cause: A single-row subquery has returned more than one row.
# *Action: Check the number of records returned by the subquery.

521,qpERR_ABORT_QTC_TOO_MANY_TABLES = Too many tables are referenced in a phrase.
# *Cause: A phrase (FROM, WHERE, etc) references over 32 tables.
# *Action: Check and modify the query so that it does not reference more than 32 tables.

600,qpERR_ABORT_QTC_NOT_EXISTS_COLUMN = Column not found
# *Cause: The specified column was not found in the table.
# *Action: Verify that the column name is correct.

732,qpERR_ABORT_QTC_HOST_VAR_LIMIT_EXCEED = The number of host variables exceed the maximum limit (<0%d>).
# *Cause: The number of host variables exceeds the maximum limit.
# *Action: Verify the number of host variables.

739,qpERR_ABORT_QTC_INVALID_WINDOW_FUNCTION = Window functions are not allowed here. <0%s>
# *Cause: Window functions are not allowed here.
# *Action: Specify a valid option value.

740,qpERR_ABORT_QTC_UNSUPPORTED_FUNCTION = Unsupported function in window functions. <0%s>
# *Cause: This function is not supported yet.
# *Action: Specify a valid option value.

871,qpERR_ABORT_QTC_MISSING_ORDER_IN_WINDOW_FUNCTION = An ORDER BY expression is required in the window function. <0%s>
# *Cause: A ranking function requires an ORDER BY expression in the OVER clause.
# *Action: Specify an ORDER BY expression.

778,qpERR_ABORT_QTC_INVALID_ENCRYPTED_COLUMN = Invalid encrypted column.
# *Cause: The column is not valid for encryption.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

779,qpERR_ABORT_QTC_INVALID_ENCRYPTION_DATATYPE = Invalid encryption datatype.
# *Cause: Invalid encryption datatype.
# *Action: Check the encryption datatype.

780,qpERR_ABORT_QTC_TOO_LONG_POLICY_NAME = Policy name is too long. <0%s>
# *Cause: A policy name is too long.
# *Action: Check the length of the policy name.

931,qpERR_ABORT_QTC_MISSING_OVER_IN_WINDOW_FUNCTION = OVER clause required for this function. <0%s>
# *Cause: The windowing function requires the OVER clause.
# *Action: Add the proper OVER clause for the windowing function.

1051,qpERR_ABORT_QTC_NOT_APPLICABLE_TYPE_IN_LOOP = An unsupported data type was specified for the loop clause. <0%s>
# *Cause: An unsupported data type was specified for the loop clause.
# *Action: Verify that the following data type is specified for the loop clause: smallint, integer, bigint, list, associative array, or record/rowtype.

1052,qpERR_ABORT_QTC_INVALID_LOOP_VALUE = A value less than 0 was specified.
# *Cause: A value that is greater than or equal to 0 must be specified for the loop clause.
# *Action: Specify a positive value.

1057,qpERR_ABORT_QTC_PARAM_NAME_NOTATION_NOW_ALLOWED = Name-based argument notation is not allowed.
# *Cause: Name-based argument notation for the expression is not allowed.
# *Action: Do not reference argument as name-based in the expression

1068,qpERR_ABORT_QTC_OVER_CLAUSE_NOT_ALLOWED = The OVER clause is not allowed for this function. <0%s>
# *Cause: The OVER clause is not allowed here.
# *Action: Do not use the OVER clause with this function.

1104,qpERR_ABORT_QTC_SUBQUERY_RETURN_VALUE_CHANGED = Return values of performance view single-row subquery has changed during execution.
# *Cause: Return values of performance view single-row subquery has changed during execution.
# *Action: This error can occur rarely when there is a change in the values of performance view single-row subquery during a query execution. You will get a result without error if you execute again the same query that produced this error. If you meet this error repeatedly, contact Altibase’s Support Center (http://support.altibase.com).

1140,qpERR_ABORT_QTC_INVALID_SRID_DATATYPE = Fail to specify the Spatial Reference ID(SRID) because a SRID can be set only in the geometry dataype.
# *Cause: A Spatial Reference ID(SRID) can be set only in the geometry dataype.
# *Action:  Check the datatype for the Spatial Reference ID(SRID).

###############################################################################
#########                 QCU ERROR CODE                              #########
###############################################################################
291,qpERR_ABORT_QCU_RESOURCE_BUSY = Resource busy. ( <0%s> )
# *Cause: The resource is busy.
# *Action: Try again later.

682,qpERR_REBUILD_QCU_RESOURCE_BUSY = Resource busy.
# *Cause: The resource is busy.
# *Action: No action is necessary.

396,qpERR_ABORT_QCU_NO_SUCH_LATCH_OBJECT = Procedure or function not found.
# *Cause: The procedure or function was not found in the database.
# *Action: Verify that the name of the procedure or function is correct.

515,qpERR_FATAL_QCU_INVALID_INSERT_LIMIT_PROPERTY = Invalid values for INSERT_LOW_LIMIT and INSERT_HIGH_LIMIT properties.
# *Cause: The value of the INSERT_HIGH_LIMIT property must be greater than that for the INSERT_LOW_LIMIT property.
# *Action: Check the INSERT_LOW_LIMIT and INSERT_HIGH_LIMIT properties.

743,qpERR_FATAL_QCU_INVALID_TTL_SIZE_PROPERTY = Invalid values for INIT_TRANS and MAX_TRANS properties.
# *Cause: The value of the MAX_TRANS property must be greater than that for the INIT_TRANS property.
# *Action: Check the INIT_TRANS and MAX_TRANS properties.

557,qpERR_ABORT_QCU_PSM_FILE_OPEN_LIMIT_EXCEED = The number of opened file handles exceeds the maximum limit.
# *Cause: The number of opened file handles exceeds the maximum limit.
# *Action: Call the FCLOSE_ALL function or change the PSM_FILE_OPEN_LIMIT property value.

788,qpERR_ABORT_QCU_TOO_LONG_MODULE_NAME = A security module name is too long.
# *Cause: A security module name is too long.
# *Action: Check the length of the security module name.

789,qpERR_ABORT_QCU_TOO_LONG_LIBRARY_NAME = A security library name is too long.
# *Cause: A security library name is too long.
# *Action: Check the length of the security library name.

943,qpERR_ABORT_QCU_INVALID_FEATURE_ENABLE_VALUE = Invalid value for OPTIMIZER_FEATURE_ENABLE property.
# *Cause: The specified value for OPTIMIZER_FEATURE_ENABLE property is invalid.
# *Action: Check for appropriate values for OPTIMIZER_FEATURE_ENABLE property.Refer to the General Reference Manual for information about supported options. Select an available option.

1021,qpERR_ABORT_QCU_NO_SUCH_LATCH_PKG_OBJECT = Package not found.
# *Cause: The package or package body was not found in the database.
# *Action: Verify that the name of the package or package body is correct.

1050,qpERR_ABORT_QCU_CONNECT_TYPE_OPEN_LIMIT_EXCEED = The number of opened sokcet handles exceeds the maximum limit.
# *Cause: The number of opened socket handles exceeds the maximum limit.
# *Action: Call the CLOSEALL_CONNECT function or change the CONNECT_TYPE_OPEN_LIMIT property value.

1062,qpERR_ABORT_QCU_MEM_POOL_ALLOC_FAILED = Failed to allocate a memory pool
# *Cause:
#   - Failed to allocate a memory pool.
# *Action:
#   - Verify that there is enough available memory.

1063,qpERR_ABORT_QCU_HASH_TABLE_ALLOC_FAILED = Failed to allocate a hash table
# *Cause:
#   - Failed to allocate a hash table.
# *Action:
#   - Verify that there is enough available memory.

1064,qpERR_ABORT_QCU_HASH_TABLE_FIND_FAILED = Failed to find a record from a hash table
# *Cause:
#   - Failed to find a record from a hash table.
# *Action:
#   - Contact Altibase’s Support Center (http://support.altibase.com).

1065,qpERR_ABORT_QCU_HASH_TABLE_ADD_FAILED = Failed to add a record to a hash table
# *Cause:
#   - Failed to add a record to a hash table.
# *Action:
#   - Contact Altibase’s Support Center (http://support.altibase.com).

1066,qpERR_ABORT_QCU_USER_LOCK_REQUEST_LIMIT_EXCEED = The number of requested user locks exceeds the maximum limit.
# *Cause:
#   - The number of requested user locks exceeds the maximum limit.
# *Action:
#   - Call the USER_LOCK_RELEASE function or change the USER_LOCK_REQUEST_LIMIT property value.

1142,qpERR_ABORT_QCU_NOT_SUPPORT_SYSTEM_PROPERTY = <0%s> property cannot be changed using ALTER SYSTEM statement.
# *Cause: 
#   - The property can be changed using ALTER SESSION statement, becuase Alter level of the property is SESSION.
# *Action:
#   - Use ALTER SESSION statement

###############################################################################
#########                 QCI ERROR CODE                              #########
###############################################################################
294,qpERR_ABORT_QCI_INVALID_BINDING = Invalid binding to host variables
# *Cause: Invalid binding to host variables has been detected.
# *Action: Check the binding to host variables.

295,qpERR_ABORT_QCI_BUFFER_FULL = Buffer full in QCI
# *Cause:
#   - Buffer full in QCI
# *Action:
#   - Buffer full in QCI

440,qpERR_ABORT_QCI_INVALID_HOST_DATA_SIZE = Size of data to bind to host variable is invalid [ Param ID = <0%d>, Data Type = <1%u>, Data Size = <2%d>, Declared Size of Host Variable = <3%d> ]
# *Cause: The size of the data and the size of the host variable differ from one another.
# *Action: Check the size of the data to be bound to host variables.

575,qpERR_ABORT_TOO_LONG_IDENTIFIER_NAME = The identifier name is too long
# *Cause: The identifier name is too long.
# *Action: Give the identifier a shorter name.

576,qpERR_ABORT_INVALID_STATEMENT_STATE_ERROR = Invalid request to process SQL statement
# *Cause: A request was made to perform an operation on an SQL statement that has already been freed.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

583,qpERR_REBUILD_QCI_PROC_INVALID = A procedure or function was invalidated during execution.
# *Cause: A procedure or function has been invalidated.
# *Action: Recompile the required procedure or function again.

584,qpERR_ABORT_BIND_COLUMN_COUNT_MISMATCH = Mismatched bind column count
# *Cause: The number of host variables and the number of data items to bind to them are different.
# *Action: Check client applications.

585,qpERR_REBUILD_QCI_EXEC = The statement failed during execution.
# *Cause: The statement failed during execution.
# *Action: No action is necessary. The statement will be automatically rebuilt.

880,qpERR_ABORT_QCI_ACCOUNT_STATUS_LOCKED = The account is locked.
# *Cause: The user has exceeded the allowed number of incorrect password attempts, or the DBA has locked the account.
# *Action: Wait until the lock is lifted.
                        
881,qpERR_ABORT_QCI_ACCOUNT_STATUS_EXPIRED = The password has expired.
# *Cause: The user's account has expired and the password needs to be changed.
# *Action: Change the password.
    
882,qpERR_ABORT_QCI_INVALID_PASSWORD_ERROR = Invalid password.
# *Cause: The password is incorrect.
# *Action: Enter the correct password for the user.

1071,qpERR_ABORT_QCI_CANNOT_EXEC_DDL = DDL statements cannot be executed in DA mode. Check the connection type.
# *Cause: DDL statements cannot be executed in DA mode.
# *Action: Check the connection type.

###############################################################################
#########            STORED PROCEDURE ERROR CODE                      #########
###############################################################################
297,qpERR_ABORT_QSV_NOT_EXIST_PROC_SQLTEXT = Procedure or function not found : <0%s>.
# *Cause: The specified procedure or function name was not found in the database.
# *Action: Verify that the procedure or function exists.

298,qpERR_ABORT_QSV_DUPLICATED_PROC_NAME_SQLTEXT = Duplicate procedure or function name <0%s>
# *Cause: A procedure or function with that name already exists in the database.
# *Action: Give the procedure or function a different name.

301,qpERR_ABORT_QSV_DUPLICATE_PARAMETER_SQLTEXT = Duplicate parameter names. <0%s>
# *Cause: A parameter with the same name has already been defined.
# *Action: Use a different name for the parameter.

302,qpERR_ABORT_QSV_PROC_HAVE_RETURN = A procedure cannot have a RETURN type.
# *Cause: The procedure has a RETURN type.
# *Action: Remove the RETURN type specification from the procedure declaration.

303,qpERR_ABORT_QSV_PROC_NOT_SUPPORTED_DATATYPE_SQLTEXT = Unsupported data type for parameter, RETURN, or local variable. <0%s>
# *Cause: A procedure must not use an unsupported data type for a parameter, RETURN, or local variable.
# *Action: Verify the data type for the parameter, RETURN, or local variable.

305,qpERR_ABORT_QSV_PROC_HAVE_RETURN_VALUE_SQLTEXT = A procedure cannot have a RETURN statement. <0%s>
# *Cause: A procedure cannot have a RETURN statement.
# *Action: Remove the RETURN statement from the procedure declaration.

306,qpERR_ABORT_QSV_PROC_NOT_HAVE_RETURN_VALUE_SQLTEXT = A function must return a value. <0%s>
# *Cause: A function must return a value.
# *Action: Specify a return value in the RETURN statement.

307,qpERR_ABORT_QSV_PROC_NOT_HAVE_RETURN_STMT = A function must have a RETURN statement.
# *Cause: A function must have a RETURN statement.
# *Action: Specify a RETURN statement.

308,qpERR_ABORT_QSV_PROC_MULTIPLE_REC_TARGET_SQLTEXT = Multiple targets exist in an INTO clause. <0%s>
# *Cause: Multiple targets exist in an INTO clause.
# *Action: Check the INTO clause.

309,qpERR_ABORT_QSV_PROC_SELF_TYPE_SQLTEXT = Recursive type declaration using %TYPE, %ROWTYPE <0%s>
# *Cause: A recursive type declaration using %TYPE, %ROWTYPE has been detected.
# *Action: Check local variable declarations.

310,qpERR_ABORT_QSV_PROC_NOT_EXIST_STMT_SQLTEXT = The label statement must not be the last statement.<0%s>
# *Cause: The label statement must not be the last statement.
# *Action: Specify another statement or a NULL statement after the label.

311,qpERR_ABORT_QSV_PROC_CANNOT_ASSIGN_SQLTEXT = Mismatched number of fields in ROWTYPE expressions <0%s>
# *Cause: The number of fields in the L-value and R-value of a ROWTYPE assignment must be the same.
# *Action: Check the number of fields in the ROWTYPE variables.

312,qpERR_ABORT_QSV_PROC_NOT_USE_IN_LOOP_SQLTEXT = EXIT and CONTINUE statements must be used only in loops. <0%s>
# *Cause: An EXIT or CONTINUE statement was used outside of a loop.
# *Action: Check the EXIT or CONTINUE statement.

313,qpERR_ABORT_QSV_PROC_DUP_EXCEPTION_IN_HANDLER_SQLTEXT = Duplicate exception handlers <0%s>
# *Cause: An exception handler with the same name already exists in the block.
# *Action: Remove the duplicate exception handler from the block.

314,qpERR_ABORT_QSV_PROC_NOT_EXIST_EXCEPTION_SQLTEXT = Undefined exception name. <0%s>
# *Cause: Undefined exception name
# *Action: Create an exception handler with that name.

315,qpERR_ABORT_QSV_PROC_NOT_ALLOW_DEFAULT_SQLTEXT = OUT or IN OUT parameters cannot have default values. <0%s>
# *Cause: OUT or IN OUT parameters cannot have default values.
# *Action: Remove the DEFAULT value specification from the OUT or IN OUT parameters.

316,qpERR_ABORT_QSV_PROC_NOT_LOOP_LABEL_SQLTEXT = A label name in an EXIT statement must point to a loop statement. <0%s>
# *Cause: A label name in an EXIT statement must be a label that indicates a LOOP statement.
# *Action: Check the label name in the EXIT statement.

317,qpERR_ABORT_QSV_PROC_MAX_PARA_NUM_INT_ARG1 = The number of parameters in a stored procedure or function exceeds the maximum limit (<0%d>).
# *Cause: The number of parameters in a stored procedure or function exceeds the maximum limit.
# *Action: Verify the maximum number of parameters.

319,qpERR_ABORT_QSV_PROC_ASSIGN_LVALUE_NO_READONLY_VAR_SQLTEXT = The L-value of the ASSIGN statement is either a CONSTANT variable or an IN parameter. <0%s>
# *Cause: The L-value of the ASSIGN statement is either a CONSTANT variable or an IN parameter.
# *Action: Check the L-value of the ASSIGN statement.

320,qpERR_ABORT_QSV_NOT_ROWTYPE_EXPRESSION = Not a ROWTYPE expression <0%s>
# *Cause: Not a ROWTYPE expression
# *Action: Check the expression.

322,qpERR_ABORT_QSV_PROC_SELECT_INTO_NO_READONLY_VAR_SQLTEXT = The INTO clause of the SELECT statement references a CONSTANT variable or an IN parameter. <0%s>
# *Cause: The INTO clause of the SELECT statement references a CONSTANT variable or an IN parameter.
# *Action: Check the INTO clause of the SELECT statement.

323,qpERR_ABORT_QSV_CURSOR_NAME_IS_SQL_SQLTEXT = A cursor name must not be "SQL". <0%s>
# *Cause: A cursor is named "SQL".
# *Action: Do not use "SQL" as a cursor name.

324,qpERR_ABORT_QSV_INSUFFICIENT_ARGUEMNT_SQLTEXT = Insufficient arguments <0%s>
# *Cause: Insufficient arguments.
# *Action: Verify that the number of arguments matches the number specified in the parameter declaration.

325,qpERR_ABORT_QSV_TOO_MANY_ARGUEMNT_SQLTEXT = Too many arguments <0%s>
# *Cause: Too many arguments.
# *Action: Verify that the number of arguments matches the number specified in the parameter declaration.

326,qpERR_ABORT_QSV_ROWTYPE_IN_EXPRESSION_SQLTEXT = No ROWTYPE is allowed in this expression. <0%s>
# *Cause: No ROWTYPE is allowed in this expression.
# *Action: Check the type of the local variable.

327,qpERR_ABORT_QSV_INVALID_OUT_ARGUEMNT_SQLTEXT = Invalid argument in OUT parameter <0%s>
# *Cause: Invalid argument in the OUT parameter.
# *Action: Verify that the argument is correct.

328,qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT = Undefined identifier <0%s>
# *Cause: A variable, cursor, or exception name is undefined.
# *Action: Check the names of variables, cursors, and exceptions.

329,qpERR_ABORT_QSV_DUPLICATE_VARIABLE_SQLTEXT = Duplicate type or variable names : <0%s>
# *Cause: A parameter, type, variable, cursor, or exception with the same name has already been defined.
# *Action: Use a different name.

330,qpERR_ABORT_QSV_PROC_NOT_CONSTANT_SQLTEXT = A constant variable must have a DEFAULT value. <0%s>
# *Cause: A constant variable must have a DEFAULT value.
# *Action: Specify a DEFAULT value for the constant local variable.

331,qpERR_ABORT_QSV_PROC_WRONG_CONSTANT_SQLTEXT = A non-primitive type variable cannot be defined as a CONSTANT variable. <0%s>
# *Cause: A non-primitive variable cannot be defined as a CONSTANT variable.
# *Action: Check the variable declaration.

332,qpERR_ABORT_QSV_INAPPROPRIATE_INTO_SQLTEXT = Inappropriate INTO clause. The INTO clause must be specified immediately after the first target list of a SELECT statement. <0%s>
# *Cause: The INTO clause must be specified immediately after the first target list of a SELECT statement.
# *Action: Verify that the INTO clause immediately follows the target list of the SELECT statement.

333,qpERR_ABORT_QSV_NOT_HAVE_INTO_SQLTEXT = A SELECT statement in a procedure or function must have an INTO clause. <0%s>
# *Cause: A SELECT statement in a procedure or function must have an INTO clause.
# *Action: Verify that the SELECT statement has an INTO clause.

334,qpERR_ABORT_QSV_PROC_WRONG_ROWTYPE_DEFINITION = The declaration of the ROWTYPE attribute is invalid. <0%s>
# *Cause: The declaration of the ROWTYPE attribute is invalid.
# *Action: Check the ROWTYPE attribute.

335,qpERR_ABORT_QSV_MISMATCHED_INTO_LIST_SQLTEXT = The number of columns in the target list does not match the number of variables in the INTO clause. <0%s>
# *Cause: The number of columns in the target list does not match the number of variables in the INTO clause.
# *Action: Verify that the number of columns and the number of variables are exactly the same.

336,qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE = A cursor, cursor attribute, SQLCODE, or SQLERRM is used in an inappropriate clause.<0%s>
# *Cause: A Cursor, cursor attribute, SQLCODE, or SQLERRM is used in an inappropriate clause.
# *Action: Remove the cursor or the cursor attribute from the expression.

389,qpERR_ABORT_QSV_NOT_ALLOWED_SUBQUERY_SQLTEXT = A subquery is not allowed here. <0%s>
# *Cause: A subquery is not allowed here.
# *Action: Remove the subquery.

392,qpERR_ABORT_QSV_NOT_CURSOR = The argument of the CURSOR attribute is not a CURSOR. <0%s>
# *Cause: The argument of the CURSOR attribute is not a CURSOR.
# *Action: Verify that the argument of the CURSOR attribute is a CURSOR.

441,qpERR_ABORT_QSV_NOT_ALLOWED_COUNTER_VAR_SQLTEXT = A counter variable is not allowed here. <0%s>
# *Cause: A counter variable is not allowed here.
# *Action: Use another variable.

526,qpERR_ABORT_QSV_DUPLICATE_LABEL = Duplicate label name in same block. <0%s>
# *Cause: The same label name exists in this block.
# *Action: Check the label name.

527,qpERR_ABORT_QSV_ILLEGAL_GOTO = Illegal GOTO statement. <0%s>
# *Cause: The label is not defined, or GOTO cannot pass control to the label.
# *Action: Check the label.

530,qpERR_ABORT_QSV_PROC_NOT_USE_IN_EXCEPTION_HANDLER = A RAISE statement with no exception name must be inside an exception handler. <0%s>
# *Cause: The program contains a RAISE statement that is not followed by an exception name: RAISE; and that statement is located outside of an exception handler.
# *Action: Check the RAISE statement.

601,qpERR_ABORT_QSV_TRIGGER_UNSUPPORTED_RESULT_SET = Trigger does not support result set cursor. <0%s>
# *Cause: Trigger does not support result set cursors.
# *Action: Do not use a result set cursor.

848,qpERR_ABORT_QSV_RECORD_WRONG = Wrong number of columns in record type. <0%s>
# *Cause: The record type did not contain any columns.
# *Action: Check the record type definition.

849,qpERR_ABORT_QSV_ONLY_ASSOCIATIVE = An associative array must be used. <0%s>
# *Cause: An associative array was expected.
# *Action: Check the host variables in the INTO clause.

850,qpERR_ABORT_QSV_BULK_ASSOCIATIVE = Use the BULK COLLECT INTO statement to return multiple rows. <0%s>
# *Cause: Multiple targets exist in an INTO clause.
# *Action: Use BULK COLLECT.

935,qpERR_ABORT_QSV_NOT_EXIST_LIBRARY_SQLTEXT = Library not found : <0%s>.
# *Cause: The specified library was not found in the database.
# *Action: Verify that the library exists.

936,qpERR_ABORT_QSV_FORMAL_PARAMETER_MISSING = Formal parameter missing in the parameters : <0%s>.
# *Cause: Formal parameter missing in the parameters.
# *Action: Provide a matching parameter in the parameters clause for every formal parameter.

937,qpERR_ABORT_QSV_MULTIPLE_DECLARATIONS = Multiple declarations in foreign function formal parameter list : <0%s>.
# *Cause: Multiple declarations in foreign function formal parameter list.
# *Action: Remove the duplicate parameter from the foreign function formal parameter list.

938,qpERR_ABORT_QSV_EXTERNAL_PARAMETER_NAME_NOT_FOUND = External parameter name not found in formal parameter list : <0%s>.
# *Cause: Failed to find the external parameter name in the formal parameter list.
# *Action: Provide a parameter in the parameters clause that matches one of the formal parameters.

939,qpERR_ABORT_QSV_INCORRECT_USAGE_OF_XXX = Incorrect Usage of <0%s> in parameters clause : <1%s>.
# *Cause: Incorrect usage of RETURN in parameters clause.
# *Action: Refer to the C/C++ External Procedure Manual for the formal parameters available for specifying a property. Use a valid formal parameter.

940,qpERR_ABORT_QSV_RETURN_MUST_BE_IN_THE_LAST = RETURN, for actual function return, must be last in the parameters clause: <0%s>.
# *Cause: RETURN, for actual function return, must be last in the parameters clause.
# *Action: Place RETURN at the end of the parameters clause.

941,qpERR_ABORT_QSV_PSM_ROW_SIZE_EXCEED_LIMIT = The row size in the stored procedure or function exceeds the maximum limit. ( requested size : <0%ld>, max size : <1%ld> )
# *Cause: The row size is too big to process in the stored procedure or function.
# *Action: Reduce the number of columns to process.

955,qpERR_ABORT_QSV_PROC_USED_BY_CONSTRAINT = The procedure or function is used by constraint.
# *Cause: A constraint is using the procedure or function.
# *Action: Drop a constraint that is using the procedure or function.

956,qpERR_ABORT_QSV_PROC_USED_BY_INDEX = The procedure or function is used by index.
# *Cause: An index is using the procedure or function.
# *Action: Drop the index that is using the procedure or function.

957,qpERR_ABORT_QSV_DUPLICATE_PKG = Duplicate package specification or package body names <0%s>
# *Cause: The package specification or package body name already exists in the database.
# *Action: Use another package specification or package body name.

958,qpERR_ABORT_QSV_NOT_EXIST_PACKAGE_SPECIFICATION = Package specification not found. <0%s>
# *Cause: An attempt was made to create a package body before creating the package specification.
# *Action: Create the package specification before creating the package body.

959,qpERR_ABORT_QSV_NOT_EXIST_PKG = Package not found. <0%s>
# *Cause: The specified package name was not found in the database.
# *Action: Verify that the package or package body exists.

960,qpERR_ABORT_QSV_DUPLICATE_PKG_SUBPROGRAM = Duplicate procedure or function names found in the package. <0%s>
# *Cause: Duplicate procedure or function names were declared in the package specification.
# *Action: Verify that no duplicate procedure or function names are declared in the package specification.

961,qpERR_ABORT_QSV_INVALID_OR_MISSING_SUBPROGRAM = A subprogram [<0%s>] found in the package is missing in the package body.
# *Cause: A subprogram found in the package specification is missing in the package body.
# *Action: Define all of the subprograms from the package specification in the package body.

962,qpERR_ABORT_QSV_NOT_EXIST_PKG_BODY = Package body not found. <0%s>
# *Cause: The package body has not been created yet.
# *Action: Create the package body.

963,qpERR_ABORT_QSV_INVALID_IDENTIFIER = Identifier must be declared. <0%s>
# *Cause: The specified identifier is invalid.
# *Action: Check the specified user name or object name.

964,qpERR_ABORT_QSV_UNDEFINED_SUBPROGRAM = Undefined subprogram. <0%s>
# *Cause: The specified subprogram was undefined in the package body.
# *Action: Define the subprogram in the package body.

965,qpERR_ABORT_QSV_CANNOT_DECLARE_REF_CURSOR_VARIABLE = Cursor Variables cannot be declared in the package specification.
# *Cause: A cursor variable was declared in the package specification.
# *Action: Check the variable type.

966,qpERR_ABORT_QSV_INVALID_REFERENCE_VARIABLE = Invalid reference variable
# *Cause: Invalid reference variable was declared.
# *Action: Check the variable type.

967,qpERR_ABORT_QSV_SUBPROGRAM_INVALID = Subprogram must be valid.
# *Cause: Undefined subprogram was called by another subprogram definition.
# *Action: Change the order of the function definition chronologically.

973,qpERR_ABORT_QSV_UNDECLARED_SUBPROGRAM = Subprogram was undeclared in the package specification. <0%s>
# *Cause: A subprogram was undeclared in the package specification.
# *Action: Declare the subprogram in the package specification.

974,qpERR_ABORT_QSV_VIOLATES_ASSOCIATED_PRAGMA = Subprogram violates its associated pragma.
# *Cause: A subprogram has violated its associated pragma.
# *Action: Check the pragma.

975,qpERR_ABORT_QSV_INVALID_PRAGMA_RESTRICT_REFERENCE = Pragma RESTRICT_REFERENCES must be declared in a package specification
# *Cause: Pragma RESTRICT_REFERENCES must be declared in a package specification.
# *Action: Check the object type.

1001,qpERR_ABORT_QSV_INVALID_PACKAGE_SPECIFICATION = Invalid package specification. <0%s>
# *Cause: A package specification is invalid.
# *Action: Recompile the package specification.

1003,qpERR_ABORT_QSV_CANNOT_DECLARE_TWICE_PRAGMA_AUTONOMOUS_TRANSACTION = Pragma AUTONOMOUS_TRANSACTION cannot be declared twice. <0%s>
# *Cause: Pragma AUTONOMOUS_TRANSACTION was declared twice in the same block.
# *Action: Remove the duplicate declaration of the Pragma AUTONOMOUS_TRANSACTION.

1004,qpERR_ABORT_QSV_CANNOT_SPECIFIED_PRAGMA_AUTONOMOUS_TRANSACTION = Pragma AUTONOMOUS_TRANSACTION cannot be specified here. <0%s>
# *Cause: This pragma AUTONOMOUS_TRANSACTION must be declared in the declarative section of a procedure, a function, a top level psm block.
# *Action: Check the the object type.

1005,qpERR_ABORT_QSV_CANNOT_USE_IN_AUTONOMOUS_TRANSACTION = Cannot use a reference cursor type in autonomous transaction block. <0%s>
# *Cause: Cannot use a reference cursor type in autonomous transaction block.
# *Action: Do not use a reference cursor type in autonomous transaction block.

1023,qpERR_ABORT_QSV_PACKAGE_VARIABLE_NOT_HAVE_RETURN = The identifier is not a procedure or is not defined. <0%s>
# *Cause: The identifier is not a procedure or is not defined.
# *Action: Check the identifier.

1058,qpERR_ABORT_QSV_EXCEPTION_IS_NOT_DECLARED_IN_SAME_BLOCK = PRAGMA EXCEPTION_INIT of <0%s> must follow declaration of its exception in the same declarative part. <1%s>
# *Cause: Place the EXCEPTION_INIT pragma directly after the exception declaration referenced by the pragma. 
# *Action: An EXCEPTION_INIT pragma was not declared in the same block as its exception. They must be declared in the proper order in the same block, with the pragma declaration following the exception declaration.

1059,qpERR_ABORT_QSV_REDUNDANT_EXCEPTIONS_IN_HANDLER = Redundant exceptions <0%s> and <1%s> cannot appear in same exception handler. <2%s>
# *Cause: Remove one of the exceptions or initialize it to a different Altibase error number. 
# *Action: The EXCEPTION_INIT pragma requires each exception in the exception handler to be initialized to a different Altibase error number. Verify that the exceptions do not have duplicate error numbers.

1060,qpERR_ABORT_QSV_INVALID_EXCEPTION_ERROR_CODE = Illegal Altibase error number <0%d> for PRAGMA EXCEPTION_INIT. <1%s>
# *Cause: The error number that was passed to an EXCEPTION_INIT pragma is out of range.
# *Action: Use a valid error number. For further information about error numbers, refer to the Error Message Reference.

1054,qpERR_ABORT_QSV_NORMAL_PARAM_PASSING_AFTER_NAMED_NOTATION = A positional argument notation may not follow a name-based one. <0%s>
# *Cause: In a stored procedure, a normal parameter notation may not follow a named parameter notation.
# *Action: Reference arguments as positional notation first, then do the others as name-based one.

1055,qpERR_ABORT_QSV_WRONG_PARAM_NAME_NOTATION = Name of an argument notation does not exist in declared parameters. <0%s>
# *Cause: Name of a parameter notation doesn't exist in declared parameters.
# *Action: Check the parameters of specified stored procedure.

1056,qpERR_ABORT_QSV_DUPLICATE_PARAM_NAME_NOTATION = Multiple notation of the same parameter is used. <0%s>
# *Cause: Multiple notation of the same parameter is used.
# *Action: Do not reference argument again.

1067,qpERR_ABORT_QSV_TOO_MANY_MATCH_THIS_CALL = Too many declarations match this call. <0%s>
# *Cause: Too many declarations match this call.
# *Action: Modify this call or its package declaration so that it is unambiguous.

1086,qpERR_ABORT_MAX_OPEN_CURSOR = The number of opened cursor exceeded the maximum limit.
# *Cause: The number of opened cursor exceeded the maximum limit. 
# *Action: Reduce the number of opened cursors.

1087,qpERR_ABORT_INVALID_CURSOR_OPERATION = Invalid cursor operation.
# *Cause: Invalid cursor operation.
# *Action: Check the cursor variable.

1088,qpERR_ABORT_ILLEGAL_VARIABLE_NAME = Illegal host variable name.
# *Cause: Illegal host variable name.
# *Action: Check the host variable name.

1153,qpERR_ABORT_ROLLBACKABLE_DDL_GLOBAL_NOT_ALLOWED_NON_PART_INDEX = A rollbackable DDL execution fails, because the table has global non partitioned index ( <0%s> ).
# *Cause: A rollbackable DDL for the table that has global non partitioned index is not supported.
# *Action: Retry after removing the global non partitioned index.

1154,qpERR_ABORT_ROLLBACKABLE_DDL_GLOBAL_NOT_ALLOWED_DDL_ASYNC = A rollbackable DDL execution fails, because the table has DDL replicate option.
# *Cause: A rollbackable DDL for table that has DDL replicate option is not supported.
# *Action: Retry after removing DDL replicate option on the table.

#############################################################################
#########            STORED PROCEDURE ERROR CODE                    #########
#############################################################################
337,qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG = [QSX] Internal server error (<0%s>)
# *Cause: Program Error
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

339,qpERR_ABORT_QSX_ROWTYPE_ON_PARAM_NOT_ALLOWED_ERROR_SQLTEXT = ROWTYPE is not allowed for a parameter declaration. <0%s>
# *Cause: One or more parameters was declared using ROWTYPE.
# *Action: Do not use ROWTYPE for parameter declarations.

342,qpERR_ABORT_QSX_NOT_ENOUGH_MEMORY_ARG2 = Not enough memory to execute the stored procedure or function. <0%s> ( requested size : <1%d> )
# *Cause: There was not enough memory.
# *Action: Install more memory in the system.

343,qpERR_ABORT_QSX_UNHANDLED_EXCEPTION = Unhandled exception : <0%s>
# *Cause: The exception is not handled.
# *Action: Add an exception handler that corresponds to the raised exception.

344,qpERR_ABORT_QSX_FUNCTION_WITH_NO_RETURN = Not all controls of the <0%s> function return a value.
# *Cause: Not all controls of the function return a value.
# *Action: Use a RETURN statement to return a value.

345,qpERR_ABORT_QSX_STEP_SIZE_LE_ZERO_SQLTEXT = Step size must be greater than 0 <0%s>
# *Cause: The step size is less than 1.
# *Action: Verify that the step size is greater than 0.

346,qpERR_ABORT_QSX_TOO_HIGH_CALL_DEPTH_ARG1 = The procedure or function call depth has exceeded <0%d>.
# *Cause: Procedure or function calls are nested too deeply.
# *Action: Verify that the conditions for the recursive call are valid.

347,qpERR_ABORT_QSX_COMMIT_USING_QCI = A stored procedure or function failed to commit using the QCI interface.
# *Cause:
#   - The stored procedure or function failed to commit.
#   - This error is thrown only when Altibase is linked with QCI.
# *Action
#   - Please send a bug report to the vendor.

348,qpERR_ABORT_QSX_ROLLBACK_USING_QCI = A stored procedure or function could not be rolled back using the QCI interface.
# *Cause:
#   - The stored procedure or function could not be rolled back.
#   - This error is thrown only when Altibase is linked with QCI.
# *Action
#   - Please send a bug report to the vendor.

350,qpERR_ABORT_QSX_SQLTEXT_WRAPPER = <0%s> <1%s>
# *Cause:
#   - Internal use only
# *Action
#   - Internal use only

354,qpERR_ABORT_QSX_PLAN_DROPPED = A procedure or function was dropped during execution.
# *Cause: A procedure or function has been dropped.
# *Action: Rewrite the required procedure or function.

355,qpERR_ABORT_QSX_PLAN_CHANGED = A procedure or function was changed during execution.
# *Cause: A procedure or function has been changed.
# *Action: Recompile the required procedure or function again.

356,qpERR_ABORT_QSX_PLAN_INVALID = A procedure or function was invalidated during execution.
# *Cause: A procedure or function has been invalidated.
# *Action: Recompile the required procedure or function.

357,qpERR_ABORT_QSX_CONNECTION_CLOSED = The client connection is closed.
# *Cause: The client connection is closed.
# *Action: Check the network status or check whether the customer application has been terminated.

358,qpERR_ABORT_QSX_CURSOR_ALREADY_OPEN = The CURSOR is already open.
# *Cause: The CURSOR_ALREADY_OPEN exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

359,qpERR_ABORT_QSX_DUP_VAL_ON_INDEX = Duplicate values in an index
# *Cause: The DUP_VAL_ON_INDEX exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

360,qpERR_ABORT_QSX_INVALID_CURSOR = Invalid CURSOR
# *Cause: The INVALID_CURSOR exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

361,qpERR_ABORT_QSX_INVALID_NUMBER = Invalid number
# *Cause: The INVALID_NUMBER exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

362,qpERR_ABORT_QSX_NO_DATA_FOUND = No data found.
# *Cause: The NO_DATA_FOUND exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

363,qpERR_ABORT_QSX_PROGRAM_ERROR = Program error
# *Cause: The PROGRAM_ERROR exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

364,qpERR_ABORT_QSX_STORAGE_ERROR = Storage error
# *Cause: The STORAGE_ERROR exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

365,qpERR_ABORT_QSX_TIMEOUT_ON_RESOURCE = Timeout on resource
# *Cause: The TIMEOUT_ON_RESOURCE exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

366,qpERR_ABORT_QSX_TOO_MANY_ROWS = Too many rows
# *Cause: The TOO_MANY_ROWS exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

367,qpERR_ABORT_QSX_VALUE_ERROR = Value error
# *Cause: The VALUE_ERROR exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

368,qpERR_ABORT_QSX_ZERO_DIVIDE = Divide by zero.
# *Cause: The ZERO_DIVIDE exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

369,qpERR_ABORT_QSX_NOT_ENOUGH_MEMORY_ARG_SQLTEXT = There was not enough memory to execute the stored procedure or function. (Requested Size : <0%d>) <1%s>
# *Cause: Not enough memory
# *Action: Install more memory in the system.

370,qpERR_ABORT_QSX_NOT_SUPPORTED_SQLTEXT = Unsupported statement <0%s>
# *Cause: The statement is not supported.
# *Action: Do not use unsupported statements.

388,qpERR_ABORT_QSX_STACK_OVERFLOW = Mathematic stack overflow.
# *Cause: Stack overflow
# *Action: Use the ALTER SESSION SET STACK SIZE statement to increase the stack size.

528,qpERR_ABORT_QSX_USER_DEFINED_EXCEPTION = User-Defined Exception.
# *Cause: User-defined exception raised.
# *Action: No action is necessary.

531,qpERR_ABORT_QSX_INVALID_FILEOPEN_MODE = Invalid file open mode.
# *Cause: The INVALID_MODE exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

532,qpERR_ABORT_QSX_DIRECTORY_ACCESS_DENIED = Directory access denied.
# *Cause: The ACCESS_DENIED exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

533,qpERR_ABORT_QSX_FILE_INVALID_PATH = Invalid directory path.
# *Cause: The INVALID_PATH exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

534,qpERR_ABORT_QSX_FILE_INVALID_FILEHANDLE = Invalid file handle.
# *Cause: The INVALID_FILEHANDLE exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

535,qpERR_ABORT_QSX_FILE_INVALID_OPERATION = Invalid file operation.
# *Cause: The INVALID_OPERATION exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

536,qpERR_ABORT_QSX_FILE_DELETE_FAILED = File remove operation failed.
# *Cause: The DELETE_FAILED exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

537,qpERR_ABORT_QSX_FILE_RENAME_FAILED = File rename operation failed.
# *Cause: The RENAME_FAILED exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

538,qpERR_ABORT_QSX_FILE_READ_ERROR = File read error.
# *Cause: The READ_ERROR exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

539,qpERR_ABORT_QSX_FILE_WRITE_ERROR = File write error.
# *Cause:The WRITE_ERROR exception is not handled in the stored procedure or function.
# *Action: Create an exception handler or prevent this exception from being raised.

561,qpERR_ABORT_QSX_UNSUPPORTED_ARRAY_INDEX_TYPE = Unsupported array index type.
# *Cause: In the INDEX BY clause of an array type declaration, a datatype other than INTEGER or VARCHAR was specified.
# *Action: Use one of the supported key types (INTEGER or VARCHAR) in the INDEX BY clause.

676,qpERR_ABORT_QSX_NOT_QUERY = The target of OPEN must be a query.
# *Cause: The program attempted to perform an OPEN cursor operation on a dynamic statement that was not a query.
# *Action: Ensure that the OPEN cursor operation is performed on a dynamic query statement.

902,qpERR_ABORT_QSX_PSM_INSIDE_QUERY = Cannot perform a DML, commit, or rollback inside a query.
# *Cause: The program attempted to perform a DML, commit, or rollback inside a query.
# *Action: Do not use a DML, COMMIT, or ROLLBACK statement inside a query.

968,qpERR_ABORT_QSX_NOT_EXIST_SUBPROGRAM_AND_VARIABLE = Subprogram or variable not found in the package.
# *Cause: The specified subprogram or variable does not exist in the package.
# *Action: Check the specified subprogram and variable name.

969,qpERR_ABORT_QSX_ARRAY_INDEX_OVERFLOW = The value of index array is out of range.
# *Cause: The value of the index array is out of range.
# *Action: Check the value of the index array.

970,qpERR_ABORT_QSX_FUNCTION_HAS_OUT_ARGUMENTS = A function having OUT or IN-OUT arguments cannot be invoked inside a query or DML.
# *Cause: A function having OUT or IN-OUT arguments cannot be invoked inside a query or DML.
# *Action: Check the function's argument types.

986,qpERR_ABORT_QSV_UNSUPPORTED_ARRAY_ELEMENT_TYPE = Unsupported array element type. <0%s>
# *Cause: Array type is not supported as an array element.
# *Action: Use primitive type or ROWTYPE as an array element.

1002,qpERR_ABORT_QSX_MISMATCHED_INTO_LIST_SQLTEXT = The number of columns in the target list does not match the number of variables in the INTO clause. <0%s>
# *Cause: The number of columns in the target list does not match the number of variables in the INTO clause.
# *Action: Verify that the number of columns and the number of variables are exactly the same.

1081,qpERR_ABORT_QSX_UNINITIALIZED_ARRAY = An array (element) is not initialized.
# *Cause: An array (element) is not initialized.
# *Action: Verify that a referenced array is initialized.

1111,qpERR_ABORT_QSX_INTO_OR_BULK_COLLECT_INTO_CLAUSE_NOT_ALLOWED = Into clause is not allowed here.
# *Cause: The INTO clause and BULK COLLECTION INTO clause can be used only when executing the syntax of SELECT, SELECT FOR UPDATE, and DEQUEUE.
# *Action: Select and use a query among the syntax of SELECT, SELECT FOR UPDATE or DEQUEUE; otherwise, do not use the INTO clause and BULK COLLECTION INTO clause.

1113,qpERR_ABORT_QSX_INVALID_OBJ_RECOMPILE_FAILED = The attempt to recompile the object was aborted. <0%s> <1%s>
# *Cause: The attempt to recompile the object failed due to creating an invalid statement.
# *Action: Modify the object creating statement.

1137,qpERR_ABORT_QSX_ARRAY_INDEX_OUT_OF_RANGE = Index out of range for host language array.
# *Cause: An index in the array is either less than one or greater than the maximum size of the host language array.
# *Action: Verify an index in the array or size of the host language array.

1138,qpERR_ABORT_QSX_INVALID_ARRAY_BINDING_PROTOCOL = Invalid host language array binding protocol.
# *Cause: Invalid host language array binding protocol.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

###############################################################################
#########                         QDC ERROR CODE                      #########
###############################################################################
415,qpERR_ABORT_QCC_CANNOT_EXEC_DDL = A DDL statement cannot be executed.  Check the EXEC_DDL_DISABLE property.
# *Cause: A DDL statement cannot be executed.
# *Action: Check the EXEC_DDL_DISABLE property.

790,qpERR_ABORT_QDC_TOO_LONG_PROPERTY = The length of a property name or value is too long.
# *Cause: The length of a property name or value is too long.
# *Action: Check the property name or value.

802,qpERR_ABORT_QDC_SECURITY_NOT_IN_SERVICE_PHASE = A security module can be invoked only in the SERVICE phase.
# *Cause: A security module can be invoked only in the SERVICE phase.
# *Action: Do not use the security module in phases other than the SERVICE phase.

946,qpERR_ABORT_QDC_AUDIT_ALREADY_STARTED = Audit has already started.
# *Cause: Audit has already started.
# *Action: Stop auditing and try to start again.

947,qpERR_ABORT_QDC_AUDIT_NO_AUDIT_CONDITION_EXISTS = No audit condition exists.
# *Cause: There is no audit condition.
# *Action: Check the audit conditions.

948,qpERR_ABORT_QDC_AUDIT_NO_AUDIT_CONDITION_EXISTS_ON_THE_OBJ = NO audit condition exists on <0%s>
# *Cause: There is no audit condition on the object.
# *Action: Check the audit conditions on the object.

949,qpERR_ABORT_QDC_AUDIT_NOT_STARTED = Audit is not running.
# *Cause: Auditing has not been started.
# *Action: Start auditing first.

1114,qpERR_ABORT_QDC_SNAPSHOT_NOT_IN_SERVICE_PHASE = Snapshot cannot be run since the database is in the service phase.
# *Cause: Snapshot can be running when the database is in the service phase.
# *Action: Retry the statement only with the service phase.

1170,qpERR_ABORT_QDC_CANNOT_CHANGE_AUTOCOMMIT_IN_SHARD_ENV = AUTO_COMMIT property cannot be changed to "1" in shard environment.
# *Cause: AUTO_COMMIT property cannot be changed to "1" in shard environment.
# *Action: Do not attempt to change AUTO_COMMIT property to "1" in shard environment.

1177,qpERR_ABORT_QDC_GCTX_NOT_ALLOW = Global Consistent Transaction level is only supported by Altibase sharding.
# *Cause: Global Consistent Transaction level is only supported by Altibase sharding.
# *Action: Use another Global Transaction level.


###############################################################################
#########                         QDM ERROR CODE                      #########
###############################################################################
886,qpERR_ABORT_QDM_NOT_SUPPORTED_REFRESH_OPTION = Refresh option not supported
# *Cause: The refresh option is not supported.
# *Action: Check the SQL statement.

887,qpERR_ABORT_QDM_RENAME_NOT_SUPPORT_MVIEW = Cannot rename materialized views.
# *Cause: An attempt was made to rename a materialized view.
# *Action: Check the name and type of the table to be renamed.

983,qpERR_ABORT_QDM_ACCESS_NOT_SUPPORT_MVIEW = Cannot set or change access mode of materialized views.
# *Cause: An attempt was made to set or change the access mode of a materialized view.
# *Action: Do not attempt to change the access mode of a materialized view.

1179,qpERR_ABORT_QDM_USABLE_NOT_SUPPORT_OBJECT = Cannot change the usable state of objects other than user tables.
# *Cause: An attempt was made to change the usable state of an object other than a user table.
# *Action: Do not attempt to change the usable state of an object other than a user table.

###############################################################################
#########                 QDP ERROR CODE                              #########
###############################################################################
432,qpERR_ABORT_QDP_SELF_GRANT_OR_REVOKE = A user cannot grant or revoke privileges to the same user.
# *Cause: A user cannot grant or revoke privileges to the same user.
# *Action: Connect as a user other than the grantee.

433,qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES = The user must have <0%s> privilege(s) to execute this statement.
# *Cause: The user must have sufficient privileges to execute this statement.
# *Action: Check the user's privileges.

434,qpERR_ABORT_QDP_REVOKE_PRIV_NOT_GRANTED = Unable to revoke privileges because they have not been granted (Privilege ID = <0%d>).
# *Cause: It is impossible to revoke privileges that have not been granted.
# *Action: Check the granted privileges.

435,qpERR_ABORT_QDP_DUPLICATE_GRANTEE = Duplicate grantee names : <0%s>
# *Cause: Duplicate grantee names
# *Action: Check the specified grantee name.

436,qpERR_ABORT_QDP_DUPLICATE_PRIVILEGE = Duplicate privilege names : <0%s>
# *Cause: Duplicate privilege names
# *Action: Check the specified privilege name.

437,qpERR_ABORT_QDP_WITHOUT_CASCADE_OPTION = A user refers to the table.
# *Cause: The CASCADE CONSTRAINTS option must be specified to perform this REVOKE statement.
# *Action: Specify the CASCADE CONSTRAINTS option.

438,qpERR_ABORT_QDP_NOT_ALLOWED_PRIV = <0%s> privilege is not allowed for this object.
# *Cause: This privilege is not allowed for this object.
# *Action: Check the privileges.

439,qpERR_ABORT_QDP_NOT_EXISTS_OBJECT = Object not found. The name of the specified object was not found in the database. <0%s>
# *Cause: The name of the specified object was not found in the database.
# *Action: Verify that the object exists in the database.

442,qpERR_ABORT_QMO_TERM_TOOMANY = Normalized term is too big.
# *Cause: The normalized term is too large.
# *Action: Tune the query string.

447,qpERR_ABORT_QDP_NO_GRANT_DML_PRIV_OF_META_TABLE = Unable to insert, delete, or update meta tables.
# *Cause: Meta tables cannot be inserted, deleted, or updated.
# *Action: Check the name and type of the table.

518,qpERR_ABORT_QDP_EXISTS_PRIVILEGE = <0%s> already has privileges (Privilege ID = <1%d>).
# *Cause: You attempted to grant privileges to a grantee who already has those privileges.
# *Action: Check the grantee's privileges.

559,qpERR_ABORT_QDP_NOT_EXIST_PRIVILEGE = Privilege does not exist. <0%s>
# *Cause: This privilege does not exist in the database.
# *Action: Verify that the name of the privilege is correct. Refer to the SQL User's Manual.

735,qpERR_ABORT_QDP_INVALID_DB_CHAR_SET = An invalid database character set was specified.
# *Cause: UTF16 cannot be used as the database character set.
# *Action: Ensure that the specified national character set is valid.

736,qpERR_ABORT_QDP_INVALID_NATIONAL_CHAR_SET = An invalid national character set was specified [Only UTF8 and UTF16]
# *Cause: Only UTF8 and UTF16 can be used as the national character set.
# *Action: Ensure that the specified national character set is valid.

825,qpERR_ABORT_QDP_GRANT_NOT_SUPPORT_OBJ = This object is not allowed in a GRANT statement. <0%s>
# *Cause: The GRANT statement can only be executed on user tables.
# *Action: Check the object type.

984,qpERR_ABORT_QDP_GRANT_INSUFFICIENT_PRIVILEGES = The user has insufficient privileges.
# *Cause: The user cannot grant the object privilege.
# *Action: Check the privileges of the user.

1024,qpERR_ABORT_QDP_NOT_EXISTS_ROLE = Role does not exist. <0%s>
# *Cause: The specified role does not exist.
# *Action: Check whether the role exists.
 
1025,qpERR_ABORT_QDP_NOT_SUPPORTED_ROLE_TO_PUBLIC = Cannot grant a role to PUBLIC.
# *Cause: The statement for granting a role to PUBLIC is currently unsupported.
# *Action: Do not grant a role to PUBLIC.
 
1026,qpERR_ABORT_QDP_NOT_SUPPORTED_ROLE_TO_ROLE = Cannot grant a role to another role.
# *Cause: The statement for granting a role to another role is not currently supported.
# *Action: Do not grant a role to another role.
 
1027,qpERR_ABORT_QDP_NOT_SUPPORTED_ROLE_WITH_GRANT_OPTION = Cannot GRANT to a role WITH GRANT OPTION.
# *Cause: The WITH GRANT OPTION cannot be granted to a role.
# *Action: GRANT without WITH GRANT OPTION.

1030,qpERR_ABORT_QDP_REVOKE_PRIV_NOT_GRANTED_ROLE = Unable to revoke an ungranted role. <0%s>
# *Cause: The specified role cannot be revoked as it has not been granted.
# *Action: Check the granted roles.

1031,qpERR_ABORT_QDP_EXISTS_ROLE = <0%s> already has role. <1%s>
# *Cause: A role cannot be granted to a grantee that already has that role.
# *Action: Check the grantee's role.

###############################################################################
#########                 QDV ERROR CODE                              #########
###############################################################################
444,qpERR_ABORT_QDV_ALIAS_NAME_IS_LEVEL = "LEVEL" cannot be used as an alias name. <0%s>
# *Cause: "LEVEL" cannot be used as an alias name.
# *Action: Specify another alias name.

445,qpERR_ABORT_QCV_DML_ON_READ_ONLY_VIEW = A DML statement cannot be executed on a read-only view. <0%s>
# *Cause: A DML statement cannot be executed on a read-only view.
# *Action: Verify that the view is not read-only.

446,qpERR_ABORT_QDV_INVALID_VIEW = The view is invalid. <0%s>
# *Cause: The view is invalid.
# *Action: Recompile the view.

449,qpERR_ABORT_QDV_NOT_EXISTS_VIEW = View not found
# *Cause: The name of the specified view was not found in the database.
# *Action: Verify that the view name is correct.

450,qpERR_ABORT_QDV_CIRCULAR_VIEW = Circular view definition detected <0%s>
# *Cause: A circular view definition has been detected.
# *Action: Remove the circular view definition.

556,qpERR_ABORT_QDV_DDL_ON_VIEW = A view is not appropriate here. <0%s>
# *Cause: A view is not appropriate here.
# *Action: Verify that the specified table is not a view.

663,qpERR_ABORT_QDV_ALIAS_NAME_IS_ROWNUM = "ROWNUM" cannot be used as an alias name. <0%s>
# *Cause: “ROWNUM" cannot be used as an alias name.
# *Action: Specify another alias name.

971,qpERR_ABORT_QDB_NOT_ALLOWED_FOREIGN_KEY_CONSTRAINT = A CREATE TABLE AS SELECT statement with foreign key constraints is not allowed.
# *Cause: A CREATE TABLE AS SELECT statement with foreign key constraints is not allowed.
# *Action: Remove the foreign key constraint.

###############################################################################
#########                 QDT ERROR CODE                              #########
###############################################################################
1044,qpERR_ABORT_QDT_DB_MAXSIZE_EXCEED = The total size of all disk tablespaces exceeds the maximum limit. (max size : <0%u>MB, available size : <1%u>MB)
# *Cause: The total size of all disk tablespaces exceeds the maximum limit.
# *Action: Reduce the size of the tablespace that is being altered with the CREATE TABLESPACE or ALTER TABLESPACE statement.

471,qpERR_ABORT_QDT_DUPLICATE_TBS_NAME = Duplicate tablespace names
# *Cause: Duplicate tablespace names.
# *Action: Check the specified tablespace name.

472,qpERR_ABORT_QDT_NOT_EXIST_TBS = Tablespace not found. The name of the specified tablespace was not found in the database.
# *Cause: The name of the specified tablespace was not found in the database.
# *Action: Verify that the tablespace exists in the database. Verify that there is no typo in the tablespace name.

473,qpERR_ABORT_QDT_NOT_EXIST_FILE = File not found. The name of the specified file was not found in the database.
# *Cause: The name of the specified file was not found in the database.
# *Action: Verify that the file exists in the database. Verify that there is no typo in the file name.

474,qpERR_ABORT_QDT_MISMATCH_TBS_TYPE = Mismatched tablespace type. The type of the tablespace and the type of the file are different.
# *Cause: The type of the tablespace and the type of the file are different.
# *Action: Provide a suitable type for the tablespace and file.

475,qpERR_ABORT_QDT_NO_DROP_SYSTEM_TBS = Unable to drop the SYSTEM tablespace.
# *Cause: The SYSTEM tablespace cannot be dropped.
# *Action: Check the tablespace name.

476,qpERR_ABORT_QDT_REFERENTIAL_CONSTRAINT_EXIST = A dependent foreign key constraint was found in another tablespace.
# *Cause: An object in another tablespace depends on an object in the specified tablespace via foreign key constraints.
# *Action: Drop the dependent object first, and then try to execute this statement again.

477,qpERR_ABORT_QDT_OBJECT_EXIST = The tablespace has objects.
# *Cause: The tablespace has objects.
# *Action: Check whether there are unnecessary objects in the tablespace. If so, delete them explicitly or use the DROP TABLESPACE ... INCLUDING CONTENTS statement.

478,qpERR_ABORT_QDT_NO_CREATE_IN_SYSTEM_TBS = It is forbidden to create objects in a DICTIONARY, UNDO or TEMP tablespace.
# *Cause: The user tried to create objects in a DICTIONARY, UNDO or TEMP tablespace.
# *Action: Do not create objects in a DICTIONARY, UNDO or TEMP tablespace. Create objects in other tablespaces.

479,qpERR_ABORT_QDT_NO_ACCESS_TBS = You cannot access the tablespace.
# *Cause: You cannot access the tablespace.
# *Action: Get access privileges on the specified tablespace.

480,qpERR_ABORT_QDX_MAXIMUM_KEY_SIZE_EXCEED = The estimated size of the index key exceeds the maximum limit.
# *Cause: The estimated size of the index key exceeds the maximum limit.
# *Action: Reduce the number of key columns.

481,qpERR_ABORT_QDB_INVALID_INSERT_HIGH_LIMIT = Invalid INSERT HIGH LIMIT value
# *Cause: An INSERT HIGH LIMIT value must be between 0 and 100.
# *Action: Check the INSERT HIGH LIMIT value.

482,qpERR_ABORT_QDB_INVALID_INSERT_LOW_LIMIT = Invalid INSERT LOW LIMIT value
# *Cause: An INSERT LOW LIMIT value must be between 0 and 100.
# *Action: Check the INSERT LOW LIMIT value.

483,qpERR_ABORT_QDT_ERR_INVALID_DATA_TBS = Invalid data tablespace
# *Cause: The tablespace is not a data tablespace.
# *Action: Check the tablespace name.

484,qpERR_ABORT_QDT_ERR_INVALID_TEMP_TBS = Invalid temporary tablespace
# *Cause: The tablespace is not a temporary tablespace.
# *Action: Check the tablespace name.

485,qpERR_ABORT_QDB_NO_MEMORY_OR_VOLATILE_TABLE = The table is not a memory or volatile table.
# *Cause: The table is not a memory or volatile table.
# *Action: Check the table name.

747,qpERR_ABORT_QDB_NO_DISK_TABLE = The table is not a disk table.
# *Cause: The table is not a disk table.
# *Action: Check the table name.

748,qpERR_ABORT_QDB_NO_DISK_INDEX = The index is not a disk index.
# *Cause: The index is not a disk index.
# *Action: Check the index name.

486,qpERR_ABORT_QDX_UNSUPPORTED_INDEXTYPE = Unsupported index type.
# *Cause: R-tree is not supported for disk tablespaces.
# *Action: Check the type of the index.

487,qpERR_ABORT_QDT_CANNOT_ONOFFLINE = Unable to bring the specified tablespace online or offline.
# *Cause: The specified tablespace is not a user or temp tablespace.
# *Action: Check the specified tablespace.

488,qpERR_ABORT_QDT_INVALID_NEXT_SIZE = Invalid NEXT SIZE value.
# *Cause: The NEXT SIZE value should be more than EXTEND SIZE value when AUTOEXTEND is on.
# *Action: Check the specified NEXT SIZE value.

489,qpERR_ABORT_QCV_NOT_ALLOWED_GROUPING_SET = Grouping set is not allowed.
# *Cause: A grouping set is not allowed.
# *Action: Verify that no grouping set is used.

490,qpERR_ABORT_QDT_EXIST_FILE = The specified file is already in use by another tablespace.
# *Cause: The specified file is already in use by another tablespace.
# *Action: Try again later.

492,qpERR_ABORT_QDX_INVALID_INDEX_MEDIA = The type (memory/disk/volatile) of tablespace in which to create the index is not the same as the type of the table.
# *Cause: The user tried to create an index in an invalid tablespace.
# *Action: To create an index, use a memory tablespace for a memory table, a disk tablespace for a disk table, or a volatile tablespace for a volatile table.

493,qpERR_ABORT_QDN_NOT_ALLOWED_PRIMARY_AND_UNIQUE_KEY = Unable to create a primary key or a unique key constraint for the data type (GEOMETRY,LOB). <0%s>
# *Cause: The user tried to create a primary key or a unique key constraint for the specified (GEOMETRY,LOB) data type.
# *Action: Do not create a primary key or a unique key constraint for the specified (GEOMETRY,LOB) data type.

512,qpERR_ABORT_QDT_INVALID_EXTENTSIZE = The extent size EXTENTSIZE must be greater than PAGESIZE * 2.
# *Cause: The user tried to create a tablespace with an invalid (too small) extent size.
# *Action: Create a tablespace with a valid (greater than PAGESIZE * 2) extent size.

581,qpERR_ABORT_QDT_NO_CREATE_LOB_TYPE_IN_TBS = Unable to create a LOB type column with the specified tablespace.
# *Cause: Unable to create a LOB type column with the specified tablespace.
# *Action: Check the specified tablespace.

591,qpERR_ABORT_QDT_DUPLICATE_CHECKPOINT_PATH = Duplicate checkpoint path
# *Cause: Duplicate checkpoint path
# *Action: Check the specified checkpoint path.

592,qpERR_ABORT_QDT_NO_MEM_TBS_SPLIT_FILE_SIZE = No SPLIT EACH clause.
# *Cause: There is no SPLIT EACH clause.
# *Action: Specify a SPLIT EACH clause.

593,qpERR_ABORT_QDT_INVALID_ALTER_ON_DISK_TBS = Invalid ALTER DISK TABLESPACE Statement. The tablespace to alter is not a disk tablespace.
# *Cause: The user has tried to use an ALTER DISK TBS statement that is not allowed for a non-disk tablespace.
# *Action: Verify that the tablespace type and ALTER statement match.

594,qpERR_ABORT_QDT_INVALID_ALTER_ON_MEM_TBS = Invalid ALTER MEMORY TABLESPACE Statement. The tablespace to alter is not a memory tablespace.
# *Cause: The user has tried to use an ALTER MEMORY TBS statement that is not allowed for a non-memory tablespace.
# *Action: Verify that the tablespace type and ALTER statement match.

595,qpERR_ABORT_QDT_INVALID_ALTER_ON_VOLATILE_TBS = Invalid ALTER VOLATILE TABLESPACE Statement. The tablespace to alter is not a volatile tablespace.
# *Cause: The user has tried to use an ALTER VOLATILE TBS statement that is not allowed for a non-volatile tablespace.
# *Action: Verify that the tablespace type and ALTER statement match.

596,qpERR_ABORT_QDT_INVALID_ALTER_ON_MEM_OR_VOL_TBS = Invalid ALTER TABLESPACE Statement. The tablespace to alter is neither a volatile tablespace nor a memory tablespace.
# *Cause: The user has tried to use an ALTER VOLATILE TBS statement that is not allowed for a non-volatile or non-memory tablespace.
# *Action: Verify that the tablespace type and ALTER statement match.

597,qpERR_ABORT_QDT_CANNOT_DISCARD = Unable to discard the specified tablespace.
# *Cause: The specified tablespace is not a user or temp tablespace.
# *Action: Check the specified tablespace.

598,qpERR_ABORT_QDT_CANNOT_ALTER_SYSTEM_TABLESPACE = Unable to alter the system tablespace.
# *Cause: The specified tablespace is not a user or temp tablespace.
# *Action: Check the specified tablespace.

651,qpERR_ABORT_QDT_PART_TABLE_IN_DIFFERENT_TBS = A partitioned table contains partitions in different tablespaces.
# *Cause: A partitioned table contains partitions in different tablespaces.
# *Action: Drop the partitioned table first, and then retry this statement.

652,qpERR_ABORT_QDT_PART_INDEX_IN_DIFFERENT_TBS = A partitioned index contains partitions in different tablespaces.
# *Cause: A partitioned index contains partitions in different tablespaces.
# *Action: Drop the partitioned index first, and then retry this statement.

661,qpERR_ABORT_QDT_ERR_INVALID_USER_DEFAULT_TEMP_TBS = Invalid user default temporary tablespace ( ID : <0%d> )
# *Cause: The user default temporary tablespace is invalid.
# *Action: Check the user default temporary tablespace.

672,qpERR_ABORT_QDT_DUPLICATE_TBS_ATTRIBUTE = Duplicate tablespace attribute.  <0%s>
# *Cause: The user tried to specify a duplicate attribute name in the list of tablespace attributes.
# *Action: Check the list of tablespace attributes.

681,qpERR_ABORT_QDT_UNABLE_TO_COMPRESS_VOLATILE_TBS_LOG = Log compression is not supported for volatile tablespaces.
# *Cause: The user tried to compress a volatile tablespace log.
# *Action: No action is necessary.

737,qpERR_ABORT_QDT_NON_ASCII_TBS_NAME = Invalid tablespace name character set.
# *Cause: Invalid tablespace name character set.
# *Action: Use the ASCII character set for tablespace names.

742,qpERR_ABORT_QDT_CANNOT_RENAME_SYS_TBS = The system tablespace cannot be renameed.
# *Cause: Renaming system tablespace is not allowed.
# *Action: Check if the specified tablespace is the system tablespace.

869,qpERR_ABORT_QDT_DROP_TBS_DISABLE_BECAUSE_TEMP_TABLE = Could not drop volatile tablespace
# *Cause: The volatile tablespace could not be dropped because it contains one or more temporary tables.
# *Action: Truncate all temporary tables in the volatile tablespace and try again.

1117,qpERR_ABORT_QDT_CANNOT_USE_USER_MEMORY_TABLESPACE_IN_DISK_EDITION = Memory tablespace created by a user cannot be used in the disk edition. (Tablespace : <0%s>)
# *Cause: Only disk/volatile tablespace is allowed to use in the disk edition.
# *Action: Use disk or volatile tablespace, otherwise purchase the standard/enterprise edition.

###############################################################################
#########               TRIGGER ERROR CODE                            #########
###############################################################################

494,qpERR_ABORT_TRIGGER_INVALID_REFERENCING_GRANULARITY = A REFERENCING clause can be used only in a FOR EACH ROW context.
# *Cause: A REFERENCING clause is used without a FOR EACH ROW option.
# *Action: Use the FOR EACH ROW option.

495,qpERR_ABORT_TRIGGER_UNSUPPORTED_EVENT_TIMING = Unsuported trigger event timing. Event timing using the BEFORE trigger is not supported yet. Use AFTER trigger event timing instead.
# *Cause: BEFORE trigger event timing was used.
# *Action: Do not use BEFORE trigger event timing.

497,qpERR_ABORT_TRIGGER_DUPLICATED_NAME = Duplicate trigger names <0%s>
# *Cause: The same trigger name already exists in the database.
# *Action: Use another trigger name.

498,qpERR_ABORT_TRIGGER_NOT_EXIST = Trigger not found <0%s>
# *Cause: The trigger does not exist.
# *Action: Check the trigger name.

499,qpERR_ABORT_TRIGGER_INVALID_REFERENCING = Invalid referencing keyword on DML event
# *Cause: The NEW[/OLD] referencing keyword cannot be used in a DELETE[/INSERT] statement.
# *Action: In a DELETE statement, use the OLD referencing keyword. In an INSERT statement, use the NEW referencing keyword.

501,qpERR_ABORT_TRIGGER_WHEN_SUBQUERY = A subquery is not allowed as part of a WHEN condition <0%s>.
# *Cause: The WHEN condition contains a subquery.
# *Action: Do not use a subquery as part of the WHEN condition.

502,qpERR_ABORT_TRIGGER_PSM = A trigger cannot invoke a stored procedure or function <0%s>.
# *Cause: A trigger cannot invoke a stored procedure or function.
# *Action: Do not reference a stored procedure or function in the trigger.

503,qpERR_ABORT_TRIGGER_INVALID_ACTION_STMT = The action body of the trigger cannot contain a restricted statement. <0%s>
# *Cause: The action body of the trigger contains a transaction control statement or a call to a stored procedure or function.
# *Action: Do not use a transaction control statement. Do not invoke a stored procedure or function.

504,qpERR_ABORT_TRIGGER_CYCLE_DETECTED = The action body of the trigger cannot contain a cyclic modification.
# *Cause: The action body of the trigger contains a cyclic modification.
# *Action: Remove the cyclic modification from the action body of the trigger.

505,qpERR_ABORT_TRIGGER_RECOMPILE = The attempt to recompile the trigger <0%s> was aborted because the creation statement was invalid. : <1%s> <2%s>
# *Cause: The attempt to recompile the trigger failed because the creation statement was invalid.
# *Action: Check the trigger creation statement.

506,qpERR_ABORT_TRIGGER_UNSUPPORTED = Unsupported trigger feature. <0%s>
# *Cause: Unsupported trigger feature (Ex> BEFORE Event, TABLE Referencing )
# *Action: Do not use features that are not supported for triggers.

507,qpERR_ABORT_TRIGGER_WHEN_REFERENCING = A WHEN condition requires a REFERENCING clause.
# *Cause: A WHEN condition is used without a REFERENCING clause.
# *Action: Use the WHEN condition with a REFERENCING clause.

508,qpERR_ABORT_TRIGGER_INVALID_TABLE_NAME = Invalid trigger object. <0%s>
# *Cause: The user tried to create a trigger for a non-user table, for example: a sequence, a view, etc.
# *Action: Attempt this action on a normal user table.

509,qpERR_ABORT_NO_DDL_FOR_META = No DDL (data definition language) statements can be executed on meta tables
# *Cause: DDL execution on meta tables is not allowed.
# *Action: Do not execute DDL statements on meta tables.

558,qpERR_ABORT_INVALID_TRIGGER_TYPE = Invalid trigger type
# *Cause: An invalid trigger type was used.
# *Action: Specify either INSERT, UPDATE or DELETE.

723,qpERR_REBUILD_TRIGGER_INVALID = A trigger was invalidated during execution.
# *Cause: A trigger has been invalidated.
# *Action: Recompile the trigger again.

752,qpERR_ABORT_TRIGGER_INVALID_TABLE_OWNER_NAME = The table owner's name is required when the trigger owner's name is specified. <0%s>
# *Cause: The table owner's name was not specified but the trigger owner's name was specified.
# *Action: Specify the table owner's name.

838,qpERR_ABORT_TRIGGER_UNSUPPORTED_INSTEAD_OF_EACH_STMT = The FOR EACH STATEMENT clause is not supported for use with INSTEAD OF triggers.
# *Cause: The FOR EACH STATEMENT clause was specified in an INSTEAD OF trigger creation statement.
# *Action: Do not use the FOR EACH STATEMENT clause with an INSTEAD OF trigger.

839,qpERR_ABORT_TRIGGER_UNSUPPORTED_INSTEAD_OF_WHEN_COND = WHEN conditions are not supported for use with INSTEAD OF triggers.
# *Cause: A WHEN condition was specified in an INSTEAD OF trigger creation statement.
# *Action: Do not use a WHEN condition with an INSTEAD OF trigger.

840,qpERR_ABORT_TRIGGER_UNSUPPORTED_INSTEAD_OF_COLUMN_LIST = UPDATE OF trigger events are not supported for use with INSTEAD OF triggers.
# *Cause: An UPDATE OF trigger event was specified in an INSTEAD OF trigger creation statement.
# *Action: Do not specify an UPDATE OF trigger event with an INSTEAD OF trigger.

841,qpERR_ABORT_TRIGGER_INSTEAD_OF_NOT_VIEW = INSTEAD OF triggers can only be used with views.
# *Cause: An attempt was made to define an INSTEAD OF trigger based on an object other than a view.
# *Action: Specify a view in the ON clause of an INSTEAD OF trigger.

842,qpERR_ABORT_TRIGGER_BEFORE_AFTER_VIEW = BEFORE and AFTER triggers are not supported for use with views.
# *Cause: An attempt was made to define a BEFORE or AFTER trigger based on a view.
# *Action: Specify a user table in the ON clause of a BEFORE or AFTER trigger.

############################################################################
#                         SYNONYM ERROR CODE                               #
############################################################################

519,qpERR_ABORT_NOT_FOUND_SYNONYM = The specified synonym was not found
# *Cause: The synonym does not exist.
# *Action: Check the statement.

520,qpERR_ABORT_SAME_SYNONYM = Unable to create a synonym with the same name as the object
# *Cause: The synonym and target object are the same.
# *Action: Check the statement.

793,qpERR_ABORT_CIRCULAR_SYNONYM = Circular synonym definition detected. <0%s>
# *Cause: The synonym forms a circular loop.
# *Action: Check the synonym definition.

795,qpERR_ABORT_SYNONYM_RESOLVE_OVERFLOW = Over <0%s> consecutive synonyms cannot be resolved. <1%s>
# *Cause: A large number of consecutive synonyms cannot be resolved.
# *Action: Check the statement.

###############################################################################
#########                 QSF ERROR CODE                              #########
###############################################################################

397,qpERR_ABORT_QSF_FUNCTION_NOT_ALLOWED = This function is executable only in an execute procedure/function statement.
# *Cause: The function was called in DML.
# *Action: Use this function only in an EXECUTE statement.

524,qpERR_ABORT_QSF_INVALID_FILEOPEN_MODE = Invalid file open mode.
# *Cause: An invalid value was specified for the file open mode.
# *Action: Correct the mode so that it is one of the values: 'r', 'a', or 'w'.

525,qpERR_ABORT_QSF_DIRECTORY_ACCESS_DENIED = Directory access denied.
# *Cause: A directory object was specified for which access has not been granted.
# *Action: Grant access to the directory object using the statement GRANT READ[WRITE] ON DIRECTORY [object] TO [username].

529,qpERR_ABORT_QSF_INVALID_ERROR_RANGE = Error number argument to raise_application_error of <0%d> is out of range.
# *Cause: An attempt was made to specify a number that does not fall within the allowed range.
# *Action: Use an error number in the range of 990000 to 991000, inclusive.

551,qpERR_ABORT_QSF_INVALID_IPADDRESS = The Internet address is incorrectly formed or is nonexistent.
# *Cause: The internet address is incorrectly formed or does not exist.
# *Action: Enter the internet address in its correct form and retry.

552,qpERR_ABORT_QSF_INVALID_PORT = The port number is not valid.
# *Cause: The port number is out of range.
# *Action: Enter a valid port number (a positive integer value between 1025 and 65535) and retry.

553,qpERR_ABORT_QSF_INVALID_TTL = The ttl is not valid.
# *Cause: The ttl is out of range.
# *Action: Enter a valid ttl (positive integer value between 0 and 255) and retry.

554,qpERR_ABORT_QSF_SOCKET_FAILED = SENDMSG failed due to failure opening or setting the socket.
# *Cause: The system failed to open or set the socket.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

555,qpERR_ABORT_QSF_SENDMSG_FAILED = SENDMSG failed due to an internal error while doing the actual send.
# *Cause: The system failed to execute the SENDMSG command.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

602,qpERR_ABORT_QSX_TABLE_MODIFIED = The table structure has been modified.  <0%s>
# *Cause: The table structure has been modified.
# *Action: Execute the query again.

1043,qpERR_ABORT_QSF_RECURSIVE_CALL_TO_DCEP_IS_NOT_ALLOWED = You cannot call the DBMS_CONCURRENT_EXEC package recursively.
# *Cause: A function or procedure defined in the DBMS_CONCURRENT_EXEC package calls itself again.
# *Action: Modify the procedure so that recursive calls to the DBMS_CONCURRENT_EXEC package are not made.

1146,qpERR_ABORT_QSF_NO_ROWS_DELETED = No rows deleted.
# *Cause: There is no matching data in USER_SRS_ meta table.
# *Action: Check the USER_SRS_ meta table for data that matches SRID arguement and AUTH_NAME argument.

1046,qpERR_ABORT_QCI_DISABLED_TCP = The user cannot connect using TCP.
# *Cause: This user cannot connect using TCP.
# *Action: Connect using SSL/UNIX domain/IPC. JAVA users can change the connection method in the application (for iSQL, change the value of the iSQL_CONNECTION environment variable).

###############################################################################
#########                 QUEUE ERROR CODE                            #########
###############################################################################

543,qpERR_ABORT_QDQ_QUEUE_ALTER_DENIED = Cannot alter queue table
# *Cause: A queue table cannot be altered.
# *Action: Use drop queue / create queue.

544,qpERR_ABORT_QDQ_QUEUE_DML_DENIED = Cannot execute DML statements on queue table
# *Cause: DML statements cannot be executed on queue tables.
# *Action: Use enqueue or dequeue instead.

545,qpERR_ABORT_QDQ_TABLE_ENQUEUE_DENIED = Cannot use an ENQUEUE statement on a normal table
# *Cause: An ENQUEUE statement cannot be used on a normal table.
# *Action: Do not use the ENQUEUE statement on a normal table.

546,qpERR_ABORT_QDQ_TABLE_DEQUEUE_DENIED = Cannot use a DEQUEUE statement on a normal table
# *Cause: A DEQUEUE statement cannot be used on a normal table.
# *Action: Do not use the DEQUEUE statement on a normal table.

547,qpERR_ABORT_QDQ_TABLE_DROP_QUEUE_DENIED = Cannot use a DROP QUEUE statement on a normal table
# *Cause: DROP QUEUE statements cannot be used on a normal table.
# *Action: Do not use the DROP QUEUE statement on a normal table.

548,qpERR_ABORT_QDQ_QUEUE_DROP_TABLE_DENIED = Cannot use a DROP TABLE statement on a queue table
# *Cause: DROP TABLE statements cannot be used on a queue table.
# *Action: Do not use the DROP TABLE statement on a queue table.

549,qpERR_ABORT_QDQ_QUEUE_NAME_TOO_LONG = Queue name is too long
# *Cause: A queue with a name longer than 28 characters cannot be created.
# *Action: Shorten the name of the queue.

550,qpERR_ABORT_QDQ_QUEUE_DEQUEUE_SUBQ_EXIST = Subquery exist in dequeue statement
# *Cause: Subqueries cannot be used in DEQUEUE statements.
# *Action: Do not use subqueries with DEQUEUE statements.

741,qpERR_ABORT_QDQ_NOT_FOUND_QUEUE = Queue not found
# *Cause: The queue is not in the meta database.
# *Action: Verify that the queue exists.

685,qpERR_ABORT_QDB_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE = A volatile table cannot have a geometry column.
# *Cause: Volatile tables cannot have geometry columns.
# *Action: Remove the geometry column specification.

686,qpERR_ABORT_QCP_DUPLICATE_LOGGING_MODE = Only one LOGGING or NOLOGGING clause must be specified.
# *Cause: Duplicate logging clause.
# *Action: Specify only one LOGGING or NOLOGGING clause.

687,qpERR_ABORT_QCP_INVALID_PARALLEL_DEGREE = The value for PARALLEL DEGREE must be larger than 0 and less than 65536.
# *Cause: Invalid parallel degree value.
# *Action: Set PARALLEL DEGREE to a value between 0 ~65536.

699,qpERR_ABORT_QCMLINK_NOT_INIT_REMOTE_TABLE_FUNC = The database link module did not start.
# *Cause: Failed to execute this statement because the dblink module did not start.
# *Action: Check DBLINK_ENABLE , LINKER_PORT_NO property.

724,qpERR_ABORT_QDB_NO_SUPP_LOGGING_ON_VOLATILE_TABLE = Supplemental logging is not supported for volatile tables.
# *Cause: The user tried to alter a volatile table in order to perform supplemental logging.
# *Action: Ensure that the table is not a volatile table.

725,qpERR_ABORT_QDB_NO_SUPP_LOGGING_WITHOUT_PK = Supplemental logging is not supported without a primary key.
# *Cause: The user tried to perform supplemental logging without a primary key.
# *Action: Ensure that the table has a primary key.

753,qpERR_ABORT_QDB_INVALID_PCTFREE_VALUE = Invalid PCTFREE value
# *Cause: A PCTFREE value must be between 0 and 99.
# *Action: Check the PCTFREE value.

754,qpERR_ABORT_QDB_INVALID_PCTUSED_VALUE = Invalid PCTUSED value
# *Cause: A PCTUSED value must be between 0 and 99.
# *Action: Check the PCTUSED value.

755,qpERR_ABORT_QDB_INVALID_PCTFREE_PCTUSED_VALUE = the sum of PCTUSED and PCTFREE cannot exceed 100.
# *Cause: The sum of PCTUSED and PCTFREE cannot exceed 100.
# *Action: Check the PCTFREE and PCTUSED values.

798,qpERR_ABORT_QDB_CANNOT_SET_NULL_DEFAULT_VALUE = Unable to add PRIMARY KEY or NOT NULL constraint on a column with a NULL default value.
# *Cause: Unable to add a NOT NULL or PRIMARY KEY constraint on a column with a NULL default value.
# *Action: Remove the NULL default value.

832,qpERR_ABORT_QDB_CANNOT_SET_DEFAULT_VALUE_WITH_ENCRYPT = An encrypted column with a default value other than NULL cannot be added.
# *Cause: An encrypted column with a default value other than NULL cannot be added.
# *Action: Remove the default value or set null value.

1136,qpERR_ABORT_QDQ_NOT_EMPTY_QUEUE = A queue is not empty. Only empty queue can reset a msgid.
# *Cause: Can not perform to reset a msgid because it is not a empty queue.
# *Action: Check whether the queue is empty, otherwise remove the data from the queue.

1194,qpERR_ABORT_QDQ_USE_QUEUE_TABLE_IN_DISK_TABLESPACE = Failed to create queue table in disk tablespace.
# *Cause: Cannot create queue table in disk tablespace.
# *Action: Create queue table in memory tablespace or volatile tablespace.

###############################################################################
#########                 QDSD ERROR CODE                             #########
###############################################################################

1143,qpERR_ABORT_QDSD_ALTER_DATABASE_SHARD = Only SYS user can execute SHARD DDL.
# *Cause: Only SYS user can execute SHARD DDL statement.
# *Action: Log in as the SYS user and try again.

1144,qpERR_ABORT_QDSD_INSUFFICIENT_ATTRIBUTE =To execute SHARD DDL statement, the property must be set "<0%s>".
# *Cause: SHARD DDL statement cannot be executed because of insufficient property settings.
# *Action: Change the session property value.

1145,qpERR_ABORT_QDSD_SHARD_META_NOT_CREATED = There is no shard meta SYS_SHARD.
# *Cause: The shard meta is not created.
# *Action: Create shard meta.

1147,qpERR_ABORT_QDSD_INVALID_NODE_NAME = The node name is not found. (Node Name : <0%s>).
# *Cause: Wrong node name is inputted.
# *Action: Check the node name.

1148,qpERR_ABORT_QDSD_ZKC_DEADNODE_EXIST = Unable to perform the command, because failed node exists in sharding cluster.
# *Cause: The command cannot be performed when there are failed node exists.
# *Action: Recover failed node.

1149,qpERR_ABORT_QDSD_ZKC_NOT_MY_TURN = Unable to failback, because the node is not the most recently failed node in sharding cluster.
# *Cause: Only the most recent failed node can be a target for failback.
# *Action: Recover the most recent failed node.

1150,qpERR_ABORT_QDSD_ZKC_CONNECTION_FAIL = Failed to connect Zookeeper.
# *Cause: It is unable to connect to ZooKeeper.
# *Action: Check the network connection with ZooKeeper.

1151,qpERR_ABORT_QDSD_ZKC_NOT_SUPPORT_OS = This command can be used only Linux.
# *Cause: Zookeeper C client only support Linux.
# *Action: Check OS and contact Altibase's Support Center (http://support.altibase.com).

1152,qpERR_ABORT_QDSD_INVALID_SHARD_NODE = Invalid shard data node was used.
# *Cause: The host IP and port number for shard data node cannot be found.
# *Action: Verify the host IP and port number for shard data node.

1156,qpERR_ABORT_QDSD_INVALID_LOCAL_NODE_NAME = The local node name is not valid.
# *Cause: The local node name was not inputted.
# *Action: Check the local node name and execute dbms_shard.set_local_node that is shard package procedure to input local node name.

1157,qpERR_ABORT_QDSD_INVALID_KSAFETY = The k-safety value of local node is not valid.
# *Cause: The k-safety value is wrong.
# *Action: Check the k-safety value. The value of k-safety can be 0, 1 or 2.

1158,qpERR_ABORT_QDSD_EXECUTE_REMOTE_SQL_FAILED= Failed to execute remote SQL. <0%s>
# *Cause: Failed to execute remote SQL.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

1159,qpERR_ABORT_QDSD_SYNTAX_ERROR_SHARD_DROP= Invalid Syntax : SHARD DROP statement cannot be used with node name. (Node Name : <0%s>).
# *Cause: Syntax Error. SHARD DROP statement cannot be used with node name.
# *Action: Execute SHARD DROP statement without node name on local node.

1160,qpERR_ABORT_QDSD_TOO_MANY_SOURCE_NODES_FOR_RESHARDING= There is more than one source node for resharding.
# *Cause: There must be one source node for resharding.
# *Action: Check the number of source nodes.

1161,qpERR_ABORT_QDSD_NOT_EXIST_OBJECT_NAME_FOR_RESHARDING= Object name for resharding is NULL due to unexpected internal error.
# *Cause: Object name for resharding is NULL due to unexpected internal error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

1162,qpERR_ABORT_QDSD_TOO_FEW_NODES_FOR_RESHARDING= Insufficient number of nodes for resharding.
# *Cause: Cannot reshard when the number of nodes is less than 2.
# *Action: Check the number of nodes for resharding.

1163,qpERR_ABORT_QDSD_NOT_SUPPORT_REMOVE_SHARD_TABLE_WITH_DEFAULT_NODE= Shard table with default node cannot be removed in shard object.
# *Cause: Shard table with default node cannot be removed in shard object.
# *Action: Check if the table has default node.

1164,qpERR_ABORT_QDSD_SAME_NODE_NAME= Unable to reshard, because source node name and destination node name are the same. <0%s>: <1%s>
# *Cause: Unable to reshard when source node name and destination node name are same.
# *Action: Check the source node name and destination node name.

1165,qpERR_ABORT_QDSD_TOO_MANY_REPLICA_SETS= Unable to reshard, because there are several replica sets for the same SMN.
# *Cause: The number of replica sets for the same SMN must be one for resharding.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

1166,qpERR_ABORT_QDSD_INVALID_REPLICA_SET_INFO= Replica set information is invalid.
# *Cause: Replica set information is invalid.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

1167,qpERR_ABORT_QDSD_TRANSACTION_COMMIT_ERROR= Transaction commit fails due to unexpected internal error.
# *Cause: Transaction commit fails due to unexpected internal error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

1168,qpERR_ABORT_QDSD_INVALID_PROPERTY_FOR_SHARDING= Invalid property value for sharding. <0%s>
# *Cause: Invalid property value for sharding.
# *Action: Check the property value.

1169,qpERR_ABORT_QDSD_SHARD_NOT_SAME_CLUSTER_META_DATA_SMN = Unable to execute SHARD DDL, because the cluster meta SMN(<0%lu>) is different from the data SMN(<1%lu>).
# *Cause: Unable to execute SHARD DDL, because the cluster meta SMN is different from the data SMN.
# *Action: Check the node metadata and the cluster meta information.

1171,qpERR_ABORT_QDSD_INVALID_FAILOVER_NODE_NAME = Failed to failover, because the target node name is same as local node name. (Node Name : <0%s>).
# *Cause: Unable to failover when target node name and local node name are same.
# *Action: Check the target node name.

1172,qpERR_ABORT_QDSD_SHARD_KEY_MAX_VALUE_TOO_LONG = The shard key value is too long.
# *Cause: The length of the shard key exceeds 100 characters.
# *Action: Refer to the sharding manual to verify the permissible length of the shard key value.

1173,qpERR_ABORT_QDSD_SHARD_OBJECT_NOT_FOUND = The specified shard object [<0%s>.<1%s>] is not found.
# *Cause: The table is not found in shard metadata(SYS_SHARD.OBJECTS_).
# *Action: Check if the table is a shard object or not.
 
1174,qpERR_ABORT_QDSD_ZKC_ALREADY_CONNECTED = The connection to the sharding cluster already exists.
# *Cause: The node is already connected to the ZooKeeper.
# *Action: Check the node status.
 
1175,qpERR_ABORT_QDSD_NOT_SUPP_CLONE_RESHRD = The clone shard object cannot be the target for resharding.
# *Cause: The table list includes a clone shard objects, which is not supported in resharding. 
# *Action:  Check if the table list includes a clone shard object. Retry after removing the clone shard object from the table list for resharding. 
 
1176,qpERR_ABORT_QDSD_SHARD_NOT_JOIN = The shard node has not yet been joined to the sharding cluster.
# *Cause: Status of the shard node is not joined.
# *Action: Check the shard node status and execute a shard DDL to join.

1178,qpERR_ABORT_QDSD_EXIST_NOT_FAILEDOVER_DEADNODE = Failed to run failback because there are failed nodes that has not failed over.
# *Cause: There are failed nodes that has not failed over.
# *Action: Execute failover SHARD DDL for the failed nodes before failback.

1188,qpERR_ABORT_QDSD_TOO_MANY_SESSION = The shard <0%s> DDL cannot be executed because another session is connected.
# *Cause: The shard DDL joining the cluster cannot be executed it when there is another session.
# *Action: Close another session and try again.

1189,qpERR_ABORT_QDSD_SHARD_DDL_TRY_WITH_SHARD_CLI = The shard DDL cannot be executed with a shard CLI connection.
# *Cause: The shard DDL cannot be executed with a shard CLI connection.
# *Action: Connect with ODBC or isql and execute the shard DDL again.

1190,qpERR_ABORT_QDSD_DROP_FORCE_DETECTED = Unable to fail back because one or more failed over nodes were removed by the DROP FORCE operation.
# *Cause: One or more failed over nodes were removed by the DROP FORCE operation.
# *Action: Use FAILBACK SYNC syntax instead of FAILBACK syntax.
 
1191,qpERR_ABORT_QDSD_TARGET_NODE_NOT_FAILED_OVER = DROP FORCE cannot be executed because the target node was not failed over.
# *Cause: The target node was not failed over.
# *Action: Fail over the target node before executing DROP FORCE.
 
1192,qpERR_ABORT_QDSD_DROPFORCE_NOT_MY_TURN = DROP FORCE cannot be executed because the target node is not the node most recently failed over.
# *Cause:  DROP FORCE can only be executed by the most recently failed over node.
# *Action: Fail over the target node or switch the target node.
 
1193,qpERR_ABORT_QDSD_EXIST_NOT_FAILEDOVER_DEADNODE_DROPFORCE = The target node has to be failed over to execute DROP FORCE.
# *Cause: The target node was not failed over.
# *Action: Execute ALTER DATABASE SHARD FAILOVER on the target node.


INTERNAL_TRACE_MESSAGE_BEGIN

QP_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
                "signed long =<%ld>\n unsigned long = <%lu>\n"
                "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>\n";

QP_TRC_EXEC_DDL_BEGIN   = "[EXEC_DDL_BEGIN : <%s>]";
QP_TRC_EXEC_DDL_SUCCESS = "[EXEC_DDL_END : SUCCESS]";
QP_TRC_EXEC_DDL_FAILURE = "[EXEC_DDL_END : FAILURE] ERR-<%x> : <%s>\n";

QP_TRC_EXEC_DCL_BEGIN   = "[EXEC_DCL_BEGIN : <%s>]";
QP_TRC_EXEC_DCL_SUCCESS = "[EXEC_DCL_END : SUCCESS]";
QP_TRC_EXEC_DCL_FAILURE = "[EXEC_DCL_END : FAILURE] ERR-<%x> : <%s>\n";

QP_TRC_DEBUG_MESSAGE = "[QP DEBUG] <%s>\n";

INTERNAL_TRACE_MESSAGE_END

