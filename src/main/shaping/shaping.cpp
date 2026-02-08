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

#include <lsp-plug.in/dsp-units/shaping/shaping.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp-units/misc/quickmath.h>
#include <lsp-plug.in/common/bits.h>

namespace lsp
{
    namespace dspu
    {
        namespace shaping
        {

            LSP_DSP_UNITS_PUBLIC
            float sinusoidal(shaping_t *params, float value)
            {
                if (value >= params->sinusoidal.radius)
                    return 1.0f;

                if (value <= -params->sinusoidal.radius)
                    return -1.0f;

                return quick_sinf(params->sinusoidal.slope * value);
            }

            LSP_DSP_UNITS_PUBLIC
            float polynomial(shaping_t *params, float value)
            {
                if (value >= 1.0f)
                    return 1.0f;

                if (value <= -1.0f)
                    return -1.0f;

                if ((value <= params->polynomial.radius) && (value >= -params->polynomial.radius))
                    return (2.0f * value) / (2.0f - params->polynomial.shape);

                float value2 = value * value;
                float c_m_1_2 = (params->polynomial.shape - 1.0f) * (params->polynomial.shape - 1.0f);

                if ((value > params->polynomial.radius))
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

                return quick_tanh(params->hyperbolic.shape * value) / params->hyperbolic.hyperbolic_shape;
            }

            LSP_DSP_UNITS_PUBLIC
            float exponential(shaping_t *params, float value)
            {
                if (value >= 1.0f)
                    return 1.0f;

                if (value <= -1.0f)
                    return -1.0f;

                // We must compute shape^value. We can use the fast functions to change the base into e.
                // log_shape serves this purpose.
                const float exp_arg_abs = params->exponential.log_shape * value;

                if (value > 0.0f)
                    return params->exponential.scale * (1.0f - quick_expf(-exp_arg_abs));

                return params->exponential.scale * (-1.0f + quick_expf(exp_arg_abs));
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
                if (value >= 1.0f)
                    return params->asymmetric_softclip.high_limit;

                if (value <= -1.0f)
                    return -params->asymmetric_softclip.low_limit;

                // We return 0.0f if the input is 0.0f. Exact 0.0f input might lead to issues with quick_logf.
                if (value == 0.0f)
                    return 0.0f;

                // We compute value^pos_scale, but by changing base to e so that we use the quick functions.
                if (value > 0.0f)
                    return value - quick_expf(params->asymmetric_softclip.pos_scale * quick_logf(value)) / params->asymmetric_softclip.pos_scale;

                // We compute (-value)^neg_scale, but by changing base to e so that we use the quick functions.
                // At this point we are sure value < 0, so -value > 0.
                return value + quick_expf(params->asymmetric_softclip.neg_scale * quick_logf(-value)) / params->asymmetric_softclip.neg_scale;
            }

            LSP_DSP_UNITS_PUBLIC
            float quarter_circle(shaping_t *params, float value)
            {
                if (value >= params->quarter_circle.radius)
                    return params->quarter_circle.radius;
                if (value <= -params->quarter_circle.radius)
                    return -params->quarter_circle.radius;

                if (value >= 0.0f)
                    return sqrtf(
                        params->quarter_circle.radius2 -
                        (value - params->quarter_circle.radius) * (value - params->quarter_circle.radius)
                        );

                return -sqrtf(
                    params->quarter_circle.radius2 -
                    (-value - params->quarter_circle.radius) * (-value - params->quarter_circle.radius)
                    );
            }

            LSP_DSP_UNITS_PUBLIC
            float rectifier(shaping_t *params, float value)
            {
                if (value >= 1.0f)
                    return 1.0f;

                if (value <= -1.0f)
                    return 1.0f - 2.0f * params->rectifier.shape;

                if (value > -params->rectifier.shape)
                    return value;

                return -2.0f * params->rectifier.shape - value;
            }

            LSP_DSP_UNITS_PUBLIC
            float bitcrush_floor(shaping_t *params, float value)
            {
                return floorf(params->bitcrush_floor.levels * value) / params->bitcrush_floor.levels;
            }

            LSP_DSP_UNITS_PUBLIC
            float bitcrush_ceil(shaping_t *params, float value)
            {
                return ceilf(params->bitcrush_ceil.levels * value) / params->bitcrush_ceil.levels;
            }

