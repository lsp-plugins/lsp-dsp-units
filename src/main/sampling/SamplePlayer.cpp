/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 13 марта 2016 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/sampling/SamplePlayer.h>
#include <lsp-plug.in/stdlib/math.h>

#define BUFFER_SIZE         0x1000u
#define SAMPLER_ALIGN       0x40u

namespace lsp
{
    namespace dspu
    {
        SamplePlayer::SamplePlayer()
        {
            vBuffer         = NULL;
            vSamples        = NULL;
            nSamples        = 0;
            vPlayback       = NULL;
            nPlayback       = 0;
            sActive.pHead   = NULL;
            sActive.pTail   = NULL;
            sInactive.pHead = NULL;
            sInactive.pTail = NULL;
            fGain           = 1.0f;
            pData           = NULL;
        }

        SamplePlayer::~SamplePlayer()
        {
            destroy(true);
        }
    
        inline void SamplePlayer::list_remove(list_t *list, play_item_t *pb)
        {
            if (pb->pPrev != NULL)
                pb->pPrev->pNext    = pb->pNext;
            else
                list->pHead         = pb->pNext;
            if (pb->pNext != NULL)
                pb->pNext->pPrev    = pb->pPrev;
            else
                list->pTail         = pb->pPrev;
        }

        inline SamplePlayer::play_item_t *SamplePlayer::list_remove_first(list_t *list)
        {
            play_item_t *pb         = list->pHead;
            if (pb == NULL)
                return NULL;
            list->pHead             = pb->pNext;
            if (pb->pNext != NULL)
                pb->pNext->pPrev    = pb->pPrev;
            else
                list->pTail         = pb->pPrev;
            return pb;
        }

        inline void SamplePlayer::list_add_first(list_t *list, play_item_t *pb)
        {
            if (list->pHead == NULL)
            {
                list->pHead         = pb;
                list->pTail         = pb;
                pb->pPrev           = NULL;
                pb->pNext           = NULL;
            }
            else
            {
                pb->pNext           = list->pHead;
                pb->pPrev           = NULL;
                list->pHead->pPrev  = pb;
                list->pHead         = pb;
            }
        }

        inline void SamplePlayer::list_insert_from_tail(list_t *list, play_item_t *pb)
        {
            // Find the position to insert data
            play_item_t *prev    = list->pTail;
            while (prev != NULL)
            {
                if (pb->nTimestamp <= prev->nTimestamp)
                    break;
                prev    = prev->pPrev;
            }

            // Position is before the first element?
            if (prev == NULL)
            {
                list_add_first(list, pb);
                return;
            }

            // Insert after the found element
            if (prev->pNext == NULL)
                list->pTail         = pb;
            else
                prev->pNext->pPrev  = pb;
            pb->pPrev           = prev;
            pb->pNext           = prev->pNext;
            prev->pNext         = pb;
        }

        inline void SamplePlayer::clear_batch(play_batch_t *b)
        {
            b->nTimestamp       = 0;
            b->nStart           = 0;
            b->nEnd             = 0;
            b->enType           = play_batch_t::TYPE_NONE;
        }

        inline void SamplePlayer::cleanup(playback_t *pb)
        {
            pb->nTimestamp      = 0;
            pb->nCancelTime     = 0;
            pb->pSample         = NULL;
            pb->nSerial        += 1;
            pb->nID             = -1;
            pb->nChannel        = 0;
            pb->enState         = playback_t::STATE_NONE;
            pb->nPosition       = 0;
            pb->nFadeout        = -1;
            pb->fVolume         = 0.0f;
            pb->nXFade          = 0;
            pb->enXFadeType     = SAMPLE_CROSSFADE_CONST_POWER;

            clear_batch(&pb->sBatch[0]);
            clear_batch(&pb->sBatch[1]);
        }

