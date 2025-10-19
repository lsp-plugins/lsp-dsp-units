/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Stefano Tronci <stefano.tronci@tuta.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 Oct 2025.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_SHAPER_H_
#define LSP_PLUG_IN_DSP_UNITS_SHAPER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/shaping/shaping.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        enum sh_function_t
        {
            SH_FCN_SINUSOIDAL,
            SH_FCN_POLYNOMIAL,
            SH_FCN_HYPERBOLIC,
            SH_FCN_EXPONENTIAL,
            SH_FCN_POWER,
            SH_FCN_BILINEAR,
            SH_FCN_ASYMMETRIC_CLIP,
            SH_FCN_ASYMMETRIC_SOFTCLIP,
            SH_FCN_QUARTER_CYCLE,
            SH_FCN_RECTIFIER,
            SH_FCN_BITCRUSH_FLOOR,
            SH_FCN_BITCRUSH_CEIL,
            SH_FCN_BITCRUSH_ROUND,
            SH_FCN_TAP_TUBEWARMTH
        };

        class LSP_DSP_UNITS_PUBLIC Shaper
        {
            enum update_t
            {
                UPD_SINUSOIDAL              = 1 << 0,
                UPD_POLYNOMIAL              = 1 << 1,
                UPD_HYPERBOLIC              = 1 << 2,
                UPD_EXPONENTIAL             = 1 << 3,
                UPD_POWER                   = 1 << 4,
                UPD_BILINEAR                = 1 << 5,
                UPD_ASYMMETRIC_CLIP         = 1 << 6,
                UPD_ASYMMETRIC_SOFTCLIP     = 1 << 7,
                UPD_QUARTER_CYCLE           = 1 << 8,
                UPD_RECTIFIER               = 1 << 9,
                UPD_BITCRUSH_FLOOR          = 1 << 10,
                UPD_BITCRUSH_CEIL           = 1 << 11,
                UPD_BITCRUSH_ROUND          = 1 << 12,
                UPD_TAP_TUBEWARMTH          = 1 << 13,

                UPD_ALL =
                        UPD_SINUSOIDAL | UPD_POLYNOMIAL |
                        UPD_HYPERBOLIC | UPD_EXPONENTIAL |
                        UPD_BILINEAR | UPD_ASYMMETRIC_CLIP |
                        UPD_ASYMMETRIC_SOFTCLIP | UPD_QUARTER_CYCLE |
                        UPD_RECTIFIER | UPD_BITCRUSH_FLOOR |
                        UPD_BITCRUSH_CEIL | UPD_BITCRUSH_ROUND |
                        UPD_TAP_TUBEWARMTH
            };

            private:
                dspu::shaping::shaping_t    sParams;

                size_t                      nSampleRate;
                sh_function_t               enFunction;
                size_t                      nUpdate;

            public:
                explicit Shaper();
                Shaper(const Shaper &) = delete;
                Shaper(Shaper &&) = delete;
                ~Shaper();

                Shaper & operator = (const Shaper &) = delete;
                Shaper & operator = (Shaper &&) = delete;

                void construct();
                void destroy();

            protected:
                void update_settings();

            public:

                /** Initialize the shaper.
                 *
                 */
                void init();

                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                void set_sample_rate(size_t sr);

                /** Set the sinusoidal function slope.
                 *
                 * @param slope slope
                 */
                void set_sinusoidal_slope(float slope);

                /** Set the polynomial function shape.
                 *
                 * @param shape shape
                 */
                void set_polynomial_shape(float shape);

                /** Set the hyperbolic function shape.
                 *
                 * @param shape shape
                 */
                void set_hyperbolic_shape(float shape);

                /** Set the exponential function shape.
                 *
                 * @param shape shape
                 */
                void set_exponential_shape(float shape);

                /** Set the bilinear function shape.
                 *
                 * @param shape shape
                 */
                void set_bilinear_shape(float shape);

                /** Set the asymmetric clip function high clip level.
                 *
                 * @param high_clip high clip level
                 */
                void set_asymmetric_clip_high_clip(float high_clip);

                /** Set the asymmetric clip function low clip level.
                 *
                 * @param low_clip low clip level
                 */
                void set_asymmetric_clip_low_clip(float low_clip);

                /** Set the asymmetric softclip function high clip limit level.
                 *
                 * @param high_limit high clip limit level
                 */
                void set_asymmetric_softclip_high_limit(float high_limit);

                /** Set the asymmetric softclip function low clip limit level.
                 *
                 * @param low_limit low clip limit level
                 */
                void set_asymmetric_softclip_low_limit(float low_limit);

                /** Set the quarter circle function radius.
                 *
                 * @param radius radius
                 */
                void set_quarter_circle_radius(float radius);

                /** Set the rectifier function shape.
                 *
                 * @param shape shape
                 */
                void set_rectifier_shape(float shape);

                /** Set the bitcrush floor function levels.
                 *
                 * @param levels levels
                 */
                void set_bitcrush_floor_levels(float levels);

                /** Set the bitcrush ceil function levels.
                 *
                 * @param levels levels
                 */
                void set_bitcrush_ceil_levels(float levels);

                /** Set the bitcrush round function levels.
                 *
                 * @param levels levels
                 */
                void set_bitcrush_round_levels(float levels);

                /** Set the TAP Tubewarmth function drive.
                 *
                 * @param drive drive
                 */
                void set_tap_tubewarmth_drive(float drive);

                /** Set the TAP Tubewarmth function blend.
                 *
                 * @param blend blend
                 */
                void set_tap_tubewarmth_blend(float blend);

                /** Output sequence to the destination buffer in additive mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to synthesise
                 */
                void process_add(float *dst, const float *src, size_t count);

                /** Output sequence to the destination buffer in multiplicative mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process_mul(float *dst, const float *src, size_t count);

                /** Output sequence to a destination buffer overwriting its content
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process_overwrite(float *dst, const float *src, size_t count);

                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_SHAPER_H_ */
