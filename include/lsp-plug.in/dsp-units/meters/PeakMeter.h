/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 4 окт. 2025 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_CTL_PEAK_H_
#define LSP_PLUG_IN_DSP_UNITS_CTL_PEAK_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Simple peak monitor. Detects peak and holds it for desired period
         */
        class LSP_DSP_UNITS_PUBLIC PeakMeter
        {
            protected:
                float       fPeak;      // Current value
                float       fTau;       // Time coefficient
                float       fHold;      // Hold time in milliseconds
                float       fRelease;   // Release time in milliseconds
                uint32_t    nHold;      // Hold time in samples
                uint32_t    nCounter;   // Hold time counter
                uint32_t    nSampleRate;// Sample rate
                bool        bUpdate;    // Update flag

            protected:
                void        update_settings();

            public:
                explicit PeakMeter();
                PeakMeter(const PeakMeter &) = delete;
                PeakMeter(PeakMeter &&) = delete;
                ~PeakMeter();
                PeakMeter & operator = (const PeakMeter &) = delete;
                PeakMeter & operator = (PeakMeter &&) = delete;

                /**
                 * Construct object
                 */
                void            construct();

                /**
                 * Destroy object
                 */
                void            destroy();

            public:
                /**
                 * Update current sample rate
                 *
                 * @param sample_rate current sample rate
                 */
                void            set_sample_rate(size_t sample_rate);

                /**
                 * Get current sample rate
                 * @return current sample rate
                 */
                size_t          sample_rate() const;

                /**
                 * Process the signal
                 * @param data input signal
                 * @param count number of samples to process
                 * @return current peak value
                 */
                float           process(const float *data, size_t count);

                /**
                 * Process the signal
                 * @param dst destination buffer to store peak value
                 * @param src input signal
                 * @param count number of samples to process
                 * @return current peak value
                 */
                float           process(float *dst, const float *src, size_t count);

                /**
                 * Process the single sample
                 * @param data input sample
                 * @param count number of samples to consider being processed
                 * @return current peak value
                 */
                float           process(float value, size_t count);

                /**
                 * Set hold time
                 * @param value hold time in milliseconds
                 */
                void            set_hold_time(float value);

                /**
                 * Get hold time
                 * @return hold time in milliseconds
                 */
                float           hold_time() const;

                /**
                 * Set release time
                 * @param value release time in milliseconds
                 */
                void            set_release_time(float value);

                /**
                 * Get release time
                 * @return release time release time in milliseconds
                 */
                float           release_time() const;

                /**
                 * Set hold and release time
                 * @param hold hold time in milliseconds
                 * @param release release time in milliseconds
                 */
                void            set_time(float hold, float release);

                /**
                 * Get the current peak value
                 * @return current peak value
                 */
                float           value() const;

                /**
                 * Cleanup current peak, set to 0.
                 */
                void            clear();

                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void            dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_CTL_PEAK_H_ */
