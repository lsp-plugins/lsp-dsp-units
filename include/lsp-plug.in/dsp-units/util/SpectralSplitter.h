/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 6 авг. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_SPECTRALSPLITTER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_SPECTRALSPLITTER_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Spectral splitter callback function
         * @param object the object that handles callback
         * @param subject the subject that is used to handle callback
         * @param out the buffer to store processed spectral data (packed complex numbers)
         * @param in spectral data for processing (packed complex numbers)
         * @param rank the overall rank of the FFT transform (log2(size))
         */
        typedef void (* spectral_splitter_func_t)(
            void *object,
            void *subject,
            float *out,
            const float *in,
            size_t rank);

        /**
         * Spectral processor callback function
         * @param object the object that handles callback
         * @param subject the subject that is used to handle callback
         * @param samples the pointer to the buffer containg processed samples
         * @param first the position of the first stample in the processed samples relative
         *   to the first sample in the source (unprocessed) buffer passed to the process() call
         * @param rank the overall rank of the FFT transform (log2(size))
         */
        typedef void (* spectral_splitter_sink_t)(
            void *object,
            void *subject,
            const float *samples,
            size_t first,
            size_t count);

        /**
         * Spectral processor class, performs spectral transform of the input signal
         * and launches callback function to process the signal spectrum
         */
        class LSP_DSP_UNITS_PUBLIC SpectralSplitter
        {
            private:
                SpectralSplitter & operator = (const SpectralSplitter &);
                SpectralSplitter(const SpectralSplitter &);

            protected:
                typedef struct handler_t
                {
                    void                       *pObject;    // Object to operate
                    void                       *pSubject;   // Subject to operate
                    spectral_splitter_func_t    pFunc;      // Spectral processing function
                    spectral_splitter_sink_t    pSink;      // Spectral sink function
                    float                      *vOutBuf;    // Output buffer
                } handler_t;

            protected:
                size_t                      nRank;          // Current FFT rank
                size_t                      nMaxRank;       // Maximum FFT rank
                ssize_t                     nUserChunkRank; // User chunk rank
                size_t                      nChunkRank;     // Current FFT chunk rank
                float                       fPhase;         // Phase
                float                      *vWnd;           // Window function
                float                      *vInBuf;         // Input buffer
                float                      *vFftBuf;        // FFT buffer
                float                      *vFftTmp;        // Temporary FFT buffer
                size_t                      nFrameSize;     // Current frame size
                size_t                      nInOffset;      // Offset of input buffer
                bool                        bUpdate;        // Update flag
                handler_t                  *vHandlers;      // Handlers
                size_t                      nHandlers;      // Number of handlers
                size_t                      nBindings;      // Number of bindings

                uint8_t                    *pData;          // Data buffer

            public:
                explicit SpectralSplitter();
                ~SpectralSplitter();

                /**
                 * Construct object
                 */
                void            construct();

                /**
                 * Initialize spectral processor
                 * @param max_rank maximum FFT rank
                 * @param handlers the number of handlers to use
                 * @return status of operation
                 */
                status_t        init(size_t max_rank, size_t handlers);

                /**
                 * Destroy spectral processor
                 */
                void            destroy();

            public:
                /**
                 * Bind spectral processor to the handler.
                 * @note Initialization drops all perviously bindings, so after init() the
                 * caller core is required to setub bindings again.
                 *
                 * @param id the index of the handler
                 * @param object the object to pass to the callback functions
                 * @param subject the target subject to pass to the callback functions
                 */
                status_t        bind(
                    size_t id,
                    void *object,
                    void *subject,
                    spectral_splitter_func_t func,
                    spectral_splitter_sink_t sink);

                /**
                 * Unbind spectral handler
                 * @param id the index of the handler
                 */
                status_t        unbind(size_t id);

                /**
                 * Unbind all bindings
                 */
                void            unbind_all();

                /**
                 * Check that binding is already set for the specific handler
                 * @return true if binding is set
                 */
                bool            bound(size_t id) const;

                /**
                 * Return the maximum number of possible handlers
                 * @return maximum number of possible handlers
                 */
                inline size_t   handlers() const            { return nHandlers;         }

                /**
                 * Return the overall number of bound bands
                 * @return overall number of bound bands
                 */
                inline size_t   bindings() const            { return nBindings;         }

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
                inline size_t   rank() const                { return nRank;             }

                /**
                 * Get the maximum FFT rank
                 * @return maximum FFT rank
                 */
                inline size_t   max_rank() const            { return nMaxRank;          }

                /**
                 * Get the FFT chunk rank
                 * @return FFT chunk rank
                 */
                inline ssize_t  chunk_rank() const          { return nChunkRank;        }

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
                 * Set the FFT chunk rank
                 */
                void            set_chunk_rank(ssize_t rank);

                /**
                 * Get latency of the spectral processor
                 * @return latency of the spectral processor
                 */
                size_t          latency() const;

                /**
                 * Perform audio processing, all processed data is processed by the handlers
                 * with defined spectral functions and passed to bound sinks.
                 * @param src source buffer to process
                 * @param count number of samples to process
                 */
                void            process(const float *src, size_t count);

                /**
                 * Clear internal memory and buffers
                 */
                void            clear();

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_SPECTRALSPLITTER_H_ */
