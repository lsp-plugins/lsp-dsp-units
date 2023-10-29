/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 7 нояб. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_DYNAMICS_GATE_H_
#define LSP_PLUG_IN_DSP_UNITS_DYNAMICS_GATE_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC Gate
        {
            protected:
                typedef struct curve_t
                {
                    float               fThreshold;
                    float               fZone;
                    dsp::gate_knee_t    sKnee;
                } curve_t;

            protected:
                // Parameters
                curve_t     sCurves[2];
                float       fAttack;
                float       fRelease;
                float       fTauAttack;
                float       fTauRelease;
                float       fReduction;
                float       fEnvelope;

                // Additional parameters
                size_t      nSampleRate;
                size_t      nCurve;
                bool        bUpdate;

            public:
                explicit Gate();
                Gate(const Gate &) = delete;
                Gate(Gate &&) = delete;
                ~Gate();

                Gate & operator = (const Gate &) = delete;
                Gate & operator = (Gate &&) = delete;

                /**
                 * Construct the object
                 */
                void        construct();

                /**
                 * Destroy the object
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

                /** Update gate settings
                 *
                 */
                void update_settings();

                /** Set threshold
                 *
                 * @param topen open curve threshold
                 * @param tclose close curve threshold
                 *
                 */
                inline void set_threshold(float topen, float tclose)
                {
                    if ((topen == sCurves[0].fThreshold) && (tclose == sCurves[1].fThreshold))
                        return;
                    sCurves[0].fThreshold   = topen;
                    sCurves[1].fThreshold   = tclose;
                    bUpdate                 = true;
                }

                /** Set reduction
                 *
                 * @param reduction the reduction threshold
                 */
                inline void set_reduction(float reduction)
                {
                    if (reduction == fReduction)
                        return;
                    fReduction          = reduction;
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

                /** Set reduction zone
                 *
                 * @param open open curve ratio
                 * @param close close curve ratio
                 */
                inline void set_zone(float open, float close)
                {
                    if ((open == sCurves[0].fZone) && (close == sCurves[1].fZone))
                        return;
                    sCurves[0].fZone    = open;
                    sCurves[1].fZone    = close;
                    bUpdate             = true;
                }

                /** Process sidechain signal
                 *
                 * @param out output signal gain to VCA
                 * @param env envelope signal of gate
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

                /** Get curve
                 *
                 * @param out output expansion value
                 * @param in input expansion value
                 * @param dots number of input dots
                 * @param hyst output hysteresis curve or direct curve
                 */
                void curve(float *out, const float *in, size_t dots, bool hyst) const;

                /** Get curve point
                 *
                 * @param in input level
                 * @param hyst output hysteresis curve or direct curve
                 */
                float curve(float in, bool hyst) const;

                /** Get gain amplification
                 *
                 * @param out output signal
                 * @param in input signal
                 * @param dots number of dots
                 * @param hyst output hysteresis amplification or direct amplification
                 */
                void amplification(float *out, const float *in, size_t dots, bool hyst) const;

                /** Get gain amplification at current state
                 *
                 * @param in input level
                 */
                float amplification(float in) const;

                /** Get gain amplification
                 *
                 * @param in input level
                 * @param hyst output hysteresis amplification or direct amplification
                 */
                float amplification(float in, bool hyst) const;

                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_DYNAMICS_GATE_H_ */
