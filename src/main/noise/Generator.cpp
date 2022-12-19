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

#include <lsp-plug.in/dsp-units/noise/Generator.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp-units/units.h>

#define BUF_LIM_SIZE    256u

namespace lsp
{
    namespace dspu
    {

        NoiseGenerator::NoiseGenerator()
        {
            construct();
        }

        NoiseGenerator::~NoiseGenerator()
        {
            destroy();
        }

        void NoiseGenerator::construct()
        {
            sMLS.construct();
            sLCG.construct();
            sVelvetNoise.construct();
            sColorFilter.construct();

            sMLSParams.nBits                = 0;
            sMLSParams.nSeed                = 0;

            sLCGParams.nSeed                = 0;
            sLCGParams.enDistribution       = LCG_UNIFORM;

            sVelvetParams.nRandSeed         = 0;
            sVelvetParams.nMLSnBits         = 0;
            sVelvetParams.nMLSseed          = 0;
            sVelvetParams.enCore            = VN_CORE_LCG;
            sVelvetParams.enVelvetType      = VN_VELVET_OVN;
            sVelvetParams.fWindowWidth_s    = 0.1f;
            sVelvetParams.fARNdelta         = 0.5f;
            sVelvetParams.bCrush            = false;
            sVelvetParams.fCrushProb        = 0.5f;

            sColorParams.enColor            = NG_COLOR_WHITE;
            sColorParams.nOrder             = 50;
            sColorParams.fSlope             = 0.0f;
            sColorParams.enSlopeUnit        = STLT_SLOPE_UNIT_NEPER_PER_NEPER;

            nSampleRate                     = 0;
            enGenerator                     = NG_GEN_LCG;
            fAmplitude                      = 1.0f;
            fOffset                         = 0.0f;

            nUpdate                         = UPD_ALL;
        }

        void NoiseGenerator::destroy()
        {
            sMLS.destroy();
            sLCG.destroy();
            sVelvetNoise.destroy();
            sColorFilter.destroy();
        }

        void NoiseGenerator::init(
            uint8_t mls_n_bits, MLS::mls_t mls_seed,
            uint32_t lcg_seed,
            uint32_t velvet_rand_seed, uint8_t velvet_mls_n_bits, MLS::mls_t velvet_mls_seed)
        {
            sMLSParams.nBits = mls_n_bits;
            sMLSParams.nSeed = mls_seed;

            sLCGParams.nSeed = lcg_seed;
            sLCG.init(sLCGParams.nSeed);

            sVelvetParams.nRandSeed = velvet_rand_seed;
            sVelvetParams.nMLSnBits = velvet_mls_n_bits;
            sVelvetParams.nMLSseed  = velvet_mls_seed;
            sVelvetNoise.init(sVelvetParams.nRandSeed, sVelvetParams.nMLSnBits, sVelvetParams.nMLSseed);

            sColorFilter.init();
            sColorFilter.set_norm(STLT_NORM_AUTO);

            nUpdate                 = UPD_ALL;
        }

        void NoiseGenerator::init()
        {
            sMLSParams.nBits        = sMLS.maximum_number_of_bits(); // By default, maximise so that period is maximal.
            sMLSParams.nSeed        = 0; // This forces default seed.

            sLCG.init();
            sVelvetNoise.init();

            sColorFilter.init();
            sColorFilter.set_norm(STLT_NORM_AUTO);

            nUpdate                 = UPD_ALL;
        }

        void NoiseGenerator::set_sample_rate(size_t sr)
        {
            if (nSampleRate == sr)
                return;

            nSampleRate = sr;
            nUpdate     |= UPD_COLOR | UPD_VELVET;
        }

        void NoiseGenerator::set_mls_n_bits(uint8_t nbits)
        {
            if (nbits == sMLSParams.nBits)
                return;

            sMLSParams.nBits    = nbits;
            nUpdate             |= UPD_MLS;
        }

