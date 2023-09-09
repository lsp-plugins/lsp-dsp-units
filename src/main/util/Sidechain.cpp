/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 14 сент. 2016 г.
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

#include <lsp-plug.in/dsp-units/util/Sidechain.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/stdlib/math.h>

#define REFRESH_RATE        0x1000
#define MIN_GAP_ITEMS       0x200U

namespace lsp
{
    namespace dspu
    {
        Sidechain::Sidechain()
        {
            construct();
        }

        Sidechain::~Sidechain()
        {
            destroy();
        }

        void Sidechain::construct()
        {
            sBuffer.construct();

            nReactivity         = 0;
            fReactivity         = 0.0f;
            fTau                = 0.0f;
            fRmsValue           = 0.0f;
            nSource             = SCS_MIDDLE;
            nMode               = SCM_RMS;
            nSampleRate         = 0;
            nRefresh            = 0;
            nChannels           = 0;
            fMaxReactivity      = 0.0f;
            fGain               = 1.0f;
            nFlags              = SCF_UPDATE | SCF_CLEAR;
            pPreEq              = NULL;
        }

        void Sidechain::destroy()
        {
            sBuffer.destroy();
        }

        bool Sidechain::init(size_t channels, float max_reactivity)
        {
            if ((channels != 1) && (channels != 2))
                return false;

            nReactivity         = 0;
            fReactivity         = 0.0f;
            fTau                = 0.0f;
            fRmsValue           = 0.0f;
            nSource             = SCS_MIDDLE;
            nMode               = SCM_RMS;
            nSampleRate         = 0;
            nRefresh            = 0;
            nChannels           = channels;
            fMaxReactivity      = max_reactivity;
            fGain               = 1.0f;
            nFlags              = SCF_UPDATE | SCF_CLEAR;

            return true;
        }

        void Sidechain::set_sample_rate(size_t sr)
        {
            nSampleRate         = sr;
            nFlags              = SCF_UPDATE | SCF_CLEAR;
            size_t gap          = lsp_max(millis_to_samples(sr, fMaxReactivity), 1);
            size_t buf_size     = lsp_max(gap, MIN_GAP_ITEMS);
            sBuffer.init(buf_size * 4, gap);
        }

        void Sidechain::set_reactivity(float reactivity)
        {
            if ((fReactivity == reactivity) ||
                (reactivity < 0.0f) ||
                (reactivity > fMaxReactivity))
                return;
            fReactivity         = reactivity;
            nFlags             |= SCF_UPDATE;
        }

        void Sidechain::set_stereo_mode(sidechain_stereo_mode_t mode)
        {
            sidechain_stereo_mode_t old = (nFlags & SCF_MIDSIDE) ? SCSM_MIDSIDE : SCSM_STEREO;
            if (old == mode)
                return;
            nFlags              = lsp_setflag(nFlags, SCF_MIDSIDE, mode == SCSM_MIDSIDE);
            nFlags             |= SCF_CLEAR;
        }

        void Sidechain::clear()
        {
            nFlags             |= SCF_CLEAR;
        }

        void Sidechain::update_settings()
        {
            if (!(nFlags & (SCF_UPDATE | SCF_CLEAR)))
                return;

            if (nFlags & SCF_UPDATE)
            {
                ssize_t react       = millis_to_samples(nSampleRate, fReactivity);
                nReactivity         = lsp_max(react, 1);
                fTau                = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (nReactivity)); // Tau is based on seconds
                nRefresh            = REFRESH_RATE; // Force the function to be refreshed
            }

            if (nFlags & SCF_CLEAR)
            {
                fRmsValue           = 0.0f;
                nRefresh            = 0;
                sBuffer.fill(0.0f);
                if (pPreEq != NULL)
                    pPreEq->reset();
            }

            nFlags              = 0;
        }

        void Sidechain::refresh_processing()
        {
            switch (nMode)
            {
                case SCM_PEAK:
                    fRmsValue       = 0.0f;
                    break;

                case SCM_UNIFORM:
                    fRmsValue       = dsp::h_abs_sum(sBuffer.tail(nReactivity), nReactivity);
                    break;

                case SCM_RMS:
                    fRmsValue       = dsp::h_sqr_sum(sBuffer.tail(nReactivity), nReactivity);
                    break;

                default:
                    break;
            }
        }

