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
                    float       t;
                    float       a, b, c, d;
                } compressor_t;

                enum flags_t
                {
                    F_UPDATE        = 1 << 0,
                    F_QUICK_AMP = 1 << 1,
                    F_SURGE_UP      = 1 << 2,
                    F_SURGE_DOWN    = 1 << 3
                };

            protected:
                size_t          nSampleRate;    // Current sample rate
                size_t          nFlags;         // Different flags

                timing_t        sShort;         // Stort timings
                timing_t        sLong;          // Long timings
                compressor_t    sShortComp;     // Compressor settings
                compressor_t    sOutComp;       // Output compressor
                float           fSilence;       // Silence threshold
                float           fDeviation;     // Level deviation
                float           fCurrGain;      // Current gain value

            protected:
                static void             init_compressor(compressor_t &c);
                static void             calc_compressor(compressor_t &c, float x1, float x2, float y2);
                static inline float     eval_curve(const compressor_t &c, float x);
                static inline float     eval_gain(const compressor_t &c, float x);

                static void             dump(const char *id, const timing_t *t, IStateDumper *v);
                static void             dump(const char *id, const compressor_t *c, IStateDumper *v);

            protected:
                void                    set_timing(float *ptr, float value);
                inline float            process_sample(float sl, float ss, float le);

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

                /**
                 * Enable/disable quick gain restoration for quick level surge
                 * @param enable enable/disable flag
                 */
                void            enable_quick_amplifier(bool enable);

                /**
                 * Check that guick gain restoration is enabled
                 * @return true if quick gain restoation is enabled
                 */
                inline bool     quick_amplifier() const         { return nFlags & F_QUICK_AMP; }

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
