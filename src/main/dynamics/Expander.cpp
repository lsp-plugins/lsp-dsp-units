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
            sExp.start      = 0.0f;
            sExp.end        = 0.0f;
            sExp.herm[0]    = 0.0f;
            sExp.herm[1]    = 0.0f;
            sExp.herm[2]    = 0.0f;
            sExp.tilt[0]    = 0.0f;
            sExp.tilt[1]    = 0.0f;

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
            sExp.start      = fAttackThresh * fKnee;
            sExp.end        = fAttackThresh / fKnee;
            float log_ks    = logf(sExp.start);                 // Knee start
            float log_ke    = logf(sExp.end);                   // Knee end
            float log_th    = logf(fAttackThresh);              // Attack threshold

            if (bUpward)
                interpolation::hermite_quadratic(sExp.herm, log_ks, log_ks, 1.0f, log_ke, fRatio);
            else
                interpolation::hermite_quadratic(sExp.herm, log_ke, log_ke, 1.0f, log_ks, fRatio);

            sExp.tilt[0]    = fRatio - 1.0f;
            sExp.tilt[1]    = log_th * (1.0f - fRatio);

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
                dsp::uexpander_x1_curve(out, in, &sExp, dots);
            else
                dsp::dexpander_x1_curve(out, in, &sExp, dots);
        }

        float Expander::curve(float in)
        {
            float x     = fabsf(in);

            if (bUpward)
            {
                if (x > LSP_DSP_LIB_EXP_INF_SAT)
                    return LSP_DSP_LIB_EXP_INF_SAT * x;

                if (x > sExp.start)
                {
                    float lx    = logf(x);
                    return (x >= sExp.end) ?
                            x * expf(sExp.tilt[0]*lx + sExp.tilt[1]) :
                            x * expf((sExp.herm[0]*lx + sExp.herm[1] - 1.0f)*lx + sExp.herm[2]);
                }
            }
            else
            {
                if (x < FLOAT_SAT_M_INF)
                    return 0.0f;

                if (x < sExp.end)
                {
                    float lx    = logf(x);
                    return (x <= sExp.start) ?
                            x * expf(sExp.tilt[0]*lx + sExp.tilt[1]) :
                            x * expf((sExp.herm[0]*lx + sExp.herm[1] - 1.0f)*lx + sExp.herm[2]);
                }
            }

            return x;
        }

        void Expander::amplification(float *out, const float *in, size_t dots)
        {
            if (bUpward)
                dsp::uexpander_x1_gain(out, in, &sExp, dots);
            else
                dsp::dexpander_x1_gain(out, in, &sExp, dots);
        }

        float Expander::amplification(float in)
        {
            float x     = fabsf(in);

            if (bUpward)
            {
                if (x > FLOAT_SAT_P_INF)
                    return FLOAT_SAT_P_INF;

                if (x > sExp.start)
                {
                    float lx    = logf(x);
                    return (x >= sExp.end) ?
                            expf(sExp.tilt[0]*lx + sExp.tilt[1]) :
                            expf((sExp.herm[0]*lx + sExp.herm[1] - 1.0f)*lx + sExp.herm[2]);
                }
            }
            else
            {
                if (x < FLOAT_SAT_M_INF)
                    return 0.0f;

                if (x < sExp.end)
                {
                    float lx    = logf(x);
                    return (x <= sExp.start) ?
                            expf(sExp.tilt[0]*lx + sExp.tilt[1]) :
                            expf((sExp.herm[0]*lx + sExp.herm[1] - 1.0f)*lx + sExp.herm[2]);
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

            v->begin_object("sExp", &sExp, sizeof(dsp::expander_knee_t));
            {
                v->write("start", sExp.start);
                v->write("end", sExp.end);
                v->writev("herm", sExp.herm, 3);
                v->writev("tilt", sExp.tilt, 2);
            }
            v->end_object();

            v->write("nSampleRate", nSampleRate);
            v->write("bUpdate", bUpdate);
            v->write("bUpward", bUpward);
        }
    } /* namespace dspu */
} /* namespace lsp */
