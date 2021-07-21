#!/usr/bin/perl -w
# $Id: testconn.pl 11810 2005-05-18 04:43:35Z qa $

use strict;
use DBI;

my ($sid,$user,$passwd) = ('ZDB','TEST','TEST');


# My little helper function, call with:
# The DBI database handle, The SQL code and the parameters.
# It will then return the result of the execute (or $sth if it's a dataset).
sub sql {
    my $db = shift;
    my $code = shift;
    my @param = @_;

    chomp $code; # No good reason to have a tailing \n
    
    my $sth = $db->prepare_cached($code);
    if (!$sth) {
	print "Unable to prepare: ".$DBI::errstr."\nSQL: $code\n";
	return undef;
    }
    
    my $res = $sth->execute(@param);
    if (!$res) {
	my $par = '';
	foreach my $i (@param) {
	    if (defined $i) {
		$par .= "'$i',";
	    } else {
		$par .= "undef,";
	    }
	}
	chop $par;
	print "Unable to execute: ".$DBI::errstr."\nSQL: $code\nPAR: $par\n";
	return undef;
    }

    if (ref($res) eq 'ARRAY') {
	return $res;
    } else {
	return $sth;
    }
}

sub conn() {
#    print STDERR "Connecting (to $sid as $user/$passwd)...";
    my $db = DBI->connect_cached("dbi:SAP_DB:$sid", $user, $passwd,
			  { 
			      AutoCommit=>0,
			      LongReadLen=>10000,
			  }
			  );
    
    unless ($db) {
	die "Failed to connect to the database $sid as user $user with password $passwd: ".$DBI::errstr;
    }
#    print STDERR "Done.\n";
    return $db;
}

sub status($) {
    my $line = shift;

    open STATUS, "</proc/self/status";
    my %status;
    while (my $l = <STATUS>) {
	if ($l =~ /^([^:]+):\s+(\S+)/) {
	    $status{$1} = $2;
	}
    }
    close STATUS;

    return $status{$line};
}

my @leak;

for (my $i=10; $i<200; $i++) {

    my $c1 = status('VmSize');

    my $db = conn();
    print "$db\n";

    for (my $j=3;$j<10;$j++) {

	for (my $k=0;$k<2;$k++) {
	    my $res = sql($db,"select $i+$j from dual");
	    $res->fetchrow_array;
	}

#	my $q1 = status('VmSize');
	for (my $k=0;$k<30;$k++) {
	    my $res = sql($db,"select $i+$j from dual");
	    my ($r) = $res->fetchrow_array;
	    die if $i+$j != $r;
	}
#	my $q2 = status('VmSize');
#	print STDERR "Query $j leaked: ".($q2-$q1)." kB\n";

    }
        
    $db->rollback;
#    $db->disconnect;
    $db = undef;

    my $c2 = status('VmSize');

    print "Connection $i leaked: ".($c2-$c1)." kB\n";

    print "Sleeping for ".(600*$i/60)." minutes\n";
    for(my $j=0;$j<60*$i;$j++) {
	sleep(10);
	print STDERR ".";
    }
}








