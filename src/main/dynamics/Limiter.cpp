/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 25 нояб. 2016 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp-units/dynamics/Limiter.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/dsp-units/misc/interpolation.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/common/alloc.h>

#define BUF_GRANULARITY         8192U
#define GAIN_LOWERING           0.9886 /*0.891250938134 */
#define MIN_LIMITER_RELEASE     5.0f

namespace lsp
{
    namespace dspu
    {
        Limiter::Limiter()
        {
            construct();
        }

        Limiter::~Limiter()
        {
            destroy();
        }

        void Limiter::construct()
        {
            fThreshold      = GAIN_AMP_0_DB;
            fReqThreshold   = GAIN_AMP_0_DB;
            fLookahead      = 0.0f;
            fMaxLookahead   = 0.0f;
            fAttack         = 0.0f;
            fRelease        = 0.0f;
            fKnee           = GAIN_AMP_M_6_DB;
            nMaxLookahead   = 0;
            nLookahead      = 0;
            nHead           = 0;
            nMaxSampleRate  = 0;
            nSampleRate     = 0;
            nUpdate         = UP_ALL;
            nMode           = LM_HERM_THIN;

            sALR.fAttack    = 10.0f;
            sALR.fRelease   = 50.0f;
            sALR.fEnvelope  = 0.0f;
            sALR.bEnable    = false;

            vGainBuf        = NULL;
            vTmpBuf         = NULL;
            vData           = NULL;
        }

        void Limiter::destroy()
        {
            if (vData != NULL)
            {
                free_aligned(vData);
                vData = NULL;
            }

            vGainBuf    = NULL;
            vTmpBuf     = NULL;
        }

        bool Limiter::init(size_t max_sr, float max_lookahead)
        {
            nMaxLookahead       = millis_to_samples(max_sr, max_lookahead);
            nHead               = 0;
            size_t buf_gap      = nMaxLookahead*8;
            size_t buf_size     = buf_gap + nMaxLookahead*4 + BUF_GRANULARITY;
            size_t alloc        = buf_size + BUF_GRANULARITY;
            float *ptr          = alloc_aligned<float>(vData, alloc, DEFAULT_ALIGN);
            if (ptr == NULL)
                return false;

            vGainBuf            = ptr;
            ptr                += buf_size;
            vTmpBuf             = ptr;
            ptr                += BUF_GRANULARITY;

            dsp::fill_one(vGainBuf, buf_size);
            dsp::fill_zero(vTmpBuf, BUF_GRANULARITY);

            nMaxSampleRate      = max_sr;
            fMaxLookahead       = max_lookahead;
            return true;
        }

        float Limiter::set_attack(float attack)
        {
            float old = fAttack;
            if (attack == old)
                return old;

            fAttack         = attack;
            nUpdate        |= UP_OTHER;
            return old;
        }

        float Limiter::set_release(float release)
        {
            float old = fRelease;
            if (release == old)
                return old;

            fRelease        = release;
            nUpdate        |= UP_OTHER;
            return old;
        }

        float Limiter::set_threshold(float thresh, bool immediate)
        {
            float old = fReqThreshold;
            if (old == thresh)
                return old;

            fReqThreshold   = thresh;
            if (immediate)
                fThreshold      = thresh;
            nUpdate        |= UP_THRESH | UP_ALR;
            return old;
        }

        void Limiter::set_mode(limiter_mode_t mode)
        {
            if (mode == nMode)
                return;
            nMode = mode;
            nUpdate |= UP_MODE;
        }

        void Limiter::set_sample_rate(size_t sr)
        {
            if (sr == nSampleRate)
                return;

            nSampleRate     = sr;
            nLookahead      = millis_to_samples(nSampleRate, fLookahead);
            nUpdate        |= UP_SR;
        }

        float Limiter::set_lookahead(float lk_ahead)
        {
            float old   = fLookahead;
            lk_ahead    = lsp_min(lk_ahead, fMaxLookahead);
            if (old == lk_ahead)
                return old;

            fLookahead      = lk_ahead;
            nUpdate        |= UP_LK;
            nLookahead      = millis_to_samples(nSampleRate, fLookahead);

            return old;
        }

        float Limiter::set_knee(float knee)
        {
            float old = fKnee;
            if (old == knee)
                return old;

            fKnee           = knee;
            nUpdate        |= UP_ALR;
            return old;
        }

