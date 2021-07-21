#!perl -w
# $Id: in_test.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

# Connect to the database, and create a table and stored procedure:
my $dbh = DBI->connect("dbi:ODBC:DSN=127.0.0.1;UID=SYS;PWD=MANAGER;CONNTYPE=1;NLS_USE=US7ASCII;PORT_NO=20538", "", "", {'RaiseError' => 1});
eval {$dbh->do("DROP TABLE t2");};
eval {$dbh->do("CREATE TABLE t2 (i INTEGER)");};
eval {$dbh->do("DROP PROCEDURE proc2");};
my $proc2 =
    "CREATE PROCEDURE proc2  (p1 in integer) AS ".
    "BEGIN".
    "    insert into t2 values (p1);".     # breaks fetchrow_array 
    "END";
eval {$dbh->do ($proc2);};

# Execute it:
if (-e "dbitrace.log") {
   unlink("dbitrace.log");
}
$dbh->trace(9, "dbitrace.log");
my $sth = $dbh->prepare ("exec proc2 (60)");
   $sth->execute ();
$dbh->disconnect;
