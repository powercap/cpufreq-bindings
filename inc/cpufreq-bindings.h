/**
 * Bindings to cpufreq in Linux sysfs.
 * Linux does not guarantee that all files will be present, e.g. "bios_limit" or "scaling_setspeed".
 *
 * No function parameters are allowed to be NULL.
 * If the "fd" (file descriptor) parameter is > 0, it will be used, otherwise the file is opened and closed locally.
 *
 * Frequency values are in KHz.
 * See: https://www.kernel.org/doc/Documentation/cpu-freq/user-guide.txt
 *
 * @author Connor Imes
 * @date 2017-03-16
 */

#ifndef _CPUFREQ_BINDINGS_H_
#define _CPUFREQ_BINDINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <unistd.h>

typedef enum cpufreq_bindings_file {
  CPUFREQ_BINDINGS_FILE_AFFECTED_CPUS,
  CPUFREQ_BINDINGS_FILE_BIOS_LIMIT,
  CPUFREQ_BINDINGS_FILE_CPUINFO_CUR_FREQ,
  CPUFREQ_BINDINGS_FILE_CPUINFO_MAX_FREQ,
  CPUFREQ_BINDINGS_FILE_CPUINFO_MIN_FREQ,
  CPUFREQ_BINDINGS_FILE_CPUINFO_TRANSITION_LATENCY,
  CPUFREQ_BINDINGS_FILE_RELATED_CPUS,
  CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_FREQUENCIES,
  CPUFREQ_BINDINGS_FILE_SCALING_AVAILABLE_GOVERNORS,
  CPUFREQ_BINDINGS_FILE_SCALING_CUR_FREQ,
  CPUFREQ_BINDINGS_FILE_SCALING_DRIVER,
  CPUFREQ_BINDINGS_FILE_SCALING_GOVERNOR,
  CPUFREQ_BINDINGS_FILE_SCALING_MAX_FREQ,
  CPUFREQ_BINDINGS_FILE_SCALING_MIN_FREQ,
  CPUFREQ_BINDINGS_FILE_SCALING_SETSPEED
} cpufreq_bindings_file;

/**
 * Open a file (presumably so the file descriptor can be cached/reused).
 *
 * @param core
 * @param file
 * @param flags
 *  Usually O_RDONLY or O_RDWR; if < 0, open flags are chosen automatically
 * @return the file descriptor, or -1 on error (errno will be set)
 */
int cpufreq_bindings_file_open(uint32_t core, cpufreq_bindings_file file, int flags);

/**
 * Close a file descriptor.
 *
 * @param fd
 * @return 0 on success, or -1 on error (errno will be set)
 */
int cpufreq_bindings_file_close(int fd);

