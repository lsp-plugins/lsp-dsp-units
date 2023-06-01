/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 19 нояб. 2016 г.
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

#include <lsp-plug.in/dsp-units/util/Oversampler.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>

#define OS_UP_BUFFER_SIZE       (12 * 1024)   /* Multiple of 3 and 4 */
#define OS_DOWN_BUFFER_SIZE     (12 * 1024)   /* Multiple of 3 and 4 */
#define OS_CUTOFF               21000.0f

namespace lsp
{
    namespace dspu
    {
        IOversamplerCallback::~IOversamplerCallback()
        {
        }

        void IOversamplerCallback::process(float *out, const float *in, size_t samples)
        {
            dsp::copy(out, in, samples);
        }

        Oversampler::Oversampler()
        {
            construct();
        }

        Oversampler::~Oversampler()
        {
            destroy();
        }

        void Oversampler::construct()
        {
            pCallback       = NULL;
            fUpBuffer       = NULL;
            fDownBuffer     = NULL;
            pFunc           = dsp::copy;
            nUpHead         = 0;
            nMode           = OM_NONE;
            nSampleRate     = 0;
            nUpdate         = UP_ALL;
            bData           = NULL;
            bFilter         = true;
        }

        bool Oversampler::init()
        {
            if (!sFilter.init(NULL))
                return false;

            if (bData == NULL)
            {
                size_t samples  = OS_UP_BUFFER_SIZE + OS_DOWN_BUFFER_SIZE + LSP_DSP_RESAMPLING_RSV_SAMPLES;
                float *ptr      = alloc_aligned<float>(bData, samples, DEFAULT_ALIGN);
                if (ptr == NULL)
                    return false;
                fDownBuffer     = ptr;
                ptr            += OS_DOWN_BUFFER_SIZE;
                fUpBuffer       = reinterpret_cast<float *>(ptr);
                ptr            += OS_UP_BUFFER_SIZE + LSP_DSP_RESAMPLING_RSV_SAMPLES;
            }

            // Clear buffer
            dsp::fill_zero(fUpBuffer, OS_UP_BUFFER_SIZE + LSP_DSP_RESAMPLING_RSV_SAMPLES);
            dsp::fill_zero(fDownBuffer, OS_DOWN_BUFFER_SIZE);
            nUpHead       = 0;

            return true;
        }

        void Oversampler::destroy()
        {
            sFilter.destroy();
            if (bData != NULL)
            {
                free_aligned(bData);
                fUpBuffer   = NULL;
                fDownBuffer = NULL;
                bData       = NULL;
            }
            pCallback = NULL;
        }

        void Oversampler::set_sample_rate(size_t sr)
        {
            if (sr == nSampleRate)
                return;
            nSampleRate     = sr;
            nUpdate        |= UP_SAMPLE_RATE;
            size_t os       = get_oversampling();

            // Update filter parameters
            filter_params_t fp;
            fp.fFreq        = OS_CUTOFF;        // Calculate cutoff frequency
            fp.fFreq2       = fp.fFreq;
            fp.fGain        = 1.0f;
            fp.fQuality     = 0.5f;
            fp.nSlope       = 30;               // 30 poles = 30 * 3db/oct = 90 db/Oct
            fp.nType        = FLT_BT_BWC_LOPASS;// Chebyshev filter

            sFilter.update(nSampleRate * os, &fp);
        }

        void Oversampler::update_settings()
        {
            if (nUpdate & (UP_MODE | UP_SAMPLE_RATE))
            {
                dsp::fill_zero(fUpBuffer, OS_UP_BUFFER_SIZE + LSP_DSP_RESAMPLING_RSV_SAMPLES);
                nUpHead       = 0;
                sFilter.clear();
            }

            size_t os       = get_oversampling();
            filter_params_t fp;
            sFilter.get_params(&fp);
            sFilter.update(nSampleRate * os, &fp);

            nUpdate = 0;
            return;
        }

