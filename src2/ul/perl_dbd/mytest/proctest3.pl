#!perl -w
# $Id: proctest3.pl 11810 2005-05-18 04:43:35Z qa $


use DBI;
use strict;

my $dbh = DBI->connect();
$dbh->{LongReadLen} = 8000;

eval {
   local $dbh->{PrintError} = 0;
   $dbh->do("drop procedure PERL_DBD_TESTPRC");
};

$dbh->do("CREATE PROCEDURE  PERL_DBD_TESTPRC
\@parameter1 int = 0
AS
	if (\@parameter1 >= 0)
	    select * from systypes
        RETURN(\@parameter1)
	");

sub test()
{
   my $sth = $dbh->prepare("{call PERL_DBD_TESTPRC(?)}");
	
   $sth->bind_param(1, -1, { TYPE => 4 });
   $sth->execute();

   print '$sth->{NUM_OF_FIELDS}: ', $sth->{NUM_OF_FIELDS}, " expected: 0\n";
   if($sth->{NUM_OF_FIELDS}) {
      my @row;
      while (@row = $sth->fetchrow_array()) {
	 print join(', ', @row), "\n";
      }
   }
}

	

##########################################
### Test
##########################################

unlink("dbitrace.log") if (-e "dbitrace.log");
$dbh->trace(9, "dbitrace.log");

test();

##########################################
### Cleanup...
##########################################


$dbh->disconnect;

