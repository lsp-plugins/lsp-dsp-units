/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 29 янв. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_SHIFTBUFFER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_SHIFTBUFFER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /** Shift buffer processor
         *    This class implements shift buffer.
         *    New data is added to buffer at the tail position with append() methods
         *    Old data is removed from buffer from the head position with shift() methods
         *
         */
        class LSP_DSP_UNITS_PUBLIC ShiftBuffer
        {
            private:
                ShiftBuffer & operator = (const ShiftBuffer &);
                ShiftBuffer(const ShiftBuffer &);

            protected:
                float      *pData;
                size_t      nCapacity;
                size_t      nHead;
                size_t      nTail;

            public:
                explicit ShiftBuffer();
                ~ShiftBuffer();

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
                bool        init(size_t size, size_t gap = 0);

                /** Destroy buffer
                 *
                 */
                void        destroy();

            public:
                /** Resize buffer, if not initialized is similar to init()
                 *
                 * @param size the requested size of buffer, in terms of optimization may be allocated a bit more data
                 * @param gap number of samples initially stored in buffer, if gap > previous size() then additional samples will be zeroed
                 * @return status of operation
                 */
                bool resize(size_t size, size_t gap = 0);

                /** Add data to the end of buffer
                 *
                 * @param data amount of data to push, if NULL then buffer is filled with zeros
                 * @param count number of samples
                 * @return number of samples appended
                 */
                size_t append(const float *data, size_t count);

                /** Append the single sample
                 *
                 * @param data sample to append
                 * @return number of samples appended
                 */
                size_t append(float data);

                /** Remove data from the beginning of the buffer
                 *
                 * @param data pointer to store the samples removed from buffer, may be NULL for skipping
                 * @param count number of samples to remove
                 * @return number of samples removed
                 */
                size_t shift(float *data, size_t count);

                /** Remove data from the beginning of the buffer
                 *
                 * @param count number of samples to remove
                 * @return number of samples removed
                 */
                size_t shift(size_t count);

                /** Remove one sample from the beginning of the buffer
                 *
                 * @return removed sampler or 0 if the buffer is empty
                 */
                float shift();

                /** Return the number of items in the buffer
                 *
                 * @return number of items in the buffer
                 */
                inline size_t size() const { return nTail - nHead;  };

                /**
                 * Get the size of free space after the tail
                 * @return size of free space after the tail
                 */
                inline size_t tail_gap_size() const     { return nCapacity - nTail;     }

                /**
                 * Get the size of free space before the heade
                 * @return size of free space before the head
                 */
                inline size_t head_gap_size() const     { return nHead;                 }

                /** Get maximum size of the buffer
                 *
                 * @return maximum size of the buffer
                 */
                inline size_t capacity() const { return nCapacity; };

                /** Clear buffer
                 *
                 */
                inline void clear() { nHead = nTail = 0; }

                /** Get the pointer to the beginning of the entire buffer
                 *
                 * @return data pointer at the head of buffer
                 */
                float *data();

                /** Get the data pointer at the head of buffer
                 *
                 * @return data pointer at the head of buffer
                 */
                float *head();

                /** Get the data pointer at the tail of buffer
                 *
                 * @return data pointer at the tail of buffer
                 */
                float *tail();

                /** Get the data pointer at the head of buffer
                 * @param offset offset from the head
                 * @return data pointer at the head of buffer
                 */
                float *head(size_t offset);

                /** Get the data pointer at the tail of buffer
                 * @param offset offset from the tail
                 * @return data pointer at the tail of buffer
                 */
                float *tail(size_t offset);

                /** Get sample from tail
                 *
                 * @param offset offset
                 * @return sample
                 */
                float last(size_t offset);

                /** Get the first sample in the buffer
                 *
                 * @return the first sample in the buffer or 0 if empty
                 */
                float first() const;

                /** Get the last sample in the buffer
                 *
                 * @return the last sample in the buffer or 0 if empty
                 */
                float last() const;

                /** Get the first sample in the buffer
                 *
                 * @param offset offset from the head
                 *
                 * @return the first sample in the buffer or 0 if empty
                 */
                float first(size_t offset) const;

                /** Get the last sample in the buffer
                 * @param offset offset from the tail
                 *
                 * @return the last sample in the buffer or 0 if empty
                 */
                float last(size_t offset) const;

                /** Fill buffer with specific value
                 *
                 * @param value value used to fill
                 */
                void fill(float value);

                /** Copy data from the specified ShiftBuffer
                 *
                 * @param src buffer to copy data from
                 */
                void copy(const ShiftBuffer *src);

                /**
                 * Dump data to the shift buffer
                 * @param v dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_SHIFTBUFFER_H_ */
