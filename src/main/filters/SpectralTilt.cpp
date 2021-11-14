/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Stefano Tronci <stefano.tronci@protonmail.com>
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

#define MAX_ORDER               100u
#define DFL_LOWER_FREQUENCY     0.1f
#define DFL_UPPER_FREQUENCY     20.0e3f
#define BUF_LIM_SIZE            2048u

namespace lsp
{
    namespace dspu
    {

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
            fSlopeVal       = 0.5f;
            fSlopeNepNep    = 0.5f;

            fLowerFrequency = DFL_LOWER_FREQUENCY;
            fUpperFrequency = DFL_UPPER_FREQUENCY;

            nSampleRate     = -1;

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

        void SpectralTilt::destroy()
        {
            free_aligned(pData);
            pData = NULL;
            vBuffer = NULL;
        }

        // Compute the coefficient for the bilinear transform warping equation.
        // When this coefficient is used in bilinear_prewarp, the angularFrequency argument in the bilinear_prewarp method maps to that of this method.
        float SpectralTilt::bilinear_coefficient(float angularFrequency, float samplerate)
        {
            return angularFrequency / tanf(0.5f * angularFrequency / samplerate);
        }

//        // Compute the bilinear transform warping equation.
//        float SpectralTilt::bilinear_prewarp(float coefficient, float angularFrequency, float samplerate)
//        {
//            return coefficient * tanf(0.5f * angularFrequency / samplerate);
//        }

//        SpectralTilt::bilinear_spec_t SpectralTilt::compute_bilinear_element(float negZero, float negPole, float c_prewarp, float c_final)
//        {
//            /** Take a zero and a pole from an exponentially spaced series and construct a digital bilinear filter.
//             *
//             * First, make an analog bilinear filter: the coefficients are the same as the pole and zero values. Form of the analog filter:
//             *
//             *         s + b_a_0
//             * H(s) = -----------; b_a_1 = a_a_1 = 1
//             *         s + a_a_0
//             *
//             * Then, bilinear transform this filter to obtain a digital bilinear. We use prewarp.
//             */
//
//            float b_a_0 = bilinear_prewarp(c_prewarp, negZero, nSampleRate);
//            float b_a_1 = 1.0f;
//
//            float a_a_0 = bilinear_prewarp(c_prewarp, negPole, nSampleRate);
////            float a_a_1 = 1.0f;
//
//            // This gain provides unit magnitude response at DC.
//            float g_a = a_a_0 / b_a_0;
//
//            b_a_0 *= g_a;
//            b_a_1 *= g_a;
//
//            // We now apply the bilinear transform equations for the analog bilinear filter.
//            float g_d = 1.0f / (a_a_0 + c_final);
//
//            bilinear_spec_t spec;
//
//            spec.b0 = (b_a_0 + b_a_1 * c_final) * g_d;
//            spec.b1 = (b_a_0 - b_a_1 * c_final) * g_d;
//
//            spec.a0 = 1.0f;
//            spec.a1 = (a_a_0 - c_final) * g_d;

        SpectralTilt::bilinear_spec_t SpectralTilt::compute_bilinear_element(float negZero, float negPole)
        {
            /** Take a zero and a pole from an exponentially spaced series and construct an analog bilinear filter.
             *
             * Analog bilinear filter: the coefficients are the same as the pole and zero values. Form of the analog filter:
             *
             *         s + b0
             * H(s) = -----------; b1 = a1 = 1
             *         s + a0
             *
             */

            // Just return it analog, prewarp not necessary.
            bilinear_spec_t spec;

            spec.b0 = negZero;
            spec.b1 = 1.0f;

            spec.a0 = negPole;
            spec.a1 = 1.0f;

            float g_a = spec.a0 / spec.b0;

            spec.b0 *= g_a;
            spec.b1 *= g_a;

            return spec;
        }

        void SpectralTilt::update_settings()
        {
            if (!bSync)
                return;

            // We force even order (so all biquads have all coefficients, maximal efficiency).
            nOrder = (nOrder % 2 == 0) ? nOrder : nOrder + 1;
            nOrder = lsp_min(nOrder, MAX_ORDER);

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
                 * with ln the base e logarithm.
                 */

                case STLT_SLOPE_UNIT_DB_PER_OCTAVE:
                {
                    fSlopeNepNep = fSlopeVal * logf(10.0f) / (20.0f * logf(2.0f));
                }
                break;

                case STLT_SLOPE_UNIT_DB_PER_DECADE:
                {
                    fSlopeNepNep = fSlopeVal / 20.0f;
                }
                break;

                default:
                case STLT_SLOPE_UNIT_NEPER_PER_NEPER:
                {
                    fSlopeNepNep = fSlopeVal;
                }
                break;

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
                    bSync = true;
                    return;
                }
                else
                {
                    bBypass = false;
                }

