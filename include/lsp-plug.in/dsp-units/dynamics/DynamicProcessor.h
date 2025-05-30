/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 окт. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_DYNAMICS_DYNAMICPROCESSOR_H_
#define LSP_PLUG_IN_DSP_UNITS_DYNAMICS_DYNAMICPROCESSOR_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

#define DYNAMIC_PROCESSOR_DOTS      4
#define DYNAMIC_PROCESSOR_RANGES    (DYNAMIC_PROCESSOR_DOTS + 1)

namespace lsp
{
    namespace dspu
    {
        typedef struct dyndot_t
        {
            float   fInput;         // Negative value means off
            float   fOutput;        // Negative value means off
            float   fKnee;          // Negative value means off
        } dyndot_t;

        class LSP_DSP_UNITS_PUBLIC DynamicProcessor
        {
            private:
                DynamicProcessor & operator = (const DynamicProcessor &);
                DynamicProcessor(const DynamicProcessor &);

            protected:
                typedef struct spline_t
                {
                    float       fPreRatio;      // Pre-knee ratio
                    float       fPostRatio;     // Post-knee ratio
                    float       fKneeStart;     // Start knee threshold
                    float       fKneeStop;      // Stop knee threshold
                    float       fThresh;        // Logarithmic threshold
                    float       fMakeup;        // Makeup gain of the knee
                    float       vHermite[4];    // Hermite interpolation
                } spline_t;

                typedef struct reaction_t
                {
                    float       fLevel;
                    float       fTau;
                } reaction_t;

                enum counters_t
                {
                    CT_SPLINES,
                    CT_ATTACK,
                    CT_RELEASE,

                    CT_TOTAL
                };

            protected:
                // Input parameters
                dyndot_t    vDots[DYNAMIC_PROCESSOR_DOTS];
                float       vAttackLvl[DYNAMIC_PROCESSOR_DOTS];
                float       vReleaseLvl[DYNAMIC_PROCESSOR_DOTS];
                float       vAttackTime[DYNAMIC_PROCESSOR_RANGES];
                float       vReleaseTime[DYNAMIC_PROCESSOR_RANGES];
                float       fInRatio;   // Input ratio
                float       fOutRatio;  // Output ratio

                // Processing parameters
                spline_t    vSplines[DYNAMIC_PROCESSOR_DOTS];
                reaction_t  vAttack[DYNAMIC_PROCESSOR_RANGES];
                reaction_t  vRelease[DYNAMIC_PROCESSOR_RANGES];
                uint8_t     fCount[CT_TOTAL];  // Number of elements for AttackLvl, ReleaseLvl, ... etc

                // Dynamic patameters
                float       fEnvelope;
                float       fHold;
                float       fPeak;

                // Additional parameters
                uint32_t    nHold;
                uint32_t    nHoldCounter;
                uint32_t    nSampleRate;
                bool        bUpdate;

            protected:
                static inline float     spline_amp(const spline_t *s, float x);
                static inline float     spline_model(const spline_t *s, float x);
                void                    sort_reactions(reaction_t *s, size_t count);
                void                    sort_splines(spline_t *s, size_t count);
                static inline float     solve_reaction(const reaction_t *s, float x, size_t count);

            public:
                explicit DynamicProcessor();
                ~DynamicProcessor();

                /**
                 * Construct object
                 */
                void    construct();

                /**
                 * Destroyp object
                 */
                void    destroy();

            public:
                /** Check that some of processor's parameters have been modified
                 * and we need to call update_settings();
                 *
                 * @return true if some of processor's parameters have been modified
                 */
                inline bool modified() const
                {
                    return bUpdate;
                }

                /** Update processor's settings
                 *
                 */
                void update_settings();

                /**
                 * Get sample rate
                 * @return sample rate
                 */
                inline size_t sample_rate() const           { return nSampleRate; }

                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                void set_sample_rate(size_t sr);

