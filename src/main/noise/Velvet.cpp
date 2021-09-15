/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 27 Jun 2021
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

#include <lsp-plug.in/dsp-units/noise/Velvet.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/stdlib/math.h>

#define MIN_WINDOW_WIDTH 1.0f
#define DFL_WINDOW_WIDTH 10.0f

#define BUF_LIM_SIZE 2048

namespace lsp
{
    namespace dspu
    {
        Velvet::Velvet()
        {
            construct();
        }

        void Velvet::construct()
        {
            enCore                      = VN_CORE_MLS;

            enVelvetType                = VN_VELVET_OVN;

            sCrushParams.bCrush         = false;
            sCrushParams.fCrushProb     = 0.5f;

            fWindowWidth                = DFL_WINDOW_WIDTH;
            fARNdelta                   = 0.5f;
            fAmplitude                  = 1.0f;
            fOffset                     = 0.0f;

            pData                       = NULL;
            vBuffer                     = NULL;
        }

        Velvet::~Velvet()
        {
            destroy();
        }

        void Velvet::destroy()
        {
            free_aligned(pData);
            pData = NULL;

            if (vBuffer != NULL)
            {
                delete [] vBuffer;
                vBuffer = NULL;
            }
        }

        void Velvet::init_buffers()
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

        void Velvet::init(uint32_t randseed, uint8_t mlsnbits, MLS::mls_t mlsseed)
        {
            sRandomizer.init(randseed);

            // Ensure that the MLS sequence has unit amplitude and 0 DC bias.
            sMLS.set_amplitude(1.0f);
            sMLS.set_offset(0.0f);

            sMLS.set_n_bits(mlsnbits);
            sMLS.set_state(mlsseed);
            sMLS.update_settings();

            init_buffers();
        }

        void Velvet::init()
        {
            sRandomizer.init();

            // Ensure that the MLS sequence has unit amplitude and 0 DC bias.
            sMLS.set_amplitude(1.0f);
            sMLS.set_offset(0.0f);

            // Simply use defaults in the class.
            sMLS.update_settings();

            init_buffers();
        }

        float Velvet::get_random_value()
        {
            return sRandomizer.random(RND_LINEAR);
        }

        float Velvet::get_spike()
        {
            switch (enCore)
            {
                case VN_CORE_MLS:
                    return sMLS.single_sample_processor(); // Either 1 or -1
                default:
                case VN_CORE_LCG:
                    return 2.0f * roundf(get_random_value()) - 1.0f; // Either 1 or -1
            }
        }

        float Velvet::get_crushed_spike()
        {
            float spike = 0.0f;

            if (get_random_value() > sCrushParams.fCrushProb)
                spike = 1.0f;

            return 2.0f * spike - 1.0f; // Either 1 or -1
        }

        void Velvet::do_process(float *dst, size_t count)
        {

            switch (enVelvetType)
            {

                case VN_VELVET_OVN:
                {
                    dsp::fill_zero(dst, count);

                    size_t idx = 0;
                    size_t scan = 0;

                    while (idx < count)
                    {
                        idx = scan * fWindowWidth + get_random_value() * (fWindowWidth - 1.0f);

                        if (sCrushParams.bCrush)
                            dst[idx] = get_crushed_spike();
                        else
                            dst[idx] = get_spike();

                        ++scan;
                    }
                }
                break;

                case VN_VELVET_OVNA:
                {
                    dsp::fill_zero(dst, count);

                    size_t idx = 0;
                    size_t scan = 0;

                    while (idx < count)
                    {
                        idx = scan * fWindowWidth + get_random_value() * fWindowWidth;

                        if (sCrushParams.bCrush)
                            dst[idx] = get_crushed_spike();
                        else
                            dst[idx] = get_spike();

                        ++scan;
                    }
                }
                break;

                case VN_VELVET_ARN:
                {
                    dsp::fill_zero(dst, count);

                    size_t idx = 0;

                    while (idx < count)
                    {
                        idx += 1.0f + (1.0f - fARNdelta) * (fWindowWidth - 1.0f) + 2.0f * fARNdelta * (fWindowWidth - 1.0f) * get_random_value();

                        if (sCrushParams.bCrush)
                            dst[idx] = get_crushed_spike();
                        else
                            dst[idx] = get_spike();
                    }
                }
                break;

                case VN_VELVET_TRN:
                {
                    while (count--)
                    {
                        float value = roundf(fWindowWidth * (get_random_value() - 0.5f) / (fWindowWidth - 1.0f));

                        if (sCrushParams.bCrush)
                        {
                            float multiplier = 1.0f;

                            if (get_random_value() > sCrushParams.fCrushProb)
                                multiplier = -1.0f;

                            *(dst++) = multiplier * abs(value);
                        }
                        else
                        {
                            *(dst++) = value;
                        }
                    }
                }
                break;

                default:
                case VN_VELVET_MAX:
                {
                    dsp::fill_zero(dst, count);
                }
                break;
            }
        }

        void Velvet::process_add(float *dst, const float *src, size_t count)
        {
            if (src != NULL)
                dsp::copy(dst, src, count);
            else
                dsp::fill_zero(dst, count);

            while (count > 0)
            {
                size_t to_do = (count > BUF_LIM_SIZE) ? BUF_LIM_SIZE : count;

                do_process(vBuffer, to_do);
                dsp::mul_k2(vBuffer, fAmplitude, to_do);
                dsp::add_k2(vBuffer, fOffset, to_do);
                dsp::add2(dst, vBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }
        }

        void Velvet::process_mul(float *dst, const float *src, size_t count)
        {
            if (src != NULL)
                dsp::copy(dst, src, count);
            else
                dsp::fill_zero(dst, count);

            while (count > 0)
            {
                size_t to_do = (count > BUF_LIM_SIZE) ? BUF_LIM_SIZE : count;

                do_process(vBuffer, to_do);
                dsp::mul_k2(vBuffer, fAmplitude, to_do);
                dsp::add_k2(vBuffer, fOffset, to_do);
                dsp::mul2(dst, vBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }
        }

        void Velvet::process_overwrite(float *dst, size_t count)
        {
            while (count > 0)
            {
                size_t to_do = (count > BUF_LIM_SIZE) ? BUF_LIM_SIZE : count;

                do_process(vBuffer, to_do);
                dsp::mul_k2(vBuffer, fAmplitude, to_do);
                dsp::add_k2(vBuffer, fOffset, to_do);
                dsp::copy(dst, vBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }
        }

        void Velvet::dump(IStateDumper *v) const
        {
            v->write_object("sRandomizer", &sRandomizer);

            v->write_object("sMLS", &sMLS);

            v->write("enCore", enCore);

            v->write("enVelvetType", enVelvetType);

            v->begin_object("sCrushParams", &sCrushParams, sizeof(sCrushParams));
            {
                v->write("bCrush", sCrushParams.bCrush);
                v->write("fCrushProb", sCrushParams.fCrushProb);
            }
            v->end_object();

            v->write("fWindowWidth", fWindowWidth);
            v->write("fARNdelta", fARNdelta);
            v->write("fAmplitude", fAmplitude);
            v->write("fOffset", fOffset);

            v->write("pData", pData);
            v->write("vBuffer", vBuffer);
        }
    }
}