        float Limiter::set_alr_attack(float attack)
        {
            float old = sALR.fAttack;
            if (attack == old)
                return old;

            sALR.fAttack    = attack;
            nUpdate        |= UP_ALR;
            return old;
        }

        float Limiter::set_alr_release(float release)
        {
            float old = sALR.fRelease;
            if (release == old)
                return old;

            sALR.fRelease   = release;
            nUpdate        |= UP_ALR;
            return old;
        }

        bool Limiter::set_alr(bool enable)
        {
            bool old        = sALR.bEnable;
            sALR.bEnable    = enable;
            if (!enable)
                sALR.fEnvelope  = 0.0f;
            return old;
        }

        void Limiter::reset_sat(sat_t *sat)
        {
            sat->nAttack        = 0;
            sat->nPlane         = 0;
            sat->nRelease       = 0;
            sat->nMiddle        = 0;

            sat->vAttack[0]     = 0.0f;
            sat->vAttack[1]     = 0.0f;
            sat->vAttack[2]     = 0.0f;
            sat->vAttack[3]     = 0.0f;
            sat->vRelease[0]    = 0.0f;
            sat->vRelease[1]    = 0.0f;
            sat->vRelease[2]    = 0.0f;
            sat->vRelease[3]    = 0.0f;
        }

        void Limiter::reset_exp(exp_t *exp)
        {
            exp->nAttack        = 0;
            exp->nPlane         = 0;
            exp->nRelease       = 0;
            exp->nMiddle        = 0;

            exp->vAttack[0]     = 0.0f;
            exp->vAttack[1]     = 0.0f;
            exp->vAttack[2]     = 0.0f;
            exp->vAttack[3]     = 0.0f;
            exp->vRelease[0]    = 0.0f;
            exp->vRelease[1]    = 0.0f;
            exp->vRelease[2]    = 0.0f;
            exp->vRelease[3]    = 0.0f;
        }

        void Limiter::reset_line(line_t *line)
        {
            line->nAttack       = 0;
            line->nPlane        = 0;
            line->nRelease      = 0;
            line->nMiddle       = 0;

            line->vAttack[0]    = 0.0f;
            line->vAttack[1]    = 0.0f;
            line->vRelease[0]   = 0.0f;
            line->vRelease[1]   = 0.0f;
        }

        void Limiter::init_sat(sat_t *sat)
        {
            ssize_t attack      = millis_to_samples(nSampleRate, fAttack);
            ssize_t release     = millis_to_samples(nSampleRate, fRelease);

            attack              = lsp_limit(attack, 8, ssize_t(nLookahead));
            release             = lsp_limit(attack, 8, ssize_t(nLookahead*2));

            if (nMode == LM_HERM_THIN)
            {
                sat->nAttack        = int32_t(attack);
                sat->nPlane         = int32_t(attack);
            }
            else if (nMode == LM_HERM_TAIL)
            {
                sat->nAttack        = int32_t(attack / 2);
                sat->nPlane         = int32_t(attack);
            }
            else if (nMode == LM_HERM_DUCK)
            {
                sat->nAttack        = int32_t(attack);
                sat->nPlane         = int32_t(attack + (release / 2));
            }
            else // LM_HERM_WIDE by default
            {
                sat->nAttack        = int32_t(attack / 2);
                sat->nPlane         = int32_t(attack + (release / 2));
            }

            sat->nRelease       = int32_t(attack + release + 1);
            sat->nMiddle        = int32_t(attack);

            interpolation::hermite_cubic(sat->vAttack, -1.0f, 0.0f, 0.0f, sat->nAttack, 1.0f, 0.0f);
            interpolation::hermite_cubic(sat->vRelease, sat->nPlane, 1.0f, 0.0f, sat->nRelease, 0.0f, 0.0f);
        }

