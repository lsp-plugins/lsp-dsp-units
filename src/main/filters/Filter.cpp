/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 28 июня 2016 г.
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

#include <lsp-plug.in/dsp-units/filters/Filter.h>
#include <lsp-plug.in/dsp-units/const.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>

#define MIN_APO_Q           0.1f        /* Minimum Q for APO cannot be 0 */
#define STACK_BUF_SIZE      0x100U

namespace lsp
{
    namespace dspu
    {

        Filter::Filter()
        {
            construct();
        }

        Filter::~Filter()
        {
            destroy();
        }

        void Filter::construct()
        {
            pBank               = NULL;
            sParams.nType       = FLT_NONE;
            sParams.fFreq       = 0;
            sParams.fFreq2      = 0;
            sParams.fGain       = 0.0f;
            sParams.nSlope      = 0;
            sParams.fQuality    = 0.0f;

            nSampleRate         = 0;
            nMode               = FM_BYPASS;
            nLatency            = 0;
            nItems              = 0;

            vItems              = NULL;
            vData               = NULL;
            nFlags              = FF_REBUILD | FF_CLEAR;
        }

        bool Filter::init(FilterBank *fb)
        {
            filter_params_t fp;

            fp.nType            = FLT_NONE;
            fp.fFreq            = 1000.0f;
            fp.fFreq2           = 1000.0f;
            fp.fGain            = 1.0f;
            fp.fQuality         = 0;
            fp.nSlope           = 1;

            if (fb != NULL)
                pBank           = fb;
            else
            {
                // Try to allocate filter bank
                pBank           = new FilterBank();
                if (pBank == NULL)
                    return false;

                // Update flags that we hawe own filter bank
                nFlags         |= FF_OWN_BANK;

                // Try to initialize own filter bank
                if (!pBank->init(FILTER_CHAINS_MAX))
                    return false;
            }

            if (vData == NULL)
            {
                size_t cascade_size = align_size(sizeof(dsp::f_cascade_t) * FILTER_CHAINS_MAX, DEFAULT_ALIGN);

                size_t allocate     = cascade_size + DEFAULT_ALIGN; // + filters_size;
                vData               = new uint8_t[allocate];
                if (vData == NULL)
                    return false;

                uint8_t *ptr        = align_ptr(vData, DEFAULT_ALIGN);
                vItems              = reinterpret_cast<dsp::f_cascade_t *>(ptr);
                ptr                += cascade_size;
            }

            update(48000, &fp);
            nFlags             |= FF_REBUILD | FF_CLEAR;

            return true;
        }

        void Filter::destroy()
        {
            if (vData != NULL)
            {
                delete  [] vData;
                vItems      = NULL;
                vData       = NULL;
            }

            if (pBank != NULL)
            {
                // Destroy filter bank if it's our own filter bank
                if (nFlags & FF_OWN_BANK)
                {
                    pBank->destroy();
                    delete pBank;
                }

                pBank       = NULL;
            }

            nFlags      = 0;
        }

        void Filter::update(size_t sr, const filter_params_t *params)
        {
            // Init direct filter chain
            filter_params_t *fp     = &sParams;
            size_t type             = fp->nType;
            size_t slope            = fp->nSlope;

            // Reset parameters
            nSampleRate             = sr;
            nMode                   = FM_BYPASS;
            nLatency                = 0;

            // Copy and limit parameters
            *fp                     = *params;
            limit(sr, fp);
            nFlags                 |= FF_REBUILD;
            if ((type != fp->nType) || (slope != fp->nSlope))
                nFlags                 |= FF_CLEAR;
        }

        void Filter::limit(size_t sr, filter_params_t *fp)
        {
            float max_freq  = 0.49f * nSampleRate;
            fp->nSlope      = lsp_limit(fp->nSlope, 1U, FILTER_CHAINS_MAX);
            fp->fFreq       = lsp_limit(fp->fFreq, 0.0f, max_freq);
            fp->fFreq2      = lsp_limit(fp->fFreq2, 0.0f, max_freq);
        }

        void Filter::set_sample_rate(size_t sr)
        {
            update(sr, &sParams);
        }

        void Filter::get_params(filter_params_t *params)
        {
            if (params != NULL)
                *params =    sParams;
        }

        dsp::f_cascade_t *Filter::add_cascade()
        {
            // Get cascade
            dsp::f_cascade_t *c = (nItems >= FILTER_CHAINS_MAX) ?
                &vItems[FILTER_CHAINS_MAX-1] :
                &vItems[nItems];

            // Increment number of chains
            if (nItems < FILTER_CHAINS_MAX)
                nItems++;

            // Initialize cascade
            for (size_t i=0; i<4; ++i)
            {
                c->t[i] = 0.0;
                c->b[i] = 0.0;
            }

            // Return cascade
            return c;
        }

        float Filter::bilinear_relative(float f1, float f2)
        {
            float nf    = M_PI / float(nSampleRate);
            return tanf(f1 * nf) / tanf(f2 * nf);
        }

        void Filter::rebuild()
        {
            // Clear bank if it is internal bank
            if (nFlags & FF_OWN_BANK)
                pBank->begin();

            // Reset number of cascades
            nItems                  = 0;

            filter_params_t fp      = sParams;

            // Calculate filter
            switch (sParams.nType)
            {
                case FLT_BT_AMPLIFIER:
                case FLT_BT_RLC_LOPASS:
                case FLT_BT_RLC_HIPASS:
                case FLT_BT_RLC_LOSHELF:
                case FLT_BT_RLC_HISHELF:
                case FLT_BT_RLC_BELL:
                case FLT_BT_RLC_RESONANCE:
                case FLT_BT_RLC_NOTCH:
                case FLT_BT_RLC_ALLPASS:
                case FLT_BT_RLC_ALLPASS2:
                case FLT_BT_RLC_LADDERPASS:
                case FLT_BT_RLC_LADDERREJ:
                case FLT_BT_RLC_BANDPASS:
                case FLT_BT_RLC_ENVELOPE:
                {
                    // Calculate filter parameters
                    fp.fFreq2           = bilinear_relative(fp.fFreq, fp.fFreq2);    // Normalize frequency
                    calc_rlc_filter(sParams.nType, &fp);
                    nMode               = FM_BILINEAR;
                    break;
                }

                case FLT_MT_AMPLIFIER:
                case FLT_MT_RLC_LOPASS:
                case FLT_MT_RLC_HIPASS:
                case FLT_MT_RLC_LOSHELF:
                case FLT_MT_RLC_HISHELF:
                case FLT_MT_RLC_BELL:
                case FLT_MT_RLC_RESONANCE:
                case FLT_MT_RLC_NOTCH:
                case FLT_MT_RLC_ALLPASS:
                case FLT_MT_RLC_ALLPASS2:
                case FLT_MT_RLC_LADDERPASS:
                case FLT_MT_RLC_LADDERREJ:
                case FLT_MT_RLC_BANDPASS:
                case FLT_MT_RLC_ENVELOPE:
                {
                    // Calculate filter parameters
                    fp.fFreq2           = fp.fFreq / fp.fFreq2;    // Normalize frequency
                    calc_rlc_filter(sParams.nType - 1, &fp);
                    nMode               = FM_MATCHED;
                    break;
                }

                case FLT_BT_BWC_LOPASS:
                case FLT_BT_BWC_HIPASS:
                case FLT_BT_BWC_LOSHELF:
                case FLT_BT_BWC_HISHELF:
                case FLT_BT_BWC_BELL:
                case FLT_BT_BWC_LADDERPASS:
                case FLT_BT_BWC_LADDERREJ:
                case FLT_BT_BWC_BANDPASS:
                case FLT_BT_BWC_ALLPASS:
                {
                    // Calculate filter parameters
                    fp.fFreq2           = bilinear_relative(fp.fFreq, fp.fFreq2);    // Normalize frequency
                    calc_bwc_filter(sParams.nType, &fp);
                    nMode               = FM_BILINEAR;
                    break;
                }

                case FLT_MT_BWC_LOPASS:
                case FLT_MT_BWC_HIPASS:
                case FLT_MT_BWC_LOSHELF:
                case FLT_MT_BWC_HISHELF:
                case FLT_MT_BWC_BELL:
                case FLT_MT_BWC_LADDERPASS:
                case FLT_MT_BWC_LADDERREJ:
                case FLT_MT_BWC_BANDPASS:
                case FLT_MT_BWC_ALLPASS:
                {
                    // Calculate filter parameters
                    fp.fFreq2           = fp.fFreq / fp.fFreq2;    // Normalize frequency
                    calc_bwc_filter(sParams.nType - 1, &fp);
                    nMode               = FM_MATCHED;
                    break;
                }

                case FLT_BT_LRX_LOPASS:
                case FLT_BT_LRX_HIPASS:
                case FLT_BT_LRX_LOSHELF:
                case FLT_BT_LRX_HISHELF:
                case FLT_BT_LRX_BELL:
                case FLT_BT_LRX_LADDERPASS:
                case FLT_BT_LRX_LADDERREJ:
                case FLT_BT_LRX_BANDPASS:
                case FLT_BT_LRX_ALLPASS:
                {
                    // Calculate filter parameters
                    fp.fFreq2           = bilinear_relative(fp.fFreq, fp.fFreq2);    // Normalize frequency
                    calc_lrx_filter(sParams.nType, &fp);
                    nMode               = FM_BILINEAR;
                    break;
                }

                case FLT_MT_LRX_LOPASS:
                case FLT_MT_LRX_HIPASS:
                case FLT_MT_LRX_LOSHELF:
                case FLT_MT_LRX_HISHELF:
                case FLT_MT_LRX_BELL:
                case FLT_MT_LRX_LADDERPASS:
                case FLT_MT_LRX_LADDERREJ:
                case FLT_MT_LRX_BANDPASS:
                case FLT_MT_LRX_ALLPASS:
                {
                    // Calculate filter parameters
                    fp.fFreq2           = fp.fFreq / fp.fFreq2;    // Normalize frequency
                    calc_lrx_filter(sParams.nType - 1, &fp);
                    nMode               = FM_MATCHED;
                    break;
                }

                case FLT_DR_APO_LOPASS:
                case FLT_DR_APO_HIPASS:
                case FLT_DR_APO_BANDPASS:
                case FLT_DR_APO_NOTCH:
                case FLT_DR_APO_ALLPASS:
                case FLT_DR_APO_PEAKING:
                case FLT_DR_APO_LOSHELF:
                case FLT_DR_APO_HISHELF:
                {
                    calc_apo_filter(sParams.nType, &fp);
                    nMode               = FM_APO;
                    break;
                }

                case FLT_DR_APO_ALLPASS2:
                {
                    calc_apo_filter(FLT_DR_APO_ALLPASS, &fp);
                    fp.fFreq            = sParams.fFreq2;
                    fp.fGain            = 1.0f;
                    calc_apo_filter(FLT_DR_APO_ALLPASS, &fp);
                    nMode               = FM_APO;
                    break;
                }

                case FLT_DR_APO_LADDERPASS:
                {
                    calc_apo_filter(FLT_DR_APO_HISHELF, &fp);
                    fp.fFreq            = sParams.fFreq2;
                    fp.fGain            = 1.0f / sParams.fGain;
                    calc_apo_filter(FLT_DR_APO_HISHELF, &fp);
                    nMode               = FM_APO;
                    break;
                }

                case FLT_DR_APO_LADDERREJ:
                {
                    calc_apo_filter(FLT_DR_APO_LOSHELF, &fp);
                    fp.fFreq            = sParams.fFreq2;
                    calc_apo_filter(FLT_DR_APO_HISHELF, &fp);
                    nMode               = FM_APO;
                    break;
                }

                case FLT_A_WEIGHTED:
                case FLT_B_WEIGHTED:
                case FLT_C_WEIGHTED:
                case FLT_D_WEIGHTED:
                case FLT_K_WEIGHTED:
                {
                    calc_weighted_filter(sParams.nType, &fp);
                    break;
                }

                case FLT_NONE:
                default:
                    nMode               = FM_BYPASS;
                    break;
            }

            if (nMode == FM_BILINEAR)
                bilinear_transform();
            else if (nMode == FM_MATCHED)
                matched_transform();

            // Complete bank if it is internal bank
            if (nFlags & FF_OWN_BANK)
                pBank->end(nFlags & FF_CLEAR);

            nFlags     &= FF_OWN_BANK; // Clear all flags except FF_OWN_BANK
        }

