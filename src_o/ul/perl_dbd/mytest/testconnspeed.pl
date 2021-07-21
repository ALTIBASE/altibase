#!perl -w

use strict;
use DBI;
use Benchmark qw(timethese cmpthese timeit countit timestr);

my %conns = (
	     DBD_ORACLE => sub { doconnect("dbi:Oracle:URLWINLT"); },
	     DBD_ODBC_ORACLE => sub { doconnect("dbi:ODBC:PERL_TEST_ORACLE"); },
	     DBD_ODBC_MSORACLE => sub { doconnect("dbi:ODBC:PERL_TEST_MSORACLE"); },
	     DBD_ODBC_SQLSERVER => sub { doconnect("dbi:ODBC:PERL_TEST_SQLSERVER"); },
	     DBD_ODBC_DB2 => sub { doconnect("dbi:ODBC:PERL_TEST_DB2"); },
	     DBD_ODBC_ACCESS => sub { doconnect("dbi:ODBC:PERL_TEST_ACCESS"); },
	    );

sub doconnect ($) {
   my $connstr = shift;
   my $dbh = DBI->connect($connstr,
                          $ENV{DBI_USER},
                          $ENV{DBI_PASS},
                          { RaiseError => 1, PrintError => 1 }
   ) || die "Can't connect with $connstr: $DBI::errstr";
}

timethese 100, \%conns;

cmpthese 100, \%conns;
