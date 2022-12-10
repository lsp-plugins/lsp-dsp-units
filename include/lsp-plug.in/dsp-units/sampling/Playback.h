/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 18 нояб. 2022 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_SAMPLING_PLAYBACK_H_
#define LSP_PLUG_IN_DSP_UNITS_SAMPLING_PLAYBACK_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/sampling/types.h>
#include <lsp-plug.in/dsp-units/sampling/helpers/batch.h>
#include <lsp-plug.in/dsp-units/sampling/helpers/playback.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Playback class that allows to control the flow of the playback
         */
        class LSP_DSP_UNITS_PUBLIC Playback
        {
            protected:
                playback::playback_t   *pPlayback;
                size_t                  nSerial;

            public:
                Playback();
                explicit Playback(playback::playback_t *pb);
                explicit Playback(const Playback *src);
                Playback(const Playback &src);
                Playback(Playback &&src);
                ~Playback();

                void        construct();
                void        destroy();

            public:
                Playback & operator = (const Playback & src);

            public:
                /**
                 * Check that playback is still valid. Should be called before reading playback parameters
                 * to ensure that these parameters are properly read.
                 * @return true if playback is still valid
                 */
                bool        valid() const;

                /**
                 * Stop the playback: disable any loops and let the sample sound till it ends.
                 */
                void        stop(size_t delay = 0);

                /**
                 * Forget anything about current playback
                 */
                void        clear();

                /**
                 * Cancel the playback: set-up the delay and fade-out length before the sample stops playing
                 * @param fadeout the fadeout length in samples for sample gain fadeout
                 * @param delay the delay (in samples) of the sample relatively to the current playback time
                 */
                void        cancel(size_t fadeout = 0, size_t delay = 0);

                /**
                 * Copy from another playback
                 * @param src source playback to copy
                 */
                void        copy(const Playback &src);

                /**
                 * Copy from another playback
                 * @param src source playback to copy
                 */
                void        copy(const Playback *src);

                /**
                 * Swap with another playback
                 * @param src playback to swap
                 */
                void        swap(Playback *src);

                /**
                 * Swap with another playback
                 * @param src playback to swap
                 */
                void        swap(Playback & src);

                /**
                 * Set data from another playback, sampe as copy
                 * @param src source playback to read
                 */
                void        set(const Playback &src);

                /**
                 * Set data from another playback, sampe as copy
                 * @param src source playback to read
                 */
                void        set(const Playback *src);

                /**
                 * Dump the state of the playback
                 * @param v state dumper
                 */
                void        dump(IStateDumper *v) const;

            public:

                /**
                 * Get the actual playback timestamp since the playback event triggered
                 * @return the actual playback timestamp in samples
                 */
                wsize_t     timestamp() const;

                /**
                 * Obtain the current sample played
                 * @return current sample played
                 */
                const Sample   *sample() const;

                /**
                 * Get the ID of the instrument played
                 * @return identifier of the instrument played
                 */
                size_t      id() const;

                /**
                 * Return the channel played
                 * @return channel played
                 */
                size_t      channel() const;

                /**
                 * Get the volume of the playback
                 * @return volume of the playback
                 */
                float       volume() const;

                /**
                 * Check that the playback is working in reversive mode (playing from the tail
                 * of the sample to the head)
                 * @return true if playback is reversive
                 */
                bool        reversive() const;

                /**
                 * Get current time position of the playback inside of the audio sample
                 * @return current time position of the playback inside of the audio sample
                 */
                ssize_t     position() const;

                /**
                 * Get the loop mode
                 * @return loop mode
                 */
                sample_loop_t loop_mode() const;

                /**
                 * Get the start of the loop
                 * @return start of the loop
                 */
                size_t      loop_start() const;

                /**
                 * Get the end of the loop
                 * @return end of the loop
                 */
                size_t      loop_end() const;

                /**
                 * Get the crossfade length
                 * @return crossfade length
                 */
                size_t      crossfade_length() const;
                size_t      xfade_length() const;

                /**
                 * Get the crossfade type
                 * @return crossfade type
                 */
                sample_crossfade_t crossfade_type() const;
                sample_crossfade_t xfade_type() const;
        };
    } /* namespace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_PLAYBACK_H_ */
