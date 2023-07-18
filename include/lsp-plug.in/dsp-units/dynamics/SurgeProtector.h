/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 15 июл. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_DYNAMICS_SURGEPROTECTOR_H_
#define LSP_PLUG_IN_DSP_UNITS_DYNAMICS_SURGEPROTECTOR_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC SurgeProtector
        {
            private:
                SurgeProtector(const SurgeProtector &) = delete;
                SurgeProtector(const SurgeProtector &&) = delete;
                SurgeProtector & operator = (const SurgeProtector &) = delete;
                SurgeProtector & operator = (const SurgeProtector &&) = delete;

            protected:
                float       fGain;              // Current gain
                size_t      nTransitionTime;    // Current transition time (in samples)
                size_t      nTransitionMax;     // Maximum transition time (in samples)
                size_t      nShutdownTime;      // Shutdown time (in samples)
                size_t      nShutdownMax;       // Maximum shutdown time (in samples)
                float       fOnThreshold;       // Turn on threshold
                float       fOffThreshold;      // Turn off threshold
                bool        bOn;                // The protector is enabled (passing signal)

            public:
                explicit SurgeProtector();
                ~SurgeProtector();

                /**
                 * Construct the object
                 */
                void construct();

                /**
                 * Destroy the object
                 */
                void destroy();

            public:
                /**
                 * Reset the state to shut down
                 */
                void        reset();

                /**
                 * Set the transition time to turned on or turned off
                 * @param time transition time in samples
                 */
                void        set_transition_time(size_t time);

                /**
                 * Set the shutdown time, a time after which the protector turns off if there was no gain sample
                 * above the off threshold
                 * @param time shutdown time in samples
                 */
                void        set_shutdown_time(size_t time);

                /**
                 * Set the threshold after which the surge protector turns on
                 * @param threshold threshold
                 */
                void        set_on_threshold(float threshold);

                /**
                 * Set the threshold after which the surge protector turns off
                 * @param threshold threshold
                 */
                void        set_off_threshold(float threshold);

                /**
                 * Set both on and off thresholds
                 * @param on on threshold
                 * @param off off threshold
                 */
                void        set_threshold(float on, float off);

                /**
                 * Process single sample
                 * @param in input sample to process
                 * @return the surge protector gain
                 */
                float       process(float in);

                /**
                 * Update state, do not store the data to some output buffer
                 * @param in input buffer
                 * @param samples number of samples to process
                 */
                void        process(const float *in, size_t samples);

                /**
                 * Update state, store the gain to output buffer
                 * @param in input buffer
                 * @param out output buffer
                 * @param samples number of samples to process
                 */
                void        process(float *out, const float *in, size_t samples);

                /**
                 * Update state, multiply the output buffer by gain
                 * @param in input buffer
                 * @param out output buffer
                 * @param samples number of samples to process
                 */
                void        process_mul(float *out, const float *in, size_t samples);

                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void        dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_DYNAMICS_SURGEPROTECTOR_H_ */
