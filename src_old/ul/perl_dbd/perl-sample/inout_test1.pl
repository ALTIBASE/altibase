#!perl -w
# $Id: inout_test1.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

# Connect to the database, and create a table and stored procedure:
my $dbh = DBI->connect("dbi:ODBC:DSN=127.0.0.1;UID=SYS;PWD=MANAGER;CONNTYPE=1;NLS_USE=US7ASCII;PORT_NO=20538", "", "", {'RaiseError' => 1});
eval {$dbh->do("DROP TABLE t2");};
eval {$dbh->do("CREATE TABLE t2 (i1 INTEGER)");};
eval {$dbh->do("DROP PROCEDURE proc2");};
my $proc2 =
    "CREATE or replace PROCEDURE proc2  (a1  in out integer) AS ".
    "BEGIN".
     "   insert into t2 values (a1);".
     "   a1 := 2;".
    "END";
eval {$dbh->do ($proc2);};

if ($@) {
   print "Error creating procedure.\n$@\n";
}
# Execute it:
if (-e "dbitrace.log") {
   unlink("dbitrace.log");
}
$dbh->trace(9, "dbitrace.log");
my $sth = $dbh->prepare ("{ call proc2(?) }");
my $retValue1 = 1;

$sth->bind_param_inout(1,\$retValue1, 10, { TYPE => DBI::SQL_INTEGER });
$sth->execute;
print "$retValue1\n";
my $sth1 = $dbh->prepare("insert into t2 values(5) ");
$sth1->execute;

my $sth2 = $dbh->prepare("select * from t2");
my @row;
$sth2->execute;
while (@row = $sth2->fetchrow_array) {
   print $row[0], "\n";
}
$dbh->disconnect;
