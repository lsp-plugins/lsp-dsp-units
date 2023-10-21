/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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
            SCS_MIDDLE,         // Take the middle part out of the stereo signal
            SCS_SIDE,           // Take the side part out of the stereo signal
            SCS_LEFT,           // Take the left channel out of the stereo signal
            SCS_RIGHT,          // Take the right channel out of the stereo signal
            SCS_AMIN,           // Take the absolute minimum value out of the stereo signal
            SCS_AMAX            // Take the absolute maximum value out of the stereo signal
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
            protected:
                enum flags_t
                {
                    SCF_MIDSIDE     = 1 << 0,
                    SCF_UPDATE      = 1 << 1,
                    SCF_CLEAR       = 1 << 2
                };

            protected:
                ShiftBuffer     sBuffer;                // Shift buffer for history
                size_t          nReactivity;            // Reactivity (in samples)
                size_t          nSampleRate;            // Sample rate
                Equalizer      *pPreEq;                 // Pre-equalizer
                float           fReactivity;            // Reactivity (in time)
                float           fTau;                   // Tau for RMS
                float           fRmsValue;              // RMS value
                float           fMaxReactivity;         // Maximum reactivity
                float           fGain;                  // Sidechain gain
                uint32_t        nRefresh;               // Sidechain refresh
                uint8_t         nSource;                // Sidechain source
                uint8_t         nMode;                  // Sidechain mode
                uint8_t         nChannels;              // Number of channels
                uint8_t         nFlags;                 // Different sidechain flags

            protected:
                void            update_settings();
                void            refresh_processing();
                bool            preprocess(float *out, const float **in, size_t samples);
                bool            preprocess(float *out, const float *in);
                void            select_buffer(float **a, float **b, size_t *size);

            public:
                explicit Sidechain();
                Sidechain(const Sidechain &) = delete;
                Sidechain(Sidechain &&) = delete;
                ~Sidechain();

                Sidechain & operator = (const Sidechain &) = delete;
                Sidechain & operator = (Sidechain &&) = delete;

                /**
                 * Construct the object
                 */
                void            construct();

                /** Initialize sidechain
                 *
                 * @param channels number of input channels, possible 1 or 2
                 * @param max_reactivity maximum reactivity
                 */
                bool            init(size_t channels, float max_reactivity);

                /** Destroy sidechain
                 *
                 */
                void            destroy();

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
                void set_reactivity(float reactivity);

                /**
                 * Set stereo mode
                 * @param mode stereo mode to set
                 */
                void set_stereo_mode(sidechain_stereo_mode_t mode);

                /** Set sidechain source
                 *
                 * @param source sidechain source
                 */
                inline void set_source(size_t source)
                {
                    nSource         = source;
                }

                /**
                 * Mark the sidechain state for clear
                 */
                void clear();

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
                 * @param in array of input buffers
                 * @param samples number of samples to process
                 */
                void process(float *out, const float **in, size_t samples);

                /** Process sidechain signal (single sample)
                 *
                 * @param in input data array (one sample per each input channel)
                 */
                float process(const float *in);
    
                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_SIDECHAIN_H_ */
