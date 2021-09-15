/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 16 May 2021
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/dsp-units/noise/MLS.h>

// This is how many bits we can support, i.e. how many we put in the lookup table.
#ifdef ARCH_32_BIT
#define MAX_SUPPORTED_BITS 32
#else
#define MAX_SUPPORTED_BITS 64
#endif

namespace lsp
{
    namespace dspu
    {
        MLS::MLS()
        {
            construct();
        }

        MLS::~MLS()
        {
            destroy();
        }

        void MLS::construct()
        {
            vTapsMaskTable  = NULL;
            construct_taps_mask_table();

            nMaxBits            = sizeof(mls_t) * 8;
            nBits               = sizeof(mls_t) * 8;
            nFeedbackBit        = 0;
            nFeedbackMask       = 0;
            nActiveMask         = 0;
            nTapsMask           = 0;
            nOutputMask         = 1;
            nState              = 0;

            fAmplitude          = 1.0f;

            bSync               = true;
        }

        void MLS::construct_taps_mask_table()
        {
            vTapsMaskTable = new mls_t[MAX_SUPPORTED_BITS];

            /** From the table "Exponent of Terms of Primitive Binary Polynomials"
             *  in Primitive Binary Polynomials by Wayne Stahnke,
             *  Mathematics of Computation, Volume 27, Number 124, October 1973
             *  link: https://www.ams.org/journals/mcom/1973-27-124/S0025-5718-1973-0327722-7/S0025-5718-1973-0327722-7.pdf
             */

            #ifdef ARCH_32_BIT
            vTapsMaskTable[0]   = 1u;
            vTapsMaskTable[1]   = 3u;
            vTapsMaskTable[2]   = 3u;
            vTapsMaskTable[3]   = 3u;
            vTapsMaskTable[4]   = 5u;
            vTapsMaskTable[5]   = 3u;
            vTapsMaskTable[6]   = 3u;
            vTapsMaskTable[7]   = 99u;
            vTapsMaskTable[8]   = 17u;
            vTapsMaskTable[9]   = 9u;
            vTapsMaskTable[10]  = 5u;
            vTapsMaskTable[11]  = 153u;
            vTapsMaskTable[12]  = 27u;
            vTapsMaskTable[13]  = 6147u;
            vTapsMaskTable[14]  = 3u;
            vTapsMaskTable[15]  = 45u;
            vTapsMaskTable[16]  = 9u;
            vTapsMaskTable[17]  = 129u;
            vTapsMaskTable[18]  = 99u;
            vTapsMaskTable[19]  = 9u;
            vTapsMaskTable[20]  = 5u;
            vTapsMaskTable[21]  = 3u;
            vTapsMaskTable[22]  = 33u;
            vTapsMaskTable[23]  = 27u;
            vTapsMaskTable[24]  = 9u;
            vTapsMaskTable[25]  = 387u;
            vTapsMaskTable[26]  = 387u;
            vTapsMaskTable[27]  = 9u;
            vTapsMaskTable[28]  = 5u;
            vTapsMaskTable[29]  = 98307u;
            vTapsMaskTable[30]  = 9u;
            vTapsMaskTable[31]  = 402653187u;
            #else
            vTapsMaskTable[0]   = 1u;
            vTapsMaskTable[1]   = 3u;
            vTapsMaskTable[2]   = 3u;
            vTapsMaskTable[3]   = 3u;
            vTapsMaskTable[4]   = 5u;
            vTapsMaskTable[5]   = 3u;
            vTapsMaskTable[6]   = 3u;
            vTapsMaskTable[7]   = 99u;
            vTapsMaskTable[8]   = 17u;
            vTapsMaskTable[9]   = 9u;
            vTapsMaskTable[10]  = 5u;
            vTapsMaskTable[11]  = 153u;
            vTapsMaskTable[12]  = 27u;
            vTapsMaskTable[13]  = 6147u;
            vTapsMaskTable[14]  = 3u;
            vTapsMaskTable[15]  = 45u;
            vTapsMaskTable[16]  = 9u;
            vTapsMaskTable[17]  = 129u;
            vTapsMaskTable[18]  = 99u;
            vTapsMaskTable[19]  = 9u;
            vTapsMaskTable[20]  = 5u;
            vTapsMaskTable[21]  = 3u;
            vTapsMaskTable[22]  = 33u;
            vTapsMaskTable[23]  = 27u;
            vTapsMaskTable[24]  = 9u;
            vTapsMaskTable[25]  = 387u;
            vTapsMaskTable[26]  = 387u;
            vTapsMaskTable[27]  = 9u;
            vTapsMaskTable[28]  = 5u;
            vTapsMaskTable[29]  = 98307u;
            vTapsMaskTable[30]  = 9u;
            vTapsMaskTable[31]  = 402653187u;
            vTapsMaskTable[32]  = 8193u;
            vTapsMaskTable[33]  = 49155u;
            vTapsMaskTable[34]  = 5u;
            vTapsMaskTable[35]  = 2049u;
            vTapsMaskTable[36]  = 5125u;
            vTapsMaskTable[37]  = 99u;
            vTapsMaskTable[38]  = 17u;
            vTapsMaskTable[39]  = 2621445u;
            vTapsMaskTable[40]  = 9u;
            vTapsMaskTable[41]  = 12582915u;
            vTapsMaskTable[42]  = 99u;
            vTapsMaskTable[43]  = 201326595u;
            vTapsMaskTable[44]  = 27u;
            vTapsMaskTable[45]  = 3145731u;
            vTapsMaskTable[46]  = 33u;
            vTapsMaskTable[47]  = 402653187u;
            vTapsMaskTable[48]  = 513u;
            vTapsMaskTable[49]  = 201326595u;
            vTapsMaskTable[50]  = 98307u;
            vTapsMaskTable[51]  = 9u;
            vTapsMaskTable[52]  = 98307u;
            vTapsMaskTable[53]  = 206158430211u;
            vTapsMaskTable[54]  = 16777217u;
            vTapsMaskTable[55]  = 6291459u;
            vTapsMaskTable[56]  = 129u;
            vTapsMaskTable[57]  = 524289u;
            vTapsMaskTable[58]  = 6291459u;
            vTapsMaskTable[59]  = 3u;
            vTapsMaskTable[60]  = 98307u;
            vTapsMaskTable[61]  = 216172782113783811u;
            vTapsMaskTable[62]  = 3u;
            vTapsMaskTable[63]  = 27u;
            #endif
        }

