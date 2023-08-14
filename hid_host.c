#include <errno.h>         // errno
#include <hidapi.h>        // hid_close, hid_device, hid_device_info, hid_error,
                           // hid_exit, hid_init, hid_read, hid_write,
                           // open_device
#include <hidapi_darwin.h> // hid_darwin_set_open_exclusive
#include <limits.h>        // INT_MAX, INT_MIN
#include <stdio.h>         // printf, snprintf
#include <stdlib.h>        // strtol
#include <string.h>        // memset, strlen
#include <unistd.h>        // usleep
#include <steno_tape.h>    // MAX_MESSAGE_LENGTH, steno_tape_cleanup,
                           // steno_tape_error, steno_tape_gaming_mode,
                           // steno_tape_init, steno_tape_mode_unchanged,
                           // steno_tape_steno_mode, Tape
#include "hid_host.h"

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

int main(int argc, char *argv[]) {
  int arg = parse_arguments(argc, argv);
  if (arg == -1) {
    return -1;
  }

  Tape *tape = steno_tape_init();
  if (!tape) {
    printf("ERROR: Unable to open log file\n");
    steno_tape_cleanup(tape);
    return -1;
  }

  // Initialize the hidapi library
  int res = hid_init();
  if (res < 0) {
    printf("Unable to initialize HIDAPI library\n");
    steno_tape_error(tape, HID_INIT_FAIL_MESSAGE);
    clean_up(tape);
    return -1;
  }
  // REF: https://github.com/libusb/hidapi/blob/master/hidtest/test.c#L102
  // REF: https://github.com/libusb/hidapi/blob/master/mac/hidapi_darwin.h#L68
  hid_darwin_set_open_exclusive(HID_DARWIN_NON_EXCLUSIVE_MODE);

  hid_device *handle = get_or_open_device();
  if (!handle) {
    steno_tape_error(tape, DEVICE_OPEN_FAIL_MESSAGE);
    clean_up(tape);
    return -1;
  }

  unsigned char buf[BUFFER_LENGTH];
  memset(buf, 0, sizeof(buf));
  buf[0] = arg;

  res = hid_write(handle, buf, BUFFER_LENGTH);

  if (res < 0) {
    printf("Unable to write()\n");
    printf("Error: %ls\n", hid_error(handle));
    steno_tape_error(tape, DEVICE_WRITE_FAIL_MESSAGE);
  } else {
    hid_set_nonblocking(handle, ENABLE_NONBLOCKING);
    read_device_message(handle, buf, tape);
  }

  hid_close(handle);
  clean_up(tape);
  return 0;
}

static int parse_arguments(int argc, char *argv[]) {
  // Requires only one argument
  // REF: https://stackoverflow.com/questions/9748393/how-can-i-get-argv-as-int
  if (argc != 2 || strlen(argv[1]) == 0) {
    printf("ERROR: Must provide a single non-empty argument\n");
    return -1;
  }

  char *end_ptr;
  errno = 0;
  // REF: https://devdocs.io/c/string/byte/strtol
  int arg = strtol(argv[1], &end_ptr, 16);

  // Error out if:
  // - an invalid character was found before the end of the string
  // - overflow or underflow errors occurred
  // - number is outside the limited capacity of an int
  if (*end_ptr != '\0' || errno != 0 || arg < INT_MIN || arg > INT_MAX) {
    printf("ERROR: Invalid argument %s\n", argv[1]);
    return -1;
  }

  return arg;
}

static hid_device* get_or_open_device(void) {
  struct hid_device_info *devices, *current_device;
  hid_device *device = NULL;

  devices = hid_enumerate(0, 0);
  current_device = devices;

  while (current_device) {
    if (is_target_device(current_device)) {
      printf("Device is already open\n");
      device = hid_open_path(current_device->path);

      if (device) {
        printf("Successfully opened device path!\n");
        hid_free_enumeration(devices);
        return device;
      }

      printf("Failed to open device path!\n");
      printf("HID message: %ls\n", hid_error(device));
    }

    current_device = current_device->next;
  }

  hid_free_enumeration(devices);
  printf("Device path not openable or device not currently open\n");

  return open_device();
}

static int is_target_device(struct hid_device_info *device) {
  return device->vendor_id == VENDOR_ID && device->product_id == PRODUCT_ID;
}

static hid_device* open_device(void) {
  int num_open_retries = 0;
  hid_device *handle = NULL;

  while (!handle && num_open_retries < HID_OPEN_MAX_RETRIES) {
    printf("Attempting to open()...\n");
    handle = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
    printf("HID message: %ls\n", hid_error(handle));

    if (handle) {
      break;
    }

    printf("Device open retries: %d\n", ++num_open_retries);
    usleep(HID_OPEN_SLEEP_MICROSECONDS);
  }

  if (!handle) {
    printf("Unable to open device after %d retries\n", HID_OPEN_MAX_RETRIES);
    hid_close(handle);
    return NULL;
  }

  return handle;
}

static void read_device_message(
  hid_device *handle,
  unsigned char *buf,
  Tape *tape
) {
  int res = 0;
  int num_read_retries = 0;
  const char *message;

  while (res == 0 && num_read_retries < HID_READ_MAX_RETRIES) {
    res = hid_read(handle, buf, BUFFER_LENGTH);

    if (res == 0) {
      printf("Waiting to read()...\n");
      usleep(HID_READ_SLEEP_MICROSECONDS);
      num_read_retries++;
      continue;
    }

    if (res < 0) {
      printf("Error: Unable to read()\n");
      printf("HID message: %ls\n", hid_error(handle));
      print_buffer(buf);
      steno_tape_error(tape, DEVICE_READ_FAIL_MESSAGE);
      break;
    }
  }

  if (res == 0) {
    printf("Unable to read device after %d retries\n", HID_READ_MAX_RETRIES);
    printf("hid_read result was: %d\n", res);
    perror("hid_read");
    fprintf(stderr, "hid_read failed: %s\n", strerror(errno));
    print_buffer(buf);
    steno_tape_error(tape, DEVICE_READ_FAIL_MESSAGE);
  } else if (res > 0) {
    printf("HID message: %ls\n", hid_error(handle));
    print_buffer(buf);
    log_out_read_message(buf[1], tape);
  }
}

static void log_out_read_message(int message, Tape *tape) {
  switch (message) {
    case GAMING_MODE:
      steno_tape_gaming_mode(tape);
      break;
    case STENO_MODE:
      steno_tape_steno_mode(tape);
      break;
    case MODE_UNCHANGED:
      steno_tape_mode_unchanged(tape);
      break;
    default:
      printf("Message read from device: %d\n", message);
      const char *error_message;
      char buffer[MAX_MESSAGE_LENGTH];
      snprintf(
        buffer,
        sizeof(buffer),
        "%s%d\n",
        HID_READ_BAD_VALUE_MESSAGE,
        message
      );
      error_message = buffer;
      steno_tape_error(tape, error_message);
  }
}

static void clean_up(Tape *tape) {
  steno_tape_cleanup(tape);
  hid_exit();
}

static void print_buffer(unsigned char *buf) {
  for (int i = 0; i < BUFFER_LENGTH; i++)
    printf("%02x ", (unsigned int) buf[i]);
}
