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

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/misc/fade.h>

namespace lsp
{
    namespace dspu
    {
        LSP_DSP_UNITS_PUBLIC
        void fade_in(float *dst, const float *src, size_t fade_len, size_t buf_len)
        {
            // Apply fade at the head part
            float k     = 1.0f / fade_len;
            fade_len    = lsp_min(fade_len, buf_len);
            for (size_t i=0; i < fade_len; ++i)
                dst[i]      = src[i] * i * k;

            // Copy the tail part
            if (buf_len > fade_len)
                dsp::move(&dst[fade_len], &src[fade_len], buf_len - fade_len);
        }

        LSP_DSP_UNITS_PUBLIC
        void fade_out(float *dst, const float *src, size_t fade_len, size_t buf_len)
        {
            // Copy the head part
            if (buf_len > fade_len)
                dsp::move(dst, src, buf_len - fade_len);

            // Apply fade at the tail part
            float k     = 1.0f / fade_len;
            fade_len    = lsp_min(fade_len, buf_len);
            src        += buf_len - fade_len;
            dst        += buf_len - fade_len;

            for (size_t i=fade_len; i > 0; )
                *(dst++) = *(src++) * ((--i) * k);
        }
    }
}


