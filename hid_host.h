#include <errno.h>         // errno
#include <hidapi.h>        // hid_close, hid_device, hid_error, hid_exit,
                           // hid_init, hid_read, hid_write, open_device
#include <hidapi_darwin.h> // hid_darwin_set_open_exclusive
#include <limits.h>        // INT_MAX, INT_MIN
#include <stdio.h>         // FILE, fclose, fopen, fwrite, printf, snprintf
#include <stdlib.h>        // free, getenv, malloc, rand, srand, strtol
#include <string.h>        // memset, strcat, strcpy, strlen
#include <time.h>          // time
#include <unistd.h>        // usleep

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
  MAX_MESSAGE_LENGTH = 105,
  NO_ACTION_TAKEN = 0x9,
  STENO_MODE = 0x4
};

// REF: https://github.com/rabbitgrowth/plover-tapey-tape
static const char LOG_FILENAME[] =
  "/Library/Application Support/plover/tapey_tape.txt";
static const char SEPARATOR[] = "|";
static const char ERROR_HEADER[] = "ERROR ";
static const char GAMING_HEADER[] = "GAMING";
static const char STENO_HEADER[] = "STENO ";
// REF: http://kaomoji.ru/en/
static const char * const ERROR_EMOJIS[] = {
  "💢     ლ(ಠ_ಠ ლ)      💢",
  "💢  ( ╯°□°)╯ ┻━━┻    💢",
  "💢     (＃`Д´)       💢",
  "💢  (╯°益°)╯彡┻━┻    💢",
  "💢     (￣ω￣;)      💢",
  "💢     ლ(¯ロ¯ლ)      💢",
  "💢     (￢_￢)       💢",
  "💢.｡･ﾟﾟ･(＞_＜)･ﾟﾟ･｡.💢",
  "💢     Σ(▼□▼メ)      💢",
  "💢    ٩(╬ʘ益ʘ╬)۶     💢",
  "💢   ୧((#Φ益Φ#))୨    💢",
  "💢    ლ(¯ロ¯\"ლ)      💢",
};
static const int NUM_ERROR_EMOJIS =
  sizeof(ERROR_EMOJIS) / sizeof(ERROR_EMOJIS[0]);
static const char * const GAMING_MODE_EMOJIS[] = {
  "🎮(❁´ω`❁)　✧٩(ˊωˋ*)و✧🎮",
  "🎮 ヽ( ⌒o⌒)人(⌒-⌒ )ﾉ 🎮",
  "🎮ヽ( ⌒ω⌒)人(=^‥^= )ﾉ🎮",
  "🎮ヽ(≧◡≦)八(o^ ^o)ノ 🎮",
  "🎮(*・∀・)爻(・∀・*) 🎮",
  "🎮 (っ˘▽˘)(˘▽˘)˘▽˘ς) 🎮",
  "🎮((*°▽°*)八(*°▽°*)) 🎮",
  "🎮(*＾ω＾)人(＾ω＾*) 🎮",
  "🎮 ٩(๑･ิᴗ･ิ)۶٩(･ิᴗ･ิ๑)۶  🎮",
};
static const int NUM_GAMING_MODE_EMOJIS =
  sizeof(GAMING_MODE_EMOJIS) / sizeof(GAMING_MODE_EMOJIS[0]);
static const char * const STENO_MODE_EMOJIS[] = {
  "⌨️ ｷﾀ━━━━━(ﾟ∀ﾟ)━━━━━!!⌨️ ",
  "⌨️ ☆*:.｡.o(≧▽≦)o.｡.:*☆⌨️ ",
  "⌨️ (￣(￣(￣▽￣)￣)￣)⌨️ ",
  "⌨️  (☞°ヮ°)☞ ☜(°ヮ°☜) ⌨️ ",
  "⌨️ ﾉ≧∀≦)ﾉ ‥…━━━━━━━━━★⌨️ ",
  "⌨️ (ﾉ>ω<)ﾉ’★,｡･:*:･ﾟ’☆⌨️ ",
  "⌨️ (ノ°∀°)ノ⌒.｡.:*゜*☆⌨️ ",
  "⌨️ ╰( ͡° ͜ʖ ͡° )つ─☆*:・ﾟ⌨️ ",
};
static const int NUM_STENO_MODE_EMOJIS =
  sizeof(STENO_MODE_EMOJIS) / sizeof(STENO_MODE_EMOJIS[0]);
static const char HID_INIT_FAIL_MESSAGE[] =
  " Unable to initialize HID library!\n";
static const char DEVICE_OPEN_FAIL_MESSAGE[] =
  " Couldn't OPEN device!\n";
static const char DEVICE_WRITE_FAIL_MESSAGE[] =
  " Couldn't WRITE to device!\n";
static const char DEVICE_READ_FAIL_MESSAGE[] =
  " Couldn't READ from device!\n";
static const char HID_READ_BAD_VALUE_MESSAGE[] =
  " Unexpected response from device!\n";
static const char MODE_UNCHANGED_MESSAGE[] =
  " Attempted mode change unsuccessful!\n";
static const char GAMING_MODE_MESSAGE[] = " GAMING mode activated!\n";
static const char STENO_MODE_MESSAGE[] = " STENO mode activated!\n";

int parse_arguments(int argc, char* argv[]);
char* generate_log_filepath();
hid_device* open_device();
void read_device_message(hid_device *device, unsigned char* buf, FILE *log_file, const char *error_emoji);
void log_message(const char *message, FILE *log_file);
void log_out_read_message(int message, FILE *log_file, const char *error_emoji);
void clean_up(char *log_filepath, FILE *log_file);
void print_buffer(unsigned char* buf);
char* build_log_message(const char *header, const char *emoji, const char *message);
const char* get_random_emoji_string(const char * const collection[], int num_elements);
