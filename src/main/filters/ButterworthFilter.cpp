/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 28 Sept 2021
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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp-units/filters/ButterworthFilter.h>
#include <lsp-plug.in/stdlib/math.h>

#define MAX_ORDER       100u
#define BUF_LIM_SIZE    2048u
#define FREQUENCY_LIMIT 10.0f

namespace lsp
{
    namespace dspu
    {

        ButterworthFilter::ButterworthFilter()
        {
            construct();
        }

        ButterworthFilter::~ButterworthFilter()
        {
            destroy();
        }

        void ButterworthFilter::construct()
        {
            nOrder          = 2;

            fCutoffFreq     = 0.0f;

            nSampleRate     = -1;

            enFilterType    = FLT_TYPE_LOWPASS;

            pData           = NULL;
            vBuffer         = NULL;

            bBypass         = false;

            bSync           = true;

            // 1X buffer for processing.
            size_t samples = BUF_LIM_SIZE;

            float *ptr = alloc_aligned<float>(pData, samples);
            if (ptr == NULL)
                return;

            lsp_guard_assert(float *save = ptr);

            vBuffer = ptr;
            ptr += BUF_LIM_SIZE;

            lsp_assert(ptr <= &save[samples]);
        }

        void ButterworthFilter::destroy()
        {
            free_aligned(pData);
            pData = NULL;

            if (vBuffer != NULL)
            {
                delete [] vBuffer;
                vBuffer = NULL;
            }
        }

        void ButterworthFilter::update_settings()
        {
            if (!bSync)
                return;

            if (enFilterType == FLT_TYPE_NONE)
            {
                bBypass = true;
                bSync = true;
                return;
            }
            else
            {
                bBypass = false;
            }

            nOrder = lsp_min(nOrder, MAX_ORDER);
            // We force even order (so all biquads have all coefficients, maximal efficiency).
            nOrder = (nOrder % 2 == 0) ? nOrder : nOrder + 1;

            fCutoffFreq = lsp_min(fCutoffFreq, 0.5f * nSampleRate - FREQUENCY_LIMIT);
            fCutoffFreq = lsp_max(fCutoffFreq, FREQUENCY_LIMIT);

            float ang_f_c       = 2.0f * M_PI * fCutoffFreq;
            float bin_c         = ang_f_c / tanf(0.5f * ang_f_c / nSampleRate);
            float bin_c_sq      = bin_c * bin_c;
            size_t n_biquads    = 1 + (nOrder - 1) / 2;

            sFilter.begin();
            for (size_t k = 0; k < n_biquads; ++k)
            {
                float analog_pole_ang = 0.5f * M_PI * (2.0f * k + nOrder + 1.0f) / nOrder;

                float analog_pole_re = cosf(analog_pole_ang);
                float analog_pole_im = sinf(analog_pole_ang);

                float analog_pole_re_sq = analog_pole_re * analog_pole_re;
                float analog_pole_im_sq = analog_pole_im * analog_pole_im;

                float scale = 1.0f / (
                        bin_c_sq
                        - 2.0f * bin_c * analog_pole_re
                        + analog_pole_re_sq
                        + analog_pole_im_sq
                        );

                float digital_pole_re = scale * (
                        bin_c_sq
                        - analog_pole_re_sq
                        - analog_pole_im_sq
                        );

                float digital_pole_im = 2.0f * scale * bin_c + analog_pole_im;

                float digital_pole_sqabs = digital_pole_re * digital_pole_re + digital_pole_im * digital_pole_im;

                dsp::biquad_x1_t *f = sFilter.add_chain();
                if (f == NULL)
                    return;

                float gain;

                switch (enFilterType)
                {
                    // Sign for a[n] (denominator) coefficients needs to be inverted for assignment.
                    // In other words, with respect the maths, f->a1 and f->a2 below have inverted sign.

                    case FLT_TYPE_HIGHPASS:
                    {
                        f->b0 = 1.0f;
                        f->b1 = -2.0f;
                        f->b2 = 1.0f;
                        f->a1 = 2.0f * digital_pole_re;
                        f->a2 = -digital_pole_sqabs;
                        f->p0 = 0.0f;
                        f->p1 = 0.0f;
                        f->p2 = 0.0f;

                        gain = (1.0f + f->a1 - f->a2) / (1.0f - f->b1 + f->b2);
                    }
                    break;

                    case FLT_TYPE_LOWPASS:
                    default:
                    {
                        f->b0 = 1.0f;
                        f->b1 = 2.0f;
                        f->b2 = 1.0f;
                        f->a1 = 2.0f * digital_pole_re;
                        f->a2 = -digital_pole_sqabs;
                        f->p0 = 0.0f;
                        f->p1 = 0.0f;
                        f->p2 = 0.0f;

                        gain = (1.0f - f->a1 - f->a2) / (1.0f + f->b1 + f->b2);
                    }
                    break;
                }

                f->b0 = f->b0 * gain;
                f->b1 = f->b1 * gain;
                f->b2 = f->b2 * gain;
            }
            sFilter.end(true);

            bSync = true;
        }

        void ButterworthFilter::process_add(float *dst, const float *src, size_t count)
        {
            if (src != NULL)
                dsp::copy(dst, src, count);
            else
                dsp::fill_zero(dst, count);

            if (bBypass)
            {
                dsp::mul_k2(dst, 2.0f, count);
                return;
            }

            while (count > 0)
            {
                size_t to_do = lsp_min(count, BUF_LIM_SIZE);

                sFilter.process(vBuffer, dst, to_do);
                dsp::add2(dst, vBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }

        }

        void ButterworthFilter::process_mul(float *dst, const float *src, size_t count)
        {
            if (src != NULL)
                dsp::copy(dst, src, count);
            else
                dsp::fill_zero(dst, count);

            if (bBypass)
            {
                dsp::mul2(dst, dst, count);
                return;
            }

            while (count > 0)
            {
                size_t to_do = lsp_min(count, BUF_LIM_SIZE);

                sFilter.process(vBuffer, dst, to_do);
                dsp::mul2(dst, vBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }

        }

        void ButterworthFilter::process_overwrite(float *dst, const float *src, size_t count)
        {
            if (src != NULL)
                dsp::copy(dst, src, count);
            else
                dsp::fill_zero(dst, count);

            if (bBypass)
            {
                // Nothing to do.
                return;
            }

            while (count > 0)
            {
                size_t to_do = lsp_min(BUF_LIM_SIZE, count);

                sFilter.process(vBuffer, dst, to_do);
                dsp::copy(dst, vBuffer, to_do);

                dst     += to_do;
                count   -= to_do;
            }
        }

        void ButterworthFilter::dump(IStateDumper *v) const
        {
            v->write("nOrder", nOrder);
            v->write("fCutoffFreq", fCutoffFreq);
            v->write("nSampleRate", nSampleRate);
            v->write("enFilterType", enFilterType);
            v->write_object("sFilter", &sFilter);
            v->write("pData", pData);
            v->write("vBuffer", vBuffer);
            v->write("bBypass", bBypass);
            v->write("bSync", bSync);
        }

    }
}
