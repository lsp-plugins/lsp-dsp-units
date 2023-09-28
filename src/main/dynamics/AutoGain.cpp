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
#include <lsp-plug.in/dsp-units/dynamics/AutoGain.h>
#include <lsp-plug.in/dsp-units/units.h>

namespace lsp
{
    namespace dspu
    {
        AutoGain::AutoGain()
        {
            construct();
        }

        AutoGain::~AutoGain()
        {
            destroy();
        }

        void AutoGain::construct()
        {
            nSampleRate     = 0;
            nLookHead       = 0;
            nLookSize       = 0;
            nLookOffset     = 0;
            vLookBack       = NULL;

            sShort.fGrow    = 0.0f;
            sShort.fFall    = 0.0f;
            sShort.fKGrow   = 0.0f;
            sShort.fKFall   = 0.0f;
            sLong.fGrow     = 0.0f;
            sLong.fFall     = 0.0f;
            sLong.fKGrow    = 0.0f;
            sLong.fKFall    = 0.0f;

            sComp.x1        = GAIN_AMP_M_6_DB;
            sComp.x2        = GAIN_AMP_P_6_DB;
            sComp.a         = 0.0f;
            sComp.b         = 0.0f;
            sComp.c         = 0.0f;
            sComp.d         = 0.0f;

            fSilence        = GAIN_AMP_M_72_DB;
//            fDeviation      = GAIN_AMP_P_6_DB;
//            fRevDeviation   = GAIN_AMP_M_6_DB;
            fCurrEnv        = 0.0f;
            fCurrGain       = 1.0f;
            fMinGain        = GAIN_AMP_M_48_DB;
            fMaxGain        = GAIN_AMP_P_48_DB;
            fLookBack       = 0.0f;
            fMaxLookBack    = 0.0f;

            nFlags          = F_UPDATE;
        }

        void AutoGain::destroy()
        {
            if (vLookBack != NULL)
            {
                free(vLookBack);
                vLookBack       = NULL;
            }
        }

        status_t AutoGain::init(float max_lookback)
        {
            destroy();
            fMaxLookBack    = max_lookback;

            return STATUS_OK;
        }

        void AutoGain::set_timing(float *ptr, float value)
        {
            value           = lsp_max(value, 0.0f);
            if (*ptr == value)
                return;

            *ptr            = value;
            nFlags         |= F_UPDATE;
        }

        status_t AutoGain::set_sample_rate(size_t sample_rate)
        {
            if (nSampleRate == sample_rate)
                return STATUS_OK;

            // Reallocate look-back buffer
            size_t max_lookback = dspu::millis_to_samples(sample_rate, fMaxLookBack);
            size_t buf_len      = round_pow2(max_lookback + 1);

            float *ptr          = static_cast<float *>(realloc(vLookBack, sizeof(float) * buf_len));
            if (ptr == NULL)
                return STATUS_NO_MEM;
            vLookBack           = ptr;
            dsp::fill_zero(vLookBack, buf_len);

            // Update other settings
            nSampleRate         = sample_rate;
            nLookSize           = buf_len;
            nLookHead           = 0;
            nLookOffset         = 0;
            nFlags             |= F_UPDATE;

            return STATUS_OK;
        }

        void AutoGain::set_silence_threshold(float threshold)
        {
            fSilence        = lsp_max(0.0f, threshold);
        }

        void AutoGain::set_deviation(float deviation)
        {
            deviation       = lsp_max(1.0f, deviation);
            if (deviation == fDeviation)
                return;

            fDeviation      = deviation;
            nFlags         |= F_UPDATE;
        }

        void AutoGain::set_min_gain(float value)
        {
            fMinGain        = lsp_max(0.0f, value);
        }

        void AutoGain::set_max_gain(float value)
        {
            fMaxGain        = lsp_max(1.0f, value);
        }

        void AutoGain::set_gain(float min, float max)
        {
            fMinGain        = lsp_max(0.0f, min);
            fMaxGain        = lsp_max(1.0f, max);
        }

        void AutoGain::set_short_speed(float grow, float fall)
        {
            set_timing(&sShort.fGrow, grow);
            set_timing(&sShort.fFall, fall);
        }

        void AutoGain::set_long_speed(float grow, float fall)
        {
            set_timing(&sLong.fGrow, grow);
            set_timing(&sLong.fFall, fall);
        }

