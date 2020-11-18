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

#include <lsp-plug.in/dsp-units/ctl/Blink.h>

namespace lsp
{
    namespace dspu
    {
        Blink::Blink()
        {
            construct();
        }

        Blink::~Blink()
        {
            nCounter        = 0.0f;
            nTime           = 0.0f;
        }

        void Blink::construct()
        {
            nCounter        = 0.0f;
            nTime           = 0.0f;
            fOnValue        = 1.0f;
            fOffValue       = 0.0f;
            fTime           = 0.1f;
        }

        void Blink::dump(IStateDumper *v) const
        {
            v->write("nCounter", nCounter);
            v->write("nTime", nTime);
            v->write("fOnValue", fOnValue);
            v->write("fOffValue", fOffValue);
            v->write("fTime", fTime);
        }
    }
}


