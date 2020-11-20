# lsp-dsp-units

C++ DSP modules

This library provides set of functions and objects that perform different DSP processing of the
audio signal.

Set of modules provided:
  * Signal control
    * Blink toggle
    * Bypass switch
    * Counter
    * Crossfade switch
    * Toggle
  * Dynamic processing
    * Compressor
    * Dynamic processor
    * Expander
    * Gate
    * Limiter
  * Filters and equalization
    * Dynamic filters
    * Equalizer
    * Filter
  * Miscellaneous functions
    * Signal envelope functions
    * Interpolation functions
    * FFT windows
  * Sampling
    * Sample
    * Sample player
  * Utilities
    * Chirp Processor
    * Convolver
    * Crossover
    * Delay
    * De-popper
    * Dither
	* Impulse response taker
    * Latency detector
    * Meter history
    * Oscillator
    * Oversampler
    * Randomizer
    * Shift buffer
    * Sidechain control
    * Spectral processor
    * Spectrum analyzer


Supported platforms
======

The build and correct unit test execution has been confirmed for following platforms:
* FreeBSD
* GNU/Linux
* OpenBSD
* Windows 32-bit
* Windows 64-bit

Requirements
======

The following packages need to be installed for building:

* gcc >= 4.9
* GNU make >= 4.0

Building
======

To build the library, perform the following commands:

```bash
make config # Configure the build
make fetch # Fetch dependencies from Git repository
make
sudo make install
```

To get more build options, run:

```bash
make help
```

To uninstall library, simply issue:

```bash
make uninstall
```

To clean all binary files, run:

```bash
make clean
```

To clean the whole project tree including configuration files, run:

```bash
make prune
```

To fetch all possible dependencies and make the source code tree portable between
different architectures and platforms, run:

```bash
make tree
```

To build source code archive with all possible dependencies, run:

```bash
make distsrc
```

Usage
======

Here's the code snippet of how the library can be initialized and used in C++:



