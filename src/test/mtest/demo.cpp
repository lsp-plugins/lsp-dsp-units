/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 22 нояб. 2020 г.
 *
 * lsp-dsp-units is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-dsp-units is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-dsp-units. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/lltl/parray.h>

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/filters/Filter.h>
#include <string.h>
#include <stdio.h>

namespace
{
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

        // Apply +6 dB hi-shelf filter over 1 kHx
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

    int demo_main(int argc, const char **argv)
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
}

MTEST_BEGIN("dspu", demo)

    MTEST_MAIN
    {
        lltl::parray<char> args;
        char stub[0x10];
        strcpy(stub, "test");

        MTEST_ASSERT(args.add(stub));
        for (int i=0; i<argc; ++i)
            args.add(const_cast<char *>(argv[i]));

        MTEST_ASSERT(demo_main(args.size(), const_cast<const char **>(args.array())) == 0);
    }

MTEST_END



