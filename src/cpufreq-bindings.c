/**
 * Bindings to cpufreq in Linux sysfs.
 *
 * @author Connor Imes
 * @date 2017-03-16
 */
#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cpufreq-bindings.h"

#define TEMPLATE_AFFECTED_CPUS "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/affected_cpus"
#define TEMPLATE_BIOS_LIMIT "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/bios_limit"
#define TEMPLATE_CPUINFO_CUR_FREQ "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/cpuinfo_cur_freq"
#define TEMPLATE_CPUINFO_MAX_FREQ "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/cpuinfo_max_freq"
#define TEMPLATE_CPUINFO_MIN_FREQ "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/cpuinfo_min_freq"
#define TEMPLATE_CPUINFO_TRANSITION_LATENCY "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/cpuinfo_transition_latency"
#define TEMPLATE_RELATED_CPUS "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/related_cpus"
#define TEMPLATE_SCALING_AVAILABLE_FREQUENCIES "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/scaling_available_frequencies"
#define TEMPLATE_SCALING_AVAILABLE_GOVERNORS "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/scaling_available_governors"
#define TEMPLATE_SCALING_CUR_FREQ "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/scaling_cur_freq"
#define TEMPLATE_SCALING_DRIVER "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/scaling_driver"
#define TEMPLATE_SCALING_GOVERNOR "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/scaling_governor"
#define TEMPLATE_SCALING_MAX_FREQ "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/scaling_max_freq"
#define TEMPLATE_SCALING_MIN_FREQ "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/scaling_min_freq"
#define TEMPLATE_SCALING_SETSPEED "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/scaling_setspeed"

#define TEMPLATE_MAX_LEN 128
#define U32_MAX_LEN 12

// (hopefully) conservative length estimate without being absurd
#define GOVERNOR_NAME_MAX_LEN 128

static int cpufreq_bindings_open_file_template(const char* file_template, uint32_t core, int flags) {
  char file[TEMPLATE_MAX_LEN];
  snprintf(file, sizeof(file), file_template, core);
  int fd = open(file, flags);
#ifdef VERBOSE
  if (fd < 0) {
    perror(file);
  }
#endif
  return fd;
}

static void conditional_close(int cond, int fd) {
  int err_save = errno;
  if (cond) {
    close(fd);
#ifdef VERBOSE
    perror("close");
#endif
  }
  errno = err_save;
}

static ssize_t read_file_str_template(int fd, uint32_t core, char* buf, size_t len, const char* template) {
  ssize_t ret;
  int local_fd = fd <= 0;
  if (local_fd) {
    if ((fd = cpufreq_bindings_open_file_template(template, core, O_RDONLY)) <= 0) {
      return -1;
    }
  }
  ret = pread(fd, buf, len, 0);
  conditional_close(local_fd, fd);
  // strip newline character
  buf[strcspn(buf, "\n")] = '\0';
  return ret;
}

static ssize_t write_file_str_template(int fd, uint32_t core, const char* buf, size_t len, const char* template) {
  ssize_t ret;
  int local_fd = fd <= 0;
  if (local_fd) {
    if ((fd = cpufreq_bindings_open_file_template(template, core, O_RDWR)) <= 0) {
      return -1;
    }
  }
  ret = pwrite(fd, buf, len, 0);
  conditional_close(local_fd, fd);
  return ret;
}

static uint32_t read_file_u32arr_template(int fd, uint32_t core, uint32_t* arr, uint32_t len, const char* template) {
  uint32_t i = 0;
  // support string values for every core + a whitespace char + a terminating NULL char
  size_t buflen = len * (U32_MAX_LEN + 1) + 1;
  char buf[buflen];
  char* tok;
  char* ptr = NULL;
  int local_fd = fd <= 0;
  if (local_fd) {
    if ((fd = cpufreq_bindings_open_file_template(template, core, O_RDONLY)) <= 0) {
      return 0;
    }
  }
  if (pread(fd, buf, buflen, 0) > 0) {
    if (len == 1) {
      errno = 0;
      // optimization - work directly on the buffer rather than tokenizing
      arr[i] = strtoul(buf, NULL, 0);
      if (!errno) {
        i++;
      }
    } else {
      // tokenize and write to the array
      tok = strtok_r(buf, " ", &ptr);
      for (; tok != NULL && i < len; tok = strtok_r(NULL, " ", &ptr), i++) {
        errno = 0;
        arr[i] = strtoul(tok, NULL, 0);
        if (errno) {
          i = 0;
          tok = NULL;
          break;
        }
      }
      if (tok != NULL) {
        // the array wasn't big enough
        i = 0;
        errno = ERANGE;
      }
    }
  }
  conditional_close(local_fd, fd);
  return i;
}

