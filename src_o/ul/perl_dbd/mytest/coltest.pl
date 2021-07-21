#!perl -w
# $Id: coltest.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

my $dbh = DBI->connect() or die "Can't connect";

$dbh->{RaiseError} = 1;
$dbh->{LongReadLen} = 800;

my @tables = $dbh->tables;

my @mtable = grep(/foo/, @tables);
my ($catalog, $schema, $table) = split(/\./, $mtable[0]);
$catalog =~ s/"//g;
$schema =~ s/"//g;
$table =~ s/"//g;
print "Getting column info for: $catalog, $schema, $table\n";
my $sth = $dbh->column_info($catalog, $schema, $table, undef);
my @row;

print join(', ', @{$sth->{NAME}}), "\n";
while (@row = $sth->fetchrow_array) {

   # join prints nasty warning messages with -w. There's gotta be a better way...
   foreach (@row) { $_ = "" if (!defined); }

   print join(", ", @row), "\n";
}
$dbh->disconnect;