        bool SamplePlayer::init(size_t max_samples, size_t max_playbacks)
        {
            // Check arguments
            if ((max_samples <= 0) || (max_playbacks <= 0))
                return false;

            // Estimate the amount of data to allocate
            size_t szof_buffer      = BUFFER_SIZE*sizeof(float);
            size_t szof_samples     = align_size(sizeof(Sample *) * max_samples, SAMPLER_ALIGN);
            size_t szof_playback    = align_size(sizeof(play_item_t) * max_playbacks, SAMPLER_ALIGN);
            size_t to_alloc         = szof_buffer + szof_samples + szof_playback;

            uint8_t *data           = NULL;
            uint8_t *ptr            = alloc_aligned<uint8_t>(data, to_alloc, SAMPLER_ALIGN);
            lsp_guard_assert(
                uint8_t *end = &ptr[to_alloc];
            );
            if ((ptr == NULL) || (data == NULL))
                return false;
            lsp_finally {
                free_aligned(data);
            };

            // Update state
            lsp::swap(pData, data);
            vBuffer             = reinterpret_cast<float *>(ptr);
            ptr                += szof_buffer;
            vSamples            = reinterpret_cast<Sample **>(ptr);
            ptr                += szof_samples;
            vPlayback           = reinterpret_cast<play_item_t *>(ptr);
            ptr                += szof_playback;
            lsp_assert( ptr <= end );

            nSamples            = max_samples;
            nPlayback           = max_playbacks;
            for (size_t i=0; i<max_samples; ++i)
                vSamples[i]         = NULL;

            // Init active list (empty)
            sActive.pHead       = NULL;
            sActive.pTail       = NULL;

            // Init inactive list (full)
            play_item_t *last   = NULL;
            sInactive.pHead     = NULL;
            for (size_t i=0; i<max_playbacks; ++i)
            {
                play_item_t *curr = &vPlayback[i];

                // Initialize fields
                curr->nSerial    = 0;
                cleanup(curr);

                // Link
                curr->pPrev     = last;
                if (last == NULL)
                    sInactive.pHead = curr;
                else
                    last->pNext     = curr;

                last            = curr;
            }
            last->pNext         = NULL;
            sInactive.pTail     = last;

            return true;
        }

        void SamplePlayer::destroy(bool cascade)
        {
            if (vSamples != NULL)
            {
                // Delete all bound samples
                if (cascade)
                {
                    for (size_t i=0; i<nSamples; ++i)
                    {
                        if (vSamples[i] != NULL)
                        {
                            vSamples[i]->destroy();
                            delete vSamples[i];
                            vSamples[i] = NULL;
                        }
                    }
                }

                // Delete the array
                delete [] vSamples;
                vSamples        = NULL;
            }
            nSamples        = 0;

            if (vPlayback != NULL)
            {
                delete [] vPlayback;
                vPlayback       = NULL;
            }
            nPlayback       = 0;
            sActive.pHead   = NULL;
            sActive.pTail   = NULL;
            sInactive.pHead = NULL;
            sInactive.pTail = NULL;
        }

        bool SamplePlayer::bind(size_t id, Sample **sample)
        {
            if (id >= nSamples)
                return false;

            Sample     *old = vSamples[id];
            if (sample != NULL)
            {
                Sample     *ns  = *sample;
                if (old == ns)
                {
                    *sample     = NULL;
                    return true;
                }

                vSamples[id]    = ns;
                *sample         = old;
            }

            // Cleanup all active playbacks associated with this sample
            play_item_t *pb = sActive.pHead;
            while (pb != NULL)
            {
                play_item_t *next    = pb->pNext;
                if (pb->pSample == old)
                {
                    pb->pSample     = NULL;
                    list_remove(&sActive, pb);
                    list_add_first(&sInactive, pb);
                }
                pb          = next;
            }

            return true;
        }

        bool SamplePlayer::bind(size_t id, Sample *sample, bool destroy)
        {
            if (!bind(id, &sample))
                return false;

            if ((destroy) && (sample != NULL))
            {
                sample->destroy();
                delete [] sample;
            }

            return true;
        }

        bool SamplePlayer::unbind(size_t id, Sample **sample)
        {
            *sample     = NULL;
            return bind(id, sample);
        }

        bool SamplePlayer::unbind(size_t id, bool destroy)
        {
            return bind(id, static_cast<Sample *>(NULL), destroy);
        }

        void SamplePlayer::process(float *dst, const float *src, size_t samples)
        {
            if (src == NULL)
                dsp::fill_zero(dst, samples);
            else
                dsp::copy(dst, src, samples);
            do_process(dst, samples);
        }

        void SamplePlayer::process(float *dst, size_t samples)
        {
            dsp::fill_zero(dst, samples);
            do_process(dst, samples);
        }

        /* Batch layout:
         *                samples
         *       |<--------------------->|
         *       |                       |
         *  +---------+-------------+----------+
         *  | fade-in |    body     | fade-out |
         *  +---------+-------------+----------+
         *  |    |    |             |          |
         *  0    t0   t1            t2         t3
         */

