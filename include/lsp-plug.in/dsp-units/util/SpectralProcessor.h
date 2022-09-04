/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 1 июл. 2020 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_SPECTRALPROCESSOR_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_SPECTRALPROCESSOR_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Spectral processor callback function
         * @param object the object that handles callback
         * @param subject the subject that is used to handle callback
         * @param spectrum spectral data for processing (packed complex numbers)
         * @param rank the overall rank of the FFT transform (log2(size))
         */
        typedef void (* spectral_processor_func_t)(void *object, void *subject, float *spectrum, size_t rank);

        /**
         * Spectral processor class, performs spectral transform of the input signal
         * and launches callback function to process the signal spectrum
         */
        class LSP_DSP_UNITS_PUBLIC SpectralProcessor
        {
            private:
                SpectralProcessor & operator = (const SpectralProcessor &);
                SpectralProcessor(const SpectralProcessor &);

            protected:
                size_t                      nRank;      // Current FFT rank
                size_t                      nMaxRank;   // Maximum FFT rank
                float                       fPhase;     // Phase
                float                      *pWnd;       // Window function
                float                      *pOutBuf;    // Output buffer
                float                      *pInBuf;     // Input buffer
                float                      *pFftBuf;    // FFT buffer
                size_t                      nOffset;    // Read/Write offset
                uint8_t                    *pData;      // Data buffer
                bool                        bUpdate;    // Update flag

                // Bindings
                spectral_processor_func_t   pFunc;      // Function
                void                       *pObject;    // Object to operate
                void                       *pSubject;   // Subject to operate

            public:
                explicit SpectralProcessor();
                ~SpectralProcessor();

                /**
                 * Construct object
                 */
                void            construct();

                /**
                 * Initialize spectral processor
                 * @param max_rank maximum FFT rank
                 * @return status of operation
                 */
                bool            init(size_t max_rank);

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
                void            bind(spectral_processor_func_t func, void *object, void *subject);

                /**
                 * Unbind spectral processor
                 */
                void            unbind();

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
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param count number of samples to process
                 */
                void            process(float *dst, const float *src, size_t count);
    
                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_SPECTRALPROCESSOR_H_ */
