/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 12 мая 2017 г.
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

#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/filters/Filter.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/mm/InAudioFileStream.h>
#include <lsp-plug.in/mm/OutAudioFileStream.h>
#include <lsp-plug.in/stdlib/math.h>

#define BUFFER_FRAMES           4096
#define RESAMPLING_PERIODS      8

namespace lsp
{
    namespace dspu
    {
        static size_t gcd_euclid(size_t a, size_t b)
        {
            while (b)
            {
                size_t c = a % b;
                a = b;
                b = c;
            }
            return a;
        }

        Sample::Sample()
        {
            construct();
        }

        Sample::~Sample()
        {
            destroy();
        }

        void Sample::construct()
        {
            vBuffer     = NULL;
            nSampleRate = 0;
            nLength     = 0;
            nMaxLength  = 0;
            nChannels   = 0;
        }

        void Sample::destroy()
        {
            if (vBuffer != NULL)
            {
                free(vBuffer);
                vBuffer     = NULL;
            }
            nMaxLength      = 0;
            nLength         = 0;
            nChannels       = 0;
        }

        bool Sample::init(size_t channels, size_t max_length, size_t length)
        {
            if ((channels <= 0) || (length > max_length))
                return false;

            // Allocate new data
            size_t len      = lsp_max(max_length, size_t(DEFAULT_ALIGN));
            size_t cap      = align_size(len, DEFAULT_ALIGN);       // Make multiple of 4
            float *buf      = static_cast<float *>(::malloc(cap * channels * sizeof(float)));
            if (buf == NULL)
                return false;
            dsp::fill_zero(buf, cap * channels);

            // Destroy previous data
            if (vBuffer != NULL)
                free(vBuffer);

            vBuffer         = buf;
            nLength         = length;
            nMaxLength      = cap;
            nChannels       = channels;
            return true;
        }

        status_t Sample::copy(const Sample *s)
        {
            if ((s->nChannels <= 0) ||
                (s->nLength > s->nMaxLength) ||
                (s->vBuffer == NULL))
                return STATUS_BAD_STATE;

            // Allocate new data
            size_t len      = lsp_max(s->nLength, size_t(DEFAULT_ALIGN));
            size_t cap      = align_size(len, DEFAULT_ALIGN);       // Make multiple of 4
            float *buf      = static_cast<float *>(::malloc(cap * s->nChannels * sizeof(float)));
            if (buf == NULL)
                return STATUS_NO_MEM;

            // Copy data from the original sample
            for (size_t i=0; i<s->nChannels; ++i)
            {
                dsp::copy(&buf[i*cap], &s->vBuffer[i * s->nMaxLength], s->nLength);
                dsp::fill_zero(&buf[i*cap + s->nLength], cap - s->nLength);
            }

            // Destroy previous data
            if (vBuffer != NULL)
                free(vBuffer);

            vBuffer         = buf;
            nSampleRate     = s->nSampleRate;
            nLength         = s->nLength;
            nMaxLength      = cap;
            nChannels       = s->nChannels;

            return STATUS_OK;
        }

        bool Sample::resize(size_t channels, size_t max_length, size_t length)
        {
            if (channels <= 0)
                return false;

            // Allocate new data
            max_length      = align_size(max_length, DEFAULT_ALIGN);    // Make multiple of 4
            float *buf      = reinterpret_cast<float *>(::malloc(max_length * channels * sizeof(float)));
            if (buf == NULL)
                return false;

            // Copy previously allocated data
            if (vBuffer != NULL)
            {
                float *dptr         = buf;
                const float *sptr   = vBuffer;
                size_t to_copy      = (nMaxLength > max_length) ? max_length : nMaxLength;

                // Copy channels
                for (size_t ch=0; ch < channels; ++ch)
                {
                    if (ch < nChannels)
                    {
                        // Copy data and clear data
                        dsp::copy(dptr, sptr, to_copy);
                        dsp::fill_zero(&dptr[to_copy], max_length - to_copy);
                        sptr           += nMaxLength;
                    }
                    else
                        dsp::fill_zero(dptr, max_length);

                    dptr           += max_length;
                }

                // Destroy previously allocated data
                if (vBuffer != NULL)
                    free(vBuffer);
            }
            else
                dsp::fill_zero(buf, max_length * channels);

            vBuffer         = buf;
            nLength         = length;
            nMaxLength      = max_length;
            nChannels       = channels;
            return true;
        }

