@rem = '--*-Perl Script Wrapper-*--';
@rem = '
@echo off
set CMD=%0
set ARGS=
:loop
if .%1==. goto endloop
set ARGS=%ARGS% %1
shift
goto loop
:endloop
perl.exe -w -S %CMD% %ARGS%
goto endofperl
@rem ';

###############################################################################

use strict;

###########################################################
# Version

my $tm = localtime;
my $ver = shift || sprintf "internal %02d/%02d/%02d %02d:%02d", $tm->mday, $tm->mon+1, $tm->year-100, $tm->hour, $tm->min;
my $ver1 = $ver;
$ver1 =~ s/[\s\.]/_/g;
$ver1 =~ s/[\\\/\:]//g;

print "Building version $ver\n";

###########################################################
# Other vars

my $package = "..\\ac3file_$ver1";
my $bin = "..\\ac3file_$ver1.exe";
my $src = "ac3file_${ver1}_src.zip";

my $make_bin = "\"c:\\Devel\\NSIS\\makensis\" /NOCONFIG /DVERSION=\"$ver\" /DSETUP_FILE=$bin /DSOURCE_DIR=$package ac3file.nsi";
my $make_src = "pkzip25 -add -rec -dir -excl=CVS -lev=9 $src ac3file\\*.* valib\\*.*";

sub read_file
{
  my ($filename) = (@_);
  open (FILE, "< $filename");
  my @result = <FILE>;
  close (FILE);
  return @result;
}

sub write_file
{
  my $filename = shift;
  open (FILE, "> $filename");
  print FILE @_;
  close (FILE);
}

###############################################################################
##
## Clean
##

printf "Cleaning...\n";
`rmdir /s /q $package 2> nul`;
`mkdir $package`;


###############################################################################
##
## Build project
##

print "Building project...\n";

system('msdev ac3file.dsp /MAKE "ac3file - Win32 Release" /REBUILD')
  && die "failed!!!";

`copy release\\ac3file.ax $package`;
`rmdir /s /q debug   2> nul`;
`rmdir /s /q release 2> nul`;
`_clear.bat`;

###############################################################################
##
## Prepare documentation files
##
##
## print "Prepairing documentaion files...\n";
## 
## my $changelog = join("<br>", read_file("_changes.eng"));
## $changelog =~ s/\s(?=(\s))/\&nbsp\;/g;
## write_file("$package\\ac3filter_eng.html", grep { s/(\$\w+)/$1/gee + 1 } read_file("doc\\ac3filter_eng.html"));
## write_file("$package\\ac3filter_ita.html", grep { s/(\$\w+)/$1/gee + 1 } read_file("doc\\ac3filter_ita.html"));
## 
## my $changelog = join("<br>", read_file("_changes.rus"));
## $changelog =~ s/\s(?=(\s))/\&nbsp\;/g;
## write_file("$package\\ac3filter_rus.html", grep { s/(\$\w+)/$1/gee + 1 } read_file("doc\\ac3filter_rus.html"));
## 
## `mkdir $package\\pic`;
## `copy doc\\pic $package\\pic`;
##

###############################################################################
##
## Prepare documentation files
##

`copy _readme.txt $package`;
`copy _changes_eng.txt $package`;
`copy _changes_rus.txt $package`;
`copy GPL_eng.txt $package`;
`copy GPL_rus.txt $package`;


###############################################################################
##
## Make distributives
##

`del $bin 2> nul`;
`del $src 2> nul`;

system($make_bin);

chdir("..\\valib");
`_clear.bat`;
chdir("..");
system($make_src);

###############################################################################

__END__
:endofperl
