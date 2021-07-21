#!perl -w
# $Id: 01base.t 11810 2005-05-18 04:43:35Z qa $

use Test::More tests => 5;

require DBI;
require_ok('DBI');

import DBI;
pass("import DBI");

$switch = DBI->internal;
is(ref $switch, 'DBI::dr', "DBI->internal is DBI::dr");

$drh = DBI->install_driver('ODBC');
is(ref $drh, 'DBI::dr', "Install ODBC driver OK");

ok($drh->{Version}, "Version is not empty");

exit 0;
