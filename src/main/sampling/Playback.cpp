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

#include <lsp-plug.in/dsp-units/sampling/Playback.h>

namespace lsp
{
    namespace dspu
    {
        Playback::Playback()
        {
            pPlayback       = NULL;
            nSerial         = 0;
        }

        Playback::Playback(playback::playback_t *pb)
        {
            pPlayback       = pb;
            nSerial         = (pb != NULL) ? pb->nSerial : 0;
        }

        Playback::Playback(const Playback *src)
        {
            if (src != NULL)
            {
                pPlayback       = src->pPlayback;
                nSerial         = src->nSerial;
            }
            else
            {
                pPlayback       = NULL;
                nSerial         = 0;
            }
        }

        Playback::Playback(const Playback &src)
        {
            pPlayback       = src.pPlayback;
            nSerial         = src.nSerial;
        }

        Playback::Playback(Playback &&src)
        {
            pPlayback       = src.pPlayback;
            nSerial         = src.nSerial;
            src.pPlayback   = NULL;
            src.nSerial     = 0;
        }

        void Playback::construct()
        {
            pPlayback       = NULL;
            nSerial         = 0;
        }

        void Playback::destroy()
        {
            pPlayback       = NULL;
            nSerial         = 0;
        }

        Playback::~Playback()
        {
            destroy();
        }

        Playback & Playback::operator = (const Playback & src)
        {
            pPlayback       = src.pPlayback;
            nSerial         = src.nSerial;
            return *this;
        }

        bool Playback::valid() const
        {
            return (pPlayback != NULL) && (pPlayback->nSerial == nSerial);
        }

        void Playback::stop(size_t delay)
        {
            if (!valid())
                return;

            playback::stop_playback(pPlayback, delay);
        }

        void Playback::clear()
        {
            pPlayback       = NULL;
            nSerial         = 0;
        }

        void Playback::cancel(size_t fadeout, size_t delay)
        {
            if (!valid())
                return;

            playback::cancel_playback(pPlayback, fadeout, delay);
        }

        void Playback::copy(const Playback & src)
        {
            pPlayback       = src.pPlayback;
            nSerial         = src.nSerial;
        }

        void Playback::copy(const Playback *src)
        {
            pPlayback       = src->pPlayback;
            nSerial         = src->nSerial;
        }

        void Playback::set(const Playback &src)
        {
            copy(src);
        }

        void Playback::set(const Playback *src)
        {
            copy(src);
        }

        void Playback::swap(Playback *src)
        {
            lsp::swap(pPlayback, src->pPlayback);
            lsp::swap(nSerial, src->nSerial);
        }

        void Playback::swap(Playback & src)
        {
            lsp::swap(pPlayback, src.pPlayback);
            lsp::swap(nSerial, src.nSerial);
        }


        wsize_t Playback::timestamp() const
        {
            return (valid()) ? pPlayback->nTimestamp : 0;
        }

        const Sample *Playback::sample() const
        {
            return (valid()) ? pPlayback->pSample : NULL;
        }

        size_t Playback::id() const
        {
            return (valid()) ? pPlayback->nID : 0;
        }

        size_t Playback::channel() const
        {
            return (valid()) ? pPlayback->nChannel : 0;
        }

        float Playback::volume() const
        {
            return (valid()) ? pPlayback->fVolume : 0.0f;
        }

        ssize_t Playback::position() const
        {
            return (valid()) ? pPlayback->nPosition : -1;
        }

        sample_loop_t Playback::loop_mode() const
        {
            return (valid()) ? pPlayback->enLoopMode : SAMPLE_LOOP_NONE;
        }

        size_t Playback::loop_start() const
        {
            return (valid()) ? pPlayback->nLoopStart : 0;
        }

        size_t Playback::loop_end() const
        {
            return (valid()) ? pPlayback->nLoopEnd : 0;
        }

        size_t Playback::crossfade_length() const
        {
            return (valid()) ? pPlayback->nXFade : 0;
        }

        size_t Playback::xfade_length() const
        {
            return crossfade_length();
        }

        sample_crossfade_t Playback::crossfade_type() const
        {
            return (valid()) ? pPlayback->enXFadeType : SAMPLE_CROSSFADE_LINEAR;
        }

        sample_crossfade_t Playback::xfade_type() const
        {
            return crossfade_type();
        }

        void Playback::dump(IStateDumper *v) const
        {
            v->write("pPlayback", pPlayback);
            v->write("nSerial", nSerial);
        }
    } /* namespace dspu */
} /* namespace lsp */

