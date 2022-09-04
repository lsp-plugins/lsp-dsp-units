/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 27 нояб. 2018 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_CTL_COUNTER_H_
#define LSP_PLUG_IN_DSP_UNITS_CTL_COUNTER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC Counter
        {
            private:
                Counter & operator = (const Counter &);
                Counter(const Counter &);

            protected:
                enum flags_t
                {
                    F_INITIAL   = 1 << 0,
                    F_FIRED     = 1 << 1
                };

            protected:
                size_t      nCurrent;
                size_t      nInitial;
                size_t      nSampleRate;
                float       fFrequency;
                size_t      nFlags;

            public:
                explicit Counter();
                ~Counter();

                /**
                 * Construct object
                 */
                void        construct();

                /**
                 * Destroy object
                 */
                void        destroy();

            public:
                /**
                 * Get sample rate
                 * @return sample rate
                 */
                inline size_t get_sample_rate() const { return nSampleRate; }

                /**
                 * Set sample rate
                 * @param sr sample rate
                 * @param reset flag to reset counter to initial value
                 */
                void set_sample_rate(size_t sr, bool reset = true);

                /**
                 * Get frequency
                 * @return frequency
                 */
                inline float get_frequency() const { return fFrequency; }

                /**
                 * Set frequency
                 * @param freq frequency
                 * @param reset flag to reset counter to initial value
                 */
                void set_frequency(float freq, bool reset = true);

                /**
                 * Get initial countdown value
                 * @return initial countdown value
                 */
                inline size_t get_initial_value() const { return nInitial; }

                /**
                 * Set initial countdown value
                 * @param value initial countdown value
                 * @param reset flag to reset counter to initial value
                 */
                void set_initial_value(size_t value, bool reset = true);

            public:
                /**
                 * Check fired flag
                 * @return fired flag
                 */
                inline bool fired() const { return nFlags & F_FIRED; }

                /**
                 * Get number of samples pending for processing
                 * @return number of samples pending for processing
                 */
                size_t pending() const { return nCurrent; }

                /**
                 * Reset fired flag
                 * @return fired flag before reset
                 */
                bool commit();

                /**
                 * Reset counter to initial value
                 * @return fired flag
                 */
                bool reset();

                /**
                 * Submit number of samples been processed
                 * @param samples number of samples to submit
                 * @return fired flag
                 */
                bool submit(size_t samples);

                /**
                 * Prefer frequency over initial value when
                 * changing sample rate
                 */
                inline void preserve_frequency() {
                    nFlags &= ~F_INITIAL;
                }

                /**
                 * Prefer initial value over frequency when
                 * changing sample rate
                 */
                inline void preserve_initial_value() {
                    nFlags |= F_INITIAL;
                }

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_CTL_COUNTER_H_ */
