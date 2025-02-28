/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 23 марта 2016 г.
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

#include <lsp-plug.in/dsp-units/util/Randomizer.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/runtime/system.h>

#define RAND_RANGE          2.32830643654e-10 /* 1 / (1 << 32) */
#define RAND_LAMBDA         M_E * M_SQRT2

#define RAND_T              0.5f

namespace lsp
{
    namespace dspu
    {
        const uint32_t Randomizer::vMul1[] =
        {
            0x43ca16c1, 0x451222f3, 0x465e0183, 0x47f27263,
            0x4212ffe9, 0x4433f6ad, 0x40f31425, 0x412318bb,
            0x48f39cbf, 0x49b18a45, 0x4d341bbf, 0x4e93a169,
            0x4bacd5e5, 0x4c55e139, 0x4f11db4d, 0x4a901f8b
        };

        const uint32_t Randomizer::vMul2[] =
        {
            0x4c37c68f, 0x4d59b853, 0x4ef1d1e9, 0x4fe16c01,
            0x40fc2271, 0x44e335c1, 0x450fc1bb, 0x48cc3d07,
            0x493737a9, 0x4182e63f, 0x42198197, 0x43fc5611,
            0x4ac116eb, 0x4b0faf0d, 0x46777db9, 0x4730a64d
        };

        const uint32_t Randomizer::vAdders[] =
        {
            0x000551ff, 0x000633f5, 0x00011fcf, 0x00021b81,
            0x00075af1, 0x00080be5, 0x000330a7, 0x00040d0b,
            0x000c2521, 0x000dd113, 0x0009eea5, 0x000ae007,
            0x00092df5, 0x000b42bd, 0x000e1b15, 0x000f054d
        };

        Randomizer::Randomizer()
        {
            construct();
        }

        Randomizer::~Randomizer()
        {
            destroy();
        }

        void Randomizer::construct()
        {
            for (size_t i=0; i<4; ++i)
            {
                vRandom[i].vAdd     = 0;
                vRandom[i].vMul1    = 0;
                vRandom[i].vMul2    = 0;
                vRandom[i].vLast    = 0;
            }

            nBufID = -1;
        }

        void Randomizer::destroy()
        {
        }

        void Randomizer::init(uint32_t seed)
        {
            for (size_t i=0; i<4; ++i)
            {
                uint32_t reseed     = (i > 0) ? (seed << (i * 8)) | (seed >> ((sizeof(uint32_t) - i) * 8)) : seed;

                vRandom[i].vAdd     = vAdders[reseed & 0x0f];
                vRandom[i].vMul1    = vMul1[(reseed >> 4) & 0x0f];
                vRandom[i].vMul2    = vMul2[(reseed >> 8) & 0x0f];
                vRandom[i].vLast    = reseed ^ (seed >> 4);
            }

            nBufID      = 0;
        }

        void Randomizer::init()
        {
            system::time_t ts;
            system::get_time(&ts);
            init(uint32_t(ts.seconds ^ ts.nanos));
        }

        float Randomizer::generate_linear()
        {
            // Generate linear random number
            randgen_t *rg   = &vRandom[nBufID];
            nBufID          = (nBufID + 1) & 0x03;
            rg->vLast       = (rg->vMul1 * rg->vLast) + ((rg->vMul2 * rg->vLast) >> 16) + rg->vAdd;
            return rg->vLast * RAND_RANGE;
        }

        float Randomizer::random(random_function_t func)
        {
            float rv = generate_linear();

            // Now we can analyze algorithm
            switch (func)
            {
                case RND_EXP:
                    return (expf(RAND_LAMBDA * rv) - 1.0f) / (expf(RAND_LAMBDA) - 1.0f);

                case RND_TRIANGLE:
                    return (rv <= 0.5f) ?
                        M_SQRT2 * RAND_T * sqrtf(rv) :
                        2.0f*RAND_T - sqrtf(4.0f - 2.0f*(1.0f + rv)) * RAND_T;

                case RND_GAUSSIAN:
                {
                    // Two uniform (linear) random numbers are needed. (from https://www.dspguide.com/ch2/6.htm)
                    float rv2 = generate_linear();

                    return sqrtf(-2.0f * logf(rv)) * cosf(2.0f * M_PI * rv2);
                }

                default:
                    return rv;
            }
        }

        void Randomizer::dump(IStateDumper *v) const
        {
            v->begin_array("vRandom", vRandom, 4);
            for (size_t i=0; i<4; ++i)
            {
                const randgen_t *r = &vRandom[i];
                v->begin_object(r, sizeof(randgen_t));
                {
                    v->write("vLast", r->vLast);
                    v->write("vMul1", r->vMul1);
                    v->write("vMul2", r->vMul2);
                    v->write("vAdd", r->vAdd);
                }
                v->end_object();
            }
            v->end_array();

            v->write("nBufID", nBufID);
        }
    } /* namespace dspu */
} /* namespace lsp */
