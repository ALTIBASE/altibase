#!/usr/bin/perl
system("bison -v qpply.y");
open(GRAMMAR,"qpply.output") || die "Can't open qpply.output\n";
open(TEX,"> qpply_grammar.tex") || die "Can't open tex output file\n";

$oldline = "nothing yet";

while (<GRAMMAR>) {
    if (m/^rule [1-9][0-9]*/) {

        s/^rule [1-9][0-9]*\s*//g;

        $thisline = substr($_, 0, index($_," "));

        s/^\w+\s+\-\>\s+//g;
        s/([a-z_]+)\s/ {\\it $1} /g;
        s/([A-Z_]+)\s/ {\\bf $1} /g;
        s/\'([^\'])\'/{\\bf $1}/g;

        $_ =~ tr/[A-Z]/[a-z]/;

        s/_/\\_/g;
        $thisline =~ s/_/\\_/g;

        chop($_);

        if ($oldline ne $thisline) {

            print TEX "\\par\\noindent\n{\\it $thisline:}\\\\\n";
        }

        print TEX "\\indent $_ \\\\\n";

        $oldline = $thisline;
    }
}
