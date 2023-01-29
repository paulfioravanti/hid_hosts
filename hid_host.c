#include <stdio.h>  // FILE, fopen, printf
#include <stdlib.h> // getenv, strtol
#include <string.h> // strcat, strcpy, strlen
#include <errno.h>  // errno
#include <limits.h> // INT_MAX, INT_MIN
#include <unistd.h> // usleep
#include <assert.h> // assert
#include <hidapi.h> // hid_*

// VID and PID for Georgi
// REF: https://github.com/qmk/qmk_firmware/blob/master/keyboards/gboards/georgi/config.h
#define VENDOR_ID  0xFEED
#define PRODUCT_ID 0x1337

enum {
  MAX_RETRIES = 20
};

// REF: https://github.com/rabbitgrowth/plover-tapey-tape
static const char LOG_FILENAME[] =
  "/Library/Application Support/plover/tapey_tape.txt";
static const char HID_INIT_FAIL_MESSAGE[] =
  "ERROR: Unable to initialize HID API library\n";
static const char DEVICE_OPEN_FAIL_MESSAGE[] =
  "ERROR: Exhausted attempts to open HID device\n";
static const char DEVICE_WRITE_FAIL_MESSAGE[] =
  "ERROR: Unable to write to HID device\n";
static const char SUCCESS_MESSAGE[] =
  "SUCCESS: HID message sent successfully\n";

int main(int argc, char* argv[]) {
  // Requires only one argument
  if (argc != 2) {
    printf("Only one argument allowed\n");
    return 1;
  }
  // REF: https://stackoverflow.com/questions/9748393/how-can-i-get-argv-as-int
  if (strlen(argv[1]) == 0) {
    printf("Argument cannot be an empty string\n");
    return 1; // empty string
  }

  char* string_end;
  errno = 0;
  // REF: https://devdocs.io/c/string/byte/strtol
  long arg = strtol(argv[1], &string_end, 10);
  // Error out if:
  // - an invalid character was found before the end of the string
  // - overflow or underflow errors occurred
  if (*string_end != '\0' || errno != 0) {
    printf("Invalid character %s\n", argv[1]);
    return 1;
  }
  // Check that the number is within the limited capacity of an int
  if (arg < INT_MIN || arg > INT_MAX) {
    printf("Integer value is out of bounds");
    return 1;
  }

  char *home_dir = getenv("HOME");
  char *log_filepath = malloc(strlen(home_dir) + strlen(LOG_FILENAME) + 1);
  strcpy(log_filepath, home_dir);
  strcat(log_filepath, LOG_FILENAME);
  FILE *log_file = fopen(log_filepath, "a");
  assert(log_file);

  // Initialize the hidapi library
  int res = hid_init();
  if (res < 0) {
    printf("Unable to initialize hidapi library\n");
    fwrite(HID_INIT_FAIL_MESSAGE, 1, strlen(HID_INIT_FAIL_MESSAGE), log_file);
    fclose(log_file);
    free(log_filepath);
    return 1;
  }

  int current_retry = 1;
  // Open the device using the VID, PID,
  hid_device *device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);

  while (!device) {
    usleep(50000);
    device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
    if (current_retry > MAX_RETRIES) {
      printf("Unable to open device after %d retries\n", MAX_RETRIES);
      fwrite(
        DEVICE_OPEN_FAIL_MESSAGE,
        1,
        strlen(DEVICE_OPEN_FAIL_MESSAGE),
        log_file
      );
      fclose(log_file);
      free(log_filepath);
      return 1;
    }
    printf("Open device retry: %d\n", current_retry);
    current_retry++;
  }

  int retval = 0;
  unsigned char buf[1] = {arg};
  res = hid_write(device, buf, 1);

  if (res < 0) {
    printf("Unable to write()\n");
    printf("Error: %ls\n", hid_error(device));
    fwrite(
      DEVICE_WRITE_FAIL_MESSAGE,
      1,
      strlen(DEVICE_WRITE_FAIL_MESSAGE),
      log_file
    );
    retval = 1;
  } else {
    fwrite(SUCCESS_MESSAGE, 1, strlen(SUCCESS_MESSAGE), log_file);
  }

  fclose(log_file);
  free(log_filepath);

  // Close the device
  hid_close(device);
  // Finalize the hidapi library
  hid_exit();
  return retval;
}
