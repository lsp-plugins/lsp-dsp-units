/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 13 May 2021
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

#ifndef LSP_PLUG_IN_DSP_UNITS_NOISE_MLS_H_
#define LSP_PLUG_IN_DSP_UNITS_NOISE_MLS_H_

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /** MLS stands for Maximum Length Sequence.
         * MLS is a type of pseudorandom sequence with a number of desirable properties:
         *
         * * Smallest crest factor;
         * * 2^N - 1 period length;
         * * MLS sequences are ideally decorrelated from themselves;
         * * MLS spectrum is flat;
         *
         * Where N is the number of bits.
         *
         * MLS is implemented with a linear shift feedback register of N bits. At each step:
         * * The leftmost bit is taken as output.
         * * The values of certain bits in the register are passed through an XOR gate.
         * * The values in the registers are shifted by one to the left.
         * * The XOR gate output is now put into the rightmost bit.
         *
         * If the bits that feed the XOR gate (taps) are chosen appropriately, the resulting sequence will be an MLS of period 2^N - 1.
         * Typically, if the output bit is 1 the value of the sequence will be 1. -1 otherwise.
         * The register can be initialised (seeded) with any non-zero value.
         *
         * This class supports MLS generation with registers up to 128 bits depending on platform.
         *
         * Basic MLS Theory at:
         *
         * http://www.kempacoustics.com/thesis/node83.html
         * https://dspguru.com/dsp/tutorials/a-little-mls-tutorial/
         * http://in.ncu.edu.tw/ncume_ee/digilogi/prbs.htm
         */
        class MLS
        {
            private:
                MLS & operator = (const MLS &);
                MLS(const MLS &);

            public:
                typedef umword_t mls_t;

            private:
                size_t      nBits;
                size_t      nFeedbackBit;
                mls_t       nFeedbackMask;
                mls_t       nActiveMask;
                mls_t       nTapsMask;
                mls_t       nOutputMask;
                mls_t       nState;

                float       fAmplitude;
                float       fOffset;

                bool        bSync;

            public:
                explicit MLS();
                ~MLS();

                void construct();
                void destroy();

            protected:
                mls_t xor_gate(mls_t value);
                mls_t progress();

            public:

                /** Return the max supported number of bits by the generator (platform dependent).
                 *
                 * @return maximum number of supported bits.
                 */
                size_t maximum_number_of_bits() const;

                /** Check that MLS needs settings update.
                 *
                 * @return true if MLS needs settings update.
                 */
                inline bool needs_update() const
                {
                    return bSync;
                }

                /** This method should be called if needs_update() returns true.
                 * before calling processing methods.
                 *
                 */
                void update_settings();

                /** Set the number of bits of the generator. This causes reset.
                 *
                 * @param nbits number of bits
                 */
                inline void set_n_bits(size_t nbits)
                {
                    if (nbits == nBits)
                        return;

                    nBits = nbits;
                    bSync = true;
                }

                /** Set the state (seed). This causes reset. States must be non-zero. If 0 is passed, all the active bits will be flipped to 1.
                 *
                 * @param targetstate state to be set.
                 */
                inline void set_state(mls_t targetstate)
                {
                    if (targetstate == nState)
                        return;

                    nState = targetstate;
                    bSync = true;
                }

                /** Set the amplitude of the MLS sequence.
                 *
                 * @param amplitude amplitude value for the sequence.
                 */
                inline void set_amplitude(float amplitude)
                {
                    if (amplitude == fAmplitude)
                        return;

                    fAmplitude = amplitude;
                }

                /** Set the offset of the MLS sequence.
                 *
                 * @param offset offset value for the sequence.
                 */
                inline void set_offset(float offset)
                {
                    if (offset == fOffset)
                        return;

                    fOffset = offset;
                }

                /** Get the sequence period
                 *
                 * @return sequence period
                 */
                mls_t get_period() const;


                /** Get a sample from the MLS generator.
                 *
                 * @return the next sample in the MLS sequence.
                 */
                float process_single();

                /** Output sequence to the destination buffer in additive mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to synthesise
                 */
                void process_add(float *dst, const float *src, size_t count);

                /** Output sequence to the destination buffer in multiplicative mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process_mul(float *dst, const float *src, size_t count);

                /** Output sequence to a destination buffer overwriting its content
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULLL
                 * @param count number of samples to process
                 */
                void process_overwrite(float *dst, size_t count);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
}

#endif /* LSP_PLUG_IN_DSP_UNITS_NOISE_MLS_H_ */
