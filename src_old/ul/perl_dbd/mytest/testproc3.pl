#!perl -w
# $Id: testproc3.pl 11810 2005-05-18 04:43:35Z qa $


use strict;
use DBI;
my $dbh = DBI->connect();

eval {$dbh->do("DROP TABLE table1");};
eval {$dbh->do("CREATE TABLE table1 (i INTEGER)");};

eval {$dbh->do("DROP TABLE table2");};
eval {$dbh->do("CREATE TABLE table2 (i INTEGER)");};

eval {$dbh->do("DROP PROCEDURE proc1");};
eval {$dbh->do("CREATE PROCEDURE proc1 \@inputval int AS ".
                "INSERT INTO table1 VALUES (\@inputval); " .   
			"	return \@inputval;");};
if ($@) { print $@, "\n"; }

unlink "dbitrace.log" if (-e "dbitrace.log");

$dbh->trace(9, "dbitrace.log");
# Insert a row into table1, either directly or indirectly:
my $direct = 0;
my $sth1;
$sth1 = $dbh->prepare ("{? = call proc1(?) }");

my $output = 0;
my $i = 0;

while ($i < 4) {
   # Insert a row into table2 (this fails after an indirect insertion):
   $sth1->bind_param_inout(1, \$output, 50, DBI::SQL_INTEGER);
   $sth1->bind_param(2, $i, DBI::SQL_INTEGER);

   $sth1->execute();
   print "$output\n";
   $i++;
}

my $sth = $dbh->prepare("select * from table1");
$sth->execute;
my @row;
while (@row = $sth->fetchrow_array) {
   print join(', ', @row), "\n";
}

$dbh->disconnect;

