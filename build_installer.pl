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
my $g_ISSFilePath         = 'EShellLauncherSetup.iss';
my $g_OutputBaseFilename  = 'EShellLauncherSetup';
my $g_OutputDir           = 'install';
my $g_VersionFilePath     = 'resource.h';
my $g_ChangelistFilePath  = 'changelist.txt';
my $g_VersionTxtFilePath  = File::Spec->catfile( $g_OutputDir, 'version.txt' );
my $g_OutputFilePath      = File::Spec->catfile( $g_OutputDir, "$g_OutputBaseFilename.exe" );


#
# parse options
#

my $g_Publish;
my $gotOptions = GetOptions
(
  "publish=s" => \$g_Publish,
);

if ( !$gotOptions )
{    
  print STDERR ( "Usage: build_installer -publish=\"path\"\n" );
  exit 1;
}


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
  if ( $line =~ /LAUNCHER_VERSION_MAJOR\s+(\d+)/i )
  {
    $g_MajorVersion = $1;
  }
  elsif ( $line =~ /LAUNCHER_VERSION_MINOR\s+(\d+)/i )
  {
    $g_MinorVersion = $1;
  }
  elsif ( $line =~ /LAUNCHER_VERSION_PATCH\s+(\d+)/i )
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

system( "\"$g_ISCCFilePath\" \"$g_ISSFilePath\" /q /o\"$g_OutputDir\" /f\"$g_OutputBaseFilename\" /d\"_AppVersionMajor=$g_MajorVersion\" /d\"_AppVersionMinor=$g_MinorVersion\" /d\"_AppVersionPatch=$g_PatchVersion\" /d\"_InstallDir=$g_Publish\"" );


#
# publish installer
#

if ( $g_Publish )
{
  system ( "del /Q /F \"$g_VersionTxtFilePath\"" ) if ( -e $g_VersionTxtFilePath  );
  if ( open ( OUT, ">$g_VersionTxtFilePath" ) )
  {
    print OUT ( "$g_PrintVersion" );
  }
  close OUT;
  
  my $g_PublishVerFolder = $g_Publish;
  mkpath( $g_PublishVerFolder );

  print( "\n o Publishing to $g_PublishVerFolder...\n" );

  my $g_PublishFilePath = File::Spec->catfile( $g_PublishVerFolder, "$g_OutputBaseFilename.exe" ); 
  system ( "del /Q /F \"$g_PublishFilePath\"" ) if ( -e $g_PublishFilePath  );
  copy( $g_OutputFilePath, $g_PublishFilePath );

  my $g_PublishChangelistFilePath = File::Spec->catfile( $g_PublishVerFolder, "changelist.txt" );
  system ( "del /Q /F \"$g_PublishChangelistFilePath\"" ) if ( -e $g_PublishChangelistFilePath  );
  copy( $g_ChangelistFilePath, $g_PublishChangelistFilePath );
  
  my $g_PublishVersionFilePath = File::Spec->catfile( $g_PublishVerFolder, "version.txt" );
  system ( "del /Q /F \"$g_PublishVersionFilePath\"" ) if ( -e $g_PublishVersionFilePath  );
  copy( $g_VersionTxtFilePath, $g_PublishVersionFilePath );
}

exit 0;