        size_t Oversampler::get_oversampling() const
        {
            switch (nMode)
            {
                case OM_LANCZOS_2X2:
                case OM_LANCZOS_2X3:
                case OM_LANCZOS_2X4:
                case OM_LANCZOS_2X12BIT:
                case OM_LANCZOS_2X16BIT:
                case OM_LANCZOS_2X24BIT:
                    return 2;

                case OM_LANCZOS_3X2:
                case OM_LANCZOS_3X3:
                case OM_LANCZOS_3X4:
                case OM_LANCZOS_3X12BIT:
                case OM_LANCZOS_3X16BIT:
                case OM_LANCZOS_3X24BIT:
                    return 3;

                case OM_LANCZOS_4X2:
                case OM_LANCZOS_4X3:
                case OM_LANCZOS_4X4:
                case OM_LANCZOS_4X12BIT:
                case OM_LANCZOS_4X16BIT:
                case OM_LANCZOS_4X24BIT:
                    return 4;

                case OM_LANCZOS_6X2:
                case OM_LANCZOS_6X3:
                case OM_LANCZOS_6X4:
                case OM_LANCZOS_6X12BIT:
                case OM_LANCZOS_6X16BIT:
                case OM_LANCZOS_6X24BIT:
                    return 6;

                case OM_LANCZOS_8X2:
                case OM_LANCZOS_8X3:
                case OM_LANCZOS_8X4:
                case OM_LANCZOS_8X12BIT:
                case OM_LANCZOS_8X16BIT:
                case OM_LANCZOS_8X24BIT:
                    return 8;

                default:
                    break;
            }

            return 1;
        }

        void Oversampler::upsample(float *dst, const float *src, size_t samples)
        {
            switch (nMode)
            {
                case OM_LANCZOS_2X2:
                case OM_LANCZOS_2X3:
                case OM_LANCZOS_2X4:
                case OM_LANCZOS_2X12BIT:
                case OM_LANCZOS_2X16BIT:
                case OM_LANCZOS_2X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) >> 1;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE >> 1;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);
                        dsp::copy(dst, &fUpBuffer[nUpHead], to_do << 1);

