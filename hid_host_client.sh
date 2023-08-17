#!/bin/bash
# This client script was created for use with an Elgato Stream Deck Pedal since
# the only kinds of external scripts it can call are shell scripts.
# Assign a System > Open action to a pedal and reference this script
# in the "App/File" field.
# See `raw_hid_receive` definition in my Georgi keymap for how the parameter
# gets handled:
# https://github.com/paulfioravanti/qmk_keymaps/blob/master/keyboards/gboards/georgi/keymaps/paulfioravanti/user/hid.c

./hid_host "$1"
