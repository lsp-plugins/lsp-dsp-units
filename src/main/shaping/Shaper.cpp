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

                case SH_FCN_TAP_TUBEWARMTH:         return nUpdateFlags & (UPD_DRIVE | UPD_BLEND);
            }

            return false;
        }

        void Shaper::update_settings()
        {
            if (!needs_update())
                return;

            nUpdateFlags = 0;

            switch (enFunction)
            {
                case SH_FCN_SINUSOIDAL:
                {
                    sShaping.sinusoidal.slope = lsp_limit(0.5f * M_PI * fSlope, SINUSOIDAL_MIN_SLOPE, SINUSOIDAL_MAX_SLOPE);
                    sShaping.sinusoidal.radius = M_PI / (2.0f * sShaping.sinusoidal.slope);
                }
                break;

                case SH_FCN_POLYNOMIAL:
                {
                    sShaping.polynomial.shape = lsp_max(fShape, POLYNOMIAL_MIN_SHAPE);
                    sShaping.polynomial.radius = 1.0f - sShaping.polynomial.shape;
                }
                break;

                case SH_FCN_HYPERBOLIC:
                {
                    sShaping.hyperbolic.shape = HYPERBOLIC_MAX_SHAPE * fShape;
                    sShaping.hyperbolic.hyperbolic_shape = quick_tanh(sShaping.hyperbolic.shape);
                }
                break;

                case SH_FCN_EXPONENTIAL:
                {
                    sShaping.exponential.shape = EXPONENTIAL_CONV_SLOPE * fShape + EXPONENTIAL_CONV_INTRC;
                    sShaping.exponential.log_shape = quick_logf(sShaping.exponential.shape);
                    sShaping.exponential.scale = sShaping.exponential.shape / (sShaping.exponential.shape - 1.0f);
                }
                break;

                case SH_FCN_POWER:
                {
                    sShaping.power.shape = POWER_CONV_SLOPE * fShape + POWER_CONV_INTRC;
                }
                break;

                case SH_FCN_BILINEAR:
                {
                    sShaping.bilinear.shape = BILINEAR_CONV_SLOPE * fShape + BILINEAR_CONV_INTRC;
                }
                break;

                case SH_FCN_RECTIFIER:
                {
                    sShaping.rectifier.shape = fShape;
                }
                break;

                case SH_FCN_ASYMMETRIC_CLIP:
                {
                    sShaping.asymmetric_clip.high_clip = fHighLevel;
                    sShaping.asymmetric_clip.low_clip = fLowLevel;
                }
                break;

                case SH_FCN_ASYMMETRIC_SOFTCLIP:
                {
                    sShaping.asymmetric_softclip.high_limit = lsp_min(fHighLevel, ASYMMTERIC_SOFT_CLIP_MAX_LEVEL);
                    sShaping.asymmetric_softclip.low_limit = lsp_min(fLowLevel, ASYMMTERIC_SOFT_CLIP_MAX_LEVEL);
                    sShaping.asymmetric_softclip.pos_scale = 1.0f / (1.0f - sShaping.asymmetric_softclip.high_limit);
                    sShaping.asymmetric_softclip.neg_scale = 1.0f / (1.0f - sShaping.asymmetric_softclip.low_limit);
                }
                break;

                case SH_FCN_QUARTER_CYCLE:
                {
                    sShaping.quarter_circle.radius = QUARTER_CIRCLE_MAX_RADIUS * fRadius;
                    sShaping.quarter_circle.radius2 = 2.0f * sShaping.quarter_circle.radius;
                }
                break;

                case SH_FCN_BITCRUSH_FLOOR:
                {
                    sShaping.bitcrush_floor.levels = BITCRUSH_CONV_SLOPE * fLevels + BITCRUSH_CONV_INTRC;
                }
                break;

                case SH_FCN_BITCRUSH_CEIL:
                {
                    sShaping.bitcrush_ceil.levels = BITCRUSH_CONV_SLOPE * fLevels + BITCRUSH_CONV_INTRC;
                }
                break;

                case SH_FCN_BITCRUSH_ROUND:
                {
                    sShaping.bitcrush_round.levels = BITCRUSH_CONV_SLOPE * fLevels + BITCRUSH_CONV_INTRC;
                }
                break;

                case SH_FCN_TAP_TUBEWARMTH:
                {
                    sShaping.tap_tubewarmth.drive = TAP_TUBEWARMTH_BLEND_CONV_SLOPE * fDrive + TAP_TUBEWARMTH_DRIVE_CONV_INTRC;
                    sShaping.tap_tubewarmth.blend = TAP_TUBEWARMTH_BLEND_CONV_SLOPE * fBlend + TAP_TUBEWARMTH_DRIVE_CONV_INTRC;

                    // TODO: Rest of TAP parameters calculation.
                }
            }
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

    } /* namespace dspu */
} /* namespace lsp */
