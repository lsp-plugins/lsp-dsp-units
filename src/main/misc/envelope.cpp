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
            static constexpr float PLUS_4_5_DB_CONST    = 4.5f / (20.0f * float(M_LOG10_2));
            static constexpr float MINUS_4_5_DB_CONST   = -4.5f / (20.0f * float(M_LOG10_2));
            static constexpr float BLUE_CONST           = 0.5f; // logf(2.0f) / logf(4.0f);
            static constexpr float VIOLET_CONST         = 1.0f;
            static constexpr float BROWN_CONST          = -1.0f;
            static constexpr float PINK_CONST           = -0.5f; // logf(0.5f) / logf(4.0f);

            static inline void basic_noise_lin(float *dst, float first, float last, float center, size_t n, float k)
            {
                if (n <= 1)
                {
                    if (n > 0)
                        dst[0] = 1.0f;
                    return;
                }

                // Generate frequencies
                const float kf  = 1.0f / center;
                first          *= kf;
                last           *= kf;
                const float df  = (last - first) / (n - 1);

                for (size_t i=0; i<n; ++i)
                    dst[i]          = first + df * i;
                if (dst[0] <= 0.0f)
                    dst[0]          = dst[1];

                dsp::powvc1(dst, k, n);
            }

            LSP_DSP_UNITS_PUBLIC
            void noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                switch (type)
                {
                    case WHITE_NOISE:
                        dsp::fill_one(dst, n);
                        break;
                    case PINK_NOISE:
                        basic_noise_lin(dst, first, last, center, n, PINK_CONST);
                        break;
                    case BROWN_NOISE:
                        basic_noise_lin(dst, first, last, center, n, BROWN_CONST);
                        break;
                    case BLUE_NOISE:
                        basic_noise_lin(dst, first, last, center, n, BLUE_CONST);
                        break;
                    case VIOLET_NOISE:
                        basic_noise_lin(dst, first, last, center, n, VIOLET_CONST);
                        break;
                    case PLUS_4_5_DB:
                        basic_noise_lin(dst, first, last, center, n, PLUS_4_5_DB_CONST);
                        break;
                    case MINUS_4_5_DB:
                        basic_noise_lin(dst, first, last, center, n, MINUS_4_5_DB_CONST);
                        break;
                    default:
                        return;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void reverse_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                switch (type)
                {
                    case WHITE_NOISE:
                        dsp::fill_one(dst, n);
                        break;
                    case PINK_NOISE:
                        basic_noise_lin(dst, first, last, center, n, BLUE_CONST);
                        break;
                    case BROWN_NOISE:
                        basic_noise_lin(dst, first, last, center, n, VIOLET_CONST);
                        break;
                    case BLUE_NOISE:
                        basic_noise_lin(dst, first, last, center, n, PINK_CONST);
                        break;
                    case VIOLET_NOISE:
                        basic_noise_lin(dst, first, last, center, n, BROWN_CONST);
                        break;
                    case PLUS_4_5_DB:
                        basic_noise_lin(dst, first, last, center, n, MINUS_4_5_DB_CONST);
                        break;
                    case MINUS_4_5_DB:
                        basic_noise_lin(dst, first, last, center, n, PLUS_4_5_DB_CONST);
                        break;
                    default:
                        return;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void white_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                dsp::fill_one(dst, n);
            }

            LSP_DSP_UNITS_PUBLIC
            void pink_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                basic_noise_lin(dst, first, last, center, n, PINK_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void brown_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                basic_noise_lin(dst, first, last, center, n, BROWN_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void blue_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                basic_noise_lin(dst, first, last, center, n, BLUE_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void violet_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                basic_noise_lin(dst, first, last, center, n, VIOLET_CONST);
            }


            static inline void basic_noise_log(float *dst, float first, float last, float center, size_t n, float k)
            {
                if (n <= 1)
                {
                    if (n > 0)
                        dst[0] = 1.0f;
                    return;
                }

                // Generate frequencies
                const float kf  = 1.0f / center;
                first          *= kf;
                last           *= kf;
                const float df  = logf(last/first) / (n - 1);

                for (size_t i=0; i<n; ++i)
                    dst[i]          = df * i;
                dsp::exp1(dst, n);
                dsp::mul_k2(dst, first, n);
                dsp::powvc1(dst, k, n);
            }

            LSP_DSP_UNITS_PUBLIC
            void noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                switch (type)
                {
                    case WHITE_NOISE:
                        dsp::fill_one(dst, n);
                        break;
                    case PINK_NOISE:
                        basic_noise_log(dst, first, last, center, n, PINK_CONST);
                        break;
                    case BROWN_NOISE:
                        basic_noise_log(dst, first, last, center, n, BROWN_CONST);
                        break;
                    case BLUE_NOISE:
                        basic_noise_log(dst, first, last, center, n, BLUE_CONST);
                        break;
                    case VIOLET_NOISE:
                        basic_noise_log(dst, first, last, center, n, VIOLET_CONST);
                        break;
                    case PLUS_4_5_DB:
                        basic_noise_log(dst, first, last, center, n, PLUS_4_5_DB_CONST);
                        break;
                    case MINUS_4_5_DB:
                        basic_noise_log(dst, first, last, center, n, MINUS_4_5_DB_CONST);
                        break;
                    default:
                        return;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void reverse_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                switch (type)
                {
                    case WHITE_NOISE:
                        dsp::fill_one(dst, n);
                        break;
                    case PINK_NOISE:
                        basic_noise_log(dst, first, last, center, n, BLUE_CONST);
                        break;
                    case BROWN_NOISE:
                        basic_noise_log(dst, first, last, center, n, VIOLET_CONST);
                        break;
                    case BLUE_NOISE:
                        basic_noise_log(dst, first, last, center, n, PINK_CONST);
                        break;
                    case VIOLET_NOISE:
                        basic_noise_log(dst, first, last, center, n, BROWN_CONST);
                        break;
                    case PLUS_4_5_DB:
                        basic_noise_log(dst, first, last, center, n, MINUS_4_5_DB_CONST);
                        break;
                    case MINUS_4_5_DB:
                        basic_noise_log(dst, first, last, center, n, PLUS_4_5_DB_CONST);
                        break;
                    default:
                        return;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void white_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                dsp::fill_one(dst, n);
            }

            LSP_DSP_UNITS_PUBLIC
            void pink_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                basic_noise_log(dst, first, last, center, n, PINK_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void brown_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                basic_noise_log(dst, first, last, center, n, BROWN_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void blue_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                basic_noise_log(dst, first, last, center, n, BLUE_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void violet_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type)
            {
                basic_noise_log(dst, first, last, center, n, VIOLET_CONST);
            }



            static inline void basic_noise_list(float *dst, const float *freqs, float center, size_t n, float k)
            {
                if (n <= 0)
                    return;

                // Generate frequencies
                dsp::mul_k3(dst, freqs, 1.0f / center, n);
                dsp::powvc1(dst, k, n);
            }

            LSP_DSP_UNITS_PUBLIC
            void noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type)
            {
                switch (type)
                {
                    case WHITE_NOISE:
                        dsp::fill_one(dst, n);
                        break;
                    case PINK_NOISE:
                        basic_noise_list(dst, freqs, center, n, PINK_CONST);
                        break;
                    case BROWN_NOISE:
                        basic_noise_list(dst, freqs, center, n, BROWN_CONST);
                        break;
                    case BLUE_NOISE:
                        basic_noise_list(dst, freqs, center, n, BLUE_CONST);
                        break;
                    case VIOLET_NOISE:
                        basic_noise_list(dst, freqs, center, n, VIOLET_CONST);
                        break;
                    case PLUS_4_5_DB:
                        basic_noise_list(dst, freqs, center, n, PLUS_4_5_DB_CONST);
                        break;
                    case MINUS_4_5_DB:
                        basic_noise_list(dst, freqs, center, n, MINUS_4_5_DB_CONST);
                        break;
                    default:
                        return;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void reverse_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type)
            {
                switch (type)
                {
                    case WHITE_NOISE:
                        dsp::fill_one(dst, n);
                        break;
                    case PINK_NOISE:
                        basic_noise_list(dst, freqs, center, n, BLUE_CONST);
                        break;
                    case BROWN_NOISE:
                        basic_noise_list(dst, freqs, center, n, VIOLET_CONST);
                        break;
                    case BLUE_NOISE:
                        basic_noise_list(dst, freqs, center, n, PINK_CONST);
                        break;
                    case VIOLET_NOISE:
                        basic_noise_list(dst, freqs, center, n, BROWN_CONST);
                        break;
                    case PLUS_4_5_DB:
                        basic_noise_list(dst, freqs, center, n, MINUS_4_5_DB_CONST);
                        break;
                    case MINUS_4_5_DB:
                        basic_noise_list(dst, freqs, center, n, PLUS_4_5_DB_CONST);
                        break;
                    default:
                        return;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void white_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type)
            {
                dsp::fill_one(dst, n);
            }

            LSP_DSP_UNITS_PUBLIC
            void pink_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type)
            {
                basic_noise_list(dst, freqs, center, n, PINK_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void brown_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type)
            {
                basic_noise_list(dst, freqs, center, n, BROWN_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void blue_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type)
            {
                basic_noise_list(dst, freqs, center, n, BLUE_CONST);
            }

            LSP_DSP_UNITS_PUBLIC
            void violet_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type)
            {
                basic_noise_list(dst, freqs, center, n, VIOLET_CONST);
            }

        } /* namespace envelope */
    } /* namespace dspu */
} /* namespace lsp */

