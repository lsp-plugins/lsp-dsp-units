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

            // Update the settings with the defaults above.
            update_settings();
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

                case SH_FCN_QUARTER_CIRCLE:          return nUpdateFlags & UPD_RADIUS;

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
                    sShaping.sinusoidal.slope = lsp_limit(0.5f * M_PI * fSlope, fSinusoidal_min_slope, fSinusoidal_max_slope);
                    sShaping.sinusoidal.radius = M_PI / (2.0f * sShaping.sinusoidal.slope);
                    cbShaper = dspu::shaping::sinusoidal;
                }
                break;

                case SH_FCN_POLYNOMIAL:
                {
                    sShaping.polynomial.shape = lsp_max(fShape, fPolynomial_min_shape);
                    sShaping.polynomial.radius = 1.0f - sShaping.polynomial.shape;
                    cbShaper = dspu::shaping::polynomial;
                }
                break;

                case SH_FCN_HYPERBOLIC:
                {
                    sShaping.hyperbolic.shape = fHyperbolic_max_shape * fShape;
                    sShaping.hyperbolic.hyperbolic_shape = quick_tanh(sShaping.hyperbolic.shape);
                    cbShaper = dspu::shaping::hyperbolic;
                }
                break;

                case SH_FCN_EXPONENTIAL:
                {
                    sShaping.exponential.shape = fExponential_conv_slope * fShape + fExponential_conv_intrc;
                    sShaping.exponential.log_shape = quick_logf(sShaping.exponential.shape);
                    sShaping.exponential.scale = sShaping.exponential.shape / (sShaping.exponential.shape - 1.0f);
                    cbShaper = dspu::shaping::exponential;
                }
                break;

                case SH_FCN_POWER:
                {
                    sShaping.power.shape = fPower_conv_slope * fShape + fPower_conv_intrc;
                    cbShaper = dspu::shaping::power;
                }
                break;

                case SH_FCN_BILINEAR:
                {
                    sShaping.bilinear.shape = fBilinear_conv_slope * fShape + fBilinear_conv_intrc;
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
                    sShaping.asymmetric_softclip.high_limit = lsp_min(fHighLevel, fAsymmetric_soft_clip_max_level);
                    sShaping.asymmetric_softclip.low_limit = lsp_min(fLowLevel, fAsymmetric_soft_clip_max_level);
                    sShaping.asymmetric_softclip.pos_scale = 1.0f / (1.0f - sShaping.asymmetric_softclip.high_limit);
                    sShaping.asymmetric_softclip.neg_scale = 1.0f / (1.0f - sShaping.asymmetric_softclip.low_limit);
                    cbShaper = dspu::shaping::asymmetric_softclip;
                }
                break;

                case SH_FCN_QUARTER_CIRCLE:
                {
                    sShaping.quarter_circle.radius = fQuarter_circle_max_radius * fRadius;
                    sShaping.quarter_circle.radius2 = sShaping.quarter_circle.radius * sShaping.quarter_circle.radius;
                    cbShaper = dspu::shaping::quarter_circle;
                }
                break;

                case SH_FCN_BITCRUSH_FLOOR:
                {
                    sShaping.bitcrush_floor.levels = fBitcrush_conv_slope * fLevels + fBitcrush_conv_intrc;
                    cbShaper = dspu::shaping::bitcrush_floor;
                }
                break;

                case SH_FCN_BITCRUSH_CEIL:
                {
                    sShaping.bitcrush_ceil.levels = fBitcrush_conv_slope * fLevels + fBitcrush_conv_intrc;
                    cbShaper = dspu::shaping::bitcrush_ceil;
                }
                break;

                case SH_FCN_BITCRUSH_ROUND:
                {
                    sShaping.bitcrush_round.levels = fBitcrush_conv_slope * fLevels + fBitcrush_conv_intrc;
                    cbShaper = dspu::shaping::bitcrush_round;
                }
                break;

                case SH_FCN_TAP_TUBEWARMTH:
                {
                    sShaping.tap_tubewarmth.drive = fTap_tebewarmth_drive_conv_slope * fDrive + fTap_tubewarmth_drive_conv_intrc;
                    sShaping.tap_tubewarmth.blend = fTap_tubewarmth_blend_conv_slope * fBlend + fTap_tubewarmth_blend_conv_intrc;

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

            fSlope = slope;
            nUpdateFlags |= UPD_SLOPE;
        }

        void Shaper::set_shape(float shape)
        {
            shape = lsp_limit(shape, 0.0f, 1.0f);
            if (fShape == shape)
                return;

            fShape = shape;
            nUpdateFlags |= UPD_SHAPE;
        }

        void Shaper::set_high_level(float high_level)
        {
            high_level = lsp_limit(high_level, 0.0f, 1.0f);
            if (fHighLevel == high_level)
                return;

            fHighLevel = high_level;
            nUpdateFlags |= UPD_HIGH_LEVEL;
        }

        void Shaper::set_low_level(float low_level)
        {
            low_level = lsp_limit(low_level, 0.0f, 1.0f);
            if (fLowLevel == low_level)
                return;

            fLowLevel = low_level;
            nUpdateFlags |= UPD_LOW_LEVEL;
        }

        void Shaper::set_radius(float radius)
        {
            radius = lsp_limit(radius, 0.0f, 1.0f);
            if (fRadius == radius)
                return;

            fRadius = radius;
            nUpdateFlags |= UPD_RADIUS;
        }

        void Shaper::set_levels(float levels)
        {
            levels = lsp_limit(levels, 0.0f, 1.0f);
            if (fLevels == levels)
                return;

            fLevels = levels;
            nUpdateFlags |= UPD_LEVELS;
        }

        void Shaper::set_drive(float drive)
        {
            drive = lsp_limit(drive, 0.0f, 1.0f);
            if (fDrive == drive)
                return;

            fDrive = drive;
            nUpdateFlags |= UPD_DRIVE;
        }

        void Shaper::set_blend(float blend)
        {
            blend = lsp_limit(blend, 0.0f, 1.0f);
            if (fBlend == blend)
                return;

            fBlend = blend;
            nUpdateFlags |= UPD_BLEND;
        }

        void Shaper::set_function(sh_function_t function)
        {
            if (enFunction == function)
                return;

            enFunction = function;
            nUpdateFlags |= UPD_FUNCTION;
        }

        void Shaper::process_add(float *dst, const float *src, size_t count)
        {
            update_settings();

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
            update_settings();

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
            update_settings();

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
            v->write("fSlope", fSlope);
            v->write("fShape", fShape);
            v->write("fHighLevel", fHighLevel);
            v->write("fLowLevel", fLowLevel);
            v->write("fRadius", fRadius);
            v->write("fLevels", fLevels);
            v->write("fDrive", fDrive);
            v->write("fBlend", fBlend);

            // Of course, static compile time constants cannot, and should not, be here. They are not even state!

            v->write("enFunction", enFunction);

            switch (enFunction)
            {
                case SH_FCN_SINUSOIDAL:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("slope", sShaping.sinusoidal.slope);
                        v->write("radius", sShaping.sinusoidal.radius);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_POLYNOMIAL:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("shape", sShaping.polynomial.shape);
                        v->write("radius", sShaping.polynomial.radius);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_HYPERBOLIC:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("shape", sShaping.hyperbolic.shape);
                        v->write("hyperbolic_shape", sShaping.hyperbolic.hyperbolic_shape);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_EXPONENTIAL:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("shape", sShaping.exponential.shape);
                        v->write("hyperbolic_shape", sShaping.exponential.log_shape);
                        v->write("hyperbolic_shape", sShaping.exponential.scale);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_POWER:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("shape", sShaping.power.shape);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_BILINEAR:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("shape", sShaping.bilinear.shape);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_ASYMMETRIC_CLIP:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("high_clip", sShaping.asymmetric_clip.high_clip);
                        v->write("low_clip", sShaping.asymmetric_clip.low_clip);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_ASYMMETRIC_SOFTCLIP:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("high_limit", sShaping.asymmetric_softclip.high_limit);
                        v->write("low_limit", sShaping.asymmetric_softclip.low_limit);
                        v->write("pos_scale", sShaping.asymmetric_softclip.pos_scale);
                        v->write("neg_scale", sShaping.asymmetric_softclip.neg_scale);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_QUARTER_CIRCLE:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("radius", sShaping.quarter_circle.radius);
                        v->write("radius2", sShaping.quarter_circle.radius2);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_RECTIFIER:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("shape", sShaping.rectifier.shape);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_BITCRUSH_FLOOR:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("levels", sShaping.bitcrush_floor.levels);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_BITCRUSH_CEIL:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("levels", sShaping.bitcrush_ceil.levels);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_BITCRUSH_ROUND:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("levels", sShaping.bitcrush_round.levels);
                    }
                    v->end_object();
                }
                break;

                case SH_FCN_TAP_TUBEWARMTH:
                {
                    v->begin_object("sShaping", &sShaping, sizeof(sShaping));
                    {
                        v->write("drive", sShaping.tap_tubewarmth.drive);
                        v->write("blend", sShaping.tap_tubewarmth.blend);

                        v->write("pwrq", sShaping.tap_tubewarmth.pwrq);
                        v->write("srct", sShaping.tap_tubewarmth.srct);

                        v->write("ap", sShaping.tap_tubewarmth.ap);
                        v->write("kpa", sShaping.tap_tubewarmth.kpa);
                        v->write("kpb", sShaping.tap_tubewarmth.kpb);

                        v->write("an", sShaping.tap_tubewarmth.an);
                        v->write("kna", sShaping.tap_tubewarmth.kna);
                        v->write("knb", sShaping.tap_tubewarmth.knb);

                        v->write("last_raw_output", sShaping.tap_tubewarmth.last_raw_output);
                        v->write("last_raw_intermediate", sShaping.tap_tubewarmth.last_raw_intermediate);
                    }
                    v->end_object();
                }
                break;
            }

            v->write("cbShaper", cbShaper);
            v->write("nSampleRate", nSampleRate);
            v->write("nUpdateFlags", nUpdateFlags);
        }

    } /* namespace dspu */
} /* namespace lsp */
