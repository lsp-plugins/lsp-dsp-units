/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 6 авг. 2023 г.
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
#include <lsp-plug.in/dsp-units/util/SpectralSplitter.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/stdlib/math.h>

MTEST_BEGIN("dspu.util", spectral_splitter)

    typedef struct band_t
    {
        size_t imin;
        size_t imax;
        size_t offset;
        dspu::Sample s;
    } band_t;

    static void spectral_splitter_func(
        void *object,
        void *subject,
        float *out,
        const float *in,
        size_t rank)
    {
        band_t *band = static_cast<band_t *>(subject);
        size_t len  = 1 << rank;
        size_t freq = len >> 1;

        // Keep only the specified range of frequencies
        size_t i=0;
        for (; i<freq; ++i) // Positive frequencies
        {
            float idx = i;
            if ((idx < band->imin) || (idx >= band->imax))
            {
                out[0]  = 0.0f;
                out[1]  = 0.0f;
            }
            else
            {
                out[0]  = in[0];
                out[1]  = in[1];
            }

            in += 2;
            out += 2;
        }
        for (; i<len; ++i) // Negative frequencies
        {
            float idx = len - i;
            if ((idx < band->imin) || (idx >= band->imax))
            {
                out[0]  = 0.0f;
                out[1]  = 0.0f;
            }
            else
            {
                out[0]  = in[0];
                out[1]  = in[1];
            }

            in += 2;
            out += 2;
        }
    }

    static void spectral_splitter_sink(void *object, void *subject, float *samples, size_t count)
    {
        band_t *band = static_cast<band_t *>(subject);
        float *dst = band->s.channel(0);

        dsp::copy(&dst[band->offset], samples, count);
        band->offset   += count;
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

        band_t bands[4];
        float max_f = src.sample_rate() * 0.5f;

        constexpr size_t rank       = 12;
        constexpr size_t xlength    = 1 << rank;

        const float flist[] = { 0.0f, 100.0f, 1000.0f, 10000.0f, max_f };

        for (size_t i=0; i<4; ++i)
        {
            band_t *b   = &bands[i];

            b->imin     = freq_to_index(flist[i], src.sample_rate(), rank);
            b->imax     = freq_to_index(flist[i+1], src.sample_rate(), rank);
            b->offset   = 0;
            b->s.init(src.channels(), src.length() + xlength, src.length() + xlength);
            b->s.set_sample_rate(src.sample_rate());
        }

        dspu::SpectralSplitter split;
        MTEST_ASSERT(split.init(rank, 6) == STATUS_OK);
        split.set_rank(rank);
        split.set_chunk_rank(rank - 2);
        split.set_phase(0);
        for (size_t i=0; i<4; ++i)
            MTEST_ASSERT(split.bind(i, NULL, &bands[i], spectral_splitter_func, spectral_splitter_sink) == STATUS_OK);

        split.process(src.channel(0), src.length());
        split.process(NULL, xlength);

        for (size_t i=0; i<4; ++i)
        {
            band_t *b = &bands[i];
            MTEST_ASSERT(path.fmt("%s/%s-split-%d.wav", tempdir(), full_name(), int(i)) > 0);

            printf("Saving band %d to: %s\n", int(i), path.as_native());

            MTEST_ASSERT(b->s.save(&path) > 0);
        }
    }

MTEST_END









