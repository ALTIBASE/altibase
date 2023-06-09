#!/usr/bin/env bash

# 아래의 상황을 가정한다.
#     - Target Node : Node2, Node3
#     - All Active(Standby) Node : Node1(NodeA), Node2(NodeB), Node3(NodeC)
#     - Clone 샤드 객체 : T2(Node1, NodeA, Node2, NodeB)
#     - Node2의 Node ID가 가장 작다.
#
# T2를 Node3(NodeC)에 추가한다.
#     T2(Node1, NodeA, Node2, NodeB)
#     ->
#     T2(Node1, NodeA, Node2, NodeB, Node3, NodeC)

DIR_PATH=$( cd `dirname $0` && pwd )
source ${DIR_PATH}/configure.sh

IFS_BACKUP=${IFS}

ISQL="${ALTIBASE_HOME}/bin/isql -u SYS -p ${PASSWORD} -silent"
REP_NAME_REBUILD="REP_REBUILD"
REP_NAME_ALTERNATE="REP_ALTERNATE"

# 지정한 노드가 동일한지 검사한다.
if [ ${FROM_NODE_NAME} = ${TO_NODE_NAME} ]
then
    echo "The node names are the same."
    exit 1
fi

DIR_PATH=$(cd `dirname $0` && pwd)
/usr/bin/env bash ${DIR_PATH}/clone_add_check_prepare.sh ${1}
if [ $? = 1 ]; then
    echo "${APPLICATION} can not be executed"
    exit 1
fi

# Partition Value를 구하고 Reset Shard Resident Node SQL을 구성한다.
SHARD_META_SQL=""

for (( i = 0 ; i < $SHARD_OBJECTS_COUNT ; i++ ))
do
    SHARD_META_SQL="${SHARD_META_SQL}
        EXEC DBMS_SHARD.SET_SHARD_CLONE( '${USER_NAME_ARRAY[$i]}',
                                         '${TABLE_NAME_ARRAY[$i]}',
                                         '${TO_NODE_NAME}' );"
done

echo ""
echo "Shard Meta SQL : ${SHARD_META_SQL}"

# 모든 Node의 접속 정보를 얻는다.
META_SQL_RESULT=$(
${ISQL} -s ${SHARD_META_SERVER} -port ${SHARD_META_PORT} << EOF
    SET HEADING OFF;
    EXEC utl_shard_online_rebuild.get_connection_info();
    QUIT
EOF
)

# 모든 Node에 T2의 Node로 Node3(NodeC)를 추가한다.
echo ""
echo "[Step-1. (All nodes) Set Shard Clone]"

IFS=$'\n'
RESULT_ARRAY=$( echo "${META_SQL_RESULT}" | grep -v "^\[NODE_NAME\]" | grep -v "^Execute success." )
for VALUE in ${RESULT_ARRAY[@]}
do
    NODE_NAME=$( echo ${VALUE} | gawk '{print $1}' )
    NODE_IP=$( echo ${VALUE} | gawk '{print $2}' )
    NODE_PORT=$( echo ${VALUE} | gawk '{print $3}' )
    NODE_ALTERNATE_IP=$( echo ${VALUE} | gawk '{print $4}' )
    NODE_ALTERNATE_PORT=$( echo ${VALUE} | gawk '{print $5}' )

    IFS=$' '

    echo "Node Info. : ${NODE_NAME}, ${NODE_IP}, ${NODE_PORT}"

    ${ISQL} -s ${NODE_IP} -port ${NODE_PORT} << EOF
        ALTER SESSION SET AUTOCOMMIT = FALSE;
        ${SHARD_META_SQL}
        COMMIT;
        QUIT
EOF

    if [ ${NODE_ALTERNATE_IP} != "N/A" ]
    then
        echo "Alternate Node Info. : ${NODE_NAME}, ${NODE_ALTERNATE_IP}, ${NODE_ALTERNATE_PORT}"

        ${ISQL} -s ${NODE_ALTERNATE_IP} -port ${NODE_ALTERNATE_PORT} << EOF
            ALTER SESSION SET AUTOCOMMIT = FALSE;
            ${SHARD_META_SQL}
            COMMIT;
            QUIT
EOF
    fi

    if [ ${NODE_NAME} = ${FROM_NODE_NAME} ]
    then
        FROM_NODE_IP=${NODE_IP}
        FROM_NODE_PORT=${NODE_PORT}
    fi

    if [ ${NODE_NAME} = ${TO_NODE_NAME} ]
    then
        TO_NODE_IP=${NODE_IP}
        TO_NODE_PORT=${NODE_PORT}
        if [ ${NODE_ALTERNATE_IP} != "N/A" ]
        then
            TO_NODE_ALTERNATE_IP=${NODE_ALTERNATE_IP}
            TO_NODE_ALTERNATE_PORT=${NODE_ALTERNATE_PORT}
        fi
    fi
done

IFS=${IFS_BACKUP}

