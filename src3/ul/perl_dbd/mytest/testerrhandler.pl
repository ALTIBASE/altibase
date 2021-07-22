use strict;
# $Id: testerrhandler.pl 11810 2005-05-18 04:43:35Z qa $


use DBI;

sub err_handler {
   my ($state, $msg) = @_;
   # Strip out all of the driver ID stuff
   $msg =~ s/^(\[[\w\s]*\])+//;
   print "===> state: $state msg: $msg\n";
   return 0;
}

my $dbh = DBI->connect("dbi:ODBC:PERL_TEST_SQLSERVER", $ENV{DBI_USER}, $ENV{DBI_PASS})
       || die "Can't connect: $DBI::errstr\n";

$dbh->{odbc_err_handler} = \&err_handler;
$dbh->{odbc_async_exec} = 1;
print "odbc_async_exec is: $dbh->{odbc_async_exec}\n";

my $sth;
$sth = $dbh->prepare("dbcc checkdb(model)") || die $dbh->errstr;
$sth->execute                               || die $dbh->errstr;
$sth->finish;
$dbh->disconnect;

