/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                void        construct();

                /** Init buffer, all previously stored data will be lost
                 *
                 * @param size the requested size of buffer, in terms of optimization may be allocated a bit more data
                 * @param gap number of zero samples initially stored in buffer, can not be greater than size
                 * @return status of operation
                 */
                bool        init(size_t size);

                /** Destroy buffer
                 *
                 */
                void        destroy();

            public:
                /** Add data to the buffer
                 *
                 * @param data data to append to the buffer
                 * @param count number of samples
                 * @return actual number of samples appended
                 */
                size_t      append(const float *data, size_t count);

                /** Place the single sample to the ring buffer
                 *
                 * @param data sample to append
                 * @return number of samples appended
                 */
                void        append(float data);

                /** Return the number of items in the buffer
                 *
                 * @return number of items in the buffer
                 */
                inline size_t size() const              { return nCapacity;  };

                /** Clear buffer contents, fill all data with zero
                 *
                 */
                void        clear();

                /** Get the pointer to the beginning of the entire buffer
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
                size_t      get(float *dst, size_t offset, size_t count);

                /**
                 * Get the sample at the tail of the buffer specified by the offset
                 * @param offset offset relative to the current buffer pointer
                 * @return data the sample at the tail of the buffer
                 */
                float       get(size_t offset);

                /**
                 * Get offset of the head relative to the position
                 * @return position of the head relative to the beginning of the buffer
                 */
                inline      size_t head_offset() const  { return nHead; }

                /**
                 * Compute the tail offset in the buffer
                 * @param offset the offset of the tail relative to the head
                 * @return position of the tail relative to the beginning of the buffer
                 */
                size_t      tail_offset(size_t offset) const;

                /**
                 * Dump data to the shift buffer
                 * @param v dumper
                 */
                void        dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_RINGBUFFER_H_ */
