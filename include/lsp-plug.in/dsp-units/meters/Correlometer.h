/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 8 мар. 2024 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_METERS_CORRELOMETER_H_
#define LSP_PLUG_IN_DSP_UNITS_METERS_CORRELOMETER_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {

        /**
         * Corellometer. Computes normalized correlation between two signals.
         */
        class LSP_DSP_UNITS_PUBLIC Correlometer
        {
            private:
                enum flags_t {
                    CF_UPDATE       = 1 << 0    // Update the correlation function
                };

            private:
                dsp::correlation_t  sCorr;      // Correlation function
                float              *vInA;       // Input buffer 1
                float              *vInB;       // Input buffer 2
                uint32_t            nCapacity;  // The overall capacity
                uint32_t            nHead;      // Write position of the buffer
                uint32_t            nMaxPeriod; // Maximum measurement period
                uint32_t            nPeriod;    // Measurement period
                uint32_t            nWindow;    // Number of samples processed before reset
                uint32_t            nFlags;     // Flags

                uint8_t            *pData;      // Pointer to the allocated data

            public:
                Correlometer();
                Correlometer(const Correlometer &) = delete;
                Correlometer(Correlometer &&) = delete;
                ~Correlometer();

                Correlometer & operator = (const Correlometer &) = delete;
                Correlometer && operator = (Correlometer &&) = delete;

                /**
                 *  Construct object
                 */
                void            construct();

                /**
                 * Destroy object
                 */
                void            destroy();

                /**
                 * Initialize object
                 * @param max_chunk_size the maximum period size in samples
                 * @return
                 */
                status_t        init(size_t max_period);

            public:

                /**
                 * Set the correlation computation period
                 * @param period correlation computation period
                 */
                void            set_period(size_t period);

                /**
                 * Get the correlation computation period
                 * @return correlation computation period
                 */
                inline size_t   period() const              { return nPeriod; }

                /**
                 * Check that crossover needs to call reconfigure() before processing
                 * @return true if crossover needs to call reconfigure() before processing
                 */
                inline bool     needs_update() const            { return nFlags != 0; }

                /** Reconfigure crossover after parameter update
                 *
                 */
                void            update_settings();

                /**
                 * Clear internal state
                 */
                void            clear();

                /**
                 * Process the input data and compute the correlation function
                 * @param dst destination buffer to store the correlation function
                 * @param a pointer to the first buffer
                 * @param b pointer to the second buffer
                 * @param count number of samples to process
                 */
                void            process(float *dst, const float *a, const float *b, size_t count);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_METERS_CORRELOMETER_H_ */

