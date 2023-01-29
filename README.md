# HID Hosts

This repository contains code I use to communicate with [QMK][] firmware-powered
keyboards via the [Human Interface Device][] (HID) class of the [USB][]
specification.

Basically, I want to trigger keyboard functionality from external devices, like
USB foot pedals etc, and QMK's [Raw HID][] feature allows this to happen. But, a
"host" is required to send the messages/payloads to the keyboard.

The code is solely concerned with the sending of payloads to and from
[Georgi][] stenographic keyboards (see the `raw_hid_receive` function definition
in [my Georgi keymap][] to see how the payload is received, handled, and then
sent back to the computer), but it can be adapted for any keyboard's QMK
firmware:

- Find your keyboard's firmware in QMK's [keyboard list][]
- Look for the `VENDOR_ID` and `PRODUCT_ID` values in the top-level `config.h`
  file
- Substitute those values, as well as any payload changes you want to make, into
  whatever file in this repo you would like to use

## Prerequisites

The [HIDAPI][] library is required to open up the HID communication channels, so
install it with your operating system's package manager. HIDAPI provides some
minimal information about this in their [Installing HIDAPI][] section, but if
you are using macOS, you can install it with [Homebrew][]:

```sh
brew install hidapi
```

## Implementations

I started out by making a short HID host using [NodeJS][], but then adapted that
code to make a [C][] version. Both versions are included in this repository.

### C

This version of the host I use and will make improvements to moving forward.

#### Prerequisites

Feel free to compile as you see fit, but I use [pkg-config][] ([repo][pkg-config
repo]) to provide an easier interface to include libraries. You can install it
via your operating system package manager, or use Homebrew if you use macOS:

```sh
brew install pkg-config
```

#### Compile

```sh
gcc $(pkg-config --cflags --libs hidapi) hid_host.c -o hid_host
```

This will generate a `hid_host` executable file.

#### Run

Currently, the script only accounts for integers to be sent through as command
line parameters, which are then received in [my Georgi keymap][].

```sh
./hid_host 1
./hid_host 2
```

I'm sure there is room for improvement here as I am not a C programmer, and I do
notice that errors can happen occasionally for reasons currently unknown to me.
But, for the most part, it currently _seems_ to be fit for its limited purpose.

### Node JS

This version of the host was my first attempt at writing a host, since I find
NodeJS a bit less scary than C. However, now that I have the C version, I use it
over this one, and its inclusion in this repository can just be considered a
first iteration, and for demonstration purposes only.

#### Prerequistes

This requires global installation of the [node-hid][] library via [npm][]:

```sh
npm install -g node-hid
```

#### Run

Currently, the script only accounts for integers to be sent through as command
line parameters, which are then received in [my Georgi keymap][].

```sh
node hidHost.js 1
node hidHost.js 2
```

[C]: https://en.wikipedia.org/wiki/C_(programming_language)
[Georgi]: https://www.gboards.ca/product/georgi
[HIDAPI]: https://github.com/libusb/hidapi
[Homebrew]: https://brew.sh/
[Human Interface Device]: https://en.wikipedia.org/wiki/USB_human_interface_device_class
[Installing HIDAPI]: https://github.com/libusb/hidapi#installing-hidapi
[keyboard list]: https://github.com/qmk/qmk_firmware/tree/master/keyboards
[my Georgi keymap]: https://github.com/paulfioravanti/qmk_keymaps/blob/master/keyboards/gboards/georgi/keymaps/paulfioravanti/keymap.c
[node-hid]: https://github.com/node-hid/node-hid
[NodeJS]: https://nodejs.org/en/
[npm]: https://www.npmjs.com/
[pkg-config]: https://en.wikipedia.org/wiki/Pkg-config
[pkg-config repo]: https://gitlab.freedesktop.org/pkg-config/pkg-config
[QMK]: https://qmk.fm/
[Raw HID]: https://docs.qmk.fm/#/feature_rawhid
[USB]: https://en.wikipedia.org/wiki/USB
