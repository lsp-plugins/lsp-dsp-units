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

#define MIN_WINDOW_WIDTH    1.0f
#define DFL_WINDOW_WIDTH    10.0f
#define BUF_LIM_SIZE        256u

namespace lsp
{
    namespace dspu
    {

        Velvet::Velvet()
        {
            construct();
        }

        Velvet::~Velvet()
        {
            destroy();
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

            sRandomizer.construct();
        }

        void Velvet::destroy()
        {
            sRandomizer.destroy();
        }

        void Velvet::set_core_type(vn_core_t core)
        {
            if ((core < VN_CORE_MLS) || (core >= VN_CORE_MAX))
                return;

            enCore = core;
        }

        void Velvet::set_velvet_type(vn_velvet_type_t type)
        {
            if ((type < VN_VELVET_OVN) || (type >= VN_VELVET_MAX))
                return;

            enVelvetType = type;
        }

        void Velvet::set_velvet_window_width(float width)
        {
            fWindowWidth = lsp_max(width, MIN_WINDOW_WIDTH);
        }

        void Velvet::set_delta_value(float delta)
        {
            fARNdelta       = lsp_limit(delta, 0.0f, 1.0f);
        }

        void Velvet::set_amplitude(float amplitude)
        {
            fAmplitude      = amplitude;
        }

        void Velvet::set_offset(float offset)
        {
            fOffset         = offset;
        }

        void Velvet::set_crush(bool crush)
        {
            sCrushParams.bCrush = crush;
        }

        void Velvet::set_crush_probability(float prob)
        {
            sCrushParams.fCrushProb = lsp_limit(prob, 0.0f, 1.0f);
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
        }

        void Velvet::init()
        {
            sRandomizer.init();

            // Ensure that the MLS sequence has unit amplitude and 0 DC bias.
            sMLS.set_amplitude(1.0f);
            sMLS.set_offset(0.0f);

            // Simply use defaults in the class.
            sMLS.update_settings();
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
                    return sMLS.process_single(); // Either 1 or -1

                default:
                case VN_CORE_LCG:
                    return 2.0f * roundf(get_random_value()) - 1.0f; // Either 1 or -1
            }
        }

        float Velvet::get_crushed_spike()
        {
            return (get_random_value() > sCrushParams.fCrushProb) ? 1.0f : -1.0f;
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

                    float k = fWindowWidth - 1.0f;
                    while (true)
                    {
                        idx = scan * fWindowWidth + get_random_value() * k;
                        if (idx >= count)
                            break;

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

                    while (true)
                    {
                        idx = scan * fWindowWidth + get_random_value() * fWindowWidth;
                        if (idx >= count)
                            break;

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

                    float k = 2.0f * fARNdelta * (fWindowWidth - 1.0f);
                    float b = (1.0f - fARNdelta) * (fWindowWidth - 1.0f);
                    while (true)
                    {
                        idx += 1.0f + b + k * get_random_value();
                        if (idx >= count)
                            break;

                        if (sCrushParams.bCrush)
                            dst[idx] = get_crushed_spike();
                        else
                            dst[idx] = get_spike();
                    }
                }
                break;

                case VN_VELVET_TRN:
                {
                    size_t idx = 0;
                    float k = fWindowWidth / (fWindowWidth - 1.0f);
                    while (idx < count)
                    {
                        dst[idx] = roundf(k * (get_random_value() - 0.5f));
                        ++idx;
                    }

                    if (sCrushParams.bCrush)
                    {
                        idx = 0;
                        while (idx < count)
                        {
                            float multiplier = 1.0f;
                            if (get_random_value() > sCrushParams.fCrushProb)
                                multiplier = -1.0f;

                            dst[idx] = multiplier * abs(dst[idx]);
                            ++idx;
                        }
                    }
                }
                break;

                default:
                case VN_VELVET_MAX:
                    dsp::fill_zero(dst, count);
                    break;
            }
        }

        void Velvet::process_add(float *dst, const float *src, size_t count)
        {
            if (src == NULL)
            {
                // No inputs, interpret `src` as zeros: dst[i] = noise[i] + 0 = noise[i]
                do_process(dst, count);
                dsp::mul_k2(dst, fAmplitude, count);
                dsp::add_k2(dst, fOffset, count);
                return;
            }

            // 1X buffer for temporary processing.
            float vTemp[BUF_LIM_SIZE];

            for (size_t offset=0; offset < count; )
            {
                size_t to_do = lsp_min(count - offset, BUF_LIM_SIZE);

                // dst[i] = src[i] + noise[i]
                do_process(vTemp, to_do);
                dsp::mul_k2(vTemp, fAmplitude, to_do);
                dsp::add_k2(vTemp, fOffset, to_do);
                dsp::add3(&dst[offset], vTemp, &src[offset], to_do);

                offset += to_do;
             }
        }

        void Velvet::process_mul(float *dst, const float *src, size_t count)
        {
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
                dsp::mul_k2(vTemp, fAmplitude, to_do);
                dsp::add_k2(vTemp, fOffset, to_do);
                dsp::mul3(&dst[offset], vTemp, &src[offset], to_do);

                offset += to_do;
             }
        }

        void Velvet::process_overwrite(float *dst, size_t count)
        {
            do_process(dst, count);
            dsp::mul_k2(dst, fAmplitude, count);
            dsp::add_k2(dst, fOffset, count);
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
        }
    }
}
