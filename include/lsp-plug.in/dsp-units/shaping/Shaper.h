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
            SH_FCN_TAP_TUBEWARMTH,

            SH_FCN_DEFAULT = SH_FCN_HYPERBOLIC
        };

        class LSP_DSP_UNITS_PUBLIC Shaper
        {
            protected:
                enum update_t
                {
                    UPD_FUNCTION        = 1 << 0,
                    UPD_SLOPE           = 1 << 1,
                    UPD_SHAPE           = 1 << 2,
                    UPD_HIGH_LEVEL      = 1 << 3,
                    UPD_LOW_LEVEL       = 1 << 4,
                    UPD_RADIUS          = 1 << 5,
                    UPD_LEVELS          = 1 << 6,
                    UPD_DRIVE           = 1 << 7,
                    UPD_BLEND           = 1 << 8,
                    UPD_SAMPLE_RATE     = 1 << 9,

                    UPD_ALL =
                            UPD_SLOPE |
                            UPD_SHAPE |
                            UPD_HIGH_LEVEL | UPD_LOW_LEVEL |
                            UPD_RADIUS |
                            UPD_LEVELS |
                            UPD_DRIVE | UPD_BLEND | UPD_SAMPLE_RATE
                };

                typedef float (*shaper_fcn_t)(dspu::shaping::shaping_t*, float);

            private:
                /**
                 * Parametric Goodies.
                 */

                /**
                 * Parameters cross-section: These are in common among all shaping functions.
                 * They might need re-scaling and shifting.
                 */

                /**
                 * [0, 1], The following functions need a rescaled and/or shifted version of this:
                 * - SH_FCN_SINUSOIDAL
                 */
                float                       fSlope;

                /**
                 * [0, 1], The following functions need a rescaled and/or shifted version of this:
                 * - SH_FCN_POLYNOMIAL
                 * - SH_FCN_HYPERBOLIC
                 * - SH_FCN_EXPONENTIAL
                 * - SH_FCN_POWER
                 * - SH_FCN_BILINEAR
                 * - SH_FCN_RECTIFIER
                 */
                float                       fShape;

                /**
                 * [0, 1], The following functions need a rescaled and/or shifted version of these:
                 * - SH_FCN_ASYMMETRIC_CLIP
                 * - SH_FCN_ASYMMETRIC_SOFTCLIP
                 */
                float                       fHighLevel;
                float                       fLowLevel;

                /**
                 * [0, 1], The following functions need a rescaled and/or shifted version of this:
                 * - SH_FCN_QUARTER_CYCLE
                 */
                float                       fRadius;

                /**
                 * [0, 1], The following functions need a rescaled and/or shifted version of this:
                 * - SH_FCN_BITCRUSH_FLOOR
                 * - SH_FCN_BITCRUSH_CEIL
                 * - SH_FCN_BITCRUSH_ROUND
                 */
                float                       fLevels;

                /**
                 * [0, 1], The following functions need a rescaled and/or shifted version of these:
                 * - SH_FCN_TAP_TUBEWARMTH
                 */
                float                       fDrive;
                float                       fBlend;

                /**
                 * Linear scaling coefficients translating class parameters to functions parameters.
                 */

                // Sinusoidal
                static constexpr float fSinusoidal_min_slope                {1e-3f};   // Must be > 0
                static constexpr float fSinusoidal_max_slope                {M_PI_2};

                // Polynomial
                static constexpr float fPolynomial_min_shape                {1e-3f};   // Must be > 0

                // Hyperbolic
                static constexpr float fHyperbolic_max_shape                {10.0f};

                // Exponential
                static constexpr float fExponential_min_shape               {1.001f};  // Must be > 1
                static constexpr float fExponential_max_shape               {10.0f};
                static constexpr float fExponential_conv_slope              {fExponential_max_shape - fExponential_min_shape};
                static constexpr float fExponential_conv_intrc              {fExponential_min_shape};

                // Power
                static constexpr float fPower_min_shape                     {1.0f};    // Must be >= 1
                static constexpr float fPower_max_shape                     {10.0f};
                static constexpr float fPower_conv_slope                    {fPower_max_shape - fPower_min_shape};
                static constexpr float fPower_conv_intrc                    {fPower_min_shape};

                // Bilinear
                static constexpr float fBilinear_min_shape                  {0.0f};    // Must be >= 0
                static constexpr float fBilinear_max_shape                  {10.0f};
                static constexpr float fBilinear_conv_slope                 {fBilinear_max_shape - fBilinear_min_shape};
                static constexpr float fBilinear_conv_intrc                 {fBilinear_min_shape};

                // Asymmetric
                static constexpr float fAsymmetric_soft_clip_max_level      {0.999f};  // Must be < 1

                // Quarter Circle
                static constexpr float fQuarter_circle_max_radius           {10.0f};

                // Bitcrush
                static constexpr float fBitcrush_min_levels                 {1.0f};    // Must be >= 1
                static constexpr float fBitcrush_max_levels                 {24.0f};
                static constexpr float fBitcrush_conv_slope                 {fBitcrush_max_levels - fBitcrush_min_levels};
                static constexpr float fBitcrush_conv_intrc                 {fBitcrush_min_levels};

                // TAP Tubewarmth
                static constexpr float fTap_tubewarmth_min_drive            {-10.0f};
                static constexpr float fTap_tubewarmth_max_drive            {10.0f};
                static constexpr float fTap_tebewarmth_drive_conv_slope     {fTap_tubewarmth_max_drive - fTap_tubewarmth_min_drive};
                static constexpr float fTap_tubewarmth_drive_conv_intrc     {fTap_tubewarmth_min_drive};

                static constexpr float fTap_tubewarmth_min_blend            {0.1f};
                static constexpr float fTap_tubewarmth_max_blend            {10.0f};
                static constexpr float fTap_tubewarmth_blend_conv_slope     {fTap_tubewarmth_max_blend - fTap_tubewarmth_min_blend};
                static constexpr float fTap_tubewarmth_blend_conv_intrc     {fTap_tubewarmth_min_blend};

                /**
                 * Operational Goodies.
                 */

                sh_function_t               enFunction;
                dspu::shaping::shaping_t    sShaping;
                shaper_fcn_t                cbShaper;  // This is the shaping function, depends on enFunction
                size_t                      nSampleRate;
                uint32_t                    nUpdateFlags;

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
                bool needs_update() const;
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

                /** Set the slope parameter.
                 *
                 * @param slope slope
                 */
                void set_slope(float slope);

                /** Set the shape parameter.
                 *
                 * @param shape shape
                 */
                void set_shape(float shape);

                /** Set the high level parameter.
                 *
                 * @param high_level high level
                 */
                void set_high_level(float high_level);

                /** Set the low level parameter.
                 *
                 * @param low_level low level
                 */
                void set_low_level(float low_level);

                /** Set the radius parameter.
                 *
                 * @param radius radius
                 */
                void set_radius(float radius);

                /** Set the levels parameter.
                 *
                 * @param levels levels
                 */
                void set_levels(float levels);

                /** Set the drive.
                 *
                 * @param drive drive
                 */
                void set_drive(float drive);

                /** Set the blend parameter.
                 *
                 * @param blend blend
                 */
                void set_blend(float blend);

                /** Set the shaping function.
                 * @param function function
                 */
                void set_function(sh_function_t function);

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
