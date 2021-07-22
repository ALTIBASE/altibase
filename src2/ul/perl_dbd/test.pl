#!/usr/bin/perl

use strict;
use DBI();

#DBI->trace(3);

my $dbh = DBI->connect("dbi:altibase:DSN=127.0.0.1;UID=SYS;PWD=MANAGER;CONNTYPE=1;NLS_USE=US7ASCII;PORT_NO=20999", "", "", {'RaiseError' => 1});

# Drop table 'foo'. This may fail, if 'foo' doesn't exist.
# Thus we put an eval around it.
eval { $dbh->do("DROP TABLE foo") };
print "Dropping foo failed: $@\n" if $@;

# Create a new table 'foo'. This must not fail, thus we don't
# catch errors.
$dbh->do("CREATE TABLE foo (id number, name VARCHAR(20))");

# INSERT some data into 'foo'. We are using $dbh->quote() for
# quoting the name.
$dbh->do("INSERT INTO foo VALUES (1, " . $dbh->quote("Tim") . ")");

# Same thing, but using placeholders
$dbh->do("INSERT INTO foo VALUES (?, ?)", undef, 2, "Jochen");

# Now retrieve data from the table.
my $sth = $dbh->prepare("SELECT * FROM foo");
$sth->execute();
while (my $ref = $sth->fetchrow_hashref()) 
{
  print "Found a row: id = $ref->{'FOO.ID'}, name = $ref->{'FOO.NAME'}\n";
}
$sth->finish();

# Disconnect from the database.
$dbh->disconnect();

