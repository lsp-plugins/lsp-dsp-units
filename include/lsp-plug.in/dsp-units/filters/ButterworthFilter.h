/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 28 Sept 2021
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


#ifndef LSP_PLUG_IN_DSP_UNITS_FILTERS_BUTTER_H_
#define LSP_PLUG_IN_DSP_UNITS_FILTERS_BUTTER_H_

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/filters/FilterBank.h>

namespace lsp
{
    namespace dspu
    {

        enum bw_filt_type_t
        {
            BW_FLT_TYPE_LOWPASS,
            BW_FLT_TYPE_HIGHPASS,
            BW_FLT_TYPE_NONE,
            BW_FLT_TYPE_MAX
        };

        /** Even order high-pass and low-pass Butterworth filter, implemented as second order section.
         * Pre-warped bilinear transform of analog Butterworth prototype.
         */
        class LSP_DSP_UNITS_PUBLIC ButterworthFilter
        {
            private:
                size_t              nOrder;
                float               fCutoffFreq;
                size_t              nSampleRate;
                bw_filt_type_t      enFilterType;
                bool                bBypass;
                bool                bSync;
                dspu::FilterBank    sFilter;

            public:
                explicit ButterworthFilter();
                ButterworthFilter(const ButterworthFilter &) = delete;
                ButterworthFilter(ButterworthFilter &&) = delete;
                ~ButterworthFilter();
            
                ButterworthFilter & operator = (const ButterworthFilter &) = delete;
                ButterworthFilter & operator = (ButterworthFilter &&) = delete;

                void construct();
                void destroy();
                void init();

            protected:
                void update_settings();

            public:
                /** Set the order of the filter.
                 *
                 * @param order order of the filter.
                 */
                void set_order(size_t order);

                /** Set the cutoff frequency of the filter.
                 *
                 * @param frequency cutoff frequency.
                 */
                void set_cutoff_frequency(float frequency);

                /** Set filter type.
                 *
                 * @param type filter type.
                 */
                void set_filter_type(bw_filt_type_t type);

                /** Set sample rate for the filter.
                 *
                 * @param sr sample rate.
                 */
                void set_sample_rate(size_t sr);

                /** Output sequence to the destination buffer in additive mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to synthesise
                 */
                void process_add(float *dst, const float *src, size_t count);

                /** Output sequence to the destination buffer in multiplicative mode
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process_mul(float *dst, const float *src, size_t count);

                /** Output sequence to a destination buffer overwriting its content
                 *
                 * @param dst output wave destination
                 * @param src input source, allowed to be NULLL
                 * @param count number of samples to process
                 */
                void process_overwrite(float *dst, const float *src, size_t count);

                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_FILTERS_BUTTER_H_ */
