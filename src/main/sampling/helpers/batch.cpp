/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 24 нояб. 2022 г.
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

#include <lsp-plug.in/dsp-units/sampling/helpers/batch.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        namespace playback
        {
            LSP_DSP_UNITS_PUBLIC
            void clear_batch(batch_t *b)
            {
                b->nTimestamp       = 0;
                b->nStart           = 0;
                b->nEnd             = 0;
                b->nFadeIn          = 0;
                b->nFadeOut         = 0;
                b->enType           = BATCH_NONE;
            }

            LSP_DSP_UNITS_PUBLIC
            void dump_batch_plain(IStateDumper *v, const batch_t *b)
            {
                v->write("nTimestamp", b->nTimestamp);
                v->write("nStart", b->nStart);
                v->write("nEnd", b->nEnd);
                v->write("nFadeIn", b->nFadeIn);
                v->write("nFadeOut", b->nFadeOut);
                v->write("enType", int(b->enType));
            }

            LSP_DSP_UNITS_PUBLIC
            void dump_batch(IStateDumper *v, const batch_t *b)
            {
                v->begin_object(b, sizeof(*b));
                dump_batch_plain(v, b);
                v->end_object();
            }

            LSP_DSP_UNITS_PUBLIC
            void dump_batch(IStateDumper *v, const char *name, const batch_t *b)
            {
                v->begin_object(name, b, sizeof(*b));
                dump_batch_plain(v, b);
                v->end_object();
            }


            /* Batch layout:
             *                samples
             *       |<--------------------->|
             *       |                       |
             *  +---------+-------------+----------+
             *  | fade-in |    body     | fade-out |
             *  +---------+-------------+----------+
             *  |    |    |             |          |
             *  0    t0   t1            t2         t3
             */
            LSP_DSP_UNITS_PUBLIC
            size_t put_batch_linear_direct(float *dst, const float *src, const batch_t *b, wsize_t timestamp, size_t samples)
            {
                // Direct playback, compute batch size and position
                size_t t3   = b->nEnd   - b->nStart;        // Batch size in samples
                size_t t0   = timestamp - b->nTimestamp;    // Initial rendering position inside of the batch
                if (t0 >= t3)
                    return 0;

                size_t t    = t0;                           // Current rendering position inside of the batch
                src        += b->nStart;                    // Adjust pointer for simplification of pointer arithmetics

                // Render the fade-in
                size_t t1   = b->nFadeIn;
                if (t < t1)
                {
                    // Render contents
                    float k     = 1.0f / b->nFadeIn;
                    size_t n    = lsp_min(samples, t1 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[t] * (t * k);

                    // Update the position
                    samples    -= n;
                    if (samples <= 0)
                        return t - t0;
                    dst        += n;
                }

                // Render the body
                size_t t2   = t3 - b->nFadeOut;
                if (t < t2)
                {
                    // Render contents
                    size_t n    = lsp_min(samples, t2 - t);
                    dsp::add2(dst, &src[t], n);

                    // Update the position
                    t          += n;
                    samples    -= n;
                    if (samples <= 0)
                        return t - t0;
                    dst        += n;
                }

                // Render the fade-out
                if (t < t3)
                {
                    // Render the contents
                    float k     = 1.0f / b->nFadeOut;
                    size_t n    = lsp_min(samples, t3 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[t] * ((t3 - t) * k);
                }

                return t - t0;
            }

            LSP_DSP_UNITS_PUBLIC
            size_t put_batch_const_power_direct(float *dst, const float *src, const batch_t *b, wsize_t timestamp, size_t samples)
            {
                // Direct playback, compute batch size and position
                size_t t3   = b->nEnd   - b->nStart;        // Batch size in samples
                size_t t0   = timestamp - b->nTimestamp;    // Initial rendering position inside of the batch
                if (t0 >= t3)
                    return 0;

                size_t t    = t0;                           // Current rendering position inside of the batch
                src        += b->nStart;                    // Adjust pointer for simplification of pointer arithmetics

                // Render the fade-in
                size_t t1   = b->nFadeIn;
                if (t < t1)
                {
                    // Render contents
                    float k     = 1.0f / b->nFadeIn;
                    size_t n    = lsp_min(samples, t1 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[t] * sqrtf(t * k);

                    // Update the position
                    samples    -= n;
                    if (samples <= 0)
                        return t - t0;
                    dst        += n;
                }

                // Render the body
                size_t t2   = t3 - b->nFadeOut;
                if (t < t2)
                {
                    // Render contents
                    size_t n    = lsp_min(samples, t2 - t);
                    dsp::add2(dst, &src[t], n);

                    // Update the position
                    t          += n;
                    samples    -= n;
                    if (samples <= 0)
                        return t - t0;
                    dst        += n;
                }

                // Render the fade-out
                if (t < t3)
                {
                    // Render the contents
                    float k     = 1.0f / b->nFadeOut;
                    size_t n    = lsp_min(samples, t3 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[t] * sqrtf((t3 - t) * k);
                }

                return t - t0;
            }

            LSP_DSP_UNITS_PUBLIC
            size_t put_batch_linear_reverse(float *dst, const float *src, const batch_t *b, wsize_t timestamp, size_t samples)
            {
                // Direct playback, compute batch size and position
                size_t t3   = b->nStart - b->nEnd;          // Batch size in samples
                size_t t0   = timestamp - b->nTimestamp;    // Initial rendering position inside of the batch
                if (t0 >= t3)
                    return 0;

                size_t tr   = t3 - 1;                       // Last sample
                size_t t    = t0;                           // Current rendering position inside of the batch
                src        += b->nEnd;                      // Adjust pointer for simplification of pointer arithmetics

                // Render the fade-in
                size_t t1   = b->nFadeIn;
                if (t < t1)
                {
                    // Render contents
                    float k     = 1.0f / b->nFadeIn;
                    size_t n    = lsp_min(samples, t1 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[tr - t] * (t * k);

                    // Update the position
                    samples    -= n;
                    if (samples <= 0)
                        return t - t0;
                    dst        += n;
                }

                // Render the body
                size_t t2   = t3 - b->nFadeOut;
                if (t < t2)
                {
                    // Render contents
                    size_t n    = lsp_min(samples, t2 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[tr - t];

                    // Update the position
                    samples    -= n;
                    if (samples <= 0)
                        return t - t0;
                    dst        += n;
                }

                // Render the fade-out
                if (t < t3)
                {
                    // Render the contents
                    float k     = 1.0f / b->nFadeOut;
                    size_t n    = lsp_min(samples, t3 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[tr - t] * ((t3 - t) * k);
                }

                return t - t0;
            }

            LSP_DSP_UNITS_PUBLIC
            size_t put_batch_const_power_reverse(float *dst, const float *src, const batch_t *b, wsize_t timestamp, size_t samples)
            {
                // Direct playback, compute batch size and position
                size_t t3   = b->nStart - b->nEnd;          // Batch size in samples
                size_t t0   = timestamp - b->nTimestamp;    // Initial rendering position inside of the batch
                if (t0 >= t3)
                    return 0;

                size_t tr   = t3 - 1;                       // Last sample
                size_t t    = t0;                           // Current rendering position inside of the batch
                src        += b->nEnd;                      // Adjust pointer for simplification of pointer arithmetics

                // Render the fade-in
                size_t t1   = b->nFadeIn;
                if (t < t1)
                {
                    // Render contents
                    float k     = 1.0f / b->nFadeIn;
                    size_t n    = lsp_min(samples, t1 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[tr - t] * sqrtf(t * k);

                    // Update the position
                    samples    -= n;
                    if (samples <= 0)
                        return t - t0;
                    dst        += n;
                }

                // Render the body
                size_t t2   = t3 - b->nFadeOut;
                if (t < t2)
                {
                    // Render contents
                    size_t n    = lsp_min(samples, t2 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[tr - t];

                    // Update the position
                    samples    -= n;
                    if (samples <= 0)
                        return t - t0;
                    dst        += n;
                }

                // Render the fade-out
                if (t < t3)
                {
                    // Render the contents
                    float k     = 1.0f / b->nFadeOut;
                    size_t n    = lsp_min(samples, t3 - t);
                    for (size_t i=0; i<n; ++i, ++t)
                        dst[i]     += src[tr - t] * sqrtf((t3 - t) * k);
                }

                return t - t0;
            }

        } /* namespace playback */
    } /* namespace dspu */
} /* namespace lsp */
