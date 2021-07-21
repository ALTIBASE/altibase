#!perl -w
# $Id: proctest2.pl 11810 2005-05-18 04:43:35Z qa $


use DBI;
use strict;
use Data::Dumper;

unlink("dbitrace.log") if (-e "dbitrace.log");
DBI->trace(9, "dbitrace.log");

my $dbh = DBI->connect();
$dbh->{LongReadLen} = 8000;
$dbh->{FetchHashKeyName} = 'NAME_uc';
my $dbh2 = DBI->connect();
$dbh2->{LongReadLen} = 8000;
$dbh2->{FetchHashKeyName} = 'NAME_uc';

eval {
   local $dbh->{PrintError} = 0;
   $dbh->do("drop procedure PERL_DBD_TESTPRC");
};

$dbh->do("CREATE PROCEDURE  PERL_DBD_TESTPRC
\@parameter1 int = 22
AS
	/* SET NOCOUNT ON */
	select 1 as some_data
	select isnull(\@parameter1, 0) as parameter1, 3 as some_more_data
--	 print 'kaboom'
	RETURN(\@parameter1 + 1)");

my $innerTestSth;

sub innerTest($)
{
   my ($outputTempate) = @_;
	
   my %outputData;
   my $queryInputParameter1 = 2222;
   my $queryOutputParameter = $outputTempate;
	
   if(!defined $innerTestSth) {
      $innerTestSth = $dbh2->prepare('{? = call PERL_DBD_TESTPRC(?) }');
   }
	
   $innerTestSth->bind_param_inout(1, \$queryOutputParameter, 30, { TYPE => DBI::SQL_INTEGER });
   $innerTestSth->bind_param(2, $queryInputParameter1, { TYPE => DBI::SQL_INTEGER });
					
#	$sth->trace(1);#, 'DbiTest.txt');
   $innerTestSth->execute();

   print '$innerTestSth->{Active}: ', $innerTestSth->{Active}, "\n";
   do {
      my $rowRef;
      undef $rowRef;
      print "Columns: ", join(', ', @{$innerTestSth->{NAME}}), "\n";
      for(;$rowRef = $innerTestSth->fetchrow_hashref(); ) {
	 print '%$rowRef2 ', Dumper(\%$rowRef), "\n";
      }
   } while($innerTestSth->{odbc_more_results});

   print '$queryOutputParameter: \'', $queryOutputParameter, '\' expected: (', $queryInputParameter1 + 1, ")\n\n";
}


sub test($)
{
   my ($outputTempate) = @_;
	
   my $queryInputParameter1 = 2222;
   my $queryOutputParameter = $outputTempate;
   my $sth = $dbh->prepare('select ID from (select 1 as ID union select 2 as ID union select 3 as ID) tmp order by ID');
					
   $sth->execute();
   do {
      for(my $rowRef = undef; $rowRef = $sth->fetchrow_hashref('NAME'); ) {
	 print '%$rowRef ', Dumper(\%$rowRef), "\n";
	 innerTest($outputTempate);
      }
   } while($sth->{odbc_more_results});
   
}
	





##########################################
### Test
##########################################


test(10);

##########################################
### Cleanup...
##########################################


$dbh2->disconnect;
$dbh->disconnect;

