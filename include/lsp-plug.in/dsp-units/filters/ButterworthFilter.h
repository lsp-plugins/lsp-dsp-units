/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
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

        enum flt_type_t
        {
            FLT_TYPE_LOWPASS,
            FLT_TYPE_HIGHPASS,
            FLT_TYPE_NONE,
            FLT_TYPE_MAX
        };

        /** Even order high-pass and low-pass Butterworth filter, implemented as second order section.
         * Pre-warped bilinear transform of analog Butterworth prototype.
         */
        class ButterworthFilter
        {

            private:
                ButterworthFilter & operator = (const ButterworthFilter &);
                ButterworthFilter(const ButterworthFilter &);

            private:
                size_t              nOrder;
                float               fCutoffFreq;
                size_t              nSampleRate;
                flt_type_t          enFilterType;
                dspu::FilterBank    sFilter;
                uint8_t            *pData;
                float              *vBuffer;
                bool                bBypass;
                bool                bSync;

            public:
                explicit ButterworthFilter();
                ~ButterworthFilter();

                void construct();
                void destroy();

            public:
                /** Check that Butter needs settings update.
                 *
                 * @return true if Butter needs settings update.
                 */
                inline bool needs_update() const
                {
                    return bSync;
                }

                /** This method should be called if needs_update() returns true.
                 * before calling processing methods.
                 *
                 */
                void update_settings();

                /** Set the order of the filter.
                 *
                 * @param order order of the filter.
                 */
                inline void set_order(size_t order)
                {
                    if (order == nOrder)
                        return;

                    nOrder  = order;
                    bSync   = true;
                }

                /** Set the cutoff frequency of the filter.
                 *
                 * @param frequency cutoff frequency.
                 */
                inline void set_cutoff_frequency(float frequency)
                {
                    if (frequency == fCutoffFreq)
                        return;

                    fCutoffFreq = frequency;
                    bSync       = true;
                }

                /** Set filter type.
                 *
                 * @param type filter type.
                 */
                inline void set_filter_type(flt_type_t type)
                {
                    if ((type < FLT_TYPE_LOWPASS) && (type >= FLT_TYPE_MAX))
                        return;

                    enFilterType    = type;
                    bSync           = true;
                }

                /** Set sample rate for the filter.
                 *
                 * @param sr sample rate.
                 */
                inline void set_sample_rate(size_t sr)
                {
                    if (nSampleRate == sr)
                        return;

                    nSampleRate = sr;
                    bSync       = true;
                }

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
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };

    }
}

#endif /* LSP_PLUG_IN_DSP_UNITS_FILTERS_BUTTER_H_ */
