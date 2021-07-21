#perl -w
# $Id: contest.pl 11810 2005-05-18 04:43:35Z qa $

use DBI;

# my $DSN = 'driver=Microsoft Access Driver (*.mdb);dbq=\\\\cheese\\g$\\perltest.mdb'; 
my $DSN = 'Driver={SQL Server};SERVER=SQL1;';
my $dbh = DBI->connect("dbi:ODBC:$DSN",undef,undef,{RaiseError=>1})
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

$dbh->disconnect;

