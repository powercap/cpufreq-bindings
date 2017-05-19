/**
 * Bindings to cpufreq in Linux sysfs.
 *
 * @author Connor Imes
 * @date 2017-03-16
 */
// for pread, pwrite, strtok_r
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cpufreq-bindings.h"
#include "cpufreq-bindings-common.h"

#define FILE_AFFECTED_CPUS "affected_cpus"
#define FILE_BIOS_LIMIT "bios_limit"
#define FILE_CPUINFO_CUR_FREQ "cpuinfo_cur_freq"
#define FILE_CPUINFO_MAX_FREQ "cpuinfo_max_freq"
#define FILE_CPUINFO_MIN_FREQ "cpuinfo_min_freq"
#define FILE_CPUINFO_TRANSITION_LATENCY "cpuinfo_transition_latency"
#define FILE_RELATED_CPUS "related_cpus"
#define FILE_SCALING_AVAILABLE_FREQUENCIES "scaling_available_frequencies"
#define FILE_SCALING_AVAILABLE_GOVERNORS "scaling_available_governors"
#define FILE_SCALING_CUR_FREQ "scaling_cur_freq"
#define FILE_SCALING_DRIVER "scaling_driver"
#define FILE_SCALING_GOVERNOR "scaling_governor"
#define FILE_SCALING_MAX_FREQ "scaling_max_freq"
#define FILE_SCALING_MIN_FREQ "scaling_min_freq"
#define FILE_SCALING_SETSPEED "scaling_setspeed"

#define U32_MAX_LEN 12

// (hopefully) conservative length estimate without being absurd
#define GOVERNOR_NAME_MAX_LEN 128

static int cpufreq_bindings_open_file(const char* name, uint32_t core, int flags) {
  char file[128];
  snprintf(file, sizeof(file), "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/%s", core, name);
  int fd = open(file, flags);
  if (fd < 0) {
    PERROR(ERROR, file);
  }
  return fd;
}

static void conditional_close(int cond, int fd) {
  int err_save = errno;
  if (cond && close(fd)) {
    PERROR(WARN, "conditional_close: close");
  }
  errno = err_save;
}

static ssize_t read_file_by_fd_or_name(int fd, uint32_t core, char* buf, size_t len, const char* name, int trim) {
  ssize_t ret;
  int local_fd = fd <= 0;
  if (local_fd) {
    if ((fd = cpufreq_bindings_open_file(name, core, O_RDONLY)) <= 0) {
      return -1;
    }
  }
  ret = pread(fd, buf, len, 0);
  if (ret <= 0) {
    if (ret == 0) {
      errno = ENODATA;
    }
    PERROR(ERROR, "read_file_by_fd_or_name: pread");
  } else if (trim) {
    // strip newline character
    buf[strcspn(buf, "\n")] = '\0';
  }
  conditional_close(local_fd, fd);
  return ret;
}

static ssize_t write_file_by_fd_or_name(int fd, uint32_t core, const char* buf, size_t len, const char* name) {
  ssize_t ret;
  int local_fd = fd <= 0;
  if (local_fd) {
    if ((fd = cpufreq_bindings_open_file(name, core, O_RDWR)) <= 0) {
      return -1;
    }
  }
  if ((ret = pwrite(fd, buf, len, 0)) < 0) {
    PERROR(ERROR, "write_file_by_fd_or_name: pwrite");
  }
  conditional_close(local_fd, fd);
  return ret;
}

