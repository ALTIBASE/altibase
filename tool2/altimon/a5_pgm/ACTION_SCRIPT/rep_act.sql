set linesize 1000
select replication_name, is_started from system_.sys_replications_;
select rep_name, rep_gap from v$repgap;
