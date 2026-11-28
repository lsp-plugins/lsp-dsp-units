/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Stefano Tronci <stefano.tronci@protonmail.com>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 16 Sept 2021
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

#include <lsp-plug.in/dsp-units/filters/SpectralTilt.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp/common/filters/transform.h>

#define MAX_ORDER               128u
#define DFL_LOWER_FREQUENCY     0.1f
#define DFL_UPPER_FREQUENCY     20.0e3f
#define BUF_LIM_SIZE            256u

#define DB_PER_OCTAVE_FALLOFF   0.16609640419483184814453125f
#define DB_PER_DECADE_FALLOFF   0.05f

namespace lsp
{
    namespace dspu
    {
        static constexpr float F_PI   = float(M_PI);

        SpectralTilt::SpectralTilt()
        {
            construct();
        }

        SpectralTilt::~SpectralTilt()
        {
            destroy();
        }

        void SpectralTilt::construct()
        {
            nOrder          = 1;

            enSlopeUnit     = STLT_SLOPE_UNIT_NEPER_PER_NEPER;
            enNorm          = STLT_NORM_AUTO;
            fSlopeVal       = 0.5f;
            fSlopeNepNep    = 0.5f;

            fLowerFrequency = DFL_LOWER_FREQUENCY;
            fUpperFrequency = DFL_UPPER_FREQUENCY;

            nSampleRate     = -1;

            bBypass         = false;

            bSync           = true;

            sFilter.construct();
        }

        void SpectralTilt::init()
        {
            sFilter.init(MAX_ORDER);
        }

        void SpectralTilt::destroy()
        {
            sFilter.destroy();
        }

        void SpectralTilt::set_sample_rate(size_t sr)
        {
            if (nSampleRate == sr)
                return;

            nSampleRate = sr;
            bSync       = true;
        }

        void SpectralTilt::set_upper_frequency(float upperFrequency)
        {
            if (upperFrequency == fUpperFrequency)
                return;

            fUpperFrequency = upperFrequency;
            bSync           = true;
        }

        void SpectralTilt::set_lower_frequency(float lowerFrequency)
        {
            if (lowerFrequency == fLowerFrequency)
                return;

            fLowerFrequency = lowerFrequency;
            bSync           = true;
        }

        void SpectralTilt::set_frequency_range(float lower, float upper)
        {
            if (upper > lower)
                lsp::swap(upper, lower);

            if ((lower == fLowerFrequency) && (upper == fUpperFrequency))
                return;

            fLowerFrequency = lower;
            fUpperFrequency = upper;
            bSync           = true;
        }

        void SpectralTilt::set_norm(stlt_norm_t norm)
        {
            enNorm = norm;
            bSync = true;
        }

        void SpectralTilt::set_slope(float slope, stlt_slope_unit_t slopeType)
        {
            if ((slope == fSlopeVal) && (slopeType == enSlopeUnit))
                return;

            fSlopeVal   = slope;
            enSlopeUnit = slopeType;
            bSync       = true;
        }

        void SpectralTilt::set_order(size_t order)
        {
            if (order == nOrder)
                return;

            nOrder  = order;
            bSync   = true;
        }

        // Compute the coefficient for the bilinear transform warping equation.
        // When this coefficient is used in bilinear_prewarp, the angularFrequency argument in the bilinear_prewarp method maps to that of this method.
        float SpectralTilt::bilinear_coefficient(float angularFrequency, float samplerate)
        {
            return angularFrequency / tanf(0.5f * angularFrequency / samplerate);
        }


        float SpectralTilt::digital_biquad_gain(dsp::biquad_x1_t *digitalbq, float frequency)
        {
            // Using double and wrapped angles for maximal accuracy.
            double w = 2.0 * M_PI * frequency / nSampleRate;
            w = fmod(w + M_PI, 2.0 * M_PI);
            w = (w >= 0.0) ? w - M_PI : w + M_PI;

            double cw   = cos(w);
            double sw   = sin(w);
            double c2w  = cw*cw - sw*sw;    // cos(2.0 * w);
            double s2w  = 2.0f * cw * sw;   // sin(2.0 * w);

            double num_re = digitalbq->b0 + digitalbq->b1 * cw + digitalbq->b2 * c2w;
            double num_im = -digitalbq->b1 * sw - digitalbq->b2 * s2w;

            double den_re = 1.0 - digitalbq->a1 * cw - digitalbq->a2 * c2w;
            double den_im = digitalbq->a1 * sw + digitalbq->a2 * s2w;
            double den_sq_mag = 1.0 / (den_re * den_re + den_im * den_im);

            double gain_re = (num_re * den_re + num_im * den_im) * den_sq_mag;
            double gain_im = (num_im * den_re - num_re * den_im) * den_sq_mag;

            return sqrt(gain_re * gain_re + gain_im * gain_im);
        }

