/**
 * Common utilities, like logging.
 *
 * @author Connor Imes
 * @date 2017-05-09
 */
#ifndef _CPUFREQ_BINDINGS_COMMON_H_
#define _CPUFREQ_BINDINGS_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef enum cpufreq_bindings_loglevel {
  DEBUG = 0,
  INFO,
  WARN,
  ERROR,
  OFF
} cpufreq_bindings_loglevel;

#ifndef CPUFREQ_BINDINGS_LOG_LEVEL
  #define CPUFREQ_BINDINGS_LOG_LEVEL WARN
#endif

#define TO_FILE(severity) (severity) >= WARN ? stderr : stdout

#define TO_LOG_PREFIX(severity) \
  (severity) == DEBUG ? "[DEBUG]" : \
  (severity) == INFO  ? "[INFO] " : \
  (severity) == WARN  ? "[WARN] " : \
                        "[ERROR]"

#define LOG(severity, ...) \
  do { if ((severity) >= CPUFREQ_BINDINGS_LOG_LEVEL) { \
      fprintf(TO_FILE((severity)), "%s [cpufreq-bindings] ", TO_LOG_PREFIX((severity))); \
      fprintf(TO_FILE((severity)), __VA_ARGS__); \
    } } while (0)

#define PERROR(severity, msg) \
  LOG(severity, "%s: %s\n", msg, strerror(errno))

#ifdef __cplusplus
}
#endif

#endif
