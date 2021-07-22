#!/usr/bin/perl -I./t
# $Id: listtabs.pl 11810 2005-05-18 04:43:35Z qa $


require DBI;

my (@row);

my $dbh = DBI->connect()
    || die "Can't connect to your $ENV{DBI_DSN} using user: $ENV{DBI_USER} and pass: $ENV{DBI_PASS}\n$DBI::errstr\n";
# ------------------------------------------------------------

my $rows = 0;
my @tables;
my $table;
$| = 1;

if (@tables = $dbh->tables) {
    print join(', ', @tables), "\n";
    foreach $table (@tables) {
	my $schema = '';
	if ($table =~ m/(.*)\.(.*)$/) {
		$schema = $1;
		$table = $2;
	}
	my $sthcols = $dbh->func('',$schema, $table,'', columns);
	if ($sthcols) {
	    while (@row = $sthcols->fetchrow_array) {
		print "\t", join(', ', @row), "\n";
	    }
	} else {
	    # hmmm...none of my drivers support this...dang.  I can't test it.
	    print "SQLColumns: $DBI::errstr\n";
	}
    }
}

$dbh->disconnect();

