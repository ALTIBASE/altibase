create or replace procedure showProcBody(p1 in varchar(40)) 
as
-- 해당 프로시져의 내용을 화면으로 뿌린다.
-- 라인이 잘 맞지가 않음. --;
-- exec showProcBody('procedure_name');
		cursor c1 is
		select system_.sys_proc_parse_.parse
		from system_.sys_proc_parse_
		where system_.sys_proc_parse_.proc_oid = (
			select SYSTEM_.sys_procedures_.proc_oid
	    		from system_.sys_procedures_ 
	    		where SYSTEM_.sys_procedures_.proc_name = upper(p1)) order by system_.sys_proc_parse_.seq_no;
		v1 varchar(4000);
		begin
            	open c1;
				SYSTEM_.PRINTLN('---------------------------------');
				system_.print(p1);
            	SYSTEM_.PRINTLN(' Procedure');
            	SYSTEM_.PRINTLN('---------------------------------');
            	SYSTEM_.PRINTLN('');
                loop
                	fetch C1 into v1;
                	exit when C1%NOTFOUND;
                    SYSTEM_.PRINTLN(v1);
				end loop;
                close c1;
                SYSTEM_.PRINTLN('');
                SYSTEM_.PRINTLN('---------------------------------');
			end;
/
