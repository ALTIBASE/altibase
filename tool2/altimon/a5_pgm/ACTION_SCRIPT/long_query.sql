iset linesize 1000
select b.session_id as sid,
       a.comm_name as ip,
       b.id as stmt_id,
       trunc(b.EXECUTE_TIME/1000000, 4) as rtime,
       trunc(b.FETCH_TIME/1000000, 4) as ftime,
       trunc(b.TOTAL_TIME/1000000, 4) as ttime,
       execute_success
--     , query
from  v$session a, v$statement b
where  a.id = b.session_id
--   and comm_name like '%211.115.75.187%'
--   and CLIENT_CONSTR='JDBC'
   and b.EXECUTE_TIME/1000000 > 0
order by ttime;

