/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 06 ноя. 2024 г.
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
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/meters/LoudnessMeter.h>
#include <lsp-plug.in/dsp-units/meters/TruePeakMeter.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/util/Delay.h>
#include <lsp-plug.in/dsp-units/util/Dither.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/io/Path.h>


MTEST_BEGIN("dspu.meters", psr)

    static constexpr size_t BUFFER_SIZE = 0x400;

    MTEST_MAIN
    {
        dspu::Sample src, lufs, tpeak, psr;
        io::Path src_path, dst_path;

        // Load source file
        MTEST_ASSERT(src_path.fmt("%s/meters/loop.wav", resources()) > 0);
        MTEST_ASSERT(src.load(&src_path) == STATUS_OK);
        printf("Loading file %s...\n", src_path.as_native());

        // Initialize meter
        dspu::TruePeakMeter tpm[2];
        for (size_t i=0; i<2; ++i)
        {
            tpm[i].init();
            tpm[i].set_sample_rate(src.sample_rate());
            tpm[i].update_settings();
        }

        dspu::Dither dth[2];
        for (size_t i=0; i<2; ++i)
        {
            dth[i].init();
            dth[i].set_bits(24);
        }

        dspu::LoudnessMeter lm;
        lm.init(2, dspu::bs::LUFS_MEASURE_PERIOD_MS);

        lm.set_sample_rate(src.sample_rate());
        lm.set_period(dspu::bs::LUFS_MEASURE_PERIOD_MS);
        lm.set_weighting(dspu::bs::WEIGHT_K);
        lm.set_active(0, true);
        lm.set_active(1, true);
        lm.set_designation(0, dspu::bs::CHANNEL_LEFT);
        lm.set_designation(1, dspu::bs::CHANNEL_RIGHT);

        const size_t latency = dspu::millis_to_samples(src.sample_rate(), dspu::bs::LUFS_MEASURE_PERIOD_MS * 0.5f);
        dspu::Delay tpd;
        tpd.init(latency);
        tpd.set_delay(0); //latency - tpm[0].latency());

        const size_t length = src.length();
        MTEST_ASSERT(src.prepend(length) == STATUS_OK);
        MTEST_ASSERT(src.append(length) == STATUS_OK);

        MTEST_ASSERT(lufs.init(1, src.length(), src.length()));
        MTEST_ASSERT(tpeak.init(1, src.length(), src.length()));
        MTEST_ASSERT(psr.init(1, src.length(), src.length()));

        lufs.set_sample_rate(src.sample_rate());
        tpeak.set_sample_rate(src.sample_rate());
        psr.set_sample_rate(src.sample_rate());

        float *buffer = static_cast<float *>(malloc(sizeof(float) * BUFFER_SIZE * 2));
        MTEST_ASSERT(buffer != NULL);
        lsp_finally { free(buffer); };

        for (size_t offset=0; offset < src.length(); )
        {
            size_t to_process = lsp_min(src.length() - offset, BUFFER_SIZE);

            float *b1       = buffer;
            float *b2       = &buffer[BUFFER_SIZE];
            float *in[2];
            in[0]           = const_cast<float *>(src.channel(0, offset));
            in[1]           = const_cast<float *>(src.channel(1, offset));

            // Apply dithering
            dth[0].process(in[0], in[0], to_process);
            dth[1].process(in[1], in[1], to_process);

            // Compute TruePeak value
            tpm[0].process(b1, in[0], to_process);
            tpm[1].process(b2, in[1], to_process);
            dsp::pmax2(b1, b2, to_process);
            tpd.process(b1, b1, to_process);

            // Compute LUFS value
            lm.bind(0, NULL, in[0], 0);
            lm.bind(1, NULL, in[1], 0);
            lm.process(b2, to_process, dspu::bs::DBFS_TO_LUFS_SHIFT_GAIN);

            // Store values
            dsp::copy(lufs.channel(0, offset), b1, to_process);
            dsp::copy(tpeak.channel(0, offset), b2, to_process);

            // Compute PSR value
            for (size_t i=0; i<to_process; ++i)
            {
                const float peak    = b1[i];
                const float lufs    = b2[i];
                b1[i]               = (lufs >= GAIN_AMP_M_60_DB) ? peak / lufs : 0.0f;
            }

            dsp::copy(psr.channel(0, offset), b1, to_process);

            offset         += to_process;
        }

        // Save the results
        MTEST_ASSERT(dst_path.fmt("%s/%s-orig.wav", tempdir(), full_name()) > 0);
        printf("Saving file %s...\n", dst_path.as_native());
        MTEST_ASSERT(src.save(&dst_path) > 0);

        MTEST_ASSERT(dst_path.fmt("%s/%s-tpeak.wav", tempdir(), full_name()) > 0);
        printf("Saving file %s...\n", dst_path.as_native());
        MTEST_ASSERT(tpeak.save(&dst_path) > 0);

        MTEST_ASSERT(dst_path.fmt("%s/%s-lufs.wav", tempdir(), full_name()) > 0);
        printf("Saving file %s...\n", dst_path.as_native());
        MTEST_ASSERT(lufs.save(&dst_path) > 0);

        MTEST_ASSERT(dst_path.fmt("%s/%s-psr.wav", tempdir(), full_name()) > 0);
        printf("Saving file %s...\n", dst_path.as_native());
        MTEST_ASSERT(psr.save(&dst_path) > 0);
    }

MTEST_END



