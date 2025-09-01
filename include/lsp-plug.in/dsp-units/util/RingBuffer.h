/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 2 июн. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_RINGBUFFER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_RINGBUFFER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /** Ring buffer processor
         *
         */
        class LSP_DSP_UNITS_PUBLIC RingBuffer
        {
            protected:
                float      *pData;
                uint32_t    nCapacity;
                uint32_t    nHead;

            public:
                explicit RingBuffer();
                RingBuffer(const RingBuffer &) = delete;
                RingBuffer(RingBuffer &&) = delete;
                ~RingBuffer();

                RingBuffer & operator = (const RingBuffer &) = delete;
                RingBuffer & operator = (RingBuffer &&) = delete;

                /**
                 * Construct the buffer
                 */
                void            construct();

                /** Init buffer, all previously stored data will be lost
                 *
                 * @param size the requested size of buffer, in terms of optimization may be allocated a bit more data
                 * @return status of operation
                 */
                bool            init(size_t size);

                /** Destroy buffer
                 *
                 */
                void            destroy();

            public:
                /**
                 * Add data to the head of the buffer
                 *
                 * @param data data to append to the buffer
                 * @param count number of samples
                 * @return actual number of samples appended
                 */
                size_t          append(const float *data, size_t count);

                /**
                 * Place the single sample to the head of the buffer
                 *
                 * @param data sample to append
                 */
                void            append(float data);

                /**
                 * Return the maximum number of samples can be stored in the buffer
                 *
                 * @return number of items in the buffer
                 */
                inline size_t   size() const            { return nCapacity;  };

                /**
                 * Clear buffer contents, fill all data with zero
                 */
                void            clear();

                /**
                 * Get the pointer to the beginning of the entire buffer
                 *
                 * @return data pointer at the head of buffer
                 */
                inline float    *data()                 { return pData; }

                /**
                 * Read the data from the buffer
                 * @param dst destination buffer to store the data
                 * @param offset offset relative to the current buffer pointer
                 * @param count number of samples to read
                 * @return actual number of samples read
                 */
                size_t          get(float *dst, size_t offset, size_t count) const;

                /**
                 * Get the sample at the tail of the buffer specified by the offset
                 * @param offset offset relative to the current buffer pointer
                 * @return data the sample at the tail of the buffer
                 */
                float           get(size_t offset) const;

                /**
                 * Read contents of the buffer starting from specified absolute position.
                 * If number of samples exceed the buffer capacity, the read will loop through
                 * the buffer desired number of times and copy wrapped contents.
                 *
                 * @param dst destination buffer
                 * @param position absolute position to start read, should be less than capacity
                 * @param count number of samples to read
                 * @return actual number of samples read or 0 if start position is invalid
                 */
                size_t          read(float *dst, size_t position, size_t count) const;

                /**
                 * Read a single sample starting from specified absolute position
                 *
                 * @param position absolute position to read sample
                 * @return read sample or 0.0f if position is out of buffer bounds
                 */
                float           read(size_t position) const;

                /**
                 * Get the interpolated sample at the tail of the buffer specified by the offset.
                 * The value is computed using linear interpolation
                 * @param offset offset
                 * @return data the sample at the tail of the buffer
                 */
                float           lerp_get(float offset) const;

                /**
                 * Get absolute position of the head
                 * @return position of the head relative to the beginning of the buffer
                 */
                inline size_t   head_position() const  { return nHead; }

                /**
                 * Compute the absolute position of tail in the buffer
                 * @param offset the offset of the tail relative to the head
                 * @return position of the tail relative to the beginning of the buffer
                 */
                size_t          tail_position(size_t offset) const;

                /**
                 * Dump data to the shift buffer
                 * @param v dumper
                 */
                void            dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_RINGBUFFER_H_ */