        void Limiter::init_exp(exp_t *exp)
        {
            ssize_t attack      = millis_to_samples(nSampleRate, fAttack);
            ssize_t release     = millis_to_samples(nSampleRate, fRelease);
            if (attack > ssize_t(nLookahead))
                attack              = nLookahead;
            else if (attack < 8)
                attack              = 8;
            if (release > ssize_t(nLookahead*2))
                release             = nLookahead*2;
            else if (release < 8)
                release             = 8;

            if (nMode == LM_HERM_THIN)
            {
                exp->nAttack        = int32_t(attack);
                exp->nPlane         = int32_t(attack);
            }
            else if (nMode == LM_HERM_TAIL)
            {
                exp->nAttack        = int32_t(attack / 2);
                exp->nPlane         = int32_t(attack);
            }
            else if (nMode == LM_HERM_DUCK)
            {
                exp->nAttack        = int32_t(attack);
                exp->nPlane         = int32_t(attack + (release / 2));
            }
            else // LM_HERM_WIDE by default
            {
                exp->nAttack        = int32_t(attack / 2);
                exp->nPlane         = int32_t(attack + (release / 2));
            }

            exp->nRelease       = int32_t(attack + release + 1);
            exp->nMiddle        = int32_t(attack);

            interpolation::exponent(exp->vAttack, -1.0f, 0.0f, exp->nAttack, 1.0f, 2.0f / attack);
            interpolation::exponent(exp->vRelease, exp->nPlane, 1.0f, exp->nRelease, 0.0f, 2.0f / release);
        }

        void Limiter::init_line(line_t *line)
        {
            ssize_t attack      = millis_to_samples(nSampleRate, fAttack);
            ssize_t release     = millis_to_samples(nSampleRate, fRelease);
            if (attack > ssize_t(nLookahead))
                attack              = nLookahead;
            else if (attack < 8)
                attack              = 8;
            if (release > ssize_t(nLookahead*2))
                release             = nLookahead*2;
            else if (release < 8)
                release             = 8;

            if (nMode == LM_LINE_THIN)
            {
                line->nAttack       = int32_t(attack);
                line->nPlane        = int32_t(attack);
            }
            else if (nMode == LM_LINE_TAIL)
            {
                line->nAttack       = int32_t(attack / 2);
                line->nPlane        = int32_t(attack);
            }
            else if (nMode == LM_LINE_DUCK)
            {
                line->nAttack       = int32_t(attack);
                line->nPlane        = int32_t(attack + (release / 2));
            }
            else // LM_LINE_WIDE by default
            {
                line->nAttack       = int32_t(attack / 2);
                line->nPlane        = int32_t(attack + (release / 2));
            }

            line->nRelease      = int32_t(attack + release + 1);
            line->nMiddle       = int32_t(attack);

            interpolation::linear(line->vAttack, -1.0f, 0.0f, line->nAttack, 1.0f);
            interpolation::linear(line->vRelease, line->nPlane, 1.0f, line->nRelease, 0.0f);
        }

        void Limiter::update_settings()
        {
            if (nUpdate == 0)
                return;

            // Update delay settings
            float *gbuf = &vGainBuf[nHead];
            if (nUpdate & UP_SR)
                dsp::fill_one(gbuf, nMaxLookahead*3 + BUF_GRANULARITY);

            nLookahead          = millis_to_samples(nSampleRate, fLookahead);

            // Update threshold
            if (nUpdate & UP_THRESH)
            {
                if (fReqThreshold < fThreshold)
                {
                    // Need to lower gain sinc threshold has been lowered
                    float gnorm         = fReqThreshold / fThreshold;
                    dsp::mul_k2(gbuf, gnorm, nMaxLookahead);
                }

                fThreshold          = fReqThreshold;
            }

            // Update automatic level regulation
            if (nUpdate & UP_ALR)
            {
                float thresh        = fThreshold * fKnee * GAIN_AMP_M_6_DB;
                sALR.fKS            = thresh * (M_SQRT2 - 1.0f);
                sALR.fKE            = thresh;
                sALR.fGain          = thresh * M_SQRT1_2;
                interpolation::hermite_quadratic(sALR.vHermite, sALR.fKS, sALR.fKS, 1.0f, thresh, 0.0f);

                float att           = millis_to_samples(nSampleRate, sALR.fAttack);
                float rel           = millis_to_samples(nSampleRate, sALR.fRelease);

                sALR.fTauAttack     = (att < 1.0f)  ? 1.0f : 1.0f - expf(logf(1.0f - M_SQRT1_2) / att);
                sALR.fTauRelease    = (rel < 1.0f)  ? 1.0f : 1.0f - expf(logf(1.0f - M_SQRT1_2) / rel);
            }

            // Check that mode change has triggered
            if (nUpdate & UP_MODE)
            {
                // Clear state for the limiter
                switch (nMode)
                {
                    case LM_HERM_THIN:
                    case LM_HERM_WIDE:
                    case LM_HERM_TAIL:
                    case LM_HERM_DUCK:
                        reset_sat(&sSat);
                        break;

                    case LM_EXP_THIN:
                    case LM_EXP_WIDE:
                    case LM_EXP_TAIL:
                    case LM_EXP_DUCK:
                        reset_exp(&sExp);
                        break;

                    case LM_LINE_THIN:
                    case LM_LINE_WIDE:
                    case LM_LINE_TAIL:
                    case LM_LINE_DUCK:
                        reset_line(&sLine);
                        break;

                    default:
                        break;
                }
            }

            // Update state
            switch (nMode)
            {
                case LM_HERM_THIN:
                case LM_HERM_WIDE:
                case LM_HERM_TAIL:
                case LM_HERM_DUCK:
                    init_sat(&sSat);
                    break;

                case LM_EXP_THIN:
                case LM_EXP_WIDE:
                case LM_EXP_TAIL:
                case LM_EXP_DUCK:
                    init_exp(&sExp);
                    break;

                case LM_LINE_THIN:
                case LM_LINE_WIDE:
                case LM_LINE_TAIL:
                case LM_LINE_DUCK:
                    init_line(&sLine);
                    break;

                default:
                    break;
            }

            // Clear the update flag
            nUpdate         = 0;
        }

