#!/usr/bin/perl -W

# reg/unreg=ipv4
# key=unique key
# ver=current version
#
# if version does not match, server prints required version



use strict;
use warnings;

use CGI qw(:standard);
use CGI::Carp qw(fatalsToBrowser);


my $dbfile = './data/servers.db';
my $idfile = './data/servers.id';
my $maxrecords = 100;
my $timeout = 60;
my $version = '149b';


sub escape_chr($)
{
	my ($letter) = (@_);
	my %table  = ( 'n'=>"\n", 't'=>"\t" );
	return exists $table{$letter} ? $table{$letter} : $letter;
}

sub addrefresh($$$)
{
	my $addr = $_[0];
	my $key = $_[1];
	my $reg = $_[2];
	
	#
	# verify data
	#
	
	if( $addr !~ m/^(\d+)\.(\d+)\.(\d+)\.(\d+):(\d+)$/ or 
	    $1 > 255 or $2 > 255 or $3 > 255 or $4 > 255 or $5 > 65535 )
	{
		print "invalid addres";
		exit;
	}
	
	if( $key !~ m/^[a-zA-Z0-9]{10,10}$/ )
	{
		print "invalid key";
		exit;
	}
	

	#
	# load the data base and store it in array of hashes
	#

	my %db = ();
	my $dbsize = 0;
	my $found = 0;

	open DB, "+< $dbfile" or die "couldn't open database: $!";
	flock(DB, 2); # exclusive lock
	while( <DB> )
	{
		next unless( /^\s*(\d+): / );
		my $id = $1;
		my %current = ();
		while( /([a-z_][a-z_0-9]*)="(([^\"\\]|\\.)*)"/gi ) # detect fields with escape characters
		{
			my ($key, $value) = ($1, $2);
			$value =~ s/\\(.)/escape_chr($1)/ge; # expand escape characters
			$current{$key} = $value;
		}
		
		# skip records which are empty or out of date
		next if( time() - $current{'updated'} > $timeout or not %current );
		
		# refresh record if key and addr match
		if( $current{'addr'} eq $addr )
		{
			$found = 1;
			if( $current{'key'} eq $key )
			{
				if( $reg )
				{
					$current{'updated'} = time();
				}
				else
				{
					next; # unregisted server
				}
			}
			else
			{
				print "wrong key\n";
				exit;
			}
		}

		# add to database
		$dbsize++;
		$db{$id} = \%current;
	}
	
	# add new record if it does not exist
	if( $reg and not $found )
	{
		if( $dbsize < $maxrecords )
		{
			# generate new id
			my $newid;
			open IDFILE, "+< $idfile" or die "could not open id file: $!";
			flock(IDFILE, 2) or die "could not lock id file: $!";
			$newid = <IDFILE>;
			$newid =~ s/\D//g;  # strip non-digits
			$newid = 1 if(not $newid);
			seek(IDFILE, 0, 0) or die "cant seek id file: $!";
			truncate(IDFILE, 0);
			print IDFILE ($newid+1) or die "cant write new id value: $!";
			close IDFILE or die "cant close id file: $!";
			
			# add record
			$dbsize++;
			$db{$newid} = 
			{
				'created' => time(),
				'updated' => time(),
				'addr'    => $addr,
				'key'     => $key,
			};
		}
		else
		{
			print "limit exceeds";
		}
	}
	
	# write file
	seek(DB, 0, 0) or die "cant seek db file: $!";
	truncate(DB, 0) or die "cant truncate db file: $!";;
	foreach my $id (keys %db)
	{
		my $current = $db{$id};
		print DB "$id: ";
		foreach( keys %$current )
		{
			print DB "$_=\"$current->{$_}\" ";
		}
		print DB "\n";
	}

	print "ok";
	close DB;
}


###########################################
# main

my $ip = exists $ENV{REMOTE_ADDR} ? $ENV{REMOTE_ADDR} : '0.0.0.0';

# check version
if( param('ver') ne $version )
{
	print "Content-type: text/plain\n\n";
	print "version $version\n";
	exit;
}

# register new server?
if( param('reg') and param('key') )
{
	print "Content-type: text/plain\n\n";
	addrefresh("$ip:".param('reg'), param('key'), 1);
	exit;
}

# unregister server?
if( param('unreg') and param('key') )
{
	print "Content-type: text/plain\n\n";
	addrefresh("$ip:".param('unreg'), param('key'), 0);
	exit;
}




#
# load and print data base
#

print "Content-type: text/plain\n\n";

open DB, "< $dbfile" or die "couldn't open database: $!";
flock(DB, 1); # non-exclusive lock
while( <DB> )
{
	next unless( /^\s*(\d+): / );
	my $id = $1;
	my %current = ();
	while( /([a-z_][a-z_0-9]*)="(([^\"\\]|\\.)*)"/gi ) # detect fields with escape characters
	{
		my ($key, $value) = ($1, $2);
		$value =~ s/\\(.)/escape_chr($1)/ge; # expand escape characters
		$current{$key} = $value;
	}
	if( %current and time() - $current{'updated'} <= $timeout )
	{
		print $current{addr}, "\n";
	}
}
close DB;

print "end\n"

#print <<'_END_';
#   hello world
#_END_


#addrefresh("192.168.1.1", "qwerty1", 1);

# end of file
