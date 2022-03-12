/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 08 апр. 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_CTL_BLINK_H_
#define LSP_PLUG_IN_DSP_UNITS_CTL_BLINK_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /** Simple blink counter
         *
         */
        class Blink
        {
            private:
                Blink & operator = (const Blink &);
                Blink(const Blink &);

            protected:
                ssize_t     nCounter;
                ssize_t     nTime;
                float       fOnValue;
                float       fOffValue;
                float       fTime;

            public:
                explicit Blink();
                ~Blink();

                /**
                 * Construct object
                 */
                void            construct();

                /**
                 * Destroy object
                 */
                void            destroy();

            public:
                /** Initialize blink
                 *
                 * @param sample_rate sample rate
                 * @param time activity time
                 */
                void            init(size_t sample_rate, float time = 0.1f);

                /** Update current sample rate
                 *
                 * @param sample_rate current sample rate
                 */
                void            set_sample_rate(size_t sample_rate);

                /** Make blinking
                 *
                 */
                void            blink();

                /** Make blinking
                 * @param value value to display
                 */
                void            blink(float value);

                /** Make blinking
                 *
                 * @param value value that will be displayed if less than max value
                 */
                void            blink_max(float value);

                /** Make blinking
                 *
                 * @param value value that will be displayed if less than max value
                 */
                void            blink_min(float value);

                /** Set default values
                 *
                 * @param on default value for on state
                 * @param off default value for off state
                 */
                void            set_default(float on, float off);

                /** Set default value for off state
                 *
                 * @param off default value for off state
                 */
                void            set_default_off(float off);

                /** Process blinking
                 *
                 * @return activity value
                 */
                float           process(size_t samples);

                /** Get current activity value of the blink
                 *
                 * @return current activity value of the blink
                 */
                inline float    value() const
                {
                    return (nCounter > 0) ? fOnValue : fOffValue;
                }

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_CTL_BLINK_H_ */
