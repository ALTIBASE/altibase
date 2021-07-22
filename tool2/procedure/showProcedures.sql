create or replace procedure showProcedures
--  프로시져/펑션의 이름을 보여 줍니다.
--  exec showProcedures;
as
    cursor c1 is select SYSTEM_.sys_procedures_.proc_name, decode(SYSTEM_.sys_procedures_.object_type, 0, 'Procedure',1,'Function')
            from system_.sys_procedures_ ;
    v1 CHAR(40);
    v2 char(20);
begin
            SYSTEM_.PRINTLN('------------------------------------------------------');
            SYSTEM_.PRINT('Proc_Name');
            SYSTEM_.PRINTLN('                           Procedure/Function');
            SYSTEM_.PRINTLN('------------------------------------------------------');
            open c1;
            loop
                fetch C1 into v1, v2;
                exit when C1%NOTFOUND;
                system_.print(' ');
                SYSTEM_.PRINT(v1);
                SYSTEM_.PRINTLN(v2);
            end loop;
            SYSTEM_.PRINTLN('------------------------------------------------------');
            close c1;
end;
/
