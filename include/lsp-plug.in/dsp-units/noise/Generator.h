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

#ifndef INCLUDE_LSP_PLUG_IN_DSP_UNITS_NOISE_GENERATOR_H_
#define INCLUDE_LSP_PLUG_IN_DSP_UNITS_NOISE_GENERATOR_H_

#include <lsp-plug.in/dsp-units/noise/MLS.h>
#include <lsp-plug.in/dsp-units/noise/LCG.h>
#include <lsp-plug.in/dsp-units/noise/Velvet.h>
#include <lsp-plug.in/dsp-units/filters/SpectralTilt.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        enum ng_generator_t
        {
            NG_GEN_MLS,
            NG_GEN_LCG,
            NG_GEN_VELVET,
            NG_GEN_MAX
        };

        enum ng_color_t
        {
            NG_COLOR_WHITE,
            NG_COLOR_PINK,
            NG_COLOR_RED, // AKA Brownian or Brown
            NG_COLOR_BLUE,
            NG_COLOR_VIOLET,
            NG_COLOR_ARBITRARY,
            NG_COLOR_MAX
        };

        class NoiseGenerator
        {
            protected:

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
                size_t              nSampleRate;

                MLS                 sMLS;
                LCG                 sLCG;
                Velvet              sVelvetNoise;
                SpectralTilt        sColorFilter;

                mls_params_t        sMLSParams;
                lcg_params_t        sLCGParams;
                velvet_params_t     sVelvetParams;
                color_params_t      sColorParams;

                ng_generator_t      enGenerator;

                float               fAmplitude;
                float               fOffset;

                uint8_t            *pData;
                float              *vBuffer;

                bool                bSync;

            public:
                explicit NoiseGenerator();
                ~NoiseGenerator();

                void construct();
                void destroy();

            protected:
                void init_buffers();
                void do_process(float *dst, size_t count);

            public:

                /** Initialize random generator.
                 *
                 * @param rand_seed seed for the Randomizer generator.
                 * @param lcg_seed seed for the LCG generator.
                 */
                void init(
                        uint8_t mls_n_bits, MLS::mls_t mls_seed,
                        uint32_t lcg_seed,
                        uint32_t velvet_rand_seed, uint8_t velvet_mls_n_bits, MLS::mls_t velvet_mls_seed
                        );

                /** Initialize random generator, automatic.
                 */
                void init();

                /** Check that NoiseGenerator needs settings update.
                 *
                 * @return true if NoiseGenerator needs settings update.
                 */
                inline bool needs_update() const
                {
                    return bSync;
                }

                /** This method should be called if needs_update() returns true.
                 * before calling processing methods.
                 *
                 */
                void update_settings();

                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                inline void set_sample_rate(size_t sr)
                {
                    if (nSampleRate == sr)
                        return;

                    nSampleRate = sr;
                    bSync       = true;
                }

                /** Set the number of bits of the MLS sequence generator.
                 *
                 * @param nbits number of bits.
                 */
                inline void set_mls_n_bits(uint8_t nbits)
                {
                    if (nbits == sMLSParams.nBits)
                        return;

                    sMLSParams.nBits    = nbits;
                    bSync               = true;
                }

                /** Set MLS generator seed.
                 *
                 * @param seed MLS seed.
                 */
                inline void set_mls_seed(MLS::mls_t seed)
                {
                    if (seed == sMLSParams.nSeed)
                        return;

                    sMLSParams.nSeed    = seed;
                    bSync               = true;
                }

                /** Set LCG distribution.
                 *
                 * @param dist LCG distribution.
                 */
                inline void set_lcg_distribution(lcg_dist_t dist)
                {
                    if (dist == sLCGParams.enDistribution)
                        return;

                    sLCGParams.enDistribution = dist;
                    bSync = true;
                }

                /** Set the lcg_dist_telvet noise type. Velvet noise is emitted only if Sparsity is Velvet.
                 *
                 * @param type velvet type.
                 */
                inline void set_velvet_type(vn_velvet_type_t type)
                {
                    if (type == sVelvetParams.enVelvetType)
                        return;

                    sVelvetParams.enVelvetType = type;
                }

                /** Set the Velvet noise window width in samples. Velvet noise is emitted only if Sparsity is Velvet.
                 *
                 * @param width velvet noise width.
                 */
                inline void set_velvet_window_width(float width)
                {
                    if (width == sVelvetParams.fWindowWidth_s)
                        return;

                    sVelvetParams.fWindowWidth_s = width;
                }

                /** Set delta parameter for Velvet ARN noise.
                 *
                 * @param delta value.
                 */
                inline void set_velvet_arn_delta(float delta)
                {
                    if (delta == sVelvetParams.fARNdelta)
                        return;

                    sVelvetParams.fARNdelta = delta;
                }

                /** Set whether to crush the velvet generator.
                 *
                 * @param true to crash.
                 */
                inline void set_velvet_crush(bool crush)
                {
                    if (crush == sVelvetParams.bCrush)
                        return;

                    sVelvetParams.bCrush = crush;
                }

                /** Set the crushing probability for the velvet generator.
                 *
                 * @param crushing probability.
                 */
                inline void set_velvet_crushing_probability(float prob)
                {
                    if (prob == sVelvetParams.fCrushProb)
                        return;

                    sVelvetParams.fCrushProb = prob;
                }

                /** Set which core generator to use.
                 *
                 * @param core core generator specification.
                 */
                inline void set_generator(ng_generator_t core)
                {
                    if ((core < NG_GEN_MLS) || (core >= NG_GEN_MAX))
                        return;

                    if (core == enGenerator)
                        return;

                    enGenerator = core;
                }

                /** Set the noise color.
                 *
                 * @param noise color specification.
                 */
                inline void set_noise_color(ng_color_t color)
                {
                    if ((color < NG_COLOR_WHITE) || (color >= NG_COLOR_MAX))
                        return;

                    if (color == sColorParams.enColor)
                        return;

                    sColorParams.enColor    = color;
                    bSync                   = true;
                }

                /** Set the coloring filter order.
                 *
                 * @param order order.
                 */
                inline void set_coloring_order(size_t order)
                {
                    if (order == sColorParams.nOrder)
                        return;

                    sColorParams.nOrder     = order;
                    bSync                   = true;
                }

                /** Set the color slope.
                 *
                 * @param slope slope.
                 * çparam unit slope unit
                 */
                inline void set_color_slope(float slope, stlt_slope_unit_t unit)
                {
                    if ((slope == sColorParams.fSlope) && (unit == sColorParams.enSlopeUnit))
                        return;

                    sColorParams.fSlope         = slope;
                    sColorParams.enSlopeUnit    = unit;
                    bSync                       = true;
                }

                /** Set the noise amplitude.
                 *
                 * @param amplitude noise amplitude.
                 */
                inline void set_amplitude(float amplitude)
                {
                    if (amplitude == fAmplitude)
                        return;

                    fAmplitude = amplitude;
                    bSync = true;
                }

                /** Set the noise offset.
                 *
                 * @param offset noise offset.
                 */
                inline void set_offset(float offset)
                {
                    if (offset == fOffset)
                        return;

                    fOffset = offset;
                    bSync = true;
                }

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
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
}

#endif /* INCLUDE_LSP_PLUG_IN_DSP_UNITS_NOISE_GENERATOR_H_ */