        inline float Limiter::sat(ssize_t n)
        {
            if (n < sSat.nAttack)
            {
                if (n < 0)
                    return 0.0f;
                float x = n;
                return (((sSat.vAttack[0]*x + sSat.vAttack[1])*x + sSat.vAttack[2])*x + sSat.vAttack[3]);
            }
            else if (n > sSat.nPlane)
            {
                if (n > sSat.nRelease)
                    return 0.0f;

                float x = n;
                return (((sSat.vRelease[0]*x + sSat.vRelease[1])*x + sSat.vRelease[2])*x + sSat.vRelease[3]);
            }

            return 1.0f;
        }

        inline float Limiter::exp(ssize_t n)
        {
            if (n < sExp.nAttack)
            {
                if (n < 0)
                    return 0.0f;
                return sExp.vAttack[0] + sExp.vAttack[1] * expf(sExp.vAttack[2] * n);
            }
            else if (n > sExp.nPlane)
            {
                if (n > sExp.nRelease)
                    return 0.0f;

                return sExp.vRelease[0] + sExp.vRelease[1] * expf(sExp.vRelease[2] * n);
            }

            return 1.0f;
        }

        inline float Limiter::line(ssize_t n)
        {
            if (n < sLine.nAttack)
            {
                if (n < 0)
                    return 0.0f;
                return sLine.vAttack[0] * n + sLine.vAttack[1];
            }
            else if (n > sLine.nPlane)
            {
                if (n > sLine.nRelease)
                    return 0.0f;

                return sLine.vRelease[0] * n + sLine.vRelease[1];
            }

            return 1.0f;
        }

        void Limiter::apply_sat_patch(sat_t *sat, float *dst, float amp)
        {
            ssize_t t = 0;

            // Attack part
            while (t < sat->nAttack)
            {
                float x     = t++;
                *(dst++)   *= 1.0f - amp * (((sat->vAttack[0]*x + sat->vAttack[1])*x + sat->vAttack[2])*x + sat->vAttack[3]);
            }

            // Peak part
            while (t < sat->nPlane)
            {
                *(dst++)   *= 1.0f - amp;
                t++;
            }

            // Release part
            while (t < sat->nRelease)
            {
                float x     = t++;
                *(dst++)   *= 1.0f - amp * (((sat->vRelease[0]*x + sat->vRelease[1])*x + sat->vRelease[2])*x + sat->vRelease[3]);
            }
        }

        void Limiter::apply_exp_patch(exp_t *exp, float *dst, float amp)
        {
            ssize_t t = 0;

            // Attack part
            while (t < exp->nAttack)
                *(dst++)   *= 1.0f - amp * (exp->vAttack[0] + exp->vAttack[1] * expf(exp->vAttack[2] * (t++)));

            // Peak part
            while (t < exp->nPlane)
            {
                *(dst++)   *= 1.0f - amp;
                t++;
            }

            // Release part
            while (t < exp->nRelease)
                *(dst++)   *= 1.0f - amp * (exp->vRelease[0] + exp->vRelease[1] * expf(exp->vRelease[2] * (t++)));
        }

