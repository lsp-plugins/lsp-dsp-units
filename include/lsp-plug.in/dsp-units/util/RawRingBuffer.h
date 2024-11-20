/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 13 апр. 2024 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_RAWRINGBUFFER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_RAWRINGBUFFER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /** Ring buffer processor
         *
         */
        class LSP_DSP_UNITS_PUBLIC RawRingBuffer
        {
            protected:
                float      *pData;
                size_t      nCapacity;
                size_t      nHead;

            public:
                explicit RawRingBuffer();
                RawRingBuffer(const RawRingBuffer &) = delete;
                RawRingBuffer(RawRingBuffer &&) = delete;
                ~RawRingBuffer();

                RawRingBuffer & operator = (const RawRingBuffer &) = delete;
                RawRingBuffer & operator = (RawRingBuffer &&) = delete;

                /**
                 * Construct the buffer
                 */
                void                construct();

                /** Init buffer, all previously stored data will be lost
                 *
                 * @param size the requested size of buffer, in terms of optimization may be allocated a bit more data
                 * @param gap number of zero samples initially stored in buffer, can not be greater than size
                 * @return status of operation
                 */
                bool                init(size_t size);

                /** Destroy buffer
                 *
                 */
                void                destroy();

            public:
                /**
                 * Write data to the buffer at current head position
                 *
                 * @param src buffer to write
                 * @param count number of samples
                 * @return actual number of samples written
                 */
                size_t              write(const float *src, size_t count);

                /**
                 * Write the single sample to the ring buffer at current head position
                 *
                 * @param data sample to append
                 * @return number of samples appended
                 */
                void                write(float data);

                /**
                 * Write data to the buffer at current head position and advance position
                 *
                 * @param src buffer to write
                 * @param count number of samples
                 * @return actual number of samples written
                 */
                size_t              push(const float *data, size_t count);

                /**
                 * Write the single sample to the ring buffer at current head position and advance position
                 *
                 * @param data sample to append
                 * @return number of samples appended
                 */
                void                push(float data);

                /**
                 * Read the data from the ring buffer at the specified offset relative to the current head pointer
                 * @param dst destination buffer to store the data
                 * @param count number of samples to read
                 * @return actual number of samples read
                 */
                size_t              read(float *dst, size_t offset, size_t count);

                /**
                 * Read the single sample at the specified offset relative to the current head pointer
                 * @param offset offset behind the head
                 * @return data pointer at the head of buffer
                 */
                float               read(size_t offset) const;

                /**
                 * Advance head by the specified amount of samples
                 * @param count number of samples to advance
                 * @return pointer to new head
                 */
                float              *advance(size_t count);

            public:
                /**
                 * Return the overall buffer size
                 * @return number of items in the buffer
                 */
                inline size_t       size() const                { return nCapacity;  };

                /**
                 * Clear buffer contents, fill all data with zero
                 */
                void                clear();

                /**
                 * Reset head to zero (initial) position
                 */
                void                reset();

                /**
                 * Get the pointer to the beginning of the entire buffer
                 * @return data pointer at the head of buffer
                 */
                inline float       *begin()                     { return pData; }
                inline const float *begin() const               { return pData; }

                /**
                 * Get the pointer to the first sample after the buffer space
                 * @return data pointer at the head of buffer
                 */
                inline float       *end()                       { return &pData[nCapacity]; }
                inline const float *end() const                 { return &pData[nCapacity]; }

                /**
                 * Get pointer to the current head
                 */
                inline float       *head()                      { return &pData[nHead];     }
                inline const float *head() const                { return &pData[nHead];     }

                /**
                 * Get current write position index
                 * @return current write position
                 */
                inline size_t       position() const            { return nHead;             }

                /**
                 * Get pointer to the tail
                 * @param offset tail offset relative to the head
                 */
                float              *tail(size_t offset);
                const float        *tail(size_t offset) const;

                /**
                 * Get the number of samples available in the buffer before it's head does the flip.
                 * This can be useful when caller code does not want to care about buffer flipping
                 * and wants to limit the processing buffer size to make the flip automatically
                 * by the ring buffer.
                 * @return number of samples
                 */
                inline size_t       head_remaining() const      { return nCapacity - nHead; }

                /**
                 * Get the number of samples available in the buffer before it's tail does the flip.
                 * This can be useful when caller code does not want to care about buffer flipping
                 * and wants to limit the processing buffer size to make the flip automatically
                 * by the ring buffer.
                 * @return number of samples
                 */
                size_t              tail_remaining(size_t offset) const;

                /**
                 * Get the number of samples available in the buffer before it's tail or head does the flip.
                 * This can be useful when caller code does not want to care about buffer flipping
                 * and wants to limit the processing buffer size to make the flip automatically
                 * by the ring buffer.
                 * @return number of samples
                 */
                size_t              remaining(size_t offset) const;

                /**
                 * Fill the whole buffer with specified value
                 * @param value value to fill
                 */
                void                fill(float value);

                /**
                 * Dump data to the shift buffer
                 * @param v dumper
                 */
                void                dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_RAWRINGBUFFER_H_ */
