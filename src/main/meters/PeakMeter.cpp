/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 4 окт. 2025 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/dsp-units/meters/PeakMeter.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        PeakMeter::PeakMeter()
        {
            construct();
        }

        PeakMeter::~PeakMeter()
        {
            destroy();
        }

        void PeakMeter::construct()
        {
            fPeak       = 0.0f;
            fTau        = 1.0f;
            fHold       = 5.0f;
            fRelease    = 0.0f;
            nHold       = 0.0f;
            nCounter    = 0.0f;
            nSampleRate = 0.0f;
            bUpdate     = true;
        }

        void PeakMeter::destroy()
        {
            // Nothing
        }

        void PeakMeter::set_sample_rate(size_t sample_rate)
        {
            if (nSampleRate == sample_rate)
                return;
            nSampleRate     = sample_rate;
            bUpdate         = true;
        }

        size_t PeakMeter::sample_rate() const
        {
            return nSampleRate;
        }

        void PeakMeter::set_hold_time(float value)
        {
            if (fHold == value)
                return;
            fHold           = value;
            bUpdate         = true;
        }

        float PeakMeter::hold_time() const
        {
            return fHold;
        }

        void PeakMeter::set_release_time(float value)
        {
            if (fRelease == value)
                return;
            fRelease        = value;
            bUpdate         = true;
        }

        float PeakMeter::release_time() const
        {
            return fRelease;
        }

        void PeakMeter::set_time(float hold, float release)
        {
            if ((fHold == hold) && (fRelease == release))
                return;
            fHold           = hold;
            fRelease        = release;
            bUpdate         = true;
        }

        float PeakMeter::value() const
        {
            return fPeak;
        }

        void PeakMeter::clear()
        {
            nCounter        = 0;
            fPeak           = 0.0f;
        }

        void PeakMeter::update_settings()
        {
            if (!bUpdate)
                return;

            bUpdate         = false;
            nHold           = dspu::millis_to_samples(nSampleRate, fHold);
            fTau            = expf(logf(1.0f - float(M_SQRT1_2)) / (dspu::millis_to_samples(nSampleRate, fRelease)));
            nCounter        = lsp_min(nCounter, nHold);
        }

        float PeakMeter::process(float *dst, const float *data, size_t count)
        {
            update_settings();

            float peak = fPeak;
            for (size_t i=0; i<count; ++i)
            {
                const float s       = fabsf(data[i]);

                if (s >= peak)
                {
                    nCounter        = nHold;
                    peak            = s;
                }
                else if (nCounter > 0)
                {
                    --nCounter;
                    // Keep current value
                }
                else
                    peak           *= fTau;

                dst[i]          = peak;
            }

            fPeak = peak;

            return peak;
        }

        float PeakMeter::process(const float *data, size_t count)
        {
            update_settings();

            float peak = fPeak;
            for (size_t i=0; i<count; ++i)
            {
                const float s       = fabsf(data[i]);

                if (s >= peak)
                {
                    nCounter        = nHold;
                    peak            = s;
                }
                else if (nCounter > 0)
                {
                    --nCounter;
                    // Keep current value
                }
                else
                    peak           *= fTau;
            }

            fPeak = peak;

            return peak;
        }

        float PeakMeter::process(float value, size_t count)
        {
            update_settings();

            value = fabsf(value);

            if (value >= fPeak)
            {
                nCounter        = nHold;
                fPeak           = value;
            }
            else if (nCounter >= count)
            {
                nCounter       -= count;
                // Keep current value
            }
            else
                fPeak           = lsp_max(fPeak * expf(logf(fTau) * float(count)), value);

            return fPeak;
        }

        void PeakMeter::dump(IStateDumper *v) const
        {
            v->write("fPeak", fPeak);
            v->write("fTau", fTau);
            v->write("fHold", fHold);
            v->write("fRelease", fRelease);
            v->write("nHold", nHold);
            v->write("nCounter", nCounter);
            v->write("nSampleRate", nSampleRate);
            v->write("bUpdate", bUpdate);
        }

    } /* namespace dspu */
} /* namespace lsp */


