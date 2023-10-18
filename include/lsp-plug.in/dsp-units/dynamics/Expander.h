/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 2 нояб. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_DYNAMICS_EXPANDER_H_
#define LSP_PLUG_IN_DSP_UNITS_DYNAMICS_EXPANDER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        enum expander_mode_t
        {
            EM_DOWNWARD,
            EM_UPWARD
        };

        class LSP_DSP_UNITS_PUBLIC Expander
        {
            protected:
                // Basic parameters
                float       fAttackThresh;
                float       fReleaseThresh;
                float       fAttack;
                float       fRelease;
                float       fKnee;
                float       fRatio;
                float       fEnvelope;

                // Pre-calculated parameters
                float       fTauAttack;
                float       fTauRelease;
                float       vHermite[3];    // Knee hermite interpolation
                float       fLogKS;         // Knee start
                float       fLogKE;         // Knee end
                float       fLogTH;         // Logarithmic threshold

                // Additional parameters
                size_t      nSampleRate;
                bool        bUpdate;
                bool        bUpward;

            public:
                explicit Expander();
                Expander(const Expander &) = delete;
                Expander(Expander &&) = delete;
                ~Expander();

                Expander & operator = (const Expander &) = delete;
                Expander & operator = (Expander &&) = delete;

                /**
                 * Construct object
                 */
                void        construct();

                /**
                 * Destroy object
                 */
                void        destroy();

            public:
                /** Check that some of parameters have been modified
                 * and we need to call update_settings();
                 *
                 * @return true if some of parameters have been modified
                 */
                inline bool modified() const
                {
                    return bUpdate;
                }

                inline bool is_upward() const
                {
                    return bUpward;
                }

                inline bool is_downward() const
                {
                    return !bUpward;
                }

                /** Update expander settings
                 *
                 */
                void update_settings();

                /** Set threshold
                 *
                 * @param attack the attack threshold
                 * @param release the release threshold (relative to attack, must be positive, less or equal to 1.0)
                 */
                inline void set_threshold(float attack, float release)
                {
                    if ((fAttackThresh == attack) && (fReleaseThresh == release))
                        return;
                    fAttackThresh       = attack;
                    fReleaseThresh      = release;
                    bUpdate             = true;
                }

                /** Set timings
                 *
                 * @param attack attack time (ms)
                 * @param release release time (ms)
                 */
                inline void set_timings(float attack, float release)
                {
                    if ((fAttack == attack) && (fRelease == release))
                        return;
                    fAttack     = attack;
                    fRelease    = release;
                    bUpdate     = true;
                }

                /** Set attack time
                 *
                 * @param attack attack time (ms)
                 */
                inline void set_attack(float attack)
                {
                    if (fAttack == attack)
                        return;
                    fAttack     = attack;
                    bUpdate     = true;
                }

                /** Set release time
                 *
                 * @param release release time (ms)
                 */
                inline void set_release(float release)
                {
                    if (fRelease == release)
                        return;
                    fRelease    = release;
                    bUpdate     = true;
                }

                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                inline void set_sample_rate(size_t sr)
                {
                    if (sr == nSampleRate)
                        return;
                    nSampleRate = sr;
                    bUpdate     = true;
                }

                /** Set knee
                 *
                 * @param knee (in gain units)
                 */
                inline void set_knee(float knee)
                {
                    if (knee == fKnee)
                        return;
                    fKnee       = knee;
                    bUpdate     = true;
                }

                /** Set ratio
                 *
                 * @param ratio expansion ratio
                 */
                inline void set_ratio(float ratio)
                {
                    if (ratio == fRatio)
                        return;
                    bUpdate     = true;
                    fRatio      = ratio;
                }

                /** Set expander mode: upward/downward
                 *
                 * @param mode expander mode
                 */
                inline void set_mode(size_t mode)
                {
                    bool upward = (mode == EM_UPWARD);
                    if (upward == bUpward)
                        return;

                    bUpward     = upward;
                    bUpdate     = true;
                }

                /** Process sidechain signal
                 *
                 * @param out output signal gain to VCA
                 * @param env envelope signal of expander
                 * @param in sidechain signal
                 * @param samples number of samples to process
                 */
                void process(float *out, float *env, const float *in, size_t samples);

                /** Process one sample of sidechain signal
                 *
                 * @param s sidechain signal
                 * @param out envelope signal of expander, may be NULL
                 * @return output signal gain to VCA
                 */
                float process(float *env, float s);

                /** Get expansion curve
                 *
                 * @param out output expansion value
                 * @param in input expansion value
                 * @param dots number of input dots
                 */
                void curve(float *out, const float *in, size_t dots);

                /** Get expansion curve point
                 *
                 * @param in input level
                 */
                float curve(float in);

                /** Get gain amplification
                 *
                 * @param out output signal
                 * @param in input signal
                 * @param dots number of dots
                 */
                void amplification(float *out, const float *in, size_t dots);

                /** Get gain amplification
                 *
                 * @param in input level
                 */
                float amplification(float in);
    
                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_DYNAMICS_EXPANDER_H_ */
