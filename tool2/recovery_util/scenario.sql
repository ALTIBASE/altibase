--##########################################################################
--# BY AUTO RECOVERY TOOL
--##########################################################################

--#########################################
--# Preparation
--#########################################

select count(*) from d1;
select count(*) from d2;
select count(*) from d3;
select count(*) from d4;
select count(*) from m1;
select count(*) from m2;

drop tablespace mytbs1 INCLUDING CONTENTS AND DATAFILES;
drop tablespace mytbs2 INCLUDING CONTENTS AND DATAFILES;

DROP SEQUENCE SEQ;
DROP TABLE d1;
DROP TABLE d2;
DROP TABLE d3;
DROP TABLE d4;
DROP TABLE m1;
DROP TABLE m2;
DROP TABLE T1;
DROP TABLE T2;
DROP TABLE T3;

--#########################################
--#  use temp table 
--#########################################

ALTER SESSION SET EXPLAIN PLAN = ON;

CREATE TABLE T1 ( I1 INTEGER, I2 INTEGER ) TABLESPACE SYS_TBS_DATA;
CREATE TABLE T2 ( I1 INTEGER, I2 INTEGER ) TABLESPACE SYS_TBS_DATA;
CREATE TABLE T3 ( I1 INTEGER, I2 INTEGER ) TABLESPACE SYS_TBS_DATA;


INSERT INTO T1 VALUES (   1,    1);
INSERT INTO T1 VALUES (   0,    2);
INSERT INTO T1 VALUES (   3,    0);
INSERT INTO T1 VALUES (   1,    4);
INSERT INTO T1 VALUES (   2,    5);
INSERT INTO T1 VALUES (NULL,    2);
INSERT INTO T1 VALUES (   1,    1);
INSERT INTO T1 VALUES (   2, NULL);

INSERT INTO T2 VALUES (   1,    1);
INSERT INTO T2 VALUES (   2,    1);
INSERT INTO T2 VALUES (   1,    1);
INSERT INTO T2 VALUES (   1,    4);
INSERT INTO T2 VALUES (   3,    2);
INSERT INTO T2 VALUES (   2,    3);
INSERT INTO T2 VALUES (   1,    2);
INSERT INTO T2 VALUES (   2,    1);

INSERT INTO T3 VALUES (   1,    1);
INSERT INTO T3 VALUES (   3,    0);
INSERT INTO T3 VALUES (   2, NULL);
INSERT INTO T3 VALUES (   1,    4);
INSERT INTO T3 VALUES (   2,    5);
INSERT INTO T3 VALUES (NULL,    0);
INSERT INTO T3 VALUES (   1,    1);
INSERT INTO T3 VALUES (   0,    2);


SELECT * FROM T1, T3 WHERE T1.I1 = T3.I1
         AND T1.I2 IN ( SELECT MAX(I2)
                        FROM T2
                        WHERE I1 = T1.I1
                        GROUP BY I1 );
SELECT * FROM T1, T3 WHERE T1.I1 = T3.I1
         AND T1.I2 IN ( SELECT MAX(I2)
                        FROM T2
                        WHERE I1 = T2.I1 GROUP BY I1 )
         AND T3.I2 > 0;
SELECT * FROM T1, T3 WHERE T1.I1 = T3.I1
         AND T3.I2 > 0
         AND T1.I2 IN ( SELECT MAX(I2)
                        FROM T2
                        WHERE ( I1 = T1.I1 OR I2 = T3.I1 )
                        GROUP BY I1);

SELECT * FROM T1 WHERE T1.I2 IN ( SELECT MAX(I2)
                                  FROM T2
                                  WHERE I1 > 0
                                  AND I1 = T1.I1
                                  GROUP BY I1 ) AND T1.I1 > 0;

SELECT * FROM T1 WHERE T1.I2 IN ( SELECT MAX(T3.I2)
                                  FROM T2, T3
                                  WHERE T2.I1 = T3.I1
                                  AND T3.I1 = T1.I1
                                  GROUP BY T2.I2 ) AND T1.I1 > 0;
SELECT * FROM T1 WHERE T1.I2 IN ( SELECT MAX(T2.I2)
                                  FROM T2, T3
                                  WHERE T2.I1 = T3.I1 AND T3.I2 > 0
                                  AND ( T2.I1 = T1.I1 OR T3.I1 = T1.I2 )
                                  GROUP BY T3.I2 ) AND T1.I1 > 0;