        void Filter::apo_complex_transfer_calc_ri(float *re, float *im, const float *f, size_t count)
        {
            for (size_t i=0; i<count; ++i)
            {
                // Auxiliary variables:
                float cw    = f[0];
                float sw    = f[1];

                // These equations are valid since sw has valid sign
                float c2w   = cw * cw - sw * sw;    // cos(2 * w)
                float s2w   = 2.0 * sw * cw;        // sin(2 * w)

                // Apo will be just one biquad, but let's write this to be able to calculate any digital biquads cascade.
                float r_re  = 1.0f, r_im = 0.0f;    // The result complex number
                float b_re, b_im;                   // Temporary values for computing complex multiplication

                for (size_t i=0; i<nItems; ++i)
                {
                    dsp::f_cascade_t *c  = &vItems[i];

                    float alpha     = c->t[0] + c->t[1] * cw + c->t[2] * c2w;
                    float beta      = c->t[1] * sw + c->t[2] * s2w;
                    float gamma     = c->b[0] + c->b[1] * cw + c->b[2] * c2w;
                    float delta     = c->b[1] * sw + c->b[2] * s2w;

                    float mag       = 1.0 / (gamma * gamma + delta * delta);

                    // Compute current biquad's tranfer function
                    float w_re      = mag * (alpha * gamma - beta * delta);
                    float w_im      = mag * (alpha * delta + beta * gamma);

                    // Compute common transfer function as a product between current biquad's
                    // transfer function and previous value
                    b_re            = r_re*w_re - r_im*w_im;
                    b_im            = r_re*w_im + r_im*w_re;

                    // Commit changes to the result complex number
                    r_re            = b_re;
                    r_im            = b_im;
                }

                re[i]  = r_re;
                im[i]  = r_im;
                f     += 2;
            }
        }

        void Filter::apo_complex_transfer_calc_pc(float *ri, const float *f, size_t count)
        {
            for (size_t i=0; i<count; ++i)
            {
                // Auxiliary variables:
                float cw    = f[0];
                float sw    = f[1];

                // These equations are valid since sw has valid sign
                float c2w   = cw * cw - sw * sw;    // cos(2 * w)
                float s2w   = 2.0 * sw * cw;        // sin(2 * w)

                // Apo will be just one biquad, but let's write this to be able to calculate any digital biquads cascade.
                float r_re  = 1.0f, r_im = 0.0f;    // The result complex number
                float b_re, b_im;                   // Temporary values for computing complex multiplication

                for (size_t i=0; i<nItems; ++i)
                {
                    dsp::f_cascade_t *c  = &vItems[i];

                    float alpha     = c->t[0] + c->t[1] * cw + c->t[2] * c2w;
                    float beta      = c->t[1] * sw + c->t[2] * s2w;
                    float gamma     = c->b[0] + c->b[1] * cw + c->b[2] * c2w;
                    float delta     = c->b[1] * sw + c->b[2] * s2w;

                    float mag       = 1.0 / (gamma * gamma + delta * delta);

                    // Compute current biquad's tranfer function
                    float w_re      = mag * (alpha * gamma - beta * delta);
                    float w_im      = mag * (alpha * delta + beta * gamma);

                    // Compute common transfer function as a product between current biquad's
                    // transfer function and previous value
                    b_re            = r_re*w_re - r_im*w_im;
                    b_im            = r_re*w_im + r_im*w_re;

                    // Commit changes to the result complex number
                    r_re            = b_re;
                    r_im            = b_im;
                }

                ri[0]  = r_re;
                ri[1]  = r_im;
                f     += 2;
                ri    += 2;
            }
        }

        void Filter::freq_chart(float *re, float *im, const float *f, size_t count)
        {
            // Temporary buffer to store updated frequency
            float freqs[STACK_BUF_SIZE] __lsp_aligned32;
            filter_mode_t mode = (nItems > 0) ? nMode : FM_BYPASS;

            // Calculate frequency chart
            switch (mode)
            {
                case FM_BILINEAR:
                {
                    float nf    = M_PI / float(nSampleRate);
                    float kf    = 1.0/tanf(sParams.fFreq * nf);
                    float lf    = nSampleRate * 0.499;

                    while (count > 0)
                    {
                        size_t to_do    = lsp_min(count, STACK_BUF_SIZE);

                        // Compute transfer function
                        for (size_t i=0; i<to_do; ++i)
                        {
                            float w     = f[i];
                            freqs[i]    = tanf((w > lf ? lf : w) * nf) * kf;
                        }
                        dsp::filter_transfer_calc_ri(re, im, &vItems[0], freqs, to_do);
                        for (size_t i=1; i<nItems; ++i)
                            dsp::filter_transfer_apply_ri(re, im, &vItems[i], freqs, to_do);

                        // Update pointers
                        re         += to_do;
                        im         += to_do;
                        f          += to_do;
                        count      -= to_do;
                    }
                    break;
                }

                case FM_MATCHED:
                {
                    float kf    = 1.0 / sParams.fFreq;

                    while (count > 0)
                    {
                        size_t to_do    = lsp_min(count, STACK_BUF_SIZE);

                        // Compute transfer function
                        dsp::mul_k3(freqs, f, kf, to_do);
                        dsp::filter_transfer_calc_ri(re, im, &vItems[0], freqs, to_do);
                        for (size_t i=1; i<nItems; ++i)
                            dsp::filter_transfer_apply_ri(re, im, &vItems[i], freqs, to_do);

                        // Update pointers
                        re         += to_do;
                        im         += to_do;
                        f          += to_do;
                        count      -= to_do;
                    }
                    break;
                }

                case FM_APO:
                {
                    // Calculating normalized frequency, wrapped for maximal accuracy:
                    float kf    = 2.0 * M_PI / float(nSampleRate);
                    float lf    = nSampleRate * 0.5f;

                    while (count > 0)
                    {
                        size_t to_do    = lsp_min(count, STACK_BUF_SIZE >> 1);

                        // Compute transfer function
                        float *df       = freqs;
                        for (size_t i=0; i<to_do; ++i, df += 2)
                        {
                            float w     = f[i];
                            float f     = lsp_min(w, lf) * kf;
                            df[0]       = cosf(f);
                            df[1]       = sinf(f);
                        }

                        apo_complex_transfer_calc_ri(re, im, freqs, to_do);

                        // Update pointers
                        re         += to_do;
                        im         += to_do;
                        f          += to_do;
                        count      -= to_do;
                    }
                    break;
                }

                case FM_BYPASS:
                default:
                    dsp::fill_one(re, count);
                    dsp::fill_zero(im, count);
                    return;
            }
        }