        void Sample::swap(Sample *dst)
        {
            lsp::swap(vBuffer, dst->vBuffer);
            lsp::swap(nSampleRate, dst->nSampleRate);
            lsp::swap(nMaxLength, dst->nMaxLength);
            lsp::swap(nLength, dst->nLength);
            lsp::swap(nChannels, dst->nChannels);
        }

        ssize_t Sample::save_range(const char *path, size_t offset, ssize_t count)
        {
            io::Path p;
            status_t res = p.set(path);
            return (res == STATUS_OK) ? save_range(&p, offset, count) : res;
        }

        ssize_t Sample::save_range(const LSPString *path, size_t offset, ssize_t count)
        {
            io::Path p;
            status_t res = p.set(path);
            return (res == STATUS_OK) ? save_range(&p, offset, count) : res;
        }

        ssize_t Sample::save_range(const io::Path *path, size_t offset, ssize_t count)
        {
            if ((nSampleRate <= 0) || (nChannels < 0))
                return -STATUS_BAD_STATE;

            ssize_t avail   = lsp_max(ssize_t(nLength - offset), 0);
            count           = (count < 0) ? avail : lsp_min(count, avail);

            mm::OutAudioFileStream os;
            mm::audio_stream_t fmt;

            fmt.srate       = nSampleRate;
            fmt.channels    = nChannels;
            fmt.frames      = count;
            fmt.format      = mm::SFMT_F32;

            status_t res    = os.open(path, &fmt, mm::AFMT_WAV | mm::CFMT_PCM);
            if (res != STATUS_OK)
            {
                os.close();
                return res;
            }

            ssize_t written = save_range(&os, offset, count);
            if (written < 0)
            {
                os.close();
                return -written;
            }

            res             = os.close();
            return (res == STATUS_OK) ? written : -res;
        }

        ssize_t Sample::save_range(mm::IOutAudioStream *out, size_t offset, ssize_t count)
        {
            if ((nSampleRate <= 0) || (nChannels < 0))
                return -STATUS_BAD_STATE;

            ssize_t avail   = lsp_max(ssize_t(nLength - offset), 0);
            count           = (count < 0) ? avail : lsp_min(count, avail);
            size_t written  = 0;
            if (count <= 0)
                return written;

            // Allocate temporary buffer for writes
            size_t bufsize  = lsp_min(count, BUFFER_FRAMES);
            uint8_t *data   = NULL;
            float *buf      = alloc_aligned<float>(data, nChannels * bufsize);
            if (buf == NULL)
                return STATUS_NO_MEM;

            // Perform writes to underlying stream
            while (count > 0)
            {
                // Generate frame data
                size_t to_do    = lsp_min(count, BUFFER_FRAMES);
                for (size_t i=0; i<nChannels; ++i)
                {
                    const float *src    = &vBuffer[i * nMaxLength + offset];
                    float *dst          = &buf[i];
                    for (size_t j=0; j<to_do; ++j, ++src, dst += nChannels)
                        *dst    = *src;
                }

                // Write data to output stream
                ssize_t nframes = out->write(buf, to_do);
                if (nframes < 0)
                {
                    if (written > 0)
                        break;
                    free_aligned(data);
                    return nframes;
                }

                // Update position
                written        += nframes;
                offset         += nframes;
                count          -= nframes;
            }

            free_aligned(data);
            return written;
        }

        status_t Sample::load(const char *path, float max_duration)
        {
            io::Path p;
            status_t res = p.set(path);
            return (res == STATUS_OK) ? load(&p, max_duration) : res;
        }

        status_t Sample::load(const LSPString *path, float max_duration)
        {
            io::Path p;
            status_t res = p.set(path);
            return (res == STATUS_OK) ? load(&p, max_duration) : res;
        }

        status_t Sample::load(const io::Path *path, float max_duration)
        {
            mm::InAudioFileStream in;
            status_t res = in.open(path);
            if (res != STATUS_OK)
            {
                in.close();
                return res;
            }

            res = load(&in, max_duration);
            if (res != STATUS_OK)
            {
                in.close();
                return res;
            }

            return in.close();
        }

        status_t Sample::load(mm::IInAudioStream *in, float max_duration)
        {
            mm::audio_stream_t fmt;
            status_t res = in->info(&fmt);
            if (res != STATUS_OK)
                return res;

            ssize_t samples = (max_duration >= 0.0f) ? fmt.srate * max_duration : -1.0f;
            return loads(in, samples);
        }

        status_t Sample::loads(const char *path, ssize_t max_samples)
        {
            io::Path p;
            status_t res = p.set(path);
            return (res == STATUS_OK) ? loads(&p, max_samples) : res;
        }

