/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 2 сент. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_FILTERS_FILTERBANK_H_
#define LSP_PLUG_IN_DSP_UNITS_FILTERS_FILTERBANK_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/filters/common.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        class FilterBank
        {
            private:
                FilterBank & operator = (const FilterBank &);
                FilterBank(const FilterBank &);

            protected:
                dsp::biquad_t      *vFilters;   // Optimized list of filters
                dsp::biquad_x1_t   *vChains;    // List of biquad banks
                size_t              nItems;     // Current number of biquad_x1 filters
                size_t              nMaxItems;  // Maximum number of biquad_x1 filters
                size_t              nLastItems; // Previous number of biquad_x1 filters
                float              *vBackup;    // Delay backup to take online impulse response
                uint8_t            *vData;      // Unaligned data

            protected:
                void                clear_delays();
                void                digital_biquad_frequency_response(dsp::biquad_x1_t *bq, double omega, float *re, float* im);  // Frequency response of digital biquad at normalised frequency omega

            public:
                explicit FilterBank();
                ~FilterBank();

                /**
                 * Construct the filter bank being a chunk of memory
                 */
                void                construct();

                /** Initialize filter bank
                 *
                 * @param filters number of biquad filters
                 * @return true on success
                 */
                bool                init(size_t filters);

                /** Destroy filter bank
                 *
                 */
                void                destroy();

            public:
                /** Start filter bank, clears number of cascades
                 *
                 */
                inline void         begin()
                {
                    nLastItems      = nItems;
                    nItems          = 0;
                }

                /** Add cascade to biquad filter
                 *
                 * @return added cascade
                 */
                dsp::biquad_x1_t   *add_chain();

                /** Optimize structure of filter bank
                 * @param clear force to clear delays
                 */
                void                end(bool clear = false);

                /** Process samples
                 *
                 * @param out output buffer
                 * @param in input buffer
                 * @param samples number of samples to process
                 */
                void                process(float *out, const float *in, size_t samples);

                /** Get impulse response of the bank
                 *
                 * @param out output buffer to store impulse response
                 * @param samples length of buffer in samples
                 */
                void                impulse_response(float *out, size_t samples);

                /** Get frequency chart of the specific filter
                 *
                 * @param id ID of the filter
                 * @param re real part of the frequency chart
                 * @param im imaginary part of the frequency chart
                 * @param f normalised frequencies to calculate value
                 * @param count number of dots for the chart
                 * @return status of operation
                 */
                bool                freq_chart(size_t id, float *re, float *im, const float *f, size_t count);

                /** Get frequency chart of the specific filter
                 *
                 * @param id ID of the filter
                 * @param c complex numbers that contain the filter transfer function
                 * @param f normalised frequencies to calculate filter transfer function
                 * @param count number of points
                 * @return status of operation
                 */
                bool                freq_chart(size_t id, float *c, const float *f, size_t count);

                /**
                 * Get frequency chart of the whole equalizer
                 * @param re real part of the frequency chart
                 * @param im imaginary part of the frequency chart
                 * @param f normalised frequencies to calculate value
                 * @param count number of dots for the chart
                 */
                void                freq_chart(float *re, float *im, const float *f, size_t count);

                /**
                 * Get frequency chart of the whole equalizer
                 * @param c complex numbers that contain the filter transfer function
                 * @param f normalised frequencies to calculate filter transfer function
                 * @param count number of points
                 */
                void                freq_chart(float *c, const float *f, size_t count);

                /** Get number of biquad filters
                 *
                 * @return number of biquad filters
                 */
                inline size_t       size() const { return nItems; }

                /** Reset internal state of filters (clear filter memory)
                 *
                 */
                void                reset();

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void                dump(IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_FILTERS_FILTERBANK_H_ */
