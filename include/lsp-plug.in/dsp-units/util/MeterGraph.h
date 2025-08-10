/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 20 мая 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_METER_GRAPH_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_METER_GRAPH_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/util/ShiftBuffer.h>

namespace lsp
{
    namespace dspu
    {
        enum meter_method_t
        {
            /**
             * Absolute maximum: max(fabsf(data[0..x]))
             */
            MM_ABS_MAXIMUM,

            /**
             * Absolute minimum: min(fabsf(data[0..x]))
             */
            MM_ABS_MINIMUM,

            /**
             * Sign-dependent minimum: (fabsf(pos) >= fabsf(neg)) ? pos : neg
             *   where pos = fabsf(max(data[0..x], 0.0))
             *     and neg = fabsf(min(data[0..x], 0.0))
             */
            MM_SIGN_MAXIMUM,

            /**
             * Sign-dependent minimum: (fabsf(pos) < fabsf(neg)) ? pos : neg
             *   where pos = fabsf(max(data[0..x], 0.0))
             *     and neg = fabsf(min(data[0..x], 0.0))
             */
            MM_SIGN_MINIMUM,

            /**
             * Varying extremum: (k % 2 == 0) ? max : min
             *   where max = max(data[0..x])
             *     and min = min(data[0..x])
             */
            MM_VAR_MINMAX,
        };

        class LSP_DSP_UNITS_PUBLIC MeterGraph
        {
            protected:
                ShiftBuffer         sBuffer;
                float               fCurrent;
                uint32_t            nCount;
                uint32_t            nSampleId;
                uint32_t            nPeriod;
                meter_method_t      enMethod;

            public:
                explicit MeterGraph();
                MeterGraph(const MeterGraph &) = delete;
                MeterGraph(MeterGraph &&) = delete;
                ~MeterGraph();

                MeterGraph & operator = (const MeterGraph &) = delete;
                MeterGraph & operator = (MeterGraph &&) = delete;

                /**
                 * Construct object
                 */
                void        construct();

                /** Initialize meter graph
                 *
                 * @param frames number of frames used for graph and needed to be stored in internal buffer
                 * @param period strobe period
                 * @return true on success
                 */
                bool        init(size_t frames, size_t period);

                /** Destroy meter graph
                 *
                 */
                void        destroy();

            public:
                /**
                 * Get number of frames
                 * @return number of frames
                 */
                inline size_t get_frames() const        { return sBuffer.size(); }

                /** Set metering method
                 *
                 * @param m metering method
                 */
                inline void set_method(meter_method_t m) { enMethod = m; }

                /** Get data stored in buffer
                 *
                 * @return pointer to the first element of the buffer
                 */
                inline float *data()                    { return sBuffer.head();   }

                /** Set strobe period
                 *
                 * @param period strobe period
                 */
                inline void set_period(size_t period)
                {
                    nPeriod         = uint32_t(period);
                }

                inline float period() const             { return nPeriod; }

                /** Process single sample
                 *
                 * @param sample sample to process
                 */
                void process(float sample);

                /** Process multiple samples
                 *
                 * @param s array of samples
                 * @param n number of samples to process
                 */
                void process(const float *s, size_t n);

                /** Process multiple samples multiplied by specified value
                 *
                 * @param s array of samples
                 * @param n number of samples to process
                 */
                void process(const float *s, float gain, size_t n);

                /** Get current level
                 *
                 * @return current level
                 */
                inline float level() const      { return sBuffer.last(); }

                /** Fill graph with specific level
                 *
                 * @param level level
                 */
                inline void fill(float level)   { sBuffer.fill(level); }

                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_METER_GRAPH_H_ */
