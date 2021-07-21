#!perl -w
# $Id: timetest.pl 11810 2005-05-18 04:43:35Z qa $


use strict;
use DBI qw(:sql_types);

my $dbh=DBI->connect() or die "Can't connect";

$dbh->{RaiseError} = 1;
$dbh->{LongReadLen} = 800;
eval {
   $dbh->do("drop table foo");
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
$dbh->do("Create table foo (idcol integer not null primary key, dt $datetype)");

my @tests = (
	     "{ts '1998-05-13 00:01:00'}",
	     "{ts '1998-05-15 00:01:00.5'}",
	     "{ts '1998-05-15 00:01:00.210'}",
	    );

my $test;
my $i = 0;
my $sth = $dbh->prepare("insert into foo (idcol, dt) values (?, ?)");
foreach $test (@tests) {
   $sth->execute($i++, $test);
}

$sth = $dbh->prepare("Select idcol, dt from foo order by idcol");
$sth->execute;
while (@row = $sth->fetchrow_array) {
   print join(', ', @row), "\n";
}
$dbh->disconnect;
