/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 8 мая 2024 г.
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

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/shared/SharedAudioStream.h>

namespace lsp
{
    namespace dspu
    {

        SharedAudioStream::SharedAudioStream()
        {
            construct();
        }

        SharedAudioStream::~SharedAudioStream()
        {
            destroy();
        }

        status_t SharedAudioStream::open(const char *id)
        {

        }

        status_t SharedAudioStream::open(const LSPString *id)
        {
        }

        status_t SharedAudioStream::create(const char *id, size_t channels, size_t length)
        {
        }

        status_t SharedAudioStream::create(const LSPString *id, size_t channels, size_t length)
        {
        }

        size_t SharedAudioStream::channels() const
        {
            size_t channels = (pHeader != NULL) ? pHeader->nChannels : 0;
            return lsp_min(nChannels, channels);
        }

        size_t SharedAudioStream::length() const
        {
            return (pHeader != NULL) ? pHeader->nLength : 0;
        }

        status_t SharedAudioStream::begin()
        {
            if (pHeader == NULL)
                return STATUS_BAD_STATE;
            if (bIO)
                return STATUS_BAD_STATE;

            if (bWriteMode)
            {
                uint32_t head   = pHeader->nHead;

                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];
                    c->nHead        = head;
                    c->nPosition    = head;
                    c->nAvail       = 0;
                }
            }
            else
            {
                uint32_t src_head   = pHeader->nHead;
                uint32_t head       = vChannels[0].nPosition;
                uint32_t avail      = (src_head < head) ? src_head + pHeader->nLength - head : src_head - head;

                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];
                    c->nHead        = head;
                    c->nPosition    = head;
                    c->nAvail       = avail;
                }
            }

            bIO             = true;

            return STATUS_OK;
        }

        status_t SharedAudioStream::read(size_t channel, float *dst, size_t samples)
        {
            if (pHeader == NULL)
                return STATUS_CLOSED;
            if (!bIO)
                return STATUS_BAD_STATE;
            if (bWriteMode)
                return STATUS_BAD_STATE;

            // Check that we went out of channel
            if (channel >= nChannels)
            {
                dsp::fill_zero(dst, samples);
                return STATUS_OK;
            }

            // Perform read
            channel_t *c = &vChannels[channel];
            const uint32_t length = pHeader->nLength;

            while ((samples > 0) && (c->nAvail > 0))
            {
                size_t to_read  = lsp_min(samples, c->nAvail, length - c->nPosition);
                dsp::copy(dst, &c->pData[c->nPosition], to_read);

                samples        -= to_read;
                dst            += to_read;
                c->nPosition    = (c->nPosition + to_read) % length;
            }

            // Detected buffer underrun?
            if (samples > 0)
            {
                bUnderrun       = true;
                dsp::fill_zero(dst, samples);
            }

            return STATUS_OK;
        }

        status_t SharedAudioStream::write(size_t channel, const float *src, size_t samples)
        {
            if (pHeader == NULL)
                return STATUS_CLOSED;
            if (!bIO)
                return STATUS_BAD_STATE;
            if (bWriteMode)
                return STATUS_BAD_STATE;

            // Check that we went out of channel
            if (channel >= nChannels)
                return STATUS_OK;

            channel_t *c = &vChannels[channel];
            const uint32_t length = pHeader->nLength;

            // Perform write
            while (samples > 0)
            {
                size_t to_write = lsp_min(samples, length - c->nPosition);
                dsp::copy(&c->pData[c->nPosition], src, to_write);

                samples        -= to_write;
                src            += to_write;
                c->nPosition    = (c->nPosition + to_write) % length;
            }

            return STATUS_OK;
        }

        bool SharedAudioStream::check_channels_synchronized()
        {
            for (size_t i=1; i<nChannels; ++i)
            {
                channel_t *c        = &vChannels[i];
                if (c->nHead != vChannels[0].nHead)
                    return false;
                if (c->nPosition != vChannels[0].nPosition)
                    return false;
                if (c->nAvail != vChannels[0].nHead)
                    return false;
            }

            return true;
        }

        status_t SharedAudioStream::end()
        {
            if (pHeader == NULL)
                return STATUS_BAD_STATE;
            if (!bIO)
                return STATUS_BAD_STATE;

            if (!check_channels_synchronized())
                return STATUS_CORRUPTED;

            if (bWriteMode)
            {
                uint32_t blk_size       = pHeader->nMaxBlkSize;
                uint32_t head           = pHeader->nHead;
                uint32_t counter        = pHeader->nCounter;
                uint32_t avail          = vChannels[0].nAvail;

                // Commit new position and statistics
                pHeader->nMaxBlkSize    = lsp_max(pHeader->nMaxBlkSize, avail);
                pHeader->nCounter       = counter + avail;
                pHeader->nHead          = (head + avail) % pHeader->nLength;
            }
            else
            {
                // Check if requested number of samples was greater than available
                if (bUnderrun)
                    return STATUS_OK;

                // Update current head position for each channel
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];
                    c->nHead        = c->nPosition;
                    c->nAvail       = 0;
                }
            }

            bIO             = false;

            return STATUS_OK;
        }

    } /* namespace dspu */
} /* namespace lsp */


