#include "hid_host.h"

int main(int argc, char* argv[]) {
  long arg = parse_arguments(argc, argv);
  if (arg == -1) {
    return -1;
  }

  char *log_filepath = generate_log_filepath();
  FILE *log_file = fopen(log_filepath, "a");
  if (!log_file) {
    printf("ERROR: Unable to open log file\n");
    free(log_filepath);
    return -1;
  }

  srand(time(NULL));
  const char *error_emoji =
    get_random_emoji_string(ERROR_EMOJIS, NUM_ERROR_EMOJIS);
  const char *message;

  // Initialize the hidapi library
  int res = hid_init();
  if (res < 0) {
    printf("Unable to initialize HIDAPI library\n");
    message =
      build_log_message(ERROR_HEADER, error_emoji, HID_INIT_FAIL_MESSAGE);
    log_message(message, log_file);
    clean_up(log_filepath, log_file);
    return -1;
  }
  // REF: https://github.com/libusb/hidapi/blob/master/hidtest/test.c#L102
  // REF: https://github.com/libusb/hidapi/blob/master/mac/hidapi_darwin.h#L68
  hid_darwin_set_open_exclusive(HID_DARWIN_NON_EXCLUSIVE_MODE);

  hid_device *device = open_device();
  if (!device) {
    message =
      build_log_message(ERROR_HEADER, error_emoji, DEVICE_OPEN_FAIL_MESSAGE);
    log_message(message, log_file);
    clean_up(log_filepath, log_file);
    return -1;
  }

  unsigned char buf[BUFFER_LENGTH];
  memset(buf, 0, sizeof(buf));
  buf[0] = arg;

  res = hid_write(device, buf, BUFFER_LENGTH);

  if (res < 0) {
    printf("Unable to write()\n");
    printf("Error: %ls\n", hid_error(device));
    message =
      build_log_message(ERROR_HEADER, error_emoji, DEVICE_WRITE_FAIL_MESSAGE);
    log_message(message, log_file);
  } else {
    hid_set_nonblocking(device, ENABLE_NONBLOCKING);
    read_device_message(device, buf, log_file, error_emoji);
  }

  hid_close(device);
  clean_up(log_filepath, log_file);
  return 0;
}

long parse_arguments(int argc, char* argv[]) {
  // Requires only one argument
  // REF: https://stackoverflow.com/questions/9748393/how-can-i-get-argv-as-int
  if (argc != 2 || strlen(argv[1]) == 0) {
    printf("ERROR: Must provide a single non-empty argument\n");
    return -1;
  }

  char* end_ptr;
  errno = 0;
  // REF: https://devdocs.io/c/string/byte/strtol
  long arg = strtol(argv[1], &end_ptr, 10);

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

char* generate_log_filepath() {
  char *home_dir = getenv("HOME");
  char *log_filepath = malloc(strlen(home_dir) + strlen(LOG_FILENAME) + 1);
  strcpy(log_filepath, home_dir);
  strcat(log_filepath, LOG_FILENAME);
  return log_filepath;
}

hid_device* open_device() {
  int num_open_retries = 0;
  hid_device *device = NULL;

  while (!device && num_open_retries < MAX_HID_OPEN_RETRIES) {
    printf("Attempting to open()...\n");
    device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);

    if (device) {
      break;
    }

    printf("HID message: %ls\n", hid_error(device));
    printf("Device open retries: %d\n", ++num_open_retries);
    usleep(HID_OPEN_SLEEP_MICROSECONDS);
  }

  if (!device) {
    printf("Unable to open device after %d retries\n", MAX_HID_OPEN_RETRIES);
    hid_close(device);
    return NULL;
  }

  return device;
}

void read_device_message(hid_device *device, unsigned char* buf, FILE *log_file, const char *error_emoji) {
  int res = 0;
  int num_read_retries = 0;
  const char *message;

  while (res == 0 && num_read_retries < MAX_HID_READ_RETRIES) {
    res = hid_read(device, buf, BUFFER_LENGTH);

    if (res == 0) {
      printf("Waiting to read()...\n");
      usleep(HID_READ_SLEEP_MICROSECONDS);
      num_read_retries++;
      continue;
    }

    if (res < 0) {
      printf("Error: Unable to read()\n");
      printf("HID message: %ls\n", hid_error(device));
      print_buffer(buf);
      message =
        build_log_message(ERROR_HEADER, error_emoji, DEVICE_READ_FAIL_MESSAGE);
      log_message(message, log_file);
      break;
    }
  }

  if (res == 0) {
    printf("Unable to read device after %d retries\n", MAX_HID_READ_RETRIES);
    printf("hid_read result was: %d\n", res);
    print_buffer(buf);
    message =
      build_log_message(ERROR_HEADER, error_emoji, DEVICE_READ_FAIL_MESSAGE);
    log_message(message, log_file);
  } else if (res > 0) {
    printf("HID message: %ls\n", hid_error(device));
    log_out_read_message(buf[1], log_file, error_emoji);
  }
}

void log_out_read_message(int read_message, FILE *log_file, const char *error_emoji) {
  const char *read_message_log_message;
  const char *emoji;
  const char *header;
  const char *message;

  switch (read_message) {
    case GAMING_MODE:
      header = GAMING_HEADER;
      emoji =
        get_random_emoji_string(GAMING_MODE_EMOJIS, NUM_GAMING_MODE_EMOJIS);
      message = GAMING_MODE_MESSAGE;
      break;
    case STENO_MODE:
      header = STENO_HEADER;
      emoji = get_random_emoji_string(STENO_MODE_EMOJIS, NUM_STENO_MODE_EMOJIS);
      message = STENO_MODE_MESSAGE;
      break;
    case NO_ACTION_TAKEN:
      header = ERROR_HEADER;
      emoji = error_emoji;
      message = MODE_UNCHANGED_MESSAGE;
      break;
    default:
      printf("Message read from device: %d\n", read_message);
      header = ERROR_HEADER;
      emoji = error_emoji;
      message = HID_READ_BAD_VALUE_MESSAGE;
  }

  read_message_log_message = build_log_message(header, emoji, message);
  log_message(read_message_log_message, log_file);
}

char* build_log_message(const char *header, const char *emoji, const char *message) {
  static char log_msg[MAX_MESSAGE_LENGTH];
  snprintf(
    log_msg,
    MAX_MESSAGE_LENGTH,
    "%s%s%s%s%s",
    header,
    SEPARATOR,
    emoji,
    SEPARATOR,
    message
  );
  return log_msg;
}

void log_message(const char *message, FILE *log_file) {
  fwrite(message, 1, strlen(message), log_file);
}

void clean_up(char *log_filepath, FILE *log_file) {
  fclose(log_file);
  free(log_filepath);
  hid_exit();
}

void print_buffer(unsigned char* buf) {
  for (int i = 0; i < BUFFER_LENGTH; i++)
    printf("%02x ", (unsigned int) buf[i]);
}

const char* get_random_emoji_string(const char * const collection[], int num_elements) {
  int random_index = rand() % num_elements;
  return collection[random_index];
}
