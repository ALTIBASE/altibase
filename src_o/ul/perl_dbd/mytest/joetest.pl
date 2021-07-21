#!perl -w
# $Id: joetest.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use Getopt::Std;
use DBI qw(:sql_types);

my $usage = "perl dbtest.pl [-b].  -b binds parameters explicitly.\n";
my @data = (
    ["2001-01-01 01:01:01.111", "a" x 12],   # "aaaaaaaaaaaa"
    ["2002-02-02 02:02:02.222", "b" x 114],
    ["2003-03-03 03:03:03.333", "c" x 251],
    ["2004-04-04 04:04:04.444", "d" x 282],
    ["2005-05-05 05:05:05.555", "e" x 131]
);

# Get command line options:
my %args;
getopts ("b", \%args)  or die $usage;
my $bind = $args{"b"};

# Connect to the database and create the table:
my $dbh=DBI->connect() or die "Can't connect";
$dbh->{RaiseError} = 1;
$dbh->{LongReadLen} = 800;
eval {
   $dbh->do("DROP TABLE foo");
};
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
die "Unable to find a suitable test type for date field\n"
   unless @row;

my $dbname = $dbh->get_info(17); # sql_dbms_name
my $datetype = $row[0];
print "Date type = $datetype\n";
$dbh->do("CREATE TABLE foo (i INTEGER, time $datetype, str VARCHAR(4000))");

# Insert records into the database:
my $sth1 = $dbh->prepare("INSERT INTO FOO (i,time,str) values (?,?,?)");
for (my $i=0; $i<@data; $i++) {
    my ($time,$str) = @{$data[$i]};
    print "Inserting:  $i, $time, string length ".length($str)."\n";
    if ($bind) {
        $sth1->bind_param (1, $i,    SQL_INTEGER);
        $sth1->bind_param (2, $time, SQL_TIMESTAMP);
        $sth1->bind_param (3, $str,  SQL_LONGVARCHAR);
        $sth1->execute  or die ($DBI::errstr);
    } else {
        $sth1->execute ($i, $time, $str)  or die ($DBI::errstr);
    }
}
print "\n";

# Retrieve records from the database, and see if they match original data:
my $sth2 = $dbh->prepare("SELECT i,time,str FROM foo");
$sth2->execute  or die ($DBI::errstr);
while (my ($i,$time,$str) = $sth2->fetchrow_array()) {
    print "Retrieving: $i, $time, string length ".length($str)."\t";
    print "!time  " if ($time ne $data[$i][0]);
    print "!string" if ($str  ne $data[$i][1]);
    print "\n";
}
$dbh->disconnect;
