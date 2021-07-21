#!perl -w
# $Id: char_test.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

# Connect to the database, and create a table and stored procedure:
my $dbh = DBI->connect("dbi:ODBC:DSN=127.0.0.1;UID=SYS;PWD=MANAGER;CONNTYPE=1;NLS_USE=US7ASCII;PORT_NO=20538", "", "", {'RaiseError' => 1});
eval {$dbh->do("DROP TABLE t2");};
eval {$dbh->do("CREATE TABLE t2 (i1 char(10), i2 varchar(10))");};
eval {$dbh->do("DROP PROCEDURE proc2");};
my $proc2 =
    "CREATE or replace PROCEDURE proc2  (a1  in out char(10), a2 in out varchar(10)) AS ".
    "BEGIN".
     "   insert into t2 values (a1, a2);".
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
my $sth = $dbh->prepare ("{ call proc2(?,?) }");
my $retValue1 = "queen";
my $retValue2 = "kingdom";

$sth->bind_param_inout(1,\$retValue1, 10, { TYPE => DBI::SQL_CHAR });
$sth->bind_param_inout(2, \$retValue2,10,   { TYPE => DBI::SQL_VARCHAR} );
$sth->execute;
print "$retValue1\n";
print "$retValue2\n";
$dbh->disconnect;
