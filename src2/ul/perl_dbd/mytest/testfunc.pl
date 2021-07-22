#!/usr/bin/perl -w -I./t
# $Id: testfunc.pl 11810 2005-05-18 04:43:35Z qa $


# use strict;
use DBI qw(:sql_types);
# use DBD::ODBC::Const qw(:sql_types);

my (@row);

my $dbh = DBI->connect('dbi:ODBC:PERL_TEST_SQLSERVER',{AutoCommit=>1, RaisError=>1})
	  or exit(0);
# ------------------------------------------------------------

# dumb, for now...
# SQL_DRIVER_VER returns string
# SQL_CURSOR_COMMIT_BEHAVIOR returns 16 bit value
# SQL_ALTER_TABLE returns 32 bit value 
# SQL_ACCESSIBLE_PROCEDURES returns short string (Y or N)

my %InfoTests = (
		 'SQL_DRIVER_NAME', 6,
		 'SQL_DRIVER_VER', 7,
		 'SQL_DRIVER_ODBC_VER', 77,
		 'SQL_DATABASE_NAME', 16,
		 'SQL_DBMS_NAME', 17,
		 'SQL_DBMS_VER', 18,
		 'SQL_IDENTIFIER_QUOTE_CHAR', 29,
		 'SQL_DM_VER', 171,
		 'SQL_CATALOG_NAME_SEPARATOR', 41,
		 'SQL_CATALOG_LOCATION', 114,
		 'SQL_CURSOR_COMMIT_BEHAVIOR', 23,
		 'SQL_ALTER_TABLE', 86,
		 'SQL_ACCESSIBLE_PROCEDURES', 20,
		 'SQL_PROCEDURES', 21,
		 'SQL_MULT_RESULT_SETS', 36,
		 'SQL_PROCEDURE_TERM', 40,
		);

my %TypeTests = (
		 'SQL_ALL_TYPES' => 0,
		 'SQL_VARCHAR' => SQL_VARCHAR,
		 'SQL_CHAR' => SQL_CHAR,
		 'SQL_INTEGER' => SQL_INTEGER,
		 'SQL_SMALLINT' => SQL_SMALLINT,
		 'SQL_NUMERIC' => SQL_NUMERIC,
		 'SQL_LONGVARCHAR' => SQL_LONGVARCHAR,
		 'SQL_LONGVARBINARY' => SQL_LONGVARBINARY,
		 'SQL_WVARCHAR' => SQL_WVARCHAR,
		 'SQL_WCHAR' => SQL_WCHAR,
		 'SQL_WLONGVARCHAR' => SQL_WLONGVARCHAR,
		);

my $ret; 
print "\nInformation for DBI_DSN=$ENV{'DBI_DSN'}\n\n";
my $SQLInfo;
foreach $SQLInfo (sort keys %InfoTests) {
   $ret = 0;
   $ret = $dbh->get_info($InfoTests{$SQLInfo});
   print "$SQLInfo ($InfoTests{$SQLInfo}):\t$ret\n";
}

print "\nGetfunctions   : ", join(",", $dbh->func(0, GetFunctions)), "\n\n";
print "\nGetfunctions v3: ", join(",", $dbh->func(999, GetFunctions)), "\n\n";

foreach $SQLInfo (sort keys %TypeTests) {
   print "Listing all $SQLInfo types\n";
   $sth = $dbh->func($TypeTests{$SQLInfo}, GetTypeInfo);
   if ($sth) {
      my $colcount = $sth->func(1, 0, ColAttributes); # 1 for col (unused) 0 for SQL_COLUMN_COUNT
      # print "Column count is $colcount\n";
      my $i;
      my @coldescs = ();
      # column 0 should be an error/blank
      for ($i = 0; $i <= $colcount; $i++) {
         # $i is colno (1 based) 2 is for SQL_COLUMN_TYPE
	 # $i == 0 is intentional error...tests error handling.
	 my $stype = $sth->func($i, 2, ColAttributes);
	 my $sname = $sth->func($i, 1, ColAttributes);
	 # print "Col Attributes (TYPE): ", &nullif($stype), "\n";
	 # print "Col Attributes (NAME): ", &nullif($sname), "\n";
	 push(@coldescs, $sname);
	 # print "Desc Col: ", join(', ', &nullif($sth->func($i, DescribeCol))), "\n";
      }	
      # print join(', ', @coldescs), "\n";
      while (@row = $sth->fetchrow()) {
	 
	 print "\t$row[0]\n",
	 # &nullif($row[1]), ", " ,
	 #&nullif($row[2]), ", " ,
	 #&nullif($row[3]), ", " ,
	 #&nullif($row[4]), ", " ,
	 #&nullif($row[5]), "\n";
	 # print join(', ', @row), "\n";
      }
      $sth->finish();
   } else {
      # no info on that type...
      print "no info for this type\n";
   }
}	

my $SQL_XOPEN_CLI_YEAR = 10000;
print "\nSQL_XOPEN_CLI_YEAR = ", $dbh->get_info($SQL_XOPEN_CLI_YEAR), "\n";
$dbh->disconnect();

sub nullif ($) {
   my $val = shift;
   $val ? $val : "(null)";
}
