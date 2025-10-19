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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_SHAPING_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_SHAPING_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/stdlib/math.h>

// Epsilon value for TAP Tubewarmth
#define LSP_DSP_SHAPING_TAP_EPS     0.000000001f

namespace lsp
{
    namespace dspu
    {
        /**
         * This header defines different kinds of shaping functions.
         *
         * Shaping functions are parametrised saturating input-output mapping functions.
         * They accept any real number as input, but only return values in the [-1.0, +1.0] interval.
         * These functions are also parametrised by one or more parameters that affect their shape.
         */
        namespace shaping
        {

            /**
             * Parameters for a sinusoidal shaping function.
             * Modified from Function 1, page 204, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct sinusoidal_t
            {
                float slope;  // 0 < slope < M_PI / 2.0f
                float radius; // M_PI / (2 * slope)
            } sinusoidal_t;

            /**
             * Parameters for a polynomial shaping function.
             * Function 2, page 204, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct polynomial_t
            {
                float shape;  // 0 < shape <= 1
                float radius; // 1 - shape
            } polynomial_t;

            /**
             * Parameters for a hyperbolic shaping function.
             * Function 3, page 204, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct hyperbolic_t
            {
                float shape; // shape > 0
                float hyperbolic_shape; // quick_tanh(shape)
            } hyperbolic_t;

            /**
             * Parameters for an exponential shaping function.
             * Function 4, page 205, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct exponential_t
            {
                float shape; // shape > 1
                float log_shape; // quick_logf(shape)
                float scale; // shape / (shape - 1)
            } exponential_t;

            /**
             * Parameters for a power shaping function.
             * Function 5, page 205, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct power_t
            {
                float shape; // shape >= 1
            } power_t;

            /**
             * Parameters for a bilinear shaping function.
             * Function 6, page 205, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct bilinear_t
            {
                float shape; // shape >= 0
            } bilinear_t;

            /**
             * Parameters for an asymmetric clip shaping function.
             * Function 7, page 207, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct asymmetric_clip_t
            {
                float high_clip; // 0 <= high_clip <= 1
                float low_clip; // 0 <= low_clip <= 1
            } asymmetric_clip_t;

            /**
             * Parameters for an asymmetric softclip shaping function.
             * Function 8, page 207, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct asymmetric_softclip_t
            {
                float high_limit; // 0 <= high_clip < 1
                float low_limit; // 0 <= low_clip < 1
                float pos_scale; // 1 / (1 - high_limit)
                float neg_scale; // 1 / (1 - low_limit)
            } asymmetric_softclip_t;

            /**
             * Parameters for a quarter circle shaping function.
             * Modified from Function 9, page 209, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct quarter_circle_t
            {
                float radius; // radius > 0
                float radius2; // 2 * radius
            } quarter_circle_t;

            /**
             * Parameters for a rectifier shaping function.
             * Function 10, page 209, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             */
            typedef struct rectifier_t
            {
                float shape; // 0 <= shape <= 1
            } rectifier_t;

            /**
             * Parameters for a bitcrush shaping function.
             * Modified from Function 11, page 209, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * Floor, ceiling and round versions.
             */
            typedef struct bitcrush_floor_t
            {
                float levels; // levels >= 1
            } bitcrush_floor_t;

            typedef struct bitcrush_ceil_t
            {
                float levels; // levels >= 1
            } bitcrush_ceil_t;

            typedef struct bitcrush_round_t
            {
                float levels; // levels >= 1
            } bitcrush_round_t;

            /**
             * Parameters for a TAP Tubewarmth stateful shaping function.
             * From TAP Plugins: https://git.hq.sig7.se/tap-plugins.git
             */
            typedef struct tap_tubewarmth_t
            {
                // As in TAP Plugins: https://git.hq.sig7.se/tap-plugins.git, file tap_tubewarmth.c
                float drive;  // -10 < drive < +10
                float blend;  // 0.1 < blend < 10

                // All parameters below are computed from drive, blend and sample rate.

                // Amplitudes
                float pwrq;
                float srct;

                // Positive samples coefficients
                float ap;
                float kpa;
                float kpb;

                // Negative samples coefficients
                float an;
                float kna;
                float knb;

                // State
                float last_raw_output;
                float last_raw_intermediate;
            } tap_tubewarmth_t;

