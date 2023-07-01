/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 нояб. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_OVERSAMPLER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_OVERSAMPLER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/filters/Filter.h>

namespace lsp
{
    namespace dspu
    {
        /** Callback to perform processing of oversampled signal
         *
         */
        class LSP_DSP_UNITS_PUBLIC IOversamplerCallback
        {
            public:
                /** Virtual destructor
                 *
                 */
                virtual ~IOversamplerCallback();

                /** Processing routine
                 *
                 * @param out output buffer of samples size
                 * @param in input buffer of samples size
                 * @param samples number of samples to process
                 */
                virtual void process(float *out, const float *in, size_t samples);
        };

        /**
         * Oversampler callback routine
         * @param out output buffer (oversampled)
         * @param in input buffer (oversampled)
         * @param samples number of oversampled samples in the buffer
         * @param arg additional argument which is passed to the routine
         */
        typedef void (*oversampler_callback_t)(float *out, const float *in, size_t samples, void *arg);

        enum over_mode_t
        {
            OM_NONE,

            OM_LANCZOS_2X2,
            OM_LANCZOS_2X3,
            OM_LANCZOS_2X4,
            OM_LANCZOS_2X12BIT,
            OM_LANCZOS_2X16BIT,
            OM_LANCZOS_2X24BIT,

            OM_LANCZOS_3X2,
            OM_LANCZOS_3X3,
            OM_LANCZOS_3X4,
            OM_LANCZOS_3X12BIT,
            OM_LANCZOS_3X16BIT,
            OM_LANCZOS_3X24BIT,

            OM_LANCZOS_4X2,
            OM_LANCZOS_4X3,
            OM_LANCZOS_4X4,
            OM_LANCZOS_4X12BIT,
            OM_LANCZOS_4X16BIT,
            OM_LANCZOS_4X24BIT,

            OM_LANCZOS_6X2,
            OM_LANCZOS_6X3,
            OM_LANCZOS_6X4,
            OM_LANCZOS_6X12BIT,
            OM_LANCZOS_6X16BIT,
            OM_LANCZOS_6X24BIT,

            OM_LANCZOS_8X2,
            OM_LANCZOS_8X3,
            OM_LANCZOS_8X4,
            OM_LANCZOS_8X12BIT,
            OM_LANCZOS_8X16BIT,
            OM_LANCZOS_8X24BIT,
        };

        /** Oversampler class
         *
         */
        class LSP_DSP_UNITS_PUBLIC Oversampler
        {
            private:
                Oversampler & operator = (const Oversampler &);
                Oversampler(const Oversampler &);

            protected:
                typedef void (*resample_func_t)(float *dst, const float *src, size_t count);

            protected:
                enum update_t
                {
                    UP_MODE         = 1 << 0,
                    UP_SAMPLE_RATE  = 1 << 2,
                    UP_OTHER        = 1 << 3,

                    UP_ALL          = UP_MODE | UP_OTHER | UP_SAMPLE_RATE
                };

            protected:
                IOversamplerCallback   *pCallback;
                float                  *fUpBuffer;
                float                  *fDownBuffer;
                resample_func_t         pFunc;
                size_t                  nUpHead;
                size_t                  nMode;
                size_t                  nSampleRate;
                size_t                  nUpdate;
                Filter                  sFilter;
                uint8_t                *bData;
                bool                    bFilter;

            protected:
                static resample_func_t  get_function(size_t mode);

            public:
                explicit Oversampler();
                ~Oversampler();

                void construct();

            public:
                /** Initialize oversampler
                 *
                 */
                bool init();

                /** Destroy oversampler
                 *
                 */
                void destroy();

                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                void set_sample_rate(size_t sr);

                /** Set oversampling callback
                 *
                 * @param callback calback to call on process()
                 */
                inline void set_callback(IOversamplerCallback *callback)
                {
                    pCallback       = callback;
                }

                /** Set oversampling mode
                 *
                 * @param mode oversampling mode
                 */
                void set_mode(over_mode_t mode);

                /**
                 * Get oversampling mode
                 * @return current oversampling mode
                 */
                over_mode_t mode() const;

                /** Enable/disable low-pass filter when performing downsampling
                 *
                 * @param filter enables/diables low-pass filter
                 */
                inline void set_filtering(bool filter)
                {
                    if (bFilter == filter)
                        return;
                    bFilter     = filter;
                    nUpdate   |= UP_MODE;
                }

                /**
                 * Get filtering option
                 * @return filtering option
                 */
                bool filtering() const;

                /** Check that module needs re-configuration
                 *
                 * @return true if needs reconfiguration
                 */
                inline bool modified() const
                {
                    return nUpdate;
                }

                /** Get current oversampling multiplier
                 *
                 * @return current oversampling multiplier
                 */
                size_t get_oversampling() const;

                /** Update settings
                 *
                 */
                void update_settings();

                /** Perform upsampling of the signal
                 *
                 * @param dst destination buffer of samples*ratio size
                 * @param src source buffer of samples size
                 * @param samples number of samples that should be processed in src buffer
                 */
                void upsample(float *dst, const float *src, size_t samples);

                /** Perform downsampling of the signal
                 *
                 * @param dst destination buffer of samples size
                 * @param src source buffer of samples*ratio size
                 * @param samples number of samples that should be produced into the dst buffer
                 */
                void downsample(float *dst, const float *src, size_t samples);

                /** Perform processing of the signal
                 *
                 * @param dst destination buffer of samples size
                 * @param src source buffer of samples size
                 * @param samples number of samples to process
                 * @param callback callback to handle buffer (optional, can be NULL)
                 */
                void process(float *dst, const float *src, size_t samples, IOversamplerCallback *callback);

                /** Perform processing of the signal
                 *
                 * @param dst destination buffer of samples size
                 * @param src source buffer of samples size
                 * @param samples number of samples to process
                 * @param callback callback routine that processes the oversampled data (optional, can be NULL)
                 * @param arg additional argument passed to the callback routine (optional, can be NULL)
                 */
                void process(float *dst, const float *src, size_t samples, oversampler_callback_t callback, void *arg);

                /** Perform processing of the signal
                 *
                 * @param dst destination buffer of samples size
                 * @param src source buffer of samples size
                 * @param samples number of samples to process
                 */
                inline void process(float *dst, const float *src, size_t samples)
                {
                    process(dst, src, samples, pCallback);
                }

                /**
                 * Get oversampler latency
                 * @return oversampler latency in normal (non-oversampled) samples
                 */
                size_t latency() const;

                /**
                 * Get maximum possible latency
                 * @return maximum possible latency
                 */
                inline size_t max_latency() const       { return 8; }
    
                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_OVERSAMPLER_H_ */