        void NoiseGenerator::set_mls_seed(MLS::mls_t seed)
        {
            if (seed == sMLSParams.nSeed)
                return;

            sMLSParams.nSeed    = seed;
            nUpdate             |= UPD_MLS;
        }

        void NoiseGenerator::set_lcg_distribution(lcg_dist_t dist)
        {
            if (dist == sLCGParams.enDistribution)
                return;

            sLCGParams.enDistribution = dist;
            nUpdate             |= UPD_LCG;
        }

        void NoiseGenerator::set_velvet_type(vn_velvet_type_t type)
        {
            if (type == sVelvetParams.enVelvetType)
                return;

            sVelvetParams.enVelvetType  = type;
            nUpdate                    |= UPD_VELVET;
        }

        void NoiseGenerator::set_velvet_window_width(float width)
        {
            if (width == sVelvetParams.fWindowWidth_s)
                return;

            sVelvetParams.fWindowWidth_s    = width;
            nUpdate                        |= UPD_VELVET;
        }

        void NoiseGenerator::set_velvet_arn_delta(float delta)
        {
            if (delta == sVelvetParams.fARNdelta)
                return;

            sVelvetParams.fARNdelta     = delta;
            nUpdate                    |= UPD_VELVET;
        }

        void NoiseGenerator::set_velvet_crush(bool crush)
        {
            if (crush == sVelvetParams.bCrush)
                return;

            sVelvetParams.bCrush    = crush;
            nUpdate                |= UPD_VELVET;
        }

        void NoiseGenerator::set_velvet_crushing_probability(float prob)
        {
            if (prob == sVelvetParams.fCrushProb)
                return;

            sVelvetParams.fCrushProb    = prob;
            nUpdate                    |= UPD_VELVET;
        }

        void NoiseGenerator::set_generator(ng_generator_t core)
        {
            if ((core < NG_GEN_MLS) || (core >= NG_GEN_MAX))
                return;

            if (core == enGenerator)
                return;

            enGenerator = core;
        }

        void NoiseGenerator::set_noise_color(ng_color_t color)
        {
            if ((color < NG_COLOR_WHITE) || (color >= NG_COLOR_MAX))
                return;

            if (color == sColorParams.enColor)
                return;

            sColorParams.enColor    = color;
            nUpdate                |= UPD_COLOR;
        }

        void NoiseGenerator::set_coloring_order(size_t order)
        {
            if (order == sColorParams.nOrder)
                return;

            sColorParams.nOrder     = order;
            nUpdate                |= UPD_COLOR;
        }

        void NoiseGenerator::set_color_slope(float slope, stlt_slope_unit_t unit)
        {
            if ((slope == sColorParams.fSlope) && (unit == sColorParams.enSlopeUnit))
                return;

            sColorParams.fSlope         = slope;
            sColorParams.enSlopeUnit    = unit;
            nUpdate                    |= UPD_COLOR;
        }

        void NoiseGenerator::set_amplitude(float amplitude)
        {
            if (amplitude == fAmplitude)
                return;

            fAmplitude  = amplitude;
            nUpdate     |= UPD_OTHER;
        }

        void NoiseGenerator::set_offset(float offset)
        {
            if (offset == fOffset)
                return;

            fOffset     = offset;
            nUpdate     |= UPD_OTHER;
        }

