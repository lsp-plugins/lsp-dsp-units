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
        static constexpr float MINIMUM_TILT         = 0.001f;           // minimum tilt value
        static constexpr float UPPER_THRESHOLD      = 13.815510558f;    // logf(10e+6)
        static constexpr float LOWER_THRESHOLD      = -16.118095651f;   // logf(10e-7)
        static constexpr float MIN_LOWER_THRESHOLD  = 1e-7f;
        static constexpr float MAX_UPPER_THRESHOLD  = 1e+6f;

        typedef struct sqroot_t
        {
            float x1;
            float x2;
        } sqroot_t;

        static inline sqroot_t square_roots(const float *p, float y)
        {
            // Solve equation: p[0] * x^2 + p[1] * x + p[2] - y = 0
            const float a = p[0];
            const float b = -p[1];
            const float c = p[2] - y;
            const float d = sqrtf(b*b - 4.0f*a*c);
            const float k = 1.0f / (a + a);

            return sqroot_t {
                (b + d) * k,
                (b - d) * k
            };
        }


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
            sExp.threshold  = 0.0f;
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

            sExp.tilt[0]    = fRatio - 1.0f;
            sExp.tilt[1]    = log_th * (1.0f - fRatio);

            if (bUpward)
            {
                interpolation::hermite_quadratic(sExp.herm, log_ks, 0.0f, 0.0f, log_ke, sExp.tilt[0]);
                float ut = expf((UPPER_THRESHOLD - sExp.tilt[1])/lsp_max(sExp.tilt[0], MINIMUM_TILT));
                if (ut < sExp.end)
                {
                    sqroot_t ur     = square_roots(sExp.herm, UPPER_THRESHOLD);
                    ut              = expf(lsp_max(ur.x1, ur.x2));
                }
                sExp.threshold  = lsp_min(ut, MAX_UPPER_THRESHOLD);
            }
            else
            {
                interpolation::hermite_quadratic(sExp.herm, log_ke, 0.0f, 0.0f, log_ks, sExp.tilt[0]);
                float dt = expf((LOWER_THRESHOLD - sExp.tilt[1])/lsp_max(sExp.tilt[0], MINIMUM_TILT));
                if (dt > sExp.start)
                {
                    sqroot_t dr     = square_roots(sExp.herm, LOWER_THRESHOLD);
                    dt              = expf(lsp_min(dr.x1, dr.x2));
                }
                sExp.threshold  = lsp_max(dt, MIN_LOWER_THRESHOLD);
            }

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
                if (x > sExp.threshold)
                    x = sExp.threshold;

                if (x > sExp.start)
                {
                    float lx    = logf(x);
                    return (x >= sExp.end) ?
                            x * expf(sExp.tilt[0]*lx + sExp.tilt[1]) :
                            x * expf((sExp.herm[0]*lx + sExp.herm[1])*lx + sExp.herm[2]);
                }
            }
            else
            {
                if (x < sExp.threshold)
                    return 0.0f;

                if (x < sExp.end)
                {
                    float lx    = logf(x);
                    return (x <= sExp.start) ?
                            x * expf(sExp.tilt[0]*lx + sExp.tilt[1]) :
                            x * expf((sExp.herm[0]*lx + sExp.herm[1])*lx + sExp.herm[2]);
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
                if (x > sExp.threshold)
                    x = sExp.threshold;

                if (x > sExp.start)
                {
                    float lx    = logf(x);
                    return (x >= sExp.end) ?
                            expf(sExp.tilt[0]*lx + sExp.tilt[1]) :
                            expf((sExp.herm[0]*lx + sExp.herm[1])*lx + sExp.herm[2]);
                }
            }
            else
            {
                if (x < sExp.threshold)
                    return 0.0f;

                if (x < sExp.end)
                {
                    float lx    = logf(x);
                    return (x <= sExp.start) ?
                            expf(sExp.tilt[0]*lx + sExp.tilt[1]) :
                            expf((sExp.herm[0]*lx + sExp.herm[1])*lx + sExp.herm[2]);
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
                v->write("thresh", sExp.threshold);
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
