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

#ifndef LSP_PLUG_IN_DSP_UNITS_SAMPLING_TYPES_H_
#define LSP_PLUG_IN_DSP_UNITS_SAMPLING_TYPES_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

#define AUDIO_SAMPLE_CONTENT_TYPE       "application/x-lsp-audio-sample"

namespace lsp
{
    namespace dspu
    {
    #pragma pack(push, 1)
        typedef struct sample_header_t
        {
            uint16_t    version;        // Version + endianess
            uint16_t    channels;
            uint32_t    sample_rate;
            uint32_t    samples;
        } sample_header_t;
    #pragma pack(pop)

        enum sample_normalize_t
        {
            /**
             * No normalization
             */
            SAMPLE_NORM_NONE,

            /**
             * Normalize if maximum peak is above threshold
             */
            SAMPLE_NORM_ABOVE,

            /**
             * Normalize if maximum peak is below threshold
             */
            SAMPLE_NORM_BELOW,

            /**
             * Normalize in any case
             */
            SAMPLE_NORM_ALWAYS
        };

        enum sample_crossfade_t
        {
            /**
             * Linear crossfade
             */
            SAMPLE_CROSSFADE_LINEAR,

            /**
             * Constant-power crossfade
             */
            SAMPLE_CROSSFADE_CONST_POWER
        };

    } /* namesapace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_TYPES_H_ */
