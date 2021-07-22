create or replace procedure dbinfo(seed varchar(40))
as
  h_vinfo  char(256);
  h_dbname char(256);
  h_prosig char(256);
  h_dbsig  char(256);
  h_bit    integer;
  h_log    bigint;
  h_dbsize bigint;
  h_count1 integer;
  h_count2 integer;
  h_max    varchar(256);
  h_alloc  bigint;
  h_free   bigint;

  h_chk1   integer;
  h_chk2   integer;

  ttname   varchar(40);


  cursor cc1 is
        select A.table_name from system_.sys_tables_ A, v$memtbl_info b
        where a.table_oid = b.table_oid order by A.table_name asc;
begin
system_.println(' ');

--------------------------------------
-- get db information
--------------------------------------
    select 
             VERSION_ID , 
             DB_NAME, 
             PRODUCT_SIGNATURE, 
             DB_SIGNATURE,
             COMPILE_BIT,
             LOGFILE_SIZE,
             MEM_DBFILE_SIZE,
             MEM_DBFILE_COUNT_0,
             MEM_DBFILE_COUNT_1,
             NVL(MEM_MAX_DB_SIZE, '4G'),
             MEM_ALLOC_PAGE_COUNT*32000,
             MEM_FREE_PAGE_COUNT*32000
    into 
             h_vinfo, 
             h_dbname, 
             h_prosig, 
             h_dbsig,
             h_bit,
             h_log,
             h_dbsize,
             h_count1,
             h_count2,
             h_max,
             h_alloc,
             h_free
    from v$database;

    -- h_max   := h_max / 1024 / 1024;
    h_alloc := h_alloc / 1024 / 1024;
    h_free  := h_free / 1024 / 1024;

-----------------------------------------------------
-- print db info when 'status db all' or 'status db'
-----------------------------------------------------
    if seed = '0' or seed = '1'
    then
        system_.println('<< Database Info >>');
        system_.println('Version Info    = ' || trim(h_vinfo));
        system_.println('Database Name   = ' || trim(h_dbname));
        system_.println('Product info    = ' || trim(h_prosig));
        system_.println('Storage info    = ' || trim(h_dbsig));
        system_.println('OS/Bit ENV      = ' || trim(h_bit));
        system_.println('Log File Size   = ' || trim(h_log));
        system_.println('DB File Size    = ' || trim(h_dbsize));
        system_.println('DB File Count   = (First ) = ' || trim(h_count1));
        system_.println('DB File Count   = (Second) = ' || trim(h_count2));
        system_.println('');
        system_.println('<<  Database Usage >>');
        system_.println('Maximum   Page  = ' || trim(h_max));
        system_.println('Allocated Page  = ' || trim(h_alloc) || 'M');
        system_.println('Used      Page  = ' || trim(h_alloc - h_free) || 'M');
        system_.println('Free      Page  = ' || trim(h_free) || 'M');
        system_.println('----------------------------------------------------------');
        system_.println(' ');
        return;
    end if;


-------------------------------------------
-- if user put table_name with input-param
-------------------------------------------
    begin 
        select count(*) into h_chk1 from v$memtbl_info 
        where table_oid  = (select table_oid from system_.sys_tables_
                            where table_name = upper(seed) limit 1
                           );
        exception when NO_DATA_FOUND then
               NULL;
    end;
 
    begin
        select count(*) into h_chk2 from v$disktbl_info 
        where table_oid  = (select table_oid from system_.sys_tables_
                            where table_name = upper(seed) limit 1
                           );
        exception when NO_DATA_FOUND then
              NULL;
    end;    

------------------------------------------
-- if table not found
------------------------------------------
    if h_chk1 = 0 and h_chk2 = 0
    then
         system_.println('Table does not exist !! ');
         return;
    end if;

------------------------------------------
-- if table exists,
------------------------------------------
    if h_chk1 = 1 then
        mtinfo(seed);
    else
        dtinfo(seed);
    end if;

system_.println(' ');
end;
/

