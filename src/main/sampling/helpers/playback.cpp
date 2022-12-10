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

#include <lsp-plug.in/dsp-units/sampling/helpers/playback.h>

namespace lsp
{
    namespace dspu
    {
        namespace playback
        {
            static inline size_t compute_batch_length(const play_batch_t *b)
            {
                return (b->nStart < b->nEnd) ? b->nEnd - b->nStart : b->nStart - b->nEnd;
            }

            static bool check_batches_sequential(const play_batch_t *prev, const play_batch_t *next)
            {
                if (prev->nEnd != next->nStart)
                    return false;
                return (prev->nStart < prev->nEnd) ? (next->nStart < next->nEnd) : (next->nEnd < next->nStart);
            }

            static bool loop_not_allowed(const playback_t *pb)
            {
                switch (pb->enState)
                {
                    case STATE_PLAY:
                        return false;
                    case STATE_STOP:
                    case STATE_CANCEL:
                    {
                        // If the cancellation point is before the end of currently processed batch,
                        // then the loop should be considered to be not allowed
                        const play_batch_t *s   = &pb->sBatch[0];
                        size_t batch_size       = compute_batch_length(s);
                        return pb->nCancelTime <= s->nTimestamp + batch_size;
                    }

                    case STATE_NONE:
                    default:
                        break;
                }
                return true;
            }


            LSP_DSP_UNITS_PUBLIC
            void compute_initial_batch(playback_t *pb, const PlaySettings *settings)
            {
                // Check the length of the sample
                size_t sample_len = pb->pSample->length();
                if (sample_len <= 0)
                {
                    pb->enState     = STATE_NONE;
                    return;
                }

                // Need to check loop settings, discard loop for invalid input
                size_t start    = lsp_min(settings->start(), sample_len - 1);
                if ((pb->nLoopStart == pb->nLoopEnd) ||
                    (pb->nLoopStart >= sample_len) ||
                    (pb->nLoopEnd >= sample_len))
                    pb->enLoopMode      = SAMPLE_LOOP_NONE;

                // Initialize the batch
                play_batch_t *b = &pb->sBatch[0];
                b->nTimestamp   = settings->delay();
                b->nFadeIn      = 0;

                // Simple case? Without the loop?
                if (pb->enLoopMode == SAMPLE_LOOP_NONE)
                {
                    b->nStart       = start;
                    b->nEnd         = sample_len;
                    b->nFadeOut     = 0;
                    b->enType       = BATCH_TAIL;
                    return;
                }

                // Update the loop mode if loop bounds are reversed
                if (pb->nLoopEnd < pb->nLoopStart)
                {
                    lsp::swap(pb->nLoopEnd, pb->nLoopStart);
                    switch (pb->enLoopMode)
                    {
                        case SAMPLE_LOOP_DIRECT:            pb->enLoopMode = SAMPLE_LOOP_REVERSE;           break;
                        case SAMPLE_LOOP_REVERSE:           pb->enLoopMode = SAMPLE_LOOP_DIRECT;            break;
                        case SAMPLE_LOOP_DIRECT_HALF_PP:    pb->enLoopMode = SAMPLE_LOOP_REVERSE_HALF_PP;   break;
                        case SAMPLE_LOOP_REVERSE_HALF_PP:   pb->enLoopMode = SAMPLE_LOOP_DIRECT_HALF_PP;    break;
                        case SAMPLE_LOOP_DIRECT_FULL_PP:    pb->enLoopMode = SAMPLE_LOOP_REVERSE_FULL_PP;   break;
                        case SAMPLE_LOOP_REVERSE_FULL_PP:   pb->enLoopMode = SAMPLE_LOOP_DIRECT_FULL_PP;    break;
                        case SAMPLE_LOOP_DIRECT_SMART_PP:   pb->enLoopMode = SAMPLE_LOOP_REVERSE_SMART_PP;  break;
                        case SAMPLE_LOOP_REVERSE_SMART_PP:  pb->enLoopMode = SAMPLE_LOOP_DIRECT_SMART_PP;   break;
                        default: break;
                    }
                }

                // Determine the start position
                size_t loop_len     = pb->nLoopEnd - pb->nLoopStart;
                pb->nXFade          = lsp_min(pb->nXFade, loop_len / 2);
                b->nStart           = start;
                b->nFadeIn          = 0;
                b->nFadeOut         = 0;

                if (start < pb->nLoopStart)
                {
                    // Start position is before the loop
                    b->nEnd             = pb->nLoopStart;
                    b->enType           = BATCH_HEAD;
                }
                else if (start < pb->nLoopEnd)
                {
                    // Start position is inside of the loop
                    switch (pb->enLoopMode)
                    {
                        case SAMPLE_LOOP_DIRECT:
                        case SAMPLE_LOOP_DIRECT_HALF_PP:
                        case SAMPLE_LOOP_DIRECT_FULL_PP:
                        case SAMPLE_LOOP_DIRECT_SMART_PP:
                            // Direct loop
                            b->nEnd         = pb->nLoopEnd;
                            b->enType       = BATCH_LOOP;
                            break;

                        case SAMPLE_LOOP_REVERSE:
                        case SAMPLE_LOOP_REVERSE_HALF_PP:
                        case SAMPLE_LOOP_REVERSE_FULL_PP:
                        case SAMPLE_LOOP_REVERSE_SMART_PP:
                            // Reverse loop
                            b->nEnd         = pb->nLoopStart;
                            b->enType       = BATCH_LOOP;
                            break;

                        default:
                            // Unknown loop type
                            b->nEnd         = sample_len;
                            b->enType       = BATCH_TAIL;
                            break;
                    }
                }
                else
                {
                    // Start position is after the loop
                    b->nEnd         = sample_len;
                    b->enType       = BATCH_TAIL;
                }
            }

