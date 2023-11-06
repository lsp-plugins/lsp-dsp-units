/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 06 дек. 2015 г.
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

#include <lsp-plug.in/dsp-units/util/Delay.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>

#define DELAY_GAP       0x200

namespace lsp
{
    namespace dspu
    {
        Delay::Delay()
        {
            construct();
        }

        Delay::~Delay()
        {
            destroy();
        }
    
        void Delay::construct()
        {
            pBuffer     = NULL;
            nHead       = 0;
            nTail       = 0;
            nDelay      = 0;
            nSize       = 0;
        }

        bool Delay::init(size_t max_size)
        {
            size_t size     = align_size(max_size + DELAY_GAP, DELAY_GAP);

            float *ptr      = static_cast<float *>(::realloc(pBuffer, size * sizeof(float)));
            if (ptr == NULL)
                return false;
            pBuffer         = ptr;

            dsp::fill_zero(pBuffer, size);
            nHead           = 0;
            nTail           = 0;
            nDelay          = 0;
            nSize           = size;

            return true;
        }

        void Delay::destroy()
        {
            if (pBuffer != NULL)
            {
                ::free(pBuffer);
                pBuffer     = NULL;
            }
        }

        void Delay::process(float *dst, const float *src, size_t count)
        {
            size_t free_gap = nSize - nDelay;

            while (count > 0)
            {
                // Determine how many samples to process
                size_t to_do    = lsp_min(count, free_gap);

                // Push data to buffer
                size_t end      = nHead + to_do;
                if (end > nSize)
                {
                    size_t hcut     = nSize-nHead;
                    dsp::copy(&pBuffer[nHead], src, hcut);
                    dsp::copy(pBuffer, &src[hcut], end-nSize);
                }
                else
                    dsp::copy(&pBuffer[nHead], src, to_do);
                nHead           = (nHead + to_do) % nSize;
                src            += to_do;

                // Shift data from buffer
                end             = nTail + to_do;
                if (end > nSize)
                {
                    size_t tcut     = nSize-nTail;
                    dsp::copy(dst, &pBuffer[nTail], tcut);
                    dsp::copy(&dst[tcut], pBuffer, end-nSize);
                }
                else
                    dsp::copy(dst, &pBuffer[nTail], to_do);

                nTail           = (nTail + to_do) % nSize;
                dst            += to_do;

                // Update number of samples
                count          -= to_do;
            }
        }

        void Delay::process(float *dst, const float *src, float gain, size_t count)
        {
            size_t free_gap = nSize - nDelay;

            while (count > 0)
            {
                // Determine how many samples to process
                size_t to_do    = lsp_min(count, free_gap);

                // Push data to buffer
                size_t end      = nHead + to_do;
                if (end > nSize)
                {
                    size_t hcut     = nSize-nHead;
                    dsp::copy(&pBuffer[nHead], src, hcut);
                    dsp::copy(pBuffer, &src[hcut], end-nSize);
                }
                else
                    dsp::copy(&pBuffer[nHead], src, to_do);
                nHead           = (nHead + to_do) % nSize;
                src            += to_do;

                // Shift data from buffer
                end             = nTail + to_do;
                if (end > nSize)
                {
                    size_t tcut     = nSize-nTail;
                    dsp::mul_k3(dst, &pBuffer[nTail], gain, tcut);
                    dsp::mul_k3(&dst[tcut], pBuffer, gain, end-nSize);
                }
                else
                    dsp::mul_k3(dst, &pBuffer[nTail], gain, to_do);

                nTail           = (nTail + to_do) % nSize;
                dst            += to_do;

                // Update number of samples
                count          -= to_do;
            }
        }

        void Delay::process(float *dst, const float *src, const float *gain, size_t count)
        {
            size_t free_gap     = nSize - nDelay;

            while (count > 0)
            {
                // Determine how many samples to process
                size_t to_do    = lsp_min(count, free_gap);

                // Push data to buffer
                size_t end      = nHead + to_do;
                if (end > nSize)
                {
                    size_t hcut     = nSize-nHead;
                    dsp::copy(&pBuffer[nHead], src, hcut);
                    dsp::copy(pBuffer, &src[hcut], end-nSize);
                }
                else
                    dsp::copy(&pBuffer[nHead], src, to_do);
                nHead           = (nHead + to_do) % nSize;
                src            += to_do;

                // Shift data from buffer
                end             = nTail + to_do;
                if (end > nSize)
                {
                    size_t tcut     = nSize-nTail;
                    dsp::mul3(dst, &pBuffer[nTail], gain, tcut);
                    dsp::mul3(&dst[tcut], pBuffer, &gain[tcut], end-nSize);
                }
                else
                    dsp::mul3(dst, &pBuffer[nTail], gain, to_do);

                nTail           = (nTail + to_do) % nSize;
                dst            += to_do;
                gain           += to_do;

                // Update number of samples
                count          -= to_do;
            }
        }

