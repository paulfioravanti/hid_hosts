#pragma once
#ifndef HID_HOST_H
#define HID_HOST_H

#include <hidapi.h>     // hid_device, hid_device_info
#include <steno_tape.h> // Tape

int parse_arguments(int argc, char *argv[]);
hid_device* get_or_open_device(void);
int is_target_device(struct hid_device_info *device);
hid_device* open_device(void);
void read_device_message(hid_device *handle, unsigned char *buf, Tape *tape);
void log_out_read_message(int message, Tape *tape);
void clean_up(Tape *tape);
void print_buffer(unsigned char *buf);

#endif
