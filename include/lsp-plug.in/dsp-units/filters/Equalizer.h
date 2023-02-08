/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 26 июля 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_FILTERS_EQUALIZER_H_
#define LSP_PLUG_IN_DSP_UNITS_FILTERS_EQUALIZER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/filters/common.h>
#include <lsp-plug.in/dsp-units/filters/FilterBank.h>
#include <lsp-plug.in/dsp-units/filters/Filter.h>

namespace lsp
{
    namespace dspu
    {
        enum equalizer_mode_t
        {
            EQM_BYPASS, // Bypass signal
            EQM_IIR,    // All filters are recursive filters with infinite impulse response filters
            EQM_FIR,    // All filters are non-recursive filters with finite impulse response filters
            EQM_FFT,    // Approximation of the frequency chart in the frequency range
            EQM_SPM     // Equalizer acts as a Spectral Processing Module
        };

        /**
         * Set of multiple controllable sequential filters combined into one managed object
         */
        class LSP_DSP_UNITS_PUBLIC Equalizer
        {
            private:
                Equalizer & operator = (const Equalizer &);
                Equalizer(const Equalizer &);

            protected:
                enum eq_flags_t
                {
                    EF_REBUILD = 1 << 0,
                    EF_CLEAR   = 1 << 1
                };

            protected:
                FilterBank          sBank;              // Filter bank
                Filter             *vFilters;           // List of filters
                size_t              nFilters;           // Number of filters
                size_t              nSampleRate;        // Sample rate
                size_t              nFirSize;           // FIR filter size
                size_t              nFirRank;           // FFT rank
                size_t              nLatency;           // Equalizer latency
                size_t              nBufSize;           // Buffer size
                equalizer_mode_t    nMode;              // Equalizer mode

                float              *vInBuffer;          // Input buffer data
                float              *vOutBuffer;         // Output buffer data
                float              *vConv;              // Convolution data
                float              *vFft;               // FFT transform data buffer (real + imaginary)
                float              *vTemp;              // Temporary buffer for miscellaneous calculations

                size_t              nFlags;             // Flag that identifies that equalizer has to be rebuilt
                uint8_t            *pData;              // Allocation data

            protected:
                void                reconfigure();

            public:
                explicit Equalizer();
                ~Equalizer();

                /**
                 * Construct the object being part of memory chunk
                 */
                void                construct();

                /** Initialize equalizer
                 *
                 * @param filters number of filters
                 * @param fir FIR filter rank (impulse response size)
                 * @return true on success
                 */
                bool                init(size_t filters, size_t fir_rank);

                /** Destroy equalizer
                 *
                 */
                void                destroy();

            public:
                /**
                 * Check if the configuration of the equalizer has changed
                 * @return true if the configuration of the equalizer has changed
                 */
                bool                configuration_changed() const;

                /** Update filter parameters
                 * @param id ID of the filter
                 * @param params  filter parameters
                 * @return true on success
                 */
                bool                set_params(size_t id, const filter_params_t *params);

                /** Apply limits to filter parameters
                 * @param id ID of the filter
                 * @param fp filter parameters to process
                 * @return true on success
                 */
                bool                limit_params(size_t id, filter_params_t *fp);

                /** Get filter parameters
                 * @param id ID of the filter
                 * @param params  filter parameters
                 * @return true on success
                 */
                bool                get_params(size_t id, filter_params_t *params);

                /** Check that filter is active
                 *
                 * @param id ID of filter
                 * @return true if filter is active
                 */
                inline bool         filter_active(size_t id) const { return (id < nFilters) ? vFilters[id].active() : false; }

                /** Check that filter is inactive
                 *
                 * @param id ID of filter
                 * @return true if filter is inactive
                 */
                inline bool         filter_inactive(size_t id) const { return (id < nFilters) ? vFilters[id].inactive() : false; }

                /** Set equalizer mode
                 *
                 * @param mode equalizer mode
                 */
                void                set_mode(equalizer_mode_t mode);

                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                void                set_sample_rate(size_t sr);

                /** Get equalizer mode
                 *
                 * @return equalizer mode
                 */
                inline equalizer_mode_t get_mode() const { return nMode; }

                /** Get equalizer latency
                 *
                 * @return equalizer latency
                 */
                size_t              get_latency();

                /**
                 * Get maximum possible latency for the equalizer
                 * @return maximum possible latency
                 */
                inline size_t       max_latency() const { return nFirSize + (nFirSize >> 1); }

                /** Get frequency chart of the specific filter
                 *
                 * @param id ID of the filter
                 * @param re real part of the frequency chart
                 * @param im imaginary part of the frequency chart
                 * @param f frequencies to calculate value
                 * @param count number of dots for the chart
                 * @return status of operation
                 */
                bool                freq_chart(size_t id, float *re, float *im, const float *f, size_t count);

                /** Get frequency chart of the specific filter
                 *
                 * @param id ID of the filter
                 * @param c complex numbers that contain the filter transfer function
                 * @param f frequencies to calculate filter transfer function
                 * @param count number of points
                 * @return status of operation
                 */
                bool                freq_chart(size_t id, float *c, const float *f, size_t count);

                /**
                 * Get frequency chart of the whole equalizer
                 * @param re real part of the frequency chart
                 * @param im imaginary part of the frequency chart
                 * @param f frequencies to calculate value
                 * @param count number of dots for the chart
                 */
                void                freq_chart(float *re, float *im, const float *f, size_t count);

                /**
                 * Get frequency chart of the whole equalizer
                 * @param c complex numbers that contain the filter transfer function
                 * @param f frequencies to calculate filter transfer function
                 * @param count number of points
                 */
                void                freq_chart(float *c, const float *f, size_t count);

                /** Process the signal
                 *
                 * @param out output signal samples
                 * @param in input signal samples
                 * @param samples number of samples to process
                 */
                void                process(float *out, const float *in, size_t samples);

                /**
                 * Reset the internal memory of filters
                 */
                void                reset();

                /**
                 * Get FIR filer rank
                 * @return FIR filter rank
                 */
                inline size_t       fir_rank() const        { return nFirRank;      }

                /**
                 * Get FIR filer impulse respose size
                 * @return FIR impulse response size
                 */
                inline size_t       fir_ir_size() const     { return nFirSize << 1; }

                /**
                 * Get actual equalizer mode
                 * @return actual equalizer mode
                 */
                inline equalizer_mode_t     mode() const    { return nMode;         }

                /**
                 * Get actual impulse response size depending on the currently set equalizer mode
                 * For IIR filter, the zero IR size is returned
                 * @return actual impulse response size
                 */
                size_t              ir_size() const;

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void                dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_FILTERS_EQUALIZER_H_ */