static uint32_t read_file_u32arr(int fd, uint32_t core, uint32_t* arr, uint32_t len, const char* name) {
  char* tok;
  char* ptr = NULL;
  uint32_t i = 0;
  // support string values for every core + a whitespace char + a terminating NULL char
  size_t buflen = len * (U32_MAX_LEN + 1) + 1;
  char* buf = calloc(1, buflen);
  if (buf != NULL && read_file_by_fd_or_name(fd, core, buf, buflen, name, 0) > 0) {
    // tokenize and write to the array
    tok = strtok_r(buf, " ", &ptr);
    for (; tok != NULL && i < len; tok = strtok_r(NULL, " ", &ptr), i++) {
      errno = 0;
      arr[i] = strtoul(tok, NULL, 0);
      if (errno) {
        PERROR(ERROR, "read_file_u32arr: strtoul");
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
  free(buf);
  return i;
}

static uint32_t read_file_u32(int fd, uint32_t core, const char* name) {
  uint32_t ret = 0;
  char buf[U32_MAX_LEN];
  if (read_file_by_fd_or_name(fd, core, buf, sizeof(buf), name, 0) > 0) {
    errno = 0;
    ret = strtoul(buf, NULL, 0);
    if (errno) {
      PERROR(ERROR, "read_file_u32: strtoul");
    }
  }
  return ret;
}

static ssize_t write_file_u32(int fd, uint32_t core, uint32_t val, const char* name) {
  char buf[U32_MAX_LEN];
  snprintf(buf, sizeof(buf), "%"PRIu32, val);
  return write_file_by_fd_or_name(fd, core, buf, sizeof(buf), name);
}

static const char* cpufreq_bindings_file_to_name(cpufreq_bindings_file file) {
  switch (file) {
    case CPUFREQ_BINDINGS_FILE_AFFECTED_CPUS:
      return FILE_AFFECTED_CPUS;
    case CPUFREQ_BINDINGS_FILE_BIOS_LIMIT:
      return FILE_BIOS_LIMIT;
    case CPUFREQ_BINDINGS_FILE_CPUINFO_CUR_FREQ:
      return FILE_CPUINFO_CUR_FREQ;
    case CPUFREQ_BINDINGS_FILE_CPUINFO_MAX_FREQ:
      return FILE_CPUINFO_MAX_FREQ;
    case CPUFREQ_BINDINGS_FILE_CPUINFO_MIN_FREQ:
      return FILE_CPUINFO_MIN_FREQ;
    case CPUFREQ_BINDINGS_FILE_CPUINFO_TRANSITION_LATENCY:
      return FILE_CPUINFO_TRANSITION_LATENCY;
    case CPUFREQ_BINDINGS_FILE_RELATED_CPUS:
      return FILE_RELATED_CPUS;
    case CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_FREQUENCIES:
      return FILE_SCALING_AVAILABLE_FREQUENCIES;
    case CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_GOVERNORS:
      return FILE_SCALING_AVAILABLE_GOVERNORS;
    case CPUFREQ_BINDINGS_FILE_SCALING_CUR_FREQ:
      return FILE_SCALING_CUR_FREQ;
    case CPUFREQ_BINDINGS_FILE_SCALING_DRIVER:
      return FILE_SCALING_DRIVER;
    case CPUFREQ_BINDINGS_FILE_SCALING_GOVERNOR:
      return FILE_SCALING_GOVERNOR;
    case CPUFREQ_BINDINGS_FILE_SCALING_MAX_FREQ:
      return FILE_SCALING_MAX_FREQ;
    case CPUFREQ_BINDINGS_FILE_SCALING_MIN_FREQ:
      return FILE_SCALING_MIN_FREQ;
    case CPUFREQ_BINDINGS_FILE_SCALING_SETSPEED:
      return FILE_SCALING_SETSPEED;
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
  const char* name = cpufreq_bindings_file_to_name(file);
  if (name == NULL) {
    return -1;
  }
  if (flags < 0) {
    flags = cpufreq_bindings_file_to_flags(file);
  }
  return cpufreq_bindings_open_file(name, core, flags);
}

int cpufreq_bindings_file_close(int fd) {
  return close(fd);
}

uint32_t cpufreq_bindings_get_affected_cpus(int fd, uint32_t core, uint32_t* affected, uint32_t len) {
  return read_file_u32arr(fd, core, affected, len, FILE_AFFECTED_CPUS);
}

uint32_t cpufreq_bindings_get_bios_limit(int fd, uint32_t core) {
  return read_file_u32(fd, core, FILE_BIOS_LIMIT);
}

uint32_t cpufreq_bindings_get_cpuinfo_cur_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, FILE_CPUINFO_CUR_FREQ);
}

uint32_t cpufreq_bindings_get_cpuinfo_max_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, FILE_CPUINFO_MAX_FREQ);
}

