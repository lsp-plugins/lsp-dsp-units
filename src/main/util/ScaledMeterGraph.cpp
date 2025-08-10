/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 05 окт 2024 г.
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
#include <lsp-plug.in/dsp-units/util/ScaledMeterGraph.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr size_t FRAMES_GAP     = 0x10;

        ScaledMeterGraph::ScaledMeterGraph()
        {
            construct();
        }

        ScaledMeterGraph::~ScaledMeterGraph()
        {
            destroy();
        }

        void ScaledMeterGraph::construct()
        {
            sHistory.sBuffer.construct();
            sHistory.fCurrent       = 0.0f;
            sHistory.nCount         = 0;
            sHistory.nPeriod        = 0;
            sHistory.nFrames        = 0;

            sFrames.sBuffer.construct();
            sFrames.fCurrent        = 0.0f;
            sFrames.nCount          = 0;
            sFrames.nPeriod         = 0;
            sFrames.nFrames         = 0;

            nPeriod                 = 0;
            nMaxPeriod              = 0;
            enMethod                = MM_ABS_MAXIMUM;
        }

        void ScaledMeterGraph::destroy()
        {
            sHistory.sBuffer.destroy();
            sFrames.sBuffer.destroy();
        }

        bool ScaledMeterGraph::init(size_t frames, size_t subsampling, size_t max_period)
        {
            const size_t samples    = frames * max_period;
            const size_t subframes  = (samples + subsampling - 1) / subsampling;

            if (!sHistory.sBuffer.init(subframes + FRAMES_GAP))
                return false;
            if (!sFrames.sBuffer.init(frames + FRAMES_GAP))
                return false;

            sHistory.nFrames        = uint32_t(subframes);
            sHistory.nPeriod        = uint32_t(subsampling);
            sHistory.fCurrent       = 0;
            sHistory.nCount         = 0;

            sFrames.nFrames         = uint32_t(frames);
            sFrames.nPeriod         = 0;
            sFrames.fCurrent        = 0;
            sFrames.nCount          = 0;

            nPeriod                 = 0;
            nMaxPeriod              = uint32_t(max_period);

            return true;
        }

        void ScaledMeterGraph::set_period(size_t period)
        {
            nPeriod                 = lsp_limit(uint32_t(period), sHistory.nPeriod, nMaxPeriod);
        }

        void ScaledMeterGraph::set_method(meter_method_t m)
        {
            enMethod                = m;
        }

        void ScaledMeterGraph::process_sampler(sampler_t *sampler, float sample)
        {
            // Update current sample
            switch (enMethod)
            {
                case MM_SIGN_MINIMUM:
                    if ((sampler->nCount == 0) || (fabsf(sampler->fCurrent) > fabsf(sample)))
                        sampler->fCurrent   = sample;
                    break;

                case MM_SIGN_MAXIMUM:
                    if ((sampler->nCount == 0) || (fabsf(sampler->fCurrent) < fabsf(sample)))
                        sampler->fCurrent   = sample;
                    break;

                case MM_ABS_MINIMUM:
                    sample = fabsf(sample);
                    if ((sampler->nCount == 0) || (sampler->fCurrent > sample))
                        sampler->fCurrent   = sample;
                    break;

                case MM_PEAK:
                    if (sampler->nCount == 0)
                        sampler->fCurrent   = sample;
                    break;

                default:
                case MM_ABS_MAXIMUM:
                    sample = fabsf(sample);
                    if ((sampler->nCount == 0) || (sampler->fCurrent < sample))
                        sampler->fCurrent   = sample;
                    break;
            }

            ++sampler->nCount;

            // Append current sample to buffer if we processed enough samples
            if (sampler->nCount >= sampler->nPeriod)
            {
                sampler->sBuffer.push(sample);
                sampler->nCount     = 0;
            }
        }

        void ScaledMeterGraph::process_sampler(sampler_t *sampler, const float *src, size_t count)
        {
            // Sink full buffer to the sampler
            for (size_t offset = 0; offset < count; )
            {
                size_t to_process   = lsp_min(count - offset, sampler->nPeriod - sampler->nCount);

                if (to_process > 0)
                {
                    switch (enMethod)
                    {
                        case MM_SIGN_MINIMUM:
                        {
                            const float sample  = dsp::sign_min(&src[offset], to_process);
                            if ((sampler->nCount == 0) || (fabsf(sampler->fCurrent) > fabsf(sample)))
                                sampler->fCurrent   = sample;
                            break;
                        }
                        case MM_SIGN_MAXIMUM:
                        {
                            const float sample  = dsp::sign_max(&src[offset], to_process);
                            if ((sampler->nCount == 0) || (fabsf(sampler->fCurrent) < fabsf(sample)))
                                sampler->fCurrent   = sample;
                            break;
                        }
                        case MM_ABS_MINIMUM:
                        {
                            const float sample  = dsp::abs_min(&src[offset], to_process);
                            if ((sampler->nCount == 0) || (sampler->fCurrent > sample))
                                sampler->fCurrent   = sample;
                            break;
                        }
                        case MM_PEAK:
                        {
                            if (sampler->nCount == 0)
                                sampler->fCurrent   = src[offset];
                            break;
                        }
                        default:
                        case MM_ABS_MAXIMUM:
                        {
                            const float sample  = dsp::abs_max(&src[offset], to_process);
                            if ((sampler->nCount == 0) || (sampler->fCurrent < sample))
                                sampler->fCurrent   = sample;
                            break;
                        }
                    }

                    sampler->nCount    += to_process;
                    offset             += to_process;
                }

                // Append current sample to buffer if we processed enough samples
                if (sampler->nCount >= sampler->nPeriod)
                {
                    sampler->sBuffer.push(sampler->fCurrent);
                    sampler->nCount     = 0;
                }
            }
        }

        void ScaledMeterGraph::process_sampler(sampler_t *sampler, const float *src, float gain, size_t count)
        {
            // Sink full buffer to the sampler
            for (size_t offset = 0; offset < count; )
            {
                // Process samples if we can
                size_t to_process   = lsp_min(count - offset, sampler->nPeriod - sampler->nCount);
                if (to_process > 0)
                {
                    switch (enMethod)
                    {
                        case MM_SIGN_MINIMUM:
                        {
                            const float sample  = dsp::sign_min(&src[offset], to_process) * gain;
                            if ((sampler->nCount == 0) || (fabsf(sampler->fCurrent) > fabsf(sample)))
                                sampler->fCurrent   = sample;
                            break;
                        }
                        case MM_SIGN_MAXIMUM:
                        {
                            const float sample  = dsp::sign_max(&src[offset], to_process) * gain;
                            if ((sampler->nCount == 0) || (fabsf(sampler->fCurrent) < fabsf(sample)))
                                sampler->fCurrent   = sample;
                            break;
                        }
                        case MM_ABS_MINIMUM:
                        {
                            const float sample  = dsp::abs_min(&src[offset], to_process) * gain;
                            if ((sampler->nCount == 0) || (sampler->fCurrent > sample))
                                sampler->fCurrent   = sample;
                            break;
                        }
                        case MM_PEAK:
                        {
                            if (sampler->nCount == 0)
                                sampler->fCurrent   = src[offset] * gain;
                            break;
                        }
                        default:
                        case MM_ABS_MAXIMUM:
                        {
                            const float sample  = dsp::abs_max(&src[offset], to_process) * gain;
                            if ((sampler->nCount == 0) || (sampler->fCurrent < sample))
                                sampler->fCurrent   = sample;
                            break;
                        }
                    }

                    sampler->nCount    += to_process;
                    offset             += to_process;
                }

                // Append current sample to buffer if we processed enough samples
                if (sampler->nCount >= sampler->nPeriod)
                {
                    sampler->sBuffer.push(sampler->fCurrent);
                    sampler->nCount     = 0;
                }
            }
        }

        bool ScaledMeterGraph::update_period()
        {
            if (nPeriod == sFrames.nPeriod)
                return false;

            // Submit last measured samples to buffer if they are
            if (sHistory.nCount > 0)
            {
                sHistory.sBuffer.push(sHistory.fCurrent);
                sHistory.nCount     = 0;
            }
            else if (sFrames.nCount > 0)
                sHistory.sBuffer.push(sFrames.fCurrent);

            sFrames.nCount      = 0;
            sFrames.nPeriod     = nPeriod;
            sFrames.fCurrent    = -1.0f;

            // Perform the decimation
            const size_t samples    = sFrames.nPeriod * sFrames.nFrames;
            const size_t subsamples = (samples + sHistory.nPeriod - 1) / sHistory.nPeriod;

            sFrames.sBuffer.reset();

            // Sink full buffer to the sampler
            for (size_t i = 0; i < subsamples; ++i)
            {
                // Process sub-sample
                const float subsample = sHistory.sBuffer.read(subsamples - i);
                switch (enMethod)
                {
                    case MM_SIGN_MINIMUM:
                    {
                        if ((sFrames.fCurrent < 0.0f) || (fabsf(sFrames.fCurrent) > fabsf(subsample)))
                            sFrames.fCurrent    = subsample;
                        break;
                    }
                    case MM_SIGN_MAXIMUM:
                    {
                        if ((sFrames.fCurrent < 0.0f) || (fabsf(sFrames.fCurrent) < fabsf(subsample)))
                            sFrames.fCurrent    = subsample;
                        break;
                    }
                    case MM_ABS_MINIMUM:
                    {
                        const float sample  = fabsf(subsample);
                        if ((sFrames.fCurrent < 0.0f) || (sFrames.fCurrent > sample))
                            sFrames.fCurrent    = sample;
                        break;
                    }

                    default:
                    case MM_ABS_MAXIMUM:
                    {
                        const float sample  = fabsf(subsample);
                        if ((sFrames.fCurrent < 0.0f) || (sFrames.fCurrent < sample))
                            sFrames.fCurrent    = sample;
                        break;
                    }
                }

                sFrames.nCount     += sHistory.nPeriod;

                // Append current sample to buffer if we processed enough samples
                if (sFrames.nCount >= sFrames.nPeriod)
                {
                    sFrames.sBuffer.push(sFrames.fCurrent);
                    sFrames.nCount     -= sFrames.nPeriod;
                    sFrames.fCurrent    = -1.0f;
                }
            }

            return true;
        }

        void ScaledMeterGraph::process(float sample)
        {
            process_sampler(&sHistory, sample);
            if (!update_period())
                process_sampler(&sFrames, sample);
        }

        void ScaledMeterGraph::process(const float *s, size_t count)
        {
            process_sampler(&sHistory, s, count);
            if (!update_period())
                process_sampler(&sFrames, s, count);
        }

        void ScaledMeterGraph::process(const float *s, float gain, size_t count)
        {
            process_sampler(&sHistory, s, gain, count);
            if (!update_period())
                process_sampler(&sFrames, s, gain, count);
        }

        void ScaledMeterGraph::read(float *dst, size_t count)
        {
            count       = lsp_min(count, sFrames.nFrames);
            sFrames.sBuffer.read(dst, count, count);
        }

        float ScaledMeterGraph::level() const
        {
            return sFrames.sBuffer.read(1);
        }

        void ScaledMeterGraph::fill(float level)
        {
            sFrames.sBuffer.fill(level);
            sFrames.nCount      = 0;

            sHistory.sBuffer.fill(level);
            sHistory.nCount     = 0;
        }

        void ScaledMeterGraph::dump_sampler(IStateDumper *v, const char *name, const sampler_t *s)
        {
            v->begin_object(name, s, sizeof(sampler_t));
            {
                v->write_object("sBuffer", &s->sBuffer);
                v->write("fCurrent", s->fCurrent);
                v->write("nCount", s->nCount);
                v->write("nPeriod", s->nPeriod);
                v->write("nFrames", s->nFrames);
            }
            v->end_object();
        }

        void ScaledMeterGraph::dump(IStateDumper *v) const
        {
            dump_sampler(v, "sHistory", &sHistory);
            dump_sampler(v, "sFrames", &sFrames);

            v->write("nPeriod", nPeriod);
            v->write("nMaxPeriod", nMaxPeriod);
            v->write("enMethod", enMethod);
        }
    } /* namespace dspu */
} /* namespace lsp */



