# $Id: E_SD_US7ASCII.msg 90667 2021-04-21 08:05:29Z jake.jang $
#
# Error Message File (used by genErrMsg)
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
# 14 => sd

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.
# SD는 기존 에러 number부여 방식과 다르다. SD directory별 에러코드를
# 부여 해야한다. last number는 변경하지 말것
# Last Number = 999

SECTION = 14

###############################################################################
#########                 SDM ERROR CODE (1~)                         #########
###############################################################################

1,sdERR_ABORT_SDM_SHARD_META_NOT_CREATED = There is no shard meta <0%s>.
# *Cause: The shard meta has not been created.
# *Action: Create shard meta.

2,sdERR_ABORT_SDM_SHARD_NODE_OVERFLOW = There is overflow in the number of shard data nodes.
# *Cause: The shard data nodes are exceeded.
# *Action: Refer to the manual to verify the number of permissible shard data nodes.

3,sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST = The shard data node cannot be found.
# *Cause: The specified shard data node does not exist.
# *Action: Verify if the name of shard data node is correct.

4,sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST = The shard object cannot be found.
# *Cause: The shard object does not exist.
# *Action: Verify if the shard object is correct.

5,sdERR_ABORT_SDM_SHARD_KEY_COLUMN_NOT_EXIST = The shard key <0%s>.<1%s>.<2%s> cannot be found.
# *Cause: The specified shard key does not exist.
# *Action: Verify if the shard key is correct.

6,sdERR_ABORT_SDM_UNSUPPORTED_SHARD_KEY_COLUMN_TYPE = The data type of shard key <0%s>.<1%s>.<2%s> is not supported.
# *Cause: The data type is not supported by the shard key.
# *Action: Verify if the data type used in the shard key is correct.

7,sdERR_ABORT_SDM_INVALID_RANGE_FUNCTION = The split method in the shard key does not correspond.
# *Cause: The split method in the shard key does not correspond.
# *Action: Verify if the split method is correct.

8,sdERR_ABORT_SDM_AREADY_EXIST_SHARD_OBJECT = The object already exists.
# *Cause: The object is a pre-existing shard object.
# *Action: Verify the object name.

9,sdERR_ABORT_SDM_SYSTEM_OBJECT = A shard object cannot be created with a meta object.
# *Cause: A shard object cannot be created with a meta object.
# *Action: Verify if the object privilege is correct.

10,sdERR_ABORT_SDM_CHECK_META_VERSION = Confirmation of shard version failed.
# *Cause: The shard version cannot be confirmed.
# *Action: Verify the shard version with the altibase-v command to see if the version is correct.

11,sdERR_ABORT_SDM_MISMATCH_META_VERSION = The shard version between meta node and data node is mismatched.
# *Cause: The shard version between meta and data nodes does not correspond.
# *Action: Verify the shard version with the altibase-v command to see if the version is correct.

13,sdERR_ABORT_SDM_DUPLICATED_RANGE_VALUE = The range value of shard key is duplicated.
# *Cause: The range value of shard key is duplicated.
# *Action: Verify if the range value of shard key is correct.

14,sdERR_ABORT_SDM_INVALID_SHARD_NODE_INFO = Invalid information of shard meta node
# *Cause: Table SYS_SHARD.LOCAL_META_INFO_ has no record or more than one record.
# *Action: Execute function DBMS_SHARD.RESET_META_NODE_ID to correct the table SYS_SHARD.LOCAL_META_INFO_.

15,sdERR_ABORT_SDM_SHARD_RANGE_OVERFLOW = There is overflow in the number of shard ranges.
# *Cause: The shard ranges are exceeded.
# *Action: Refer to the manual to verify the number of permissible shard ranges.

16,sdERR_ABORT_SDM_EXIST_REFERENCES_NODE = There is an object that references a node.
# *Cause: There is an object that references a node.
# *Action: Verify the shard meta information.

17,sdERR_ABORT_SDM_DO_NOT_MATCH_SPLIT_METHOD = The partition split method of the partitioned table does not match with the shard split method.
# *Cause: For shard table, the partition split method of the partitioned table should be same with the shard split method.
# *Action: Check if the shard split method matches the partition split method of the partitioned table.

18,sdERR_ABORT_SDM_PARTITION_KEY_COUNT = The shard key is not automatically created on the table, because there are partition keys more than one.
# *Cause: The table should have only one partition key for shard table.
# *Action: Check the number of partition key columns and change the table to have one partition key.

