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

#include <lsp-plug.in/dsp-units/util/RawRingBuffer.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/stdlib/stdlib.h>

namespace lsp
{
    namespace dspu
    {
        RawRingBuffer::RawRingBuffer()
        {
            construct();
        }

        RawRingBuffer::~RawRingBuffer()
        {
            destroy();
        }

        void RawRingBuffer::construct()
        {
            pData       = NULL;
            nCapacity   = 0;
            nHead       = 0;
        }

        bool RawRingBuffer::init(size_t size)
        {
            float *data     = static_cast<float *>(realloc(pData, size * sizeof(float)));
            if (data == NULL)
                return false;

            pData           = data;
            nCapacity       = size;
            nHead           = 0;

            dsp::fill_zero(pData, size);
            return true;
        }

        void RawRingBuffer::destroy()
        {
            if (pData != NULL)
            {
                free(pData);
                pData       = NULL;
            }
            nCapacity       = 0;
            nHead           = 0;
        }

        void RawRingBuffer::clear()
        {
            nHead           = 0;
            if (pData != NULL)
                dsp::fill_zero(pData, nCapacity);
        }

        void RawRingBuffer::reset()
        {
            nHead           = 0;
        }

        size_t RawRingBuffer::write(const float *data, size_t count)
        {
            count               = lsp_min(count, nCapacity);

            if ((nHead + count) > nCapacity)
            {
                const size_t part1  = nCapacity - nHead;
                const size_t part2  = count - part1;
                dsp::copy(&pData[nHead], data, part1);
                dsp::copy(pData, &data[part1], part2);
            }
            else
                dsp::copy(&pData[nHead], data, count);

            return count;
        }

        void RawRingBuffer::write(float data)
        {
            pData[nHead]        = data;
        }

        size_t RawRingBuffer::push(const float *data, size_t count)
        {
            count               = lsp_min(count, nCapacity);

            if ((nHead + count) > nCapacity)
            {
                const size_t part1  = nCapacity - nHead;
                const size_t part2  = count - part1;
                dsp::copy(&pData[nHead], data, part1);
                dsp::copy(pData, &data[part1], part2);
                nHead               = part2;
            }
            else
            {
                dsp::copy(&pData[nHead], data, count);
                nHead              += count;
            }

            return count;
        }

        void RawRingBuffer::push(float data)
        {
            pData[nHead]        = data;
            nHead               = (nHead + 1) % nCapacity;
        }

        size_t RawRingBuffer::read(float *dst, size_t offset, size_t count)
        {
            count               = lsp_min(count, nCapacity);
            const size_t tail   = (nHead + nCapacity - offset) % nCapacity;

            if ((tail + count) > nCapacity)
            {
                const size_t part1  = nCapacity - tail;
                const size_t part2  = count - part1;
                dsp::copy(dst, &pData[tail], part1);
                dsp::copy(&dst[part1], pData, part2);
            }
            else
                dsp::copy(dst, &pData[tail], count);

            return count;
        }

        float RawRingBuffer::read(size_t offset)
        {
            const size_t tail   = (nHead + nCapacity - offset) % nCapacity;
            return pData[tail];
        }

        float *RawRingBuffer::advance(size_t count)
        {
            nHead           = (nHead + count) % nCapacity;
            return &pData[nHead];
        }

        float *RawRingBuffer::tail(size_t offset)
        {
            const size_t tail   = (nHead + nCapacity - offset) % nCapacity;
            return &pData[tail];
        }

        const float *RawRingBuffer::tail(size_t offset) const
        {
            const size_t tail   = (nHead + nCapacity - offset) % nCapacity;
            return &pData[tail];
        }

        size_t RawRingBuffer::tail_remaining(size_t offset) const
        {
            const size_t tail   = (nHead + nCapacity - offset) % nCapacity;
            return nCapacity - tail;
        }

        size_t RawRingBuffer::remaining(size_t offset) const
        {
            const size_t tail   = (nHead + nCapacity - offset) % nCapacity;
            return lsp_min(nCapacity - nHead, nCapacity - tail);
        }

        void RawRingBuffer::dump(IStateDumper *v) const
        {
            v->write("pData", pData);
            v->write("nCapacity", nCapacity);
            v->write("nHead", nHead);
        }

    } /* namespace dspu */
} /* namespace lsp */

