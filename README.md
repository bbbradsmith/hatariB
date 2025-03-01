# hatariB

A [Libretro](https://www.libretro.com/) core integrating the [Hatari](https://www.hatari-emu.org/) emulation of Atari ST, STE, TT, and Falcon computers.

* Current Build Platforms:
  * Windows 64-bit, 32-bit
  * Ubuntu
  * MacOS 11.0 Apple Arm, 10.13 Intel 64-bit
  * Raspberry Pi OS 64-bit, 32-bit
  * Android 64-bit, 32-bit
* Current Release:
  * **[hatariB v0.3](https://github.com/bbbradsmith/hatariB/releases/tag/0.3)** - 2024-04-15
* Unstable Build:
  * [Download](https://nightly.link/bbbradsmith/hatariB/workflows/build/main)
* Recent Builds:
  * [Github Actions](https://github.com/bbbradsmith/hatariB/actions)

Other platforms may be possible. See [Manual Build](#Manual-Build) below.

This is intended as an alternative or replacement for the older [Libretro Hatari Core](https://github.com/libretro/hatari).

This is a new project in the initial testing phase. Eventually I would like to submit it to Libretro.

**Bugs and Issues:** If you notice any problems or have feedback, please [create a Github issue](https://github.com/bbbradsmith/hatariB/issues). **Before creating an issue, please note:** except for user interface problems, issues are often caused by the underlying emulator Hatari. It would help *a lot* if you could first **test the problem in [Hatari's stand-alone emulator](https://hatari-emu.org/download.html)** to see if the problem exists there. If Hatari does the same thing as hatariB, the bug report should instead go directly to the [Hatari](https://hatari-emu.org/) project. [Atari-Forum's Hatari subforum](https://www.atari-forum.com/viewforum.php?f=51) might be the best place to ask about issues with Hatari.

**Emulator:** [Hatari 2.5.0](https://framagit.org/hatari/hatari/-/releases/v2.5.0) 2024-04-18

**Development notes:** [DEVELOP.md](DEVELOP.md)

**Useful files and information:** [extras/](extras/#Extras)

## Table of Contents

* [Installation](#Installation)
  * [MacOS](#MacOS)
  * [Android](#Android)
  * [Manual Build](#Manual-Build)
* [Notes](#Notes)
  * [Hatari Manual](#Hatari-Manual)
  * [Controls](#Controls)
  * [File formats](#File-Formats)
  * [Hard Disks](#Hard-Disks)
  * [M3U Playlists and Auto-Run](#M3U-Playlists-and-Auto-Run)
  * [Saving](#Saving)
  * [TOS ROMs](#TOS-ROMs)
  * [On-Screen Keyboard](#On-Screen-Keyboard)
  * [MIDI](#MIDI)
  * [Accuracy](#Accuracy)
  * [Savestates](#Savestates)
  * [Netplay](#Netplay)
  * [Unsupported Features](#Unsupported-Features)
  * [Quirks](#Quirks)
* [History](#History)
* [License](#License)
* [Authors](#Authors)

## Installation

The build artifact ZIP files contain a `core/` and `info/` folder. The core folder contains the hatariB core object, and the info folder contains metadata that allows RetroArch to know its associated file types.

On Windows, if using RetroArch's portable *Download* version instead of the installer, you can simply copy the core and info files into the RetroArch folder and it will merge into those folders. If installed, or on other platforms, you may need to locate the needed core and info folders. You can find these in RetroArch's *Settings > Directories* menu under *Cores* and *Core Info*. Similarly the `system/` folder referred to below where you can place your TOS and hard disk image files is actually the RetroArch *System/BIOS* directory.

Depending on the platform, you may also need to add executable permission to the core file (e.g. `chmod +x hatarib.so`). See below for [MacOS instructions](#MacOS).

For *IPF* and *CTR* floppy disk image support, you will also need to provide the **capsimg 5.1** support library, originally created by the [Software Preservation Society](http://www.softpres.org/download). This library will be a file named `capsimg.dll` or `capsimg.so`, depending on your platform. On Windows this DLL should be placed in your RetroArch installation folder next to `retroarch.exe`. On other platforms it must be installed [in your search path for dlopen](https://linux.die.net/man/8/ldconfig), so if it isn't found next to the SO, maybe try `/usr/lib`? An up to date version of capsimg for many platforms can be downloaded here:
* [capsimg 5.1 binaries](https://github.com/rsn8887/capsimg/releases)

If you have trouble getting the builds to work, you can turn on logging under *Settings > Logging*. Log output to a file, restart RetoArch, then try to start the core. After it fails, close RetroArch and find the log. There should hopefully be an error message near the bottom of the log, which might help if we're not unlucky.

### MacOS

Installing for MacOS requires a different method for giving permission:
* On older versions of MacOS, you may be able to right click (or ctrl+click) on `cores/hatarib.dylib` and open it, after which you can give it permission to run.
* On newer versions, you may have to [remove a download quarantine](https://cycling74.com/articles/using-unsigned-max-externals-on-mac-os-10-15-catalina):
  * Open the Terminal utility, navigate to the folder where `hatarib.dylib` was extracted.
  * Type: `xattr -d com.apple.quarantine hatarib.dylib`
  * Type: `chmod +x hatarib.dylib`
  * To make it easier, instead of navigating to the directory, instead of typing `hatarib.dylib` you can drag it into the terminal window, which will automatically paste its full path there.

Once the file is un-quarantined, and given permission to execute, you can copy it to your RetroArch cores folder.
* On MacOS the cores and info folders are usually at: `Users/[username]/Library/Application Support/RetroArch`.

The `capsimg.so` for IPF support can be placed next to the `hatarib.dylib` file, but it must also be given permission in the same way.

### Android

You can use this core with the [latest RetroArch APK](https://www.retroarch.com/?page=platforms), or with the older *RetroArch Plus* build from the Google Play store.

* After setting up RetroArch, place `hatarib.so` in your RetroArch downloads folder. You can use the *Directory* settings to find it, or choose a custom downloads location. Go to *Load Core* and choose *Install or Restore a Core* to install it from the downloads folder. If installed correctly, you should now be able to load it and start it.
* The included `hatarib.info` file allows the core to be associated with content files, but it can be inconvenient to add this to your RetroArch setup. The easiest alternative is to rename the core to `hatari_libretro.so` before installing so that it will instead use the existing info file belonging to the [other Hatari core](https://github.com/libretro/hatari).
* If you want to be more thorough, use the *Directory* settings to relocate *Core Info* to a folder that you can access and place it there. However, I've found that *Update Core Info Files* in the *Online Updater* doesn't work with a custom directory, so I had to manually download and place the info files for other cores there. If you rename the RetroArch APK to be a zip file, you can open it up and find the info files there inside the contained assets folder.
* To delete the core, you can go to *Core > Manage Cores* in the settings.
* It is not yet verified whether IPF files can be made to work by installing `capsimg.so`. I believe the way to do this would be to build your own RetroArch APK from source code, and include it in the `lib` folder inside, but I have not attempted this. It might also be possible to do this after install with a rooted device. Please let me know if you've tried it.
* I would recommend disabling *Input > Host Mouse Enabled* in the core options, as using a touchscreen overlay gamepad will also generate host mouse inputs at the same time. *Retropad 1 > Left Analog Stick* can be set to *Mouse* to allow mouse control that way instead.

### Manual Build

If you want to try building it yourself, in theory it should build on any platform supported by SDL2.

Prerequisite tools: `git`, `make`, `cmake`, `gcc`.

Open a terminal to the directory you want to use, and do the following:
* `git clone https://github.com/bbbradsmith/hatariB.git`
* `cd hatariB`
* `make full`

If successful, this should create `hatariB/build/hatarib.so` (or DLL/dylib).

If unsuccessful, you might look at [build.yml](.github/workflows/build.yml) which runs the Github Actions builds for ideas.

If unsuccessful because the system is out of memory (may look like a ["fatal error" of GCC](https://github.com/bbbradsmith/hatariB/issues/11)), try setting the MULTITHREAD variable to empty:
* `make full MULTITHREAD=`

By default all warnings are enabled and treated as werrors. If unsuccessful because of spurious warnings, try setting the WERROR variable to empty:
* `make full WERROR=`

See [DEVELOP.md](DEVELOP.md) for more details.

## Notes

### Hatari Manual
  * [Hatari Online Manual](https://www.hatari-emu.org/doc/manual.html)
### Controls
  * Left Stick and D-Pad - Joystick
  * Right Stick - Mouse
  * B - Joystick Fire
  * A - Joystick Autofire
  * Y, X - Mouse Left, Right
  * Select - Select drive A or B
  * Start - Help screen
  * L1 - On-screen keyboard
  * R1 - On-screen keyboard one-shot
  * L2, R2 - Mouse Slow, Fast
  * L3 - Space key
  * R3 - Return key
  * On-screen keyboard
    * L1 - Confirm
    * R1 - Cancel
    * X - Toggle Position
  * RetroArch
    * Scroll-Lock - Game Focus mode captures mouse, and full host keyboard input.
    * F11 - Captures or releases the mouse.
  * Keyboard
    * Atari ST special keys (Your Key -> Atari Key)
      * F11 -> Home
      * F12, End -> Undo
      * Page Up -> Numpad (
      * Page Down -> Numpad )
    * Other keys map directly to a standard keyboard, but Game Focus mode (Scroll-Lock) may be neeed to access keys normally assigned to RetroArch hotkeys.
  * Gamepad buttons can be reassigned in the *Retropad* core options:
    * *None* - To disable the button.
    * *Fire* - Joystick fire.
    * *Auto-Fire* - Automatically repeating joystick fire.
    * *Mouse Left/Right* - Mouse buttons.
    * *On-Screen Keyboard* - Raises the on-screen keyboard.
    * *On-Screen Keyboard One-Shot* - Pauses emulation and raises the on-screen keyboard. Resumes when you confirm/cancel a keypress.
    * *Select Floppy Drive* - Selects drive A or B to use with the Libretro disk controls.
    * *Swap to Next Disk* - Ejects the disk in the selected drive and inserts the next one.
    * *Help Screen / Pause* - Pauses emulation and brings up a help screen or other display. You can choose a pause display under *Video > Pause Screen Display*.
    * *Joystick Up* - For those who don't like up-to-jump controls, you can make it a button.
    * *Joystick Down/Left/Right* - The other joystick directions can be made into buttons too.
    * *STE Button A/B/C/Option/Pause* - Buttons for the enhanced STE controller ports.
    * *Mouse Speed Slow/Fast* - Hold to slow or speed the mouse, useful if you have no analog stick.
    * *Soft/Hard Reset* - If you need a button to reset the machine.
    * *CPU Speed* - Switches between 8 MHz, 16 MHz, and 32 MHz CPU speeds.
    * *Toggle Status Bar* - A quick hide/reveal of the status bar, in case you like it hidden but still want to check it sometimes.
    * *Joystick / Mouse Toggle* - Temporarily swaps stick/d-pad assigned to Joystick to Mouse, and vice versa. Also swaps the joystick fire button for mouse left.
    * *Key Space/Return/Up/Down...* - Any keyboard key can be assigned to a button.
  * The help screen mapped to *Start* can be configured to display other information, such as the floppy disk list. See *Video > Pause Screen Display* in the core options.
### File formats
  * Floppy disk: **ST**, **MSA**, **DIM**, **STX**, **IPF**, **CTR** (can be inside **ZIP/ZST** or **GZ**)
  * Hard disk: **ACSI**, **AHD**, **VHD**, **SCSI**, **SHD**, **IDE**, **GEM**
  * Muli-disk: **M3U**, **M3U8**, **ZIP**, **ZST**
  * TOS ROM: **TOS**, **IMG**, **ROM**, **BIN**
  * Cartridge: **IMG**, **ROM**, **BIN**, **CART**
  * TOS, Cartridge, and permanent Hard disk files should be placed in **system/hatarib/**.
  * When loading multiple disks, the best method is to use *M3U* playlists to specify all needed disks at once during *Load Content*. This can also include temporary hard disk images. Information: [M3U file tutorial](https://docs.retroachievements.org/Multi-Disc-Games-Tutorial/).
  * *ZIP* files will load all contained disk images, but if there is an *M3U* or *M3U8* file it will be used to index and load images from the *ZIP*. Hard disk images cannot be used from inside a *ZIP*. Only one *M3U* can be used from inside a *ZIP*.
  * *ZST* is a renamed *ZIP* file. It does the same thing as the *ZIP* but the alternate extension lets you skip the "Load Archive" menu.
  * *GZ* is a single file compressed in the *gzip* format. It can be used to contain any of the floppy disk formats.
  * *Load New Disk* can add additional disks while running, but has several caveats, especially regarding savestates. See [*Savestates*](#Savestates) section below.
  * The first two disks of an M3U list will be loaded into drive A and B at startup, but this can be overridden with `#BOOTA` or `#BOOTB`, see [*M3U* notes](#M3U-playlists-and-Auto-Run) below.
  * Libretro only has an interface for one disk drive, but you can use the *Select* button to switch between whether the Disc Control menu currently shows drive A or drive B.
  * *IPF* and *CTR* formats are only available with the addition of the `capsimg` support libraray. See [Installation](#Installation) for more information.
### Hard Disks
  * A permanent hard disk in your *system/* folder can be configured from the *System* core options menu, but this setting can be overridden by a temporary hard disk loaded from the content menu, or using an *M3U* playlist.
  * *GemDOS* type hard disks can select a subdirectory within *system/hatarib/* to use as a simulated drive.
  * Permanent hard disks:
    * A *GemDOS* folder can represent multiple paritions by having its base directory contain only single-letter folder names representing drive letters. *C/*, *D/*, etc.
    * *ACSI*, *SCSI* and *IDE* hard disks use a binary image file chosen from *system/hatarib/*.
  * Temporary hard disks:
    * You can also load one or more temporary hard disk images directly as content, or through an M3U playlist. The type of hard disk is selected by the filename extension.
    * *ACSI* images use an *ACSI*, *AHD* or *VHD* extension.
    * *SCSI* images use an *SCSI* or *SHD* extension.
    * *IDE* images use an *IDE* extension.
    * *GemDOS* folders use a dummy file with a *GEM* extension. Place this file next to a folder with the same name. The file can be empty, as its contents will not be used.
    * The *GemDOS* and *IDE* hard disk types can be adjusted futher in the core options.
    * An M3U image can load one or more temporary hard disk images. Simply add another line with the name of the hard disk image, after any floppy disks.
  * Hard disks are read-only by default for safety. This can be disabled in the *System > Hard Disk Write Protect* core option. On some Libretro platforms, temporary hard drives may not be writable due to filesystem security settings.
  * Because a hard disk image is not included with a savestate, file writes that are interrupted may cause corruption of the disk image's filesystem.
  * Later TOS versions (or EmuTOS) are recommended when using hard drives, as TOS 1.0 has only limited support for them. Without EmuTOS you may need to use a hard disk driver.
  * Using more than one permanent hard disk image at a time is unsupported, though a single image can have multiple partitions with individual drive letters. An M3U can be used for multiple temporary hard disks.
  * If you need an easy way to switch between permanent hard disk configurations, you could create a "boot" floppy disk to go along with the hard disk, and use *Manage Core Options > Save Game Options* to create a settings override associated with that floppy.
  * See [Hatari's Manual: Hard Disk Support](https://www.hatari-emu.org/doc/manual.html#Hard_disk_support) for further information.
### M3U Playlists and Auto-Run
  * M3U playlists can be used to specify a collection of disk images, accessible via *Disc Control* in the *Quick Menu* of RetroArch.
  * Each line of the M3U is the filename of a disk image, relative to directory of the M3U file.
  * A line starting with `#` will be ignored, unless it is a special command (see below), allowing you to write a comment on the rest of that line, if needed. Blank lines are also ignored.
  * The first 2 disk images will be loaded into drives A and B when the content is opened, but this behaviour can be overridden.
  * A temporary hard disk image can also be listed in the M3U. Hard disks should be listed after any/all floppy disk mages.
  * `#EXTM3U` should normally be the first line of any M3U using the `#` special commands below ([Extended M3U](https://en.wikipedia.org/wiki/M3U#Extended_M3U)) but hatariB does not enforce this requirement.
  * `#BOOTA:number` can be used to select an image from the list to insert into drive A at boot. 1 is the first image in the list. Number 0 will leave the drive empty.
  * `#BOOTB:number` selects the image for drive B at boot.
  * `#AUTO:filename` **TOS 1.04+ only**: Automatically run a TOS file at boot. **Not all games are compatible with this feature** and may crash on boot (eg. [1](https://github.com/bbbradsmith/hatariB/issues/54), [2](https://github.com/bbbradsmith/hatariB/issues/60)). For many games there might be a better solution: see [Auto-Run](extras/#Auto-Run). This feature is the same as [Hatari's --auto](https://www.hatari-emu.org/doc/manual.html#General_options) command line option. Example: `#AUTO:C:\GAMES\JOUST.PRG`
  * `#RES:res` **TOS 1.04+ only**: Automatically set the TOS resolution at boot. Valid values are `low`, `med`, `high`, `ttlow`, `ttmed`. Normally only needed in combination with `#AUTO` for medium resolution games, because it overrides the desktop `INF` file on the disk. This is the same as [Hatari's --tos-res](https://www.hatari-emu.org/doc/manual.html#General_options) command line option. Example: `#RES:med`
  * *Manage Core Options > Save Game Options* can be used to associate other core options with an M3U playlist.
  * **Auto-Run for all TOS versions**: Many games can be auto-started on the ST simply by creating an `AUTO` folder on the floppy disk, and moving the PRG/TOS inside. This is a built-in feature of all versions of TOS, but not all games are compatible. For more information, see: [Auto-Run](extras/#Auto-Run)
### Saving
  * When a modified floppy disk is ejected, or the core is closed, a modified copy of that disk image will be written to the *saves/* folder.
  * Whenever a new floppy disk is added (*Load Content*, or *Load New Disk*), the saved copy will be substituted for the original if it exists. (Also, if you want to return to the original disk, you can delete it from *saves/*.)
  * If the *System > Save Floppy Disks* core option is disabled, only the original copy of the file will be used, and the *saves/* folder will not be accessed. However, the modified disk images will still persist in memory for the duration of the session, until you close the content.
  * Only the two currently inserted disks are included in savestates. If there are other disks in your loaded collection, they may take on their most recent version from the *saves/* folder next time you open this content. A savestate will not be able to restore them to an earlier version.
  * Floppy disks in *saves/* must have a unique filename. Try not to use generic names like *savedisk.st*, and instead include the game title in its filename. (Even if contained within a *ZIP* the save disk filename should be unique.)
  * If possible, it is recommended that save disks for games use the *ST* format, because it is the simplest and least error prone format. You can download a prepared blank disk here: **[blank.st](../../raw/main/extras/blank.st)**.
  * The images written to *saves/* will be standard Atari ST image formats, and you should be able to load them in other emulators if you wish.
  * Note that Hatari is not able to simulate the formatting of a blank disk in-emulator. The [stand-alone version of Hatari](https://www.hatari-emu.org/download.html) has a utility in its menu that can be used to create a blank ST file. A different Atari ST emulator [Steem.SSE](https://sourceforge.net/projects/steemsse/) can simulate the formatting process.
  * In the core options *Advanced > Write Protect Floppy Disks* will act as if all inserted disks have their write protect tab open. This means the emulated operating system will refuse to write any further data to the disk, and will report the error. This is independent of the save feature, and can be turned on and off at will. Turning it on after a disk is modified will not prevent previous modifications from being saved when it is ejected.
  * *STX* saves will create a *WD1772* file instead of an *STX* when saved. This is an overlay of changes made to the file, because the STX format itself cannot be re-written. If you wish to use these with the stand-alone Hatari, place the overlay file in the same folder as its STX.
  * *DIM* format disks cannot be saved by Hatari. It is recommended to convert them to *ST* files instead.
  * *IPF* and *CTR* format disks cannot be saved by Hatari.
  * Hard Disk folders or images in *system/* will be written to directly when they are modified.
  * The TT and Falcon machines have a small non-volatile RAM (NVRAM) that stores system settings. This is saved to **system/hatarib.nvram** when the content is closed.
### TOS ROMs
  * The TOS ROM can be chosen in the core option *Sstem > TOS ROM*.
  * The default TOS ROM is **system/tos.img**, but *[EmuTOS](https://emutos.sourceforge.io/)* is provided built-in as a free substitute.
  * Additional TOS ROMs can be supplied in **system/hatarib/**, up to a limit of 100 files.
  * *EmuTOS* is compatible with most software, but not all. In most incompatibility cases it will show a "Panic" message at boot. Games that do not auto-run will tediously start up in low resolution with all windows closed. See [Quirks](#Quirks) below for information and suggestions about this.
  * *EmuTOS 1024k* has a full feature set, and univeral support for all emulated machine types.
  * *EmuTOS 192uk* may be slightly more compatible with *ST* software, but provides fewer features. With a colour monitor it starts up in 50hz. This was chosen as a default for the greatest compatibility with games.
  * *EmuTOS 192us* is similar to *192uk* but instead starts in 60hz.
  * Most other TOS files are only compatible with certain machines.
  * The `#AUTO` and `#RES` M3U file features can only be used with TOS version 1.04 or higher, or EMUTOS.
  * Recommendations:
    * **TOS 1.00** for best compatibility with preserved Atari ST floppy disk games. There are a small minority of games that only run on 1.00, and an even smaller minority that don't run on 1.00. **European TOS** is generally recommended, since more games were released for 50hz regions than 60hz, though a lot of games will set the ST to their intended framerate anyway, so it usually doesn't matter.
	* **TOS 1.04** for ST games from a hard disk image. 1.04 has much better support for hard disks, and games that were patched for hard disk would normally have any incompatibilities with later TOS patched at the same time.
	* **EmuTOS 1024k** for productivity. It has built-in hard disk drivers, and a lot of features that make dealing with the desktop environment a little easier. It is compatible with most games, too, but not as much as the original TOS versions are.
### On-Screen Keyboard
  * An on-screen keyboard can be used to simulate using the Atari's keyboard from your gamepad.
  * Press *L1* to raise the on-screen keyboard, select a key with the d-pad, and press *L1* again to press the key. Press *R1* to close the keyboard.
  * Press *R1* to raise the keyboard in one-shot mode, which pauses emulation and will resume immediately when you press *L1* to press the key, or *R1* to cancel.
  * To alternate between a top and bottom keyboard position, press *X*.
  * Modifier keys like *Shift*, *Control*, *Alt* are toggled instead of a single press, allowing you to hold the modifier while you press another key. When you close the keyboard, all modifiers will be released.
  * The keyboard language layout can be chosen in the *Input > On-Screen Keyboard Language* core option. This should usually be chosen to match your TOS region.
### MIDI
  * Libretro has a MIDI interface, and if you have MIDI devices installed you should be able to select them in the *Settings > Audio > MIDI* menu of RetroArch.
  * The [MUNT MT-32 Emulator](url=https://sourceforge.net/projects/munt/) is recommended. It can install on your system as a MIDI device, which you can use with MT-32 supporting Atari ST games.
### Accuracy
  * Some of the default core options are chosen to favour faster load times, but these can be adjusted:
    * *System > Fast Floppy* gives artificially faster disk access, on by default.
    * *System > Patch TOS for Fast Boot* modifies known TOS images to boot faster, on by default.
  * Other accuracy options might be adjusted for lower CPU usage:
    * *System > CPU Prefetch Emulation* - Emulates memory prefetch, needed for some games. On by default.
    * *System > Cycle-exact Cache Emulation* - More accurate cache emulation, needed for some games. On by default.
  * See the *Advanced* category for other relevant options.
### Savestates
  * Savestates are seamless, allowing run-ahead and netplay.
  * *Load New Disk* has several caveats with savesates:
      * RetroArch will change the savestate name to match the newest loaded disk, so be sure that you know what savestates are associated with that disk.
      * To restore in a later session, start the core as you did before and use *Load New Disk* to add all needed disks before attempting to restore the savestate. The last disk loaded must be the same as before, so that the savestate name will match correctly.
      * In rare cases, inserting a unusually large new disk may increase the needed savestate size and cause a failure to save. You can eject the disk and try reducing the savestate size before trying again. (RetroArch has a limitation that savestate size must be fixed, determined at *Load Content* time.)
      * It is generally recommended to use M3U playlists instead of *Load New Disk* when possible ([tutorial](https://docs.retroachievements.org/Multi-Disc-Games-Tutorial/)).
  * Hard Disk modifications are written directly to their source files, and are not included in savestates. Try to avoid making savestates during a hard disk write.
  * If you increase the size of the Atari system memory, you should close content and restart the core before using savestates, to allow RetroArch to update the savestate size.
  * For run-ahead or netplay disable *System > Floppy Savestate Safety Save* to prevent high disk activity. When enabled, this option causes any savestate reload to always rewrite a disk to your saves folder if a save file for it already exists here. This helps prevent losing unsaved data when reloading longer term save states, but makes the rapid savestates needed for run-ahead significantly slower.
    * Enabling *Advanced > Write Protect Floppy Disks* will also prevent the safety save feature, as it will not allow the disk to be modified at all.
### Netplay
  * Disable *System > Floppy Savestate Safety Save*, or consider enabling *Advanced > Write Protect Floppy Disks*. See note about [savestates](#Savestates) above.
  * Disable *Input > Host Mouse Enabled* and *Input > Host Keyboard Enabled*, because RetroArch netplay does not send this activity over the network. Instead, use the onscreen keyboard and gamepad to operate the ST keyboard and mouse.
  * RetroArch does not allow analog stick inputs during netplay. The *RetroPad > Joystick / Mouse Toggle* button assignment may be useful for switching between mouse and joystick d-pad input during netplay.
  * Make sure your core options match, especially the TOS image, before attempting to connect.
  * Netplay for hatariB may be more network and CPU intensive than other retro sytems, due to the large onboard RAM of the ST, and volatile floppy disk media.
  * The Libretro API does not provide a way to detect whether netplay is active, so appropriate setting changes like the safety save unfortunately can't be done automatically.
  * The IPF format appears to have drive state that cannot be completely restored, a limitation of the `capsimg` library. Netplay may have problems after disk activity when using IPF disk images, due to savestate divergence.
### Unsupported Features
  * [Printer](https://github.com/bbbradsmith/hatariB/issues/23)
  * [RS232](https://github.com/bbbradsmith/hatariB/issues/22)
  * [Host keyboard remapping](https://github.com/bbbradsmith/hatariB/issues/21)
  * [Falcon microphone](https://github.com/bbbradsmith/hatariB/issues/20)
### Quirks
  * Using EmuTOS, if a game program is not auto-run, it can't read the TOS `DESKTOP.INF` which normally sets the correct resolution and opens a window with the intended program ready to double-click on. If there disk is writable you can set up the resolution/window yourself then go to `Options > Save Desktop` to save `EMUDESK.INF` which will remember the equivalent settings for EmuTOS. Alternatively, you might be able to create an [M3U file with #AUTO](#M3U-Playlists-and-Auto-Run) to automatically start the program.
  * If the on-screen keyboard confirm/cancel buttons aren't mapped to dedicated buttons, you might end up suddenly holding the underlying button when the keyboard closes. (Inputs from buttons mapped to the on-screen keyboard are suppressed while it remains open.)
  * Though the on-screen keyboard is available in [several language layouts](https://tho-otto.de/keyboards/), for your physical keyboard there aren't any direct configuration options, currently. RetroArch ignores the OS keyboard layout, and [all keys report as-if in US layout](https://github.com/libretro/RetroArch/issues/13838) (e.g. German Z reports as RETROK_y). Because of this, if you pick a TOS that matches your keyboard language, the mappings are likely to be mostly compatible. Otherwise, if you need finer control of the mapping, RetroArch's *Input* settings can be used to remap individual keys.
  * The *Floppy Disk List* pause screen can only display a limited subset of unicode characters, due to Hatari's UI font. They can still be viewed through RetroArch's *Disk Control* menu when the selected drive is ejected.
  * There is no way to designate individual floppy disks as read-only. In Hatari this was detected by the host filesystem's read-only attribute on the file, but in Libretro this information is not available. The *Advanced > Write Protect Floppy Disks* provides a coarse way to override this. However, note that any modifications to the file are saved to a copy in the user's *saves/* folder, so the original floppy image is never modified, regardless of setting.
  * You can use *Load New Disk* or *M3U* playlists to load the same floppy multiple times, or multiple floppies with the same name. This mostly works okay, but a savestate restore might be unable to identify which of them was actually inserted.
  * If *IPF* support is enabled, an *M3U* playlist can also be used to load the *RAW* format supported by that library. I kept it out of the associated file types list because I have not yet encountered dumps in this format.
  * The *Advanced > CPU Clock Rate* core option is only applied at boot/reset, but you can *CPU Speed* to a gamepad button in the *RetroPad* core options to change it while running.

## History

* hatariB v0.4 - Unreleased
  * EmuTOS 192uk chosen as default for best compatibility.
  * EmuTOS 1.3 ROMs.
  * Fixed problems with IPF emulation.
  * Hatari 2.5.0 update.
  * SDL2 2.30.2 update.
  * Raspberry Pi builds now have dlopen available for capsimg support.
  * Multi-file ZIP/ZST support, also with M3U playlist inside.
  * Fixed incorrect "Failed to set last used disc..." RetroArch notification.
  * Added support for hard disk images over 2GB in size.
  * M3U can be used to set resolution at boot (TOS 1.04+).
  * Fixed `_stprintf` build issue with MinGW64 compiler update.
  * Strengthen user-requested cold boot, fixes unresponsive emulation after some crashes.
  * Cleanup of core logging code.
* [hatariB v0.3](https://github.com/bbbradsmith/hatariB/releases/tag/0.3) - 2024-04-15
  * On-screen keyboard improvements:
    * Can now hold the key continuously.
    * Auto-repeat for a held direction.
    * Pressing up from space bar now remembers last key instead of always going to Z.
  * Fixed blitter hang when using cycle-accurate cache emulation.
  * Single button disk swap.
  * Savestate optimizations, making run-ahead and netplay viable.
  * Savestate determinism improvements to reduce netplay re-synchronizations.
  * Fix CPU clock rate change on reset.
  * CPU speed button mapping.
  * Joystick / Mouse Toggle button mapping.
  * 4:3 pixel aspect ratio option.
  * Medium resolution vertical-doubling can be disabled.
  * Resolution doubling settings now work for TT and Falcon.
  * Border cropping settings now apply to Falcon.
  * Hard disk images can be loaded as content.
  * M3U can be used to auto-run a program at boot (TOS 1.04+).
  * Fixes to savestates on non-Windows platforms.
  * Attempting to make savestates as cross-platform compatible as possible.
  * Option to disable boot notification.
  * Fixed out-of-date screen image during pause/one-shot and savestate/netplay/run-ahead.
  * Android builds.
  * Multiple hard disk support.
  * MacOS Fat build (Intel 64-bit, Apple Arm).
  * M3U can be used to select disk image or empty drive at boot.
* [hatariB v0.2](https://github.com/bbbradsmith/hatariB/releases/tag/0.2) - 2023-09-07
  * Second beta test version.
  * IPF support via dynamic loading of capsimg library.
  * Raspberry Pi builds.
  * Onscreen keyboard layouts for all known ST languages.
  * Notification messages for hard disk load errors.
  * Correct aspect ratios.
  * Border cropping options.
  * Performance profiling option.
* [hatariB v0.1](https://github.com/bbbradsmith/hatariB/releases/tag/0.1) - 2023-08-22
  * First beta test version.

## License

This project is licensed under the [GNU General Public License Version 2](LICENSE).

The GPL v2 license was inherited from the Hatari project. The Hatari source code is incorporated here, in the [hatari/](hatari/) folder with minimal modifications, outlined in the [development notes](DEVELOP.md).

This project includes [EmuTOS](https://emutos.sourceforge.io/) binaries as built-in available TOS BIOS images, under the GPL v2 license.

This project incorporates header files from [Libretro](https://github.com/libretro/), under a compatible permissive license.

This project includes [SDL2](https://www.libsdl.org/) under the [zlib license](https://www.libsdl.org/license.php).

This project includes [zlib](https://zlib.net/) under the [zlib license](https://zlib.net/zlib_license.html).

## Authors

This Libretro core was begun by [Brad Smith](https://github.com/bbbradsmith).

Other contributors:
* [DJM75](https://github.com/DJM75) - [Android build](https://github.com/bbbradsmith/hatariB/issues/26).

Incorporated works contributor documentation:
* [Hatari](hatari/doc/authors.txt)
* [EmuTOS](https://raw.githubusercontent.com/emutos/emutos/master/doc/authors.txt)
* [libretro.h](https://github.com/libretro/libretro-common/blob/7edbfaf17baffa1b8a00231762aa7ead809711b5/include/libretro.h)
* [libretro_sdl_keymap.h](https://github.com/libretro/RetroArch/blob/b4143882245edd737c7e7c522b25e32f8d1f64ad/input/input_keymaps.c#L607)
* [SDL2](https://www.libsdl.org/credits.php)
* [zlib](https://zlib.net/)

I would also like to acknowledge the prior work of libretro/hatari project:
* [libretro/hatari](https://github.com/libretro/hatari)

Though none of hatariB's code is directly borrowed from libretro/hatari (aside from our mutual use of Hatari), as a past contributor I had learned much from it. As an example to work from and compare against, it was a direct inspiration and source of ideas for hatariB.