19,sdERR_ABORT_SDM_UNSUPPORTED_SHARD_TABLE_TYPE = Unable to set shard table, becuase the table is non-partitioned table.
# *Cause: It is not supported for non-partitioned table to set shard table.
# *Action: Check if the table is partitioned table.

20,sdERR_ABORT_SDM_PARTITION_KEY_COND_COUNT = The shard key is not automatically created, because there are muliple partition condition values of the partition <0%s>.
# *Cause: The partitioned table with partitions including mulitple partition condition values is not supported in shard table.
# *Action: Check if the partitioned table have partitions including mulitple partition condition values.

21,sdERR_ABORT_SDM_SHARD_PARTITION_NOT_EXIST = The partition <0%s> of the table <1%s> cannot be found.
# *Cause: The specified partition does not exist.
# *Action: Correct the partition name.

22,sdERR_ABORT_SDM_SHARD_PARTITION_VALUE_NOT_EXIST = The partition value <0%s> of the table <1%s> cannot be found.
# *Cause: The specified partition value does not exist in the shard table.
# *Action: Check the partition value of the partition table.

23,sdERR_ABORT_SDM_NOT_EXISTS_PRIMARY_KEY = PRIMARY KEY constraint not found in the shard table.
# *Cause: A shard table should have a primary key constraint.
# *Action: Add a primary key constraint to the shard table and retry the operation.
 
24,sdERR_ABORT_SDM_USE_SHARD_TABLE_IN_VIEW = A view cannot be registered in this statement. :<0%s>
# *Cause: A view cannot be registered as a shard table.
# *Action: Change to table name instead of the view and retry the operation.

25,sdERR_ABORT_SDM_TABLE_PARTITION = The specified partition is not found. <0%s>
# *Cause: The specified partition does not exist.
# *Action: Change the partition name and retry the operation.
 
26,sdERR_ABORT_SDM_DUPLICATED_TABLE_PARTITION = The specified partition name is duplicated. <0%s>
# *Cause: The specified partition name is duplicated.
# *Action: Verify if the partition name is not duplicated.
 
27,sdERR_ABORT_SDM_MISSMATCHED_PARTITION_COUNT = The specified partition_node_list are not matched with real table partitions.
# *Cause: The specified partition_node_list are not matched with real table partitions.
# *Action: Check the partition information and retry the operation.

28,sdERR_ABORT_SDM_UNSUPPORTED_SHARD_PROCEDURE_PARAMETER_TYPE = Unbindable data types are included in parameters of the shard procedure. <0%s>.<1%s>.<2%s>
# *Cause: To be registered as a shard procedure, a procedure should use a parameter with primitive data type.
# *Action: Check the data type of the parameter in the stored procedure.

###############################################################################
#########                 SDA ERROR CODE (101~)                       #########
###############################################################################

101,sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD = The statement is not supported in Altibase sharding due to the following reason. : <0%s><1%s>
# *Cause: Altibase sharding does not support the statement.
# *Action: Verify if the statement is correct.

102,sdERR_ABORT_SDA_INVALID_SHARD_KEY_CONDITION = Invalid shard key value expression was used.
# *Cause: The expression of shard key value is invalid.
# *Action: Verify if the shard key expression is correct.

103,sdERR_ABORT_SDA_NOT_EXIST_SHARD_KEY_CONDITION = The shard key value cannot be found.
# *Cause: The shard key value does not exist.
# *Action: Verify if the shard key exists.

104,sdERR_ABORT_SDA_DATA_NODE_NOT_FOUND = The data node corresponding to the shard key cannot be found.
# *Cause: The data node that matches the shard key cannot be found due to inappropriate setting of shard tables.
# *Action: Verify the distribution setting or shard key value.

###############################################################################
#########                 SDF ERROR CODE (201~)                       #########
###############################################################################

201,sdERR_ABORT_SDF_INVALID_SHARD_NODE = Invalid shard data node was used.
# *Cause: The host IP and port number for shard data node cannot be found.
# *Action: Verify the host IP and port number for shard data node.

202,sdERR_ABORT_SDF_AREADY_EXIST_SHARD_NODE = The shard data node of identical IP and port already exists.
# *Cause: The shard data node of identical IP and port already exists.
# *Action: Verify the IP and port of the shard data node.

