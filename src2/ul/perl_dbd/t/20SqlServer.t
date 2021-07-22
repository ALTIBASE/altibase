#!/usr/bin/perl -w -I./t
# $Id: 20SqlServer.t 11810 2005-05-18 04:43:35Z qa $

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
   $tests = 37;
   if (!defined $ENV{DBI_DSN}) {
      plan skip_all => "DBI_DSN is undefined";
   } else {
      plan tests => $tests;
   }
}

sub Multiple_concurrent_stmts($) {
   my $dbh = shift;
   my $sth = $dbh->prepare("select * from PERL_DBD_TABLE1");
   $dbh->{RaiseError} = 1;
   $sth->execute;
   my @row;
   eval {
      while (@row = $sth->fetchrow_array()) {
	 my $sth2 = $dbh->prepare("select * from $ODBCTEST::table_name");
	 $sth2->execute;
	 my @row2;
	 while (@row2 = $sth2->fetchrow_array()) {
	 }
      }
   };
   
   if ($@) {
      return 0;
   }
   return 1;
}

my $dbh = DBI->connect();
unless($dbh) {
   BAILOUT("Unable to connect to the database $DBI::errstr\nTests skipped.\n");
   exit 0;
}

my $dbname = $dbh->get_info(17); # DBI::SQL_DBMS_NAME
SKIP: {
   skip "Microsoft SQL Server tests not supported using $dbname", $tests-2 unless ($dbname =~ /Microsoft SQL Server/i);

   
   # the times chosen below are VERY specific to NOT cause rounding errors, but may cause different
   # errors on different versions of SQL Server.
   #
   my @data = (
	       [undef, "z" x 13 ],
	       ["2001-01-01 01:01:01.110", "a" x 12],   # "aaaaaaaaaaaa"
	       ["2002-02-02 02:02:02.123", "b" x 114],
	       ["2003-03-03 03:03:03.333", "c" x 251],
	       ["2004-04-04 04:04:04.443", "d" x 282],
	       ["2005-05-05 05:05:05.557", "e" x 131]
	      );

   eval {
      local $dbh->{PrintError} = 0;
      $dbh->do("DROP TABLE PERL_DBD_TABLE1");
   };

   $dbh->{RaiseError} = 1;
   $dbh->{LongReadLen} = 800;

   my @types = (SQL_TYPE_TIMESTAMP, SQL_TIMESTAMP);
   my $type;
   my @row;
   foreach $type (@types) {
      my $sth = $dbh->func($type, "GetTypeInfo");
      if ($sth) {
	 @row = $sth->fetchrow();
	 $sth->finish();
	 last if @row;
      } else {
       # warn "Unable to get type for type $type\n";
      }	
   }
   BAILOUT("Unable to find a suitable test type for date field\n")
	 unless @row;

   my $datetype = $row[0];
   $dbh->do("CREATE TABLE PERL_DBD_TABLE1 (i INTEGER, time $datetype, str VARCHAR(4000))");


   # Insert records into the database:
   my $sth1 = $dbh->prepare("INSERT INTO PERL_DBD_TABLE1 (i,time,str) values (?,?,?)");
   for (my $i=0; $i<@data; $i++) {
      my ($time,$str) = @{$data[$i]};
      # print "Inserting:  $i, ";
      # print  $time if (defined($time));
      # print " string length " . length($str) . "\n";
      $sth1->bind_param (1, $i,    SQL_INTEGER);
      $sth1->bind_param (2, $time, SQL_TIMESTAMP);
      $sth1->bind_param (3, $str,  SQL_LONGVARCHAR);
      $sth1->execute  or die ($DBI::errstr);
   }

   # Retrieve records from the database, and see if they match original data:
   my $sth2 = $dbh->prepare("SELECT i,time,str FROM PERL_DBD_TABLE1");
   $sth2->execute  or die ($DBI::errstr);
   my $iErrCount = 0;
   while (my ($i,$time,$str) = $sth2->fetchrow_array()) {
      if ((defined($time) && $time ne $data[$i][0]) || defined($time) != defined($data[$i][0])) {
	 diag("Retrieving: $i, $time string length: " . length($str) . "\t!time ");
	 $iErrCount++;
      }

      if ($str  ne $data[$i][1]) {
	 diag("Retrieving: $i, $time string length: " . length($str) . "\t!string ");
	 $iErrCount++;
      }
      # print "\n";
   }
   is($iErrCount, 0, "errors on data comparison");


   eval {
      local $dbh->{RaiseError} = 0;
      $dbh->do("DROP TABLE PERL_DBD_TABLE1");
   };

   my $sql = 'CREATE TABLE #PERL_DBD_TABLE1 (id INT PRIMARY KEY, val VARCHAR(4))';
   $dbh->do($sql);
   # doesn't work with prepare, etc...hmmm why not?
   # $sth = $dbh->prepare($sql);
   # $sth->execute;
   # $sth->finish;

   $sth = $dbh->prepare("INSERT INTO #PERL_DBD_TABLE1 (id, val) VALUES (?, ?)");
   $sth2 = $dbh->prepare("INSERT INTO #PERL_DBD_TABLE1 (id, val) VALUES (?, ?)");
   my @data2 = (undef, 'foo', 'bar', 'blet', undef);
   my $i = 0;
   my $val;
   foreach $val (@data2) {
      $sth2->execute($i++, $val);
   }
   $i = 0;
   $sth = $dbh->prepare("Select id, val from #PERL_DBD_TABLE1");
   $sth->execute;
   $iErrCount = 0;
   while (@row = $sth->fetchrow_array) {
      unless ((!defined($row[1]) && !defined($data2[$i])) || ($row[1] eq $data2[$i])) {
	 $iErrCount++ ;
	 print "$row[1] ne $data2[$i]\n";
      }
      $i++;
   }

   is($iErrCount, 0, "temporary table handling");
   diag("Please upgrade your ODBC drivers to the latest SQL Server drivers available.  For example, 2000.80.194.00 is known to be problematic.  Use MDAC 2.7, if possible\n") if ($iErrCount != 0);

   $dbh->{PrintError} = 0;
   eval {$dbh->do("DROP TABLE PERL_DBD_TABLE1");};
   eval {$dbh->do("CREATE TABLE PERL_DBD_TABLE1 (i INTEGER)");};

   eval {$dbh->do("DROP PROCEDURE PERL_DBD_PROC1");};
   eval {$dbh->do("CREATE PROCEDURE PERL_DBD_PROC1 \@inputval int AS ".
		  "INSERT INTO PERL_DBD_TABLE1 VALUES (\@inputval); " .   
		  "	return \@inputval;");};


   $sth1 = $dbh->prepare ("{? = call PERL_DBD_PROC1(?) }");
   my $output = undef;
   $i = 1;
   $iErrCount = 0;
   while ($i < 4) {
      $sth1->bind_param_inout(1, \$output, 50, DBI::SQL_INTEGER);
      $sth1->bind_param(2, $i, DBI::SQL_INTEGER);

      $sth1->execute();
      # print "$output";
      if ($output != $i) {
	 $iErrCount++;
	 diag("$output error!\n");
      }
      # print "\n";
      $i++;
   }
      
   is($iErrCount, 0, "bind param in out with insert result set");
   $iErrCount = 0;
   eval {$dbh->do("DROP PROCEDURE PERL_DBD_PROC1");};
   my $proc1 =
	      "CREATE PROCEDURE PERL_DBD_PROC1 (\@i int, \@result int OUTPUT) AS ".
	      "BEGIN ".
	      "    SET \@result = \@i+1;".
	      "END ";
   # print "$proc1\n";
   $dbh->do($proc1);

   # $dbh->{PrintError} = 1;
   $sth1 = $dbh->prepare ("{call PERL_DBD_PROC1(?, ?)}");
   $i = 12;
   $output = undef;
   $sth1->bind_param(1, $i, DBI::SQL_INTEGER);
   $sth1->bind_param_inout(2, \$output, 100, DBI::SQL_INTEGER);
   $sth1->execute;
   is($i, $output-1, "test output params accurate");

   $iErrCount = 0;
   $sth = $dbh->prepare("select * from PERL_DBD_TABLE1 order by i");
   $sth->execute;
   $i = 1;
   while (@row = $sth->fetchrow_array) {
      if ($i != $row[0]) {
	 diag(join(', ', @row), " ERROR!\n");
	 $iErrCount++;
      }
      $i++;
   }


   is($iErrCount, 0, "verify select data");

   eval {$dbh->do("DROP TABLE PERL_DBD_TABLE1");};
   eval {$dbh->do("CREATE TABLE PERL_DBD_TABLE1 (d DATETIME)");};
   $sth = $dbh->prepare ("INSERT INTO PERL_DBD_TABLE1 (d) VALUES (?)");
   $sth->bind_param (1, undef, SQL_TYPE_TIMESTAMP);
   $sth->execute();
   $sth->bind_param (1, "2002-07-12 05:08:37.350", SQL_TYPE_TIMESTAMP);
   $sth->execute();
   $sth->bind_param (1, undef, SQL_TYPE_TIMESTAMP);
   $sth->execute();
      
   $iErrCount = 0;
   $sth2 = $dbh->prepare("select * from PERL_DBD_TABLE1 where d is not null");
   $sth2->execute;
   while (@row = $sth2->fetchrow_array) {
      if ($row[0] ne "2002-07-12 05:08:37.350") {
	 $iErrCount++ ;
	 diag(join(", ", @row), "\n");
      }
   }
   is($iErrCount, 0, "timestamp handling");

   eval {$dbh->do("DROP TABLE PERL_DBD_TABLE1");};
   eval {$dbh->do("DROP PROCEDURE PERL_DBD_PROC1");};

   eval {$dbh->do("CREATE TABLE PERL_DBD_TABLE1 (i INTEGER, j integer)");};
   $proc1 = <<EOT;
CREATE PROCEDURE PERL_DBD_PROC1 (\@i INT) AS
DECLARE \@result INT;
BEGIN
   SET \@result = \@i;
   IF (\@i = 99)
      BEGIN
	 UPDATE PERL_DBD_TABLE1 SET i=\@i;
	 SET \@result = \@i + 1;
      END;
   SELECT \@result;
END
EOT
   $dbh->{RaiseError} = 0;
   eval {$dbh->do($proc1);};
   my $sth = $dbh->prepare ("{call PERL_DBD_PROC1 (?)}");
   my $success = -1;

   $sth->bind_param (1, 99, SQL_INTEGER);
   $sth->execute();
   $success = -1;
   while (my @data = $sth->fetchrow_array()) {($success) = @data;}
   is($success, 100, "procedure outputs results as result set");

   $sth->bind_param (1, 10, SQL_INTEGER);
   $sth->execute();
   $success = -1;
   while (my @data = $sth->fetchrow_array()) {($success) = @data;}
   is($success,10, "procedure outputs results as result set2");

   $sth->bind_param (1, 111, SQL_INTEGER);
   $sth->execute();
   $success = -1;
   do {
      my @data;
      while (@data = $sth->fetchrow_array()) {
	 if ($#data == 0) {
	    ($success) = @data;
	 }
      }
   } while ($sth->{odbc_more_results});
   is($success, 111, "procedure outputs results as result set 3");


#
# special tests for even stranger cases...
#
   eval {$dbh->do("DROP PROCEDURE PERL_DBD_PROC1");};
   $proc1 = <<EOT;
   CREATE PROCEDURE PERL_DBD_PROC1 (\@i INT) AS
   DECLARE \@result INT;
   BEGIN
   SET \@result = \@i;
   IF (\@i = 99)
      BEGIN
	 UPDATE PERL_DBD_TABLE1 SET i=\@i;
	 SET \@result = \@i + 1;
      END;
   IF (\@i > 100)
      BEGIN
	 INSERT INTO PERL_DBD_TABLE1 (i, j) VALUES (\@i, \@i);
	 SELECT i, j from PERL_DBD_TABLE1;
      END;
   SELECT \@result;
   END
EOT

   eval {$dbh->do($proc1);};

   # set the required attribute and check it.
   $dbh->{odbc_force_rebind} = 1;
   is($dbh->{odbc_force_rebind}, 1, "setting force_rebind");
   $dbh->{odbc_force_rebind} = 0;
   is($dbh->{odbc_force_rebind}, 0, "resetting force_rebind");

   $sth = $dbh->prepare ("{call PERL_DBD_PROC1 (?)}");
   is($sth->{odbc_force_rebind}, 0, "testing force rebind after procedure call");
   $success = -1;

   $sth->bind_param (1, 99, SQL_INTEGER);
   $sth->execute();
   $success = -1;
   while (my @data = $sth->fetchrow_array()) {($success) = @data;}
   is($success, 100, "force rebind test part 2");

   $sth->bind_param (1, 10, SQL_INTEGER);
   $sth->execute();
   $success = -1;
   while (my @data = $sth->fetchrow_array()) {($success) = @data;}
   is($success, 10, "force rebind test part 3");

   $sth->bind_param (1, 111, SQL_INTEGER);
   $sth->execute();
   $success = -1;
   do {
      my @data;
      while (@data = $sth->fetchrow_array()) {
	 if ($#data == 0) {
	    ($success) = @data;
	 } else {
	    # diag("Data: ", join(',', @data), "\n");
	 }
      }
   } while ($sth->{odbc_more_results});
   is($success, 111, "force rebind test part 4");

   # ensure the attribute is automatically set.
   # the multiple result sets will trigger this.
   is($sth->{odbc_force_rebind}, 1, "forced rebind final");


   #
   # more special tests
   # make sure output params are being set properly when
   # multiple result sets are available.  Also, ensure fetchrow_hashref
   # works with multiple statements.
   #
   eval {$dbh->do("DROP PROCEDURE PERL_DBD_PROC1");};
   $dbh->do("CREATE PROCEDURE  PERL_DBD_PROC1
\@parameter1 int = 22
AS
	/* SET NOCOUNT ON */
	select 1 as some_data
	select isnull(\@parameter1, 0) as parameter1, 3 as some_more_data
	RETURN(\@parameter1 + 1)");

   my $queryInputParameter1 = 2222;
   my $queryOutputParameter = 0;

   $sth = $dbh->prepare('{? = call PERL_DBD_PROC1(?) }');
   $sth->bind_param_inout(1, \$queryOutputParameter, 30, { TYPE => DBI::SQL_INTEGER });
   $sth->bind_param(2, $queryInputParameter1, { TYPE => DBI::SQL_INTEGER });

   $sth->execute();
   
   do {
      for(my $rowRef; $rowRef = $sth->fetchrow_hashref('NAME'); )  {
	 my %outputData = %$rowRef;
	 if (defined($outputData{some_data})) {
	    is($outputData{some_data},1,"Select data available");
	    ok(!defined($outputData{parameter1}), "output param not yet available");
	    ok(!defined($outputData{some_more_data}), "output param not yet available2");
	 } else {
	    is($outputData{parameter1},2222, "Output param data available");
	    is($outputData{some_more_data},3, "Output param data available 2");
	    ok(!defined($outputData{some_data}), "select data done");
	 }
	 # diag('outputData ', Dumper(\%outputData), "\n");
      }
      # print "out of for loop\n";
   } while($sth->{odbc_more_results});
   # print "out of while loop\n";
   is($queryOutputParameter, $queryInputParameter1 + 1, "valid output data");

   # test a procedure with no parameters
   eval {$dbh->do("DROP PROCEDURE PERL_DBD_PROC1");}; 
   eval {$dbh->do("CREATE PROCEDURE PERL_DBD_PROC1 AS return 1;");}; 

   $sth1 = $dbh->prepare ("{ ? = call PERL_DBD_PROC1 }"); 
   $output = undef; 
   $iErrCount = 0; 
   $sth1->bind_param_inout(1, \$output, 50, DBI::SQL_INTEGER); 

   $sth1->execute();
   is($output, 1, "test procedure with no input params");


   $dbh->{odbc_async_exec} = 1;
   # print "odbc_async_exec is: $dbh->{odbc_async_exec}\n";
   is($dbh->{odbc_async_exec}, 1, "test odbc_async_exec attribute set");

   # not sure if this should be a test.  May have permissions problems, but it's the only sample
   # of the error handler stuff I have.
   my $testpass = 0;
   my $lastmsg;

   sub err_handler {
      my ($state, $msg, $nativeerr) = @_;
      # Strip out all of the driver ID stuff
      $msg =~ s/^(\[[\w\s:]*\])+//;
      $lastmsg = $msg;
      print "===> state: $state msg: $msg nativeerr: $nativeerr\n";
      $testpass++;
      return 0;
   }

   $dbh->{odbc_err_handler} = \&err_handler;

   $sth = $dbh->prepare("dbcc TRACESTATUS(-1)");
   $sth->execute;
   cmp_ok($testpass, '>', 0, "dbcc messages being returned");
   $testpass = 0;
   $dbh->{odbc_async_exec} = 0;
   is($dbh->{odbc_async_exec}, 0, "reset async exec");
   
   # DBI->trace(9);
   $dbh->{odbc_exec_direct} = 1;
   is($dbh->{odbc_exec_direct}, 1, "test setting odbc_exec_direct");
   $sth2 = $dbh->prepare("print 'START' select count(*) from perl_dbd_table1 print 'END'");
   $sth2->execute;
   do {
      while (@row = $sth2->fetchrow_array) {
	 is($row[0], 1, "Valid select results with print statements");
      }
   } while ($sth2->{odbc_more_results});
   
   is($testpass,2, "ensure 2 error messages from two print statements");
   is($lastmsg, 'END', "validate error messages being retrieved");

   if (DBI->trace > 0) {
      DBI->trace(0);
   }
   # need the finish if there are print statements (for now)
   #$sth2->finish;

   $dbh->do("insert into perl_dbd_table1 (i, j) values (1, 2)");
   $dbh->do("insert into perl_dbd_table1 (i, j) values (3, 4)");

   ok(!&Multiple_concurrent_stmts($dbh), "Multiple concurrent statements should fail");

   $dbh->disconnect;
   $dbh = DBI->connect($ENV{DBI_DSN}, $ENV{DBI_USER}, $ENV{DBI_PASS}, { odbc_cursortype => 2 });
   # $dbh->{odbc_err_handler} = \&err_handler;
   ok(&Multiple_concurrent_stmts($dbh), "Multiple concurrent statements succeed (odbc_cursortype set)");


   # clean up test table and procedure
   # reset err handler
   # $dbh->{odbc_err_handler} = undef;
   eval {$dbh->do("DROP TABLE PERL_DBD_TABLE1");};
   eval {$dbh->do("DROP PROCEDURE PERL_DBD_PROC1");};

   eval { local $dbh->{PrintError} = 0; $dbh->do("drop table perl_dbd_test1"); };
   $dbh->do("create table perl_dbd_test1 (i integer primary key, t varchar(30))");
   $dbh->{AutoCommit} = 0;
   $dbh->do("insert into perl_dbd_test1 (i, t) values (1, 'initial')");
   $dbh->commit;
   $dbh->do("update perl_dbd_test1 set t = 'second' where i = 1");

   my $dbh2 = DBI->connect($ENV{DBI_DSN}, $ENV{DBI_USER}, $ENV{DBI_PASS}, {odbc_query_timeout => 2, PrintError=>0});
   # $dbh2->{odbc_query_timeout} = 5;
   $dbh2->{AutoCommit} = 0;
   $dbh2->do("update perl_dbd_test1 set t = 'bad' where i = ?",undef,1);
   $dbh2->rollback;
   # should timeout and get to here.  if so, test will pass
   pass("passed timeout on query using odbc_query_timeout using do with bind params");
   $dbh2->do("update perl_dbd_test1 set t = 'bad' where i = 1");
   $dbh2->rollback;
   $dbh2->disconnect;
   pass("passed timeout on query using odbc_query_timeout using do without bind params");
   $dbh->commit;
   $dbh->do("drop table perl_dbd_test1");
   $dbh->commit;

};

   $dbh->disconnect;
      

exit 0;
# get rid of use once warnings
print $DBI::errstr;
print $ODBCTEST::table_name;