/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 15 июл. 2023 г.
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

#include <lsp-plug.in/dsp-units/dynamics/SurgeProtector.h>

#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {

        SurgeProtector::SurgeProtector()
        {
            construct();
        }

        SurgeProtector::~SurgeProtector()
        {
            destroy();
        }

        void SurgeProtector::construct()
        {
            fGain               = 0.0f;
            nTransitionTime     = 0;
            nTransitionMax      = 0;
            nShutdownTime       = 0;
            nShutdownMax        = 0;
            fOnThreshold        = 0.0f;
            fOffThreshold       = 0.0f;
            bOn                 = false;
        }

        void SurgeProtector::destroy()
        {
            fGain               = 0.0f;
            nTransitionTime     = 0;
            nTransitionMax      = 0;
            nShutdownTime       = 0;
            nShutdownMax        = 0;
            fOnThreshold        = 0.0f;
            fOffThreshold       = 0.0f;
            bOn                 = false;
        }

        void SurgeProtector::reset()
        {
            bOn                 = false;
            nTransitionTime     = 0;
        }

        void SurgeProtector::set_transition_time(size_t time)
        {
            nTransitionMax      = time;
            nTransitionTime     = lsp_limit(nTransitionTime, 0U, time);
        }

        void SurgeProtector::set_shutdown_time(size_t time)
        {
            nShutdownMax        = time;
            nShutdownTime       = lsp_limit(nShutdownTime, 0U, time);
        }

        void SurgeProtector::set_on_threshold(float threshold)
        {
            fOnThreshold        = threshold;
        }

        void SurgeProtector::set_off_threshold(float threshold)
        {
            fOffThreshold       = threshold;
        }

        void SurgeProtector::set_threshold(float on, float off)
        {
            fOnThreshold        = on;
            fOffThreshold       = off;
        }

        float SurgeProtector::process(float in)
        {
            // Control the on/off state
            if (bOn)
            {
                // Protector is on
                if (in >= fOffThreshold)
                    nShutdownTime       = 0;
                else
                    ++nShutdownTime;

                // Turn off the protector if time limit has reached the end
                if (nShutdownTime >= nShutdownMax)
                    bOn                 = false;
            }
            else
            {
                // Protector is off
                if (in >= fOnThreshold)
                {
                    // Turn on the protector
                    bOn                 = true;
                    nShutdownTime       = 0;
                }
            }

            // Update gain depending on the protector state
            if (bOn)
            {
                if (nTransitionTime < nTransitionMax)
                {
                    fGain           = sqrtf(float(nTransitionTime) / float(nTransitionMax));
                    ++nTransitionTime;
                }
                else
                    fGain           = 1.0f;
            }
            else
            {
                if (nTransitionTime > 0)
                {
                    fGain           = sqrtf(float(nTransitionTime) / float(nTransitionMax));
                    --nTransitionTime;
                }
                else
                    fGain           = 0.0f;
            }

            // Return current gain value
            return fGain;
        }

        void SurgeProtector::process(const float *in, size_t samples)
        {
            for (size_t i=0; i<samples; ++i)
                process(in[i]);
        }

        void SurgeProtector::process(float *out, const float *in, size_t samples)
        {
            if (out == NULL)
            {
                process(in, samples);
                return;
            }

            for (size_t i=0; i<samples; ++i)
                out[i] = process(in[i]);
        }

        void SurgeProtector::process_mul(float *out, const float *in, size_t samples)
        {
            if (out == NULL)
            {
                process(in, samples);
                return;
            }

            for (size_t i=0; i<samples; ++i)
                out[i] *= process(in[i]);
        }

        void SurgeProtector::dump(IStateDumper *v) const
        {
            v->write("fGain", fGain);
            v->write("nTransitionTime", nTransitionTime);
            v->write("nTransitionMax", nTransitionMax);
            v->write("nShutdownTime", nShutdownTime);
            v->write("nShutdownMax", nShutdownMax);
            v->write("fOnThreshold", fOnThreshold);
            v->write("fOffThreshold", fOffThreshold);
            v->write("bOn", bOn);
        }

    } /* namespace dspu */
} /* namespace lsp */

