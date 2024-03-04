/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 07 дек. 2015 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_CTL_BYPASS_H_
#define LSP_PLUG_IN_DSP_UNITS_CTL_BYPASS_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Bypass class, provides utilitary class for implementing mono bypass function
         */
        class LSP_DSP_UNITS_PUBLIC Bypass
        {
            private:
                enum state_t
                {
                    S_ON,
                    S_ACTIVE,
                    S_OFF
                };

                state_t nState;
                float   fDelta;
                float   fGain;

            public:
                explicit Bypass();
                Bypass(const Bypass &) = delete;
                Bypass(Bypass &&) = delete;
                ~Bypass();

                Bypass & operator = (const Bypass &) = delete;
                Bypass & operator = (Bypass &&) = delete;

                /**
                 * Construct object
                 */
                void        construct();

                /**
                 * Destroy object
                 */
                void        destroy();

            public:
                /**
                 * Initialize bypass
                 * @param sample_rate sample rate
                 * @param time the bypass switch time, by default 5 milliseconds
                 */
                void        init(int sample_rate, float time = 0.005f);

                /**
                 * Process the signal. If Bypass is on, then dry signal is passed to output.
                 * If bypass is off, then wet signal is passed.
                 * When bypass is in active state, the mix of dry and wet signal is passed to
                 * output.
                 *
                 * @param dst output buffer
                 * @param dry dry signal buffer
                 * @param wet wet signal buffer
                 * @param count number of samples to process
                 */
                void        process(float *dst, const float *dry, const float *wet, size_t count);

                /**
                 * Process the signal and apply gain to wet signal. If Bypass is on, then dry signal is passed to output.
                 * If bypass is off, then wet signal is passed.
                 * When bypass is in active state, the mix of dry and wet signal is passed to
                 * output.
                 *
                 * @param dst output buffer
                 * @param dry dry signal buffer
                 * @param wet wet signal buffer
                 * @param wet_gain additional gain to apply to wet signal
                 * @param count number of samples to process
                 */
                void        process_wet(float *dst, const float *dry, const float *wet, float wet_gain, size_t count);

                /**
                 * Enable/disable bypass
                 * @param bypass bypass value
                 * @return true if bypass state has changed
                 */
                bool        set_bypass(bool bypass);

                /** Enable/disable bypass
                 *
                 * @param bypass bypass value, when less than 0.5, bypass is considered to become shut down
                 * @return true if bypass state has changed
                 */
                inline bool set_bypass(float bypass) { return set_bypass(bypass >= 0.5f); };

                /**
                 * Return true if bypass is on (final state)
                 * @return true if bypass is on
                 */
                inline bool on() const      { return nState == S_ON; };

                /**
                 * Return true if bypass is off (final state)
                 * @return true if bypass is off
                 */
                inline bool off() const     { return nState == S_OFF; };

                /**
                 * Return true if bypass is active
                 * @return true if bypass is active
                 */
                inline bool active() const  { return nState == S_ACTIVE; };

                /**
                 * Return true if bypass is on or is currently going to become on
                 * @return true if bypass is on or is currently going to become on
                 */
                bool        bypassing() const;

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void        dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_CTL_BYPASS_H_ */
