#!/bin/sh

#################################################
# Set Limit Values
#################################################

SYS_PASSWD=MANAGER
V_MEM_LIMIT=15728640000                 # 15G (in bytes) Virtual Memory Size Limit
REQUIRED_FREESPACE_ALTIBASE_HOME=100    # Specify by Mbyte Unit
REQUIRED_FREESPACE_DB=1000              # Specify by Mbyte Unit
REQUIRED_FREESPACE_LOG=1000             # Specify by Mbyte Unit
INTERVAL=300                            # in sec
#SS_INTERVAL=12                         # 300 * 12 = 1 hour
#SS_CNT=1
MAX_WAIT_TIME=30                        # Wait for this limit(second) before kill Background Process
MAX_REPL_GAP=1024000                    # 10M * 100 (in kb)
MAX_GC_GAP=102400000                    # 100M 
MAX_LOG_FILE_CNT=100                      
MAX_ARCH_LOG_CNT=100                      
MAX_TRANS_TIME=300                      # in sec
HIGH_ALARM_LIMIT=80                     # (%)
LOW_ALARM_LIMIT=50                      # (%)

# get the performance view in ther future
MEM_PAGE_SIZE=32768  # 32KB
DISK_PAGE_SIZE=8192  # 8KB

#################################################
# Macro Define
#################################################

### altimon.sh home
ALTIMON_HOME=$HOME/ALTIMON

### Temp directory
TMP_DIR=$HOME/ALTIMON/alti_tmp

### QUERY directory
MON_QUERY_DIR=$ALTIMON_HOME/QUERY

### Backup directory
BACKUP_DIR=$ALTIMON_HOME/BACKUP

### Main log file
#LOG_FILE=$ALTIMON_HOME/altimon_`date '+%Y%m%d'`.log
LOG_FILE=$ALTIMON_HOME/altimon.log

### Error log file
ERROR_LOG=$ALTIMON_HOME/altimon_error.log
ALARM_LOG=$ALTIMON_HOME/altimon_alarm.log

WHOAMI=`whoami`
SYSTEM=`uname`
AWK="awk"
REG_GREP=grep
EX=ex

if [ $SYSTEM = "HP-UX" ]
then
    UNIX95=1; export UNIX95
elif [ $SYSTEM = "SunOS" ]
then
    AWK="/usr/xpg4/bin/awk"
    REG_GREP=/usr/xpg4/bin/grep

    if [ -f /usr/ucb/ex ]
    then
        EX=/usr/ucb/ex
    fi
fi

#################################################
# Creating Query 
#################################################

###---------------------------
### COMMON
###---------------------------

## cm_1.sql 
## DATABASE NAME 
echo " \
SELECT \
  SYSDATE || \
  ',CM,CM-1,DB_NAME,'|| \
  RTRIM(DB_NAME) \
FROM V\$DATABASE \
;" > $MON_QUERY_DIR/cm_1.sql

## cm_2.sql 
## ALTIBASE PACKAGE VERSION 
echo " \
select \
  SYSDATE || \
  ',CM,CM-2,PRODUCT_VERSION,'|| \
  rtrim(PRODUCT_VERSION) \
from v\$version;" > $MON_QUERY_DIR/cm_2.sql

## cm_3.sql 
## protocol version 
echo " \
select \
  SYSDATE|| \
  ',CM,CM-3,PROTOCOL_VERSION,'|| \
  rtrim(PROTOCOL_VERSION) \
from v\$version;" > $MON_QUERY_DIR/cm_3.sql

## cm_4.sql 
## META version 
echo " \
select \
  SYSDATE|| \
  ',CM,CM-4,META_VERSION,'|| \
  rtrim(META_VERSION) \
from v\$version;" > $MON_QUERY_DIR/cm_4.sql

## cm_5.sql 
## replication protocol version 
echo " \
select \
  SYSDATE || \
  ',CM,CM-5,REPL_PROTOCOL_VERSION,'|| \
  rtrim(REPL_PROTOCOL_VERSION) \
from v\$version;" > $MON_QUERY_DIR/cm_5.sql

## cm_6.sql 
## memory database MAX SIZE 
echo " \
select \
  SYSDATE || \
  ',CM,CM-6,MEM_MAX_DB_SIZE(Bytes),'|| \
  rtrim(to_char(MEM_MAX_DB_SIZE)) \
from v\$database;" > $MON_QUERY_DIR/cm_6.sql

## cm_7.sql 
## altibase startup time 
echo " \
select \
  SYSDATE || \
  ',CM,CM-7,STARTUP_TIME,'|| \
  to_char(to_date('1970010109','YYYYMMDDHH') + STARTUP_TIME_SEC/60/60/24 , 'YYYY/MM/DD HH:MI:SS') \
from v\$INSTANCE;" > $MON_QUERY_DIR/cm_7.sql

## cm_8.sql 
## altibase working time 
echo " \
select \
  SYSDATE || \
  ',CM,CM-8,WORKING_TIME(DAY),'|| \
  round(WORKING_TIME_SEC/60/60/24,2) \
from v\$INSTANCE;" > $MON_QUERY_DIR/cm_8.sql

## cm_9.sql 
## max client count 
echo " \
SELECT \
  SYSDATE || \
  ',CM,CM-9,MAX_SESSION,'|| \
  TRIM(VALUE1) \
FROM V\$PROPERTY  \
WHERE NAME LIKE '%MAX_CLIENT%';" > $MON_QUERY_DIR/cm_9.sql

###---------------------------
### replication
###---------------------------

## rp_1.sql 
## replication startup mode 
echo " \
select \
  SYSDATE || \
  ',RP,RP-1,'|| \
  rtrim(REPLICATION_NAME)||'(START_FLAG),'|| \
  case2(IS_STARTED = 0, '0', \
    decode(START_FLAG, \
        0, 'Normal', \
        1, 'Quick', \
        2, 'Sync', \
        3, 'Sync_only')) \
from v\$repsender A right outer join system_.sys_replications_ B \
ON trim(A.rep_name) = trim(B.REPLICATION_NAME) \
;" > $MON_QUERY_DIR/rp_1.sql

## rp_2.sql 
## replication startup or not 
echo " \
select \
  SYSDATE || \
  ',RP,RP-2,'|| \
  rtrim(REPLICATION_NAME)||','|| \
  case2(IS_STARTED = 0, '0', \
      decode(NET_ERROR_FLAG, \
        0,'Connected', \
        1,'Disconnected')) \
from v\$repsender A right outer join system_.sys_replications_ B \
ON trim(A.rep_name) = trim(B.REPLICATION_NAME) \
;" > $MON_QUERY_DIR/rp_2.sql

## rp_3.sql 
## replication status 
echo " \
select \
  SYSDATE || \
  ',RP,RP-3,'|| \
  rtrim(REPLICATION_NAME)||'(STATUS),'|| \
  case2(IS_STARTED = 0, '0', \
    decode(STATUS , \
        0,'STOP', \
        1,'RUN', \
        2,'RETRY')) \
from v\$repsender A right outer join system_.sys_replications_ B \
ON trim(A.rep_name) = trim(B.REPLICATION_NAME) \
;" > $MON_QUERY_DIR/rp_3.sql

## rp_51.sql 
## tr_offset
echo " \
select \
  SYSDATE || \
  ',RP,RP-51,'|| \
  rtrim(REPLICATION_NAME)||'(TR_OFFSET(Bytes)),'|| \
  case2(IS_STARTED = 0, '0', \
      round(tr_offset, 2)) \
from \
(   select \
        rep_name name, \
        max(rep_last_fileno) * $LOG_FILE_SIZE + rep_last_offset tr_offset, \
        min(rep_fileno) * $LOG_FILE_SIZE + rep_offset rep_offset \
    from v\$repgap gap \
    group by rep_name,rep_last_offset,rep_offset \
)A right outer join system_.sys_replications_ B \
ON trim(A.name) = trim(B.REPLICATION_NAME) \
;" > $MON_QUERY_DIR/rp_51.sql

