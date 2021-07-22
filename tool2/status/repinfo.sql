create or replace procedure repinfo(in_param integer)
as

h_rep_name             char(40);
h_rep_last_fileno      integer;
h_rep_last_offset      integer;
h_rep_fileno           integer;
h_rep_offset           integer;
h_rep_gap              integer;
h_sync_table           char(40);
h_sync_record_count    integer;
h_sync_fileno          integer;
h_sync_offset          integer;

h_local_name           char(40);
h_remote_name          char(40);

begin
system_.println(' ');
---------------------------------------------
-- display replication_gap 
---------------------------------------------
    declare cursor c1 is
    select distinct
           rep_name, 
           rep_last_fileno,
           rep_last_offset,
           rep_fileno,
           rep_offset,
           rep_gap
           -- sync_table,
           -- sync_record_count,
           -- sync_fileno,
           -- sync_offset
    from v$repgap;

    begin

        open c1;
        loop
            fetch c1 into h_rep_name,
                          h_rep_last_fileno,
                          h_rep_last_offset,
                          h_rep_fileno,
                          h_rep_offset,
                          h_rep_gap;
                          -- h_sync_table,
                          -- h_sync_record_count,
                          -- h_sync_fileno,
                          -- h_sync_offset;
           exit when c1%notfound;
               
           system_.println ('RepName = ' || trim(h_rep_name) || ' ,  Gap = ' || h_rep_gap || ' byte');
           system_.println ('    [ LOCAL ] : LastLSNFile = ' || h_rep_last_fileno ||
                            '   ,   LastLSNOffset = ' || h_rep_last_offset);
           system_.println ('    [ XLOG  ] : XLNSFile    = ' || h_rep_fileno ||
                            '   ,   XLSNOffset    = ' || h_rep_offset);
          -- system_.println ('    SYNCTable   = ' || trim(h_sync_table) ||
          --                  ',  SYNCCount = ' || h_sync_record_count ||
          --                   ',  SYNCFileNo = ' || h_sync_fileno ||
          --                   ', SYNCOffset = ' || h_sync_offset );

---------------------------------------------
-- display replication_table list if you want
---------------------------------------------
           if in_param = 1 
           then
              repinfo2(h_rep_name);
           end if; 
        end loop;
        close c1;
    end;
           
system_.println(' ');
end;
/