        void SpectralTilt::normalise_digital_biquad(dsp::biquad_x1_t *digitalbq)
        {
            // The gain to apply to the biquad is the reciprocal of the gain the biquad has at the specified frequency.
            // In other words: make gain 1 at the specified frequency.
            float gain;
            switch (enNorm)
            {
                case STLT_NORM_AT_DC:
                    gain = 1.0f / digital_biquad_gain(digitalbq, 0.0f);
                    break;

                case STLT_NORM_AT_20_HZ:
                    gain = 1.0f / digital_biquad_gain(digitalbq, 20.0f);
                    break;

                case STLT_NORM_AT_1_KHZ:
                    gain = 1.0f / digital_biquad_gain(digitalbq, 1000.0f);
                    break;

                case STLT_NORM_AT_20_KHZ:
                    gain = 1.0f / digital_biquad_gain(digitalbq, 20000.0f);
                    break;

                case STLT_NORM_AT_NYQUIST:
                    gain = 1.0f / digital_biquad_gain(digitalbq, 0.5f * nSampleRate);
                    break;

                case STLT_NORM_AUTO:
                {
                    // Normalise at 20 Hz (if sample rate is big enough) when the slope is negative.
                    // Normalise at 20 kHz (if the sample rate is big enough) when the slope is positive.
                    if (fSlopeNepNep <= 0)
                        gain = (0.5f * nSampleRate > 20.0f) ? 1.0f / digital_biquad_gain(digitalbq, 20.0f) : 1.0f / digital_biquad_gain(digitalbq, 0.0f);
                    else
                        gain = (0.5f * nSampleRate > 20000.0f) ? 1.0f / digital_biquad_gain(digitalbq, 20000.0f) : 1.0f / digital_biquad_gain(digitalbq, 0.5f * nSampleRate);
                }
                break;

                default:
                case STLT_NORM_NONE:
                    gain = 1.0f;
                    break;
            }

            digitalbq->b0 *= gain;
            digitalbq->b1 *= gain;
            digitalbq->b2 *= gain;
        }

