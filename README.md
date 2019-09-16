# cpufreq-bindings - C bindings to CPUFreq in Linux sysfs

The projects provides the `cpufreq-bindings` C library.

For Linux kernel documentation on cpufreq, see: https://www.kernel.org/doc/Documentation/cpu-freq/user-guide.txt

If using this project for other scientific works or publications, please reference:

* Connor Imes, Huazhe Zhang, Kevin Zhao, Henry Hoffmann. "CoPPer: Soft Real-time Application Performance Using Hardware Power Capping". In: IEEE International Conference on Autonomic Computing (ICAC). 2019. DOI: https://doi.org/10.1109/ICAC.2019.00015

## Building

This project uses CMake.

To build, run:

``` sh
mkdir _build
cd _build
cmake ..
make
```

## Installing

To install, run with proper privileges:

``` sh
make install
```

On Linux, installation typically places libraries in `/usr/local/lib` and header files in `/usr/local/include`.

## Uninstalling

Install must be run before uninstalling in order to have a manifest.
To uninstall, run with proper privileges:

``` sh
make uninstall
```

## Linking

Get linker information with `pkg-config`:

``` sh
pkg-config --libs cpufreq-bindings
```

Or in your Makefile, add to your linker flags with:

``` Makefile
$(shell pkg-config --libs cpufreq-bindings)
```

Depending on your install location, you may also need to augment your compiler flags with:

``` sh
pkg-config --cflags cpufreq-bindings
```

## Usage

See the [inc/cpufreq-bindings.h](inc/cpufreq-bindings.h) header file for function descriptions.

A simple example of setting DVFS frequencies (error checking is ignored):

```C
  // In real scenarios, core count and IDs could be discovered dynamically using other means
  uint32_t NCORES = 4;
  uint32_t CORE_IDS[] = { 0, 1, 2, 3 };
  uint32_t available_freqs[32];
  int setspeed_fds[NCORES];
  uint32_t i, j, nfreqs, freq;

  // populate "available_freqs" array
  nfreqs = cpufreq_bindings_get_scaling_available_frequencies(-1, CORE_IDS[0], available_freqs, 32);
  for (i = 0; i < NCORES; i++) {
    // set "userspace" governor on all cores
    cpufreq_bindings_set_scaling_governor(-1, CORE_IDS[i], "userspace", sizeof("userspace"));
    // cache file descriptors for setting frequencies with "userspace" governor
    // a "performance" governor would instead use CPUFREQ_BINDINGS_FILE_SCALING_MAX_FREQ
    setspeed_fds[i] = cpufreq_bindings_file_open(CORE_IDS[i], CPUFREQ_BINDINGS_FILE_SCALING_SETSPEED, -1);
  }

  // do application work, breaking from loop when finished...
  while (do_work()) {
    // pick new frequency from "available_freqs" for 0 <= j < nfreqs
    freq = available_freqs[j];
    for (i = 0; i < NCORES; i++) {
      // set new frequencies using cached file descriptors
      cpufreq_bindings_set_scaling_setspeed(setspeed_fds[i], CORE_IDS[i], freq);
    }
  }

  for (i = 0; i < NCORES; i++) {
    // close cached file descriptors
    cpufreq_bindings_file_close(setspeed_fds[i]);
    // perhaps set scaling governor back to "ondemand", or whatever the system default is
    cpufreq_bindings_set_scaling_governor(-1, CORE_IDS[i], "ondemand", sizeof("ondemand"));
  }
```

## Project Source

Find this and related project sources at the [powercap organization on GitHub](https://github.com/powercap).  
This project originates at: https://github.com/powercap/cpufreq-bindings

Bug reports and pull requests are welcome.
