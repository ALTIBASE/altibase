#!perl -w
# $Id: proctest1.pl 11810 2005-05-18 04:43:35Z qa $


use DBI;
use strict;
use Data::Dumper;

my $dbh = DBI->connect();

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
print 'kaboom'
	RETURN(\@parameter1 + 1)");

$dbh->disconnect;

sub test($;$)
{
   my ($outputTempate, $recurse) = @_;
	
   my $queryInputParameter1 = 2222;
   my $queryOutputParameter = $outputTempate;
   my $dbh = DBI->connect;
   local $dbh->{odbc_async_exec} = 1;
   my $testpass = 0;
   sub err_handler {
      my ($state, $msg) = @_;
      # Strip out all of the driver ID stuff
      $msg =~ s/^(\[[\w\s]*\])+//;
      print "===> state: $state msg: $msg\n";
      $testpass++;
      return 0;
   }
   local $dbh->{odbc_err_handler} = \&err_handler;
	
   my $sth = $dbh->prepare('{? = call PERL_DBD_TESTPRC(?) }');
   $sth->bind_param_inout(1, \$queryOutputParameter, 30, { TYPE => DBI::SQL_INTEGER });
   $sth->bind_param(2, $queryInputParameter1, { TYPE => DBI::SQL_INTEGER });
					
   $sth->execute();

	print '$sth->{Active}: ', $sth->{Active}, "\n";
	if (1) {
	   do {
		 for(my $rowRef; $rowRef = $sth->fetchrow_hashref('NAME'); )  {
		    my %outputData = %$rowRef;
		    
		    print 'outputData ', Dumper(\%outputData), "\n";
		    if($recurse > 0)  {
		       test($dbh, --$recurse);
		    }
		 }
	   } while($sth->{odbc_more_results});
	}
	print '$queryOutputParameter: \'', $queryOutputParameter, 
		'\' expected: (', $queryInputParameter1 + 1, ")\n\n";
	print "Err handler called $testpass times\n";
}




##########################################
### Test
##########################################

unlink("dbitrace.log") if (-e "dbitrace.log");
$dbh->trace(9, "dbitrace.log");
test(0,       0);
test(10,      0);
test(100,     0);
test('     ', 0);

test(0, 1);	#recusion

##########################################
### Cleanup...
##########################################



$dbh->disconnect;

