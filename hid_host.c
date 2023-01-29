#include <stdio.h>  // FILE, fopen, printf
#include <stdlib.h> // getenv, strtol
#include <string.h> // strcat, strcpy, strlen
#include <errno.h>  // errno
#include <limits.h> // INT_MAX, INT_MIN
#include <unistd.h> // usleep
#include <assert.h> // assert
#include "hid_host.h"

int main(int argc, char* argv[]) {
  long arg = parse_arguments(argc, argv);
  if (arg == -1) {
    return 1;
  }

  char *log_filepath = generate_log_filepath();
  FILE *log_file = fopen(log_filepath, "a");
  assert(log_file);

  // Initialize the hidapi library
  int res = hid_init();
  if (res < 0) {
    printf("Unable to initialize hidapi library\n");
    log_message(HID_INIT_FAIL_MESSAGE, log_file);
    clean_up(log_filepath, log_file);
    return 1;
  }

  hid_device *device = open_device();
  if (!device) {
    log_message(DEVICE_OPEN_FAIL_MESSAGE, log_file);
    clean_up(log_filepath, log_file);
    return 1;
  }

  int retval = 0;
  unsigned char buf[BUFFER_LENGTH] = {arg};
  res = hid_write(device, buf, BUFFER_LENGTH);

  if (res < 0) {
    printf("Unable to write()\n");
    printf("Error: %ls\n", hid_error(device));
    log_message(DEVICE_WRITE_FAIL_MESSAGE, log_file);
    retval = 1;
  } else {
    hid_set_nonblocking(device, 1);
    read_device_message(device, buf, log_file);
  }

  hid_close(device);
  clean_up(log_filepath, log_file);
  return retval;
}

long parse_arguments(int argc, char* argv[]) {
  // Requires only one argument
  if (argc != 2) {
    printf("ERROR: Must have only one argument\n");
    return -1;
  }
  // REF: https://stackoverflow.com/questions/9748393/how-can-i-get-argv-as-int
  if (strlen(argv[1]) == 0) {
    printf("ERROR: Argument cannot be an empty string\n");
    return -1; // empty string
  }

  char* string_end;
  errno = 0;
  // REF: https://devdocs.io/c/string/byte/strtol
  long arg = strtol(argv[1], &string_end, 10);
  // Error out if:
  // - an invalid character was found before the end of the string
  // - overflow or underflow errors occurred
  if (*string_end != '\0' || errno != 0) {
    printf("ERROR: Invalid character %s\n", argv[1]);
    return -1;
  }
  // Check that the number is within the limited capacity of an int
  if (arg < INT_MIN || arg > INT_MAX) {
    printf("ERROR: Integer value is out of bounds");
    return -1;
  }

  return arg;
}

char* generate_log_filepath() {
  char *home_dir = getenv("HOME");
  char *log_filepath = malloc(strlen(home_dir) + strlen(LOG_FILENAME) + 1);
  strcpy(log_filepath, home_dir);
  strcat(log_filepath, LOG_FILENAME);
  return log_filepath;
}

hid_device* open_device() {
  int current_retry = 1;
  // Open the device using the VID, PID,
  hid_device *device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
  printf("HID message: %ls\n", hid_error(device));

  while (!device) {
    usleep(50000);
    device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
    printf("HID message: %ls\n", hid_error(device));
    if (current_retry > MAX_RETRIES) {
      printf("Unable to open device after %d retries\n", MAX_RETRIES);
      hid_close(device);
      return NULL;
    }
    printf("Open device retry: %d\n", current_retry);
    current_retry++;
  }

  return device;
}

void read_device_message(hid_device *device, unsigned char* buf, FILE *log_file) {
  int res;
  for (int i = 0; i < MAX_TIMEOUT; i++) {
    res = hid_read(device, buf, BUFFER_LENGTH);
    if (res != 0) {
      break;
    }
    usleep(1000);
  }

  if (res < 0) {
    printf("Unable to read()\n");
    printf("Error: %ls\n", hid_error(device));
    log_message(DEVICE_READ_FAIL_MESSAGE, log_file);
  } else {
    log_out_read_message(buf[1], log_file);
  }
}

void log_out_read_message(int message, FILE *log_file) {
  switch (message) {
    case GAMING_MODE:
      log_message(GAMING_MODE_MESSAGE, log_file);
      break;
    case STENO_MODE:
      log_message(STENO_MODE_MESSAGE, log_file);
      break;
    default:
      log_message(HID_READ_BAD_VALUE_MESSAGE, log_file);
  }
}

void log_message(const char *message, FILE *log_file) {
  fwrite(message, 1, strlen(message), log_file);
}

void clean_up(char *log_filepath, FILE *log_file) {
  fclose(log_file);
  free(log_filepath);
  hid_exit();
}