static uint32_t read_file_u32_template(int fd, uint32_t core, const char* template) {
  uint32_t ret = 0;
  read_file_u32arr_template(fd, core, &ret, 1, template);
  return ret;
}

static ssize_t write_file_u32_template(int fd, uint32_t core, uint32_t val, const char* template) {
  char buf[U32_MAX_LEN];
  ssize_t ret;
  int local_fd = fd <= 0;
  if (local_fd) {
    if ((fd = cpufreq_bindings_open_file_template(template, core, O_RDWR)) <= 0) {
      return -1;
    }
  }
  snprintf(buf, sizeof(buf), "%"PRIu32, val);
  ret = pwrite(fd, buf, sizeof(buf), 0);
  conditional_close(local_fd, fd);
  return ret;
}

static const char* cpufreq_bindings_file_to_template(cpufreq_bindings_file file) {
  switch (file) {
    case CPUFREQ_BINDINGS_FILE_AFFECTED_CPUS:
      return TEMPLATE_AFFECTED_CPUS;
    case CPUFREQ_BINDINGS_FILE_BIOS_LIMIT:
      return TEMPLATE_BIOS_LIMIT;
    case CPUFREQ_BINDINGS_FILE_CPUINFO_CUR_FREQ:
      return TEMPLATE_CPUINFO_CUR_FREQ;
    case CPUFREQ_BINDINGS_FILE_CPUINFO_MAX_FREQ:
      return TEMPLATE_CPUINFO_MAX_FREQ;
    case CPUFREQ_BINDINGS_FILE_CPUINFO_MIN_FREQ:
      return TEMPLATE_CPUINFO_MIN_FREQ;
    case CPUFREQ_BINDINGS_FILE_CPUINFO_TRANSITION_LATENCY:
      return TEMPLATE_CPUINFO_TRANSITION_LATENCY;
    case CPUFREQ_BINDINGS_FILE_RELATED_CPUS:
      return TEMPLATE_RELATED_CPUS;
    case CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_FREQUENCIES:
      return TEMPLATE_SCALING_AVAILABLE_FREQUENCIES;
    case CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_GOVERNORS:
      return TEMPLATE_SCALING_AVAILABLE_GOVERNORS;
    case CPUFREQ_BINDINGS_FILE_SCALING_CUR_FREQ:
      return TEMPLATE_SCALING_CUR_FREQ;
    case CPUFREQ_BINDINGS_FILE_SCALING_DRIVER:
      return TEMPLATE_SCALING_DRIVER;
    case CPUFREQ_BINDINGS_FILE_SCALING_GOVERNOR:
      return TEMPLATE_SCALING_GOVERNOR;
    case CPUFREQ_BINDINGS_FILE_SCALING_MAX_FREQ:
      return TEMPLATE_SCALING_MAX_FREQ;
    case CPUFREQ_BINDINGS_FILE_SCALING_MIN_FREQ:
      return TEMPLATE_SCALING_MIN_FREQ;
    case CPUFREQ_BINDINGS_FILE_SCALING_SETSPEED:
      return TEMPLATE_SCALING_SETSPEED;
    default:
      break;
  }
  errno = EINVAL;
  return NULL;
}

static int cpufreq_bindings_file_to_flags(cpufreq_bindings_file file) {
  switch (file) {
    case CPUFREQ_BINDINGS_FILE_SCALING_GOVERNOR:
    case CPUFREQ_BINDINGS_FILE_SCALING_MAX_FREQ:
    case CPUFREQ_BINDINGS_FILE_SCALING_MIN_FREQ:
    case CPUFREQ_BINDINGS_FILE_SCALING_SETSPEED:
      return O_RDWR;
    default:
      break;
  }
  return O_RDONLY;
}

int cpufreq_bindings_file_open(uint32_t core, cpufreq_bindings_file file, int flags) {
  const char* template = cpufreq_bindings_file_to_template(file);
  if (template == NULL) {
    return -1;
  }
  if (flags < 0) {
    flags = cpufreq_bindings_file_to_flags(file);
  }
  return cpufreq_bindings_open_file_template(template, core, flags);
}

int cpufreq_bindings_file_close(int fd) {
  return close(fd);
}

