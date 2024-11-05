/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 05 окт 2024 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_SCALEDMETERGRAPH_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_SCALEDMETERGRAPH_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/util/RawRingBuffer.h>
#include <lsp-plug.in/dsp-units/util/MeterGraph.h>

namespace lsp
{
    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC ScaledMeterGraph
        {
            protected:
                typedef struct sampler_t
                {
                    RawRingBuffer       sBuffer;        // History buffer
                    float               fCurrent;       // Current sample
                    uint32_t            nCount;         // Number of samples processed
                    uint32_t            nPeriod;        // Sampling period
                    uint32_t            nFrames;        // Number of frames
                } sampler_t;

            protected:
                sampler_t           sHistory;
                sampler_t           sFrames;

                uint32_t            nPeriod;
                uint32_t            nMaxPeriod;
                meter_method_t      enMethod;

            private:
                bool                update_period();

                static void         dump_sampler(IStateDumper *v, const char *name, const sampler_t *s);

                void                process_sampler(sampler_t *sampler, float sample);
                void                process_sampler(sampler_t *sampler, const float *src, size_t count);
                void                process_sampler(sampler_t *sampler, const float *src, float gain, size_t count);

            public:
                explicit ScaledMeterGraph();
                ScaledMeterGraph(const ScaledMeterGraph &) = delete;
                ScaledMeterGraph(ScaledMeterGraph &&) = delete;
                ~ScaledMeterGraph();

                ScaledMeterGraph & operator = (const ScaledMeterGraph &) = delete;
                ScaledMeterGraph & operator = (ScaledMeterGraph &&) = delete;

                /**
                 * Construct object
                 */
                void        construct();

                /** Initialize meter graph
                 *
                 * @param frames number of frames used for graph and needed to be stored in internal buffer
                 * @param subsampling the sub-sampling period (in samples): the number of samples being aggregated into one single sub-sample
                 * @param max_period the maximum period per
                 * @return true on success
                 */
                bool        init(size_t frames, size_t subsampling, size_t max_period);

                /** Destroy meter graph
                 *
                 */
                void        destroy();

            public:
                /**
                 * Get sub-sampling period
                 * @return sub-sampling period
                 */
                inline size_t subsampling() const           { return sHistory.nPeriod;      }

                /**
                 * Get number of frames
                 * @return number of frames
                 */
                inline size_t frames() const                { return sFrames.nFrames;       }

                /**
                 * Set metering method
                 *
                 * @param m metering method
                 */
                void set_method(meter_method_t m);

                /**
                 * Get metering method
                 * @return metering method
                 */
                inline meter_method_t method() const        { return enMethod;              }

                /**
                 * Set strobe period
                 *
                 * @param period strobe period
                 */
                void        set_period(size_t period);

                /**
                 * Get metering period
                 * @return metering period
                 */
                inline float period() const                 { return nPeriod;               }

                /**
                 * Read last actual frames data from the frame buffer. The maximum allowed frames
                 * to read is passed in the init() method
                 *
                 * @param dst pointer to store data
                 * @param offset negative offset from the current head
                 * @param count number of frames to read
                 */
                void        read(float *dst, size_t count);

                /** Process single sample
                 *
                 * @param sample sample to process
                 */
                void        process(float sample);

                /** Process multiple samples
                 *
                 * @param s array of samples
                 * @param n number of samples to process
                 */
                void        process(const float *s, size_t count);

                /** Process multiple samples multiplied by specified value
                 *
                 * @param s array of samples
                 * @param n number of samples to process
                 */
                void        process(const float *s, float gain, size_t count);

                /** Get current level
                 *
                 * @return current level
                 */
                float       level() const;

                /** Fill graph with specific level
                 *
                 * @param level level
                 */
                void        fill(float level);

                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void        dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_SCALEDMETERGRAPH_H_ */
