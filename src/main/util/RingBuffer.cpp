/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/util/RingBuffer.h>
#include <lsp-plug.in/stdlib/stdlib.h>

namespace lsp
{
    namespace dspu
    {
        RingBuffer::RingBuffer()
        {
            construct();
        }

        RingBuffer::~RingBuffer()
        {
            destroy();
        }

        void RingBuffer::construct()
        {
            pData       = NULL;
            nCapacity   = 0;
            nHead       = 0;
        }

        bool RingBuffer::init(size_t size)
        {
            float *data     = static_cast<float *>(realloc(pData, size * sizeof(float)));
            if (data == NULL)
                return false;

            lsp::swap(pData, data);
            nCapacity       = size;
            nHead           = 0;

            dsp::fill_zero(pData, size);
            return true;
        }

        void RingBuffer::destroy()
        {
            if (pData != NULL)
            {
                free(pData);
                pData       = NULL;
            }
            nCapacity       = 0;
            nHead           = 0;
        }

        size_t RingBuffer::append(const float *data, size_t count)
        {
            if (count > nCapacity)
            {
                nHead           = 0;
                dsp::copy(pData, &data[count - nCapacity], nCapacity);
                return nCapacity;
            }

            if ((nHead + count) > nCapacity)
            {
                size_t part1    = nCapacity - nHead;
                size_t part2    = count - part1;
                dsp::copy(&pData[nHead], data, part1);
                dsp::copy(pData, &data[part1], part2);
                nHead           = part2;
            }
            else
            {
                dsp::copy(&pData[nHead], data, count);
                nHead          += count;
            }

            return count;
        }

        void RingBuffer::append(float data)
        {
            pData[nHead]    = data;
            nHead           = (nHead + 1) % nCapacity;
        }

        void RingBuffer::clear()
        {
            nHead           = 0;
            if (pData != NULL)
                dsp::fill_zero(pData, nCapacity);
        }

        float *RingBuffer::data()
        {
            return pData;
        }

        float RingBuffer::get(size_t offset)
        {
            if (offset >= nCapacity)
                return 0.0f;

            size_t index    = (nHead + nCapacity - offset - 1) % nCapacity;
            return pData[index];
        }

        size_t RingBuffer::get(float *dst, size_t offset, size_t count)
        {
            size_t to_read;

            // Check if we are outside of the buffer
            if (offset >= nCapacity)
            {
                to_read         = lsp_min(count, offset - nCapacity + 1);
                dsp::fill_zero(dst, to_read);

                offset         -= to_read;
                if (offset >= nCapacity)
                    return 0;

                count          -= to_read;
                dst            += to_read;
            }

            // Perform the read
            size_t tail     = (nHead + nCapacity - offset - 1) % nCapacity;
            to_read         = lsp_min(count, offset + 1);
            if ((tail + to_read) > nCapacity)
            {
                size_t part1    = nCapacity - tail;
                size_t part2    = to_read - part1;
                dsp::copy(dst, &pData[tail], part1);
                dsp::copy(&dst[part1], pData, part2);
            }
            else
                dsp::copy(dst, &pData[tail], to_read);

            // Is there a tail present?
            if (count > to_read)
                dsp::fill_zero(&dst[to_read], count - to_read);

            return to_read;
        }

        void RingBuffer::dump(IStateDumper *v) const
        {
            v->write("pData", pData);
            v->write("nCapacity", nCapacity);
            v->write("nHead", nHead);
        }
    } /* namespace dspu */
} /* namespace lsp */