## rp_52.sql 
## rep_offset 
echo " \
select \
  SYSDATE || \
  ',RP,RP-52,'|| \
  rtrim(REPLICATION_NAME)||'(REP_OFFSET(Bytes)),'|| \
  case2(IS_STARTED = 0, '0', \
      round(rep_offset,2) ) \
from \
(   select \
        rep_name name, \
        max(rep_last_fileno) * $LOG_FILE_SIZE + rep_last_offset tr_offset, \
        min(rep_fileno) * $LOG_FILE_SIZE + rep_offset rep_offset \
    from v\$repgap gap \
    group by rep_name,rep_last_offset,rep_offset \
)A right outer join system_.sys_replications_ B \
ON trim(A.name) = trim(B.REPLICATION_NAME) \
;" > $MON_QUERY_DIR/rp_52.sql

## rp_53.sql        
## replication gap
echo " \
select \
    SYSDATE || \
    ',RP,RP-53,'|| \
    rtrim(REPLICATION_NAME)||'(REP_GAP(Bytes)),'|| \
    case2(IS_STARTED = 0, '0', rep_gap) \
from \
    (   \
    select \
        rep_name name, \
        rep_gap \
    from v\$repgap gap \
    group by rep_name,rep_gap \
    )A right outer join system_.sys_replications_ B \
    ON trim(A.name) = trim(B.REPLICATION_NAME) \
;" > $MON_QUERY_DIR/rp_53.sql

###---------------------------
### session
###---------------------------

## ss_1.sql 
## session count
echo " \
SELECT \
  SYSDATE || \
  ',SS,SS-1,SESSION_COUNT,'|| \
  COUNT(*) \
FROM \
v\$SESSION;" > $MON_QUERY_DIR/ss_1.sql

## ss_2.sql 
## statement count
echo " \
SELECT \
  SYSDATE || \
  ',SS,SS-2,STMT_COUNT,'|| \
  COUNT(*) \
FROM \
V\$STATEMENT;" > $MON_QUERY_DIR/ss_2.sql

## ss_3.sql 
## idle start time of session 
echo " \
SELECT \
  SYSDATE || \
  ',SS,SS-3,ID:'|| \
  ID ||',IDLE_START_TIME:' || \
  CASE2(IDLE_START_TIME = 0, '0', TO_CHAR(TO_DATE('1970010109','YYYYMMDDHH') +  \
IDLE_START_TIME/60/60/24 , 'YYYY/MM/DD HH:MI:SS')) \
FROM \
V\$SESSION;" > $MON_QUERY_DIR/ss_3.sql

###---------------------------
### table
###---------------------------

## tb_1.sql 
## alloc size for memory tables
echo " \
SELECT \
  SYSDATE || \
  ',TB,TB-1,'|| \
        RTRIM(B.TABLE_NAME)||',ALLOC_SIZE(Bytes):'|| \
  ROUND(A.MEM_PAGE_CNT*$MEM_PAGE_SIZE, 2)+ROUND(A.MEM_VAR_PAGE_CNT*$MEM_PAGE_SIZE, 2) \
FROM V\$MEMTBL_INFO A, SYSTEM_.SYS_TABLES_ B \
WHERE A.TABLE_OID = B.TABLE_OID \
  AND B.TABLE_TYPE = 'T' \
  AND B.TABLE_NAME NOT LIKE 'SYS_%' \
;" > $MON_QUERY_DIR/tb_1.sql

## tb_2.sql 
## used size of memory tables  
echo " \
SELECT \
  SYSDATE || \
  ',TB,TB-2,'|| \
  RTRIM(B.TABLE_NAME)||',USED_SIZE(Bytes):'|| \
  ROUND((VAR_USED_MEM + FIXED_USED_MEM) , 2) \
FROM V\$MEMTBL_INFO A, SYSTEM_.SYS_TABLES_ B \
WHERE A.TABLE_OID = B.TABLE_OID \
  AND B.TABLE_TYPE = 'T' \
  AND B.TABLE_NAME NOT LIKE 'SYS_%' \
;" > $MON_QUERY_DIR/tb_2.sql

## tb_3.sql 
## memory efficiency(vs max_size) of memory tables
echo " \
SELECT \
  SYSDATE || \
  ',TB,TB-3,'|| \
  RTRIM(B.TABLE_NAME)||',EFFICIENCY(MAXSIZE:%):' || \
    ROUND(((VAR_USED_MEM + FIXED_USED_MEM)/1024/1024)/(rtrim(c.MEM_MAX_DB_SIZE)/1024/1024), 4) * 100  \
FROM V\$MEMTBL_INFO A, SYSTEM_.SYS_TABLES_ B, v\$database C \
WHERE A.TABLE_OID = B.TABLE_OID \
  AND B.TABLE_TYPE = 'T' \
  AND B.TABLE_NAME NOT LIKE 'SYS_%' \
;" > $MON_QUERY_DIR/tb_3.sql

## tb_4.sql 
## memory efficiency(vs alloc_size) of memory tables
echo " \
SELECT \
  SYSDATE || \
  ',TB,TB-4,'|| \
  RTRIM(B.TABLE_NAME)||',EFFICIENCY(%):' || \
    ROUND(((VAR_USED_MEM + FIXED_USED_MEM)/1024/1024)*100/((VAR_ALLOC_MEM + FIXED_ALLOC_MEM)/1024/1024), 4) \
FROM V\$MEMTBL_INFO A, SYSTEM_.SYS_TABLES_ B, v\$database C \
WHERE A.TABLE_OID = B.TABLE_OID \
  AND B.TABLE_TYPE = 'T' \
  AND B.TABLE_NAME NOT LIKE 'SYS_%' \
;" > $MON_QUERY_DIR/tb_4.sql

## tb_11.sql 
## disk tables info
echo " \
SELECT \
  SYSDATE || \
  ',TB,TB-11,'|| \
  RTRIM(B.TABLE_NAME)||'('||RTRIM(C.NAME)||'),ALLOC_SIZE:'|| \
  A.DISK_TOTAL_PAGE_CNT * $DISK_PAGE_SIZE \
FROM V\$DISKTBL_INFO A, SYSTEM_.SYS_TABLES_ B, V\$TABLESPACES C \
WHERE A.TABLE_OID = B.TABLE_OID \
AND B.TABLE_NAME NOT LIKE 'SYS_%' \
AND A.TABLESPACE_ID = C.ID \
;" > $MON_QUERY_DIR/tb_11.sql

## tb_21.sql 
## lock count
echo " \
SELECT \
  SYSDATE || \
  ',TB,TB-21,'|| \
  RTRIM(B.TABLE_NAME)||',LOCK_CNT:'|| \
  COUNT(*) \
FROM V\$LOCK A, SYSTEM_.SYS_TABLES_ B \
WHERE A.TABLE_OID = B.TABLE_OID \
AND B.TABLE_NAME NOT LIKE 'SYS_%' \
GROUP BY TABLE_NAME;" > $MON_QUERY_DIR/tb_21.sql

###---------------------------
### tablespaces
###---------------------------

