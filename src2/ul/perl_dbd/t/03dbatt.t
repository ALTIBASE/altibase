#!perl -w -I./t
# $Id: 03dbatt.t 11810 2005-05-18 04:43:35Z qa $

use Test::More;

$|=1;

use_ok('DBI', qw(:sql_types));
use_ok('ODBCTEST');

# to help ActiveState's build process along by behaving (somewhat) if a dsn is not provided
BEGIN {
   if (!defined $ENV{DBI_DSN}) {
      plan skip_all => "DBI_DSN is undefined";
   } else {
      # num tests + one for each table_info column (5)
      plan tests => 19 + 5;
   }
}


my @row;

my $dbh = DBI->connect();
unless($dbh) {
   BAILOUT("Unable to connect to the database $DBI::errstr\nTests skipped.\n");
   exit 0;
}

$dbh->{LongReadLen} = 1000;
is($dbh->{LongReadLen}, 1000, "Set Long Read Len");
my $dbname = $dbh->{odbc_SQL_DBMS_NAME};

#### testing set/get of connection attributes
$dbh->{RaiseError} = 0;
$dbh->{AutoCommit} = 1;
ok($dbh->{AutoCommit}, "AutoCommit set on dbh");

my $rc = commitTest($dbh);

diag(" Strange: " . $dbh->errstr . "\n") if ($rc < -1);
SKIP: {
    skip "skipped due to lack of transaction support", 3 if ($rc == -1);

    is($rc, 1, "commitTest with AutoCommit");

    $dbh->{AutoCommit} = 0;
    ok(!$dbh->{AutoCommit}, "AutoCommit turned off");
    $rc = commitTest($dbh);
    diag(" Strange: " . $dbh->errstr . "\n") if ($rc < -1);
    is($rc, 0, "commitTest with AutoCommit off");
};

$dbh->{AutoCommit} = 1;
ok($dbh->{AutoCommit}, "Ensure autocommit back on");

# ------------------------------------------------------------

my $rows = 0;
# Check for tables function working.
my $sth;

my @table_info_cols = (
		       'TABLE_CAT',
		       'TABLE_SCHEM',
		       'TABLE_NAME',
		       'TABLE_TYPE',
		       'REMARKS',
		      );

SKIP:  {
    $sth = $dbh->table_info();
    skip "table_info returned undef sth", 7 unless $sth;
    my $cols = $sth->{NAME};
    isa_ok($cols, 'ARRAY', "sth {NAME} returns ref to array");
    for (my $i = 0; $i < @$cols; $i++) {
       # print ${$cols}[$i], ": ", $sth->func($i+1, 3, ColAttributes),
       # "\n";
       is(${$cols}[$i], $table_info_cols[$i], "Column test for table_info $i");
    }
    while (@row = $sth->fetchrow()) {
        $rows++;
    }
    cmp_ok($rows, '>', 0, "must be some tables out there?");
    $sth->finish();
};


$rows = 0;
$dbh->{PrintError} = 0;
my @tables = $dbh->tables;

cmp_ok($#tables, '>', 0, "tables returnes array");
$rows = 0;
if ($sth = $dbh->column_info(undef, undef, $ODBCTEST::table_name, undef)) {
    while (@row = $sth->fetchrow()) {
        $rows++;
    }
    $sth->finish();
}
cmp_ok($rows, '>', 0, "column info returns more than one row for test table");

$rows = 0;

if ($sth = $dbh->primary_key_info(undef, undef, $ODBCTEST::table_name, undef)) {
    while (@row = $sth->fetchrow()) {
        $rows++;
    }
    $sth->finish();
}

SKIP: {
    skip "Primary Key Known to fail using MS Access through 2000", 1 if ($dbname =~ /Access/i);
    cmp_ok($rows, '>', 0, "primary key count");
};

# test $sth->{NAME} when using non-select statements
$sth = $dbh->prepare("update $ODBCTEST::table_name set COL_A = 100 WHERE COL_A = 100");
ok($sth, "prepare update statement returns valid sth ");

is(@{$sth->{NAME}}, 0, "update statement has 0 columns returned");
$sth->execute;
is(@{$sth->{NAME}}, 0, "update statement has 0 columns returned 2");

$dbh->{odbc_query_timeout} = 30;
is($dbh->{odbc_query_timeout}, 30, "Verify odbc_query_timeout set ok");

my $sth_timeout = $dbh->prepare("select COL_A from $ODBCTEST::table_name");
is($sth_timeout->{odbc_query_timeout}, 30, "verify dbh setting for query_timeout passed to sth");
$sth_timeout->{odbc_query_timeout} = 1;
is($sth_timeout->{odbc_query_timeout}, 1, "verify sth query_timeout can be overridden");

$dbh->disconnect;
exit 0;
# avoid annoying warning
print $DBI::errstr;
# print STDERR $dbh->{odbc_SQL_DRIVER_ODBC_VER}, "\n";

# ------------------------------------------------------------
# returns true when a row remains inserted after a rollback.
# this means that autocommit is ON. 
# ------------------------------------------------------------
sub commitTest {
    my $dbh = shift;
    my @row;
    my $rc = -2;
    my $sth;

    # since this test deletes the record, we should do it regardless
    # of whether or not it the db supports transactions.
    $dbh->do("DELETE FROM $ODBCTEST::table_name WHERE COL_A = 100") or return undef;

    { # suppress the "commit ineffective" warning
      local($SIG{__WARN__}) = sub { };
      $dbh->commit();
    }

    my $supported = $dbh->get_info(46); # SQL_TXN_CAPABLE 
    # print "Transactions supported: $supported\n";
    if (!$supported) {
	return -1;
    }

    @row = ODBCTEST::get_type_for_column($dbh, 'COL_D');
    my $dateval;
    if (ODBCTEST::isDateType($row[1])) {
       $dateval = "{d '1997-01-01'}";
    } else {
       $dateval = "{ts '1997-01-01 00:00:00'}";
    }
    $dbh->do("insert into $ODBCTEST::table_name values(100, 'x', 'y', $dateval)");
    { # suppress the "rollback ineffective" warning
	  local($SIG{__WARN__}) = sub { };
      $dbh->rollback();
    }
    $sth = $dbh->prepare("SELECT COL_A FROM $ODBCTEST::table_name WHERE COL_A = 100");
    $sth->execute();
    if (@row = $sth->fetchrow()) {
        $rc = 1;
    }
    else {
	$rc = 0;
    }
    # in case not all rows have been returned..there shouldn't be more than one.
    $sth->finish(); 
    $rc;
}

# ------------------------------------------------------------