203,sdERR_ABORT_SDF_SHARD_USER_NAME_TOO_LONG = The object user name is too long.
# *Cause: The user name of a shard object is permitted to contain less than 128 characters.
# *Action: Verify the length of the object user name.

204,sdERR_ABORT_SDF_SHARD_TABLE_NAME_TOO_LONG = The object name is too long.
# *Cause: The shard object name is permitted to contain less than 128 characters.
#Action : Verify the length of the object name.

205,sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG = The name of shard data node is too long.
# *Cause: The name of shard data node is permitted to contain less than 40 characters.
# *Action: Verify the length of the shard data node name.

206,sdERR_ABORT_SDF_SHARD_MAX_VALUE_TOO_LONG = The maximum value for shard split method is too large.
# *Cause: The maximum value for shard split method is too large.
# *Action: Refer to the manual to verify permissible range for the shard split method.

207,sdERR_ABORT_SDF_SHARD_KEYCOLUMN_NAME_TOO_LONG = The shard key name is too long.
# *Cause: The shard key name is permitted to contain less than 40 characters.
# *Action: Verify the length of the shard key name.

208,sdERR_ABORT_SDF_INVALID_SHARD_SPLIT_METHOD_NAME = The shard split method is invalid.
# *Cause: Invalid shard split method is used.
# *Action: Refer to the manual verify if the shard split method is correct.

209,sdERR_ABORT_SDF_INVALID_SHARD_TABLE = The specified object cannot be found.
# *Cause: The specified object does not exist.
# *Action: Retry after verifying the object and user name.

210,sdERR_ABORT_SDF_INVALID_RANGE_VALUE = The permissible range of shard key <0%s> is invalid.
# *Cause: The permissible range for the shard key is invalid.
# *Action: Refer to the manual to verify the permissible range for the shard key.

211,sdERR_ABORT_SDF_INVALID_SUB_SHARD_KEY_NAME = Invalid sub-shard key name.
# *Cause: Sub-shard key name and shard key name are the same.
# *Action: Verify if the name of sub-shard key is correct.

212,sdERR_ABORT_SDF_UNSUPPORTED_SUB_SHARD_KEY_SPLIT_TYPE = The split method of sub-shard key is not supported.
# *Cause: Unsupported sub-shard key split method.
# *Action: Verify that the split method of the sub-shard key.

213,sdERR_ABORT_SDF_UNSUPPORTED_SHARD_SPLIT_METHOD_NAME = The shard split method is not supported.
# *Cause: The shard split method is not supported.
# *Action: Verify that the split method name.

214,sdERR_ABORT_SDF_UNSUPPORTED_META_CONNTYPE = The internal(meta) connection type is not supported. : <0%d>
# *Cause: Supported internal connection types are 1, 8.
# *Action: Verify the internal connection type.

215,sdERR_ABORT_SDF_CANNOT_DELETE_CURRENT_SMN = The shard meta data as the current SMN can not be deleted.
# *Cause: Only the shard meta data with the old shard meta number can be deleted.
# *Action: Check the current shard meta number of SYS_SHARD.GLOBAL_META_INFO_.

216,sdERR_ABORT_SDF_INVALID_META_CHANGE = Invalid shard meta change information
# *Cause: The shard meta information that you need is not exist. 
# *Action: Check the the shard meta information of the object.

217,sdERR_ABORT_SDF_SHARD_REP_ARG_WRONG = The <0%s> argument for shard replication is not applicable.
# *Cause: The argument is not applicable to the function.
# *Action: Verify that the argument falls within the valid range.

218,sdERR_ABORT_SDF_SHARD_LOCAL_META_ERROR = The local_meta_info_ of shard meta has problem.
# *Cause: An unexpected error related to shard meta has occurred.
# *Action: Check shard meta and the error number in the trace log file and contact Altibase Support Center (http://support.Altibase.com).

219,sdERR_ABORT_SDF_CANNOT_EXECUTE_IN_AUTOCOMMIT_MODE = Unable to modify shard meta in auto-commit mode.
# *Cause: A modification of shard meta is allowed only in non-autocommit mode.
# *Action: Change system to non-autocommit mode and try agian.

220,sdERR_ABORT_SDF_INVALID_GTX_LEVEL = Unable to modify shard meta in non global transaction mode.
# *Cause: A modification of shard meta is allowed only in global transaction level.
# *Action: Change the global transaction level and try again.