## ts_0.sql 
## max tablespace size per tablespace   
echo " \
SELECT \
    SYSDATE || \
    ',TS,TS-0,'|| \
    rtrim(NAME)||',MAX_SIZE(Bytes):'|| \
    MAXSIZE \
FROM \
( \
    SELECT \
        DF.SPACEID SPACEID, \
        MAXSIZE, \
        CURRSIZE, \
        NVL(UF.USEDSIZE,'N/A') USEDSIZE \
    FROM \
        (SELECT \
            SUM(MAXSIZE) * $DISK_PAGE_SIZE MAXSIZE , \
            SUM(CURRSIZE) * $DISK_PAGE_SIZE CURRSIZE , \
            SPACEID \
        FROM \
            X\$DATAFILES \
        GROUP BY SPACEID) DF , \
        (SELECT \
            A.ID SPACEID, \
            to_char(B.USED_PAGE_CNT * $DISK_PAGE_SIZE) AS USEDSIZE \
        FROM X\$TABLESPACES A, X\$TABLESPACES_HEADER B \
        WHERE   A.ID=B.ID \
        AND A.ID != 0 ) UF \
    where DF.SPACEID = UF.SPACEID \
    UNION ALL \
    SELECT \
        0 SPACEID, \
        MEM_MAX_DB_SIZE MAXSIZE, \
        MEM_ALLOC_PAGE_COUNT * $MEM_PAGE_SIZE CURRSIZE, \
        TO_CHAR((MEM_ALLOC_PAGE_COUNT - MEM_FREE_PAGE_COUNT) * $MEM_PAGE_SIZE) USEDSIZE \
    FROM \
        (SELECT MEM_ALLOC_PAGE_COUNT, MEM_FREE_PAGE_COUNT FROM V\$DATABASE), \
        (SELECT VALUE1 MEM_MAX_DB_SIZE FROM X\$PROPERTY WHERE NAME = 'MEM_MAX_DB_SIZE') \
) TBS_SZ \
left outer join X\$TABLESPACES TBS_INFO \
ON TBS_SZ.SPACEID = TBS_INFO.ID \
ORDER BY \
TBS_SZ.SPACEID;" > $MON_QUERY_DIR/ts_0.sql

## ts_1.sql 
## alloc size per tablespace   
echo " \
SELECT \
    SYSDATE || \
    ',TS,TS-1,'|| \
    rtrim(NAME)||',ALLOC_SIZE(Bytes):'|| \
    ALLOCSIZE \
FROM \
    ( \
    SELECT \
        DF.SPACEID SPACEID, \
        MAXSIZE, \
        ALLOCSIZE, \
        NVL(UF.USEDSIZE,'N/A') USEDSIZE \
    FROM \
        (SELECT \
            SUM(MAXSIZE) * $DISK_PAGE_SIZE MAXSIZE , \
            SUM(CURRSIZE) * $DISK_PAGE_SIZE ALLOCSIZE , \
            SPACEID \
        FROM \
            X\$DATAFILES \
        GROUP BY SPACEID) DF , \
        (SELECT \
            A.ID SPACEID, \
            to_char(B.USED_PAGE_CNT * $DISK_PAGE_SIZE) AS USEDSIZE \
        FROM X\$TABLESPACES A, X\$TABLESPACES_HEADER B \
        WHERE   A.ID=B.ID \
            AND A.ID != 0 ) UF \
    where DF.SPACEID = UF.SPACEID \
    UNION ALL \
    SELECT \
        0 SPACEID, \
        MEM_MAX_DB_SIZE MAXSIZE, \
        MEM_ALLOC_PAGE_COUNT * $MEM_PAGE_SIZE ALLOCSIZE, \
        TO_CHAR((MEM_ALLOC_PAGE_COUNT - MEM_FREE_PAGE_COUNT) * $MEM_PAGE_SIZE) USEDSIZE \
        FROM \
            (SELECT MEM_ALLOC_PAGE_COUNT, MEM_FREE_PAGE_COUNT FROM V\$DATABASE), \
            (SELECT VALUE1 MEM_MAX_DB_SIZE FROM X\$PROPERTY WHERE NAME = 'MEM_MAX_DB_SIZE') \
) TBS_SZ \
left outer join X\$TABLESPACES TBS_INFO \
ON TBS_SZ.SPACEID = TBS_INFO.ID \
ORDER BY \
TBS_SZ.SPACEID;" > $MON_QUERY_DIR/ts_1.sql

## ts_2.sql 
## used size per tablespace   
echo " \
SELECT \
    SYSDATE || \
    ',TS,TS-2,'|| \
    rtrim(NAME)||',USED_SIZE(Bytes):'|| \
    USEDSIZE \
FROM \
    ( \
    SELECT \
        DF.SPACEID SPACEID, \
        MAXSIZE, \
        CURRSIZE, \
        NVL(UF.USEDSIZE,'N/A') USEDSIZE \
    FROM \
        (SELECT \
            SUM(MAXSIZE) * $DISK_PAGE_SIZE MAXSIZE , \
            SUM(CURRSIZE) * $DISK_PAGE_SIZE CURRSIZE , \
            SPACEID \
        FROM \
            X\$DATAFILES \
        GROUP BY SPACEID) DF , \
        (SELECT \
            A.ID SPACEID, \
            to_char(B.USED_PAGE_CNT * $DISK_PAGE_SIZE) AS USEDSIZE \
        FROM X\$TABLESPACES A, X\$TABLESPACES_HEADER B \
        WHERE   A.ID=B.ID \
            AND A.ID != 0 ) UF \
    where DF.SPACEID = UF.SPACEID \
    UNION ALL \
    SELECT \
        0 SPACEID, \
        MEM_MAX_DB_SIZE MAXSIZE, \
        MEM_ALLOC_PAGE_COUNT * $MEM_PAGE_SIZE CURRSIZE, \
        TO_CHAR((MEM_ALLOC_PAGE_COUNT - MEM_FREE_PAGE_COUNT) * $MEM_PAGE_SIZE) USEDSIZE \
        FROM \
            (SELECT MEM_ALLOC_PAGE_COUNT, MEM_FREE_PAGE_COUNT FROM V\$DATABASE), \
            (SELECT VALUE1 MEM_MAX_DB_SIZE FROM X\$PROPERTY WHERE NAME = 'MEM_MAX_DB_SIZE') \
) TBS_SZ \
left outer join X\$TABLESPACES TBS_INFO \
ON TBS_SZ.SPACEID = TBS_INFO.ID \
ORDER BY \
TBS_SZ.SPACEID;" > $MON_QUERY_DIR/ts_2.sql

## ts_3.sql 
## tablespace efficienty  
echo " \
SELECT \
    SYSDATE || \
    ',TS,TS-3,'|| \
    rtrim(NAME)||',EFFICIENCY(%):'|| \
    case2( ROUND(USEDSIZE/MAXSIZE,4) * 100 > 100, \
    '99.99', \
    ROUND(USEDSIZE/MAXSIZE,4) * 100) \
FROM \
( \
    SELECT \
        DF.SPACEID SPACEID, \
        MAXSIZE, \
        CURRSIZE, \
        NVL(UF.USEDSIZE,'N/A') USEDSIZE \
    FROM \
        (SELECT \
            SUM(MAXSIZE) * $DISK_PAGE_SIZE MAXSIZE , \
            SUM(CURRSIZE) * $DISK_PAGE_SIZE CURRSIZE , \
            SPACEID \
        FROM \
            X\$DATAFILES \
        GROUP BY SPACEID) DF , \
        (SELECT \
            A.ID SPACEID, \
            to_char(B.USED_PAGE_CNT * $DISK_PAGE_SIZE) AS USEDSIZE \
        FROM X\$TABLESPACES A, X\$TABLESPACES_HEADER B \
        WHERE   A.ID=B.ID \
            AND A.ID != 0 ) UF \
    where DF.SPACEID = UF.SPACEID \
    UNION ALL \
    SELECT \
        0 SPACEID, \
        MEM_MAX_DB_SIZE MAXSIZE, \
        MEM_ALLOC_PAGE_COUNT * $MEM_PAGE_SIZE CURRSIZE, \
        TO_CHAR((MEM_ALLOC_PAGE_COUNT - MEM_FREE_PAGE_COUNT) * $MEM_PAGE_SIZE) USEDSIZE \
    FROM \
        (SELECT MEM_ALLOC_PAGE_COUNT, MEM_FREE_PAGE_COUNT FROM V\$DATABASE), \
        (SELECT VALUE1 MEM_MAX_DB_SIZE FROM X\$PROPERTY WHERE NAME = 'MEM_MAX_DB_SIZE') \
) TBS_SZ \
left outer join X\$TABLESPACES TBS_INFO \
ON TBS_SZ.SPACEID = TBS_INFO.ID \
ORDER BY TBS_SZ.SPACEID;" > $MON_QUERY_DIR/ts_3.sql

