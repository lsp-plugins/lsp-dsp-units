/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 8 мар. 2024 г.
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
#include <lsp-plug.in/dsp-units/meters/Correlometer.h>

namespace lsp
{
    namespace dspu
    {
        constexpr size_t BUFFER_SIZE        = 0x400;

        Correlometer::Correlometer()
        {
            construct();
        }

        Correlometer::~Correlometer()
        {
            destroy();
        }

        void Correlometer::construct()
        {
            sCorr.v     = 0.0f;
            sCorr.a     = 0.0f;
            sCorr.b     = 0.0f;

            vInA        = NULL;
            vInB        = NULL;
            nCapacity   = 0;
            nHead       = 0;
            nMaxPeriod  = 0;
            nPeriod     = 0;
            nWindow     = 0;
            nFlags      = CF_UPDATE;

            pData       = NULL;
        }

        void Correlometer::destroy()
        {
            free_aligned(pData);

            vInA        = NULL;
            vInB        = NULL;
            pData       = NULL;
        }

        status_t Correlometer::init(size_t max_period)
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
            sCorr.v     = 0.0f;
            sCorr.a     = 0.0f;
            sCorr.b     = 0.0f;

            vInA        = advance_ptr_bytes<float>(ptr, szof_buf);
            vInB        = advance_ptr_bytes<float>(ptr, szof_buf);
            nCapacity   = uint32_t(capacity);
            nHead       = 0;
            nMaxPeriod  = uint32_t(max_period);
            nPeriod     = 0;
            nFlags      = 0;

            pData       = data;

            // Cleanup buffers (both vInA and vInB -> nCapacity * 2)
            dsp::fill_zero(vInA, nCapacity * 2);

            return STATUS_OK;
        }

        void Correlometer::set_period(size_t period)
        {
            period      = lsp_min(period, nMaxPeriod);
            if (period == nPeriod)
                return;

            nPeriod     = uint32_t(period);
            nFlags     |= CF_UPDATE;
        }

        void Correlometer::update_settings()
        {
            if (nFlags == 0)
                return;

            nWindow     = nPeriod;
            nFlags      = 0;
        }

        void Correlometer::clear()
        {
            dsp::fill_zero(vInA, nCapacity);
            dsp::fill_zero(vInB, nCapacity);

            sCorr.v     = 0.0f;
            sCorr.a     = 0.0f;
            sCorr.b     = 0.0f;

            nHead       = nPeriod;
        }

        void Correlometer::process(float *dst, const float *a, const float *b, size_t count)
        {
            update_settings();

            for (size_t offset=0; offset<count; )
            {
                const size_t tail = (nHead + nCapacity - nPeriod) % nCapacity;

                // Check that we need to reset the window
                if (nWindow >= nPeriod)
                {
                    sCorr.v     = 0.0f;
                    sCorr.a     = 0.0f;
                    sCorr.b     = 0.0f;

                    if (nHead < tail)
                    {
                        dsp::corr_init(&sCorr, &vInA[tail], &vInB[tail], nCapacity - tail);
                        dsp::corr_init(&sCorr, vInA, vInB, nHead);
                    }
                    else
                        dsp::corr_init(&sCorr, &vInA[tail], &vInB[tail], nPeriod);
                    nWindow     = 0;
                }

                const size_t can_do = nPeriod - nWindow;
                size_t to_do = lsp_min(
                    count - offset,                             // Number of samples left in input buffers
                    size_t(nCapacity - nMaxPeriod),             // Number of free samples in the ring buffer
                    size_t(nCapacity - nHead),                  // The number of samples before head goes out of ring buffer
                    size_t(nCapacity - tail));                  // The number of samples before tail goes out of ring buffer
                to_do = lsp_min(to_do, can_do);

                // Fill buffers with data
                dsp::copy(&vInA[nHead], &a[offset], to_do);
                dsp::copy(&vInB[nHead], &b[offset], to_do);

                // Compute the correlation
                dsp::corr_incr(
                    &sCorr,
                    &dst[offset],
                    &vInA[nHead], &vInB[nHead],
                    &vInA[tail], &vInB[tail],
                    to_do);

                // Update pointers and counters
                nHead       = (nHead + to_do) % nCapacity;
                nWindow    += to_do;
                offset     += to_do;
            }
        }

        void Correlometer::dump(IStateDumper *v) const
        {
            v->begin_object("sCorr", &sCorr, sizeof(dsp::correlation_t));
            {
                v->write("v", sCorr.v);
                v->write("a", sCorr.a);
                v->write("b", sCorr.b);
            }
            v->end_object();

            v->write("vInA", vInA);
            v->write("vInB", vInB);
            v->write("nCapacity", nCapacity);
            v->write("nHead", nHead);
            v->write("nMaxPeriod", nMaxPeriod);
            v->write("nPeriod", nPeriod);
            v->write("nWindow", nWindow);
            v->write("nFlags", nFlags);

            v->write("pData", pData);
        }

    } /* namespace dspu */
} /* namespace lsp */


