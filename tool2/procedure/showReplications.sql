create or replace procedure showReplications
as
-- 리플리케이션 이름과 정보를 보여 줍니다.
-- exec showReplications;

    cursor c1 is
    select a.replication_name, b.host_ip, b.port_no,
        decode(a.is_started,1,'Running',0,'Not Running')
    from system_.sys_replications_ a, system_.sys_repl_hosts_ b
    WHERE a.replication_name = b.replication_name;

    r_name varchar(40);
    r_ip varchar(40);
    r_port varchar(20);
    r_status varchar(20);

    r_local_user_name varchar(40);
    r_local_table_name varchar(40);
    r_remote_user_name varchar(40);
    r_remote_table_name varchar(40);


    cursor c2 is
    select a.local_user_name, a.local_table_name, a.remote_user_name, a.remote_table_name
    from system_.sys_repl_items_ a
    where a.replication_name = r_name;

    begin
        open c1;
        SYSTEM_.PRINTLN('----------------------------------------------------------');
        SYSTEM_.PRINTLN('     Replications   Infos');
        SYSTEM_.PRINTLN('----------------------------------------------------------');
        SYSTEM_.PRINTLN(' Name                 Ip             Port       Status');
        SYSTEM_.PRINTLN('----------------------------------------------------------');
        SYSTEM_.PRINTLN('');
        loop
            fetch C1 into r_name, r_ip, r_port, r_status;
            exit when C1%NOTFOUND;

            SYSTEM_.PRINT(' ');
            SYSTEM_.PRINT(r_name);
            SYSTEM_.PRINT('              ');
            SYSTEM_.PRINT(r_ip);
            SYSTEM_.PRINT('        ');
            SYSTEM_.PRINT(r_port);
            SYSTEM_.PRINT('      ');
            SYSTEM_.PRINTLN(r_status);

            SYSTEM_.PRINTLN('  ++++++++++++++++++++++++++++++++++++++++++++++++++++');
            SYSTEM_.PRINTLN('    Local Table Name               Remote Table Name');
            SYSTEM_.PRINTLN('  ++++++++++++++++++++++++++++++++++++++++++++++++++++');
            open c2;
            loop
                fetch c2 into r_local_user_name, r_local_table_name, r_remote_user_name, r_remote_table_name;
                exit when C2%NOTFOUND;

                SYSTEM_.PRINT('         ');
                SYSTEM_.PRINT(r_local_user_name);
                SYSTEM_.PRINT('.');
                SYSTEM_.PRINT(r_local_table_name);
                SYSTEM_.PRINT('                          ');
                SYSTEM_.PRINT(r_remote_user_name);
                SYSTEM_.PRINT('.');
                SYSTEM_.PRINTLN(r_remote_table_name);
            end loop;
            close c2;
        end loop;
        close c1;

        SYSTEM_.PRINTLN('');
        SYSTEM_.PRINTLN('----------------------------------------------------------');
    end;
/
