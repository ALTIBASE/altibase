use DBI;
# $Id: testmulti.pl 11810 2005-05-18 04:43:35Z qa $

#$ENV{'ODBCINI'}="/export/cmn/etc/odbc.ini" ;
my($connectString) = "dbi:ODBC:DSN=TESTDB;Database=xxxxxdata;uid=usrxxxxx;pwd=xxxxx" ;
# DBI->trace(9) ;
my($dbh)=DBI->connect() ;
if ( !defined($dbh) ) {
   die "Connection failed" ;
}
my($sqlStr) ;

$sqlStr = "select id,name from sysobjects where id=1; select * from sysobjects where id=1; select \@\@rowcount";


my($sth) = $dbh->prepare($sqlStr);
$sth->execute;
if ( $sth->errstr ){
   die $sth->errstr ;
}
my(@aRefResult) = qw() ;
my(@data) = qw() ;
my($cnt);
do {
   while ( @data = $sth->fetchrow ) {
       print join("|",@data), "\n" ;
   }
} while ( $sth->{odbc_more_results}  ) ;
