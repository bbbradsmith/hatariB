# hatariB

A [Libretro](https://www.libretro.com/) core integrating the [Hatari](https://hatari.tuxfamily.org/) emulation of Atari ST, STE, TT, and Falcon computers.

* Current Build Platforms:
  * Windows 64-bit
  * Windows 32-bit
  * Ubuntu
  * MacOS
  * Raspberry Pi OS 32-bit
  * Raspberry Pi OS 64-bit
* Current Release:
  * **[hatariB v0.2](https://github.com/bbbradsmith/hatariB/releases/tag/0.2)** - 2023-09-07
* Unstable Build:
  * [Download](https://nightly.link/bbbradsmith/hatariB/workflows/build/main)
* Recent Builds:
  * [Github Actions](https://github.com/bbbradsmith/hatariB/actions)

Other platforms may be possible. See [Manual Build](#Manual-Build) below.

This is intended as an alternative or replacement for the older [Libretro Hatari Core](https://github.com/libretro/hatari).

This is a new project in the initial testing phase. Eventually I would like to submit it to Libretro.

If you notice any problems or have feedback, please [create a Github issue](https://github.com/bbbradsmith/hatariB/issues).

Emulator: [Hatari 2.4.1](https://git.tuxfamily.org/hatari/hatari.git/tag/?id=v2.4.1) 2022-08-03

Development notes: [DEVELOP.md](DEVELOP.md)

## Installation

The build artifact ZIP files contain a `core/` and `info/` folder. The core folder contains the hatariB core object, and the info folder contains metadata that allows RetroArch to know its associated file types.

On Windows, if using RetroArch's portable *Download* version instead of the installer, you can simply copy the core and info files into the RetroArch folder and it will merge into those folders. If installed, or on other platforms, you may need to locate the needed core and info folders. You can find these in RetroArch's *Settings > Directories* menu under *Cores* and *Core Info*. Similarly the `system/` folder referred to below where you can place your TOS and hard disk image files is actually the RetroArch *System/BIOS* directory.

Depending on the platform, you may also need to add executable permission to the core file (e.g. `chmod +x hatarib.so`). See below for MacOS instructions.

For *IPF* and *CTR* floppy disk image support, you will also need to provide the **capsimg 5.1** support library, originally created by the [Software Preservation Society](http://www.softpres.org/download). This library will be a file named `capsimg.dll` or `capsimg.so`, depending on your platform. On Windows this DLL should be placed in your RetroArch installation folder next to `retroarch.exe`. On other platforms it must be installed [in your search path for dlopen](https://linux.die.net/man/8/ldconfig), so if it isn't found next to the SO, maybe try `/usr/lib`? An up to date version of capsimg for many platforms can be downloaded here:
* [capsimg 5.1 binaries](https://github.com/rsn8887/capsimg/releases)

If you have trouble getting the builds to work, you can turn on logging under *Settings > Logging*. Log output to a file, restart RetoArch, then try to start the core. After it fails, close RetroArch and find the log. There should hopefully be an error message near the bottom of the log, which might help if we're not unlucky.

### MacOS

Installing for MacOS requires a different method for giving permission:
* After downloading the core, right click on `cores/hatarib.dylib` and open it, here you can give it permission to run.
* On MacOS the cores and info folders are likely at `Users/[username]/Library/Application Support/RetroArch`.

The `capsimg.so` for IPF support can be placed next to the `hatarib.dylib` file, but it also must be given permission, in [a slightly more difficult way](https://cycling74.com/articles/using-unsigned-max-externals-on-mac-os-10-15-catalina):
* Open the Terminal utility.
* You can drag the `capsimg.so` into the terminal to copy its filename path there.
* Use `xattr` to remove the quarantine:
  * `xattr -d com.apple.quarantine Users/[username]/Library/Application\ Support/RetroArch/cores/capsimg.so`

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

* Hatari Manual:
  * [Hatari Online Manual](https://hatari.tuxfamily.org/doc/manual.html)
* Controls:
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
    * L2 - Cancel
    * X - Toggle Position
  * RetroArch
    * Scroll-Lock - Game Focus mode captures mouse, and full host keyboard input.
    * F11 - Captures or releases the mouse.
  * Keyboard
    * Atari ST special keys
      * F11 - Home
      * F12, End - Undo
      * Page Up - Numpad (
      * Page Down - Numpad )
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
* File formats:
  * Floppy disk: ST, MSA, DIM, STX, IPF, CTR (can be inside ZIP or GZ)
  * Muli-disk: M3U, M3U8
  * TOS ROM: TOS, IMG, ROM, BIN
  * Cartridge: IMG, ROM, BIN, CART
  * Hard disk: AHD (ACSI), SHD (SCSI), IDE, GEM (GemDOS), VHD (ACSI)
  * TOS, Cartridge, and Hard disk files should be placed in **system/hatarib/**.
  * When loading multiple disks, the best method is to use M3U playlists to specify all needed disks at once during *Load Content*. Information: [M3U file tutorial](https://docs.retroachievements.org/Multi-Disc-Games-Tutorial/). This can also include a hard disk image.
  * *Load New Disk* can add additional disks while running, but has several caveats, especially regarding savestates. See below.
  * The first two disks of an M3U list will be loaded into drive A and B at startup.
  * Libretro only has an interface for one disk drive, but you can use the Select button to switch between whether the Disc Control menu currently shows drive A or drive B.
  * *IPF* and *CTR* formats are only available with the addition of the `capsimg` support libraray. See [Installation](#Installation) for more information.
  * *ZIP* files will only load the first floppy image file found inside, though the RetroArch *Load Content* menu may be able to select a specific file inside.
* Hard Disks:
  * A permanent hard disk in your *system/* folder can be configured from the *System* core options menu, but this setting can be overridden by a temporary hard disk loaded from the content menu, or using an M3U playlist.
  * *GemDOS* type hard disks can select a subdirectory within *system/hatarib/* to use as a simulated drive.
  * Permanent hard disks:
    * A *GemDOS* folder can represent multiple paritions by having its base directory contain only   single-letter folder names representing drive letters. *C/*, *D/*, etc.
    * *ACSI*, *SCSI* and *IDE* hard disks use a binary image file chosen from *system/hatarib/*.
  * Temporary hard disks:
    * You can also load a temporary hard disk image directly as content, or through an M3U playlist. The type of hard disk is selected by the filename extension.
    * *ACSI* images use an *AHD* or *VHD* extension.
    * *SCSI* images use an *SHD* extension.
    * *IDE* images use an *IDE* extension.
    * *GemDOS* folders use a dummy file with a *GEM* extension. Place this file next to a folder with the same name. The file can be empty, as its contents will not be used.
    * The *GemDOS* and *IDE* hard disk types can be adjusted futher in the core options.
    * An M3U image can load a temporary hard disk image. Simply add another line with the name of the hard disk image, after any floppy disks.
  *   Hard disks are read-only by default for safety. This can be disabled in the *System > Hard Disk Write Protect* core option. On some Libretro platforms, temporary hard drives may not be writable due to filesystem security settings.
  * Because a hard disk image is not included with a savestate, file writes that are interrupted may cause corruption of the disk image's filesystem.
  * Later TOS versions (or EmuTOS) are recommended when using hard drives, as TOS 1.0 has only limited support for them. Without EmuTOS you may need to use a hard disk driver.
  * Using more than one hard disk image at a time is unsupported, though a single image can have multiple partitions with individual drive letters.
  * If you need an easy way to switch between permanent hard disk configurations, you could create a "boot" floppy disk to go along with the hard disk, and use *Manage Core Options > Save Game Options* to create a settings override associated with that floppy.
  * See [Hatari's Manual: Hard Disk Support](https://hatari.tuxfamily.org/doc/manual.html#Hard_disk_support) for further information.
* M3U playlists and Auto-Run:
  * M3U playlists can be used to specify a collection of disk images, accessible via *Disc Control* in the *Quick Menu* of RetroArch.
  * Each line of the M3U is the filename of a disk image, relative to the M3U file.
  * A line starting with `#` will normally be ignored, allowing you to write a comment on the rest of that line, if needed.
  * The first 2 disk images will be loaded into drives A and B when the content is opened.
  * A temporary hard disk image can also be listed in the M3U. Place this line after any floppy disk mages.
  * `#AUTO:filename` can be used to automatically run a TOS file at boot. This is the same as [Hatari's --auto command line option](https://hatari.tuxfamily.org/doc/manual.html#General_options). **TOS 1.04** or later is required to use this feature. Example: `#AUTO:C:\GAMES\JOUST.PRG`
  * If using the `#AUTO` feature, technically `#EXTM3U` should be added as the first line of the M3U to indicate this is an [Extended M3U](https://en.wikipedia.org/wiki/M3U#Extended_M3U), but hatariB will not enforce this.
  * *Manage Core Options > Save Game Options* can be used to associate other core options with an M3U playlist.
* Saving:
  * When a modified floppy disk is ejected, or the core is closed, a modified copy of that disk image will be written to the *saves/* folder.
  * Whenever a new floppy disk is added (*Load Content*, or *Load New Disk*), the saved copy will be substituted for the original if it exists. (Also, if you want to return to the original disk, you can delete it from *saves/*.)
  * If the *System > Save Floppy Disks* core option is disabled, only the original copy of the file will be used, and the *saves/* folder will not be accessed. However, the modified disk images will still persist in memory for the duration of the session, until you close the content.
  * Only the two currently inserted disks are included in savestates. If there are other disks in your loaded collection, they may take on their most recent version from the *saves/* folder next time you open this content. A savestate will not be able to restore them to an earlier version.
  * Floppy disks in *saves/* must have a unique filename. Try not to use generic names like *savedisk.st*, and instead include the game title in its filename.
  * If possible, it is recommended that save disks for games use the *ST* format, because it is the simplest and least error prone format. You can download a prepared blank disk here: **[blank.st](../../raw/main/blank.st)**.
  * The images written to *saves/* will be standard Atari ST image formats, and you should be able to load them in other emulators if you wish.
  * Note that Hatari is not able to simulate the formatting of a blank disk in-emulator. The [stand-alone version of Hatari](https://hatari.tuxfamily.org/download.html) has a utility in its menu that can be used to create a blank ST file. A different Atari ST emulator [Steem.SSE](https://sourceforge.net/projects/steemsse/) can simulate the formatting process.
  * In the core options *Advanced > Write Protect Floppy Disks* will act as if all inserted disks have their write protect tab open. This means the emulated operating system will refuse to write any further data to the disk, and will report the error. This is independent of the save feature, and can be turned on and off at will. Turning it on after a disk is modified will not prevent previous modifications from being saved when it is ejected.
  * *STX* saves will create a *WD1772* file instead of an *STX* when saved. This is an overlay of changes made to the file, because the STX format itself cannot be re-written. If you wish to use these with the stand-alone Hatari, place the overlay file in the same folder as its STX.
  * *DIM* format disks cannot be saved by Hatari. It is recommended to convert them to *ST* files instead.
  * *IPF* and *CTR* format disks cannot be saved by Hatari.
  * Hard Disk folders or images in *system/* will be written to directly when they are modified.
  * The TT and Falcon machines have a small non-volatile RAM (NVRAM) that stores system settings. This is saved to **system/hatarib.nvram** when the content is closed.
* TOS ROMs:
  * The TOS ROM can be chosen in the core option *Sstem > TOS ROM*.
  * The default TOS ROM is **system/tos.img**, but *[EmuTOS 1024k](https://emutos.sourceforge.io/)* is provided as a free substitute.
  * Additional TOS ROMs can be supplied in **system/hatarib/**, up to a limit of 100 files.
  * *EmuTOS* is compatible with most software, but not all. In most incompatibility cases it will show a "Panic" message at boot.
  * *EmuTOS 1024k* is the default, with a full feature set, and univeral support for all emulated machine types.
  * *EmuTOS 192uk* may be slightly more compatible with *ST* software, but provides fewer features. With a colour monitor it starts up in 50hz by default.
  * *EmuTOS 192us* is similar to *192uk* but instead starts in 60hz.
  * Most other TOS files are only compatible with certain machines.
* On-Screen Keyboard:
  * An on-screen keyboard can be used to simulate using the Atari's keyboard from your gamepad.
  * Press L1 to raise the on-screen keyboard, select a key with the d-pad, and press L1 again to press the key. Press R1 to close the keyboard.
  * Press L2 to raise the keyboard in one-shot mode, which pauses emulation and will resume immediately when you press L1 or R2.
  * To alternate between a top and bottom keyboard position, press X.
  * Modifier keys like Shift, Control, Alt are toggled instead of a single press, allowing you to hold the modifier while you press another key. When you close the keyboard, all modifiers will be released.
  * The keyboard language layout can be chosen in the *Input > On-Screen Keyboard Language* core option. This should usually be chosen to match your TOS region.
* MIDI:
  * Libretro has a MIDI interface, and if you have MIDI devices installed you should be able to select them in the *Settings > Audio > MIDI* menu of RetroArch.
  * The [MUNT MT-32 Emulator](url=https://sourceforge.net/projects/munt/) is recommended. It can install on your system as a MIDI device, which you can use with MT-32 supporting Atari ST games.
  * MIDI Maze is reported as incompatible for Hatari 2.4.1, but it appears this is being improved for 2.5.0, so perhaps it will eventually be playable over emulated MIDI.
* Accuracy:
  * Some of the default core options are chosen to favour faster load times, but these can be adjusted:
    * *System > Fast Floppy* gives artificially faster disk access, on by default.
    * *System > Patch TOS for Fast Boot* modifies known TOS images to boot faster, on by default.
  * Other accuracy options might be adjusted for lower CPU usage:
    * *System > CPU Prefetch Emulation* - Emulates memory prefetch, needed for some games. On by default.
    * *System > Cycle-exact Cache Emulation* - More accurate cache emulation, needed for some games. On by default.
  * See the *Advanced* category for other relevant options.
* Savestates:
  * Savestates are seamless, allowing run-ahead and netplay.
  * *Load New Disk* has several caveats with savesates:
      * RetroArch will change the savestate name to match the newest loaded disk, so be sure that you know that savestates associated with that disk.
      * To restore in a later session, start the core as you did before and use *Load New Disk* to add all needed disks before attempting to restore the savestate. The last disk loaded must be the same as before, so that the savestate name will match correctly.
      * In rare cases, inserting a unusually large new disk may increase the needed savestate size and cause a failure to save. You can eject the disk and try reducing the savestate size before trying again. (RetroArch has a limitation that savestate size must be fixed, determined at Load Content time.)
      * It is generally recommended to use M3U playlists instead of *Load New Disk* when possible ([tutorial](https://docs.retroachievements.org/Multi-Disc-Games-Tutorial/)).
  * Hard Disk modifications are written directly to their source files, and are not included in savestates. Try to avoid making savestates during a hard disk write.
  * If you increase the size of memory, you should close content and restart the core before using savestates, to allow RetroArch to update the savestate size.
  * For run-ahead or netplay disable *System > Floppy Savestate Safety Save* to prevent high disk activity.
* Netplay:
  * Disable *System > Floppy Savestate Safety Save* to prevent unnecessary disk activity.
  * Disable *Input > Host Mouse Enabled* and *Input > Host Keyboard Enabled*, because RetroArch netplay does not send this activity over the network. Instead, use the onscreen keyboard and gamepad to operate the ST keyboard and mouse.
  * RetroArch does not allow analog stick inputs during netplay. The *RetroPad > Joystick / Mouse Toggle* button assignment may be useful for switching between mouse and joystick d-pad input during netplay.
  * Make sure your core options match, especially the TOS image, before attempting to connect.
  * The IPF format appears to have drive state that cannot be completely restored. Netplay may stutter during activity if using IPF disk images due to savestate divergence.
* Quirks:
  * Restoring a savestate, or using netplay/run-ahead into the pause or one-shot keyboard will have an outdated/blank background until unpaused, as Hatari can't rebuild the image until it runs a frame. We could consider adding the framebuffer to the savestate to prevent this, though it would significantly increase the data size.
  * If the on-screen keyboard confirm/cancel buttons aren't mapped to dedicated buttons, you might end up suddenly holding the underlying button when the keyboard closes. (Inputs from buttons mapped to the on-screen keyboard are suppressed while it remains open.)
  * Though the on-screen keyboard is available in [several language layouts](https://tho-otto.de/keyboards/), for your physical keyboard there aren't any direct configuration options, currently. RetroArch ignores the OS keyboard layout, and [all keys report as-if in US layout](https://github.com/libretro/RetroArch/issues/13838) (e.g. German Z reports as RETROK_y). Because of this, if you pick a TOS that matches your keyboard language, the mappings are likely to be mostly compatible. Otherwise, if you need finer control of the mapping, RetroArch's *Input* settings can be used to remap individual keys.
  * The *Floppy Disk List* pause screen won't display unicode filenames correctly, though they can still be viewed through RetroArch's *Disk Control* menu when the selected drive is ejected.
  * There is no way to designate individual floppy disks as read-only. In Hatari this was detected by the host filesystem's read-only attribute on the file, but in Libretro this information is not available. The *Advanced > Write Protect Floppy Disks* provides a coarse way to override this. However, note that any modifications to the file are saved to a copy in the user's *saves/* folder, so the original floppy image is never modified, regardless of setting.
  * You can use *Load New Disk* or *M3U* playlists to load the same floppy multiple times, or multiple floppies with the same name. This mostly works okay, but a savestate restore might be unable to identify which of them was actually inserted.
  * If *IPF* support is enabled, an *M3U* playlist can also be used to load the *RAW* format supported by that library. I kept it out of the associated file types list because I have not yet encountered dumps in this format.
  * The *Advanced > CPU Clock Rate* core option is only applied at boot/reset, but you can *CPU Speed* to a gamepad button in the *RetroPad* core options to change it while running.

## History

* hatariB v0.3 - Unreleased
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
  * M3U can be used to auto-run a program at boot.
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

Authors of incorporated works:
* [Hatari](hatari/doc/authors.txt)
* [EmuTOS](https://raw.githubusercontent.com/emutos/emutos/master/doc/authors.txt)
* [libretro.h](https://github.com/libretro/libretro-common/blob/7edbfaf17baffa1b8a00231762aa7ead809711b5/include/libretro.h)
* [libretro_sdl_keymap.h](https://github.com/libretro/RetroArch/blob/b4143882245edd737c7e7c522b25e32f8d1f64ad/input/input_keymaps.c#L607)
* [SDL2](https://www.libsdl.org/credits.php)
* [zlib](https://zlib.net/)

I would also like to acknowledge the prior work of libretro/hatari project:
* [libretro/hatari](https://github.com/libretro/hatari)

Though none of hatariB's code is directly borrowed from libretro/hatari (aside from our mutual use of Hatari), as a past contributor I had learned much from it. As an example to work from and compare against, it was a direct inspiration and source of ideas for hatariB.
