/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 7 нояб. 2016 г.
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

#include <lsp-plug.in/dsp-units/dynamics/Gate.h>
#include <lsp-plug.in/dsp-units/misc/interpolation.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        Gate::Gate()
        {
            construct();
        }

        Gate::~Gate()
        {
            destroy();
        }

        void Gate::construct()
        {
            for (size_t i=0; i<2; ++i)
            {
                curve_t *c = &sCurves[i];
    
                c->fThreshold   = 0.0f;
                c->fZone        = 1.0f;
                c->fZS          = 0.0f;
                c->fZE          = 0.0f;
                c->vHermite[0]  = 0.0f;
                c->vHermite[1]  = 0.0f;
                c->vHermite[2]  = 0.0f;
                c->vHermite[3]  = 0.0f;
            }

            fAttack         = 0.0f;
            fRelease        = 0.0f;
            fTauAttack      = 0.0f;
            fTauRelease     = 0.0f;
            fReduction      = 0.0f;
            fEnvelope       = 0.0f;

            // Additional parameters
            nSampleRate     = 0;
            nCurve          = 0;
            bUpdate         = true;
        }

        void Gate::destroy()
        {
        }
    
        void Gate::update_settings()
        {
            // Update settings if necessary
            fTauAttack      = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fAttack)));
            fTauRelease     = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fRelease)));

            // Calculate interpolation parameters
            for (size_t i=0; i<2; ++i)
            {
                curve_t *c      = &sCurves[i];

                c->fZS          = c->fThreshold * c->fZone;
                c->fZE          = c->fThreshold;
                c->fZSGain      = (fReduction <= 1.0f) ? fReduction : 1.0f;
                c->fZEGain      = (fReduction <= 1.0f) ? 1.0f : 1.0f / fReduction;

                interpolation::hermite_cubic(
                    c->vHermite,
                    logf(c->fZS), logf(c->fZSGain), 0.0f,
                    logf(c->fZE), logf(c->fZEGain), 0.0f);
            }

            // Reset update flag
            bUpdate         = false;
        }

        void Gate::curve(float *out, const float *in, size_t dots, bool hyst) const
        {
            const curve_t *c    = &sCurves[(hyst) ? 1 : 0];

            for (size_t i=0; i<dots; ++i)
            {
                float x     = lsp_abs(in[i]);
                float lx    = logf(lsp_limit(x, c->fZS, c->fZE));
                if (x <= c->fZS)
                    x          *= c->fZSGain;
                else if (x >= c->fZE)
                    x          *= c->fZEGain;
                else
                    x          *= expf(((c->vHermite[0]*lx + c->vHermite[1])*lx + c->vHermite[2])*lx + c->vHermite[3]);
                out[i]      = x;
            }
        }

        float Gate::curve(float in, bool hyst) const
        {
            const curve_t *c    = &sCurves[(hyst) ? 1 : 0];
            float x             = lsp_abs(in);
            float lx            = logf(lsp_limit(x, c->fZS, c->fZE));
            if (x <= c->fZS)
                x          *= c->fZSGain;
            else if (x >= c->fZE)
                x          *= c->fZEGain;
            else
                x          *= expf(((c->vHermite[0]*lx + c->vHermite[1])*lx + c->vHermite[2])*lx + c->vHermite[3]);
            return x;
        }

        void Gate::amplification(float *out, const float *in, size_t dots, bool hyst) const
        {
            const curve_t *c    = &sCurves[(hyst) ? 1 : 0];

            for (size_t i=0; i<dots; ++i)
            {
                float x     = lsp_abs(in[i]);
                float lx    = logf(lsp_limit(x, c->fZS, c->fZE));
                if (x <= c->fZS)
                    x           = c->fZSGain;
                else if (x >= c->fZE)
                    x           = c->fZEGain;
                else
                    x           = expf(((c->vHermite[0]*lx + c->vHermite[1])*lx + c->vHermite[2])*lx + c->vHermite[3]);
                out[i]      = x;
            }
        }

        float Gate::amplification(float in, bool hyst) const
        {
            const curve_t *c    = &sCurves[(hyst) ? 1 : 0];
            float x             = lsp_abs(in);
            float lx            = logf(lsp_limit(x, c->fZS, c->fZE));
            if (x <= c->fZS)
                x           = c->fZSGain;
            else if (x >= c->fZE)
                x           = c->fZEGain;
            else
                x           = expf(((c->vHermite[0]*lx + c->vHermite[1])*lx + c->vHermite[2])*lx + c->vHermite[3]);

            return x;
        }

        float Gate::amplification(float in) const
        {
            const curve_t *c    = &sCurves[nCurve];
            float x             = lsp_abs(in);
            float lx            = logf(lsp_limit(x, c->fZS, c->fZE));
            if (x <= c->fZS)
                x           = c->fZSGain;
            else if (x >= c->fZE)
                x           = c->fZEGain;
            else
                x           = expf(((c->vHermite[0]*lx + c->vHermite[1])*lx + c->vHermite[2])*lx + c->vHermite[3]);

            return x;
        }

        void Gate::process(float *out, float *env, const float *in, size_t samples)
        {
            // Calculate envelope of gate
            for (size_t i=0; i<samples; ++i)
            {
                float s         = in[i];

                // Change state
                curve_t *c      = &sCurves[nCurve];
                fEnvelope      += (s > fEnvelope) ? fTauAttack * (s - fEnvelope) : fTauRelease * (s - fEnvelope);
                if (fEnvelope < c->fZS)
                    nCurve          = 0;
                else if (fEnvelope > c->fZE)
                    nCurve          = 1;

                // Update result
                c               = &sCurves[nCurve];
                if (env != NULL)
                    env[i]          = fEnvelope;
                out[i]          = amplification(fEnvelope);
            }
        }

        float Gate::process(float *env, float s)
        {
            // Change state
            curve_t *c      = &sCurves[nCurve];
            fEnvelope      += (s > fEnvelope) ? fTauAttack * (s - fEnvelope) : fTauRelease * (s - fEnvelope);
            if (fEnvelope < c->fZS)
                nCurve          = 0;
            else if (fEnvelope > c->fZE)
                nCurve          = 1;

            // Compute result
            c               = &sCurves[nCurve];
            s               = amplification(fEnvelope);
            if (env != NULL)
                *env = fEnvelope;

            return s;
        }

        void Gate::dump(IStateDumper *v) const
        {
            v->begin_array("sCurves", sCurves, 2);
            for (size_t i=0; i<2; ++i)
            {
                const curve_t *c = &sCurves[i];
                v->begin_object(c, sizeof(curve_t));
                {
                    v->write("fThreshold", c->fThreshold);
                    v->write("fZone", c->fZone);
                    v->write("fZS", c->fZS);
                    v->write("fZE", c->fZE);
                    v->write("fZSGain", c->fZSGain);
                    v->write("fZEGain", c->fZEGain);
                    v->writev("vHermite", c->vHermite, 4);
                }
                v->end_object();
            }
            v->end_array();

            v->write("fAttack", fAttack);
            v->write("fRelease", fRelease);
            v->write("fTauAttack", fTauAttack);
            v->write("fTauRelease", fTauRelease);
            v->write("fReduction", fReduction);
            v->write("fEnvelope", fEnvelope);

            v->write("nSampleRate", nSampleRate);
            v->write("nCurve", nCurve);
            v->write("bUpdate", bUpdate);
        }
    }
} /* namespace lsp */
