#!/usr/bin/perl -W

use strict;
use File::Copy;

sub test
{
	my $path = shift(@_);
	my $patterns = shift(@_);
	
	if( $patterns )
	{
		foreach my $i ( @$patterns )
		{
			if( $path =~ /$i/ )
			{
				return 1;
			}
		}
	}
	return 0;
}

sub copy_dir
{
	my $path = shift(@_);
	my $path_out = shift(@_);
	my $ignore = shift(@_);
	my $force = shift(@_);
	my $recursive = shift(@_);
	
	my $dir;

	opendir $dir, $path or die "can't open dir '$path': $!";

	while( my $file = readdir($dir) )
	{
		next if( '.' eq $file or '..' eq $file );

		my $in = "$path/$file";
		my $out = "$path_out/$file";

		if( not $force or not test($in, $force) )
		{
			if( $ignore and test($in, $ignore) )
			{
				print "skipping file '$in'\n";
				next;
			}
		}


		if( $recursive and -d $in )
		{
			mkdir($out) and print "created directory $out\n";
			print ">> entering directory '$in'\n";
			copy_dir($in, $out, $ignore, $force, $recursive);
			print "<< leaving directory '$in'\n";
			next;
		}

		if( -f $in )
		{
			if( copy($in, $out) )
			{
				print "copy '$in' >> '$out' - ok\n";
			}
			else
			{
				print "couldn't copy '$in' to '$out' - $!\n";
			}
		}
	}

	closedir $dir;
}

###############################################################################


mkdir "../out/Tank" or die "direcory 'out/Tank' is already exists";
mkdir "../out/Tank/data" or die "direcory 'out/Tank/data' is already exists";

# copy binary files from 'out/release'
copy_dir("../out/release", "../out/Tank",
	['.'], # ignore all
	[      # but force this files
		'tank\.exe$',
	],
	0   # non-recursive
);

# copy resource files
copy_dir("../data", "../out/Tank/data",
	[      # ignore this files
		'\.svn$',
		'\.psd$',
		'\.wav$',
		'\.sav$',
		'\.scc$',
		'screenshot\d+\.tga$',
		'Копия fonts\.tga$',
		'sounds\/interface',
		'maps\/\_.+\.map$',
		'vsluaplugin_log\.txt$',
	],
	undef, # force none
	1      # recursive
);

# other stuff
copy_dir("..", "../out/Tank",
	['.'], # ignore all
	[      # but force this files
		'Readme\.txt$',
		'History\.txt$',
		'license\.txt$',
	],
	0   # non-recursive
);

print "Done.\n";


#end of file