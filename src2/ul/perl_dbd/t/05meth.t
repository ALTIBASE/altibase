#!/usr/bin/perl -I./t
# $Id: 05meth.t 11810 2005-05-18 04:43:35Z qa $

## TBd: these tests don't seem to be terribly useful
use Test::More;

$| = 1;

use_ok('DBI', qw(:sql_types));
use strict;

# to help ActiveState's build process along by behaving (somewhat) if a dsn is not provided
BEGIN {
   if (!defined $ENV{DBI_DSN}) {
      plan skip_all => "DBI_DSN is undefined";
   } else {
      plan tests => 8;
   }
}


my @row;

my $dbh = DBI->connect();
unless($dbh) {
   BAILOUT("Unable to connect to the database $DBI::errstr\nTests skipped.\n");
   exit 0;
}


#### testing Tim's early draft DBI methods

my $r1 = $DBI::rows;
$dbh->{AutoCommit} = 0;
my $sth;
$sth = $dbh->prepare("DELETE FROM PERL_DBD_TEST");
ok($sth, "delete prepared statement");
$sth->execute();
cmp_ok($sth->rows, '>=', 0, "Number of rows > 0");
cmp_ok($DBI::rows, '==', $sth->rows, "Number of rows from DBI matches sth");
$sth->finish();
$dbh->rollback();
pass("finished and rolled back");

$sth = $dbh->prepare('SELECT * FROM PERL_DBD_TEST WHERE 1 = 0');
$sth->execute();
@row = $sth->fetchrow();
if ($sth->err) {
   diag(" $sth->err: " . $sth->err . "\n");
   diag(" $sth->errstr: " . $sth->errstr . "\n");
   diag(" $dbh->state: " . $dbh->state . "\n");
}
ok(!$sth->err, "no error");
$sth->finish();

my ($a, $b);
$sth = $dbh->prepare('SELECT COL_A, COL_B FROM PERL_DBD_TEST');
$sth->execute();
while (@row = $sth->fetchrow())
    {
    print " \@row     a,b:", $row[0], ",", $row[1], "\n";
    }
$sth->finish();

$sth->execute();
$sth->bind_col(1, \$a);
$sth->bind_col(2, \$b);
while ($sth->fetch())
    {
    print " bind_col a,b:", $a, ",", $b, "\n";
    unless (defined($a) && defined($b))
    	{
	print "not ";
	last;
	}
    }
pass("?");
$sth->finish();

($a, $b) = (undef, undef);
$sth->execute();
$sth->bind_columns(undef, \$b, \$a);
while ($sth->fetch())
    {
    print " bind_columns a,b:", $b, ",", $a, "\n";
    unless (defined($a) && defined($b))
    	{
	print "not ";
	last;
	}
    }
pass("??");

$sth->finish();

# turn off error warnings.  We expect one here (invalid transaction state)
$dbh->{RaiseError} = 0;
$dbh->{PrintError} = 0;
$dbh->disconnect();
exit 0;

# avoid warning on one use of DBI::errstr
print $DBI::errstr;

# make sure there is an invalid transaction state error at the end here.
# (XXX not reliable, iodbc-2.12 with "INTERSOLV dBase IV ODBC Driver" == -1)
#print "# DBI::err=$DBI::err\nnot " if $DBI::err ne "25000";
#print "ok 7\n"; 

