# hatariB

A [Libretro](https://www.libretro.com/) core integrating the [Hatari](https://hatari.tuxfamily.org/) emulation of Atari ST, STE, TT, and Falcon computers.

This is intended as an alternative or replacement for the older [Libretro Hatari Core](https://github.com/libretro/hatari).

This is a work in progress. Once it has had some initial testing, I would like to submit it to Libretro.

Emulator: [Hatari 2.4.1](https://git.tuxfamily.org/hatari/hatari.git/tag/?id=v2.4.1) 2022-08-03

Development notes: [DEVELOP.md](DEVELOP.md)

## Notes

* Currently only builds for Windows 64-bit. If you'd like to help set up other platforms, please create an issue.
* The default TOS ROM is `system/tos.img` but EmuTOS is provided if this file is unavailable.
* Additional TOS ROMs, as well as Cartridge and Hard Disk images can be supplied in `system/hatarib/`, up to a limit of 100 files.
* Load New Disk may not play well with savestates.
  * Inserting a new disk may increase the needed savestate size and cause a failure to save, but you can eject the disk and try again.
  * RetroArch will change the savestate name to match the new loaded disk, as well, so you may have to Load Content with that disk later to be able to load its savesate, and you may need to manually eject and re-insert the disk after reloading.
* Savestates should be seamless, allowing run-ahead and netplay in theory, but the restore action is very CPU intensive and may cause a delay at every rollback.
* Default core options favour lower CPU usage, faster load times. This can be changed in the core options.

Remaining tasks before ready for public testing:
* Support Libretro virtual file system.
* Load M3U playlists.
* Load ZIP files.
* Disks and savestates:
  * Ejecting a modified disk should save it to `saves/` as a replacement for future loads.
  * Hatari savestate size depends on disk image size when it's measured. Try padding up to a minimum size in the serialization code?
  * After restore, core disk files may not match what's in the drive. After loading a savestate, update the core inserted state to match hatari's, and set the indices to match the filenames currently in the drive. (If index becomes invalid, it won't be able to save on eject. Give a warning in this case.)
* Scan system/hatarib/ directory for TOS, Cartridge, and Hard Disk images.
* On-screen keyboard.
* Help screen.
* Add the rest of the keyboard keys to the button mapping config.
* Add reset to button mappings.
* Option to automatically cold-reset after crash with a timer.
* Test unicode filenames. Does Libretro expect/convert to UTF-8?

Optional tasks:
* Investigate Libretro MIDI interface. I wonder if I could play MIDI Maze with my real ST?
* See if I can set up a Mingw32 build.
* Can savestate restore be more lightweight? What takes so much CPU time?

## History

No releases yet.

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
