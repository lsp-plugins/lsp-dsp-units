/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 23 марта 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_RANDOMIZER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_RANDOMIZER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        enum random_function_t
        {
            RND_LINEAR,     // Uniform over [0, 1)
            RND_EXP,        // Decaying exponential over [0, 1]
            RND_TRIANGLE,   // Triangle over [0, 1]
            RND_GAUSSIAN,   // Gaussian of mean 0 and standard deviation 1 (has negative values)
            RND_MAX
        };

        class LSP_DSP_UNITS_PUBLIC Randomizer
        {
            private:
                static const uint32_t vMul1[];
                static const uint32_t vMul2[];
                static const uint32_t vAdders[];

                typedef struct randgen_t
                {
                    uint32_t    vLast;
                    uint32_t    vMul1;
                    uint32_t    vMul2;
                    uint32_t    vAdd;
                } randgen_t;

                randgen_t   vRandom[4];
                size_t      nBufID;
                
			protected:
                float generate_linear();

            public:
                explicit Randomizer();
                Randomizer(const Randomizer &) = delete;
                Randomizer(Randomizer &&) = delete;
                ~Randomizer();

                Randomizer &operator = (const Randomizer &) = delete;
                Randomizer &operator = (Randomizer &&) = delete;

                /**
                 * Construct the randomizer
                 */
                void construct();

                /**
                 * Destroy the randomizer
                 */
                void destroy();

                /** Initialize random generator
                 *
                 * @param seed seed
                 */
                void init(uint32_t seed);

                /** Initialize random generator, take current time as seed
                 */
                void init();

            public:
                /** Generate float random number in range of [0..1) - excluding 1.0f
                 * The guaranteed tolerance is 1e-6 or 0.000001
                 *
                 * @param func function
                 * @return random number
                 */
                float random(random_function_t func = RND_LINEAR);

                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_RANDOMIZER_H_ */
