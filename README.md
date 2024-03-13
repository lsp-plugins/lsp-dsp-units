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
    * Autogain
    * Compressor
    * Dynamic processor
    * Expander
    * Gate
    * Limiter
    * Simple autogain
    * Surge protector
  * Filters and equalization
    * Butterworth filter
    * Dynamic filters
    * Equalizer
    * Filter
    * Spectral tilt
  * Metering
    * Correlation meter
    * Loudness meter
  * Miscellaneous functions
    * Broadcasting related functions and constants
    * Signal envelope functions
    * Interpolation functions
    * FFT windows
    * Fading functions
    * LFO functions
  * Noise generators
    * LGC generator
    * MLS generator
    * Velvet generator
  * Sampling
    * Sample
    * Sample player
    * Sample playbakc control
  * Utilities
    * Chirp Processor
    * Convolver
    * Crossover
    * Delay
    * Dynamic Delay
    * De-popper
    * Dither
    * FFT crossover
    * Impulse response taker
    * Latency detector
    * Meter history
    * Oscillator
    * Oversampler
    * Randomizer
    * Ring buffer
    * Shift buffer
    * Sidechain control
    * Spectral processor
    * Spectrum analyzer
    * Trigger


## Supported platforms

The build and correct unit test execution has been confirmed for following platforms:
* FreeBSD
* GNU/Linux
* Windows 32-bit
* Windows 64-bit

## Requirements

The following packages need to be installed for building:

* gcc >= 4.9
* GNU make >= 4.0

## Building

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

## Usage

Here's the code snippet of how the library can be initialized and used in C++:

```C++
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/filters/Filter.h>
#include <string.h>
#include <stdio.h>

static int process_file(const char *in, const char *out)
{
    // We will process file as a single audio sample
    // That can take many memory for large files
    // but for demo it's OK
    lsp::dspu::Sample s;
    lsp::dspu::Filter f;
    lsp::dspu::filter_params_t fp;
    lsp::io::Path path;

    // We consider pathnames encoded using native system encoding.
    // That's why we use io::Path::set_native() method
    path.set_native(in);
    if (s.load(&path) != lsp::STATUS_OK)
    {
        fprintf(stderr, "Error loading audio sample from file: %s\n", in);
        return -1;
    }

    // Apply +6 dB hi-shelf filter at 1 kHz
    fp.nType        = lsp::dspu::FLT_BT_BWC_HISHELF;
    fp.fFreq        = 1000.0f;
    fp.fFreq2       = 1000.0f;
    fp.fGain        = lsp::dspu::db_to_gain(6.0f);
    fp.nSlope       = 2;
    fp.fQuality     = 0.0f;

    f.init(NULL);   // Use own internal filter bank
    f.update(s.sample_rate(), &fp); // Apply filter settings

    // Now we need to process each channel in the sample
    for (size_t i=0; i<s.channels(); ++i)
    {
        float *c    = s.channel(i);     // Get channel data
        f.clear();                      // Reset internal memory of filter
        f.process(c, c, s.samples());
    }

    // Resample the processed sample to 48 kHz sample rate
    s.resample(48000);

    // Save sample to file
    path.set_native(out);
    if (s.save(&path) < 0)
    {
        fprintf(stderr, "Error saving audio sample to file: %s\n", out);
        return -2;
    }

    return 0;
}

int main(int argc, const char **argv)
{
    lsp::dsp::context_t ctx;

    if (argc < 3)
    {
        fprintf(stderr, "Input file name and output file name required");
        return -1;
    }

    // Initialize DSP
    lsp::dsp::init();
    lsp::dsp::start(&ctx);

    int res = process_file(argv[1], argv[2]);

    lsp::dsp::finish(&ctx);

    return res;
}
```

## SAST Tools

* [PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.
