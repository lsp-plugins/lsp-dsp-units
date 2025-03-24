/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-phaser
 * Created on: 24 мар. 2025 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_QUICKMATH_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_QUICKMATH_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr float      QMATH_LOG2TOLN              = 1.0f / M_LOG2E;
        static constexpr float      QMATH_PI_DIV_2              = M_PI * 0.5f;
        static constexpr float      QMATH_LNTOLOG2              = M_LOG2E;
        static constexpr float      QMATH_LN2                   = M_LN2;

        /**
         * Quick computation of sine value with precision loss
         * @param x the argument, for precise computation it should be within range [-M_PI/2, M_PI/2]
         * @return sine value
         */
        inline float quick_sinf(float x)
        {
            const float x2 = x*x;
            return x * (1.0f + x2*(-0.166666666667f + x2*(0.00833333333333f + x2 * -0.000198412698413f)));
        }

        /**
         * Quick computation of cosine value with precision loss
         * @param x the argument, for precise computation it should be within range [0, M_PI]
         * @return cosine value
         */
        inline float quick_cosf(float x)
        {
            return quick_sinf(QMATH_PI_DIV_2 - x);
        }

        /**
         * Quick logarithm computation with precision loss
         * @param x the argument
         * @return logarithm value
         */
        inline float quick_logf(float x)
        {
            // Using taylor series of arctanh function that can express the logarithm value as:
            // ln(x) = 2 * arctanh((x-1)/(x+1))
            union { float f; uint32_t i; } u;

            // Get mantissa and reset it for the input argument
            u.f = x;
            const int f = (u.i >> 23) - 127; // f = mant(x)
            u.i = (u.i & 0x007fffff) | 0x3f800000;

            // Prepare for calculations
            const float X = u.f;
            const float y = (X - 1.0f)/(X + 1.0f);
            const float y2 = y*y;

            return 2.0f*y * (1.0f + y2*(0.333333333333f + y2*(0.2f + y2*(0.142857142857f)))) + f * QMATH_LOG2TOLN;
        }

        /**
         * Quick exponent function computation with precision loss
         * @param x the argument
         * @return exponent value
         */
        inline float quick_expf(float x)
        {
            const float xp  = fabsf(x) * QMATH_LNTOLOG2;
            const int n     = xp;
            const float r   = (xp - float(n)) * QMATH_LN2;

            union { float f; uint32_t i; } u;
            u.i             = (n + 127) << 23;       // u.f = 2 ^ r

            const float p   = u.f *
                (1.0f + r * (1.0f + r * (0.5f + r * (0.166666666667f + r * 0.0416666666667f))));

            return (x < 0.0f) ? 1.0f / p : p;
        }

        /**
         * Linear interpolation between two floating-point values
         * @param a the value at k = 0.0f
         * @param b the value at k = 1.0f
         * @param k the interpolation coefficient in range [0.0f, 1.0f]
         * @return the interpolated value
         */
        inline float lerp(float a, float b, float k)
        {
            return a + (b - a) * k;
        }

        /**
         * Linear interpolation between two integer values
         * @param a the value at k = 0.0f
         * @param b the value at k = 1.0f
         * @param k the interpolation coefficient in range [0.0f, 1.0f]
         * @return the interpolated value
         */
        inline int32_t ilerp(int32_t a, int32_t b, float k)
        {
            return a + (b - a) * k;
        }

        /**
         * Constant power (quadratic) interpolation between two floating-point values
         * @param a the value at k = 0.0f
         * @param b the value at k = 1.0f
         * @param k the interpolation coefficient in range [0.0f, 1.0f]
         * @return the interpolated value
         */
        inline float qlerp(float a, float b, float k)
        {
            return a * sqrtf(1.0f - k) + b * sqrtf(k);
        }

        /**
         * Exponential interpolation between two logarithmic floating-point values
         * @param a the value at k=0.0f
         * @param b the value at k=1.0f
         * @param k the interpolation coefficient in range [0.0f, 1.0f]
         * @return the interpolated value
         */
        inline float elerp(float a, float b, float k)
        {
            return a * expf(logf(b / a) * k);
        }

        /**
         * Exponential interpolation between two logarithmic floating-point values using quick logarithm
         * and exponent functions
         *
         * @param a the value at k=0.0f
         * @param b the value at k=1.0f
         * @param k the interpolation coefficient in range [0.0f, 1.0f]
         * @return the interpolated value
         */
        inline float quick_elerp(float a, float b, float k)
        {
            return a * quick_expf(quick_logf(b / a) * k);
        }

    } /* namespace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_QUICKMATH_H_ */
