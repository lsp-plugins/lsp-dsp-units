/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/finally.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/filters/Filter.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/fmt/lspc/File.h>
#include <lsp-plug.in/fmt/lspc/lspc.h>
#include <lsp-plug.in/fmt/lspc/util.h>
#include <lsp-plug.in/fmt/sfz/DocumentProcessor.h>
#include <lsp-plug.in/fmt/sfz/IDocumentHandler.h>
#include <lsp-plug.in/io/OutFileStream.h>
#include <lsp-plug.in/mm/InAudioFileStream.h>
#include <lsp-plug.in/mm/OutAudioFileStream.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr size_t BUFFER_FRAMES       = 0x1000;
        static constexpr float  RESAMPLING_KPERIODS = 32.0f;
        static constexpr float  RESAMPLING_RPERIODS = 1.0f / RESAMPLING_KPERIODS;
        static constexpr float  RESAMPLING_PI       = M_PI;

        namespace
        {
            // SFZ file handler
            class SFZHandler: public sfz::IDocumentHandler
            {
                private:
                    LSPString                   sFileName;
                    io::Path                    sBaseDir;
                    const io::Path             *pLookup;
                    io::Path                    sTempFile;

                public:
                    SFZHandler()
                    {
                        pLookup         = NULL;
                    }

                    virtual ~SFZHandler() override
                    {
                        // Remove temporary file if it is present
                        if (!sTempFile.is_empty())
                            sTempFile.remove();
                    }

                public:
                    status_t init(const io::Path *file, const io::Path *sample)
                    {
                        status_t res;

                        pLookup     = sample;

                        if ((res = file->get_parent(&sBaseDir)) != STATUS_OK)
                            return res;
                        if ((res = file->get_last(&sFileName)) != STATUS_OK)
                            return res;

                        return STATUS_OK;
                    }

                public:
                    io::Path *sample_file()
                    {
                        return &sTempFile;
                    }

                public:
                    virtual status_t sample(
                        const char *name, io::IInStream *data,
                        const char **opcodes, const char **values) override
                    {
                        io::Path path, temp;
                        LSPString last;
                        status_t res;

                        // Check that sample name matches
                        if ((res = path.set(name)) != STATUS_OK)
                            return res;
                        if ((res = path.canonicalize()) != STATUS_OK)
                            return res;
                        if (!path.equals(pLookup))
                            return STATUS_OK;

                        // Generate temporary file name and open the file
                        io::OutFileStream ofs;
                        if ((res = ofs.open_temp(&sTempFile)) != STATUS_OK)
                            return res;
                        lsp_finally { ofs.close(); };

                        // Write data to file and exit
                        wssize_t n = data->sink(&ofs);
                        if (n < 0)
                            return status_t(-n);
                        return ofs.close();
                    }

                    virtual status_t include(sfz::PullParser *parser, const char *name) override
                    {
                        io::Path child;
                        status_t res = child.set(&sBaseDir, name);
                        if (res != STATUS_OK)
                            return res;

                        lsp_trace("Processing file '%s'...", child.as_utf8());

                        return parser->open(&child);
                    }

                    virtual const char *root_file_name() override
                    {
                        return sFileName.get_utf8();
                    }
            };

            static void delete_temporary_file(void *ptr)
            {
                io::Path *path = static_cast<io::Path *>(ptr);
                lsp_trace("Remove temporary file %s", path->as_utf8());
                path->remove();
                delete path;
            }
        } /* namespace */

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
            vBuffer         = NULL;
            nSampleRate     = 0;
            nLength         = 0;
            nMaxLength      = 0;
            nChannels       = 0;
            nGcRefs         = 0;
            pGcNext         = NULL;
            pUserData       = NULL;
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
            nGcRefs         = 0;
            pGcNext         = NULL;
            pUserData       = NULL;
        }

        bool Sample::init(size_t channels, size_t length)
        {
            return init(channels, length, length);
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
            if (s == this)
                return STATUS_OK;

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
            if (length > max_length)
                return false;

            // More simplified algorithm if only length of the sample is changed
            if ((channels == nChannels) && (max_length == nMaxLength))
            {
                if (length > nLength)
                {
                    float *dptr         = &vBuffer[nLength];
                    for (size_t ch=0; ch < channels; ++ch)
                    {
                        dsp::fill_zero(dptr, length - nLength);
                        dptr           += nMaxLength;
                    }
                }

                nLength         = length;
                return true;
            }

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
                size_t to_copy      = lsp_min(max_length, nMaxLength);

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

        status_t Sample::insert(size_t pos, size_t samples)
        {
            if (samples <= 0)
                return STATUS_OK;
            if (pos > nLength)
                return STATUS_INVALID_VALUE;
            if (vBuffer == NULL)
                return STATUS_BAD_STATE;

            if ((nLength + samples) < nMaxLength)
            {
                // We can do the things in-place
                float *dptr         = vBuffer;

                // Copy channels
                for (size_t ch=0; ch < nChannels; ++ch)
                {
                    dsp::move(&dptr[pos + samples], &dptr[pos], nLength - pos);
                    dsp::fill_zero(&dptr[pos], samples);

                    dptr           += nMaxLength;
                }
            }
            else
            {
                // We can not do the things in-place, need to allocate new data
                size_t max_length   = align_size(nMaxLength + samples, DEFAULT_ALIGN);    // Make multiple of 4
                float *buf          = reinterpret_cast<float *>(::malloc(max_length * nChannels * sizeof(float)));
                if (buf == NULL)
                    return STATUS_NO_MEM;

                // Copy previously allocated data
                float *dptr         = buf;
                const float *sptr   = vBuffer;

                // Copy channels
                for (size_t ch=0; ch < nChannels; ++ch)
                {
                    dsp::copy(dptr, sptr, pos);
                    dsp::fill_zero(&dptr[pos], samples);
                    dsp::copy(&dptr[pos + samples], &sptr[pos], nLength - pos);
                    dsp::fill_zero(&dptr[nLength + samples], max_length - nLength - samples);

                    dptr           += max_length;
                    sptr           += nMaxLength;
                }

                // Destroy previously allocated data
                free(vBuffer);

                vBuffer         = buf;
                nMaxLength      = max_length;
            }

            nLength        += samples;
            return STATUS_OK;
        }

        status_t Sample::append(size_t samples)
        {
            return insert(nLength, samples);
        }

        status_t Sample::prepend(size_t samples)
        {
            return insert(0, samples);
        }

        void Sample::put_chunk_linear(float *dst, const float *src, size_t len, size_t fade_in, size_t fade_out)
        {
            // Apply the fade-in (if present)
            if (fade_in > 0)
            {
                float k = 1.0f / fade_in;
                for (size_t i=0; i<fade_in; ++i)
                    dst[i] += src[i] * (i * k);
                dst    += fade_in;
                src    += fade_in;
            }

            // Apply non-modified data
            size_t unmodified = len - fade_in - fade_out;
            if (unmodified > 0)
            {
                dsp::add2(dst, src, unmodified);
                dst    += unmodified;
                src    += unmodified;
            }

            // Apply the fade-out (if present)
            if (fade_out > 0)
            {
                float k = 1.0f / fade_out;
                for (size_t i=0; i<fade_out; ++i)
                    dst[i] += src[i] * ((fade_out - i) * k);
            }
        }

        void Sample::put_chunk_const_power(float *dst, const float *src, size_t len, size_t fade_in, size_t fade_out)
        {
            // Apply the fade-in (if present)
            if (fade_in > 0)
            {
                float k = 1.0f / fade_in;
                for (size_t i=0; i<fade_in; ++i)
                    dst[i] += src[i] * sqrtf(i * k);
                dst    += fade_in;
                src    += fade_in;
            }

            // Apply non-modified data
            size_t unmodified = len - fade_in - fade_out;
            if (unmodified > 0)
            {
                dsp::add2(dst, src, unmodified);
                dst    += unmodified;
                src    += unmodified;
            }

            // Apply the fade-out (if present)
            if (fade_out > 0)
            {
                float k = 1.0f / fade_out;
                for (size_t i=0; i<fade_out; ++i)
                    dst[i] += src[i] * sqrtf((fade_out - i) * k);
            }
        }

        status_t Sample::do_simple_stretch(size_t new_length, size_t start, size_t end, put_chunk_t put_chunk)
        {
            dspu::Sample tmp;

            // Compute the length as the beginning part + end part + stretched part
            size_t len      = start + (nLength - end) + new_length;
            if (!tmp.init(nChannels, len, len))
                return STATUS_NO_MEM;
            tmp.set_sample_rate(nSampleRate);

            // Perform stretching
            for (size_t i=0; i<nChannels; ++i)
            {
                const float *src    = channel(i);
                float *dst          = tmp.channel(i);
                float s             = (end > start) ? src[start] : 0.0f;

                dsp::copy(dst, src, start);
                dsp::fill(&dst[start], s, new_length);
                dsp::copy(&dst[start + new_length], &src[end], nLength-end);
            }

            // Swap the contents and return success
            tmp.swap(this);
            return STATUS_OK;
        }

        status_t Sample::do_single_crossfade_stretch(size_t new_length, size_t fade_len, size_t start, size_t end, put_chunk_t put_chunk)
        {
            dspu::Sample tmp;

            // Compute the length as the beginning part + end part + stretched part
            size_t len      = start + (nLength - end) + new_length;
            if (!tmp.init(nChannels, len, len))
                return STATUS_NO_MEM;
            tmp.set_sample_rate(nSampleRate);

            // Check that crossfade is not longer than new_length
            fade_len        = lsp_min(fade_len, new_length);
            // Chunks should be shorter than passed in arguments
            size_t c1_size  = (new_length + fade_len) >> 1;
            size_t c2_size  = new_length - c1_size + fade_len;

            // Perform stretching
            for (size_t i=0; i<nChannels; ++i)
            {
                const float *src    = channel(i);
                float *dst          = tmp.channel(i);

                // Copy data, fill the stretched area with zeros
                dsp::copy(dst, src, start);
                dsp::fill_zero(&dst[start], new_length);
                dsp::copy(&dst[start + new_length], &src[end], nLength-end);

                // Put two chunks onto the stretched area with crossfades applied
                put_chunk(&dst[start], &src[start], c1_size, 0, fade_len);
                put_chunk(&dst[start + new_length - c2_size], &src[end - c2_size], c2_size, fade_len, 0);
            }

            // Swap the contents and return success
            tmp.swap(this);
            return STATUS_OK;
        }

        status_t Sample::stretch(
            size_t new_length, size_t chunk_size,
            sample_crossfade_t fade_type, float fade_size,
            size_t start, size_t end)
        {
            size_t doff, soff;

            // Verify that the proper values have been submitted
            if ((start > nLength) || (end > nLength) || (start > end))
                return STATUS_BAD_ARGUMENTS;

            // What crossfade to use?
            put_chunk_t put_chunk;
            switch (fade_type)
            {
                case SAMPLE_CROSSFADE_LINEAR:       put_chunk   = put_chunk_linear; break;
                case SAMPLE_CROSSFADE_CONST_POWER:  put_chunk   = put_chunk_const_power; break;
                default:
                    return STATUS_BAD_ARGUMENTS;
            }

            // Special case: the new length of the region is equal to the length of the region to stretch
            size_t src_length   = end - start;
            if (src_length == new_length)
                return STATUS_OK;

            // Special case: the region for stretching is of not longer than 1 sample
            if (src_length <= 1)
                return do_simple_stretch(new_length, start, end, put_chunk);

            // Limit the chunk size if it is greater than source length
            // If chunk_size is zero, try to select the best chunk size from this equation:
            //   chunk_size * 2 = src_size + fade_size * chunk_size
            //   chunk_size * (2 - fade_size) = src_length
            fade_size               = lsp_limit(fade_size * 0.5f, 0.0f, 0.5f);
            if (chunk_size == 0)
                chunk_size              = src_length / (2.0f - fade_size);
            else
                chunk_size              = lsp_min(chunk_size, src_length);

            // Special case: the new length does not allow to cross-fade at least 2 chunks
            size_t fade_length      = chunk_size * fade_size;
            if ((new_length + fade_length) <= (chunk_size * 2))
                return do_single_crossfade_stretch(new_length, fade_length, start, end, put_chunk);

            // Compute the effective length of the chunk and number of chunks
            // We also need to make the new length multiple of the effective_length of the chunk
            size_t eff_chunk_len    = chunk_size - fade_length;
            size_t n_chunks         = (new_length - fade_length) / eff_chunk_len;
            size_t last_chunk_len   = new_length - n_chunks * eff_chunk_len;
            if (end <= start)
                return STATUS_UNKNOWN_ERR; // This normally should not ever happen.

            // Create new sample
            dspu::Sample tmp;
            size_t len      = nLength + new_length - src_length;
            if (!tmp.init(nChannels, len, len))
                return STATUS_NO_MEM;
            tmp.set_sample_rate(nSampleRate);

            // Perform stretching
            for (size_t i=0; i<nChannels; ++i)
            {
                const float *src    = channel(i);
                float *dst          = tmp.channel(i);

                // Copy data, fill the stretched area with zeros
                dsp::copy(dst, src, start);
                dsp::fill_zero(&dst[start], new_length);
                dsp::copy(&dst[start + new_length], &src[end], nLength-end);
                src    += start;
                dst    += start;

                // Put chunks onto the stretched area with crossfades applied
//                lsp_trace("put chunk 0 -> 0 x %d", int(chunk_size));
                put_chunk(dst, src, chunk_size, 0, fade_length); // First chunk
                for (size_t j=1; j<n_chunks; ++j)
                {
                    soff      = (j * (src_length - chunk_size)) / (n_chunks - 1);
                    doff      = j * eff_chunk_len;
//                    lsp_trace("put chunk %d -> %d x %d", int(soff), int(doff), int(chunk_size));
                    put_chunk(&dst[doff], &src[soff], chunk_size, fade_length, fade_length); // Intermediate chunk
                }
//                lsp_trace("put chunk %d -> %d x %d", int(src_length - last_chunk_len), int(new_length - last_chunk_len), int(last_chunk_len));
                put_chunk(&dst[new_length - last_chunk_len], &src[src_length - last_chunk_len], last_chunk_len, fade_length, 0); // Last chunk
            }

            // Swap the contents and return success
            tmp.swap(this);
            return STATUS_OK;
        }

        status_t Sample::stretch(size_t new_length, size_t chunk_size, sample_crossfade_t fade_type, float fade_size)
        {
            // Here we just define the stretch range as [0, nLength)
            return stretch(new_length, chunk_size, fade_type, fade_size, 0, nLength);
        }

        bool Sample::set_channels(size_t channels)
        {
            if (channels == nChannels)
                return true;
            return resize(channels, nMaxLength, nLength);
        }

        void Sample::swap(Sample *dst)
        {
            lsp::swap(vBuffer, dst->vBuffer);
            lsp::swap(nSampleRate, dst->nSampleRate);
            lsp::swap(nMaxLength, dst->nMaxLength);
            lsp::swap(nLength, dst->nLength);
            lsp::swap(nChannels, dst->nChannels);
        }

        ssize_t Sample::save_range(const char *path, size_t offset, ssize_t count) const
        {
            io::Path p;
            status_t res = p.set(path);
            return (res == STATUS_OK) ? save_range(&p, offset, count) : res;
        }

        ssize_t Sample::save_range(const LSPString *path, size_t offset, ssize_t count) const
        {
            io::Path p;
            status_t res = p.set(path);
            return (res == STATUS_OK) ? save_range(&p, offset, count) : res;
        }

        ssize_t Sample::save_range(const io::Path *path, size_t offset, ssize_t count) const
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

        ssize_t Sample::save_range(mm::IOutAudioStream *out, size_t offset, ssize_t count) const
        {
            if ((nSampleRate <= 0) || (nChannels < 0))
                return -STATUS_BAD_STATE;
            if ((out->channels() != nChannels) || (out->sample_rate() != nSampleRate))
                return STATUS_INCOMPATIBLE;

            ssize_t avail   = lsp_max(ssize_t(nLength - offset), 0);
            count           = (count < 0) ? avail : lsp_min(count, avail);
            size_t written  = 0;
            if (count <= 0)
                return written;

            // Allocate temporary buffer for writes
            size_t bufsize  = lsp_min(count, ssize_t(BUFFER_FRAMES));
            uint8_t *data   = NULL;
            float *buf      = alloc_aligned<float>(data, nChannels * bufsize);
            if (buf == NULL)
                return STATUS_NO_MEM;
            lsp_finally { free_aligned(data); };

            // Perform writes to underlying stream
            while (count > 0)
            {
                // Generate frame data
                size_t to_do    = lsp_min(count, ssize_t(BUFFER_FRAMES));
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
                    return nframes;
                }

                // Update position
                written        += nframes;
                offset         += nframes;
                count          -= nframes;
            }

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

            // Temporary sample is used to store the intermediate data
            Sample tmp;

            if (fmt.frames >= 0)
            {
                // Initialize the temporary sample
                ssize_t count = (max_samples < 0) ? fmt.frames : lsp_min(fmt.frames, max_samples);
                if (!tmp.init(fmt.channels, count, count))
                    return STATUS_NO_MEM;

                // Allocate temporary buffer for reads
                size_t bufsize  = lsp_min(count, ssize_t(BUFFER_FRAMES));
                float *buf      = reinterpret_cast<float *>(malloc(fmt.channels * bufsize * sizeof(float)));
                if (buf == NULL)
                    return STATUS_NO_MEM;
                lsp_finally { free(buf); };

                // Perform reads from underlying stream
                size_t offset = 0;
                while (count > 0)
                {
                    // Read data from input stream
                    size_t to_do    = lsp_min(count, ssize_t(BUFFER_FRAMES));
                    ssize_t nframes = in->read(buf, to_do);
                    if (nframes < 0)
                        return -status_t(nframes);

                    // Unpack buffer
                    for (size_t i=0; i<fmt.channels; ++i)
                    {
                        const float *src    = &buf[i];
                        float *dst          = &tmp.vBuffer[i * tmp.nMaxLength + offset];
                        for (ssize_t j=0; j<nframes; ++j, src += fmt.channels, ++dst)
                            *dst    = *src;
                    }

                    // Update position
                    offset         += nframes;
                    count          -= nframes;
                }
            }
            else
            {
                // We do not know number of frames, perform another read algorithm
                if (!tmp.init(fmt.channels, BUFFER_FRAMES, 0))
                    return STATUS_NO_MEM;

                // Allocate temporary buffer for reads
                float *buf      = reinterpret_cast<float *>(malloc(fmt.channels * BUFFER_FRAMES * sizeof(float)));
                if (buf == NULL)
                    return STATUS_NO_MEM;
                lsp_finally { free(buf); };

                size_t limit = (max_samples >= 0) ? max_samples : size_t(size_t(0) - 1);
                while (tmp.nLength < limit)
                {
                    // Try to read frames
                    ssize_t nframes = in->read(buf, BUFFER_FRAMES);
                    if (nframes < 0)
                    {
                        if (nframes == -STATUS_EOF)
                            break;
                        return -status_t(nframes);
                    }

                    // Ensure that we have enough space
                    if (tmp.nMaxLength < (tmp.nLength + nframes))
                    {
                        if (!tmp.resize(fmt.channels, tmp.nMaxLength*2, tmp.nLength))
                            return STATUS_NO_MEM;
                    }

                    // Unpack buffer
                    for (size_t i=0; i<fmt.channels; ++i)
                    {
                        const float *src    = &buf[i];
                        float *dst          = &tmp.vBuffer[i * tmp.nMaxLength + tmp.nLength];
                        for (ssize_t j=0; j<nframes; ++j, src += fmt.channels, ++dst)
                            *dst    = *src;
                    }

                    // Update position
                    tmp.nLength    += nframes;
                }

                // Reduce memory usage by the file
                if (!tmp.resize(fmt.channels, tmp.nLength, tmp.nLength))
                    return STATUS_NO_MEM;
            }

            // All is OK, free temporary buffer and swap self state with the temporary sample
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

        void Sample::normalize(float gain, sample_normalize_t mode)
        {
            if (mode == SAMPLE_NORM_NONE)
                return;

            // Estimate the maximum peak value for each channel
            float peak  = 0.0f;
            for (size_t i=0; i<nChannels; ++i)
            {
                float cpeak = dsp::abs_max(channel(i), nLength);
                peak        = lsp_max(peak, cpeak);
            }

            // No peak detected?
            if (peak < 1e-8)
                return;

            switch (mode)
            {
                case SAMPLE_NORM_BELOW:
                    if (peak >= gain)
                        return;
                    break;
                case SAMPLE_NORM_ABOVE:
                    if (peak <= gain)
                        return;
                    break;
                default:
                    break;
            }

            // Adjust gain
            float k = gain / peak;
            for (size_t i=0; i<nChannels; ++i)
                dsp::mul_k2(channel(i), k, nLength);
        }

        status_t Sample::apply_gain(float gain)
        {
            return apply_gain(gain, 0, nLength);
        }

        status_t Sample::apply_gain(float gain, size_t first, size_t count)
        {
            if ((first + count) > nLength)
                return STATUS_OVERFLOW;

            for (size_t i=0; i<nChannels; ++i)
                dsp::mul_k2(channel(i, first), gain, count);

            return STATUS_OK;
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
            const float rkf     = RESAMPLING_PI / kf;

            // Prepare kernel for resampling
            ssize_t k_base      = RESAMPLING_KPERIODS * kf;
            ssize_t k_center    = k_base + 1;
            ssize_t k_len       = (k_center << 1) + 1;
            ssize_t k_size      = align_size(k_len + 1, 4); // Additional sample for time offset
            float *k            = static_cast<float *>(malloc(sizeof(float) * k_size));
            if (k == NULL)
                return STATUS_NO_MEM;
            lsp_finally { free(k); };

            // Estimate resampled sample size
            size_t new_samples  = kf * nLength;
            size_t b_len        = new_samples + k_size;

            // Prepare new data structure to store resampled data
            if (!s->init(nChannels, b_len, b_len))
                return STATUS_NO_MEM;
            s->set_sample_rate(new_sample_rate);

            // Generate Lanczos kernel
            dsp::lanczos1(
                k,
                rkf, k_center * rkf,
                RESAMPLING_KPERIODS * RESAMPLING_PI, RESAMPLING_RPERIODS,
                k_size);

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
            s->nLength -= k_len;

            return STATUS_OK;
        }

        status_t Sample::complex_upsample(Sample *s, size_t new_sample_rate)
        {
            // Calculate parameters of transformation
            ssize_t gcd         = gcd_euclid(new_sample_rate, nSampleRate);
            ssize_t src_step    = nSampleRate / gcd;
            ssize_t dst_step    = new_sample_rate / gcd;
            float kf            = float(dst_step) / float(src_step);
            const float rkf     = (RESAMPLING_PI * float(src_step)) / float(dst_step);

            // Prepare kernel for resampling
            ssize_t k_base      = RESAMPLING_KPERIODS * kf;
            ssize_t k_center    = k_base + 1;
            ssize_t k_len       = (k_center << 1) + 1; // Centered impulse response
            ssize_t k_size      = align_size(k_len + 1, 4); // Additional sample for time offset
            float *k            = static_cast<float *>(malloc(sizeof(float) * k_size));
            if (k == NULL)
                return STATUS_NO_MEM;
            lsp_finally { free(k); };

            // Estimate resampled sample size
            size_t new_samples  = kf * nLength;
            size_t b_len        = new_samples + k_size;

            // Prepare new data structure to store resampled data
            if (!s->init(nChannels, b_len, b_len))
                return STATUS_NO_MEM;

            s->set_sample_rate(new_sample_rate);

            // Iterate each channel
            for (ssize_t i=0; i<src_step; ++i)
            {
                // calculate the offset between nearest samples
                const ssize_t p = kf * i;
                const float dt  = float(i)*kf - float(p);

                // Generate Lanczos kernel
                dsp::lanczos1(
                    k,
                    rkf, (k_center + dt) * rkf,
                    RESAMPLING_KPERIODS * RESAMPLING_PI, RESAMPLING_RPERIODS,
                    k_size);

                for (size_t c=0; c<nChannels; ++c)
                {
                    const float *src    = &vBuffer[c * nMaxLength];
                    float *dst          = &s->vBuffer[c * s->nMaxLength];
                    ssize_t xp          = p;

                    // Perform convolutions
                    for (size_t j=i; j<nLength; j += src_step)
                    {
                        dsp::fmadd_k3(&dst[xp], k, src[j], k_size);
                        xp             += dst_step;
                    }
                }
            }

            // Copy the data to the file content
            for (size_t c=0; c<nChannels; ++c)
            {
                float *dst          = &s->vBuffer[c * s->nMaxLength];
                dsp::move(dst, &dst[k_center], s->nLength - k_center);
            }

            // Delete temporary buffer and decrease length of sample
            s->nLength -= k_len;

            return STATUS_OK;
        }

        status_t Sample::complex_downsample(Sample *s, size_t new_sample_rate)
        {
            // Calculate parameters of transformation
            ssize_t gcd         = gcd_euclid(new_sample_rate, nSampleRate);
            ssize_t src_step    = nSampleRate / gcd;
            ssize_t dst_step    = new_sample_rate / gcd;
            float kf            = float(dst_step) / float(src_step);
            const float rkf     = (RESAMPLING_PI * float(src_step)) / float(dst_step);

            // Prepare kernel for resampling
            float k_periods     = RESAMPLING_KPERIODS * RESAMPLING_PI * rkf; // Number of periods
            ssize_t k_center    = RESAMPLING_KPERIODS + 1.0f;
            ssize_t k_len       = (k_center << 1) + rkf + 1; // Centered impulse response
            ssize_t k_size      = align_size(k_len + 1, 4); // Additional sample for time offset
            float *k            = static_cast<float *>(malloc(sizeof(float) * k_size));
            if (k == NULL)
                return STATUS_NO_MEM;
            lsp_finally { free(k); };

            // Estimate resampled sample size
            size_t new_samples  = kf * nLength;
            size_t b_len        = new_samples + k_size;

            // Prepare new data structure to store resampled data
            if (!s->init(nChannels, b_len, b_len))
                return STATUS_NO_MEM;
            s->set_sample_rate(new_sample_rate);

            // Iterate each channel
            for (ssize_t i=0; i<src_step; ++i)
            {
                // calculate the offset between nearest samples
                const ssize_t p = kf * i;
                const float dt  = float(i)*kf - float(p); // Always positive, in range of [0..1]

                // Generate Lanczos kernel
                dsp::lanczos1(
                    k,
                    rkf, (k_center + dt) * rkf,
                    k_periods, RESAMPLING_RPERIODS,
                    k_size);

                for (size_t c=0; c<nChannels; ++c)
                {
                    const float *src    = &vBuffer[c * nMaxLength];
                    float *dst          = &s->vBuffer[c * s->nMaxLength];
                    ssize_t xp          = p;

                    // Perform convolutions
                    for (size_t j=i; j<nLength; j += src_step)
                    {
                        dsp::fmadd_k3(&dst[xp], k, src[j], k_size);
                        xp                 += dst_step;
                    }
                }
            }

            // Copy the data to the file content
            for (size_t c=0; c<nChannels; ++c)
            {
                float *dst          = &s->vBuffer[c * s->nMaxLength];
                dsp::move(dst, &dst[k_center], s->nLength - k_center);
            }

            // Delete temporary buffer and decrease length of sample
            s->nLength -= k_len;

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

        status_t Sample::load_ext(const char *path, float max_duration)
        {
            io::Path tmp;
            status_t res = tmp.set(path);
            return (res == STATUS_OK) ? load_ext(&tmp, max_duration) : res;
        }

        status_t Sample::load_ext(const LSPString *path, float max_duration)
        {
            io::Path tmp;
            status_t res = tmp.set(path);
            return (res == STATUS_OK) ? load_ext(&tmp, max_duration) : res;
        }

        status_t Sample::load_ext(const io::Path *path, float max_duration)
        {
            mm::IInAudioStream *in = NULL;
            status_t res = open_stream_ext(&in, path);
            if (res != STATUS_OK)
                return res;
            res             = load(in, max_duration);
            status_t res2   = in->close();
            delete in;
            return (res != STATUS_OK) ? res : res2;
        }

        status_t Sample::loads_ext(const char *path, ssize_t max_samples)
        {
            io::Path tmp;
            status_t res = tmp.set(path);
            return (res == STATUS_OK) ? loads_ext(&tmp, max_samples) : res;
        }

        status_t Sample::loads_ext(const LSPString *path, ssize_t max_samples)
        {
            io::Path tmp;
            status_t res = tmp.set(path);
            return (res == STATUS_OK) ? loads_ext(&tmp, max_samples) : res;
        }

        status_t Sample::loads_ext(const io::Path *path, ssize_t max_samples)
        {
            mm::IInAudioStream *in = NULL;
            status_t res = open_stream_ext(&in, path);
            if (res != STATUS_OK)
                return res;

            res             = loads(in, max_samples);
            status_t res2   = in->close();
            delete in;
            return (res != STATUS_OK) ? res : res2;
        }

        status_t Sample::open_stream_ext(mm::IInAudioStream **is, const io::Path *path)
        {
            // Try to load regular file
            status_t res = try_open_regular_file(is, path);
            if (res == STATUS_OK)
                return res;
            const status_t unsuccess_result = res;

            // Now we need to walk the whole path from the end to the begin and look for archive
            LSPString item;
            io::Path parent, child;
            if ((res = parent.set(path)) != STATUS_OK)
                return res;
            if ((res = parent.canonicalize()) != STATUS_OK)
                return res;

            while (true)
            {
                // Check that there is nothing to do with
                if ((parent.is_root()) || (parent.is_empty()))
                    return unsuccess_result;

                // Remove child entry from parent and prepend for the child
                if ((res = parent.get_last(&item)) != STATUS_OK)
                    return res;
                if ((res = parent.remove_last()) != STATUS_OK)
                    return res;
                if (child.is_empty())
                {
                    if ((res = child.set(&item)) != STATUS_OK)
                        return res;
                }
                else
                {
                    if ((res = child.set_parent(&item)) != STATUS_OK)
                        return res;
                }

                // Try to read as SFZ
                if ((res = try_open_sfz(is, &parent, &child)) == STATUS_OK)
                    return res;
                // Try to read as LSPC
                if ((res = try_open_lspc(is, &parent, &child)) == STATUS_OK)
                    return res;
            }
        }

        status_t Sample::try_open_lspc(mm::IInAudioStream **is, const io::Path *lspc, const io::Path *item)
        {
            // Try to open LSPC
            lspc::File fd;
            status_t res = fd.open(lspc);
            if (res != STATUS_OK)
                return res;
            lsp_finally { fd.close(); };

            // We have LSPC file, lookup for the audio entry
            lspc::chunk_id_t *path_list = NULL;
            ssize_t path_count = fd.enumerate_chunks(LSPC_CHUNK_PATH, &path_list);
            if (path_count < 0)
                return -status_t(path_count);
            lsp_finally { free(path_list); };

            // Now iterate over all chunks and check it for match to the item
            io::Path path_item;
            size_t flags = 0;
            lspc::chunk_id_t ref_id;
            for (ssize_t i=0; i<path_count; ++i)
            {
                if ((res = lspc::read_path(path_list[i], &fd, &path_item, &flags, &ref_id)) != STATUS_OK)
                    return res;
                if (flags & lspc::PATH_DIR)
                    continue;
                if (!item->equals(&path_item))
                    continue;

                return lspc::read_audio(ref_id, &fd, is);
            }

            return STATUS_NOT_FOUND;
        }

        status_t Sample::try_open_sfz(mm::IInAudioStream **is, const io::Path *sfz, const io::Path *item)
        {
            // Try to open SFZ
            status_t res;
            SFZHandler handler;
            sfz::DocumentProcessor processor;

            // Open the document
            if ((res = processor.open(sfz)) != STATUS_OK)
                return res;
            lsp_finally { processor.close(); };

            // Init handler
            io::Path lookup;
            if ((res = lookup.set(item)) != STATUS_OK)
                return res;
            if ((res = lookup.canonicalize()) != STATUS_OK)
                return res;
            if ((res = handler.init(sfz, &lookup)) != STATUS_OK)
                return res;

            // Process the document
            if ((res = processor.process(&handler)) != STATUS_OK)
                return res;
            if ((res = processor.close()) != STATUS_OK)
                return res;

            // Try to read sample file if it is present
            if (handler.sample_file()->is_empty())
                return STATUS_NOT_FOUND;
            io::Path *p = new io::Path;
            if (p == NULL)
                return STATUS_NO_MEM;
            lsp_finally {
                if (p != NULL)
                    delete p;
            };

            // Try to read the file
            mm::IInAudioStream *ias = NULL;
            if ((res = try_open_regular_file(&ias, handler.sample_file())) != STATUS_OK)
                return res;

            // Read was successful, bind user data to the audio stream and return
            p->swap(handler.sample_file());
            ias->set_user_data(p, delete_temporary_file);
            *is = ias;
            p = NULL;
            return STATUS_OK;
        }

        status_t Sample::try_open_regular_file(mm::IInAudioStream **is, const io::Path *path)
        {
            mm::InAudioFileStream *ias = new mm::InAudioFileStream();
            if (ias == NULL)
                return STATUS_NO_MEM;

            status_t res = ias->open(path);
            if (res != STATUS_OK)
            {
                ias->close();
                delete ias;
                return res;
            }

            *is = ias;
            return STATUS_OK;
        }

        Sample *Sample::gc_link(Sample *next)
        {
            Sample *old = pGcNext;
            pGcNext     = next;
            return old;
        }

        void *Sample::set_user_data(void *user)
        {
            void *result    = pUserData;
            pUserData       = user;
            return result;
        }

        status_t Sample::remove(size_t position, size_t count)
        {
            size_t tail = position + count;
            if (tail > nLength)
                return STATUS_BAD_ARGUMENTS;

            // Cut the part of the sample
            for (size_t i=0; i<nChannels; ++i)
            {
                float *buf = channel(i);
                dsp::move(&buf[position], &buf[tail], nLength - tail);
            }
            nLength    -= count;

            return STATUS_OK;
        }

        void Sample::dump(IStateDumper *v) const
        {
            v->write("vBuffer", vBuffer);
            v->write("nSampleRate", nSampleRate);
            v->write("nLength", nLength);
            v->write("nMaxLength", nMaxLength);
            v->write("nChannels", nChannels);
            v->write("nGcRefs", nGcRefs);
            v->write("pGcNext", pGcNext);
            v->write("pUserData", pUserData);
        }
    } /* namespace dspu */
} /* namespace lsp */