        status_t Sample::loads(const LSPString *path, ssize_t max_samples)
        {
            io::Path p;
            status_t res = p.set(path);
            return (res == STATUS_OK) ? loads(&p, max_samples) : res;
        }

        status_t Sample::loads(const io::Path *path, ssize_t max_samples)
        {
            mm::InAudioFileStream in;
            status_t res = in.open(path);
            if (res != STATUS_OK)
            {
                in.close();
                return res;
            }

            res = loads(&in, max_samples);
            if (res != STATUS_OK)
            {
                in.close();
                return res;
            }

            return in.close();
        }

        status_t Sample::loads(mm::IInAudioStream *in, ssize_t max_samples)
        {
            mm::audio_stream_t fmt;
            status_t res = in->info(&fmt);
            if (res != STATUS_OK)
                return res;

            // Allocate temporary sample to perform read
            ssize_t count = (max_samples < 0) ? fmt.frames : lsp_min(fmt.frames, max_samples);
            size_t offset = 0;
            Sample tmp;
            if (!tmp.init(fmt.channels, count, count))
                return STATUS_NO_MEM;

            // Allocate temporary buffer for reads
            size_t bufsize  = lsp_min(count, BUFFER_FRAMES);
            uint8_t *data   = NULL;
            float *buf      = alloc_aligned<float>(data, fmt.channels * bufsize);
            if (buf == NULL)
                return STATUS_NO_MEM;

            // Perform reads from underlying stream
            while (count > 0)
            {
                // Read data from input stream
                size_t to_do    = lsp_min(count, BUFFER_FRAMES);
                ssize_t nframes = in->read(buf, to_do);
                if (nframes < 0)
                    return -nframes;

                // Unpack buffer
                for (size_t i=0; i<fmt.channels; ++i)
                {
                    const float *src    = &buf[i];
                    float *dst          = &tmp.vBuffer[i * tmp.nMaxLength + offset];
                    for (size_t j=0; j<to_do; ++j, src += fmt.channels, ++dst)
                        *dst    = *src;
                }

                // Update position
                offset         += nframes;
                count          -= nframes;
            }

            // All is OK, free temporary buffer and swap self state with the temporary sample
            free_aligned(data);
            tmp.set_sample_rate(fmt.srate);
            tmp.swap(this);

            return STATUS_OK;
        }

        bool Sample::reverse(size_t channel)
        {
            if (channel >= nChannels)
                return false;

            dsp::reverse1(&vBuffer[channel * nMaxLength], nLength);
            return true;
        }

        void Sample::reverse()
        {
            float *dst = vBuffer;
            for (size_t i=0; i<nChannels; ++i)
            {
                dsp::reverse1(dst, nLength);
                dst     += nMaxLength;
            }
        }

        status_t Sample::fast_downsample(Sample *s, size_t new_sample_rate)
        {
            size_t rkf          = nSampleRate / new_sample_rate;
            size_t new_samples  = nLength / rkf;

            // Allocate sample
            if (!s->init(nChannels, new_samples, new_samples))
                return STATUS_NO_MEM;
            s->set_sample_rate(new_sample_rate);

            // Iterate each channel
            for (size_t c=0; c<nChannels; ++c)
            {
                const float *src    = &vBuffer[c * nMaxLength];
                float *dst          = &s->vBuffer[c * new_samples];

                for (size_t i=0; i < new_samples; ++i, src += rkf, ++dst)
                    *dst                = *src;
            }

            return STATUS_OK;
        }

