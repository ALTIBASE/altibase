create or replace procedure diskdbinfo()
as
  h_spacename varchar(40);
  h_spacetype varchar(20);
  h_totalsize double;
  h_usedsize  double;

cursor c1 is
	select 
			name, 
			decode(type, 	1, 'SYS_TBS_DATA', 
						 	2, 'USER_TBS_DATA', 
							3, 'SYS_TBS_TEMP', 
							4, 'USER_TBS_TEMP', 
							5, 'SYS_TBS_UNDO') type,
			TOTAL_PAGE_COUNT*32/1024 total_size,
			USED_PAGE_CNT*32/1024 used_size
	from 	v$tablespaces;
begin

system_.println('<<  Disk Database Usage >>');

	open c1;
	loop
		fetch c1 into h_spacename, h_spacetype, h_totalsize, h_usedsize;
		exit when c1%notfound;
		
		system_.println('Tablespace Name  = ' || h_spacename);
		system_.println('    Type    = ' || h_spacetype);
		system_.println('    Total   = ' || h_totalsize || 'M');
		system_.println('    Used    = ' || h_usedsize || 'M');
/*
		system_.println(h_spacename || ': Total - ' || h_totalsize || 'M, Used - ' || h_usedsize || 'M');
*/
	end loop;
	close c1;

system_.println(' ');
end;
/


