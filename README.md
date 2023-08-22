# hatariB

A [Libretro](https://www.libretro.com/) core integrating the [Hatari](https://hatari.tuxfamily.org/) emulation of Atari ST, STE, TT, and Falcon computers.

* Supported platforms:
  * Windows 64-bit
  * Windows 32-bit
  * Ubuntu
  * MacOS
* Current Release:
  * **[hatariB v0.1](https://github.com/bbbradsmith/hatariB/releases/tag/0.1)** - 2023-08-22
* Unstable Build:
  * [Download](https://nightly.link/bbbradsmith/hatariB/workflows/build/main)
* Recent Builds:
  * [Github Actions](https://github.com/bbbradsmith/hatariB/actions)

This is intended as an alternative or replacement for the older [Libretro Hatari Core](https://github.com/libretro/hatari).

This is a new project in the initial testing phase. Eventually I would like to submit it to Libretro.

If you notice any problems or have feedback, please [create a Github issue](https://github.com/bbbradsmith/hatariB/issues).

Emulator: [Hatari 2.4.1](https://git.tuxfamily.org/hatari/hatari.git/tag/?id=v2.4.1) 2022-08-03

Development notes: [DEVELOP.md](DEVELOP.md)

## Installation

The build artifact ZIP files contain a `core/` and `info/` folder. The core folder contains the hatariB core object, and the info folder contains metadata that allows RetroArch to know its associated file types.

On Windows, if using RetroArch's portable *Download* version instead of the installer, you can simply copy the core and info files into the RetroArch folder and it will merge into those folders.

If installed, or on other platforms, you may need to locate the needed core and info folders. You can find these in RetroArch's *Settings > Directories* menu under *Cores* and *Core Info*.

On MacOS, there are some extra requirements:
* After downloading the core, right click on `cores/hatarib.dylib` and open it, here you can give it permission to run.
* On MacOS the cores and info folders are likely at `Users/[username]/Library/Application Support/RetroArch`.

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
    * *Help Screen / Pause* - Pauses emulation and brings up a help screen or other display. You can choose a pause display under *Video > Pause Screen Display*.
    * *Joystick Up* - For those who don't like up-to-jump controls, you can make it a button.
    * *Joystick Down/Left/Right* - The other joystick directions can be made into buttons too.
    * *STE Button A/B/C/Option/Pause* - Buttons for the enhanced STE controller ports.
    * *Mouse Speed Slow/Fast* - Hold to slow or speed the mouse, useful if you have no analog stick.
    * *Soft/Hard Reset* - If you need a button to reset the machine.
    * *Toggle Status Bar* - A quick hide/reveal of the status bar, in case you like it hidden but still want to check it sometimes.
    * *Key Space/Return/Up/Down...* - Any keyboard key can be assigned to a button.
* File formats:
  * Floppy disk: ST, MSA, DIM, STX (can be inside ZIP or GZ)
  * Muli-disk: M3U, M3U8
  * TOS ROM: TOS, IMG, ROM, BIN
  * Cartridge: IMG, ROM, BIN, CART
  * Hard disk: (no standard file extensions)
  * TOS, Cartridge, and Hard disk files should be placed in **system/hatarib/**.
  * When loading multiple disks, the best method is to use M3U playlists to specify all needed disks at once during *Load Content*. Information: [M3U file tutorial](https://docs.retroachievements.org/Multi-Disc-Games-Tutorial/).
  * *Load New Disk* can add additional disks while running, but has several caveats, especially regarding savestates. See below.
  * The first two disks of an M3U list will be loaded into drive A and B at startup, 
  * Libretro only has an interface for one disk drive, but you can use the Select button to switch between whether the Disc Control menu currently shows drive A or drive B.
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
  * If possible, it is recommended that save disks for games use the *ST* format, because it is the simplest and least error prone format. You can download a prepared blank disk here: **[blank.st](../../raw/main/blank.st)**.
  * The images written to *saves/* will be standard Atari ST image formats, and you should be able to load them in other emulators if you wish.
  * Note that Hatari is not able to simulate the formatting of a blank disk in-emulator. The [stand-alone version of Hatari](https://hatari.tuxfamily.org/download.html) has a utility in its menu that can be used to create a blank ST file. A different Atari ST emulator [Steem.SSE](https://sourceforge.net/projects/steemsse/) can simulate the formatting process.
  * In the core options *Advanced > Write Protect Floppy Disks* will act as if all inserted disks have their write protect tab open. This means the emulated operating system will refuse to write any further data to the disk, and will report the error. This is independent of the save feature, and can be turned on and off at will. Turning it on after a disk is modified will not prevent previous modifications from being saved when it is ejected.
  * *STX* saves will create a *WD1772* file instead of an *STX* when saved. This is an overlay of changes made to the file, because the STX format itself cannot be re-written. If you wish to use these with the stand-alone Hatari, place the overlay file in the same folder as its STX.
  * *DIM* format disks cannot be saved by Hatari. It is recommended to convert them to *ST* files instead.
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
* On-Screen Keyboard
  * An on-screen keyboard can be used to simulate using the Atari's keyboard from your gamepad.
  * Press L1 to raise the on-screen keyboard, select a key with the d-pad, and press L1 again to press the key. Press R1 to close the keyboard.
  * Press L2 to raise the keyboard in one-shot mode, which pauses emulation and will resume immediately when you press L1 or R2.
  * To alternate between a top and bottom keyboard position, press X.
  * Modifier keys like Shift, Control, Alt are toggled instead of a single press, allowing you to hold the modifier while you press another key. When you close the keyboard, all modifiers will be released.
  * The keyboard language layout can be chosen in the *Input > On-Screen Keyboard Language* core option.
* MIDI
  * Libretro has a MIDI interface, and if you have MIDI devices installed you should be able to select them in the *Settings > Audio > MIDI* menu of RetroArch.
  * The [MUNT MT-32 Emulator](url=https://sourceforge.net/projects/munt/) is recommended. It can install on your system as a MIDI device, which you can use with MT-32 supporting Atari ST games.
  * MIDI Maze is reported as incompatible for Hatari 2.4.1, but it appears this is being improved for 2.5.0, so perhaps it will eventually be playable over emulated MIDI.
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
  * If you increase the size of memory, you should close and restart the core before using savestates, to allow RetroArch to update the savestate size.
* Quirks:
  * Restoring a savestate, or using netplay/run-ahead into the pause or one-shot keyboard will have an outdated/blank background until unpaused, as Hatari can't rebuild the image until it runs a frame. We might consider fixing this by adding the framebuffer to the savestate, though it would significantly increase the data size.
  * If the on-screen keyboard confirm/cancel buttons aren't mapped to dedicated keys, you might end up suddenly holding the underlying button when the keyboard closes.
  * The *Floppy Disk List* pause screen won't display unicode filenames correctly.
  * You can use *Load New Disk* or M3U playlists to load the same floppy multiple times, or multiple floppies with the same name. This mostly works okay, but a savestate restore might be unable to identify which of them was actually inserted.

Possible Future Tasks:
* Can savestate restore be more lightweight? What takes so much CPU time? Are there any lingering spurious disk accesses? Also, I think netplay efficiency may rely on stable data positions, so double check this to see if structures are moving around from frame to frame (I suspect it only really changes at disk insert/eject).
* Falcon microphone support? Need to find relevant Falcon software to test against.
* RS232 emulation?
* Printer emulation?
* IPF support, figure out library issues. There seems to be a MAME IPF reader?
* Keyboard languages and layouts? Need to know a bit more about how to test this. Think about host keyboard vs. simulated keyboard, etc. EmuTOS can set German and 'Q' will still map to 'Q' in Hatari, though keys like - = etc may change, so probably the on-screen keyboard remaps can still use the same letter semantics at least, but the Atari-specific keys probably just need a new description. (Could possibly need to provide RETROK remap sets too for host keyboard, but not sure. What RETROK do the brackets give on a QWERTZ keyboard, for example? SDL's keyboard system might be good reference, since it's one of the potential RETROK drivers. [tho-otto](https://tho-otto.de/keyboards/) has great diagrams of the Atari keyboards.)
* Do some aspect ratio research and calculation for PIXEL_ASPECT_RATIO in core.c.

## History

* [hatariB v0.1](https://github.com/bbbradsmith/hatariB/releases/tag/0.1) - 2023-08-22
  * First beta test version.

No releases yet. See nightly builds above for a preview.

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
