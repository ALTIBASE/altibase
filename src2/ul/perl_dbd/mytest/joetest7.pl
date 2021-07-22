#!perl -w
# $Id: joetest7.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI qw(:sql_types);

my $dbh=DBI->connect() or die "Can't connect";

eval {$dbh->do("DROP TABLE table1");};
eval {$dbh->do("CREATE TABLE table1 (i INTEGER)");};

eval {$dbh->do("DROP PROCEDURE proc1");};
my $proc1 = <<EOT;
CREATE PROCEDURE proc1 (\@i INT) AS
DECLARE \@result INT;
BEGIN
   SET \@result = 1;
   select \@result;
   select \@result, \@result;
   IF (\@i = 99)
      BEGIN
         UPDATE table1 SET i=\@i;
      END;
   SELECT \@result;
END
EOT
eval {$dbh->do($proc1);};

if (-e "dbitrace.log") {
   unlink("dbitrace.log");
}
$dbh->trace(9,"dbitrace.log");
my $sth = $dbh->prepare ("{call proc1 (?)}");
my $success = -1;

$sth->bind_param (1, 99, SQL_INTEGER);
$sth->execute();
$success = -1;
do {
   print "Num of fields: $sth->{NUM_OF_FIELDS}\n";

   while (my @data = $sth->fetchrow_array()) {
      ($success) = @data;
      print "Num of fields: $sth->{NUM_OF_FIELDS}\n"
   }
} while $sth->{odbc_more_results};
print "$success Finished #1\n";

$sth->bind_param (1, 10, SQL_INTEGER);
$sth->execute();
$success = -1;
do {
   while (my @data = $sth->fetchrow_array()) {($success) = @data;}
} while $sth->{odbc_more_results};
print "$success Finished #2\n";

$sth->bind_param (1, 99, SQL_INTEGER);
$sth->execute();
$success = -1;
do {
   while (my @data = $sth->fetchrow_array()) {($success) = @data;}
} while $sth->{odbc_more_results};
print "$success Finished #3\n";

$sth->bind_param (1, 99, SQL_INTEGER);
$sth->execute();
$success = -1;
do {
   while (my @data = $sth->fetchrow_array()) {($success) = @data;}
} while $sth->{odbc_more_results};
print "$success Finished #4\n";

$dbh->disconnect;