                /** Get input ratio
                 *
                 * @return input ratio
                 */
                inline float in_ratio() const               { return fInRatio; }

                /** Set input ratio
                 *
                 * @param ratio input ratio
                 */
                void set_in_ratio(float ratio);

                /** Get output ratio
                 *
                 * @return output ratio
                 */
                inline float out_ratio() const              { return fOutRatio; }

                /** Set output ratio
                 *
                 * @param ratio output ratio
                 */
                void set_out_ratio(float ratio);

                /** Get dot configuration
                 *
                 * @param id identifier of dot
                 * @param dst pointer to store data
                 * @return status of operation
                 */
                bool get_dot(size_t id, dyndot_t *dst) const;

                /** Set dot configuration
                 *
                 * @param id identifier of dot
                 * @param src new configuration pointer or NULL
                 * @return status of operation
                 */
                bool set_dot(size_t id, const dyndot_t *src);

                /** Set dot configuration
                 *
                 * @param id identifier of dot
                 * @param in input level
                 * @param out output level
                 * @param knee knee size
                 * @return status of operation
                 */
                bool set_dot(size_t id, float in, float out, float knee);

                /** Get attack level
                 *
                 * @param id split level
                 * @return attack level
                 */
                float attack_level(size_t id) const;

                /** Set attack level
                 *
                 * @param id split level
                 * @param value level value
                 */
                void set_attack_level(size_t id, float value);

                /** Get release level
                 *
                 * @param id split level
                 * @return attack level
                 */
                float release_level(size_t id) const;

                /** Set release level
                 *
                 * @param id split level
                 * @param value level value
                 */
                void set_release_level(size_t id, float value);

                /** Get attack time of the specified range
                 *
                 * @param id identifier of the range
                 * @return attack time
                 */
                float attack_time(size_t id) const;

                /** Set attack time
                 *
                 * @param id identifier of the range
                 * @param value attack time value
                 */
                void set_attack_time(size_t id, float value);

                /** Get release time of the specified range
                 *
                 * @param id identifier of the range
                 * @return release time
                 */
                float release_time(size_t id) const;

                /** Set release time
                 *
                 * @param id identifier of the range
                 * @param value attack time value
                 */
                void set_release_time(size_t id, float value);

                /**
                 * Get the hold time
                 * @return hold time
                 */
                float hold() const                          { return fHold; }

                /**
                 * Set hold time in milliseconds
                 * @param hold hold time in milliseconds
                 */
                void set_hold(float hold);

                /** Process sidechain signal
                 *
                 * @param out output signal gain to VCA
                 * @param env envelope signal of processor
                 * @param in sidechain signal
                 * @param samples number of samples to process
                 */
                void process(float *out, float *env, const float *in, size_t samples);

                /** Process one sample of sidechain signal
                 *
                 * @param s sidechain signal
                 * @param env pointer to store sample of envelope signal of processor, may be NULL
                 * @return output signal gain to VCA
                 */
                float process(float *env, float s);

                /** Get dynamic curve
                 *
                 * @param out output compression value
                 * @param in input compression value
                 * @param dots number of input dots
                 */
                void curve(float *out, const float *in, size_t dots);

                /** Get dynamic curve point
                 *
                 * @param in input level
                 */
                float curve(float in);

                /** Get dynamic curve model
                 *
                 * @param out output compression value
                 * @param in input compression value
                 * @param dots number of input dots
                 */
                void model(float *out, const float *in, size_t dots);

                /** Get dynamic curve point
                 *
                 * @param in input level
                 */
                float model(float in);

                /** Get dynamic gain reduction
                 *
                 * @param out output signal
                 * @param in input signal
                 * @param dots number of dots
                 */
                void reduction(float *out, const float *in, size_t dots);

                /** Get dynamic gain reduction
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

#endif /* LSP_PLUG_IN_DSP_UNITS_DYNAMICS_DYNAMICPROCESSOR_H_ */
