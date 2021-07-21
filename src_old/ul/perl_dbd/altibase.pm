# Copyright (c) 1994,1995,1996,1998  Tim Bunce
# portions Copyright (c) 1997-2004  Jeff Urlwin
# portions Copyright (c) 1997  Thomas K. Wenrich
#
# You may distribute under the terms of either the GNU General Public
# License or the Artistic License, as specified in the Perl README file.

require 5.004;

$DBD::altibase::VERSION = '1.13';

{
    package DBD::altibase;

    use DBI ();
    use DynaLoader ();
    use Exporter ();
    
    @ISA = qw(Exporter DynaLoader);

    # my $Revision = substr(q$Id: altibase.pm 11810 2005-05-18 04:43:35Z qa $, 13,2);

    require_version DBI 1.21;

    bootstrap DBD::altibase $VERSION;

    $err = 0;		# holds error code   for DBI::err
    $errstr = "";	# holds error string for DBI::errstr
    $sqlstate = "00000";
    $drh = undef;	# holds driver handle once initialised

    sub driver{
	return $drh if $drh;
	my($class, $attr) = @_;

	$class .= "::dr";

	# not a 'my' since we use it above to prevent multiple drivers

	$drh = DBI::_new_drh($class, {
	    'Name' => 'altibase',
	    'Version' => $VERSION,
	    'Err'    => \$DBD::altibase::err,
	    'Errstr' => \$DBD::altibase::errstr,
	    'State' => \$DBD::altibase::sqlstate,
	    'Attribution' => 'altibase DBD by Jeff Urlwin',
	    });

	$drh;
    }

    sub CLONE { undef $drh }
    1;
}


{   package DBD::altibase::dr; # ====== DRIVER ======
    use strict;

    sub connect {
	my $drh = shift;
	my($dbname, $user, $auth, $attr)= @_;
	$user = '' unless defined $user;
	$auth = '' unless defined $auth;

	# create a 'blank' dbh
	my $this = DBI::_new_dbh($drh, {
	    'Name' => $dbname,
	    'USER' => $user, 
	    'CURRENT_USER' => $user,
	    });

	# Call altibase logon func in altibase.xs file
	# and populate internal handle data.

	DBD::altibase::db::_login($this, $dbname, $user, $auth, $attr) or return undef;

	$this;
    }

}


