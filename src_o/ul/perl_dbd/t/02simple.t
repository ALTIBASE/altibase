#!perl -w -I./t
# $Id: 02simple.t 11810 2005-05-18 04:43:35Z qa $

use Test::More;
$| = 1;

use_ok('DBI', qw(:sql_types));
use_ok('ODBCTEST');

# to help ActiveState's build process along by behaving (somewhat) if a dsn is not provided
BEGIN {
   if (!defined $ENV{DBI_DSN}) {
      plan skip_all => "DBI_DSN is undefined";
   } else {
      plan tests => 36;
   }
}


#DBI->trace(2);
my $dbh = DBI->connect();
unless($dbh) {
   BAILOUT("Unable to connect to the database ($DBI::errstr)\nTests skipped.\n");
   exit 0;
}

$dbh->{AutoCommit} = 1;
pass("Set Auto commit");
is($dbh->{AutoCommit}, 1, "Auto commit retrieved to what was set");

#### testing a simple select

my $rc = 0;
ok(ODBCTEST::tab_create($dbh), "create test table");

cmp_ok(ODBCTEST::tab_exists($dbh), '>=', 0, "test table exists");

ok(ODBCTEST::tab_insert($dbh), "insert test data");

ok(tab_select($dbh), "select test data");

$rc = undef;
$dbh->{LongReadLen} = 50;
is($dbh->{LongReadLen}, 50, "Set Long Read len");
$dbh->{LongTruncOk} = 1;
is($dbh->{LongTruncOk}, 1, "Set Long Truncok 1");
$dbh->{PrintError} = 0;
is($dbh->{PrintError}, '', "Set Print Error");

ok(select_long($dbh), "Select Long data");

# now force an error and ensure we get a long truncated event.
$dbh->{LongTruncOk} = 0;
is($dbh->{LongTruncOk}, '', "Set Long Truncok 0");
ok(!select_long($dbh), "Select Long Data failure");

my $sth = $dbh->prepare("SELECT * FROM $ODBCTEST::table_name ORDER BY COL_A");
ok(defined($sth), "prepare select from table");
if ($sth) {
   ok($sth->execute(), "Execute select");
   my $colcount = $sth->func(1, 0, ColAttributes); # 1 for col (unused) 0 for SQL_COLUMN_COUNT
   # diag("Column count is: $colcount\n");
   my ($coltype, $colname, $i, @row);
   my $is_ok = 0;
   for ($i = 1; $i <= $colcount; $i++) {
		# $i is colno (1 based) 2 is for SQL_COLUMN_TYPE, 1 is for SQL_COLUMN_NAME
      $coltype = $sth->func($i, 2, ColAttributes);
      $colname = $sth->func($i, 1, ColAttributes);
      #diag("$i: $colname = $coltype ", $coltype+1-1);
      if (grep { $coltype == $_ } @{$ODBCTEST::TestFieldInfo{$colname}}) {
	 $is_ok++;
      } else {
	 diag("Coltype $coltype not found in list ", join(', ', @{$ODBCTEST::TestFieldInfo{$colname}}), "\n");
      }
   }
   is($is_ok, $colcount, "Col count matches correct col count");
   # print "not " unless $is_ok == $colcount;
   # print "ok 9\n";
	
   $sth->finish;
} else {
   fail("select didn't work, so column count won't work");
}


$dbh->{RaiseError} = 0;
is($dbh->{RaiseError}, '', "Set RaiseError 0");
$dbh->{PrintError} = 0;
is($dbh->{PrintError}, '', "Set PrintError 0");
#
# some ODBC drivers will prepare this OK, but not execute.
# 
$sth = $dbh->prepare("SELECT XXNOTCOLUMN FROM $ODBCTEST::table_name");
$sth->execute() if $sth;
cmp_ok(length($DBI::errstr), '>', 0, "Error reported on bad query");

my @row = ODBCTEST::get_type_for_column($dbh, 'COL_D');

my $dateval;
if (ODBCTEST::isDateType($row[1])) {
   $dateval = "{d '1998-05-13'}";
} else {
   $dateval = "{ts '1998-05-13 12:13:01'}";
}

$sth = $dbh->prepare("SELECT COL_D FROM $ODBCTEST::table_name WHERE COL_D > $dateval");
ok(defined($sth), "date check select");
ok($sth->execute(), "date check execute");
my $count = 0;
while (@row = $sth->fetchrow) {
	$count++ if ($row[0]);
	# diag("$row[0]\n");
}
is($count, 1, "date check rows");

$sth = $dbh->prepare("SELECT COL_A, COUNT(*) FROM $ODBCTEST::table_name GROUP BY COL_A");
ok($sth, "group by query prepare");
ok($sth->execute(), "group by query execute");
$count = 0;
while (@row = $sth->fetchrow) {
	$count++ if ($row[0]);
	# diag("$row[0], $row[1]\n");
}
cmp_ok($count, '!=', 0, "group by query returned rows");

$rc = ODBCTEST::tab_delete($dbh);

