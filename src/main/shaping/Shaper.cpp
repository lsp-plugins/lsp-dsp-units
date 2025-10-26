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

#include <lsp-plug.in/dsp-units/shaping/Shaper.h>
#include <lsp-plug.in/dsp-units/misc/quickmath.h>
#include <lsp-plug.in/dsp/dsp.h>

#define SINUSOIDAL_MIN_SLOPE                1e-3f   // Must be > 0
#define SINUSOIDAL_MAX_SLOPE                M_PI_2

#define POLYNOMIAL_MIN_SHAPE                1e-3f   // Must be > 0

#define HYPERBOLIC_MAX_SHAPE                10.0f

#define EXPONENTIAL_MIN_SHAPE               1.001f  // Must be > 1
#define EXPONENTIAL_MAX_SHAPE               10.0f
#define EXPONENTIAL_CONV_SLOPE              (EXPONENTIAL_MAX_SHAPE - EXPONENTIAL_MIN_SHAPE)
#define EXPONENTIAL_CONV_INTRC              EXPONENTIAL_MIN_SHAPE

#define POWER_MIN_SHAPE                     1.0f    // Must be >= 1
#define POWER_MAX_SHAPE                     10.0f
#define POWER_CONV_SLOPE                    (POWER_MAX_SHAPE - POWER_MIN_SHAPE)
#define POWER_CONV_INTRC                    POWER_MIN_SHAPE

#define BILINEAR_MIN_SHAPE                  0.0f    // Must be >= 0
#define BILINEAR_MAX_SHAPE                  10.0f
#define BILINEAR_CONV_SLOPE                 (BILINEAR_MAX_SHAPE - BILINEAR_MIN_SHAPE)
#define BILINEAR_CONV_INTRC                 BILINEAR_MIN_SHAPE

#define ASYMMTERIC_SOFT_CLIP_MAX_LEVEL      0.999f  // Must be < 1

#define QUARTER_CIRCLE_MAX_RADIUS           10.0f

#define BITCRUSH_MIN_LEVELS                 1.0f    // Must be >= 1
#define BITCRUSH_MAX_LEVELS                 24.0f
#define BITCRUSH_CONV_SLOPE                 (BITCRUSH_MAX_LEVELS - BITCRUSH_MIN_LEVELS)
#define BITCRUSH_CONV_INTRC                 BITCRUSH_MIN_LEVELS

#define TAP_TUBEWARMTH_MIN_DRIVE            -10.0f
#define TAP_TUBEWARMTH_MAX_DRIVE            10.0f
#define TAP_TUBEWARMTH_DRIVE_CONV_SLOPE     (TAP_TUBEWARMTH_MAX_DRIVE - TAP_TUBEWARMTH_MIN_DRIVE)
#define TAP_TUBEWARMTH_DRIVE_CONV_INTRC     TAP_TUBEWARMTH_MIN_DRIVE

#define TAP_TUBEWARMTH_MIN_BLEND            0.1f
#define TAP_TUBEWARMTH_MAX_BLEND            10.0f
#define TAP_TUBEWARMTH_BLEND_CONV_SLOPE     (TAP_TUBEWARMTH_MAX_BLEND - TAP_TUBEWARMTH_MIN_BLEND)
#define TAP_TUBEWARMTH_BLEND_CONV_INTRC     TAP_TUBEWARMTH_MIN_BLEND

namespace lsp
{
    namespace dspu
    {

        Shaper::Shaper()
        {
            construct();
        }

        Shaper::~Shaper()
        {
            destroy();
        }

        void Shaper::construct()
        {
            enFunction      = SH_FCN_DEFAULT;

            // Initialize all state trackers
            sShaping.tap_tubewarmth.last_raw_output         = 0.0f;
            sShaping.tap_tubewarmth.last_raw_intermediate   = 0.0f;

            fSlope          = 0.5f;
            fShape          = 0.5f;
            fHighLevel      = 1.0f;
            fLowLevel       = 1.0f;
            fRadius         = 1.0f;
            fLevels         = 0.5f;
            fDrive          = 0.5f;
            fBlend          = 0.5f;

            nSampleRate     = 0;
            nUpdateFlags    = UPD_ALL;
        }

        void Shaper::destroy()
        {

        }

