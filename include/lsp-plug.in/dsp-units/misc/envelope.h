/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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

            LSP_DSP_UNITS_PUBLIC
            void noise(float *dst, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void reverse_noise(float *dst, size_t n, envelope_t type);

            LSP_DSP_UNITS_PUBLIC
            void white_noise(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void pink_noise(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void brown_noise(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void blue_noise(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void violet_noise(float *dst, size_t n);
        }
    }
}

#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_ENVELOPE_H_ */