uint32_t cpufreq_bindings_get_affected_cpus(int fd, uint32_t core, uint32_t* affected, uint32_t len) {
  return read_file_u32arr_template(fd, core, affected, len, TEMPLATE_AFFECTED_CPUS);
}

uint32_t cpufreq_bindings_get_bios_limit(int fd, uint32_t core) {
  return read_file_u32_template(fd, core, TEMPLATE_BIOS_LIMIT);
}

uint32_t cpufreq_bindings_get_cpuinfo_cur_freq(int fd, uint32_t core) {
  return read_file_u32_template(fd, core, TEMPLATE_CPUINFO_CUR_FREQ);
}

uint32_t cpufreq_bindings_get_cpuinfo_max_freq(int fd, uint32_t core) {
  return read_file_u32_template(fd, core, TEMPLATE_CPUINFO_MAX_FREQ);
}

uint32_t cpufreq_bindings_get_cpuinfo_min_freq(int fd, uint32_t core) {
  return read_file_u32_template(fd, core, TEMPLATE_CPUINFO_MIN_FREQ);
}

uint32_t cpufreq_bindings_get_cpuinfo_transition_latency(int fd, uint32_t core) {
  return read_file_u32_template(fd, core, TEMPLATE_CPUINFO_TRANSITION_LATENCY);
}

uint32_t cpufreq_bindings_get_related_cpus(int fd, uint32_t core, uint32_t* related, uint32_t len) {
  return read_file_u32arr_template(fd, core, related, len, TEMPLATE_RELATED_CPUS);
}

uint32_t cpufreq_bindings_get_scaling_available_frequencies(int fd, uint32_t core, uint32_t* freqs, uint32_t len) {
  return read_file_u32arr_template(fd, core, freqs, len, TEMPLATE_SCALING_AVAILABLE_FREQUENCIES);
}

uint32_t cpufreq_bindings_get_scaling_available_governors(int fd, uint32_t core, char* governors, size_t len, size_t width) {
  char buf[len * GOVERNOR_NAME_MAX_LEN];
  char* ptr;
  char* tok;
  uint32_t i = 0;
  if (read_file_str_template(fd, core, buf, sizeof(buf), TEMPLATE_SCALING_AVAILABLE_GOVERNORS) <= 0) {
    return 0;
  }
  tok = strtok_r(buf, " ", &ptr);
  for (; tok != NULL && i < len; tok = strtok_r(NULL, " ", &ptr), i++) {
    strncpy(&governors[i * width], tok, width);
  }
  if (tok != NULL) {
    // the array wasn't big enough
    i = 0;
    errno = ERANGE;
  }
  return i;
}

uint32_t cpufreq_bindings_get_scaling_cur_freq(int fd, uint32_t core) {
  return read_file_u32_template(fd, core, TEMPLATE_SCALING_CUR_FREQ);
}

ssize_t cpufreq_bindings_get_scaling_driver(int fd, uint32_t core, char* driver, size_t len) {
  return read_file_str_template(fd, core, driver, len, TEMPLATE_SCALING_DRIVER);
}

ssize_t cpufreq_bindings_get_scaling_governor(int fd, uint32_t core, char* governor, size_t len) {
  return read_file_str_template(fd, core, governor, len, TEMPLATE_SCALING_GOVERNOR);
}

ssize_t cpufreq_bindings_set_scaling_governor(int fd, uint32_t core, const char* governor, size_t len) {
  return write_file_str_template(fd, core, governor, len, TEMPLATE_SCALING_GOVERNOR);
}

uint32_t cpufreq_bindings_get_scaling_max_freq(int fd, uint32_t core) {
  return read_file_u32_template(fd, core, TEMPLATE_SCALING_MAX_FREQ);
}

ssize_t cpufreq_bindings_set_scaling_max_freq(int fd, uint32_t core, uint32_t freq) {
  return write_file_u32_template(fd, core, freq, TEMPLATE_SCALING_MAX_FREQ);
}

uint32_t cpufreq_bindings_get_scaling_min_freq(int fd, uint32_t core) {
  return read_file_u32_template(fd, core, TEMPLATE_SCALING_MIN_FREQ);
}

ssize_t cpufreq_bindings_set_scaling_min_freq(int fd, uint32_t core, uint32_t freq) {
  return write_file_u32_template(fd, core, freq, TEMPLATE_SCALING_MIN_FREQ);
}

ssize_t cpufreq_bindings_set_scaling_setspeed(int fd, uint32_t core, uint32_t freq) {
  return write_file_u32_template(fd, core, freq, TEMPLATE_SCALING_SETSPEED);
}