        bool Shaper::needs_update() const
        {
            if (nUpdateFlags & UPD_FUNCTION)
                return true;

            switch (enFunction)
            {
                case SH_FCN_SINUSOIDAL:             return nUpdateFlags & UPD_SLOPE;

                case SH_FCN_POLYNOMIAL:             return nUpdateFlags & UPD_SHAPE;
                case SH_FCN_HYPERBOLIC:             return nUpdateFlags & UPD_SHAPE;
                case SH_FCN_EXPONENTIAL:            return nUpdateFlags & UPD_SHAPE;
                case SH_FCN_POWER:                  return nUpdateFlags & UPD_SHAPE;
                case SH_FCN_BILINEAR:               return nUpdateFlags & UPD_SHAPE;
                case SH_FCN_RECTIFIER:              return nUpdateFlags & UPD_SHAPE;

                case SH_FCN_ASYMMETRIC_CLIP:        return nUpdateFlags & (UPD_HIGH_LEVEL | UPD_LOW_LEVEL);
                case SH_FCN_ASYMMETRIC_SOFTCLIP:    return nUpdateFlags & (UPD_HIGH_LEVEL | UPD_LOW_LEVEL);

                case SH_FCN_QUARTER_CYCLE:          return nUpdateFlags & UPD_RADIUS;

                case SH_FCN_BITCRUSH_FLOOR:         return nUpdateFlags & UPD_LEVELS;
                case SH_FCN_BITCRUSH_CEIL:          return nUpdateFlags & UPD_LEVELS;
                case SH_FCN_BITCRUSH_ROUND:         return nUpdateFlags & UPD_LEVELS;

                case SH_FCN_TAP_TUBEWARMTH:         return nUpdateFlags & (UPD_DRIVE | UPD_BLEND | UPD_SAMPLE_RATE);
            }

            return false;
        }

