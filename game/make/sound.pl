#!/usr/bin/perl -W

use strict;

my %ignore = ( 'interface'=>1 );

sub process_dir
{
	my $path = shift(@_);
	my $path_out = shift(@_);
	my $dir;

	opendir $dir, $path or die "can't open dir '$path': $!";

	while ( my $file = readdir( $dir ) )
	{
		next if( '.' eq $file or '..' eq $file or exists $ignore{$file} );

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
			my $cmd = "../bin_tools/venc -q9 $in";
			print "$cmd\n";
			print `$cmd`;

			rename("$in.ogg", $out) or die "couldn't rename: $!";
		}
	}

	closedir $dir;
}


process_dir('../workdir/sounds', '../workdir/sounds');

print "---------------------------------------------------\n";
print "Done.\n";

<STDIN>
