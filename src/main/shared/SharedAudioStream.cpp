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
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/common/new.h>
#include <lsp-plug.in/dsp-units/shared/SharedAudioStream.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr uint32_t shared_stream_magic = 0x5354524d; // "STRM"


        SharedAudioStream::SharedAudioStream()
        {
            construct();
        }

        SharedAudioStream::~SharedAudioStream()
        {
            destroy();
        }

        void SharedAudioStream::construct()
        {
            new (&hMem, inplace_new_tag_t()) ipc::SharedMem();

            pHeader         = NULL;
            vChannels       = NULL;
            nChannels       = 0;
            bWriteMode      = false;
            bIO             = false;
            bUnderrun       = false;
        }

        void SharedAudioStream::destroy()
        {
            pHeader         = NULL;
            if (vChannels != NULL)
            {
                free(vChannels);
                vChannels       = NULL;
            }
            nChannels       = 0;
            bWriteMode      = false;
            bIO             = false;
            bUnderrun       = false;

            hMem::~SharedMem();
        }

        status_t SharedAudioStream::open(const char *id)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            status_t res = hMem.open(id, ipc::SharedMem::SHM_READ | ipc::SharedMem::SHM_PERSIST, 0);
            if (res == STATUS_OK)
                res = open_internal();

            if (res != STATUS_OK)
                close();

            return res;
        }

        status_t SharedAudioStream::open(const LSPString *id)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            status_t res = hMem.open(id, ipc::SharedMem::SHM_READ | ipc::SharedMem::SHM_PERSIST, 0);
            if (res == STATUS_OK)
                res = open_internal();

            if (res != STATUS_OK)
                close();

            return res;
        }

        status_t SharedAudioStream::open_internal()
        {
            status_t res;

            // Map header
            const size_t hdr_size = align_size(sizeof(sh_header_t), 0x40);
            if ((res = hMem.map(0, hdr_size)) != STATUS_OK)
                return res;

            // Parse header information
            sh_header_t *hdr = reinterpret_cast<sh_header_t *>(hMem.data());
            if (hdr == NULL)
                return STATUS_UNKNOWN_ERR;

            if (CPU_TO_BE(hdr->nMagic) != shared_stream_magic)
                return STATUS_BAD_FORMAT;
            uint32_t version= pHeader->nVersion;
            if (version != 1)
                return STATUS_UNSUPPORTED_FORMAT;

            nChannels       = hdr->nChannels;

            // Allocate channel information
            vChannels       = static_cast<channel_t *>(malloc(sizeof(channel_t) * nChannels));
            if (vChannels == NULL)
                return STATUS_NO_MEM;

            // Remap header
            const size_t channel_size   = sizeof(float) * hdr->nLength;
            const size_t payload_size   = align_size(channel_size * hdr->nChannels, 0x40);
            if ((res = hMem.map(0, hdr_size + payload_size)) != STATUS_OK)
                return res;

            hdr             = NULL;
            uint8_t *ptr    = reinterpret_cast<uint8_t *>(hMem.data());
            if (ptr == NULL)
                return STATUS_UNKNOWN_ERR;

            pHeader         = advance_ptr_bytes<sh_header_t>(ptr, hdr_size);
            uint32_t head   = pHeader->nHead;
            uint32_t counter= pHeader->nCounter - 0x80000000; // Make out of sync

            // Initialize channels
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];

                c->nHead        = head;
                c->nPosition    = 0;
                c->nAvail       = 0;
                c->nCounter     = counter;
                c->pData        = advance_ptr_bytes<float>(ptr, channel_size);
            }

            // Initialize fields
            bWriteMode      = false;
            bIO             = false;
            bUnderrun       = false;

            return STATUS_OK;
        }

        status_t SharedAudioStream::create(const char *id, size_t channels, size_t length)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            const size_t hdr_size       = align_size(sizeof(sh_header_t), 0x40);
            const size_t channel_size   = align_size(sizeof(float) * channels, 0x40);
            const size_t payload_size   = channel_size * channels;

            status_t res = hMem.open(id, ipc::SharedMem::SHM_PERSIST | ipc::SharedMem::SHM_CREATE | ipc::SharedMem::SHM_PERSIST, hdr_size + payload_size);
            if (res == STATUS_OK)
                res = create_internal(channels, hdr_size, channel_size);

            if (res != STATUS_OK)
                close();

            return res;
        }

        status_t SharedAudioStream::create(const LSPString *id, size_t channels, size_t length)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            const size_t hdr_size       = align_size(sizeof(sh_header_t), 0x40);
            const size_t channel_size   = align_size(sizeof(float) * channels, 0x40);
            const size_t payload_size   = channel_size * channels;

            status_t res = hMem.open(id, ipc::SharedMem::SHM_PERSIST | ipc::SharedMem::SHM_CREATE | ipc::SharedMem::SHM_PERSIST, hdr_size + payload_size);
            if (res == STATUS_OK)
                res = create_internal(channels, hdr_size, channel_size);

            if (res != STATUS_OK)
                close();

            return res;
        }

        status_t SharedAudioStream::create_internal(size_t channels, size_t hdr_size, size_t channel_size)
        {
            status_t res;

            // Map memory
            const size_t payload_size   = channel_size * channels;
            res = hMem.map(0, hdr_size + payload_size);
            if (res != STATUS_OK)
                return res;

            uint8_t *ptr    = reinterpret_cast<uint8_t *>(hMem.data());
            if (ptr == NULL)
                return STATUS_UNKNOWN_ERR;

            // Initialize buffer
            pHeader             = advance_ptr_bytes<sh_header_t>(ptr, hdr_size);
            uint32_t head       = pHeader->nHead;

            pHeader->nMagic     = BE_TO_CPU(shared_stream_magic);   // Magic identifier 'STRM'
            pHeader->nVersion   = 1;                                // Version of the buffer
            pHeader->nChannels  = channels;                         // Number of channels
            pHeader->nLength    = channel_size / sizeof(float);     // Length of the channel
            pHeader->nMaxBlkSize= 0;                                // Maximum block size written
            pHeader->nHead      = 0;                                // Position of the head of the buffer
            pHeader->nCounter   = 0;                                // Auto-incrementing counter for each change

            // Initialize channels
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];

                c->nHead        = head;
                c->nPosition    = 0;
                c->nAvail       = 0;
                c->nCounter     = 0;
                c->pData        = advance_ptr_bytes<float>(ptr, channel_size);
            }

            // Initialize fields
            bWriteMode      = true;
            bIO             = false;
            bUnderrun       = false;

            return res;
        }

        status_t SharedAudioStream::close()
        {
            pHeader         = NULL;
            if (vChannels != NULL)
            {
                free(vChannels);
                vChannels       = NULL;
            }
            nChannels       = 0;
            bWriteMode      = false;
            bIO             = false;
            bUnderrun       = false;

            return hMem.close();
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
                // Check that we are in sync
                uint32_t src_counter= pHeader->nCounter;
                uint32_t src_head   = pHeader->nHead;
                uint32_t blk_size   = pHeader->nMaxBlkSize;

                uint32_t head       = vChannels[0].nPosition;
                uint32_t counter    = vChannels[0].nCounter;
                size_t avail        = src_counter - counter;

                if (avail > blk_size * 4)
                {
                    // We went out of sync, need to re-sync
                    uint32_t length     = pHeader->nLength;
                    head                = (src_head + length - blk_size) % length;
                    avail               = blk_size;
                    counter             = src_counter - avail;

                    for (size_t i=0; i<nChannels; ++i)
                    {
                        channel_t *c        = &vChannels[i];
                        c->nHead            = head;
                        c->nCounter         = counter;
                    }
                }

                // Set-up read position and number of available frames
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c        = &vChannels[i];
                    c->nPosition        = c->nHead;
                    c->nAvail           = avail;
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
                c->nAvail      += to_write;
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
                if (c->nCounter != vChannels[0].nCounter)
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
                uint32_t counter        = vChannels[0].nCounter;
                uint32_t avail          = vChannels[0].nAvail;

                // Commit new position and statistics
                pHeader->nMaxBlkSize    = lsp_max(pHeader->nMaxBlkSize, avail);
                pHeader->nCounter       = counter;
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
                    channel_t *c            = &vChannels[i];
                    c->nHead                = c->nPosition;
                    c->nPosition            = 0;
                    c->nCounter             = 0;
                    c->nAvail               = 0;
                }
            }

            bIO                     = false;

            return STATUS_OK;
        }

    } /* namespace dspu */
} /* namespace lsp */


