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

#ifndef LSP_PLUG_IN_DSP_UNITS_SAMPLING_HELPERS_PLAYBACK_H_
#define LSP_PLUG_IN_DSP_UNITS_SAMPLING_HELPERS_PLAYBACK_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/sampling/helpers/batch.h>
#include <lsp-plug.in/dsp-units/sampling/types.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/sampling/PlaySettings.h>

namespace lsp
{
    namespace dspu
    {
        namespace playback
        {
            /**
             * The actual state of the playback of the sample
             */
            enum playback_state_t
            {
                STATE_NONE,                     // No active state
                STATE_PLAY,                     // Regular play
                STATE_STOP,                     // Playback till the end of the sample, do not loop
                STATE_CANCEL                    // Immediately cancel the playback
            };

            /**
             * Low-level data structure that holds the state of the finite machine
             * that introduces the playback of the single sample
             */
            typedef struct playback_t
            {
                wsize_t             nTimestamp;     // The actual playback timestamp in stamples
                wsize_t             nCancelTime;    // The actual cancel timestamp
                Sample             *pSample;        // Pointer to the sample
                size_t              nSerial;        // Serial version of playback object
                ssize_t             nID;            // ID of instrument
                size_t              nChannel;       // Channel to play
                playback_state_t    enState;        // State of the playback
                float               fVolume;        // The volume of the sample
                ssize_t             nPosition;      // Current playback position
                size_t              nFadeout;       // Fadeout length for cancelling
                sample_loop_t       enLoopMode;     // Loop mode
                size_t              nLoopStart;     // Start of the loop
                size_t              nLoopEnd;       // End of the loop
                size_t              nXFade;         // The crossfade time in stamples
                sample_crossfade_t  enXFadeType;    // The crossfade type
                play_batch_t        sBatch[2];      // Batch queue for execution
            } playback_t;

            LSP_DSP_UNITS_PUBLIC
            void        compute_initial_batch(playback_t *pb, const PlaySettings *settings);

            LSP_DSP_UNITS_PUBLIC
            void        compute_next_batch(playback_t *pb);

            LSP_DSP_UNITS_PUBLIC
            void        recompute_next_batch(playback_t *pb);

            LSP_DSP_UNITS_PUBLIC
            void        complete_current_batch(playback_t *pb);

            LSP_DSP_UNITS_PUBLIC
            size_t      execute_batch(float *dst, const batch_t *b, playback_t *pb, size_t samples);

            LSP_DSP_UNITS_PUBLIC
            size_t      apply_fade_out(float *dst, playback_t *pb, size_t samples);


            LSP_DSP_UNITS_PUBLIC
            void        clear_playback(playback_t *pb);

            LSP_DSP_UNITS_PUBLIC
            void        reset_playback(playback_t *pb);

            LSP_DSP_UNITS_PUBLIC
            void        start_playback(playback_t *pb, Sample *sample, const PlaySettings *settings);

            LSP_DSP_UNITS_PUBLIC
            size_t      process_playback(float *dst, playback_t *pb, size_t samples);

            LSP_DSP_UNITS_PUBLIC
            void        stop_playback(playback_t *pb, size_t delay = 0);

            LSP_DSP_UNITS_PUBLIC
            bool        cancel_playback(playback_t *pb, size_t fadeout = 0, size_t delay = 0);

            LSP_DSP_UNITS_PUBLIC
            void        dump_playback_plain(IStateDumper *v, const playback_t *pb);

            LSP_DSP_UNITS_PUBLIC
            void        dump_playback(IStateDumper *v, const playback_t *pb);

            LSP_DSP_UNITS_PUBLIC
            void        dump_playback(IStateDumper *v, const char *name, const playback_t *pb);

        } /* namespace playback */
    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_HELPERS_PLAYBACK_H_ */
