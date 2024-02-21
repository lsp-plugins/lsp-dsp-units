/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 16 сент. 2016 г.
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
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/const.h>
#include <lsp-plug.in/dsp-units/dynamics/Compressor.h>
#include <lsp-plug.in/dsp-units/misc/interpolation.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/stdlib/math.h>

#define RATIO_PREC              1e-5f

namespace lsp
{
    namespace dspu
    {
        Compressor::Compressor()
        {
            construct();
        }

        Compressor::~Compressor()
        {
            destroy();
        }

        void Compressor::construct()
        {
            // Basic parameters
            fAttackThresh   = 0.0f;
            fReleaseThresh  = 0.0f;
            fBoostThresh    = GAIN_AMP_M_72_DB;
            fAttack         = 0.0f;
            fRelease        = 0.0f;
            fKnee           = 0.0f;
            fRatio          = 1.0f;
            fHold           = 0.0f;
            fEnvelope       = 0.0f;
            fPeak           = 0.0f;

            // Pre-calculated parameters
            fTauAttack      = 0.0f;
            fTauRelease     = 0.0f;

            for (size_t i=0; i<2; ++i)
            {
                dsp::compressor_knee_t *k   = &sComp.k[i];
                k->start        = 0.0f;
                k->end          = 0.0f;
                k->gain         = 1.0f;
                k->herm[0]      = 0.0f;
                k->herm[1]      = 0.0f;
                k->herm[2]      = 0.0f;
                k->herm[0]      = 0.0f;
                k->herm[1]      = 0.0f;
            }

            // Additional parameters
            nHold           = 0;
            nHoldCounter    = 0;
            nSampleRate     = 0;
            nMode           = CM_DOWNWARD;
            bUpdate         = true;
        }

        void Compressor::destroy()
        {
        }