            static void compute_next_batch_range_after_head(playback_t *pb)
            {
                // NOTE: loop mode is always enabled for TYPE_HEAD batch
                size_t sample_len       = pb->pSample->length();
                play_batch_t *b         = &pb->sBatch[1];

                 // Loop not allowed anymore?
                if (loop_not_allowed(pb))
                {
                    b->nStart               = pb->nLoopStart;
                    b->nEnd                 = sample_len;
                    b->enType               = BATCH_TAIL;
                    return;
                }

                // We need to add the loop batch, check the loop type and decide what to do
                switch (pb->enLoopMode)
                {
                    case SAMPLE_LOOP_DIRECT:
                    case SAMPLE_LOOP_DIRECT_HALF_PP:
                    case SAMPLE_LOOP_DIRECT_FULL_PP:
                    case SAMPLE_LOOP_DIRECT_SMART_PP:
                        // Initial direct loop
                        b->nStart               = pb->nLoopStart;
                        b->nEnd                 = pb->nLoopEnd;
                        b->enType               = BATCH_LOOP;
                        break;

                    case SAMPLE_LOOP_REVERSE:
                    case SAMPLE_LOOP_REVERSE_HALF_PP:
                    case SAMPLE_LOOP_REVERSE_FULL_PP:
                    case SAMPLE_LOOP_REVERSE_SMART_PP:
                        // Initial reverse loop
                        b->nStart               = pb->nLoopEnd;
                        b->nEnd                 = pb->nLoopStart;
                        b->enType               = BATCH_LOOP;
                        break;

                    default:
                        // Unknown loop mode, actually do not loop
                        b->nStart               = pb->nLoopStart;
                        b->nEnd                 = sample_len;
                        b->enType               = BATCH_TAIL;
                        break;
                }
            }

