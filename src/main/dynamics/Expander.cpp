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
            vHermite[0]     = 0.0f;
            vHermite[1]     = 0.0f;
            vHermite[2]     = 0.0f;
            fLogKS          = 0.0f;
            fLogKE          = 0.0f;
            fLogTH          = 0.0f;

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
            // Update settings if necessary
            fTauAttack      = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fAttack)));
            fTauRelease     = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fRelease)));

            // Calculate interpolation parameters
            fLogKS          = logf(fAttackThresh * fKnee);      // Knee start
            fLogKE          = logf(fAttackThresh / fKnee);      // Knee end
            fLogTH          = logf(fAttackThresh);              // Attack threshold

            if (bUpward)
                interpolation::hermite_quadratic(vHermite, fLogKS, fLogKS, 1.0f, fLogKE, fRatio);
            else
                interpolation::hermite_quadratic(vHermite, fLogKE, fLogKE, 1.0f, fLogKS, fRatio);

            // Reset update flag
            bUpdate         = false;
        }

        void Expander::process(float *out, float *env, const float *in, size_t samples)
        {
            // Calculate envelope of expander
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
                    float x     = fabsf(*(in++));
                    if (x > FLOAT_SAT_P_INF)
                        x       = FLOAT_SAT_P_INF;

                    float lx    = logf(x);
                    if (lx > fLogKS)
                    {
                        x   = (lx >= fLogKE) ?
                            expf(fRatio*(lx - fLogTH) + fLogTH) :
                            expf((vHermite[0]*lx + vHermite[1])*lx + vHermite[2]);
                        *out        = x;
                    }
                    else
                        *out        = x;
                    out++;
                }
            }
            else
            {
                for (size_t i=0; i<dots; ++i)
                {
                    float x     = fabsf(*(in++));
                    float lx    = logf(x);
                    if (lx < fLogKE)
                    {
                        x   = (lx <= fLogKS) ?
                            expf(fRatio*(lx - fLogTH) + fLogTH) :
                            expf((vHermite[0]*lx + vHermite[1])*lx + vHermite[2]);
                        *out        = x;
                    }
                    else
                        *out        = x;
                    out++;
                }
            }
        }

        float Expander::curve(float in)
        {
            in      = fabsf(in);

            if (bUpward)
            {
                if (in > FLOAT_SAT_P_INF)
                    in      = FLOAT_SAT_P_INF;

                float lx    = logf(in);
                if (lx > fLogKS)
                    in = (lx >= fLogKE) ?
                        expf(fRatio*(lx - fLogTH) + fLogTH) :
                        expf((vHermite[0]*lx + vHermite[1])*lx + vHermite[2]);
            }
            else
            {
                float lx    = logf(in);
                if (lx < fLogKE)
                    in = (lx <= fLogKS) ?
                        expf(fRatio*(lx - fLogTH) + fLogTH) :
                        expf((vHermite[0]*lx + vHermite[1])*lx + vHermite[2]);
            }

            return in;
        }

        void Expander::amplification(float *out, const float *in, size_t dots)
        {
            if (bUpward)
            {
                for (size_t i=0; i<dots; ++i)
                {
                    float x     = fabsf(*(in++));
                    if (x > FLOAT_SAT_P_INF)
                        x       = FLOAT_SAT_P_INF;

                    float lx    = logf(x);
                    if (lx > fLogKS)
                    {
                        *out    = (lx >= fLogKE) ?
                            expf((fRatio - 1.0f)*(lx - fLogTH)) :
                            expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                    }
                    else
                        *out        = 1.0f;
                    out++;
                }
            }
            else
            {
                for (size_t i=0; i<dots; ++i)
                {
                    float x     = fabsf(*(in++));
                    float lx    = logf(x);
                    if (lx < fLogKE)
                    {
                        *out    = (lx <= fLogKS) ?
                            expf((fRatio - 1.0f)*(lx - fLogTH)) :
                            expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
                    }
                    else
                        *out        = 1.0f;
                    out++;
                }
            }
        }

        float Expander::amplification(float in)
        {
            in      = fabsf(in);

            if (bUpward)
            {
                if (in > FLOAT_SAT_P_INF)
                    in      = FLOAT_SAT_P_INF;

                float lx    = logf(in);
                if (lx > fLogKS)
                    return (lx >= fLogKE) ?
                        expf((fRatio - 1.0f)*(lx - fLogTH)) :
                        expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);

            }
            else
            {
                float lx    = logf(in);
                if (lx < fLogKE)
                    return (lx <= fLogKS) ?
                        expf((fRatio - 1.0f)*(lx - fLogTH)) :
                        expf((vHermite[0]*lx + vHermite[1] - 1.0f)*lx + vHermite[2]);
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
            v->writev("vHermite", vHermite, 3);
            v->write("fLogKS", fLogKS);
            v->write("fLogKE", fLogKE);
            v->write("fLogTH", fLogTH);
            v->write("nSampleRate", nSampleRate);
            v->write("bUpdate", bUpdate);
            v->write("bUpward", bUpward);
        }
    } /* namespace dspu */
} /* namespace lsp */
