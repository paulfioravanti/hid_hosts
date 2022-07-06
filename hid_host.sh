#!/bin/bash
# This script was created for use with an Elgato Stream Deck Pedal
# Assign a System > Open action to a pedal and reference this script
# in the "App/File" field.
# In this case, the argument `1` will trigger a Gaming layer toggle in my Georgi
# firmware. See `raw_hid_receive` definition in:
# https://github.com/paulfioravanti/qmk_keymaps/blob/master/keyboards/gboards/georgi/keymaps/paulfioravanti/keymap.c

./hid_host 1