## ts_4.sql 
## datafile count per tablespace  
echo " \
SELECT \
    SYSDATE || \
    ',TS,TS-4,'|| \
    RTRIM(NAME)||',FILE_CNT:'|| \
    MEM_DBFILE_COUNT_0 \
FROM V\$DATABASE a ,X\$TABLESPACES b \
WHERE b.ID = 0 \
union all \
SELECT \
    SYSDATE || \
    ',TS,TS-4,'|| \
    RTRIM(NAME)||',FILE_CNT:'|| \
    DATAFILE_COUNT \
FROM \
( \
    SELECT \
        DF.SPACEID SPACEID, \
        MAXSIZE, \
        CURRSIZE, \
        NVL(UF.USEDSIZE,'N/A') USEDSIZE \
    FROM \
        (SELECT \
            SUM(MAXSIZE) * $DISK_PAGE_SIZE MAXSIZE , \
            SUM(CURRSIZE) * $DISK_PAGE_SIZE CURRSIZE , \
            SPACEID \
        FROM \
            X\$DATAFILES \
        GROUP BY SPACEID) DF , \
        (SELECT \
            A.ID SPACEID, \
            to_char(B.USED_PAGE_CNT * $DISK_PAGE_SIZE) AS USEDSIZE \
        FROM  \
            X\$TABLESPACES A, X\$TABLESPACES_HEADER B \
        WHERE   A.ID=B.ID \
            AND A.ID != 0 ) UF \
    WHERE DF.SPACEID = UF.SPACEID \
) TBS_SZ \
left outer join X\$TABLESPACES TBS_INFO \
ON TBS_SZ.SPACEID = TBS_INFO.ID;" > $MON_QUERY_DIR/ts_4.sql

## ts_5.sql 
## tablespace status 
echo " \
SELECT \
    SYSDATE || \
    ',TS,TS-5,'|| \
    rtrim(NAME)||','|| \
    DECODE(STATE, \
            0, 'OFFLINE', \
            1, 'ONLINE', \
            2, 'IN BACKUP PROCESSING', \
            'UNKNOWN STATUS' \
    ) \
FROM \
( \
    SELECT \
        DF.SPACEID SPACEID, \
        MAXSIZE, \
        CURRSIZE, \
        NVL(UF.USEDSIZE,'N/A') USEDSIZE \
    FROM \
        (SELECT \
            SUM(MAXSIZE) * $DISK_PAGE_SIZE MAXSIZE , \
            SUM(CURRSIZE) * $DISK_PAGE_SIZE CURRSIZE , \
            SPACEID \
        FROM \
            X\$DATAFILES \
        WHERE   STATE<>1 \
		GROUP BY SPACEID) DF , \
        (SELECT \
            A.ID SPACEID, \
            to_char(B.USED_PAGE_CNT * $DISK_PAGE_SIZE) AS USEDSIZE \
        FROM X\$TABLESPACES A, X\$TABLESPACES_HEADER B \
        WHERE   A.ID=B.ID \
            AND A.ID != 0 ) UF \
    where DF.SPACEID = UF.SPACEID \
) TBS_SZ \
left outer join X\$TABLESPACES TBS_INFO \
ON TBS_SZ.SPACEID = TBS_INFO.ID \
ORDER BY TBS_SZ.SPACEID;" > $MON_QUERY_DIR/ts_5.sql

###---------------------------
### view
###---------------------------

## vi_1.sql 
## VIEW status 
echo " \
SELECT \
  SYSDATE || \
  ',VI,VI-1,'|| \
  RTRIM(TABLE_NAME)||','|| \
  DECODE(STATUS, \
        0,'VALID', \
        1,'INVALID') \
FROM \
SYSTEM_.SYS_TABLES_ T, SYSTEM_.SYS_VIEWS_ V, SYSTEM_.SYS_USERS_ U \
WHERE U.USER_ID = V.USER_ID \
AND T.TABLE_ID = V.VIEW_ID;" > $MON_QUERY_DIR/vi_1.sql
 
###---------------------------
### memory
###---------------------------

## am_1.sql 
## main modual size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-1,MainModual(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT \
WHERE NAME LIKE 'Main%';" > $MON_QUERY_DIR/am_1.sql
  
## am_2.sql 
## query modual size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-2,QueryModual(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT \
WHERE NAME LIKE 'Query%';" > $MON_QUERY_DIR/am_2.sql
  
## am_3.sql 
## Math modual size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-3,Mathematics(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT \
WHERE NAME = 'Mathematics';" > $MON_QUERY_DIR/am_3.sql
  
## am_4.sql 
## storage modual size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-4,Storage(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT \
WHERE NAME LIKE 'Storage%';" > $MON_QUERY_DIR/am_4.sql
  
## am_5.sql 
## temp modual size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-5,Temp(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT \
WHERE NAME = 'Temp_Memory';" > $MON_QUERY_DIR/am_5.sql
  
## am_6.sql 
## trans modual size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-6,Transaction(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT \
WHERE NAME LIKE 'Trans%';" > $MON_QUERY_DIR/am_6.sql
  
## am_7.sql 
## index modual size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-7,Index(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT \
WHERE NAME = 'Index_Memory';" > $MON_QUERY_DIR/am_7.sql
  
## am_8.sql 
## log modual size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-8,Log(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT \
WHERE NAME = 'LOG_Memory';" > $MON_QUERY_DIR/am_8.sql
  
## am_9.sql 
## other modual size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-9,Other(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT \
WHERE NAME LIKE 'O%';" > $MON_QUERY_DIR/am_9.sql
  
## am_10.sql 
## total size 
echo " \
SELECT \
  SYSDATE || \
  ',AM,AM-10,Total(Bytes),'|| \
  SUM(ALLOC_SIZE) \
FROM \
V\$MEMSTAT ;" > $MON_QUERY_DIR/am_10.sql
  
###---------------------------
### garbage collector
###---------------------------

## gc_1.sql 
## garbage collection gap 
echo " \
SELECT \
  SYSDATE || \
  ',GC,GC-1,'||GC_NAME||',GC_GAP:'|| \
  ADD_TSS_CNT-GC_TSS_CNT \
FROM \
V\$DISKGC \
UNION ALL \
SELECT \
  SYSDATE || \
  ',GC,GC-1,'||GC_NAME||',GC_GAP:'|| \
  ADD_OID_CNT-GC_OID_CNT \
FROM \
V\$MEMGC;" > $MON_QUERY_DIR/gc_1.sql
 

#################################################
# pasing argument
#################################################
# BUG-30945 : Restart parameter of altimon is wrong
case $1 in
    view) tail -f $LOG_FILE
          exit 0
          ;;
    stop) p=`ps -ef|$REG_GREP altimon.sh|$REG_GREP -v $$|$REG_GREP -v grep | $REG_GREP -v vi | $REG_GREP -v tail |$AWK '{print $2}'`
          kill -9 $p 2>/dev/null
          exit 0
          ;;
    help) echo "Usage : altimon.sh [view|start|stop|help]"
          echo "        view : Show the up to date log file continuously"
          echo "        start: Run altimon (this option can be omitted)"
          echo "        stop : Kill altimon"
          echo "        help : Display this screen"
          exit 0
          ;;
    ''|'start') p=`ps -ef | $REG_GREP altimon.sh |$REG_GREP $WHOAMI| $REG_GREP -v grep |$REG_GREP -v $$|$REG_GREP -v view|$REG_GREP -v vi | $REG_GREP -v tail`
       echo $!
       echo $p
       if [ "x$p" != "x" ];then
          echo "\n`basename $0` is already running !!"
          exit 0
       fi
       ;;
    *) echo "Usage : altimon.sh [view|start|stop|help]"
       echo "        view : Show the up to date log file continuously"
       echo "        start: Run altimon (this option can be omitted)"
       echo "        stop : Kill altimon"
       echo "        help : Display this screen"
       exit 0
       ;;
