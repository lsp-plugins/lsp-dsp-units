/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Stefano Tronci <stefano.tronci@tuta.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 28 Sept 2025.
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

#include <lsp-plug.in/dsp-units/misc/shaping.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp-units/misc/quickmath.h>

namespace lsp
{
    namespace dspu
    {
        namespace shaping
        {

            LSP_DSP_UNITS_PUBLIC
            float sinusoidal(shaping_t *params, float value)
            {
                const float radius = M_PI / (2.0f * params->sinusoidal.slope);

                if (value >= radius)
                    return 1.0f;

                if (value <= -radius)
                    return -1.0f;

                return quick_sinf(params->sinusoidal.slope * value);
            }

            LSP_DSP_UNITS_PUBLIC
            float polynomial(shaping_t *params, float value)
            {
                const float radius = 1.0f - params->polynomial.shape;

                if (value >= 1.0f)
                    return 1.0f;

                if (value <= -1.0f)
                    return -1.0f;

                if ((value <= radius) || (value >= -radius))
                    return (2.0f * value) / (2.0f - params->polynomial.shape);

                float value2 = value * value;
                float c_m_1_2 = (params->polynomial.shape - 1.0f) * (params->polynomial.shape - 1.0f);

                if ((value > radius))
                    return (value2 - 2 * value + c_m_1_2) / (params->polynomial.shape * (params->polynomial.shape - 2.0f));

                return (value2 + 2 * value + c_m_1_2) / (params->polynomial.shape * (2.0f - params->polynomial.shape));
            }

            LSP_DSP_UNITS_PUBLIC
            float hyperbolic(shaping_t *params, float value)
            {
                if (value >= 1.0f)
                    return 1.0f;

                if (value <= -1.0f)
                    return -1.0f;

                return quick_tanh(params->polynomial.shape * value) / quick_tanh(params->polynomial.shape);
            }

            LSP_DSP_UNITS_PUBLIC
            float exponential(shaping_t *params, float value)
            {
                if (value >= 1.0f)
                    return 1.0f;

                if (value <= -1.0f)
                    return -1.0f;

                // We must compute shape^value. We can use the fast functions to change the base into e.
                const float log_shape = quick_logf(params->exponential.shape);
                const float exp_arg_abs = log_shape * value;
                const float prop = params->exponential.shape / (params->exponential.shape - 1.0f);

                if (value > 0.0f)
                    return prop * (1.0f - quick_expf(-exp_arg_abs));

                return prop * (-1.0f + quick_expf(exp_arg_abs));
            }

            LSP_DSP_UNITS_PUBLIC
            float power(shaping_t *params, float value)
            {
                if (value >= 1.0f)
                    return 1.0f;

                if (value <= -1.0f)
                    return -1.0f;

                // We compute (1 - value)^shape by converting to base e so we can use the quick functions.
                if (value > 0.0f)
                    return 1.0f - quick_expf(params->power.shape * quick_logf(1.0f - value));

                // We compute (1 + value)^shape by converting to base e so we can use the quick functions.
                return -1.0f + quick_expf(params->power.shape * quick_logf(1.0f + value));
            }

            LSP_DSP_UNITS_PUBLIC
            float bilinear(shaping_t *params, float value)
            {
                if (value >= 1.0f)
                    return 1.0f;

                if (value <= -1.0f)
                    return -1.0f;

                const float numerator = (1.0f + params->bilinear.shape) * value;

                if (value > 0.0f)
                    return numerator / (1.0f + params->bilinear.shape * value);

                return numerator / (1.0f - params->bilinear.shape * value);
            }

            LSP_DSP_UNITS_PUBLIC
            float asymmetric_clip(shaping_t *params, float value)
            {
                if (value >= params->asymmetric_clip.high_clip)
                    return params->asymmetric_clip.high_clip;

                if (value <= -params->asymmetric_clip.low_clip)
                    return -params->asymmetric_clip.low_clip;

                return value;
            }

            LSP_DSP_UNITS_PUBLIC
            float asymmetric_softclip(shaping_t *params, float value)
            {
                if (value >= params->asymmetric_softclip.high_limit)
                    return params->asymmetric_softclip.high_limit;

                if (value <= -params->asymmetric_softclip.low_limit)
                    return -params->asymmetric_softclip.low_limit;

                // We return 0.0f if the input is 0.0f. Exact 0.0f input might lead to issues with quick_logf.
                if (value == 0.0f)
                    return 0.0f;

                const float pos_scale = 1.0f / (1.0f - params->asymmetric_softclip.high_limit);

                // We compute value^pos_scale, but by changing base to e so that we use the quick functions.
                if (value > 0.0f)
                    return value - quick_expf(pos_scale * quick_logf(value)) / pos_scale;

                const float neg_scale = 1.0f / (1.0f - params->asymmetric_softclip.low_limit);

                // We compute (-value)^neg_scale, but by changing base to e so that we use the quick functions.
                // At this point we are sure value < 0, so -value > 0.
                return value + quick_expf(neg_scale * quick_logf(-value)) / neg_scale;
            }

            LSP_DSP_UNITS_PUBLIC
            float quarter_circle(shaping_t *params, float value)
            {
                if (value >= params->quarter_circle.radius)
                    return params->quarter_circle.radius;
                if (value <= -params->quarter_circle.radius)
                    return -params->quarter_circle.radius;

                const float radius2 = 2.0f * params->quarter_circle.radius;

                if (value >= 0.0f)
                    return sqrtf(value * (radius2 - value));

                return -sqrtf(-value * (radius2 + value));
            }

            LSP_DSP_UNITS_PUBLIC
            float rectifier(shaping_t *params, float value)
            {
                if (value > -params->rectifier.shape)
                    return value;

                return -2.0f * params->rectifier.shape - value;
            }

            LSP_DSP_UNITS_PUBLIC
            float bitcrush(shaping_t *params, float value)
            {
                switch (params->bitcrush.fcn)
                {
                    case ROUND_FCN_FLOOR:
                        return floorf(params->bitcrush.levels * value) / params->bitcrush.levels;
                    case ROUND_FCN_CEIL:
                        return ceilf(params->bitcrush.levels * value) / params->bitcrush.levels;

                    default:
                    case ROUND_FCN_ROUND:
                        return roundf(params->bitcrush.levels * value) / params->bitcrush.levels;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            float tap_tubewarmth(shaping_t *params, float value)
            {
                float raw_intermediate;

                // For convenience: the equations below need less typing and no accidental overwrites will happen.
                const tap_tubewarmth_t *pars = &params->tap_tubewarmth;

                if (value >= 0.0f)
                {
                    raw_intermediate = pars->pwrq * (
                            tap_rect_sqrt(pars->ap + value * (pars->kpa - value)) + pars->kpb
                            );
                }
                else
                {
                    raw_intermediate = -pars->pwrq * (
                            tap_rect_sqrt(pars->an - value * (pars->kna + value)) + pars->knb
                            );
                }

                const float last_gated_intermediate = tap_gate(pars->last_raw_intermediate);
                const float last_gated_output = tap_gate(pars->last_raw_output);

                float output = pars->srct * (raw_intermediate - last_gated_intermediate + last_gated_output);

                // Update the state before leaving.
                params->tap_tubewarmth.last_raw_intermediate = raw_intermediate;
                params->tap_tubewarmth.last_raw_output = output;
                return output;
            }
        } /* namespace shaping */
    } /* dspu */
} /* namespace lsp */