221,sdERR_ABORT_SDF_INVALID_TABLE_TYPE = Unable to set shard table, because the table <0%s> is non-partitioned table and k-safety is not 0.
# *Cause: A non-partitioned table can be set shard table only when k-safety is 0.
# *Action: Check if the table is a partitioned table.

222,sdERR_ABORT_SDF_SHARD_PARTITION_NAME_TOO_LONG = The partition name is too long.
# *Cause: The partition name of shard table is permitted to contain less than 128 characters.
# *Action: Check the length of the partition name.

223,sdERR_ABORT_SDF_INVALID_COMPOSITE_TABLE = The shard table with composite shard key is not supported when k-safety is not 0.
# *Cause: The shard table with composite shard key can be used only when k-safety is 0.
# *Action: Check the shard configuration information.           

224,sdERR_ABORT_SDF_INVALID_META_CHANGE_ARG = Invalid shard meta change information [ <0%s>, <1%s>, <2%s>, <3%s>, <4%s>, <5%s>, <6%s>, <7%s> ]
# *Cause: The shard meta information that you need is not exist. 
# *Action: Check the the shard meta information of the object.

225,sdERR_ABORT_SDF_RECORD_EXIST = The record already exists.
# *Cause: The table should not have any data in your specified option.
# *Action: Remove existing data or specify a option that allows existing data.
 
226,sdERR_ABORT_SDF_INVALID_OPTION = Invalid option : <0%s>
# *Cause: A function option was missing or invalid.
# *Action: Check the function options.
 
227,sdERR_ABORT_SDF_DIFFERENT_SCHEMA = The shard object schema is different with other shard nodes. 
# *Cause: A shard object should have same schema in all shard nodes.
# *Action: Check the shard object schema with a existing shard node's matching shard object schema.
 
228,sdERR_ABORT_SDF_TOO_LONG_REPL_NAME = Replication name is too long.
# *Cause: A replication name is permitted to contain less than 35 characters.
# *Action: Check the length of replication name.
 
229,sdERR_ABORT_SDF_CHECK_RECORD_EXIST = The irregular record exists : node <0%s>.
# *Cause: A shard table in a shard node cannot have any data not complying with the shard table's data distribution specification.
# *Action: Remove not complying data or specify a option that allows not complying data.

230,sdERR_ABORT_SDF_INVALID_ARGUMENT = Invalid argument : <0%s>
# *Cause: A function argument was missing or invalid.
# *Action: Check the function argument.

###############################################################################
#########                 SDL ERROR CODE (301~)                       #########
###############################################################################

301,sdERR_ABORT_SHARD_LIBRARY_ERROR = An error occurred in the library function call when executing <1%s> for shard data node <0%s>.
# *Cause: There is an error in library function call of shard data node.
# *Action: Verify the state of shard data node.

302,sdERR_ABORT_SHARD_LIBRARY_ERROR_1 = The following error occurs when <1%s> of shard data node <0%s> is performed.: <2%s>
# *Cause: An error occurred during the library function call of the shard data node.
# *Action: Verify the state of shard data node.

303,sdERR_ABORT_SHARD_LIBRARY_ERROR_2 = The following error occurs when <1%s> of shard data node <0%s> is performed.: <2%s><3%s>
# *Cause: An error occurred during the library function call of the shard data node.
# *Action: Verify the state of shard data node.

304,sdERR_ABORT_SHARD_LIBRARY_ERROR_3 = The following error occurs when <1%s> of shard data node <0%s> is performed.: <2%s><3%s><4%s>
# *Cause: An error occurred during the library function call of the shard data node.
# *Action: Verify the state of shard data node.

305,sdERR_ABORT_SHARD_LIBRARY_ERROR_4 = The following error occurs when <1%s> of shard data node <0%s> is performed.: <2%s><3%s><4%s><5%s>
# *Cause: An error occurred during the library function call of the shard data node.
# *Action: Verify the state of shard data node.

306,sdERR_ABORT_SHARD_LIBRARY_LINK_FAILURE_ERROR = The link failed when performing <1%s> on shard data node <0%s>.: <2%s>
# *Cause: The link on the shard data node failed.
# *Action: Verify the state of link on the shard data node.

