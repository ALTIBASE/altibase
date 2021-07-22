create or replace procedure SHOWTABLES(p1 in varchar(40)) 
as

-- 해당 유저의 테이블을 보여 줍니다.
-- exec showTables('User_name');
		cursor c1 is
        	select SYSTEM_.SYS_TABLES_.TABLE_NAME 
            from SYSTEM_.SYS_TABLES_ 
            where SYSTEM_.SYS_TABLES_.USER_ID = 
				(select SYSTEM_.SYS_USERS_.USER_ID 
                from SYSTEM_.SYS_USERS_ 
				where SYSTEM_.SYS_USERS_.USER_NAME = upper(p1) 
				AND system_.SYS_TABLES_.TABLE_TYPE = 'T');
			v1 CHAR(40);     
			
			begin
            	open c1;
				SYSTEM_.PRINTLN('-------------------');
				system_.print(p1);
            	SYSTEM_.PRINTLN(' Table');
            	SYSTEM_.PRINTLN('-------------------');
                loop
                	fetch C1 into v1;
                	exit when C1%NOTFOUND;
                	system_.print(' ');
                    SYSTEM_.PRINTLN(v1);
				end loop;
            	SYSTEM_.PRINTLN('-------------------');
            
                close c1;
			end;
/

