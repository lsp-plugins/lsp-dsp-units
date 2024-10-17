/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 16 окт. 2024 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_METERS_TRUEPEAKMETER_H_
#define LSP_PLUG_IN_DSP_UNITS_METERS_TRUEPEAKMETER_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {

        /**
         * True Peak Mmeter. Computes True Peak value according to the BS-1770 recommendations.
         */
        class LSP_DSP_UNITS_PUBLIC TruePeakMeter
        {
            private:
                typedef void (*reduce_t)(float *dst, const float *src, size_t count);

            private:
                uint32_t            nSampleRate;        // Sample rate
                uint32_t            nHead;              // Head of the buffer
                uint8_t             nTimes;             // Oversampling times
                bool                bUpdate;            // Requires settings update

                dsp::resampling_function_t pFunc;       // Resampling function
                reduce_t            pReduce;            // Reducing function
                float              *vBuffer;            // Buffer for oversampled data
                uint8_t            *pData;              // Pointer to the allocated data

            protected:
                static uint8_t      calc_oversampling_multiplier(size_t sample_rate);
                static void         reduce_2x(float *dst, const float *src, size_t count);
                static void         reduce_3x(float *dst, const float *src, size_t count);
                static void         reduce_4x(float *dst, const float *src, size_t count);
                static void         reduce_6x(float *dst, const float *src, size_t count);
                static void         reduce_8x(float *dst, const float *src, size_t count);

            public:
                TruePeakMeter();
                TruePeakMeter(const TruePeakMeter &) = delete;
                TruePeakMeter(TruePeakMeter &&) = delete;
                ~TruePeakMeter();

                TruePeakMeter & operator = (const TruePeakMeter &) = delete;
                TruePeakMeter && operator = (TruePeakMeter &&) = delete;

                /**
                 *  Construct object
                 */
                void            construct();

                /**
                 * Destroy object
                 */
                void            destroy();

                /**
                 * Initialize
                 */
                void            init();

            public:
                void            update_settings();

                /**
                 * Set sample rate of the true peak meter
                 */
                void            set_sample_rate(uint32_t sr);

                /**
                 * Get sample rate
                 * @return sample rate
                 */
                size_t          sample_rate() const;

                /**
                 * Clear internal state
                 */
                void            clear();

                /**
                 * Process the input data and compute the true peak value for each sample
                 * @param dst destination buffer to store the true peak values
                 * @param src source buffer to process
                 * @param count number of samples to process
                 */
                void            process(float *dst, const float *src, size_t count);

                /**
                 * Process the buffer and compute the true peak value for each sample
                 * @param buf buffer to process and store true peak values
                 * @param count number of samples to process
                 */
                void            process(float *buf, size_t count);

                /**
                 * Process the input data and compute the maximum true peak value
                 * @param channel the channel number to process
                 * @param src source buffer to process
                 * @param count number of samples to process
                 * @return maximum value of the true peak
                 */
                float           process_max(const float *src, size_t count);

                /**
                 * Return latency for the true peak meter
                 * @return latency of the true peak meter
                 */
                size_t          latency() const;

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_DSP_UNITS_METERS_TRUEPEAKMETER_H_ */