        size_t SamplePlayer::put_batch_linear_direct(float *dst, const float *src, const play_batch_t *b, wsize_t timestamp, size_t samples)
        {
            // Direct playback, compute batch size and position
            size_t t3   = b->nEnd - b->nStart;          // Batch size in samples
            size_t t0   = timestamp - b->nTimestamp;    // Initial rendering position inside of the batch
            if (t0 >= t3)
                return 0;

            size_t t    = t0;                           // Current rendering position inside of the batch
            src        += b->nStart;                    // Adjust pointer for simplification of pointer arithmetics

            // Render the fade-in
            size_t t1   = b->nFadeIn;
            if (t < t1)
            {
                // Render contents
                float k     = 1.0f / b->nFadeIn;
                size_t n    = lsp_min(samples, t1 - t);
                for (size_t i=0; i<n; ++i, ++t)
                    dst[i]     += src[t] * (t * k);

                // Update the position
                samples    -= n;
                if (samples <= 0)
                    return t - t0;
                dst        += n;
            }

            // Render the body
            size_t t2   = t3 - b->nFadeOut;
            if (t < t2)
            {
                // Render contents
                size_t n    = lsp_min(samples, t2 - t);
                dsp::add2(dst, &src[t], n);

                // Update the position
                samples    -= n;
                if (samples <= 0)
                    return t - t0;
                dst        += n;
            }

            // Render the fade-out
            if (t < t3)
            {
                // Render the contents
                float k     = 1.0f / b->nFadeOut;
                size_t n    = lsp_min(samples, t3 - t);
                for (size_t i=0; i<n; ++i, ++t)
                    dst[i]     += src[t] * ((t3 - t) * k);
            }

            return t - t0;
        }

        size_t SamplePlayer::put_batch_const_power_direct(float *dst, const float *src, const play_batch_t *b, wsize_t timestamp, size_t samples)
        {
            // Direct playback, compute batch size and position
            size_t t3   = b->nStart - b->nEnd;          // Batch size in samples
            size_t t0   = timestamp - b->nTimestamp;    // Initial rendering position inside of the batch
            if (t0 >= t3)
                return 0;

            size_t tr   = t3 - 1;                       // Last sample
            size_t t    = t0;                           // Current rendering position inside of the batch
            src        += b->nEnd;                      // Adjust pointer for simplification of pointer arithmetics

            // Render the fade-in
            size_t t1   = b->nFadeIn;
            if (t < t1)
            {
                // Render contents
                float k     = 1.0f / b->nFadeIn;
                size_t n    = lsp_min(samples, t1 - t);
                for (size_t i=0; i<n; ++i, ++t)
                    dst[i]     += src[tr - t] * sqrtf(t * k);

                // Update the position
                samples    -= n;
                if (samples <= 0)
                    return t - t0;
                dst        += n;
            }

            // Render the body
            size_t t2   = t3 - b->nFadeOut;
            if (t < t2)
            {
                // Render contents
                size_t n    = lsp_min(samples, t2 - t);
                for (size_t i=0; i<n; ++i, ++t)
                    dst[i]      = src[tr-t];

                // Update the position
                samples    -= n;
                if (samples <= 0)
                    return t - t0;
                dst        += n;
            }

            // Render the fade-out
            if (t < t3)
            {
                // Render the contents
                float k     = 1.0f / b->nFadeOut;
                size_t n    = lsp_min(samples, t3 - t);
                for (size_t i=0; i<n; ++i, ++t)
                    dst[i]     += src[tr - t] * sqrtf((t3 - t) * k);
            }

            return t - t0;
        }

