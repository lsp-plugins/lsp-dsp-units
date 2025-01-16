/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 13 марта 2016 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_SAMPLING_SAMPLEPLAYER_H_
#define LSP_PLUG_IN_DSP_UNITS_SAMPLING_SAMPLEPLAYER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/sampling/helpers/batch.h>
#include <lsp-plug.in/dsp-units/sampling/helpers/playback.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/sampling/Playback.h>
#include <lsp-plug.in/dsp-units/sampling/PlaySettings.h>

namespace lsp
{
    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC SamplePlayer
        {
            protected:
                typedef struct play_item_t: public playback::playback_t
                {
                    play_item_t *pNext;     // Pointer to the next playback in the list
                    play_item_t *pPrev;     // Pointer to the previous playback in the list
                } play_item_t;

                typedef struct list_t
                {
                    play_item_t *pHead;     // The head of the list
                    play_item_t *pTail;     // The tail of the list
                } list_t;

            private:
                float          *vBuffer;    // Temporary buffer for rendering samples
                Sample        **vSamples;
                size_t          nSamples;
                play_item_t    *vPlayback;
                size_t          nPlayback;
                list_t          sActive;
                list_t          sInactive;
                float           fGain;
                uint8_t        *pData;
                Sample         *pGcList;    // List of garbage samples not used by the sample player

            protected:
                static inline void list_remove(list_t *list, play_item_t *pb);
                static inline play_item_t *list_remove_first(list_t *list);
                static inline void list_add_first(list_t *list, play_item_t *pb);
                static inline void list_insert_from_tail(list_t *list, play_item_t *pb);

                static void dump_list(IStateDumper *v, const char *name, const list_t *list);

            protected:
                void            release_sample(Sample * &s);
                static Sample  *acquire_sample(Sample *s);

            protected:
                void            do_process(float *dst, size_t samples);

            public:
                explicit SamplePlayer();
                SamplePlayer(const SamplePlayer &) = delete;
                SamplePlayer(SamplePlayer &&) = delete;
                ~SamplePlayer();

                SamplePlayer & operator = (const SamplePlayer &) = delete;
                SamplePlayer & operator = (SamplePlayer &&) = delete;

                /**
                 * Construct sample player
                 */
                void construct();

                /** Initialize player
                 *
                 * @param max_samples maximum available samples
                 * @param max_playbacks maximum number of simultaneous played samples
                 * @return true on success
                 */
                bool init(size_t max_samples, size_t max_playbacks);

                /** Destroy player
                 * @param cascade destroy the samples which have been put to the GC list
                 * @return garbage collection list if present
                 */
                dspu::Sample   *destroy(bool cascade = true);

                /**
                 * Return the list of garbage-collected samples. After calling this method,
                 * the internal list of the garbage-collected items inside of the sampler
                 * becomes cleared.
                 * @return pointer to the first item in the list of garbage-collected samples
                 */
                Sample         *gc();

            public:
                /** Set output gain
                 *
                 * @param gain output gain
                 */
                inline void set_gain(float gain) { fGain = gain; }

                /** Bind sample to specified ID
                 *
                 * @param id id of the sample
                 * @param sample pointer to the sample
                 * @return true on success
                 */
                bool bind(size_t id, Sample *sample);

                /** Unbind sample
                 *
                 * @param id id of the sample
                 * @return false if invalid index has been specified
                 */
                bool unbind(size_t id);

                /**
                 * Get currently bound sample
                 * @param id id of the sample
                 * @return pointer to the sample or NULL
                 */
                Sample *get(size_t id);
                const Sample *get(size_t id) const;

                /**
                 * Unbind all previously bound samples
                 */
                void unbind_all();

                /** Process the audio data
                 *
                 * @param dst destination buffer to store data
                 * @param src source buffer to read data
                 * @param samples amount of audio samples to process
                 */
                void process(float *dst, const float *src, size_t samples);

                /** Process the audio data
                 *
                 * @param dst destination buffer to store data
                 * @param samples amount of audio samples to process
                 */
                void process(float *dst, size_t samples);

                /** Trigger the playback of the sample
                 *
                 * @param id ID of the sample
                 * @param channel ID of the sample's channel
                 * @param volume the volume of the sample
                 * @param delay the delay (in samples) of the sample relatively to the next process() call
                 * @return true if parameters are valid
                 */
                bool play(size_t id, size_t channel, float volume, ssize_t delay = 0);

                /**
                 * Trigger the playback of the sample
                 * @param settings playback settings
                 * @return true if parameters are valid
                 */
                Playback play(const PlaySettings *settings = NULL);

                /** Soft cancel playback of the sample
                 *
                 * @param id ID of the sample
                 * @param channel ID of the sample's channel
                 * @param fadeout the fadeout length in samples for sample gain fadeout
                 * @param delay the delay (in samples) of the sample relatively to the next process() call
                 * @return number of playbacks cancelled, negative value on error
                 */
                ssize_t cancel_all(size_t id, size_t channel, size_t fadeout = 0, ssize_t delay = 0);

                /** Reset the playback state of the player,
                 * force all playbacks to be stopped immediately
                 */
                void stop();

            public:
                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_SAMPLEPLAYER_H_ */
