## Extras
This folder contains extra files, information and links, which might be useful with hatariB, or Atari ST in general (both physical and emulated).

#### Contents
* [Files](#Files)
* [Auto-Run](#Auto-Run)
  * [DESKTOP.INF](#DESKTOP.INF)
  * [GEM Programs](#GEM-Programs)
    * [TOS 1.04+ and EmuTOS](#TOS-1.04+-and-EmuTOS)
	* [TOS 1.00 and 1.02](#TOS-1.00-and-1.02)
  * [Medium Resolution](#Medium-Resolution)
  * [Incompatible Games](#Incompatible-Games)
* [Links](#Links)

### Files

* [**blank.st**](../../../raw/main/extras/blank.st) - A standard format 720k Atari ST floppy disk image. The perfect place for your saved games.
* [**utils.st**](../../../raw/main/extras/utils.st) - A collection of utility programs that may be useful.
  * [**AUTOSORT.PRG**](../../../raw/main/extras/AUTOSORT.PRG) - Sorts the disk order of files in a folder, useful when you need an `AUTO` folder to run programs in a specific order. Version 1.1, Public Domain, 1998. Author: Gene Sothan
  * [**MEDRES.PRG**](../../../raw/main/extras/MEDRES.PRG) - Sets the resolution to medium, intended to be placed into an `AUTO` folder before another program that needs its resolution changed. [Compute Magazine, 1987](https://www.atarimagazines.com/compute/issue86/057_1_Medium-Resolution_Autorun.php). Author: Brian McCorkle
  * [**STARTGEM.PRG**](../../../raw/main/extras/STARTGEM.PRG) - **TOS 1.00 and 1.02 only**: for auto-starting programs that need the GEM window system. See [Auto-Run: GEM Programs](#GEM-Programs) below. Author: *Unknown*
    * [**STARTGEM.DOC**](../../../raw/main/extras/STARTGEM.DOC) - Original ASCII instructions for STARTGEM.

### Auto-Run

A few tips for making Atari ST disks that auto-run a program.

In many cases you can simply create an `AUTO` folder on your floppy disk, and then move the program you want to run at boot into this folder. This works on all versions of TOS, and is usually the most ideal solution. If you want to modify a disk with hatariB you will have to find the modified copy made in your saves folder (see: [Saving](../#Saving)), though it may be more convenient to use the stand-alone version of Hatari, which will save directly to the image.

Some programs are not compatible with the AUTO folder, and may crash, or simply not run. There are alternative solutions to some of these compatibility problems.

Please note that only `.ST` image file or other standard uncompressed image formats can be modified. `.STX` and `.IPF` are preservation formats that are not modifyable.

#### DESKTOP.INF

If the AUTO folder does not work, know that all TOS versions can save a desktop layout that is applied at startup. You simply have to open the folder windows, and arrange them as you would like to see them, and then go to `Options > Save Desktop` to save it to disk.

You can use this to open a window showing exactly the program you want to open in the middle of the screen where it's easy to click on. It also remembers the current resolution option. It may not be auto-run, but it can at least make your startup much quicker.

Some caveats:
  * The original ST TOS saves the desktop to `DESKTOP.INF`, but unfortunately EmuTOS uses its own file `EMUDESK.INF`. To create an image that supports both, you will have to save the desktop twice, once using each TOS.
  * Because EmuTOS does not support DESKTOP.INF, most Atari ST disks intended to boot this way will unfortunately boot with an empty desktop that you have to open manually on EmuTOS.
  * TOS 1.04+ adds an auto-run feature to its DESKTOP.INF (see [below](#TOS-1.04+-and-EmuTOS)), and though this isn't supported by earlier TOS, the saved layout still works in all ST TOS versions, so you can still use this as a fallback for that case.

#### GEM Programs

The AUTO folder programs are executed before the GEM window system starts up. While `.TOS` do not require the GEM window system and should normally run well this way, many `.PRG` programs require the use of GEM. Unfortunately there is no ideal auto-run solution that works for all TOS versions.

##### TOS 1.04+ and EmuTOS
These OS versions support GEM auto-run through their desktop preferences:
  * Select the `.PRG` in the window, then go to `Options > Install Application...`. Select `Auto` boot status. (For EmuTOS also select `Application` for default dir.) Next click `Install` to close the dialog. Finally go to `Options > Save Desktop` to commit this to disk.
  * TOS 1.04+ saves the auto-run information to `DESKTOP.INF`, and EmuTOS saves it to `EMUDESK.INF`. To make auto-run compatible with both, you will have to save both.
  * To remove an auto-run application, return to the `Install Application...` menu and click instead `Remove`, and then `Save Desktop` to write the change to the disk.
  * hatariB can create a virtual auto-run INF file without having to modify the disk image. Using an `.M3U` file with an `#AUTO` statement, the emulator will pretend that your chosen application has already been installed to the INF file. See [M3U Playlists and Auto-Run](../#M3U-Playlists-and-Auto-Run) for more information. (In Hatari this is the command line option `--auto`.)

##### TOS 1.00 and 1.02
These can sometimes use the STARTGEM program (see [Files](#Files) above) to initialize the GEM system before running your chosen program automatically:
  * Create the `AUTO` folder on your floppy disk, and place `STARTGEM.PRG` inside. At boot, this will look for `A:\STARTGEM.INF` which is a text file containing the path of your chosen program. It will then run that program after starting the GEM system for you.
  * To create `STARTGEM.INF`, you can use *EmuTOS 1024k*, go to `File > Execute EmuCon` and type `echo A:\MYFILE.PRG > A:\STARTGEM.INF`. Alternatively if you have a [GemDOS hard disk folder](../#Hard-Disks) configured, you can just make one there with a text editor and then copy the file to your floppy.
  * STARTGEM unfortunately does not work with TOS 1.04+ or EmuTOS. It will boot with a screen saying so.
  * If the program should run in medium resolution, save the desktop with that resolution. This resolution will be automatically applied when it starts GEM.

#### Medium Resolution

Another category of AUTO folder incompatibility is that some non-GEM programs expect to be run from medium resolution, which is normally applied when GEM starts up with the saved `DESKTOP.INF` first. This might be corrected by placing the MEDRES utility (see [Files](#Files) above) in the AUTO folder before you add the program you want to run.

MEDRES was written by Brian McCorkle for Compute! magazine in July 1987. It comes from a great article that explains how the AUTO folder works, and some of its limitations. You can read it here:
  * [Compute! July 1987: Medium-Resolution Autorun](https://www.atarimagazines.com/compute/issue86/057_1_Medium-Resolution_Autorun.php)

AUTO folder files are run in their natural disk order, which is normally the same order that these files were added to the folder.

#### Incompatible Games

Some games will crash, or otherwise not successfully auto-run with any of the methods given above. Your best option may be to [save the Desktop](#DESKTOP.INF) as described above to make opening the game as convenient as you can manage.

If you can't get a game to auto-run, please don't submit a bug report to me until you have already tried the equivalent in [stand-alone Hatari]((https://hatari.tuxfamily.org/download.html)). If they only fail in hatariB, but work in Hatari 2.5.0, then it is an issue that I can try to address.

However, it it fails in Hatari stand-alone it could also just be that the game is incompatible with that auto-run method, and it may not be considered an emulation bug at all. Nevertheless, if you wish to report an issue like this as a bug, please submit it to [Hatari](https://hatari.tuxfamily.org/) directly. Thank you.

### Links

  * [Hatari](https://hatari.tuxfamily.org/) / [User Manual](https://hatari.tuxfamily.org/doc/manual.html) - The emulator that hatariB is built on.
  * [EmuTOS](https://emutos.sourceforge.io/) / [User Manual](https://emutos.github.io/manual/) - The free TOS included with hatariB.
  * [SPS Downloads](http://www.softpres.org/download) - **capsimg 5.1** can be downloaded here, for using **IPF** files. (More info: [Installation](../#Installation))
  * [MUNT MT-32 Emulator](https://sourceforge.net/projects/munt/) - Simulates the Roland MT-32 MIDI sound device.