        void AutoGain::set_lookback(float value)
        {
            set_timing(&fLookBack, value);
        }

        size_t AutoGain::latency() const
        {
            if (nFlags & F_UPDATE)
            {
                float look_back     = lsp_limit(fLookBack, 0.0f, fMaxLookBack);
                return millis_to_samples(nSampleRate, look_back);
            }
            return nLookOffset;
        }

        void AutoGain::update()
        {
            if (!(nFlags & F_UPDATE))
                return;

            float ksr           = (M_LN10 / 20.0f) / nSampleRate;

            sShort.fKGrow       = expf(sShort.fGrow * ksr);
            sShort.fKFall       = expf(-sShort.fFall * ksr);
            sLong.fKGrow        = expf(sLong.fGrow * ksr);
            sLong.fKFall        = expf(-sLong.fFall * ksr);

            float look_back     = lsp_limit(fLookBack, 0.0f, fMaxLookBack);
            nLookOffset         = millis_to_samples(nSampleRate, look_back);

            calc_compressor();

            nFlags             &= ~size_t(F_UPDATE);
        }

        void AutoGain::calc_compressor()
        {
        /* non-optimized
            c.x1        = th / deviation;
            c.x2        = th * deviation;

            float y1    = c.x1;
            float y2    = th;
            float dy    = y2 - y1;
            float dx    = c.x2 - c.x1;
            float dx1   = 1.0f/dx;
            float dx2   = dx1*dx1;

            c.d         = y1;
            c.c         = 1.0f;
            c.b         = 3.0f*dy*dx2 - 2.0f*dx1;
            c.a         = (1.0f - 2.0*dy*dx1)*dx2;
        */

            sComp.x1    = 1.0f / fDeviation;
            sComp.x2    = fDeviation;

            float dy    = 1.0f - sComp.x1;
            float dx    = sComp.x2 - sComp.x1;
            float dx1   = 1.0f / dx;
            float dx2   = dx1 * dx1;

            sComp.d     = sComp.x1;
            sComp.c     = 1.0f;
            sComp.b     = 3.0f*dy*dx2 - 2.0f*dx1;
            sComp.a     = (1.0f - 2.0*dy*dx1)*dx2;

            lsp_trace("comp a=%f, b=%f, c=%f, d=%f",
                sComp.a, sComp.b, sComp.c, sComp.d);
        }

        float AutoGain::eval_curve(float x)
        {
            if (x >= sComp.x2)
                return 1.0f;
            if (x <= sComp.x1)
                return x;

            float v    = x - sComp.x1;
            return ((sComp.a*v + sComp.b)*v + sComp.c*v) + sComp.d;
        }

        float AutoGain::eval_gain(float x)
        {
            return eval_curve(x)/x;
        }

        float AutoGain::process_sample(float sl, float ss, float le)
        {
            // Do not perform any gain adjustment if we are in silence
            if (ss <= fSilence)
                return fCurrGain;

            float nl    = sl * fCurrGain;
            float ns    = ss * fCurrGain;

            // Reset surge flag if possible
            if (nFlags & F_SURGE)
            {
                if (fCurrGain * ss <= le * fDeviation)
                    nFlags     &= ~size_t(F_SURGE);
            }

            // Compute the short gain reduction, trigger surge if it grows rapidly
            float red   = eval_gain(ns/le);
            if (red * fDeviation < 1.0f)
                nFlags     |= F_SURGE;

            // Compute the final gain
            if (nFlags & F_SURGE)
                fCurrGain  *= sShort.fKFall;
            else if (nl > le)
                fCurrGain  *= sLong.fKFall;
            else if (nl < le)
                fCurrGain  *= sLong.fKGrow;

            return fCurrGain; // = gain;
        }

        void AutoGain::process(float *vca, const float *llong, const float *lshort, const float *lexp, size_t count)
        {
            update();

            for (size_t i=0; i<count; ++i)
                vca[i]  = process_sample(llong[i], lshort[i], lexp[i]);
        }

        void AutoGain::process(float *vca, const float *llong, const float *lshort, float lexp, size_t count)
        {
            update();

            for (size_t i=0; i<count; ++i)
                vca[i]  = process_sample(llong[i], lshort[i], lexp);
        }

        void AutoGain::dump(IStateDumper *v) const
        {
        }

    } /* namespace dspu */
} /* namespace lsp */



