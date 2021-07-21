#/usr/bin/perl -w
# $Id: longbin.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI qw (:sql_types);
use Digest::MD5 qw(md5 md5_hex);

my $dbh = DBI->connect();
$dbh->{RaiseError} = 1;	 # raise the error
$dbh->{PrintError} = 0;	 # but don't print it.
$dbh->{odbc_default_bind_type} = 0;

eval {
   # if it's not already created, the eval will silently ignore this
   $dbh->do("drop table longtest;");
};

# probably should use get_info to get the type for long here...
my $dbname = $dbh->get_info(17); # DBI::SQL_DBMS_NAME
my $longbinary_type = get_first_type_info($dbh, SQL_LONGVARBINARY);
my $integer_type = get_first_type_info($dbh, SQL_INTEGER);
print "$dbname, ($integer_type, $longbinary_type)\n";

$dbh->do("Create table longtest (id $integer_type, picture $longbinary_type)");
   
my $sth = $dbh->prepare("insert into longtest (id, picture) values (?, ?)");
my $id = 0;
my $file;
my @md5sums = ();

foreach $file (@ARGV) {
   my $blob;
   eval {
      print "Reading: $file\n";
      $blob = readblobfile($file);
   };
   if (!$@) {
      $md5sums[$id] = md5_hex($blob);
      $sth->bind_param(1, $id); #DBI::SQL_INTEGER);
      # with access, you must bind to SQL_LONGVARBINARY!  Otherwise, it doesn't work.
      # oracle and SQL Server handle the types correctly...
      if ($dbname =~ /Access/i) {
	 $sth->bind_param(2, $blob, DBI::SQL_LONGVARBINARY);
      } else {
	 $sth->bind_param(2, $blob);
      }
      $sth->execute;
      $id++;
   } else {
      printf("Couldn't read file: $@\n");
   }
}

# now check the data, just out of paranoia...
$dbh->{LongReadLen} = 2000000;
$dbh->{LongTruncOk} = 0;
my $sthr = $dbh->prepare("select id, picture from longtest order by id");
$sthr->execute;
my @row;
while (@row = $sthr->fetchrow_array) {
   my $digest = md5_hex($row[1]);
   if ($digest ne $md5sums[$row[0]]) {
      print "$row[0]: Digests don't match $digest, $md5sums[$row[0]]!\n";
   } else {
      print "Good read!\n";
   }
}


$dbh->disconnect();

sub readblobfile($) {
   my $filename = shift;
   local(*FILE, $\);	# automatically close file at end of scope
   open(FILE, "<$filename") or die "Can't open file $!\n";
   binmode(FILE);
   <FILE>;
}

sub getFileMD5 ($) {
    my $filename = shift;

    open(F, $filename) or die "Can't open file name $filename\n";
    binmode(F);
    my $md5 = new MD5;
    seek(F, 0, 0);	# just in case?  part of docs, I left in.
    $md5->reset;
    $md5->addfile(\*F);
    close(F);
    $md5->hexdigest;
}

sub get_first_type_info($$) {
   my $dbh = shift;
   my $type = shift;

   my @typeinfo = $dbh->type_info($type);

   return $typeinfo[0]->{TYPE_NAME};
   
}


	