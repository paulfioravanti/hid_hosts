#!/bin/bash
gcc $(pkg-config --cflags --libs hidapi) hid_host.c -o hid_host
