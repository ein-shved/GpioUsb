# GPIO CDC

This is simple project for stm32 to control gpio pins of blue pill board with
simple text-based command set via CDC device.

## Build and flash with Linux

Prepare your distributive to cross-compile and use `make` to compile project.
Flash `GpioUsb.bin` file from build directory to your board any way you prefer
(e.g. st-link).

## Build and flash with Nix

If you have Nix installed on your system (e.g. NixOs), simply run `nix-build`
from root of project. Then you can simply run

```bash
./result/flasher
```

to flash firware with st-link. Or use `result/GpioUsb.bin` to flash firmware the
way you prefer. Use `result/GpioUsb.elf` for debugging.
