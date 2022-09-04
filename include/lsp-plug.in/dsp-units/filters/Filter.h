/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 28 июня 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_FILTERS_FILTER_H_
#define LSP_PLUG_IN_DSP_UNITS_FILTERS_FILTER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/filters/common.h>
#include <lsp-plug.in/dsp-units/filters/FilterBank.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Single filter implementation
         */
        class LSP_DSP_UNITS_PUBLIC Filter
        {
            private:
                Filter & operator = (const Filter &);
                Filter(const Filter &);

            protected:
                enum filter_mode_t
                {
                    FM_BYPASS,          // Bypass filter
                    FM_BILINEAR,        // Bilinear Z-transform
                    FM_MATCHED,         // Matched Z-transform
                    FM_APO              // APO single biquad filter implementation, based on textbook bilinar transforms
                };

                enum filter_flags_t
                {
                    FF_OWN_BANK     = 1 << 0,       // Filter has it's own filter bank
                    FF_REBUILD      = 1 << 1,       // Need to rebuild filter
                    FF_CLEAR        = 1 << 2        // Need to clear filter memory
                };

            protected:
                FilterBank         *pBank;          // External bank of filters
                filter_params_t     sParams;        // Filter parameters
                size_t              nSampleRate;    // Sample rate
                filter_mode_t       nMode;          // Filter mode
                size_t              nItems;         // Number of cascades
                dsp::f_cascade_t   *vItems;         // Filter cascades
                uint8_t            *vData;          // Allocated data
                size_t              nFlags;         // Filter flags
                size_t              nLatency;       // Filter latency

            protected:

                void                apo_complex_transfer_calc(float *re, float *im, float f);
                dsp::f_cascade_t   *add_cascade();

                void                calc_rlc_filter(size_t type, const filter_params_t *fp);
                void                calc_bwc_filter(size_t type, const filter_params_t *fp);
                void                calc_lrx_filter(size_t type, const filter_params_t *fp);
                void                calc_apo_filter(size_t type, const filter_params_t *fp);
                float               bilinear_relative(float f1, float f2);
                void                bilinear_transform();
                void                matched_transform();

            public:
                explicit Filter();
                ~Filter();

                /**
                 * Construct the object being the chunk of memory
                 */
                void                construct();

                /**  Initialize filter
                 *
                 * @param fb filter bank to use, NULL for using internal filter bank
                 * @return true on success
                 */
                bool                init(FilterBank *fb);

                /** Destroy filter data
                 *
                 */
                void                destroy();

            public:
                /** Update filter parameters
                 *
                 * @param params filter parameters
                 */
                void                update(size_t sr, const filter_params_t *params);

                /**
                 * Update sample rate
                 * @param sr sample rate
                 */
                void                set_sample_rate(size_t sr);

                /** Get current filter parameters
                 *
                 * @param params pointer to filter parameters to store
                 */
                void                get_params(filter_params_t *params);

                /** Process signal
                 *
                 * @param out output signal
                 * @param in input signal
                 * @param samples number of samples to process
                 */
                void                process(float *out, const float *in, size_t samples);

                /** Get the impulse response of the filter
                 *
                 * @param out output buffer to store the impulse response
                 * @param length length of the impulse response
                 * @return true if impulse response can be taken, false if need to take it from bank
                 */
                bool                impulse_response(float *out, size_t length);

                /** Get frequency chart
                 *
                 * @param re real part of the frequency chart
                 * @param im imaginary part of the frequency chart
                 * @param f frequencies to calculate value
                 * @param count number of dots for the chart
                 */
                void                freq_chart(float *re, float *im, const float *f, size_t count);

                /** Get frequency chart
                 *
                 * @param c complex transfer function results
                 * @param f frequencies to calculate value
                 * @param count number of dots for the chart
                 */
                void                freq_chart(float *c, const float *f, size_t count);

                /** Mark filter to be cleared
                 *
                 */
                inline void         clear()             { nFlags     |= FF_CLEAR;       }

                /** Rebuild filter
                 * Forces the filter to rebuild into bank of filters
                 */
                void                rebuild();

                /** Get filter latency
                 *
                 * @return filter latency in samples
                 */
                inline size_t       latency() const     { return nLatency;  };

                /** Check if the filter is bypassed
                 *
                 * @return true if the filter is bypassed
                 */
                inline bool         inactive() const    { return nMode == FM_BYPASS; }

                /** Check if the filter is active
                 *
                 * @return true if the filter is active
                 */
                inline bool         active() const      { return nMode != FM_BYPASS; }

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void                dump(IStateDumper *v) const;
        };
    }
}


#endif /* LSP_PLUG_IN_DSP_UNITS_FILTERS_FILTER_H_ */
