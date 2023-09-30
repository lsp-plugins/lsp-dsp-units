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

            sShort.fGrow    = 0.0f;
            sShort.fFall    = 0.0f;
            sShort.fKGrow   = 0.0f;
            sShort.fKFall   = 0.0f;
            sLong.fGrow     = 0.0f;
            sLong.fFall     = 0.0f;
            sLong.fKGrow    = 0.0f;
            sLong.fKFall    = 0.0f;

            init_compressor(sShortComp);
            init_compressor(sOutComp);

            fSilence        = GAIN_AMP_M_72_DB;
            fDeviation      = GAIN_AMP_P_6_DB;
            fCurrGain       = 1.0f;

            nFlags          = F_UPDATE;
        }

        void AutoGain::destroy()
        {
        }

        void AutoGain::init_compressor(compressor_t &c)
        {
            c.x1            = GAIN_AMP_0_DB;
            c.x2            = GAIN_AMP_0_DB;
            c.t             = 1.0f;
            c.a             = 0.0f;
            c.b             = 0.0f;
            c.c             = 0.0f;
            c.d             = 0.0f;
        }

        status_t AutoGain::init()
        {
            destroy();

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

            nSampleRate         = sample_rate;
            nFlags             |= F_UPDATE;

            return STATUS_OK;
        }

        void AutoGain::set_silence_threshold(float threshold)
        {
            fSilence        = lsp_max(0.0f, threshold);
            lsp_trace("silence level = %f (%.2f LUFS)", fSilence, dspu::gain_to_lufs(fSilence));
        }

        void AutoGain::set_deviation(float deviation)
        {
            deviation       = lsp_max(1.0f, deviation);
            if (deviation == fDeviation)
                return;

            fDeviation      = deviation;
            nFlags         |= F_UPDATE;
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

        void AutoGain::set_max_gain(float value, bool enable)
        {
            fMaxGain        = lsp_max(0.0f, value);
            nFlags          = lsp_setflag(nFlags, F_MAX_GAIN, enable);
        }

        void AutoGain::set_max_gain(float value)
        {
            fMaxGain        = lsp_max(0.0f, value);
        }

        void AutoGain::enable_max_gain(bool enable)
        {
            nFlags          = lsp_setflag(nFlags, F_MAX_GAIN, enable);
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

            float q_deviation   = sqrtf(fDeviation);

            calc_compressor(sShortComp, 1.0f / fDeviation, fDeviation, 1.0f);
            calc_compressor(sOutComp, q_deviation, fDeviation * q_deviation, fDeviation);

            nFlags             &= ~size_t(F_UPDATE);
        }

        void AutoGain::enable_quick_amplifier(bool enable)
        {
            nFlags              = lsp_setflag(nFlags, F_QUICK_AMP, enable);
        }

        void AutoGain::calc_compressor(compressor_t &c, float x1, float x2, float y2)
        {
            c.x1        = x1;
            c.x2        = x2;

            float dy    = y2 - c.x1;
            float dx    = c.x2 - c.x1;
            float dx1   = 1.0f/dx;
            float dx2   = dx1*dx1;

            c.t         = y2;
            c.d         = c.x1;
            c.c         = 1.0f;
            c.b         = 3.0f*dy*dx2 - 2.0f*dx1;
            c.a         = (1.0f - 2.0*dy*dx1)*dx2;
        }

        float AutoGain::eval_curve(const compressor_t &c, float x)
        {
            if (x >= c.x2)
                return c.t;
            if (x <= c.x1)
                return x;

            float v    = x - c.x1;
            return ((c.a*v + c.b)*v + c.c*v) + c.d;
        }

        float AutoGain::eval_gain(const compressor_t &c, float x)
        {
            return eval_curve(c, x)/x;
        }

        float AutoGain::process_sample(float sl, float ss, float le)
        {
            // Do not perform any gain adjustment if we are in silence
            if (ss <= fSilence)
                return fCurrGain;

            // STAGE 1: perform gain adjustment
            float nl    = sl * fCurrGain;
            float ns    = ss * fCurrGain;

            // Reset surge flag if possible
            size_t srg  = nFlags & (F_SURGE_UP | F_SURGE_DOWN);
            if (srg == F_SURGE_UP)
            {
                if (ns <= le * fDeviation)
                    nFlags     &= ~size_t(F_SURGE_UP);
            }
            else if ((nFlags & F_QUICK_AMP) && (srg == F_SURGE_DOWN))
            {
                if (ns * fDeviation > le)
                    nFlags     &= ~size_t(F_SURGE_DOWN);
            }
            else
                nFlags     &= ~size_t(F_SURGE_UP | F_SURGE_DOWN);

            // Compute the short gain reduction, trigger surge if it grows rapidly
            float red   = eval_gain(sShortComp, ns/le);
            if (red * fDeviation < 1.0f)
                nFlags     |= F_SURGE_UP;
            else if ((nFlags & F_QUICK_AMP) && (ns * fDeviation <= le))
                nFlags     |= F_SURGE_DOWN;

            // Compute the final gain
            float gain  = fCurrGain;

            if (nFlags & F_SURGE_UP)
                gain       *= sShort.fKFall;
            else if (nFlags & F_SURGE_DOWN)
                gain       *= sShort.fKGrow;
            else
            {
                if (nl > le)
                    gain       *= sLong.fKFall;
                else if (nl < le)
                    gain       *= sLong.fKGrow;
            }

            // STAGE 2: perform gain clipping as protection from surges and pops
            red         = eval_gain(sOutComp, (ss * gain) / le);
            gain       *= red;

            return fCurrGain = gain;
        }

        void AutoGain::process(float *vca, const float *llong, const float *lshort, const float *lexp, size_t count)
        {
            // Update settings if needed
            update();

            // Process the samples
            for (size_t i=0; i<count; ++i)
                vca[i]  = process_sample(llong[i], lshort[i], lexp[i]);

            // Apply gain limitation at the output
            if (nFlags & F_MAX_GAIN)
                dsp::limit1(vca, 0.0f, fMaxGain, count);
        }

        void AutoGain::process(float *vca, const float *llong, const float *lshort, float lexp, size_t count)
        {
            // Update settings if needed
            update();

            // Process the samples
            for (size_t i=0; i<count; ++i)
                vca[i]  = process_sample(llong[i], lshort[i], lexp);

            // Apply gain limitation at the output
            if (nFlags & F_MAX_GAIN)
                dsp::limit1(vca, 0.0f, fMaxGain, count);
        }

        void AutoGain::dump(const char *id, const timing_t *t, IStateDumper *v)
        {
            v->begin_object(id, t, sizeof(timing_t));
            {
                v->write("fGrow", t->fGrow);
                v->write("fFall", t->fFall);
                v->write("fKGrow", t->fKGrow);
                v->write("fKFall", t->fKFall);
            }
            v->end_object();
        }

        void AutoGain::dump(const char *id, const compressor_t *c, IStateDumper *v)
        {
            v->begin_object(id, c, sizeof(compressor_t));
            {
                v->write("x1", c->x1);
                v->write("x2", c->x2);
                v->write("t", c->t);
                v->write("a", c->a);
                v->write("b", c->b);
                v->write("c", c->c);
                v->write("d", c->d);
            }
            v->end_object();
        }

        void AutoGain::dump(IStateDumper *v) const
        {
            v->write("nSampleRate", nSampleRate);
            v->write("nFlags", nFlags);

            dump("sShort", &sShort, v);
            dump("sLong", &sLong, v);
            dump("sShortComp", &sShortComp, v);
            dump("sOutComp", &sOutComp, v);

            v->write("fSilence", fSilence);
            v->write("fDeviation", fDeviation);
            v->write("fCurrGain", fCurrGain);
        }

    } /* namespace dspu */
} /* namespace lsp */



