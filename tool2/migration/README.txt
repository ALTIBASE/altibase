[ mig.sh ] : altibase3용 migration script

1. 사용 방법 :

mig3.sh username/passwd {out|in}
	out : 해당 유저가 만들어 놓은 모든 테이블을 export
	in  : 만들어 놓은 파일들을 이용하여 데이터를 import
 
2. 수행 시 생성되는 파일들
 'username'_mig_list.txt     : 해당 유저의 마이그레이션 대상 테이블의 이름들.
 'username'_'tablename'.sql  : 해당 테이블 생성 스크립트 
 'username'_'tablename'.fmt  : 해당 테이블 iloader 폼파일
 'username'_'tablename'.dat  : 해당 테이블 iloader 데이터 파일 
 'username'_'tablename'.olog : 해당 테이블 iloader export 로그파일 - out 시에만 생김 
 'username'_'tablename'.obad : 해당 테이블 iloader export 배드파일 - out 시에만 생김 
 'username'_'tablename'.ilog : 해당 테이블 iloader import 로그파일 - in 시에만 생김 
 'username'_'tablename'.ibad : 해당 테이블 iloader import 배드파일 - in 시에만 생김
 
이러한 파일들은 수행시킨 디렉터리에 바로 생기므로 작업시 임시 디렉터리를
만들어서 수행하는 것이 이후에 정리하는데 편합니다.
 
3. functions in main

        in)
                makeTables : 테이블생성 스크립트를 이용하여 서버에 테이블을 생성.
                dataImport : dat, fmt file을 이용하여 data import.
        ;;
        out)
                getTableName     : 마이그레이션 대상 테이블 목록을 만듬
                makeTableScript  : 테이블 생성 스크립트를 만듬
                makeFormFile     : 폼파일을 생성
                dataExport       : .fmt와 테이블 이름을 이용하여 export
        ;;
        *)
                usage
        ;;
esac
 
위와 같이 진행 하도록 되어 있습니다.
상황에 따라 필요없는 절차는 주석으로 막고 사용하셔도 됩니다.
각 항목은 수행하기 전에 사용자에게 작업진행을 물어보고 'y'가 입력될 때만 수행하도록
하였습니다


