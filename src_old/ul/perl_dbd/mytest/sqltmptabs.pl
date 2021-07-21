use DBI;
# $Id: sqltmptabs.pl 11810 2005-05-18 04:43:35Z qa $


my $dbh = DBI->connect($ENV{DBI_DSN}, $ENV{DBI_USER}, $ENV{DBI_PASS}, { RaiseError => 1});
my $sth;
my $sql = 'CREATE TABLE #foo (id INT PRIMARY KEY, val CHAR(4))';
$dbh->do($sql);
# $sth = $dbh->prepare($sql);
# $sth->execute;
# $sth->finish;

print "Now inserting!\n";
$sth = $dbh->prepare("INSERT INTO #foo (id, val) VALUES (?, ?)");
my $sth2 = $dbh->prepare("INSERT INTO #foo (id, val) VALUES (?, ?)");
$sth2->execute(1, 'foo');
$sth2->execute(2, 'bar');

$sth = $dbh->prepare("Select id, val from #foo");
$sth->execute;
my @row;

while (@row = $sth->fetchrow_array) {
   print join(', ', @row), "\n";
}

$dbh->disconnect;