esac

#################################################
# Initialize
#################################################
### make directories
if [ ! -d $ALTIMON_HOME -a ! -x $ALTIMON_HOME ]
then
  echo "mkdir $ALTIMON_HOME"
  mkdir $ALTIMON_HOME
  if [ $? -ne 0 ]
  then
    echo "Error : mkdir $ALTIMON_HOME" >&2
    exit 0;
  fi
fi

if [ ! -d $BACKUP_DIR -a ! -x $BACKUP_DIR ]
then
  echo "mkdir $BACKUP_DIR"
  mkdir $BACKUP_DIR
  if [ $? -ne 0 ]
  then
    echo "Error : mkdir $BACKUP_DIR" >&2
    exit 0;
  fi
fi

if [ ! -d $MON_QUERY_DIR -a ! -x $MON_QUERY_DIR ]
then
  echo "mkdir $MON_QUERY_DIR"
  mkdir $MON_QUERY_DIR
  if [ $? -ne 0 ]
  then
    echo "Error : mkdir $MON_QUERY_DIR" >&2
    exit 0;
  fi
fi

if [ ! -d $TMP_DIR -a ! -x $TMP_DIR ]
then
  echo "mkdir $TMP_DIR"
  mkdir $TMP_DIR
  if [ $? -ne 0 ]
  then
    echo "Error : mkdir $TMP_DIR" >&2
    exit 0;
  fi
fi

#################################################
# Function Define
#################################################
make_error_msg ()
{
    echo "!! [`date '+%Y/%m/%d %H:%M:%S'`] : $*"  | tee -a $ERROR_LOG
}

make_alarm_msg ()
{
    echo "!! [`date '+%Y/%m/%d %H:%M:%S'`] : $*"  | tee -a $ALARM_LOG
}

sound_alarm_high ()
{
	tmp=`echo $1 | awk '{print int($1)}'`
    if [ $tmp -gt $HIGH_ALARM_LIMIT ]
    then
       	make_alarm_msg "[ $2 ] Usage : $1%"
    fi
}

sound_alarm_user_define ()
{
	tmp1=`echo $1 | awk '{print int($1)}'`
	tmp2=`echo $2 | awk '{print int($1)}'`
    if [ $tmp1 -gt $tmp2 ]
    then
       	make_alarm_msg "[ $3 ] $1"
    fi
}

sound_alarm_low ()
{
	tmp=`echo $1 | awk '{print int($1)}'`
    if [ $tmp -lt $LOW_ALARM_LIMIT ]
    then
       	make_alarm_msg "[ $2 ] Usage : $1%"
    fi
}

check_proc ()
{
    c_=0
    p=`ps -p $1 |$REG_GREP $1 2> /dev/null`

    while
                [ "x$p" != "x" ]
        do
        p=`ps -p $1 |$REG_GREP $1 2> /dev/null`
        sleep 1
        c_=`expr $c_ + 1`

        if [ $c_ -gt $MAX_WAIT_TIME ]
        then
            make_error_msg "DB HANG!!"
            kill -9 $p
            break
        fi
    done
}

getProperty()
{
	i=1

	while [ $i -lt 9 ]
	do
		echo "select 'PR '||value$i from v\$property where name=upper('$1');" > $MON_QUERY_DIR/pr_1.sql
		isql -S 127.0.0.1 -U SYS -P $SYS_PASSWD -silent -f $MON_QUERY_DIR/pr_1.sql -o $TMP_DIR/pr_1.res &
		check_proc $!
		PROPERTY[$i]=`$REG_GREP PR $TMP_DIR/pr_1.res | $REG_GREP -v "||" | $AWK '{print $2}'`

	    if [ "x${PROPERTY[$i]}" = "x" ]
	    then
			break;
	    fi
		i=`expr $i + 1`
	done
}

getSpaceInfo()
{

    if [ $SYSTEM = "HP-UX" ]
    then
        TotalSpace=`bdf $1| $AWK '{  if( NR == 2) printf "%d", $2 }'`
        FreeSpace=`bdf $1| $AWK '{  if( NR == 2) printf "%d", $4 }'`

    elif [ $SYSTEM = "AIX" ]
    then
        FreeSpace=`df -k $1| $AWK '{  if( NR == 2) printf "%d", $3*1024 }'`

    else
        TotalSpace=`df -k $1| $AWK '{  if( NR == 2) printf "%d", $2*1024 }'`
        FreeSpace=`df -k $1| $AWK '{  if( NR == 2) printf "%d", $4*1024 }'`
    fi

    Usage=`expr \( $TotalSpace - $FreeSpace \) \* 100 / $TotalSpace`

}

checkAltibaseHomeDiskSpace()
{
    getSpaceInfo $ALTIBASE_HOME
	sound_alarm_high $Usage "ALTIBASE_HOME_DIR : $ALTIBASE_HOME"
    if [ $SYSTEM = "HP-UX" ]
    then
    	echo "$CUR_TIME_TMP,SY,SY-11,ALTIBASE_HOME_FREE(KBytes),$FreeSpace" >> $LOG_FILE
	else
    	echo "$CUR_TIME_TMP,SY,SY-11,ALTIBASE_HOME_FREE(Bytes),$FreeSpace" >> $LOG_FILE
	fi
}

checkAltibaseDBDiskSpace()
{
	# MEM_DB_DIR
	# DEFAULT_DISK_DB_DIR
    # USER DATAFILES

    getProperty "MEM_DB_DIR" 

	i=1
	while [ $i -lt 9 ]
	do
	    if [ "x${PROPERTY[$i]}" = "x" ]
	    then
			break;
	    fi
			
    	getSpaceInfo ${PROPERTY[$i]}
		sound_alarm_high $Usage "MEM_DB_DIR : ${PROPERTY[$i]}"
    	if [ $SYSTEM = "HP-UX" ]
    	then
       		echo "$CUR_TIME_TMP,SY,SY-21,MEM_DB_DIR_FREE(KBytes),$FreeSpace" >> $LOG_FILE
		else
       		echo "$CUR_TIME_TMP,SY,SY-21,MEM_DB_DIR_FREE(Bytes),$FreeSpace" >> $LOG_FILE
		fi

		i=`expr $i + 1`
	done

    getProperty "DEFAULT_DISK_DB_DIR" 

	i=1
	while [ $i -lt 9 ]
	do
	    if [ "x${PROPERTY[$i]}" = "x" ]
	    then
			break;
	    fi
			
    	getSpaceInfo ${PROPERTY[$i]}
		sound_alarm_high $Usage "DEFAULT_DISK_DB_DIR : ${PROPERTY[$i]}"
    	if [ $SYSTEM = "HP-UX" ]
    	then
       		echo "$CUR_TIME_TMP,SY,SY-22,DEFAULT_DISK_DB_DIR_FREE(KBytes),$FreeSpace" >> $LOG_FILE
		else
       		echo "$CUR_TIME_TMP,SY,SY-22,DEFAULT_DISK_DB_DIR_FREE(Bytes),$FreeSpace" >> $LOG_FILE
		fi

		i=`expr $i + 1`
	done

	echo "select name from v\$datafiles;" > $TMP_DIR/df.sql
  	isql -S 127.0.0.1 -U SYS -P $SYS_PASSWD -silent -f $TMP_DIR/df.sql -o $TMP_DIR/df1.res &
	check_proc $!
	cat $TMP_DIR/df1.res | $REG_GREP "/" | tr -d " " | while read val 
	do 
		echo `dirname $val`; 
	done | uniq > $TMP_DIR/df2.res

	cat $TMP_DIR/df2.res | while read val
	do
		getSpaceInfo $val
		sound_alarm_high $Usage "DATAFILE_DIR : $val"
    	if [ $SYSTEM = "HP-UX" ]
    	then
       		echo "$CUR_TIME_TMP,SY,SY-23,DATAFILE_DB_DIR_FREE(KBytes),$FreeSpace" >> $LOG_FILE
		else
       		echo "$CUR_TIME_TMP,SY,SY-23,DATAFILE_DB_DIR_FREE(Bytes),$FreeSpace" >> $LOG_FILE
		fi
	done
}

