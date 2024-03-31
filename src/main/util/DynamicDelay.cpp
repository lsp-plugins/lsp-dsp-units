/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 15 дек. 2020 г.
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

#include <lsp-plug.in/dsp-units/util/DynamicDelay.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr size_t BUF_SIZE        = 0x400;

        DynamicDelay::DynamicDelay()
        {
            construct();
        }

        DynamicDelay::~DynamicDelay()
        {
            destroy();
        }

        void DynamicDelay::construct()
        {
            vDelay      = NULL;
            nHead       = 0;
            nCapacity   = 0;
            nMaxDelay   = 0;
            pData       = NULL;
        }

        void DynamicDelay::destroy()
        {
            if (pData != NULL)
            {
                free_aligned(pData);

                vDelay      = NULL;
                nHead       = 0;
                nCapacity   = 0;
                nMaxDelay   = 0;
                pData       = NULL;
            }
        }

        status_t DynamicDelay::init(size_t max_size)
        {
            size_t delay        = max_size + 1;
            size_t buf_sz       = delay - (delay % BUF_SIZE) + BUF_SIZE * 2;
            size_t alloc        = buf_sz * sizeof(float);

            uint8_t *data       = NULL;
            uint8_t *ptr        = alloc_aligned<uint8_t>(data, alloc);
            if (ptr == NULL)
                return STATUS_NO_MEM;

            if (pData != NULL)
                free_aligned(pData);

            vDelay              = advance_ptr_bytes<float>(ptr, buf_sz * sizeof(float));

            nHead               = 0;
            nCapacity           = buf_sz;
            nMaxDelay           = max_size;
            pData               = data;

            dsp::fill_zero(vDelay, buf_sz);

            return STATUS_OK;
        }

        void DynamicDelay::clear()
        {
            dsp::fill_zero(vDelay, nCapacity);
            nHead               = 0;
        }

        void DynamicDelay::process(float *out, const float *in, const float *delay, const float *fgain, const float *fdelay, size_t samples)
        {
            for (size_t i=0; i < samples; ++i)
            {
                ssize_t shift   = lsp_limit(ssize_t(delay[i]), 0, nMaxDelay);   // Delay
                ssize_t tail    = nHead - shift;
                if (tail < 0)
                    tail           += nCapacity;
                size_t feed     = tail  + lsp_limit(fdelay[i], 0, shift);       // Feedback delay
                if (feed > nCapacity)
                    feed           -= nCapacity;

                vDelay[nHead]   = in[i];            // Save input sample to buffer
                float s         = vDelay[tail];     // Read delayed sample
                vDelay[feed]   += s * fgain[i];     // Add feedback to the buffer
                out[i]          = vDelay[tail];     // Read the final sample to output buffer

                // Update head pointer
                if ((++nHead) >= nCapacity)
                    nHead = 0;
            }
        }

        /**
         * Copy the contents of the dynamic delay
         * @param s delay to copy contents from
         */
        void DynamicDelay::copy(DynamicDelay *s)
        {
            // Estimate the amount of samples to copy
            size_t count    = lsp_min(nCapacity, s->nCapacity);
            ssize_t dt      = nCapacity - count;    // Position of destination tail
            ssize_t st      = s->nHead - count;     // Position of source tail
            if (st < 0)
                st         += s->nCapacity;

            // Perform data copy
            size_t tail     = s->nCapacity - st;
            if (tail < count)
            {
                dsp::copy(&vDelay[dt], &s->vDelay[st], tail);
                dsp::copy(&vDelay[dt + tail], s->vDelay, count - tail);
            }
            else
                dsp::copy(&vDelay[dt], &s->vDelay[st], count);

            // Clear the rest samples
            dsp::fill_zero(vDelay, dt);

            // Reset head to first sample
            nHead           = 0;
        }

        void DynamicDelay::swap(DynamicDelay *d)
        {
            lsp::swap(vDelay, d->vDelay);
            lsp::swap(nHead, d->nHead);
            lsp::swap(nCapacity, d->nCapacity);
            lsp::swap(nMaxDelay, d->nMaxDelay);
            lsp::swap(pData, d->pData);
        }

        void DynamicDelay::dump(IStateDumper *v) const
        {
            v->write("vDelay", vDelay);
            v->write("nHead", nHead);
            v->write("nCapacity", nCapacity);
            v->write("nMaxDelay", nMaxDelay);
            v->write("pData", pData);
        }

    } /* namespace dspu */
} /* namespace lsp */


