==========
EShellMenu
==========

EShellMenu is a wxWidgets/Windows system tray application that provides
a frontend to the eshell.pl perl script.  It parses shortcuts from the
ehsell configuration file and populates a context menu with application
icons so users can easy run different programs within an environment
specificed by the eshell configuration file.

EShellMenu includes an installer to make deploying to computers easy.
The installer also packages up a distribution of Strawberry Perl so no
perl environment needs to be installed on the target computer for eshell
or EShellMenu to work.

To get setup:
* clone the repository
* run 'git submodule update --init' to pull down submodules
* run 'build_wx.bat' to build wxWidgets
* build EShellMenu.sln
* create install_dir.txt in the repo root (it should contain the UNC
  path to where EShellMenu should look for updates)
* run 'build_installer.pl' to build the installer (add -publish to have
  it copied to the update location)

-Geoff (AKA gorlak)