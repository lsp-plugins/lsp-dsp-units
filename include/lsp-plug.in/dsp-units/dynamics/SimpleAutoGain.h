/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 28 ноя. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_DYNAMICS_SIMPLEAUTOGAIN_H_
#define LSP_PLUG_IN_DSP_UNITS_DYNAMICS_SIMPLEAUTOGAIN_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/misc/broadcast.h>

namespace lsp
{
    /*
        This class performs the simplified automatic gain control
        If audio signal level is below the threshold, it adds some gain.
        If audio signal level is above the threshold, it removes some gain.
     */

    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC SimpleAutoGain
        {
            protected:
                enum flags_t
                {
                    F_UPDATE        = 1 << 0
                };

            protected:
                uint32_t        nSampleRate;    // Current sample rate
                uint32_t        nFlags;         // Different flags

                float           fKGrow;         // Gain grow coefficient
                float           fKFall;         // Gain fall coefficient
                float           fGrow;          // Grow speed
                float           fFall;          // Fall speed
                float           fThreshold;     // The expected gain threshold
                float           fCurrGain;      // Current gain value
                float           fMinGain;       // Minimum possible amplification
                float           fMaxGain;       // Maximum possible amplification

            public:
                explicit SimpleAutoGain();
                SimpleAutoGain(const SimpleAutoGain &) = delete;
                SimpleAutoGain(SimpleAutoGain &&) = delete;
                ~SimpleAutoGain();

                SimpleAutoGain & operator = (const SimpleAutoGain &) = delete;
                SimpleAutoGain & operator = (SimpleAutoGain &&) = delete;

                /** Construct object
                 *
                 */
                void            construct();

                /** Destroy object
                 *
                 */
                void            destroy();

                /**
                 * Initialize look-back buffer
                 * @return status of operation
                 */
                status_t        init();

            public:
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
                 * Set the long gain grow speed
                 * @param value long gain grow speed in dB/s
                 */
                void            set_grow(float value);

                /**
                 * Get the long gain grow speed
                 * @return long gain grow speed in dB/s
                 */
                inline float    grow() const                    { return fGrow;                         }

                /**
                 * Set the long gain fall-off speed
                 * @param value long gain fall-off speed in dB/s
                 */
                void            set_fall(float value);

                /**
                 * Get the long gain fall-off speed
                 * @return long gain fall-off speed in dB/s
                 */
                float           fall() const                    { return fFall;                         }

                /**
                 * Set reaction speed for long-time loudness signal
                 * @param grow the gain grow speed in dB/s
                 * @param fall the gain fall-off speed in dB/s
                 */
                void            set_speed(float grow, float fall);

                /**
                 * Set the maximum possible amplification gain
                 * @param value maximum possible amplification gain
                 */
                void            set_max_gain(float value);

                /**
                 * Get the maximum possible gain amplification value
                 * @return maximum possible gain amplification value
                 */
                inline bool     max_gain() const                { return fMaxGain;                      }

                /**
                 * Set the minimum possible amplification gain
                 * @param value minimum possible amplification gain
                 */
                void            set_min_gain(float value);

                /**
                 * Get the minimum possible gain amplification value
                 * @return minimum possible gain amplification value
                 */
                bool            min_gain() const                { return fMinGain;                      }

                /**
                 * Set gain adjustment range
                 * @param min minimum gain
                 * @param max maximum gain
                 */
                void            set_gain(float min, float max);

                /**
                 * Get current gain amplification
                 * @return current gain amplification
                 */
                inline float    gain() const                    { return lsp_limit(fCurrGain, fMinGain, fMaxGain); }

                /**
                 * Check that the module needs settings update
                 * @return
                 */
                inline bool     needs_update() const            { return nFlags & F_UPDATE; }

                /**
                 * Force settings to update
                 */
                void            update();

                /**
                 * Get threshold
                 * @return threshold
                 */
                inline float    threshold() const               { return fThreshold; }

                /**
                 * Set threshold
                 * @param threshold threshold to set
                 */
                void            set_threshold(float threshold);

                /**
                 * Analyze the gain value and generate gain adjusment samples
                 * @param dst destination buffer to store the gain adjustment
                 * @param src measured gain
                 * @param count number of samples to process
                 */
                void            process(float *dst, const float *src, size_t count);

                /**
                 * Process simple measured gain sample and compute the output gain
                 * @param src measured gain
                 * @param count number of samples to process
                 */
                float           process(float src);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_DYNAMICS_AUTOGAIN_H_ */
