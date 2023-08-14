#pragma once
#ifndef HID_HOST_H
#define HID_HOST_H

#include <hidapi.h>     // hid_device, hid_device_info
#include <steno_tape.h> // Tape

static int parse_arguments(int argc, char *argv[]);
static hid_device* get_or_open_device(void);
static int is_target_device(struct hid_device_info *device);
static hid_device* open_device(void);
static void read_device_message(
  hid_device *handle,
  unsigned char *buf,
  Tape *tape
);
static void log_out_read_message(int message, Tape *tape);
static void clean_up(Tape *tape);
static void print_buffer(unsigned char *buf);

#endif
