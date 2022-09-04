/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 14 сент. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_SIDECHAIN_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_SIDECHAIN_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/util/ShiftBuffer.h>
#include <lsp-plug.in/dsp-units/filters/Equalizer.h>

namespace lsp
{
    namespace dspu
    {
        // Sidechain signal source
        enum sidechain_source_t
        {
            SCS_MIDDLE,
            SCS_SIDE,
            SCS_LEFT,
            SCS_RIGHT
        };

        enum sidechain_mode_t
        {
            SCM_PEAK,
            SCM_RMS,
            SCM_LPF,
            SCM_UNIFORM
        };

        enum sidechain_stereo_mode_t
        {
            SCSM_STEREO,
            SCSM_MIDSIDE
        };

        class LSP_DSP_UNITS_PUBLIC Sidechain
        {
            private:
                Sidechain & operator = (const Sidechain &);
                Sidechain(const Sidechain &);

            protected:
                ShiftBuffer     sBuffer;                // Shift buffer for history
                size_t          nReactivity;            // Reactivity (in samples)
                float           fReactivity;            // Reactivity (in time)
                float           fTau;                   // Tau for RMS
                float           fRmsValue;              // RMS value
                size_t          nSource;                // Sidechain source
                size_t          nMode;                  // Sidechain mode
                size_t          nSampleRate;            // Sample rate
                size_t          nRefresh;               // Sidechain refresh
                size_t          nChannels;              // Number of channels
                float           fMaxReactivity;         // Maximum reactivity
                float           fGain;                  // Sidechain gain
                bool            bUpdate;                // Update sidechain parameters flag
                bool            bMidSide;               // Mid-side mode
                Equalizer      *pPreEq;                 // Pre-equalizer

            protected:
                void            update_settings();
                void            refresh_processing();
                bool            preprocess(float *out, const float **in, size_t samples);
                bool            preprocess(float *out, const float *in);

            public:
                explicit Sidechain();
                ~Sidechain();

                /**
                 * Construct the object
                 */
                void            construct();

                /** Initialize sidechain
                 *
                 * @param channels number of input channels, possible 1 or 2
                 * @param max_reactivity maximum reactivity
                 */
                bool init(size_t channels, float max_reactivity);

                /** Destroy sidechain
                 *
                 */
                void destroy();

            public:
                /** Set pre-processing equalizer
                 *
                 * @param eq equalizer
                 */
                inline void set_pre_equalizer(Equalizer *eq)            { pPreEq = eq; }

                /** Set sample rate
                 *
                 * @param sr sample rate
                 */
                void set_sample_rate(size_t sr);

                /** Set sidechain reactivity
                 *
                 * @param reactivity sidechain reactivity
                 */
                inline void set_reactivity(float reactivity)
                {
                    if ((fReactivity == reactivity) ||
                        (reactivity <= 0.0) ||
                        (reactivity >= fMaxReactivity))
                        return;
                    fReactivity     = reactivity;
                    bUpdate         = true;
                }

                inline void set_stereo_mode(sidechain_stereo_mode_t mode)
                {
                    bMidSide        = mode == SCSM_MIDSIDE;
                }

                /** Set sidechain source
                 *
                 * @param source sidechain source
                 */
                inline void set_source(size_t source)
                {
                    nSource         = source;
                }

                /** Set sidechain mode
                 *
                 * @param mode sidechain mode
                 */
                inline void set_mode(size_t mode)
                {
                    if (nMode == mode)
                        return;
                    fRmsValue       = 0.0f;
                    nMode           = mode;
                }

                /** Set-up pre-amplification gain
                 *
                 * @param gain sidechain pre-amplification gain
                 */
                inline void set_gain(float gain)
                {
                    fGain           = gain;
                }

                /** Get pre-amplification gain
                 *
                 * @return pre-amplification gain
                 */
                inline float get_gain() const
                {
                    return fGain;
                }

                /** Process sidechain signal
                 *
                 * @param out output buffer
                 * @param in input buffers
                 * @param samples number of samples to process
                 */
                void process(float *out, const float **in, size_t samples);

                /** Process sidechain signal (single sample)
                 *
                 * @param in input data (one sample per channel)
                 */
                float process(const float *in);
    
                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }

} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_SIDECHAIN_H_ */
