/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 10 авг. 2023 г.
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
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/util/FFTCrossover.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/stdlib/math.h>

MTEST_BEGIN("dspu.util", fft_crossover)

    typedef struct band_t
    {
        size_t offset;
        dspu::Sample s;
    } band_t;

    static void crossover_func(void *object, void *subject, size_t band, const float *data, size_t first, size_t count)
    {
        band_t *b= static_cast<band_t *>(subject);
        float *dst      = b->s.channel(0);

        dsp::copy(&dst[b->offset], data, count);
        b->offset      += count;
    }

    float freq_to_index(float f, float sample_rate, size_t rank)
    {
        size_t n = 1 << rank;
        return (f * n) / sample_rate;
    }

    MTEST_MAIN
    {
        dspu::Sample src;
        io::Path path;

        MTEST_ASSERT(path.fmt("%s/util/noise.wav", resources()) > 0);
        MTEST_ASSERT(src.load(&path) == STATUS_OK);
        MTEST_ASSERT(src.channels() == 1);

        band_t bands[5];

        constexpr size_t rank       = 12;
        constexpr size_t xlength    = 1 << rank;

        for (size_t i=0; i<5; ++i)
        {
            band_t *b   = &bands[i];

            b->offset   = 0;
            b->s.init(src.channels(), src.length() + xlength, src.length() + xlength);
            b->s.set_sample_rate(src.sample_rate());
        }

        dspu::FFTCrossover crossover;
        MTEST_ASSERT(crossover.init(rank, 5) == STATUS_OK);
        crossover.set_sample_rate(src.sample_rate());

        // Band 0
        crossover.set_lpf(0, 50, 0.0f, true);
        crossover.set_flatten(0, dspu::db_to_gain(-3.0f));
        crossover.enable_band(0, true);

        // Band 1
        crossover.set_hpf(1, 90, -32.0f, true);
        crossover.set_lpf(1, 425, -32.0f, true);
        crossover.set_flatten(1, dspu::db_to_gain(-3.0f));
        crossover.enable_band(1, true);

        // Band 2
        crossover.set_hpf(2, 425, -32.0f, true);
        crossover.set_lpf(2, 1750, -32.0f, true);
        crossover.set_flatten(2, dspu::db_to_gain(-3.0f));
        crossover.enable_band(2, true);

        // Band 3
        crossover.set_hpf(3, 1750, -32.0f, true);
        crossover.set_lpf(3, 7300, -32.0f, true);
        crossover.set_flatten(3, dspu::db_to_gain(-3.0f));
        crossover.enable_band(3, true);

        // Band 4
        crossover.set_hpf(4, 7300, -64.0f, true);
        crossover.set_flatten(4, dspu::db_to_gain(-3.0f));
        crossover.enable_band(4, true);

        for (size_t i=0; i<5; ++i)
            MTEST_ASSERT(crossover.set_handler(i, crossover_func, NULL, &bands[i]));


        crossover.process(src.channel(0), src.length());
        crossover.process(NULL, xlength);

        for (size_t i=0; i<5; ++i)
        {
            band_t *b = &bands[i];
            MTEST_ASSERT(path.fmt("%s/%s-xover-%d.wav", tempdir(), full_name(), int(i)) > 0);

            printf("Saving band %d to: %s\n", int(i), path.as_native());

            MTEST_ASSERT(b->s.save(&path) > 0);
        }
    }

MTEST_END




