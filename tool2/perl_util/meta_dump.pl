#!/usr/bin/perl -w
#
# ch06/tabledump: Dumps information about all the tables.

use DBI;
#DBI->trace(3);
### Connect to the database
$|=1;#disable buffered in/out
my $match = shift;
my $not = 0;
 if (not defined $match or $match=~m/~:/ ) {
	$match = '*'   # Show only yor Tables
     }elsif ($match eq ':QPM') {
        $match = 'QPM_*';# Show only MetaTables
     }elsif ($match eq ':TBL') { 
        $match = 'QPM_*';
        $not=1 ;  }
    else{
      $not=0 ;  
    };	
my $dsn=$ENV{DBI_DSN};
my $dbh = DBI->connect($dsn, "", "",
			{'RaiseError' => 0,  
			 'PrintError' => 1,
			 'AutoCommit' => 1
			});


### Create a new statement handle to fetch table information
my $tabsth = $dbh->table_info();

### Iterate through all the tables...
while ( my ( $qual, $owner, $name, $type ) = $tabsth->fetchrow_array() ) {
    next unless ($name=~m/^$match/ ^ $not ); 
    ### The table to fetch data for
    my $table = $name;

    ### Build the full table name with quoting if required
    $table = qq{"$owner"."$table"} if defined $owner;

    ### The SQL statement to fetch the table metadata
    my $statement = "SELECT * FROM $table";

    print "\n";
    print "Table Information\n";
    print "=================\n\n";
    print "Statement:     $statement\n";

    ### Prepare and execute the SQL statement
    my $sth = $dbh->prepare( $statement );
    #### Run Execute 
    DBI->trace(3);# DEBUG mode 3
    $sth->execute();
    DBI->trace(0);# DEBUG Mode 1
    my $fields = $sth->{NUM_OF_FIELDS};
    print "NUM_OF_FIELDS: $fields\n\n";

    print "Column Name                     Type  Precision  Scale  Nullable?\n";
    print "------------------------------  ----  ---------  -----  ---------\n\n";

    ### Iterate through all the fields and dump the field information
    for ( my $i = 0 ; $i < $fields ; $i++ ) {

        my $name = $sth->{NAME}->[$i];

        ### Describe the NULLABLE value
        my $nullable = ("No", "Yes", "Unknown")[ $sth->{NULLABLE}->[$i] ];

        ### Tidy the other values, which some drivers don't provide
        my $scale = ($sth->{SCALE})     ? $sth->{SCALE}->[$i]     : "N/A";
        my $prec  = ($sth->{PRECISION}) ? $sth->{PRECISION}->[$i] : "N/A";
        my $type  = ($sth->{TYPE})      ? $sth->{TYPE}->[$i]      : "N/A";

        ### Display the field information
        printf "%-30s %5d      %4d   %4d   %s\n",
                $name, $type, $prec, $scale, $nullable;
    }

    ### Explicitly de-allocate the statement resources
    ### because we didn't fetch all the data
    $sth->finish();
}

exit;
