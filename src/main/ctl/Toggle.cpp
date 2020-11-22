/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 9 июл. 2020 г.
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

#include <lsp-plug.in/dsp-units/ctl/Toggle.h>

namespace lsp
{
    namespace dspu
    {
        Toggle::Toggle()
        {
            construct();
        }

        Toggle::~Toggle()
        {
            fValue          = 0.0f;
            nState          = TRG_OFF;
        }

        void Toggle::construct()
        {
            fValue          = 0.0f;
            nState          = TRG_OFF;
        }

        void Toggle::init()
        {
            fValue          = 0.0f;
            nState          = TRG_OFF;
        }

        void Toggle::submit(float value)
        {
            if (value >= 0.5f)
            {
                if (nState == TRG_OFF)
                    nState      = TRG_PENDING;
            }
            else
            {
                if (nState == TRG_ON)
                    nState      = TRG_OFF;
            }
            fValue      = value;
        }

        void Toggle::commit(bool off)
        {
            if (nState != TRG_PENDING)
                return;
            if (off)
            {
                if (fValue < 0.5f)
                    nState      = TRG_OFF;
            }
            else
                nState      = (fValue >= 0.5f) ? TRG_ON : TRG_OFF;
        }

        void Toggle::dump(IStateDumper *v) const
        {
            v->write("fValue", fValue);
            v->write("nState", nState);
        }
    }
}


