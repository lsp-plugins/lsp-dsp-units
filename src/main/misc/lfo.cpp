/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 10 июн. 2023 г.
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

#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp-units/misc/lfo.h>

namespace lsp
{
    namespace dspu
    {
        namespace lfo
        {
            static constexpr float      REV_LN100               = 0.5f / M_LN10;
            static constexpr float      LOG2TOLN                = 1.0f / M_LOG2E;

            static inline float taylor_sinf(float x)
            {
                // Taylor series sine approximation for better performance
                // The argument should be within range -M_PI/2 ... M_PI/2
                const float x2 = x*x;
                return x * (1.0f + x2*(-0.166666666667f + x2*(0.00833333333333f + x2 * -0.000198412698413f)));
            }

            static inline float taylor_logf(float x)
            {
                // Using taylor series of arctanh function that can express the logarithm value as:
                // ln(x) = 2 * arctanh((x-1)/(x+1))
                union { float f; uint32_t i; } u;

                // Get mantissa and reset it for the input argument
                u.f = x;
                const int f = (u.i >> 23) - 127; // f = mant(x)
                u.i = (u.i & 0x007fffff) | 0x3f800000;

                // Prepare for calculations
                const float X = u.f;
                const float y = (X - 1.0f)/(X + 1.0f);
                const float y2 = y*y;

                return 2.0f*y * (1.0f + y2*(0.333333333333f + y2*(0.2f + y2*(0.142857142857f)))) + f * LOG2TOLN;
            }


            LSP_DSP_UNITS_PUBLIC
            float triangular(float phase)
            {
                return (phase < 0.5f) ? phase * 2.0f : (1.0f - phase) * 2.0f;
            }

            LSP_DSP_UNITS_PUBLIC
            float sine(float phase)
            {
                return (phase >=0.5f) ?
                    0.5f + 0.5f*taylor_sinf((0.75f - phase) * M_PI * 2.0f) :
                    0.5f + 0.5f*taylor_sinf((phase - 0.25f) * M_PI * 2.0f);
            }

            LSP_DSP_UNITS_PUBLIC
            float step_sine(float phase)
            {
                if (phase >= 0.5f)
                {
                    return (phase >= 0.75f) ?
                        0.25f + 0.25f * taylor_sinf((0.875f - phase) * M_PI * 4.0f) :
                        0.75f + 0.25f * taylor_sinf((0.625f - phase) * M_PI * 4.0f);
                }

                return (phase >= 0.25f) ?
                    0.75f + 0.25f * sinf((phase - 0.375f) * M_PI * 4.0f) :
                    0.25f + 0.25f * sinf((phase - 0.125f) * M_PI * 4.0f);
            }

            LSP_DSP_UNITS_PUBLIC
            float cubic(float phase)
            {
                if (phase >= 0.5f)
                    phase      = 1.0f - phase;

                return phase * phase * (12.0f - 16.0f * phase);
            }

            LSP_DSP_UNITS_PUBLIC
            float step_cubic(float phase)
            {
                if (phase >= 0.5f)
                    phase      = 1.0f - phase;

                phase      -= 0.25f;
                return 0.5f + 32.0f * phase * phase * phase;
            }

            LSP_DSP_UNITS_PUBLIC
            float parabolic(float phase)
            {
                phase -= 0.5f;
                return 1.0f - 4.0f * phase * phase;
            }

            LSP_DSP_UNITS_PUBLIC
            float rev_parabolic(float phase)
            {
                if (phase >= 0.5f)
                    phase      = 1.0f - phase;

                return 4.0f * phase * phase;
            }

            LSP_DSP_UNITS_PUBLIC
            float logarithmic(float phase)
            {
                if (phase >= 0.5f)
                    phase      = 1.0f - phase;
                return taylor_logf(1.0f + 198.0f *phase) * REV_LN100;
            }

            LSP_DSP_UNITS_PUBLIC
            float rev_logarithmic(float phase)
            {
                if (phase >= 0.5f)
                    phase      = 1.0f - phase;
                return 1.0f - logf(100.0f - 198.0f * phase) * REV_LN100;
            }

            LSP_DSP_UNITS_PUBLIC
            float sqrt(float phase)
            {
                phase      -= 0.5f;
                return sqrtf(1.0f - 4.0f * phase * phase);
            }

            LSP_DSP_UNITS_PUBLIC
            float rev_sqrt(float phase)
            {
                if (phase >= 0.5f)
                    phase          -= 1.0f;
                return 1.0f - sqrtf(1.0f - 4.0f * phase * phase);
            }

            LSP_DSP_UNITS_PUBLIC
            float circular(float phase)
            {
                if (phase < 0.25f)
                    return 0.5f - sqrtf(0.25f - 4.0f * phase * phase);

                if (phase > 0.75f)
                {
                    phase          -= 1.0f;
                    return 0.5f - sqrtf(0.25f - 4.0f * phase * phase);
                }

                phase      -= 0.5f;
                return 0.5f + sqrtf(0.25f - 4.0f * phase * phase);
            }

            LSP_DSP_UNITS_PUBLIC
            float rev_circular(float phase)
            {
                if (phase >= 0.5f)
                    phase   = 1.0f - phase;

                phase -= 0.25f;
                return (phase < 0.0f) ?
                    sqrtf(0.25f - 4.0f * phase * phase) :
                    1.0f - sqrtf(0.25f - 4.0f * phase * phase);
            }

        } /* namespace lfo */
    } /* dspu */
} /* namespace lsp */