        void Filter::freq_chart(float *c, const float *f, size_t count)
        {
            // Temporary buffer to store updated frequency
            float freqs[STACK_BUF_SIZE] __lsp_aligned32;
            filter_mode_t mode = (nItems > 0) ? nMode : FM_BYPASS;

            // Calculate frequency chart
            switch (mode)
            {
                case FM_BILINEAR:
                {
                    float nf    = M_PI / float(nSampleRate);
                    float kf    = 1.0/tanf(sParams.fFreq * nf);
                    float lf    = nSampleRate * 0.499;

                    while (count > 0)
                    {
                        size_t to_do    = lsp_min(count, STACK_BUF_SIZE);

                        // Compute transfer function
                        for (size_t i=0; i<to_do; ++i)
                        {
                            float w         = f[i];
                            freqs[i]        = tanf((w > lf ? lf : w) * nf) * kf;
                        }
                        dsp::filter_transfer_calc_pc(c, &vItems[0], freqs, to_do);
                        for (size_t i=1; i<nItems; ++i)
                            dsp::filter_transfer_apply_pc(c, &vItems[i], freqs, to_do);

                        // Update pointers
                        c          += to_do*2;
                        f          += to_do;
                        count      -= to_do;
                    }

                    break;
                }

                case FM_MATCHED:
                {
                    float kf    = 1.0 / sParams.fFreq;

                    while (count > 0)
                    {
                        size_t to_do    = lsp_min(count, STACK_BUF_SIZE);

                        // Compute transfer function
                        dsp::mul_k3(freqs, f, kf, to_do);
                        dsp::filter_transfer_calc_pc(c, &vItems[0], freqs, to_do);
                        for (size_t i=1; i<nItems; ++i)
                            dsp::filter_transfer_apply_pc(c, &vItems[i], freqs, to_do);

                        // Update pointers
                        c          += to_do*2;
                        f          += to_do;
                        count      -= to_do;
                    }

                    break;
                }

                case FM_APO:
                {
                    // Calculating normalized frequency, wrapped for maximal accuracy:
                    float kf    = 2.0 * M_PI / float(nSampleRate);
                    float lf    = nSampleRate * 0.5f;

                    while (count > 0)
                    {
                        size_t to_do    = lsp_min(count, STACK_BUF_SIZE >> 1);

                        // Compute transfer function
                        float *df       = freqs;
                        for (size_t i=0; i<to_do; ++i, df += 2)
                        {
                            float w     = f[i];
                            float f     = lsp_min(w, lf) * kf;
                            df[0]       = cosf(f);
                            df[1]       = sinf(f);
                        }

                        apo_complex_transfer_calc_pc(c, freqs, to_do);

                        // Update pointers
                        c          += to_do * 2;
                        f          += to_do;
                        count      -= to_do;
                    }
                    break;
                }

                case FM_BYPASS:
                default:
                    dsp::pcomplex_fill_ri(c, 1.0f, 0.0f, count);
                    return;
            }
        }

        void Filter::process(float *out, const float *in, size_t samples)
        {
            // Check whether we need to rebuild filter
            if (nFlags & (~FF_OWN_BANK))
                rebuild();

            switch (nMode)
            {
                case FM_BILINEAR:
                case FM_MATCHED:
                case FM_APO:
                {
                    pBank->process(out, in, samples);
                    break;
                }

                default:
                {
                    dsp::copy(out, in, samples);
                    break;
                }
            }
        }

        void Filter::calc_rlc_filter(size_t type, const filter_params_t *fp)
        {
            dsp::f_cascade_t *c         = NULL;
            nMode                       = FM_BILINEAR;

            switch (type)
            {
                case FLT_BT_AMPLIFIER:
                {
                    c           = add_cascade();
                    c->t[0]     = fp->fGain;
                    c->t[1]     = 0.0f;
                    c->t[2]     = 0.0f;

                    c->b[0]     = 1.0f;
                    c->b[1]     = 0.0f;
                    c->b[2]     = 0.0f;

                    break;
                }

                case FLT_BT_RLC_LOPASS:
                case FLT_BT_RLC_HIPASS:
                {
                    // Add cascade with one pole
                    float k         = 2.0 / (1.0 + fp->fQuality);
                    size_t i        = fp->nSlope & 1;
                    if (i)
                    {
                        c           = add_cascade();
                        c->b[0]     = 1.0;
                        c->b[1]     = 1.0;

                        if (type == FLT_BT_RLC_LOPASS)
                            c->t[0]     = fp->fGain;
                        else
                            c->t[1]     = fp->fGain;
                    }

                    // Add additional cascades
                    for (size_t j=i; j < fp->nSlope; j+=2)
                    {
                        c           = add_cascade();
                        c->b[0]     = 1.0;
                        c->b[1]     = k;
                        c->b[2]     = 1.0;

                        if (type == FLT_BT_RLC_LOPASS)
                            c->t[0]     = (j == 0) ? fp->fGain : 1.0;
                        else
                            c->t[2]     = (j == 0) ? fp->fGain : 1.0;
                    }

                    break;
                }

                case FLT_BT_RLC_LOSHELF:
                case FLT_BT_RLC_HISHELF:
                {
                    size_t slope            = fp->nSlope * 2;
                    float gain              = sqrtf(fp->fGain);
                    float fg                = expf(logf(gain)/slope);

                    for (size_t j=0; j < fp->nSlope; j++)
                    {
                        c                       = add_cascade();
                        float *t                = (type == FLT_BT_RLC_LOSHELF) ? c->t : c->b;
                        float *b                = (type == FLT_BT_RLC_LOSHELF) ? c->b : c->t;

                        // Create transfer function
                        t[0]                    = fg;
                        t[1]                    = 2.0 / (1.0 + fp->fQuality);
                        t[2]                    = 1.0 / fg;

                        b[0]                    = 1.0 / fg;
                        b[1]                    = 2.0 / (1.0 + fp->fQuality);
                        b[2]                    = fg;

                        if (j == 0)
                        {
                            c->t[0]                *= gain;
                            c->t[1]                *= gain;
                            c->t[2]                *= gain;
                        }
                    }

                    break;
                }

                case FLT_BT_RLC_LADDERPASS:
                case FLT_BT_RLC_LADDERREJ:
                {
                    size_t slope            = fp->nSlope * 2;
                    float gain1             = (type == FLT_BT_RLC_LADDERREJ) ? sqrtf(1.0/fp->fGain) : sqrtf(fp->fGain);
                    float gain2             = (type == FLT_BT_RLC_LADDERREJ) ? sqrtf(fp->fGain) : sqrtf(1.0/fp->fGain);

                    float fg1               = expf(logf(gain1)/slope);
                    float fg2               = expf(logf(gain2)/slope);
                    float kf                = fp->fFreq2;

                    for (size_t j=0; j < fp->nSlope; j++)
                    {
                        // First shelf cascade, lo-shelf for LADDERREJ, hi-shelf for LADDERPASS
                        c                       = add_cascade();
                        float *t                = (type == FLT_BT_RLC_LADDERREJ) ? c->t : c->b;
                        float *b                = (type == FLT_BT_RLC_LADDERREJ) ? c->b : c->t;
                        float fg                = (type == FLT_BT_RLC_LADDERREJ) ? fg2 : fg1;
                        float gain              = (type == FLT_BT_RLC_LADDERREJ) ? gain2 : gain1;

                        // Create transfer function
                        t[0]                    = fg;
                        t[1]                    = 2.0 / (1.0 + fp->fQuality);
                        t[2]                    = 1.0 / fg;

                        b[0]                    = 1.0 / fg;
                        b[1]                    = 2.0 / (1.0 + fp->fQuality);
                        b[2]                    = fg;

                        if (j == 0)
                        {
                            c->t[0]                *= gain;
                            c->t[1]                *= gain;
                            c->t[2]                *= gain;
                        }

                        // Second shelf cascade, hi-shelf always
                        c                       = add_cascade();
                        t                       = c->b;
                        b                       = c->t;

                        // Create transfer function
                        t[0]                    = fg2;
                        t[1]                    = 2.0 * kf / (1.0 + fp->fQuality);
                        t[2]                    = kf*kf / fg2;

                        b[0]                    = 1.0 / fg2;
                        b[1]                    = 2.0 * kf / (1.0 + fp->fQuality);
                        b[2]                    = fg2 * kf * kf;

                        if (j == 0)
                        {
                            c->t[0]                *= gain2;
                            c->t[1]                *= gain2;
                            c->t[2]                *= gain2;
                        }
                    }

                    break;
                }

                case FLT_BT_RLC_BANDPASS:
                {
                    // Add cascade with one pole
                    float kf        = fp->fFreq2;
                    float kf2       = kf * kf;
                    float k         = 2.0f / (1.0f + fp->fQuality);
                    size_t i        = fp->nSlope & 1;
                    if (i)
                    {
                        // lo-pass + hi-pass cascades
                        c           = add_cascade();
                        c->t[1]     = fp->fGain * fp->fGain;
                        c->b[0]     = 1.0f;
                        c->b[1]     = 1.0f + kf;
                        c->b[2]     = kf;
                    }

                    // Add additional cascades
                    for (size_t j=i; j < fp->nSlope; j+=2)
                    {
                        // Lo-pass cascade
                        c           = add_cascade();
                        c->b[0]     = 1.0f;
                        c->b[1]     = k;
                        c->b[2]     = 1.0f;
                        c->t[0]     = (j == 0) ? fp->fGain : 1.0f;

                        // Hi-pass cascade
                        c           = add_cascade();
                        c->b[0]     = 1.0f;
                        c->b[1]     = k * kf;
                        c->b[2]     = kf2;
                        c->t[2]     = (j == 0) ? fp->fGain : 1.0f;
                    }

                    break;
                }

                case FLT_BT_RLC_BELL:
                {
                    float fg                = expf(logf(fp->fGain)/fp->nSlope);
                    float angle             = atanf(fg);
                    float k                 = 2.0 * (1.0/fg + fg) / (1.0 + (2.0 * fp->fQuality) / fp->nSlope);
                    float kt                = k * sinf(angle);
                    float kb                = k * cosf(angle);

                    for (size_t j=0; j < fp->nSlope; j++)
                    {
                        c                       = add_cascade();

                        // Create transfer function
                        c->t[0]                 = 1.0;
                        c->t[1]                 = kt;
                        c->t[2]                 = 1.0;

                        c->b[0]                 = 1.0;
                        c->b[1]                 = kb;
                        c->b[2]                 = 1.0;
                    }

                    break;
                }

                // Resonance filter
                case FLT_BT_RLC_RESONANCE:
                {
                    float angle             = atanf(expf(logf(fp->fGain) / fp->nSlope));
                    float k                 = 2.0 / (1.0 + fp->fQuality);
                    float kt                = k * sinf(angle);
                    float kb                = k * cosf(angle);

                    for (size_t j=0; j < fp->nSlope; j++)
                    {
                        c                       = add_cascade();

                        // Create transfer function
                        c->t[0]                 = 1.0;
                        c->t[1]                 = kt;
                        c->t[2]                 = 1.0;

                        c->b[0]                 = 1.0;
                        c->b[1]                 = kb;
                        c->b[2]                 = 1.0;
                    }

                    break;
                }

                case FLT_BT_RLC_NOTCH:
                {
                    c                       = add_cascade();

                    // Create transfer function
                    c->t[0]                 = fp->fGain;
                    c->t[1]                 = 0;
                    c->t[2]                 = fp->fGain;

                    // Bottom polynom
                    c->b[0]                 = 1.0;
                    c->b[1]                 = 2.0 / (1.0 + fp->fQuality);
                    c->b[2]                 = 1.0;

                    break;
                }

                case FLT_BT_RLC_ALLPASS:
                {
                    // Single all-pass filter
                    size_t i        = fp->nSlope & 1;
                    if (i)
                    {
                        c                       = add_cascade();
                        c->t[0]                 = -1.0;
                        c->t[1]                 = 1.0;
                        c->t[2]                 = 0.0;

                        c->b[0]                 = 1.0;
                        c->b[1]                 = 1.0;
                        c->b[2]                 = 0.0;
                    }

                    // 2x all-pass filters in one cascade
                    for (size_t j=i; j < fp->nSlope; j += 2)
                    {
                        c                       = add_cascade();
                        // Create transfer function
                        c->t[0]                 = 1.0;
                        c->t[1]                 = -2.0;
                        c->t[2]                 = 1.0;

                        // Bottom polynom
                        c->b[0]                 = 1.0;
                        c->b[1]                 = 2.0;
                        c->b[2]                 = 1.0;
                    }

                    // Adjust gain for the last cascade
                    if (c != NULL)
                    {
                        c->t[0]    *= fp->fGain;
                        c->t[1]    *= fp->fGain;
                        c->t[2]    *= fp->fGain;
                    }

                    break;
                }

                case FLT_BT_RLC_ALLPASS2:
                {
                    float kf                = fp->fFreq2;
                    float kfp1              = 1.0 + kf;

                    // 2x all-pass filters in one cascade
                    for (size_t j=0; j < fp->nSlope; j++)
                    {
                        c                       = add_cascade();
                        // Create transfer function
                        c->t[0]                 = 1.0;
                        c->t[1]                 = -kfp1;
                        c->t[2]                 = kf;

                        // Bottom polynom
                        c->b[0]                 = 1.0;
                        c->b[1]                 = kfp1;
                        c->b[2]                 = kf;
                    }

                    // Adjust gain for the last cascade
                    if (c != NULL)
                    {
                        c->t[0]    *= fp->fGain;
                        c->t[1]    *= fp->fGain;
                        c->t[2]    *= fp->fGain;
                    }

                    break;
                }

                case FLT_BT_RLC_ENVELOPE:
                {
                    size_t slope    = fp->nSlope;
                    size_t cj       = 0;
                    if (slope & 1)
                    {
                        float k                 = 1.0f;

                        for (size_t i=0; i<3; ++i)
                        {
                            c               = add_cascade();
                            c->t[0]         = 1.0f;
                            c->t[1]         = (1.0f + 0.25f)*k;
                            c->t[2]         = 0.25f * k*k;

                            c->b[0]         = 1.0f;
                            c->b[1]         = (0.5f + 0.125f)*k;
                            c->b[2]         = 0.5f * 0.125f * k*k;

                            k              *= 0.0625f;
                            if (!(cj++))
                            {
                                c->t[0]        *= fp->fGain;
                                c->t[1]        *= fp->fGain;
                                c->t[2]        *= fp->fGain;
                            }
                        }
                    }
                    slope >>= 1;

                    for (size_t j=0; j < slope; j++)
                    {
                        c                       = add_cascade();
                        c->t[0]                 = (cj == 0) ? fp->fGain : 1.0f;
                        c->t[1]                 = (cj == 0) ? fp->fGain : 1.0f;
                        c->b[0]                 = 1.0f;
                        c->b[1]                 = 0.0005f;
                        cj ++;
                    }
                    break;
                }

                default:
                    nMode           = FM_BYPASS;
                    break;
            }
        }