307,sdERR_ABORT_INIT_SDL_ODBCCLI = The library initialization failed and the following error occurred: <0%s>
# *Cause: Library initialization failed.
# *Action: Verify the library of shard meta node.

308,sdERR_ABORT_EXECUTE_NULL_DBC = The connection cannot be found when shard data node <0%s> is <1%s>.
# *Cause: The connection state of shard data node does not exist.
# *Action: Verify the connection state.

309,sdERR_ABORT_EXECUTE_NULL_STMT = The statement cannot be found when shard data node <0%s> is <1%s>.
# *Cause: The statement in the shard data node does not exist.
# *Action: Verify the statement in the shard data node.

310,sdERR_ABORT_UNINITIALIZED_LIBRARY = Shard data node <0%s> fails to perform <1%s> because the library was not initialized.
# *Cause: The library of shard meta node was not initialized.
# *Action: Restart the server after verifying the library of shard meta node.

311,sdERR_ABORT_DBCLINK_ALLOC = <1%s> on shard data node <0%s> failed.
# *Cause: There is insufficient memory for the shard data node connection.
# *Action: Verify the memory usage.

312,sdERR_ABORT_SHARD_XA_LIBRARY_ERROR = An error occurred in the library function call when executing <1%s> for shard <0%s>.
# *Cause: There is an error in library function call of shard xa function
# *Action: Verify the state of shard library.

313,sdERR_ABORT_SHARD_LIBRARY_FAILOVER_SUCCESS = The <1%s> of server-side failover success.: <0%s> <2%s>
# *Cause: Session failover success
# *Action: Re-execute application logic.

314,sdERR_ABORT_INTERNAL_ALTERNATE_NODE_SETTING_IS_MISSING = Alternate shard node <0%s> information is missing from external or internal network settings.
# *Cause: Alternate shard node information is missing from external or internal network settings.
# *Action: Verify the alternate host IP and port number of the shard node.

315,sdERR_ABORT_SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE = Failover is not available.
# *Cause: Failed to connect to shard library or coordinates.
# *Action: Check the shard node status or network.

316,sdERR_ABORT_EXECUTE_NULL_SD_STMT = The shard statement cannot be found when performing <1%s> to shard data node <0%s>.
# *Cause: The statement in the shard coordinator does not exist.
# *Action: Verify the statement in the shard coordinator.

317,sdERR_ABORT_REMOTE_COMMIT_FAILED = Failed to commit a remote transaction when performing <1%s> on remote node <0%s>.: <2%s>
# *Cause: A network problem occurred.
# *Action: Verify the state of link on the remote node. Execute rollback and try again.

318,sdERR_ABORT_PREPARE_DID_NOT_BEGIN_TX = Failed to commit a transaction, because the transaction did not begin.
# *Cause: The transaction is not begin.
# *Action: Verify the state of link on the remote node. Execute rollback and try again.

319,sdERR_ABORT_SHARD_MULTIPLE_ERRORS = Multiple errors occurred on each of shard nodes.
# *Cause: Multiple errors occurred on each of shard nodes.
# *Action: Check the error messages on each shard node.

###############################################################################
#########                 SDP ERROR CODE (401~)                       #########
###############################################################################

401,sdERR_ABORT_SDPJ_SYNTAX = JSON syntax error
# *Cause: The unusable reserved words or delimiter inapplicable was used. Or the reserved word cannot be used.
# *Action: Refer to JSON format.

402,sdERR_ABORT_SDPJ_ALLOC = JSON parsering failed at <0%d>% due to insufficient memory buffer.
# *Cause: There is insufficient memory buffer in JSON parser.
# *Action: Verify the memory buffer size.

403,sdERR_ABORT_SDPJ_CONVERT = Fail to convert the condition to shard analyze information. (<0%s>)
# *Cause: The condition does not contain shard information or is incorrect.
# *Action: Verify the condition.

###############################################################################
#########                 SDI ERROR CODE (801~)                       #########
###############################################################################

801,sdERR_ABORT_SDI_SHARD_LINKER_NOT_INITIALIZED = The meta connection cannot be initialized.
# *Cause: The meta connection was not initialized.
# *Action: Verify the setting of shard meta and data is correct.

802,sdERR_ABORT_SDI_INCOMPLETE_RANGE_SET = The shard key range of <0%s>.<1%s> is invalid.
# *Cause: The shard key range is invalid.
# *Action: Verify the key range for shard split method.

