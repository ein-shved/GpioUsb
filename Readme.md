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

## Requests scheme

Each request has next view:

```
<cmd> <reg> <pin> [ <arg> ]
```

### Commands

`<cmd>` can be one of:

* up - pull pin(s) up
* down - pull pin(s) down
* toggle - toggle pin(s)
* get - read state of pin. The `up` or `down` will be echoed back
* init - initialize pin(s) with one of `out` or `in` passed with `<arg>`
* deinit - deinitialize pin(s)

### Reg

The `<reg>` can be a letter from a to e, denoting corresponding port

### Pin

The `<pin>` can be in one of next forms:

* `<pin number>` - the number of pin port from 0 to 15
* `m<pin mask>` - the mask of pins predeceased by `m` letter in bounds from 0 to
   0xffff

Each number can be in decimal, octal (with leading `0`) or hexadecimal (with
leading `0x`) form.
