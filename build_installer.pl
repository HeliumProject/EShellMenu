#!/usr/bin/perl -w
use strict;
use Data::Dumper;
use File::Basename;
use File::Copy;
use File::Path;
use Getopt::Long;


#
# globals
#

my $g_ISCCFilePath        = File::Spec->catfile( 'submodule', 'InnoSetup', 'ISCC.exe' );
my $g_ISSFilePath         = 'EShellMenuSetup.iss';
my $g_OutputBaseFilename  = 'EShellMenuSetup';
my $g_OutputDir           = 'install';
my $g_VersionFilePath     = 'resource.h';
my $g_ChangelistFilePath  = 'changelist.txt';
my $g_VersionTxtFilePath  = File::Spec->catfile( $g_OutputDir, 'version.txt' );
my $g_OutputFilePath      = File::Spec->catfile( $g_OutputDir, "$g_OutputBaseFilename.exe" );


#
# parse options
#

my $g_Publish = 0;
my $gotOptions = GetOptions
(
  "publish" => \$g_Publish,
);

if ( !$gotOptions )
{
  print STDERR ( "Usage: build_installer [-publish]\n" );
  exit 1;
}

open (FOO, "install_dir.txt") || die "ERROR Unable to open install_dir.txt, please create one with the path to the installer directory: $!\n";
my @install_dir = <FOO>;
close FOO;

my $g_InstallDir = shift (@install_dir);


#
# extract version
#

my $g_MajorVersion = 0;
my $g_MinorVersion = 0;
my $g_PatchVersion = 0;

open IN, "<$g_VersionFilePath" or die "could not read $g_VersionFilePath: $!";
my @VersionFileContent = <IN>;
close IN or die "could not close $g_VersionFilePath: $!";
chomp @VersionFileContent;

foreach my $line ( @VersionFileContent )
{
  if ( $line =~ /VERSION_MAJOR\s+(\d+)/i )
  {
    $g_MajorVersion = $1;
  }
  elsif ( $line =~ /VERSION_MINOR\s+(\d+)/i )
  {
    $g_MinorVersion = $1;
  }
  elsif ( $line =~ /VERSION_PATCH\s+(\d+)/i )
  {
    $g_PatchVersion = $1;
  }
}
my $g_PrintVersion = "${g_MajorVersion}.${g_MinorVersion}.${g_PatchVersion}";


#
# build installer
#

print( "\n o Building installer for $g_PrintVersion...\n" );

if ( -e $g_OutputFilePath )
{
  system( "del /Q /F \"$g_OutputFilePath\"" );
}

mkpath( $g_OutputDir );

system( "\"$g_ISCCFilePath\" \"$g_ISSFilePath\" /o\"$g_OutputDir\" /f\"$g_OutputBaseFilename\" /d\"_AppVersionMajor=$g_MajorVersion\" /d\"_AppVersionMinor=$g_MinorVersion\" /d\"_AppVersionPatch=$g_PatchVersion\" /d\"_InstallDir=$g_InstallDir\"" );


#
# publish installer
#

if ( $g_Publish && $g_InstallDir )
{
  system ( "del /Q /F \"$g_VersionTxtFilePath\"" ) if ( -e $g_VersionTxtFilePath  );
  if ( open ( OUT, ">$g_VersionTxtFilePath" ) )
  {
    print OUT ( "$g_PrintVersion" );
  }
  close OUT;
  
  my $g_InstallDirVerFolder = $g_InstallDir;
  mkpath( $g_InstallDirVerFolder );

  print( "\n o Publishing to $g_InstallDirVerFolder...\n" );

  my $g_InstallDirFilePath = File::Spec->catfile( $g_InstallDirVerFolder, "$g_OutputBaseFilename.exe" ); 
  system ( "del /Q /F \"$g_InstallDirFilePath\"" ) if ( -e $g_InstallDirFilePath  );
  copy( $g_OutputFilePath, $g_InstallDirFilePath );

  my $g_InstallDirChangelistFilePath = File::Spec->catfile( $g_InstallDirVerFolder, "changelist.txt" );
  system ( "del /Q /F \"$g_InstallDirChangelistFilePath\"" ) if ( -e $g_InstallDirChangelistFilePath  );
  copy( $g_ChangelistFilePath, $g_InstallDirChangelistFilePath );
  
  my $g_InstallDirVersionFilePath = File::Spec->catfile( $g_InstallDirVerFolder, "version.txt" );
  system ( "del /Q /F \"$g_InstallDirVersionFilePath\"" ) if ( -e $g_InstallDirVersionFilePath  );
  copy( $g_VersionTxtFilePath, $g_InstallDirVersionFilePath );
}

exit 0;