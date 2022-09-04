/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/dsp-units/sampling/Sample.h>

namespace lsp
{
    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC SamplePlayer
        {
            private:
                SamplePlayer & operator = (const SamplePlayer &);
                SamplePlayer(const SamplePlayer &);

            protected:
                typedef struct playback_t
                {
                    Sample     *pSample;    // Pointer to the sample
                    ssize_t     nID;        // ID of playback
                    size_t      nChannel;   // Channel to play
                    ssize_t     nOffset;    // Current offset
                    ssize_t     nFadeout;   // Fadeout (cancelling)
                    ssize_t     nFadeOffset;// Fadeout offset
                    float       nVolume;    // The volume of the sample
                    playback_t *pNext;      // Pointer to the next playback in the list
                    playback_t *pPrev;      // Pointer to the previous playback in the list
                } playback_t;

                typedef struct list_t
                {
                    playback_t *pHead;      // The head of the list
                    playback_t *pTail;      // The tail of the list
                } list_t;

            private:
                Sample        **vSamples;
                size_t          nSamples;
                playback_t     *vPlayback;
                size_t          nPlayback;
                list_t          sActive;
                list_t          sInactive;
                float           fGain;

            protected:
                static inline void cleanup(playback_t *pb);
                static inline void list_remove(list_t *list, playback_t *pb);
                static inline playback_t *list_remove_first(list_t *list);
                static inline void list_add_first(list_t *list, playback_t *pb);
                static inline void list_insert_from_tail(list_t *list, playback_t *pb);
                void do_process(float *dst, size_t samples);

                static void dump_list(IStateDumper *v, const char *name, const list_t *list);

            public:
                explicit SamplePlayer();
                ~SamplePlayer();

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
                 * @param cascade destroy the bound samples
                 */
                void destroy(bool cascade = true);

            public:
                /** Set output gain
                 *
                 * @param gain output gain
                 */
                inline void set_gain(float gain) { fGain = gain; }

                /** Bind sample to specified ID, cancel all active playbacks previously associated
                 * with this sample
                 *
                 * @param id id of the sample
                 * @param sample pointer to the sample
                 * @return true on success, sample contains pointer to the replaced sample
                 */
                bool bind(size_t id, Sample **sample);

                /** Bind sample to specified ID, cancel all active playbacks previously associated
                 * with this sample
                 *
                 * @param id id of the sample
                 * @param sample pointer to the sample
                 * @param destroy auto-destroy sample
                 * @return true on success, previous sample will be automatically destroyed
                 */
                bool bind(size_t id, Sample *sample, bool destroy = false);

                /** Unbind sample
                 *
                 * @param id id of the sample
                 * @param sample pointer to store the unbound sample
                 * @return true on success, pointer to the sample will be stored in sample variable
                 */
                bool unbind(size_t id, Sample **sample);

                /** Unbind sample
                 *
                 * @param id id of the sample
                 * @param destroy destroy the sample
                 * @return true on success, the bound sample will be automatically destroyed
                 */
                bool unbind(size_t id, bool destroy = false);

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

                /** Softly cancel playback of the sample
                 *
                 * @param id ID of the sample
                 * @param channel ID of the sample's channel
                 * @param fadeout the fadeout length in samples for sample gain fadeout
                 * @param delay the delay (in samples) of the sample relatively to the next process() call
                 * @return number of playbacks cancelled, negative value on error
                 */
                ssize_t cancel_all(size_t id, size_t channel, size_t fadeout = 0, ssize_t delay = 0);

                /** Reset the playback state of the player, force all playbacks to be stopped
                 *
                 */
                void stop();

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_SAMPLEPLAYER_H_ */
