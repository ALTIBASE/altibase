#!/usr/bin/perl -I./t -w
# $Id: 09multi.t 11810 2005-05-18 04:43:35Z qa $

use Test::More;

$| = 1;

use_ok('strict');
use_ok('DBI');
use_ok('ODBCTEST');

my $tests;
# to help ActiveState's build process along by behaving (somewhat) if a dsn is not provided
BEGIN {
   $tests = 7;
   if (!defined $ENV{DBI_DSN}) {
      plan skip_all => "DBI_DSN is undefined";
   } else {
      plan tests => $tests;
   }
}

# $ENV{'ODBCINI'}="/export/cmn/etc/odbc.ini" ;
#my($connectString) = "dbi:ODBC:DSN=TESTDB;Database=xxxxx;uid=usrxxxxx;pwd=xxxxx" ;

my $dbh=DBI->connect();
unless($dbh) {
   BAILOUT("Unable to connect to the database $DBI::errstr\nTests skipped.\n");
   exit 0;
}

$dbh->{RaiseError} = 1;
$dbh->{PrintError} = 0;
$dbh->{LongReadLen} = 10000;
SKIP:
{
   skip "Multiple statements not supported using " . $dbh->get_info(17) . " (SQL_MULT_RESULT_SETS)", $tests-3 unless ($dbh->get_info(36) eq "Y");


   my($sqlStr) ;
   my @test_colnames = sort(keys(%ODBCTEST::TestFieldInfo));
   $sqlStr = "select $test_colnames[0] FROM $ODBCTEST::table_name
	      select $test_colnames[0] from $ODBCTEST::table_name" ;
   #$sqlStr = "select emp_id from employee where emp_id = 2
   #           select emp_id, emp_name, address1, address2 from employee where emp_id = 2" ;


   my $result_sets = 0;
   
   my $sth;
   eval {
      $sth = $dbh->prepare($sqlStr);
      $sth->execute;
   };
   
   if ($@) {
      skip("Multiple statements not supported using " . $dbh->get_info(17) . "\n", $tests-3);
   }


   my @row;
   my $cnt = 0;
   $result_sets = 0;

   do {
      # print join(":", @{$sth->{NAME}}), "\n";
      while ( my $ref = $sth->fetch ) {
	 # print join(":", @$ref), "\n";
      }
      $result_sets++;
   } while ( $sth->{odbc_more_results}  ) ;

   is($result_sets, 2, "count number of result sets");

   my $sql;
   my @expected_result_cols;

   # lets get some dummy data for testing.
   ODBCTEST::tab_insert($dbh);

   $sql = "select $test_colnames[0] from $ODBCTEST::table_name order by $test_colnames[0]
	   select $test_colnames[0],$test_colnames[1]  from $ODBCTEST::table_name order by $test_colnames[0]";
   @expected_result_cols = (1, 2);
   ok(RunMultiTest($sql, \@expected_result_cols), "Multiple result sets with different column counts (less then more)");


   $sql = "select $test_colnames[0],$test_colnames[1]  from $ODBCTEST::table_name order by $test_colnames[0]
	   select $test_colnames[0] from $ODBCTEST::table_name order by $test_colnames[0]";

   @expected_result_cols = (2, 1);
   ok(RunMultiTest($sql, \@expected_result_cols), "Multiple result sets with different column counts (more then less)");

   $sql = "select " . join(", ", grep {/COL_[ABC]/} @test_colnames) . " from $ODBCTEST::table_name order by $test_colnames[0]
	   select $test_colnames[0] from $ODBCTEST::table_name order by $test_colnames[0]";

   @expected_result_cols = ($#test_colnames, 1);
   ok(RunMultiTest($sql, \@expected_result_cols), "Multiple result sets with multiple cols, then second result set with one col");


   # clean up the dummy data.
   ODBCTEST::tab_delete($dbh);
};

$dbh->disconnect();


sub RunMultiTest {
   my $sql = shift;
   my $ref_expected_result_cols = shift;

   my @expected_result_cols = @$ref_expected_result_cols;
   my $test_pass = 1;
   my $result_sets = 0;
   $sth = $dbh->prepare($sql);
   $sth->execute;

   do {

      # $#expected_result_cols is the array of number of result cols
      # and the count/array size represents the number of result sets...
      if ($result_sets > $#expected_result_cols) {
	 print "Number of result sets not correct in test $result_sets is more than the expected $#expected_result_cols.\n";
	 $test_pass = 0;
      } else {
	 if ($sth->{NUM_OF_FIELDS} != $expected_result_cols[$result_sets]) {
	    print "Num of fields not correct in result set $result_sets.  Expected $expected_result_cols[$result_sets], found $sth->{NUM_OF_FIELDS}\n";
	    $test_pass = 0;
	 }
      }
      # print join(", ", @{$sth->{NAME}}), "\n";
      my $i = 0;
      while ( my $ref = $sth->fetchrow_arrayref ) {
	 # if ($] > 5.005) {
	 #   no warnings;
	    # print join(":", @$ref), "\n";
         #}
	 my $row = $ODBCTEST::tab_insert_values[$i];
	 
	 my $j;
	 for ($j = 0; $j < $sth->{NUM_OF_FIELDS}; $j++) {
	    if ($row->[$j] ne $ref->[$j]) {
	       print "Data mismatch, result set $result_sets, row $i, col $j ($row->[$j] != $ref->[$j])\n";
	       $test_pass = 0;
	    }
	 }

	 $i++;
      }
      $result_sets++;
   } while ( $sth->{odbc_more_results}  ) ;

   if ($result_sets <= $#expected_result_cols) {
      print "Number of result sets not correct in test (fewer than expected)\n";
      $test_pass = 0;
   }
   $test_pass;
}

exit(0);
print $DBI::errstr;
print $ODBCTEST::tab_insert_values[0];
print sort(keys(%ODBCTEST::TestFieldInfo));

