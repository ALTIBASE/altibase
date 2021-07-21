#!perl -w
# $Id: joetest5.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI qw(:sql_types);
my $dbh=DBI->connect() or die "Can't connect";
$dbh->{RaiseError} = 1;
$dbh->{PrintError} = 0;

eval {$dbh->do("DROP PROCEDURE PERL_DBD_PROC1");};
eval {$dbh->do("CREATE PROCEDURE PERL_DBD_PROC1 \@inputval int AS ".
			"	return \@inputval;");};


my $sth1 = $dbh->prepare ("{? = call PERL_DBD_PROC1(?) }");
my $output = undef;
my $i = 1;
my $iErrCount = 0;
while ($i < 4) {
   $sth1->bind_param_inout(1, \$output, 50, DBI::SQL_INTEGER);
   $sth1->bind_param(2, $i, DBI::SQL_INTEGER);

   $sth1->execute();
   print "$output";
   if ($output != $i) {
      $iErrCount++;
      print " error!";
   }
   print "\n";
   $i++;
}

eval {$dbh->do("DROP PROCEDURE proc1");};
my $proc1 =
    "CREATE PROCEDURE proc1 (\@i int, \@result int OUTPUT) AS ".
    "BEGIN ".
    "    SET \@result = \@i+1;".
    "END ";
print "$proc1\n";
$dbh->do($proc1);

my $sth = $dbh->prepare ("{call proc1(?,?)}");
my $val = 12;
my $result = undef;
$sth->bind_param (1, $val, SQL_INTEGER);
$sth->bind_param_inout (2, \$result, 100, SQL_INTEGER);
$sth->execute;
print "result = $result\n";

$result = undef;
$sth->execute;

$dbh->disconnect;