SELECT * FROM T1 A
         WHERE A.I2 > 0
         AND A.I2 IN ( SELECT MAX(B.I2)
                       FROM T1 B
                       WHERE B.I1 = A.I1
                       AND B.I1 > 0
                       AND B.I1 IN ( SELECT MAX(C.I1)
                                 FROM T1 C
                                 WHERE C.I2 > 0
                                 AND ( C.I1 = B.I1 OR C.I2 = A.I2 )
                                 GROUP BY C.I2 )
                       GROUP BY I1 );


CREATE INDEX T1_I1 ON T1(I1);        
CREATE INDEX T2_I1 ON T2(I1);
CREATE INDEX T3_I1 ON T3(I1);
CREATE INDEX T1_I2 ON T1(I2);
CREATE INDEX T2_I2 ON T2(I2);
CREATE INDEX T3_I2 ON T3(I2);

SELECT * FROM T1 WHERE T1.I2 IN ( SELECT MAX(I2)
                                  FROM T2
                                  WHERE I1 > 0
                                  AND I1 = T1.I1
                                  GROUP BY I1 ) AND T1.I1 > 0;
SELECT * FROM T1 WHERE T1.I2 IN ( SELECT MAX(T3.I2)
                                  FROM T2, T3
                                  WHERE T2.I1 = T3.I1
                                  AND T3.I1 = T1.I1
                                  GROUP BY T2.I2 ) AND T1.I1 > 0;
SELECT * FROM T1 WHERE T1.I2 IN ( SELECT MAX(T2.I2)
                                  FROM T2, T3
                                  WHERE T2.I1 = T3.I1 AND T3.I2 > 0
                                  AND ( T2.I1 = T1.I1 OR T3.I1 = T1.I2 )
                                  GROUP BY T3.I2 ) AND T1.I1 > 0;

DROP INDEX T1_I1;
DROP INDEX T2_I1;
DROP INDEX T3_I1;
DROP INDEX T1_I2;
DROP INDEX T2_I2;
DROP INDEX T3_I2;

ALTER SESSION SET EXPLAIN PLAN = OFF;
                     
--#########################################
--#  tablespace 의 생성
--#########################################

create tablespace mytbs1 datafile 'mytbs1.dbf' size 1M;
alter tablespace mytbs1 add datafile 'mytbs1_1.dbf', 'mytbs1_2.dbf' size 1M;
alter tablespace mytbs1 add datafile 'mytbs1_3.dbf' size 1M;

alter tablespace mytbs1 alter datafile 'mytbs1_2.dbf' size 10M;
alter tablespace mytbs1 alter datafile 'mytbs1_3.dbf' size 10M;
alter tablespace mytbs1 alter datafile 'mytbs1_3.dbf' size 2M;
alter tablespace mytbs1 alter datafile 'mytbs1_1.dbf' autoextend on;

create table d0 (i1 double, i2 double, i3 char(640)) tablespace mytbs1;

insert into d0 values (1.0, 3.7, 'test');
insert into d0 values (3.19, 2.11, 'test');
insert into d0 values (2.2, 2, 'test');
insert into d0 values (7.09, 0.91, 'test');
insert into d0 values (0, 9.11, 'test');
insert into d0 values (2, 5, 'test');
insert into d0 values (8.18, 918, 'test');
insert into d0 values (2, 2, 'test');
insert into d0 values (10.2, 15, 'test');

insert into d0 select * from d0;
insert into d0 select * from d0;
insert into d0 select * from d0;
insert into d0 select * from d0;
insert into d0 select * from d0;
insert into d0 select * from d0;
insert into d0 select * from d0;
insert into d0 select * from d0;
insert into d0 select * from d0;
insert into d0 select * from d0;

drop table d0;

alter tablespace mytbs1 drop datafile 'mytbs1_3.dbf';
alter tablespace mytbs1 drop datafile 'mytbs1_3.dbf';
alter tablespace mytbs1 alter datafile 'mytbs1_1.dbf' autoextend on;

create tablespace mytbs2 datafile 'mytbs2_1.dbf'size 3M reuse;
alter tablespace mytbs2 add datafile 'mytbs2_2.dbf' size 5M, 'mytbs1_3.dbf'size 1M;

--#########################################
--#  Disk Table 의 생성
--#########################################

create table d1 ( i1 double, i2 double ) tablespace sys_tbs_data;