                float l_angf = 2.0f * M_PI * fLowerFrequency;
                float u_angf = 2.0f * M_PI * fUpperFrequency;

                // Exponential spacing ratio for poles.
                float r = powf(u_angf / l_angf, 1.0f / (nOrder - 1));

//                float c_pw = bilinear_coefficient(l_angf, nSampleRate);
                float c_fn = bilinear_coefficient(1.0f, nSampleRate);

                float negZero = l_angf * powf(r, fSlopeNepNep);
                float negPole = l_angf;

                // We have nOrder bilinears. We combine them 2 by 2 to get biquads.

                sFilter.begin();
                for (size_t n = 0; n < nOrder; ++n)
                {
                    if (n % 2 != 0)
                        continue;

//                    bilinear_spec_t spec_now = compute_bilinear_element(
//                            negZero,
//                            negPole,
//                            c_pw,
//                            c_fn
//                            );
//                    negZero *= r;
//                    negPole *= r;
//
//                    bilinear_spec_t spec_nxt = compute_bilinear_element(
//                            negZero,
//                            negPole,
//                            c_pw,
//                            c_fn
//                            );
//                    negZero *= r;
//                    negPole *= r;

                    bilinear_spec_t spec_now = compute_bilinear_element(negZero, negPole);
                    negZero *= r;
                    negPole *= r;

                    bilinear_spec_t spec_nxt = compute_bilinear_element(negZero, negPole);
                    negZero *= r;
                    negPole *= r;

                    dsp::biquad_x1_t *digitalbq = sFilter.add_chain();
                    if (digitalbq == NULL)
                        return;

                    // Sign for a[n] (denominator) coefficients needs to be inverted for assignment.
                    // In other words, with respect the maths, f->a1 and f->a2 below have inverted sign.

                    dsp::f_cascade_t analogbq;
                    analogbq.t[0] = spec_now.b0 * spec_nxt.b0;
                    analogbq.t[1] = spec_now.b0 * spec_nxt.b1 + spec_now.b1 * spec_nxt.b0;
                    analogbq.t[2] = spec_now.b1 * spec_nxt.b1;
                    analogbq.b[0] = 1.0f;
                    analogbq.b[1] = -spec_now.a1 - spec_nxt.a1;
                    analogbq.b[2] = -spec_now.a1 * spec_nxt.a1;

//                    digitalbq->b0 = spec_now.b0 * spec_nxt.b0;
//                    digitalbq->b1 = spec_now.b0 * spec_nxt.b1 + spec_now.b1 * spec_nxt.b0;
//                    digitalbq->b2 = spec_now.b1 * spec_nxt.b1;
//                    digitalbq->a1 = -spec_now.a1 - spec_nxt.a1;
//                    digitalbq->a2 = -spec_now.a1 * spec_nxt.a1;
//                    digitalbq->p0 = 0.0f;
//                    digitalbq->p1 = 0.0f;
//                    digitalbq->p2 = 0.0f;

                    dsp::bilinear_transform_x1(digitalbq, &analogbq, c_fn, 1);
                }
                sFilter.end(true);

            }

            bSync = true;
        }

        void SpectralTilt::process_add(float *dst, const float *src, size_t count)
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

        void SpectralTilt::process_mul(float *dst, const float *src, size_t count)
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

        void SpectralTilt::process_overwrite(float *dst, const float *src, size_t count)
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

        void SpectralTilt::dump(IStateDumper *v) const
        {
            v->write("nOrder", nOrder);

            v->write("enSlopeType", enSlopeUnit);
            v->write("fSlopeVal", fSlopeVal);
            v->write("fSlopeNepNep", fSlopeNepNep);

            v->write("fLowerFrequency", fLowerFrequency);
            v->write("fUpperFrequency", fUpperFrequency);

            v->write("nSampleRate", nSampleRate);

            v->write_object("sFilter", &sFilter);

            v->write("pData", pData);
            v->write("vBuffer", vBuffer);

            v->write("bBypass", bBypass);

            v->write("bSync", bSync);
        }
    }
}