{   package DBD::altibase::db; # ====== DATABASE ======
    use strict;

    sub prepare {
	my($dbh, $statement, @attribs)= @_;

	# create a 'blank' dbh
	my $sth = DBI::_new_sth($dbh, {
	    'Statement' => $statement,
	    });

	# Call altibase func in altibase.xs file.
	# (This will actually also call SQLPrepare for you.)
	# and populate internal handle data.

	DBD::altibase::st::_prepare($sth, $statement, @attribs)
	    or return undef;

	$sth;
    }

    sub column_info {
	my ($dbh, $catalog, $schema, $table, $column) = @_;

	$catalog = "" if (!$catalog);
	$schema = "" if (!$schema);
	$table = "" if (!$table);
	$column = "" if (!$column);
	# create a "blank" statement handle
	my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLColumns" });

	_columns($dbh,$sth, $catalog, $schema, $table, $column)
	    or return undef;

	$sth;
    }
    
    sub columns {
	my ($dbh, $catalog, $schema, $table, $column) = @_;

	$catalog = "" if (!$catalog);
	$schema = "" if (!$schema);
	$table = "" if (!$table);
	$column = "" if (!$column);
	# create a "blank" statement handle
	my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLColumns" });

	_columns($dbh,$sth, $catalog, $schema, $table, $column)
	    or return undef;

	$sth;
    }


    sub table_info {
 	my($dbh, $catalog, $schema, $table, $type) = @_;

	if ($#_ == 1) {
	   my $attrs = $_[1];
	   $catalog = $attrs->{TABLE_CAT};
	   $schema = $attrs->{TABLE_SCHEM};
	   $table = $attrs->{TABLE_NAME};
	   $type = $attrs->{TABLE_TYPE};
 	}

	$catalog = "" if (!$catalog);
	$schema = "" if (!$schema);
	$table = "" if (!$table);
	$type = "" if (!$type);

	# create a "blank" statement handle
	my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLTables" });

	DBD::altibase::st::_tables($dbh,$sth, $catalog, $schema, $table, $type)
	      or return undef;
	$sth;
    }

    sub primary_key_info {
       my ($dbh, $catalog, $schema, $table ) = @_;
 
       # create a "blank" statement handle
       my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLPrimaryKeys" });
 
       $catalog = "" if (!$catalog);
       $schema = "" if (!$schema);
       $table = "" if (!$table);
       DBD::altibase::st::_primary_keys($dbh,$sth, $catalog, $schema, $table )
	     or return undef;
       $sth;
    }

    sub foreign_key_info {
       my ($dbh, $pkcatalog, $pkschema, $pktable, $fkcatalog, $fkschema, $fktable ) = @_;
 
       # create a "blank" statement handle
       my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLForeignKeys" });
 
       $pkcatalog = "" if (!$pkcatalog);
       $pkschema = "" if (!$pkschema);
       $pktable = "" if (!$pktable);
       $fkcatalog = "" if (!$fkcatalog);
       $fkschema = "" if (!$fkschema);
       $fktable = "" if (!$fktable);
       _GetForeignKeys($dbh, $sth, $pkcatalog, $pkschema, $pktable, $fkcatalog, $fkschema, $fktable) or return undef;
       $sth;
    }

    sub ping {
	my $dbh = shift;
	my $state = undef;

 	my ($catalog, $schema, $table, $type);

	$catalog = "";
	$schema = "";
	$table = "NOXXTABLE";
	$type = "";

	# create a "blank" statement handle
	my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLTables_PING" });

	DBD::altibase::st::_tables($dbh,$sth, $catalog, $schema, $table, $type)
	      or return 0;
	$sth->finish;
	return 1;

    }

    # saved, just for posterity.
    sub oldping  {
	my $dbh = shift;
	my $state = undef;

	# should never 'work' but if it does, that's okay!
	# JLU incorporated patches from Jon Smirl 5/4/99
	{
	    local $dbh->{RaiseError} = 0 if $dbh->{RaiseError};
	    # JLU added local PrintError handling for completeness.
	    # it shouldn't print, I think.
	    local $dbh->{PrintError} = 0 if $dbh->{PrintError};
	    my $sql = "select sysdate from dual1__NOT_FOUND__CANNOT";
	    my $sth = $dbh->prepare($sql);
	    # fixed "my" $state = below.  Was causing problem with
	    # ping!  Also, fetching fields as some drivers (Oracle 8)
	    # may not actually check the database for activity until
	    # the query is "described".
	    # Right now, Oracle8 is the only known version which
	    # does not actually check the server during prepare.
	    my $ok = $sth && $sth->execute();

	    $state = $dbh->state;
	    $DBD::altibase::err = 0;
	    $DBD::altibase::errstr = "";
	    $DBD::altibase::sqlstate = "00000";
	    return 1 if $ok;
	}
	return 1 if $state eq 'S0002';	# Base table not found
 	return 1 if $state eq '42S02';  # Base table not found.Solid EE v3.51
	return 1 if $state eq 'S0022';	# Column not found
	return 1 if $state eq '37000';  # statement could not be prepared (19991011, JLU)
	# return 1 if $state eq 'S1000';  # General Error? ? 5/30/02, JLU.  This is what Openlink is returning
	# We assume that any other error means the database
	# is no longer connected.
	# Some special cases may need to be added to the code above.
	return 0;
    }

    # New support for the next DBI which will have a get_info command.
    # leaving support for ->func(xxx, GetInfo) (above) for a period of time
    # to support older applications which used this.
    sub get_info {
	my ($dbh, $item) = @_;
	# handle SQL_DRIVER_HSTMT, SQL_DRIVER_HLIB and
	# SQL_DRIVER_HDESC specially
	if ($item == 5 || $item == 135 || $item == 76) {
	   return undef;
	}
	return _GetInfo($dbh, $item);
    }

    # new override of do method provided by Merijn Broeren
    # this optimizes "do" to use SQLExecDirect for simple
    # do statements without parameters.
    sub do {
        my($dbh, $statement, $attr, @params) = @_;
        my $rows = 0;
        if( -1 == $#params )
        {
          # No parameters, use execute immediate
          $rows = ExecDirect( $dbh, $statement );
          if( 0 == $rows )
          {
            $rows = "0E0";
          }
          elsif( $rows < -1 )
          {
            undef $rows;
          }
        }
        else
        {
          $rows = $dbh->SUPER::do( $statement, $attr, @params );
        }
        return $rows
    }

    #
    # can also be called as $dbh->func($sql, ExecDirect);
    # if, for some reason, there are compatibility issues
    # later with DBI's do.
    #
    sub ExecDirect {
       my ($dbh, $sql) = @_;
       _ExecDirect($dbh, $sql);
    }

    # Call the altibase function SQLGetInfo
    # Args are:
    #	$dbh - the database handle
    #	$item: the requested item.  For example, pass 6 for SQL_DRIVER_NAME
    # See the altibase documentation for more information about this call.
    #
    sub GetInfo {
	my ($dbh, $item) = @_;
	get_info($dbh, $item);
    }

    # Call the altibase function SQLStatistics
    # Args are:
    # See the altibase documentation for more information about this call.
    #
    sub GetStatistics {
			my ($dbh, $Catalog, $Schema, $Table, $Unique) = @_;
			# create a "blank" statement handle
			my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLStatistics" });
			_GetStatistics($dbh, $sth, $Catalog, $Schema, $Table, $Unique) or return undef;
			$sth;
    }

    # Call the altibase function SQLForeignKeys
    # Args are:
    # See the altibase documentation for more information about this call.
    #
    sub GetForeignKeys {
			my ($dbh, $PK_Catalog, $PK_Schema, $PK_Table, $FK_Catalog, $FK_Schema, $FK_Table) = @_;
			# create a "blank" statement handle
			my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLForeignKeys" });
			_GetForeignKeys($dbh, $sth, $PK_Catalog, $PK_Schema, $PK_Table, $FK_Catalog, $FK_Schema, $FK_Table) or return undef;
			$sth;
    }

    # Call the altibase function SQLPrimaryKeys
    # Args are:
    # See the altibase documentation for more information about this call.
    #
    sub GetPrimaryKeys {
			my ($dbh, $Catalog, $Schema, $Table) = @_;
			# create a "blank" statement handle
			my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLPrimaryKeys" });
			_GetPrimaryKeys($dbh, $sth, $Catalog, $Schema, $Table) or return undef;
			$sth;
    }

    # Call the altibase function SQLSpecialColumns
    # Args are:
    # See the altibase documentation for more information about this call.
    #
    sub GetSpecialColumns {
	my ($dbh, $Identifier, $Catalog, $Schema, $Table, $Scope, $Nullable) = @_;
	# create a "blank" statement handle
	my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLSpecialColumns" });
	_GetSpecialColumns($dbh, $sth, $Identifier, $Catalog, $Schema, $Table, $Scope, $Nullable) or return undef;
	$sth;
    }
	
    sub GetTypeInfo {
	my ($dbh, $sqltype) = @_;
	# create a "blank" statement handle
	my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLGetTypeInfo" });
	# print "SQL Type is $sqltype\n";
	_GetTypeInfo($dbh, $sth, $sqltype) or return undef;
	$sth;
    }

    sub type_info_all {
	my ($dbh, $sqltype) = @_;
	$sqltype = DBI::SQL_ALL_TYPES unless defined $sqltype;
	my $sth = DBI::_new_sth($dbh, { 'Statement' => "SQLGetTypeInfo" });
	_GetTypeInfo($dbh, $sth, $sqltype) or return undef;
	my $info = $sth->fetchall_arrayref;
	unshift @$info, {
	    map { ($sth->{NAME}->[$_] => $_) } 0..$sth->{NUM_OF_FIELDS}-1
	};
	return $info;
    }

}


{   package DBD::altibase::st; # ====== STATEMENT ======
    use strict;

    sub ColAttributes {		# maps to SQLColAttributes
	my ($sth, $colno, $desctype) = @_;
	# print "before ColAttributes $colno\n";
	my $tmp = _ColAttributes($sth, $colno, $desctype);
	# print "After ColAttributes\n";
	$tmp;
    }

    sub cancel {
	my $sth = shift;
	my $tmp = _Cancel($sth);
	$tmp;
    }
}

1;
__END__

=head1 NAME

DBD::altibase - altibase Driver for DBI

=head1 SYNOPSIS

  use DBI;

  $dbh = DBI->connect('dbi:altibase:DSN', 'user', 'password');

See L<DBI> for more information.

=head1 DESCRIPTION

=head2 Notes:

=over 4

=item B<Change log/recent updates now in DBD::altibase::Changes.pm>

 Please note that the change log has been moved to DBD::altibase::Changes.pm
 To easily access this documentation, use perldoc DBD::altibase::Changes

 Also, while this document documents features, etc, I'm really trying to keep
 the FAQ type questions answered in the DBI FAQ itself... see
 http://dbi.perl.org for more information.
   
=item B<An Important note about the tests!>

 Please note that some tests may fail or report they are
 unsupported on this platform.  Notably Oracle's altibase driver
 will fail the "advanced" binding tests in t/08bind2.t.
 These tests run perfectly under SQL Server 2000. This is
 normal and expected.  Until Oracle fixes their drivers to
 do the right thing from an altibase perspective, it's going to
 be tough to fix the issue.  The workaround for Oracle is to
 bind date types with SQL_TIMESTAMP.
   
 Also note that some tests may be skipped, such as
 t/09multi.t, if your driver doesn't seem to support
 returning multiple result sets.  This is normal.

=item B<Version Control>

 I have recently placed all of the DBD::altibase source code
 under version control at svn.perl.org.  If you would like
 to use the "bleeding" edge version, you can get the latest
 from svn.perl.org via Subversion version control.  Note that I
 don't guarantee that this version is any different than what
 you get from the tarball from CPAN, but hey, it might be :)

 You may read about Subversion from:
   http://subversion.tigris.org

 You can get a subversion client from there and check dbd-odbc out via:

   svn checkout http://svn.perl.org/modules/dbd-odbc/trunk <your directory name here>

 Which will, after a short bit a of work, grab all the files into your
 local directory tree that you name.  For now, you can authenticate via the guest
 user and guest password.  That will change to be unauthenticated in the future for
 simple read-only checkout.
				    
