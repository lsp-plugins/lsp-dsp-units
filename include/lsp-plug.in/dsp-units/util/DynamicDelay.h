/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_DYNAMICDELAY_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_DYNAMICDELAY_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/common/status.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Dynamic Delay: Delay with varying in time delay, gain and feedback gain
         */
        class LSP_DSP_UNITS_PUBLIC DynamicDelay
        {
            protected:
                float      *vDelay;
                uint32_t    nHead;
                uint32_t    nCapacity;
                int32_t     nMaxDelay;
                uint8_t    *pData;

            public:
                explicit DynamicDelay();
                DynamicDelay(const DynamicDelay &) = delete;
                DynamicDelay(DynamicDelay &&) = delete;
                ~DynamicDelay();

                DynamicDelay & operator = (const DynamicDelay &) = delete;
                DynamicDelay & operator = (DynamicDelay &&) = delete;

                void        construct();
                void        destroy();

            public:
                /**
                 * Obtain the maximum possible value for the delay
                 * @return the maximum possible value for the delay in samples
                 */
                inline size_t max_delay() const     { return nMaxDelay;     }

                /**
                 * Obtain the overall delay capacity
                 * @return the overall delay capacity in samples
                 */
                inline size_t capacity() const      { return nCapacity;     }

                /**
                 * Initialize delay
                 * @param max_size maximum delay size
                 * @return status of operation
                 */
                status_t    init(size_t max_size);

                /**
                 * Process the signal using dynamic settings of delay and feedback
                 * @param out output buffer
                 * @param in input buffer
                 * @param delay the delay values
                 * @param fgain feedback gain values
                 * @param fdelay feedback delay values
                 * @param samples number of samples to process
                 */
                void        process(float *out, const float *in,
                                    const float *delay, const float *fgain, const float *fdelay,
                                    size_t samples);

                /**
                 * Clear delay state
                 */
                void        clear();

                /**
                 * Copy the contents of the dynamic delay
                 * @param s delay to copy contents from
                 */
                void        copy(DynamicDelay *s);

                /**
                 * Swap contents with some another delay
                 * @param d delay to swap contents
                 */
                void        swap(DynamicDelay *d);

                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void        dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_DYNAMICDELAY_H_ */
