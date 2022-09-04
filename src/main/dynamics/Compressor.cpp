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

            for (size_t i=0; i<2; ++i)
            {
                knee_t *k       = &vKnees[i];
                k->fKS          = 0.0f;
                k->fKE          = 0.0f;
                k->fGain        = 1.0f;
                k->vKnee[0]     = 0.0f;
                k->vKnee[1]     = 0.0f;
                k->vKnee[2]     = 0.0f;
                k->vTilt[0]     = 0.0f;
                k->vTilt[1]     = 0.0f;
            }

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
            if (!bUpdate)
                return;

            // Update settings if necessary
            fTauAttack          = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fAttack)));
            fTauRelease         = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fRelease)));

            // Mode-dependent configuration
            switch (nMode)
            {
                case CM_UPWARD:
                {
                    float rr            = 1.0f / fRatio;
                    float th1           = logf(fAttackThresh);
                    float th2           = logf(fBoostThresh);
                    float b             = (rr - 1.0f) * (th2 - th1);

                    vKnees[0].fKS       = fAttackThresh * fKnee;
                    vKnees[0].fKE       = fAttackThresh / fKnee;
                    vKnees[0].fGain     = 1.0f;
                    vKnees[0].vTilt[0]  = 1.0f - rr;
                    vKnees[0].vTilt[1]  = (rr - 1.0f) * th1;

                    vKnees[1].fKS       = fBoostThresh * fKnee;
                    vKnees[1].fKE       = fBoostThresh / fKnee;
                    vKnees[1].fGain     = expf(b);
                    vKnees[1].vTilt[0]  = rr - 1.0f;
                    vKnees[1].vTilt[1]  = (1.0f - rr) * th1;

                    interpolation::hermite_quadratic(
                        vKnees[0].vKnee,
                        logf(vKnees[0].fKS), 0.0f, 0.0f,
                        logf(vKnees[0].fKE), vKnees[0].vTilt[0]);
                    interpolation::hermite_quadratic(
                        vKnees[1].vKnee,
                        logf(vKnees[1].fKS), b, 0.0f,
                        logf(vKnees[1].fKE), vKnees[1].vTilt[0]);

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
                        vKnees[0].fKS       = fAttackThresh * fKnee;
                        vKnees[0].fKE       = fAttackThresh / fKnee;
                        vKnees[0].fGain     = 1.0f;
                        vKnees[0].vTilt[0]  = 1.0f - rr;
                        vKnees[0].vTilt[1]  = (rr - 1.0f) * th1;

                        vKnees[1].fKS       = eth2 * fKnee;
                        vKnees[1].fKE       = eth2 / fKnee;
                        vKnees[1].fGain     = fBoostThresh;
                        vKnees[1].vTilt[0]  = rr - 1.0f;
                        vKnees[1].vTilt[1]  = (1.0f - rr) * th1;

                        interpolation::hermite_quadratic(
                            vKnees[0].vKnee,
                            logf(vKnees[0].fKS), 0.0f, 0.0f,
                            logf(vKnees[0].fKE), vKnees[0].vTilt[0]);
                        interpolation::hermite_quadratic(
                            vKnees[1].vKnee,
                            logf(vKnees[1].fKS), b, 0.0f,
                            logf(vKnees[1].fKE), vKnees[1].vTilt[0]);
                    }
                    else
                    {
                        vKnees[0].fKS       = fAttackThresh * fKnee;
                        vKnees[0].fKE       = fAttackThresh / fKnee;
                        vKnees[0].fGain     = 1.0f;
                        vKnees[0].vTilt[0]  = rr - 1.0f;
                        vKnees[0].vTilt[1]  = (1.0f - rr) * th1;

                        vKnees[1].fKS       = eth2 * fKnee;
                        vKnees[1].fKE       = eth2 / fKnee;
                        vKnees[1].fGain     = 1.0f;
                        vKnees[1].vTilt[0]  = 1.0f - rr;
                        vKnees[1].vTilt[1]  = (rr - 1.0f) * th2;

                        interpolation::hermite_quadratic(
                            vKnees[0].vKnee,
                            logf(vKnees[0].fKS), 0.0f, 0.0f,
                            logf(vKnees[0].fKE), vKnees[0].vTilt[0]);
                        interpolation::hermite_quadratic(
                            vKnees[1].vKnee,
                            logf(vKnees[1].fKS), 0.0f, 0.0f,
                            logf(vKnees[1].fKE), vKnees[1].vTilt[0]);
                    }

                    break;
                }

                case CM_DOWNWARD:
                default:
                {
                    float rr            = 1.0f / fRatio;
                    float th1           = logf(fAttackThresh);

                    vKnees[0].fKS       = fAttackThresh * fKnee;
                    vKnees[0].fKE       = fAttackThresh / fKnee;
                    vKnees[0].fGain     = 1.0f;
                    vKnees[0].vTilt[0]  = rr - 1.0f;
                    vKnees[0].vTilt[1]  = (1.0f - rr) * th1;

                    vKnees[1].fKS       = 0.0f;
                    vKnees[1].fKE       = 0.0f;
                    vKnees[1].fGain     = 1.0f;
                    vKnees[1].vTilt[0]  = 0.0f;
                    vKnees[1].vTilt[1]  = 0.0f;

                    interpolation::hermite_quadratic(
                        vKnees[0].vKnee,
                        logf(vKnees[0].fKS), 0.0f, 0.0f,
                        logf(vKnees[0].fKE), vKnees[0].vTilt[0]);

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
            for (size_t i=0; i<samples; ++i)
            {
                float x     = fabs(out[i]);
                float lx    = logf(x);

                float g1    = (x <= vKnees[0].fKS) ? vKnees[0].fGain :
                              (x >= vKnees[0].fKE) ? expf(lx * vKnees[0].vTilt[0] + vKnees[0].vTilt[1]) :
                              expf((vKnees[0].vKnee[0]*lx + vKnees[0].vKnee[1])*lx + vKnees[0].vKnee[2]);
                float g2    = (x <= vKnees[1].fKS) ? vKnees[1].fGain :
                              (x >= vKnees[1].fKE) ? expf(lx * vKnees[1].vTilt[0] + vKnees[1].vTilt[1]) :
                              expf((vKnees[1].vKnee[0]*lx + vKnees[1].vKnee[1])*lx + vKnees[1].vKnee[2]);

                out[i]      = g1 * g2;
            }
        }

        float Compressor::process(float *env, float s)
        {
            update_settings();

            if (fEnvelope > fReleaseThresh)
                fEnvelope       += (s > fEnvelope) ? fTauAttack * (s - fEnvelope) : fTauRelease * (s - fEnvelope);
            else
                fEnvelope       += fTauAttack * (s - fEnvelope);

            if (env != NULL)
                *env    = fEnvelope;

            float x     = fabs(fEnvelope);
            float lx    = logf(x);

            float g1    = (x <= vKnees[0].fKS) ? vKnees[0].fGain :
                          (x >= vKnees[0].fKE) ? expf(lx * vKnees[0].vTilt[0] + vKnees[0].vTilt[1]) :
                          expf((vKnees[0].vKnee[0]*lx + vKnees[0].vKnee[1])*lx + vKnees[0].vKnee[2]);
            float g2    = (x <= vKnees[1].fKS) ? vKnees[1].fGain :
                          (x >= vKnees[1].fKE) ? expf(lx * vKnees[1].vTilt[0] + vKnees[1].vTilt[1]) :
                          expf((vKnees[1].vKnee[0]*lx + vKnees[1].vKnee[1])*lx + vKnees[1].vKnee[2]);

            x           = g1 * g2;
            return x;
        }

        void Compressor::curve(float *out, const float *in, size_t dots)
        {
            update_settings();

            for (size_t i=0; i<dots; ++i)
            {
                float x     = fabs(in[i]);
                float lx    = logf(x);

                float g1    = (x <= vKnees[0].fKS) ? vKnees[0].fGain :
                              (x >= vKnees[0].fKE) ? expf(lx * vKnees[0].vTilt[0] + vKnees[0].vTilt[1]) :
                              expf((vKnees[0].vKnee[0]*lx + vKnees[0].vKnee[1])*lx + vKnees[0].vKnee[2]);
                float g2    = (x <= vKnees[1].fKS) ? vKnees[1].fGain :
                              (x >= vKnees[1].fKE) ? expf(lx * vKnees[1].vTilt[0] + vKnees[1].vTilt[1]) :
                              expf((vKnees[1].vKnee[0]*lx + vKnees[1].vKnee[1])*lx + vKnees[1].vKnee[2]);

                out[i]      = g1 * g2 * x;
            }
        }

        float Compressor::curve(float in)
        {
            update_settings();

            float x     = fabs(in);
            float lx    = logf(x);

            float g1    = (x <= vKnees[0].fKS) ? vKnees[0].fGain :
                          (x >= vKnees[0].fKE) ? expf(lx * vKnees[0].vTilt[0] + vKnees[0].vTilt[1]) :
                          expf((vKnees[0].vKnee[0]*lx + vKnees[0].vKnee[1])*lx + vKnees[0].vKnee[2]);
            float g2    = (x <= vKnees[1].fKS) ? vKnees[1].fGain :
                          (x >= vKnees[1].fKE) ? expf(lx * vKnees[1].vTilt[0] + vKnees[1].vTilt[1]) :
                          expf((vKnees[1].vKnee[0]*lx + vKnees[1].vKnee[1])*lx + vKnees[1].vKnee[2]);

            x           = g1 * g2 * x;
            return x;
        }

        void Compressor::reduction(float *out, const float *in, size_t dots)
        {
            update_settings();

            for (size_t i=0; i<dots; ++i)
            {
                float x     = fabs(in[i]);
                float lx    = logf(x);

                float g1    = (x <= vKnees[0].fKS) ? vKnees[0].fGain :
                              (x >= vKnees[0].fKE) ? expf(lx * vKnees[0].vTilt[0] + vKnees[0].vTilt[1]) :
                              expf((vKnees[0].vKnee[0]*lx + vKnees[0].vKnee[1])*lx + vKnees[0].vKnee[2]);
                float g2    = (x <= vKnees[1].fKS) ? vKnees[1].fGain :
                              (x >= vKnees[1].fKE) ? expf(lx * vKnees[1].vTilt[0] + vKnees[1].vTilt[1]) :
                              expf((vKnees[1].vKnee[0]*lx + vKnees[1].vKnee[1])*lx + vKnees[1].vKnee[2]);

                out[i]      = g1 * g2;
            }
        }

        float Compressor::reduction(float in)
        {
            update_settings();

            float x     = fabs(in);
            float lx    = logf(x);

            float g1    = (x <= vKnees[0].fKS) ? vKnees[0].fGain :
                          (x >= vKnees[0].fKE) ? expf(lx * vKnees[0].vTilt[0] + vKnees[0].vTilt[1]) :
                          expf((vKnees[0].vKnee[0]*lx + vKnees[0].vKnee[1])*lx + vKnees[0].vKnee[2]);
            float g2    = (x <= vKnees[1].fKS) ? vKnees[1].fGain :
                          (x >= vKnees[1].fKE) ? expf(lx * vKnees[1].vTilt[0] + vKnees[1].vTilt[1]) :
                          expf((vKnees[1].vKnee[0]*lx + vKnees[1].vKnee[1])*lx + vKnees[1].vKnee[2]);

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
            v->begin_array("vKnees", vKnees, 2);
            {
                for (size_t i=0; i<2; ++i)
                {
                    const knee_t *k = &vKnees[i];
                    v->write("fKS", k->fKS);
                    v->write("fKE", k->fKE);
                    v->write("fGain", k->fGain);
                    v->writev("vKnee", k->vKnee, 3);
                    v->writev("vTilt", k->vTilt, 2);
                }
            }
            v->end_array();
            v->write("nSampleRate", nSampleRate);
            v->write("nMode", nMode);
            v->write("bUpdate", bUpdate);
        }
    }
} /* namespace lsp */
