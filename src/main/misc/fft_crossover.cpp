/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 8 авг. 2023 г.
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

#include <lsp-plug.in/dsp-units/misc/fft_crossover.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        namespace crossover
        {
            constexpr float filter_xover_level  = 0.5f;                         // ~ -6 dB
            constexpr float slope_scale         = (0.05f * M_LN10) / M_LN2;     // Slope scaling factor
            constexpr float slope_scale_m6dbo   = (-0.3f * M_LN10) / M_LN2;     // Slope scaling factor for -6 dB/oct

            LSP_DSP_UNITS_PUBLIC
            float hipass(float f, float f0, float slope)
            {
                // Special case?
                if (slope > -3.0f)
                {
                    if (f <= f0)
                        return filter_xover_level;
                    if (f >= f0 * 2.0f)
                        return 1.0f;

                    return expf(slope_scale_m6dbo * logf(f0 / f)) * filter_xover_level;
                }

                // Usual case
                const float k           = slope * slope_scale;
                return (f >= f0) ?
                    1.0f - expf(k * logf(f / f0)) * filter_xover_level :
                    expf(k * logf(f0 / f)) * filter_xover_level;
            }

            LSP_DSP_UNITS_PUBLIC
            float lopass(float f, float f0, float slope)
            {
                // Special case?
                if (slope > -3.0f)
                {
                    if (f >= f0)
                        return filter_xover_level;
                    if (f <= f0 * 0.5f)
                        return 1.0f;

                    return expf(slope_scale_m6dbo * logf(f / f0)) * filter_xover_level;
                }

                // Usual case
                const float k           = slope * slope_scale;   // -slope dB/oct
                return (f >= f0) ?
                    expf(k * logf(f / f0)) * filter_xover_level :
                    1.0f - expf(k * logf(f0 / f)) * filter_xover_level;
            }

            LSP_DSP_UNITS_PUBLIC
            void hipass_set(float *gain, const float *vf, float f0, float slope, size_t count)
            {
                // Special case?
                if (slope > -3.0f)
                {
                    for (size_t i=0; i<count; ++i)
                    {
                        float f = vf[i];
                        if (f <= f0)
                            gain[i] = filter_xover_level;
                        else if (f >= f0 * 2.0f)
                            gain[i] = 1.0f;
                        else
                            gain[i] = expf(slope_scale_m6dbo * logf(f0 / f)) * filter_xover_level;
                    }
                    return;
                }

                // Usual case
                const float k           = slope * slope_scale;   // -slope dB/oct
                for (size_t i=0; i<count; ++i)
                {
                    float f = vf[i];
                    gain[i] = (f >= f0) ?
                        1.0f - expf(k * logf(f / f0)) * filter_xover_level :
                        expf(k * logf(f0 / f)) * filter_xover_level;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void hipass_apply(float *gain, const float *vf, float f0, float slope, size_t count)
            {
                // Special case?
                if (slope > -3.0f)
                {
                    for (size_t i=0; i<count; ++i)
                    {
                        float f = vf[i];
                        if (f <= f0)
                            gain[i] *= filter_xover_level;
                        else if (f < f0 * 2.0f)
                            gain[i] *= expf(slope_scale_m6dbo * logf(f0 / f)) * filter_xover_level;
                    }
                    return;
                }

                // Usual case
                const float k           = slope * slope_scale;   // -slope dB/oct
                for (size_t i=0; i<count; ++i)
                {
                    float f = vf[i];
                    gain[i] *= (f >= f0) ?
                        1.0f - expf(k * logf(f / f0)) * filter_xover_level :
                        expf(k * logf(f0 / f)) * filter_xover_level;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void lopass_set(float *gain, const float *vf, float f0, float slope, size_t count)
            {
                // Special case?
                if (slope > -3.0f)
                {
                    for (size_t i=0; i<count; ++i)
                    {
                        float f = vf[i];
                        if (f >= f0)
                            gain[i] = filter_xover_level;
                        else if (f <= f0 * 0.5f)
                            gain[i] = 1.0f;
                        else
                            gain[i] = expf(slope_scale_m6dbo * logf(f / f0)) * filter_xover_level;
                    }
                    return;
                }

                // Usual case
                const float k           = slope * slope_scale;   // -slope dB/oct
                for (size_t i=0; i<count; ++i)
                {
                    float f = vf[i];
                    gain[i] = (f >= f0) ?
                        expf(k * logf(f / f0)) * filter_xover_level :
                        1.0f - expf(k * logf(f0 / f)) * filter_xover_level;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void lopass_apply(float *gain, const float *vf, float f0, float slope, size_t count)
            {
                // Special case?
                if (slope > -3.0f)
                {
                    for (size_t i=0; i<count; ++i)
                    {
                        float f = vf[i];
                        if (f >= f0)
                            gain[i] *= filter_xover_level;
                        else if (f > f0 * 0.5f)
                            gain[i] *= expf(slope_scale_m6dbo * logf(f / f0)) * filter_xover_level;
                    }
                    return;
                }

                // Usual case
                const float k           = slope * slope_scale;   // -slope dB/oct
                for (size_t i=0; i<count; ++i)
                {
                    float f = vf[i];
                    gain[i] *= (f >= f0) ?
                        expf(k * logf(f / f0)) * filter_xover_level :
                        1.0f - expf(k * logf(f0 / f)) * filter_xover_level;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void hipass_fft_set(float *gain, float f0, float slope, float sample_rate, size_t rank)
            {
                const size_t fft_size   = 1 << rank;
                const size_t fft_half   = fft_size >> 1;
                const float kf          = sample_rate / fft_size;
                size_t  i               = 1;
                gain[0]                 = 0.0f;

                // Special case?
                if (slope > -3.0f)
                {
                    for (; i <= fft_half; ++i)
                    {
                        float f = i * kf;
                        if (f <= f0)
                            gain[i] = filter_xover_level;
                        else if (f >= f0 * 2.0f)
                            gain[i] = 1.0f;
                        else
                            gain[i] = expf(slope_scale_m6dbo * logf(f0 / f)) * filter_xover_level;
                    }
                    for (; i < fft_size; ++i)
                    {
                        float f = (fft_size - i) * kf;
                        if (f <= f0)
                            gain[i] = filter_xover_level;
                        else if (f >= f0 * 2.0f)
                            gain[i] = 1.0f;
                        else
                            gain[i] = expf(slope_scale_m6dbo * logf(f0 / f)) * filter_xover_level;
                    }
                    return;
                }

                // Usual case
                const float k           = slope * slope_scale;   // -slope dB/oct
                for (; i <= fft_half; ++i)
                {
                    float f = i * kf;
                    gain[i] = (f >= f0) ?
                        1.0f - expf(k * logf(f / f0)) * filter_xover_level :
                        expf(k * logf(f0 / f)) * filter_xover_level;
                }
                for (; i < fft_size; ++i)
                {
                    float f = (fft_size - i) * kf;
                    gain[i] = (f >= f0) ?
                        1.0f - expf(k * logf(f / f0)) * filter_xover_level :
                        expf(k * logf(f0 / f)) * filter_xover_level;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void hipass_fft_apply(float *gain, float f0, float slope, float sample_rate, size_t rank)
            {
                const size_t fft_size   = 1 << rank;
                const size_t fft_half   = fft_size >> 1;
                const float kf          = sample_rate / fft_size;
                size_t  i               = 1;
                gain[0]                 = 0.0f;

                // Special case?
                if (slope > -3.0f)
                {
                    for (; i <= fft_half; ++i)
                    {
                        float f = i * kf;
                        if (f <= f0)
                            gain[i] *= filter_xover_level;
                        else if (f < f0 * 2.0f)
                            gain[i] *= expf(slope_scale_m6dbo * logf(f0 / f)) * filter_xover_level;
                    }
                    for (; i < fft_size; ++i)
                    {
                        float f = (fft_size - i) * kf;
                        if (f <= f0)
                            gain[i] *= filter_xover_level;
                        else if (f < f0 * 2.0f)
                            gain[i] *= expf(slope_scale_m6dbo * logf(f0 / f)) * filter_xover_level;
                    }
                    return;
                }

                // Usual case
                const float k           = slope * slope_scale;   // -slope dB/oct
                for (; i <= fft_half; ++i)
                {
                    float f = i * kf;
                    gain[i] *= (f >= f0) ?
                        1.0f - expf(k * logf(f / f0)) * filter_xover_level :
                        expf(k * logf(f0 / f)) * filter_xover_level;
                }
                for (; i < fft_size; ++i)
                {
                    float f = (fft_size - i) * kf;
                    gain[i] *= (f >= f0) ?
                        1.0f - expf(k * logf(f / f0)) * filter_xover_level :
                        expf(k * logf(f0 / f)) * filter_xover_level;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void lopass_fft_set(float *gain, float f0, float slope, float sample_rate, size_t rank)
            {
                const size_t fft_size   = 1 << rank;
                const size_t fft_half   = fft_size >> 1;
                const float kf          = sample_rate / fft_size;
                size_t  i               = 1;
                gain[0]                 = 1.0f;

                // Special case?
                if (slope > -3.0f)
                {
                    for (; i <= fft_half; ++i)
                    {
                        float f = i * kf;
                        if (f >= f0)
                            gain[i] = filter_xover_level;
                        else if (f <= f0 * 0.5f)
                            gain[i] = 1.0f;
                        else
                            gain[i] = expf(slope_scale_m6dbo * logf(f / f0)) * filter_xover_level;
                    }
                    for (; i < fft_size; ++i)
                    {
                        float f = (fft_size - i) * kf;
                        if (f >= f0)
                            gain[i] = filter_xover_level;
                        else if (f <= f0 * 0.5f)
                            gain[i] = 1.0f;
                        else
                            gain[i] = expf(slope_scale_m6dbo * logf(f / f0)) * filter_xover_level;
                    }
                    return;
                }

                // Usual case
                const float k           = slope * slope_scale;   // -slope dB/oct
                for (; i <= fft_half; ++i)
                {
                    float f = i * kf;
                    gain[i] = (f >= f0) ?
                        expf(k * logf(f / f0)) * filter_xover_level :
                        1.0f - expf(k * logf(f0 / f)) * filter_xover_level;
                }
                for (; i < fft_size; ++i)
                {
                    float f = (fft_size - i) * kf;
                    gain[i] = (f >= f0) ?
                        expf(k * logf(f / f0)) * filter_xover_level :
                        1.0f - expf(k * logf(f0 / f)) * filter_xover_level;
                }
            }

            LSP_DSP_UNITS_PUBLIC
            void lopass_fft_apply(float *gain, float f0, float slope, float sample_rate, size_t rank)
            {
                const size_t fft_size   = 1 << rank;
                const size_t fft_half   = fft_size >> 1;
                const float kf          = sample_rate / fft_size;
                size_t  i               = 1;

                // Special case?
                if (slope > -3.0f)
                {
                    for (; i <= fft_half; ++i)
                    {
                        float f = i * kf;
                        if (f >= f0)
                            gain[i] *= filter_xover_level;
                        else if (f > f0 * 0.5f)
                            gain[i] *= expf(slope_scale_m6dbo * logf(f / f0)) * filter_xover_level;
                    }
                    for (; i < fft_size; ++i)
                    {
                        float f = (fft_size - i) * kf;
                        if (f >= f0)
                            gain[i] *= filter_xover_level;
                        else if (f > f0 * 0.5f)
                            gain[i] *= expf(slope_scale_m6dbo * logf(f / f0)) * filter_xover_level;
                    }
                    return;
                }

                // Usual case
                const float k           = slope * slope_scale;   // -slope dB/oct
                for (; i <= fft_half; ++i)
                {
                    float f = i * kf;
                    gain[i] *= (f >= f0) ?
                        expf(k * logf(f / f0)) * filter_xover_level :
                        1.0f - expf(k * logf(f0 / f)) * filter_xover_level;
                }
                for (; i < fft_size; ++i)
                {
                    float f = (fft_size - i) * kf;
                    gain[i] *= (f >= f0) ?
                        expf(k * logf(f / f0)) * filter_xover_level :
                        1.0f - expf(k * logf(f0 / f)) * filter_xover_level;
                }
            }

        } /* namespace crossover */
    } /* namespace dspu */
} /* namespace lsp */