        void Filter::calc_bwc_filter(size_t type, const filter_params_t *fp)
        {
            dsp::f_cascade_t *c     = NULL;

            switch (type)
            {
                case FLT_BT_BWC_LOPASS:
                case FLT_BT_BWC_HIPASS:
                {
                    float k     = 1.0f / (1.0f + fp->fQuality);
                    size_t i    = fp->nSlope & 1;
                    if (i)
                    {
                        c           = add_cascade();
                        c->b[0]     = 1.0;
                        c->b[1]     = 1.0;

                        if (type == FLT_BT_BWC_LOPASS)
                            c->t[0]     = fp->fGain;
                        else
                            c->t[1]     = fp->fGain;
                    }

                    for (size_t j=i; j < fp->nSlope; j += 2)
                    {
                        float theta     = ((j - i + 1)*M_PI_2)/fp->nSlope;
                        float tsin      = sinf(theta);
                        float tcos      = sqrtf(1.0 - tsin*tsin);
                        float kf        = tsin*tsin + k*k * tcos*tcos;

                        c               = add_cascade();

                        // Tranfer function
                        if (type == FLT_BT_BWC_HIPASS)
                        {
                            c->t[2]         = (j == 0) ? fp->fGain : 1.0;

                            c->b[0]         = 1.0 / kf;
                            c->b[1]         = 2.0 * k * tcos / kf;
                            c->b[2]         = 1.0;
                        }
                        else
                        {
                            c->t[0]         = (j == 0) ? fp->fGain : 1.0;

                            c->b[0]         = 1.0;
                            c->b[1]         = 2.0 * k * tcos / kf;
                            c->b[2]         = 1.0 / kf;
                        }
                    }

                    break;
                }

                case FLT_BT_BWC_ALLPASS:
                {
                    float k     = 1.0f / (1.0f + fp->fQuality);
                    size_t i    = fp->nSlope & 1;
                    if (i)
                    {
                        c           = add_cascade();
                        c->t[0]     = -fp->fGain;
                        c->t[1]     = fp->fGain;
                        c->t[2]     = 0.0;

                        c->b[0]     = 1.0;
                        c->b[1]     = 1.0;
                        c->b[2]     = 0.0;
                    }

                    for (size_t j=i; j < fp->nSlope; j += 2)
                    {
                        float theta     = ((j - i + 1)*M_PI_2)/fp->nSlope;
                        float tsin      = sinf(theta);
                        float tcos      = sqrtf(1.0 - tsin*tsin);
                        float kf        = tsin*tsin + k*k * tcos*tcos;

                        c               = add_cascade();

                        // Tranfer function
                        c->t[0]         = 1.0;
                        c->t[1]         = -2.0 * tcos;
                        c->t[2]         = 1.0;

                        c->b[0]         = 1.0 / kf;
                        c->b[1]         = 2.0 * k * tcos / kf;
                        c->b[2]         = 1.0;

                        if (j == 0)
                        {
                            c->t[0]        *= fp->fGain;
                            c->t[1]        *= fp->fGain;
                            c->t[2]        *= fp->fGain;
                        }
                    }

                    break;
                }

                case FLT_BT_BWC_HISHELF:
                case FLT_BT_BWC_LOSHELF:
                {
                    float gain              = sqrtf(fp->fGain);
                    float fg                = expf(logf(gain)/(2.0*fp->nSlope));
                    float k                 = 1.0f / (1.0 + fp->fQuality * (1.0 - expf(2.0 - gain - 1.0/gain)));

                    for (size_t j=0; j < fp->nSlope; ++j)
                    {
                        float theta         = ((2*j + 1)*M_PI_2)/(2*fp->nSlope);
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0 - tsin*tsin);
                        float kf            = tsin*tsin + k*k * tcos*tcos;

                        c                   = add_cascade();
                        float *t            = (type == FLT_BT_BWC_HISHELF) ? c->t : c->b;
                        float *b            = (type == FLT_BT_BWC_HISHELF) ? c->b : c->t;

                        // Transfer function
                        t[0]                = kf / fg;
                        t[1]                = 2.0 * k * tcos;
                        t[2]                = fg;

                        b[0]                = fg;
                        b[1]                = 2.0 * k * tcos;
                        b[2]                = kf / fg;

                        if (j == 0)
                        {
                            c->t[0]            *= gain;
                            c->t[1]            *= gain;
                            c->t[2]            *= gain;
                        }
                    }

                    break;
                }

                case FLT_BT_BWC_LADDERPASS:
                case FLT_BT_BWC_LADDERREJ:
                {
                    size_t slope            = fp->nSlope * 2;
                    float gain1             = (type == FLT_BT_BWC_LADDERPASS) ? sqrtf(fp->fGain) : sqrtf(1.0/fp->fGain);
                    float gain2             = (type == FLT_BT_BWC_LADDERPASS) ? sqrtf(1.0/fp->fGain) : sqrtf(fp->fGain);

                    float fg1               = expf(logf(gain1)/(2.0*fp->nSlope));
                    float fg2               = expf(logf(gain2)/(2.0*fp->nSlope));
                    float k1                = 1.0f / (1.0f + fp->fQuality * (1.0f - expf(2.0f - gain1 - 1.0f/gain1)));
                    float k2                = 1.0f / (1.0f + fp->fQuality * (1.0f - expf(2.0f - gain2 - 1.0f/gain2)));
                    float xf                = fp->fFreq2;
                    float xf2               = xf * xf;

                    for (size_t j=0; j < fp->nSlope; ++j)
                    {
                        float theta         = ((2*j + 1)*M_PI_2)/float(slope);
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0f - tsin*tsin);

                        // First shelf cascade, lo-shelf for LADDERREJ, hi-shelf for LADDERPASS
                        float k             = (type == FLT_BT_BWC_LADDERPASS) ? k1 : k2;
                        float fg            = (type == FLT_BT_BWC_LADDERPASS) ? fg1 : fg2;
                        float gain          = (type == FLT_BT_BWC_LADDERPASS) ? gain1 : gain2;
                        float kf            = tsin*tsin + k*k * tcos*tcos;
                        c                   = add_cascade();
                        float *t            = (type == FLT_BT_BWC_LADDERPASS) ? c->t : c->b;
                        float *b            = (type == FLT_BT_BWC_LADDERPASS) ? c->b : c->t;

                        // Transfer function
                        t[0]                = kf / fg;
                        t[1]                = 2.0f * k * tcos;
                        t[2]                = fg;

                        b[0]                = fg;
                        b[1]                = t[1]; // 2.0 * k * tcos;
                        b[2]                = t[0]; // kf / fg;

                        if (j == 0)
                        {
                            c->t[0]            *= gain;
                            c->t[1]            *= gain;
                            c->t[2]            *= gain;
                        }

                        // Second shelf cascade, always hi-shelf
                        kf                  = tsin*tsin + k1*k1 * tcos*tcos;
                        c                   = add_cascade();
                        t                   = c->b;
                        b                   = c->t;

                        // Transfer function
                        t[0]                = kf / fg1;
                        t[1]                = 2.0f * k1 * xf * tcos;
                        t[2]                = fg1 * xf2;

                        b[0]                = fg1;
                        b[1]                = t[1]; // 2.0f * k1 * xf * tcos;
                        b[2]                = t[0] * xf2; // kf * xf * xf / fg1;

                        if (j == 0)
                        {
                            c->t[0]            *= gain2;
                            c->t[1]            *= gain2;
                            c->t[2]            *= gain2;
                        }
                    }

                    break;
                }

