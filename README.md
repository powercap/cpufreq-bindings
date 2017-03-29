# cpufreq-bindings - C bindings to cpufreq in Linux sysfs

The projects provides the `cpufreq-bindings` C library.

For Linux kernel documentation on cpufreq, see: https://www.kernel.org/doc/Documentation/cpu-freq/user-guide.txt

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

## Project Source

Find this and related project sources at the [powercap organization on GitHub](https://github.com/powercap).  
This project originates at: https://github.com/powercap/cpufreq-bindings

Bug reports and pull requests are welcome.
