/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 20 сент. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_METERS_LOUDNESSMETER_H_
#define LSP_PLUG_IN_DSP_UNITS_METERS_LOUDNESSMETER_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/dsp-units/misc/broadcast.h>
#include <lsp-plug.in/dsp-units/filters/Filter.h>
#include <lsp-plug.in/dsp-units/filters/FilterBank.h>


namespace lsp
{
    namespace dspu
    {
        /**
         * Loudness meter. Allows to specify multiple channels and their roles to measure the
         * loudness according to the BS.1770-4 standard specification.
         * The meter does not output LKFS (LUFS) values nor LU (Loudness Unit) values.
         * Instead, it provides the regular RMS value which then can be converted into
         * DBFS, LKFS/LUFS or LU values by applying logarithmic function.
         */
        class LSP_DSP_UNITS_PUBLIC LoudnessMeter
        {
            protected:
                enum c_flags_t {
                    C_ENABLED       = 1 << 0
                };

                enum flags_t
                {
                    F_UPD_FILTERS   = 1 << 0,
                    F_UPD_TIME      = 1 << 1,

                    F_UPD_ALL       = F_UPD_FILTERS | F_UPD_TIME
                };

                typedef struct channel_t
                {
                    dspu::FilterBank    sBank;          // Filter bank
                    dspu::Filter        sFilter;        // Band filter

                    const float        *vIn;            // The input buffer
                    float              *vOut;           // Output gain buffer
                    float              *vData;          // Ring Buffer for measuring mean square
                    float              *vMS;            // Temporary buffer for measured mean square

                    float               fMS;            // Current Mean square value
                    float               fWeight;        // Weighting coefficient
                    float               fLink;          // Channel linking
                    bs::channel_t       enDesignation;  // Channel designation

                    size_t              nFlags;         // Flags
                    size_t              nOffset;        // The offset relative to the beginning of the buffer
                } split_t;

            protected:
                channel_t              *vChannels;      // List of channels

                float                  *vBuffer;        // Temporary buffer for processing

                float                   fPeriod;        // Measuring period
                float                   fMaxPeriod;     // Maximum measuring period
                float                   fAvgCoeff;      // Averaging coefficient

                size_t                  nSampleRate;    // Sample rate
                size_t                  nPeriod;        // Measuring period
                size_t                  nMSRefresh;    // RMS refresh counter
                size_t                  nChannels;      // Number of channels
                size_t                  nFlags;         // Update flags
                size_t                  nDataHead;      // Position in the data buffer
                size_t                  nDataSize;      // Size of data buffer
                bs::weighting_t         enWeight;       // Weighting function

                uint8_t                *pData;          // Unaligned data
                uint8_t                *pVarData;       // Unaligned variable data

            protected:
                void                    refresh_rms();
                size_t                  process_channels(size_t offset, size_t samples);

            public:
                explicit LoudnessMeter();
                LoudnessMeter(const LoudnessMeter &) = delete;
                LoudnessMeter(LoudnessMeter &&) = delete;
                ~LoudnessMeter();

                LoudnessMeter & operator = (const LoudnessMeter &) = delete;
                LoudnessMeter & operator = (LoudnessMeter &&) = delete;

                /** Construct object
                 *
                 */
                void            construct();

                /** Destroy object
                 *
                 */
                void            destroy();

                /** Initialize object
                 *
                 * @param channels number of input channels
                 * @param max_period maximum measurement period in milliseconds
                 * @return status of operation
                 */
                status_t        init(size_t channels, float max_period);

            public:
                /**
                 * Bind the buffer to corresponding channel
                 * @param id channel index to bind
                 * @param in input buffer buffer to bind
                 * @param out output buffer to store measured loudness of channel (optional)
                 * @param pos position to start processing
                 * @return status of operation
                 */
                status_t        bind(size_t id, float *out, const float *in, size_t pos = 0);

                /**
                 * Unbind channel
                 * @param id channel identifier
                 * @return status of operation
                 */
                inline status_t unbind(size_t id)           { return bind(id, NULL, 0);     }

                /**
                 * Set channel designation
                 * @param id channel identifier
                 * @param designation channel designation
                 */
                status_t        set_designation(size_t id, bs::channel_t designation);

                /**
                 * Set channel linking to the overall loudness
                 * @param id channel id
                 * @param link link amount [0..1], 0 means no linking, 1 means full linking
                 * @return status of operation
                 */
                status_t        set_link(size_t id, float link);

                /**
                 * Get the linking of the channel
                 * @param id channel identifier
                 * @return linking of the channel
                 */
                float           link(size_t id) const;

                /**
                 * Set channel active
                 * @param id channel identifier
                 * @param active channel activity
                 * @return status of operation
                 */
                status_t        set_active(size_t id, bool active=true);

                /**
                 * Check that channel is active
                 * @param id channel identifier
                 * @return true if channel is active
                 */
                bool            active(size_t id) const;

                /**
                 * Get channel designation
                 * @param id identifier of the channel
                 * @return channel designation
                 */
                bs::channel_t   designation(size_t id) const;

                /**
                 * Set weighting function
                 * @param weighting weighting function
                 */
                void            set_weighting(bs::weighting_t weighting);

                /**
                 * Get the weighting function
                 * @return weighting function
                 */
                inline bs::weighting_t  weighting() const       { return enWeight; }

                /**
                 * Set the measurement period
                 * @param period measurement period
                 */
                void            set_period(float period);

                /**
                 * Get the measurement period
                 * @return measurement period
                 */
                inline float    period() const                  { return fPeriod; }

                /**
                 * Set sample rate
                 * @param sample_rate sample rate to set
                 * @return status of operation
                 */
                status_t        set_sample_rate(size_t sample_rate);

                /**
                 * Get the sample rate
                 * @return sample rate
                 */
                inline size_t   sample_rate() const             { return nSampleRate; }

                /**
                 * Get actual latency
                 * @return actual latency
                 */
                size_t          latency() const;

                /**
                 * Process signal from channels and form the gain control signal
                 * @param out buffer to store the overall loudness (optional)
                 * @param count number of samples to process
                 */
                void            process(float *out, size_t count);

                /**
                 * Process signal from channels and form the gain control signal
                 * @param out buffer to store the overall loudness (optional)
                 * @param count number of samples to process
                 * @param gain output gain correction
                 */
                void            process(float *out, size_t count, float gain);

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
                 * Dump the state
                 * @param dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_METERS_LOUDNESSMETER_H_ */
