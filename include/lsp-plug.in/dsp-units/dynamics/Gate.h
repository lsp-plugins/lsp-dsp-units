/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                float       fHold;
                float       fPeak;

                // Additional parameters
                uint32_t    nHold;
                uint32_t    nHoldCounter;
                uint32_t    nSampleRate;
                uint8_t     nCurve;
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
                void set_threshold(float topen, float tclose);

                /**
                 * Get open curve threshold
                 * @return open curve threshold
                 */
                inline float open_threshold() const                 { return sCurves[0].fThreshold; }

                /**
                 * Set open curve threshold
                 * @param threshold open curve threshold
                 */
                void set_open_threshold(float threshold);

                /**
                 * Get close curve threshold
                 * @return close curve threshold
                 */
                inline float close_threshold() const                { return sCurves[1].fThreshold; }

                /**
                 * Set close curve threshold
                 * @param threshold close curve threshold
                 */
                void set_close_threshold(float threshold);

                /** Set reduction
                 *
                 * @param reduction the reduction threshold
                 */
                void set_reduction(float reduction);

                /**
                 * Get reduction
                 * @return gain reduction threshold
                 */
                inline float reduction() const                      { return fReduction; }

                /** Set timings
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

                /**
                 * Get attack time
                 * @return attack time (ms)
                 */
                inline float attack() const                         { return fAttack; }

                /** Set release time
                 *
                 * @param release release time (ms)
                 */
                void set_release(float release);

                /**
                 * Get release time
                 * @return release time (ms)
                 */
                inline float release() const                        { return fRelease; }

                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                void set_sample_rate(size_t sr);

                /**
                 * Get sample rate
                 * @return sample rate
                 */
                inline size_t sample_rate() const                   { return nSampleRate; }

                /** Set transition zone
                 *
                 * @param open open curve transition zone
                 * @param close close curve transition zone
                 */
                void set_zone(float open, float close);

                /**
                 * Set open curve transition zone
                 * @param zone open curve transition zone
                 */
                void set_open_zone(float zone);

                /**
                 * Get open curve transition zone
                 * @return open curve transition zone
                 */
                inline float open_zone() const                      { return sCurves[0].fZone; }

                /**
                 * Set close curve transition zone
                 * @param zone close curve transition zone
                 */
                void set_close_zone(float zone);

                /**
                 * Get close curve transition zone
                 * @return close curve transition zone
                 */
                inline float close_zone() const                     { return sCurves[1].fZone; }

                /**
                 * Set hold time in milliseconds
                 * @param hold hold time in milliseconds
                 */
                void set_hold(float hold);

                /**
                 * Get the hold time
                 * @return hold time
                 */
                float hold() const                                  { return fHold; }

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
                 * @param env pointer to store sample of envelope signal of expander, may be NULL
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
