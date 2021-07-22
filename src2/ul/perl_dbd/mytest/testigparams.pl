#!/usr/bin/perl -w -I./t
# $Id: testigparams.pl 11810 2005-05-18 04:43:35Z qa $


# use strict;
use DBI qw(:sql_types);

my (@row);


my $dbh = DBI->connect('dbi:ODBC(RaiseError=>1):PERL_TEST_ORACLE');
#$dbh->{RaiseError} = 1;
# ------------------------------------------------------------
# oracle specific 
$dbh->do("create or replace function testfunc(a in integer, b in integer) return integer is c integer; begin if b is null then c := 0; else c := b; end if; return a * c + 1; end;");
#my $sth = $dbh->prepare("begin ? := testfunc(?, ?); end;");
DBI->trace(9,"c:/trace.txt");
#$dbh->do('CREATE FUNCTION testfunc (@p1 int, @p2 int) RETURNS INT AS BEGIN RETURN (@p1+@p2) END');
my $sth = $dbh->prepare("{ ? = call testfunc(?, ?) }");
my $value = 0;
my $b = 30;
$sth->bind_param_inout(1, \$value, 50, SQL_INTEGER);
$sth->bind_param(2, 10, SQL_INTEGER);
$sth->bind_param(3, 30, SQL_INTEGER);
$sth->execute;
print $value, "\n";
$b = undef;
$sth->bind_param_inout(1, \$value, 50, SQL_INTEGER);
$sth->bind_param(2, 20, SQL_INTEGER);
$sth->bind_param(3, undef, SQL_INTEGER);
$sth->execute;
print $value, "\n";


$dbh->do("create or replace function testfunc(a in integer, b in out integer) return integer is begin if b is null then b := 0; end if; b := b + 1; return a * b + 1; end;");
$sth = $dbh->prepare("{ ? = call testfunc(?, ?) }");
$value = 0;
$b = 30;
$sth->bind_param_inout(1, \$value, 50, SQL_INTEGER);
$sth->bind_param(2, 10, SQL_INTEGER);
$sth->bind_param_inout(3, \$b, 50, SQL_INTEGER);
$sth->execute;
print $value, ", $b\n";
$b = 10;
$sth->bind_param_inout(1, \$value, 50, SQL_INTEGER);
$sth->bind_param(2, 20, SQL_INTEGER);
$sth->bind_param_inout(3, \$b, 50, SQL_INTEGER);
$sth->execute;
print $value, ", $b\n";
$dbh->disconnect();

