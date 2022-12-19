/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 29 янв. 2016 г.
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

#include <lsp-plug.in/dsp-units/util/ShiftBuffer.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        ShiftBuffer::ShiftBuffer()
        {
            construct();
        }

        ShiftBuffer::~ShiftBuffer()
        {
            destroy();
        }

        void ShiftBuffer::construct()
        {
            pData       = NULL;
            nCapacity   = 0;
            nHead       = 0;
            nTail       = 0;
        }

        bool ShiftBuffer::init(size_t size, size_t gap)
        {
            // Check gap
            if (gap > size)
                return false;

            // Make size multiple of 0x10
            size_t new_capacity     = align_size(size, 0x10);
            if ((pData == NULL) || (new_capacity != nCapacity))
            {
                // Allocate new buffer
                float *new_data     = new float[new_capacity];
                if (new_data == NULL)
                    return false;

                // Delete old buffer
                if (pData != NULL)
                    delete [] pData;
                pData       = new_data;
            }
            nCapacity   = new_capacity;
            nHead       = 0;
            nTail       = gap;

    //        lsp_trace("capacity = %d, head = %d, tail = %d", int(nCapacity), int(nHead), int(nTail));

            // Zero the gap
            dsp::fill_zero(pData, gap);
            return true;
        }

        bool ShiftBuffer::resize(size_t size, size_t gap)
        {
            // Check that need simply allocate new buffer
            if (pData == NULL)
                return init(size, gap);

            // Check gap
            if (gap > size)
                return false;

            // Make size multiple of 0x10
            size_t new_capacity = align_size(size, 0x10);
            size_t avail        = nTail - nHead;            // Current gap size
            ssize_t fill        = gap - avail;              // Number of additional gap elements

            if (new_capacity != nCapacity)
            {
                // Allocate new buffer
                float *dst     = new float[new_capacity];
                if (dst == NULL)
                    return false;

                // Copy data
                if (fill > 0)
                {
                    dsp::fill_zero(dst, fill);
                    dsp::copy(&dst[fill], &pData[nHead], avail);
                }
                else
                    dsp::copy(dst, &pData[nTail + fill], gap);
                delete [] pData;

                // Update pointers
                pData       = dst;
                nCapacity   = new_capacity;
                nHead       = 0;
                nTail       = gap;
            }
            else
            {
                // Process data if gap size changed
                if (fill > 0)
                {
                    // Check that there is possibility simply to zero data before the head
                    ssize_t reserve     = fill - nHead;
                    if (reserve > 0)
                    {
                        // We need to free some space before head
                        dsp::move(&pData[nHead + reserve], &pData[nHead], avail);
                        nTail   += reserve;
                        nHead   += reserve;
                    }

                    nHead      -= fill;
                    dsp::fill_zero(&pData[nHead], fill);
                }
                else if (fill < 0) // Simply forget data at the head
                    nHead -= fill;
            }

            return true;
        }

        void ShiftBuffer::destroy()
        {
            if (pData != NULL)
            {
                delete [] pData;
                pData       = NULL;
            }
            nCapacity   = 0;
            nHead       = 0;
            nTail       = 0;
        }

        size_t ShiftBuffer::append(const float *data, size_t count)
        {
            // Check state
            if (pData == NULL)
                return 0;

            // Check free space in buffer
            size_t can_append       = nCapacity - nTail;
            if (can_append <= 0)
            {
                if (nHead <= 0)
                    return 0;
                dsp::move(pData, &pData[nHead], nTail - nHead);
                can_append  = nHead;
                nTail      -= nHead;
                nHead       = 0;
            }
            else if ((can_append < count) && (nHead > 0))
            {
                dsp::move(pData, &pData[nHead], nTail - nHead);
                can_append += nHead;
                nTail      -= nHead;
                nHead       = 0;
            }

            // Determine the amount of samples to copy
            if (count > can_append)
                count               = can_append;

            // Fill the buffer
            if (data != NULL)
                dsp::copy(&pData[nTail], data, count);
            else
                dsp::fill_zero(&pData[nTail], count);
            nTail      += count;

            return count;
        }

        size_t ShiftBuffer::append(float data)
        {
            // Check state
            if (pData == NULL)
                return 0;

            // Check free space in buffer
            if (nTail >= nCapacity)
            {
                if (nHead <= 0)
                    return 0;
                dsp::move(pData, &pData[nHead], nTail - nHead);
                nTail  -= nHead;
                nHead   = 0;
            }

            // Append sample
            pData[nTail++]  = data;
            return 1;
        }

        size_t ShiftBuffer::shift(float *data, size_t count)
        {
            // Check state
            if (pData == NULL)
                return 0;

            // Determine the amount of samples to copy
            size_t can_shift    = nTail - nHead;
            if (count > can_shift)
                count   = can_shift;

            // Flush the buffer
            if (data != NULL)
                dsp::copy(data, &pData[nHead], count);
            nHead      += count;

    //        lsp_trace("count=%d, capacity=%d, head=%d, tail=%d", int(count), int(nCapacity), int(nHead), int(nTail));

            return count;
        }

        size_t ShiftBuffer::shift(size_t count)
        {
            // Check state
            if (pData == NULL)
                return 0;

            // Determine the amount of samples to copy
            size_t can_shift    = nTail - nHead;
            if (count > can_shift)
                count   = can_shift;

            // Flush the buffer
            nHead      += count;
    //        lsp_trace("count=%d, capacity=%d, head=%d, tail=%d", int(count), int(nCapacity), int(nHead), int(nTail));

            return count;
        }

        float ShiftBuffer::shift()
        {
            // Check state
            if ((pData == NULL) || (nTail <= nHead))
                return 0.0f;
            return pData[nHead++];
        }

        void ShiftBuffer::copy(const ShiftBuffer *src)
        {
            size_t amount   = nTail - nHead;
            size_t can      = src->nTail - src->nHead;
            if (amount > can)
                amount = can;
        }

        float *ShiftBuffer::data()
        {
            return pData;
        }

        float *ShiftBuffer::head()
        {
            return (pData != NULL) ? &pData[nHead] : NULL;
        }

        float *ShiftBuffer::tail()
        {
            return (pData != NULL) ? &pData[nTail] : NULL;
        }

        float *ShiftBuffer::head(size_t offset)
        {
            if (pData == NULL)
                return NULL;
            offset += nHead;
            return (offset >= nTail) ? NULL : &pData[offset];
        }

        float *ShiftBuffer::tail(size_t offset)
        {
            if (pData == NULL)
                return NULL;
            ssize_t off = nTail - offset;
            return (off < ssize_t(nHead)) ? NULL : &pData[off];
        }

        float ShiftBuffer::last(size_t offset)
        {
            if (pData == NULL)
                return 0.0f;
            ssize_t off = nTail - offset;
            return (off < ssize_t(nHead)) ? 0.0f : pData[off];
        }

        float ShiftBuffer::first() const
        {
            return ((pData != NULL) && (nTail > nHead)) ? pData[nHead] : 0.0f;
        }

        float ShiftBuffer::last() const
        {
            return ((pData != NULL) && (nTail > nHead)) ? pData[nTail-1] : 0.0f;
        }

        float ShiftBuffer::first(size_t offset) const
        {
            if (pData == NULL)
                return 0.0f;
            offset += nHead;
            return (offset >= nTail) ? 0.0f : pData[offset];
        }

        float ShiftBuffer::last(size_t offset) const
        {
            if (pData == NULL)
                return 0.0f;
            ssize_t off     = nTail - offset;
            return (off >= ssize_t(nHead)) ? pData[off] : 0.0f;
        }

        void ShiftBuffer::fill(float value)
        {
            if (nHead < nTail)
                dsp::fill(&pData[nHead], value, nTail - nHead);
        }

        void ShiftBuffer::dump(IStateDumper *v) const
        {
            v->write("pData", pData);
            v->write("nCapacity", nCapacity);
            v->write("nHead", nHead);
            v->write("nTail", nTail);
        }
    }
} /* namespace lsp */
