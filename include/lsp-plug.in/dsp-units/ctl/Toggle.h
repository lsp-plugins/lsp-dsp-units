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

#ifndef LSP_PLUG_IN_DSP_UNITS_CTL_TOGGLE_H_
#define LSP_PLUG_IN_DSP_UNITS_CTL_TOGGLE_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace dspu
    {
        /** Simple toggle class
         *
         */
        class LSP_DSP_UNITS_PUBLIC Toggle
        {
            private:
                Toggle &operator = (const Toggle &);
                Toggle(const Toggle &);

            private:
                enum state_t
                {
                    TRG_OFF,
                    TRG_PENDING,
                    TRG_ON
                };

                float           fValue;
                state_t         nState;

            public:
                explicit Toggle();
                ~Toggle();

                /**
                 * Construct object
                 */
                void        construct();

                /**
                 * Destroy object
                 */
                void        destroy();


            public:
                /** Initialize toggle
                 *
                 */
                void        init();

                /** Submit toggle state
                 *
                 * @param value toggle value [0..1]
                 */
                void        submit(float value);

                /** Check that there is pending request for the toggle
                 *
                 * @return true if there is pending request for the toggle
                 */
                inline bool pending() const
                {
                    return nState == TRG_PENDING;
                }

                /** Check that toggle is in 'ON' state
                 *
                 * @return true toggle is in 'ON' state
                 */
                inline bool on() const
                {
                    return nState == TRG_ON;
                }

                /** Check that toggle is in 'OFF' state
                 *
                 * @return true toggle is in 'OFF' state
                 */
                inline bool off() const
                {
                    return nState == TRG_OFF;
                }

                /** Commit the pending request of the toggle
                 * @param off disable pending state only if toggle is in OFF state
                 * @return true if current state of the toggle is ON
                 */
                bool        commit(bool off = false);

                /**
                 * Dump internal state
                 * @param v state dumper
                 */
                void        dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_CTL_H_ */
