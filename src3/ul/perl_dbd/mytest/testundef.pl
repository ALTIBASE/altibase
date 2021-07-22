#perl -w
# $Id: testundef.pl 11810 2005-05-18 04:43:35Z qa $

use DBI;

my $dbh = DBI->connect()
	or die "$DBI::errstr\n";


eval {
   # if it's not already created, the eval will silently ignore this
   $dbh->do("drop table longtest;");
};

$dbh->do("create table hashtest (id integer, value varchar2(200))");

my %foo;

$foo{1} = "bless me";

DBI->trace(9,"c:/trace.txt");
my $sth = $dbh->prepare("insert into hashtest values (?, ?)");
$sth->execute((2, $foo{2}));
$sth->execute((1, $foo{1}));

my $sth2 = $dbh->prepare("select id, value from hashtest order by id");
$sth2->execute;
my @row;
while (@row = $sth2->fetch) {
   print join(', ', @row), "\n";
}





$dbh->disconnect;