        void NoiseGenerator::update_settings()
        {
            if (!nUpdate)
                return;

            // MLS
            sMLS.set_amplitude(fAmplitude);
            sMLS.set_offset(fOffset);
            if (nUpdate & UPD_MLS)
            {
                sMLS.set_n_bits(sMLSParams.nBits);
                sMLS.set_state(sMLSParams.nSeed);
            }

            // LCG
            sLCG.set_amplitude(fAmplitude);
            sLCG.set_offset(fOffset);
            if (nUpdate & UPD_LCG)
            {
                sLCG.set_distribution(sLCGParams.enDistribution);
            }

            // Velvet
            sVelvetNoise.set_amplitude(fAmplitude);
            sVelvetNoise.set_offset(fOffset);
            if (nUpdate & UPD_VELVET)
            {
                sVelvetNoise.set_core_type(sVelvetParams.enCore);
                sVelvetNoise.set_velvet_type(sVelvetParams.enVelvetType);
                sVelvetNoise.set_velvet_window_width(seconds_to_samples(nSampleRate, sVelvetParams.fWindowWidth_s));
                sVelvetNoise.set_delta_value(sVelvetParams.fARNdelta);
                sVelvetNoise.set_crush(sVelvetParams.bCrush);
                sVelvetNoise.set_crush_probability(sVelvetParams.fCrushProb);
            }

            // Color
            if (nUpdate & UPD_COLOR)
            {
                sColorFilter.set_sample_rate(nSampleRate);

                float slope;
                stlt_slope_unit_t unit;

                switch (sColorParams.enColor)
                {
                    case NG_COLOR_PINK:
                        slope   = -0.5f;
                        unit    = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                        break;

                    case NG_COLOR_RED:
                        slope   = -1.0f;
                        unit    = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                        break;

                    case NG_COLOR_BLUE:
                        slope   = 0.5f;
                        unit    = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                        break;

                    case NG_COLOR_VIOLET:
                        slope   = 1.0f;
                        unit    = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                        break;

                    case NG_COLOR_ARBITRARY:
                        slope   = sColorParams.fSlope;
                        unit    = sColorParams.enSlopeUnit;
                        break;

                    default:
                    case NG_COLOR_WHITE:
                        slope   = 0.0f;
                        unit    = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                        break;
                }

                sColorFilter.set_order(sColorParams.nOrder);
                sColorFilter.set_slope(slope, unit);
                // Filter seems to be most nice with the bandwidth below.
                sColorFilter.set_lower_frequency(10.0f); // from 10 Hz
                // Use 90% of the digital bandwidth, as this prevents a steep rise in the high end of the frequency response.
                sColorFilter.set_upper_frequency(0.9f * 0.5f * nSampleRate);
            }

            nUpdate     = 0;
        }

        void NoiseGenerator::do_process(float *dst, size_t count)
        {
            switch (enGenerator)
            {
                case NG_GEN_MLS:
                    sMLS.process_overwrite(dst, count);
                    break;

                case NG_GEN_VELVET:
                    sVelvetNoise.process_overwrite(dst, count);
                    break;

                default:
                case NG_GEN_LCG:
                    sLCG.process_overwrite(dst, count);
                    break;
            }

            switch (sColorParams.enColor)
            {
                case NG_COLOR_PINK:
                case NG_COLOR_RED:
                case NG_COLOR_BLUE:
                case NG_COLOR_VIOLET:
                case NG_COLOR_ARBITRARY:
                    sColorFilter.process_overwrite(dst, dst, count);
                    break;

                default:
                case NG_COLOR_WHITE:
                    break;
            }
        }

        void NoiseGenerator::process_add(float *dst, const float *src, size_t count)
        {
            update_settings();

            if (src == NULL)
            {
                // No inputs, interpret `src` as zeros: dst[i] = noise[i] + 0 = noise[i]
                do_process(dst, count);
                return;
            }

            // 1X buffer for temporary processing.
            float vTemp[BUF_LIM_SIZE];

            for (size_t offset=0; offset < count; )
            {
                size_t to_do = lsp_min(count - offset, BUF_LIM_SIZE);

                // dst[i] = src[i] + do_process[i]
                do_process(vTemp, to_do);
                dsp::add3(&dst[offset], vTemp, &src[offset], to_do);

                offset += to_do;
             }
        }