checkAltibaseLOGDiskSpace()
{
	# LOG_DIR
	# LOGANCHOR_DIR
	# ARCHIVE_DIR

    getProperty "LOG_DIR" 

	i=1
	while [ $i -lt 9 ]
	do
	    if [ "x${PROPERTY[$i]}" = "x" ]
	    then
			break;
	    fi
			
    	getSpaceInfo ${PROPERTY[$i]}
		sound_alarm_high $Usage "LOG_DIR : ${PROPERTY[$i]}"
    	if [ $SYSTEM = "HP-UX" ]
    	then
       		echo "$CUR_TIME_TMP,SY,SY-31,LOG_DIR_FREE(KBytes),$FreeSpace" >> $LOG_FILE
		else
       		echo "$CUR_TIME_TMP,SY,SY-31,LOG_DIR_FREE(Bytes),$FreeSpace" >> $LOG_FILE
		fi

		i=`expr $i + 1`
	done

    getProperty "LOGANCHOR_DIR" 

	i=1
	while [ $i -lt 9 ]
	do
	    if [ "x${PROPERTY[$i]}" = "x" ]
	    then
			break;
	    fi
			
    	getSpaceInfo ${PROPERTY[$i]}
		sound_alarm_high $Usage "LOGANCHOR_DIR : ${PROPERTY[$i]}"
    	if [ $SYSTEM = "HP-UX" ]
    	then
       		echo "$CUR_TIME_TMP,SY,SY-32,LOGANCHOR_DIR_FREE(KBytes),$FreeSpace" >> $LOG_FILE
		else
       		echo "$CUR_TIME_TMP,SY,SY-32,LOGANCHOR_DIR_FREE(Bytes),$FreeSpace" >> $LOG_FILE
		fi

		i=`expr $i + 1`
	done

    getProperty "ARCHIVE_DIR" 

	i=1
	while [ $i -lt 9 ]
	do
	    if [ "x${PROPERTY[$i]}" = "x" ]
	    then
			break;
	    fi
			
    	getSpaceInfo ${PROPERTY[$i]}
		sound_alarm_high $Usage "ARCHIVE_DIR : ${PROPERTY[$i]}"
    	if [ $SYSTEM = "HP-UX" ]
    	then
       		echo "$CUR_TIME_TMP,SY,SY-33,ARCHIVE_DIR_FREE(KBytes),$FreeSpace" >> $LOG_FILE
		else
       		echo "$CUR_TIME_TMP,SY,SY-33,ARCHIVE_DIR_FREE(Bytes),$FreeSpace" >> $LOG_FILE
		fi

		i=`expr $i + 1`
	done
}

checkLogFileCount()
{
	# LOG_DIR
	# ARCHIVE_DIR

    getProperty "LOG_DIR" 
	logfile_cnt=`ls ${PROPERTY[1]}/logfile* | wc -l | tr -d " "`
	sound_alarm_user_define $logfile_cnt $MAX_LOG_FILE_CNT "Logfile Count"
    echo "$CUR_TIME_TMP,LF,LF-1,LOGFILE_CNT,$logfile_cnt" >> $LOG_FILE

    getProperty "ARCHIVE_DIR" 
	archlog_cnt=`ls ${PROPERTY[1]}/logfile* | wc -l | tr -d " "`
	sound_alarm_user_define $archlog_cnt $MAX_ARCH_LOG_CNT "Archive Logfile Count"
    echo "$CUR_TIME_TMP,LF,LF-2,ARCHFILE_CNT,$archlog_cnt" >> $LOG_FILE
}

getAltibaseSize()
{
    if [ $SYSTEM = "AIX" ]
    then
        VSZ=`ps v $AltiPid |$REG_GREP -v -i size| $REG_GREP  -e ^[0-9] -e [^A-Za-z]|$AWK '{print $6}'`
    elif [ $SYSTEM = "SunOS" ]
    then
        VSZ=`ps -p $AltiPid -o vsz|$REG_GREP -v -i vsz| $AWK '{print int($1*1024);}'`
    elif [ $SYSTEM = "HP-UX" ]
    then
        VSZ=`ps -p $AltiPid -o vsz|$REG_GREP -v -i vsz| $AWK '{print int($1*1024);}'`
    elif [ $SYSTEM = "OSF1" ]
    then
        VSZ_G=`ps -p $AltiPid -o vsz|$REG_GREP -v -i vsz| $AWK '/G$/' | sed s/G$// |$AWK '{print int($1*1024*1024*1024);}'`
        VSZ_M=`ps -p $AltiPid -o vsz|$REG_GREP -v -i vsz| $AWK '/M$/' | sed s/M$// |$AWK '{print int($1*1024*1024);}'`

        if [ $VSZ_G ]
        then
                VSZ=`echo $VSZ_G`
        elif [ $VSZ_M ]
        then
                VSZ=`echo $VSZ_M`
        fi
	fi

	sound_alarm_user_define $VSZ $V_MEM_LIMIT "ALTIBASE Process Size (Bytes)"
    echo "$CUR_TIME_TMP,AM,AM-0,ALTIBASE_VSZ(Bytes),$VSZ" >> $LOG_FILE
}

