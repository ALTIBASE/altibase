#!/usr/bin/perl -w -I./t
# $Id: testkeys.pl 11810 2005-05-18 04:43:35Z qa $


# use strict;
use DBI qw(:sql_types);
# use DBD::ODBC::Const qw(:sql_types);

my (@row);

my $dbh = DBI->connect('dbi:ODBC:PERL_TEST_ACCESS', '', '', {PrintError=>1})
	  or exit(0);
# ------------------------------------------------------------

my @tables;
my $table;
my $sth;
$| = 1;

if (@tables = $dbh->tables) {
    # print join(', ', @tables), "\n";
    foreach $table (@tables) {
	my $schema = '';
	if ($table =~ m/(.*)\.(.*)$/) {
		$schema = $1;
		$table = $2;
	}

	# DBI->trace(3);
	$sth = $dbh->func('', $schema, $table, GetPrimaryKeys);
	if (!$sth) {
	    print "No Primary keys for $schema.$table (", $dbh->errstr, ")\n";
	} else {
	    print "$table\n";
	    my @row;
	    while (@row = $sth->fetchrow_array) {
		print "\t", join(', ', @row), "\n";
	    }
	}
    }
}

$dbh->disconnect();

sub nullif ($) {
   my $val = shift;
   $val ? $val : "(null)";
}