# hatariB

A [Libretro](https://www.libretro.com/) core integrating the [Hatari](https://hatari.tuxfamily.org/) emulation of Atari ST, STE, TT, and Falcon computers.

* Stable Build: not yet available
* Nightly Build: [Windows 64-bit hatariB](https://nightly.link/bbbradsmith/hatariB/workflows/win64/main)
* Recent Builds: [Github Actions](https://github.com/bbbradsmith/hatariB/actions)

This is intended as an alternative or replacement for the older [Libretro Hatari Core](https://github.com/libretro/hatari).

This is a work in progress. Once it has had some initial testing, I would like to submit it to Libretro.

Emulator: [Hatari 2.4.1](https://git.tuxfamily.org/hatari/hatari.git/tag/?id=v2.4.1) 2022-08-03

Development notes: [DEVELOP.md](DEVELOP.md)

## Notes

* Hatari Manual:
  * [Hatari Online Manual](https://hatari.tuxfamily.org/doc/manual.html)
* Supported platforms:
  * Windows 64-bit
  * If you'd like to help set up other platforms, please [create a Github issue](https://github.com/bbbradsmith/hatariB/issues) to dicuss it.
* Default controls:
  * Left Stick and D-Pad - Joystick
  * Right Stick - Mouse
  * B - Joystick Fire
  * A - Joystick Autofire
  * Y, X - Mouse Left, Right
  * Select - Select drive A or B
  * Start - Help screen
  * L1 - On-screen keyboard
  * R1 - On-screen keyboard one-shot (pauses emulation, resumes at keypress)
  * L2, R2 - Unassigned
  * L3 - Space key
  * R3 - Return key
  * These can be configured in the core options.
* File formats:
  * Floppy disk: ST, MSA, DIM, STX
  * Muli-disk: M3U, M3U8, ZIP
  * TOS ROM: TOS, IMG, ROM, BIN
  * Cartridge: IMG, ROM, BIN, CART
  * Hard disk: (no standard file extensions)
  * TOS, Cartridge, and Hard disk files should be placed in *system/hatarib/*.
  * When loading multiple disks, the best method is to use M3U playlists to specify all needed disks at once during *Load Content*. Information: [M3U file tutorial](https://docs.retroachievements.org/Multi-Disc-Games-Tutorial/).
  * *Load New Disk* can add additional disks while running, but has several caveats, especially regarding savestates. See below.
* Hard Disks:
  * *GemDOS* type hard disks can select a subdirectory within *system/hatarib/* to use as a simulated drive.
  * A *GemDOS* folder can represent multiple paritions by having its base directory contain only single-letter folder names representing drive letters. *C/*, *D/*, etc.
  * *ASCI*, *SCSI* and *IDE* hard disks use a binary image file chosen from *system/hatarib/*.
  * *Hard disks are read-only by default for safety. This can be disabled in the *System > Hard Disk Write Protect* core option.
* Saving:
  * When a modified floppy disk is ejected, or the core is closed, a modified copy of that disk image will be written to the *saves/* folder.
  * Whenever a new floppy disk is added (*Load Content*, or *Load New Disk*), the saved copy will be substituted for the original if it exists. (Also, if you want to return to the original disk, you can delete it from *saves/*.)
  * If the *System > Save Floppy Disks* core option is disabled, only the original copy of the file will be used, and the *saves/* folder will not be accessed. However, the modified disk images will still persist in memory for the duration of the session, until you close the content.
  * Only the two currently inserted disks are included in savestates. If there are other disks in your loaded collection, they may take on their most recent version from the saves/ folder next time you open this content. A savestate will not be able to restore them to an earlier version.
  * Floppy disks in *saves/* must have a unique filename. Try not to use generic names like *savedisk.st*, and instead include the game title in its filename.
  * If possible, it is recommended that save disks for games use the *ST* format, because it is the simplest and least likely to have errors. You can download a prepared blank disk here: **[blank.st](../../raw/main/blank.st)**.
  * The images written to *saves/* will be standard Atari ST image formats, and you should be able to load them in other emulators if you wish.
  * Note that Hatari is not able to simulate the formatting of a blank disk in-emulator. The [stand-alone version of Hatari](https://hatari.tuxfamily.org/download.html) has a utility in its menu that can be used to create a blank ST file. A different Atari ST emulator [Steem.SSE](https://sourceforge.net/projects/steemsse/) can simulate the formatting process.
  * In the core options *Advanced > Write Protect Floppy Disks* will act as if all inserted disks have their write protect tab open. This means the emulated operating floppy disk will refuse to modify them, and no further data will be written to the disk. This is independent of the save feature, and can be turned on and off at will. Turning it on after a disk is modified will not prevent the previous modifications from being saved.
  * *STX* saves will create a *WD1772* file instead of an *STX* when saved. This is an overlay of changes made to the file, because the STX format itself cannot be re-written. If you wish to use these with the stand-alone Hatari, place the overlay file in the same folder as its STX.
  * *DIM* format disks cannot be saved by Hatari. It is recommended to convert them to *ST* files instead.
  * Hard Disk folders or images in *system/* will be written to directly when they 
* TOS ROMs:
  * The TOS ROM can be chosen in the core option *Sstem > TOS ROM*.
  * The default TOS ROM is *system/tos.img*, but *[EmuTOS 1024k](https://emutos.sourceforge.io/)* is provided as a free substitute.
  * Additional TOS ROMs can be supplied in *system/hatarib/*, up to a limit of 100 files.
  * *EmuTOS* is compatible with most software, but not all. In most incompatibility cases it will show a "Panic" message at boot.
  * *EmuTOS 1024k* is the default, with a full feature set, and univeral support for all emulated machine types.
  * *EmuTOS 192uk* may be slightly more compatible with *ST* software, but provides fewer features. With a colour monitor it starts up in 50hz by default.
  * *EmuTOS 192us* is similar to *192uk* but instead starts in 60hz.
* Accuracy:
  * Some of the default core options are chosen to favour lower CPU usage, and faster load times, but these can be adjusted.
  * *System > Fast Floppy* gives artificially faster disk access, on by default.
  * *System > Patch TOS for Fast Boot* modifies known TOS images to boot faster, on by default.
  * *System > CPU Prefetch Emulation* - Emulates memory prefetch, needed for some games. On by default.
  * *System > Cycle-exact Cache Emulation* - Very accurate cache emulation, not usually needed for compatibility. Off by default.
  * See the *Advanced* category for other relevant options.
* Savestates:
  * Savestates are seamless, allowing run-ahead and netplay in theory, but the restore action is very CPU intensive and may cause stuttering in the live output.
  * *Load New Disk* has several caveats with savesates:
      * RetroArch will change the savestate name to match the newest loaded disk, so be sure that you know that savestates associated with that disk.
      * To restore in a later session, start the core as you did before and use *Load New Disk* to add all needed disks before attempting to restore the savestate. The last disk loaded must be the same as before, so that the savestate name will match correctly.
      * In rare cases, inserting a unusually large new disk may increase the needed savestate size and cause a failure to save. You can eject the disk and try reducing the savestate size before trying again. (RetroArch has a limitation that savestate size must be fixed, determined at Load Content time.)
      * It is generally recommended to use M3U playlists instead of *Load New Disk* when possible ([tutorial](https://docs.retroachievements.org/Multi-Disc-Games-Tutorial/)).
  * Hard Disk modifications are written directly to their source files, and are not included in savestates.
* Quirks:
  * We cannot delete directories in a GemDOS hard disk, because of [a bug in the RetroArch virtual file system](https://github.com/libretro/RetroArch/issues/15578) that affects windows only. This will likely be fixed in the future by an update to RetroArch. There's a working fallback if the VFS isn't provided by the host, but this isn't something easily accessible by the user, and the VFS provides other advantages so it should not be turned off. You can work around this by deleting the folder on your host computer instead.

Remaining tasks before ready for public testing:
* On-screen keyboard.
* Help screen.
* Button-mapped reset.
* Option to automatically cold-reset after crash with a timer.
* Test unicode filenames. Does Libretro expect/convert to UTF-8?
* nvram.c seems to want to load a file, send it to saves? what is it?
* Make sure there isn't a lot of log spam, especially during successful savestate save/reload. Wrap the core_file debug in a define. unstuck, etc.

Optional tasks:
* Investigate Libretro MIDI interface. I wonder if I could play MIDI Maze against my real ST?
* See if a MinGW 32-bit built is reasonable? Might provide a stepping stone to other targets, and additional compile checks.
* Can savestate restore be more lightweight? What takes so much CPU time? Maybe it's disk access?

## History

No releases yet. See nightly builds above for a preview.

## License

This project is licensed under the [GNU General Public License Version 2](LICENSE).

The GPL v2 license was inherited from the Hatari project. The Hatari source code is incorporated here, in the [hatari/](hatari/) folder with minimal modifications, outlined in the [development notes](DEVELOP.md).

This project also incorporates header files from [Libretro](https://github.com/libretro/), under a compatible permissive license.

This project includes [EmuTOS](https://emutos.sourceforge.io/) binaries as built-in available TOS BIOS images, under the GPL v2 license.

## Authors

This Libretro core was begun by [Brad Smith](https://github.com/bbbradsmith).

Authors of incorporated works:
* [Hatari](hatari/doc/authors.txt)
* [EmuTOS](https://raw.githubusercontent.com/emutos/emutos/master/doc/authors.txt)
* [libretro.h](https://github.com/libretro/libretro-common/blob/7edbfaf17baffa1b8a00231762aa7ead809711b5/include/libretro.h)
* [libretro_sdl_keymap.h](https://github.com/libretro/RetroArch/blob/b4143882245edd737c7e7c522b25e32f8d1f64ad/input/input_keymaps.c#L607)
* [miniz](https://github.com/richgel999/miniz/)
