#!/bin/sh

#-----
# User Define
#-----
SERVER_IP='127.0.0.1'
FTERM='!*@&$^%'
RTERM='%^$&@*!'

# For DEBUG
#echo ${SERVER_IP}
#echo ${FTERM}
#echo ${RTERM}

#-----
# Env check
#-----

if [ "x$ALTIBASE_HOME"  = "x" ]; then
    echo "ALTIBASE_HOME is not defined. please Check your environments"
    exit
elif [ ! -d $ALTIBASE_HOME ]; then
    echo "ALTIBASE_HOME is not a directory"
    exit
else
    echo "ALTIBASE_HOME = $ALTIBASE_HOME"
fi

#-----
# Argument Check
#-----

usage() {
    echo "mig.sh [username/passwd] {out|in}"
    exit 0
}

if [ $# -eq 1 ]; then
    ARGV1=
    ARGV2=$1
elif [ $# -eq 2 ]; then
    ARGV1=$1
    ARGV2=$2
else
    usage
fi

if [ "x${ARGV1}" = "x" ]; then
    USER=SYS
    PASSWD=MANAGER
else
    if [ `expr ${ARGV1} : '.*/'` -eq 0 ]; then
        usage
    fi

    USER=`echo ${ARGV1} | cut -d'/' -f1`
    PASSWD=`echo ${ARGV1} | cut -d'/' -f2`
fi

ILOADER="${ALTIBASE_HOME}/bin/iloader -S ${SERVER_IP} -U ${USER} -P ${PASSWD}"
ISQL="${ALTIBASE_HOME}/bin/isql -S ${SERVER_IP} -U SYSTEM_ -P MANAGER -silent"
ISQLU="${ALTIBASE_HOME}/bin/isql -S ${SERVER_IP} -U ${USER} -P ${PASSWD} -silent"

# For DEBUG
#echo ${ISQL}
#echo ${ILOADER}

#-----
# Functions
#-----
getUserID() {
    QUERY="
select
    user_name||' '||
    user_id
from
    sys_users_
where
    user_name = upper('${USER}')
;
"

        rm aaa_.tmp >/dev/null 2>&1

    TMPBUF=`${ISQL} << EOF
set heading off;
spool aaa_.tmp
$QUERY
spool off
exit    
EOF
`

    USER=`grep -i "^${USER} " aaa_.tmp | awk '{print $1}'`
    USERID=`grep -i "^${USER} " aaa_.tmp | awk '{print $2}'`
# For DEBUG
#echo "USER = $USER"
#echo "USERID = ${USERID}"

    rm aaa_.tmp

    if [ -z "$USERID" ]; then
        echo "Invalid Username : ${USER}"
        exit
    fi
}

getTableName() {
    QUERY="
select
    user_id||' '||
    table_name 
from
    sys_tables_
where
    user_id = ${USERID} and
    table_type = 'T';
"

    TMPBUF=`${ISQL} << EOF
set heading off;
spool aaa_.tmp
$QUERY
spool off
exit
EOF
`

    if [ -f ${USER}_mig_list.txt ]; then
        rm ${USER}_mig_list.txt
    fi

    grep "^${USERID} " aaa_.tmp | grep -v selected | awk '{print $2}' > ${USER}_mig_list.txt
    rm aaa_.tmp

    if [ ! -s ${USER}_mig_list.txt ]; then
        echo "There is no Table to Migration"
        exit
    fi
}

getDataType ()
{
    if [ $1 -eq -5 ]; then
        echo "BIGINT"
    elif [ $1 -eq 1 ]; then
        echo "CHAR"
    elif [ $1 -eq 2 ]; then
        echo "NUMERIC"
    elif [ $1 -eq 4 ]; then
        echo "INTEGER"
    elif [ $1 -eq 5 ]; then
        echo "SMALLINT"
    elif [ $1 -eq 6 ]; then
        echo "NUMBER"
    elif [ $1 -eq 7 ]; then
        echo "REAL"
    elif [ $1 -eq 8 ]; then
        echo "DOUBLE"
    elif [ $1 -eq 9 ]; then
        echo "DATE"
    elif [ $1 -eq 12 ]; then
        echo "VARCHAR"
    elif [ $1 -eq 30 ]; then
        echo "BLOB"
    elif [ $1 -eq 20001 ]; then
        echo "BYTE"
    elif [ $1 -eq 20002 ]; then
        echo "NIBBLE"
    else
        echo ""
    fi
}

makeColDef ()
{
    QUERY="
select
    a.column_order||' '||
    a.column_name||' '||
    a.data_type||' '||
    a.precision||' '||
    a.scale||' '||
    a.is_nullable||' '||
    a.is_varing||' '||
    a.default_val
from
    sys_columns_ a,
    sys_tables_  b
where
    a.table_id = b.table_id and
    b.table_name = '$TNAME' and
    a.user_id = $USERID
; "

    TMPBUF=`${ISQL} << EOF
set linesize 200
set heading off;
spool aaa_.tmp
$QUERY
spool off
exit    
EOF
`

    grep "^[0-9][0-9]* " aaa_.tmp | grep -v selected | sort > ${USER}_${TNAME}_col.tmp
    rm aaa_.tmp

    START=0
    ISVARIABLE=F

    while
        read  ORDER CNAME DTYPE PREC SCALE ISNULL ISVARIABLE DEFAULT
    do
        if [ $START -eq 0 ]; then
            START=1
        else
            echo ","
        fi
        if [ "X$CNAME" != "X" -a "X$DTYPE" != "X" ]; then
            echo "    $CNAME \c"
            DTYPE=`getDataType $DTYPE`
            echo "$DTYPE \c"
            if [ $PREC -ne 0 ]; then
                echo "($PREC\c"
                if [ $SCALE -ne 0 ]; then
                    echo ",$SCALE) \c"
                else
                    echo ") \c"
                fi
            fi
            if [ $DTYPE = "VARCHAR" ]; then
                if [ "$ISVARIABLE" = "F" ]; then
                    echo "FIXED \c"
                elif [ "$ISVARIABLE" = "T" ]; then
                    echo "VARIABLE \c"
                fi
            fi
            if [ "X$DEFAULT" != "X" ];then
                echo "default $DEFAULT \c"
            fi
            if [ "$ISNULL" = "F" ]; then
                echo "NOT NULL \c"
            fi
        fi
    done < ${USER}_${TNAME}_col.tmp
    echo ");"

    rm ${USER}_${TNAME}_col.tmp
}

makeUnique ()
{
    QUERY="
select
    c.constraint_col_order||' '||
    a.constraint_name||' '||
    d.column_name
from
    sys_constraints_ a,
    sys_tables_  b ,
    sys_constraint_columns_ c,
    sys_columns_ d
where
    a.table_id = b.table_id and
    a.constraint_id = c.constraint_id and
    c.column_id = d.column_id and
    b.table_name = '$TNAME' and
    a.user_id = $USERID and
    a.constraint_type = 2
;
"

    TMPBUF=`${ISQL} << EOF
set linesize 200
set heading off;
spool aaa_.tmp
$QUERY
spool off
exit    
EOF
`

    grep "^[0-9][0-9]* " aaa_.tmp | grep -v selected | sort > ${USER}_${TNAME}_unique.tmp
    rm aaa_.tmp

    START=0
    while
        read ORDER PKNAME CNAME
    do
        if [ "X$CNAME" != "X" ]; then
            if [ $START -eq 0 ]; then
                                echo ""
                echo "alter table $TNAME add unique ($CNAME\c"
                START=1
            else
                echo ",$CNAME\c"
            fi
        fi
    done < ${USER}_${TNAME}_unique.tmp

        if [ -s ${USER}_${TNAME}_unique.tmp ]; then
                echo ");"
        fi

    rm ${USER}_${TNAME}_unique.tmp
}

makePrimary ()
{
    QUERY="
select
    c.constraint_col_order||' '||
    a.constraint_name||' '||
    d.column_name
from
    sys_constraints_ a,
    sys_tables_  b ,
    sys_constraint_columns_ c,
    sys_columns_ d
where
    a.table_id = b.table_id and
    a.constraint_id = c.constraint_id and
    c.column_id = d.column_id and
    b.table_name = '$TNAME' and
    a.user_id = $USERID and
    a.constraint_type = 3
;
"

    TMPBUF=`${ISQL} << EOF
set linesize 200
set heading off;
spool aaa_.tmp
$QUERY
spool off
exit    
EOF
`

    grep "^[0-9][0-9]* " aaa_.tmp | grep -v selected | sort > ${USER}_${TNAME}_pri.tmp
    rm aaa_.tmp

    START=0
    while
        read ORDER PKNAME CNAME
    do
        if [ "X$CNAME" != "X" ]; then
            if [ $START -eq 0 ]; then
                                echo ""
                #echo "alter table $TNAME add constraint $PKNAME primary key ($CNAME\c"
                echo "alter table $TNAME add primary key ($CNAME\c"
                START=1
            else
                echo ",$CNAME\c"
            fi
        fi
    done < ${USER}_${TNAME}_pri.tmp

        if [ -s ${USER}_${TNAME}_pri.tmp ]; then
                echo ");"
        fi

    rm ${USER}_${TNAME}_pri.tmp
}

makeEachIndex ()
{
    QUERY="
select
    b.index_col_order||' '||
    a.index_type||' '||
    a.is_unique||' '||
    c.column_name||' '||
    b.sort_order
from
    sys_indices_ a,
    sys_index_columns_ b,
    sys_columns_ c
where
    a.index_id = b.index_id and
    b.column_id = c.column_id and
    a.index_name = '$IDXNAME' and
    a.user_id = $USERID
;
"

    TMPBUF=`${ISQL} << EOF
set linesize 200
set heading off;
spool aaa_.tmp
$QUERY
spool off
exit    
EOF
`

    grep "^[0-9][0-9]* " aaa_.tmp | grep -v selected | sort > ${USER}_${TNAME}_${IDXNAME}.tmp
    rm aaa_.tmp

    if [ `expr ${IDXNAME} : "^__SYS"` -gt 0 ]; then
        rm ${USER}_${TNAME}_${IDXNAME}.tmp
        return;
    fi

    START=0
    while
        read  ORDER TYPE ISUNIQUE CNAME SORDER
    do
        if [ $START -eq 0 ]; then
            echo ""
            echo "create \c"
            if [ "$ISUNIQUE" = "T" ]; then
                echo "unique \c"
            fi
            echo "index $IDXNAME on $TNAME ($CNAME \c"
            if [ "$SORDER" = "D" ]; then
                echo "DESC \c"
            fi
            START=1
        else
            echo ",$CNAME \c"
            if [ "$SORDER" = "D" ]; then
                echo "DESC \c"
            fi
        fi
    done < ${USER}_${TNAME}_${IDXNAME}.tmp
    
    echo ");"

    rm ${USER}_${TNAME}_${IDXNAME}.tmp
}

makeIndex()
{
    QUERY="
select
    a.user_id||' '||
    a.index_name
from
    sys_indices_ a,
    sys_tables_  b
where
    a.table_id = b.table_id and
    b.table_name = '$TNAME' and
    a.user_id = $USERID
;
"

    TMPBUF=`${ISQL} << EOF
set heading off;
spool aaa_.tmp
$QUERY
spool off
exit    
EOF
`

    INDEX_LIST=`grep "^${USERID} " aaa_.tmp | grep -v selected | awk '{print $2}'`
    rm aaa_.tmp

    for IDXNAME in $INDEX_LIST
    do
        makeEachIndex
    done
}

makeEachTable ()
{
    echo "connect $USER/$PASSWD;"
    echo ""
    echo "drop table $TNAME;"
    echo "create table $TNAME ("
    makeColDef
    makePrimary
    makeUnique
    makeIndex
}

makeTableScript() {
    echo "Make Table Script(y/n)? \c"
    read input_
    if [ "$input_" != "y" ]; then
        echo ">>>> Make Table Create Script (Skiped)"
        return
    fi

    echo ""
    echo ">>>> Make Table Create Script ... "
    echo ""

    while
        read TNAME
    do
        echo "... [$TNAME]"
        makeEachTable > ${USER}_${TNAME}.sql
        echo "... DONE !!"
    done < ${USER}_mig_list.txt

    echo "!!! Done !!!"
}

makeTables () {
    echo "Create Tables(y/n)? \c"
    read input_
    if [ "$input_" != "y" ]; then
        echo ">>>> Create Tables (Skiped)"
        return
    fi

    echo ""
    echo ">>>> Create Tables ... "

    while
        read TNAME
    do
        echo "... [$TNAME](y/n)? \c"
        read input_ <&1
        if [ "$input_" != "y" ]; then
            echo ">>>> Create Tables (Skiped)"
            continue
        fi
        if [ ! -r ${USER}_${TNAME}.sql ]; then
            echo "Error : ${USER}_${TNAME}.sql file does not exsist or readable !!"
            continue
        fi
        $ISQLU -f ${USER}_${TNAME}.sql
        echo "... DONE !!"
    done < ${USER}_mig_list.txt

    echo "!!! Done !!!"
}

makeFormFile() {
    echo "Make Formfile(y/n)? \c"
    read input_
    if [ "$input_" != "y" ]; then
        echo ">>>> Make formfile (Skiped)"
        return
    fi
    echo ""
    echo ">>>> Make formfile ..."
    echo ""

    if [ ! -r ${USER}_mig_list.txt ]; then
        echo "${USER}_mig_list.txt file does not exist or not readable"
        exit
    fi

    while
        read TNAME
    do
        TMPBUF=`${ILOADER} << EOF
formout -f ${USER}_${TNAME}.fmt -T ${TNAME}
quit
EOF
`

    echo "dateform YYYY/MM/DD HH:MI:SS:SSSSSS" >> ${USER}_${TNAME}.fmt

    done < ${USER}_mig_list.txt

    echo "!!! Done !!!"
}

dataImport () {
    echo "Import Data(y/n)? \c"
    read input_
    if [ "$input_" != "y" ]; then
        echo ">>>> Data Import (Skiped)"
        return
    fi
    echo ""
    echo ">>>> Start Import ..."
    echo ""

    if [ ! -r ${USER}_mig_list.txt ]; then
        echo "${USER}_mig_list.txt file does not exist or not readable"
        exit
    fi

    while
        read TNAME
    do
        ${ILOADER} << EOF
in -f ${USER}_${TNAME}.fmt -d ${USER}_${TNAME}.dat -t ${FTERM} -r ${RTERM} -log ${USER}_${TNAME}.ilog -bad ${USER}_${TNAME}.ibad
quit
EOF
    done < ${USER}_mig_list.txt

    echo "!!! Done !!!"
}

dataExport () {
    echo "Export Data(y/n)? \c"
    read input_
    if [ "$input_" != "y" ]; then
        echo ">>>> Data Export (Skiped)"
        return
    fi
    echo ""
    echo ">>>> Start Export ..."
    echo ""

    if [ ! -r ${USER}_mig_list.txt ]; then
        echo "${USER}_mig_list.txt file does not exist or not readable"
        exit
    fi

    while
        read TNAME
    do
        ${ILOADER} << EOF
out -f ${USER}_${TNAME}.fmt -d ${USER}_${TNAME}.dat -t ${FTERM} -r ${RTERM} -log ${USER}_${TNAME}.olog -bad ${USER}_${TNAME}.obad
quit
EOF
    done < ${USER}_mig_list.txt

    echo "!!! Done !!!"
}

#-----
# Main
#-----
getUserID

case ${ARGV2} in 
    in)
        makeTables
        dataImport
    ;;
    out)
        getTableName
        makeTableScript
        makeFormFile
        dataExport
    ;;
    *)
        usage
    ;;
esac


