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

#ifndef LSP_PLUG_IN_DSP_UNITS_DYNAMICS_UTIL_DELAY_HPP_
#define LSP_PLUG_IN_DSP_UNITS_DYNAMICS_UTIL_DELAY_HPP_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Delay processor
         */
        class LSP_DSP_UNITS_PUBLIC Delay
        {
            protected:
                float      *pBuffer;
                uint32_t    nHead;
                uint32_t    nTail;
                uint32_t    nDelay;
                uint32_t    nSize;

            public:
                explicit Delay();
                Delay(const Delay &) = delete;
                Delay(Delay &&) = delete;
                ~Delay();

                Delay & operator = (const Delay &) = delete;
                Delay & operator = (Delay &&) = delete;

                /** Construct the processor, can be called
                 * when there is no possibility to explicitly call
                 * the constructor
                 *
                 */
                void construct();

                /** Destroy delay
                 *
                 */
                void destroy();

            public:
                /** Initialize delay
                 *
                 * @param max_size maximum delay in samples
                 * @return status of operation
                 */
                bool init(size_t max_size);

                /** Push data to delay buffer and update internal pointers,
                 * do not return anything.
                 *
                 * @param src source buffer
                 * @param count number of samples to process
                 */
                void append(const float *src, size_t count);

                /** Process input data and copy result to the destination buffer
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param count number of samples to process
                 */
                void process(float *dst, const float *src, size_t count);

                /** Process input data, apply constant gain and copy result to the destination buffer.
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param gain gain to adjust
                 * @param count number of samples to process
                 */
                void process(float *dst, const float *src, float gain, size_t count);

                /** Process input data, apply variable gain and copy result to the destination buffer.
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param gain gain buffer to apply to the output
                 * @param count number of samples to process
                 */
                void process(float *dst, const float *src, const float *gain, size_t count);

                /** Process input data and add result to the destination buffer
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param count number of samples to process
                 */
                void process_add(float *dst, const float *src, size_t count);

                /** Process input data, apply constant gain and add result to the destination buffer.
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param gain gain to adjust
                 * @param count number of samples to process
                 */
                void process_add(float *dst, const float *src, float gain, size_t count);

                /** Process input data, apply variable gain and copy result to the destination buffer.
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param gain gain buffer to apply to the output
                 * @param count number of samples to process
                 */
                void process_add(float *dst, const float *src, const float *gain, size_t count);

                /** Process data
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param delay the final delay that will be set at the end of processing
                 * @param count number of samples to process
                 */
                void process_ramping(float *dst, const float *src, size_t delay, size_t count);

                /** Process data and apply constant gain to the output.
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param gain the amount of gain to apply to the output
                 * @param delay the final delay that will be set at the end of processing
                 * @param count number of samples to process
                 */
                void process_ramping(float *dst, const float *src, float gain, size_t delay, size_t count);

                /** Process data and apply variable gain to the output.
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param gain gain buffer to apply to the output
                 * @param delay the final delay that will be set at the end of processing
                 * @param count number of samples to process
                 */
                void process_ramping(float *dst, const float *src, const float *gain, size_t delay, size_t count);

                /** Process one sample
                 *
                 * @param src sample to process
                 * @return output sample
                 */
                float process(float src);

                /** Process one sample
                 *
                 * @param src sample to process
                 * @param gain gain to adjust
                 * @return output sample
                 */
                float process(float src, float gain);

                /** Clear internal delay buffer
                 *
                 */
                void clear();

                /** Set delay in samples
                 *
                 * @param delay delay in samples
                 */
                void set_delay(size_t delay);

                /** Get delay in samples
                 *
                 * @return delay in samples
                 */
                inline size_t get_delay() const { return nDelay; };

                /** Get delay in samples
                 *
                 * @return delay in samples
                 */
                inline size_t delay() const { return nDelay; };

                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_DYNAMICS_UTIL_DELAY_HPP_ */
