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

            public:
                Playback & operator = (const Playback & src);

            public:
                /**
                 * Check that playback is still valid
                 * @return true if playback is still valid
                 */
                bool        valid() const;

                /**
                 * Get current time position of the playback inside of the audio sample
                 * @return current time position of the playback inside of the audio sample
                 */
                size_t      position() const;

                /**
                 * Get the actual playback timestamp since the playback event triggered
                 * @return the actual playback timestamp in samples
                 */
                wsize_t     timestamp() const;

                /**
                 * Stop the playback: disable any loops and let the sample sound till it ends.
                 */
                void        stop();

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
        };
    } /* namespace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_SAMPLING_PLAYBACK_H_ */
