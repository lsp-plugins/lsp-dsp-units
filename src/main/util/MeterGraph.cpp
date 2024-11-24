/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 20 мая 2016 г.
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

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/util/MeterGraph.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        MeterGraph::MeterGraph()
        {
            construct();
        }

        MeterGraph::~MeterGraph()
        {
            destroy();
        }

        void MeterGraph::construct()
        {
            sBuffer.construct();

            fCurrent    = 0.0f;
            nCount      = 0;
            nPeriod     = 1;
            enMethod    = MM_ABS_MAXIMUM;
        }

        void MeterGraph::destroy()
        {
            sBuffer.destroy();
        }

        bool MeterGraph::init(size_t frames, size_t period)
        {
            if (period <= 0)
                return false;

            if (!sBuffer.init(frames * 4, frames))
                return false;

            fCurrent    = 0.0f;
            nCount      = 0;
            nPeriod     = period;
            return true;
        }

        void MeterGraph::process(float sample)
        {
            // Update current sample
            switch (enMethod)
            {
                case MM_SIGN_MINIMUM:
                    if ((nCount == 0) || (fabsf(fCurrent) > fabsf(sample)))
                        fCurrent    = sample;
                    break;

                case MM_SIGN_MAXIMUM:
                    if ((nCount == 0) || (fabsf(fCurrent) < fabsf(sample)))
                        fCurrent    = sample;
                    break;

                case MM_ABS_MINIMUM:
                    sample      = fabsf(sample);
                    if ((nCount == 0) || (fCurrent > sample))
                        fCurrent    = sample;
                    break;

                default:
                case MM_ABS_MAXIMUM:
                    sample      = fabsf(sample);
                    if ((nCount == 0) || (fCurrent < sample))
                        fCurrent    = sample;
                    break;
            }

            // Increment number of samples processed
            if ((++nCount) >= nPeriod)
            {
                // Append current sample to buffer
                sBuffer.process(fCurrent);
                nCount      = 0;
            }
        }

        void MeterGraph::process(const float *s, size_t n)
        {
            while (n > 0)
            {
                // Determine amount of samples to process
                ssize_t can_do      = lsp_min(ssize_t(n), ssize_t(nPeriod - nCount));

                // Process the samples
                if (can_do > 0)
                {
                    // Process samples
                    switch (enMethod)
                    {
                        case MM_SIGN_MINIMUM:
                        {
                            const float sample  = dsp::sign_min(s, can_do);
                            if ((nCount == 0) || (fabsf(fCurrent) > fabsf(sample)))
                                fCurrent        = sample;
                            break;
                        }
                        case MM_SIGN_MAXIMUM:
                        {
                            const float sample  = dsp::sign_max(s, can_do);
                            if ((nCount == 0) || (fabsf(fCurrent) < fabsf(sample)))
                                fCurrent        = sample;
                            break;
                        }
                        case MM_ABS_MINIMUM:
                        {
                            const float sample  = dsp::abs_min(s, can_do);
                            if ((nCount == 0) || (fCurrent > sample))
                                fCurrent        = sample;
                            break;
                        }
                        default:
                        case MM_ABS_MAXIMUM:
                        {
                            const float sample  = dsp::abs_max(s, can_do);
                            if ((nCount == 0) || (fCurrent > sample))
                                fCurrent        = sample;
                            break;
                        }
                    }

                    // Update counters and pointers
                    nCount             += can_do;
                    n                  -= can_do;
                    s                  += can_do;
                }

                // Check that need to switch to next sample
                if (nCount >= nPeriod)
                {
                    // Append current sample to buffer
                    sBuffer.process(fCurrent);
                    nCount      = 0;
                }
            }
        }

        void MeterGraph::process(const float *s, float gain, size_t n)
        {
            while (n > 0)
            {
                // Determine amount of samples to process
                ssize_t can_do      = lsp_min(ssize_t(n), ssize_t(nPeriod - nCount));

                // Process the samples
                if (can_do > 0)
                {
                    // Process samples
                    switch (enMethod)
                    {
                        case MM_SIGN_MINIMUM:
                        {
                            const float sample  = dsp::sign_min(s, can_do) * gain;
                            if ((nCount == 0) || (fabsf(fCurrent) > fabsf(sample)))
                                fCurrent        = sample;
                            break;
                        }
                        case MM_SIGN_MAXIMUM:
                        {
                            const float sample  = dsp::sign_max(s, can_do) * gain;
                            if ((nCount == 0) || (fabsf(fCurrent) < fabsf(sample)))
                                fCurrent        = sample;
                            break;
                        }
                        case MM_ABS_MINIMUM:
                        {
                            const float sample  = dsp::abs_min(s, can_do) * gain;
                            if ((nCount == 0) || (fCurrent > sample))
                                fCurrent        = sample;
                            break;
                        }
                        default:
                        case MM_ABS_MAXIMUM:
                        {
                            const float sample  = dsp::abs_max(s, can_do) * gain;
                            if ((nCount == 0) || (fCurrent > sample))
                                fCurrent        = sample;
                            break;
                        }
                    }

                    // Update counters and pointers
                    nCount             += can_do;
                    n                  -= can_do;
                    s                  += can_do;
                }

                // Check that need to switch to next sample
                if (nCount >= nPeriod)
                {
                    // Append current sample to buffer
                    sBuffer.process(fCurrent);
                    nCount      = 0;
                }
            }
        }

        void MeterGraph::dump(IStateDumper *v) const
        {
            v->write_object("sBuffer", &sBuffer);
            v->write("fCurrent", fCurrent);
            v->write("nCount", nCount);
            v->write("nPeriod", nPeriod);
            v->write("enMethod", enMethod);
        }
    } /* namespace dspu */
} /* namespace lsp */



