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

static const char* BINDINGS_FILE[CPUFREQ_BINDINGS_FILE_SCALING_SETSPEED + 1] = {
  "affected_cpus",
  "bios_limit",
  "cpuinfo_cur_freq",
  "cpuinfo_max_freq",
  "cpuinfo_min_freq",
  "cpuinfo_transition_latency",
  "related_cpus",
  "scaling_available_frequencies",
  "scaling_available_governors",
  "scaling_cur_freq",
  "scaling_driver",
  "scaling_governor",
  "scaling_max_freq",
  "scaling_min_freq",
  "scaling_setspeed"
};

#define U32_MAX_LEN 12

// (hopefully) conservative length estimate without being absurd
#define GOVERNOR_NAME_MAX_LEN 128

static int cpufreq_bindings_open_file(cpufreq_bindings_file file, uint32_t core, int flags) {
  char buf[128];
  int fd;
  snprintf(buf, sizeof(buf), "/sys/devices/system/cpu/cpu%"PRIu32"/cpufreq/%s", core, BINDINGS_FILE[file]);
  fd = open(buf, flags);
  if (fd < 0) {
    PERROR(ERROR, buf);
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

static ssize_t read_file_by_fd_or_name(int fd, uint32_t core, char* buf, size_t len, cpufreq_bindings_file file, int trim) {
  ssize_t ret;
  int local_fd = fd <= 0;
  if (local_fd) {
    if ((fd = cpufreq_bindings_open_file(file, core, O_RDONLY)) <= 0) {
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

static ssize_t write_file_by_fd_or_name(int fd, uint32_t core, const char* buf, size_t len, cpufreq_bindings_file file) {
  ssize_t ret;
  int local_fd = fd <= 0;
  if (local_fd) {
    if ((fd = cpufreq_bindings_open_file(file, core, O_RDWR)) <= 0) {
      return -1;
    }
  }
  if ((ret = pwrite(fd, buf, len, 0)) < 0) {
    PERROR(ERROR, "write_file_by_fd_or_name: pwrite");
  }
  conditional_close(local_fd, fd);
  return ret;
}

static uint32_t read_file_u32arr(int fd, uint32_t core, uint32_t* arr, uint32_t len, cpufreq_bindings_file file) {
  char* tok;
  char* ptr = NULL;
  uint32_t i = 0;
  // support string values for every core + a whitespace char + a terminating NULL char
  size_t buflen = len * (U32_MAX_LEN + 1) + 1;
  char* buf = calloc(1, buflen);
  if (buf != NULL && read_file_by_fd_or_name(fd, core, buf, buflen, file, 0) > 0) {
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

static uint32_t read_file_u32(int fd, uint32_t core, cpufreq_bindings_file file) {
  uint32_t ret = 0;
  char buf[U32_MAX_LEN];
  if (read_file_by_fd_or_name(fd, core, buf, sizeof(buf), file, 0) > 0) {
    errno = 0;
    ret = strtoul(buf, NULL, 0);
    if (errno) {
      PERROR(ERROR, "read_file_u32: strtoul");
    }
  }
  return ret;
}

static ssize_t write_file_u32(int fd, uint32_t core, uint32_t val, cpufreq_bindings_file file) {
  char buf[U32_MAX_LEN];
  snprintf(buf, sizeof(buf), "%"PRIu32, val);
  return write_file_by_fd_or_name(fd, core, buf, sizeof(buf), file);
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
  if (file < 0 || file > CPUFREQ_BINDINGS_FILE_SCALING_SETSPEED) {
    errno = EINVAL;
    return -1;
  }
  if (flags < 0) {
    flags = cpufreq_bindings_file_to_flags(file);
  }
  return cpufreq_bindings_open_file(file, core, flags);
}

int cpufreq_bindings_file_close(int fd) {
  return close(fd);
}

uint32_t cpufreq_bindings_get_affected_cpus(int fd, uint32_t core, uint32_t* affected, uint32_t len) {
  return read_file_u32arr(fd, core, affected, len, CPUFREQ_BINDINGS_FILE_AFFECTED_CPUS);
}

uint32_t cpufreq_bindings_get_bios_limit(int fd, uint32_t core) {
  return read_file_u32(fd, core, CPUFREQ_BINDINGS_FILE_BIOS_LIMIT);
}

uint32_t cpufreq_bindings_get_cpuinfo_cur_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, CPUFREQ_BINDINGS_FILE_CPUINFO_CUR_FREQ);
}

uint32_t cpufreq_bindings_get_cpuinfo_max_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, CPUFREQ_BINDINGS_FILE_CPUINFO_MAX_FREQ);
}

uint32_t cpufreq_bindings_get_cpuinfo_min_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, CPUFREQ_BINDINGS_FILE_CPUINFO_MIN_FREQ);
}

uint32_t cpufreq_bindings_get_cpuinfo_transition_latency(int fd, uint32_t core) {
  return read_file_u32(fd, core, CPUFREQ_BINDINGS_FILE_CPUINFO_TRANSITION_LATENCY);
}

uint32_t cpufreq_bindings_get_related_cpus(int fd, uint32_t core, uint32_t* related, uint32_t len) {
  return read_file_u32arr(fd, core, related, len, CPUFREQ_BINDINGS_FILE_RELATED_CPUS);
}

uint32_t cpufreq_bindings_get_scaling_available_frequencies(int fd, uint32_t core, uint32_t* freqs, uint32_t len) {
  return read_file_u32arr(fd, core, freqs, len, CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_FREQUENCIES);
}

uint32_t cpufreq_bindings_get_scaling_available_governors(int fd, uint32_t core, char* governors, size_t len, size_t width) {
  char* ptr;
  char* tok;
  uint32_t i = 0;
  // support string values for every governor + a whitespace char + a terminating NULL char
  size_t buflen = len * (GOVERNOR_NAME_MAX_LEN + 1) + 1;
  char* buf = calloc(1, buflen);
  if (buf != NULL && read_file_by_fd_or_name(fd, core, buf, buflen, CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_GOVERNORS, 1) > 0) {
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
  return read_file_u32(fd, core, CPUFREQ_BINDINGS_FILE_SCALING_CUR_FREQ);
}

ssize_t cpufreq_bindings_get_scaling_driver(int fd, uint32_t core, char* driver, size_t len) {
  return read_file_by_fd_or_name(fd, core, driver, len, CPUFREQ_BINDINGS_FILE_SCALING_DRIVER, 1);
}

ssize_t cpufreq_bindings_get_scaling_governor(int fd, uint32_t core, char* governor, size_t len) {
  return read_file_by_fd_or_name(fd, core, governor, len, CPUFREQ_BINDINGS_FILE_SCALING_GOVERNOR, 1);
}

ssize_t cpufreq_bindings_set_scaling_governor(int fd, uint32_t core, const char* governor, size_t len) {
  return write_file_by_fd_or_name(fd, core, governor, len, CPUFREQ_BINDINGS_FILE_SCALING_GOVERNOR);
}

uint32_t cpufreq_bindings_get_scaling_max_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, CPUFREQ_BINDINGS_FILE_SCALING_MAX_FREQ);
}

ssize_t cpufreq_bindings_set_scaling_max_freq(int fd, uint32_t core, uint32_t freq) {
  return write_file_u32(fd, core, freq, CPUFREQ_BINDINGS_FILE_SCALING_MAX_FREQ);
}

uint32_t cpufreq_bindings_get_scaling_min_freq(int fd, uint32_t core) {
  return read_file_u32(fd, core, CPUFREQ_BINDINGS_FILE_SCALING_MIN_FREQ);
}

ssize_t cpufreq_bindings_set_scaling_min_freq(int fd, uint32_t core, uint32_t freq) {
  return write_file_u32(fd, core, freq, CPUFREQ_BINDINGS_FILE_SCALING_MIN_FREQ);
}

ssize_t cpufreq_bindings_set_scaling_setspeed(int fd, uint32_t core, uint32_t freq) {
  return write_file_u32(fd, core, freq, CPUFREQ_BINDINGS_FILE_SCALING_SETSPEED);
}
