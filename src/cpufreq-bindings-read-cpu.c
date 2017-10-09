/**
 * Read all the cpufreq files for a core.
 *
 * @author Connor Imes
 * @date 2017-03-16
 */
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpufreq-bindings.h"

#define MAX_CPUS 1024
#define MAX_FREQS 32
#define MAX_GOVS 16
#define MAX_GOV_LEN 32

static void print_or_perror_str(ssize_t bytes, const char* buf, const char* name) {
  if (bytes > 0) {
    printf("%s: %s\n", name, buf);
  } else {
    perror(name);
  }
}

static void print_or_perror_strarr(char arr[][MAX_GOV_LEN], uint32_t len, const char* name) {
  uint32_t i;
  if (len > 0) {
    printf("%s: ", name);
    for (i = 0; i < len; i++) {
      printf("%s ", arr[i]);
    }
    printf("\n");
  } else {
    perror(name);
  }
}

static void print_or_perror_u32(uint32_t u32_val, const char* name) {
  if (u32_val > 0) {
    printf("%s: %"PRIu32"\n", name, u32_val);
  } else {
    perror(name);
  }
}

static void print_or_perror_u32arr(uint32_t* arr, uint32_t len, const char* name) {
  uint32_t i;
  if (len > 0) {
    printf("%s: ", name);
    for (i = 0; i < len; i++) {
      printf("%"PRIu32" ", arr[i]);
    }
    printf("\n");
  } else {
    perror(name);
  }
}

static void print_cpu(uint32_t core, const int* fds) {
  char buf[2014];
  char governors[MAX_GOVS][MAX_GOV_LEN];
  uint32_t freqs[MAX_FREQS];
  uint32_t cpu_aff_rel[MAX_CPUS];
  uint32_t u32_val;
  ssize_t bytes;

  u32_val = cpufreq_bindings_get_affected_cpus(fds[CPUFREQ_BINDINGS_FILE_AFFECTED_CPUS], core, cpu_aff_rel, MAX_CPUS);
  print_or_perror_u32arr(cpu_aff_rel, u32_val, "affected_cpus");

  u32_val = cpufreq_bindings_get_bios_limit(fds[CPUFREQ_BINDINGS_FILE_BIOS_LIMIT], core);
  print_or_perror_u32(u32_val, "bios_limit");

  u32_val = cpufreq_bindings_get_cpuinfo_cur_freq(fds[CPUFREQ_BINDINGS_FILE_CPUINFO_CUR_FREQ], core);
  print_or_perror_u32(u32_val, "cpuinfo_cur_freq");

  u32_val = cpufreq_bindings_get_cpuinfo_max_freq(fds[CPUFREQ_BINDINGS_FILE_CPUINFO_MAX_FREQ], core);
  print_or_perror_u32(u32_val, "cpuinfo_max_freq");

  u32_val = cpufreq_bindings_get_cpuinfo_min_freq(fds[CPUFREQ_BINDINGS_FILE_CPUINFO_MIN_FREQ], core);
  print_or_perror_u32(u32_val, "cpuinfo_min_freq");

  u32_val = cpufreq_bindings_get_cpuinfo_transition_latency(fds[CPUFREQ_BINDINGS_FILE_CPUINFO_TRANSITION_LATENCY], core);
  print_or_perror_u32(u32_val, "cpuinfo_transition_latency");

  u32_val = cpufreq_bindings_get_related_cpus(fds[CPUFREQ_BINDINGS_FILE_RELATED_CPUS], core, cpu_aff_rel, MAX_CPUS);
  print_or_perror_u32arr(cpu_aff_rel, u32_val, "related_cpus");

  u32_val = cpufreq_bindings_get_scaling_available_frequencies(fds[CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_FREQUENCIES],
                                                               core, freqs, MAX_FREQS);
  print_or_perror_u32arr(freqs, u32_val, "scaling_available_frequencies");

  u32_val = cpufreq_bindings_get_scaling_available_governors(fds[CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_GOVERNORS],
                                                             core, governors[0], MAX_GOVS, MAX_GOV_LEN);
  print_or_perror_strarr(governors, u32_val, "scaling_available_governors");

  u32_val = cpufreq_bindings_get_scaling_cur_freq(fds[CPUFREQ_BINDINGS_FILE_SCALING_CUR_FREQ], core);
  print_or_perror_u32(u32_val, "scaling_cur_freq");

  bytes = cpufreq_bindings_get_scaling_driver(fds[CPUFREQ_BINDINGS_FILE_SCALING_DRIVER], core, buf, sizeof(buf));
  print_or_perror_str(bytes, buf, "scaling_driver");

  bytes = cpufreq_bindings_get_scaling_governor(fds[CPUFREQ_BINDINGS_FILE_SCALING_GOVERNOR], core, buf, sizeof(buf));
  print_or_perror_str(bytes, buf, "scaling_governor");

  u32_val = cpufreq_bindings_get_scaling_max_freq(fds[CPUFREQ_BINDINGS_FILE_SCALING_MAX_FREQ], core);
  print_or_perror_u32(u32_val, "scaling_max_freq");

  u32_val = cpufreq_bindings_get_scaling_min_freq(fds[CPUFREQ_BINDINGS_FILE_SCALING_MIN_FREQ], core);
  print_or_perror_u32(u32_val, "scaling_min_freq");
}

static const char short_options[] = "hc:";
static const struct option long_options[] = {
  {"help",                no_argument,        NULL, 'h'},
  {"cpu",                 required_argument,  NULL, 'c'},
  {0, 0, 0, 0}
};

static void print_usage(void) {
  printf("Usage: cpufreq-bindings-read-cpu [OPTION]...\n");
  printf("Options:\n");
  printf("  -h, --help                   Print this message and exit\n");
  printf("  -c, --cpu=CPU                The processor core to read (default is 0)\n");
}

int main(int argc, char** argv) {
  uint32_t core = 0;
  int fds[CPUFREQ_BINDINGS_FILE_SCALING_SETSPEED + 1] = { 0 };
  int c;
  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
    switch (c) {
      case 'h':
        print_usage();
        return 0;
      case 'c':
        core = atoi(optarg);
        break;
      case '?':
      default:
        print_usage();
        return -EINVAL;
    }
  }
  print_cpu(core, fds);
  return 0;
}
