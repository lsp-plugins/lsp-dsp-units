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
            construct();
        }

        SamplePlayer::~SamplePlayer()
        {
            destroy(true);
        }

        void SamplePlayer::destroy(bool cascade)
        {
            // Stop any pending playbacks
            stop();

            // Release all held pointers to samples
            if (vSamples != NULL)
            {
                for (size_t i=0; i<nSamples; ++i)
                    release_sample(vSamples[i]);
            }

            // Free all associated data
            if (pData != NULL)
                free_aligned(pData);

            // Cascade drop all samples in the GC list
            if (cascade)
            {
                for (Sample *s = pGcList; s != NULL; )
                {
                    Sample *next = s->gc_next();
                    delete s;
                    s            = next;
                }
            }

            // Clear pointers
            pGcList         = NULL;
            pData           = NULL;
            vSamples        = NULL;
            vPlayback       = NULL;
            nPlayback       = 0;
            sActive.pHead   = NULL;
            sActive.pTail   = NULL;
            sInactive.pHead = NULL;
            sInactive.pTail = NULL;
        }

        Sample *SamplePlayer::gc()
        {
            Sample *list    = pGcList;
            pGcList         = NULL;
            return list;
        }

        void SamplePlayer::construct()
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
            pGcList         = NULL;
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

        Sample *SamplePlayer::acquire_sample(Sample *s)
        {
            if (s != NULL)
                s->gc_acquire();
            return s;
        }

        void SamplePlayer::release_sample(Sample * &s)
        {
            // Release the sample
            if (s == NULL)
                return;
            if (s->gc_release() == 0)
            {
                // Add sample to the GC list
                s->gc_link(pGcList);
                pGcList     = s;
            }
            s = NULL;
        }

        bool SamplePlayer::bind(size_t id, Sample *sample)
        {
            if ((id >= nSamples) || (vSamples == NULL))
                return false;

            // If the caller tries to bind the same sample, then do nothing.
            if (vSamples[id] == sample)
                return true;

            release_sample(vSamples[id]);
            vSamples[id] = acquire_sample(sample);

            return true;
        }

        bool SamplePlayer::unbind(size_t id)
        {
            if ((id >= nSamples) || (vSamples == NULL))
                return false;

            release_sample(vSamples[id]);
            return true;
        }

        Sample *SamplePlayer::get(size_t id)
        {
            return ((vSamples != NULL) && (id < nSamples)) ? vSamples[id] : NULL;
        }

        void SamplePlayer::unbind_all()
        {
            if (vSamples == NULL)
                return;

            for (size_t i=0; i<nSamples; ++i)
                release_sample(vSamples[i]);
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
            // Iterate over all active playbacks and apply changes to the output
            for (play_item_t *pb = sActive.pHead; pb != NULL; )
            {
                // The link of the playback can be changed, so we need
                // to remember the pointer to the next playback in list first
                play_item_t *next   = pb->pNext;

                for (size_t offset=0; offset<samples;)
                {
                    size_t to_do    = lsp_min(samples - offset, BUFFER_SIZE);

                    // Prepare the buffer
                    dsp::fill_zero(vBuffer, to_do);

                    // Apply playback to the temporary buffer and deploy temporary buffer to output
                    size_t processed    = playback::process_playback(vBuffer, pb, to_do);
                    if (processed <= 0)
                    {
                        // Reset playback
                        release_sample(pb->pSample);
                        playback::reset_playback(pb);

                        // Move to inactive
                        list_remove(&sActive, pb);
                        list_add_first(&sInactive, pb);
                        break;
                    }

                    // Deploy playback to the destination buffer
                    dsp::fmadd_k3(&dst[offset], vBuffer, pb->fVolume * fGain, processed);

                    // Update pointers
                    offset             += processed;
                } // offset

                // Move to the next playback
                pb                  = next;
            } // play_item_t
        }

        bool SamplePlayer::play(size_t id, size_t channel, float volume, ssize_t delay)
        {
            PlaySettings settings;
            settings.set_channel(id, channel);
            settings.set_playback(0, size_t(delay), volume);

            Playback result = play(&settings);
            return result.valid();
        }

        Playback SamplePlayer::play(const PlaySettings *settings)
        {
            // Check that ID of the sample is correct
            size_t id       = settings->sample_id();
            if (settings->sample_id() >= nSamples)
                return Playback();

            // Check that the sample is bound and valid
            Sample *s       = acquire_sample(vSamples[id]);
            if ((s == NULL) || (!s->valid()))
                return Playback();
            lsp_finally { release_sample(s); };

            // Check that ID of channel matches
            size_t channel  = settings->sample_channel();
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
            playback::start_playback(pb, acquire_sample(s), settings);

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
            // Ensure that we are in the correct state
            if ((vSamples == NULL) || (sActive.pHead == NULL))
                return;

            // Reset all playbacks
            for (play_item_t *pb = sActive.pHead; pb != NULL; pb = pb->pNext)
            {
                release_sample(pb->pSample);
                playback::reset_playback(pb);
            }

            // Move all data from active list to the beginning of inactive
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
            v->write("pData", pData);

            // Estimate size of the GC list
            size_t gc_size = 0;
            for (Sample *s = pGcList; s != NULL; s = s->gc_next())
                ++gc_size;

            // Dump the GC list
            v->begin_array("pGcList", &pGcList, gc_size);
            {
                for (Sample *s = pGcList; s != NULL; s = s->gc_next())
                    v->write(s);
            }
            v->end_array();
        }
    }
} /* namespace lsp */
