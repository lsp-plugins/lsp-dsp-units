/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/dsp-units/misc/quickmath.h>

namespace lsp
{
    namespace dspu
    {
        namespace lfo
        {
            static constexpr float      LFO_REV_LN100           = 0.5f / M_LN10;
            static constexpr float      LFO_M_2PI               = 2.0f * M_PI;
            static constexpr float      LFO_M_4PI               = 4.0f * M_PI;

            LSP_DSP_UNITS_PUBLIC
            float triangular(float phase)
            {
                return (phase < 0.5f) ? phase * 2.0f : (1.0f - phase) * 2.0f;
            }

            LSP_DSP_UNITS_PUBLIC
            float sine(float phase)
            {
                return (phase >=0.5f) ?
                    0.5f + 0.5f * quick_sinf((0.75f - phase) * LFO_M_2PI) :
                    0.5f + 0.5f * quick_sinf((phase - 0.25f) * LFO_M_2PI);
            }

            LSP_DSP_UNITS_PUBLIC
            float step_sine(float phase)
            {
                if (phase >= 0.5f)
                {
                    return (phase >= 0.75f) ?
                        0.25f + 0.25f * quick_sinf((0.875f - phase) * LFO_M_4PI) :
                        0.75f + 0.25f * quick_sinf((0.625f - phase) * LFO_M_4PI);
                }

                return (phase >= 0.25f) ?
                    0.75f + 0.25f * sinf((phase - 0.375f) * LFO_M_4PI) :
                    0.25f + 0.25f * sinf((phase - 0.125f) * LFO_M_4PI);
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
                return quick_logf(1.0f + 198.0f *phase) * LFO_REV_LN100;
            }

            LSP_DSP_UNITS_PUBLIC
            float rev_logarithmic(float phase)
            {
                if (phase >= 0.5f)
                    phase      = 1.0f - phase;
                return 1.0f - quick_logf(100.0f - 198.0f * phase) * LFO_REV_LN100;
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


