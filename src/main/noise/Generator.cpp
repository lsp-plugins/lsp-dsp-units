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

#define BUF_LIM_SIZE    2048u

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
            sColorParams.nOrder             = 0.0f;
            sColorParams.enSlopeUnit        = STLT_SLOPE_UNIT_NEPER_PER_NEPER;

            enGenerator                     = NG_GEN_LCG;
            pData                           = NULL;
            vBuffer                         = NULL;

            bSync                           = true;
        }

        void NoiseGenerator::destroy()
        {
            sMLS.destroy();
            sLCG.destroy();
            sVelvetNoise.destroy();

            free_aligned(pData);
            pData = NULL;

            if (vBuffer != NULL)
            {
                delete [] vBuffer;
                vBuffer = NULL;
            }
        }

        void NoiseGenerator::init_buffers()
        {
            // 1X buffer for processing.
            size_t samples = BUF_LIM_SIZE;

            float *ptr = alloc_aligned<float>(pData, samples);
            if (ptr == NULL)
                return;

            lsp_guard_assert(float *save = ptr);

            vBuffer = ptr;
            ptr += BUF_LIM_SIZE;

            lsp_assert(ptr <= &save[samples]);
        }

        void NoiseGenerator::init(
                uint8_t mls_n_bits, MLS::mls_t mls_seed,
                uint32_t lcg_seed,
                uint32_t velvet_rand_seed, uint8_t velvet_mls_n_bits, MLS::mls_t velvet_mls_seed
                )
        {
            sMLSParams.nBits = mls_n_bits;
            sMLSParams.nSeed = mls_seed;

            sLCGParams.nSeed = lcg_seed;
            sLCG.init(sLCGParams.nSeed);

            sVelvetParams.nRandSeed = velvet_rand_seed;
            sVelvetParams.nMLSnBits = velvet_mls_n_bits;
            sVelvetParams.nMLSseed = velvet_mls_seed;
            sVelvetNoise.init(sVelvetParams.nRandSeed, sVelvetParams.nMLSnBits, sVelvetParams.nMLSseed);

            init_buffers();

            bSync = true;
        }

        void NoiseGenerator::init()
        {
            sMLSParams.nBits = sMLS.maximum_number_of_bits(); // By default, maximise so that period is maximal.
            sMLSParams.nSeed = 0; // This forces default seed.

            sLCG.init();
            sVelvetNoise.init();

            init_buffers();

            bSync = true;
        }

        void NoiseGenerator::update_settings()
        {
            if (!bSync)
                return;

            // MLS
            sMLS.set_n_bits(sMLSParams.nBits);
            sMLS.set_state(sMLSParams.nSeed);
            sMLS.set_amplitude(fAmplitude);
            sMLS.set_offset(fOffset);
            sMLS.update_settings();

            // LCG
            sLCG.set_distribution(sLCGParams.enDistribution);
            sLCG.set_amplitude(fAmplitude);
            sLCG.set_offset(fOffset);
            // sLCG has no update_settings method.

            // Velvet
            sVelvetNoise.set_core_type(sVelvetParams.enCore);
            sVelvetNoise.set_velvet_type(sVelvetParams.enVelvetType);
            sVelvetNoise.set_velvet_window_width(seconds_to_samples(nSampleRate, sVelvetParams.fWindowWidth_s));
            sVelvetNoise.set_delta_value(sVelvetParams.fARNdelta);
            sVelvetNoise.set_amplitude(fAmplitude);
            sVelvetNoise.set_offset(fOffset);
            sVelvetNoise.set_crush(sVelvetParams.bCrush);
            sVelvetNoise.set_crush_probability(sVelvetParams.fCrushProb);
            // sVelvetNoise has no update_settings method.

            // Color

            sColorFilter.set_sample_rate(nSampleRate);

            float slope;
            stlt_slope_unit_t unit;

            switch (sColorParams.enColor)
            {
                case (NG_COLOR_PINK):
                {
                    slope = -0.5f;
                    unit = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                }
                break;

                case (NG_COLOR_RED):
                {
                    slope = -1.0f;
                    unit = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                }
                break;

                case (NG_COLOR_BLUE):
                {
                    slope = 0.5f;
                    unit = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                }
                break;

                case (NG_COLOR_VIOLET):
                {
                    slope = 1.0f;
                    unit = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                }
                break;

                case (NG_COLOR_ARBITRARY):
                {
                    slope = sColorParams.fSlope;
                    unit = sColorParams.enSlopeUnit;
                }
                break;

                default:
                case (NG_COLOR_WHITE):
                {
                    slope = 0.0f;
                    unit = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
                }
                break;
            }

            sColorFilter.set_order(sColorParams.nOrder);
            sColorFilter.set_slope(slope, unit);
            // Filter seems to be most nice with the bandwidth below.
            sColorFilter.set_lower_frequency(10.0f); // from 10 Hz
            // Use 90% of the digital bandwidth, as this prevents a steep rise in the high end of the frequency response.
            sColorFilter.set_upper_frequency(0.9f * 0.5f * nSampleRate);
            sColorFilter.update_settings();

            //

            bSync = true;
        }

        void NoiseGenerator::do_process(float *dst, size_t count)
        {
            switch (enGenerator)
            {
                case NG_GEN_MLS:
                {
                    sMLS.process_overwrite(dst, count);
                }
                break;

                case NG_GEN_VELVET:
                {
                    sVelvetNoise.process_overwrite(dst, count);
                }
                break;

                default:
                case NG_GEN_LCG:
                {
                    sLCG.process_overwrite(dst, count);
                }
                break;
            }

            switch (sColorParams.enColor)
            {
                case NG_COLOR_PINK:
                case NG_COLOR_RED:
                case NG_COLOR_ARBITRARY:
                {
                    sColorFilter.process_overwrite(dst, dst, count);
                }
                break;

                default:
                case (NG_COLOR_WHITE):
                {
                    return;
                }
                break;
            }
        }

        void NoiseGenerator::process_add(float *dst, const float *src, size_t count)
        {
            if (src != NULL)
                dsp::copy(dst, src, count);
            else
                dsp::fill_zero(dst, count);

            while (count > 0)
            {
                size_t to_do = lsp_min(BUF_LIM_SIZE, count);

                do_process(vBuffer, to_do);
                dsp::add2(dst, vBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }
        }

        void NoiseGenerator::process_mul(float *dst, const float *src, size_t count)
        {
            if (src != NULL)
                dsp::copy(dst, src, count);
            else
                dsp::fill_zero(dst, count);

            while (count > 0)
            {
                size_t to_do = lsp_min(BUF_LIM_SIZE, count);

                do_process(vBuffer, to_do);
                dsp::mul2(dst, vBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }
        }

        void NoiseGenerator::process_overwrite(float *dst, size_t count)
        {
            while (count > 0)
            {
                size_t to_do = lsp_min(BUF_LIM_SIZE, count);

                do_process(vBuffer, to_do);
                dsp::copy(dst, vBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
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

            v->write("pData", pData);
            v->write("vBuffer", vBuffer);

            v->write("bSync", bSync);
        }
    }
}