insert into d1 values (1.0, 3.7);
insert into d1 values (3.19, 2.11);
insert into d1 values (2.2, 2);
insert into d1 values (7.09, 0.91);
insert into d1 values (0, 9.11);
insert into d1 values (2, 5);
insert into d1 values (8.18, 918);
insert into d1 values (2, 2);
insert into d1 values (10.2, 15);

insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;

--#########################################
--# PR-7775
--#  Disk Table 에 대한 Full Scan Test
--#########################################

select * from d1;

--#########################################
--# PR-7776
--#  Disk Table 에 대한 Filtering
--#########################################

select * from d1 where i1 > 5;
select * from d1 where i1 = 2;

--#########################################
--# PR-7778
--#  Disk Table 에 대한 Index Scan
--#########################################

--# PR-7816
create index d1_i1 on d1(i1);
select * from d1 where i1 > 5;
select * from d1 where i1 = 2;

--#########################################
--# PR-7472
--# 실수형 Type을 Numeric 형태로 표현
--#########################################

select i1 * i2 from d1;
select i1 / i2 from d1;

--#########################################
--# UPDATE
--#########################################

create table d2 ( i1 integer, i2 bigint ) tablespace sys_tbs_data;
create index d2_i1 on d2(i1);

insert into d2 values ( 3, 7 );
insert into d2 values ( 4, 7 );
insert into d2 values ( 9, 7 );

--# PR-7821
update d2 set i2 = 0;
select * from d2;
update d2 set i2 = 1 where i1 = 4;
select * from d2;

--#########################################
--# DELETE
--#########################################

--# PR-7819
delete from d2;
select * from d2;
delete from d1 where i1 = 2;
select * from d1;

--#########################################
--# INSERT .. SELECT
--#########################################

truncate table d2;
insert into d2 values ( 3, 7 );
insert into d2 values ( 2, 5 );
--# PR-7820
insert into d2 select * from d2;
select * from d2;

truncate table d1;
insert into d1 values ( 1.9, 2.1 );
insert into d1 values ( 0.9, 21.1 );
--# PR-7820
insert into d1 select * from d1;
insert into d1 select * from d1 where i1 > 1;

select * from d1;

--#########################################
--# Primary Key 
--#########################################

drop index d2_i1;
!sleep 3

drop table d2;

create table d2 ( i1 integer primary key, i2 bigint ) tablespace sys_tbs_data;
create sequence SEQ NOCACHE;

insert into d2 values ( 3, 7 );
insert into d2 values ( 4, 7 );
insert into d2 values ( 9, 7 );
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);
insert into d2 values ( SEQ.NEXTVAL, 7);

select * from d2 where i1 > 4;
select * from d2 where i1 = 4;

--#########################################
--# 올바른 Index 선택
--#########################################

drop table d1;
create table d1 ( i1 integer, i2 integer ) tablespace sys_tbs_data;

insert into d1 values ( 1, 1 );
insert into d1 values ( 2, 1 );
insert into d1 values ( 3, 1 );
insert into d1 values ( 4, 2 );
insert into d1 values ( 5, 2 );
insert into d1 values ( 6, 2 );
insert into d1 values ( 7, 3 );
insert into d1 values ( 8, 3 );
insert into d1 values ( 9, 3 );

insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;

create index d1_i2 on d1(i2);
create index d1_i1 on d1(i1);

select * from d1;

select * from d1 where i1 = 3 and i2 = 1;
select * from d1 where i1 > 7 and i2 = 3;
select * from d1 where i1 > 1 and i2 = 3;

drop index d1_i1;
drop index d1_i2;

--#########################################
--# composit index 생성 
--#########################################

drop table d1;

create table d1 ( i1 integer, i2 integer, i3 integer ) tablespace sys_tbs_data;
create index d1_i1_i2_i3 on d1( i1, i2, i3 );

insert into d1 values ( 1, 1, 1 );
insert into d1 values ( 1, 1, 2 );
insert into d1 values ( 1, 1, 3 );
insert into d1 values ( 1, 2, 1 );
insert into d1 values ( 1, 2, 2 );
insert into d1 values ( 1, 2, 3 );

insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;
insert into d1 select * from d1;

select * from d1 where i1 = 1;
select * from d1 where i1 = 1 and i2 = 1;
select * from d1 where i1 = 1 and i2 = 2 and i3 = 3;
select * from d1 where i1 = 1 and i3 = 3;

