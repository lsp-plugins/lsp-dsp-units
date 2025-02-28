/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 12 нояб. 2024 г.
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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp-units/meters/Panometer.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        constexpr size_t BUFFER_SIZE        = 0x400;

        Panometer::Panometer()
        {
            construct();
        }

        Panometer::~Panometer()
        {
            destroy();
        }

        void Panometer::construct()
        {
            vInA        = NULL;
            vInB        = NULL;
            enPanLaw    = PAN_LAW_EQUAL_POWER;
            fValueA     = 0.0f;
            fValueB     = 0.0f;
            fNorm       = 0.0f;
            fDefault    = 0.5f;
            nCapacity   = 0;
            nHead       = 0;
            nMaxPeriod  = 0;
            nPeriod     = 0;
            nWindow     = 0;

            pData       = NULL;
        }

        void Panometer::destroy()
        {
            if (pData != NULL)
            {
                free_aligned(pData);
                vInA        = NULL;
                vInB        = NULL;
                pData       = NULL;
            }
        }

        status_t Panometer::init(size_t max_period)
        {
            destroy();

            // Allocate data
            const size_t capacity   = align_size(max_period + BUFFER_SIZE, DEFAULT_ALIGN);
            const size_t szof_buf   = capacity * sizeof(float);
            const size_t to_alloc   = 2 * szof_buf;
            uint8_t *data           = NULL;

            uint8_t *ptr            = alloc_aligned<uint8_t>(data, to_alloc);
            if (ptr == NULL)
                return STATUS_NO_MEM;

            // Commit state
            vInA        = advance_ptr_bytes<float>(ptr, szof_buf);
            vInB        = advance_ptr_bytes<float>(ptr, szof_buf);
            nCapacity   = uint32_t(capacity);
            nHead       = 0;
            nMaxPeriod  = uint32_t(max_period);
            nPeriod     = 0;

            free_aligned(pData);
            pData       = data;

            // Cleanup buffers (both vInA and vInB -> nCapacity * 2)
            dsp::fill_zero(vInA, nCapacity * 2);

            return STATUS_OK;
        }

        void Panometer::set_pan_law(pan_law_t law)
        {
            enPanLaw    = law;
        }

        void Panometer::set_default_pan(float dfl)
        {
            fDefault    = dfl;
        }

        void Panometer::set_period(size_t period)
        {
            period      = lsp_min(period, nMaxPeriod);
            if (period == nPeriod)
                return;

            nPeriod     = uint32_t(period);
            nWindow     = uint32_t(period);
            fValueA     = 0.0f;
            fValueB     = 0.0f;
            fNorm       = (period > 0) ? 1.0f / period : 1.0f;
        }

        void Panometer::clear()
        {
            dsp::fill_zero(vInA, nCapacity);
            dsp::fill_zero(vInB, nCapacity);

            nHead       = nPeriod;
        }

        void Panometer::process(float *dst, const float *a, const float *b, size_t count)
        {
            for (size_t offset=0; offset<count; )
            {
                // Check that we need to reset the window
                const size_t tail = (nHead + nCapacity - nPeriod) % nCapacity;

                if (nWindow >= nPeriod)
                {
                    fValueA     = 0.0f;
                    fValueB     = 0.0f;

                    if (nHead < tail)
                    {
                        fValueA     = dsp::h_sum(&vInA[tail], nCapacity - tail);
                        fValueB     = dsp::h_sum(&vInB[tail], nCapacity - tail);
                        fValueA    += dsp::h_sum(&vInA[0], nHead);
                        fValueB    += dsp::h_sum(&vInB[0], nHead);
                    }
                    else
                    {
                        fValueA     = dsp::h_sum(&vInA[tail], nPeriod);
                        fValueB     = dsp::h_sum(&vInB[tail], nPeriod);
                    }
                    nWindow     = 0;
                }

                const size_t can_do = nPeriod - nWindow;
                size_t to_do = lsp_min(
                    count - offset,                             // Number of samples left in input buffers
                    size_t(nCapacity - nMaxPeriod),             // Number of free samples in the ring buffer
                    size_t(nCapacity - nHead),                  // The number of samples before head goes out of ring buffer
                    nCapacity - tail);                          // The number of samples before tail goes out of ring buffer
                to_do = lsp_min(to_do, can_do);

                // Fill buffers with data
                float *ah   = &vInA[nHead];
                float *bh   = &vInB[nHead];
                float *at   = &vInA[tail];
                float *bt   = &vInB[tail];

                dsp::sqr2(ah, &a[offset], to_do);
                dsp::sqr2(bh, &b[offset], to_do);

                // Estimate the actual pan value
                float va    = fValueA;
                float vb    = fValueB;
                if (enPanLaw == PAN_LAW_LINEAR)
                {
                    for (size_t i=0; i<to_do; ++i)
                    {
                        va      = va + ah[i] - at[i];
                        vb      = vb + bh[i] - bt[i];

                        const float sl  = sqrtf(fabsf(va) * fNorm);
                        const float sr  = sqrtf(fabsf(vb) * fNorm);
                        const float den = sl + sr;
                        dst[i]          = (den > 1e-18f) ? sr / den : fDefault;
                    }
                }
                else
                {
                    for (size_t i=0; i<to_do; ++i)
                    {
                        va              = va + ah[i] - at[i];
                        vb              = vb + bh[i] - bt[i];

                        const float sl  = fabsf(va) * fNorm;
                        const float sr  = fabsf(vb) * fNorm;

                        const float den = sl + sr;
                        dst[i]          = (den > 1e-36f) ? sr / den : fDefault;
                    }
                }
                fValueA     = va;
                fValueB     = vb;

                // Update pointers and counters
                nHead       = (nHead + to_do) % nCapacity;
                nWindow    += to_do;
                offset     += to_do;
                dst        += to_do;
            }
        }

        void Panometer::dump(IStateDumper *v) const
        {
            v->write("vInA", vInA);
            v->write("vInB", vInB);
            v->write("enPanLaw", enPanLaw);
            v->write("fValueA", fValueA);
            v->write("fValueB", fValueB);
            v->write("fNorm", fNorm);
            v->write("fDefault", fDefault);
            v->write("nCapacity", nCapacity);
            v->write("nHead", nHead);
            v->write("nMaxPeriod", nMaxPeriod);
            v->write("nPeriod", nPeriod);
            v->write("nWindow", nWindow);

            v->write("pData", pData);
        }

    } /* namespace dspu */
} /* namespace lsp */


