#!perl -w
# $Id: joetest4.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI qw(:sql_types);

my $dbh=DBI->connect() or die "Can't connect";

eval {$dbh->do("DROP TABLE table1");};
eval {$dbh->do("CREATE TABLE table1 (d DATETIME)");};

if (-e "dbitrace.log") {
   unlink("dbitrace.log");
}
$dbh->trace(9,"dbitrace.log");
my $sth = $dbh->prepare ("INSERT INTO table1 (d) VALUES (?)");
$sth->bind_param (1, undef, SQL_TYPE_TIMESTAMP);

#$sth->bind_param (1, "2002-07-12 05:08:37.350", SQL_TYPE_TIMESTAMP);
$sth->execute("2002-07-12 05:08:37.350");
#$sth->bind_param (1, undef, SQL_TYPE_TIMESTAMP);
$sth->execute(undef);

my @row;
my $sth2 = $dbh->prepare("select * from table1");
$sth2->execute;
while (@row = $sth2->fetchrow_array) {
   print join(", ", @row), "\n";
}
$dbh->disconnect;