        void Delay::process_ramping(float *dst, const float *src, size_t delay, size_t count)
        {
            // If delay does not change - use faster algorithm
            if (delay == nDelay)
            {
                process(dst, src, count);
                return;
            }
            else if (count <= 0)
                return;

            // More slower algorithm
            const size_t free_gap   = nSize - lsp_max(delay, nDelay);
            const float delta       = 1.0f + float(ssize_t(nDelay) - ssize_t(delay)) / float(count);
            const size_t old_tail   = nTail;

            for (size_t offset = 0; offset < count; )
            {
                // Determine how many samples to process
                size_t to_do    = lsp_min(count - offset, free_gap);

                // Push data to buffer
                size_t end      = nHead + to_do;
                if (end > nSize)
                {
                    size_t hcut     = nSize-nHead;
                    dsp::copy(&pBuffer[nHead], src, hcut);
                    dsp::copy(pBuffer, &src[hcut], end-nSize);
                }
                else
                    dsp::copy(&pBuffer[nHead], src, to_do);

                // Shift data from buffer, slower because delay is changing
                for (size_t i=0; i<to_do; ++i, ++offset)
                {
                    size_t tail     = (old_tail + ssize_t(delta * offset)) % nSize;
                    dst[i]          = pBuffer[tail];
                }

                // Update pointers
                nHead           = (nHead + to_do) % nSize;
                src            += to_do;
                dst            += to_do;
            }

            nTail           = (nHead + nSize - delay) % nSize;
            nDelay          = delay;
        }

        void Delay::process_ramping(float *dst, const float *src, float gain, size_t delay, size_t count)
        {
            // If delay does not change - use faster algorithm
            if (delay == nDelay)
            {
                process(dst, src, gain, count);
                return;
            }
            else if (count <= 0)
                return;

            // More slower algorithm
            const size_t free_gap   = nSize - lsp_max(delay, nDelay);
            const float delta       = 1.0f + float(ssize_t(nDelay) - ssize_t(delay)) / float(count);
            const size_t old_tail   = nTail;

            for (size_t offset = 0; offset < count; )
            {
                // Determine how many samples to process
                size_t to_do    = lsp_min(count - offset, free_gap);

                // Push data to buffer
                size_t end      = nHead + to_do;
                if (end > nSize)
                {
                    size_t hcut     = nSize-nHead;
                    dsp::copy(&pBuffer[nHead], src, hcut);
                    dsp::copy(pBuffer, &src[hcut], end-nSize);
                }
                else
                    dsp::copy(&pBuffer[nHead], src, to_do);

                // Shift data from buffer, slower because delay is changing
                for (size_t i=0; i<to_do; ++i, ++offset)
                {
                    size_t tail     = (old_tail + ssize_t(delta * offset)) % nSize;
                    dst[i]          = pBuffer[tail] * gain;
                }

                // Update pointers
                nHead           = (nHead + to_do) % nSize;
                src            += to_do;
                dst            += to_do;
            }

            nTail           = (nHead + nSize - delay) % nSize;
            nDelay          = delay;
        }

        void Delay::process_ramping(float *dst, const float *src, const float *gain, size_t delay, size_t count)
        {
            // If delay does not change - use faster algorithm
            if (delay == nDelay)
            {
                process(dst, src, gain, count);
                return;
            }
            else if (count <= 0)
                return;

            // More slower algorithm
            const size_t free_gap   = nSize - lsp_max(delay, nDelay);
            const float delta       = 1.0f + float(ssize_t(nDelay) - ssize_t(delay)) / float(count);
            const size_t old_tail   = nTail;

            for (size_t offset = 0; offset < count; )
            {
                // Determine how many samples to process
                size_t to_do    = lsp_min(count - offset, free_gap);

                // Push data to buffer
                size_t end      = nHead + to_do;
                if (end > nSize)
                {
                    size_t hcut     = nSize-nHead;
                    dsp::copy(&pBuffer[nHead], src, hcut);
                    dsp::copy(pBuffer, &src[hcut], end-nSize);
                }
                else
                    dsp::copy(&pBuffer[nHead], src, to_do);

                // Shift data from buffer, slower because delay is changing
                for (size_t i=0; i<to_do; ++i, ++offset)
                {
                    size_t tail     = (old_tail + ssize_t(delta * offset)) % nSize;
                    dst[i]          = pBuffer[tail] * gain[i];
                }

                // Update pointers
                nHead           = (nHead + to_do) % nSize;
                src            += to_do;
                dst            += to_do;
                gain           += to_do;
            }

            nTail           = (nHead + nSize - delay) % nSize;
            nDelay          = delay;
        }

        float Delay::process(float src)
        {
            pBuffer[nHead]  = src;
            float ret       = pBuffer[nTail];
            nHead           = (nHead + 1) % nSize;
            nTail           = (nTail + 1) % nSize;

            return ret;
        }

        float Delay::process(float src, float gain)
        {
            pBuffer[nHead]  = src;
            float ret       = pBuffer[nTail] * gain;
            nHead           = (nHead + 1) % nSize;
            nTail           = (nTail + 1) % nSize;

            return ret;
        }

        void Delay::set_delay(size_t delay)
        {
            delay      %= nSize;
            nDelay      = delay;
            nTail       = (nHead + nSize - delay) % nSize;
        }

        void Delay::clear()
        {
            if (pBuffer == NULL)
                return;
            dsp::fill_zero(pBuffer, nSize);
        }

        void Delay::dump(IStateDumper *v) const
        {
            v->write("pBuffer", pBuffer);
            v->write("nHead", nHead);
            v->write("nTail", nTail);
            v->write("nDelay", nDelay);
            v->write("nSize", nSize);
        }
    } /* namespace dspu */
} /* namespace lsp */
