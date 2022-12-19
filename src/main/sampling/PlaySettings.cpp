/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-sampler
 * Created on: 18 нояб. 2022 г.
 *
 * lsp-plugins-sampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-sampler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-sampler. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/dsp-units/sampling/PlaySettings.h>

namespace lsp
{
    namespace dspu
    {
        const PlaySettings PlaySettings::default_settings;

        PlaySettings::PlaySettings()
        {
            construct();
        }

        PlaySettings::~PlaySettings()
        {
            destroy();
        }

        void PlaySettings::construct()
        {
            nID             = 0;
            nChannel        = 0;
            fVolume         = 1.0f;
            bReverse        = false;
            nDelay          = 0;
            nStart          = 0;
            nLoopMode       = SAMPLE_LOOP_NONE;
            nLoopStart      = 0;
            nLoopEnd        = 0;
            nLoopXFadeType  = SAMPLE_CROSSFADE_CONST_POWER;
            nLoopXFadeLength= 0;
        }

        void PlaySettings::destroy()
        {
        }
    } /* namespace dspu */
} /* namespace lsp */


