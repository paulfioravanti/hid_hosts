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
#include <steno_tape.h>    // STENO_TAPE_ENTRY_MAX_LENGTH, steno_tape_cleanup,
                           // steno_tape_init, steno_tape_log_error,
                           // steno_tape_log_gaming_mode,
                           // steno_tape_log_mode_unchanged,
                           // steno_tape_log_steno_mode, Tape

// VID and PID for Georgi
// REF: https://github.com/qmk/qmk_firmware/blob/master/keyboards/gboards/georgi/config.h
#define VENDOR_ID  0xFEED
#define PRODUCT_ID 0x1337
// Usage values may be different depending on machine/OS etc but are crucial for
// stability in talking to the keyboard.
#define USAGE 0x61
#define USAGE_PAGE 0xFF60

enum {
  BUFFER_LENGTH = 2,
  ENABLE_NONBLOCKING = 1,
  HID_DARWIN_NON_EXCLUSIVE_MODE = 0,
  HID_OPEN_MAX_RETRIES = 30,
  HID_OPEN_SLEEP_MICROSECONDS = 15000, // 15 ms
  HID_READ_MAX_RETRIES = 2,
  HID_READ_SLEEP_MICROSECONDS = 500000, // 500 ms
  MODE_GAMING = 0x3,
  MODE_STENO = 0x4,
  MODE_UNCHANGED = 0x9
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

static int parse_arguments(int argc, char *argv[]);
static void read_device_message(
  hid_device *handle,
  unsigned char *buf,
  Tape *tape
);
static void log_out_read_message(int message, Tape *tape);
static void clean_up(Tape *tape);
static void print_buffer(unsigned char *buf);

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
    steno_tape_log_error(tape, HID_INIT_FAIL_MESSAGE);
    clean_up(tape);
    return -1;
  }
  // REF: https://github.com/libusb/hidapi/blob/master/hidtest/test.c#L102
  // REF: https://github.com/libusb/hidapi/blob/master/mac/hidapi_darwin.h#L68
  hid_darwin_set_open_exclusive(HID_DARWIN_NON_EXCLUSIVE_MODE);

  struct hid_device_info *devices, *current_device;
  devices = hid_enumerate(VENDOR_ID, PRODUCT_ID);
  current_device = devices;
  hid_device *handle = NULL;

  while (current_device) {
    if (
      current_device->usage != USAGE ||
      current_device->usage_page != USAGE_PAGE
    ) {
      printf(
        "Skipping -- Usage (page): 0x%hx (0x%hx)\n",
        current_device->usage,
        current_device->usage_page
      );
      current_device = current_device->next;
      continue;
    }

    handle = hid_open_path(current_device->path);

    if (!handle) {
      printf("Failed to open device path!\n");
      printf("HID message: %ls\n", hid_error(handle));
      steno_tape_log_error(tape, DEVICE_OPEN_FAIL_MESSAGE);
      break;
    }

    unsigned char buf[BUFFER_LENGTH];
    memset(buf, 0, sizeof(buf));
    buf[0] = arg;

    res = hid_write(handle, buf, BUFFER_LENGTH);

    if (res < 0) {
      printf("Unable to write() to handle\n");
      printf("Error: %ls\n", hid_error(handle));
      steno_tape_log_error(tape, DEVICE_WRITE_FAIL_MESSAGE);
    } else {
      hid_set_nonblocking(handle, ENABLE_NONBLOCKING);
      read_device_message(handle, buf, tape);
    }
    break;
  }

  if (handle) {
    hid_close(handle);
  }

  hid_free_enumeration(devices);
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
    }
  }

  if (res == 0) {
    printf("Unable to read device after %d retries\n", HID_READ_MAX_RETRIES);
    fprintf(stderr, "hid_read failed: %s\n", strerror(errno));
    steno_tape_log_error(tape, DEVICE_READ_FAIL_MESSAGE);
  } else if (res < 0) {
    printf("Error: Unable to read()\n");
    printf("hid_read result was: %d\n", res);
    steno_tape_log_error(tape, DEVICE_READ_FAIL_MESSAGE);
  } else {
    log_out_read_message(buf[1], tape);
  }

  printf("HID message: %ls\n", hid_error(handle));
  print_buffer(buf);
}

static void log_out_read_message(int message, Tape *tape) {
  switch (message) {
    case MODE_GAMING:
      steno_tape_log_gaming_mode(tape);
      break;
    case MODE_STENO:
      steno_tape_log_steno_mode(tape);
      break;
    case MODE_UNCHANGED:
      steno_tape_log_mode_unchanged(tape);
      break;
    default:
      printf("Message read from device: %d\n", message);
      const char *error_message;
      char buffer[STENO_TAPE_ENTRY_MAX_LENGTH];
      snprintf(
        buffer,
        sizeof(buffer),
        "%s%d\n",
        HID_READ_BAD_VALUE_MESSAGE,
        message
      );
      error_message = buffer;
      steno_tape_log_error(tape, error_message);
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
