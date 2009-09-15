#!/usr/bin/perl -W

use strict;

my %ignore = ( 'interface'=>1, '.'=>1, '..'=>1, '.svn'=>1 );

sub process_dir
{
	my $path = shift(@_);
	my $path_out = shift(@_);
	my $dir;

	opendir $dir, $path or die "can't open dir '$path': $!";

	while ( my $file = readdir( $dir ) )
	{
		next if( exists $ignore{$file} );

		my $in = "$path/$file";

		if( -d $in )
		{
			print ">> entering directory '$in'\n";
			process_dir($in, $path_out);
			print "<< leaving directory '$in'\n";
		}

		if( -f "$in" and $in =~ /\.wav$/ )
		{
		#	my $out = "$path_out/$in";
			my $out = $in;
			$out =~ s/wav$/ogg/;
			my $cmd = "venc -q9 $in";
			print "$cmd\n";
			print `$cmd`;

			if( not rename("$in.ogg", $out) )
			{
				-f $out or die "no output file produced";
			}
		}
	}

	closedir $dir;
}


process_dir('../data/sounds', '../data/sounds');

print "---------------------------------------------------\n";
print "Done.\n";

