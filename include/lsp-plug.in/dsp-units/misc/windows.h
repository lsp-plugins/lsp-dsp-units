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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_WINDOWS_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_WINDOWS_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace dspu
    {
        namespace windows
        {
            enum window_t
            {
                HANN,
                HAMMING,
                BLACKMAN,
                LANCZOS,
                GAUSSIAN,
                POISSON,
                PARZEN,
                TUKEY,
                WELCH,
                NUTTALL,
                BLACKMAN_NUTTALL,
                BLACKMAN_HARRIS,
                HANN_POISSON,
                BARTLETT_HANN,
                BARTLETT_FEJER,
                TRIANGULAR,
                RECTANGULAR,
                FLAT_TOP,
                COSINE,
                SQR_COSINE,
                CUBIC,

                // Special variables
                TOTAL,
                FIRST = HANN,
                LAST = TOTAL - 1
            };

            LSP_DSP_UNITS_PUBLIC
            void window(float *dst, size_t n, window_t type);

            LSP_DSP_UNITS_PUBLIC
            void rectangular(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void triangular_general(float *dst, size_t n, int dn);

            LSP_DSP_UNITS_PUBLIC
            void triangular(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void bartlett_fejer(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void parzen(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void welch(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void hamming_general(float *dst, size_t n, float a, float b);

            LSP_DSP_UNITS_PUBLIC
            void hann(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void hamming(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void blackman_general(float *dst, size_t n, float a);

            LSP_DSP_UNITS_PUBLIC
            void blackman(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void nutall_general(float *dst, size_t n, float a0, float a1, float a2, float a3);

            LSP_DSP_UNITS_PUBLIC
            void nuttall(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void blackman_nuttall(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void blackman_harris(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void flat_top_general(float *dst, size_t n, float a0, float a1, float a2, float a3, float a4);

            LSP_DSP_UNITS_PUBLIC
            void flat_top(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void cosine(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void sqr_cosine(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void cubic(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void gaussian_general(float *dst, size_t n, float s);

            LSP_DSP_UNITS_PUBLIC
            void gaussian(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void poisson_general(float *dst, size_t n, float t);

            LSP_DSP_UNITS_PUBLIC
            void poisson(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void lanczos(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void bartlett_hann_general(float *dst, size_t n, float a0, float a1, float a2);

            LSP_DSP_UNITS_PUBLIC
            void hann_poisson_general(float *dst, size_t n, float a);

            LSP_DSP_UNITS_PUBLIC
            void hann_poisson(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void bartlett_hann(float *dst, size_t n);

            LSP_DSP_UNITS_PUBLIC
            void tukey_general(float *dst, size_t n, float a);

            LSP_DSP_UNITS_PUBLIC
            void tukey(float *dst, size_t n);
        }
    }
}

#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_WINDOWS_H_ */