            static void compute_next_batch_range_inside_loop(playback_t *pb)
            {
                // NOTE: loop mode is always enabled for TYPE_LOOP batch
                size_t sample_len       = pb->pSample->length();
                const play_batch_t *s   = &pb->sBatch[0];
                play_batch_t *b         = &pb->sBatch[1];

                // Loop not allowed anymore?
                if (loop_not_allowed(pb))
                {
                    switch (pb->enLoopMode)
                    {
                        case SAMPLE_LOOP_DIRECT_FULL_PP:
                            // If current batch is in direct direction, then we need to process
                            // one more batch in the reverse direction
                            if (s->nStart < s->nEnd)
                                break;

                            b->nStart               = pb->nLoopEnd;
                            b->nEnd                 = sample_len;
                            b->enType               = BATCH_TAIL;
                            return;

                        case SAMPLE_LOOP_REVERSE_FULL_PP:
                            // If current batch is in reverse direction, then we need to process
                            // one more batch in the direct direction
                            if (s->nEnd < s->nStart)
                                break;

                            b->nStart               = pb->nLoopEnd;
                            b->nEnd                 = sample_len;
                            b->enType               = BATCH_TAIL;
                            return;

                        case SAMPLE_LOOP_DIRECT_SMART_PP:
                        case SAMPLE_LOOP_REVERSE_SMART_PP:
                            // If current batch is in reverse direction, then we need to process
                            // one more batch in the direct direction
                            if (s->nEnd < s->nStart)
                                break;

                            b->nStart               = pb->nLoopEnd;
                            b->nEnd                 = sample_len;
                            b->enType               = BATCH_TAIL;
                            return;

                        case SAMPLE_LOOP_DIRECT:
                        case SAMPLE_LOOP_REVERSE:
                        case SAMPLE_LOOP_DIRECT_HALF_PP:
                        case SAMPLE_LOOP_REVERSE_HALF_PP:
                        default:
                            // Just immediately play tail after the loop
                            b->nStart               = pb->nLoopEnd;
                            b->nEnd                 = sample_len;
                            b->enType               = BATCH_TAIL;
                            return;
                    }
                }

                // We need to add the loop batch, check the loop type and decide what to do
                switch (pb->enLoopMode)
                {
                    case SAMPLE_LOOP_DIRECT:
                        // Yet another direct loop
                        b->nStart               = pb->nLoopStart;
                        b->nEnd                 = pb->nLoopEnd;
                        b->enType               = BATCH_LOOP;
                        break;

                    case SAMPLE_LOOP_REVERSE:
                        // Yet another reverse loop
                        b->nStart               = pb->nLoopEnd;
                        b->nEnd                 = pb->nLoopStart;
                        b->enType               = BATCH_LOOP;
                        break;

                    case SAMPLE_LOOP_DIRECT_HALF_PP:
                    case SAMPLE_LOOP_DIRECT_FULL_PP:
                    case SAMPLE_LOOP_DIRECT_SMART_PP:
                    case SAMPLE_LOOP_REVERSE_HALF_PP:
                    case SAMPLE_LOOP_REVERSE_FULL_PP:
                    case SAMPLE_LOOP_REVERSE_SMART_PP:
                        // Just reverse the order of the current loop batch
                        if (s->nStart < s->nEnd)
                        {
                            b->nStart               = pb->nLoopEnd;
                            b->nEnd                 = pb->nLoopStart;
                        }
                        else
                        {
                            b->nStart               = pb->nLoopStart;
                            b->nEnd                 = pb->nLoopEnd;
                        }
                        b->enType               = BATCH_LOOP;
                        break;

                    default:
                        // Unknown loop mode, actually do not loop
                        b->nStart               = pb->nLoopStart;
                        b->nEnd                 = sample_len;
                        b->enType               = BATCH_TAIL;
                        break;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void compute_next_batch(playback_t *pb)
            {
                play_batch_t *s     = &pb->sBatch[0];
                play_batch_t *b     = &pb->sBatch[1];

                // We can compute the next batch only after BATCH_HEAD and BATCH_LOOP batches
                switch (s->enType)
                {
                    case BATCH_HEAD:
                        compute_next_batch_range_after_head(pb);
                        break;
                    case BATCH_LOOP:
                        compute_next_batch_range_inside_loop(pb);
                        break;
                    case BATCH_TAIL:
                    case BATCH_NONE:
                    default:
                        clear_batch(b);
                        return;
                }

                // Set-up batch timings
                b->nTimestamp           = s->nTimestamp + compute_batch_length(s);
                s->nFadeOut             = 0;
                b->nFadeIn              = 0;
                b->nFadeOut             = 0;

                // Perform the crossfade depending on the previous batch type and the new one
                // Do not perform cross-fade if the next batch follows exactly after the previous
                if ((pb->nXFade > 0) && (!check_batches_sequential(s, b)))
                {
                    // Depending on the type of the previous and next batch, perform time shifting
                    // and apply cross-fades for batches
                    s->nFadeOut     = pb->nXFade;
                    b->nFadeIn      = pb->nXFade;

                    if (s->enType != BATCH_HEAD)
                    {
                        b->nTimestamp  -= pb->nXFade;
                        if (b->enType == BATCH_TAIL)
                            b->nStart      -= pb->nXFade;
                    }
                    else
                        s->nEnd        += pb->nXFade;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void recompute_next_batch(playback_t *pb)
            {
                play_batch_t *s     = &pb->sBatch[0];
                play_batch_t *b     = &pb->sBatch[1];

                // Ensure that we can recompute the next batch
                switch (b->enType)
                {
                    case BATCH_HEAD:
                    case BATCH_LOOP:
                        break;
                    case BATCH_TAIL:
                    case BATCH_NONE:
                    default: // Nothing to do with batch
                        return;
                }

                // Ensure that the right conditions have occurred:
                //   - the cancellation time is inside of current batch
                //   - the cancellation time has not reached the beginning of the next batch
                if ((pb->nCancelTime >= s->nTimestamp) && (pb->nCancelTime <= b->nTimestamp))
                    compute_next_batch(pb);
            }

            LSP_DSP_UNITS_PUBLIC
            void complete_current_batch(playback_t *pb)
            {
                pb->sBatch[0]   = pb->sBatch[1];
                if (pb->sBatch[0].enType == BATCH_NONE)
                {
                    pb->enState         = STATE_NONE;
                    return;
                }

                compute_next_batch(pb);
            }

            LSP_DSP_UNITS_PUBLIC
            size_t execute_batch(float *dst, const batch_t *b, playback_t *pb, size_t samples)
            {
                // Check type of batch
                if (b->enType == BATCH_NONE)
                    return 0;

                // Initialize position
                wsize_t timestamp   = pb->nTimestamp;
                size_t offset       = 0;

                // The playback has still not happened?
                if (timestamp < b->nTimestamp)
                {
                    // If we sill didn't reach the start of batch, just return
                    wsize_t skip        = b->nTimestamp - timestamp;
                    if (skip >= samples)
                        return samples;

                    // We have reached the start of the batch, need to adjust the time
                    timestamp          += skip;
                    offset             += skip;
                }

                // Initialize parameters
                size_t batch_offset = timestamp - b->nTimestamp;
                size_t processed    = 0;
                const float *src    = pb->pSample->channel(pb->nChannel);

                if (b->nStart < b->nEnd)
                {
                    // Perform processing depending on the cross-fade type
                    switch (pb->enXFadeType)
                    {
                        case SAMPLE_CROSSFADE_CONST_POWER:
                            processed       = put_batch_const_power_direct(&dst[offset], src, b, timestamp, samples - offset);
                            break;
                        case SAMPLE_CROSSFADE_LINEAR:
                        default:
                            processed       = put_batch_linear_direct(&dst[offset], src, b, timestamp, samples - offset);
                            break;
                    }

                    // Update the offset
                    pb->nPosition       = b->nStart + batch_offset + processed;
                }
                else // b->nStart >= b->nEnd
                {
                    // Perform processing depending on the cross-fade type
                    switch (pb->enXFadeType)
                    {
                        case SAMPLE_CROSSFADE_CONST_POWER:
                            processed       = put_batch_const_power_reverse(&dst[offset], src, b, timestamp, samples - offset);
                            break;
                        case SAMPLE_CROSSFADE_LINEAR:
                        default:
                            processed       = put_batch_linear_reverse(&dst[offset], src, b, timestamp, samples - offset);
                            break;
                    }

                    // Update the offset
                    pb->nPosition       = b->nStart - batch_offset - processed;
                }

                return offset + processed;
            }

            LSP_DSP_UNITS_PUBLIC
            size_t apply_fade_out(float *dst, playback_t *pb, size_t samples)
            {
                // Initialize position
                wsize_t timestamp   = pb->nTimestamp;
                size_t offset       = 0;

                // The fade-out has still not happened?
                if (timestamp < pb->nCancelTime)
                {
                    // If we sill didn't reach the start of batch, just return
                    wsize_t skip        = pb->nCancelTime - timestamp;
                    if (skip >= samples)
                        return samples;

                    // We have reached the start of the batch, need to adjust the time
                    timestamp          += skip;
                    offset             += skip;
                }

                // The fade-out has been fully processed?
                if (timestamp >= (pb->nCancelTime + pb->nFadeout))
                    return offset;

                // Apply fade-out part
                size_t to_do        = lsp_min(samples - offset, pb->nCancelTime + pb->nFadeout - timestamp);
                dst                += offset;
                float k             = 1.0f / pb->nFadeout;
                for (size_t i=0, t=timestamp - pb->nCancelTime; i<to_do; ++i, ++t)
                    dst[i]              = dst[i] * (1.0f - t*k);

                return offset + to_do;
            }

            LSP_DSP_UNITS_PUBLIC
            void clear_playback(playback_t *pb)
            {
                pb->nTimestamp      = 0;
                pb->nCancelTime     = 0;
                pb->pSample         = NULL;
                pb->nSerial         = 0;
                pb->nID             = -1;
                pb->nChannel        = 0;
                pb->enState         = STATE_NONE;
                pb->fVolume         = 0.0f;
                pb->bReverse        = false;
                pb->nPosition       = -1;
                pb->nFadeout        = -1;
                pb->enLoopMode      = SAMPLE_LOOP_NONE;
                pb->nLoopStart      = 0;
                pb->nLoopEnd        = 0;
                pb->nXFade          = 0;
                pb->enXFadeType     = SAMPLE_CROSSFADE_CONST_POWER;

                clear_batch(&pb->sBatch[0]);
                clear_batch(&pb->sBatch[1]);
            }

            LSP_DSP_UNITS_PUBLIC
            void reset_playback(playback_t *pb)
            {
                pb->nTimestamp      = 0;
                pb->nCancelTime     = 0;
                pb->pSample         = NULL;
                pb->nSerial        += 1;
                pb->nID             = -1;
                pb->nChannel        = 0;
                pb->enState         = STATE_NONE;
                pb->fVolume         = 0.0f;
                pb->bReverse        = false;
                pb->nPosition       = -1;
                pb->nFadeout        = -1;
                pb->enLoopMode      = SAMPLE_LOOP_NONE;
                pb->nLoopStart      = 0;
                pb->nLoopEnd        = 0;
                pb->nXFade          = 0;
                pb->enXFadeType     = SAMPLE_CROSSFADE_CONST_POWER;

                clear_batch(&pb->sBatch[0]);
                clear_batch(&pb->sBatch[1]);
            }

            LSP_DSP_UNITS_PUBLIC
            void start_playback(playback_t *pb, Sample *sample, const PlaySettings *settings)
            {
                pb->nTimestamp  = 0;
                pb->nCancelTime = 0;
                pb->pSample     = sample;
                pb->nSerial    += 1;
                pb->nID         = settings->sample_id();
                pb->nChannel    = settings->sample_channel();
                pb->enState     = STATE_PLAY;
                pb->fVolume     = settings->volume();
                pb->bReverse    = settings->reverse();
                pb->nPosition   = -1;
                pb->nFadeout    = 0;
                pb->enLoopMode  = settings->loop_mode();
                pb->nLoopStart  = settings->loop_start();
                pb->nLoopEnd    = settings->loop_end();
                pb->nXFade      = settings->loop_xfade_length();
                pb->enXFadeType = settings->loop_xfade_type();

                clear_batch(&pb->sBatch[0]);
                clear_batch(&pb->sBatch[1]);

                // Compute the playback plan
                compute_initial_batch(pb, settings);
                compute_next_batch(pb);
            }

            LSP_DSP_UNITS_PUBLIC
            size_t process_playback(float *dst, playback_t *pb, size_t samples)
            {
                size_t processed, to_do;
                size_t offset = 0;

                while (offset < samples)
                {
                    to_do               = samples - offset;

                    // Switch what to do with batch depending on the state
                    switch (pb->enState)
                    {
                        case STATE_PLAY:
                        case STATE_STOP:
                            // Play batches as usual, loop planning depends on the state
                            processed       = execute_batch(&dst[offset], &pb->sBatch[0], pb, to_do);
                            execute_batch(&dst[offset], &pb->sBatch[1], pb, processed);
                            if (processed < to_do)
                                complete_current_batch(pb);
                            offset         += processed;
                            break;

                        case STATE_CANCEL:
                            // Ensure that we still didn't reach the end of fade-out
                            if (pb->nTimestamp >= pb->nCancelTime + pb->nFadeout)
                            {
                                pb->enState     = STATE_NONE;
                                processed       = 0;
                                break;
                            }

                            // We do not need to process more than feedback allows
                            to_do           = lsp_min(to_do, pb->nCancelTime + pb->nFadeout - pb->nTimestamp);

                            // Play batches, do not allow loops
                            processed       = execute_batch(&dst[offset], &pb->sBatch[0], pb, to_do);
                            execute_batch(&dst[offset], &pb->sBatch[1], pb, processed);
                            processed       = apply_fade_out(&dst[offset], pb, processed);
                            if (processed < to_do)
                                complete_current_batch(pb);

                            offset         += processed;
                            break;

                        case STATE_NONE:
                        default:
                            return offset;
                    }

                    // Upate timestamp
                    pb->nTimestamp += processed;
                }

                return offset;
            }

            LSP_DSP_UNITS_PUBLIC
            void stop_playback(playback_t *pb, size_t delay)
            {
                // We can stop the playback only if it is active at this moment
                if (pb->enState != playback::STATE_PLAY)
                    return;

                pb->enState     = playback::STATE_STOP;
                pb->nCancelTime = pb->nTimestamp + delay;
                recompute_next_batch(pb);
            }

            LSP_DSP_UNITS_PUBLIC
            bool cancel_playback(playback_t *pb, size_t fadeout, size_t delay)
            {
                // Do not cancel the playback if it already has been cancelled
                switch (pb->enState)
                {
                    case playback::STATE_PLAY:
                    case playback::STATE_STOP:
                        pb->enState     = playback::STATE_CANCEL;
                        pb->nCancelTime = pb->nTimestamp + delay;
                        pb->nFadeout    = fadeout;
                        recompute_next_batch(pb);
                        return true;

                    case playback::STATE_NONE:
                    case playback::STATE_CANCEL:
                    default:
                        break;
                }

                return false;
            }

            LSP_DSP_UNITS_PUBLIC
            void dump_playback_plain(IStateDumper *v, const playback_t *pb)
            {
                v->write("nTimestamp", pb->nTimestamp);
                v->write("nCancelTime", pb->nCancelTime);
                v->write("pSample", pb->pSample);
                v->write("nSerial", pb->nSerial);
                v->write("nID", pb->nID);
                v->write("nChannel", pb->nChannel);
                v->write("enState", int(pb->enState));
                v->write("fVolume", pb->fVolume);
                v->write("nPosition", pb->nPosition);
                v->write("nFadeout", pb->nFadeout);
                v->write("enLoopMode", int(pb->enLoopMode));
                v->write("nLoopStart", pb->nLoopStart);
                v->write("nLoopEnd", pb->nLoopEnd);
                v->write("nXFade", pb->nXFade);
                v->write("enXFadeType", int(pb->enXFadeType));

                v->begin_array("sBatch", pb->sBatch, 2);
                {
                    for (size_t i=0; i<2; ++i)
                        dump_batch(v, &pb->sBatch[i]);
                }
                v->end_array();
            }

            LSP_DSP_UNITS_PUBLIC
            void dump_playback(IStateDumper *v, const playback_t *pb)
            {
                v->begin_object(pb, sizeof(*pb));
                dump_playback_plain(v, pb);
                v->end_object();
            }

            LSP_DSP_UNITS_PUBLIC
            void dump_playback(IStateDumper *v, const char *name, const playback_t *pb)
            {
                v->begin_object(name, pb, sizeof(*pb));
                dump_playback_plain(v, pb);
                v->end_object();
            }

        } /* namespace playback */
    } /* namespace dspu */
} /* namespace lsp */



