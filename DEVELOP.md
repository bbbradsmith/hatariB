# hatariB Development Notes

This document will contain build setup instructions, notes on the development of the Libretro core, notes on the integration of Hatari, and notes on the modification of Hatari for this project.

This is intended as reference documentation for future maintenance of this project.

Incorporated sources:

* [hatari/](https://git.tuxfamily.org/hatari/hatari.git/tag/?id=v2.4.1) 2.4.1 2022-08-03
* [libretro/libretro.h](https://github.com/libretro/libretro-common/blob/7edbfaf17baffa1b8a00231762aa7ead809711b5/include/libretro.h) 24a9210 2023-07-16
* [libretro/libretro_sdl_keymap.h](https://github.com/libretro/RetroArch/blob/b4143882245edd737c7e7c522b25e32f8d1f64ad/input/input_keymaps.c#L607) 9ca5c5e 2023-07-08

## Prerequisites

This has been built and tested with MSYS2 UCRT64. The following tools are required:

* gcc
* make
* cmake
* SDL2
* zlib

## Changes to Hatari

Changes to the C source code are all contained in `__LIBRETRO__` defines. Otherwise there are minor changes to the CMake build files, marked with a comment that beings with: `# hatariB`

* **hatari/src/CMakeLists.txt**
  * Disabled `hatari` exectuable build target, added `core` library target with `__LIBRETRO__` define.
  * `set_target_properties(hatari PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)`
  * `add_library(core ${SOURCES})`
  * `target_compile_definitions(core PUBLIC __LIBRETRO__)`
* **hatari/src/cpu/CMakeLists.txt**
* **hatari/src/debug/CMakeLists.txt**
* **hatari/src/falcon/CMakeLists.txt**
* **hatari/src/gui-sdl/CMakeLists.txt**
  * Define `__LIBRETRO__`for all internal hatari libraries.
  * target_compile_definitions(xxx PUBLIC __LIBRETRO__)`

* **hatari/src/includes/main.h**
  * Added common include `../../core/core.h` to give quick common access to hatariB core.
* **hatari/src/main.c**
  * Split `main` into `main_init`, and `main_deinit`.
  * Skip `Win_OpenCon` and `setenv`.
  * `Main_WaitOnVbl` now sets `SPCFLAG_BRK` to exit the CPU run loop on each simulated vblank.
  * Always assume perfect frame timing rather than measuring system time and manually delaying.
  * Replace SDL event queue polling with a simulated input event queue from the Libretro core.
  * Disable `Main_WarpMouse` because Libretro will capture the mouse with its focus mode.
  * Replace default status bar help message with a note to press Scroll-Lock for focus mode.
* **hatari/src/cpu/newcpu.c**
  * Split `m68k_go` into `m68k_go`, `m68k_go_frame` and `m68k_go_quit`.
* **hatari/src/screen.c**
  * Don't create or use window or SDL renderer.
  * Use existing SDL software surface, but send its data to the core (`core_video_update`) instead of using a renderer present.
  * Use SDL software rendering to support all 3 pixel formats allowed by Libretro.
* **hatari/src/video.c**
  * Notify core of video framerate changes.
* **hatari/src/audio.c**
  * Disable SDL audio initialization, but report Audio_Init as successful to enable audio generation.
  * Notify core of audio samplerate changes.
* **hatari/src/sound.c**
  * Send generated audio to core in `Sound_Update`.
* **hatari/src/joy.c**
  * Disable SDL josyick initialization.
  * Disable using keys as joystick, since Libretro can do this already.
  * Use core-provided joystick state instead.