=item B<Contributing>

 I am usually very open to contributions, but I have tended
 to get behind.  That's one strong reason why I've put everything
 in a shared version control environment.

 Please use Subversion (see above) to appropriately get the latest
 versions.

 Please, before submitting a patch:
    svn update
    <test your patch>
    svn diff > describe_my_diffs.patch

 and send the resuting file to me...

 It's probably best to send to dbi-users@perl.org, as I monitor that group.
 
=item B<Private DBD::altibase Attributes>

=item odbc_more_results (applies to statement handle only!)

Use this attribute to determine if there are more result sets
available.  SQL Server supports this feature.  Use this as follows:

do {
   my @row;
   while (@row = $sth->fetchrow_array()) {
      # do stuff here
   }
} while ($sth->{odbc_more_results});

Note that with multiple result sets and output parameters (i.e. using
bind_param_inout, don't expect output parameters to be bound until ALL
result sets have been retrieved.

=item odbc_ignore_named_placeholders

Use this if you have special needs (such as Oracle triggers, etc) where
:new or :name mean something special and are not just place holder names
You I<must> then use ? for binding parameters.  Example:
 $dbh->{odbc_ignore_named_placeholders} = 1;
 $dbh->do("create trigger foo as if :new.x <> :old.x then ... etc");

Without this, DBD::altibase will think :new and :old are placeholders for binding
and get confused.
 
=item odbc_default_bind_type

This value defaults to 0.  Older versions of DBD::altibase assumed that the binding
type was 12 (SQL_VARCHAR).  Newer versions default to 0, which means that
DBD::altibase will attempt to query the driver via SQLDescribeParam to determine
the correct type.  If the driver doesn't support SQLDescribeParam, then DBD::altibase
falls back to using SQL_VARCHAR as the default, unless overridden by bind_param()

=item odbc_force_rebind

This is to handle special cases, especially when using multiple result sets.
Set this before execute to "force" DBD::altibase to re-obtain the result set's
number of columns and column types for each execute.  Especially useful for
calling stored procedures which may return different result sets each
execute.  The only performance penalty is during execute(), but I didn't
want to incur that penalty for all circumstances.  It is probably fairly
rare that this occurs.  This attribute will be automatically set when
multiple result sets are triggered.  Most people shouldn't have to worry
about this.

=item odbc_async_exec

Allow asynchronous execution of queries.  Right now, this causes a spin-loop
(with a small "sleep") until the sql is complete.  This is useful, however,
if you want the error handling and asynchronous messages (see the err_handler)
below.  See t/20SQLServer.t for an example of this.

=item odbc_exec_direct

Force DBD::altibase to use SQLExecDirect instead of SQLPrepare() then SQLExecute.
There are drivers that only support SQLExecDirect and the DBD::altibase
do() override doesn't allow returning result sets.  Therefore, the
way to do this now is to set the attributed odbc_exec_direct.
There are currently two ways to get this:
	$dbh->prepare($sql, { odbc_exec_direct => 1}); 
 and
	$dbh->{odbc_exec_direct} = 1;
 When $dbh->prepare() is called with the attribute "ExecDirect" set to a non-zero value 
 dbd_st_prepare do NOT call SQLPrepare, but set the sth flag odbc_exec_direct to 1.
 
=item odbc_err_handler

Allow errors to be handled by the application.  A call-back function supplied
by the application to handle or ignore messages.  If the error handler returns
0, the error is ignored, otherwise the error is passed through the normal
DBI error handling structure(s).

This can also be used for procedures under MS SQL Server (Sybase too, probably)
to obtain messages from system procedures such as DBCC.  Check t/20SQLServer.t
and mytest/testerrhandler.pl

The callback function takes three parameters: the SQLState, the ErrorMessage and
the native server error.
 
=item odbc_SQL_ROWSET_SIZE

Here is the information from the original patch, however, I've learned
since from other sources that this could/has caused SQL Server to "lock up".
Please use at your own risk!
   
SQL_ROWSET_SIZE attribute patch from Andrew Brown 
> There are only 2 additional lines allowing for the setting of
> SQL_ROWSET_SIZE as db handle option.
>
> The purpose to my madness is simple. SqlServer (7 anyway) by default
> supports only one select statement at once (using std altibase cursors).
> According to the SqlServer documentation you can alter the default setting
> of
> three values to force the use of server cursors - in which case multiple
> selects are possible.
>
> The code change allows for:
> $dbh->{SQL_ROWSET_SIZE} = 2;    # Any value > 1
>
> For this very purpose.
>
> The setting of SQL_ROWSET_SIZE only affects the extended fetch command as
> far as I can work out and thus setting this option shouldn't affect
> DBD::altibase operations directly in any way.
>
> Andrew
>

=item SQL_DRIVER_altibase_VER

This, while available via get_info() is captured here.  I may get rid of this
as I only used it for debugging purposes.  
 
=item odbc_cursortype (applies to connect only!)

This allows multiple concurrent statements on SQL*Server.  In your connect, add
{ odbc_cursortype => 2 }.  If you are using DBI > 1.41, you should also be able
to use { odbc_cursortype => DBI::SQL_CURSOR_DYNAMIC } instead.  For example:

 my $dbh = DBI->connect("dbi:altibase:$DSN", $user, $pass, { RaiseError => 1, odbc_cursortype => 2});
 my $sth = $dbh->prepare("one statement");
 my $sth2 = $dbh->prepare("two statement");
 $sth->execute;
 my @row;
 while (@row = $sth->fetchrow_array) {
    $sth2->execute($row[0]);
 }

See t/20SqlServer.t for an example.


=item odbc_query_timeout 

This allows the end user to set a timeout for queries on the altibase side.  After your connect, add
{ odbc_query_timeout => 30 } or set on the dbh before executing the statement.  The default is 0, no timeout.
Note that some drivers may not support this attribute.

See t/20SqlServer.t for an example.

   
=item odbc_version (applies to connect only!)

This was added prior to the move to altibase 3.x to allow the caller to "force" altibase 3.0
compatibility.  It's probably not as useful now, but it allowed get_info and get_type_info
to return correct/updated information that altibase 2.x didn't permit/provide.
Since DBD::altibase is now 3.x, this can be used to force 2.x behavior via something like:
  my $dbh = DBI->connect("dbi:altibase:$DSN", $user, $pass, { odbc_version => 2});
       
   
=item B<Private DBD::altibase Functions>

=item GetInfo (superceded by get_info(), the DBI standard)

This function maps to the altibase SQLGetInfo call.  This is a Level 1 altibase
function.  An example of this is:

  $value = $dbh->func(6, GetInfo);

This function returns a scalar value, which can be a numeric or string value.  
This depends upon the argument passed to GetInfo.


=item SQLGetTypeInfo (superceded by get_type_info(), the DBI standard)

This function maps to the altibase SQLGetTypeInfo call.  This is a Level 1
altibase function.  An example of this is:

  use DBI qw(:sql_types);

  $sth = $dbh->func(SQL_ALL_TYPES, GetInfo);
  while (@row = $sth->fetch_row) {
    ...
  }

This function returns a DBI statement handle, which represents a result
set containing type names which are compatible with the requested
type.  SQL_ALL_TYPES can be used for obtaining all the types the altibase
driver supports.  NOTE: It is VERY important that the use DBI includes
the qw(:sql_types) so that values like SQL_VARCHAR are correctly
interpreted.  This "imports" the sql type names into the program's name
space.  A very common mistake is to forget the qw(:sql_types) and
obtain strange results.

=item GetFunctions (now supports altibase V3)

This function maps to the altibase API SQLGetFunctions.  This is a Level 1
API call which returns supported driver funtions.  Depending upon how
this is called, it will either return a 100 element array of true/false
values or a single true false value.  If it's called with
SQL_API_ALL_FUNCTIONS (0), it will return the 100 element array.
Otherwise, pass the number referring to the function.  (See your altibase
docs for help with this).

=item SQLColumns 

Support for this function has been added in version 0.17.  It looks to be
fixed in version 0.20.

Use the DBI statement handle attributes NAME, NULLABLE, TYPE, PRECISION and
SCALE, unless you have a specific reason.

=item Connect without DSN
The ability to connect without a full DSN is introduced in version 0.21.

Example (using MS Access):
	my $DSN = 'driver=Microsoft Access Driver (*.mdb);dbq=\\\\cheese\\g$\\perltest.mdb';
	my $dbh = DBI->connect("dbi:altibase:$DSN", '','') 
		or die "$DBI::errstr\n";

=item SQLStatistics

=item SQLForeignKeys

See DBI's get_foreign_keys.
   
=item SQLPrimaryKeys

See DBI's get_primary_keys
   
=item SQLDataSources

Handled, currently (as of 0.21), also see DBI's data_sources()

=item SQLSpecialColumns

Handled as of version 0.28
 
=item Others/todo?

Level 1

    SQLTables (use tables()) call

Level 2

    SQLColumnPrivileges
    SQLProcedureColumns
    SQLProcedures
    SQLTablePrivileges
    SQLDrivers
    SQLNativeSql

=back

=head2 Using DBD::altibase with web servers under Win32. 

=over 4

=item General Commentary re web database access

This should be a DBI faq, actually, but this has somewhat of an
Win32/altibase twist to it.

Typically, the Web server is installed as an NT service or a Windows
95/98 service.  This typically means that the web server itself does
not have the same environment and permissions the web developer does.
This situation, of course, can and does apply to Unix web servers.
Under Win32, however, the problems are usually slightly different.

=item Defining your DSN -- which type should I use?

Under Win32 take care to define your DSN as a system DSN, not as a user
DSN.  The system DSN is a "global" one, while the user is local to a
user.  Typically, as stated above, the web server is "logged in" as a
different user than the web developer.  This helps cause the situation
where someone asks why a script succeeds from the command line, but
fails when called from the web server.

=item Defining your DSN -- careful selection of the file itself is important!

For file based drivers, rather than client server drivers, the file
path is VERY important.  There are a few things to keep in mind.  This
applies to, for example, MS Access databases.

1) If the file is on an NTFS partition, check to make sure that the Web
B<service> user has permissions to access that file.