        void SpectralTilt::update_settings()
        {
            if (!bSync)
                return;

            // We force even order (so all biquads have all coefficients, maximal efficiency).
            nOrder = lsp_min((nOrder + 1) & (~size_t(1)), MAX_ORDER);

            // Convert provided slope value to Neper-per-Neper.
            switch (enSlopeUnit)
            {

                /** Log-Magnitude of the desired response is given by:
                 *
                 * g * log_{b1} (b2^{x * a})
                 *
                 * where x = log_{b2}(w), w being the angular frequency.
                 * a is the exponent for w: we want the magnitude to go like w^a.
                 *
                 * Neper-per-neper means g = 1, b1 = b2 = e
                 * dB-per-octave means g = 20, b1 = 10, b2 = 2
                 * dB-per-decade means g = 20, b1 = b2 = 10
                 *
                 * To convert from any arbitrary choice of g, b1 and b2 to neper-per-neper use:
                 *
                 * a_g_b1_b2 = a_neper_per_neper * g * ln(b2) / ln(b1)
                 *
                 * => a_neper_per_neper = ln(b1) * a_g_b1_b2 / (g * ln(b2))
                 *
                 * with ln the base e logarithm.
                 *
                 * Hence:
                 *
                 * To convert from dB-per-octave to neper-per-neper multiply by:
                 * ln(10) / (20 * ln(2)) = 0.16609640419483184814453125
                 *
                 * To convert from dB-per-decade to neper-per-neper multiply by:
                 * ln(10) / (20 * ln(10)) = 1 / 20 = 0.05
                 *
                 */

                case STLT_SLOPE_UNIT_DB_PER_OCTAVE:
                    fSlopeNepNep = fSlopeVal * DB_PER_OCTAVE_FALLOFF;
                    break;

                case STLT_SLOPE_UNIT_DB_PER_DECADE:
                    fSlopeNepNep = fSlopeVal * DB_PER_DECADE_FALLOFF;
                    break;

                default:
                case STLT_SLOPE_UNIT_NEPER_PER_NEPER:
                    fSlopeNepNep = fSlopeVal;
                    break;
            }

            if (fLowerFrequency >= 0.5f * nSampleRate)
                fLowerFrequency = DFL_LOWER_FREQUENCY;

            if (fUpperFrequency >= 0.5f * nSampleRate)
                fUpperFrequency = DFL_UPPER_FREQUENCY;

            if (fLowerFrequency >= fUpperFrequency)
            {
                fLowerFrequency = DFL_LOWER_FREQUENCY;
                fUpperFrequency = DFL_UPPER_FREQUENCY;
            }

            if ((enSlopeUnit == STLT_SLOPE_UNIT_NONE) || (fSlopeNepNep == 0.0f))
            {
                bBypass = true;
                bSync = false;
                return;
            }
            else
                bBypass = false;

            const float l_angf = 2.0f * F_PI * fLowerFrequency;
            const float u_angf = 2.0f * F_PI * fUpperFrequency;

            // Exponential spacing ratio for poles.
            const float r  = powf(u_angf / l_angf, 1.0f / (nOrder - 1));
            const float r2 = r*r;
            const float c_fn = bilinear_coefficient(1.0f, nSampleRate);
            float negZero = l_angf * powf(r, -fSlopeNepNep);
            float negPole = l_angf;

            // We have nOrder bilinears. We combine them 2 by 2 to get nOrder / 2 biquads.
            dsp::f_cascade_t analogbq;

            sFilter.begin();
            for (size_t n = 0; n < nOrder; n += 2)
            {
                /** Take a zero and a pole from an exponentially spaced series and construct an analog bilinear filter.
                 *
                 * Analog bilinear filter: the coefficients are the same as the pole and zero values. Form of the analog filter:
                 *
                 *         b0[0] + b1[0]*s     b0[1] + b1[1]*s
                 * H(s) = ----------------- x -----------------
                 *         a0[0] + a1[0]*s     a0[1] + a1[1]*s
                 *
                 * Here b1[i] = a1[i] = 1, then:
                 *
                 *         b0[0]*b0[1] + (b0[0]+b0[1])*s + s^2
                 * H(s) = -------------------------------------
                 *         a0[0]*a0[1] + (a0[0]+a0[1])*s + s^2
                 *
                 */

                const float now_b0 = negZero;
                const float now_a0 = negPole;
                const float nxt_b0 = negZero*r;
                const float nxt_a0 = negPole*r;
                negZero *= r2;
                negPole *= r2;

//                lsp_trace("this=%p, analog filter %2d: poles = {%f, %f}, zeros={%f, %f}, kf=%f, r=%f",
//                    this, int(n),
//                    now_a0, nxt_a0,
//                    now_b0, nxt_b0,
//                    c_fn, r);

                dsp::biquad_x1_t * const digitalbq = sFilter.add_chain();
                if (digitalbq == NULL)
                    return;

                analogbq.t[0] = now_b0 * nxt_b0;
                analogbq.t[1] = now_b0 + nxt_b0;
                analogbq.t[2] = 1.0f;
                analogbq.b[0] = now_a0 * nxt_a0;
                analogbq.b[1] = now_a0 + nxt_a0;
                analogbq.b[2] = 1.0f;
//                lsp_trace("this=%p, analog filter %2d: top={%f, %f, %f}, bottom={%f, %f, %f}",
//                    this, int(n),
//                    analogbq.t[0], analogbq.t[1], analogbq.t[2],
//                    analogbq.b[0], analogbq.b[1], analogbq.b[2]);

                // The denominator coefficients in digitalbq will have opposite sign with respect the maths.
                // This is correct, as this is the LSP convention.
                dsp::bilinear_transform_x1(digitalbq, &analogbq, c_fn, 1);
//                lsp_trace("this=%p, biquad filter %2d: b={%f, %f, %f} a={%f, %f, %f}",
//                    this, int(n),
//                    digitalbq->b0, digitalbq->b1, digitalbq->b2,
//                    1.0f, -digitalbq->a1, -digitalbq->a2);

                normalise_digital_biquad(digitalbq);
//                lsp_trace("this=%p, normalized biquad %2d: b={%f, %f, %f} a={%f, %f, %f}",
//                    this, int(n),
//                    digitalbq->b0, digitalbq->b1, digitalbq->b2,
//                    1.0f, -digitalbq->a1, -digitalbq->a2);
            }
            sFilter.end(true);

            bSync = false;
        }

        void SpectralTilt::process_add(float *dst, const float *src, size_t count)
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
            float vTemp[BUF_LIM_SIZE] __lsp_aligned64;

