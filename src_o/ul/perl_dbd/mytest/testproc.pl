#!perl -w
# $Id: testproc.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

# Connect to the database, and create a table and stored procedure:
my $dbh=DBI->connect("dbi:ODBC:PERL_TEST_SQLSERVER", $ENV{DBI_USER}, $ENV{DBI_PASS}, { RaiseError => 1 }) or die "Can't connect";
eval {$dbh->do("DROP TABLE table1");};
eval {$dbh->do("CREATE TABLE table1 (i INTEGER)");};
eval {$dbh->do("DROP PROCEDURE proc1");};
my $proc1 =
    "CREATE PROCEDURE proc1 AS ".
    "BEGIN".
    "    INSERT INTO table1 VALUES (100);".     # breaks fetchrow_array 
    "    SELECT 9;".
    "END";
eval {$dbh->do ($proc1);};

# Execute it:
if (-e "dbitrace.log") {
   unlink("dbitrace.log");
}
$dbh->trace(9, "dbitrace.log");
my $sth = $dbh->prepare ("exec proc1");
   $sth->execute ();
do {
   while (my $result = $sth->fetchrow_array()) {
      print "result = $result\n";
   }
} while ($sth->{odbc_more_results});
$dbh->disconnect;
