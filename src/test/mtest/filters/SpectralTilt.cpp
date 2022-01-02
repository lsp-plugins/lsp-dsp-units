/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 21 Nov 2021
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
#include <lsp-plug.in/dsp-units/noise/MLS.h>
#include <lsp-plug.in/dsp-units/filters/SpectralTilt.h>
#include <lsp-plug.in/fmt/lspc/File.h>

#define MAX_N_BITS 32u

using namespace lsp;

MTEST_BEGIN("dspu.filters", SPECTRALTILT)

    void write_buffer(const char *filePath, const char *description, const float *buf, size_t count)
    {
        printf("Writing %s to file %s\n", description, filePath);

        FILE *fp = NULL;
        fp = fopen(filePath, "w");

        if (fp == NULL)
            return;

        while (count--)
            fprintf(fp, "%.30f\n", *(buf++));

        if(fp)
            fclose(fp);
    }

    MTEST_MAIN
    {
        size_t nBits = 18;
        nBits = lsp_min(nBits, MAX_N_BITS);
        dspu::MLS::mls_t nState = 0;  // Use 0 to force default state.

        size_t nOrder = 16;
        float fSlope = -0.5f;
        dspu::stlt_slope_unit_t enSlopeUnit = dspu::STLT_SLOPE_UNIT_NEPER_PER_NEPER;
        size_t nSampleRate = 48000;
        float fLowFreq = 10.0f;
        float fUpFreq = 0.45f * nSampleRate;

        dspu::MLS mls;
        mls.set_n_bits(nBits);
        mls.set_state(nState);
        mls.update_settings();
        dspu::MLS::mls_t nPeriod = mls.get_period();

        dspu::SpectralTilt filter;
        filter.set_order(nOrder);
        filter.set_slope(fSlope, enSlopeUnit);
        filter.set_lower_frequency(fLowFreq);
        filter.set_upper_frequency(fUpFreq);
        filter.set_sample_rate(nSampleRate);
        filter.update_settings();

        float *vIn = new float[nPeriod];
        float *vOut = new float[nPeriod];

        for (size_t n = 0; n < nPeriod; ++n)
            vIn[n] = mls.process_single();

        filter.process_overwrite(vOut, vIn, nPeriod);

        io::Path path_in;
        MTEST_ASSERT(path_in.fmt("%s/stilt_in-%s.csv", tempdir(), full_name()));
        write_buffer(path_in.as_native(), "MLS Period - In", vIn, nPeriod);

        io::Path path_out;
        MTEST_ASSERT(path_out.fmt("%s/stilt_out-%s.csv", tempdir(), full_name()));
        write_buffer(path_out.as_native(), "MLS Period - Out", vOut, nPeriod);

        delete [] vIn;
        delete [] vOut;

        mls.destroy();
    }

MTEST_END