ISQL_FROM_NODE="${ISQL} -s ${FROM_NODE_IP} -port ${FROM_NODE_PORT}"
ISQL_TO_NODE="${ISQL} -s ${TO_NODE_IP} -port ${TO_NODE_PORT}"
if [ ${TO_NODE_ALTERNATE_IP} ]
then
    ISQL_TO_NODE_ALTERNATE="${ISQL} -s ${TO_NODE_ALTERNATE_IP} -port ${TO_NODE_ALTERNATE_PORT}"
fi

# Node2, Node3(NodeC)에서 Replication Port No를 얻는다.
echo ""
echo "[Step-2. (Target nodes) Get replication port number]"
echo "Node Info. : ${FROM_NODE_NAME}, ${FROM_NODE_IP}, ${FROM_NODE_PORT}"

NODE_SQL_RESULT=$(
${ISQL_FROM_NODE} << EOF
    SET HEADING OFF;
    SELECT MEMORY_VALUE1 FROM X\$PROPERTY WHERE NAME = 'REPLICATION_PORT_NO';
    QUIT
EOF
)
FROM_NODE_REPL_PORT_NO=$( echo "${NODE_SQL_RESULT}" | grep -v "^1 row selected." | grep -o '[0-9]*' | gawk '{print $1}' )
echo "Replication port number : ${FROM_NODE_REPL_PORT_NO}"

echo "Node Info. : ${TO_NODE_NAME}, ${TO_NODE_IP}, ${TO_NODE_PORT}"

NODE_SQL_RESULT=$(
${ISQL_TO_NODE} << EOF
    SET HEADING OFF;
    SELECT MEMORY_VALUE1 FROM X\$PROPERTY WHERE NAME = 'REPLICATION_PORT_NO';
    QUIT
EOF
)
TO_NODE_REPL_PORT_NO=$( echo "${NODE_SQL_RESULT}" | grep -v "^1 row selected." | grep -o '[0-9]*' | gawk '{print $1}' )
echo "Replication port number : ${TO_NODE_REPL_PORT_NO}"

if [ ${TO_NODE_ALTERNATE_IP} ]
then
    echo "Alternate Node Info. : ${TO_NODE_NAME}, ${TO_NODE_ALTERNATE_IP}, ${TO_NODE_ALTERNATE_PORT}"

    NODE_SQL_RESULT=$(
    ${ISQL_TO_NODE_ALTERNATE} << EOF
        SET HEADING OFF;
        SELECT MEMORY_VALUE1 FROM X\$PROPERTY WHERE NAME = 'REPLICATION_PORT_NO';
        QUIT
EOF
)
    TO_NODE_ALTERNATE_REPL_PORT_NO=$( echo "${NODE_SQL_RESULT}" | grep -v "^1 row selected." | grep -o '[0-9]*' | gawk '{print $1}' )
    echo "Replication port number : ${TO_NODE_ALTERNATE_REPL_PORT_NO}"
fi

# Node3(NodeC)에 Replication 시스템 프라퍼티를 설정한다.
echo ""
echo "[Step-3. (Target nodes) Apply replication properties]"
echo "Node Info. : ${TO_NODE_NAME}, ${TO_NODE_IP}, ${TO_NODE_PORT}"

${ISQL_TO_NODE} << EOF
    EXEC utl_shard_online_rebuild.set_replication_properties();
    QUIT
EOF

if [ ${TO_NODE_ALTERNATE_IP} ]
then
    echo "Alternate Node Info. : ${TO_NODE_NAME}, ${TO_NODE_ALTERNATE_IP}, ${TO_NODE_ALTERNATE_PORT}"

    ${ISQL_TO_NODE_ALTERNATE} << EOF
        EXEC utl_shard_online_rebuild.set_replication_properties();
        QUIT
EOF
fi

# Node3(NodeC)에 T2의 Schema를 복제한 T2_REBUILD를 생성한다.
echo ""
echo "[Step-4. (Target nodes) Copy table schema and create/start replication for alternate]"

for (( i = 0 ; i < $SHARD_OBJECTS_COUNT ; i++ ))
do
    echo "Node Info. : ${TO_NODE_NAME}, ${TO_NODE_IP}, ${TO_NODE_PORT}"

    ${ISQL_TO_NODE} << EOF
        EXEC utl_shard_online_rebuild.copy_table_schema( '${USER_NAME_ARRAY[$i]}',
                                                         '${TABLE_NAME_ARRAY[$i]}_REBUILD',
                                                         '${TABLE_NAME_ARRAY[$i]}' );
        QUIT
EOF

    if [ ${TO_NODE_ALTERNATE_IP} ]
    then
        echo "Alternate Node Info. : ${TO_NODE_NAME}, ${TO_NODE_ALTERNATE_IP}, ${TO_NODE_ALTERNATE_PORT}"

        ${ISQL_TO_NODE_ALTERNATE} << EOF
            EXEC utl_shard_online_rebuild.copy_table_schema( '${USER_NAME_ARRAY[$i]}',
                                                             '${TABLE_NAME_ARRAY[$i]}_REBUILD',
                                                             '${TABLE_NAME_ARRAY[$i]}' );
            QUIT
EOF
    fi
done