        status_t Sample::fast_upsample(Sample *s, size_t new_sample_rate)
        {
            // Calculate parameters of transformation
            ssize_t kf          = new_sample_rate / nSampleRate;
            float rkf           = 1.0f / kf;

            // Prepare kernel for resampling
            ssize_t k_periods   = RESAMPLING_PERIODS; // * (kf >> 1);
            ssize_t k_base      = k_periods * kf;
            ssize_t k_center    = k_base + 1;
            ssize_t k_len       = (k_center << 1) + 1;
            ssize_t k_size      = align_size(k_len + 1, 4); // Additional sample for time offset
            float *k            = static_cast<float *>(malloc(sizeof(float) * k_size));
            if (k == NULL)
                return STATUS_NO_MEM;

            // Estimate resampled sample size
            size_t new_samples  = kf * nLength;
            size_t b_len        = new_samples + k_size;

            // Prepare new data structure to store resampled data
            if (!s->init(nChannels, b_len, b_len))
            {
                free(k);
                return STATUS_NO_MEM;
            }
            s->set_sample_rate(new_sample_rate);

            // Generate Lanczos kernel
            for (ssize_t j=0; j<k_size; ++j)
            {
                float t         = (j - k_center) * rkf;

                if ((t > -k_periods) && (t < k_periods))
                {
                    float t2    = M_PI * t;
                    k[j]        = (t != 0) ? k_periods * sinf(t2) * sinf(t2 / k_periods) / (t2 * t2) : 1.0f;
                }
                else
                    k[j]        = 0.0f;
            }

            // Iterate each channel
            for (size_t c=0; c<nChannels; ++c)
            {
                const float *src    = &vBuffer[c * nMaxLength];
                float *dst          = &s->vBuffer[c * s->nMaxLength];

                // Perform convolutions
                for (size_t i=0, p=0; i<nLength; i++, p += kf)
                    dsp::fmadd_k3(&dst[p], k, src[i], k_size);

                dsp::move(dst, &dst[k_center], s->nLength - k_center);
            }

            // Delete temporary buffer and decrease length of sample
            free(k);
            s->nLength -= k_size;

            return STATUS_OK;
        }

        status_t Sample::complex_upsample(Sample *s, size_t new_sample_rate)
        {
            // Calculate parameters of transformation
            ssize_t gcd         = gcd_euclid(new_sample_rate, nSampleRate);
            ssize_t src_step    = nSampleRate / gcd;
            ssize_t dst_step    = new_sample_rate / gcd;
            float kf            = float(dst_step) / float(src_step);
            float rkf           = float(src_step) / float(dst_step);

            // Prepare kernel for resampling
            ssize_t k_periods   = RESAMPLING_PERIODS; // Number of periods
            ssize_t k_base      = k_periods * kf;
            ssize_t k_center    = k_base + 1;
            ssize_t k_len       = (k_center << 1) + 1; // Centered impulse response
            ssize_t k_size      = align_size(k_len + 1, 4); // Additional sample for time offset
            float *k            = static_cast<float *>(malloc(sizeof(float) * k_size));
            if (k == NULL)
                return STATUS_NO_MEM;

            // Estimate resampled sample size
            size_t new_samples  = kf * nLength;
            size_t b_len        = new_samples + k_size;

            // Prepare new data structure to store resampled data
            if (!s->init(nChannels, b_len, b_len))
            {
                free(k);
                return STATUS_NO_MEM;
            }
            s->set_sample_rate(new_sample_rate);

            // Iterate each channel
            for (size_t c=0; c<nChannels; ++c)
            {
                const float *src    = &vBuffer[c * nMaxLength];
                float *dst          = &s->vBuffer[c * s->nMaxLength];

                for (ssize_t i=0; i<src_step; ++i)
                {
                    // calculate the offset between nearest samples
                    ssize_t p       = kf * i;
                    float dt        = i*kf - p;

                    // Generate Lanczos kernel
                    for (ssize_t j=0; j<k_size; ++j)
                    {
                        float t         = (j - k_center - dt) * rkf;

                        if ((t > -k_periods) && (t < k_periods))
                        {
                            float t2    = M_PI * t;
                            k[j]        = (t != 0.0f) ? k_periods * sinf(t2) * sinf(t2 / k_periods) / (t2 * t2) : 1.0f;
                        }
                        else
                            k[j]        = 0.0f;
                    }

                    // Perform convolutions
                    for (size_t j=i; j<nLength; j += src_step)
                    {
                        dsp::fmadd_k3(&dst[p], k, src[j], k_size);
                        p   += dst_step;
                    }
                }

                // Copy the data to the file content
                dsp::move(dst, &dst[k_center], s->nLength - k_center);
            }

            // Delete temporary buffer and decrease length of sample
            free(k);
            s->nLength -= k_size;

            return STATUS_OK;
        }