803,sdERR_ABORT_SDI_NOT_EXIST_SHARD_ANALYSIS = The result of shard analysis does not exist.
# *Cause: The result of shard analysis does not exist.
# *Action: Verify the distribution setting or shard key value.

804,sdERR_ABORT_SDI_DATA_NODE_NOT_FOUND = The data node corresponding to the shard key cannot be found.
# *Cause: The data node that matches the shard key cannot be found due to inappropriate setting of shard tables.
# *Action: Verify the distribution setting or shard key value.

805,sdERR_ABORT_SDI_DUPLICATED_NODE_NAME = Duplicate node name <0%s>
# *Cause: Duplicate node name
# *Action: Verify that no duplicate node names are specified.

806,sdERR_ABORT_SDI_INVALID_NODE_NAME = Invalid node name <0%s>
# *Cause: Invalid node name
# *Action: Verify that the node name is valid.

807,sdERR_ABORT_SDI_INVALID_NODE_NAME2 = Invalid node name: <0%s>
# *Cause: Invalid node name
# *Action: Verify that the node name is valid.

808,sdERR_ABORT_SDI_SHARD_META_PROPAGATION_TIMEOUT = Shard meta update propagation timeout.
# *Cause: The changes of the shard meta information are not arrived in time limit.
# *Action: Check shard meta number and information.

809, sdERR_ABORT_SDI_ZKC_CONNECTION_INFO_MISSING = Unable to connect ZooKeeper, because wrong configurations exist in zoo.cfg.
# *Cause: Wrong configurations exist in zoo.cfg.
# *Action: Check the configuration in zoo.cfg.

810, sdERR_ABORT_SDI_ZKC_ZOOCFG_NO_EXIST = The file zoo.cfg is not found.
# *Cause: zoo.cfg file is missing.
# *Action: Copy zoo.cfg file from other server.

811, sdERR_ABORT_SDI_ZKC_CONNECTION_FAIL = Failed to connect ZooKeeper.
# *Cause: It is unable to connect to ZooKeeper.
# *Action: Check the network connection with ZooKeeper.

812, sdERR_FATAL_SDI_ZKC_CONNECTION_MISSING = Disconnected from ZooKeeper.
# *Cause: Connection with ZooKeeper has been lost.
# *Action: Check the connection with ZooKeeper.

813, sdERR_ABORT_SDI_ZKC_NODE_EXISTS = The node name already exists.
# *Cause: The node name is duplicated.
# *Action: Retry with the new node name.

814, sdERR_ABORT_SDI_ZKC_NO_NODE = The node name is not found.
# *Cause: Invalid node name entered.
# *Action: Check the entered node name.

815, sdERR_ABORT_SDI_ZKC_WAITING_TIMEOUT = ZooKeeper lock timeout occurred.
# *Cause: It fails to lock ZooKeeper due to timeout.
# *Action: Check the ZooKeeper meta information.

816, sdERR_FATAL_SDI_ZKC_LOCK_ERROR = There is a problem with ZooKeeper lock.
# *Cause: ZooKeeper lock is Corrupted.
# *Action: Check ZooKeeper lock metadata information.

817, sdERR_ABORT_SDI_ZKC_VALIDATE_FAIL = Unable to join sharding cluster because the node metadata is different from ZooKeeper metadata [<0%s>].
# *Cause: The node metadata is not same with Zookeper meta information.
# *Action: Check the node metadata information.

818, sdERR_ABORT_SDI_ZKC_STATE_VALIDATE_FAIL = Unable to execute the command, because it cannot alter the node state.
# *Cause: It is unable to alter the node state, because it tried prohibited change.
# *Action: Check the command and node state.

819, sdERR_ABORT_SDI_ZKC_TOO_MUCH_SERVER_INFO = There is several server information in zoo.cfg more than 7.
# *Cause: It can set a maximum of 7 servers in zoo.cfg.
# *Action: Check zoo.cfg file and server infomation in it.

820, sdERR_ABORT_SDI_ZKC_DB_VALIDATE_FAIL = Unable to create database, because node metadata is different from ZooKeeper metadata.
# *Cause: The node metadata is not same with Zookeper meta information.
# *Action: Check the node metadata.

821, sdERR_ABORT_SDI_INVALID_KSAFETY = The k-safety of local node is not valid.
# *Cause: The k-safety value is wrong.
# *Action: Check the k-safety and execute dbms_shard.set_replication that is shard package procedure to set k-safety.

