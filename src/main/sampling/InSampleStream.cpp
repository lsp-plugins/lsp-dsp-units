/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 30 окт. 2022 г.
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

#include <lsp-plug.in/dsp-units/sampling/InSampleStream.h>

namespace lsp
{
    namespace dspu
    {
        InSampleStream::InSampleStream()
        {
            pSample             = NULL;
            bDelete             = false;
        }

        InSampleStream::InSampleStream(const Sample *s, bool drop)
        {
            pSample             = s;
            bDelete             = drop;
            nOffset             = 0;
        }

        InSampleStream::~InSampleStream()
        {
            do_close();
        }

        void InSampleStream::do_close()
        {
            nOffset     = -1;
            if (pSample == NULL)
                return;
            if (bDelete)
                delete const_cast<Sample *>(pSample);
            pSample     = 0;
        }

        status_t InSampleStream::close()
        {
            do_close();
            return set_error(STATUS_OK);
        }

        status_t InSampleStream::info(mm::audio_stream_t *dst) const
        {
            if (dst == NULL)
                return STATUS_BAD_ARGUMENTS;

            if (pSample != NULL)
            {
                dst->srate          = pSample->sample_rate();
                dst->channels       = pSample->channels();
                dst->frames         = pSample->length();
                dst->format         = mm::SFMT_F32_CPU;
            }
            else
            {
                dst->srate          = 0;
                dst->channels       = 0;
                dst->frames         = -1;
                dst->format         = mm::SFMT_NONE;
            }

            return STATUS_OK;
        }

        size_t InSampleStream::sample_rate() const
        {
            return (pSample != NULL) ? pSample->sample_rate() : 0;
        }

        size_t InSampleStream::channels() const
        {
            return (pSample != NULL) ? pSample->channels() : 0;
        }

        wssize_t InSampleStream::length() const
        {
            return (pSample != NULL) ? pSample->length() : -1;
        }

        size_t InSampleStream::format() const
        {
            return (pSample != NULL) ? mm::SFMT_F32_CPU : mm::SFMT_NONE;
        }

        status_t InSampleStream::wrap(const dspu::Sample *s, bool drop)
        {
            do_close();

            pSample             = s;
            bDelete             = drop;
            nOffset             = 0;

            return STATUS_OK;
        }

        size_t InSampleStream::select_format(size_t fmt)
        {
            return mm::SFMT_F32_CPU;
        }

        ssize_t InSampleStream::direct_read(void *dst, size_t nframes, size_t fmt)
        {
            if (fmt != mm::SFMT_F32_CPU)
                return -set_error(STATUS_BAD_FORMAT);
            if (pSample == NULL)
                return -set_error(STATUS_CLOSED);

            // Here we're estimating the amount of frames that we can read
            if (nframes <= 0)
            {
                set_error(STATUS_OK);
                return 0;
            }
            size_t to_read  = lsp_min(nframes, pSample->length() - wsize_t(nOffset));
            if (to_read <= 0)
                return -set_error(STATUS_EOF);

            // Here we're packing single audio channels into frames
            size_t channels = pSample->channels();
            for (size_t c=0, nc=pSample->channels(); c < nc; ++c)
            {
                const float *sptr = pSample->channel(c) + nOffset;
                float *dptr = static_cast<float *>(dst) + c;

                for (size_t i=0; i<to_read; ++i, ++sptr, dptr += channels)
                    *dptr       = *sptr;
            }

            set_error(STATUS_OK);
            return to_read;
        }

        wssize_t InSampleStream::skip(wsize_t nframes)
        {
            if ((nOffset < 0) || (pSample == NULL))
                return -set_error(STATUS_CLOSED);

            wssize_t len = pSample->length();
            if (nOffset < len)
                nOffset    += lsp_min(nframes, wsize_t(len - nOffset));

            set_error(STATUS_OK);
            return nOffset;
        }

        wssize_t InSampleStream::seek(wsize_t nframes)
        {
            if ((nOffset < 0) || (pSample == NULL))
                return -set_error(STATUS_CLOSED);

            nOffset = lsp_limit(nframes, 0u, pSample->length());

            set_error(STATUS_OK);
            return nOffset;
        }

    } /* namespace dspu */
} /* namespace lsp */


