#!perl -w
# $Id: leakcheck.pl 11810 2005-05-18 04:43:35Z qa $


use Devel::Leak;
use DBI qw(:sql_types);
use strict;

my $insert_value = 0;
my $long = "a" x 1000;

sub connecttest {
   my $dbh = DBI->connect();

   $dbh->disconnect;
}

sub preparetest {
   my $dbh = DBI->connect();
   $dbh->{LongReadLen} = 800;
   my $sth=$dbh->prepare("select * from PERL_DBD_TEST");
   my @row;
   $sth->execute;
   while (@row = $sth->fetchrow_array) {
   }

}

sub inserttest ($) {
   my $delete = shift;

   
   my $dbh = DBI->connect();
   $dbh->{LongReadLen} = 1500;
   if ($delete) {
      $dbh->do("delete from perl_dbd_test");
   }
   my $sth=$dbh->prepare("insert into PERL_DBD_TEST (COL_A, COL_C) values ($insert_value, ?)");
   my @row;
   $sth->bind_param(1, $long, SQL_LONGVARCHAR);
   $sth->execute;
   $insert_value++;
}

sub selecttest {
   my $dbh = DBI->connect();
   $dbh->{LongReadLen} = 1500;
   my $sth=$dbh->prepare("select COL_A, COL_C FROM PERL_DBD_TEST order by col_a");
   my @row;
   $sth->execute;
   while (@row = $sth->fetchrow_array) {
   }

}

my $handle;
my $i =0;

my $count;
my $count2;
my $count3;
my $count4;
my $count5;
my $count6;
my $count7;
my $count8;

$count = Devel::Leak::NoteSV($handle);
$i = 0;
while ($i < 100) {
   connecttest;
   $i++;
}

$count2 = Devel::Leak::CheckSV($handle);
$count2 = Devel::Leak::NoteSV($handle);

preparetest;

$count3 = Devel::Leak::CheckSV($handle);
$count3 = Devel::Leak::NoteSV($handle);

$i = 0;
while ($i < 100) {
   preparetest;
   $i++;
}

$count4 = Devel::Leak::CheckSV($handle);
$count4 = Devel::Leak::NoteSV($handle);

inserttest(1);

$count5 = Devel::Leak::CheckSV($handle);
$count5 = Devel::Leak::NoteSV($handle);

$i = 0;
while ($i < 100) {
   inserttest(0);
   $i++;
}
$count6 = Devel::Leak::CheckSV($handle);
$count6 = Devel::Leak::NoteSV($handle);

selecttest;

$count7 = Devel::Leak::CheckSV($handle);
$count7 = Devel::Leak::NoteSV($handle);
$i = 0;
while ($i < 100) {
   selecttest;
   $i++;
}

$count8 = Devel::Leak::CheckSV($handle);
# $count8 = Devel::Leak::NoteSV($handle);

print "$count, $count2, $count3, $count4, $count5, $count6, $count7, $count8\n";
