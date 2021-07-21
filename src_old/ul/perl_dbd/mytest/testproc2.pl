#!perl -w
# $Id: testproc2.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

# Connect to the database, and create two tables and a stored procedure:
my $dbh=DBI->connect() or die "Can't connect";

eval {$dbh->do("DROP TABLE table1");};
eval {$dbh->do("CREATE TABLE table1 (i INTEGER)");};

eval {$dbh->do("DROP TABLE table2");};
eval {$dbh->do("CREATE TABLE table2 (i INTEGER)");};

eval {$dbh->do("DROP PROCEDURE proc1");};
eval {$dbh->do("CREATE PROCEDURE proc1 AS ".
                "BEGIN  INSERT INTO table1 VALUES (1);  END;");};

unlink "dbitrace.log" if (-e "dbitrace.log");

$dbh->trace(9, "dbitrace.log");

# Insert a row into table1, either directly or indirectly:
my $direct = 0;
my $sth1;
if ($direct) {
   $sth1 = $dbh->prepare ("INSERT INTO table1 VALUES (1)");
} else {
   $sth1 = $dbh->prepare ("{ call proc1 }");
}
$sth1->execute();
$sth1->execute();
$sth1->execute();

# Insert a row into table2 (this fails after an indirect insertion):
my $sth2 = $dbh->prepare ("INSERT INTO table2 VALUES (2)");
$sth2->execute();

my $sth = $dbh->prepare("select i from table1 union select i from table2");
my @row;
$sth->execute;
while (@row = $sth->fetchrow_array) {
   print $row[0], "\n";
}
$dbh->disconnect;