--#########################################
--# transaction commit/rollback
--#########################################

autocommit off;
create table d3 ( i1 integer, i2 varchar(128), i3 char(128)) tablespace sys_tbs_data;

create index d3_i2 on d3(i2);
create index d3_i1 on d3(i1);

insert into d3 values ( 1, 'KOREA', '1-KOREA' );
insert into d3 values ( 2, 'KOREA', '1-KOREA' );
insert into d3 values ( 3, 'KOREA', '1-KOREA' );
insert into d3 values ( 4, 'KOREA', '1-KOREA' );
insert into d3 values ( 5, 'KOREA', '1-KOREA' );

insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;

update d3 set i1 = SEQ.NEXTVAL+1, i2 = 'ALTIBASE IV HYBRID DATABASE';
update d3 set i1 = SEQ.NEXTVAL+2, i2 = 'ALTIBASE IV HYBRID DATABASE SYSTEM';

delete from d3 where i1 < 50;

insert into d3 select * from d3;
insert into d3 select * from d3;

commit;

drop table d3;
create table d3 ( i1 integer, i2 varchar(128), i3 char(128)) tablespace sys_tbs_data;

create index d3_i2 on d3(i2);
create index d3_i1 on d3(i1);

insert into d3 values ( 1, 'KOREA', '1-KOREA' );
insert into d3 values ( 2, 'KOREA', '1-KOREA' );
insert into d3 values ( 3, 'KOREA', '1-KOREA' );
insert into d3 values ( 4, 'KOREA', '1-KOREA' );
insert into d3 values ( 5, 'KOREA', '1-KOREA' );

insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;

update d3 set i1 = SEQ.NEXTVAL+1, i2 = 'ALTIBASE IV HYBRID DATABASE';
update d3 set i1 = SEQ.NEXTVAL+2, i2 = 'ALTIBASE IV HYBRID DATABASE SYSTEM';

delete from d3 where i1 < 50;

insert into d3 select * from d3;
insert into d3 select * from d3;

rollback;

autocommit on;
drop table d3;
create table d3 ( i1 integer, i2 varchar(128), i3 char(128)) tablespace sys_tbs_data;

insert into d3 values ( 1, 'KOREA', '1-KOREA' );
insert into d3 values ( 2, 'KOREA', '1-KOREA' );
insert into d3 values ( 3, 'KOREA', '1-KOREA' );
insert into d3 values ( 4, 'KOREA', '1-KOREA' );
insert into d3 values ( 5, 'KOREA', '1-KOREA' );

insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;
insert into d3 select * from d3;

update d3 set i1 = SEQ.NEXTVAL+1, i2 = 'ALTIBASE IV HYBRID DATABASE';
update d3 set i1 = SEQ.NEXTVAL+2, i2 = 'ALTIBASE IV HYBRID DATABASE SYSTEM';

delete from d3 where i1 < 50;

insert into d3 select * from d3;
insert into d3 select * from d3;

select count(*) from d3 where i1 > 0;
select count(*) from d3 where i2 like 'ALTIBASE%';

create table m2 ( i1 integer, i2 varchar(128), i3 char(128), i4 double);

insert into M2 values ( 1, 'KOREA', '1-KOREA', 1.10);
insert into M2 values ( 2, 'KOREA', '1-KOREA', 2.10);
insert into M2 values ( 3, 'KOREA', '1-KOREA', 2.98);
insert into M2 values ( 4, 'KOREA', '1-KOREA', 1.10);
insert into M2 values ( 5, 'KOREA', '1-KOREA', 1.10);

insert into M2 select * from M2;
insert into M2 select * from M2;
insert into M2 select * from M2;
insert into M2 select * from M2;
insert into M2 select * from M2;
insert into M2 select * from M2;
insert into M2 select * from M2;

update M2 set i1 = SEQ.NEXTVAL+1, i2 = 'ALTIBASE IV HYBRID DATABASE';
update M2 set i1 = SEQ.NEXTVAL+2, i2 = 'ALTIBASE IV HYBRID DATABASE SYSTEM';

delete from M2 where i1 < 50;

insert into M2 select * from M2;
insert into M2 select * from M2;

alter table m2 add column (i5 integer default 0);

--#########################################
--# checkpoint 수행
--#########################################

alter system checkpoint;
alter system checkpoint;

