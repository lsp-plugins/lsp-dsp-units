/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 16 нояб. 2024 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_METERS_ILUFSMETER_H_
#define LSP_PLUG_IN_DSP_UNITS_METERS_ILUFSMETER_H_

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
         * Integrated Loudness meter. Allows to specify multiple channels and their roles
         * to measure the loudness according to the BS.1770-5 standard specification.
         * The meter does not output LKFS (LUFS) values nor LU (Loudness Unit) values.
         * Instead, it provides the regular mean square value which then can be converted into
         * DBFS, LKFS/LUFS or LU values by applying corresponding logarithmic function.
         *
         * This meter is useful for measuring Integrated LUFS, for Momentary LUFS and
         * Short-Term LUFS consider using LoudnessMeter.
         *
         * By default, it uses the standardized K-weighted filter over 400 ms measurement
         * window as described by the BS.1770-5 specification.
         * If number of channels in the configuration is 1 or 2, then the meter automatically
         * sets designation value for inputs to CENTER for mono configuration or LEFT/RIGHT
         * for stereo configuration.
         */
        class LSP_DSP_UNITS_PUBLIC ILUFSMeter
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
                    float               vBlock[4];      // Buffer for computing gating block with overlapping of 75%

                    float               fWeight;        // Weighting coefficient
                    bs::channel_t       enDesignation;  // Channel designation

                    uint32_t            nFlags;         // Flags
                } split_t;

            protected:
                channel_t              *vChannels;      // List of channels

                float                  *vBuffer;        // Temporary buffer for processing
                float                  *vLoudness;      // Loudness of the gating block

                float                   fBlockPeriod;   // Block measuring period in milliseconds
                float                   fIntTime;       // Integration time
                float                   fMaxIntTime;    // Maximum integration time
                float                   fAvgCoeff;      // Averaging coefficient
                float                   fLoudness;      // Currently measured loudness

                uint32_t                nBlockSize;     // Block measuring samples
                uint32_t                nBlockOffset;   // Block measuring period offset
                uint32_t                nBlockPart;     // The index of the current value to update in the gating block
                uint32_t                nMSSize;        // Overall number of blocks available in buffer
                uint32_t                nMSHead;        // Current position to write new block to buffer
                int32_t                 nMSInt;         // Number of blocks to integrate
                int32_t                 nMSCount;       // Count of processed blocks

                uint32_t                nSampleRate;    // Sample rate
                uint32_t                nChannels;      // Number of channels
                uint32_t                nFlags;         // Update flags
                bs::weighting_t         enWeight;       // Weighting function

                uint8_t                *pData;          // Unaligned data
                uint8_t                *pVarData;       // Unaligned variable data

            public:
                explicit ILUFSMeter();
                ILUFSMeter(const ILUFSMeter &) = delete;
                ILUFSMeter(ILUFSMeter &&) = delete;
                ~ILUFSMeter();

                ILUFSMeter & operator = (const ILUFSMeter &) = delete;
                ILUFSMeter & operator = (ILUFSMeter &&) = delete;

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
                 * @param max_int_time maximum integration time in seconds
                 * @param block_period the block measurement period in milliseconds
                 * @return status of operation
                 */
                status_t        init(size_t channels, float max_int_time = 60, float block_period = dspu::bs::LUFS_MEASURE_PERIOD_MS);

            private:
                float           compute_gated_loudness(float threshold);

            public:
                /**
                 * Bind the buffer to corresponding channel.
                 * @param id channel index to bind
                 * @param in input buffer buffer to bind
                 * @return status of operation
                 */
                status_t        bind(size_t id, const float *in);

                /**
                 * Unbind buffer from channel
                 * @param id channel identifier
                 * @return status of operation
                 */
                inline status_t unbind(size_t id)           { return bind(id, NULL);        }

                /**
                 * Set channel designation
                 * @param id channel identifier
                 * @param designation channel designation
                 */
                status_t        set_designation(size_t id, bs::channel_t designation);

                /**
                 * Get channel designation
                 * @param id identifier of the channel
                 * @return channel designation
                 */
                bs::channel_t   designation(size_t id) const;

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
                 * Set weighting function
                 * @param weighting weighting function
                 */
                void            set_weighting(bs::weighting_t weighting);

                /**
                 * Get the weighting function
                 * @return weighting function
                 */
                inline bs::weighting_t  weighting() const       { return enWeight;      }

                /**
                 * Set the integration period
                 * @param period measurement period
                 */
                void            set_integration_period(float period);

                /**
                 * Get the measurement period
                 * @return measurement period
                 */
                inline float    integration_period() const      { return fIntTime;      }

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
                inline size_t   sample_rate() const             { return nSampleRate;   }

                /**
                 * Process signal from channels and form the gain control signal
                 * @param out buffer to store the overall loudness (optional)
                 * @param count number of samples to process
                 * @param gain additional gain to apply
                 */
                void            process(float *out, size_t count, float gain = bs::DBFS_TO_LUFS_SHIFT_GAIN);

                /**
                 * Get currently measured loudness
                 * @return currently measured loudness
                 */
                inline float    loudness() const                { return fLoudness;     }

                /**
                 * Check that crossover needs to call reconfigure() before processing
                 * @return true if crossover needs to call reconfigure() before processing
                 */
                inline bool     needs_update() const            { return nFlags != 0;   }

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
                 * @param v state dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_DSP_UNITS_METERS_ILUFSMETER_H_ */
