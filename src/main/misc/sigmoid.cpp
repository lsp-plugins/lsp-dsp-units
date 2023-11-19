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

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/misc/sigmoid.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        namespace sigmoid
        {
            LSP_DSP_UNITS_PUBLIC
            float hard_clip(float x)
            {
                if (x < -1.0f)
                    return -1.0f;
                if (x > 1.0f)
                    return 1.0f;
                return x;
            }

            LSP_DSP_UNITS_PUBLIC
            float quadratic(float x)
            {
                if (x < 0.0f)
                    return (x > -2.0f) ? x * (1.0f - 0.25f * x) : -1.0f;

                return (x < 2.0f) ? x * (1.0f - 0.25f * x) : 1.0f;
            }

            LSP_DSP_UNITS_PUBLIC
            float sine(float x)
            {
                if (x < -M_PI_2)
                    return -1.0f;
                if (x > M_PI_2)
                    return 1.0f;
                return sinf(x);
            }

            LSP_DSP_UNITS_PUBLIC
            float logistic(float x)
            {
                return 1.0f / (1.0f + expf(x));
            }

            LSP_DSP_UNITS_PUBLIC
            float arctangent(float x)
            {
                return M_2_PI * atanf(M_PI_2 * x);
            }

            LSP_DSP_UNITS_PUBLIC
            float hyperbolic_tangent(float x)
            {
                return x / (1.0f + fabsf(x));
            }

            LSP_DSP_UNITS_PUBLIC
            float hyperbolic(float x)
            {
                float t = expf(x + x);
                return (t - 1.0f) / (t + 1.0f);
            }

            LSP_DSP_UNITS_PUBLIC
            float guidermannian(float x)
            {
                float t = expf(M_PI * x);
                return 4.0f * M_1_PI * atanf((t - 1.0f) / (t + 1.0f));
            }

            LSP_DSP_UNITS_PUBLIC
            float error(float x)
            {
                float t;
                const float nx = (1.0f/M_2_SQRTPI) * x;
                const float ex = expf(-nx*nx);

                if (x >= 0.0f)
                {
                    t = 1.0f / (1.0f + 0.3275911f * x);
                    return 1.0f - t * ex * (0.254829592f + t*(-0.284496736f + t*(1.421413741f + t*(-1.453152027f + t*1.061405429f))));
                }

                t = 1.0f / (1.0f - 0.3275911f * x);
                return -1.0f + t * ex * (0.254829592f + t*(-0.284496736f + t*(1.421413741f + t*(-1.453152027f + t*1.061405429f))));
            }

            LSP_DSP_UNITS_PUBLIC
            float smoothstep(float x)
            {
                if (x <= -1.0f)
                    return -1.0f;
                if (x >= 1.0f)
                    return 1.0f;

                float t = 0.5f * x + 1.0f;
                return 2.0f * t*t * (3.0f - 2.0f*t) - 1.0f;
            }

            LSP_DSP_UNITS_PUBLIC
            float smootherstep(float x)
            {
                if (x <= -1.0f)
                    return -1.0f;
                if (x >= 1.0f)
                    return 1.0f;

                float t = 0.5f * x + 1.0f;
                return 2.0f * t*t*t * (10.0f + t*(-15.0f + 6.0f*t)) - 1.0f;
            }

            LSP_DSP_UNITS_PUBLIC
            float circle(float x)
            {
                return x / sqrtf(1.0f + x*x);
            }

        } /* namespace sigmoid */
    } /* dspu */
} /* namespace lsp */


