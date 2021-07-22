drop user altimon cascade;
create user altimon identified by altimon;
drop tablespace altimon_tbs including contents and datafiles;
create tablespace altimon_tbs datafile 'altimon1.dbf' size 100M autoextend on ;
alter user altimon  access altimon_tbs on;
drop table altimon_log;
create table altimon_log
(
    sitename  varchar(100),
    intime char(14),
    logType    char(1),
    tagName    varchar(255),
    tagValue   varchar(1024)
) tablespace altimon_tbs;
create index idx1_altimon_log on altimon_log (sitename, intime);
create index idx2_altimon_log on altimon_log (sitename, intime, logType);
create index idx3_altimon_log on altimon_log (sitename, intime, logType, tagName);
create index idx4_altimon_log on altimon_log (sitename, tagName);

