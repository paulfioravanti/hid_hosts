# HID Hosts

This repository contains code I use to communicate with [QMK][] firmware-powered
keyboards via the [Human Interface Device][] (HID) class of the [USB][]
specification.

Basically, I want to trigger keyboard functionality from external devices, like
USB foot pedals etc, and QMK's [Raw HID][] feature allows this to happen. But, a
"host" is required to send and receive the messages to the keyboard.

The code is currently solely concerned with passing messages to and from
[Georgi][] stenographic keyboards (see the `raw_hid_receive` function definition
in [my Georgi keymap][] to see how the message is received, handled, and then
sent back to the computer), but it can be adapted for any keyboard's QMK
firmware:

- Find your keyboard's firmware in QMK's [keyboard list][]
- Look for the `VENDOR_ID` and `PRODUCT_ID` values in the top-level `config.h`
  file
- Substitute those values, as well as any payload changes you want to make, into
  whatever file in this repo you would like to use

## Prerequisites

### HIDAPI

The [HIDAPI][] library is required to open up HID communication channels, so
install it with your operating system's package manager. HIDAPI provides some
minimal information about this in their [Installing HIDAPI][] section, but if
you are using macOS, you can install it with [Homebrew][]:

```sh
brew install hidapi
```

### Steno Tape

The HID host uses the [Steno Tape][] library to output custom entries to the
steno tape, so that needs to be installed:

```sh
git clone git@github.com:paulfioravanti/steno_tape.git
cd steno_tape
make install
```

### `pkg-config`

Feel free to compile the host as you see fit, but I use [pkg-config][]
([repo][pkg-config repo]) to provide an easier interface to include libraries.
You can install it via your operating system package manager, or use Homebrew if
you use macOS:

```sh
brew install pkg-config
```

## Compile

If you use `pkg-config`, you can use the included build file:

```sh
./build.sh
```

Otherwise, feel free to use it as a guide to make your own.

## Run

Currently, the C executable only accounts for integers to be sent through as
command line parameters, which are then received in [my Georgi keymap][].

```sh
./hid_host 1
./hid_host 2
```

In order to run the host with an [Elgato Stream Deck Pedal][], I needed to
create a simple [shell][] wrapper around the [C][] executable
(`hid_host_client.sh`), since the only kinds of external scripts [Stream Deck][]
seems to be able to run are shell scripts:

![Stream Deck Doom Typist config][Stream Deck Doom Typist config image url]

```sh
./hid_host_client.sh 1
./hid_host_client.sh 2
```

## Troubleshooting

I have had issues where I was getting random errors when reading to or writing
from the HID device (in this case, the Georgi). Going into the Keyboard
settings for macOS and changing/resetting the "Key repeat rate" and/or the
"Delay until repeat" settings, and _then_ restarting the computing seemed to get
things more stable.

More info at:

- [Is it possible to adjust the key repeat rate?][]
- [How to increase keyboard key repeat rate on OS X?][]

[C]: https://en.wikipedia.org/wiki/C_(programming_language)
[Elgato Stream Deck Pedal]: https://www.elgato.com/us/en/p/stream-deck-pedal
[Georgi]: https://www.gboards.ca/product/georgi
[HIDAPI]: https://github.com/libusb/hidapi
[Homebrew]: https://brew.sh/
[How to increase keyboard key repeat rate on OS X?]: https://apple.stackexchange.com/questions/10467/how-to-increase-keyboard-key-repeat-rate-on-os-x
[Human Interface Device]: https://en.wikipedia.org/wiki/USB_human_interface_device_class
[Installing HIDAPI]: https://github.com/libusb/hidapi#installing-hidapi
[Is it possible to adjust the key repeat rate?]: https://karabiner-elements.pqrs.org/docs/help/how-to/key-repeat/
[keyboard list]: https://github.com/qmk/qmk_firmware/tree/master/keyboards
[my Georgi keymap]: https://github.com/paulfioravanti/qmk_keymaps/blob/master/keyboards/gboards/georgi/keymaps/paulfioravanti/user/hid.c
[pkg-config]: https://en.wikipedia.org/wiki/Pkg-config
[pkg-config repo]: https://gitlab.freedesktop.org/pkg-config/pkg-config
[QMK]: https://qmk.fm/
[Raw HID]: https://docs.qmk.fm/#/feature_rawhid
[shell]: https://en.wikipedia.org/wiki/Shell_script
[Steno Tape]: https://github.com/paulfioravanti/steno_tape
[Stream Deck]: https://www.elgato.com/us/en/s/welcome-to-stream-deck
[Stream Deck Doom Typist config image url]: ./assets/stream-deck-doom-typist.jpg
[USB]: https://en.wikipedia.org/wiki/USB
