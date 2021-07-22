create or replace procedure ssinfo(session_id varchar(3000))
as 
now_count          integer;

tmp_x              bigint;
ssid               bigint;
comm_name          char(256);
client_pid         bigint;
autocommit         char(40);
iso_level          integer;
state              char(256);
protocol   	       char(256);
hcurrent_stmt_id   bigint;
hidle_start        integer;
hidle_time         integer;
idle_gap           integer;

hstmt_id           bigint;
htx_id             bigint;
hquery             char(1024);
hbase_time         bigint;
stmt_state         char(256);
stmt_start         integer;
stmt_fetch         integer;

start_gap          integer;
fetch_gap          integer;

cursor c1 is
    select ID, COMM_NAME, CLIENT_PID, 
           DECODE(AUTOCOMMIT_FLAG , '1', 'AutoCommit' , 'Non-AutoCommit'),
           ISOLATION_LEVEL,
           DECODE(PROTOCOL_SESSION, 1397051990, 'Handshake for sysdba dispatch',
                                    1094995278, 'Handshake for users dispatch', 
                                    1129270862, 'Connect', 
                                    1145652046, 'Disconnect', 
                                    1229870668, 'Invalid', 
                                    1280460613, 'Large', 
                                    1414352724, 'Timeout', 
                                    1163022162, 'Error', 
                                    1347568976, 'Prepare',
                                    1163412803, 'Execute', 
                                    1162103122, 'ExecDirect', 
                                    1179927368, 'Fetch', 
                                    1179796805, 'Free', 
                                    1229931858, 'I/O Error', 
                                    1161905998, 'EAGIN Error',
                                    1480672077, 'XA Commands',
                                    1094929998, 'Acknowledge', 
                                    1195725908, 'Get' ), 
           DECODE(CURRENT_STMT_ID, 1024, -1, CURRENT_STMT_ID),
           IDLE_START_TIME, IDLE_TIME_LIMIT,
           DECODE(SESSION_STATE, 0, 'Initializing', 
                      1, 'Connected for Auth', 
                      2, 'Autherization Init', 
                      3, 'Autherization Process', 
                      4, 'Autherization Complete', 
                      5, 'On Service', 
                      6, 'Disconnecting', 
                      7, 'Rollback & Disconnecting')
    from v$session order by id asc;

begin

    system_.println(' ');
    begin
        select to_number(session_id) into tmp_x from dual;
        exception when others then
        begin
            system_.println('[ERR-21011 : Invalid literal.], Check SessionID');
            return;
        end;
    end;
   
--------------------------------------
-- get base time
--------------------------------------
    select BASE_TIME into hbase_time FROM V$SESSIONMGR;

--------------------------------------
-- get total session count
--------------------------------------
   select count(*) into now_count from v$session;
   system_.println('Current Session Count = [' || now_count || ']');
   system_.println(' ');

--------------------------------------
-- if print total session info
--------------------------------------
   if session_id = 0
   then
      begin
         open c1;
         loop
            fetch c1 into ssid, comm_name, client_pid, 
                          autocommit, 
                          iso_level, 
                          protocol,
                          hcurrent_stmt_id, 
                          hidle_start, hidle_time,
                          state; 
            exit when c1%notfound ;

--------------------------------------
-- print session info
--------------------------------------
            system_.println('Session [' || ssid || '] <' || trim(comm_name) || 
                            '> <pid = ' || client_pid || '>');
            system_.println('        State           : ' || trim(state));
            system_.println('        Attribute       : ' || autocommit);
            system_.println('        Isolation Level : ' || iso_level || 
                            '            Current stmt ID : ' || hcurrent_stmt_id );
            system_.println('        Protocol        : [' || trim(protocol) || ']');

--------------------------------------
-- print idle Time
--------------------------------------
            if hidle_start > 0
            then
                idle_gap := hbase_time - hidle_start;
            else
                idle_gap := 0;
            end if;
            system_.println('        Idle Time       : ' || idle_gap || 
                                                   ' / ' || hidle_time);
--------------------------------------
-- print statement TXID 
--------------------------------------
            begin
                select tx_id into htx_id 
                from v$statement 
                where session_id = ssid and tx_id > 0 limit 1;
                exception when  NO_DATA_FOUND then
                         htx_id :=0;
            end;
            if htx_id > 0 then
               system_.println('        TxID : ' || htx_id);
            end if;

--------------------------------------
-- for get statement info 
--------------------------------------
            declare cursor c2 is 
            select id, query, 
                   DECODE(state , 0, 'State Init',
                                  1, 'Prepared'  ,
                                  2, 'Fetch Ready' ,
                                  3, 'Fetch Proceeding'),
                   query_start_time, fetch_start_time
            from v$statement where session_id = ssid;
 
            begin
               open c2;
               loop 
                  fetch c2 into hstmt_id, hquery, stmt_state, stmt_start, stmt_fetch;
                  exit when c2%notfound;

