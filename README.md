# FRANK OS

A FreeRTOS-based operating system for the RP2350 microcontroller, featuring a multi-window terminal environment with PS/2 keyboard input, VGA display output, SD card filesystem support, and the ability to load and run ELF binaries.

Built on work from [Murmulator OS 2](https://github.com/DnCraptor/murmulator-os2) by DnCraptor.

## Requirements

- Raspberry Pi Pico 2 (RP2350) or compatible board
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) installed and configured
- ARM GCC toolchain
- CMake 3.13+
- `picotool` (for flashing)

## Build

```bash
./build.sh
```

This removes any previous build directory, runs CMake, and compiles with `make -j4`. The output firmware is placed in `build/`.

## Flash

```bash
./flash.sh
```

Loads `build/frankos.elf` onto a connected Pico device using `picotool`. You can also specify a firmware path:

```bash
./flash.sh path/to/firmware.elf
```

## License

FRANK OS is licensed under the [GNU General Public License v3.0 or later](LICENSE).

Portions derived from Murmulator OS 2 by DnCraptor are used under the same license.

Third-party components retain their original licenses (MIT, BSD-3-Clause, ISC, Apache-2.0) as noted in their respective source files.
