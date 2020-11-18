/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        enum compressor_mode_t
        {
            CM_DOWNWARD,
            CM_UPWARD
        };

        /** Compressor class implementation
         *
         */
        class Compressor
        {
            private:
                Compressor & operator = (const Compressor &);

            protected:
                // Basic parameters
                float       fAttackThresh;
                float       fReleaseThresh;
                float       fBoostThresh;
                float       fAttack;
                float       fRelease;
                float       fKnee;
                float       fRatio;
                float       fEnvelope;

                // Pre-calculated parameters
                float       fTauAttack;
                float       fTauRelease;
                float       fXRatio;        // Compression ratio

                float       fLogTH;         // Logarithmic threshold
                float       fKS;            // Knee start
                float       fKE;            // Knee end
                float       vHermite[3];    // Knee hermite interpolation

                float       fBLogTH;        // Logarithmic boost threshold
                float       fBKS;           // Boost knee start
                float       fBKE;           // Boost knee end
                float       vBHermite[3];   // Boost hermite interpolation
                float       fBoost;         // Overall gain boost

                // Additional parameters
                size_t      nSampleRate;
                bool        bUpward;
                bool        bUpdate;

            public:
                explicit Compressor();
                ~Compressor();

                /**
                 * Construct object
                 */
                void        construct();

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

                /** Set compressor threshold
                 *
                 * @param attack the attack threshold
                 * @param release the release threshold (relative to attack, must be positive, less or equal to 1.0)
                 */
                void set_threshold(float attack, float release);

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

                /** Set attack time
                 *
                 * @param attack attack time (ms)
                 */
                void set_attack(float attack);

                /** Set release time
                 *
                 * @param release release time (ms)
                 */
                void set_release(float release);

                /** Set compressor's sample rate
                 *
                 * @param sr sample rate
                 */
                void set_sample_rate(size_t sr);

                /** Set compressor's knee
                 *
                 * @param knee (in gain units)
                 */
                void set_knee(float knee);

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
                 * @param in sidechain signal
                 * @param out envelope signal of compressor, may be NULL
                 * @return output signal gain to VCA
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
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_DYNAMICS_COMPRESSOR_H_ */