2) If the file is on a remote computer, check to make sure the Web
B<service> user has permissions to access the file.

3) If the file is on a remote computer, try using a UNC path the file,
rather than a X:\ notation.  This can be VERY important as services
don't quite get the same access permissions to the mapped drive letters
B<and>, more importantly, the drive letters themselves are GLOBAL to
the machine.  That means that if the service tries to access Z:, the Z:
it gets can depend upon the user who is logged into the machine at the
time.  (I've tested this while I was developing a service -- it's ugly
and worth avoiding at all costs).

Unfortunately, the Access altibase driver that I have does not allow one to
specify the UNC path, only the X:\ notation.  There is at least one way
around that.  The simplest is probably to use Regedit and go to
(assuming it's a system DSN, of course)
HKEY_LOCAL_USERS\SOFTWARE\altibase\"YOUR DSN" You will see a few settings
which are typically driver specific.  The important value to change for
the Access driver, for example, is the DBQ value.  That's actually the
file name of the Access database.

=item Connect without DSN
The ability to connect without a full DSN is introduced in version 0.21.

Example (using MS Access):
	my $DSN = 'driver=Microsoft Access Driver
(*.mdb);dbq=\\\\cheese\\g$\\perltest.mdb';
	my $dbh = DBI->connect("dbi:altibase:$DSN", '','') 
		or die "$DBI::errstr\n";

The above sample uses Microsoft's UNC naming convention to point to the MSAccess
file (\\\\cheese\\g$\\perltest.mdb).  The dbq parameter tells the access driver
which file to use for the database.
   
Example (using MSSQL Server):
      my $DSN = 'driver={SQL Server};Server=server_name;
      database=database_name;uid=user;pwd=password;';
      my $dbh  = DBI->connect("dbi:altibase:$DSN") or die "$DBI::errstr\n";

=back

=head2 Random Links

These are in need of sorting and annotating. Some are relevant only
to altibase developers (but I don't want to loose them).

	http://www.ids.net/~bjepson/freealtibase/index.html

	http://dataramp.com/

	http://www.syware.com

	http://www.microsoft.com/odbc

   For Linux/Unix folks, compatible altibase driver managers can be found at:
   
        http://www.easysoft.com		unixaltibase driver manager source
				        *and* altibase-altibase bridge for accessing Win32 altibase sources from Linux

        http://www.iodbc.org		ialtibase driver manager source

   For Linux/Unix folks, you can checkout the following for another altibase-altibase bridge and support for ialtibase.

	http://www.openlink.co.uk 
		or
	http://www.openlinksw.com 

   And another:
        OpenRDA
   
        http://www.atinet.com/support/openrda_samples.asp
 


=head2 Frequently Asked Questions

Answers to common DBI and DBD::altibase questions:

=over 4
 
=item How do I read more than N characters from a Memo | BLOB | LONG field?

See LongReadLen in the DBI docs.  

Example:
	$dbh->{LongReadLen} = 20000;
	$sth = $dbh->prepare("select long_col from big_table");
	$sth->execute;
	etc

=item What is DBD::altibase?  Why can't I connect?  Do I need an altibase driver?  What is the altibase driver manager?

These, general questions lead to needing definitions.

1) altibase Driver - the driver that the altibase manager uses to connect
and interact with the RDBMS.  You DEFINITELY need this to 
connect to any database.  For Win32, they are plentiful and installed
with many applications.  For Linux/Unix, some hunting is required, but
you may find something useful at:

	http://www.openlinksw.com
        http://www.easysoft.com
	http://www.intersolv.com
	http://www.atinet.com/support/openrda_samples.asp
	      

2) altibase Driver Manager - the piece of software which interacts with the drivers
for the application.  It "hides" some of the differences between the
drivers (i.e. if a function call is not supported by a driver, it 'hides'
that and informs the application that the call is not supported.
DBD::altibase needs this to talk to drivers.  Under Win32, it is built in
to the OS.  Under Unix/Linux, in most cases, you will want to use freealtibase,
unixaltibase or ialtibase.  ialtibase was bundled with DBD::altibase, but you will need to find one
which suits your needs.  Please see www.openlinksw.com, www.easysoft.com or www.iodbc.org

3) DBD::altibase.  DBD::altibase uses the driver manager to talk to the altibase driver(s) on
your system.  You need both a driver manager and driver installed and tested
before working with DBD::altibase.  You need to have a DSN (see below) configured
*and* TESTED before being able to test DBD::altibase.

