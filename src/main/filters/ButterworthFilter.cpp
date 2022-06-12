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

#define MAX_ORDER           128u
#define BUF_LIM_SIZE        256u
#define FREQUENCY_LIMIT     10.0f

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
            enFilterType    = BW_FLT_TYPE_LOWPASS;
            bBypass         = false;
            bSync           = true;

            sFilter.construct();
            sFilter.init(MAX_ORDER);
        }

        void ButterworthFilter::destroy()
        {
            sFilter.destroy();
        }

        void ButterworthFilter::set_order(size_t order)
        {
            if (order == nOrder)
                return;

            nOrder  = order;
            bSync   = true;
        }

        void ButterworthFilter::set_cutoff_frequency(float frequency)
        {
            if (frequency == fCutoffFreq)
                return;

            fCutoffFreq = frequency;
            bSync       = true;
        }

        void ButterworthFilter::set_filter_type(bw_filt_type_t type)
        {
            if ((type < BW_FLT_TYPE_LOWPASS) && (type >= BW_FLT_TYPE_MAX))
                return;

            enFilterType    = type;
            bSync           = true;
        }

        void ButterworthFilter::set_sample_rate(size_t sr)
        {
            if (nSampleRate == sr)
                return;

            nSampleRate = sr;
            bSync       = true;
        }

        void ButterworthFilter::update_settings()
        {
            if (!bSync)
                return;

            if (enFilterType == BW_FLT_TYPE_NONE)
            {
                bBypass = true;
                bSync = true;
                return;
            }
            else
                bBypass = false;

            nOrder = lsp_min(nOrder, MAX_ORDER);
            // We force even order (so all biquads have all coefficients, maximal efficiency).
            nOrder = (nOrder % 2 == 0) ? nOrder : nOrder + 1;

            fCutoffFreq = lsp_limit(fCutoffFreq, FREQUENCY_LIMIT, 0.5f * nSampleRate - FREQUENCY_LIMIT);

            // Accuracy of the filter can maybe improved using double precision.

            float ang_f_c       = 2.0f * M_PI * fCutoffFreq;
            float bin_c         = ang_f_c / tanf(0.5f * ang_f_c / nSampleRate);
            float bin_c_sq      = bin_c * bin_c;
            size_t n_biquads    = 1 + (nOrder - 1) / 2;

            sFilter.begin();
            for (size_t k = 0; k < n_biquads; ++k)
            {
                float analog_pole_ang = 0.5f * M_PI * (2.0f * k + nOrder + 1.0f) / nOrder;

                float analog_pole_re = ang_f_c * cosf(analog_pole_ang);
                float analog_pole_im = ang_f_c * sinf(analog_pole_ang);

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

                float digital_pole_im = 2.0f * scale * bin_c * analog_pole_im;

                float digital_pole_sqabs = digital_pole_re * digital_pole_re + digital_pole_im * digital_pole_im;

                dsp::biquad_x1_t *f = sFilter.add_chain();
                if (f == NULL)
                    return;

                float gain;

                switch (enFilterType)
                {
                    // Sign for a[n] (denominator) coefficients needs to be inverted for assignment.
                    // In other words, with respect the maths, f->a1 and f->a2 below have inverted sign.

                    case BW_FLT_TYPE_HIGHPASS:
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

                    case BW_FLT_TYPE_LOWPASS:
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
            update_settings();

            if (src == NULL)
            {
                // No inputs, interpret `src` as zeros: dst[i] = dst[i] + 0 = dst[i]
                // => Nothing to do
                return;
            }
            else if (bBypass)
            {
                // Bypass is set: dst[i] = dst[i] + src[i]
                dsp::add2(dst, src, count);
                return;
            }

            // 1X buffer for temporary processing.
            float vTemp[BUF_LIM_SIZE];

            for (size_t offset=0; offset < count; )
            {
                size_t to_do = lsp_min(count - offset, BUF_LIM_SIZE);

                // dst[i] = dst[i] + filter(src[i])
                sFilter.process(vTemp, &src[offset], to_do);
                dsp::add2(&dst[offset], vTemp, to_do);

                offset += to_do;
             }
        }

        void ButterworthFilter::process_mul(float *dst, const float *src, size_t count)
        {
            update_settings();

            if (src == NULL)
            {
                // No inputs, interpret `src` as zeros: dst[i] = dst[i] * 0 = 0
                dsp::fill_zero(dst, count);
                return;
            }
            else if (bBypass)
            {
                // Bypass is set: dst[i] = dst[i] * src[i]
                dsp::mul2(dst, src, count);
                return;
            }

            // 1X buffer for temporary processing.
            float vTemp[BUF_LIM_SIZE];

            for (size_t offset=0; offset < count; )
            {
                size_t to_do = lsp_min(count - offset, BUF_LIM_SIZE);

                // dst[i] = dst[i] * filter(src[i])
                sFilter.process(vTemp, &src[offset], to_do);
                dsp::mul2(&dst[offset], vTemp, to_do);

                offset += to_do;
             }
        }

        void ButterworthFilter::process_overwrite(float *dst, const float *src, size_t count)
        {
            update_settings();

            if (src == NULL)
                dsp::fill_zero(dst, count);
            else if (bBypass)
                dsp::copy(dst, src, count);
            else
                sFilter.process(dst, src, count);
        }

        void ButterworthFilter::dump(IStateDumper *v) const
        {
            v->write("nOrder", nOrder);
            v->write("fCutoffFreq", fCutoffFreq);
            v->write("nSampleRate", nSampleRate);
            v->write("enFilterType", enFilterType);
            v->write_object("sFilter", &sFilter);
            v->write("bBypass", bBypass);
            v->write("bSync", bSync);
        }

    }
}
