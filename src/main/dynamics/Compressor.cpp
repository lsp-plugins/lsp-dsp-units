/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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
            fEnvelope       = 0.0f;

            // Pre-calculated parameters
            fTauAttack      = 0.0f;
            fTauRelease     = 0.0f;
            fXRatio         = 0.0f;

            fLogTH          = 0.0f;
            fKS             = 0.0f;
            fKE             = 0.0f;
            vHermite[0]     = 0.0f;
            vHermite[1]     = 0.0f;
            vHermite[2]     = 0.0f;

            fBLogTH         = 0.0f;
            vBHermite[0]    = 0.0f;
            vBHermite[1]    = 0.0f;
            vBHermite[2]    = 0.0f;
            fBKS            = 0.0f;
            fBKE            = 0.0f;
            fBoost          = 1.0f;

            // Additional parameters
            nSampleRate     = 0;
            nMode           = CM_DOWNWARD;
            bUpdate         = true;
        }

        void Compressor::destroy()
        {
        }

        void Compressor::update_settings()
        {
            // Update settings if necessary
            fTauAttack      = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fAttack)));
            fTauRelease     = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fRelease)));

            // Configure according to the mode
            float ratio     = 1.0f / fRatio;
            fKS             = fAttackThresh * fKnee;        // Knee start
            fKE             = fAttackThresh / fKnee;        // Knee end
            fXRatio         = ratio;

            fLogTH          = logf(fAttackThresh);

            // Mode-dependent configuration
            switch (nMode)
            {
                case CM_UPWARD:
                {
                    float r         = lsp_max(fRatio, 1.0f + RATIO_PREC);
                    float rr        = 1.0f/r;

                    float th1       = logf(fAttackThresh);
                    float th2       = logf(fBoostThresh);
                    float b         = (rr - 1.0) * (th2 - th1);

                    vBefore[0]      = 0.0f;
                    vBefore[1]      = 0.0f;
                    fPreGain        = 1.0f;
                    vAfter[0]       = 1.0f - rr;
                    vAfter[1]       = (rr - 1.0f) * th1;

                    vBBefore[0]     = 0.0f;
                    vBBefore[1]     = b;
                    fBPreGain       = expf(b);
                    vBAfter[0]      = rr - 1.0f;
                    vBAfter[1]      = (1.0f - rr) * th2 + b;

                    float log_s1    = th1 + logf(fKnee);
                    float log_e1    = th1 - logf(fKnee);
                    float log_s2    = th2 + logf(fKnee);
                    float log_e2    = th2 - logf(fKnee);

                    fKS             = expf(th1) * fKnee;
                    fKE             = expf(th1) / fKnee;
                    fBKS            = expf(th2) * fKnee;
                    fBKE            = expf(th2) / fKnee;

                    interpolation::hermite_quadratic(
                        vHermite,
                        log_s1, 0.0f, 0.0f,
                        log_e1, vAfter[0]);
                    interpolation::hermite_quadratic(
                        vBHermite,
                        log_s2, b, 0.0f,
                        log_e2, vBAfter[0]);

                    break;
                }

                case CM_BOOSTING:
                {
                    float r         = lsp_max(fRatio, 1.0f + RATIO_PREC);
                    float rr        = 1.0f/r;
                    float b         = logf(fBoostThresh);

                    if (fBoostThresh >= 1.0f)
                    {
                        float th1       = logf(fAttackThresh);
                        float th2       = th1 + b/(rr - 1.0f);

                        vBefore[0]      = 0.0f;
                        vBefore[1]      = 0.0f;
                        fPreGain        = 1.0f;
                        vAfter[0]       = 1.0f - rr;
                        vAfter[1]       = (rr - 1.0f) * th1;

                        vBBefore[0]     = 0.0f;
                        vBBefore[1]     = b;
                        fBPreGain       = expf(b);
                        vBAfter[0]      = rr - 1.0f;
                        vBAfter[1]      = (1.0f - rr) * th2 + b;

                        float log_s1    = th1 + logf(fKnee);
                        float log_e1    = th1 - logf(fKnee);
                        float log_s2    = th2 + logf(fKnee);
                        float log_e2    = th2 - logf(fKnee);

                        fKS             = expf(th1) * fKnee;
                        fKE             = expf(th1) / fKnee;
                        fBKS            = expf(th2) * fKnee;
                        fBKE            = expf(th2) / fKnee;

                        lsp_trace("r = %f, b = %f", r, gain_to_db(expf(b)));
                        lsp_trace("th1 = %f, th2 = %f", gain_to_db(expf(th1)), gain_to_db(expf(th2)));

                        interpolation::hermite_quadratic(
                            vHermite,
                            log_s1, 0.0f, 0.0f,
                            log_e1, vAfter[0]);
                        interpolation::hermite_quadratic(
                            vBHermite,
                            log_s2, b, 0.0f,
                            log_e2, vBAfter[0]);
                    }
                    else
                    {
                        float th2       = logf(fAttackThresh);
                        float th1       = th2 + b/(rr - 1.0f);

                        vBefore[0]      = 0.0f;
                        vBefore[1]      = 0.0f;
                        fPreGain        = 1.0f;
                        vAfter[0]       = 1.0f - rr;
                        vAfter[1]       = (rr - 1.0f) * th1;

                        vBBefore[0]     = 0.0f;
                        vBBefore[1]     = 0.0f;
                        fBPreGain       = 1.0f;
                        vBAfter[0]      = rr - 1.0f;
                        vBAfter[1]      = (1.0f - rr) * th1 + b;

                        float log_s1    = th1 + logf(fKnee);
                        float log_e1    = th1 - logf(fKnee);
                        float log_s2    = th2 + logf(fKnee);
                        float log_e2    = th2 - logf(fKnee);

                        fKS             = expf(th1) * fKnee;
                        fKE             = expf(th1) / fKnee;
                        fBKS            = expf(th2) * fKnee;
                        fBKE            = expf(th2) / fKnee;

                        lsp_trace("r = %f, b = %f", r, gain_to_db(expf(b)));
                        lsp_trace("th1 = %f, th2 = %f", gain_to_db(expf(th1)), gain_to_db(expf(th2)));

                        interpolation::hermite_quadratic(
                            vHermite,
                            log_s1, 0.0f, 0.0f,
                            log_e1, vAfter[0]);
                        interpolation::hermite_quadratic(
                            vBHermite,
                            log_s2, 0.0f, 0.0f,
                            log_e2, vBAfter[0]);
                    }

                    break;
                }

                case CM_DOWNWARD:
                default:
                {
                    float r         = fRatio;
                    float rr        = 1.0f/r;
                    float th1       = logf(fAttackThresh);

                    vBefore[0]      = 0.0f;
                    vBefore[1]      = 0.0f;
                    fPreGain        = 1.0f;
                    vAfter[0]       = rr - 1.0f;
                    vAfter[1]       = (1.0f - rr) * th1;

                    vBBefore[0]     = 0.0f;
                    vBBefore[1]     = 0.0f;
                    fBPreGain       = 1.0f;
                    vBAfter[0]      = 0.0f;
                    vBAfter[1]      = 0.0f;

                    float log_s1    = th1 + logf(fKnee);
                    float log_e1    = th1 - logf(fKnee);

                    fKS             = expf(th1) * fKnee;
                    fKE             = expf(th1) / fKnee;
                    fBKS            = expf(th1);
                    fBKE            = expf(th1);

                    interpolation::hermite_quadratic(
                        vHermite,
                        log_s1, 0.0f, 0.0f,
                        log_e1, vAfter[0]);
                    break;
                }
            }

            // Reset update flag
            bUpdate         = false;
        }

        void Compressor::process(float *out, float *env, const float *in, size_t samples)
        {
            // Calculate envelope of compressor
            for (size_t i=0; i<samples; ++i)
            {
                float s         = *(in++);

                if (fEnvelope > fReleaseThresh)
                    fEnvelope       += (s > fEnvelope) ? fTauAttack * (s - fEnvelope) : fTauRelease * (s - fEnvelope);
                else
                    fEnvelope       += fTauAttack * (s - fEnvelope);

                out[i]          = fEnvelope;
            }

            // Copy envelope to array if specified
            if (env != NULL)
                dsp::copy(env, out, samples);

            // Now calculate compressor's curve
            reduction(out, out, samples);
        }

        float Compressor::process(float *env, float s)
        {
            if (fEnvelope > fReleaseThresh)
                fEnvelope       += (s > fEnvelope) ? fTauAttack * (s - fEnvelope) : fTauRelease * (s - fEnvelope);
            else
                fEnvelope       += fTauAttack * (s - fEnvelope);

            if (env != NULL)
                *env    = fEnvelope;

            return reduction(fEnvelope);
        }

        void Compressor::curve(float *out, const float *in, size_t dots)
        {
            for (size_t i=0; i<dots; ++i)
            {
                float x     = fabs(in[i]);
                float lx    = logf(x);

                float g1    = (x <= fKS) ? fPreGain :
                              (x >= fKE) ? expf(lx * vAfter[0] + vAfter[1]) :
                              expf((vHermite[0]*lx + vHermite[1])*lx + vHermite[2]);
                float g2    = (x <= fBKS) ? fBPreGain :
                              (x >= fBKE) ? expf(lx * vBAfter[0] + vBAfter[1]) :
                              expf((vBHermite[0]*lx + vBHermite[1])*lx + vBHermite[2]);

                out[i]      = g1 * g2 * x;
            }
        }

        float Compressor::curve(float in)
        {
            float x     = fabs(in);
            float lx    = logf(x);

            float g1    = (x <= fKS) ? fPreGain :
                          (x >= fKE) ? expf(lx * vAfter[0] + vAfter[1]) :
                          expf((vHermite[0]*lx + vHermite[1])*lx + vHermite[2]);
            float g2    = (x <= fBKS) ? fBPreGain :
                          (x >= fBKE) ? expf(lx * vBAfter[0] + vBAfter[1]) :
                          expf((vBHermite[0]*lx + vBHermite[1])*lx + vBHermite[2]);

            x           = g1 * g2 * x;
            return x;
        }

        void Compressor::reduction(float *out, const float *in, size_t dots)
        {
            for (size_t i=0; i<dots; ++i)
            {
                float x     = fabs(in[i]);
                float lx    = logf(lsp_max(x, GAIN_AMP_M_120_DB));

                float g1    = (x <= fKS) ? fPreGain :
                              (x >= fKE) ? expf(lx * vAfter[0] + vAfter[1]) :
                              expf((vHermite[0]*lx + vHermite[1])*lx + vHermite[2]);
                float g2    = (x <= fBKS) ? fBPreGain :
                              (x >= fBKE) ? expf(lx * vBAfter[0] + vBAfter[1]) :
                              expf((vBHermite[0]*lx + vBHermite[1])*lx + vBHermite[2]);

                out[i]      = g1 * g2;
            }
        }

        float Compressor::reduction(float in)
        {
            float x     = fabs(in);
            float lx    = logf(lsp_max(x, GAIN_AMP_M_120_DB));

            float g1    = (x <= fKS) ? fPreGain :
                          (x >= fKE) ? expf(lx * vAfter[0] + vAfter[1]) :
                          expf((vHermite[0]*lx + vHermite[1])*lx + vHermite[2]);
            float g2    = (x <= fBKS) ? fBPreGain :
                          (x >= fBKE) ? expf(lx * vBAfter[0] + vBAfter[1]) :
                          expf((vBHermite[0]*lx + vBHermite[1])*lx + vBHermite[2]);

            x           = g1 * g2;
            return x;
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

        void Compressor::dump(IStateDumper *v) const
        {
            v->write("fAttackThresh", fAttackThresh);
            v->write("fReleaseThresh", fReleaseThresh);
            v->write("fBoostThresh", fBoostThresh);
            v->write("fAttack", fAttack);
            v->write("fRelease", fRelease);
            v->write("fKnee", fKnee);
            v->write("fRatio", fRatio);
            v->write("fEnvelope", fEnvelope);
            v->write("fTauAttack", fTauAttack);
            v->write("fTauRelease", fTauRelease);
            v->write("fXRatio", fXRatio);
            v->write("fLogTH", fLogTH);
            v->write("fKS", fKS);
            v->write("fKE", fKE);
            v->writev("vHermite", vHermite, 3);
            v->write("fBLogTH", fBLogTH);
            v->write("fBKS", fBKS);
            v->write("fBKE", fBKE);
            v->writev("vBHermite", vBHermite, 3);
            v->write("fBoost", fBoost);
            v->write("nSampleRate", nSampleRate);
            v->write("nMode", nMode);
            v->write("bUpdate", bUpdate);
        }
    }
} /* namespace lsp */
