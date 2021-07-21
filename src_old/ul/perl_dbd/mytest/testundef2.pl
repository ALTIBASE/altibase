#!perl -w
# $Id: testundef2.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI qw(:sql_types);

my $dbh=DBI->connect() or die "Can't connect";

eval {$dbh->do("DROP TABLE table1");};
eval {$dbh->do("CREATE TABLE table1 (v varchar(4000), d datetime)");};

unlink("dbitrace.log") if (-e "dbitrace.log") ;
$dbh->trace(8, "dbitrace.log");

my $sth = $dbh->prepare ("INSERT INTO table1 (d, v) VALUES (?, ?)");
$sth->bind_param (1, undef, SQL_TYPE_TIMESTAMP);
$sth->bind_param (2, undef, SQL_LONGVARCHAR);
$sth->execute();
$sth->bind_param (1, "2002-07-12 17:07:37.350", SQL_TYPE_TIMESTAMP);
$sth->bind_param (2, "real data", SQL_LONGVARCHAR);
$sth->execute();
$sth->bind_param (1, undef, SQL_TYPE_TIMESTAMP);
$sth->bind_param (2, undef, SQL_LONGVARCHAR);
$sth->execute();

$sth = $dbh->prepare("select d, v from table1");
$sth->execute;
my @row;

while (@row = $sth->fetchrow_array) {
   foreach (@row) { $_ = "" if (!defined($_)); }
   print join(", ", @row), "\n";
}
$dbh->disconnect;