# Note, this test will fail if no data sources defined or if
# data_sources is unsupported.
my @data_sources = DBI->data_sources('ODBC');
#diag("Data sources:\n\t", join("\n\t",@data_sources),"\n\n");
cmp_ok($#data_sources, '>', 0, "data sources test");


ok($dbh->ping, "test ping method");

is($dbh->{odbc_ignore_named_placeholders}, 0, "Attrib odbc_ignore_named_placeholders 0 to start");
$dbh->{odbc_ignore_named_placeholders} = 1;
is($dbh->{odbc_ignore_named_placeholders}, 1, "Attrib odbc_ignore_named_placeholders set to 1");

my $dbh2 = DBI->connect();
ok(defined($dbh2), "test connecting twice to the same database");
$dbh2->disconnect;


my $dbname;
$dbname = $dbh->get_info(17); # SQL_DBMS_NAME
# diag(" connected to $dbname\n");
ok(defined($dbname) && $dbname ne '', "database name is returned successfully");
#print "\nnot " unless (defined($dbname) && $dbname ne '');
#print "ok 17\n";

$sth = $dbh->prepare("select count(*) from $ODBCTEST::table_name");
$sth->execute;
$sth->fetch;
ok($sth->execute, "automatically finish when execute run again");

#DBI->trace(9, "c:/trace.txt");
# TBD: Make skip block!
my $connstr = $ENV{DBI_DSN};
SKIP: {
   skip "DSN already contains DRIVER= or DSN=", 3 unless (!($connstr =~ /DSN=/i || $connstr =~ /DRIVER=/i));
   $connstr =~ s/ODBC:/ODBC:DSN=/;

   my $dbh3 = DBI->connect($ENV{DBI_DSN} . "x", $ENV{DBI_USER}, $ENV{DBI_PASS}, {RaiseError=>0, PrintError=>0});
   ok(defined($DBI::errstr), "INVALID DSN Test: " . $DBI::errstr . "\n");
   $dbh3->disconnect if (defined($dbh3));
   
   $dbh3 = DBI->connect($connstr, $ENV{DBI_USER}, $ENV{DBI_PASS}, {RaiseError=>0, PrintError=>0});
   ok(defined($dbh3), "Connection with DSN=");
   $dbh3->disconnect if (defined($dbh3));

   $dbh3 = DBI->connect($connstr . ";UID=$ENV{DBI_USER};PWD=$ENV{DBI_PASS}",undef,undef, {RaiseError=>0, PrintError=>0});
   ok(defined($dbh3), "Connection with DSN= and uid and pwd are set");
   $dbh3->disconnect if (defined($dbh3));

};
	    
# Test(1);
# clean up
$sth->finish;
exit(0);

sub tab_select
{
    my $dbh = shift;
    my @row;
    my $rowcount = 0;

    $dbh->{LongReadLen} = 1000;

    my $sth = $dbh->prepare("SELECT * FROM $ODBCTEST::table_name ORDER BY COL_A")
		or return undef;
    $sth->execute();
    while (@row = $sth->fetchrow())	{
       # print "$row[0]|$row[1]|$row[2]|\n";
       ++$rowcount;
       if ($rowcount != $row[0]) {
	    # print "Basic retrieval of rows not working!\nRowcount = $rowcount, while retrieved value = $row[0]\n";
	  $sth->finish;
	  return 0;
       }
    }
    $sth->finish();
    
    $sth = $dbh->prepare("SELECT COL_A,COL_C FROM $ODBCTEST::table_name WHERE COL_A>=4")
	   or return undef;
    $sth->execute();
    while (@row = $sth->fetchrow()) {
	if ($row[0] == 4) {
	    if ($row[1] eq $ODBCTEST::longstr) {
	       # print "retrieved ", length($ODBCTEST::longstr), " byte string OK\n";
	    } else {
	       diag("Basic retrieval of longer rows not working!\nRetrieved value = $row[0] vs $ODBCTEST::longstr\n");
		return 0;
	    }
	} elsif ($row[0] == 5) {
	    if ($row[1] eq $ODBCTEST::longstr2) {
	       # print "retrieved ", length($ODBCTEST::longstr2), " byte string OK\n";
	    } else {
	       diag(print "Basic retrieval of row longer than 255 chars not working!" . 
						"\nRetrieved ", length($row[1]), " bytes instead of " .
						length($ODBCTEST::longstr2) . "\nRetrieved value = $row[1]\n");
		return 0;
	    }
	}
    }

    return 1;
}


sub select_long
{
	my $dbh = shift;
	my @row;
	my $sth;
	my $rc = undef;
	
	$dbh->{RaiseError} = 1;
	$sth = $dbh->prepare("SELECT COL_A,COL_C FROM $ODBCTEST::table_name WHERE COL_A=4");
	if ($sth) {
		$sth->execute();
		eval {
			while (@row = $sth->fetchrow()) {
			}
		};
		$rc = 1 unless ($@) ;
	}
	$rc;
}

__END__