        status_t Sample::complex_downsample(Sample *s, size_t new_sample_rate)
        {
            // Calculate parameters of transformation
            ssize_t gcd         = gcd_euclid(new_sample_rate, nSampleRate);
            ssize_t src_step    = nSampleRate / gcd;
            ssize_t dst_step    = new_sample_rate / gcd;
            float kf            = float(dst_step) / float(src_step);
            float rkf           = float(src_step) / float(dst_step);

            // Prepare kernel for resampling
            ssize_t k_base      = RESAMPLING_PERIODS;
            ssize_t k_periods   = k_base * rkf; // Number of periods
            ssize_t k_center    = k_base + 1;
            ssize_t k_len       = (k_center << 1) + rkf + 1; // Centered impulse response
            ssize_t k_size      = align_size(k_len + 1, 4); // Additional sample for time offset
            float *k            = static_cast<float *>(malloc(sizeof(float) * k_size));
            if (k == NULL)
                return STATUS_NO_MEM;

            // Estimate resampled sample size
            size_t new_samples  = kf * nLength;
            size_t b_len        = new_samples + k_size;

            // Prepare new data structure to store resampled data
            if (!s->init(nChannels, b_len, b_len))
            {
                free(k);
                return STATUS_NO_MEM;
            }
            s->set_sample_rate(new_sample_rate);

            // Iterate each channel
            for (size_t c=0; c<nChannels; ++c)
            {
                const float *src    = &vBuffer[c * nMaxLength];
                float *dst          = &s->vBuffer[c * s->nMaxLength];

                for (ssize_t i=0; i<src_step; ++i)
                {
                    // calculate the offset between nearest samples
                    ssize_t p       = kf * i;
                    float dt        = i*kf - p; // Always positive, in range of [0..1]

                    // Generate Lanczos kernel
                    for (ssize_t j=0; j<k_size; ++j)
                    {
                        float t         = (j - k_center - dt) * rkf;

                        if ((t > -k_periods) && (t < k_periods))
                        {
                            float t2    = M_PI * t;
                            k[j]        = (t != 0.0f) ? k_periods * sinf(t2) * sinf(t2 / k_periods) / (t2 * t2) : 1.0f;
                        }
                        else
                            k[j]        = 0.0f;
                    }

                    // Perform convolutions
                    for (size_t j=i; j<nLength; j += src_step)
                    {
                        dsp::fmadd_k3(&dst[p], k, src[j], k_size);
                        p   += dst_step;
                    }
                }

                // Copy the data to the file content
                dsp::move(dst, &dst[k_center], s->nLength - k_center);
            }

            // Delete temporary buffer and decrease length of sample
            free(k);
            s->nLength -= k_size;

            return STATUS_OK;
        }

        status_t Sample::resample(size_t new_sample_rate)
        {
            if (nChannels <= 0)
                return STATUS_BAD_STATE;

            Sample tmp;
            status_t res    = STATUS_OK;

            // Check that resampling is actually needed
            if (new_sample_rate > nSampleRate)
            {
                // Need to up-sample data
                res = ((new_sample_rate % nSampleRate) == 0) ?
                        fast_upsample(&tmp, new_sample_rate) :
                        complex_upsample(&tmp, new_sample_rate);
            }
            else if (new_sample_rate < nSampleRate)
            {
                // Step 1: prepare temporary sample and filter
                Sample ff;
                Filter flt;
                filter_params_t fp;

                fp.nType    = FLT_BT_LRX_LOPASS;
                fp.fFreq    = new_sample_rate * 0.475f;
                fp.fFreq2   = fp.fFreq;
                fp.fGain    = 1.0f;
                fp.nSlope   = 4;
                fp.fQuality = 0.75f;

                if (!flt.init(NULL))
                    return STATUS_NO_MEM;
                if (!ff.init(nChannels, nLength, nLength))
                    return STATUS_NO_MEM;

                ff.set_sample_rate(nSampleRate);
                flt.update(nSampleRate, &fp);

                // Step 2: remove all frequencies above new nyquist frequency
                for (size_t c=0; c<nChannels; ++c)
                {
                    const float *src    = &vBuffer[c * nMaxLength];
                    float *dst          = &ff.vBuffer[c * ff.nMaxLength];
                    flt.clear();
                    flt.process(dst, src, nLength);
                }

                // Need to down-sample data of the pre-filtered sample
                res = ((nSampleRate % new_sample_rate) == 0) ?
                        ff.fast_downsample(&tmp, new_sample_rate) :
                        ff.complex_downsample(&tmp, new_sample_rate);
            }
            else
                return STATUS_OK; // Sample rate matches

            // Replace content
            if (res == STATUS_OK)
                tmp.swap(this);

            // Return OK status
            return STATUS_OK;
        }

        void Sample::dump(IStateDumper *v) const
        {
            v->write("vBuffer", vBuffer);
            v->write("nSampleRate", nSampleRate);
            v->write("nLength", nLength);
            v->write("nMaxLength", nMaxLength);
            v->write("nChannels", nChannels);
        }
    }
} /* namespace lsp */