            LSP_DSP_UNITS_PUBLIC
            float bitcrush_round(shaping_t *params, float value)
            {
                return roundf(params->bitcrush_round.levels * value) / params->bitcrush_round.levels;
            }

            LSP_DSP_UNITS_PUBLIC
            float continuous_a_law_compression(shaping_t *params, float value)
            {
                float magv = fabsf(value);

                if (magv < params->continuous_a_law_compression.compression_reciprocal)
                {
                    return (params->continuous_a_law_compression.compression * value) * params->continuous_a_law_compression.scale;
                }

                return quick_signf(value) * (1.0f + quick_logf(params->continuous_a_law_compression.compression * magv)) * params->continuous_a_law_compression.scale;
            }

            LSP_DSP_UNITS_PUBLIC
            float continuous_a_law_expansion(shaping_t *params, float value)
            {
                float magv = fabsf(value);

                if (magv < params->continuous_a_law_expansion.radius)
                {
                    return value * params->continuous_a_law_expansion.radius_reciprocal * params->continuous_a_law_expansion.expansion_reciprocal;
                }

                float power = quick_expf(-1.0f + magv * params->continuous_a_law_expansion.radius_reciprocal);
                return quick_signf(value) * power * params->continuous_a_law_expansion.expansion_reciprocal;
            }

            LSP_DSP_UNITS_PUBLIC
            float continuous_mu_law_compression(shaping_t *params, float value)
            {
                return quick_signf(value) * quick_logf(1.0f + params->continuous_mu_law_compression.compression * fabsf(value)) * params->continuous_mu_law_compression.scale;
            }

            LSP_DSP_UNITS_PUBLIC
            float continuous_mu_law_expansion(shaping_t *params, float value)
            {
                // We need to compute (1 - μ)^|value|. We convert to base e so that we can use quick_expf.
                float power = quick_expf(fabsf(value) * quick_logf(1.0f + params->continuous_mu_law_expansion.expansion));
                return quick_signf(value) * (power- 1.0f) * params->continuous_mu_law_expansion.expansion_reciprocal;
            }

            LSP_DSP_UNITS_PUBLIC
            int16_t a_law_float_to_pcm(shaping_t *params, float value)
            {
                return lsp_limit(value, -1.0f, 1.0f) * params->quantized_a_law_companding.pcm_clip;
            }

            LSP_DSP_UNITS_PUBLIC
            uint8_t quantized_a_law_compression(int16_t value)
            {
                // Also see https://en.wikipedia.org/wiki/G.711#A-law

                // We first find the sign flag.
                uint8_t sign = 0x00;
                if (value >= 0)
                    sign = 0x80;

                // Then, we compute the magnitude.
                int16_t magnitude = value;
                if (magnitude < 0)
                    magnitude *= -1;

                // Then, we find the segment value (column 3, page 3, https://www.itu.int/rec/dologin_pub.asp?lang=e&id=T-REC-G.711-198811-I!!PDF-E&type=items).
                // Essentially, we figure out in what power of 2 magnitude is.
                uint8_t segment;
                if (magnitude < 64)
                    segment = 1;
                else if (magnitude >= 4096)
                    segment = 7;
                else
                    segment = int_log2(magnitude) - 4;

                // The low nibble is easily constructed by shifting the magnitude by segment.
                uint8_t low_nibble = (magnitude >> segment) & 0x000F;

                // The high nibble is simply the segment number, except if magnitude < 32. In that case it is 0x00.
                uint8_t high_nibble = 0x00;
                if (magnitude >= 32)
                    high_nibble = segment;

                // Now we put everything together: high nibble, low nibble and we set the sign.
                // (we should also XOR with 0b01010101 now but that is an useless operation for us, as we do no need to transmit this value).
                return sign | (((high_nibble << 4) | low_nibble) & 0x7F);
            }

            LSP_DSP_UNITS_PUBLIC
            float a_law_compressed_to_float(uint8_t value)
            {
                // Also see https://en.wikipedia.org/wiki/G.711#A-law

                // Let's get the sign. It is in the leftmost bit).
                float sign = -1.0f;
                if (value & 0x80)
                    sign = 1.0f;

                // Let's get the magnitude. Basically, it is everything but the sign bit.
                // We scale by the highest possible 7 bit value, 127.
                float magnitude = static_cast<float>(value & 0x7F) / 127.0f;

                // Ready to return:
                return sign * magnitude;
            }

