#!/usr/bin/perl -I./t -w
# $Id: 30Oracle.t 11810 2005-05-18 04:43:35Z qa $

use Test::More;

$| = 1;


# use_ok('DBI', qw(:sql_types));
# can't seem to get the imports right this way
use DBI qw(:sql_types);
use_ok('ODBCTEST');
use_ok('Data::Dumper');

my $tests;
# to help ActiveState's build process along by behaving (somewhat) if a dsn is not provided
BEGIN {
   $tests = 4;
   if (!defined $ENV{DBI_DSN}) {
      plan skip_all => "DBI_DSN is undefined";
   } else {
      plan tests => $tests;
   }
}

my $dbh = DBI->connect();
unless($dbh) {
   BAILOUT("Unable to connect to the database $DBI::errstr\nTests skipped.\n");
   exit 0;
}

my $dbname = $dbh->get_info(17); # DBI::SQL_DBMS_NAME
SKIP:
{
   skip "Oracle tests not supported using " . $dbname, $tests-2 unless ($dbname =~ /Oracle/i);



   $dbh->do("create or replace function PERL_DBD_TESTFUNC(a in integer, b in integer) return integer is c integer; begin if b is null then c := 0; else c := b; end if; return a * c + 1; end;");
   my $sth = $dbh->prepare("{ ? = call PERL_DBD_TESTFUNC(?, ?) }");
   my $value = undef;
   my $b = 30;
   $sth->bind_param_inout(1, \$value, 50, SQL_INTEGER);
   $sth->bind_param(2, 10, SQL_INTEGER);
   $sth->bind_param(3, 30, SQL_INTEGER);
   $sth->execute;
   is($value, 301);

   $b = undef;
   $sth->bind_param_inout(1, \$value, 50, SQL_INTEGER);
   $sth->bind_param(2, 20, SQL_INTEGER);
   $sth->bind_param(3, undef, SQL_INTEGER);
   $sth->execute;
   is($value,1);

};

if (DBI->trace > 0) {
   DBI->trace(0);
}

$dbh->disconnect;
