#!perl -w
# $Id: inout_test2.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

# Connect to the database, and create a table and stored procedure:
my $dbh = DBI->connect("dbi:ODBC:DSN=127.0.0.1;UID=SYS;PWD=MANAGER;CONNTYPE=1;NLS_USE=US7ASCII;PORT_NO=20538", "", "", {'RaiseError' => 1});
eval {$dbh->do("DROP TABLE t1");};
eval {$dbh->do("CREATE TABLE t1 (i1 INTEGER, i2 integer, i3 integer)");};
eval {$dbh->do("DROP PROCEDURE proc1");};
eval {$dbh->do("DROP PROCEDURE proc2");};
my $proc1 =
    "CREATE or replace PROCEDURE proc1  (p1  integer, p2 in out integer, p3 out integer) AS ".
    "BEGIN".
     "   p2 := p1 + p2;".
     "   p3 := p1 + 100;".
    "END";
eval {$dbh->do ($proc1);};

my $proc2 =
    "CREATE or replace PROCEDURE proc2  (p1  integer, p2 integer, p3 integer) AS ".
    " v1 integer;".
    " v2 integer;".
    " v3 integer;".
    "BEGIN".
     "   v1 := p1 ;".
     "   v2 := p2 ;".
     "   v3 := p3 ;".
     "   proc1(v1,v2, v3) ;".
     "   insert into t1 values(v1, v2, v3) ;".
    "END";
eval {$dbh->do ($proc2);};
if ($@) {
   print "Error creating procedure.\n$@\n";
}
print "Success .\n";
# Execute it:
if (-e "dbitrace.log") {
   unlink("dbitrace.log");
}
$dbh->trace(9, "dbitrace.log");
my $sth = $dbh->prepare ("{ call proc2(?, ?, ?) }");
my $retValue1 = 50;
my $retValue2 = 3;
my $retValue3 = 7;

$sth->bind_param(1,$retValue1,  { TYPE => DBI::SQL_INTEGER });
$sth->bind_param_inout(2,\$retValue2, 20, { TYPE => DBI::SQL_INTEGER });
$sth->bind_param_inout(3,\$retValue3, 10, { TYPE => DBI::SQL_INTEGER });
$sth->execute;

print "$retValue1, $retValue2, $retValue3\n";

my $sth1 = $dbh->prepare("insert into t1 values(10,100,1000) ");
$sth1->execute;

my $sth2 = $dbh->prepare("select * from t1");
my @row;
$sth2->execute;
while (@row = $sth2->fetchrow_array) {
	  print $row[0], " ", 
	  $row[1], " ",
	  $row[2], "\n";
}
$dbh->disconnect;