            LSP_DSP_UNITS_PUBLIC
            uint8_t a_law_linear_encoding(float value)
            {
                uint8_t sign = 0x00;
                if (value >= 0.0f)
                    sign = 0x80;

                // We scale by the highest possible 7 bit value, 127.
                uint8_t magnitude = static_cast<uint8_t>(lsp_min(fabs(value), 1.0f) * 127.0f);

                // We return a value encoded in a A-law fashion:
                return sign | magnitude;
            }

            LSP_DSP_UNITS_PUBLIC
            int16_t quantized_a_law_expansion(uint8_t value)
            {
                // Also see https://en.wikipedia.org/wiki/G.711#A-law
                // However, note that here we use int16_t's 2 complement to represent signed values,
                // as that makes it easier down the line (converting to float) and is less ambiguous.

                // Let's get the sign. It is in the leftmost bit).
                int16_t sign = -1;
                if (value & 0x80)
                    sign = 1;

                // Let's get the exponent. This is bits 5 to 7.
                int16_t exponent = (value & 0x70) >> 4;

                // Lets get the mantissa. This is the first 4 bits.
                int16_t mantissa = value & 0x0F;

                // Let's construct the word. This is 1mmmm1, where m are the mantissa bits.
                // Unless exponent == 0. In that case it is 0mmmm1.
                int16_t word;
                if (exponent == 0)
                    word = 0x0001 | (mantissa << 1);
                else
                    word = 0x0021 | (mantissa << 1);

                // We now shift the word by the amount set by exponent and set the sign.
                return sign * (word << lsp_max(0, exponent - 1));
            }

            LSP_DSP_UNITS_PUBLIC
            float a_law_expandend_to_float(shaping_t *params, int16_t value)
            {
                return static_cast<float>(value) / static_cast<float>(params->quantized_a_law_companding.pcm_clip);
            }

            LSP_DSP_UNITS_PUBLIC
            float quatized_a_law_compression_shaper(shaping_t *params, float value)
            {
                return a_law_compressed_to_float(quantized_a_law_compression(a_law_float_to_pcm(params, value)));
            }

            LSP_DSP_UNITS_PUBLIC
            float quatized_a_law_expansion_shaper(shaping_t *params, float value)
            {
                return a_law_expandend_to_float(params, quantized_a_law_expansion(a_law_linear_encoding(value)));
            }

            LSP_DSP_UNITS_PUBLIC
            int16_t mu_law_float_to_pcm(shaping_t *params, float value)
            {
                return lsp_limit(value, -1.0f, 1.0f) * params->quantized_mu_law_companding.pcm_clip;
            }

            LSP_DSP_UNITS_PUBLIC
            uint8_t quantized_mu_law_compression(shaping_t *params, int16_t value)
            {
                // Also see https://en.wikipedia.org/wiki/G.711#%CE%BC-law

                // We first find the sign flag.
                uint8_t sign = 0x00;
                if (value >= 0)
                    sign = 0x80;

                // Then, we compute the magnitude.
                int16_t magnitude = value;
                if (magnitude < 0)
                    magnitude *= -1;
                // μ-law has to be biased:
                magnitude += params->quantized_mu_law_companding.pcm_bias;

                // Ensure we do not bump into illegal values after bias.
                magnitude = lsp_limit(
                        magnitude,
                        -params->quantized_mu_law_companding.pcm_max_magnitude,
                        params->quantized_mu_law_companding.pcm_max_magnitude
                    );

                // Then, we find the segment value (column 3, page 5, https://www.itu.int/rec/dologin_pub.asp?lang=e&id=T-REC-G.711-198811-I!!PDF-E&type=items).
                // Essentially, we figure out in what power of 2 magnitude is.
                // Note how we already added the bias to the magnitude, so we need to add 33 to the levels in column 3, page 5.
                uint8_t segment;
                if (magnitude < 64)
                    segment = 1;
                else if (magnitude >= 8192)
                    segment = 8;
                else
                    segment = int_log2(magnitude) - 4;

                // The low nibble is easily constructed by shifting the magnitude by segment.
                uint8_t low_nibble = (magnitude >> segment) & 0x000F;

                // The high nibble is simply segment - 1.
                uint8_t high_nibble = segment - 1;

                // G.711 prescribes that the bits must be flipped (with the exception of the sign bit).
                return sign | (~(((high_nibble << 4) | low_nibble)) & 0x7F);
            }