                        // Update pointers
                        nUpHead        += to_do << 1;
                        dst            += to_do << 1;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_3X2:
                case OM_LANCZOS_3X3:
                case OM_LANCZOS_3X4:
                case OM_LANCZOS_3X12BIT:
                case OM_LANCZOS_3X16BIT:
                case OM_LANCZOS_3X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) / 3;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE / 3;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);
                        dsp::copy(dst, &fUpBuffer[nUpHead], to_do * 3);

                        // Update pointers
                        nUpHead        += to_do * 3;
                        dst            += to_do * 3;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_4X2:
                case OM_LANCZOS_4X3:
                case OM_LANCZOS_4X4:
                case OM_LANCZOS_4X12BIT:
                case OM_LANCZOS_4X16BIT:
                case OM_LANCZOS_4X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) >> 2;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE >> 2;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);
                        dsp::copy(dst, &fUpBuffer[nUpHead], to_do << 2);

                        // Update pointers
                        nUpHead        += to_do << 2;
                        dst            += to_do << 2;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_6X2:
                case OM_LANCZOS_6X3:
                case OM_LANCZOS_6X4:
                case OM_LANCZOS_6X12BIT:
                case OM_LANCZOS_6X16BIT:
                case OM_LANCZOS_6X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) / 6;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE / 6;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);
                        dsp::copy(dst, &fUpBuffer[nUpHead], to_do * 6);

                        // Update pointers
                        nUpHead        += to_do * 6;
                        dst            += to_do * 6;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_8X2:
                case OM_LANCZOS_8X3:
                case OM_LANCZOS_8X4:
                case OM_LANCZOS_8X12BIT:
                case OM_LANCZOS_8X16BIT:
                case OM_LANCZOS_8X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) >> 3;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE >> 3;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);
                        dsp::copy(dst, &fUpBuffer[nUpHead], to_do << 3);

                        // Update pointers
                        nUpHead        += to_do << 3;
                        dst            += to_do << 3;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }


                case OM_NONE:
                default:
                    dsp::copy(dst, src, samples);
                    break;
            }
        }

        void Oversampler::downsample(float *dst, const float *src, size_t samples)
        {
            switch (nMode)
            {
                case OM_LANCZOS_2X2:
                case OM_LANCZOS_2X3:
                case OM_LANCZOS_2X4:
                case OM_LANCZOS_2X12BIT:
                case OM_LANCZOS_2X16BIT:
                case OM_LANCZOS_2X24BIT:
                {
                    while (samples > 0)
                    {
                        // Perform filtering
                        size_t can_do   = OS_DOWN_BUFFER_SIZE >> 1;
                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        if (bFilter)
                        {
                            sFilter.process(fDownBuffer, src, to_do << 1);
                            dsp::downsample_2x(dst, fDownBuffer, to_do);
                        }
                        else
                            dsp::downsample_2x(dst, src, to_do);

                        // Update pointers
                        src            += to_do << 1;
                        dst            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_3X2:
                case OM_LANCZOS_3X3:
                case OM_LANCZOS_3X4:
                case OM_LANCZOS_3X12BIT:
                case OM_LANCZOS_3X16BIT:
                case OM_LANCZOS_3X24BIT:
                {
                    while (samples > 0)
                    {
                        // Perform filtering
                        size_t can_do   = OS_DOWN_BUFFER_SIZE / 3;
                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        if (bFilter)
                        {
                            sFilter.process(fDownBuffer, src, to_do * 3);
                            dsp::downsample_3x(dst, fDownBuffer, to_do);
                        }
                        else
                            dsp::downsample_3x(dst, src, to_do);

                        // Update pointers
                        src            += to_do * 3;
                        dst            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_4X2:
                case OM_LANCZOS_4X3:
                case OM_LANCZOS_4X4:
                case OM_LANCZOS_4X12BIT:
                case OM_LANCZOS_4X16BIT:
                case OM_LANCZOS_4X24BIT:
                {
                    while (samples > 0)
                    {
                        // Perform filtering
                        size_t can_do   = OS_DOWN_BUFFER_SIZE >> 2;
                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        if (bFilter)
                        {
                            sFilter.process(fDownBuffer, src, to_do << 2);
                            dsp::downsample_4x(dst, fDownBuffer, to_do);
                        }
                        else
                            dsp::downsample_4x(dst, src, to_do);

                        // Update pointers
                        src            += to_do << 2;
                        dst            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_6X2:
                case OM_LANCZOS_6X3:
                case OM_LANCZOS_6X4:
                case OM_LANCZOS_6X12BIT:
                case OM_LANCZOS_6X16BIT:
                case OM_LANCZOS_6X24BIT:
                {
                    while (samples > 0)
                    {
                        // Perform filtering
                        size_t can_do   = OS_DOWN_BUFFER_SIZE / 6;
                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Pack samples to dst
                        if (bFilter)
                        {
                            sFilter.process(fDownBuffer, src, to_do * 6);
                            dsp::downsample_6x(dst, fDownBuffer, to_do);
                        }
                        else
                            dsp::downsample_6x(dst, src, to_do);

                        // Update pointers
                        src            += to_do * 6;
                        dst            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_8X2:
                case OM_LANCZOS_8X3:
                case OM_LANCZOS_8X4:
                case OM_LANCZOS_8X12BIT:
                case OM_LANCZOS_8X16BIT:
                case OM_LANCZOS_8X24BIT:
                {
                    while (samples > 0)
                    {
                        // Perform filtering
                        size_t can_do   = OS_DOWN_BUFFER_SIZE >> 3;
                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Pack samples to dst
                        if (bFilter)
                        {
                            sFilter.process(fDownBuffer, src, to_do << 3);
                            dsp::downsample_8x(dst, fDownBuffer, to_do);
                        }
                        else
                            dsp::downsample_8x(dst, src, to_do);

                        // Update pointers
                        src            += to_do << 3;
                        dst            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_NONE:
                default:
                    dsp::copy(dst, src, samples);
                    break;
            }
        }

        void Oversampler::process(float *dst, const float *src, size_t samples, IOversamplerCallback *callback)
        {
            switch (nMode)
            {
                case OM_LANCZOS_2X2:
                case OM_LANCZOS_2X3:
                case OM_LANCZOS_2X4:
                case OM_LANCZOS_2X12BIT:
                case OM_LANCZOS_2X16BIT:
                case OM_LANCZOS_2X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) >> 1;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE >> 1;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);

                        // Call handler
                        if (callback != NULL)
                            callback->process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do << 1);

                        // Do downsampling
                        if (bFilter)
                            sFilter.process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do << 1);
                        dsp::downsample_2x(dst, &fUpBuffer[nUpHead], to_do);

                        // Update pointers
                        nUpHead        += to_do << 1;
                        dst            += to_do << 1;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_3X2:
                case OM_LANCZOS_3X3:
                case OM_LANCZOS_3X4:
                case OM_LANCZOS_3X12BIT:
                case OM_LANCZOS_3X16BIT:
                case OM_LANCZOS_3X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) / 3;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE / 3;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);

                        // Call handler
                        if (callback != NULL)
                            callback->process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do * 3);

                        // Do downsampling
                        if (bFilter)
                            sFilter.process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do * 3);
                        dsp::downsample_3x(dst, &fUpBuffer[nUpHead], to_do);

                        // Update pointers
                        nUpHead        += to_do * 3;
                        dst            += to_do;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_4X2:
                case OM_LANCZOS_4X3:
                case OM_LANCZOS_4X4:
                case OM_LANCZOS_4X12BIT:
                case OM_LANCZOS_4X16BIT:
                case OM_LANCZOS_4X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) >> 2;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE >> 2;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);

                        // Call handler
                        if (callback != NULL)
                            callback->process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do << 2);

                        // Do downsampling
                        if (bFilter)
                            sFilter.process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do << 2);
                        dsp::downsample_4x(dst, &fUpBuffer[nUpHead], to_do);

                        // Update pointers
                        nUpHead        += to_do << 2;
                        dst            += to_do;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_6X2:
                case OM_LANCZOS_6X3:
                case OM_LANCZOS_6X4:
                case OM_LANCZOS_6X12BIT:
                case OM_LANCZOS_6X16BIT:
                case OM_LANCZOS_6X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) / 6;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE / 6;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);

                        // Call handler
                        if (callback != NULL)
                            callback->process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do * 6);

                        // Do downsampling
                        if (bFilter)
                            sFilter.process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do * 6);
                        dsp::downsample_6x(dst, &fUpBuffer[nUpHead], to_do);

                        // Update pointers
                        nUpHead        += to_do * 6;
                        dst            += to_do;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_LANCZOS_8X2:
                case OM_LANCZOS_8X3:
                case OM_LANCZOS_8X4:
                case OM_LANCZOS_8X12BIT:
                case OM_LANCZOS_8X16BIT:
                case OM_LANCZOS_8X24BIT:
                {
                    while (samples > 0)
                    {
                        // Check that there is enough space in buffer
                        size_t can_do   = (OS_UP_BUFFER_SIZE - nUpHead) >> 3;
                        if (can_do <= 0)
                        {
                            dsp::move(fUpBuffer, &fUpBuffer[nUpHead], LSP_DSP_RESAMPLING_RSV_SAMPLES);
                            dsp::fill_zero(&fUpBuffer[LSP_DSP_RESAMPLING_RSV_SAMPLES], OS_UP_BUFFER_SIZE);
                            nUpHead         = 0;
                            can_do          = OS_UP_BUFFER_SIZE >> 3;
                        }

                        size_t to_do    = (samples > can_do) ? can_do : samples;

                        // Do oversampling
                        pFunc(&fUpBuffer[nUpHead], src, to_do);

                        // Call handler
                        if (callback != NULL)
                            callback->process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do << 3);

                        // Do downsampling
                        if (bFilter)
                            sFilter.process(&fUpBuffer[nUpHead], &fUpBuffer[nUpHead], to_do << 3);
                        dsp::downsample_8x(dst, &fUpBuffer[nUpHead], to_do);

                        // Update pointers
                        nUpHead        += to_do << 3;
                        dst            += to_do;
                        src            += to_do;
                        samples        -= to_do;
                    }
                    break;
                }

                case OM_NONE:
                default:
                    if (callback != NULL)
                        callback->process(dst, src, samples);
                    else
                        dsp::copy(dst, src, samples);
                    break;
            }
        }

        size_t Oversampler::latency() const
        {
            switch (nMode)
            {
                case OM_LANCZOS_2X2:
                case OM_LANCZOS_3X2:
                case OM_LANCZOS_4X2:
                case OM_LANCZOS_6X2:
                case OM_LANCZOS_8X2:
                    return 2;

                case OM_LANCZOS_2X3:
                case OM_LANCZOS_3X3:
                case OM_LANCZOS_4X3:
                case OM_LANCZOS_6X3:
                case OM_LANCZOS_8X3:
                    return 3;

                case OM_LANCZOS_2X4:
                case OM_LANCZOS_3X4:
                case OM_LANCZOS_4X4:
                case OM_LANCZOS_6X4:
                case OM_LANCZOS_8X4:
                    return 4;

                case OM_LANCZOS_2X12BIT:
                case OM_LANCZOS_3X12BIT:
                case OM_LANCZOS_4X12BIT:
                case OM_LANCZOS_6X12BIT:
                case OM_LANCZOS_8X12BIT:
                    return 4;

                case OM_LANCZOS_2X16BIT:
                case OM_LANCZOS_3X16BIT:
                case OM_LANCZOS_4X16BIT:
                case OM_LANCZOS_6X16BIT:
                case OM_LANCZOS_8X16BIT:
                    return 10;

                case OM_LANCZOS_2X24BIT:
                case OM_LANCZOS_3X24BIT:
                case OM_LANCZOS_4X24BIT:
                case OM_LANCZOS_6X24BIT:
                case OM_LANCZOS_8X24BIT:
                    return 62;

                default:
                    break;
            }

            return 0;
        }

        Oversampler::resample_func_t Oversampler::get_function(size_t mode)
        {
            switch (mode)
            {
                case OM_LANCZOS_2X2:        return dsp::lanczos_resample_2x2;
                case OM_LANCZOS_2X3:        return dsp::lanczos_resample_2x3;
                case OM_LANCZOS_2X4:        return dsp::lanczos_resample_2x4;
                case OM_LANCZOS_2X12BIT:    return dsp::lanczos_resample_2x12bit;
                case OM_LANCZOS_2X16BIT:    return dsp::lanczos_resample_2x16bit;
                case OM_LANCZOS_2X24BIT:    return dsp::lanczos_resample_2x24bit;

                case OM_LANCZOS_3X2:        return dsp::lanczos_resample_3x2;
                case OM_LANCZOS_3X3:        return dsp::lanczos_resample_3x3;
                case OM_LANCZOS_3X4:        return dsp::lanczos_resample_3x4;
                case OM_LANCZOS_3X12BIT:    return dsp::lanczos_resample_3x12bit;
                case OM_LANCZOS_3X16BIT:    return dsp::lanczos_resample_3x16bit;
                case OM_LANCZOS_3X24BIT:    return dsp::lanczos_resample_3x24bit;

                case OM_LANCZOS_4X2:        return dsp::lanczos_resample_4x2;
                case OM_LANCZOS_4X3:        return dsp::lanczos_resample_4x3;
                case OM_LANCZOS_4X4:        return dsp::lanczos_resample_4x4;
                case OM_LANCZOS_4X12BIT:    return dsp::lanczos_resample_4x12bit;
                case OM_LANCZOS_4X16BIT:    return dsp::lanczos_resample_4x16bit;
                case OM_LANCZOS_4X24BIT:    return dsp::lanczos_resample_4x24bit;

                case OM_LANCZOS_6X2:        return dsp::lanczos_resample_6x2;
                case OM_LANCZOS_6X3:        return dsp::lanczos_resample_6x3;
                case OM_LANCZOS_6X4:        return dsp::lanczos_resample_6x4;
                case OM_LANCZOS_6X12BIT:    return dsp::lanczos_resample_6x12bit;
                case OM_LANCZOS_6X16BIT:    return dsp::lanczos_resample_6x16bit;
                case OM_LANCZOS_6X24BIT:    return dsp::lanczos_resample_6x24bit;

                case OM_LANCZOS_8X2:        return dsp::lanczos_resample_8x2;
                case OM_LANCZOS_8X3:        return dsp::lanczos_resample_8x3;
                case OM_LANCZOS_8X4:        return dsp::lanczos_resample_8x4;
                case OM_LANCZOS_8X12BIT:    return dsp::lanczos_resample_8x12bit;
                case OM_LANCZOS_8X16BIT:    return dsp::lanczos_resample_8x16bit;
                case OM_LANCZOS_8X24BIT:    return dsp::lanczos_resample_8x24bit;

                default:
                    break;
            }

            return dsp::copy;
        }


        void Oversampler::set_mode(over_mode_t mode)
        {
            if (mode < OM_NONE)
                mode = OM_NONE;
            else if (mode > OM_LANCZOS_8X24BIT)
                mode = OM_LANCZOS_8X24BIT;
            if (nMode == mode)
                return;
            nMode       = mode;
            pFunc       = get_function(mode);

            nUpdate   |= UP_MODE;
        }

        void Oversampler::dump(IStateDumper *v) const
        {
            v->write("pCallback", pCallback);
            v->write("fUpBuffer", fUpBuffer);
            v->write("fDownBuffer", fDownBuffer);
            v->write("pFunc", pFunc);
            v->write("nUpHead", nUpHead);
            v->write("nMode", nMode);
            v->write("nSampleRate", nSampleRate);
            v->write("nUpdate", nUpdate);
            v->write_object("sFilter", &sFilter);
            v->write("bData", bData);
            v->write("bFilter", bFilter);
        }

    } /* namespace dspu */
} /* namespace lsp */
