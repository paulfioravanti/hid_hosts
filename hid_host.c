#include <stdlib.h> // strtol
#include <string.h> // strlen
#include <errno.h> // errno
#include <limits.h> // INT_MIN, INT_MAX
#include <hidapi.h>

// VID and PID for Georgi
// REF: https://github.com/qmk/qmk_firmware/blob/master/keyboards/gboards/georgi/config.h
#define VENDOR_ID  0xFEED
#define PRODUCT_ID 0x1337

int main(int argc, char* argv[]) {
  // REF: https://stackoverflow.com/questions/9748393/how-can-i-get-argv-as-int
  if (strlen(argv[1]) == 0) {
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
      return 1;
  }
  // Check that the number is within the limited capacity of an int
  if (arg < INT_MIN || arg > INT_MAX) {
      return 1;
  }
  int arg_int = arg;

  int res;
  unsigned char buf[65];
  hid_device *device;

  // Initialize the hidapi library
  res = hid_init();

  // Open the device using the VID, PID,
  device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);

  buf[0] = arg_int;
  res = hid_write(device, buf, 65);

  // Close the device
  hid_close(device);

  // Finalize the hidapi library
  res = hid_exit();

  return 0;
}