        void NoiseGenerator::process_mul(float *dst, const float *src, size_t count)
        {
            update_settings();

            if (src == NULL)
            {
                // No inputs, interpret `src` as zeros: dst[i] = noise[i] * 0 = 0
                dsp::fill_zero(dst, count);
                return;
            }

            // 1X buffer for temporary processing.
            float vTemp[BUF_LIM_SIZE];

            for (size_t offset=0; offset < count; )
            {
                size_t to_do = lsp_min(count - offset, BUF_LIM_SIZE);

                // dst[i] = src[i] * noise[i]
                do_process(vTemp, to_do);
                dsp::mul3(&dst[offset], vTemp, &src[offset], to_do);

                offset += to_do;
             }
        }

        void NoiseGenerator::process_overwrite(float *dst, size_t count)
        {
            update_settings();

            do_process(dst, count);
        }

        void NoiseGenerator::freq_chart(float *re, float *im, const float *f, size_t count)
        {
            switch (sColorParams.enColor)
            {
                case NG_COLOR_PINK:
                case NG_COLOR_RED:
                case NG_COLOR_BLUE:
                case NG_COLOR_VIOLET:
                case NG_COLOR_ARBITRARY:
                    sColorFilter.freq_chart(re, im, f, count);
                    break;

                default:
                case NG_COLOR_WHITE:
                    dsp::fill(re, 1.0f, count);
                    dsp::fill(im, 0.0f, count);
                    break;
            }
        }

        void NoiseGenerator::freq_chart(float *c, const float *f, size_t count)
        {
            switch (sColorParams.enColor)
            {
                case NG_COLOR_PINK:
                case NG_COLOR_RED:
                case NG_COLOR_BLUE:
                case NG_COLOR_VIOLET:
                case NG_COLOR_ARBITRARY:
                    sColorFilter.freq_chart(c, f, count);
                    break;

                default:
                case NG_COLOR_WHITE:
                    dsp::pcomplex_fill_ri(c, 1.0f, 0.0f, count);
                    break;
            }
        }

        void NoiseGenerator::dump(IStateDumper *v) const
        {
            v->write("nSampleRate", nSampleRate);

            v->write_object("sMLS", &sMLS);
            v->write_object("sLCG", &sLCG);
            v->write_object("sVelvetNoise", &sVelvetNoise);

            v->begin_object("sMLSParams", &sMLSParams, sizeof(sMLSParams));
            {
                v->write("nBits", sMLSParams.nBits);
                v->write("nSeed", sMLSParams.nSeed);
            }
            v->end_object();

            v->begin_object("sLCGParams", &sLCGParams, sizeof(sLCGParams));
            {
                v->write("nSeed", sLCGParams.nSeed);
                v->write("enDistribution", sLCGParams.enDistribution);
            }
            v->end_object();

            v->begin_object("sVelvetParams", &sVelvetParams, sizeof(sVelvetParams));
            {
                v->write("nRandSeed", sVelvetParams.nRandSeed);
                v->write("nMLSnBits", sVelvetParams.nMLSnBits);
                v->write("nMLSseed", sVelvetParams.nMLSseed);
                v->write("enCore", sVelvetParams.enCore);
                v->write("enVelvetType", sVelvetParams.enVelvetType);
                v->write("fWindowWidth_s", sVelvetParams.fWindowWidth_s);
                v->write("fARNdelta", sVelvetParams.fARNdelta);
                v->write("bCrush", sVelvetParams.bCrush);
                v->write("fCrushProb", sVelvetParams.fCrushProb);
            }
            v->end_object();

            v->begin_object("sColorParams", &sColorParams, sizeof(sColorParams));
            {
                v->write("enColor", sColorParams.enColor);
                v->write("nOrder", sColorParams.nOrder);
                v->write("fSlope", sColorParams.fSlope);
                v->write("enSlopeUnit", sColorParams.enSlopeUnit);
            }
            v->end_object();

            v->write("enGenerator", enGenerator);

            v->write("fAmplitude", fAmplitude);
            v->write("fOffset", fOffset);
        }
    }
}

