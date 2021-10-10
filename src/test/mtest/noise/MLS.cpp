/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 10 Oct 2021
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

using namespace lsp;

#define MAX_N_BITS 32

MTEST_BEGIN("dspu.noise", MLS)

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
        uint8_t nBits = 24;
        nBits = lsp_min(nBits, MAX_N_BITS);
        dspu::MLS::mls_t nState = 0;  // Use 0 to force default state.

        dspu::MLS mls;
        mls.set_n_bits(nBits);
        mls.set_state(nState);
        mls.update_settings();
        dspu::MLS::mls_t nPeriod = mls.get_period();

        float *vBuffer = new float[nPeriod];
        for (size_t n = 0; n < nPeriod; ++n)
            vBuffer[n] = mls.single_sample_processor();

        write_buffer("tmp/mls.csv", "MLS Period", vBuffer, nPeriod);

        delete [] vBuffer;

        mls.destroy();
    }

MTEST_END