        void Shaper::update_settings()
        {
            if (!needs_update())
                return;

            // If the function is what changed we need to reset the state trackers for all stateful functions.
            if (nUpdateFlags & UPD_FUNCTION)
            {
                sShaping.tap_tubewarmth.last_raw_output         = 0.0f;
                sShaping.tap_tubewarmth.last_raw_intermediate   = 0.0f;
            }

            nUpdateFlags = 0;

            switch (enFunction)
            {
                case SH_FCN_SINUSOIDAL:
                {
                    sShaping.sinusoidal.slope = lsp_limit(0.5f * M_PI * fSlope, SINUSOIDAL_MIN_SLOPE, SINUSOIDAL_MAX_SLOPE);
                    sShaping.sinusoidal.radius = M_PI / (2.0f * sShaping.sinusoidal.slope);
                    cbShaper = dspu::shaping::sinusoidal;
                }
                break;

                case SH_FCN_POLYNOMIAL:
                {
                    sShaping.polynomial.shape = lsp_max(fShape, POLYNOMIAL_MIN_SHAPE);
                    sShaping.polynomial.radius = 1.0f - sShaping.polynomial.shape;
                    cbShaper = dspu::shaping::polynomial;
                }
                break;

                case SH_FCN_HYPERBOLIC:
                {
                    sShaping.hyperbolic.shape = HYPERBOLIC_MAX_SHAPE * fShape;
                    sShaping.hyperbolic.hyperbolic_shape = quick_tanh(sShaping.hyperbolic.shape);
                    cbShaper = dspu::shaping::hyperbolic;
                }
                break;

                case SH_FCN_EXPONENTIAL:
                {
                    sShaping.exponential.shape = EXPONENTIAL_CONV_SLOPE * fShape + EXPONENTIAL_CONV_INTRC;
                    sShaping.exponential.log_shape = quick_logf(sShaping.exponential.shape);
                    sShaping.exponential.scale = sShaping.exponential.shape / (sShaping.exponential.shape - 1.0f);
                    cbShaper = dspu::shaping::exponential;
                }
                break;

                case SH_FCN_POWER:
                {
                    sShaping.power.shape = POWER_CONV_SLOPE * fShape + POWER_CONV_INTRC;
                    cbShaper = dspu::shaping::power;
                }
                break;

                case SH_FCN_BILINEAR:
                {
                    sShaping.bilinear.shape = BILINEAR_CONV_SLOPE * fShape + BILINEAR_CONV_INTRC;
                    cbShaper =dspu::shaping::bilinear;
                }
                break;

                case SH_FCN_RECTIFIER:
                {
                    sShaping.rectifier.shape = fShape;
                    cbShaper = dspu::shaping::rectifier;
                }
                break;

                case SH_FCN_ASYMMETRIC_CLIP:
                {
                    sShaping.asymmetric_clip.high_clip = fHighLevel;
                    sShaping.asymmetric_clip.low_clip = fLowLevel;
                    cbShaper = dspu::shaping::asymmetric_clip;
                }
                break;

                case SH_FCN_ASYMMETRIC_SOFTCLIP:
                {
                    sShaping.asymmetric_softclip.high_limit = lsp_min(fHighLevel, ASYMMTERIC_SOFT_CLIP_MAX_LEVEL);
                    sShaping.asymmetric_softclip.low_limit = lsp_min(fLowLevel, ASYMMTERIC_SOFT_CLIP_MAX_LEVEL);
                    sShaping.asymmetric_softclip.pos_scale = 1.0f / (1.0f - sShaping.asymmetric_softclip.high_limit);
                    sShaping.asymmetric_softclip.neg_scale = 1.0f / (1.0f - sShaping.asymmetric_softclip.low_limit);
                    cbShaper = dspu::shaping::asymmetric_softclip;
                }
                break;

                case SH_FCN_QUARTER_CYCLE:
                {
                    sShaping.quarter_circle.radius = QUARTER_CIRCLE_MAX_RADIUS * fRadius;
                    sShaping.quarter_circle.radius2 = 2.0f * sShaping.quarter_circle.radius;
                    cbShaper = dspu::shaping::quarter_circle;
                }
                break;

                case SH_FCN_BITCRUSH_FLOOR:
                {
                    sShaping.bitcrush_floor.levels = BITCRUSH_CONV_SLOPE * fLevels + BITCRUSH_CONV_INTRC;
                    cbShaper = dspu::shaping::bitcrush_floor;
                }
                break;

                case SH_FCN_BITCRUSH_CEIL:
                {
                    sShaping.bitcrush_ceil.levels = BITCRUSH_CONV_SLOPE * fLevels + BITCRUSH_CONV_INTRC;
                    cbShaper = dspu::shaping::bitcrush_ceil;
                }
                break;

                case SH_FCN_BITCRUSH_ROUND:
                {
                    sShaping.bitcrush_round.levels = BITCRUSH_CONV_SLOPE * fLevels + BITCRUSH_CONV_INTRC;
                    cbShaper = dspu::shaping::bitcrush_round;
                }
                break;

                case SH_FCN_TAP_TUBEWARMTH:
                {
                    sShaping.tap_tubewarmth.drive = TAP_TUBEWARMTH_BLEND_CONV_SLOPE * fDrive + TAP_TUBEWARMTH_DRIVE_CONV_INTRC;
                    sShaping.tap_tubewarmth.blend = TAP_TUBEWARMTH_BLEND_CONV_SLOPE * fBlend + TAP_TUBEWARMTH_DRIVE_CONV_INTRC;

                    float rd = 12.0f / sShaping.tap_tubewarmth.drive;
                    float rbdr = (780.0f * rd) / (33.0f * (10.5f - sShaping.tap_tubewarmth.blend));

                    sShaping.tap_tubewarmth.kpa = dspu::shaping::tap_rect_sqrt(2.0f * rd * rd - 1.0f) + 1.0f;
                    sShaping.tap_tubewarmth.kpb = 0.5f * (2.0f - sShaping.tap_tubewarmth.kpa);
                    sShaping.tap_tubewarmth.ap = 0.5f * (rd * rd - sShaping.tap_tubewarmth.kpa + 1.0f);

                    float kc = sShaping.tap_tubewarmth.kpa / dspu::shaping::tap_rect_sqrt(
                            dspu::shaping::tap_rect_sqrt(
                                    2.0f * dspu::shaping::tap_rect_sqrt(2.0f * rd * rd - 1.0f) - 2.0f * rd *rd
                                    )
                            );
                    float sq = kc * kc + 1.0f;

                    sShaping.tap_tubewarmth.srct = (0.1f * nSampleRate) / (0.1f * nSampleRate + 1.0f);

                    sShaping.tap_tubewarmth.knb = -rbdr / dspu::shaping::tap_rect_sqrt(sq);
                    sShaping.tap_tubewarmth.kna = 2.0f * kc * rbdr / dspu::shaping::tap_rect_sqrt(sq);
                    sShaping.tap_tubewarmth.an = rbdr * rbdr / sq;

                    float imr = 2.0f * sShaping.tap_tubewarmth.knb * dspu::shaping::tap_rect_sqrt(
                            2.0f * sShaping.tap_tubewarmth.kna + 4.0f * sShaping.tap_tubewarmth.an - 1.0f
                            );

                    sShaping.tap_tubewarmth.pwrq = 2.0f / (imr + 1.0f);

                    cbShaper = dspu::shaping::tap_tubewarmth;
                }
            }
        }

