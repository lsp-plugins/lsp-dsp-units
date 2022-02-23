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

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/dsp-units/noise/MLS.h>
#include <lsp-plug.in/stdlib/math.h>

using namespace lsp;

// MAX_N_BITS set the maximum number of bits to set for. All bits from 2 to MAX_N_BITS will be tested.
// MAX_N_BITS should not be higher than 18, or it will take way too much time.
// MLS supports up to a max of 32 or 64 bits (depending on architecture). Unfortunately testing for more than 18 is not practical...
#define MAX_N_BITS 16
// Set the numerical tolerance for error here.
#define NUM_TOL    1e-15f

// The code below checks whether the MLS implementation is correct by checking whether the circular cross-corrleation of one period is correct.

UTEST_BEGIN("dspu.noise", MLS)

    // Circular autocorrelation. Can be implemented with FFT (much faster), but needs arbitrary size FFT.
    void cautocorr(float *dst, float *src, size_t count)
    {
        for (size_t n = 0; n < count; ++n)
        {
            dst[n] = 0.0f;
            for (size_t m = 0; m < count; ++ m)
            {
                dst[n] += src[m] * src[(m + n) % count];
            }
            dst[n] /= count;
        }
    }

    UTEST_MAIN
    {
        dspu::MLS mls;
        dspu::MLS::mls_t nState = 0;  // Use 0 to force default state.
        uint8_t nMaxBits = mls.maximum_number_of_bits();
        dspu::MLS::mls_t nBufSize;
        if (MAX_N_BITS >= nMaxBits)
            nBufSize = -1;
        else
            nBufSize = (dspu::MLS::mls_t(1) << MAX_N_BITS) - dspu::MLS::mls_t(1);

        float *vPeriod = new float[nBufSize];
        float *vCautoX = new float[nBufSize];

        for (uint8_t nBits = 2; nBits < MAX_N_BITS; ++nBits)
        {
            mls.set_n_bits(nBits);
            mls.set_state(nState);
            mls.update_settings();
            dspu::MLS::mls_t nPeriod = mls.get_period();

            for (size_t n = 0; n < nPeriod; ++n)
                vPeriod[n] = mls.process_single();

            cautocorr(vCautoX, vPeriod, nPeriod);
            float target = -1.0f / nPeriod;
            for (size_t n = 0; n < nPeriod; ++n)
            {
                if (n == 0)
                {
                    UTEST_ASSERT_MSG(abs(1.0f - vCautoX[n]) <= NUM_TOL, "Value out of tolerance (sample 0)!");
                }
                else
                {
                    UTEST_ASSERT_MSG(abs(target - vCautoX[n]) <= NUM_TOL, "Value out of tolerance (other sample)!");
                }
            }
        }

        delete [] vPeriod;
        delete [] vCautoX;

        mls.destroy();
    }


UTEST_END;
