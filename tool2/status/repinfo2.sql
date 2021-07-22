create or replace procedure repinfo2 (rep_names varchar(40))
as
h_local_user    char(40);
h_local_name    char(40);
h_remote_user   char(40);
h_remote_name   char(40);
h_lcount        integer;
begin
    h_lcount := 1;

    system_.println('    +++++++++++++++ replication table list ++++++++++++++++++++' );
    declare cursor c2 is
        select LOCAL_TABLE_NAME, REMOTE_TABLE_NAME, LOCAL_USER_NAME, REMOTE_USER_NAME
        from system_.sys_repl_items_
        where replication_name = trim(rep_names);
        
    begin
        open c2;      
        loop          
            fetch c2 into h_local_name, h_remote_name, h_local_user, h_remote_user;
            exit when c2%notfound;
                          
            system_.println('     ' || h_lcount || '. LOCAL From (' || 
                            trim(h_local_user) || '.' || trim(h_local_name) ||
                            ') To Remote (' || 
                            trim(h_remote_user) || '.' || trim(h_remote_name) || ')' );
            h_lcount := h_lcount + 1;
        end loop;     
        close c2;     
    end;
end;
/

