#!/usr/bin/perl

# This script generate the deployement file for a given platform

use strict;
use warnings;

if($#ARGV!=0) {
        die "Usage: perl gen_deployment.pl <file.xml>";
}

my($platform_file)=$ARGV[0];

open FILE, $platform_file or die "Unable to open $platform_file";

my $line;

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">\n";
print "<platform version=\"3\">\n";

while (defined($line = <FILE>)) {
        if ($line =~ /<cluster/) {
                $line =~ /prefix="([^"]+)"/;
                my $prefix = $1;
                $line =~ /suffix="([^"]+)"/;
                my $suffix = $1;
                my $core = 4;
                if ($line =~ /core="([0-9]+)"/) {
                        $core = scalar $1;
                }
                
                if ($line =~ /radical="([0-9]+)-([0-9]+)"/) {
                        print_cluster($prefix, $suffix, $core, $1, $2);
                } elsif ($line =~ /radical="([0-9]+)-([0-9]+),([0-9]+)-([0-9]+)"/) {
                        print_cluster($prefix, $suffix, $core, $1, $2);
                        print_cluster($prefix, $suffix, $core, $3, $4);
                }
        } elsif ($line =~ /<host/) {
                $line =~ /id="([^"]+)"/;
                my $host = $1;
                my $core = 4;
                if ($line =~ /core="([0-9]+)"/) {
                        $core = scalar $1;
                }
                print_host($host, $core);
        } elsif ($line =~ /<peer/) {
                $line =~ /id="([^"]+)"/;
                my $host = $1;
                my $core = 4;
                print_host($host, $core);
        }
}
print "</platform>";

close(FILE);

sub print_host {
        my $host = shift;
        my $core = shift;
                
        print "        <process host=\"$host\" function=\"comm\"></process>\n";
        for (my $j=0;$j<$core;$j++) {
                print "        <process host=\"$host\" function=\"worker\"></process>\n";
        }
}

sub print_cluster {
        my $prefix = shift;
        my $suffix = shift;
        my $core = shift;
        my $radical_first = shift;
        my $radical_last = shift;
        for(my $i=$radical_first;$i<=$radical_last;$i++) {
                my $host = "$prefix$i$suffix";
                print_host($host, $core);
        }
}
