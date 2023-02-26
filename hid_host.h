#include <errno.h>         // errno
#include <hidapi.h>        // hid_close, hid_device, hid_error, hid_exit,
                           // hid_init, hid_read, hid_write, open_device
#include <hidapi_darwin.h> // hid_darwin_set_open_exclusive
#include <limits.h>        // INT_MAX, INT_MIN
#include <stdio.h>         // printf, snprintf
#include <stdlib.h>        // strtol
#include <string.h>        // memset, strlen
#include <unistd.h>        // usleep
#include "steno_tape.h"    // steno_tape_cleanup, steno_tape_error,
                           // steno_tape_gaming_mode, steno_tape_init,
                           // steno_tape_mode_unchanged, steno_tape_steno_mode

// VID and PID for Georgi
// REF: https://github.com/qmk/qmk_firmware/blob/master/keyboards/gboards/georgi/config.h
#define VENDOR_ID  0xFEED
#define PRODUCT_ID 0x1337

enum {
  BUFFER_LENGTH = 2,
  ENABLE_NONBLOCKING = 1,
  GAMING_MODE = 0x3,
  HID_DARWIN_NON_EXCLUSIVE_MODE = 0,
  HID_OPEN_MAX_RETRIES = 30,
  HID_OPEN_SLEEP_MICROSECONDS = 15000, // 15 ms
  HID_READ_MAX_RETRIES = 3,
  HID_READ_SLEEP_MICROSECONDS = 500000, // 500 ms
  MODE_UNCHANGED = 0x9,
  STENO_MODE = 0x4
};

static const char HID_INIT_FAIL_MESSAGE[] =
  " Unable to initialize HID library!\n";
static const char DEVICE_OPEN_FAIL_MESSAGE[] =
  " Couldn't OPEN device!\n";
static const char DEVICE_WRITE_FAIL_MESSAGE[] =
  " Couldn't WRITE to device!\n";
static const char DEVICE_READ_FAIL_MESSAGE[] =
  " Couldn't READ from device!\n";
static const char HID_READ_BAD_VALUE_MESSAGE[] =
  " Unexpected response from device: ";

int parse_arguments(int argc, char *argv[]);
hid_device* get_or_open_device(void);
int is_target_device(struct hid_device_info *device);
hid_device* open_device(void);
void read_device_message(hid_device *device, unsigned char *buf, Tape *tape);
void log_out_read_message(int message, Tape *tape);
void clean_up(Tape *tape);
void print_buffer(unsigned char *buf);
