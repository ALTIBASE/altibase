use DBI;
# $Id: testdatasources.pl 11810 2005-05-18 04:43:35Z qa $

print join(', ', DBI->data_sources("ODBC")), "\n";
print $DBI::errstr;
print "\n";
