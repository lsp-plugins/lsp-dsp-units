/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 24 нояб. 2022 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_SAMPLING_HELPERS_BATCH_H_
#define LSP_PLUG_IN_DSP_UNITS_SAMPLING_HELPERS_BATCH_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/sampling/types.h>

namespace lsp
{
    namespace dspu
    {
        namespace playback
        {
            /**
             * The type of the sample range
             */
            enum batch_type_t
            {
                BATCH_NONE,         // The batch is empty, nothing to do
                BATCH_HEAD,         // The batch is associated with the head part of the sample
                BATCH_LOOP,         // The batch is associated with the loop part of the sample
                BATCH_TAIL          // The batch is associated with the tail part of the sample
            };

            /**
             * Low-level data stucture that defines the range of the sample that should be played
             */
            typedef struct batch_t
            {
                wsize_t             nTimestamp;     // The start of the batch in the sample timeline
                size_t              nStart;         // Start of the sample segment to play
                size_t              nEnd;           // End of the sample segment to play
                size_t              nFadeIn;        // The fade-in time in samples
                size_t              nFadeOut;       // The fade-out time in samples
                batch_type_t        enType;         // Type of the batch
            } play_batch_t;

            LSP_DSP_UNITS_PUBLIC
            void        clear_batch(batch_t *b);

            LSP_DSP_UNITS_PUBLIC
            void        dump_batch_plain(IStateDumper *v, const batch_t *b);

            LSP_DSP_UNITS_PUBLIC
            void        dump_batch(IStateDumper *v, const batch_t *b);

            LSP_DSP_UNITS_PUBLIC
            void        dump_batch(IStateDumper *v, const char *name, const batch_t *b);

            LSP_DSP_UNITS_PUBLIC
            size_t      put_batch_linear_direct(float *dst, const float *src, const batch_t *b, wsize_t timestamp, size_t samples);

            LSP_DSP_UNITS_PUBLIC
            size_t      put_batch_const_power_direct(float *dst, const float *src, const batch_t *b, wsize_t timestamp, size_t samples);

            LSP_DSP_UNITS_PUBLIC
            size_t      put_batch_linear_reverse(float *dst, const float *src, const batch_t *b, wsize_t timestamp, size_t samples);

            LSP_DSP_UNITS_PUBLIC
            size_t      put_batch_const_power_reverse(float *dst, const float *src, const batch_t *b, wsize_t timestamp, size_t samples);

        } /* namespace playback */
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_HELPERS_BATCH_H_ */
