/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 26 нояб. 2022 г.
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

static constexpr float xfl(float a, float b, float k)
{
    return a * (1.0f - k) + b * k;
}

static const float sample_data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
static const float test_playback_no_delay[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
static const float test_playback_short_delay[] = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
static const float test_playback_with_start_position[] = { 5, 6, 7, 8, 9, 10, 11, 12 };

static const float test_direct_loop_simple[] =
{
    1, 2, 3, 4,     // 0..3:   head
    5, 6, 7, 8,     // 4..7:   loop 1
    5, 6, 7, 8,     // 8..11:  loop 2
    5, 6, 7, 8,     // 11..15: loop 3
    9, 10, 11, 12   // 16..19: tail
};

static const float test_direct_loop_xfade[] =  {
    1, 2, // 0..1: head
    xfl(3, 3, 0.0f), xfl(4, 4, 0.25f), xfl(5, 5, 0.5f), xfl(6, 6, 0.75f), // 2..5: head-loop 1: no crossfade
    xfl(7, 3, 0.0f), xfl(8, 4, 0.25f), xfl(9, 5, 0.5f), xfl(10, 6, 0.75f), // 6..9: loop 1-2 crossfade
    xfl(7, 3, 0.0f), xfl(8, 4, 0.25f), xfl(9, 5, 0.5f), xfl(10, 6, 0.75f), // 10..13: loop 2-3 crossfade
    xfl(7, 3, 0.0f), xfl(8, 4, 0.25f), xfl(9, 5, 0.5f), xfl(10, 6, 0.75f), // 14..17: loop 3-tail crossfade
    11, 12 // 18..19: tail
};


UTEST_BEGIN("dspu.sampling.helpers", playback)

    void test_playback(const dspu::playback::playback_t *pb, const float *buf_data, size_t buf_size)
    {
        for (size_t real_buf_size = buf_size / 2; real_buf_size <= buf_size * 2; real_buf_size += 2)
        {
            for (size_t step=1; step < lsp_max(real_buf_size, buf_size); ++step)
            {
                printf("  testing playback: real_buf_size=%d, step=%d\n",
                    int(real_buf_size), int(step));

                FloatBuffer dst(real_buf_size);
                FloatBuffer buf(step);
                FloatBuffer chk(real_buf_size);
                dst.randomize(0.0f, 0.1f); // Add some noise to check that batches are applied in additive mode
                chk.copy(dst);

                // Obtain the copy of playback and validate state
                dspu::playback::playback_t xpb = *pb;
                UTEST_ASSERT(xpb.nTimestamp == 0);
                UTEST_ASSERT(xpb.nPosition == -1);

                size_t offset               = 0;
                size_t est_processed        = lsp_min(buf_size, real_buf_size);
                dsp::add2(chk.data(), buf_data, est_processed);

                if ((real_buf_size == 10) && (step == 1))
                    printf("debug\n");

                // Do the processing
                while (true)
                {
                    size_t to_do    = lsp_min(real_buf_size - offset, step);
                    if (to_do <= 0)
                        break;
                    dsp::fill_zero(buf.data(), to_do);

                    size_t done     = dspu::playback::process_playback(buf.data(), &xpb, to_do);
                    if (done <= 0)
                        break;
                    dsp::fmadd_k3(&dst[offset], buf.data(), pb->fVolume, done);

                    offset         += done;
                    printf("    xpb.timestamp = %d, xpb.position = %d, done=%d\n", int(xpb.nTimestamp), int(xpb.nPosition), int(done));
                }

                // Check the final state and result
                UTEST_ASSERT(offset == est_processed);
                UTEST_ASSERT(!chk.corrupted());
                UTEST_ASSERT(!dst.corrupted());
                if (!dst.equals_relative(chk))
                {
                    dst.dump("dst");
                    chk.dump("chk");
                    UTEST_FAIL_MSG("The processing result differs at sample %d: %.6f vs %.6f",
                       int(dst.last_diff()), dst.get_diff(), chk.get_diff());
                }
            } // step
        } // real_buf_size
    };

    UTEST_MAIN
    {
        // Init sample
        dspu::Sample s;
        size_t sample_len = sizeof(sample_data)/sizeof(sample_data[0]);
        UTEST_ASSERT(s.init(1, sample_len, sample_len));
        dsp::copy(s.channel(0), sample_data, sample_len);

        // Init playback
        dspu::playback::playback_t pb;
        dspu::playback::clear_playback(&pb);
        dspu::PlaySettings ps;

        // Initial settings
        ps.set_volume(1.0f);

//        // Test the simple playback
//        printf("Testing playback of full sample without delay...\n");
//        ps.set_delay(0);
//        ps.set_start(0);
//        ps.set_loop_range(dspu::SAMPLE_LOOP_NONE, 0, 0);
//        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
//        dspu::playback::start_playback(&pb, 0, 0, &s, &ps);
//        test_playback(&pb, test_playback_no_delay, 12);
//
//        // Test the simple playback with delay
//        printf("Testing playback of full sample with short delay...\n");
//        ps.set_delay(4);
//        ps.set_start(0);
//        ps.set_loop_range(dspu::SAMPLE_LOOP_NONE, 0, 0);
//        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
//        dspu::playback::start_playback(&pb, 0, 0, &s, &ps);
//        test_playback(&pb, test_playback_short_delay, 16);
//
//        // Test the simple playback with delay
//        printf("Testing playback of full sample with start position...\n");
//        ps.set_delay(0);
//        ps.set_start(4);
//        ps.set_loop_range(dspu::SAMPLE_LOOP_NONE, 0, 0);
//        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
//        dspu::playback::start_playback(&pb, 0, 0, &s, &ps);
//        test_playback(&pb, test_playback_with_start_position, 8);

        // Test simple direct loop without crossfade
//        printf("Testing simple direct loop...\n");
//        ps.set_delay(0);
//        ps.set_start(0);
//        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT, 4, 8);
//        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
//        dspu::playback::start_playback(&pb, 0, 0, &s, &ps);
//        dspu::playback::stop_playback(&pb, 14);
//        test_playback(&pb, test_direct_loop_simple, 20);

        // Test simple direct loop with crossfade
        printf("Testing simple direct loop with crossfade...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT, 2, 10);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 4);
        dspu::playback::start_playback(&pb, 0, 0, &s, &ps);
        dspu::playback::stop_playback(&pb, 16);
        test_playback(&pb, test_direct_loop_xfade, 20);
    }
UTEST_END;