getMemoryInfo()
{
    if [ $SYSTEM = "AIX" ]
    then
        VSZ=`ps v $AltiPid |$REG_GREP -v -i size| $REG_GREP  -e ^[0-9] -e [^A-Za-z]|$AWK '{print $6}'`
    elif [ $SYSTEM = "SunOS" ]
    then
        VSZ=`ps -p $AltiPid -o vsz|$REG_GREP -v -i vsz| $AWK '{print int($1*1024);}'`

        swap_used_size=`swap -s | $AWK '{print $9}' | tr -d "k" | $AWK '{print $1*1024}'`
        swap_free_size=`swap -s | $AWK '{print $11}' | tr -d "k" | $AWK '{print $1*1024}'`
        swap_total_size=`expr $swap_used_size + $swap_free_size`
        swap_usage=`echo $VSZ $swap_free_size| $AWK '{print 100-int(($2/($1+$2))*100)}'`

		vmstat 1 3 > $TMP_DIR/vm.res
		memory_total_size=`prtconf | $REG_GREP Memory | $AWK '{print int($3*1024*1024);}'`
		memory_free_size=`tail -1 $TMP_DIR/vm.res | $AWK '{print int($5*1024);}'`
        memory_usage=`expr \( $memory_total_size - $memory_free_size \) \* 100 / $memory_total_size`

        echo "$CUR_TIME_TMP,SY,SY-1,SWAP_TOTAL(Bytes),$swap_total_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-2,SWAP_FREE(Bytes),$swap_free_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-3,SWAP_USAGE(%),$swap_usage" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-4,PHYSICAL_TOTAL(Bytes),$memory_total_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-5,PHYSICAL_FREE(Bytes),$memory_free_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-6,PHYSICAL_MEM_USAGE(%),$memory_usage" >> $LOG_FILE

		sound_alarm_high $swap_usage Swap
		sound_alarm_high $memory_usage "Physical Memory"

    elif [ $SYSTEM = "HP-UX" ]
    then
        VSZ=`ps -p $AltiPid -o vsz|$REG_GREP -v -i vsz| $AWK '{print $1}'`

        swap_total_size=`/etc/swapinfo -t | $REG_GREP total | $AWK '{print $2}'`
        swap_free_size=`/etc/swapinfo -t | $REG_GREP total | $AWK '{print $4}'`
        #swap_total_size=`expr $swap_total_size \* 1024`
        #swap_free_size=`expr $swap_free_size \* 1024`
        swap_usage=`echo $VSZ $swap_free_size| $AWK '{print 100-int(($2/($1+$2))*100)}'`

        echo "$CUR_TIME_TMP,SY,SY-1,SWAP_TOTAL(KBytes),$swap_total_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-2,SWAP_FREE(KBytes),$swap_free_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-3,SWAP_USAGE(%),$swap_usage" >> $LOG_FILE

		sound_alarm_high $swap_usage "Swap"

        ker_max_size=`/usr/sbin/kmtune | $REG_GREP maxdsiz_64bit | $AWK '{print int($2/1024)}'`
        ker_usage=`echo $VSZ $ker_max_size | $AWK '{print int($1*100/$2)}'`

        echo "$CUR_TIME_TMP,SY,SY-7,KERNEL_MAX_PROCESS_SIZE(Bytes),$ker_max_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-8,ALTIBASE_USAGE(KERNEL,%),$ker_usage" >> $LOG_FILE

		sound_alarm_high $ker_usage "Altibase Memory"

    elif [ $SYSTEM = "OSF1" ]
    then
        VSZ_G=`ps -p $AltiPid -o vsz|$REG_GREP -v -i vsz| $AWK '/G$/' | sed s/G$// |$AWK '{print int($1*1024*1024*1024);}'`
        VSZ_M=`ps -p $AltiPid -o vsz|$REG_GREP -v -i vsz| $AWK '/M$/' | sed s/M$// |$AWK '{print int($1*1024*1024);}'`

        if [ $VSZ_G ]
        then
                VSZ=`echo $VSZ_G`
        elif [ $VSZ_M ]
        then
                VSZ=`echo $VSZ_M`
        fi

        page_size=`pagesize`
        swap_alloc_pages=`swapon -s | $REG_GREP "Allocated space" | head -1 | $AWK '{print $3}'`
        swap_free_pages=`swapon -s | $REG_GREP "Available space" | head -1 | $AWK '{print $3}'`
        swap_alloc_size=`echo $swap_alloc_pages $page_size | $AWK '{print $1*$2}'`
        swap_free_size=`echo $swap_free_pages $page_size | $AWK '{print $1*$2}'`
        swap_usage=`echo $VSZ $swap_free_size| $AWK '{print 100-int(($2/($1+$2))*100)}'`

        echo "$CUR_TIME_TMP,SY,SY-1,TOTAL_SWAP(Bytes),$swap_total_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-2,SWAP_FREE(Bytes),$swap_free_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-3,SWAP_USAGE(%),$swap_usage" >> $LOG_FILE

		sound_alarm_high $swap_usage "Swap"

        sysconfig -q proc | $REG_GREP per_proc_data | $AWK '{print $3}' > $TMP_DIR/ker_tmp.txt
        sysconfig -q proc | $REG_GREP per_proc_address | $AWK '{print $3}' >> $TMP_DIR/ker_tmp.txt
        ker_max_size=`sort -n $TMP_DIR/ker_tmp.txt | head -1`
        ker_usage=`echo $VSZ $ker_max_size | $AWK '{print int($1/$2*100)}'`

        echo "$CUR_TIME_TMP,SY,SY-7,KERNEL_MAX_PROCESS_SIZE(Bytes),$ker_max_size" >> $LOG_FILE
        echo "$CUR_TIME_TMP,SY,SY-8,ALTIBASE_USAGE(KERNEL,%),$ker_usage" >> $LOG_FILE

		sound_alarm_high $ker_usage "Altibase Memory"
    fi
}

run()
{
  query=$1
  class=$2

echo "run $1 $2"

cp $MON_QUERY_DIR/$query.sql $TMP_DIR/$class.sql

# Permission denied error check
# »ç¿ë Àåºñ¿¡¼­ Permission °ü·Ã ¿¡·¯°¡ ÀÚÁÖ ¹߻ýÇϿ© À̸¦ Á¡°ËÇÔ.
# °°Àº ½ð£´뿡 µ¿ÀÛÇÑ Äõ¸®µéÀÇ ½ð£ÀÌ Á¶±ݾ¿ Ʋ¸± ¼ö À־î shell¿¡¼­ À̸¦ ¾ò¾î 
# Äõ¸® ³»¿¡ ÀִÂ SYSDATE ¿Í ´ëġ ÇÔ.
while :
do

$EX -s $TMP_DIR/$class.sql <<EOF >$TMP_DIR/ex.res
%s/SYSDATE/'$CUR_TIME'/g
wq
EOF

err=`wc -l $TMP_DIR/ex.res|$AWK '{print $1}'`

if [ $err -eq 0 ]
then
    break
fi
done


  #is -f $MON_QUERY_DIR/$query.sql -o $TMP_DIR/$class.res
  isql -S 127.0.0.1 -U SYS -P $SYS_PASSWD -silent -f $TMP_DIR/$class.sql -o $TMP_DIR/$class.res &
  check_proc $!

  # For Debugging
  #cat $TMP_DIR/$class.res

  $REG_GREP -i ",$class," $TMP_DIR/$class.res |$REG_GREP -v '||'|$REG_GREP -v selected > $TMP_DIR/$class.tmp

  cnt=`wc -l $TMP_DIR/$class.tmp|$AWK '{print $1}'`
  
  #echo $query $cnt

  if [ $cnt -gt 0 ]
  then
    # Permission denied error check
    while :
    do
      $EX -s $TMP_DIR/$class.tmp <<EOF >$TMP_DIR/ex.res
%s/  //g
wq
EOF
    if [ $err -eq 0 ]
    then
        break
    fi
    done
  
  fi 

  if [ $query = ts_3 -o $query = tb_4 -o $query = vi_1 -o $query = ss_3 -o $query = gc_1 -o $query = rp_3 -o $query = rp_53 ]
  then
    cat $TMP_DIR/$class.tmp | while read val
    do
      item=`echo $val | awk 'BEGIN { FS="," } { print $4 }'` 
      case $query in
      ts_3)
        value=`echo $val | awk 'BEGIN { FS="," } { print $5 }' | tr -d "EFFICIENCY(%):"` 
        sound_alarm_high $value "Tablespace : $item";;
      tb_4)
        value=`echo $val | awk 'BEGIN { FS="," } { print $5 }' | tr -d "EFFICIENCY(%):"` 
        sound_alarm_low $value "Table : $item";;
      vi_1)
        value=`echo $val | awk 'BEGIN { FS="," } { print $5 }'` 
        if [ $value = "INVALID" ]
        then
          make_alarm_msg "[ View : $item ] $value"
        fi;;
      ss_3)
        value=`echo $val | awk 'BEGIN { FS="," } { print $5 }' | tr -d "IDLE_START_TIME:"` 
        if [ $value != 0 ]
        then
			echo "select 'Elapsed ' || (sysdate - to_date($value, 'YYYY/MM/DD HH:MI:SS'))*24*60*60 from v\$database;" > $MON_QUERY_DIR/ss_3_tmp.sql
			isql -S 127.0.0.1 -U SYS -P $SYS_PASSWD -silent -f $MON_QUERY_DIR/ss_3_tmp.sql -o $TMP_DIR/ss_3_tmp.res &
        	value=`$REG_GREP Elapsed $TMP_DIR/ss_3_tmp.res | awk '{ print int($2) }'` 
        	sound_alarm_user_define $value $MAX_TRANS_TIME "Session $item, Elapsed Time"
        fi;;
      gc_1)
        value=`echo $val | awk 'BEGIN { FS="," } { print $5 }' | tr -d "GC_GAP:"` 
        sound_alarm_user_define $value $MAX_GC_GAP "GC_GAP : $item";;
      rp_3)
        value=`echo $val | awk 'BEGIN { FS="," } { print $5 }'` 
        if [ $value = "RETRY" ]
        then
          make_alarm_msg "[ Replication : $item ] $value"
        fi;;
      rp_53)
        sound_alarm_user_define $value $MAX_REPL_GAP "Replication Gap : $item";;
      *)
        continue;;
      esac
    done
  fi

  cat $TMP_DIR/$class.tmp >> $LOG_FILE
#  echo " " >> $LOG_FILE
}


