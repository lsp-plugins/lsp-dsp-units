/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 23 сент. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_INTERPOLATION_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_INTERPOLATION_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace dspu
    {
        namespace interpolation
        {
            /** Perform quadratic Hermite interpolation
             *
             * The resulting polynom equation is the following:
             * y(x) = p[0]*x^2 + p[1]*x^ + p[2]
             *
             * @param p destination (3 floats) to store the final hermite polynom
             * @param x0 x-coordinate of first point used for interpolation
             * @param y0 y-coordinate of first point used for interpolation
             * @param k0 the tangent angle of the line at first point
             * @param x1 x-coordinate of second point used for interpolation
             * @param k1 the tangent angle of the line at second point
             */
            LSP_DSP_UNITS_PUBLIC
            void hermite_quadratic(float *p, float x0, float y0, float k0, float x1, float k1);

            /** Perform cubic Hermite interpolation.
             *
             * The resulting polynom equation is the following:
             * y(x) = p[0]*x^3 + p[1]*x^2 + p[2]*x + p[3]
             *
             * @param p destination (4 floats) to store the final hermite polynom
             * @param x0 x-coordinate of first point used for interpolation
             * @param y0 y-coordinate of first point used for interpolation
             * @param k0 the tangent angle of the line at first point
             * @param x1 x-coordinate of second point used for interpolation
             * @param y1 y-coordinate of second point used for interpolation
             * @param k1 the tangent angle of the line at second point
             */
            LSP_DSP_UNITS_PUBLIC
            void hermite_cubic(float *p, float x0, float y0, float k0, float x1, float y1, float k1);

            /** Perform quadro Hermite interpolation.
             *
             * The resulting polynom equation is the following:
             * y(x) = p[0]*x^4 + p[1]*x^3 + p[2]*x^2 + p[3]*x + p[4]
             *
             * @param p destination (4 floats) to store the final hermite polynom
             * @param x0 x-coordinate of first point used for interpolation
             * @param y0 y-coordinate of first point used for interpolation
             * @param k0 the tangent angle of the line at first point
             * @param x1 x-coordinate of second point used for interpolation
             * @param y1 y-coordinate of second point used for interpolation
             * @param k1 the tangent angle of the line at second point
             * @param x2 x-coordinate of third point used for interpolation
             * @param y2 y-coordinate of third point used for interpolation
             */
            LSP_DSP_UNITS_PUBLIC
            void hermite_quadro(
                float *p,
                float x0, float y0, float k0,
                float x1, float y1, float k1,
                float x2, float y2);

            /** Perform penta Hermite interpolation.
             *
             * The resulting polynom equation is the following:
             * y(x) = p[0]*x^5 + p[1]*x^4 + p[2]*x^3 + p[3]*x^2 + p[4]*x + p[5]
             *
             * @param p destination (4 floats) to store the final hermite polynom
             * @param x0 x-coordinate of first point used for interpolation
             * @param y0 y-coordinate of first point used for interpolation
             * @param k0 the tangent angle of the line at first point
             * @param x1 x-coordinate of second point used for interpolation
             * @param y1 y-coordinate of second point used for interpolation
             * @param k1 the tangent angle of the line at second point
             * @param x2 x-coordinate of third point used for interpolation
             * @param y2 y-coordinate of third point used for interpolation
             * @param k2 the tangent angle of the line at third point
             */
            LSP_DSP_UNITS_PUBLIC
            void hermite_penta(
                float *p,
                float x0, float y0, float k0,
                float x1, float y1, float k1,
                float x2, float y2, float k2);

            /** Perform exponent interpolation
             *
             * @param p destination (3 floats) to store the formula: y(x) = p[0] + p[1] * exp(p[2] * x)
             * @param x0 x-coordinate of first point used for interpolation
             * @param y0 y-coordinate of first point used for interpolation
             * @param x1 x-coordinate of second point used for interpolation
             * @param y1 y-coordinate of second point used for interpolation
             * @param k growing/lowering coefficient
             */
            LSP_DSP_UNITS_PUBLIC
            void exponent(float *p, float x0, float y0, float x1, float y1, float k);

            /** Perform linear interpolation
             *
             * @param p destination (2 floats) to store the formula: y(x) = p[0]*x + p[1]
             * @param x0 x-coordinate of first point used for interpolation
             * @param y0 y-coordinate of first point used for interpolation
             * @param x1 x-coordinate of second point used for interpolation
             * @param y1 y-coordinate of second point used for interpolation
             */
            LSP_DSP_UNITS_PUBLIC
            void linear(float *p, float x0, float y0, float x1, float y1);

        } /* namespace interpolation */
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_INTERPOLATION_H_ */