        void Sidechain::select_buffer(float **a, float **b, size_t *size)
        {
            size_t buf_size;
            float *base;

            // Allocate some space in the shift buffer which can be reused for sure
            if (sBuffer.tail_gap_size() > sBuffer.head_gap_size())
            {
                buf_size    = sBuffer.tail_gap_size() >> 1;
                base        = sBuffer.tail();
            }
            else
            {
                buf_size    = sBuffer.head_gap_size() >> 1;
                base        = sBuffer.data();
            }

            *a          = &base[0];
            *b          = &base[buf_size];
            *size       = buf_size;
        }

        bool Sidechain::preprocess(float *out, const float **in, size_t samples)
        {
            float *a, *b;
            size_t max_samples;

            // Special case, treat NULL as zero input
            if (in == NULL)
            {
                dsp::fill_zero(out, samples);
                return true;
            }

            if (nChannels == 2)
            {
                if (nFlags & SCF_MIDSIDE)
                {
                    switch (nSource)
                    {
                        case SCS_LEFT:
                            dsp::ms_to_left(out, in[0], in[1], samples);
                            if (pPreEq != NULL)
                                pPreEq->process(out, out, samples);
                            dsp::abs1(out, samples);
                            break;
                        case SCS_RIGHT:
                            dsp::ms_to_right(out, in[0], in[1], samples);
                            if (pPreEq != NULL)
                                pPreEq->process(out, out, samples);
                            dsp::abs1(out, samples);
                            break;
                        case SCS_MIDDLE:
                            if (pPreEq != NULL)
                            {
                                pPreEq->process(out, in[0], samples);
                                dsp::abs1(out, samples);
                            }
                            else
                                dsp::abs2(out, in[0], samples);
                            break;
                        case SCS_SIDE:
                            if (pPreEq != NULL)
                            {
                                pPreEq->process(out, in[1], samples);
                                dsp::abs1(out, samples);
                            }
                            else
                                dsp::abs2(out, in[1], samples);
                            break;
                        case SCS_AMIN:
                            select_buffer(&a, &b, &max_samples);
                            if (pPreEq != NULL)
                            {
                                for (size_t off=0; off<samples; )
                                {
                                    size_t count    = lsp_min(samples, max_samples);
                                    dsp::ms_to_lr(a, b, &in[0][off], &in[1][off], count);
                                    dsp::psmin3(&out[off], a, b, count);
                                    off            += count;
                                }
                                pPreEq->process(out, out, samples);
                                dsp::abs1(out, samples);
                            }
                            else
                            {
                                for (size_t off=0; off<samples; )
                                {
                                    size_t count    = lsp_min(samples, max_samples);
                                    dsp::ms_to_lr(a, b, &in[0][off], &in[1][off], count);
                                    dsp::pamin3(&out[off], a, b, samples);
                                    off            += count;
                                }
                            }
                            break;
                        case SCS_AMAX:
                            select_buffer(&a, &b, &max_samples);
                            if (pPreEq != NULL)
                            {
                                for (size_t off=0; off<samples; )
                                {
                                    size_t count    = lsp_min(samples, max_samples);
                                    dsp::ms_to_lr(a, b, &in[0][off], &in[1][off], count);
                                    dsp::psmax3(&out[off], a, b, count);
                                    off            += count;
                                }
                                pPreEq->process(out, out, samples);
                                dsp::abs1(out, samples);
                            }
                            else
                            {
                                for (size_t off=0; off<samples; )
                                {
                                    size_t count    = lsp_min(samples, max_samples);
                                    dsp::ms_to_lr(a, b, &in[0][off], &in[1][off], count);
                                    dsp::pamax3(&out[off], a, b, samples);
                                    off            += count;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }
                else
                {
                    switch (nSource)
                    {
                        case SCS_LEFT:
                            if (pPreEq != NULL)
                            {
                                pPreEq->process(out, in[0], samples);
                                dsp::abs1(out, samples);
                            }
                            else
                                dsp::abs2(out, in[0], samples);
                            break;
                        case SCS_RIGHT:
                            if (pPreEq != NULL)
                            {
                                pPreEq->process(out, in[1], samples);
                                dsp::abs1(out, samples);
                            }
                            else
                                dsp::abs2(out, in[1], samples);
                            break;
                        case SCS_MIDDLE:
                            dsp::lr_to_mid(out, in[0], in[1], samples);
                            if (pPreEq != NULL)
                                pPreEq->process(out, out, samples);
                            dsp::abs1(out, samples);
                            break;
                        case SCS_SIDE:
                            dsp::lr_to_side(out, in[0], in[1], samples);
                            if (pPreEq != NULL)
                                pPreEq->process(out, out, samples);
                            dsp::abs1(out, samples);
                            break;
                        case SCS_AMIN:
                            if (pPreEq != NULL)
                            {
                                dsp::psmin3(out, in[0], in[1], samples);
                                pPreEq->process(out, out, samples);
                                dsp::abs1(out, samples);
                            }
                            else
                                dsp::pamin3(out, in[0], in[1], samples);
                            break;
                        case SCS_AMAX:
                            if (pPreEq != NULL)
                            {
                                dsp::psmax3(out, in[0], in[1], samples);
                                pPreEq->process(out, out, samples);
                                dsp::abs1(out, samples);
                            }
                            else
                                dsp::pamax3(out, in[0], in[1], samples);
                            break;
                        default:
                            break;
                    }
                }
            }
            else if (nChannels == 1)
            {
                if (pPreEq != NULL)
                {
                    pPreEq->process(out, in[0], samples);
                    dsp::abs1(out, samples);
                }
                else
                    dsp::abs2(out, in[0], samples);
            }
            else
            {
                dsp::fill_zero(out, samples);
                if (pPreEq != NULL)
                {
                    pPreEq->process(out, out, samples);
                    dsp::abs1(out, samples);
                }
                return false;
            }

            return true;
        }

        bool Sidechain::preprocess(float *out, const float *in)
        {
            float s, a, b;

            if (nChannels == 2)
            {
                if (nFlags & SCF_MIDSIDE)
                {
                    switch (nSource)
                    {
                        case SCS_LEFT:
                            s = in[0] + in[1];
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        case SCS_RIGHT:
                            s = in[0] - in[1];
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        case SCS_MIDDLE:
                            s = in[0];
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        case SCS_SIDE:
                            s = in[1];
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        case SCS_AMIN:
                            a = in[0] + in[1];
                            b = in[0] - in[1];
                            s   = (fabs(a) < fabs(b)) ? a : b;
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        case SCS_AMAX:
                            a = in[0] + in[1];
                            b = in[0] - in[1];
                            s   = (fabs(b) < fabs(a)) ? a : b;
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        default:
                            s = in[0];
                            break;
                    }
                }
                else
                {
                    switch (nSource)
                    {
                        case SCS_LEFT:
                            s = in[0];
                            break;
                        case SCS_RIGHT:
                            s = in[1];
                            break;
                        case SCS_MIDDLE:
                            s = (in[0] + in[1])*0.5f;
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        case SCS_SIDE:
                            s = (in[0] - in[1])*0.5f;
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        case SCS_AMIN:
                            s   = (fabs(in[0]) < fabs(in[1])) ? in[0] : in[1];
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        case SCS_AMAX:
                            s   = (fabs(in[1]) < fabs(in[0])) ? in[0] : in[1];
                            if (pPreEq != NULL)
                                pPreEq->process(&s, &s, 1);
                            break;
                        default:
                            s = (in[0] + in[1])*0.5f;
                            break;
                    }
                }
            }
            else if (nChannels == 1)
            {
                s       = in[0];
                if (pPreEq != NULL)
                    pPreEq->process(&s, &s, 1);
            }
            else
            {
                s       = 0.0f;
                if (pPreEq != NULL)
                    pPreEq->process(&s, &s, 1);
                *out    = s;
                return false;
            }

            *out    = (s < 0.0f) ? -s : s;
            return true;
        }

        void Sidechain::process(float *out, const float **in, size_t samples)
        {
            // Check if need update settings
            update_settings();

            // Determine what source to use
            if (!preprocess(out, in, samples))
                return;

            // Adjust pre-amplification
            if (fGain != 1.0f)
                dsp::mul_k2(out, fGain, samples);

            for (size_t offset=0; offset < samples; )
            {
                // Update refresh counter
                if (nRefresh >= REFRESH_RATE)
                {
                    refresh_processing();
                    nRefresh   %= REFRESH_RATE;
                }

                // Calculate sidechain function
                const size_t to_do  = lsp_min(samples - offset, REFRESH_RATE - nRefresh);

                switch (nMode)
                {
                    // Peak processing
                    case SCM_PEAK:
                    {
                        for (size_t processed = 0; processed < to_do; )
                        {
                            size_t n    = sBuffer.append(out, to_do - processed);
                            sBuffer.shift(n);
                            processed  += n;
                            out        += n;
                        }
                        break;
                    }

                    // Lo-pass filter processing
                    case SCM_LPF:
                    {
                        for (size_t processed = 0; processed < to_do; )
                        {
                            size_t n    = sBuffer.append(out, to_do - processed);
                            sBuffer.shift(n);
                            processed  += n;

                            while (n--)
                            {
                                fRmsValue      += fTau * ((*out) - fRmsValue);
                                *(out++)        = (fRmsValue < 0.0f) ? 0.0f : fRmsValue;
                            }
                        }
                        break;
                    }

                    // Uniform processing
                    case SCM_UNIFORM:
                    {
                        if (nReactivity <= 0)
                            break;
                        float interval  = 1.0f / nReactivity;

                        for (size_t processed = 0; processed < to_do; )
                        {
                            size_t n    = sBuffer.append(out, to_do - processed);
                            float *p    = sBuffer.tail(nReactivity + n);

                            for (size_t i=0; i<n; ++i)
                            {
                                fRmsValue      += *(out) - *(p++);
                                *(out++)        = (fRmsValue < 0.0f) ? 0.0f : fRmsValue * interval;
                            }

                            // Remove old samples
                            sBuffer.shift(n);
                            processed  += n;
                        }
                        break;
                    }

                    // RMS processing
                    case SCM_RMS:
                    {
                        if (nReactivity <= 0)
                            break;
                        float interval  = 1.0f / nReactivity;

                        for (size_t processed = 0; processed < to_do; )
                        {
                            size_t n        = sBuffer.append(out, to_do - processed);
                            float *p        = sBuffer.tail(nReactivity + n);

                            for (size_t i=0; i<n; ++i)
                            {
                                float sample    = *out;
                                float last      = *(p++);
                                fRmsValue      += sample*sample - last*last;
                                *(out++)        = (fRmsValue < 0.0f) ? 0.0f : sqrtf(fRmsValue * interval);
                            }

                            sBuffer.shift(n);
                            processed      += n;
                        }
                        break;
                    }

                    default:
                        break;
                }

                // Update offset
                offset         += to_do;
                nRefresh       += to_do;
            }
        }

        float Sidechain::process(const float *in)
        {
            // Check if need update settings
            update_settings();

            float out   = 0.0f;
            if (!preprocess(&out, in))
                return out;

            // Adjust pre-amplification
            out *= fGain;

            // Update refresh counter
            if ((++nRefresh) >= REFRESH_RATE)
            {
                refresh_processing();
                nRefresh   %= REFRESH_RATE;
            }

            // Calculate sidechain function
            switch (nMode)
            {
                // Peak processing
                case SCM_PEAK:
                {
                    sBuffer.append(out);
                    sBuffer.shift();
                    break;
                }

                // Lo-pass filter processing
                case SCM_LPF:
                {
                    sBuffer.append(out);
                    sBuffer.shift();
                    fRmsValue      += fTau * (out - fRmsValue);
                    out             = (fRmsValue < 0.0f) ? 0.0f : fRmsValue;
                    break;
                }

                // Uniform processing
                case SCM_UNIFORM:
                {
                    if (nReactivity <= 0)
                        break;
                    sBuffer.append(out);
                    fRmsValue      += out - sBuffer.last(nReactivity + 1);
                    out             = (fRmsValue < 0.0f) ? 0.0f : fRmsValue / float(nReactivity);
                    sBuffer.shift();
                    break;
                }

                // RMS processing
                case SCM_RMS:
                {
                    if (nReactivity <= 0)
                        break;
                    sBuffer.append(out);
                    float last      = sBuffer.last(nReactivity + 1);
                    fRmsValue      += out*out - last*last;
                    out             = (fRmsValue < 0.0f) ? 0.0f : sqrtf(fRmsValue / float(nReactivity));
                    sBuffer.shift();
                    break;
                }

                default:
                    break;
            }

            return out;
        }

        void Sidechain::dump(IStateDumper *v) const
        {
            v->write_object("sBuffer", &sBuffer);
            v->write("nReactivity", nReactivity);
            v->write("fReactivity", fReactivity);
            v->write("fTau", fTau);
            v->write("fRmsValue", fRmsValue);
            v->write("nSource", nSource);
            v->write("nMode", nMode);
            v->write("nSampleRate", nSampleRate);
            v->write("nRefresh", nRefresh);
            v->write("nChannels", nChannels);
            v->write("fMaxReactivity", fMaxReactivity);
            v->write("fGain", fGain);
            v->write("nFlags", nFlags);
            v->write("pPreEq", pPreEq);
        }
    } /* namespace dspu */
} /* namespace lsp */
