/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 13 Jun 2021
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

#ifndef LSP_PLUG_IN_DSP_UNITS_NOISE_LCG_H_
#define LSP_PLUG_IN_DSP_UNITS_NOISE_LCG_H_

#include <lsp-plug.in/dsp-units/util/Randomizer.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        enum lcg_dist_t
        {
            LCG_UNIFORM,        // Uniform over [-1, 1), X fAmplitude + fOffset
            LCG_EXPONENTIAL,    // Double sided exponential over [-1, 1], X fAmplitude + fOffset
            LCG_TRIANGULAR,     // Triangular over [-1, 1], X fAmplitude + fOffset
            LCG_GAUSSIAN,       // Gaussian of mean 0 and standard deviation 1, X fAmplitude + fOffset
            LCG_MAX
        };

        class LCG
        {
            private:
            LCG & operator = (const LCG &);

            private:
                Randomizer  sRand;

                lcg_dist_t  enDistribution;

                float       fAmplitude;
                float       fOffset;

            public:
                explicit LCG();
                ~LCG();

                void construct();
                void destroy();

            public:

                /** Initialize random generator
                 *
                 * @param seed seed
                 */
                void init(uint32_t seed);

                /** Initialize random generator, take current time as seed
                 */
                void init();

                /** Set the distribution for the noise.
                 *
                 * @param dist distribution for the noise sequence.
                 */
                inline void set_distribution(lcg_dist_t dist)
                {
                    if ((dist < LCG_UNIFORM) || (dist >= LCG_MAX))
                        return;

                    enDistribution = dist;
                }

                /** Set the amplitude of the LCG sequence.
                 *
                 * @param amplitude amplitude value for the sequence.
                 */
                inline void set_amplitude(float amplitude)
                {
                    if (amplitude == fAmplitude)
                        return;

                    fAmplitude = amplitude;
                }

                /** Set the offset of the LCG sequence.
                 *
                 * @param offset offset value for the sequence.
                 */
                inline void set_offset(float offset)
                {
                    if (offset == fOffset)
                        return;

                    fOffset = offset;
                }

                /** Get a sample from the LCG generator.
                 *
                 * @return the next sample in the LCG sequence.
                 */
                float single_sample_processor();

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

#endif /* LSP_PLUG_IN_DSP_UNITS_NOISE_LCG_H_ */