################################################e
# Script Start
#################################################
### check ALTIBASE_HOME & echo Configration
if [ "x$ALTIBASE_HOME"  = "x" ]; then
    echo "ALTIBASE_HOME is not defined. please Check your environments" >&2
    exit
elif [ ! -d $ALTIBASE_HOME ]; then
    echo "ALTIBASE_HOME is not a directory" >&2
    exit
else
    echo "ALTIBASE_HOME   = $ALTIBASE_HOME"
fi

getProperty "LOG_FILE_SIZE"
LOG_FILE_SIZE=`echo ${PROPERTY[1]}`

echo "ALTIMON_HOME    = $ALTIMON_HOME"
echo "MON_QUERY_DIR   = $MON_QUERY_DIR"
echo "TMP_DIR         = $TMP_DIR"
#echo "BACKUP_DIR      = $BACKUP_DIR"
#echo "STAT_DIR        = $STAT_DIR"
echo "LOG_FILE        = $LOG_FILE"
echo "ERROR_LOG       = $ERROR_LOG"
echo "ALARM_LOG       = $ALARM_LOG"

echo "INTERVAL        = $INTERVAL sec"


###------------------------------------- 
### make Deamon
###-------------------------------------

trap "" 1 2 3 6 10 12 13

OLD_TIME="x"

while :
do
    echo " " >> $LOG_FILE
    echo ================`date '+%Y/%m/%d %H:%M:%S'`================= >> $LOG_FILE
	echo " " >> $LOG_FILE

	CUR_TIME=`date +"%Y\/%m\/%d %H:%M:%S"`
	CUR_TIME_TMP=`date +"%Y/%m/%d %H:%M:%S"`
	CUR_TIME2=`date +%Y%m%d`

	#LOG_FILE=$ALTIMON_HOME/altimon_`date '+%Y%m%d'`.log

    ##########################
    ## Altibase Process check
    ##########################

    altihome=`echo $ALTIBASE_HOME/bin/altibase|tr -d '/'`
    AltiPid=`ps -ef |tr -d '/'| $REG_GREP -v grep |$REG_GREP "$altihome -p" | $AWK '{ print $2}'`

    if [ "x$AltiPid" = "x" ]
    then
        make_error_msg "Altibase Die !!"
        sleep 60
        continue
    fi

	echo "[[[[[   SYSTEM CHECK   ]]]]]" >> $LOG_FILE
	echo " " >> $LOG_FILE

    ##########################
    ### Memory Check 
    ##########################

	echo "[ MEMORY CHECK ]" >> $LOG_FILE
    getMemoryInfo
	echo " " >> $LOG_FILE

    ##########################
    ### File System Check 
    ##########################

	echo "[ FILE SYSTEM CHECK ]" >> $LOG_FILE
    checkAltibaseHomeDiskSpace
    checkAltibaseDBDiskSpace
    checkAltibaseLOGDiskSpace
	echo " " >> $LOG_FILE

	echo "[[[[[   ALTIBASE CHECK   ]]]]]" >> $LOG_FILE
	echo " " >> $LOG_FILE

	########################
	# COMMON QUERY
	########################
	#  COMMON Ç׸ñÀ» ÀÏ 1ȸ µ¿ÀÛ ½ÃŰ±â À§ÇØ
	if [ "x$OLD_TIME"  != "x$CUR_TIME2" ]; then
	
	    isql -S 127.0.0.1 -U SYS -P $SYS_PASSWD -silent -f $MON_QUERY_DIR/backup_day.sql -o $TMP_DIR/backup_day.res &
		check_proc $!
	    $REG_GREP -i [0-9] $TMP_DIR/backup_day.res |$REG_GREP -v '||'|$REG_GREP -v selected > $TMP_DIR/backup_day.tmp
	    BACKUP_DAY=`is -f $MON_QUERY_DIR/backup_day.sql |$REG_GREP -i [0-9]|$REG_GREP -v [A-z]`
	
	    BACKUP_FILE=`echo "$ALTIMON_HOME/altimon_$BACKUP_DAY.log" |tr -d ' '`
	    #echo $BACKUP_FILE
	
	    #mv $BACKUP_FILE $BACKUP_DIR
	    echo "[ COMMON ]" >> $LOG_FILE
	     
	    ### CM-1
	    run cm_1 cm
	    
	    ### CM-2
	    run cm_2 cm
	    
	    ### CM-3
	    run cm_3 cm
	    
	    ### CM-4
	    run cm_4 cm
	    
	    ### CM-5
	    run cm_5 cm
	    
	    ### CM-6
	    run cm_6 cm
	    
	    ### CM-7
	    run cm_7 cm
	    
	    ### CM-8
	    run cm_8 cm
	
	    ### CM-9
	    run cm_9 cm
	    
	    echo " " >> $LOG_FILE
	fi
	
    ##########################
    ### Altibase Process Size
    ##########################

	echo "[ MEMORY ]" >> $LOG_FILE

	getAltibaseSize

	########################
	# MEMORY QUERY
	########################
	
	### AM-1
	run am_1 am
	 
	### AM-2
	run am_2 am
	
	### AM-3
	run am_3 am
	
	### AM-4
	run am_4 am
	 
	### AM-5
	run am_5 am
	 
	### AM-6
	run am_6 am
	
	### AM-7
	run am_7 am
	
	### AM-8
	run am_8 am
	
	### AM-9
	run am_9 am
	
	### AM-10
	run am_10 am
	
	echo " " >> $LOG_FILE
	 
	########################
	# TABLESPACE QUERY
	########################
	
	echo "[ TABLESPACE ]" >> $LOG_FILE
	
	### TS-0
	run ts_0 ts
	 
	### TS-1
	run ts_1 ts
	 
	### TS-2
	run ts_2 ts
	
	### TS-3
	run ts_3 ts
	
	### TS-4
	run ts_4 ts
	
	### TS-5
	run ts_5 ts
	
	echo " " >> $LOG_FILE
	 
	########################
	# TABLE QUERY
	########################
	
	echo "[ TABLE ]" >> $LOG_FILE
	### TB-1
	run tb_1 tb
	 
	### TB-2
	run tb_2 tb
	
	### TB-3
	#run tb_3 tb
	
	### TB-4
	run tb_4 tb
	
	### TB-11
	run tb_11 tb
	 
	### TB-21
	run tb_21 tb
	
	echo " " >> $LOG_FILE
	 
	########################
	# VIEW QUERY
	########################
	
	echo "[ VIEW ]" >> $LOG_FILE
	
	### VI-1
	run vi_1 vi 
	
	echo " " >> $LOG_FILE
	 
	########################
	# SESSION QUERY
	########################
	
	echo "[ SESSION ]" >> $LOG_FILE
	 
	### SS-1
	run ss_1 ss 
	 
	### SS-2
	run ss_2 ss 
	
	### SS-3
	run ss_3 ss 
	
	echo " " >> $LOG_FILE
	 
	########################
	# GARBAGE COLLECTION 
	########################
	
	echo "[ GARBAGE COLLECTION ]" >> $LOG_FILE
	 
	### GC-1
	run gc_1 gc 
	 
	echo " " >> $LOG_FILE
	 
	########################
	# REPLICATION QUERY
	########################
	
	echo "[ REPLICATION ]" >> $LOG_FILE
	 
	### RP-1
	run rp_1 rp 
	 
	### RP-2
	run rp_2 rp 
	 
	### RP-3
	run rp_3 rp 
	 
	### RP-51
	run rp_51 rp 
	 
	### RP-52
	run rp_52 rp 
	 
	### RP-53
	run rp_53 rp 
	 
	echo " " >> $LOG_FILE

	########################
	# LOGFILE COUNT 
	########################
	
	echo "[ LOGFILE COUNT ]" >> $LOG_FILE
	 
	checkLogFileCount

	echo " " >> $LOG_FILE

	OLD_TIME=$CUR_TIME2
	sleep $INTERVAL

done  &

