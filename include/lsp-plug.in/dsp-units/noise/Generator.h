/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 31 May 2021
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_DSP_UNITS_NOISE_GENERATOR_H_
#define LSP_PLUG_IN_DSP_UNITS_NOISE_GENERATOR_H_

#include <lsp-plug.in/dsp-units/filters/SpectralTilt.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/noise/LCG.h>
#include <lsp-plug.in/dsp-units/noise/MLS.h>
#include <lsp-plug.in/dsp-units/noise/Velvet.h>

namespace lsp
{
    namespace dspu
    {
        enum ng_generator_t
        {
            NG_GEN_MLS,
            NG_GEN_LCG,
            NG_GEN_VELVET
        };

        enum ng_color_t
        {
            NG_COLOR_WHITE,
            NG_COLOR_PINK,
            NG_COLOR_RED,
            NG_COLOR_BLUE,
            NG_COLOR_VIOLET,
            NG_COLOR_ARBITRARY,

            NG_COLOR_BROWN = NG_COLOR_RED,
            NG_COLOR_BROWNIAN = NG_COLOR_RED
        };

        class LSP_DSP_UNITS_PUBLIC NoiseGenerator
        {
            private:
                NoiseGenerator & operator = (const NoiseGenerator &);
                NoiseGenerator(const NoiseGenerator &);

            protected:
                enum update_t
                {
                    UPD_MLS     = 1 << 0,
                    UPD_LCG     = 1 << 1,
                    UPD_VELVET  = 1 << 2,
                    UPD_COLOR   = 1 << 3,
                    UPD_OTHER   = 1 << 4,

                    UPD_ALL     = UPD_MLS | UPD_LCG | UPD_VELVET | UPD_COLOR | UPD_OTHER
                };

                typedef struct mls_params_t
                {
                    uint8_t             nBits;
                    MLS::mls_t          nSeed;
                } mls_params_t;

                typedef struct lcg_params_t
                {
                    uint32_t            nSeed;
                    lcg_dist_t          enDistribution;
                } lcg_params_t;

                typedef struct velvet_params_t
                {
                    uint32_t            nRandSeed;
                    uint8_t             nMLSnBits;
                    MLS::mls_t          nMLSseed;
                    vn_core_t           enCore;
                    vn_velvet_type_t    enVelvetType;
                    float               fWindowWidth_s;
                    float               fARNdelta;
                    bool                bCrush;
                    float               fCrushProb;
                } velvet_params_t;

                typedef struct color_params_t
                {
                    ng_color_t          enColor;
                    size_t              nOrder;
                    float               fSlope;
                    stlt_slope_unit_t   enSlopeUnit;
                } color_params_t;

            private:
                MLS                 sMLS;
                LCG                 sLCG;
                Velvet              sVelvetNoise;
                SpectralTilt        sColorFilter;

                mls_params_t        sMLSParams;
                lcg_params_t        sLCGParams;
                velvet_params_t     sVelvetParams;
                color_params_t      sColorParams;

                size_t              nSampleRate;
                ng_generator_t      enGenerator;

                float               fAmplitude;
                float               fOffset;

                size_t              nUpdate;

            public:
                explicit NoiseGenerator();
                ~NoiseGenerator();

                void construct();
                void destroy();

            protected:
                void do_process(float *dst, size_t count);
                void update_settings();

            public:

                /** Initialize random generator.
                 *
                 * @param rand_seed seed for the Randomizer generator.
                 * @param lcg_seed seed for the LCG generator.
                 */
                void init(
                    uint8_t mls_n_bits, MLS::mls_t mls_seed,
                    uint32_t lcg_seed,
                    uint32_t velvet_rand_seed, uint8_t velvet_mls_n_bits, MLS::mls_t velvet_mls_seed);

                /** Initialize random generator, automatic.
                 */
                void init();

                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                void set_sample_rate(size_t sr);

                /** Set the number of bits of the MLS sequence generator.
                 *
                 * @param nbits number of bits.
                 */
                void set_mls_n_bits(uint8_t nbits);

                /** Set MLS generator seed.
                 *
                 * @param seed MLS seed.
                 */
                void set_mls_seed(MLS::mls_t seed);

                /** Set LCG distribution.
                 *
                 * @param dist LCG distribution.
                 */
                void set_lcg_distribution(lcg_dist_t dist);

                /** Set the lcg_dist_telvet noise type. Velvet noise is emitted only if Sparsity is Velvet.
                 *
                 * @param type velvet type.
                 */
                void set_velvet_type(vn_velvet_type_t type);

                /** Set the Velvet noise window width in samples. Velvet noise is emitted only if Sparsity is Velvet.
                 *
                 * @param width velvet noise width.
                 */
                void set_velvet_window_width(float width);

                /** Set delta parameter for Velvet ARN noise.
                 *
                 * @param delta value.
                 */
                void set_velvet_arn_delta(float delta);

                /** Set whether to crush the velvet generator.
                 *
                 * @param true to crash.
                 */
                void set_velvet_crush(bool crush);

                /** Set the crushing probability for the velvet generator.
                 *
                 * @param crushing probability.
                 */
                void set_velvet_crushing_probability(float prob);

                /** Set which core generator to use.
                 *
                 * @param core core generator specification.
                 */
                void set_generator(ng_generator_t core);

                /** Set the noise color.
                 *
                 * @param noise color specification.
                 */
                void set_noise_color(ng_color_t color);

                /** Set the coloring filter order.
                 *
                 * @param order order.
                 */
                void set_coloring_order(size_t order);

                /** Set the color slope.
                 *
                 * @param slope slope.
                 * çparam unit slope unit
                 */
                void set_color_slope(float slope, stlt_slope_unit_t unit);

                /** Set the noise amplitude.
                 *
                 * @param amplitude noise amplitude.
                 */
                void set_amplitude(float amplitude);

                /** Set the noise offset.
                 *
                 * @param offset noise offset.
                 */
                void set_offset(float offset);

                /** Output noise to the destination buffer in additive mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to synthesise
                 */
                void process_add(float *dst, const float *src, size_t count);

                /** Output noise to the destination buffer in multiplicative mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process_mul(float *dst, const float *src, size_t count);

                /** Output noise to a destination buffer overwriting its content
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULLL
                 * @param count number of samples to process
                 */
                void process_overwrite(float *dst, size_t count);

                /**
                 * Get frequency chart of the colouring filter
                 * @param re real part of the frequency chart
                 * @param im imaginary part of the frequency chart
                 * @param f frequencies to calculate value
                 * @param count number of dots for the chart
                 */
                void freq_chart(float *re, float *im, const float *f, size_t count);

                /**
                 * Get frequency chart of the whole equalizer
                 * @param c complex numbers that contain the filter transfer function
                 * @param f frequencies to calculate filter transfer function
                 * @param count number of points
                 */
                void freq_chart(float *c, const float *f, size_t count);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
}

#endif /* LSP_PLUG_IN_DSP_UNITS_NOISE_GENERATOR_H_ */
