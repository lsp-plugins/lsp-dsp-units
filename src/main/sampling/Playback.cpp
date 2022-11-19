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

        Playback::Playback(playback_t *pb)
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

        Playback::~Playback()
        {
            pPlayback       = NULL;
            nSerial         = 0;
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

        size_t Playback::offset() const
        {
            if (!valid())
                return -1;
            return pPlayback->nOffset;
        }

        wsize_t Playback::timestamp() const
        {
            if (!valid())
                return 0;
            return pPlayback->nTimestamp;
        }

        void Playback::cancel(size_t fadeout, ssize_t delay)
        {
            if (!valid())
                return;

            // Do not cancel the playback if it already has been cancelled
            if ((pPlayback->pSample != NULL) &&
                (pPlayback->nFadeout < 0))
            {
                pPlayback->nFadeout    = fadeout;
                pPlayback->nFadeOffset = -delay;
            }
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

    } /* namespace dspu */
} /* namespace lsp */