                case FLT_BT_BWC_BELL:
                {
                    float fg                = expf(logf(fp->fGain)/float(2*fp->nSlope));
                    float k                 = 1.0f / (1.0 + fp->fQuality);

                    for (size_t j=0; j < fp->nSlope; ++j)
                    {
                        float theta         = ((2*j + 1)*M_PI_2)/(2*fp->nSlope);
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0 - tsin*tsin);
                        float kf            = tsin*tsin + k*k * tcos*tcos;

                        if (fp->fGain >= 1.0)
                        {
                            // First cascade
                            c                   = add_cascade();

                            c->t[0]             = 1.0;
                            c->t[1]             = 2.0 * k * tcos * fg / kf;
                            c->t[2]             = 1.0 * fg * fg / kf;

                            c->b[0]             = 1.0;
                            c->b[1]             = 2.0 * k * tcos / kf;
                            c->b[2]             = 1.0 / kf;

                            // Second cascade
                            c                   = add_cascade();

                            c->t[0]             = 1.0;
                            c->t[1]             = 2.0 * k * tcos / fg;
                            c->t[2]             = kf / (fg * fg);

                            c->b[0]             = 1.0;
                            c->b[1]             = 2.0 * k * tcos;
                            c->b[2]             = kf;
                        }
                        else
                        {
                            // First cascade
                            c                   = add_cascade();

                            c->t[0]             = 1.0;
                            c->t[1]             = 2.0 * k * tcos / kf;
                            c->t[2]             = 1.0 / kf;

                            c->b[0]             = 1.0;
                            c->b[1]             = 2.0 * k * tcos / (fg * kf);
                            c->b[2]             = 1.0 / (fg * fg * kf);

                            // Second cascade
                            c                   = add_cascade();

                            c->t[0]             = 1.0;
                            c->t[1]             = 2.0 * k * tcos;
                            c->t[2]             = kf;

                            c->b[0]             = 1.0;
                            c->b[1]             = 2.0 * k * tcos * fg;
                            c->b[2]             = kf * fg * fg;
                        }
                    }

                    break;
                }

                case FLT_BT_BWC_BANDPASS:
                {
                    float f2            = fp->fFreq2;
                    float k             = 1.0f / (1.0f + fp->fQuality);

                    for (size_t j=0; j < fp->nSlope; ++j)
                    {
                        float theta         = ((2*j + 1)*M_PI_2)/(2*fp->nSlope);
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0 - tsin*tsin);
                        float kf            = tsin*tsin + k*k * tcos*tcos;

                        // Hi-pass cascade
                        c                   = add_cascade();

                        c->t[2]             = (j == 0) ? fp->fGain : 1.0;

                        c->b[0]             = 1.0 / kf;
                        c->b[1]             = 2.0 * k * tcos / kf;
                        c->b[2]             = 1.0;

                        // Lo-pass cascade
                        c                   = add_cascade();

                        c->t[0]             = 1.0;

                        c->b[0]             = 1.0;
                        c->b[1]             = 2.0 * k * tcos * f2 / kf;
                        c->b[2]             = f2 * f2 / kf;
                    }