4) DSN -- Data Source Name.  It's a way of referring to a particular database by any
name you wish.  The name itself can be configured to hide the gory details of
which type of driver you need and the connection information you need to provide.
For example, for some databases, you need to provide a TCP address and port.
You can configure the DSN to have use information when you refer to the DSN.

=item Where do I get an altibase driver manager for Unix/Linux?

DBD::altibase comes with one (ialtibase).  In the DBD::altibase source release is a directory named iodbcsrc.  
There are others.  Unixaltibase, Freealtibase and some of the drivers will come with one of these managers.
For example Openlink's drivers (see below) come with the ialtibase driver manager.  Easysoft
supplies both altibase-altibase bridge software and unixaltibase.

=item How do I access a MS SQL Server database from Linux?

Try using drivers from http://www.openlinksw.com or www.easysoft.com
The multi-tier drivers have been tested with Linux and Redhat 5.1.

=item How do I access an MS-Access database from Linux?

I believe you can use the multi-tier drivers from http://www.openlinksw.com, however, I have
not tested this.  Also, I believe there is a commercial solution from http://www.easysoft.com.  I
have not tested this.

If someone does have more information, please, please send it to me and I will put it in this
FAQ.

=item Almost all of my tests for DBD::altibase fail.  They complain about not being able to connect
or the DSN is not found.  

Please, please test your configuration of altibase and driver before trying to test DBD::altibase.  Most
of the time, this stems from the fact that the DSN (or altibase) is not configured properly.  ialtibase
comes with a odbctest program.  Please use it to verify connectivity.

=item For Unix -> Windows DB see Tom Lowery's write-up.

http://tlowery.hypermart.net/perl_dbi_dbd_faq.html#HowDoIAccessMSWindowsDB

=item I'm attempting to bind a Long Var char (or other specific type) and the binding is not working.
The code I'm using is below:

	$sth->bind_param(1, $str, $DBI::SQL_LONGVARCHAR);
                                 ^^^
The problem is that DBI::SQL_LONGVARCHAR is not the same as $DBI::SQL_LONGVARCHAR and that
$DBI::SQL_LONGVARCHAR is an error!

It should be:

	$sth->bind_param(1, $str, DBI::SQL_LONGVARCHAR);


=cut
