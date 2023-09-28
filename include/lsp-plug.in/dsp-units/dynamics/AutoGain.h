/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 сент. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_DYNAMICS_AUTOGAIN_H_
#define LSP_PLUG_IN_DSP_UNITS_DYNAMICS_AUTOGAIN_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/misc/broadcast.h>

namespace lsp
{
    /*
        This class performs the normalization of audio signal level.
        If audio signal level changes slowly, then it slowly compensates the level.
        If audio signal level grows rapidly, then it rapidly reduces the gain.
        The normalizer just outputs the gain compensation level to match the desired level.
     */

    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC AutoGain
        {
            protected:
                typedef struct timing_t
                {
                    float           fGrow;
                    float           fFall;
                    float           fKGrow;
                    float           fKFall;
                } timing_t;

                typedef struct
                {
                    float       x1, x2;
                    float       a, b, c, d;
                } compressor_t;

                enum flags_t
                {
                    F_UPDATE        = 1 << 0,
                    F_SURGE         = 1 << 1
                };

            protected:
                size_t          nSampleRate;    // Current sample rate
                size_t          nFlags;         // Different flags
                size_t          nLookHead;      // Look-back buffer write position
                size_t          nLookSize;      // Look-back buffer size
                size_t          nLookOffset;    // Look-back buffer offset
                float          *vLookBack;      // Look-back buffer

                timing_t        sShort;         // Stort timings
                timing_t        sLong;          // Long timings
                compressor_t    sComp;          // Compressor settings
                float           fSilence;       // Silence threshold
                float           fDeviation;     // Level deviation
//                float           fRevDeviation;  // Reverse deviation
                float           fCurrGain;      // Current gain value
                float           fCurrEnv;       // Current short envelope
                float           fMinGain;       // Minimum possible gain value
                float           fMaxGain;       // Maximum possible gain value
                float           fLookBack;      // Look-back time
                float           fMaxLookBack;   // Maximum look-back time

            protected:
                void            set_timing(float *ptr, float value);
                void            calc_compressor();
                inline float    process_sample(float sl, float ss, float le);
                inline float    eval_curve(float x);
                inline float    eval_gain(float x);

            public:
                explicit AutoGain();
                AutoGain(const AutoGain &) = delete;
                AutoGain(AutoGain &&) = delete;
                ~AutoGain();

                AutoGain & operator = (const AutoGain &) = delete;
                AutoGain & operator = (AutoGain &&) = delete;

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
                 * @param max_lookback maximum look-back time
                 * @return status of operation
                 */
                status_t        init(float max_lookback);

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
                 * Set silence threshold, signal below the threshold is treated as silence and no
                 * gain adjustment is performed
                 * @param threshold silence threshold
                 */
                void            set_silence_threshold(float threshold);

                /**
                 * Get current gain value
                 * @return current gain value
                 */
                inline float    silence_threshold() const       { return fSilence;      }

                /**
                 * Set the possible signal deviation for switching from long-time reactivity to
                 * short-time reactivity
                 * @param deviation deviation multiplier
                 */
                void            set_deviation(float deviation);

                /**
                 * Get the deviation multiplier
                 * @return deviatio multiplier
                 */
                inline float    deviation() const               { return fDeviation;    }

                /**
                 * Set the minimum gain value in the output gain control signal
                 * @param value minimum gain value
                 */
                void            set_min_gain(float value);

                /**
                 * Get the minimum gain value in the output gain control signal
                 * @return minimum gain value in the output gain control signal
                 */
                inline float    min_gain() const                { return fMinGain;      }

                /**
                 * Set the maximum gain value in the output gain control signal
                 * @param value maximum gain value
                 */
                void            set_max_gain(float value);

                /**
                 * Get the maximum gain value in the output gain control signal
                 * @return maximum gain value in the output gain control signal
                 */
                inline float    max_gain() const                { return fMaxGain;      }

                /**
                 * Set the minimum and maximum possible value of the gain control signal
                 * @param min minimum value
                 * @param max maximum value
                 */
                void            set_gain(float min, float max);

                /**
                 * Set the short gain grow speed
                 * @param value short gain grow speed in dB/s
                 */
                inline void     set_short_grow(float value)     { set_timing(&sShort.fGrow, value);     }

                /**
                 * Get the short gain grow speed
                 * @return short gain grow speed in dB/s
                 */
                inline float    short_grow() const              { return sShort.fGrow;                  }

                /**
                 * Set the short gain fall-off speed
                 * @param value short gain fall-off speed in dB/s
                 */
                inline void     set_short_fall(float value)     { set_timing(&sShort.fFall, value);     }

                /**
                 * Get the short gain fall-off speed
                 * @return short gain fall-off speed in dB/s
                 */
                inline float    short_fall() const              { return sShort.fFall;                  }

                /**
                 * Set reaction speed for short-time loudness signal
                 * @param grow the gain grow speed in dB/s
                 * @param fall the gain fall-off speed in dB/s
                 */
                void            set_short_speed(float grow, float fall);

                /**
                 * Set the long gain grow speed
                 * @param value long gain grow speed in dB/s
                 */
                inline void     set_long_grow(float value)      { set_timing(&sLong.fGrow, value);      }

                /**
                 * Get the long gain grow speed
                 * @return long gain grow speed in dB/s
                 */
                inline float    long_grow() const               { return sLong.fGrow;                   }

                /**
                 * Set the long gain fall-off speed
                 * @param value long gain fall-off speed in dB/s
                 */
                inline void     set_long_fall(float value)      { set_timing(&sLong.fFall, value);      }

                /**
                 * Get the long gain fall-off speed
                 * @return long gain fall-off speed in dB/s
                 */
                inline float    long_fall() const               { return sLong.fFall;                   }

                /**
                 * Set reaction speed for long-time loudness signal
                 * @param grow the gain grow speed in dB/s
                 * @param fall the gain fall-off speed in dB/s
                 */
                void            set_long_speed(float grow, float fall);

                void            set_lookback(float value);

                inline float    get_lookback() const            { return fLookBack;     }

                size_t          latency() const;

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
                 * Process signal from channels and form the gain control signal
                 * @param vca destination buffer to store the gain adjustment
                 * @param llong the long-period loudness estimation
                 * @param lshort the short-period loudness estimation
                 * @param lexp the expected (desired) loudness level
                 * @param count number of samples to process
                 */
                void            process(float *vca, const float *llong, const float *lshort, const float *lexp, size_t count);

                /**
                 * Process signal from channels and form the gain control signal
                 * @param vca destination buffer to store the gain adjustment
                 * @param llong the long-period loudness estimation
                 * @param lshort the short-period loudness estimation
                 * @param lexp the expected (desired) loudness level
                 * @param count number of samples to process
                 */
                void            process(float *vca, const float *llong, const float *lshort, float lexp, size_t count);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_DYNAMICS_AUTOGAIN_H_ */
