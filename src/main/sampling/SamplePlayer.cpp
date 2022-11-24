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

                // Initialize playback fields
                playback::clear_playback(curr);

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
                    size_t processed    = playback::process_playback(vBuffer, pb, to_do);
                    if (processed <= 0)
                    {
                        // Reset playback
                        playback::reset_playback(pb);

                        // Move to inactive
                        list_remove(&sActive, pb);
                        list_add_first(&sInactive, pb);
                    }
                    else
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
            if (settings == NULL)
                settings        = &PlaySettings::default_settings;

            // Initialize playback state
            playback::start_playback(pb, id, channel, s, settings);

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
                    (pb->pSample != NULL))
                {
                    if (playback::cancel_playback(pb, fadeout, delay))
                        ++result;
                }
            }

            return result;
        }

        void SamplePlayer::stop()
        {
            // Stop all playbacks
            for (play_item_t *pb = sActive.pHead; pb != NULL; pb = pb->pNext)
                playback::reset_playback(pb);

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
                        playback::dump_playback_plain(v, p);
                        v->write("pNext", p->pNext);
                        v->write("pPrev", p->pPrev);
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