        void MLS::destroy()
        {
            if (vTapsMaskTable != NULL)
                delete [] vTapsMaskTable;

            vTapsMaskTable = NULL;
        }

        uint8_t MLS::maximum_number_of_bits()
        {
            return MAX_SUPPORTED_BITS;
        }

        void MLS::update_settings()
        {
            if (!needs_update())
                return;

            nMaxBits = lsp_min(nMaxBits, MAX_SUPPORTED_BITS);

            nBits = lsp_max(nBits, 1u);
            nBits = lsp_min(nBits, nMaxBits);

            nFeedbackBit = nBits - 1u;
            nFeedbackMask = mls_t(1) << nFeedbackBit;

            // Switch on all the first nBits bits.
            nActiveMask = ~(~mls_t(0) << nBits);

            nTapsMask = vTapsMaskTable[nBits - 1];

            nState &= nActiveMask;

            // State cannot be 0. If that happens, flip all the active bits to 1.
            if (nState == 0)
                nState |= nActiveMask;

            bSync = false;
        }

        // Compute the xor of all the bits in value
        MLS::mls_t MLS::xor_gate(mls_t value)
        {
            mls_t nXorValue;
            for (nXorValue = 0; value; nXorValue ^= 1)
            {
                value &= value - 1;
            }

            return nXorValue;
        }

        MLS::mls_t MLS::get_period()
        {
            if (nBits == nMaxBits)
            {
                return -1;
            }
            else
            {
                mls_t period = 1;
                return (period << nBits) - 1;
            }

        }

        MLS::mls_t MLS::progress()
        {
            mls_t nOutput = nState & nOutputMask;

            mls_t nFeedbackValue = xor_gate(nState & nTapsMask);

            nState = nState >> 1;
            nState = (nState & ~nFeedbackMask) | (nFeedbackValue << nFeedbackBit);

            return nOutput;
        }

        float MLS::single_sample_processor()
        {
            return progress() ? fAmplitude + fOffset : -fAmplitude + fOffset;
        }

        void MLS::process_add(float *dst, const float *src, size_t count)
        {
            while (count--)
            {
                *(dst++) = *(src++) + single_sample_processor();
            }
        }

        void MLS::process_mul(float *dst, const float *src, size_t count)
        {
            while (count--)
            {
                *(dst++) = *(src++) * single_sample_processor();
            }
        }

        void MLS::process_overwrite(float *dst, size_t count)
        {
            while (count--)
            {
                *(dst++) = single_sample_processor();
            }
        }

        void MLS::dump(IStateDumper *v) const
        {
            v->write("vTapsMaskTable", vTapsMaskTable);

            v->write("nMaxBits", nMaxBits);
            v->write("nBits", nBits);
            v->write("nFeedbackBit", nFeedbackBit);
            v->write("nFeedbackMask", nFeedbackMask);
            v->write("nActiveMask", nActiveMask);
            v->write("nTapsMask", nTapsMask);
            v->write("nOutputMask", nOutputMask);
            v->write("nState", nState);

            v->write("fAmplitude", fAmplitude);
            v->write("fOffset", fOffset);

            v->write("bSync", bSync);
        }
    }
}