        size_t SamplePlayer::put_batch_linear_reverse(float *dst, const float *src, const play_batch_t *b, wsize_t timestamp, size_t samples)
        {
            // Direct playback, compute batch size and position
            size_t t3   = b->nEnd - b->nStart;          // Batch size in samples
            size_t t0   = timestamp - b->nTimestamp;    // Initial rendering position inside of the batch
            if (t0 >= t3)
                return 0;

            size_t t    = t0;                           // Current rendering position inside of the batch
            src        += b->nStart;                    // Adjust pointer for simplification of pointer arithmetics

            // Render the fade-in
            size_t t1   = b->nFadeIn;
            if (t < t1)
            {
                // Render contents
                float k     = 1.0f / b->nFadeIn;
                size_t n    = lsp_min(samples, t1 - t);
                for (size_t i=0; i<n; ++i, ++t)
                    dst[i]     += src[t] * (t * k);

                // Update the position
                samples    -= n;
                if (samples <= 0)
                    return t - t0;
                dst        += n;
            }

            // Render the body
            size_t t2   = t3 - b->nFadeOut;
            if (t < t2)
            {
                // Render contents
                size_t n    = lsp_min(samples, t2 - t);
                dsp::add2(dst, &src[t], n);

                // Update the position
                samples    -= n;
                if (samples <= 0)
                    return t - t0;
                dst        += n;
            }

            // Render the fade-out
            if (t < t3)
            {
                // Render the contents
                float k     = 1.0f / b->nFadeOut;
                size_t n    = lsp_min(samples, t3 - t);
                for (size_t i=0; i<n; ++i, ++t)
                    dst[i]     += src[t] * ((t3 - t) * k);
            }

            return t - t0;
        }

        size_t SamplePlayer::put_batch_const_power_reverse(float *dst, const float *src, const play_batch_t *b, wsize_t timestamp, size_t samples)
        {
            // Direct playback, compute batch size and position
            size_t t3   = b->nEnd - b->nStart;          // Batch size in samples
            size_t t0   = timestamp - b->nTimestamp;    // Initial rendering position inside of the batch
            if (t0 >= t3)
                return 0;

            size_t t    = t0;                           // Current rendering position inside of the batch
            src        += b->nStart;                    // Adjust pointer for simplification of pointer arithmetics

            // Render the fade-in
            size_t t1   = b->nFadeIn;
            if (t < t1)
            {
                // Render contents
                float k     = 1.0f / b->nFadeIn;
                size_t n    = lsp_min(samples, t1 - t);
                for (size_t i=0; i<n; ++i, ++t)
                    dst[i]     += src[t] * sqrtf(t * k);

                // Update the position
                samples    -= n;
                if (samples <= 0)
                    return t - t0;
                dst        += n;
            }

            // Render the body
            size_t t2   = t3 - b->nFadeOut;
            if (t < t2)
            {
                // Render contents
                size_t n    = lsp_min(samples, t2 - t);
                dsp::add2(dst, &src[t], n);

                // Update the position
                samples    -= n;
                if (samples <= 0)
                    return t - t0;
                dst        += n;
            }

            // Render the fade-out
            if (t < t3)
            {
                // Render the contents
                float k     = 1.0f / b->nFadeOut;
                size_t n    = lsp_min(samples, t3 - t);
                for (size_t i=0; i<n; ++i, ++t)
                    dst[i]     += src[t] * sqrtf((t3 - t) * k);
            }

            return t - t0;
        }

        size_t SamplePlayer::execute_batch(float *dst, const play_batch_t *b, playback_t *pb, size_t samples)
        {
            // Check type of batch
            if (b->enType == play_batch_t::TYPE_NONE)
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
                        processed       = put_batch_const_power_reverse(&dst[offset], src, b, batch_offset, samples - offset);
                        break;
                    case SAMPLE_CROSSFADE_LINEAR:
                    default:
                        processed       = put_batch_linear_reverse(&dst[offset], src, b, batch_offset, samples - offset);
                        break;
                }

                // Update the offset
                pb->nPosition       = b->nEnd + batch_offset + processed;
            }