        void Limiter::apply_line_patch(line_t *line, float *dst, float amp)
        {
            ssize_t t = 0;

            // Attack part
            while (t < line->nAttack)
                *(dst++)   *= 1.0f - amp * (line->vAttack[0] * (t++) + line->vAttack[1]);

            // Peak part
            while (t < line->nPlane)
            {
                *(dst++)   *= 1.0f - amp;
                t++;
            }

            // Release part
            while (t < line->nRelease)
                *(dst++)   *= 1.0f - amp * (line->vRelease[0] * (t++) + line->vRelease[1]);
        }

        void Limiter::process_alr(float *gbuf, const float *sc, size_t samples)
        {
            float e = sALR.fEnvelope;

            for (size_t i=0; i<samples; ++i)
            {
                float s     = sc[i];
                e          += (s > e) ? sALR.fTauAttack * (s - e) : sALR.fTauRelease * (s - e);

                if (e >= sALR.fKE)
                    gbuf[i]    *= sALR.fGain / e;
                else if (e > sALR.fKS)
                    gbuf[i]    *= sALR.vHermite[0]*e + sALR.vHermite[1] + sALR.vHermite[2] / e;
//                else
//                    gbuf[i]    *= 1.0f;
            }

            sALR.fEnvelope  = e;
        }

        void Limiter::process(float *gain, const float *sc, size_t samples)
        {
            // Force settings update if there are any
            update_settings();

            size_t buf_gap      = nMaxLookahead*8;
            while (samples > 0)
            {
                size_t to_do    = lsp_min(samples, BUF_GRANULARITY);
                float *gbuf     = &vGainBuf[nHead + nMaxLookahead];

                // Fill gain buffer
                dsp::fill_one(&gbuf[nMaxLookahead*3], to_do);
                dsp::abs_mul3(vTmpBuf, gbuf, sc, to_do);    // Apply current gain buffer to the side chain signal
                if (sALR.bEnable) // Apply ALR if necessary
                {
                    process_alr(gbuf, vTmpBuf, to_do);
                    dsp::abs_mul3(vTmpBuf, gbuf, sc, to_do);    // Apply gain to sidechain
                }

                float knee          = 1.0f;
                size_t iterations   = 0;

                while (true)
                {
                    // Find peak
                    ssize_t peak    = dsp::max_index(vTmpBuf, to_do);
                    float s         = vTmpBuf[peak];
                    if (s <= fThreshold) // No more peaks are present
                        break;

                    // Apply patch to the gain buffer
                    float k         = (s - (fThreshold * knee - 0.000001f)) / s;
//                    lsp_trace("patch: this=%p, peak_idx=%d, s=%f (%f), thresh=%f (%f), k=%f (%f)",
//                        this,
//                        int(peak),
//                        dspu::gain_to_db(s), s,
//                        dspu::gain_to_db(fThreshold), fThreshold,
//                        dspu::gain_to_db(k), k);

                    switch (nMode)
                    {
                        case LM_HERM_THIN:
                        case LM_HERM_WIDE:
                        case LM_HERM_TAIL:
                        case LM_HERM_DUCK:
                            apply_sat_patch(&sSat, &gbuf[peak - sSat.nMiddle], k);
                            break;

                        case LM_EXP_THIN:
                        case LM_EXP_WIDE:
                        case LM_EXP_TAIL:
                        case LM_EXP_DUCK:
                            apply_exp_patch(&sExp, &gbuf[peak - sExp.nMiddle], k);
                            break;

                        case LM_LINE_THIN:
                        case LM_LINE_WIDE:
                        case LM_LINE_TAIL:
                        case LM_LINE_DUCK:
                            apply_line_patch(&sLine, &gbuf[peak - sLine.nMiddle], k);
                            break;

                        default:
                            break;
                    }

                    // Apply new gain to sidechain
                    dsp::abs_mul3(vTmpBuf, gbuf, sc, to_do);    // Apply gain to sidechain

                    // Lower the knee if necessary
                    if (((++iterations) % LIMITER_PEAKS_MAX) == 0)
                        knee     *=       GAIN_LOWERING;
                }

                // Copy gain value and shift gain buffer
                dsp::copy(gain, &gbuf[-nLookahead], to_do);
                nHead          += to_do;
                if (nHead >= buf_gap)
                {
                    dsp::move(vGainBuf, &vGainBuf[nHead], nMaxLookahead*4);
                    nHead       = 0;
                }

                // Decrement number of samples and update pointers
                gain           += to_do;
                sc             += to_do;
                samples        -= to_do;
            }
        }

