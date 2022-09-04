/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 1 февр. 2018 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_FILTERS_DYNAMICFILTERS_H_
#define LSP_PLUG_IN_DSP_UNITS_FILTERS_DYNAMICFILTERS_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/filters/common.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/common/status.h>

namespace lsp
{
    namespace dspu
    {
        /** This class implements set of sequential dynamic filters grouped
         * into one object for resource economy purpose.
         *
         */
        class LSP_DSP_UNITS_PUBLIC DynamicFilters
        {
            private:
                DynamicFilters & operator = (const DynamicFilters &);
                DynamicFilters(const DynamicFilters &);

            protected:
                typedef struct filter_t
                {
                    filter_params_t     sParams;                // Filter parameters
                    bool                bActive;                // Filter activity
                } filter_t;

                union biquad_bank_t
                {
                    void               *ptr;
                    dsp::biquad_x1_t   *x1;
                    dsp::biquad_x2_t   *x2;
                    dsp::biquad_x4_t   *x4;
                    dsp::biquad_x8_t   *x8;
                };

                static const dsp::f_cascade_t    sNormal;

            protected:
                filter_t           *vFilters;           // Array of filters
                dsp::f_cascade_t   *vCascades;          // Analog filter cascade bank
                float              *vMemory;            // Filter memory
                biquad_bank_t       vBiquads;           // Biquad bank
                size_t              nFilters;           // Number of filters
                size_t              nSampleRate;        // Sample rate
                void               *pData;              // Aligned pointer data
                bool                bClearMem;          // Clear memory

            protected:
                size_t              quantify(size_t c, size_t nc);
                size_t              build_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, const float *sfg, size_t samples);
                size_t              build_lrx_ladder_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, const float *sfg, size_t samples, size_t ftype);
                size_t              build_lrx_shelf_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, const float *sfg, size_t samples, size_t ftype);

                size_t              precalc_lrx_ladder_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, const float *sfg, size_t samples);
                void                calc_lrx_ladder_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, size_t samples, size_t ftype, size_t nc);

                void                vcomplex_transfer_calc(float *re, float *im, const dsp::f_cascade_t *fc, const float *freq, size_t cj, size_t nc, size_t nf);
                void                vcomplex_transfer_calc(float *dst, const dsp::f_cascade_t *fc, const float *freq, size_t cj, size_t nc, size_t nf);

            public:
                explicit DynamicFilters();
                ~DynamicFilters();

                /**
                 * Construct the object as a part of memory chunkk
                 */
                void                construct();

                /** Initialize the dynamic filters set
                 *
                 * @param filters
                 * @return
                 */
                status_t            init(size_t filters);

                /** Destroy the dynamic filters set
                 *
                 */
                void                destroy();

            public:
                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                void                set_sample_rate(size_t sr);

                /** Check that filter is active
                 *
                 * @param id ID of filter
                 * @return true if filter is active
                 */
                inline bool         filter_active(size_t id) const { return (id < nFilters) ? vFilters[id].bActive : false; };

                /** Check that filter is inactive
                 *
                 * @param id ID of filter
                 * @return true if filter is inactive
                 */
                inline bool         filter_inactive(size_t id) const { return (id < nFilters) ? !vFilters[id].bActive : true; };

                /** Set activity of the specific filter
                 *
                 * @param id filter identifier
                 * @param active activity of the specific filter
                 * @return true on success
                 */
                inline bool         set_filter_active(size_t id, bool active)
                {
                    if (id >= nFilters)
                        return false;
                    vFilters[id].bActive        = true;
                    return true;
                }

                /** Update filter parameters
                 * @param id ID of the filter
                 * @param params  filter parameters
                 * @return true on success
                 */
                bool                set_params(size_t id, const filter_params_t *params);

                /** Get filter parameters
                 * @param id ID of the filter
                 * @param params  filter parameters
                 * @return true on success
                 */
                bool                get_params(size_t id, filter_params_t *params);

                /** Process signal with filter varying by it's gain parameter
                 *
                 * @param id filer identifier
                 * @param out output signal
                 * @param in input signal
                 * @param gain the gain level of the filter
                 * @param samples number of samples to process
                 */
                void                process(size_t id, float *out, const float *in, const float *gain, size_t samples);

                /** Get frequency chart of the specific filter
                 *
                 * @param id ID of the filter
                 * @param re real part of the frequency chart
                 * @param im imaginary part of the frequency chart
                 * @param f frequencies to calculate value
                 * @param count number of dots for the chart
                 */
                bool                freq_chart(size_t id, float *re, float *im, const float *f, float gain, size_t count);

                /** Get frequency chart of the specific filter
                 *
                 * @param id ID of the filter
                 * @param dst array of complex numbers to store data
                 * @param f frequencies to calculate value
                 * @param count number of dots for the chart
                 */
                bool                freq_chart(size_t id, float *dst, const float *f, float gain, size_t count);

            public:
                void                dump(dspu::IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_FILTERS_DYNAMICFILTERS_H_ */
