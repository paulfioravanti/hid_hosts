#!/bin/bash
gcc $(pkg-config --cflags --libs hidapi steno_tape) hid_host.c -o hid_host