822, sdERR_ABORT_SDI_ZKC_ETC_ERROR = There is a problem the zookeeper server [error code: <0%d>].
# *Cause: The zookeeper server has an unkown problem.
# *Action: Check the zookeeper server.

823,sdERR_ABORT_SDI_SHARD_NOT_SAME_DATA_SESSION_SMN = The data smn and the session smn are different.
# *Cause: The data smn and the session smn are not the same.
# *Action: Check the node metadata information.

824,sdERR_ABORT_SDI_SHARD_NOT_JOIN= The shard node has not yet been joined to the cluster.
# *Cause: Status of the shard node is not joined.
# *Action: Check the shard node status and execute shard ddl to join.

825,sdERR_ABORT_SDI_INVALID_TRANSACTION= The transaction is invalid.
# *Cause: The transaction has not been begun.
# *Action: Check stste of the transaction.  

826,sdERR_ABORT_SDI_NOT_EXIST_RANGE_VALUE= Range value does not exist.
# *Cause: Range value does not exist.
# *Action: Check range value exists. 

827,sdERR_ABORT_SDI_INVALID_SMN= Session SMN is invalid.
# *Cause: Value of Session SMN is 0. 
# *Action: Check meta connection.

828,sdERR_ABORT_SDI_NOT_EXIST_SHARD_KEY_COLUMN= Shard key column does not exist. 
# *Cause: Shard key column does not exist. 
# *Action: Check column value (KEY_COLUMN_NAME) in SYS_SHARD.OBJECTS_ table. 

829,sdERR_ABORT_NOT_EXIST_STMT= Statement does not exist.
# *Cause: Statement does not exist.
# *Action: Verify the error number in the trace log file and contact Altibase Support Center (http://support.Altibase.com).

830,sdERR_ABORT_NOT_EXIST_SD_STMT= Shard statement does not exist.
# *Cause: Shard statement does not exist.
# *Action: Verify the error number in the trace log file and contact Altibase Support Center (http://support.Altibase.com).

831,sdERR_ABORT_NOT_EXIST_PROPERTY= The property does not exist.
# *Cause: The property does not exist.
# *Action: Verify the property.

832,sdERR_ABORT_NOT_EXIST_PARTITION_NAME= Partition name does not exist.
# *Cause: Partition name does not exist.
# *Action: Verify the error number in the trace log file and contact Altibase Support Center (http://support.Altibase.com).

833,sdERR_ABORT_NOT_EXIST_REPLICA_SET= Replica set does not exist.
# *Cause: Replica set does not exist.
# *Action: Check SYS_SHARD.REPLICA_SETS_ table. 

834,sdERR_ABORT_NOT_RUNNING_REPLICATION= Replication <0%s> is not running.
# *Cause: Replication for Backup is not running.
# *Action: Check replication state.

835,sdERR_ABORT_TOO_MANY_SOURCE_NODE_FOR_RESHARDING= There are too many source nodes.
# *Cause: There are too many source nodes.
# *Action: Verify count of source node is one.  

836,sdERR_ABORT_ZKC_DEADNODE_EXIST = Unable to perform the command, because dead node exists in sharding cluster.
# *Cause: The command cannot be performed when there are dead node exists.
# *Action: Failback or drop dead node.

837,sdERR_ABORT_ZKC_FAILOVER_HISTORY_TOO_LONG = Failed to Failover, because there are too many failed nodes.
# *Cause: Failed to failover, because failover history is full due to too many failed nodes in sharding cluster system.
# *Action: Try to recover the failed nodes.

838,sdERR_ABORT_EXECUTE_REMOTE_SQL_FAILED= Failed to execute remote SQL. <0%s>
# *Cause: Failed to execute remote SQL.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

839,sdERR_ABORT_SDI_NOT_EXIST_PROC_SQLTEXT = Procedure or function not found : <0%s>.
# *Cause: The specified procedure or function name was not found in the database.
# *Action: Verify that the procedure or function exists.

840,sdERR_ABORT_SDI_DATA_NODE_NOT_FOUND_BY_VALUE = The data node corresponding to the shard key value [<0%s>] cannot be found.
# *Cause: The data node that matches the shard key cannot be found due to inappropriate setting of shard tables.
# *Action: Verify the distribution setting or shard key value.

