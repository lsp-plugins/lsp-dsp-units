/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 21 февр. 2021 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_FADE_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_FADE_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace dspu
    {
        /** Fade-in (with range check)
         *
         * @param dst destination buffer
         * @param src source buffer
         * @param fade_len length of fade (in elements)
         * @param buf_len length of the buffer
         */
        void fade_in(float *dst, const float *src, size_t fade_len, size_t buf_len);

        /** Fade-out (with range check)
         *
         * @param dst destination buffer
         * @param src source buffer
         * @param fade_len length of fade (in elements)
         * @param buf_len length of the buffer
         */
        void fade_out(float *dst, const float *src, size_t fade_len, size_t buf_len);
    }
}



#endif /* INCLUDE_LSP_PLUG_IN_DSP_UNITS_MISC_FADE_H_ */
