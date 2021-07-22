#!perl -w
# $Id: moreresults.pl 11810 2005-05-18 04:43:35Z qa $

#
# Sorry -- this test is pretty specific to MSSQL Server and Sybase...
#

use DBI;
my (@row);

my $dbh;

$dbh = DBI->connect()
       || die "Can't connect to your $ENV{DBI_DSN} using user: $ENV{DBI_USER} and pass: $ENV{DBI_PASS}\n$DBI::errstr\n";
# ------------------------------------------------------------

my $result_sets = 0;
$| = 1;

my $sth;

$sth = $dbh->prepare("{call sp_spaceused}")
	  or die $dbh->errstr;
$sth->execute
   or die $sth->errstr;

do {
    print join(":", @{$sth->{NAME}}), "\n";
    while ( my $ref = $sth->fetch ) {
	print join(":", @$ref), "\n";
    }
} while ($sth->{odbc_more_results});
    print "(", $sth->rows, " rows affected)\n";
    $sth->finish;

$dbh->disconnect();