            return offset + processed;
        }

        void SamplePlayer::apply_fade_out(float *dst, playback_t *pb, size_t samples)
        {
        }

        void SamplePlayer::compute_next_batch(playback_t *pb, bool loop)
        {
        }

        void SamplePlayer::complete_current_batch(playback_t *pb, bool loop)
        {
            pb->sBatch[0]   = pb->sBatch[1];
            if (pb->sBatch[0].enType == play_batch_t::TYPE_NONE)
            {
                pb->enState         = playback_t::STATE_NONE;
                return;
            }

            compute_next_batch(pb, loop);
        }


        size_t SamplePlayer::process_single_playback(float *dst, play_item_t *pb, size_t samples)
        {
            size_t processed, to_do;
            size_t offset = 0;

            while (offset < samples)
            {
                to_do               = samples - offset;

                // Switch what to do with batch depending on the state
                switch (pb->enState)
                {
                    case playback_t::STATE_PLAY:
                        // Play batches, allow loops
                        processed   = execute_batch(&dst[offset], &pb->sBatch[0], pb, to_do);
                        if (processed >= to_do)
                        {
                            execute_batch(&dst[offset], &pb->sBatch[1], pb, processed);
                            offset         += processed;
                            pb->nTimestamp += processed;
                        }
                        else
                            complete_current_batch(pb, true);
                        break;

                    case playback_t::STATE_STOP:
                        // Play batches, do not allow loops
                        processed   = execute_batch(&dst[offset], &pb->sBatch[0], pb, to_do);
                        if (processed >= to_do)
                        {
                            execute_batch(&dst[offset], &pb->sBatch[1], pb, processed);
                            offset         += processed;
                            pb->nTimestamp += processed;
                        }
                        else
                            complete_current_batch(pb, false);
                        break;

                    case playback_t::STATE_CANCEL:
                        // Ensure that we still didn't reach the end of fade-out
                        if (pb->nTimestamp >= pb->nCancelTime + pb->nFadeout)
                        {
                            pb->enState     = playback_t::STATE_NONE;
                            break;
                        }

                        // We do not need to process more than feedback allows
                        to_do           = lsp_min(to_do, pb->nCancelTime + pb->nFadeout - pb->nTimestamp);

                        // Play batches, do not allow loops
                        processed       = execute_batch(&dst[offset], &pb->sBatch[0], pb, to_do);
                        if (processed >= to_do)
                        {
                            execute_batch(&dst[offset], &pb->sBatch[1], pb, processed);
                            apply_fade_out(&dst[offset], pb, processed);
                            offset         += processed;
                            pb->nTimestamp += processed;
                        }
                        else
                            complete_current_batch(pb, false);
                        break;

                    case playback_t::STATE_NONE:
                    default:
                        // Cleanup playback
                        cleanup(pb);
                        // Move to inactive
                        list_remove(&sActive, pb);
                        list_add_first(&sInactive, pb);
                        return offset;
                }
            }

            return offset;
        }

        void SamplePlayer::do_process(float *dst, size_t samples)
        {
            // Operate with not greater than BUFFER_SIZE parts
            for (size_t offset=0; offset<samples;)
            {
                size_t to_do    = lsp_min(samples - offset, BUFFER_SIZE);

                // Iterate over all active playbacks and apply changes to the output
                for (play_item_t *pb = sActive.pHead; pb != NULL; )
                {
                    // The link of the playback can be changed, so we need
                    // to remember the pointer to the next playback in list first
                    play_item_t *next   = pb->pNext;

                    // Prepare the buffer
                    dsp::fill_zero(vBuffer, to_do);

                    // Apply playback to the temporary buffer and deploy temporary buffer to output
                    size_t processed    = process_single_playback(vBuffer, pb, to_do);
                    dsp::fmadd_k3(dst, vBuffer, pb->fVolume * fGain, processed);

                    // Move to the next playback
                    pb                  = next;
                }

                // Update pointers
                dst                += to_do;
                offset             += to_do;
            }
        }

        bool SamplePlayer::play(size_t id, size_t channel, float volume, ssize_t delay)
        {
            PlaySettings settings;
            settings.set_playback(0, size_t(delay), volume);
            Playback result = play(id, channel, &settings);
            return result.valid();
        }

        Playback SamplePlayer::play(size_t id, size_t channel, const PlaySettings *settings)
        {
            // Check that ID of the sample is correct
            if (id >= nSamples)
                return Playback();

            // Check that the sample is bound and valid
            Sample *s       = vSamples[id];
            if ((s == NULL) || (!s->valid()))
                return Playback();

            // Check that ID of channel matches
            if (channel >= s->channels())
                return Playback();

            // Try to allocate new playback
            play_item_t *pb  = list_remove_first(&sInactive);
            if (pb == NULL)
                pb              = list_remove_first(&sActive);
            if (pb == NULL)
                return Playback();

    //        lsp_trace("acquired playback %p", pb);

            // Now we are ready to activate sample
            // TODO: implement playback planning
            if (settings == NULL)
                settings        = &PlaySettings::default_settings;

            pb->nTimestamp  = 0;
            pb->pSample     = s;
            pb->nSerial    += 1;
            pb->nID         = id;
            pb->nChannel    = channel;
            pb->enState     = playback_t::STATE_PLAY;
            pb->nPosition   = 0;
            pb->nFadeout    = 0;
            pb->enLoopMode  = settings->loop_mode();
            pb->fVolume     = settings->volume();

            clear_batch(&pb->sBatch[0]);
            clear_batch(&pb->sBatch[1]);

            // Add the playback to the active list
            list_insert_from_tail(&sActive, pb);

            return Playback(pb);
        }

        ssize_t SamplePlayer::cancel_all(size_t id, size_t channel, size_t fadeout, ssize_t delay)
        {
            // Check that ID of the sample is correct
            if (id >= nSamples)
                return -1;

            // Stop all playbacks
            ssize_t result = 0;

            for (play_item_t *pb = sActive.pHead; pb != NULL; pb = pb->pNext)
            {
                // Cancel playback if not already cancelled
                if ((pb->nID == ssize_t(id)) &&
                    (pb->pSample != NULL) &&
                    (pb->nFadeout < 0))
                {
                    pb->enState     = play_item_t::STATE_CANCEL;
                    pb->nCancelTime = pb->nTimestamp + delay;
                    pb->nFadeout    = fadeout;
                    result          ++;
                }
            }

            return result;
        }

        void SamplePlayer::stop()
        {
            // Stop all playbacks
            for (play_item_t *pb = sActive.pHead; pb != NULL; pb = pb->pNext)
                cleanup(pb);

            // Move all data from active list to inactive
            if (sInactive.pHead == NULL)
                sInactive.pTail         = sActive.pTail;
            else
            {
                sActive.pTail->pNext    = sInactive.pHead;
                sInactive.pHead->pPrev  = sActive.pTail;
            }

            sInactive.pHead     = sActive.pHead;
            sActive.pHead       = NULL;
            sActive.pTail       = NULL;
        }

        void SamplePlayer::dump_list(IStateDumper *v, const char *name, const list_t *l)
        {
            v->begin_object(name, l, sizeof(*l));
            {
                v->write("pHead", l->pHead);
                v->write("pTail", l->pTail);
            }
            v->end_object();
        }

        void SamplePlayer::dump_batch(IStateDumper *v, const play_batch_t *b)
        {
            v->begin_object(b, sizeof(*b));
            {
                v->write("nTimestamp", b->nTimestamp);
                v->write("nStart", b->nStart);
                v->write("nEnd", b->nEnd);
                v->write("nFadeIn", b->nFadeIn);
                v->write("nFadeOut", b->nFadeOut);
                v->write("enType", int(b->enType));
            }
            v->end_object();
        }

        void SamplePlayer::dump(IStateDumper *v) const
        {
            v->begin_array("vSamples", vSamples, nSamples);
            {
                for (size_t i=0; i<nSamples; ++i)
                    v->write_object(vSamples[i]);
            }
            v->end_array();
            v->write("nSamples", nSamples);

            v->begin_array("vPlayback", vPlayback, nPlayback);
            {
                for (size_t i=0; i<nPlayback; ++i)
                {
                    const play_item_t *p = &vPlayback[i];
                    v->begin_object(p, sizeof(play_item_t));
                    {
                        v->write("nTimestamp", p->nTimestamp);
                        v->write("nCancelTime", p->nCancelTime);
                        v->write("pSample", p->pSample);
                        v->write("nSerial", p->nSerial);
                        v->write("nID", p->nID);
                        v->write("nChannel", p->nChannel);
                        v->write("nState", int(p->enState));
                        v->write("fVolume", p->fVolume);
                        v->write("nPosition", p->nPosition);
                        v->write("nFadeout", p->nFadeout);
                        v->write("pNext", p->pNext);
                        v->write("pPrev", p->pPrev);
                        v->write("enLoopMode", int(p->enLoopMode));
                        v->write("nLoopStart", p->nLoopStart);
                        v->write("nLoopEnd", p->nLoopEnd);
                        v->write("nXFade", p->nXFade);
                        v->write("enXFadeType", int(p->enXFadeType));

                        v->begin_array("sBatch", p->sBatch, 2);
                        {
                            for (size_t i=0; i<2; ++i)
                                dump_batch(v, &p->sBatch[i]);
                        }
                        v->end_array();
                    }
                    v->end_object();
                }
            }
            v->end_array();
            v->write("nPlayback", nPlayback);

            dump_list(v, "sActive", &sActive);
            dump_list(v, "sInactive", &sInactive);

            v->write("fGain", fGain);
        }
    }
} /* namespace lsp */
