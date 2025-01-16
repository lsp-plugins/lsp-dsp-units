/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 16 сент. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_DYNAMICS_COMPRESSOR_H_
#define LSP_PLUG_IN_DSP_UNITS_DYNAMICS_COMPRESSOR_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        enum compressor_mode_t
        {
            CM_DOWNWARD,
            CM_UPWARD,
            CM_BOOSTING
        };

        /** Compressor class implementation
         *
         */
        class LSP_DSP_UNITS_PUBLIC Compressor
        {
            protected:
                typedef dsp::compressor_x2_t comp_t;

            protected:
                // Basic parameters
                float       fAttackThresh;
                float       fReleaseThresh;
                float       fBoostThresh;
                float       fAttack;
                float       fRelease;
                float       fKnee;
                float       fRatio;
                float       fHold;
                float       fEnvelope;
                float       fPeak;

                // Pre-calculated parameters
                float       fTauAttack;
                float       fTauRelease;
                comp_t      sComp;          // Two compressor knees

                // Additional parameters
                uint32_t    nHold;
                uint32_t    nHoldCounter;
                uint32_t    nSampleRate;
                uint32_t    nMode;
                bool        bUpdate;

            public:
                explicit Compressor();
                Compressor(const Compressor &) = delete;
                Compressor(Compressor &&) = delete;
                ~Compressor();

                Compressor & operator = (const Compressor &) = delete;
                Compressor & operator = (Compressor &&) = delete;

                /**
                 * Construct object
                 */
                void        construct();

                /**
                 * Destroy object
                 */
                void        destroy();

            public:
                /** Check that some of compressor's parameters have been modified
                 * and we need to call update_settings();
                 *
                 * @return true if some of compressor's parameters have been modified
                 */
                inline bool modified() const
                {
                    return bUpdate;
                }

                /** Update compressor's settings
                 *
                 */
                void update_settings();

                /**
                 * Get attack threshold of compressor
                 * @return attack threshold of compressor
                 */
                inline float attack_threshold() const       { return fAttackThresh; }

                /**
                 * Set attack threshold
                 * @param threshold attack threshold
                 */
                void set_attack_threshold(float threshold);

                /**
                 * Get release threshold of compressor
                 * @return release threshold of compressor
                 */
                inline float release_threshold() const      { return fReleaseThresh; }

                /**
                 * Set release threshold
                 * @param threshold release threshold
                 */
                void set_release_threshold(float threshold);

                /** Set compressor threshold
                 *
                 * @param attack the attack threshold
                 * @param release the release threshold (relative to attack, must be positive, less or equal to 1.0)
                 */
                void set_threshold(float attack, float release);

                /**
                 * Get boost threshold of compressor
                 * @return boost threshold of compressor
                 */
                inline float boost_threshold() const    { return fBoostThresh; }

                /**
                 * Set boost threshold, valid for upward compression only
                 * @param boost boost threshold
                 */
                void set_boost_threshold(float boost);

                /** Set compressor timings
                 *
                 * @param attack attack time (ms)
                 * @param release release time (ms)
                 */
                void set_timings(float attack, float release);

                /**
                 * Get attack time
                 * @return attack time (ms)
                 */
                inline float attack() const         { return fAttack;   }

                /** Set attack time
                 *
                 * @param attack attack time (ms)
                 */
                void set_attack(float attack);

                /**
                 * Get release time of compressor
                 * @return release time of compressor (ms)
                 */
                inline float release() const        { return fRelease;  }

                /** Set release time
                 *
                 * @param release release time (ms)
                 */
                void set_release(float release);

                /**
                 * Get sample rate
                 * @return sample rate
                 */
                inline size_t sample_rate() const   { return nSampleRate; }

                /** Set compressor's sample rate
                 *
                 * @param sr sample rate
                 */
                void set_sample_rate(size_t sr);

                /**
                 * Get compressor knee
                 * @return compressor knee
                 */
                inline float knee() const           { return fKnee; }

                /** Set compressor's knee
                 *
                 * @param knee (in gain units)
                 */
                void set_knee(float knee);

                /**
                 * Get compression ratio
                 * @return compression ratio
                 */
                inline float ratio() const          { return fRatio; }

                /** Set compression ratio
                 *
                 * @param ratio compression ratio
                 */
                void set_ratio(float ratio);

                /** Set compression mode: upward/downward
                 *
                 * @param mode compression mode
                 */
                void set_mode(size_t mode);

                inline size_t mode() const          { return nMode; }

                /**
                 * Get the hold time of compressor
                 * @return hold time of compressor
                 */
                float hold() const                  { return fHold; }

                /**
                 * Set hold time in milliseconds
                 * @param hold hold time in milliseconds
                 */
                void set_hold(float hold);

                /** Process sidechain signal
                 *
                 * @param out output signal gain to VCA
                 * @param env envelope signal of compressor
                 * @param in sidechain signal
                 * @param samples number of samples to process
                 */
                void process(float *out, float *env, const float *in, size_t samples);

                /** Process one sample of sidechain signal
                 *
                 * @param env pointer to store the compressor's envelope signal
                 * @param in sidechain signal sample
                 * @return output the VCA sample
                 */
                float process(float *env, float in);

                /** Get compression curve
                 *
                 * @param out output compression value
                 * @param in input compression value
                 * @param dots number of input dots
                 */
                void curve(float *out, const float *in, size_t dots);

                /** Get compression curve point
                 *
                 * @param in input level
                 */
                float curve(float in);

                /** Get compressor gain reduction
                 *
                 * @param out output signal
                 * @param in input signal
                 * @param dots number of dots
                 */
                void reduction(float *out, const float *in, size_t dots);

                /** Get compressor gain reduction
                 *
                 * @param in input level
                 */
                float reduction(float in);

                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_DYNAMICS_COMPRESSOR_H_ */
