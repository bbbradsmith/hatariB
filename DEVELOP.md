# hatariB Development Notes

This document will contain build setup instructions, notes on the development of the Libretro core, notes on the integration of Hatari, and notes on the modification of Hatari for this project.

This is intended as reference documentation for future maintenance of this project.

Incorporated sources:

* [hatari/](https://git.tuxfamily.org/hatari/hatari.git/tag/?id=v2.4.1) 2.4.1 2022-08-03
* [libretro/libretro.h](https://github.com/libretro/libretro-common/blob/7edbfaf17baffa1b8a00231762aa7ead809711b5/include/libretro.h) 24a9210 2023-07-16
* [libretro/libretro_sdl_keymap.h](https://github.com/libretro/RetroArch/blob/b4143882245edd737c7e7c522b25e32f8d1f64ad/input/input_keymaps.c#L607) 9ca5c5e 2023-07-08
* [emutos/](https://emutos.sourceforge.io/) 1.2.1 2022-08-16
* [miniz](https://github.com/richgel999/miniz/tree/3.0.2) 3.0.2 2023-01-15

## Prerequisites

This has been built and tested with MSYS2 UCRT64. The following tools are required:

* gcc
* make
* cmake
* SDL2
* zlib

## Changes to Hatari

Changes to the C source code are all contained in `__LIBRETRO__` defines. This is important for merging future versions. None of the original code has been modified or deleted, only disabled within `#ifdef` blocks.

Otherwise there are minor changes to the CMake build files, marked with a comment that beings with: `# hatariB`

* **hatari/src/CMakeLists.txt**
  * Disabled `hatari` exectuable build target, added `core` library target with `__LIBRETRO__` define.
  * `set_target_properties(hatari PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)`
  * `add_library(core ${SOURCES})`
  * `target_compile_definitions(core PUBLIC __LIBRETRO__)`
  * Removed `HAVE_STATVFS` check because we use the Libretro VFS instead.
* **hatari/src/cpu/CMakeLists.txt**
* **hatari/src/debug/CMakeLists.txt**
* **hatari/src/falcon/CMakeLists.txt**
* **hatari/src/gui-sdl/CMakeLists.txt**
  * Define `__LIBRETRO__`for all internal hatari libraries.
  * target_compile_definitions(xxx PUBLIC __LIBRETRO__)`
* **hatari/src/tools/CMakeLists.txt**
  * Disabled `hmsa` support tool build (not needed, can't like with changes to floppy system).
* Use `-DENABLE_SMALL_MEM=0` and `-DENABLE_TRACING=0` to slightly improve performance.
  * Note that ENABLE_SMALL_MEM can save ~14-15MB of RAM, so it might be worthwhile for platforms with smaller memory.

* **hatari/src/includes/main.h**
  * Added common include `../../core/core.h` to give quick common access to hatariB core.
  * Added `SDL2/` prefix to SDL header include.
* **hatari/src/main.c**
  * Split `main` into `main_init`, and `main_deinit`.
  * Skip `Win_OpenCon` and `setenv`.
  * `Main_WaitOnVbl` now sets `SPCFLAG_BRK` to exit the CPU run loop on each simulated vblank.
  * Always assume perfect frame timing rather than measuring system time and manually delaying.
  * Replace SDL event queue polling with a simulated input event queue from the Libretro core. Limit only to key/mouse input events.
  * Don't SDL_Init or SDL_Quit, SDL is not used except for creating the software texture, which does not require init. (SDL_Quit will conflict with some versions of RetroArch SDL2 input drivers and crash on close.)
  * Disable `Main_WarpMouse` because Libretro will capture the mouse with its focus mode.
  * Replace default status bar help message with a note to press Scroll-Lock for focus mode.
  * Replace loading config from file with core configuration.
  * Allow missing TOS file to pass through initialization so we can halt the CPU and report the error to the user later.
* **hatari/src/cpu/newcpu.c**
  * Split `m68k_go` into `m68k_go`, `m68k_go_frame` and `m68k_go_quit`.
  * Add `core_init_return` to exit after CPU initialization but before first cycles run.
  * Exit the run loop just after `savestate_restore_final` to allow resume from restored point at next `retro_run`.
  * Edited `m68k_disasm_file` hardcoded `TraceFile` to use `LOG_TRACE_PRINT` instead so it can redirect to libretro.
* **hatari/src/screen.c**
  * Don't create or use window or SDL renderer, avoid sending renderer hints.
  * Use existing SDL software surface, but send its data to the core (`core_video_update`) instead of using a renderer present.
  * Use SDL software rendering to support all 3 pixel formats allowed by Libretro.
  * Don't SDL_Quit on error.
  * Add option for low resolution pixel doubling.
* **hatari/src/resolution.c**
  * Don't use SDL_GetDesktopDisplayMode to get resolution.
* **hatari/src/video.c**
  * Notify core of video framerate changes.
  * Suppress unused-variable warnings due to `ENABLE_TRACING`.
* **hatari/src/audio.c**
  * Disable SDL audio initialization, but report Audio_Init as successful to enable audio generation.
  * Notify core of audio samplerate changes.
* **hatari/src/sound.c**
  * Send generated audio to core in `Sound_Update`.
  * Save `YM_Buffer` to savestates for seamless audio across savestates/runahead.
  * Added IIR lowpass filter option for lower audio distortion.
  * Fixed broken lowpass filter implementation which was running at 250kHz instead of 44/48kHz.
* **hatari/src/joy.c**
  * Disable SDL josyick initialization.
  * Use core-provided joystick state instead.
* **hatari/src/memorySnapShot.c**
  * Replaced snapshot file access with a core-provided in-memory buffer.
  * Disable snapshot compression because Libretro needs it uncompressed.
  * Make `bCaptureError` externally accessible to check for errors after restore.
  * Disable some `Log_AlertDlg`s to prevent spam.
* **hatari/src/hatari-glue.c**
  * Add `hatari_libretro_save_state` and `hatari_libretro_restore_state` to execute savestates between `retro_run` calls.
  * Add `hatari_libretro_flush_audio` to reset audio queue when needed.
* **hatari/src/tos.c**
  * Allow built-in TOS bios (EmuTOS).
  * Use core_file to load the TOS image.
  * Keep a cached copy of last used TOS to prevent reloading the file on restore.
  * Allow missing TOS to proceed, signal failure to core to be reported there later.
* **hatari/src/cart.c**
  * Use core_file to load the cartridge image.
* **hatari/src/debug/log.h**
* **hatari/src/debug/log.c**
  * Redirect `Log_Printf` to libretro debug log (optional define for debug).
  * Redirect `LOG_TRACE` and `LOG_TRACE_PRINT` to libretro debug log.
  * Disable blocking `DlgAlert_Notice`, send to onscreen notification and error log instead.
* **hatari/src/reset.c**
  * Signal the core when a reset happens so it can be handled properly.
* **hatari/src/gui-sdl/dlgHalt.c**
  * Disable blocking `Dialog_HaltDlg`, replace with a signal to the core to halt its own emulation.
* **hatari/src/configuration.c**
* **hatari/src/includes/configuration.h**
  * Add option for built-in TOS bios (EmuTOS).
  * Add option for low resolution pixel doubling.
  * Add options for YM filter configuration.
  * Override some of the default options.
  * Don't use SDL_NumJoysticks.
* **hatari/src/keymap.c**
  * Block Scroll Lock, F11, F12, F13 from generating keypresses.
  * Disable all Hatari shortcut keys.
  * Disable Hatari key to joystick mappings.
  * Provide modifier key state instead of `SDL_GetModState`.
* **hatari/src/change.c**
  * Signal screen change when Low Resolution pixel doubling option changes.
  * Save/Reload NvRam when machine changes, only if TT or Falcon.
* **hatari/src/cpu/memory.c**
  * Don't SDL_Quit on error.
* **hatari/src/floppy.c**
* **hatari/src/includes/floppy.h**
  * Load floppy images from memory instead of from file.
  * Read back inserted floppy filenames after savestate restore.
  * Mark disks potentially changed by savestates for re-save.
  * Disable check for filesystem write protect on floppy images, not available to libretro core.
* **hatari/src/includes/file.h**
  * Access to core_file interface.
* **hatari/src/st.c**
* **hatari/src/dim.c**
* **hatari/src/msa.c**
* **hatari/src/floppy_ipf.c**
* **hatari/src/floppy_stx.c**
  * Load floppy image from memory instead of from file.
  * Use core interfaces to save modified floppies to saves/ folder.
* **hatari/src/midi.c**
  * Suppress unused-variable warnings due to `ENABLE_TRACING`.
* **hatari/src/gemdos.c**
* **hatari/src/ide.c**
* **hatari/src/ide.c**
* **hatari/src/ncr5380.c**
* **hatari/src/hdc.c**
* **hataru/src/inffile.c**
* **hatari/src/includes/hdc.h**
* **hataru/src/includes/inffile.h**
  * Replace hard disk direct filesystem/image access with core_file.
  * Add write protection option for ACSI/SCSI/IDE hard disks as well.
  * Suppress unused-variable warnings due to `ENABLE_TRACING`.
* **hatari/src/paths.c**
  * Disable all attempts to find system paths, replace with `<nopath>` to help find and eliminate remaining attempts to use them.
* **hatari/src/falcon/nvram.c**
  * NVRAM file should only save/load when using TT or Falcon mode.
  * Redirect file to system/hatarib.nvram
* **hatar


`SDL_Init` and `SDL_Quit` almost all use of the SDL library have been suppressed. There are some remaining uses of the SDL libraries but I do not believe any of them require Init. These include:
* SDL ending defines and utilities (e.g. `SDL_SwapLE16`) which are used throughout.
* `SDL_CreateRGBSurface` to create `SDL_SWSURFACE` only. I do not believe this relies on `INIT_VIDEO` (which would have the collateral of `INIT_EVENTS` etc.).
* `statusbar.c` and `gui-dsl/sdlgui.c` draw to the surface, using `SDL_FillRect`, `SDL_BlitSurface`, `SDL_LockSurface`, etc. which I believe are all fine without `INIT_VIDEO`.
