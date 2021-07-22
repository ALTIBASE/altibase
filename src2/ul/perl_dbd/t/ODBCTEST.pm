#
# Package ODBCTEST
# 
# This package is a common set of routines for the DBD::ODBC tests.
# This is a set of routines to create, drop and test for existance of
# a table for a given DBI database handle (dbh).
#
# This set of routines currently depends greatly upon some ODBC meta-data.
# The meta data required is the driver's native type name for various ODBC/DBI
# SQL types.  For example, SQL_VARCHAR would produce VARCHAR2 under Oracle and TEXT
# under MS-Access.  This uses the function SQLGetTypeInfo.  This is obtained via
# the DBI C<func> method, which is implemented as a call to the driver.  In this case,
# of course, this is the DBD::ODBC.
#
# the SQL_TIMESTAMP may be dubious on many platforms, but SQL_DATE was not supported
# under Oracle, MS SQL Server or Access.  Those are pretty common ones.
#

require 5.004;
{
   package ODBCTEST;

   use DBI qw(:sql_types);
   use Test::More;
   
   $VERSION = '0.01';
   $table_name = "PERL_DBD_TEST";

   $longstr = "THIS IS A STRING LONGER THAN 80 CHARS.  THIS SHOULD BE CHECKED FOR TRUNCATION AND COMPARED WITH ITSELF.";
   $longstr2 = $longstr . "  " . $longstr . "  " . $longstr . "  " . $longstr;

    # really dumb work around:
    # MS SQL Server 2000 (MDAC 2.5 and ODBC driver 2000.080.0194.00) have a bug if
    # the column is named C, CA, or CAS and there is a call to SQLDescribeParam...
    # there is an error, referring to a syntax error near keyword 'by'
    # I figured it's just best to rename the columns.
    # changed SQL_BIGINT below to -5, as DBI has removed that constant.
    # ODBC's value is -5.
   %TestFieldInfo = (
		     'COL_A' => [SQL_SMALLINT,-5, SQL_TINYINT, SQL_NUMERIC, SQL_DECIMAL, SQL_FLOAT, SQL_REAL],
		     'COL_B' => [SQL_VARCHAR, SQL_CHAR],
		     'COL_C' => [SQL_LONGVARCHAR, -1],
		     'COL_D' => [SQL_TYPE_TIMESTAMP, SQL_TYPE_DATE, SQL_DATE, SQL_TIMESTAMP ],
		    );

   sub get_type_for_column {
      my $dbh = shift;
      my $column = shift;

      my $type;
      my @row;
      my $sth;
      foreach $type (@{ $TestFieldInfo{$column} }) {
	 $sth = $dbh->func($type, GetTypeInfo);
	    # may not be correct behavior, but get the first compat type
	 if ($sth) {
	    @row = $sth->fetchrow();
	    $sth->finish();
	    last if @row;
	 } else {
		    # warn "Unable to get type for type $type\n";
	 }
      }
      die "Unable to find a suitable test type for field $column"
	    unless @row;
	# warn join(", ",@row);
      return @row;
   }
   sub tab_create {
      my $dbh = shift;
      $dbh->{PrintError} = 0;
      eval {
	 $dbh->do("DROP TABLE $table_name");
      };
      $dbh->{PrintError} = 1;

      # trying to use ODBC to tell us what type of data to use
      my $fields = undef;
      my $f;
      foreach $f (sort keys %TestFieldInfo) {
	    # print "$f: $TestFieldInfo{$f}\n";
	 $fields .= ", " unless !$fields;
	 $fields .= "$f ";
	    # print "-- $fields\n";
	 my @row = get_type_for_column($dbh, $f);
	 $fields .= $row[0];
	 if ($row[5]) {
	    $fields .= "($row[2])"	 if ($row[5] =~ /LENGTH/i);
	    $fields .= "($row[2],0)" if ($row[5] =~ /PRECISION,SCALE/i);
	 }
	 if ($f eq 'COL_A') {
	    $fields .= " NOT NULL PRIMARY KEY ";
	 }
	    # print "-- $fields\n";
      }
      # diag("Using fields: $fields\n");
      $dbh->do("CREATE TABLE $table_name ($fields)");
   }


   sub tab_delete {
      my $dbh = shift;
      $dbh->do("DELETE FROM $table_name");
   }

   sub tab_exists {
      my $dbh = shift;
      my (@rows, @row, $rc);

      $rc = -1;

      unless ($sth = $dbh->table_info()) {
	 # diag("Can't list tables: $DBI::errstr\n");
	 return -1;
      }
	# TABLE_QUALIFIER,TABLE_OWNER,TABLE_NAME,TABLE_TYPE,REMARKS
      while ($row = $sth->fetchrow_hashref()) {
	    # XXX not fully true.  The "owner" could be different.  Need to check!
	    # In Oracle, testing $user against $row[1] works, but does NOT in SQL Server.
	    # SQL server returns the device and something else I haven't quite taken the time
	    # to figure it out, since I'm not a SQL server expert.  Anyone out there?
	    # (mine returns "dbo" for the owner on ALL my tables.  This is obviously something
	    # significant for SQL Server...one of these days I'll dig...
	 if (($table_name eq uc($row->{TABLE_NAME}))) {
		# and (uc($user) eq uc($row[1]))) 
		# qeDBF driver returns null for TABLE_OWNER
	    my $owner = $row->{TABLE_OWNER} || '(unknown owner)';
	    # diag("$owner.$row->{TABLE_NAME}\n");
	    $rc = 1;
	    last;
	 }
      }
      $sth->finish();
      $rc;
   }

   #
   # show various ways of inserting data without binding parameters.
   # Note, these are not necessarily GOOD ways to
   # show this...
   #

   @tab_insert_values = ( 
			 [1, 'foo', 'foo varchar', "{d '1998-05-11'}", "{ts '1998-05-11 00:00:00'}"],
			 [2, 'bar', 'bar varchar', "{d '1998-05-12'}", "{ts '1998-05-12 00:00:00'}"],
			 [3, "bletch", "bletch varchar", "{d '1998-05-10'}", "{ts '1998-05-10 00:00:00'}"],
			 [4, "80char", $longstr, "{d '1998-05-13'}", "{ts '1998-05-13 12:00:00'}"],
			 [5, "gt250char", $longstr2, "{d '1998-05-14'}", "{ts '1998-05-14 00:00:00'}"],
			);
			   
   sub tab_insert {
      my $dbh = shift;

       # qeDBF needs a space after the table name!
      foreach (@tab_insert_values) {
      
	 @row = ODBCTEST::get_type_for_column($dbh, 'COL_D');
	 # print "TYPE FOUND = $row[1]\n";
	 if (!$dbh->do("INSERT INTO $table_name (COL_A, COL_B, COL_C, COL_D) VALUES ("
		 . join(", ", $_->[0],
			$dbh->quote($_->[1]),
			$dbh->quote($_->[2]), 
			$_->[isDateType($row[1]) ? 3 : 4]). ")")) {
	    return 0;
	 }
      }
      1;
   }

   sub isDateType($) {
      my $type = shift;
      if ($type == SQL_DATE  || $type == SQL_TYPE_DATE) {
	 return 1;
      } else {
	 return 0;
      }
   }

   sub tab_insert_bind {
      my $dbh = shift;
      my $dref = shift;
      my $handle_column_type = shift;
      my @data = @{$dref};

      my $sth = $dbh->prepare("INSERT INTO $table_name (COL_A, COL_B, COL_C, COL_D) VALUES (?, ?, ?, ?)");
      unless ($sth) {
	 warn $DBI::errstr;
	 return 0;
      }
      # $sth->{PrintError} = 1;
      foreach (@data) {
	 my @row;
	 if ($handle_column_type) {
	    @row = ODBCTEST::get_type_for_column($dbh, 'COL_A');
	    # diag("Binding the value: $_->[0] type = $row[1]\n");
	    $sth->bind_param(1, $_->[0], { TYPE => $row[1] });
	 } else {
	    $sth->bind_param(1, $_->[0]);
	 }
	 if ($handle_column_type) {
	    @row = ODBCTEST::get_type_for_column($dbh, 'COL_B');
	    $sth->bind_param(2, $_->[1], { TYPE => $row[1] });
	 } else {
	    $sth->bind_param(2, $_->[1]);
	 }
	 if ($handle_column_type) {
	    @row = ODBCTEST::get_type_for_column($dbh, 'COL_C');
	    $sth->bind_param(3, $_->[2], { TYPE => $row[1] });
	 } else {
	    $sth->bind_param(3, $_->[2]);
	 }
	 
	 # print "SQL_DATE = ", SQL_DATE, " SQL_TIMESTAMP = ", SQL_TIMESTAMP, "\n";
	 @row = ODBCTEST::get_type_for_column($dbh, 'COL_D');
	 # diag("TYPE FOUND = $row[1]\n");
	 # if ($row[1] == SQL_TYPE_TIMESTAMP) {
	 #   $row[1] = SQL_TIMESTAMP;
	 #}
	 # print "Binding the date value: \"$_->[$row[1] == SQL_DATE ? 3 : 4]\"\n";
	 if ($handle_column_type) {
	    $sth->bind_param(4, $_->[isDateType($row[1]) ? 3 : 4], { TYPE => $row[1] });
	 } else {
	    # sigh, couldn't figure out how to get rid of the warning nicely,
	    # so I turned it off!!!  Now, I have to turn it back on due
	    # to  problems in other perl versions.
	    $sth->bind_param(4, $_->[isDateType($row[1]) ? 3 : 4]);
	 }
	 return 0 unless $sth->execute;
      }
      1;
   }
   1;
}