uint32_t cpufreq_bindings_get_cpuinfo_min_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, FILE_CPUINFO_MIN_FREQ);
}

uint32_t cpufreq_bindings_get_cpuinfo_transition_latency(int fd, uint32_t core) {
  return read_file_u32(fd, core, FILE_CPUINFO_TRANSITION_LATENCY);
}

uint32_t cpufreq_bindings_get_related_cpus(int fd, uint32_t core, uint32_t* related, uint32_t len) {
  return read_file_u32arr(fd, core, related, len, FILE_RELATED_CPUS);
}

uint32_t cpufreq_bindings_get_scaling_available_frequencies(int fd, uint32_t core, uint32_t* freqs, uint32_t len) {
  return read_file_u32arr(fd, core, freqs, len, FILE_SCALING_AVAILABLE_FREQUENCIES);
}

uint32_t cpufreq_bindings_get_scaling_available_governors(int fd, uint32_t core, char* governors, size_t len, size_t width) {
  char* ptr;
  char* tok;
  uint32_t i = 0;
  // support string values for every governor + a whitespace char + a terminating NULL char
  size_t buflen = len * (GOVERNOR_NAME_MAX_LEN + 1) + 1;
  char* buf = calloc(1, buflen);
  if (buf != NULL && read_file_by_fd_or_name(fd, core, buf, buflen, FILE_SCALING_AVAILABLE_GOVERNORS, 1) > 0) {
    tok = strtok_r(buf, " ", &ptr);
    for (; tok != NULL && i < len; tok = strtok_r(NULL, " ", &ptr), i++) {
      strncpy(&governors[i * width], tok, width);
    }
    if (tok != NULL) {
      // the array wasn't big enough
      i = 0;
      errno = ERANGE;
    }
  }
  free(buf);
  return i;
}

uint32_t cpufreq_bindings_get_scaling_cur_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, FILE_SCALING_CUR_FREQ);
}

ssize_t cpufreq_bindings_get_scaling_driver(int fd, uint32_t core, char* driver, size_t len) {
  return read_file_by_fd_or_name(fd, core, driver, len, FILE_SCALING_DRIVER, 1);
}

ssize_t cpufreq_bindings_get_scaling_governor(int fd, uint32_t core, char* governor, size_t len) {
  return read_file_by_fd_or_name(fd, core, governor, len, FILE_SCALING_GOVERNOR, 1);
}

ssize_t cpufreq_bindings_set_scaling_governor(int fd, uint32_t core, const char* governor, size_t len) {
  return write_file_by_fd_or_name(fd, core, governor, len, FILE_SCALING_GOVERNOR);
}

uint32_t cpufreq_bindings_get_scaling_max_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, FILE_SCALING_MAX_FREQ);
}

ssize_t cpufreq_bindings_set_scaling_max_freq(int fd, uint32_t core, uint32_t freq) {
  return write_file_u32(fd, core, freq, FILE_SCALING_MAX_FREQ);
}

uint32_t cpufreq_bindings_get_scaling_min_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, FILE_SCALING_MIN_FREQ);
}

ssize_t cpufreq_bindings_set_scaling_min_freq(int fd, uint32_t core, uint32_t freq) {
  return write_file_u32(fd, core, freq, FILE_SCALING_MIN_FREQ);
}

ssize_t cpufreq_bindings_set_scaling_setspeed(int fd, uint32_t core, uint32_t freq) {
  return write_file_u32(fd, core, freq, FILE_SCALING_SETSPEED);
}
