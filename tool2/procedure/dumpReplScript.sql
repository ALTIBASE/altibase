create or replace procedure dumpReplScript(p1 varchar(40))
as

-- 해당 리플리케이션의 생성  스크립터를 화면에 푸려 줍니다.
-- exec dumpReplScript('replication_name');
    cursor c1 is
    select a.replication_name, b.host_ip, b.port_no, a.item_count
    from system_.sys_replications_ a, system_.sys_repl_hosts_ b
    where a.replication_name = UPPER(P1)
        AND a.replication_name = b.replication_name ;

    r_name varchar(40);
    r_ip varchar(40);
    r_port varchar(20);
    r_item_cnt integer;

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
        SYSTEM_.PRINTLN('');

        loop
            fetch C1 into r_name, r_ip, r_port, r_item_cnt;
            exit when C1%NOTFOUND;
            SYSTEM_.PRINT(' CREATE REPLICATION ');
            SYSTEM_.PRINT(r_name);
            SYSTEM_.PRINT(' WITH ''');
            SYSTEM_.PRINT(r_ip);
            SYSTEM_.PRINT(''',');
            SYSTEM_.PRINT(r_port);
            SYSTEM_.PRINTLN(' ');

            open c2;
            for i in 1 .. r_item_cnt loop
                fetch c2 into r_local_user_name, r_local_table_name, r_remote_user_name, r_remote_table_name;
                SYSTEM_.PRINT(' FROM ');
                SYSTEM_.PRINT(r_local_user_name);
                SYSTEM_.PRINT('.');
                SYSTEM_.PRINT(r_local_table_name);
                SYSTEM_.PRINT(' TO ');
                SYSTEM_.PRINT(r_remote_user_name);
                SYSTEM_.PRINT('.');
                SYSTEM_.PRINT(r_remote_table_name);

                if i <> r_item_cnt then
                    SYSTEM_.PRINTLN(',');
                else
                    SYSTEM_.PRINTLN(';');
                end if;
            end loop;
            close c2;
        end loop;

        close c1;
        SYSTEM_.PRINTLN('');
        SYSTEM_.PRINTLN('----------------------------------------------------------');
end;
/
