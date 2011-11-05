#!/usr/bin/perl -w
use strict;
use Data::Dumper;
use File::Basename;
use File::Copy;
use File::Path;
use Getopt::Long 2.36;


# get just the binary name, not the full path to it
my ( $s_ProgramPath, $s_ProgramName ) = $0 =~ /^(.*)[\\\/]+(.*?)$/i;

my $s_ISCCFilePath = File::Spec->catfile( $s_ProgramPath, 'Inno Setup 5/ISCC.exe' );
my $s_ISSFilePath = File::Spec->catfile( $s_ProgramPath, 'LauncherSetup.iss' );

my $s_OutputDir = File::Spec->canonpath( "$s_ProgramPath\\..\\release\\" );
my $s_OutputBaseFilename = "LauncherSetup";
my $s_OutputFilePath = "$s_OutputDir\\$s_OutputBaseFilename.exe";
my $s_ChangelistFilePath = "$s_OutputDir\\changelist.txt";
my $s_VersionTxtFilePath = "$s_OutputDir\\version.txt";

my $s_PublishRoot = "\\\\locutus\\toolshed\\installs\\Tools Launcher";


################################################################################
my $s_Publish = 0;
my $gotOptions = GetOptions
(
  "publish" => \$s_Publish,
);

if ( !$gotOptions )
{    
  print STDERR "Usage: build_installer [-publish]\n";
  exit 1;
}

  
################################################################################
my $s_VersionFilePath = File::Spec->rel2abs( File::Spec->catfile( $s_ProgramPath, '..' ) );
$s_VersionFilePath = File::Spec->catfile( $s_VersionFilePath, 'Version.h' );

my $s_MajorVersion = 0;
my $s_MinorVersion = 0;
my $s_ReleaseVersion = 0;
my $s_BuildVersion = 0;

#define LAUNCHER_VERSION_COMPATIBLE  1
#define LAUNCHER_VERSION_FEATURE     1
#define LAUNCHER_VERSION_PATCH       9

open IN, "<$s_VersionFilePath" or die "could not read $s_VersionFilePath: $!";
my @VersionFileContent = <IN>;
close IN or die "could not close $s_VersionFilePath: $!";
chomp @VersionFileContent;

foreach my $line ( @VersionFileContent )
{
  if ( $line =~ /LAUNCHER_VERSION_COMPATIBLE\s+(\d+)/i )
  {
    $s_MajorVersion = $1;
  }
  elsif ( $line =~ /LAUNCHER_VERSION_FEATURE\s+(\d+)/i )
  {
    $s_MinorVersion = $1;
  }
  elsif ( $line =~ /LAUNCHER_VERSION_PATCH\s+(\d+)/i )
  {
    $s_ReleaseVersion = $1;
  }
  elsif ( $line =~ /LAUNCHER_VERSION_BUILD\s+(\d+)/i )
  {
    $s_BuildVersion = $1;
  }
}
my $s_PrintVersion = "${s_MajorVersion}.${s_MinorVersion}.${s_ReleaseVersion}";

################################################################################
print STDOUT ("Building Launcher's installer...\n");

system( "p4 edit \"$s_OutputFilePath\"" );
system( "del /Q /F \"$s_OutputFilePath\"" );

mkpath( $s_OutputDir );
system( "\"$s_ISCCFilePath\" \"$s_ISSFilePath\" /q /o\"$s_OutputDir\" /f\"$s_OutputBaseFilename\" /d\"IG_AppVersionMayor\"=$s_MajorVersion /d\"IG_AppVersionMinor\"=$s_MinorVersion /d\"IG_AppVersionRelease\"=$s_ReleaseVersion" );

print STDOUT ("  o Created Launcher $s_PrintVersion installer:\n    $s_OutputFilePath...\n");


################################################################################
if ( $s_Publish )
{
  system( "p4 edit \"$s_VersionTxtFilePath\"" );
  system ( "del /Q /F \"$s_VersionTxtFilePath\"" ) if ( -e $s_VersionTxtFilePath  );
  if ( open ( OUT, ">$s_VersionTxtFilePath" ) )
  {
    print OUT "$s_PrintVersion";
  }
  close OUT;
  
  my $s_PublishVerFolder = "$s_PublishRoot\\$s_PrintVersion";  
  mkpath( $s_PublishVerFolder );
  
  my $s_PublishFilePath = "$s_PublishVerFolder\\$s_OutputBaseFilename.exe"; 
  my $s_PublishChangelistFilePath = "$s_PublishVerFolder\\changelist.txt";
  my $s_PublishVersionFilePath = "$s_PublishVerFolder\\version.txt";
  
  print STDOUT ("  o Publishing to:\n   $s_PublishFilePath...\n");
  system ( "del /Q /F \"$s_PublishFilePath\"" ) if ( -e $s_PublishFilePath  );
  copy( $s_OutputFilePath, $s_PublishFilePath );

  system ( "del /Q /F \"$s_PublishChangelistFilePath\"" ) if ( -e $s_PublishChangelistFilePath  );
  copy( $s_ChangelistFilePath, $s_PublishChangelistFilePath );
  
  system ( "del /Q /F \"$s_PublishVersionFilePath\"" ) if ( -e $s_PublishVersionFilePath  );
  copy( $s_VersionTxtFilePath, $s_PublishVersionFilePath );

}

exit 0;

