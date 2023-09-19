/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 сент. 2023 г.
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

#include <lsp-plug.in/dsp-units/misc/broadcast.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        namespace bs
        {
            LSP_DSP_UNITS_PUBLIC
            float channel_weighting(channel_t designation)
            {
                switch (designation)
                {
                    case CHANNEL_FRONT_LEFT: // BS.2051-3 M+060
                    case CHANNEL_FRONT_RIGHT: // BS.2051-3 M-060
                    case CHANNEL_LEFT_SIDE: // BS.2051-3 M+090
                    case CHANNEL_RIGHT_SIDE: // BS.2051-3 M-090
                    case CHANNEL_LEFT_SURROUND: // BS.2051-3 M+110
                    case CHANNEL_RIGHT_SURROUND: // BS.2051-3 M-110
                        // +1.5 dB
                        return 1.41253754462f;

                    case CHANNEL_LFE1:
                    case CHANNEL_LFE2:
                        // BS.1770-4: The low frequency effects (LFE) channel is not included in the measurement
                        return 0.0f;

                    default:
                        break;
                }

                return 1.0f;
            }

        } /* namespace bs */
    } /* namespace dspu */
} /* namespace lsp */



