/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 20 февр. 2016 г.
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

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/misc/envelope.h>
#include <lsp-plug.in/dsp-units/const.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        namespace envelope
        {
            static constexpr float FREQ_RANGE           = LSP_DSP_UNITS_SPEC_FREQ_MAX / LSP_DSP_UNITS_SPEC_FREQ_MIN;
            static constexpr float PLUS_4_5_DB_CONST    = 4.5f / (20.0f * float(M_LOG10_2));
            static constexpr float MINUS_4_5_DB_CONST   = -4.5f / (20.0f * float(M_LOG10_2));
            static constexpr float BLUE_CONST           = 0.5f; // logf(2.0f) / logf(4.0f);
            static constexpr float VIOLET_CONST         = 1.0f;
            static constexpr float BROWN_CONST          = -1.0f;
            static constexpr float PINK_CONST           = -0.5f; // logf(0.5f) / logf(4.0f);

            static inline void basic_noise(float *dst, size_t n, float k)
            {
                if (n == 0)
                    return;

                dsp::lramp_set1(dst, 0.0f, FREQ_RANGE, n);
                dst[0] = 1.0f;
                dsp::powvc1(dst, k, n);
            }

            LSP_DSP_UNITS_PUBLIC
            void white_noise(float *dst, size_t n)
            {
                dsp::fill_one(dst, n);
            }

            LSP_DSP_UNITS_PUBLIC
            void pink_noise(float *dst, size_t n)
            {
                basic_noise(dst, n, PINK_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void brown_noise(float *dst, size_t n)
            {
                basic_noise(dst, n, BROWN_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void blue_noise(float *dst, size_t n)
            {
                basic_noise(dst, n, BLUE_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void violet_noise(float *dst, size_t n)
            {
                basic_noise(dst, n, VIOLET_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void noise(float *dst, size_t n, envelope_t type)
            {
                switch (type)
                {
                    case WHITE_NOISE:   white_noise(dst, n);    return;
                    case PINK_NOISE:    pink_noise(dst, n);     return;
                    case BROWN_NOISE:   brown_noise(dst, n);    return;
                    case BLUE_NOISE:    blue_noise(dst, n);     return;
                    case VIOLET_NOISE:  violet_noise(dst, n);   return;
                    case PLUS_4_5_DB:   basic_noise(dst, n, PLUS_4_5_DB_CONST);     return;
                    case MINUS_4_5_DB:  basic_noise(dst, n, MINUS_4_5_DB_CONST);    return;
                    default:
                        return;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void reverse_noise(float *dst, size_t n, envelope_t type)
            {
                switch (type)
                {
                    case WHITE_NOISE:   white_noise(dst, n);    return;
                    case PINK_NOISE:    blue_noise(dst, n);     return;
                    case BROWN_NOISE:   violet_noise(dst, n);   return;
                    case BLUE_NOISE:    pink_noise(dst, n);     return;
                    case VIOLET_NOISE:  brown_noise(dst, n);    return;
                    case PLUS_4_5_DB:   basic_noise(dst, n, MINUS_4_5_DB_CONST);    return;
                    case MINUS_4_5_DB:  basic_noise(dst, n, PLUS_4_5_DB_CONST);     return;
                    default:
                        return;
                }
            }

        } /* namespace envelope */
    } /* namespace dspu */
} /* namespace lsp */

