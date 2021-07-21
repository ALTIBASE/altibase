#!perl -w
# $Id: out_test.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

# Connect to the database, and create a table and stored procedure:
my $dbh = DBI->connect("dbi:ODBC:DSN=127.0.0.1;UID=SYS;PWD=MANAGER;CONNTYPE=1;NLS_USE=US7ASCII;PORT_NO=20538", "", "", {'RaiseError' => 1});
eval {$dbh->do("DROP TABLE t1");};
eval {$dbh->do("CREATE TABLE t1 (i1 INTEGER, i2 integer)");};
eval {$dbh->do("DROP PROCEDURE proc1");};
eval {$dbh->do("DROP PROCEDURE proc2");};
my $proc1 =
    "CREATE or replace PROCEDURE proc1  (p1  integer, p2 out integer) AS ".
    "BEGIN".
     "   p2 := p1 + 100;".
    "END";
eval {$dbh->do ($proc1);};

my $proc2 =
    "CREATE or replace PROCEDURE proc2  (p1  integer, p2 out integer) AS ".
    " v1 integer;".
    " v2 integer;".
    "BEGIN".
     "   v1 := p1 ;".
     "   v2 := p2 ;".
     "   proc1(v1,v2) ;".
     "   insert into t1 values(v1, v2) ;".
    "END";
eval {$dbh->do ($proc2);};
if ($@) {
   print "Error creating procedure.\n$@\n";
}
print "Error1 .\n";
# Execute it:
if (-e "dbitrace.log") {
   unlink("dbitrace.log");
}
$dbh->trace(9, "dbitrace.log");
my $sth = $dbh->prepare ("{call proc2(?, ?) }");
my $retValue1 = 10;
my $retValue2 = 20;

$sth->bind_param_inout(1,\$retValue1, 30, { TYPE => DBI::SQL_INTEGER });
$sth->bind_param_inout(2,\$retValue1, 30, { TYPE => DBI::SQL_INTEGER });
$sth->execute;

print "$retValue1\n";
$dbh->disconnect;
