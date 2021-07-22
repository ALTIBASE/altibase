#perl -w
# $Id: testver.pl 11810 2005-05-18 04:43:35Z qa $

use DBI;

my $dbh = DBI->connect()
	or die "$DBI::errstr\n";


my %InfoTests = (
	'SQL_DRIVER_NAME', 6,
	'SQL_DRIVER_VER', 7,
	'SQL_CURSOR_COMMIT_BEHAVIOR', 23,
	'SQL_ALTER_TABLE', 86,
	'SQL_ACCESSIBLE_PROCEDURES', 20,
);

foreach $SQLInfo (sort keys %InfoTests) {
	$ret = 0;
	$ret = $dbh->func($InfoTests{$SQLInfo}, GetInfo);
	print "$SQLInfo ($InfoTests{$SQLInfo}):\t$ret\n";
}
DBI->trace(9,"c:/trace.txt");
eval { print "SQL_ROWSET_SIZE = $dbh->{odbc_SQL_ROWSET_SIZE}\n"; };
eval { print "Driver version = $dbh->{odbc_SQL_DRIVER_ODBC_VER}\n"; };
$dbh->disconnect;

