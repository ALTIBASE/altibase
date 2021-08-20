/***********************************************************************
 * Copyright 1999-2015, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

create or replace package body dbms_stats
as

procedure gather_system_stats()
as
begin
    system_.gather_system_stats();
end gather_system_stats;

procedure gather_database_stats( estimate_percent    in float   default 0,
                                 degree              in integer default 0,
                                 gather_system_stats in boolean default false,
                                 no_invalidate       in boolean default false )
AS
    V1 INTEGER;
    V2 VARCHAR(5);
    V3 BOOLEAN := FALSE;
BEGIN
    DECLARE
        CURSOR C1 IS SELECT U.USER_NAME AS OWNER,
                            T.TABLE_NAME AS TABLE_NAME
            FROM SYSTEM_.DBA_USERS_ AS U, SYSTEM_.SYS_TABLES_ AS T
                WHERE  U.USER_ID = T.USER_ID
                    AND TABLE_TYPE = 'T';
        TAB_REC C1%ROWTYPE;
    BEGIN

        IF GATHER_SYSTEM_STATS = TRUE THEN
            V1 := SP_GATHER_SYSTEM_STATS();
        END IF;

        V1 := DBMS_CONCURRENT_EXEC.INITIALIZE(DEGREE);
        V3 := TRUE;

        IF DEGREE <= 1 AND V1 <= 1 THEN
            PRINTLN('SERIAL EXECUTE');
            system_.gather_database_stats( estimate_percent,
                                           degree,
                                           gather_system_stats,
                                           no_invalidate );

            V1 := DBMS_CONCURRENT_EXEC.FINALIZE();
            RETURN;
        ELSE
            PRINTLN('PARALLEL EXECUTE DEGREE: '|| V1);
        END IF;

        IF NO_INVALIDATE = TRUE THEN
            V2 := 'TRUE';
        ELSE
            V2 := 'FALSE';
        END IF;

        OPEN C1;
            LOOP
                FETCH C1 INTO TAB_REC;
                EXIT WHEN C1%NOTFOUND;
                PRINTLN( TAB_REC.OWNER ||'.'||
                         TAB_REC.TABLE_NAME );
                V1 := DBMS_CONCURRENT_EXEC.REQUEST( 'GATHER_TABLE_STATS(''"'||TAB_REC.OWNER||'"'',''"'||TAB_REC.TABLE_NAME||'"'',NULL,'||
                                                     ESTIMATE_PERCENT||','||
                                                     DEGREE||','||
                                                     V2||')' );
                IF V1 = -1 THEN
                    EXIT;
                END IF;
            END LOOP;

            V1 := DBMS_CONCURRENT_EXEC.WAIT_ALL();

            IF DBMS_CONCURRENT_EXEC.GET_ERROR_COUNT() > 0 THEN
                FOR V1 in 0 .. DBMS_CONCURRENT_EXEC.GET_LAST_REQ_ID() LOOP
                    DBMS_CONCURRENT_EXEC.PRINT_ERROR( V1 );
                END LOOP;
            END IF;

        CLOSE C1;

        V3 := FALSE;
        V1 := DBMS_CONCURRENT_EXEC.FINALIZE();

    EXCEPTION
        WHEN OTHERS THEN
        IF V3 = TRUE THEN
            V1 := DBMS_CONCURRENT_EXEC.FINALIZE();
        END IF;
    END;
end gather_database_stats;

procedure gather_table_stats( ownname          in varchar(128),
                              tabname          in varchar(128),
                              partname         in varchar(128) default null,
                              estimate_percent in float   default 0,
                              degree           in integer default 0,
                              no_invalidate    in boolean default false )
as
begin
    system_.gather_table_stats( ownname,
                                tabname,
                                partname,
                                estimate_percent,
                                degree,
                                no_invalidate );
end gather_table_stats;

procedure gather_index_stats( ownname          in varchar(128),
                              idxname          in varchar(128),
                              estimate_percent in float   default 0,
                              degree           in integer default 0,
                              no_invalidate    in boolean default false )
as
begin
    system_.gather_index_stats( ownname,
                                idxname,
                                estimate_percent,
                                degree,
                                no_invalidate );
end gather_index_stats;

procedure set_system_stats( statname  in varchar(100),
                            statvalue in double )
as
begin
    system_.set_system_stats( statname,
                              statvalue );
end set_system_stats;

procedure set_table_stats( ownname        in varchar(128),
                           tabname        in varchar(128),
                           partname       in varchar(128) default null,
                           numrow         in bigint  default null,
                           numpage        in bigint  default null,
                           avgrlen        in bigint  default null,
                           onerowreadtime in double  default null,
                           no_invalidate  in boolean default false )
as
begin
    system_.set_table_stats( ownname,
                             tabname,
                             partname,
                             numrow,
                             numpage,
                             avgrlen,
                             onerowreadtime,
                             no_invalidate );
end set_table_stats;

procedure set_column_stats( ownname       in varchar(128),
                            tabname       in varchar(128),
                            colname       in varchar(128),
                            partname      in varchar(128) default null,
                            numdist       in bigint  default null,
                            numnull       in bigint  default null,
                            avgclen       in bigint  default null,
                            minvalue      in varchar(48) default null,
                            maxvalue      in varchar(48) default null,
                            no_invalidate in boolean default false )
as
begin
    system_.set_column_stats( ownname,
                              tabname,
                              colname,
                              partname,
                              numdist,
                              numnull,
                              avgclen,
                              minvalue,
                              maxvalue,
                              no_invalidate );
end set_column_stats;

procedure set_index_stats( ownname          in varchar(128),
                           idxname          in varchar(128),
                           keycount         in bigint  default null,
                           numpage          in bigint  default null,
                           numdist          in bigint  default null,
                           clusteringfactor in bigint  default null,
                           indexheight      in bigint  default null,
                           avgslotcnt       in bigint  default null,
                           no_invalidate    in boolean default false )
as
begin
    system_.set_index_stats( ownname,
                             idxname,
                             keycount,
                             numpage,
                             numdist,
                             clusteringfactor,
                             indexheight,
                             avgslotcnt,
                             no_invalidate );
end set_index_stats;

procedure delete_system_stats()
as
begin
    system_.delete_system_stats();
end delete_system_stats;

procedure delete_table_stats( ownname         in varchar(128),
                              tabname         in varchar(128),
                              partname        in varchar(128) default null,
                              cascade_part    in boolean default true,
                              cascade_columns in boolean default true,
                              cascade_indexes in boolean default true,
                              no_invalidate   in boolean default false )
as
begin
    system_.delete_table_stats( ownname,
                                tabname,
                                partname,
                                cascade_part,
                                cascade_columns,
                                cascade_indexes,
                                no_invalidate );
end delete_table_stats;

procedure delete_column_stats( ownname       in varchar(128),
                               tabname       in varchar(128),
                               colname       in varchar(128),
                               partname      in varchar(128) default null,
                               cascade_part  in boolean default true,
                               no_invalidate in boolean default false )
as
begin
    system_.delete_column_stats( ownname,
                                 tabname,
                                 colname,
                                 partname,
                                 cascade_part,
                                 no_invalidate );
end delete_column_stats;

procedure delete_index_stats( ownname       in varchar(128),
                              idxname       in varchar(128),
                              no_invalidate in boolean default false )
as
begin
    system_.delete_index_stats( ownname,
                                idxname,
                                no_invalidate );
end delete_index_stats;

procedure delete_database_stats( no_invalidate in boolean default false )
as
begin
    system_.delete_database_stats( no_invalidate );
end delete_database_stats;

procedure get_system_stats( statname  in varchar(100),
                            statvalue out double )
as
begin
    system_.get_system_stats( statname,
                              statvalue );
end get_system_stats;

procedure get_table_stats( ownname        in varchar(128),
                           tabname        in varchar(128),
                           partname       in varchar(128) default null,
                           numrow         out bigint,
                           numpage        out bigint,
                           avgrlen        out bigint,
                           cachedpage     out bigint,
                           onerowreadtime out double )
as
begin
    system_.get_table_stats( ownname,
                             tabname,
                             partname,
                             numrow,
                             numpage,
                             avgrlen,
                             cachedpage,
                             onerowreadtime );
end get_table_stats;

procedure get_column_stats( ownname  in varchar(128),
                            tabname  in varchar(128),
                            colname  in varchar(128),
                            partname in varchar(128) default null,
                            numdist  out bigint,
                            numnull  out bigint,
                            avgclen  out bigint,
                            minvalue out varchar(48),
                            maxvalue out varchar(48) )
as
begin
    system_.get_column_stats( ownname,
                              tabname,
                              colname,
                              partname,
                              numdist,
                              numnull,
                              avgclen,
                              minvalue,
                              maxvalue );
end get_column_stats;

procedure get_index_stats( ownname          in varchar(128),
                           idxname          in varchar(128),
                           partname         in varchar(128) default null,
                           keycount         out bigint,
                           numpage          out bigint,
                           numdist          out bigint,
                           clusteringfactor out bigint,
                           indexheight      out bigint,
                           cachedpage       out bigint,
                           avgslotcnt       out bigint )
as
begin
    system_.get_index_stats( ownname,
                             idxname,
                             partname,
                             keycount,
                             numpage,
                             numdist,
                             clusteringfactor,
                             indexheight,
                             cachedpage,
                             avgslotcnt );
end get_index_stats;

procedure copy_table_stats( ownname     in varchar(128),
                            tabname     in varchar(128),
                            srcpartname in varchar(128),
                            dstpartname in varchar(128) )
as
    up_ownname varchar(128);
    up_tabname varchar(128);
    up_srcpartname varchar(128);
    up_dstpartname varchar(128);

    numrow BIGINT;
    numpage BIGINT;
    avgrlen BIGINT;
    cachedpage BIGINT;
    onerowreadtime BIGINT;

    numdist BIGINT;
    numnull BIGINT;
    avgclen BIGINT;
    minvalue varchar(48);
    maxvalue varchar(48);

    e1 exception;
    pragma exception_init(e1, 200753 );

begin
    IF (ownname IS NULL) OR
       (tabname IS NULL) OR
       (srcpartname IS NULL) OR
       (dstpartname IS NULL) THEN
        RAISE e1;
    END IF;

    IF INSTR( ownname, CHR(34) ) = 0 THEN
        up_ownname := UPPER( ownname );
    ELSE
        up_ownname := REPLACE2( ownname, CHR(34) );
    END IF;
    IF INSTR( tabname, CHR(34) ) = 0 THEN
        up_tabname := UPPER( tabname );
    ELSE
        up_tabname := REPLACE2( tabname, CHR(34) );
    END IF;
    IF INSTR( srcpartname, CHR(34) ) = 0 THEN
        up_srcpartname := UPPER( srcpartname );
    ELSE
        up_srcpartname := REPLACE2( srcpartname, CHR(34) );
    END IF;
    IF INSTR( dstpartname, CHR(34) ) = 0 THEN
        up_dstpartname := UPPER( dstpartname );
    ELSE
        up_dstpartname := REPLACE2( dstpartname, CHR(34) );
    END IF;

    DECLARE
        CURSOR C1 IS select column_name from SYSTEM_.SYS_USERS_ u,
                                             SYSTEM_.SYS_COLUMNS_ c,
                                             system_.sys_tables_ t
                            where u.user_name = up_ownname and
                                  t.table_name = up_tabname and
                                  t.user_id = u.user_id and
                                  c.table_id = t.table_id;
        TAB_REC C1%ROWTYPE;

        CURSOR C2 IS select v.column_name as column_name,
                           (select regexp_substr(partition_max_value,'[^,]+',1,v.pc_order+1) from SYSTEM_.SYS_TABLE_PARTITIONS_ p where table_id=v.table_id and partition_name=up_srcpartname)+0 as minvalue,
                           (select regexp_substr(partition_max_value,'[^,]+',1,v.pc_order+1) from SYSTEM_.SYS_TABLE_PARTITIONS_ p where table_id=v.table_id and partition_name=up_dstpartname)+0 as maxvalue
                        from
                            (select c.column_name as column_name,
                                    t.table_id as table_id,
                                    k.part_col_order as pc_order
                                from SYSTEM_.SYS_USERS_ u,
                                     SYSTEM_.SYS_PART_KEY_COLUMNS_ k,
                                     SYSTEM_.SYS_TABLES_ t,
                                     SYSTEM_.SYS_COLUMNS_ c
                                where u.user_name = up_ownname and 
                                      t.table_name = up_tabname and
                                      u.user_id = t.user_id and
                                      t.table_id = c.table_id and
                                      t.table_id = k.partition_obj_id and
                                      u.user_id = k.user_id and
                                      k.column_id = c.column_id
                                order by k.part_col_order ) v;
        TAB_REC2 C2%ROWTYPE;
        BEGIN
    system_.get_table_stats( ownname,
                             tabname,
                             srcpartname,
                             numrow,
                             numpage,
                             avgrlen,
                             cachedpage,
                             onerowreadtime );

    system_.set_table_stats( ownname,
                             tabname,
                             dstpartname,
                             numrow,
                             numpage,
                             avgrlen,
                             onerowreadtime,
                             FALSE );

    OPEN C1;
        LOOP
            FETCH C1 INTO TAB_REC;
            EXIT WHEN C1%NOTFOUND;

            system_.get_column_stats( ownname,
                                      tabname,
                                      '"' || TAB_REC.COLUMN_NAME || '"',
                                      srcpartname,
                                      numdist,
                                      numnull,
                                      avgclen,
                                      minvalue,
                                      maxvalue );

            system_.set_column_stats( ownname,
                                      tabname,
                                      '"' || TAB_REC.COLUMN_NAME || '"',
                                      dstpartname,
                                      numdist,
                                      numnull,
                                      avgclen,
                                      minvalue,
                                      maxvalue,
                                      FALSE );
        END LOOP;
    CLOSE C1;

    OPEN C2;
        LOOP
            FETCH C2 INTO TAB_REC2;
            EXIT WHEN C2%NOTFOUND;

            system_.set_column_stats( ownname,
                                      tabname,
                                      '"' || TAB_REC2.COLUMN_NAME || '"',
                                      dstpartname,
                                      NULL,
                                      NULL,
                                      NULL,
                                      TAB_REC2.MINVALUE,
                                      TAB_REC2.MAXVALUE,
                                      FALSE );
        END LOOP;
    CLOSE C2;

    END;

    EXCEPTION
        WHEN e1 THEN
            println('invalid input value');

end copy_table_stats;

procedure set_primary_key_stats( ownname          in varchar(128),
                                 tabname          in varchar(128),
                                 keycount         in bigint  default null,
                                 numpage          in bigint  default null,
                                 numdist          in bigint  default null,
                                 clusteringfactor in bigint  default null,
                                 indexheight      in bigint  default null,
                                 avgslotcnt       in bigint  default null,
                                 no_invalidate    in boolean default false )
as
idxname varchar(128);
up_ownname varchar(128);
up_tabname varchar(128);
 
cursor c1 is
select a.index_name
from   system_.sys_indices_ a,
       system_.sys_tables_ b,
       system_.sys_users_ c,
       system_.sys_constraints_ d
where  a.index_name like '__SYS_IDX_ID_%'
       and b.table_name = up_tabname
       and c.user_name = up_ownname
       and d.constraint_type = 3
       and a.user_id = c.user_id
       and a.table_id = b.table_id
       and a.index_id = d.index_id;
begin
if instr( ownname, chr(34) ) = 0 then
    up_ownname := upper( ownname );
else
    up_ownname := replace2( ownname, chr(34) );
end if;
 
if instr( tabname, chr(34) ) = 0 then
    up_tabname := upper( tabname );
else
    up_tabname := replace2( tabname, chr(34) );
end if;
 
open c1;
fetch c1 into idxname;
close c1;
 
println( idxname );
 
set_index_stats( ownname,
                 idxname,
                 keycount,
                 numpage,
                 numdist,
                 clusteringfactor,
                 indexheight,
                 avgslotcnt,
                 no_invalidate );
end set_primary_key_stats;

procedure set_unique_key_stats( ownname          in varchar(128),
                                tabname          in varchar(128),
                                colnamelist      in varchar(32000),
                                keycount         in bigint  default null,
                                numpage          in bigint  default null,
                                numdist          in bigint  default null,
                                clusteringfactor in bigint  default null,
                                indexheight      in bigint  default null,
                                avgslotcnt       in bigint  default null,
                                no_invalidate    in boolean default false )
as
idxname varchar(128);
up_ownname varchar(128);
up_tabname varchar(128);
up_colnamelist varchar(32000);
 
cursor c1 is
select a.index_name
from   system_.sys_indices_ a,
       system_.sys_tables_ b,
       system_.sys_users_ c,
       system_.sys_constraints_ d
where  a.index_name like '__SYS_IDX_ID_%'
       and b.table_name = up_tabname
       and c.user_name = up_ownname
       and d.constraint_type = 2
       and a.user_id = c.user_id
       and a.table_id = b.table_id
       and a.index_id = d.index_id
       and ( up_colnamelist =
             (select listagg(f.column_name || decode(e.sort_order, 'A', ' ASC', 'D', ' DESC'), ',')
                         within group(order by e.index_col_order)
              from   system_.sys_index_columns_ e,
                     system_.sys_columns_ f
              where  a.index_id = e.index_id
                     and e.column_id = f.column_id) );
begin
 
if instr( ownname, chr(34) ) = 0 then
  up_ownname := upper( ownname );
else
  up_ownname := replace2( ownname, chr(34) );
end if;
if instr( tabname, chr(34) ) = 0 then
  up_tabname := upper( tabname );
else
  up_tabname := replace2( tabname, chr(34) );
end if;

with t as (select case2(instr(colnamelist, chr(34)) = 0, upper(colnamelist),
                        replace2(colnamelist, chr(34))) as str
           from dual),
     x as (select regexp_substr(str,'[^,]+',1,level) as val
           from t
           connect by regexp_substr(str,'[^,]+',1,level) is not null)
select listagg(case2(instr(val, ' ASC')  > 0, val,
                     instr(val, ' DESC') > 0, val,
                     val || ' ASC'), ',')
       within group(order by rownum) as val2 into up_colnamelist
from x;
 
open c1;
fetch c1 into idxname;
close c1;
 
println( idxname );
 
set_index_stats( ownname,
                 idxname,
                 keycount,
                 numpage,
                 numdist,
                 clusteringfactor,
                 indexheight,
                 avgslotcnt,
                 no_invalidate );
end set_unique_key_stats;

end dbms_stats;
/
