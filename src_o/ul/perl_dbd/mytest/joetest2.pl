#!perl -w
# $Id: joetest2.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use Getopt::Std;
use DBI qw(:sql_types);

# Connect to the database and create the table:
my $dbh=DBI->connect() or die "Can't connect";
$dbh->{RaiseError} = 1;
$dbh->{AutoCommit} = 0;
$dbh->{LongReadLen} = 800;
if (-e "dbitrace.log") {
   unlink("dbitrace.log");
}
$dbh->trace(9,"dbitrace.log");
my $sth = $dbh->prepare("EXEC setLock ?,?,?,?,?");


$sth->bind_param (1, "JOET_log2_20020712170736", SQL_VARCHAR);
$sth->bind_param (2, "LOCKED",    SQL_VARCHAR);
$sth->bind_param (3, "JOET",    SQL_VARCHAR);
$sth->bind_param (4, 0, SQL_INTEGER);
$sth->bind_param (5, "2002-07-12 17:07:36", SQL_TYPE_TIMESTAMP);
$sth->execute;
my @data;
my $success;
while (my @data = $sth->fetchrow_array()) {
   ($success) = @data;
}

$dbh->disconnect;