create or replace procedure dtinfo(tname varchar(40))
as
h_spaceid    bigint;     
h_spacename  char(40);
h_pagecnt    bigint;
h_high_limit bigint;
h_low_limit  bigint;
h_tableid    bigint;
h_totalpage  bigint;
h_rate       double;
h_tid        bigint;
h_desc       char(256);
h_cnt        integer;
h_grant      integer;
h_f1tag      integer;
h_f2tag      integer;
t_desc       char(256);
t_Ycount     bigint;
t_Ncount     bigint;

h_session_id bigint;
h_tx_id2     bigint;
h_wait       integer;
htable_oid bigint;
husername  char(40);

cursor_count integer;
begin

    cursor_count :=0;
--------------------------------------------
-- if two user have table with the same name,
--------------------------------------------
    declare cursor ex1 is
    select A.table_oid , B.User_name
    from system_.sys_tables_ A, system_.sys_users_ B
    where A.table_name = upper(tname)
    and   A.user_id    = B.user_id;

    begin
        open ex1;

        loop
        fetch ex1 into htable_oid, husername;
        exit when ex1%notfound;

        if cursor_count > 0 then
            system_.println(' ');
            system_.println(' ');
        end if;

        h_f1tag  := 0;
        h_f2tag  := 0;

-------------------------------
-- read usage-information of table user request 
-------------------------------
        begin
            select
                 A.TABLESPACE_ID,
                 A.DISK_PAGE_CNT,
                 A.INSERT_HIGH_LIMIT,
                 A.INSERT_LOW_LIMIT,
                 B.NAME
            into 
                 h_spaceid,
                 h_pagecnt,
                 h_high_limit,
                 h_low_limit,
                 h_spacename
            from v$disktbl_info A, v$tablespaces B
            where  a.table_oid = htable_oid 
            and    a.tablespace_id = b.id;

            select 
                 TOTAL_PAGE_COUNT 
            into 
                 h_totalpage
            from v$tablespaces where ID = h_spaceid;

            exception when NO_DATA_FOUND then 
                system_.println('Check if table exists : [ ' || trim(upper(tname)) || ' ]');
                return;
            end;

        h_rate := h_pagecnt / h_totalpage * 100.0;

-------------------------------
-- print title
-------------------------------
        system_.println('Table : ' || trim(upper(husername)) || '.' || trim(upper(tname)));


-------------------------------
-- for reading lock_desc title
-------------------------------
            begin
                select lock_desc into t_desc from v$lock
                where  table_oid = htable_oid
                and    is_grant = 1 order by trans_id limit 1;
                exception when NO_DATA_FOUND then 
                    t_desc := 'NO LOCK';
            end;
         
-------------------------------
-- for reading lock_count not granted
-------------------------------
            begin
                select count(*) into t_Ncount from v$lock
                where  table_oid = htable_oid
                and    is_grant = 0 ;

-------------------------------
-- for reading lock_count granted
-------------------------------
                select count(*) into t_Ycount from v$lock
                where  table_oid = htable_oid
                and    is_grant = 1 ;
            end;


-------------------------------
-- print table lock summary
-------------------------------
            system_.println('        LockMode: [ ' || trim(t_desc) || 
                            ' ], Grant Count:' || t_Ycount || ',  Request Count: ' || t_Ncount);

            declare cursor c1 is
            select trans_id, lock_desc, lock_cnt, is_grant
            from   v$lock
            where  table_oid = htable_oid
            order by is_grant desc;

            begin
                open c1;
                loop
                    fetch c1 into h_tid, h_desc, h_cnt, h_grant;
                    exit when c1%notfound;

-------------------------------
-- print title if grant or not
-------------------------------
                    if h_grant = 1 and h_f1tag = 0
                    then
                        system_.println('        Grant List---------------------' ||
                                        '-----------------------------');
                        h_f1tag := 1;
                    end if;
                    if h_grant = 0 and h_f2tag = 0
                    then
                        system_.println('        Request List-------------------' ||
                                        '-----------------------------');
                        h_f2tag := 1;
                    end if;

-------------------------------
-- print lock information of table
-------------------------------
                    system_.println('           - Trans ID: ' || trim(h_tid) || 
                                    ', Lock Mode: ' || trim(h_desc) );

                    h_wait := 1;
                    begin
                        select session_id, tx_id 
                        into h_session_id, h_tx_id2
                        from v$lock_statement
                        where  table_oid = htable_oid and begin_flag = 0
                        and    tx_id <> h_tid ;
                        exception when NO_DATA_FOUND then 
                                 h_wait := 0;
                   end;

                   if h_wait = 1
                   then
                       system_.println ('               ( Wait for [ session = ' || h_session_id ||
                                        ',  TxID = ' || h_tx_id2 || ' ] )' );
                   end if;

                end loop;
                close c1;
            end ;

-------------------------------
-- print information of table-usage
-------------------------------
            system_.println('         TableSpace ID = ' || h_spaceid   ||  
                            ', Name: ' || trim(h_spacename) );
            system_.println('         Used portion  = ' || h_rate      || '%' || 
                            ' , (Total PageCount = '    || h_totalpage || ')' || 
                            ' , Used PageCount = '      || h_pagecnt);

            cursor_count := cursor_count  + 1;
        end loop;

        close ex1;
    end; 

end;
/