841,sdERR_ABORT_SDI_DATA_NODE_NOT_FOUND_BY_PART = The data node corresponding to the partition [<0%s>] cannot be found.
# *Cause: The data node that matches the shard key cannot be found due to inappropriate setting of shard tables.
# *Action: Verify the distribution setting or shard key value.

842,sdERR_ABORT_SDI_PART_NAME_ERROR = The solo table [<0%s>] cannot use a partition name [<1%s>] in a shard operation.
# *Cause: Unable to use partition name in a solo shard object.
# *Action: Verify the shard distribution setting.
 
843,sdERR_ABORT_SDI_SHARD_OBJECT_NOT_EXIST = The shard object cannot be found [<0%s>]. 
# *Cause: The shard object does not exist.
# *Action: Verify if the shard object name is correct.
 
844,sdERR_ABORT_SDI_SHARD_KEY_ERROR = Unable to use shard key with a solo shard object or a clone shard object.
# *Cause: Unable to use a shard key with a solo shard object or a clone shard object.
# *Action: Verify the shard distribution setting.

845, sdERR_ABORT_FAILED_TO_PROPAGATE_SHARD_META_NUMBER = Failed to propagate shard meta number.
# *Cause: Failed to propagate shard meta number.
# *Action: Check the shard node status or network.

846, sdERR_ABORT_SESSION_SMN_REVERSED = Fail to perform shard rebuild because the current and last shard meta numbers are reversed.
# *Cause: The current and last shard meta numbers of the session are reversed.
# *Action: Reconnect the client application and check the shard configuration information.

847,sdERR_ABORT_SHARD_META_OUT_OF_DATE = Shard meta information of session is out of date.
# *Cause: Shard meta information has been changed.
# *Action: Check the shard configuration information or shard node status.

848,sdERR_ABORT_SDI_SHARD_OBJECT_CANNOT_EXEC_DDL = A DDL statement cannot be executed on a shard object(table/index/procedure).
# *Cause: A DDL statement was tried to be executed on shard object(table/index/procedure).
# *Action: Don't use a DDL statement on a shard object(table/index/procedure).

849,sdERR_ABORT_SDI_SHARD_REPL_CANNOT_EXEC_DCLDDL = A DCL/DDL statement cannot be executed on a system management replication for sharding.
# *Cause: A DCL/DDL statement was tried to be executed on a system management replication for sharding.
# *Action: Don't use a DCL/DDL statement on a system managed replication for sharding.

850,sdERR_ABORT_SDI_ALREADY_IN_CLUSTER = Executing SET_LOCAL_NODE procedure is not allowed for the already added node.
# *Cause: The command cannot be performed on a node that is already in the cluster.
# *Action: Drop the node and try again.

###############################################################################
#########                 SDC ERROR CODE (901~)                       #########
###############################################################################

901,sdERR_ABORT_EXIST_SHARD_TABLE_OUTSIDE_SHARD_VIEW = The shard table is only available within the shard view.: <0%s>
# *Cause: The shard table is only available within the shard view.
# *Action: Rewrite the shard query.

902,sdERR_ABORT_INVALID_SHARD_QUERY = <0%s> <1%s>
# *Cause: The shard query is invalid.
# *Action: Rewrite the shard query.

903,sdERR_ABORT_UNSUPPORTED_SHARD_DATA_IN_DML = The shard keyword is not supported in DML statements
# *Cause: The shard query is invalid.
# *Action: Rewrite the shard query.

904,sdERR_ABORT_SHARD_REBUILD_ERROR = Shard rebuild error
# *Cause: The changes of the shard meta information are not applicable on online.
# *Action: Re-connect the client program.

905,sdERR_ABORT_SDC_INSUFFICIENT_ATTRIBUTE = To execute SHARD DDL statement, the property must be set "<0%s>".
# *Cause: SHARD DDL statement cannot be executed because of insufficient property settings.
# *Action: Change the session property value.

999,sdERR_ABORT_SDC_UNEXPECTED_ERROR = Unexpected errors have occurred.: <0%s>: <1%s>
# *Cause: An unexpected error has occurred.
# *Action: Verify the error number in the trace log file and contact Altibase Support Center (http://support.Altibase.com).

INTERNAL_TRACE_MESSAGE_BEGIN

INTERNAL_TRACE_MESSAGE_END