        void Compressor::update_settings()
        {
            if (!bUpdate)
                return;

            // Update settings if necessary
            fTauAttack          = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fAttack)));
            fTauRelease         = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fRelease)));
            nHold               = millis_to_samples(nSampleRate, fHold);

            // Mode-dependent configuration
            switch (nMode)
            {
                case CM_UPWARD:
                {
                    float rr            = 1.0f / fRatio;
                    float th1           = logf(fAttackThresh);
                    float th2           = logf(fBoostThresh);
                    float b             = (rr - 1.0f) * (th2 - th1);

                    sComp.k[0].start    = fAttackThresh * fKnee;
                    sComp.k[0].end      = fAttackThresh / fKnee;
                    sComp.k[0].gain     = 1.0f;
                    sComp.k[0].tilt[0]  = 1.0f - rr;
                    sComp.k[0].tilt[1]  = (rr - 1.0f) * th1;

                    sComp.k[1].start    = fBoostThresh * fKnee;
                    sComp.k[1].end      = fBoostThresh / fKnee;
                    sComp.k[1].gain     = expf(b);
                    sComp.k[1].tilt[0]  = rr - 1.0f;
                    sComp.k[1].tilt[1]  = (1.0f - rr) * th1;

                    interpolation::hermite_quadratic(
                        sComp.k[0].herm,
                        logf(sComp.k[0].start), 0.0f, 0.0f,
                        logf(sComp.k[0].end), sComp.k[0].tilt[0]);
                    interpolation::hermite_quadratic(
                        sComp.k[1].herm,
                        logf(sComp.k[1].start), b, 0.0f,
                        logf(sComp.k[1].end), sComp.k[1].tilt[0]);

                    break;
                }

                case CM_BOOSTING:
                {
                    float rr            = 1.0f / lsp_max(fRatio, 1.0f + RATIO_PREC);
                    float b             = logf(fBoostThresh);
                    float th1           = logf(fAttackThresh);
                    float th2           = th1 + b/(rr - 1.0f);
                    float eth2          = expf(th2);

                    if (fBoostThresh >= 1.0f)
                    {
                        sComp.k[0].start    = fAttackThresh * fKnee;
                        sComp.k[0].end      = fAttackThresh / fKnee;
                        sComp.k[0].gain     = 1.0f;
                        sComp.k[0].tilt[0]  = 1.0f - rr;
                        sComp.k[0].tilt[1]  = (rr - 1.0f) * th1;

                        sComp.k[1].start    = eth2 * fKnee;
                        sComp.k[1].end      = eth2 / fKnee;
                        sComp.k[1].gain     = fBoostThresh;
                        sComp.k[1].tilt[0]  = rr - 1.0f;
                        sComp.k[1].tilt[1]  = (1.0f - rr) * th1;

                        interpolation::hermite_quadratic(
                            sComp.k[0].herm,
                            logf(sComp.k[0].start), 0.0f, 0.0f,
                            logf(sComp.k[0].end), sComp.k[0].tilt[0]);
                        interpolation::hermite_quadratic(
                            sComp.k[1].herm,
                            logf(sComp.k[1].start), b, 0.0f,
                            logf(sComp.k[1].end), sComp.k[1].tilt[0]);
                    }
                    else
                    {
                        sComp.k[0].start    = fAttackThresh * fKnee;
                        sComp.k[0].end      = fAttackThresh / fKnee;
                        sComp.k[0].gain     = 1.0f;
                        sComp.k[0].tilt[0]  = rr - 1.0f;
                        sComp.k[0].tilt[1]  = (1.0f - rr) * th1;

                        sComp.k[1].start    = eth2 * fKnee;
                        sComp.k[1].end      = eth2 / fKnee;
                        sComp.k[1].gain     = 1.0f;
                        sComp.k[1].tilt[0]  = 1.0f - rr;
                        sComp.k[1].tilt[1]  = (rr - 1.0f) * th2;

                        interpolation::hermite_quadratic(
                            sComp.k[0].herm,
                            logf(sComp.k[0].start), 0.0f, 0.0f,
                            logf(sComp.k[0].end), sComp.k[0].tilt[0]);
                        interpolation::hermite_quadratic(
                            sComp.k[1].herm,
                            logf(sComp.k[1].start), 0.0f, 0.0f,
                            logf(sComp.k[1].end), sComp.k[1].tilt[0]);
                    }

                    break;
                }

                case CM_DOWNWARD:
                default:
                {
                    float rr            = 1.0f / fRatio;
                    float th1           = logf(fAttackThresh);

                    sComp.k[0].start    = fAttackThresh * fKnee;
                    sComp.k[0].end      = fAttackThresh / fKnee;
                    sComp.k[0].gain     = 1.0f;
                    sComp.k[0].tilt[0]  = rr - 1.0f;
                    sComp.k[0].tilt[1]  = (1.0f - rr) * th1;

                    sComp.k[1].start    = FLOAT_SAT_P_INF;
                    sComp.k[1].end      = FLOAT_SAT_P_INF;
                    sComp.k[1].gain     = 1.0f;
                    sComp.k[1].tilt[0]  = 0.0f;
                    sComp.k[1].tilt[1]  = 0.0f;

                    interpolation::hermite_quadratic(
                        sComp.k[0].herm,
                        logf(sComp.k[0].start), 0.0f, 0.0f,
                        logf(sComp.k[0].end), sComp.k[0].tilt[0]);

                    break;
                }
            }

            // Reset update flag
            bUpdate         = false;
        }

        void Compressor::process(float *out, float *env, const float *in, size_t samples)
        {
            update_settings();

            // Calculate envelope of compressor
            float e         = fEnvelope;
            float peak      = fPeak;
            uint32_t hold   = nHoldCounter;

            for (size_t i=0; i<samples; ++i)
            {
                float s         = in[i];
                float d         = s - e;
                if (d < 0.0f)
                {
                    if (hold > 0)
                        --hold;
                    else
                    {
                        e              += ((e > fReleaseThresh) ? fTauRelease : fTauAttack) * d;
                        peak            = e;
                    }
                }
                else
                {
                    e              += fTauAttack * d;
                    if (e >= peak)
                    {
                        peak            = e;
                        hold            = nHold;
                    }
                }

                out[i]          = e;
            }
            fEnvelope       = e;
            fPeak           = peak;
            nHoldCounter    = hold;

            // Copy envelope to array if specified
            if (env != NULL)
                dsp::copy(env, out, samples);

            // Now calculate compressor's curve
            dsp::compressor_x2_gain(out, out, &sComp, samples);
        }

        float Compressor::process(float *env, float s)
        {
            update_settings();

            float d         = s - fEnvelope;
            if (d < 0.0f)
            {
                if (nHoldCounter > 0)
                    --nHoldCounter;
                else
                {
                    fEnvelope      += ((fEnvelope > fReleaseThresh) ? fTauRelease : fTauAttack) * d;
                    fPeak           = fEnvelope;
                }
            }
            else
            {
                fEnvelope      += fTauAttack * d;
                if (fEnvelope >= fPeak)
                {
                    fPeak           = fEnvelope;
                    nHoldCounter    = nHold;
                }
            }

            if (env != NULL)
                *env    = fEnvelope;

            float x     = fabsf(fEnvelope);
            if ((x <= sComp.k[0].start) && (x <= sComp.k[1].start))
                return sComp.k[0].gain * sComp.k[1].gain;

            float lx    = logf(x);
            float g1    = (x <= sComp.k[0].start) ? sComp.k[0].gain :
                          (x >= sComp.k[0].end) ? expf(lx * sComp.k[0].tilt[0] + sComp.k[0].tilt[1]) :
                          expf((sComp.k[0].herm[0]*lx + sComp.k[0].herm[1])*lx + sComp.k[0].herm[2]);
            float g2    = (x <= sComp.k[1].start) ? sComp.k[1].gain :
                          (x >= sComp.k[1].end) ? expf(lx * sComp.k[1].tilt[0] + sComp.k[1].tilt[1]) :
                          expf((sComp.k[1].herm[0]*lx + sComp.k[1].herm[1])*lx + sComp.k[1].herm[2]);

            x           = g1 * g2;
            return x;
        }

        void Compressor::curve(float *out, const float *in, size_t dots)
        {
            dsp::compressor_x2_curve(out, in, &sComp, dots);
        }

        float Compressor::curve(float in)
        {
            float x     = fabsf(in);
            if ((x <= sComp.k[0].start) && (x <= sComp.k[1].start))
                return sComp.k[0].gain * sComp.k[1].gain * x;

            float lx    = logf(x);
            float g1    = (x <= sComp.k[0].start) ? sComp.k[0].gain :
                          (x >= sComp.k[0].end) ? expf(lx * sComp.k[0].tilt[0] + sComp.k[0].tilt[1]) :
                          expf((sComp.k[0].herm[0]*lx + sComp.k[0].herm[1])*lx + sComp.k[0].herm[2]);
            float g2    = (x <= sComp.k[1].start) ? sComp.k[1].gain :
                          (x >= sComp.k[1].end) ? expf(lx * sComp.k[1].tilt[0] + sComp.k[1].tilt[1]) :
                          expf((sComp.k[1].herm[0]*lx + sComp.k[1].herm[1])*lx + sComp.k[1].herm[2]);

            x           = g1 * g2 * x;
            return x;
        }

        void Compressor::reduction(float *out, const float *in, size_t dots)
        {
            update_settings();
            dsp::compressor_x2_curve(out, in, &sComp, dots);
        }

        float Compressor::reduction(float in)
        {
            update_settings();

            float x     = fabsf(in);
            if ((x <= sComp.k[0].start) && (x <= sComp.k[1].start))
                return sComp.k[0].gain * sComp.k[1].gain;

            float lx    = logf(x);
            float g1    = (x <= sComp.k[0].start) ? sComp.k[0].gain :
                          (x >= sComp.k[0].end) ? expf(lx * sComp.k[0].tilt[0] + sComp.k[0].tilt[1]) :
                          expf((sComp.k[0].herm[0]*lx + sComp.k[0].herm[1])*lx + sComp.k[0].herm[2]);
            float g2    = (x <= sComp.k[1].start) ? sComp.k[1].gain :
                          (x >= sComp.k[1].end) ? expf(lx * sComp.k[1].tilt[0] + sComp.k[1].tilt[1]) :
                          expf((sComp.k[1].herm[0]*lx + sComp.k[1].herm[1])*lx + sComp.k[1].herm[2]);

            x           = g1 * g2;
            return x;
        }

        void Compressor::set_attack_threshold(float threshold)
        {
            if (fAttackThresh == threshold)
                return;
            fAttackThresh       = threshold;
            bUpdate             = true;
        }

        void Compressor::set_release_threshold(float threshold)
        {
            if (fReleaseThresh == threshold)
                return;
            fReleaseThresh      = threshold;
            bUpdate             = true;
        }

        void Compressor::set_threshold(float attack, float release)
        {
            if ((fAttackThresh == attack) && (fReleaseThresh == release))
                return;
            fAttackThresh       = attack;
            fReleaseThresh      = release;
            bUpdate             = true;
        }

        void Compressor::set_boost_threshold(float boost)
        {
            if (fBoostThresh == boost)
                return;
            fBoostThresh        = boost;
            bUpdate             = true;
        }

        void Compressor::set_timings(float attack, float release)
        {
            if ((fAttack == attack) && (fRelease == release))
                return;
            fAttack     = attack;
            fRelease    = release;
            bUpdate     = true;
        }

        void Compressor::set_attack(float attack)
        {
            if (fAttack == attack)
                return;
            fAttack     = attack;
            bUpdate     = true;
        }

        void Compressor::set_release(float release)
        {
            if (fRelease == release)
                return;
            fRelease    = release;
            bUpdate     = true;
        }

        void Compressor::set_sample_rate(size_t sr)
        {
            if (sr == nSampleRate)
                return;
            nSampleRate = sr;
            bUpdate     = true;
        }

        void Compressor::set_knee(float knee)
        {
            knee        = lsp_limit(knee, 0.0f, 1.0f);
            if (knee == fKnee)
                return;
            fKnee       = knee;
            bUpdate     = true;
        }

        void Compressor::set_ratio(float ratio)
        {
            if (ratio == fRatio)
                return;
            bUpdate     = true;
            fRatio      = ratio;
        }

        void Compressor::set_mode(size_t mode)
        {
            if (nMode == mode)
                return;

            nMode       = mode;
            bUpdate     = true;
        }

        void Compressor::set_hold(float hold)
        {
            hold        = lsp_max(hold, 0.0f);
            if (hold == fHold)
                return;
            fHold       = hold;
            bUpdate     = true;
        }

        void Compressor::dump(IStateDumper *v) const
        {
            v->write("fAttackThresh", fAttackThresh);
            v->write("fReleaseThresh", fReleaseThresh);
            v->write("fBoostThresh", fBoostThresh);
            v->write("fAttack", fAttack);
            v->write("fRelease", fRelease);
            v->write("fKnee", fKnee);
            v->write("fRatio", fRatio);
            v->write("fHold", fHold);
            v->write("fEnvelope", fEnvelope);
            v->write("fPeak", fPeak);
            v->write("fTauAttack", fTauAttack);
            v->write("fTauRelease", fTauRelease);
            v->begin_object("sComp", &sComp, sizeof(sComp));
            {
                v->begin_array("k", sComp.k, 2);
                {
                    for (size_t i=0; i<2; ++i)
                    {
                        const dsp::compressor_knee_t *k = &sComp.k[i];
                        v->begin_object(k, sizeof(dsp::compressor_knee_t));
                        {
                            v->write("start", k->start);
                            v->write("end", k->end);
                            v->write("gain", k->gain);
                            v->writev("herm", k->herm, 3);
                            v->writev("tilt", k->tilt, 2);
                        }
                        v->end_object();
                    }
                }
                v->end_array();
            }
            v->end_array();
            v->write("nSampleRate", nSampleRate);
            v->write("nMode", nMode);
            v->write("bUpdate", bUpdate);
        }

    } /* namespace dspu */
} /* namespace lsp */
