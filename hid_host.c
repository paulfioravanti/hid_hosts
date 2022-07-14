#include <stdio.h> // printf
#include <stdlib.h> // strtol
#include <string.h> // strlen
#include <errno.h> // errno
#include <limits.h> // INT_MIN, INT_MAX
#include <unistd.h> // usleep
#include <hidapi.h>

// VID and PID for Georgi
// REF: https://github.com/qmk/qmk_firmware/blob/master/keyboards/gboards/georgi/config.h
#define VENDOR_ID  0xFEED
#define PRODUCT_ID 0x1337

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

  // Initialize the hidapi library
  int res = hid_init();
  if (res < 0) {
    printf("Unable to initialize hidapi library\n");
    return 1;
  }

  int max_retries = 20;
  int current_retry = 1;
  // Open the device using the VID, PID,
  hid_device* device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);

  while (!device) {
    usleep(50000);
    device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
    if (current_retry > max_retries) {
      printf("Unable to open device after %d retries\n", max_retries);
      return 1;
    }
    printf("Open device retry: %d\n", current_retry);
    current_retry++;
  }

  unsigned char buf[1] = {arg};
  res = hid_write(device, buf, 1);

  if (res < 0) {
    printf("Unable to write()\n");
    printf("Error: %ls\n", hid_error(device));
  }

  // Close the device
  hid_close(device);

  // Finalize the hidapi library
  hid_exit();

  return 0;
}