        void Limiter::dump(IStateDumper *v, const char *name, const sat_t *sat)
        {
            v->begin_object(name, sat, sizeof(sat_t));
            {
                v->write("nAttack", sat->nAttack);
                v->write("nPlane", sat->nPlane);
                v->write("nRelease", sat->nRelease);
                v->write("nMiddle", sat->nMiddle);
                v->writev("vAttack", sat->vAttack, 4);
                v->writev("vRelease", sat->vRelease, 4);
            }
            v->end_object();
        }

        void Limiter::dump(IStateDumper *v, const char *name, const exp_t *exp)
        {
            v->begin_object(name, exp, sizeof(exp_t));
            {
                v->write("nAttack", exp->nAttack);
                v->write("nPlane", exp->nPlane);
                v->write("nRelease", exp->nRelease);
                v->write("nMiddle", exp->nMiddle);
                v->writev("vAttack", exp->vAttack, 4);
                v->writev("vRelease", exp->vRelease, 4);
            }
            v->end_object();
        }

        void Limiter::dump(IStateDumper *v, const char *name, const line_t *line)
        {
            v->begin_object(name, line, sizeof(line_t));
            {
                v->write("nAttack", line->nAttack);
                v->write("nPlane", line->nPlane);
                v->write("nRelease", line->nRelease);
                v->write("nMiddle", line->nMiddle);
                v->writev("vAttack", line->vAttack, 2);
                v->writev("vRelease", line->vRelease, 2);
            }
            v->end_object();
        }

        void Limiter::dump(IStateDumper *v) const
        {
            v->write("fThreshold", fThreshold);
            v->write("fReqThreshold", fReqThreshold);
            v->write("fLookahead", fLookahead);
            v->write("fMaxLookahead", fMaxLookahead);
            v->write("fAttack", fAttack);
            v->write("fRelease", fRelease);
            v->write("fKnee", fKnee);
            v->write("nMaxLookahead", nMaxLookahead);
            v->write("nLookahead", nLookahead);
            v->write("nHead", nHead);
            v->write("nMaxSampleRate", nMaxSampleRate);
            v->write("nSampleRate", nSampleRate);
            v->write("nUpdate", nUpdate);
            v->write("nMode", nMode);
            v->begin_object("sALR", &sALR, sizeof(alr_t));
            {
                v->write("fKS", sALR.fKS);
                v->write("fKE", sALR.fKE);
                v->write("fGain", sALR.fGain);
                v->write("fTauAttack", sALR.fTauAttack);
                v->write("fTauRelease", sALR.fTauRelease);
                v->writev("vHermite", sALR.vHermite, 3);
                v->write("fAttack", sALR.fAttack);
                v->write("fRelease", sALR.fRelease);
                v->write("fEnvelope", sALR.fEnvelope);
                v->write("bEnable", sALR.bEnable);
            }
            v->end_object();

            v->write("vGainBuf", vGainBuf);
            v->write("vTmpBuf", vTmpBuf);
            v->write("vData", vData);

            switch (nMode)
            {
                case LM_HERM_THIN:
                case LM_HERM_WIDE:
                case LM_HERM_TAIL:
                case LM_HERM_DUCK:
                    dump(v, "sSat", &sSat);
                    break;

                case LM_EXP_THIN:
                case LM_EXP_WIDE:
                case LM_EXP_TAIL:
                case LM_EXP_DUCK:
                    dump(v, "sExp", &sExp);
                    break;

                case LM_LINE_THIN:
                case LM_LINE_WIDE:
                case LM_LINE_TAIL:
                case LM_LINE_DUCK:
                    dump(v, "sLine", &sLine);
                    break;

                default:
                    break;
            }
        }
    } /* namespace dspu */
} /* namespace lsp */