            /**
             * Parameters for shaping functions.
             */
            union shaping_t
            {
                sinusoidal_t sinusoidal;
                polynomial_t polynomial;
                hyperbolic_t hyperbolic;
                exponential_t exponential;
                power_t power;
                bilinear_t bilinear;
                asymmetric_clip_t asymmetric_clip;
                asymmetric_softclip_t asymmetric_softclip;
                quarter_circle_t quarter_circle;
                rectifier_t rectifier;
                bitcrush_floor_t bitcrush_floor;
                bitcrush_ceil_t bitcrush_ceil;
                bitcrush_round_t bitcrush_round;
                tap_tubewarmth_t tap_tubewarmth;
            };

            /**
             * Sinusoidal shaping function.
             * Modified from Function 1, page 204, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float sinusoidal(shaping_t *params, float value);

            /**
             * Polynomial shaping function.
             * Function 2, page 204, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float polynomial(shaping_t *params, float value);

            /**
             * Hyperbolic shaping function.
             * Function 3, page 204, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float hyperbolic(shaping_t *params, float value);

            /**
             * Exponential shaping function.
             * Function 4, page 205, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float exponential(shaping_t *params, float value);

            /**
             * Power shaping function.
             * Function 5, page 205, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float power(shaping_t *params, float value);

            /**
             * Bilinear shaping function.
             * Function 6, page 205, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float bilinear(shaping_t *params, float value);

            /**
             * Asymmetric clip shaping function.
             * Function 7, page 207, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float asymmetric_clip(shaping_t *params, float value);

            /**
             * Asymmetric softclip shaping function.
             * Function 8, page 207, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float asymmetric_softclip(shaping_t *params, float value);

            /**
             * Quarter circle shaping function.
             * Modified from Function 9, page 209, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float quarter_circle(shaping_t *params, float value);

            /**
             * Rectifier shaping function.
             * Function 10, page 209, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float rectifier(shaping_t *params, float value);

            /**
             * Bitcrush floor shaping function.
             * Modified from Function 11, page 209, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float bitcrush_floor(shaping_t *params, float value);

            /**
             * Bitcrush ceil shaping function.
             * Modified from Function 11, page 209, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float bitcrush_ceil(shaping_t *params, float value);

            /**
             * Bitcrush round shaping function.
             * Modified from Function 11, page 209, Audio Processes, 1st Edition, ISBN: 978-1-138-10011-4.
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float bitcrush_round(shaping_t *params, float value);

            /**
             * TAP Plugins gate function, building block for TAP Tubewarmth.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            inline float tap_gate(float value)
            {
                if ((value < -LSP_DSP_SHAPING_TAP_EPS) || (value > LSP_DSP_SHAPING_TAP_EPS))
                    return value;
                else
                    return 0.0f;
            }

            /**
             * TAP Plugins rectifying square root function, building block for TAP Tubewarmth.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            inline float tap_rect_sqrt(float value)
            {
               if (value > LSP_DSP_SHAPING_TAP_EPS)
                   return sqrtf(value);
               else if (value < -LSP_DSP_SHAPING_TAP_EPS)
                   return sqrtf(-value);
               else
                   return 0.0f;
            }

            /**
             * TAP tubewarmth stateful shaping function.
             * As in TAP Plugins: https://git.hq.sig7.se/tap-plugins.git, file tap_tubewarmth.c
             * @param params shaping function parameters.
             * @param value argument for the shaping function.
             * @return shaping function output.
             */
            LSP_DSP_UNITS_PUBLIC
            float tap_tubewarmth(shaping_t *params, float value);

        } /* namespace shaping */
    } /* dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_SHAPING_H_ */
