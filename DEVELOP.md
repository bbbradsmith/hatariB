# hatariB Development Notes

This document will contain build setup instructions, notes on the development of the Libretro core, notes on the integration of Hatari, and notes on the modification of Hatari for this project.

This is intended as reference documentation for future maintenance of this project.

Incorporated sources:

* [Hatari/](https://git.tuxfamily.org/hatari/hatari.git/tag/?id=v2.4.1) 2.4.1 2022-08-03
* [EmuTOS/](https://emutos.sourceforge.io/) 1.2.1 2022-08-16
* [libretro/libretro.h](https://github.com/libretro/libretro-common/blob/7edbfaf17baffa1b8a00231762aa7ead809711b5/include/libretro.h) 24a9210 2023-07-16
* [libretro/libretro_sdl_keymap.h](https://github.com/libretro/RetroArch/blob/b4143882245edd737c7e7c522b25e32f8d1f64ad/input/input_keymaps.c#L607) 9ca5c5e 2023-07-08
* [SDL2](https://github.com/libsdl-org/SDL/releases/tag/release-2.28.2) 2.28.2 2023-08-02
* [zlib](https://github.com/madler/zlib/releases/tag/v1.3) 1.3 2023-08-18

## Build

This has been built and tested with MSYS2 UCRT64. The following packages are required:

* git
* make
* gcc (mingw-w64-ucrt-x86_64-gcc)
* cmake (mingw-w64-ucrt-x86_64-cmake)

Before running make to build hatariB, build the static libraries for zlib and SDL2:
* `make -f makefile.zlib`
* `make -f makefile.sdl`

With the static libraries prepared, the default make target will create `build/hatarib.dll`:
* `make`

Other targets:
* `make clean` - removes the build and all temporary files to start fresh.
* `make full` - cleans and rebuilds both the static libraries and hatariB.
* `make sdlreconfig` - for testing SDL2 configuration changes: cleans and rebuilds SDL2, then incrementally builds hatariB.
* `make zlib` - shorthand for `make -f makefile.zlib`
* `make sdl` - shorthand for `make -f makefile.sdl`

By default SDL ant hatariB are built with the `-j` option to multithread the build process. You can disable this by adding `MULTITHREAD=` to the command line. This may be needed if the system runs out of memory, or otherwise can't handle the threading.

## Changes to Hatari

Changes to the C source code are all contained in `__LIBRETRO__` defines. This is important for merging future versions, because we always be able to see what code was replaced. None of the original code has been modified or deleted, only _disabled_ within `#ifdef` blocks.

Otherwise there are minor changes to the CMake build files, each marked with a comment that contains: `# hatariB`

* **hatari/CMakeLists.txt**
  * `include(CheckSymbolExists)` - this was somehow implicitly included normally, but some modifications required it to be explicitly included.
  * Disabled `DALERT_HOOKS` for MacOS build which produced unwanted MacOS native GUI dependencies.
  * Disabled `HAVE_STATVFS` because we use Libretro's own virtual file system instead.
  * Disabled `tests` subdirectory, because we do not build the executable needed for these tests.
  * Disabled `FindPythonInterp`, because we don't need the python interpreter or GTK, which are used for Hatari's debugger.
  * Add `HAVE_DLOPEN` to use for dynamic loading of CAPSIMAGE library for IPF support.
* **hatari/src/CMakeLists.txt**
  * Removed platform-specific GUI dependencies.
  * Removed stand-alone executable build.
  * Added `core` library target to be linked with our Libretro core instead of a stand-alone executable.
* **hatari/tools/CMakeLists.txt**
  * Removed `hmsa` support tool build, which is unnecessary but also can't link with our changes to the `floppy` library.
* **makefile** - Our makefile also controls Hatari's cmake in the following ways:
  * Exports `CFLAGS` for global compiler settings.
    * `__LIBRETRO__` define provides our primary means to contain our C code alterations.
    * `-fPIC` produces relocatable code necessary for a shared-object.
  * `CMAKEFLAGS` provides some command-line control to Hatari's cmake:
    * Provide our static SDL2 and zlib libraries.
    * Disable `Readline`, `X11`, `PNG`, `PortMidi` and `CapsImage` library dependencies.
    * `ENABLE_SMALL_MEM=0` allocates 16MB always instead of ST configured memory size (typically 1MB), allowing speed optimizations. If building for a platform with very limited RAM we might reconsider this.
  * `CMAKEBUILDFLAGS` provides -j` allowing faster parallel builds.
  * `DEBUG=1` adds `ENABLE_TRACING=1` which can log an emulation trace, sometimes useful when debugging behaviour, especially savestate consistency, but at significant CPU overhead.
  * `VERBOSE_CMAKE=1` will show all the cmake build steps, `=2` will also show the cmake configuration trace.

* **hatari/src/acia.c**
  * Suppress savestate pointer data to prevent divergence.
* **hatari/src/audio.c**
  * Disable all use of SDL audio system.
  * `Audio_SetOutputAudioFreq` calls `core_set_samplerate` to notify the core of the current samplerate.
  * Disable automatic lowpass-filter selection (see: sound.c).
* **hatari/src/cart.c**
  * Use core's file system to load cartridge ROM.
* **hatari/src/change.c**
  * New reset cases for added configuration changes (EmuTOS, low resolution doubling).
  * NVRAM is no longer accessed unless using TT or Falcon machines, so it has a new reset case here.
  * Disable alert dialog for reset due to changes.
* **hatari/src/configuration.c**
* **hatari/src/includes/configuration.h**
  * New settings:
    * Built-in EmuTOS with region/framerate override option.
    * Sound highpass and lowpass filter choice.
    * Low resolution doubling.
    * CPU frequency at-boot. This copies to the existing `nCpuFreq` setting during a reset. Hatari expects to change `nCpuFreq` directly in its configuration while running, so it reflects the immediate live state of the CPU, but Libretro options are instead only changed by the user. This provides a way for the user to give a setting without conflicting with Hatari's direct usage.
    * Remove `SDL_NumJoysticks`.
    * Replace some default settings:
      * `Screen.nFrameSkips = 0` - Libretro does not allow frameskips, every frame must be fully realized.
      * `Sound.nPlaybackFreq = 48000` - Use the current standard audio frequency by default.
      * `DiskImage.FastFloppy = true` - Artificially fast floppy disk loading so the user spends less time waiting.
      * `System.bFastBoot = true` - Patches known TOS ROMs to boot more quickly.
      * `System.bCycleExactCpu = false` - Cycle exact caching adds significant CPU overhead, only very rarely needed for accuracy.
      * `HardDisk.nWriteProtection = WRITEPROT_ON` - Hard Disk should be write protected by default. The user's file system should not be modifiable unless deliberately requested.
      * `Midi.bEnableMidi = true` - Allows MIDI signals to be sent through Libretro's MIDI interface.
    * Remove `File_MakeAbsoluteSpecialName` path conversions, which modify the paths we provide directly. Since all file access is through our core's file system, absolute paths are inappropriate. This also prevents Hatari from making modifications to the paths which might have caused a reset check, disk re-insertion, etc. on options change.
  * Saved `MachineClocks` (from clocks_timings.c) to prevent state divergence.
* **hatari/src/crossbar.c**
  * Removed `Crossbar_Recalculate_Clocks_Cycles()` from savestate restore because it seemed to be unnecessary and caused state divergence.
* **hatari/src/cycInt.c**
  * Save `CycInt_From_Opcode` to prevent state divergence.
* **hatari/src/cycles.c**
  * Update counters before save or restore of state to prevent divergence.
* **hatari/src/dialog.c**
  * Disable `Dialog_DoProperty`.
* **hatari/src/dim.c**
  * Use core's file system to load floppy image.
  * Error notification for attempting to save DIM image (unsupported).
* **hatari/src/file.c**
  **hatari/src/include/file.h**
  * `File_QueryOverwrite` always returns true instead of checking a file. In all instances where this is used (savestates, floppy saves) we are using the core's file system and don't need to ensure this (usually writing to memory instead of a file when this is checked).
  * Provide extern access to core file system in header.
* **hatari/src/floppy.c**
  **hatari/src/include/floppy.h**
  * `Floppy_IsWriteProtected` formerly checked the file on disk's write-protect state. This is not available from the Libretro virtual filesystem, so we cannot use this information. Assuming all disks are not write protected. Can use core options to write protect the drives manually, but we lack a per-disk-image setting. However, since we do not save back to the original floppy file, there is less of a need for this.
  * Provide extern access to core file system in header.
* **hatari/src/floppy_ipf.c**
  * Use core's file system to load floppy image.
  * Convert implicit capsimg linking to one loaded at runtime if available.
  * Suppress savestate pointer data to prevent divergence.
* **hatari/src/floppy_stx.c**
  * Use core's file system to load floppy image.
  * Use core's file system to save floppy overlay image.
  * Suppress Hatari's warning that STX saves to an overlay instead of the image file, since we never save back to the original floppy image files.
  * Suppress savestate pointer data to prevent divergence.
* **hatari/src/gemdos.c**
  * Use core's file system to provide folder hard disk support.
  * Provide `core_scandir_system` as a simplified replacement for `scandir` using what is available through the virtual file system.
  * Disable use of stdout/stderr as internal file handles (only needed for a TOS-less test mode).
  * Disable use of chmod (not available through virtual file system). The emulated TOS will not be able to modify file permissions directly.
* **hatari/src/hdc.c**
* **hatari/src/includes/hdc.h**
  * Use core's file system to provide ACSI/SCSI image hard disk support.
  * Replace `FILE` with `corefile`.
  * Unused variable warning suppression for `ENABLE_TRACING`.
* **hatari/src/ide.c**
  * Use core's file system to provide IDE image hard disk support.
  * File locking is not directly provided by the virtual file system (though the host OS might do it automatically).
* **hatari/src/ioMem.c**
  * Saved `IoAccessInstrPrevClock` and `IoAccessInstrCount` to prevent state divergence.
* **hatari/src/infile.c**
* **hatari/src/includes/infile.c**
  * Use core's file system to provide INF-file support for GEMDOS hard drives.
  * Replace `FILE` with `corefile`.
* **hatari/src/joy.c**
  * Disable SDL joystick system use.
  * Assume 4 attached joysticks, named "Retropad", and poll input from core instead of SDL.
* **hatari/src/keymap.c**
  * Disable handling of `SDLK_SCROLLLOCK`, which conflicts with game focus key (and isn't on ST keyboards).
  * Disable dispatch of keypresses to `ShortCut` system (Hatari's own GUI hotkeys).
  * Disable using Num Lock to remap Numpad. (ST has no Num Lock. Numpad is Numpad.)
  * Disable use of `SDL_GetKeyFromName`/`SDL_GetKeyName`, only needed by configuration GUI.
  * Fix broken mappings for minus (`- _`), bracket (`[ { ] }`), and backquote/tilde (~) keys. Also [submitted to Hatari](https://github.com/hatari/hatari/pull/26).
* **hatari/src/m68000.c**
  * Saved `WaitStateCycles`,`BusMode`,`CPU_IACK`,`LastInstrCycles`,`Pairing` to prevent state divergence.
  * Saved `currcycle` and `extra_cycle` from cpu/custom.c to prevent state divergence.
  * Saved `BusCyclePenalty` from cpu/newcpu.c to prevent state divergence.
* **hatari/src/main.c**
* **hatari/src/includes/main.h**
  * Disable `SDL_GetTicks` timer.
  * Disable SDL mouse state changes.
  * Disable dialog box prompt for quit.
  * `Main_WaitOnVbl` replaced measured time with assumption that every frame takes exactly its expected time.
  * `Main_WaitOnVbl` use `M68000_SetSpecial(SPCFLAG_BRK)` to signal emulation loop to exit and return to Libretro host at the end of each frame.
  * Replace SDL event polling with `core_poll_event` which only provides mouse and keyboard input.
  * Disable handling of other events besides mouse and keyboard (window, etc.).
  * Disable `SDL_SetWindowTitle`.
  * Disable `SDL_Init`, `SDL_Quit`.
  * Disable TOS load failure dialog.
  * `Main_LoadInitialConfig` replace configuration file load with `core_config_init`.
  * `Main_StatusbarSetup` replace initial welcome message on status bar with a message to press START for help.
  * Disable `Win_OpenCon` and `setenv` uses.
  * `main` split into `main_init` and `main_deinit` so that we can begin emulation, but return when ready to start simulating frames.
    * `main_init` does the initialization, and starts the main emulation loop `M68000_Start` (see: m68000.c, cpu/newcpu.c) which is also modified to exit before the first frame is emulated, instead of entering a loop until the user quits.
    * `main_deinit` is some final shutdown after the main emulation loop exits.
  * Defer `IPF_Init` until first use, allowing it to be dynamically loaded only if needed and available.
  * Include `core.h` in main header to provide global extern access to some core functions.
  * Disable log output requirement.
* **hatari/src/memorySnapShot.c**
  * Disable compression of savestate data, Libretro does its own compression for save to disk, but also needs an uncompressed form for run-ahead or netplay to work.
  * Instead of saving to a file, write to a provided memory buffer.
  * Suppress error dialogs and alerts.
  * Suppress saving `DebugUI` information.
  * Add `LIBRETRO_DEBUG_SNAPSHOT` macro to debug snapshot memory regions.
* **hatari/src/mfp.c**
  * Save `PendingCyclesOver` to prevent state divergence.
* **hatari/src/midi.c**
  * Connect MIDI read and write to the core's MIDI interface, assume the host device is always open/available from Hatari's perspective.
* **hatari/src/msa.c**
  * Use core's file system to load and save floppy image.
* **hatari/src/ncr5380.c**
  * Use core's file system to provide SCSI image hard disk support.
* **hatari/src/paths.c**
  * Disable attempt to read host's working directory for absolute paths, replace any attempt with "<nopath>".
* **hatari/src/reset.c**
  * Notify core when the system is reset.
* **hatari/src/resolution.c**
  * Disable `SDL_GetDesktopDisplayMode` and assume the desktop is the size we need.
* **hatari/src/scandir.c**
  * Remove scandir implementations enabled by !HAVE_SCANDIR, because they are unused, but also to suppress a warning-as-error on size_t vs. int mismatch.
* **hatari/src/screen.c**
  * Disable SDL rendering, reduce use of SDL to merely creating a software render SDL_Surface which can be used by the gui-sdl system to render the status bar and onscreen keyboard.
  * Replace `SDL_RenderPresent` with `core_video_update` to deliver the new frame buffer.
  * Provide Libretro's 3 available pixel formats.
  * Implement option to use doubled pixels for low resolution.
  * Use palette 0 to clear the screen after mode changes, because it looks more natural than black. (Needed if the resolution changes while emulation is paused.)
  * Provide border cropping options.
* **hatari/src/screenSnapShot.c**
  * Disable `SDL_SaveBMP`.
* **hatari/src/shortcut.c**
  * Disable SDL uses.
  * Disable `ShortCut_InsertDisk` file dialog.
* **hatari/src/sound.c**
* **hatari/src/includes/sound.c**
  * Fix incorrect lowpass filter frequency, and provide cleaner lowpass filter implementation to replace the existing compromised ones. Also [submitted to Hatari](https://github.com/hatari/hatari/pull/25).
  * Store `YM_Buffer_250` and `pos_fract` in savestates, needed for seamless audio after restore.
  * Deliver generated audio to core with `core_audio_update`.
* **hatari/src/st.c**
  * Use core's file system to load and save floppy image.
* **hatari/src/statusbar.c**
  * LED and message timers changed to count frames instead of using `SDL_GetTicks`.
  * Make floppy LED in top right slightly larger.
  * Fix LED logic that caused its apperance to corrupt on resolution changes. Also [submitted to Hatari](https://github.com/hatari/hatari/pull/27).
* **hatari/src/tos.c**
  * Add EmuTOS built-in ROMs.
  * Prevent Hatari from switching the machine configuration due to TOS mismatch. Display the notification onscreen, but let the user modify their own config. This prevents Libretro's core options model from causing spurious resets in these cases (Hatari is modelled on just modifying the config live, but Libretro core options should be provided by the user only, not modified by the running emulation).
  * Cache loaded TOS ROM for faster reload.
  * On TOS ROM load failure, notify user and allow emulation to continue (usually crash or halt) instead of trying to exit.
  * Give the core a pointer to the ROM memory for Libretro `retro_memory_maps` implementation.
  * EmuTOS region and framerate override options.
* **hatari/src/unzip.c**
* **hatari/src/includes/unzip.h**
  * Replace direct file access to unzip from a memory buffer instead.
* **hatari/src/video.c**
  * `Video_ResetShifterTimings` relays current framerate to `core_set_fps`.
  * Unused variable warning suppression for `ENABLE_TRACING`.
  * Save `VBL_ClockCounter` to prevent state divergence.
* **hatari/src/zip.c**
  * Disable use of `unzOpen` which was modified (see: unzip.c) and not needed by this core.
* **hatari/cpu/custom.c**
  * Make `extra_cycle` externally accessible for savestate.
* **hatari/cpu/hatari-glue.c**
  * Added `core_save_state`, `core_restore_state` and `core_flush_audio` to facilitate seamless savestates.
* **hatari/cpu/memory.c**
  * Disable `SDL_Quit`.
* **hatari/cpu/newcpu.c**
  * Split `m68k_go` into `m68k_go`, `m68k_go_frame`, and `m68k_go_quit` to allow emulation loop to return to the Libretro core after each frame.
    * `m68k_go` initializes the CPU and prepares to emulate the first frame before it exits. This is the last thing done during `retro_init`.
    * `m68k_go_frame` runs the main emulation loop, returning after one frame. This is called once each frame from `retro_run`.
    * `m68k_go_quit` provides some final shutdown when exiting the emulation loop. This is called in `retro_deinit`.
    * When performing a configuration change or other full reset, we also use `m68k_go_quit -> m68k_go` to provide the equivalent of exiting and re-entering the original `m68k_go`, which the stand-alone Hatari does during resets.
  * Use `LOG_TRACE_PRINT` to direct traces to the log instead of the CPU system's separate log file.
  * Track and restore blitter's override of `set_x_func` so that leaving the frame loop while the blitter is active does not hang the blitter.
  * Drastic savestate restore time reduction by only running `init_table68k` if the CPU model has changed.
* **hatari/src/debug/debugui.c**
  * Disable `SDL_SetRelativeMouseMode`
* **hatari/src/debug/log.c**
* **hatari/src/debug/log.h**
  * Send log message to the Libretro log.
  * Disable log to stderr.
  * Redirect alert dialogs instead to a Libretro onscreen notification.
  * Send trace logs to Libretro log.
* **hatari/src/falcon/microphone.c**
  * Disable SDL audio device usage. (No microphone support at this time.)
* **hatari/src/falcon/nvram.c**
  * Use core's file system to load and save NVRAM.
  * Only save/load NVRAM if using TT or Falcon system which had it.
* **hatari/src/gui-sdl/dlgAlert.c**
  * Disable dialog to remove SDL use.
* **hatari/src/gui-sdl/dlgFileSelect.c**
  * Disable dialog to remove SDL use.
* **hatari/src/gui-sdl/dlgHalt.c**
  * Disable dialog to remove SDL use.
  * Send halt notification to core for onscreen message, and use `M68000_SetSpecial(SPCFLAG_BRK)` to allow the emulation loop to return to Libretro.
* **hatari/src/gui-sdl/dlgJoy.c**
  * Disable dialog to remove SDL use.
* **hatari/src/gui-sdl/dlgJoy.c**
  * Disable dialog to remove SDL use.
* **hatari/src/gui-sdl/dlgKeyboard.c**
  * Disable dialog to remove SDL use.
* **hatari/src/gui-sdl/dlgMain.c**
  * Disable dialog to remove SDL use.
* **hatari/src/gui-sdl/sdlgui.c**
  * Make `pSdlGuiScrn` public, so that `core_osk.c` can use it to render the on-screen keyboard.
  * Tweak colours of buttons for use by on-screen keyboard.
  * Modify `SDLGui_DrawBox` to provide `SDLGui_DirectBox` for use by on-screen keyboard.
  * Disable `SDLGui_EditField`, `SDLGui_ScaleMouseButtonCoordinates`, `SDLGui_DoDialogExt` to remove SDL use.

## SDL2 Usage

The SDL library is not initialized. Aside from some type definitions, it is mostly only needed to provide palette colour translations, and software-rendering the status bar + onscreen keyboard. Only the video subsystem is needed, though the events subsystem is also included because it cannot be disabled in SDL2's configuration. This is the short list of SDL functions used:
* SDL_CreateRGBSurface
* SDL_FreeSurface
* SDL_LockSurface
* SDL_UnlockSurface
* SDL_MapRGB
* SDL_SetPaletteColors
* SDL_SetColorKey
* SDL_UpperBlit
* SDL_FillRect
* SDL_strlcpy

If direct replacements for these were provided, we could remove SDL entirely. Most have a simple function and not used in high-performance code, but `SDL_UpperBlit` and `SDL_FillRect` are both used extensively by the status bar and onscreen keyboard. A naive replacement of those would be simple, but they both have very intensive target-specific optimizations which seem worth keeping, despite the dependency overhead.

You may provide your own SDL2 by overriding the `SDL2_INCLUDE`, `SDL2_LIB`, and `SDL2_LINK` variables found in `makefile`. A minimal static build was chosen instead because on some platforms the dependency was difficult to provide to the user, and it also appeared that it could cause conflicts with RetroArch's SDL2 drivers, if used. (These conflicts seemed to be resolved by removing any SDL initialization, but it seemed prudent to avoid using the global shared object altogether.)

Notes for removing SDL2:
* We probably need to keep the configure step of `makefile.sdl` to generate headers, but the make is no longer needed. Instead just `make install-hdrs` will copy the needed header files.
* Disable the check for SDL2 in `CMakeLists.txt`, just set `SDL2_FOUND` instead.
* Remove SDL2_LINK from `makefile`.
* Implement the functions listed above in out own core implementation.
* See [PR #16](https://github.com/bbbradsmith/hatariB/pull/16) for reference.
