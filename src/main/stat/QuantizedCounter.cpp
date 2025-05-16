/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 13 нояб. 2024 г.
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
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/stat/QuantizedCounter.h>

namespace lsp
{
    namespace dspu
    {
        constexpr size_t BUFFER_GAP     = 0x400;

        QuantizedCounter::QuantizedCounter()
        {
            construct();
        }

        QuantizedCounter::~QuantizedCounter()
        {
            destroy();
        }

        void QuantizedCounter::construct()
        {
            nPeriod         = 0;
            nLevels         = 0;
            nHead           = 0;
            nCapacity       = 0;
            nCount          = 0;
            nMaxPeriod      = 0;
            nMaxLevels      = 0;

            fMinValue       = 0.0f;
            fMaxValue       = 0.0f;
            fRStep          = 0.0f;

            bUpdate         = true;

            vHistory        = NULL;
            vCounters       = NULL;

            pData           = NULL;
        }

        void QuantizedCounter::destroy()
        {
            free_aligned(pData);

            vHistory        = NULL;
            vCounters       = NULL;
        }

        status_t QuantizedCounter::init(size_t max_period, size_t max_levels)
        {
            size_t szof_history     = align_size((max_period + BUFFER_GAP) * sizeof(float), DEFAULT_ALIGN);
            size_t szof_levels     = align_size((max_levels + 2) * sizeof(uint32_t), DEFAULT_ALIGN);

            uint8_t *data           = NULL;
            uint8_t *ptr            = alloc_aligned<uint8_t>(data, szof_history + szof_levels);
            if (ptr == NULL)
                return STATUS_NO_MEM;

            vHistory                = advance_ptr_bytes<float>(ptr, szof_history);
            vCounters               = advance_ptr_bytes<uint32_t>(ptr, szof_levels);
            nHead                   = 0;
            nCapacity               = uint32_t(szof_history / sizeof(float));
            nCount                  = 0;
            nMaxPeriod              = uint32_t(max_period);
            nMaxLevels              = uint32_t(max_levels);

            dsp::fill_zero(vHistory, nCapacity);
            for (size_t i=0; i<max_levels + 2; ++i)
                vCounters[i]            = 0;

            free_aligned(pData);
            pData                   = data;

            return STATUS_OK;
        }

        void QuantizedCounter::set_period(size_t period)
        {
            period          = lsp_limit(period, 1u, nMaxPeriod);
            if (period == nPeriod)
                return;

            nPeriod         = uint32_t(period);
        }

        void QuantizedCounter::set_levels(size_t levels)
        {
            levels          = lsp_limit(levels, 1u, nMaxLevels);
            if (levels == nLevels)
                return;

            nLevels         = uint32_t(levels);
            bUpdate         = true;
        }

        void QuantizedCounter::set_min_value(float min)
        {
            if (fMinValue == min)
                return;

            fMinValue       = min;
            bUpdate         = true;
        }

        void QuantizedCounter::set_max_value(float max)
        {
            if (fMaxValue == max)
                return;

            fMaxValue       = max;
            bUpdate         = true;
        }

        void QuantizedCounter::set_value_range(float min, float max)
        {
            if ((fMinValue == min) &&
                (fMaxValue == max))
                return;

            fMinValue       = min;
            fMaxValue       = max;
            bUpdate         = true;
        }

        void QuantizedCounter::set_range(float min, float max, size_t levels)
        {
            levels          = lsp_limit(levels, 1u, nMaxLevels);
            if ((nLevels == levels) &&
                (fMinValue == min) &&
                (fMaxValue == max))
                return;

            nLevels         = uint32_t(levels);
            fMinValue       = min;
            fMaxValue       = max;
            bUpdate         = true;
        }

        void QuantizedCounter::update_settings()
        {
            if (!bUpdate)
                return;
            bUpdate         = false;

            // Update step and cleanup counters
            fRStep          = float(nLevels) / (fMaxValue - fMinValue);
            for (size_t i=0; i<nMaxLevels + 2; ++i)
                vCounters[i]            = 0;

            // Update counters according to the stored samples in the history buffer
            if (nCount <= 0)
                return;

            size_t tail     = (nHead + nCapacity - nCount) % nCapacity;
            for (size_t i=0; i<nCount; )
            {
                const size_t to_do  = lsp_min(nCount - i, nCapacity - tail);

                inc_counters(&vHistory[tail], to_do);

                i                  += to_do;
                tail                = (tail + to_do) % nCapacity;
            }
        }

        void QuantizedCounter::clear()
        {
            nHead           = 0;
            nCount          = 0;

            for (size_t i=0; i<nMaxLevels + 2; ++i)
                vCounters[i]            = 0;
        }

        void QuantizedCounter::inc_counters(const float *src, size_t count)
        {
            const int32_t max_level     = nLevels;
            for (size_t i=0; i<count; ++i)
            {
                int32_t index           = (src[i] - fMinValue) * fRStep;
                if (index < 0)
                    index                   = nMaxLevels;
                else if (index >= max_level)
                    index                   = nMaxLevels + 1;

                ++vCounters[index];
            }
        }

        void QuantizedCounter::dec_counters(const float *src, size_t count)
        {
            const int32_t max_level     = nLevels;
            for (size_t i=0; i<count; ++i)
            {
                int32_t index           = (src[i] - fMinValue) * fRStep;
                if (index < 0)
                    index                   = nMaxLevels;
                else if (index >= max_level)
                    index                   = nMaxLevels + 1;

                --vCounters[index];
            }
        }

        void QuantizedCounter::evict_values()
        {
            size_t tail     = (nHead + nCapacity - nCount) % nCapacity;
            while (nCount > nPeriod)
            {
                const size_t to_do  = lsp_min(size_t(nCount - nPeriod), size_t(nCapacity - tail));

                dec_counters(&vHistory[tail], to_do);

                nCount             -= to_do;
                tail                = (tail + to_do) % nCapacity;
            }
        }

        void QuantizedCounter::process(const float *data, size_t count)
        {
            // Update settings if necessary
            update_settings();

            // First, we need to evict values that exceed the maximum allowed period
            evict_values();

            for (size_t offset=0; offset < count; )
            {
                const size_t to_do  = lsp_min(count - offset, nCapacity - nHead);

                // Put new values to the buffer and update statistics
                dsp::copy(&vHistory[nHead], &data[offset], to_do);
                inc_counters(&data[offset], to_do);
                nCount             += to_do;
                nHead               = (nHead + to_do) % nCapacity;

                // Evict old values from the buffer
                evict_values();

                // Advance in cycle
                offset             += to_do;
            }
        }

        void QuantizedCounter::dump(IStateDumper *v) const
        {
            v->write("nPeriod", nPeriod);
            v->write("nLevels", nLevels);
            v->write("nHead", nHead);
            v->write("nCapacity", nCapacity);
            v->write("nCount", nCount);
            v->write("nMaxPeriod", nMaxPeriod);
            v->write("nMaxLevels", nMaxLevels);

            v->write("fMinValue", fMinValue);
            v->write("fMaxValue", fMaxValue);
            v->write("fRStep", fRStep);
            v->write("bUpdate", bUpdate);

            v->write("vHistory", vHistory);
            v->write("vCounters", vCounters);

            v->write("pData", pData);
        }

    } /* namespace dspu */
} /* namespace lsp */

