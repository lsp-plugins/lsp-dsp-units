/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 13 нояб. 2025 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_MULTISPECTRALPROCESSOR_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_MULTISPECTRALPROCESSOR_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Multi-Spectral processor callback function
         * @param object the object that handles callback
         * @param subject the subject that is used to handle callback
         * @param spectrum spectral data for processing (packed complex numbers)
         * @param rank the overall rank of the FFT transform (log2(size))
         */
        typedef void (* multi_spectral_processor_func_t)(void *object, void *subject, float * const * spectrum, size_t rank);

        /**
         * Multi-Spectral processor class, performs multi-channel spectral transform of the input signal
         * and launches callback function to process the signal spectrum
         */
        class LSP_DSP_UNITS_PUBLIC MultiSpectralProcessor
        {
            protected:
                typedef struct channel_t
                {
                    const float            *pIn;        // Input data pointer
                    float                  *pOut;       // Output data pointer
                    float                  *pInBuf;     // Input buffer
                    float                  *pOutBuf;    // Output buffer
                    float                  *pFftBuf;    // FFT buffer
                } channel_t;

            protected:
                void                        clear_buffers();

            protected:
                uint32_t                    nChannels;  // Number of channels
                uint32_t                    nRank;      // Current FFT rank
                uint32_t                    nMaxRank;   // Maximum FFT rank
                uint32_t                    nOffset;    // Read/Write offset
                channel_t                  *vChannels;  // Channels
                float                     **vFftBuf;    // FFT transform buffers
                float                      *pWnd;       // Window function
                float                       fPhase;     // Phase
                bool                        bUpdate;    // Update flag

                // Bindings
                multi_spectral_processor_func_t pFunc;  // Function
                void                       *pObject;    // Object to operate
                void                       *pSubject;   // Subject to operate

                // Misc
                uint8_t                    *pData;      // Data buffer

            public:
                explicit MultiSpectralProcessor();
                MultiSpectralProcessor(const MultiSpectralProcessor &) = delete;
                MultiSpectralProcessor(MultiSpectralProcessor &&) = delete;
                ~MultiSpectralProcessor();

                MultiSpectralProcessor & operator = (const MultiSpectralProcessor &) = delete;
                MultiSpectralProcessor & operator = (MultiSpectralProcessor &&) = delete;

                /**
                 * Construct object
                 */
                void            construct();

                /**
                 * Initialize spectral processor
                 * @param channels number of channels
                 * @param max_rank maximum FFT rank
                 * @return status of operation
                 */
                bool            init(size_t channels, size_t max_rank);

                /**
                 * Destroy spectral processor
                 */
                void            destroy();

            public:
                /**
                 * Bind spectral processor to the handler
                 * @param func function to call
                 * @param object the target object to pass to the function
                 * @param subject the target subject to pass to the function
                 */
                void            bind_handler(multi_spectral_processor_func_t func, void *object, void *subject);

                /**
                 * Unbind spectral processor
                 */
                void            unbind_handler();

                /**
                 * Bind buffers to the channel.
                 * @param index index of the channel
                 * @param out output buffer. If output buffer is not specified, back reverse FFT is not performed
                 * @param in input buffer. If input buffer is not specified, processing of the channel is disabled
                 * @return status of operation
                 */
                status_t        bind(size_t index, float *out, const float *in);

                /**
                 * Bind input buffer to the channel.
                 * @param index index of the channel
                 * @param in input buffer. If input buffer is not specified, processing of the channel is disabled
                 * @return status of operation
                 */
                status_t        bind_in(size_t index, const float *in);

                /**
                 * Bind output buffer to the channel.
                 * @param index index of the channel
                 * @param out output buffer. If output buffer is not specified, back reverse FFT is not performed
                 * @return status of operation
                 */
                status_t        bind_out(size_t index, float *out);

                /**
                 * Unbind input and output buffers from specific channel
                 * @param index index of the channel
                 * @return status of operation
                 */
                status_t        unbind(size_t index);

                /**
                 * Unbind input buffer from specific channel
                 * @param index index of the channel
                 * @return status of operation
                 */
                status_t        unbind_in(size_t index);

                /**
                 * Unbind output buffer from specific channel
                 * @param index index of the channel
                 * @return status of operation
                 */
                status_t        unbind_out(size_t index);

                /**
                 * Unbind all buffers
                 * @param index index of the channel
                 */
                void            unbind_all();

                /**
                 * Check that spectral processor needs update
                 * @return true if spectral processor needs update
                 */
                inline bool     needs_update() const        { return bUpdate;           }

                /**
                 * Update settings of the spectral processor
                 */
                void            update_settings();

                /**
                 * Get the FFT rank
                 * @return FFT rank
                 */
                inline size_t   get_rank() const            { return nRank;             }

                /**
                 * Get processing phase
                 * @return processing phase
                 */
                inline float    phase() const               { return fPhase;            }

                /**
                 * Set processing phase
                 * @param phase the phase value between 0 and 1
                 */
                void            set_phase(float phase);

                /**
                 * Set the FFT rank
                 */
                void            set_rank(size_t rank);

                /**
                 * Get latency of the spectral processor
                 * @return latency of the spectral processor
                 */
                inline size_t   latency() const             { return 1 << nRank;        }

                /**
                 * Perform audio processing
                 * @param count number of samples to process
                 */
                void            process(size_t count);

                /**
                 * Reset state: cleanup internal buffers
                 */
                void            reset();

                /**
                 * Return number of samples remaining before the FFT transform operation occurs
                 * @return number of samples remaining before the FFT transform occurs
                 */
                size_t          remaining() const;

                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void            dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_MULTISPECTRALPROCESSOR_H_ */
