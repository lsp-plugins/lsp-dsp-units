/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 сент. 2023 г.
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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/bits.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/dynamics/SimpleAutoGain.h>
#include <lsp-plug.in/dsp-units/units.h>

namespace lsp
{
    namespace dspu
    {
        SimpleAutoGain::SimpleAutoGain()
        {
            construct();
        }

        SimpleAutoGain::~SimpleAutoGain()
        {
            destroy();
        }

        void SimpleAutoGain::construct()
        {
            nSampleRate     = 0;
            nFlags          = F_UPDATE;

            fKGrow          = 0.0f;
            fKFall          = 0.0f;
            fGrow           = 0.0f;
            fFall           = 0.0f;
            fThreshold      = 0.0f;
            fCurrGain       = 1.0f;
            fMinGain        = 0.000001f;
            fMaxGain        = 1.0f;
        }

        void SimpleAutoGain::destroy()
        {
        }

        status_t SimpleAutoGain::init()
        {
            destroy();
            return STATUS_OK;
        }

        status_t SimpleAutoGain::set_sample_rate(size_t sample_rate)
        {
            if (nSampleRate == sample_rate)
                return STATUS_OK;

            nSampleRate         = uint32_t(sample_rate);
            nFlags             |= F_UPDATE;

            return STATUS_OK;
        }

        void SimpleAutoGain::set_grow(float value)
        {
            if (fGrow == value)
                return;

            fGrow               = value;
            nFlags             |= F_UPDATE;
        }

        void SimpleAutoGain::set_fall(float value)
        {
            if (fFall == value)
                return;

            fFall               = value;
            nFlags             |= F_UPDATE;
        }

        void SimpleAutoGain::set_speed(float grow, float fall)
        {
            if ((fGrow == grow) &&
                (fFall == fall))
                return;

            fGrow               = grow;
            fFall               = fall;
            nFlags             |= F_UPDATE;
        }

        void SimpleAutoGain::set_max_gain(float value)
        {
            if (fMaxGain == value)
                return;

            fMaxGain            = value;
            fCurrGain           = lsp_min(fCurrGain, fMaxGain);
        }

        void SimpleAutoGain::set_min_gain(float value)
        {
            if (fMinGain == value)
                return;

            fMinGain            = value;
            fCurrGain           = lsp_max(fCurrGain, fMinGain);
        }

        void SimpleAutoGain::set_gain(float min, float max)
        {
            if ((fMinGain == min) &&
                (fMaxGain == max))
                return;

            fMinGain            = min;
            fMaxGain            = max;
            fCurrGain           = lsp_limit(fCurrGain, fMinGain, fMaxGain);
        }

        void SimpleAutoGain::set_threshold(float threshold)
        {
            fThreshold          = threshold;
        }

        void SimpleAutoGain::update()
        {
            if (!(nFlags & F_UPDATE))
                return;

            float ksr           = (M_LN10 * 0.05f) / nSampleRate;

            fKGrow              = expf(fGrow * ksr);
            fKFall              = expf(-fFall * ksr);

            nFlags             &= ~uint32_t(F_UPDATE);
        }

        void SimpleAutoGain::process(float *dst, const float *src, size_t count)
        {
            // Update settings if needed
            update();

            // Process the samples
            float cgain     = fCurrGain;
            for (size_t i=0; i<count; ++i)
            {
                const float s   = src[i] * cgain;

                if (s < fThreshold)
                    cgain      *= fKGrow;
                else if (s > fThreshold)
                    cgain      *= fKFall;

                cgain       = lsp_limit(cgain, fMinGain, fMaxGain);
                dst[i]      = cgain;
            }
            fCurrGain       = cgain;
        }

        float SimpleAutoGain::process(float src)
        {
            // Update settings if needed
            update();

            float cgain     = fCurrGain;
            const float s   = src * cgain;

            if (s < fThreshold)
                cgain      *= fKGrow;
            else if (s > fThreshold)
                cgain      *= fKFall;

            cgain       = lsp_limit(cgain, fMinGain, fMaxGain);
            fCurrGain   = cgain;

            return cgain;
        }

        void SimpleAutoGain::dump(IStateDumper *v) const
        {
            v->write("nSampleRate", nSampleRate);
            v->write("nFlags", nFlags);

            v->write("fKGrow", fKGrow);
            v->write("fKFall", fKFall);
            v->write("fGrow", fGrow);
            v->write("fFall", fFall);
            v->write("fThreshold", fThreshold);
            v->write("fCurrGain", fCurrGain);
            v->write("fMinGain", fMinGain);
            v->write("fMaxGain", fMaxGain);
        }

    } /* namespace dspu */
} /* namespace lsp */



