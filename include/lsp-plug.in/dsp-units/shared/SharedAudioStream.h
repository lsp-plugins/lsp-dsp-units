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

#ifndef LSP_PLUG_IN_DSP_UNITS_SHARED_SHAREDAUDIOSTREAM_H_
#define LSP_PLUG_IN_DSP_UNITS_SHARED_SHAREDAUDIOSTREAM_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/ipc/SharedMem.h>
#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Shared audio stream FIFO.
         */
        class SharedAudioStream
        {
            protected:

                typedef struct sh_header_t
                {
                    uint32_t            nMagic;         // Magic number
                    uint32_t            nVersion;       // Version of the buffer
                    uint32_t            nChannels;      // Number of channels
                    uint32_t            nLength;        // Number of samples per channel
                    volatile uint32_t   nMaxBlkSize;    // Maximum block size written
                    volatile uint32_t   nHead;          // Position of the head of the buffer
                    volatile uint32_t   nCounter;       // Auto-incrementing counter for each change
                } sh_header_t;

                typedef struct channel_t
                {
                    uint32_t            nHead;          // Read/Write head
                    uint32_t            nPosition;      // Read/Write position
                    uint32_t            nAvail;         // Number of samples available
                    uint32_t            nCounter;       // Counter
                    float              *pData;          // Pointer to channel data
                } channel_t;

            protected:
                ipc::SharedMem      hMem;           // Shared memory descriptor
                sh_header_t        *pHeader;        // Header of the shared buffer
                channel_t          *vChannels;      // Pointer to channel descriptions
                uint32_t            nChannels;      // Number of channels
                bool                bWriteMode;     // Stream is opened for writing
                bool                bIO;            // I/O mode (begin() called)
                bool                bUnderrun;      // Underrun detected

            protected:
                bool                check_channels_synchronized();
                status_t            open_internal();
                status_t            create_internal(size_t channels, size_t hdr_size, size_t channel_size);

            public:
                SharedAudioStream();
                SharedAudioStream(const SharedAudioStream &) = delete;
                SharedAudioStream(SharedAudioStream &&);
                ~SharedAudioStream();

                SharedAudioStream & operator = (const SharedAudioStream &) = delete;
                SharedAudioStream & operator = (SharedAudioStream &&);

                /** Construct object
                 *
                 */
                void            construct();

                /** Destroy object
                 *
                 */
                void            destroy();


            public:
                /**
                 * Open named audio stream for reading
                 * @param id identifier of the named audio stream
                 * @return status of operation
                 */
                status_t        open(const char *id);

                /**
                 * Open named audio stream for reading
                 * @param id identifier of the named audio stream
                 * @return status of operation
                 */
                status_t        open(const LSPString *id);

                /**
                 * Create and open named audio stream for writing
                 * @param id identifier of the named audio stream
                 * @param channels number of audio channels
                 * @param length length of each channel
                 * @return status of operation
                 */
                status_t        create(const char *id, size_t channels, size_t length);

                /**
                 * Create and open named audio stream for writing
                 * @param id identifier of the named audio stream
                 * @param channels number of audio channels
                 * @param length length of each channel
                 * @return status of operation
                 */
                status_t        create(const LSPString *id, size_t channels, size_t length);

                /**
                 * Close
                 * @return status of operation
                 */
                status_t        close();

            public:
                /**
                 * Return number of channels
                 * @return number of channels
                 */
                size_t          channels() const;

                /**
                 * Get number of samples per channel
                 * @return number of samples per channel
                 */
                size_t          length() const;

                /**
                 * Begin I/O operation on the stream
                 * @return status of operation
                 */
                status_t        begin();

                /**
                 * Read contents of specific channel
                 * Should be called between begin() and end() calls
                 *
                 * @param channel number of channel
                 * @param dst destination buffer to store data
                 * @param samples number of samples to read
                 * @return status of operation
                 */
                status_t        read(size_t channel, float *dst, size_t samples);

                /**
                 * Read contents of specific channel
                 * Should be called between begin() and end() calls
                 *
                 * @param channel number of channel
                 * @param dst destination buffer to store data
                 * @param samples number of samples to read
                 * @return status of operation
                 */
                status_t        write(size_t channel, const float *src, size_t samples);

                /**
                 * End I/O operations on the stream
                 * @return status of operation
                 */
                status_t        end();
        };

    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_SHARED_SHAREDAUDIOSTREAM_H_ */