            for (size_t offset=0; offset < count; )
            {
                const size_t to_do  = lsp_min(count - offset, BUF_LIM_SIZE);

                // dst[i] = dst[i] + filter(src[i])
                sFilter.process(vTemp, &src[offset], to_do);
                dsp::add2(&dst[offset], vTemp, to_do);

                offset += to_do;
             }
        }

        void SpectralTilt::process_mul(float *dst, const float *src, size_t count)
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
            float vTemp[BUF_LIM_SIZE] __lsp_aligned64;

            for (size_t offset=0; offset < count; )
            {
                const size_t to_do  = lsp_min(count - offset, BUF_LIM_SIZE);

                // dst[i] = dst[i] * filter(src[i])
                sFilter.process(vTemp, &src[offset], to_do);
                dsp::mul2(&dst[offset], vTemp, to_do);

                offset += to_do;
             }
        }

        void SpectralTilt::process_overwrite(float *dst, const float *src, size_t count)
        {
            update_settings();

            if (src == NULL)
                dsp::fill_zero(dst, count);
            else if (bBypass)
                dsp::copy(dst, src, count);
            else
                sFilter.process(dst, src, count);
        }

        void SpectralTilt::complex_transfer_calc(float *re, float *im, float f)
        {
            // Calculating normalized frequency, wrapped for maximal accuracy:
            float kf    = f / float(nSampleRate);
            float w     = 2.0f * F_PI * kf;
            w           = fmodf(w + F_PI, 2.0f * F_PI);
            w           = w >= 0.0f ? (w - F_PI) : (w + F_PI);

            // Auxiliary variables:
            float cw    = cosf(w);
            float sw    = sinf(w);

            // These equations are valid since sw has valid sign
            float c2w   = cw * cw - sw * sw;    // cos(2 * w)
            float s2w   = 2.0f * sw * cw;       // sin(2 * w)

            float r_re  = 1.0f, r_im = 0.0f;    // The result complex number
            float b_re, b_im;                   // Temporary values for computing complex multiplication

            for (size_t i=0, count=sFilter.size(); i<count; ++i)
            {
                dsp::biquad_x1_t *bq = sFilter.chain(i);
                if (!bq)
                    continue;

                float num_re = bq->b0 + bq->b1 * cw + bq->b2 * c2w;
                float num_im = -bq->b1 * sw - bq->b2 * s2w;

                // Denominator coefficients have opposite sign in LSP with respect maths conventions.
                float den_re = 1.0 - bq->a1 * cw - bq->a2 * c2w;
                float den_im = bq->a1 * sw + bq->a2 * s2w;

                float den_sq_mag = den_re * den_re + den_im * den_im;

                // Compute current biquad's frequency response
                float w_re = (num_re * den_re + num_im * den_im) / den_sq_mag;
                float w_im = (den_re * num_im - num_re * den_im) / den_sq_mag;

                // Compute common transfer function as a product between current biquad's
                // transfer function and previous value
                b_re            = r_re*w_re - r_im*w_im;
                b_im            = r_re*w_im + r_im*w_re;

                // Commit changes to the result complex number
                r_re            = b_re;
                r_im            = b_im;
            }

            *re     = r_re;
            *im     = r_im;
        }

        void SpectralTilt::freq_chart(float *re, float *im, const float *f, size_t count)
        {
            update_settings();

            for (size_t i = 0; i<count; ++i)
                complex_transfer_calc(&re[i], &im[i], f[i]);
        }

        void SpectralTilt::freq_chart(float *c, const float *f, size_t count)
        {
            update_settings();

            size_t c_idx = 0;
            for (size_t i = 0; i<count; ++i)
            {
                complex_transfer_calc(&c[c_idx], &c[c_idx + 1], f[i]);
                c_idx += 2;
            }
        }

        void SpectralTilt::dump(IStateDumper *v) const
        {
            v->write("nOrder", nOrder);

            v->write("enSlopeType", enSlopeUnit);
            v->write("enNorm", enNorm);
            v->write("fSlopeVal", fSlopeVal);
            v->write("fSlopeNepNep", fSlopeNepNep);

            v->write("fLowerFrequency", fLowerFrequency);
            v->write("fUpperFrequency", fUpperFrequency);

            v->write("nSampleRate", nSampleRate);

            v->write_object("sFilter", &sFilter);

            v->write("bBypass", bBypass);
            v->write("bSync", bSync);
        }
    } /* namespace dspu */
} /* namespace lsp */