/**
 * Get the affected cores specified by "affected_cpus".
 *
 * @param fd
 * @param core
 * @param affected
 *  The array to be written to - should be as large as the number of cores in the system
 * @param len
 *  The length of the array
 * @return the number of affected cpus, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_affected_cpus(int fd, uint32_t core, uint32_t* affected, uint32_t len);

/**
 * Get the frequency specified by "bios_limit".
 *
 * @param fd
 * @param core
 * @return the frequency on success, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_bios_limit(int fd, uint32_t core);

/**
 * Get the frequency specified by "cpuinfo_cur_freq".
 *
 * @param fd
 * @param core
 * @return the frequency on success, 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_cpuinfo_cur_freq(int fd, uint32_t core);

/**
 * Get the frequency specified by "cpuinfo_max_freq".
 *
 * @param fd
 * @param core
 * @return the frequency on success, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_cpuinfo_max_freq(int fd, uint32_t core);

/**
 * Get the frequency specified by "cpuinfo_min_freq".
 *
 * @param fd
 * @param core
 * @return the frequency on success, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_cpuinfo_min_freq(int fd, uint32_t core);

/**
 * Get frequency switching latency specified by "cpuinfo_transition_latency".
 * Note that the kernel documentation appears outdated - instead of returning -1, UINT32_MAX may be returned.
 *
 * @param fd
 * @param core
 * @return the frequency on success, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_cpuinfo_transition_latency(int fd, uint32_t core);

/**
 * Get the related cpus specified by "related_cpus".
 *
 * @param fd
 * @param core
 * @param related
 *  The array to be written to - should be as large as the number of cores in the system
 * @param len
 *  The length of the array
 * @return the number of related cpus, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_related_cpus(int fd, uint32_t core, uint32_t* related, uint32_t len);

/**
 * Get the available frequencies specified by "scaling_available_frequencies".
 *
 * @param fd
 * @param core
 * @param freqs
 *  An array of frequencies to be written to
 * @param len
 *  The length of the "freqs" array
 * @return the number of frequencies found, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_scaling_available_frequencies(int fd, uint32_t core, uint32_t* freqs, uint32_t len);

/**
 * Get the available governors specified by "scaling_available_governors".
 *
 * @param fd
 * @param core
 * @param governors
 *  A character buffer to be written to (treated as a 2D char array) - should be len * width in size
 * @param len
 *  The number of entries in the "governors" buffer array
 * @param width
 *  the width of each entry in the "governors" buffer array
 * @return the number of frequencies found, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_scaling_available_governors(int fd, uint32_t core, char* governors, size_t len, size_t width);

/**
 * Get the frequency specified by "scaling_cur_freq".
 *
 * @param fd
 * @param core
 * @return the frequency on success, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_scaling_cur_freq(int fd, uint32_t core);

/**
 * Get the scaling driver specified by "scaling_driver".
 *
 * @param fd
 * @param core
 * @param driver
 * @param len
 * @return the number of bytes read, or -1 on failure (errno will be set)
 */
ssize_t cpufreq_bindings_get_scaling_driver(int fd, uint32_t core, char* driver, size_t len);

/**
 * Get the scaling governor specified by "scaling_governor".
 *
 * @param fd
 * @param core
 * @param governor
 * @param len
 * @return the number of bytes read, or -1 on failure (errno will be set)
 */
ssize_t cpufreq_bindings_get_scaling_governor(int fd, uint32_t core, char* governor, size_t len);

/**
 * Get the scaling governor on "scaling_governor".
 *
 * @param fd
 * @param core
 * @param governor
 * @param len
 * @return the number of bytes written, or -1 on failure (errno will be set)
 */
ssize_t cpufreq_bindings_set_scaling_governor(int fd, uint32_t core, const char* governor, size_t len);

/**
 * Get the frequency specified by "scaling_max_freq".
 *
 * @param fd
 * @param core
 * @return the frequency on success, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_scaling_max_freq(int fd, uint32_t core);

/**
 * Set the frequency on "scaling_max_freq".
 *
 * @param fd
 * @param core
 * @param freq
 * @return the number of bytes written, or -1 on failure (errno will be set)
 */
ssize_t cpufreq_bindings_set_scaling_max_freq(int fd, uint32_t core, uint32_t freq);

/**
 * Get the frequency specified by "scaling_min_freq".
 *
 * @param fd
 * @param core
 * @return the frequency on success, or 0 on failure (errno will be set)
 */
uint32_t cpufreq_bindings_get_scaling_min_freq(int fd, uint32_t core);

/**
 * Set the frequency on "scaling_min_freq".
 *
 * @param fd
 * @param core
 * @param freq
 * @return the number of bytes written, or -1 on failure (errno will be set)
 */
ssize_t cpufreq_bindings_set_scaling_min_freq(int fd, uint32_t core, uint32_t freq);

/**
 * Set the frequency on "scaling_setspeed".
 *
 * @param fd
 * @param core
 * @param freq
 * @return the number of bytes written, or -1 on failure (errno will be set)
 */
ssize_t cpufreq_bindings_set_scaling_setspeed(int fd, uint32_t core, uint32_t freq);

#ifdef __cplusplus
}
#endif

#endif