            LSP_DSP_UNITS_PUBLIC
            float mu_law_compressed_to_float(uint8_t value)
            {
                // Also see https://en.wikipedia.org/wiki/G.711#%CE%BC-law

                // Let's get the sign. It is in the leftmost bit).
                float sign = -1.0f;
                if (value & 0x80)
                    sign = 1.0f;

                // Let's get the magnitude. Basically, it is everything but the sign bit.
                // However, remember that G.711 wanted us to flip the bits.
                // We scale by the highest possible 7 bit value, 127.
                float magnitude = static_cast<float>(~value & 0x7F) / 127.0f;

                // Ready to return:
                return sign * magnitude;
            }

            LSP_DSP_UNITS_PUBLIC
            float mu_law_linear_encoding(float value)
            {
                uint8_t sign = 0x00;
                 if (value >= 0.0f)
                     sign = 0x80;

                 // We scale by the highest possible 7 bit value, 127.
                 uint8_t magnitude = static_cast<uint8_t>(lsp_min(fabs(value), 1.0f) * 127.0f);

                 // We return a value encoded in a  μ-law fashion.
                 // G.711 prescribes that the bits must be flipped (with the exception of the sign bit).
                 return sign | (~magnitude & 0x7F);
            }

            LSP_DSP_UNITS_PUBLIC
            int16_t quantized_mu_law_expansion(uint8_t value)
            {
                // Also see https://en.wikipedia.org/wiki/G.711#%CE%BC-law
                // However, note that here we use int16_t's 2 complement to represent signed values,
                // as that makes it easier down the line (converting to float) and is less ambiguous.

                 // Let's get the sign. It is in the leftmost bit).
                 int16_t sign = -1;
                 if (value & 0x80)
                     sign = 1;

                 // Let's get the exponent. This is bits 5 to 7.
                 // Remember that G.711 wanted us to flip the bits.
                 int16_t exponent = (~value & 0x70) >> 4;

                 // Lets get the mantissa. This is the first 4 bits.
                 // Remember that G.711 wanted us to flip the bits.
                 int16_t mantissa = ~value & 0x0F;

                 // Let's construct the word. This is 1mmmm1, where m are the mantissa bits.
                 int16_t word = 0x0021 | (mantissa << 1);

                 // We now shift the word by exponent and set the sign.
                 return sign * (word << exponent);
            }

            LSP_DSP_UNITS_PUBLIC
            float mu_law_expandend_to_float(shaping_t *params, int16_t value)
            {
                return static_cast<float>(value) / static_cast<float>(params->quantized_mu_law_companding.pcm_clip);
            }

            LSP_DSP_UNITS_PUBLIC
            float quatized_mu_law_compression_shaper(shaping_t *params, float value)
            {
                return mu_law_compressed_to_float(quantized_mu_law_compression(params, mu_law_float_to_pcm(params, value)));
            }

            LSP_DSP_UNITS_PUBLIC
            float quatized_mu_law_expansion_shaper(shaping_t *params, float value)
            {
                return mu_law_expandend_to_float(params, quantized_mu_law_expansion(mu_law_linear_encoding(value)));
            }

            LSP_DSP_UNITS_PUBLIC
            float tap_gate(float value)
            {
                if ((value < -LSP_DSP_SHAPING_TAP_EPS) || (value > LSP_DSP_SHAPING_TAP_EPS))
                    return value;
                else
                    return 0.0f;
            }

            LSP_DSP_UNITS_PUBLIC
            float tap_rect_sqrt(float value)
            {
               if (value > LSP_DSP_SHAPING_TAP_EPS)
                   return sqrtf(value);
               else if (value < -LSP_DSP_SHAPING_TAP_EPS)
                   return sqrtf(-value);
               else
                   return 0.0f;
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
