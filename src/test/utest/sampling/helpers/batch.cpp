/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 25 нояб. 2022 г.
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

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/sampling/helpers/batch.h>
#include <lsp-plug.in/dsp-units/sampling/helpers/playback.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>
#include <lsp-plug.in/test-fw/helpers.h>
#include <lsp-plug.in/test-fw/utest.h>

#define SAMPLE_LENGTH       8

static const float test_data0[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
static const float test_data1[] = { 8, 7, 6, 5, 4, 3, 2, 1 };
static const float test_data2[] = { 2, 3, 4, 5, 6, 7 };
static const float test_data3[] = { 7, 6, 5, 4, 3, 2 };

static const float test_xfade_linear0[] = { 1*0.0f, 2*0.25f, 3*0.5f, 4*0.75f, 5, 6, 7, 8 };
static const float test_xfade_linear1[] = { 1, 2, 3, 4, 5 * 1.0f, 6 * 0.75f, 7 * 0.5f, 8 * 0.25f };
static const float test_xfade_linear2[] = { 8*0.0f, 7*0.25f, 6*0.5f, 5*0.75f, 4, 3, 2, 1 };
static const float test_xfade_linear3[] = { 8, 7, 6, 5, 4*1.0f, 3*0.75f, 2*0.5f, 1*0.25f };

static const float test_xfade_cpower0[]  = { 1*sqrtf(0.0f), 2*sqrtf(0.25f), 3*sqrtf(0.5f), 4*sqrtf(0.75f), 5, 6, 7, 8 };
static const float test_xfade_cpower1[]  = { 1, 2, 3, 4, 5 * sqrtf(1.0f), 6 * sqrtf(0.75f), 7 * sqrtf(0.5f), 8 * sqrtf(0.25f) };
static const float test_xfade_cpower2[]  = { 8*sqrtf(0.0f), 7*sqrtf(0.25f), 6*sqrtf(0.5f), 5*sqrtf(0.75f), 4, 3, 2, 1 };
static const float test_xfade_cpower3[]  = { 8, 7, 6, 5, 4*sqrtf(1.0f), 3*sqrtf(0.75f), 2*sqrtf(0.5f), 1*sqrtf(0.25f) };

UTEST_BEGIN("dspu.sampling.helpers", batch)

    void test_batch(dspu::playback::playback_t *pb, const float *buf_data, size_t buf_size)
    {
        for (size_t real_buf_size = buf_size / 2; real_buf_size <= buf_size * 2; real_buf_size += 2)
        {
            for (size_t delay=0; delay < 8; delay += 2)
            {
                for (size_t step=1; step < buf_size; ++step)
                {
                    printf("  testing playback: real_buf_size=%d, delay=%d, step=%d\n",
                        int(real_buf_size), int(delay), int(step));

                    FloatBuffer tmp(real_buf_size);
                    FloatBuffer chk(real_buf_size);
                    tmp.randomize(0.0f, 0.1f); // Add some noise to check that batches are applied in additive mode
                    chk.copy(tmp);

                    pb->nTimestamp              = 0;
                    pb->nPosition               = -1;
                    pb->sBatch[0].nTimestamp    = delay;
                    size_t offset               = 0;
                    size_t est_processed        = lsp_min(buf_size + delay, real_buf_size);

                    if (real_buf_size > delay)
                    {
                        size_t to_copy = lsp_min(buf_size, real_buf_size - delay);
                        dsp::add2(chk.data(delay), buf_data, to_copy);
                    }

                    while (true)
                    {
                        size_t to_do    = lsp_min(real_buf_size - offset, step);
                        if (to_do <= 0)
                            break;
                        size_t done     = dspu::playback::execute_batch(&tmp[offset], &pb->sBatch[0], pb, to_do);
                        if (done <= 0)
                            break;
                        offset         += done;
                        pb->nTimestamp += done;

                        printf("    pb->timestamp = %d, pb->position = %d, done=%d\n", int(pb->nTimestamp), int(pb->nPosition), int(done));
                    }

                    UTEST_ASSERT(offset == est_processed);
                    UTEST_ASSERT(!chk.corrupted());
                    UTEST_ASSERT(!tmp.corrupted());
                    if (!tmp.equals_relative(chk))
                    {
                        tmp.dump("src");
                        chk.dump("chk");
                        UTEST_FAIL_MSG("The processing result differs at sample %d: %.6f vs %.6f",
                           int(tmp.last_diff()), tmp.get_diff(), chk.get_diff());
                    }
                } // step
            } // delay
        } // real_buf_size
    };

    UTEST_MAIN
    {
        // Init sample
        dspu::Sample s;
        UTEST_ASSERT(s.init(1, 8, 8));
        float *dst = s.channel(0);
        for (size_t i=0; i<8; ++i)
            dst[i] = 1.0f + i;

        // Init playback
        dspu::playback::playback_t pb;
        dspu::playback::clear_playback(&pb);
        pb.enState      = dspu::playback::STATE_PLAY;
        pb.pSample      = &s;
        pb.fVolume      = 1.0f;
        pb.nChannel     = 0;
        pb.nXFade       = 0;
        pb.enXFadeType  = dspu::SAMPLE_CROSSFADE_LINEAR;

        dspu::playback::batch_t *b = &pb.sBatch[0];
        b->enType       = dspu::playback::BATCH_TAIL;

        // Perform tests for non-modified samples
        printf("Testing direct playback of full sample...\n");
        b->nStart       = 0;
        b->nEnd         = 8;
        b->nFadeIn      = 0;
        b->nFadeOut     = 0;
        test_batch(&pb, test_data0, 8);

        printf("Testing reverse playback of full sample...\n");
        b->nStart       = 8;
        b->nEnd         = 0;
        b->nFadeIn      = 0;
        b->nFadeOut     = 0;
        test_batch(&pb, test_data1, 8);

        // Perform tests for partially-cut sample
        printf("Testing direct playback of partial sample...\n");
        b->nStart       = 1;
        b->nEnd         = 7;
        b->nFadeIn      = 0;
        b->nFadeOut     = 0;
        test_batch(&pb, test_data2, 6);

        printf("Testing reverse playback of partial sample...\n");
        b->nStart       = 7;
        b->nEnd         = 1;
        b->nFadeIn      = 0;
        b->nFadeOut     = 0;
        test_batch(&pb, test_data3, 6);

        // Perform test of the linear cross-fade of the sample
        printf("Testing direct playback of linear faded-in sample...\n");
        pb.enXFadeType  = dspu::SAMPLE_CROSSFADE_LINEAR;
        b->nStart       = 0;
        b->nEnd         = 8;
        b->nFadeIn      = 4;
        b->nFadeOut     = 0;
        test_batch(&pb, test_xfade_linear0, 8);

        printf("Testing direct playback of linear faded-out sample...\n");
        pb.enXFadeType  = dspu::SAMPLE_CROSSFADE_LINEAR;
        b->nStart       = 0;
        b->nEnd         = 8;
        b->nFadeIn      = 0;
        b->nFadeOut     = 4;
        test_batch(&pb, test_xfade_linear1, 8);

        printf("Testing reverse playback of linear faded-in sample...\n");
        pb.enXFadeType  = dspu::SAMPLE_CROSSFADE_LINEAR;
        b->nStart       = 8;
        b->nEnd         = 0;
        b->nFadeIn      = 4;
        b->nFadeOut     = 0;
        test_batch(&pb, test_xfade_linear2, 8);

        printf("Testing reverse playback of linear faded-out sample...\n");
        pb.enXFadeType  = dspu::SAMPLE_CROSSFADE_LINEAR;
        b->nStart       = 8;
        b->nEnd         = 0;
        b->nFadeIn      = 0;
        b->nFadeOut     = 4;
        test_batch(&pb, test_xfade_linear3, 8);

        // Perform test of the linear cross-fade of the sample
        printf("Testing direct playback of constant-power faded-in sample...\n");
        pb.enXFadeType  = dspu::SAMPLE_CROSSFADE_CONST_POWER;
        b->nStart       = 0;
        b->nEnd         = 8;
        b->nFadeIn      = 4;
        b->nFadeOut     = 0;
        test_batch(&pb, test_xfade_cpower0, 8);

        printf("Testing direct playback of constant-power faded-out sample...\n");
        pb.enXFadeType  = dspu::SAMPLE_CROSSFADE_CONST_POWER;
        b->nStart       = 0;
        b->nEnd         = 8;
        b->nFadeIn      = 0;
        b->nFadeOut     = 4;
        test_batch(&pb, test_xfade_cpower1, 8);

        printf("Testing reverse playback of constant-power faded-in sample...\n");
        pb.enXFadeType  = dspu::SAMPLE_CROSSFADE_CONST_POWER;
        b->nStart       = 8;
        b->nEnd         = 0;
        b->nFadeIn      = 4;
        b->nFadeOut     = 0;
        test_batch(&pb, test_xfade_cpower2, 8);

        printf("Testing reverse playback of constant-power faded-out sample...\n");
        pb.enXFadeType  = dspu::SAMPLE_CROSSFADE_CONST_POWER;
        b->nStart       = 8;
        b->nEnd         = 0;
        b->nFadeIn      = 0;
        b->nFadeOut     = 4;
        test_batch(&pb, test_xfade_cpower3, 8);
    }
UTEST_END;



