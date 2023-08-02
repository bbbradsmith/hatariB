# hatariB Development Notes

This document will contain build setup instructions, notes on the development of the Libretro core, notes on the integration of Hatari, and notes on the modification of Hatari for this project.

This is intended as reference documentation for future maintenance of this project.


* [hatari/](https://git.tuxfamily.org/hatari/hatari.git/tag/?id=v2.4.1) 2.4.1 2022-08-03
* [libretro/libretro.h](https://github.com/libretro/libretro-common/blob/7edbfaf17baffa1b8a00231762aa7ead809711b5/include/libretro.h) 24a9210 2023-07-16

## Prerequisites

This has been built and tested with MSYS2 UCRT64. The following tools are required:

* gcc
* make
* cmake
* SDL2

## Changes to Hatari

* **hatari/src/CMakeLists.txt**
  * Disabled `hatari` exectuable build target, added `core` library target.
  * `set_target_properties(hatari PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)`
  * `add_library(core ${SOURCES})`
