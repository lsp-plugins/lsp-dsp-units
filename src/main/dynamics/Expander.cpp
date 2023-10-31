/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 2 нояб. 2016 г.
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

#include <lsp-plug.in/dsp-units/dynamics/Expander.h>
#include <lsp-plug.in/dsp-units/misc/interpolation.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/units.h>

namespace lsp
{
    namespace dspu
    {
        Expander::Expander()
        {
            construct();
        }

        Expander::~Expander()
        {
            destroy();
        }

        void Expander::construct()
        {
            // Basic parameters
            fAttackThresh   = 0.0f;
            fReleaseThresh  = 0.0f;
            fAttack         = 0.0f;
            fRelease        = 0.0f;
            fKnee           = 0.0f;
            fRatio          = 1.0f;
            fEnvelope       = 0.0f;

            // Pre-calculated parameters
            fTauAttack      = 0.0f;
            fTauRelease     = 0.0f;
            fKS             = 0.0f;
            fKE             = 0.0f;
            vHermite[0]     = 0.0f;
            vHermite[1]     = 0.0f;
            vHermite[2]     = 0.0f;
            vTilt[0]        = 0.0f;
            vTilt[1]        = 0.0f;

            // Additional parameters
            nSampleRate     = 0;
            bUpdate         = true;
            bUpward         = true;
        }

        void Expander::destroy()
        {
        }

