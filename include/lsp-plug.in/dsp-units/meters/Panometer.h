/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 12 нояб. 2024 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_METERS_PANOMETER_H_
#define LSP_PLUG_IN_DSP_UNITS_METERS_PANOMETER_H_

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
         * Pan law
         */
        enum pan_law_t
        {
            /**
             * Linear pan law between left and right channels
             */
            PAN_LAW_LINEAR,

            /**
             * Equal power pan law between left and right channels
             */
            PAN_LAW_EQUAL_POWER
        };

        /**
         * Panorama meter. Computes panorama between two signals.
         */
        class LSP_DSP_UNITS_PUBLIC Panometer
        {
            private:
                float              *vInA;       // History of input buffer 1
                float              *vInB;       // History of input buffer 2
                pan_law_t           enPanLaw;   // Pan law
                float               fValueA;    // Current mean square value for input 1
                float               fValueB;    // Current mean square value for input 2
                float               fNorm;      // Norming factor
                float               fDefault;   // Default value
                uint32_t            nCapacity;  // The overall capacity
                uint32_t            nHead;      // Write position of the buffer
                uint32_t            nMaxPeriod; // Maximum measurement period
                uint32_t            nPeriod;    // Measurement period
                uint32_t            nWindow;    // Number of samples processed before reset

                uint8_t            *pData;      // Pointer to the allocated data

            public:
                Panometer();
                Panometer(const Panometer &) = delete;
                Panometer(Panometer &&) = delete;
                ~Panometer();

                Panometer & operator = (const Panometer &) = delete;
                Panometer && operator = (Panometer &&) = delete;

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
                 * @param max_period the maximum period size in samples
                 * @return status of operation
                 */
                status_t        init(size_t max_period);

            public:
                /**
                 * Set pan law
                 * @param law pan law
                 */
                void            set_pan_law(pan_law_t law);

                /**
                 * Get pan law
                 * @return pan law
                 */
                inline pan_law_t pan_law() const                { return enPanLaw;      }

                /**
                 * Set default value for panorama if it is not possible to compute it
                 * @param dfl default value for panorama
                 */
                void            set_default_pan(float dfl);

                /**
                 * Get default value for panorama if it is not possible to compute it
                 * @return default value for panorama
                 */
                inline float    default_pan() const             { return fDefault;      }

                /**
                 * Set the correlation computation period
                 * @param period correlation computation period
                 */
                void            set_period(size_t period);

                /**
                 * Get the correlation computation period
                 * @return correlation computation period
                 */
                inline size_t   period() const                  { return nPeriod;       }

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
                 * @param v state dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_METERS_PANOMETER_H_ */
