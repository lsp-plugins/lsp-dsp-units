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

#ifndef LSP_PLUG_IN_DSP_UNITS_STAT_QUANTIZEDCOUNTER_H_
#define LSP_PLUG_IN_DSP_UNITS_STAT_QUANTIZEDCOUNTER_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Counter that allows to compute distibution of some signal among it's values
         */
        class QuantizedCounter
        {
            private:
                uint32_t        nPeriod;        // Duration for statistics
                uint32_t        nLevels;        // Number of quantization levels
                uint32_t        nHead;          // Position of the head in the history buffer
                uint32_t        nCapacity;      // Overall capacity of the history buffer
                uint32_t        nCount;         // Number of records in buffer
                uint32_t        nMaxPeriod;     // Maximum possible period
                uint32_t        nMaxLevels;     // Maximum possible levels
                float           fMinValue;      // Minimum value
                float           fMaxValue;      // Maximum value
                float           fRStep;         // The reciprocal of quantization step
                bool            bUpdate;        // Need to update computations

                float          *vHistory;       // History buffer
                uint32_t       *vCounters;      // History counters

                uint8_t        *pData;

            private:
                void            inc_counters(const float *src, size_t count);
                void            dec_counters(const float *src, size_t count);
                void            evict_values();

            public:
                QuantizedCounter();
                QuantizedCounter(const QuantizedCounter &) = delete;
                QuantizedCounter(QuantizedCounter &&) = delete;
                ~QuantizedCounter();

                QuantizedCounter & operator = (const QuantizedCounter &) = delete;
                QuantizedCounter & operator = (QuantizedCounter &&) = delete;

                void            construct();
                void            destroy();

            public:
                /**
                 * Initialize counter
                 * @param max_period maximum period (in samples)
                 * @param max_levels maximum number of quantization levels
                 * @return status of operation
                 */
                status_t        init(size_t max_period, size_t max_levels);

            public:
                /**
                 * Set the statistics estimation period
                 * @param period statistics estimation period
                 */
                void            set_period(size_t period);

                /**
                 * Get the statistics estimation period
                 * @return statistics estimation period
                 */
                inline size_t   period() const          { return nPeriod;       }

                /**
                 * Get maximum possible statistics estimation period
                 * @return maximum possible statistics estimation period
                 */
                inline size_t   max_period() const      { return nMaxPeriod;    }

                /**
                 * Set number of quantization levels
                 * @param levels number of quantization levels
                 */
                void            set_levels(size_t levels);

                inline size_t   levels() const          { return nLevels;       }

                /**
                 * Set minimum quantization range value
                 * @param min minimum range value
                 */
                void            set_min_value(float min);

                /**
                 * Set maximum quantization range value
                 * @param max maximum range value
                 */
                void            set_max_value(float max);

                /**
                 * Set minimum and maximum range values
                 * @param min minimum range value
                 * @param max maximum range value
                 */
                void            set_value_range(float min, float max);

                /**
                 * Set quantization range parameters
                 * @param min minimum range value
                 * @param max maximum range value
                 * @param levels number of quantization levels
                 */
                void            set_range(float min, float max, size_t levels);

                /**
                 * Check if distribution counter needs to be updated
                 * @return true if distribution counter needs to be updated
                 */
                inline bool     needs_update() const        { return bUpdate; }

                /**
                 * Update distribution counter settings
                 */
                void            update_settings();

                /**
                 * Process signal and update distribution counters
                 * @param data data to process
                 * @param count number of samples to process
                 */
                void            process(const float *data, size_t count);

                /**
                 * Clear the statistics
                 */
                void            clear();

                /**
                 * Get actual values of counters
                 * @return actual values of counters
                 */
                inline const uint32_t *counters() const     { return vCounters;     }

                /**
                 * Get count of values that are below the quantization range
                 * (the number of values that give the negative counter index)
                 * @return count of values that are below the quantization range
                 */
                inline uint32_t below() const               { return vCounters[nMaxLevels];         }

                /**
                 * Get count of values that are above the quantization range
                 * (the number of values that give the counter index not less then number of levels set)
                 * @return count of values that are below the quantization range
                 */
                inline uint32_t above() const               { return vCounters[nMaxLevels + 1];     }

                /**
                 * Return number of samples used to compute counters
                 * @return number of samples used to compute counters
                 */
                inline size_t   count() const               { return nCount;        }

                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_STAT_QUANTIZEDCOUNTER_H_ */