        void Expander::update_settings()
        {
            if (!bUpdate)
                return;

            // Update settings if necessary
            fTauAttack      = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fAttack)));
            fTauRelease     = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fRelease)));

            // Calculate interpolation parameters
            fKS             = fAttackThresh * fKnee;
            fKE             = fAttackThresh / fKnee;
            float log_ks    = logf(fKS);                        // Knee start
            float log_ke    = logf(fKE);                        // Knee end
            float log_th    = logf(fAttackThresh);              // Attack threshold

            if (bUpward)
                interpolation::hermite_quadratic(vHermite, log_ks, log_ks, 1.0f, log_ke, fRatio);
            else
                interpolation::hermite_quadratic(vHermite, log_ke, log_ke, 1.0f, log_ks, fRatio);

            vTilt[0]        = fRatio - 1.0f;
            vTilt[1]        = log_th * (1.0f - fRatio);

            // Reset update flag
            bUpdate         = false;
        }

        void Expander::process(float *out, float *env, const float *in, size_t samples)
        {
            // Calculate envelope of expander
            float e         = fEnvelope;
            for (size_t i=0; i<samples; ++i)
            {
                float s         = in[i];
                float d         = s - e;
                float k         = ((e > fReleaseThresh) && (d < 0.0f)) ? fTauRelease : fTauAttack;
                e              += k * d;
                out[i]          = e;
            }
            fEnvelope       = e;

            // Copy envelope to array if specified
            if (env != NULL)
                dsp::copy(env, out, samples);
    
            // Now calculate expander curve
            amplification(out, out, samples);
        }
    
        float Expander::process(float *env, float s)
        {
            if (fEnvelope > fReleaseThresh)
                fEnvelope       += (s > fEnvelope) ? fTauAttack * (s - fEnvelope) : fTauRelease * (s - fEnvelope);
            else
                fEnvelope       += fTauAttack * (s - fEnvelope);

            if (env != NULL)
                *env    = fEnvelope;

            return amplification(fEnvelope);
        }

        void Expander::curve(float *out, const float *in, size_t dots)
        {
            if (bUpward)
            {
                for (size_t i=0; i<dots; ++i)
                {
                    float x     = fabsf(in[i]);
                    if (x > FLOAT_SAT_P_INF)
                        out[i]      = FLOAT_SAT_P_INF * x;
                    else if (x > fKS)
                    {
                        float lx    = logf(x);
                        out[i]      = (x >= fKE) ?
                                      x * expf(vTilt[0]*lx + vTilt[1]) :
                                      x * expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                    }
                    else
                        out[i]      = x;
                }
            }
            else
            {
                for (size_t i=0; i<dots; ++i)
                {
                    float x     = lsp_max(fabsf(in[i]), FLOAT_SAT_M_INF);
                    if (x < FLOAT_SAT_M_INF)
                        out[i]      = 0.0f;
                    else if (x < fKE)
                    {
                        float lx    = logf(x);
                        out[i]      = (x <= fKS) ?
                                       x * expf(vTilt[0]*lx + vTilt[1]) :
                                       x * expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                    }
                    else
                        out[i]      = x;
                }
            }
        }

        float Expander::curve(float in)
        {
            float x     = fabsf(in);

            if (bUpward)
            {
                if (x > FLOAT_SAT_P_INF)
                    return FLOAT_SAT_P_INF * x;

                if (x > fKS)
                {
                    float lx    = logf(x);
                    return (x >= fKE) ?
                            x * expf(vTilt[0]*lx + vTilt[1]) :
                            x * expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                }
            }
            else
            {
                if (x < FLOAT_SAT_M_INF)
                    return 0.0f;

                if (x < fKE)
                {
                    float lx    = logf(x);
                    return (x <= fKS) ?
                            x * expf(vTilt[0]*lx + vTilt[1]) :
                            x * expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                }
            }

            return x;
        }

        void Expander::amplification(float *out, const float *in, size_t dots)
        {
            if (bUpward)
            {
                for (size_t i=0; i<dots; ++i)
                {
                    float x     = fabsf(in[i]);
                    if (x > FLOAT_SAT_P_INF)
                        out[i]      = FLOAT_SAT_P_INF;
                    else if (x > fKS)
                    {
                        float lx    = logf(x);
                        out[i]      = (x >= fKE) ?
                                      expf(vTilt[0]*lx + vTilt[1]) :
                                      expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                    }
                    else
                        out[i]      = 1.0f;
                }
            }
            else
            {
                for (size_t i=0; i<dots; ++i)
                {
                    float x     = lsp_max(fabsf(in[i]), FLOAT_SAT_M_INF);
                    if (x < FLOAT_SAT_M_INF)
                        out[i]      = 0.0f;
                    else if (x < fKE)
                    {
                        float lx    = logf(x);
                        out[i]      = (x <= fKS) ?
                                       expf(vTilt[0]*lx + vTilt[1]) :
                                       expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                    }
                    else
                        out[i]      = 1.0f;
                }
            }
        }

        float Expander::amplification(float in)
        {
            float x     = fabsf(in);

            if (bUpward)
            {
                if (x > FLOAT_SAT_P_INF)
                    return FLOAT_SAT_P_INF;

                if (x > fKS)
                {
                    float lx    = logf(x);
                    return (x >= fKE) ?
                            expf(vTilt[0]*lx + vTilt[1]) :
                            expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                }
            }
            else
            {
                if (x < FLOAT_SAT_M_INF)
                    return 0.0f;

                if (x < fKE)
                {
                    float lx    = logf(x);
                    return (x <= fKS) ?
                            expf(vTilt[0]*lx + vTilt[1]) :
                            expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                }
            }

            return 1.0f;
        }

        void Expander::dump(IStateDumper *v) const
        {
            v->write("fAttackThresh", fAttackThresh);
            v->write("fReleaseThresh", fReleaseThresh);
            v->write("fAttack", fAttack);
            v->write("fRelease", fRelease);
            v->write("fKnee", fKnee);
            v->write("fRatio", fRatio);
            v->write("fEnvelope", fEnvelope);
            v->write("fTauAttack", fTauAttack);
            v->write("fTauRelease", fTauRelease);
            v->write("fKS", fKS);
            v->write("fKE", fKE);
            v->writev("vHermite", vHermite, 3);
            v->writev("vTilt", vTilt, 2);
            v->write("nSampleRate", nSampleRate);
            v->write("bUpdate", bUpdate);
            v->write("bUpward", bUpward);
        }
    } /* namespace dspu */
} /* namespace lsp */