--------------------------------------
-- print statement information
--------------------------------------
                  if stmt_start > 0 
                  then
                     start_gap := hbase_time - stmt_start;
                  else
                     start_gap := 0;
                  end if;

                  if stmt_fetch > 0 
                  then
                     fetch_gap := hbase_time - stmt_fetch;
                  else
                     fetch_gap := 0;
                  end if;
    
                  system_.println('        Statement [' || hstmt_id || ']' || 
                                  ' : Mode = ' || trim(stmt_state) || 
                                  '   Time [Q=>' || start_gap || ', F=>' || fetch_gap || ']' );
                  system_.println('         => ' || trim(hquery));
               end loop;
               close c2;
            end;
system_.println(' ');
         end loop;
         close c1;
      end; 
   else
--------------------------------------
-- if print session info user request
--------------------------------------
     begin
       select ID, COMM_NAME, CLIENT_PID, 
           DECODE(AUTOCOMMIT_FLAG , '1', 'AutoCommit' , 'Non-AutoCommit'),
           ISOLATION_LEVEL,
           DECODE(PROTOCOL_SESSION, 1397051990, 'Handshake for sysdba dispatch',
                                    1094995278, 'Handshake for users dispatch', 
                                    1129270862, 'Connect', 
                                    1145652046, 'Disconnect', 
                                    1229870668, 'Invalid', 
                                    1280460613, 'Large', 
                                    1414352724, 'Timeout', 
                                    1163022162, 'Error', 
                                    1347568976, 'Prepare',
                                    1163412803, 'Execute', 
                                    1162103122, 'ExecDirect', 
                                    1179927368, 'Fetch', 
                                    1179796805, 'Free', 
                                    1229931858, 'I/O Error', 
                                    1161905998, 'EAGIN Error',
                                    1480672077, 'XA Commands',
                                    1094929998, 'Acknowledge', 
                                    1195725908, 'Get' ), 
           DECODE(CURRENT_STMT_ID, 1024, -1, CURRENT_STMT_ID),
           IDLE_START_TIME, IDLE_TIME_LIMIT,
           DECODE(SESSION_STATE, 0, 'Initializing', 
                      1, 'Connected for Auth', 
                      2, 'Autherization Init', 
                      3, 'Autherization Process', 
                      4, 'Autherization Complete', 
                      5, 'On Service', 
                      6, 'Disconnecting', 
                      7, 'Rollback & Disconnecting')
        into ssid, comm_name, client_pid, 
             autocommit, 
             iso_level, 
             protocol,
             hcurrent_stmt_id, 
             hidle_start, hidle_time,
             state
        from v$session 
        where id  = session_id;
        exception when NO_DATA_FOUND then
                system_.println('Session not found');
                return;
        end;        

        system_.println('Session [' || ssid || '] <' || trim(comm_name) || 
                            '> <pid = ' || client_pid || '>');
        system_.println('        State           : ' || trim(state));
        system_.println('        Attribute       : ' || autocommit);
        system_.println('        Isolation Level : ' || iso_level || 
                            '            Current stmt ID : ' || hcurrent_stmt_id );
        system_.println('        Protocol        : [' || trim(protocol) || ']');
--------------------------------------
-- print idle Time
--------------------------------------
        if hidle_start > 0
        then
                idle_gap := hbase_time - hidle_start;
            else
                idle_gap := 0;
        end if;
        system_.println('        Idle Time       : ' || idle_gap || 
                                                   ' / ' || hidle_time);
--------------------------------------
-- print statement TXID 
--------------------------------------
        begin
                select tx_id into htx_id 
                from v$statement 
                where session_id = ssid and tx_id > 0 limit 1;
                exception when  NO_DATA_FOUND then
                         htx_id :=0;
        end;
        if htx_id > 0 then
               system_.println('        TxID : ' || htx_id);
        end if;

--------------------------------------
-- for get statement info 
--------------------------------------
            declare cursor c2 is 
            select id, query, 
                   DECODE(state , 0, 'State Init',
                                  1, 'Prepared'  ,
                                  2, 'Fetch Ready' ,
                                  3, 'Fetch Proceeding'),
                   query_start_time, fetch_start_time
            from v$statement where session_id = ssid;
 
            begin
               open c2;
               loop 
                  fetch c2 into hstmt_id, hquery, stmt_state, stmt_start, stmt_fetch;
                  exit when c2%notfound;

--------------------------------------
-- print statement information
--------------------------------------
                  if stmt_start > 0 
                  then
                     start_gap := hbase_time - stmt_start;
                  else
                     start_gap := 0;
                  end if;

                  if stmt_fetch > 0 
                  then
                     fetch_gap := hbase_time - stmt_fetch;
                  else
                     fetch_gap := 0;
                  end if;

                  system_.println('        Statement [' || hstmt_id || ']' || 
                                  ' : Mode = ' || trim(stmt_state) || 
                                  '   Time [Q=>' || start_gap || ', F=>' || fetch_gap || ']' );
                  system_.println('         => ' || trim(hquery));
               end loop;
               close c2;
            end;
       system_.println(' ');
   end if;

end;
/