        void Shaper::set_sample_rate(size_t sr)
        {
            if (nSampleRate == sr)
                return;

            nSampleRate = sr;
            nUpdateFlags |= UPD_SAMPLE_RATE;
        }

        void Shaper::set_slope(float slope)
        {
            slope = lsp_limit(slope, 0.0f, 1.0f);
            if (fSlope == slope)
                return;
            nUpdateFlags |= UPD_SLOPE;
        }

        void Shaper::set_shape(float shape)
        {
            shape = lsp_limit(shape, 0.0f, 1.0f);
            if (fShape == shape)
                return;
            nUpdateFlags |= UPD_SHAPE;
        }

        void Shaper::set_high_level(float high_level)
        {
            high_level = lsp_limit(high_level, 0.0f, 1.0f);
            if (fHighLevel == high_level)
                return;
            nUpdateFlags |= UPD_HIGH_LEVEL;
        }

        void Shaper::set_low_level(float low_level)
        {
            low_level = lsp_limit(low_level, 0.0f, 1.0f);
            if (fLowLevel == low_level)
                return;
            nUpdateFlags |= UPD_LOW_LEVEL;
        }

        void Shaper::set_radius(float radius)
        {
            radius = lsp_limit(radius, 0.0f, 1.0f);
            if (fRadius == radius)
                return;
            nUpdateFlags |= UPD_RADIUS;
        }

        void Shaper::set_levels(float levels)
        {
            levels = lsp_limit(levels, 0.0f, 1.0f);
            if (fLevels == levels)
                return;
            nUpdateFlags |= UPD_LEVELS;
        }

        void Shaper::set_drive(float drive)
        {
            drive = lsp_limit(drive, 0.0f, 1.0f);
            if (fDrive == drive)
                return;
            nUpdateFlags |= UPD_DRIVE;
        }

        void Shaper::set_blend(float blend)
        {
            blend = lsp_limit(blend, 0.0f, 1.0f);
            if (fBlend == blend)
                return;
            nUpdateFlags |= UPD_BLEND;
        }

        void Shaper::process_add(float *dst, const float *src, size_t count)
        {
            if (src == NULL)
            {
                // No inputs, interpret `src` as zeros: dst[i] = distortion(0) + 0 = distortion(0) (might be non-zero)
                for (size_t n = 0; n < count; ++n)
                {
                    dst[n] = cbShaper(&sShaping, 0.0f);
                }
                return;
            }

            for (size_t n = 0; n < count; ++n)
            {
                dst[n] += cbShaper(&sShaping, src[n]);
            }
        }

        void Shaper::process_mul(float *dst, const float *src, size_t count)
        {
            if (src == NULL)
            {
                // No inputs, interpret `src` as zeros: dst[i] = distortion(0) * 0 = 0
                dsp::fill_zero(dst, count);
                return;
            }

            for (size_t n = 0; n < count; ++n)
            {
                dst[n] *= cbShaper(&sShaping, src[n]);
            }
        }

        void Shaper::process_overwrite(float *dst, const float *src, size_t count)
        {
            if (src == NULL)
            {
                // No inputs, interpret `src` as zeros: dst[i] = distortion(0) (might be non-zero)
                for (size_t n = 0; n < count; ++n)
                {
                    dst[n] = cbShaper(&sShaping, 0.0f);
                }
                return;
            }

            for (size_t n = 0; n < count; ++n)
            {
                dst[n] = cbShaper(&sShaping, src[n]);
            }
        }

        void Shaper::dump(IStateDumper *v) const
        {

        }

    } /* namespace dspu */
} /* namespace lsp */
