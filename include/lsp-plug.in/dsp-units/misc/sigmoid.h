/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 18 нояб. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_SIGMOID_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_SIGMOID_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * This header defines different kind of sigmoid functions.
         *
         * Each function computes an output value in range of [-1.0 .. +1.0]
         * depending on the input argument. The function is odd and symmetric
         * across the zero. For the zero value, the function value is 0.0 and
         * the first derivative is 1.0.
         */
        namespace sigmoid
        {
            /**
             * Pointer to sigmoid function
             * @param x argument for sigmoid function
             * @return computed value
             */
            typedef float (* function_t)(float x);

            /** Hard-clipping function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        hard_clip(float x);

            /** Quadratic function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        quadratic(float x);

            /** Sine function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        sine(float x);

            /** Logistic function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        logistic(float x);

            /** Arctangent function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        arctangent(float x);

            /** Hyperbolic tangent function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        hyperbolic_tangent(float x);

            /** Hyperbolic function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        hyperbolic(float x);

            /** Guidermannian function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        guidermannian(float x);

            /** Error function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        error(float x);

            /** Smoothstep function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        smoothstep(float x);

            /** Smootherstep function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        smootherstep(float x);

            /** Circle function
             * @param x argument for the sigmoid function
             * @return computed value
             */
            LSP_DSP_UNITS_PUBLIC
            float        circle(float x);

        } /* namespace sigmoid */
    } /* dspu */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_SIGMOID_H_ */
