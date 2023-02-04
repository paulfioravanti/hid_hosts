#include <hidapi.h> // hid_*

// VID and PID for Georgi
// REF: https://github.com/qmk/qmk_firmware/blob/master/keyboards/gboards/georgi/config.h
#define VENDOR_ID  0xFEED
#define PRODUCT_ID 0x1337

enum {
  BUFFER_LENGTH = 2,
  ENABLE_NONBLOCKING = 1,
  GAMING_MODE = 3,
  HID_DARWIN_NON_EXCLUSIVE_MODE = 0,
  HID_OPEN_SLEEP_MICROSECONDS = 15000, // 15 ms
  HID_READ_SLEEP_MICROSECONDS = 100000, // 100 ms
  MAX_HID_OPEN_RETRIES = 30,
  MAX_HID_READ_RETRIES = 4,
  NO_ACTION_TAKEN = 9,
  STENO_MODE = 4
};

// REF: https://github.com/rabbitgrowth/plover-tapey-tape
static const char LOG_FILENAME[] =
  "/Library/Application Support/plover/tapey_tape.txt";
static const char HID_INIT_FAIL_MESSAGE[] =
  "ERROR |       ლ(ಠ_ಠ ლ)        | Unable to initialize HID API library\n";
static const char DEVICE_OPEN_FAIL_MESSAGE[] =
  "ERROR |   💢( ╯°□°)╯ ┻━━┻     | Failed to open HID device\n";
static const char DEVICE_WRITE_FAIL_MESSAGE[] =
  "ERROR |        (＃`Д´)        | Unable to write to HID device\n";
static const char DEVICE_READ_FAIL_MESSAGE[] =
  "ERROR |   💢(╯°益°)╯彡┻━┻     | Unable to read from HID device\n";
static const char HID_READ_BAD_VALUE_MESSAGE[] =
  "ERROR |        (￣ω￣;)       | Unexpected value received from HID device\n";
static const char MODE_UNCHANGED_MESSAGE[] =
  "ERROR |        ლ(¯ロ¯ლ)       | Attempted mode change unsuccessful\n";
static const char GAMING_MODE_MESSAGE[] =
  "GAMING|🎮(❁´ω`❁)　✧٩(ˊωˋ*)و✧🎮| GAMING mode activated!\n";
static const char STENO_MODE_MESSAGE[] =
  "STENO |⌨️ ｷﾀ━━━━━(ﾟ∀ﾟ)━━━━━!!⌨️ | STENO mode activated!\n";

long parse_arguments(int argc, char* argv[]);
char* generate_log_filepath();
hid_device* open_device();
void read_device_message(hid_device *device, unsigned char* buf, FILE *log_file);
void log_message(const char *message, FILE *log_file);
void log_out_read_message(int message, FILE *log_file);
void clean_up(char *log_filepath, FILE *log_file);
void print_buffer(unsigned char* buf);
