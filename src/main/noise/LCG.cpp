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

#include <lsp-plug.in/dsp-units/noise/LCG.h>

namespace lsp
{
    namespace dspu
    {
        LCG::LCG()
        {
            construct();
        }

        LCG::~LCG()
        {
            destroy();
        }

        void LCG::construct()
        {
            enDistribution  = LCG_UNIFORM;
            fAmplitude      = 1.0f;
            fOffset         = 0.0f;
        }

        void LCG::destroy()
        {

        }

        void LCG::init(uint32_t seed)
        {
            sRand.init(seed);
        }

        void LCG::init()
        {
            sRand.init();
        }

        float LCG::process_single()
        {
            switch (enDistribution)
            {
                case LCG_EXPONENTIAL:
                {
                    float sign;
                    if (sRand.random(RND_LINEAR) >= 0.5f)
                        sign = 1.0f;
                    else
                        sign = -1.0f;

                    return sign * fAmplitude * sRand.random(RND_EXP) + fOffset;
                }

                case LCG_TRIANGULAR:
                    return 2.0f * fAmplitude * sRand.random(RND_TRIANGLE) - 0.5f + fOffset;

                case LCG_GAUSSIAN:
                    return fAmplitude * sRand.random(RND_GAUSSIAN) + fOffset;

                default:
                case LCG_UNIFORM:
                    return 2.0f * fAmplitude * (sRand.random(RND_LINEAR) - 0.5f) + fOffset;
            }
        }

        void LCG::process_add(float *dst, const float *src, size_t count)
        {
            while (count--)
                *(dst++) = *(src++) + process_single();
        }

        void LCG::process_mul(float *dst, const float *src, size_t count)
        {
            while (count--)
                *(dst++) = *(src++) * process_single();
        }

        void LCG::process_overwrite(float *dst, size_t count)
        {
            while (count--)
                *(dst++) = process_single();
        }

        void LCG::dump(IStateDumper *v) const
        {
            v->write_object("sRand", &sRand);

            v->write("enDistribution", enDistribution);

            v->write("fAmplitude", fAmplitude);
            v->write("fOffset", fOffset);
        }
    }
}

