#!perl -w
# $Id: fetch_parameter.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

# Connect to the database, and create a table and stored procedure:
my $dbh = DBI->connect("dbi:ODBC:DSN=127.0.0.1;UID=SYS;PWD=MANAGER;CONNTYPE=1;NLS_USE=US7ASCII;PORT_NO=20538", "", "", {'RaiseError' => 1});
eval {$dbh->do("DROP TABLE t1");};
eval {$dbh->do("CREATE TABLE t1 (i1 INTEGER, i2 integer)");};
eval {$dbh->do("insert into  t1 values(1,1)");};
eval {$dbh->do("DROP PROCEDURE proc1");};
my $proc1 =
    "CREATE or replace PROCEDURE proc1  (  OUT1 OUT INTEGER, OUT2 OUT INTEGER ) AS ".
    " CURSOR C1 IS SELECT * FROM T1;".
    "BEGIN".
     " 	 OPEN C1;".
     "   FETCH C1 INTO OUT1, OUT2;".
    "END";
eval {$dbh->do ($proc1);};

if ($@) {
   print "Error creating procedure.\n$@\n";
}
# Execute it:
if (-e "dbitrace.log") {
   unlink("dbitrace.log");
}
$dbh->trace(9, "dbitrace.log");
my $sth = $dbh->prepare ("{call proc1(?, ?) }");
my $retValue1 = 1;
my $retValue2 = 2;

$sth->bind_param_inout(1,\$retValue1, 10, { TYPE => DBI::SQL_INTEGER });
$sth->bind_param_inout(2,\$retValue1, 10, { TYPE => DBI::SQL_INTEGER });
$sth->execute;

my $sth2 = $dbh->prepare("SELECT * FROM t1");
my @row;
$sth2->execute();
while (@row = $sth2->fetchrow_array) {
   print $row[0], " ", $row[1], "\n";
}
$sth2->finish();
$dbh->disconnect;
