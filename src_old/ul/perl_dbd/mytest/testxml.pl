#perl -w
# $Id: testxml.pl 11810 2005-05-18 04:43:35Z qa $

$| = 1;


use DBI qw(:sql_types);
use Data::Dumper;

my $dbh = DBI->connect() || die "Connect failed: $DBI::errstr\n";


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
$dbh->{LongReadLen} = 8000;

my @types = (SQL_TYPE_TIMESTAMP, SQL_TIMESTAMP);
my $type;
my @row;
my $rowcount = 0;
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
die "Unable to find a suitable test type for date field\n"
   unless @row;

my $datetype = $row[0];
$dbh->do("CREATE TABLE PERL_DBD_TABLE1 (i INTEGER, time $datetype, str VARCHAR(4000))");


# Insert records into the database:
my $sth1 = $dbh->prepare("INSERT INTO PERL_DBD_TABLE1 (i,time,str) values (?,?,?)");
for (my $i=0; $i<@data; $i++) {
    my ($time,$str) = @{$data[$i]};
    print "Inserting:  $i, ";
    print  $time if (defined($time));
    print " string length " . length($str) . "\n";
    $sth1->bind_param (1, $i,    SQL_INTEGER);
    $sth1->bind_param (2, $time, SQL_TIMESTAMP);
    $sth1->bind_param (3, $str,  SQL_LONGVARCHAR);
    $sth1->execute  or die ($DBI::errstr);
}

# Retrieve records from the database, and see if they match original data:
my $sth2 = $dbh->prepare("SELECT i,time,str FROM PERL_DBD_TABLE1 for xml auto");
$sth2->execute  or die ($DBI::errstr);
my $iErrCount = 0;
while (my @row = $sth2->fetchrow_array()) {
   print join(', ', @row), "\n";
   $rowcount++;
}

print "retrieved $rowcount rows.\n";
$dbh->disconnect;
