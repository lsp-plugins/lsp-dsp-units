/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 29 янв. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_CONVOLVER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_CONVOLVER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

#define CONVOLVER_RANK_MIN          8                               /* buffer of 256 samples (128 effective)        */
#define CONVOLVER_RANK_MAX          16                              /* buffer of 62256 samples (32768 effective)    */

namespace lsp
{
    namespace dspu
    {
        /**
         * Real-time convolver module. Performs hybrid convolution using FFT and direct implementation
         * to reach zero latency output.
         */
        class LSP_DSP_UNITS_PUBLIC Convolver
        {
            private:
                float          *vDataBuffer;            // Buffer for storing convolution tail data
                float          *vFrameStart;            // Start of the frame
                float          *vFrame;                 // Current pointer to the beginning of the input data frame
                float          *vConvBuffer;            // Convolution buffer to perform convolution
                float          *vTaskData;              // Task data for tail convolution
                float          *vConvData;              // FFT convolution data
                float          *vDirectData;            // Direct convolution data

                uint32_t        nDataBufferSize;        // Size of data buffer
                uint32_t        nDirectSize;            // Size of direct convolution data
                uint32_t        nFrameSize;             // Size of input data frame
                uint32_t        nFrameOff;              // Offset from the beginning of the input data frame
                uint32_t        nConvSize;              // The actual convolution size in samples
                uint32_t        nLevels;                // Number of raising convolution levels
                uint32_t        nBlocks;                // Number of constant-size blocks
                uint32_t        nBlocksDone;            // Number of applied constant-size blocks
                uint32_t        nRank;                  // The actual rank of the convolution
                uint32_t        nBlkInit;               // Initial number of blocks to apply at step # 0
                float           fBlkCoef;               // The actual coefficient to compute proper block number per formula

                uint8_t        *vData;                  // Non-aligned pointer to the whole allocated data

            public:
                explicit Convolver();
                Convolver(const Convolver &) = delete;
                Convolver(Convolver &&) = delete;
                ~Convolver();

                Convolver & operator = (const Convolver &) = delete;
                Convolver & operator = (Convolver &&) = delete;

                /** Construct the convolver
                 *
                 */
                void construct();

                /** Destroy convolver
                 *
                 */
                void destroy();

            public:

                /**
                 * Initialize convolver. Performs memory reallocation if passed rank
                 * or convolution plan differs to the previous setup.
                 *
                 * @param data convolution data
                 * @param count number of samples in convolution
                 * @param rank convolution rank
                 * @return true on success
                 */
                bool init(const float *data, size_t count, size_t rank, float phase);

                /** Process samples
                 *
                 * @param dst destination buffer
                 * @param src source buffer
                 * @param count number of samples to process
                 */
                void process(float *dst, const float *src, size_t count);

                /**
                 * Cleanup convolver state (internal memory)
                 */
                void clear();

                /** Get the actual convolution size in samples
                 *
                 * @return actual convolution size in samples
                 */
                inline size_t data_size() const             { return nConvSize;     }

                /**
                 * Get actual convolution rank of the convolver
                 * @return convolution rank
                 */
                inline size_t rank() const                  { return nRank;         }

                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_CONVOLVER_H_ */
