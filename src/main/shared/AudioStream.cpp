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
#include <lsp-plug.in/dsp-units/shared/AudioStream.h>
#include <lsp-plug.in/runtime/system.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr uint32_t shared_stream_magic = 0x5354524d; // "STRM"

        AudioStream::AudioStream()
        {
            pHeader         = NULL;
            vChannels       = NULL;
            nChannels       = 0;
            nHead           = 0;
            nAvail          = 0;
            nBlkSize        = 0;
            nCounter        = 0;
            bWriteMode      = false;
            bIO             = false;
            bUnderrun       = false;
        }

        AudioStream::~AudioStream()
        {
            pHeader         = NULL;
            if (vChannels != NULL)
            {
                free(vChannels);
                vChannels       = NULL;
            }
            nChannels       = 0;
            nHead           = 0;
            nAvail          = 0;
            nBlkSize        = 0;
            nCounter        = 0;
            bWriteMode      = false;
            bIO             = false;
            bUnderrun       = false;
        }

        status_t AudioStream::open(const char *id)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            status_t res = hMem.open(id, ipc::SharedMem::SHM_READ, 0);
            if (res == STATUS_OK)
                res = open_internal();

            if (res != STATUS_OK)
                close();

            return res;
        }

        status_t AudioStream::open(const LSPString *id)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            status_t res = hMem.open(id, ipc::SharedMem::SHM_READ, 0);
            if (res == STATUS_OK)
                res = open_internal();

            if (res != STATUS_OK)
                close();

            return res;
        }

        bool AudioStream::calc_params(alloc_params_t *params, size_t channels, size_t length)
        {
            if ((channels < 1) || (length <= 0))
                return false;

            const size_t page_size      = system::page_size();

            params->nChannels           = channels;
            params->nHdrBytes           = align_size(sizeof(sh_header_t), page_size);
            params->nChannelBytes       = align_size(sizeof(float) * length, page_size);
            params->nSegmentBytes       = params->nHdrBytes + params->nChannelBytes*params->nChannels;

            return true;
        }

        status_t AudioStream::create(const char *id, size_t channels, size_t length)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            status_t res;
            alloc_params_t params;
            if (!calc_params(&params, channels, length))
                return STATUS_INVALID_VALUE;

            if ((res = hMem.open(id, ipc::SharedMem::SHM_RW | ipc::SharedMem::SHM_CREATE, params.nSegmentBytes)) == STATUS_OK)
                res = create_internal(channels, &params);

            if (res != STATUS_OK)
                close();

            return res;
        }

        status_t AudioStream::create(const LSPString *id, size_t channels, size_t length)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            status_t res;
            alloc_params_t params;
            if (!calc_params(&params, channels, length))
                return STATUS_INVALID_VALUE;

            if ((res = hMem.open(id, ipc::SharedMem::SHM_RW | ipc::SharedMem::SHM_CREATE, params.nSegmentBytes)) == STATUS_OK)
                res = create_internal(channels, &params);

            if (res != STATUS_OK)
                close();

            return res;
        }

        status_t AudioStream::allocate(LSPString *name, const char *postfix, size_t channels, size_t length)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            status_t res;
            alloc_params_t params;
            if (!calc_params(&params, channels, length))
                return STATUS_INVALID_VALUE;

            LSPString tmp;
            if ((res = hMem.create(&tmp, postfix, ipc::SharedMem::SHM_RW | ipc::SharedMem::SHM_CREATE, params.nSegmentBytes)) == STATUS_OK)
                res = create_internal(channels, &params);

            if (res == STATUS_OK)
                tmp.swap(name);
            else
                close();

            return res;
        }

        status_t AudioStream::allocate(LSPString *name, const LSPString *postfix, size_t channels, size_t length)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            status_t res;
            alloc_params_t params;
            if (!calc_params(&params, channels, length))
                return STATUS_INVALID_VALUE;

            LSPString tmp;
            if ((res = hMem.create(&tmp, postfix, ipc::SharedMem::SHM_RW | ipc::SharedMem::SHM_CREATE, params.nSegmentBytes)) == STATUS_OK)
                res = create_internal(channels, &params);

            if (res == STATUS_OK)
                tmp.swap(name);
            else
                close();

            return res;
        }

        status_t AudioStream::create_internal(size_t channels, const alloc_params_t *params)
        {
            status_t res;

            // Map memory
            res = hMem.map(0, params->nSegmentBytes);
            if (res != STATUS_OK)
                return res;

            uint8_t *ptr    = reinterpret_cast<uint8_t *>(hMem.data());
            if (ptr == NULL)
                return STATUS_UNKNOWN_ERR;

            // Initialize buffer
            pHeader             = advance_ptr_bytes<sh_header_t>(ptr, params->nHdrBytes);
            const uint32_t len  = params->nChannelBytes / sizeof(float);

            pHeader->nMagic     = CPU_TO_BE(shared_stream_magic);   // Magic identifier 'STRM'
            pHeader->nVersion   = 1;                                // Version of the buffer
            pHeader->nFlags     = 0;                                // Clean up flags
            pHeader->nChannels  = channels;                         // Number of channels
            pHeader->nLength    = len;                              // Length of the channel
            pHeader->nMaxBlkSize= 0;                                // Maximum block size written
            pHeader->nHead      = 0;                                // Position of the head of the buffer
            pHeader->nCounter   = 0;                                // Auto-incrementing counter for each change

            // Allocate channel information
            nChannels           = channels;
            vChannels           = static_cast<channel_t *>(malloc(sizeof(channel_t) * nChannels));
            if (vChannels == NULL)
                return STATUS_NO_MEM;

            // Initialize channels
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c        = &vChannels[i];

                c->nPosition        = 0;
                c->nCount           = 0;
                c->pData            = advance_ptr_bytes<float>(ptr, params->nChannelBytes);

                dsp::fill_zero(c->pData, len);
            }

            // Initialize fields
            pHeader->nFlags     = SS_INITIALIZED;
            nHead               = 0;
            nAvail              = 0;
            nBlkSize            = 0;
            nCounter            = 0;
            bWriteMode          = true;
            bIO                 = false;
            bUnderrun           = false;

            return res;
        }

        status_t AudioStream::open_internal()
        {
            status_t res;

            // Map header
            if ((res = hMem.map(0, sizeof(sh_header_t))) != STATUS_OK)
                return res;

            // Parse header information
            sh_header_t *hdr = reinterpret_cast<sh_header_t *>(hMem.data());
            if (hdr == NULL)
                return STATUS_UNKNOWN_ERR;

            if (BE_TO_CPU(hdr->nMagic) != shared_stream_magic)
                return STATUS_BAD_FORMAT;
            uint32_t version= hdr->nVersion;
            if (version != 1)
                return STATUS_UNSUPPORTED_FORMAT;

            // Do not open already terminated stream
            uint32_t flags  = hdr->nFlags;
            if ((flags & SS_TERM_MASK) == SS_TERMINATED)
                return STATUS_CLOSED;

            alloc_params_t params;
            if (!calc_params(&params, hdr->nChannels, hdr->nLength))
                return STATUS_CORRUPTED;
            hdr             = NULL;

            // Allocate channel information
            nChannels       = params.nChannels;
            vChannels       = static_cast<channel_t *>(malloc(sizeof(channel_t) * params.nChannels));
            if (vChannels == NULL)
                return STATUS_NO_MEM;

            // Remap header
            if ((res = hMem.map(0, params.nSegmentBytes)) != STATUS_OK)
                return res;

            uint8_t *ptr    = reinterpret_cast<uint8_t *>(hMem.data());
            if (ptr == NULL)
                return STATUS_UNKNOWN_ERR;

            // Initialize header pointer
            pHeader         = advance_ptr_bytes<sh_header_t>(ptr, params.nHdrBytes);

            // Initialize channels
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];

                c->nPosition    = 0;
                c->nCount       = 0;
                c->pData        = advance_ptr_bytes<float>(ptr, params.nChannelBytes);
            }

            // Initialize fields
            nHead           = pHeader->nHead;
            nAvail          = 0;
            nBlkSize        = 0;
            nCounter        = pHeader->nCounter + 0x80000000; // Make out of sync
            bWriteMode      = false;
            bIO             = false;
            bUnderrun       = false;

            return STATUS_OK;
        }

        status_t AudioStream::close()
        {
            if (pHeader != NULL)
            {
                if (bWriteMode)
                {
                    uint32_t flags      = pHeader->nFlags;
                    pHeader->nFlags     = flags | SS_TERMINATED;
                }

                pHeader         = NULL;
            }

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

        size_t AudioStream::channels() const
        {
            size_t channels = (pHeader != NULL) ? pHeader->nChannels : 0;
            return lsp_min(nChannels, channels);
        }

        size_t AudioStream::length() const
        {
            return (pHeader != NULL) ? pHeader->nLength : 0;
        }

        status_t AudioStream::begin(ssize_t block_size)
        {
            if (pHeader == NULL)
                return STATUS_BAD_STATE;
            if (bIO)
                return STATUS_BAD_STATE;

            nBlkSize        = block_size;

            if (!bWriteMode)
            {
                // Check that we are in sync
                uint32_t flags      = pHeader->nFlags;
                uint32_t src_counter= pHeader->nCounter;
                uint32_t src_head   = pHeader->nHead;
                uint32_t blk_size   = pHeader->nMaxBlkSize;

                // Compute number of available samples
                nAvail              = src_counter - nCounter;
                if ((flags & (SS_UPD_MASK | SS_INIT_MASK)) != (SS_UPDATED | SS_INITIALIZED))
                    nAvail              = 0;

                // Synchronize position
                if (nAvail > blk_size * 4)
                {
                    if ((flags & SS_TERM_MASK) == SS_TERMINATED)
                        return STATUS_EOF;

                    // We went out of sync, need to re-sync
                    uint32_t length     = pHeader->nLength;
                    nHead               = (src_head + length - blk_size) % length;
                    nAvail              = blk_size;
                    nCounter            = src_counter - nAvail;
                }
                else if (nAvail <= 0)
                {
                    if ((flags & SS_TERM_MASK) == SS_TERMINATED)
                        return STATUS_EOF;
                }

                // Limit number of samples to read if limit is set
                if ((nBlkSize > 0) && (nAvail > size_t(nBlkSize)))
                    nAvail              = nBlkSize;
            }
            else
            {
                nHead           = pHeader->nHead;
                nCounter        = pHeader->nCounter;
                nAvail          = 0;
            }

            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];
                c->nPosition    = nHead;
                c->nCount       = 0;
            }

            bIO             = true;
            bUnderrun       = false;

            return STATUS_OK;
        }

        status_t AudioStream::read(size_t channel, float *dst, size_t samples)
        {
            if (pHeader == NULL)
                return STATUS_CLOSED;
            if (!bIO)
                return STATUS_BAD_STATE;
            if (bWriteMode)
                return STATUS_BAD_STATE;

            // Check that audio stream is in valid state
            uint32_t flags = pHeader->nFlags;
            if ((flags & (SS_UPD_MASK | SS_INIT_MASK)) != (SS_UPDATED | SS_INITIALIZED))
            {
                bUnderrun       = true;
                dsp::fill_zero(dst, samples);
                return STATUS_OK;
            }

            // Check that we went out of channel
            if (channel >= nChannels)
            {
                dsp::fill_zero(dst, samples);
                return STATUS_OK;
            }

            // Perform read
            channel_t *c = &vChannels[channel];
            const uint32_t length = pHeader->nLength;

            while ((samples > 0) && (c->nCount < nAvail))
            {
                size_t to_read  = lsp_min(samples, nAvail - c->nCount, length - c->nPosition);
                dsp::copy(dst, &c->pData[c->nPosition], to_read);

                samples        -= to_read;
                dst            += to_read;
                c->nPosition    = (c->nPosition + to_read) % length;
                c->nCount    += to_read;
            }

            // Detected buffer underrun?
            if (samples > 0)
            {
                bUnderrun       = true;
                dsp::fill_zero(dst, samples);
            }

            return STATUS_OK;
        }

        status_t AudioStream::write(size_t channel, const float *src, size_t samples)
        {
            if (pHeader == NULL)
                return STATUS_CLOSED;
            if (!bIO)
                return STATUS_BAD_STATE;
            if (!bWriteMode)
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
                c->nCount      += to_write;
            }

            return STATUS_OK;
        }

        bool AudioStream::check_channels_synchronized()
        {
            for (size_t i=1; i<nChannels; ++i)
            {
                channel_t *c        = &vChannels[i];
                if (c->nPosition != vChannels[0].nPosition)
                    return false;
                if (c->nCount != vChannels[0].nCount)
                    return false;
            }

            return true;
        }

        status_t AudioStream::end()
        {
            if (pHeader == NULL)
                return STATUS_BAD_STATE;
            if (!bIO)
                return STATUS_BAD_STATE;

            // Estimate size of I/O block
            size_t block_size = nBlkSize;
            if (block_size == 0)
            {
                for (size_t i=0; i<nChannels; ++i)
                    block_size = lsp_max(block_size, vChannels[i].nCount);
            }

            uint32_t length         = pHeader->nLength;

            if (bWriteMode)
            {
                uint32_t hdr_blk_size   = pHeader->nMaxBlkSize;
                uint32_t flags          = pHeader->nFlags;

                // Fill missing data with zeros
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c = &vChannels[i];

                    // Perform write zeros
                    size_t samples = block_size - c->nCount;
                    while (samples > 0)
                    {
                        size_t to_write = lsp_min(samples, length - c->nPosition);
                        dsp::fill_zero(&c->pData[c->nPosition], to_write);

                        samples        -= to_write;
                        c->nPosition    = (c->nPosition + to_write) % length;
                    }
                }

                // Commit new position and statistics
                pHeader->nMaxBlkSize    = lsp_max(hdr_blk_size, block_size);
                pHeader->nCounter       = nCounter + block_size;
                pHeader->nHead          = (nHead + block_size) % length;
                pHeader->nFlags         = flags | SS_UPDATED;
            }
            else if (!bUnderrun)
            {
                // Check if requested number of samples was greater than available
                nHead                   = (nHead + block_size) % length;
                nCounter               += block_size;
            }

            bIO                     = false;
            bUnderrun               = false;

            return STATUS_OK;
        }

    } /* namespace dspu */
} /* namespace lsp */


