/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 8 мар. 2024 г.
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

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/dsp-units/meters/Correlometer.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/units.h>


#define MAX_N_BITS 32u

using namespace lsp;

UTEST_BEGIN("dspu.meters", correlometer)

    void process_file(const char *out, const char *in, size_t step, float duration)
    {
        io::Path ifn, ofn;

        UTEST_ASSERT(ifn.fmt("%s/%s", resources(), in) > 0);
        UTEST_ASSERT(ofn.fmt("%s/%s-%s.wav", tempdir(), full_name(), out) > 0);

        printf("Processing '%s' -> '%s'\n", ifn.as_native(), ofn.as_native());

        // Initialize audio files
        dspu::Sample is, os;

        UTEST_ASSERT(is.load(&ifn) == STATUS_OK);
        UTEST_ASSERT(is.channels() >= 2);

        const size_t range = dspu::millis_to_samples(is.sample_rate(), duration);

        UTEST_ASSERT(is.append(range) == STATUS_OK);
        UTEST_ASSERT(os.init(1, is.length()));
        os.set_sample_rate(is.sample_rate());

        // Initialize corellometer
        dspu::Correlometer xc;
        UTEST_ASSERT(xc.init(range) == STATUS_OK);
        xc.set_period(range);

        // Apply processing
        const float *a = is.channel(0);
        const float *b = is.channel(1);
        float *dst = os.channel(0);

        for (size_t offset=0; offset < is.length(); offset += step)
        {
            const size_t samples = lsp_min(is.length() - offset, step);
            xc.process(dst, a, b, samples);

            dst += step;
            a   += step;
            b   += step;
        }

        UTEST_ASSERT(os.remove(0, range) == STATUS_OK);

        // Save the result
        UTEST_ASSERT(os.save(&ofn) == ssize_t(is.length() - range));
    }

    UTEST_MAIN
    {
        process_file("guitar1-di", "corr/guitar1-di.wav", 113, 200.0f);
        process_file("guitar1-od", "corr/guitar1-od.wav", 127, 300.0f);
        process_file("guitar2-di", "corr/guitar2-di.wav", 131, 400.0f);
        process_file("guitar2-od", "corr/guitar2-od.wav", 149, 500.0f);
        process_file("mix-dirty", "corr/mix-dirty.wav", 151, 200.0f);
    }

UTEST_END



