set linesize 1000;
select * from v$memstat order by max_total_size desc;
select a.table_name, (b.fixed_alloc_mem+b.var_alloc_mem) alloc, 
       (b.fixed_alloc_mem+b.var_alloc_mem)-(b.fixed_used_mem+b.var_used_mem) free
from system_.sys_tables_ a, v$memtbl_info b
where a.table_oid = b.table_oid
order by 2 ;
