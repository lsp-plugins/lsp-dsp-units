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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_ENVELOPE_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_ENVELOPE_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace dspu
    {
        namespace envelope
        {
            enum envelope_t
            {
                VIOLET_NOISE,
                BLUE_NOISE,
                WHITE_NOISE,
                PINK_NOISE,
                BROWN_NOISE,
                MINUS_4_5_DB,
                PLUS_4_5_DB,

                // Special variables
                TOTAL,
                FIRST = VIOLET_NOISE,
                LAST = TOTAL - 1
            };

            /**
             * Generate noise envelope using linear frequency scale
             * @param dst pointer to write envelope coefficients
             * @param first the first frequency in the range, non-negative
             * @param last the last frequency in the range, non-negative, greater than first
             * @param center frequency that will have amplification coefficient 1.0, non-negative
             * @param n number of element to generate
             * @param type envelope type
             */
            LSP_DSP_UNITS_PUBLIC
            void noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type);

            /**
             * Generate compensating (reverse) noise envelope using linear frequency scale
             * @param dst pointer to write envelope coefficients
             * @param first the first frequency in the range, non-negative
             * @param last the last frequency in the range, non-negative, greater than first
             * @param center frequency that will have amplification coefficient 1.0, non-negative
             * @param n number of element to generate
             * @param type envelope type
             */
            LSP_DSP_UNITS_PUBLIC
            void reverse_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type);

            /**
             * Generate noise envelope using logarithmic frequency scale
             * @param dst pointer to write envelope coefficients
             * @param first the first frequency in the range, non-negative
             * @param last the last frequency in the range, non-negative, greater than first
             * @param center frequency that will have amplification coefficient 1.0, non-negative
             * @param n number of element to generate
             * @param type envelope type
             */
            LSP_DSP_UNITS_PUBLIC
            void noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type);

            /**
             * Generate compensating (reverse) noise envelope using logarithmic frequency scale
             * @param dst pointer to write envelope coefficients
             * @param first the first frequency in the range, non-negative
             * @param last the last frequency in the range, non-negative, greater than first
             * @param center frequency that will have amplification coefficient 1.0, non-negative
             * @param n number of element to generate
             * @param type envelope type
             */
            LSP_DSP_UNITS_PUBLIC
            void reverse_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type);

            /**
             * Generate noise envelope using frequency list
             * @param dst pointer to write envelope coefficients
             * @param freq list of frequencies, each frequency should be positive
             * @param center frequency that will have amplification coefficient 1.0, non-negative
             * @param n number of element to generate
             * @param type envelope type
             */
            LSP_DSP_UNITS_PUBLIC
            void noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type);

            /**
             * Generate compensating (reverse) noise envelope using frequency list
             * @param dst pointer to write envelope coefficients
             * @param freq list of frequencies, each frequency should be positive
             * @param center frequency that will have amplification coefficient 1.0, non-negative
             * @param n number of element to generate
             * @param type envelope type
             */
            LSP_DSP_UNITS_PUBLIC
            void reverse_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type);


            LSP_DSP_UNITS_PUBLIC
            void white_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void pink_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void brown_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void blue_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void violet_noise_lin(float *dst, float first, float last, float center, size_t n, envelope_t type);


            LSP_DSP_UNITS_PUBLIC
            void white_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void pink_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void brown_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void blue_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void violet_noise_log(float *dst, float first, float last, float center, size_t n, envelope_t type);


            LSP_DSP_UNITS_PUBLIC
            void white_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void pink_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void brown_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void blue_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void violet_noise_list(float *dst, const float *freqs, float center, size_t n, envelope_t type);

        } /* namespace envelope */
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_ENVELOPE_H_ */
