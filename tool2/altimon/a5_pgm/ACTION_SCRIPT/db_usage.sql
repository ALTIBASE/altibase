set linesize 1000
select
    trunc(mem_max_db_size/1024/1024, 2) as max_mem_mb,
    trunc(mem_alloc_page_count*8192/1024/1024, 2) as alloc_mem_mb,
    trunc(mem_free_page_count*8192/1024/1024, 2) as free_mem_mb
from v$database;