                    break;
                }

                default:
                    nMode           = FM_BYPASS;
                    break;
            }
        }

        void Filter::calc_lrx_filter(size_t type, const filter_params_t *fp)
        {
            dsp::f_cascade_t *c1, *c2;

            // LRX filter is just twice repeated BWC filter
            // Calculate the same chain twice
            switch (type)
            {
                case FLT_BT_LRX_LOPASS:
                    type = FLT_BT_BWC_LOPASS;
                    break;
                case FLT_BT_LRX_HIPASS:
                    type = FLT_BT_BWC_HIPASS;
                    break;
                case FLT_BT_LRX_LOSHELF:
                    type = FLT_BT_BWC_LOSHELF;
                    break;
                case FLT_BT_LRX_HISHELF:
                    type = FLT_BT_BWC_HISHELF;
                    break;
                case FLT_BT_LRX_BELL:
                    type = FLT_BT_BWC_BELL;
                    break;
                case FLT_BT_LRX_BANDPASS:
                    type = FLT_BT_BWC_BANDPASS;
                    break;
                case FLT_BT_LRX_LADDERPASS:
                    type = FLT_BT_BWC_LADDERPASS;
                    break;
                case FLT_BT_LRX_LADDERREJ:
                    type = FLT_BT_BWC_LADDERREJ;
                    break;
                case FLT_BT_LRX_ALLPASS:
                {
                    float k     = 1.0f / (1.0f + fp->fQuality);
                    size_t i    = sParams.nSlope * 2;

                    // Emit 2x butterworth filters
                    for (size_t j=0; j < i; j += 2)
                    {
                        float theta     = ((j+1) * M_PI_2)/i;
                        float tsin      = sinf(theta);
                        float tcos      = sqrtf(1.0 - tsin*tsin);
                        float kf        = tsin*tsin + k*k * tcos*tcos;

                        c1              = add_cascade();
                        c2              = add_cascade();

                        // Top part
                        float xeta      = ((j+0.5) * M_PI)/i;
                        c1->t[0]        = 1.0;
                        c1->t[1]        = -2.0 * cosf(xeta);
                        c1->t[2]        = 1.0;

                        xeta            = ((j+1.5) * M_PI)/i;
                        c2->t[0]        = 1.0;
                        c2->t[1]        = -2.0 * cosf(xeta);
                        c2->t[2]        = 1.0;

                        // Bottom part
                        c1->b[0]        = 1.0 / kf;
                        c1->b[1]        = 2.0 * k * tcos / kf;
                        c1->b[2]        = 1.0;

                        c2->b[0]        = c1->b[0];
                        c2->b[1]        = c1->b[1];
                        c2->b[2]        = c1->b[2];

                        if (j == 0)
                        {
                            c1->t[0]       *= fp->fGain;
                            c1->t[1]       *= fp->fGain;
                            c1->t[2]       *= fp->fGain;
                        }
                    }
                    return;
                }
                default:
                    nMode           = FM_BYPASS;
                    return;
            }

            // Have to do some hacks with chain parameters: reduce slope and gain
            filter_params_t bfp = *fp;
            bfp.nSlope          = sParams.nSlope*2;
            bfp.fGain           = sqrtf(bfp.fGain);

            // Calculate two similar chains
            calc_bwc_filter(type, &bfp);
            calc_bwc_filter(type, &bfp);
        }

        void Filter::calc_apo_filter(size_t type, const filter_params_t *fp)
        {
            float a0, a1, a2;
            float b0, b1, b2;

            float omega    = 2.0 * M_PI * fp->fFreq / float(nSampleRate);
            float cs       = sinf(omega);
            float cc       = cosf(omega); // Have to use trig functions for both to have correct sign
            float Q        = (fp->fQuality > MIN_APO_Q) ? fp->fQuality : MIN_APO_Q;
            float alpha    = 0.5 * cs / Q;

            // In LSP convention, the b coefficients are in the denominator. The a coefficients are in the
            // numerator. This is opposite to the most usual convention.
            // Coefficients are normalised so that b0 = 1

            switch (type)
            {
                case FLT_DR_APO_LOPASS:
                {
                    float A     = fp->fGain;

                    a0 = A * 0.5 * (1.0 - cc);
                    a1 = A * (1.0 - cc);
                    a2 = a0;
                    b0 = 1.0 + alpha;
                    b1 = -2.0 * cc;
                    b2 = 1.0 - alpha;

                    break;
                }

                case FLT_DR_APO_HIPASS:
                {
                    float A     = fp->fGain;

                    a0 = A * 0.5 * (1.0 + cc);
                    a1 = A * (-1.0 - cc);
                    a2 = a0;
                    b0 = 1.0 + alpha;
                    b1 = -2.0 * cc;
                    b2 = 1.0 - alpha;

                    break;
                }

                case FLT_DR_APO_BANDPASS:
                {
                    float A     = fp->fGain;

                    a0 = A * alpha;
                    a1 = 0.0;
                    a2 = A * -alpha;
                    b0 = 1.0 + alpha;
                    b1 = -2.0 * cc;
                    b2 = 1 - alpha;

                    break;
                }

                case FLT_DR_APO_NOTCH:
                {
                    float A     = fp->fGain;

                    a0 = A;
                    a1 = A * -2.0 * cc;
                    a2 = a0;
                    b0 = 1.0 + alpha;
                    b1 = -2.0 * cc;
                    b2 = 1.0 - alpha;

                    break;
                }

                case FLT_DR_APO_ALLPASS:
                {
                    float A     = fp->fGain;

                    a0 = A * (1.0 - alpha);
                    a1 = A * -2.0 * cc;
                    a2 = A * (1.0 + alpha);
                    b0 = a2;
                    b1 = a1;
                    b2 = a0;

                    break;
                }

                case FLT_DR_APO_PEAKING:
                {
                    float A     = sqrtf(fp->fGain);

                    a0 = 1.0 + alpha * A;
                    a1 = -2.0 * cc;
                    a2 = 1.0 - alpha * A;
                    b0 = 1.0 + alpha / A;
                    b1 = a1;
                    b2 = 1.0 - alpha / A;

                    break;
                }

                case FLT_DR_APO_LOSHELF:
                {
                    float A     = sqrtf(fp->fGain);
                    float beta  = 2.0 * alpha * sqrtf(A);

                    a0 = A * ((A + 1.0) - (A - 1.0) * cc + beta);
                    a1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cc);
                    a2 = A * ((A + 1.0) - (A - 1.0) * cc - beta);
                    b0 = (A + 1.0) + (A - 1.0) * cc + beta;
                    b1 = -2.0 * ((A - 1.0) + (A + 1.0) * cc);
                    b2 = (A + 1.0) + (A - 1.0) * cc - beta;

                    break;
                }

                case FLT_DR_APO_HISHELF:
                {
                    float A     = sqrtf(fp->fGain);
                    float beta  = 2.0 * alpha * sqrtf(A);

                    a0 = A * ((A + 1.0) + (A - 1.0) * cc + beta);
                    a1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cc);
                    a2 = A * ((A + 1.0) + (A - 1.0) * cc - beta);
                    b0 = (A + 1.0) - (A - 1.0) * cc + beta;
                    b1 = 2.0 * ((A - 1.0) - (A + 1.0) * cc);
                    b2 = (A + 1.0) - (A - 1.0) * cc - beta;

                    break;
                }

                default:
                    return;
            }

            dsp::biquad_x1_t *f = pBank->add_chain();
            if (f == NULL)
                return;

            // Storing with appropriate normalisation and sign as required by biquad_process_x1().
            f->b0   = a0 / b0;
            f->b1   = a1 / b0;
            f->b2   = a2 / b0;
            f->a1   = -b1 / b0;
            f->a2   = -b2 / b0;
            f->p0   = 0.0f;
            f->p1   = 0.0f;
            f->p2   = 0.0f;

            // Storing the coefficient for plotting
            dsp::f_cascade_t *c  = add_cascade();
            c->t[0] = f->b0;
            c->t[1] = f->b1;
            c->t[2] = f->b2;
            c->b[0] = 1.0;
            c->b[1] = -f->a1;
            c->b[2] = -f->a2;
        }

        void Filter::normalize(dsp::biquad_x1_t *f, float frequency, float gain)
        {
            float xf        = 2.0 * M_PI * lsp_min(frequency, nSampleRate * 0.5f) / float(nSampleRate);

            float cw        = cosf(xf);
            float sw        = sinf(xf);

            // These equations are valid since sw has valid sign
            float c2w       = cw * cw - sw * sw;    // cos(2 * w)
            float s2w       = 2.0 * sw * cw;        // sin(2 * w)

            float alpha     = f->b0 + f->b1 * cw + f->b2 * c2w;
            float beta      = f->b1 * sw + f->b2 * s2w;
            float gamma     = 1.0f - f->a1 * cw - f->a2 * c2w;
            float delta     = -f->a1 * sw - f->a2 * s2w;

            float mag       = gamma * gamma + delta * delta;

            // Compute current biquad's tranfer function
            float w_re      = alpha * gamma - beta * delta;
            float w_im      = alpha * delta + beta * gamma;

            float egain     = (gain * mag) / sqrtf(w_re * w_re + w_im * w_im);

            f->b0          *= egain;
            f->b1          *= egain;
            f->b2          *= egain;
        }

        void Filter::calc_weighted_filter(size_t type, const filter_params_t *fp)
        {
            const float T       = 1.0f / float(nSampleRate);

            switch (type)
            {
                case FLT_A_WEIGHTED:
                    // https://en.wikipedia.org/wiki/A-weighting
                    // Final equations for non-normalized A-weighted filter given as follows:
                    //
                    // Ha[p]=(ka*p^4)/((p+129.4)*(p+129.4)*(p+676.7)*(p+4636.0)*(p+76655)*(p+76655));
                    //
                    // The final A-weighted filter should give 0 dB at 1 kHz frequency.

                    // Zeros: 0, 0
                    // Poles: -129.4, -129.4
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 129.4f;
                        float ww            = p0 * T;
                        float ws            = sinf(ww);
                        float wc            = cosf(ww);

                        float ka0           = 1.0f / (1.0f + ws);

                        f->b0               = 0.5f * (1.0f + wc) * ka0;
                        f->b1               = (-1.0f - wc) * ka0;
                        f->b2               = f->b0;
                        f->a1               = 2.0f * wc * ka0;
                        f->a2               = (ws - 1.0f) * ka0;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    // Zeros: 0, 0
                    // Poles: -676.7, -4636.0
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 676.7f;
                        constexpr float p1  = 4636.0f;

                        float ww0           = p0 * T;
                        float ww1           = p1 * T;
                        float ws0           = sinf(ww0);
                        float wc0           = cosf(ww0);
                        float ws1           = sinf(ww1);
                        float wc1           = cosf(ww1);

                        float kx0           = 1.0f / (1.0f + ws0 - wc0);
                        float kx1           = 1.0f / (1.0f + ws1 - wc1);
                        float ka0           = kx0 * kx1;
                        float ky0           = (1.0f - wc0 - ws0);
                        float ky1           = (1.0f - wc1 - ws1);

                        f->b0               = ws0 * ws1 * ka0;
                        f->b1               = -2.0f * f->b0;
                        f->b2               = f->b0;
                        f->a1               = -(ky0 * kx0 + ky1 * kx1);
                        f->a2               = - ky0 * ky1 * ka0;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    // Zeros: x
                    // Poles: -76655.0, -76655.0
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 76655.0f;
                        float ww            = p0 * T;
                        float ws            = sinf(ww);
                        float wc            = cosf(ww);

                        float ka0           = 1.0f / (1.0f + ws);

                        f->b0               = 0.5f * (1.0f - wc) * ka0;
                        f->b1               = (1.0f - wc) * ka0;
                        f->b2               = f->b0;
                        f->a1               = -2.0f * wc * ka0;
                        f->a2               = (1.0f - ws) * ka0;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    nMode               = FM_APO;
                    break;

                case FLT_B_WEIGHTED:
                    // https://en.wikipedia.org/wiki/A-weighting
                    // Final equations for non-normalized B-weighted filter given as follows:
                    //
                    // Hb[p]=kb*p^3/((p+129.4)*(p+129.4)*(p+995.9)*(p+76655)*(p+76655));
                    //
                    // The final B-weighted filter should give 0 dB at 1 kHz frequency.

                    // Zeros: 0, 0
                    // Poles: -129.4, -129.4
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 129.4f;
                        float ww            = p0 * T;
                        float ws            = sinf(ww);
                        float wc            = cosf(ww);

                        float ka0           = 1.0f / (1.0f + ws);

                        f->b0               = 0.5f * (1.0f + wc) * ka0;
                        f->b1               = (-1.0f - wc) * ka0;
                        f->b2               = f->b0;
                        f->a1               = 2.0f * wc * ka0;
                        f->a2               = (ws - 1.0f) * ka0;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    // Zeros: 0
                    // Poles: -995.9
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 995.9f;
                        float ww            = p0 * T;
                        float ws            = sinf(ww);
                        float wc            = cosf(ww);

                        float ka0           = 1.0f / (1.0f + ws - wc);

                        f->b0               = ws * ka0;
                        f->b1               = - f->b0;
                        f->b2               = 0.0f;
                        f->a1               = (ws + wc - 1.0f) * ka0;
                        f->a2               = 0.0f;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    // Zeros: x
                    // Poles: -76655.0, -76655.0
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 76655.0f;
                        float ww            = p0 * T;
                        float ws            = sinf(ww);
                        float wc            = cosf(ww);

                        float ka0           = 1.0f / (1.0f + ws);

                        f->b0               = 0.5f * (1.0f - wc) * ka0;
                        f->b1               = (1.0f - wc) * ka0;
                        f->b2               = f->b0;
                        f->a1               = -2.0f * wc * ka0;
                        f->a2               = (1.0f - ws) * ka0;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    nMode               = FM_APO;
                    break;

                case FLT_C_WEIGHTED:
                    // https://en.wikipedia.org/wiki/A-weighting
                    // Final equations for non-normalized C-weighted filter given as follows:
                    //
                    // Hc[p]=p^2/((p+129.4)*(p+129.4)*(p+76655)*(p+76655));
                    //
                    // The final C-weighted filter should give 0 dB at 1 kHz frequency.

                    // Zeros: 0, 0
                    // Poles: -129.4, -129.4
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 129.4f;
                        float ww            = p0 * T;
                        float ws            = sinf(ww);
                        float wc            = cosf(ww);

                        float ka0           = 1.0f / (1.0f + ws);

                        f->b0               = 0.5f * (1.0f + wc) * ka0;
                        f->b1               = (-1.0f - wc) * ka0;
                        f->b2               = f->b0;
                        f->a1               = 2.0f * wc * ka0;
                        f->a2               = (ws - 1.0f) * ka0;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    // Zeros: x
                    // Poles: -76655.0, -76655.0
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 76655.0f;
                        float ww            = p0 * T;
                        float ws            = sinf(ww);
                        float wc            = cosf(ww);

                        float ka0           = 1.0f / (1.0f + ws);

                        f->b0               = 0.5f * (1.0f - wc) * ka0;
                        f->b1               = (1.0f - wc) * ka0;
                        f->b2               = f->b0;
                        f->a1               = -2.0f * wc * ka0;
                        f->a2               = (1.0f - ws) * ka0;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    nMode               = FM_APO;
                    break;

                case FLT_D_WEIGHTED:
                    // https://en.wikipedia.org/wiki/A-weighting
                    // Final equations for non-normalized A-weighted filter given as follows:
                    //
                    // Hd[p]=(p*(p^2 +6532*p + 4.0975e7))/((p+1776.3)*(p+7288.5)*(p^2+21514*p+3.8836e8));
                    //
                    // The final D-weighted filter should give 0 dB at 1 kHz frequency.

                    // Zeros: 0
                    // Poles: -1776.3, -7288.5
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 1776.3f;
                        constexpr float p1  = 7288.5f;

                        float ww0           = p0 * T;
                        float ww1           = p1 * T;
                        float ws0           = sinf(ww0);
                        float wc0           = cosf(ww0);
                        float ws1           = sinf(ww1);
                        float wc1           = cosf(ww1);

                        float kx0           = 1.0f / (1.0f + ws0 - wc0);
                        float kx1           = 1.0f / (1.0f + ws1 - wc1);
                        float ka0           = kx0 * kx1;
                        float ky0           = (1.0f - wc0 - ws0);
                        float ky1           = (1.0f - wc1 - ws1);

                        f->b0               = ws0 * (1.0f - wc1) * ka0;
                        f->b1               = 0.0f;
                        f->b2               = -f->b0;
                        f->a1               = -(ky0 * kx0 + ky1 * kx1);
                        f->a2               = - ky0 * ky1 * ka0;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    // Zeros: complex pair at frequency 6401.17, Q=0.98, R = 1/Q = 1.02
                    // Poles: complex pair at frequency 19706.85, Q = 0,916, R = 1/Q = 1.092
                    {
                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        constexpr float p0  = 6401.17f;
                        constexpr float p1  = 19706.85f;
                        constexpr float r0  = 1.02f;
                        constexpr float r1  = 1.092f;

                        float ww0           = p0 * T * 0.5f;
                        float ww1           = p1 * T * 0.5f;
                        float wt0           = 1.0f / tanf(ww0);
                        float wt1           = 1.0f / tanf(ww1);

                        float ka0           = 1.0f / (1.0f + wt1*(wt1 + r1));

                        f->b0               = (1.0f + wt0*(wt0 + r0)) * ka0;
                        f->b1               = 2.0f * (1.0f - wt0*wt0) * ka0;
                        f->b2               = (1.0f + wt0*(wt0 - r0)) * ka0;
                        f->a1               = -2.0f * (1.0f - wt1*wt1) * ka0;
                        f->a2               = -(1.0f + wt1*(wt1 - r1)) * ka0;
                        f->p0               = 0.0f;
                        f->p1               = 0.0f;
                        f->p2               = 0.0f;

                        normalize(f, 1000.0f, 1.0f);

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]             = f->b0;
                        c->t[1]             = f->b1;
                        c->t[2]             = f->b2;
                        c->b[0]             = 1.0f;
                        c->b[1]             = -f->a1;
                        c->b[2]             = -f->a2;
                    }

                    nMode               = FM_APO;
                    break;

                case FLT_K_WEIGHTED:

                    // Original ITU-R BS.1770-4 (10/2015) Specifies biquad filer coefficients for 48 kHz sample rate
                    //
                    // High-shelving filter:
                    // b0 = 1.53512485958697f, b1 = -2.69169618940638f, b2 = 1.19839281085285f,
                    // a1 = -1.69065929318241f, a2 = 0.73248077421585f
                    //
                    // High-pass filter:
                    // b0 = 1.0f, b1 = -2.0f, b2 = 1.0f,
                    // a1 = -1.99004745483398f, a2 = 0.99007225036621f
                    //
                    // For another sample rates they should be recomputed, which is not trivial process.
                    // This piece of code has been found somewhere in the internet and is used as a good approximation.

                    // High-shelving filter
                    {
                        constexpr float Vh  = 1.58486470113f; // power(10.0, db / 20.0); db = 3.999843853973347f;
                        constexpr float Vb  = 1.25872093023f; // power(Vh, 0,4996667741545416);
                        constexpr float f0  = 1681.974450955533f;
                        constexpr float Q   = 0.7071752369554196f;
                        float K             = tanf(M_PI * f0 * T);
                        float K2            = K*K;
                        float KQ            = K / Q;

                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        // Storing with appropriate normalisation and sign as required by biquad_process_x1().
                        float ka0       = 1.0f / (1.0f + KQ + K2);
                        f->b0           = (Vh + Vb * KQ + K2) * ka0;
                        f->b1           = 2.0f * (K2 -  Vh) * ka0;
                        f->b2           = (Vh - Vb * KQ + K2) * ka0;
                        f->a1           = -2.0f * (K2 - 1.0f) * ka0;
                        f->a2           = - (1.0f - KQ + K2) * ka0;
                        f->p0           = 0.0f;
                        f->p1           = 0.0f;
                        f->p2           = 0.0f;

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]         = f->b0;
                        c->t[1]         = f->b1;
                        c->t[2]         = f->b2;
                        c->b[0]         = 1.0;
                        c->b[1]         = -f->a1;
                        c->b[2]         = -f->a2;
                    }

                    // Hi-pass filter
                    {
                        constexpr float f0  = 38.13547087602444f;
                        constexpr float Q   = 0.5003270373238773f;
                        float K             = tanf(M_PI * f0 * T);
                        float K2            = K*K;
                        float KQ            = K / Q;

                        dsp::biquad_x1_t *f = pBank->add_chain();
                        if (f == NULL)
                            return;

                        // Storing with appropriate normalisation and sign as required by biquad_process_x1().
                        float ka0       = 1.0f / (1.0f + KQ + K2);
                        f->b0           = 1.0f;
                        f->b1           = -2.0f;
                        f->b2           = 1.0f;
                        f->a1           = - 2.0f * (K2 - 1.0f) * ka0;
                        f->a2           = - (1.0f - KQ + K2) * ka0;
                        f->p0           = 0.0f;
                        f->p1           = 0.0f;
                        f->p2           = 0.0f;

                        // Storing the coefficient for plotting
                        dsp::f_cascade_t *c  = add_cascade();
                        c->t[0]         = f->b0;
                        c->t[1]         = f->b1;
                        c->t[2]         = f->b2;
                        c->b[0]         = 1.0;
                        c->b[1]         = -f->a1;
                        c->b[2]         = -f->a2;
                    }

                    nMode               = FM_APO;
                    break;

                default:
                    break;
            }
        }

        /*
            Original filter chain:

                       t[0] + t[1] * p + t[2] * p^2
              H(p) =  ------------------------------
                       b[0] + b[1] * p + b[2] * p^2

            Bilinear transform:

              x    = z^-1

              kf   = 1 / tan(pi * frequency / sample_rate) - frequency shift factor

                           1 - x
              p    = kf * -------   - analog -> digital bilinear transform expression
                           1 + x

            Applied bilinear transform:

                       (t[0] + t[1]*kf + t[2]*kf^2) + 2*(t[0] - t[2]*kf^2)*x + (t[0] - t[1]*kf + t[2]*kf^2)*x^2
              H[x] =  -----------------------------------------------------------------------------------------
                       (b[0] + b[1]*kf + b[2]*kf^2) + 2*(b[0] - b[2]*kf^2)*x + (b[0] - b[1]*kf + b[2]*kf^2)*x^2

            Finally:

              T    =   { t[0], t[1]*kf, t[2]*kf*kf }
              B    =   { b[0], b[1]*kf, b[2]*kf*kf }

                       (T[0] + T[1] + T[2]) + 2*(T[0] - T[2])*z^-1 + (T[0] - T[1] + T[2])*z^-2
              H[z] =  -------------------------------------------------------------------------
                       (B[0] + B[1] + B[2]) + 2*(B[0] - B[2])*z^-1 + (B[0] - B[1] + B[2])*z^-2
         */

        void Filter::bilinear_transform()
        {
            float kf        = 1.0/tanf(sParams.fFreq * M_PI / float(nSampleRate));
            float kf2       = kf * kf;
            float T[4], B[4], N;
            size_t chains   = 0;

            for (size_t i=0; i<nItems; ++i)
            {
                dsp::f_cascade_t *c  = &vItems[i];
                float *t        = c->t;
                float *b        = c->b;

                // Calculate top coefficients
                T[0]            = t[0];
                T[1]            = t[1]*kf;
                T[2]            = t[2]*kf2;

                // Calculate bottom coefficients
                B[0]            = b[0];
                B[1]            = b[1]*kf;
                B[2]            = b[2]*kf2;

                // Calculate the convolution
                N               = 1.0 / (B[0] + B[1] + B[2]);

                // Initialize filter parameters
                if ((++chains) > FILTER_CHAINS_MAX)
                    break;
                dsp::biquad_x1_t *f  = pBank->add_chain();
                if (f == NULL)
                    break;

                f->b0           = (T[0] + T[1] + T[2]) * N;
                f->b1           = 2.0 * (T[0] - T[2]) * N;
                f->b2           = (T[0] - T[1] + T[2]) * N;
                f->a1           = 2.0 * (B[2] - B[0]) * N; // Sign negated
                f->a2           = (B[1] - B[2] - B[0]) * N; // Sign negated
                f->p0           = 0.0f;
                f->p1           = 0.0f;
                f->p2           = 0.0f;
            }
        }

        /*
            Original filter chain:

                       t[0] + t[1] * p + t[2] * p^2     k1 * (p + a[0]) * (p + a[1])
              H(p) =  ------------------------------ = -----------------------------
                       b[0] + b[1] * p + b[2] * p^2     k2 * (p + b[0]) * (p + b[1])

              a[0], a[1], b[0], b[1] may not exist, so there are series of solutions

            Matched Z-transform:

              T    = discretization period

              x    = z^-1

              p + a = 1 - x*exp(-a * T)

              kf   = 1 / f, f = filter frequency

            After the Matched Z-transform the Frequency Chart of the filter has to be normalized!

        */
        void Filter::matched_transform()
        {
            float T[4], B[4], A[2], I[2];
            float f         = sParams.fFreq;
            float TD        = 2.0*M_PI / nSampleRate;
            size_t chains   = 0;

            // Iterate each cascade
            for (size_t i=0; i<nItems; ++i)
            {
                dsp::f_cascade_t *c = &vItems[i];

                // Process each polynom (top, bottom) individually
                for (size_t i=0; i<2; ++i)
                {
                    float *p    = (i) ? c->b : c->t;
                    float *P    = (i) ? B : T;

                    if (p[2] == 0.0f) // Test polynom for second-order
                    {
                        P[2]    = 0.0f;
                        if (p[1] == 0.0f) // Test polynom for first order
                        {
                            // Zero-order polynom
                            P[0]    = p[0];
                            P[1]    = 0.0f;
                        }
                        else
                        {
                            // First-order polynom:
                            //   p(s) = p[0] + p[1]*(s/f)
                            //
                            // Transformed polynom:
                            //   P[z] = p[1]/f - p[1]/f * exp(-f*p[0]*T/p[1]) * z^-1
                            float k     = p[1]/f;
                            float R     = -p[0]/k;
                            P[0]        = k;
                            P[1]        = -k * expf(R*TD);
                        }
                    }
                    else
                    {
                        // Second-order polynom:
                        //   p(s) = p[0] + p[1]*(s/f) + p[2]*(s/f)^2 = p[2]/f^2 * (p[0]*f^2/p[2] + p[1]*f/p[2]*s + s^2)
                        //
                        // Calculate the roots of the second-order polynom equation a*x^2 + b*x + c = 0
                        float k     = p[2];
                        float a     = 1.0f/(f*f);
                        float b     = p[1]/(f*p[2]);
                        float c     = p[0]/p[2];
                        float D     = b*b - 4.0f*a*c;

                        if (D >= 0)
                        {
                            // Has real roots R0 and R1
                            // Transformed form is:
                            //   P[z] = k*(1 - (exp(R0*T) + exp(R1*T))*z^-1 + exp((R0+R1)*T)*z^-2)
                            D           = sqrtf(D);
                            float R0    = (-b - D)/(2.0f*a);
                            float R1    = (-b + D)/(2.0f*a);
                            P[0]        = k;
                            P[1]        = -k * (expf(R0*TD) + expf(R1*TD));
                            P[2]        = k * expf((R0+R1)*TD);
                        }
                        else
                        {
                            // Has complex roots R+j*K and R-j*K
                            // Transformed form is:
                            //   P[z] = k*(1 - 2*exp(R*T)*cos(K*T)*z^-1 + exp(2*R*T)*z^-2)
                            D           = sqrtf(-D);
                            float R     = -b / (2.0f*a);
                            float K     = D / (2.0f*a);
                            P[0]        = k;
                            P[1]        = -2.0f * k * expf(R*TD) * cosf(K*TD);
                            P[2]        = k * expf(2.0f*R*TD);
                        }
                    }

                    // We have to calculate the norming factor of the digital filter
                    // To do this, we should get the amplitude of the discrete transfer function
                    // at the control frequency and the amplitude of the continuous transfer function
                    // at the same frequency.
                    // As control frequency we take the f/10 value
                    // For the discrete transfer function it will be PI*0.2*f / SR
                    // For the normalized continuous transfer function it will be always 0.1

                    // Calculate the discrete transfer function part at specified frequency
                    double w    = M_PI * 0.2f * sParams.fFreq / nSampleRate;
                    double re   = P[0]*cos(2.0*w) + P[1]*cos(w) + P[2];
                    double im   = P[0]*sin(2.0*w) + P[1]*sin(w);
                    A[i]        = sqrt(re*re + im*im);

                    // Calculate the continuous transfer function part at 1 Hz
                    w           = 0.1;
                    re          = p[0] - p[2]*w*w;
                    im          = p[1]*w;
                    I[i]        = sqrt(re*re + im*im);
                }

                // Now calculate the convolution for the new polynom:
                /*
                           T[0] + T[1]*z^-1 + T[2]*z^-2
                  H(z) =  ------------------------------
                           B[0] + B[1]*z^-1 + B[2]*z^-2

                 */
                double AN       = (A[1]*I[0]) / (A[0]*I[1]); // Normalizing factor for the amplitude to match the analog filter
                double N        = 1.0 / B[0];

                // Initialize filter parameters
                if ((++chains) > FILTER_CHAINS_MAX)
                    break;
                dsp::biquad_x1_t *f  = pBank->add_chain();
                if (f == NULL)
                    break;

                f->b0           = T[0] * N * AN;
                f->b1           = T[1] * N * AN;
                f->b2           = T[2] * N * AN;
                f->a1           = -B[1] * N; // Sign negated
                f->a2           = -B[2] * N; // Sign negated
                f->p0           = 0.0f;
                f->p1           = 0.0f;
                f->p2           = 0.0f;
            }
        }

        bool Filter::impulse_response(float *out, size_t length)
        {
            if (!(nFlags & FF_OWN_BANK))
                return false;

            if (nFlags & (~FF_OWN_BANK))
                rebuild();

            pBank->impulse_response(out, length);
            return true;
        }

        void Filter::dump(IStateDumper *v) const
        {
            if (nFlags & FF_OWN_BANK)
                v->write_object("pBank", pBank);
            else
                v->write("pBank", pBank);

            v->begin_object("sParams", &sParams, sizeof(filter_params_t));
            {
                v->write("nType", sParams.nType);
                v->write("fFreq", sParams.fFreq);
                v->write("fFreq2", sParams.fFreq2);
                v->write("fGain", sParams.fGain);
                v->write("nSlope", sParams.nSlope);
                v->write("fQuality", sParams.fQuality);
            }
            v->end_object();

            v->write("nSampleRate", nSampleRate);
            v->write("nMode", nMode);
            v->write("nItems", nItems);
            v->begin_array("vItems", vItems, nItems);
            for (size_t i=0; i<nItems; ++i)
            {
                dsp::f_cascade_t *c = &vItems[i];
                v->begin_object(c, sizeof(dsp::f_cascade_t));
                {
                    v->writev("t", c->t, 4);
                    v->writev("b", c->b, 4);
                }
                v->end_object();
            }
            v->end_array();
            v->write("vData", vData);
            v->write("nFlags", nFlags);
            v->write("nLatency", nLatency);
        }

    } /* namespace dspu */
} /* namespace lsp */