--#########################################
--#  temp table 사용
--#########################################

CREATE TABLE M1 (
    I1 VARCHAR(31) VARIABLE,
    I2 DOUBLE, 
    I3 VARCHAR(41) VARIABLE,
    I4 INTEGER )
TABLESPACE SYS_TBS_MEMORY
;

CREATE TABLE D3 (
    I1 VARCHAR(31) VARIABLE,
    I2 DOUBLE, 
    I3 VARCHAR(41) VARIABLE,
    I4 INTEGER )
TABLESPACE SYS_TBS_DATA
;

CREATE TABLE D4 (
    I1 VARCHAR(31) VARIABLE,
    I2 DOUBLE,
    I3 VARCHAR(41) VARIABLE,
    I4 INTEGER )
TABLESPACE SYS_TBS_DATA 
;

INSERT INTO D3 VALUES ( 'Writing Solid Code', 4.82,   '  data mining',    9 );
INSERT INTO D3 VALUES (      'Garcia-Molina', 2.09,   'Prentice Hall',    9 );
INSERT INTO D3 VALUES ( 'Writing Solid Code', NULL,     'data mining',    2 );
INSERT INTO D3 VALUES (                 NULL, 1.77,     'data mining',    9 );
INSERT INTO D3 VALUES (            'kapamax', 4.82, 'modern database', NULL );
 
INSERT INTO D4 VALUES ( 'Writing Solid Code', 1.77,      'Han Kamber',    9 );
INSERT INTO D4 VALUES (      'Garcia-Molina', 1.77, 'modern database',    4 );
INSERT INTO D4 VALUES ( 'Writing Solid Code', NULL,              NULL,    4 );
INSERT INTO D4 VALUES (                 NULL, 6.23, 'modern database', NULL );

--# Disk Temp Table로부터 Hash Key값 획득

SELECT DISTINCT I1 FROM D3;
SELECT DISTINCT I2 FROM D3;

SELECT I4, I1 FROM D3
 INTERSECT 
SELECT I4, I1 FROM D4;

SELECT D3.I2, D4.I2, D3.I1, D4.I1 FROM D4, D3 WHERE D3.I2 = D4.I2;

SELECT /*+ USE_HASH( D4, D3 ) */ D3.I2, D4.I2, D3.I1, D4.I1
  FROM D3, D4 WHERE D3.I2 = D4.I2;

--# Memory Temp Table

SELECT * FROM D3 ORDER BY D3.I3 LIMIT 1;
SELECT * FROM D3 ORDER BY D3.I4 LIMIT 9;

SELECT /*+ TEMP_TBS_MEMORY */ DISTINCT I1 FROM D3;
SELECT /*+ TEMP_TBS_MEMORY */ DISTINCT I2 FROM D3;
SELECT /*+ TEMP_TBS_MEMORY */ DISTINCT I1, I4 FROM D3;
SELECT /*+ TEMP_TBS_MEMORY */ DISTINCT I1, I3, I1 FROM D3;

SELECT /*+ TEMP_TBS_MEMORY */ DISTINCT D3.I1, D4.I1 FROM D3, D4;
SELECT /*+ TEMP_TBS_MEMORY */ DISTINCT D3.I2, D4.I2 FROM D3, D4;

SELECT /*+ TEMP_TBS_MEMORY */ D3.I2, D4.I2, D3.I1, D4.I1
  FROM D4, D3 WHERE D3.I2 = D4.I2;

SELECT /*+ USE_HASH( D4, D3 ) TEMP_TBS_MEMORY */ D3.I2, D4.I2, D3.I1, D4.I1
  FROM D3, D4 WHERE D3.I2 = D4.I2;

SELECT /*+ USE_TWO_PASS_HASH( D4, D3, 4 ) TEMP_TBS_MEMORY */
  D3.I2, D4.I2, D3.I1, D4.I1
  FROM D3, D4 WHERE D3.I2 = D4.I2;

--#########################################
--# Finalization
--#########################################

DROP SEQUENCE SEQ;
drop table d0;
DROP TABLE d1;
DROP TABLE d2;
DROP TABLE d3;
DROP TABLE d4;
DROP TABLE M1;
DROP TABLE M2;
DROP TABLE T1;
DROP TABLE T2;
DROP TABLE T3;

drop tablespace mytbs1 including contents and datafiles;
drop tablespace mytbs2 including contents and datafiles;
