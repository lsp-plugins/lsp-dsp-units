/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 21 дек. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_DITHER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_DITHER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/util/Randomizer.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Dither class: generates dither noise with specified characteristics
         */
        class Dither
        {
            private:
                Dither &operator = (const Dither &);
                Dither(const Dither &);

            protected:
                size_t      nBits;
                float       fGain;
                float       fDelta;
                Randomizer  sRandom;

            public:
                explicit Dither();
                ~Dither();

                /**
                 * Create object
                 */
                void        construct();

            public:
                /** Initialize dither
                 *
                 */
                inline void init() { sRandom.init(); };

                /** Set number of bits per sample
                 *
                 * @param bits number of bits per sample
                 */
                void set_bits(size_t bits);

                /** Process signal
                 *
                 * @param out output signal
                 * @param in input signal
                 * @param count number of samples to process
                 */
                void process(float *out, const float *in, size_t count);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_DITHER_H_ */