if [ ${TO_NODE_ALTERNATE_IP} ]
then
    # (Node3 -> NodeC) Shard Data Replication을 생성하고 시작한다. (T2_REBUILD -> T2_REBUILD)
    #   Node3 Replication : FOR PROPAGATION

    TO_NODE_SQL="CREATE REPLICATION \"${REP_NAME_ALTERNATE}\" FOR PROPAGATION OPTIONS DDL_REPLICATE
                        WITH '${TO_NODE_ALTERNATE_IP}', ${TO_NODE_ALTERNATE_REPL_PORT_NO}"
    TO_NODE_ALTERNATE_SQL="CREATE REPLICATION \"${REP_NAME_ALTERNATE}\" OPTIONS DDL_REPLICATE
                                  WITH '${TO_NODE_IP}', ${TO_NODE_REPL_PORT_NO}"

    for (( i = 0 ; i < $SHARD_OBJECTS_COUNT ; i++ ))
    do
        if [ $i -ne 0 ]
        then
            TO_NODE_SQL="${TO_NODE_SQL} , "
            TO_NODE_ALTERNATE_SQL="${TO_NODE_ALTERNATE_SQL} , "
        fi

        TO_NODE_SQL="${TO_NODE_SQL}
            FROM \"${USER_NAME_ARRAY[$i]}\".\"${TABLE_NAME_ARRAY[$i]}_REBUILD\"
            TO   \"${USER_NAME_ARRAY[$i]}\".\"${TABLE_NAME_ARRAY[$i]}_REBUILD\""

        TO_NODE_ALTERNATE_SQL="${TO_NODE_ALTERNATE_SQL}
            FROM \"${USER_NAME_ARRAY[$i]}\".\"${TABLE_NAME_ARRAY[$i]}_REBUILD\"
            TO   \"${USER_NAME_ARRAY[$i]}\".\"${TABLE_NAME_ARRAY[$i]}_REBUILD\""
    done

    echo "Alternate Node Info. : ${TO_NODE_NAME}, ${TO_NODE_ALTERNATE_IP}, ${TO_NODE_ALTERNATE_PORT}"
    echo "DDL : ${TO_NODE_ALTERNATE_SQL}"

    ${ISQL_TO_NODE_ALTERNATE} << EOF
        ${TO_NODE_ALTERNATE_SQL};
        QUIT
EOF

    echo "Node Info. : ${TO_NODE_NAME}, ${TO_NODE_IP}, ${TO_NODE_PORT}"
    echo "DDL : ${TO_NODE_SQL}"

    ${ISQL_TO_NODE} << EOF
        ${TO_NODE_SQL};
        EXEC utl_shard_online_rebuild.start_replication( '${REP_NAME_ALTERNATE}' );
        QUIT
EOF
fi

# Node2의 T2를 Node3의 T2_REBUILD로 복제하기 시작한다. (Replication Sync)
#   (Node2 -> Node3) Shard Data Replication을 생성하고 시작한다. (T2 -> T2_REBUILD)
#       Node3 Replication : FOR PROPAGABLE LOGGING
#   Replication Flush
echo ""
echo "[Step-5. create/sync replication for rebuild]"

FROM_NODE_SQL="CREATE REPLICATION \"${REP_NAME_REBUILD}\"
                      WITH '${TO_NODE_IP}', ${TO_NODE_REPL_PORT_NO}"
TO_NODE_SQL="CREATE REPLICATION \"${REP_NAME_REBUILD}\" FOR PROPAGABLE LOGGING
                    WITH '${FROM_NODE_IP}', ${FROM_NODE_REPL_PORT_NO}"

for (( i = 0 ; i < $SHARD_OBJECTS_COUNT ; i++ ))
do
    if [ $i -ne 0 ]
    then
        FROM_NODE_SQL="${FROM_NODE_SQL} , "
        TO_NODE_SQL="${TO_NODE_SQL} , "
    fi

    FROM_NODE_SQL="${FROM_NODE_SQL}
        FROM \"${USER_NAME_ARRAY[$i]}\".\"${TABLE_NAME_ARRAY[$i]}\"
        TO   \"${USER_NAME_ARRAY[$i]}\".\"${TABLE_NAME_ARRAY[$i]}_REBUILD\""

    TO_NODE_SQL="${TO_NODE_SQL}
        FROM \"${USER_NAME_ARRAY[$i]}\".\"${TABLE_NAME_ARRAY[$i]}_REBUILD\"
        TO   \"${USER_NAME_ARRAY[$i]}\".\"${TABLE_NAME_ARRAY[$i]}\""
done

echo "Node Info. : ${TO_NODE_NAME}, ${TO_NODE_IP}, ${TO_NODE_PORT}"
echo "DDL : ${TO_NODE_SQL}"

${ISQL_TO_NODE} << EOF
    ${TO_NODE_SQL};
    QUIT
EOF

echo "Node Info. : ${FROM_NODE_NAME}, ${FROM_NODE_IP}, ${FROM_NODE_PORT}"
echo "DDL : ${FROM_NODE_SQL}"

${ISQL_FROM_NODE} << EOF
    ${FROM_NODE_SQL};
    EXEC utl_shard_online_rebuild.sync_replication( '${REP_NAME_REBUILD}',
                                                    1 );
    QUIT
EOF

echo "Done."
