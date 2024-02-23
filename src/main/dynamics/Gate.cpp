/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
    
                c->fThreshold       = 0.0f;
                c->fZone            = 1.0f;
                c->sKnee.start      = 0.0f;
                c->sKnee.end        = 0.0f;
                c->sKnee.gain_start = 0.0f;
                c->sKnee.gain_end   = 0.0f;
                c->sKnee.herm[0]    = 0.0f;
                c->sKnee.herm[1]    = 0.0f;
                c->sKnee.herm[2]    = 0.0f;
                c->sKnee.herm[3]    = 0.0f;
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

        void Gate::set_threshold(float topen, float tclose)
        {
            if ((topen == sCurves[0].fThreshold) && (tclose == sCurves[1].fThreshold))
                return;
            sCurves[0].fThreshold   = topen;
            sCurves[1].fThreshold   = tclose;
            bUpdate                 = true;
        }

        void Gate::set_open_threshold(float threshold)
        {
            if (threshold == sCurves[0].fThreshold)
                return;
            sCurves[0].fThreshold   = threshold;
            bUpdate                 = true;
        }

        void Gate::set_close_threshold(float threshold)
        {
            if (threshold == sCurves[1].fThreshold)
                return;
            sCurves[1].fThreshold   = threshold;
            bUpdate                 = true;
        }

        void Gate::set_reduction(float reduction)
        {
            if (reduction == fReduction)
                return;
            fReduction          = reduction;
            bUpdate             = true;
        }

        void Gate::set_timings(float attack, float release)
        {
            if ((fAttack == attack) && (fRelease == release))
                return;
            fAttack     = attack;
            fRelease    = release;
            bUpdate     = true;
        }

        void Gate::set_attack(float attack)
        {
            if (fAttack == attack)
                return;
            fAttack     = attack;
            bUpdate     = true;
        }

        void Gate::set_release(float release)
        {
            if (fRelease == release)
                return;
            fRelease    = release;
            bUpdate     = true;
        }

        void Gate::set_sample_rate(size_t sr)
        {
            if (sr == nSampleRate)
                return;
            nSampleRate = sr;
            bUpdate     = true;
        }

        void Gate::set_zone(float open, float close)
        {
            if ((open == sCurves[0].fZone) && (close == sCurves[1].fZone))
                return;
            sCurves[0].fZone    = open;
            sCurves[1].fZone    = close;
            bUpdate             = true;
        }

        void Gate::set_open_zone(float zone)
        {
            if (zone == sCurves[0].fZone)
                return;
            sCurves[0].fZone    = zone;
            bUpdate             = true;
        }

        void Gate::set_close_zone(float zone)
        {
            if (zone == sCurves[1].fZone)
                return;
            sCurves[1].fZone    = zone;
            bUpdate             = true;
        }

        void Gate::set_hold(float hold)
        {
            hold                = lsp_max(hold, 0.0f);
            if (hold == fHold)
                return;
            fHold               = hold;
            bUpdate             = true;
        }
    
        void Gate::update_settings()
        {
            // Update settings if necessary
            fTauAttack      = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fAttack)));
            fTauRelease     = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (millis_to_samples(nSampleRate, fRelease)));
            nHold           = millis_to_samples(nSampleRate, fHold);

            // Calculate interpolation parameters
            for (size_t i=0; i<2; ++i)
            {
                curve_t *c          = &sCurves[i];

                c->sKnee.start      = c->fThreshold * c->fZone;
                c->sKnee.end        = c->fThreshold;
                c->sKnee.gain_start = (fReduction <= 1.0f) ? fReduction : 1.0f;
                c->sKnee.gain_end   = (fReduction <= 1.0f) ? 1.0f : 1.0f / fReduction;

                interpolation::hermite_cubic(
                    c->sKnee.herm,
                    logf(c->sKnee.start), logf(c->sKnee.gain_start), 0.0f,
                    logf(c->sKnee.end), logf(c->sKnee.gain_end), 0.0f);
            }

            // Reset update flag
            bUpdate         = false;
        }

        void Gate::curve(float *out, const float *in, size_t dots, bool hyst) const
        {
            dsp::gate_x1_curve(out, in, &sCurves[(hyst) ? 1 : 0].sKnee, dots);
        }

        float Gate::curve(float in, bool hyst) const
        {
            const dsp::gate_knee_t *c   = &sCurves[(hyst) ? 1 : 0].sKnee;
            float x             = fabsf(in);
            if (x <= c->start)
                x          *= c->gain_start;
            else if (x >= c->end)
                x          *= c->gain_end;
            else
            {
                float lx        = logf(x);
                x              *= expf(((c->herm[0]*lx + c->herm[1])*lx + c->herm[2])*lx + c->herm[3]);
            }
            return x;
        }

        void Gate::amplification(float *out, const float *in, size_t dots, bool hyst) const
        {
            dsp::gate_x1_gain(out, in, &sCurves[(hyst) ? 1 : 0].sKnee, dots);
        }

        float Gate::amplification(float in, bool hyst) const
        {
            const dsp::gate_knee_t *c   = &sCurves[(hyst) ? 1 : 0].sKnee;
            float x             = fabsf(in);
            if (x <= c->start)
                x           = c->gain_start;
            else if (x >= c->end)
                x           = c->gain_end;
            else
            {
                float lx    = logf(x);
                x           = expf(((c->herm[0]*lx + c->herm[1])*lx + c->herm[2])*lx + c->herm[3]);
            }

            return x;
        }

        float Gate::amplification(float in) const
        {
            const dsp::gate_knee_t *c   = &sCurves[nCurve].sKnee;
            float x             = fabsf(in);
            if (x <= c->start)
                x           = c->gain_start;
            else if (x >= c->end)
                x           = c->gain_end;
            else
            {
                float lx    = logf(x);
                x           = expf(((c->herm[0]*lx + c->herm[1])*lx + c->herm[2])*lx + c->herm[3]);
            }

            return x;
        }

        void Gate::process(float *out, float *env, const float *in, size_t samples)
        {
            size_t curr_i = 0, prev_i = 0;

            while (prev_i < samples)
            {
                // Process envelope, split the input stream into bulk parts of usage of the same curve.
                curve_t *c      = &sCurves[nCurve];
                float e         = fEnvelope;
                float peak      = fPeak;
                uint32_t hold   = nHoldCounter;

                if (nCurve == 0)
                {
                    // Calculate envelope of the gate
                    for (; curr_i < samples; ++curr_i)
                    {
                        float s         = in[curr_i];
                        float d         = s - e;

                        // Change state
                        if (d < 0.0f)
                        {
                            if (hold > 0)
                                --hold;
                            else
                            {
                                e              += fTauRelease * d;
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
                        out[curr_i]     = e;

                        // Change current curve and leave cycle if the value of envelope is above threshold
                        if (e > c->sKnee.end)
                        {
                            nCurve          = 1;
                            break;
                        }
                    }
                }
                else
                {
                    // Calculate envelope of the gate
                    for (; curr_i < samples; ++curr_i)
                    {
                        float s         = in[curr_i];
                        float d         = s - e;

                        // Change state
                        if (d < 0.0f)
                        {
                            if (hold > 0)
                                --hold;
                            else
                            {
                                e              += fTauRelease * d;
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
                        out[curr_i]     = e;

                        // Change current curve and leave cycle if the value of envelope is below threshold
                        if (e < c->sKnee.start)
                        {
                            nCurve          = 0;
                            break;
                        }
                    }
                }
                fEnvelope       = e;
                fPeak           = peak;
                nHoldCounter    = hold;

                // Update result
                if (env != NULL)
                    dsp::copy(&env[prev_i], &out[prev_i], curr_i - prev_i);
                dsp::gate_x1_gain(&out[prev_i], &out[prev_i], &c->sKnee, curr_i - prev_i);

                // Update position
                prev_i  = curr_i;
            }
        }

        float Gate::process(float *env, float s)
        {
            // Change state
            curve_t *c      = &sCurves[nCurve];
            float d         = s - fEnvelope;
            if (d < 0.0f)
            {
                if (nHoldCounter > 0)
                    --nHoldCounter;
                else
                {
                    fEnvelope      += fTauRelease * d;
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

            // Select curve depending on the value of envelope
            if (fEnvelope < c->sKnee.start)
                nCurve          = 0;
            else if (fEnvelope > c->sKnee.end)
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

                    const dsp::gate_knee_t *k = &c->sKnee;
                    v->begin_object("sKnee", k, sizeof(dsp::gate_knee_t));
                    {
                        v->write("start", k->start);
                        v->write("end", k->end);
                        v->write("gain_start", k->gain_start);
                        v->write("gain_end", k->gain_end);
                        v->writev("herm", k->herm, 4);
                    }
                    v->end_object();
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
            v->write("fHold", fHold);
            v->write("fPeak", fPeak);

            v->write("nHold", nHold);
            v->write("nHoldCounter", nHoldCounter);
            v->write("nSampleRate", nSampleRate);
            v->write("nCurve", nCurve);
            v->write("bUpdate", bUpdate);
        }
    } /* namespace dspu */
} /* namespace lsp */
