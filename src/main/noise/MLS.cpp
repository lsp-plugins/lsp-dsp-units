/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
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

namespace lsp
{
    namespace dspu
    {

        /** From the table "Exponent of Terms of Primitive Binary Polynomials"
         *  in Primitive Binary Polynomials by Wayne Stahnke,
         *  Mathematics of Computation, Volume 27, Number 124, October 1973
         *  link: https://www.ams.org/journals/mcom/1973-27-124/S0025-5718-1973-0327722-7/S0025-5718-1973-0327722-7.pdf
         */
        static const size_t nMaxBits = sizeof(MLS::mls_t) * 8;

        static const MLS::mls_t vTapsMaskTable[] = {
            // These first 32 elements would use the condition:
            // #if defined(ARCH_32BIT) || defined(ARCH_64BIT) || defined(ARCH_128BIT)
            // but all platforms have 32 bit support, so we can simply omit.
                1u,     3u,     3u,     3u,     5u,     3u,     3u,     99u,
                17u,    9u,     5u,     153u,   27u,    6147u,  3u,     45u,
                9u,     129u,   99u,    9u,     5u,     3u,     33u,    27u,
                9u,     387u,   387u,   9u,     5u,     98307u, 9u,     402653187u,
            #if defined(ARCH_64BIT) || defined(ARCH_128BIT)
                8193u,                  49155u,                 5u,                     2049u,                  5125u,                  99u,                    17u,                2621445u,
                9u,                     12582915u,              99u,                    201326595u,             27u,                    3145731u,               33u,                402653187u,
                513u,                   201326595u,             98307u,                 9u,                     98307u,                 206158430211u,          16777217u,          6291459u,
                129u,                   524289u,                6291459u,               3u,                     98307u,                 216172782113783811u,    3u,                 27u,
            #endif
            #if defined(ARCH_128BIT)
                262145u,                                        1539u,                                          1539u,                                          513u,                                   671088645u,                                     98307u,                                     65u,                                        9147936743096385u,
                33554433u,                                      98307u,                                         3075u,                                          103079215107u,                          3221225475u,                                    1572867u,                                   513u,                                       412316860419u,
                17u,                                            309237645321u,                                  105553116266499u,                               8193u,                                  402653187u,                                     12291u,                                     8193u,                                      7083549724304467820547u,
                274877906945u,                                  786435u,                                        29014219670751100192948227u,                    12291u,                                 5u,                                             2097153u,                                   2049u,                                      703687441776645u,
                65u,                                            2049u,                                          175921860444165u,                               137438953473u,                          195u,                                           226673591177742970257411u,                  513u,                                       3075u,
                65537u,                                         32769u,                                         46116860184273879045u,                          2147483649u,                            195u,                                           12291u,                                     1025u,                                      43980465111045u,
                513u,                                           7253554917687775048237059u,                     49155u,                                         3541774862152233910275u,                1310725u,                                       8589934593u,                                257u,                                       334903147375496382040217013234696321u,
                262145u,                                        1729382256910270467u,                           5u,                                             137438953473u,                          486777830487640090174734030864387u,             206158430211u,                              3u,                                         671088645u
            #endif
        };

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

        void MLS::destroy()
        {
        }

        size_t MLS::maximum_number_of_bits() const
        {
            return nMaxBits;
        }

        void MLS::update_settings()
        {
            if (!needs_update())
                return;

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
            /* Note: value should be unsigned for proper binary shift calculations */
            #ifdef ARCH_128BIT
            value ^= value >> 64;
            #endif /* ARCH_128BIT */
            #ifdef ARCH_64BIT
            value ^= value >> 32;
            #endif /* ARCH_64BIT */
            value ^= value >> 16;
            value ^= value >> 8;
            value ^= value >> 4;
            value ^= value >> 2;
            value ^= value >> 1;
            return value & mls_t(1u);
        }

        MLS::mls_t MLS::get_period() const
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

        float MLS::process_single()
        {
            return progress() ? fAmplitude + fOffset : -fAmplitude + fOffset;
        }

        void MLS::process_add(float *dst, const float *src, size_t count)
        {
            while (count--)
            {
                *(dst++) = *(src++) + process_single();
            }
        }

        void MLS::process_mul(float *dst, const float *src, size_t count)
        {
            while (count--)
            {
                *(dst++) = *(src++) * process_single();
            }
        }

        void MLS::process_overwrite(float *dst, size_t count)
        {
            while (count--)
            {
                *(dst++) = process_single();
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
