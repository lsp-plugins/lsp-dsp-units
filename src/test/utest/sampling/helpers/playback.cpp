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

static constexpr float S0  = 1.0f;
static constexpr float S1  = 2.01f;
static constexpr float S2  = 3.13f;
static constexpr float S3  = 4.23f;
static constexpr float S4  = 5.47f;
static constexpr float S5  = 6.11f;
static constexpr float S6  = 7.97f;
static constexpr float S7  = 8.31f;
static constexpr float S8  = 9.03f;
static constexpr float S9  = 10.29f;
static constexpr float S10 = 11.79f;
static constexpr float S11 = 12.41f;

static constexpr float xfl(float a, float b, float k)
{
    return a * (1.0f - k) + b * k;
}

static const float sample_data[] = { S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11 };
static const float test_playback_no_delay[] = { S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11 };
static const float test_playback_short_delay[] = { 0, 0, 0, 0, S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11 };
static const float test_playback_with_start_position[] = { S4, S5, S6, S7, S8, S9, S10, S11 };

static const float test_direct_loop_simple[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S4, S5, S6, S7,     // 4..7:   loop 1
    S4, S5, S6, S7,     // 8..11:  loop 2
    S4, S5, S6, S7,     // 11..15: loop 3
    S8, S9, S10, S11    // 16..19: tail
};

static const float test_direct_loop_xfade[] =  {
    S0, S1, // 0..1: head
    S2, S3, S4, S5,  // 2..5: loop 1 start (no crossfade between head and loop)
    xfl(S6, S2, 0.0f), xfl(S7, S3, 0.25f), xfl(S8, S4, 0.5f), xfl(S9, S5, 0.75f),   // 6..9: loop 1-2 crossfade
    xfl(S6, S2, 0.0f), xfl(S7, S3, 0.25f), xfl(S8, S4, 0.5f), xfl(S9, S5, 0.75f),   // 10..13: loop 2-3 crossfade
    S6, S7, S8, S9, // 14..17: loop 3 end (no crossfade between tail and loop)
    S10, S11 // 18..19: tail
};

static const float test_reverse_loop_simple[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S7, S6, S5, S4,     // 4..7:   loop 1
    S7, S6, S5, S4,     // 8..11:  loop 2
    S7, S6, S5, S4,     // 11..15: loop 3
    S8, S9, S10, S11    // 16..19: tail
};

static const float test_reverse_loop_xfade[] =  {
    S0, S1, // 0..1: head
    xfl(S2, S9, 0.0f), xfl(S3, S8, 0.25f), xfl(S4, S7, 0.5f), xfl(S5, S6, 0.75f),   // 2..5: loop 1 start, crossfade to the head
    xfl(S5, S9, 0.0f), xfl(S4, S8, 0.25f), xfl(S3, S7, 0.5f), xfl(S2, S6, 0.75f),   // 6..9: loop 1-2 crossfade
    xfl(S5, S9, 0.0f), xfl(S4, S8, 0.25f), xfl(S3, S7, 0.5f), xfl(S2, S6, 0.75f),   // 10..13: loop 2-3 crossfade
    xfl(S5, S6, 0.0f), xfl(S4, S7, 0.25f), xfl(S3, S8, 0.5f), xfl(S2, S9, 0.75f),   // 14..17: loop 3-tail crossfade
    S10, S11 // 18..19: tail
};

static const float test_direct_full_pp_simple[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S4, S5, S6, S7,     // 4..7:   loop 1
    S7, S6, S5, S4,     // 8..11:  loop 2
    S4, S5, S6, S7,     // 12..15: loop 3
    S7, S6, S5, S4,     // 16..19: loop 4
    S8, S9, S10, S11    // 20..23: tail
};

static const float test_reverse_full_pp_simple[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S7, S6, S5, S4,     // 4..7:   loop 1
    S4, S5, S6, S7,     // 8..11:  loop 2
    S7, S6, S5, S4,     // 12..15: loop 3
    S4, S5, S6, S7,     // 16..19: loop 4
    S8, S9, S10, S11    // 20..23: tail
};

static const float test_direct_half_pp_simple1[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S4, S5, S6, S7,     // 4..7:   loop 1
    S7, S6, S5, S4,     // 8..11:  loop 2
    S4, S5, S6, S7,     // 12..15: loop 3
    S8, S9, S10, S11    // 16..19: tail
};

static const float test_direct_half_pp_simple2[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S4, S5, S6, S7,     // 4..7:   loop 1
    S7, S6, S5, S4,     // 8..11:  loop 2
    S4, S5, S6, S7,     // 12..15: loop 3
    S7, S6, S5, S4,     // 16..19: loop 4
    S8, S9, S10, S11    // 20..23: tail
};

static const float test_reverse_half_pp_simple1[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S7, S6, S5, S4,     // 4..7:   loop 1
    S4, S5, S6, S7,     // 8..11:  loop 2
    S7, S6, S5, S4,     // 12..15: loop 3
    S8, S9, S10, S11    // 16..19: tail
};

static const float test_reverse_half_pp_simple2[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S7, S6, S5, S4,     // 4..7:   loop 1
    S4, S5, S6, S7,     // 8..11:  loop 2
    S7, S6, S5, S4,     // 12..15: loop 3
    S4, S5, S6, S7,     // 16..19: loop 4
    S8, S9, S10, S11    // 20..23: tail
};

static const float test_direct_smart_pp_simple1[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S4, S5, S6, S7,     // 4..7:   loop 1
    S7, S6, S5, S4,     // 8..11:  loop 2
    S4, S5, S6, S7,     // 12..15: loop 3
    S8, S9, S10, S11    // 16..19: tail
};

static const float test_direct_smart_pp_simple2[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S4, S5, S6, S7,     // 4..7:   loop 1
    S7, S6, S5, S4,     // 8..11:  loop 2
    S4, S5, S6, S7,     // 12..15: loop 3
    S8, S9, S10, S11    // 16..19: tail
};

static const float test_direct_smart_pp_simple3[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S4, S5, S6, S7,     // 4..7:   loop 1
    S7, S6, S5, S4,     // 8..11:  loop 2
    S4, S5, S6, S7,     // 12..15: loop 3
    S7, S6, S5, S4,     // 16..19: loop 4
    S4, S5, S6, S7,     // 20..23: loop 5
    S8, S9, S10, S11    // 24..27: tail
};

static const float test_reverse_smart_pp_simple1[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S7, S6, S5, S4,     // 4..7:   loop 1
    S4, S5, S6, S7,     // 8..11:  loop 2
    S7, S6, S5, S4,     // 12..15: loop 3
    S4, S5, S6, S7,     // 16..19: loop 4
    S8, S9, S10, S11    // 20..23: tail
};

static const float test_reverse_smart_pp_simple2[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S7, S6, S5, S4,     // 4..7:   loop 1
    S4, S5, S6, S7,     // 8..11:  loop 2
    S7, S6, S5, S4,     // 12..15: loop 3
    S4, S5, S6, S7,     // 16..19: loop 4
    S8, S9, S10, S11    // 20..23: tail
};

static const float test_reverse_smart_pp_simple3[] =
{
    S0, S1, S2, S3,     // 0..3:   head
    S7, S6, S5, S4,     // 4..7:   loop 1
    S4, S5, S6, S7,     // 8..11:  loop 2
    S7, S6, S5, S4,     // 12..15: loop 3
    S4, S5, S6, S7,     // 16..19: loop 4
    S7, S6, S5, S4,     // 20..23: loop 5
    S4, S5, S6, S7,     // 24..27: loop 6
    S8, S9, S10, S11    // 28..31: tail
};

static const float test_direct_inside[] =
{
    S6, S7, S8, S9,                 // 0..3:   loop 1
    S2, S3, S4, S5, S6, S7, S8, S9, // 4..11:  loop 2
    S2, S3, S4, S5, S6, S7, S8, S9, // 12..19: loop 3
    S10, S11                        // 20..21: tail
};

static const float test_reverse_inside[] =
{
    S5, S4, S3, S2,                 // 0..3:   loop 1
    S9, S8, S7, S6, S5, S4, S3, S2, // 4..11:  loop 2
    S9, S8, S7, S6, S5, S4, S3, S2, // 12..19: loop 3
    S10, S11                        // 20..21: tail
};

static const float test_direct_tail[] =
{
    S8, S9, S10, S11                // 0..3:   tail
};

static const float test_reverse_tail[] =
{
    S8, S9, S10, S11                // 0..3:   tail
};

static const float test_playback_cancel1[] =
{
    S0, S1, S2, S3, S4, S5, S6, S7, S8,
    xfl(S9, 0.0f, 0.0f), xfl(S10, 0.0f, 0.25f), xfl(S11, 0.0f, 0.5f)
};

static const float test_playback_cancel2[] =
{
    S0, S1, S2, S3, S4,
    xfl(S5, 0.0f, 0.0f), xfl(S6, 0.0f, 0.25f), xfl(S7, 0.0f, 0.5f), xfl(S8, 0.0f, 0.75f)
};

static const float test_playback_cancel3[] =
{
    S0, S1, S2,
    xfl(S3, 0.0f, 0.0f), xfl(S4, 0.0f, 0.25f), xfl(S5, 0.0f, 0.5f), xfl(S6, 0.0f, 0.75f)
};

static const float test_playback_cancel_direct_loop1[] =
{
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 0..7: delay
    S0, S1,                         // 8..9:   head
    S2, S3, S4, S5, S6, S7, S8, S9, // 10..17: loop 1
    S2, S3, S4, S5, S6, S7,         // 18..23: loop 2
    xfl(S8, 0.0f, 0.0f), xfl(S9, 0.0f, 0.25f), xfl(S10, 0.0f, 0.5f), xfl(S11, 0.0f, 0.75f) // 24..27: loop 2 + tail
};

static const float test_playback_cancel_direct_loop2[] =
{
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 0..7: delay
    S0, S1,                         // 8..9:   head
    S2, S3, S4, S5, S6, S7, S8, S9, // 10..17: loop 1
    S2, S3, S4, S5, S6, S7,         // 18..23: loop 2
    xfl(S8, 0.0f, 0.0f), xfl(S9, 0.0f, 0.25f), xfl(S10, 0.0f, 0.5f), xfl(S11, 0.0f, 0.75f) // 24..27: loop 2 + tail
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
                dst.randomize(0.0f, 0.001f); // Add some noise to check that batches are applied in additive mode
                chk.copy(dst);

                // Obtain the copy of playback and validate state
                dspu::playback::playback_t xpb = *pb;
                UTEST_ASSERT(xpb.nTimestamp == 0);
                UTEST_ASSERT(xpb.nPosition == -1);

                size_t offset               = 0;
                size_t est_processed        = lsp_min(buf_size, real_buf_size);
                dsp::add2(chk.data(), buf_data, est_processed);

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

    void test_playback_without_cancel(dspu::Sample &s)
    {
        // Init playback
        dspu::playback::playback_t pb;
        dspu::playback::clear_playback(&pb);
        dspu::PlaySettings ps;

        // Initial settings
        ps.set_volume(1.0f);
        ps.set_channel(0, 0);

        // Test the simple playback
        printf("Testing playback of full sample without delay...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_NONE, 0, 0);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        test_playback(&pb, test_playback_no_delay, 12);

        // Test the simple playback with delay
        printf("Testing playback of full sample with short delay...\n");
        ps.set_delay(4);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_NONE, 0, 0);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        test_playback(&pb, test_playback_short_delay, 16);

        // Test the simple playback with delay
        printf("Testing playback of full sample with start position...\n");
        ps.set_delay(0);
        ps.set_start(4);
        ps.set_loop_range(dspu::SAMPLE_LOOP_NONE, 0, 0);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        test_playback(&pb, test_playback_with_start_position, 8);

        // Test simple direct loop without crossfade
        printf("Testing simple direct loop...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 14);
        test_playback(&pb, test_direct_loop_simple, 20);

        // Test simple direct loop with crossfade
        printf("Testing simple direct loop with crossfade...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT, 2, 10);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 4);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 16);
        test_playback(&pb, test_direct_loop_xfade, 20);

        // Test simple reverse loop without crossfade
        printf("Testing simple reverse loop...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 14);
        test_playback(&pb, test_reverse_loop_simple, 20);

        // Test simple reverse loop with crossfade
        printf("Testing simple reverse loop with crossfade...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE, 2, 10);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 4);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 16);
        test_playback(&pb, test_reverse_loop_xfade, 20);

        // Test simple direct full ping-pong loop without crossfade
        printf("Testing simple direct full ping-pong loop...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT_FULL_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 14);
        test_playback(&pb, test_direct_full_pp_simple, 24);

        // Test simple reverse full ping-pong loop without crossfade
        printf("Testing simple reverse full ping-pong loop...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE_FULL_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 14);
        test_playback(&pb, test_reverse_full_pp_simple, 24);

        // Test simple direct half ping-pong loop without crossfade (version 1)
        printf("Testing simple direct half ping-pong loop (version 1)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT_HALF_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 14);
        test_playback(&pb, test_direct_half_pp_simple1, 20);

        // Test simple direct half ping-pong loop without crossfade (version 2)
        printf("Testing simple direct half ping-pong loop (version 2)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT_HALF_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 18);
        test_playback(&pb, test_direct_half_pp_simple2, 24);

        // Test simple reverse half ping-pong loop without crossfade (version 1)
        printf("Testing simple reverse half ping-pong loop...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE_HALF_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 14);
        test_playback(&pb, test_reverse_half_pp_simple1, 20);

        // Test simple reverse half ping-pong loop without crossfade (version 2)
        printf("Testing simple reverse half ping-pong loop...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE_HALF_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 18);
        test_playback(&pb, test_reverse_half_pp_simple2, 24);

        // Test simple direct half ping-pong loop without crossfade (version 1)
        printf("Testing simple direct smart ping-pong loop (version 1)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT_SMART_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 10);
        test_playback(&pb, test_direct_smart_pp_simple1, 20);

        // Test simple direct half ping-pong loop without crossfade (version 2)
        printf("Testing simple direct smart ping-pong loop (version 2)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT_SMART_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 14);
        test_playback(&pb, test_direct_smart_pp_simple2, 20);

        // Test simple direct half ping-pong loop without crossfade (version 2)
        printf("Testing simple direct smart ping-pong loop (version 3)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT_SMART_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 18);
        test_playback(&pb, test_direct_smart_pp_simple3, 28);

        // Test simple direct half ping-pong loop without crossfade (version 1)
        printf("Testing simple reverse smart ping-pong loop (version 1)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE_SMART_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 14);
        test_playback(&pb, test_reverse_smart_pp_simple1, 24);

        // Test simple direct half ping-pong loop without crossfade (version 2)
        printf("Testing simple reverse smart ping-pong loop (version 2)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE_SMART_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 18);
        test_playback(&pb, test_reverse_smart_pp_simple2, 24);

        // Test simple direct half ping-pong loop without crossfade (version 2)
        printf("Testing simple reverse smart ping-pong loop (version 3)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE_SMART_PP, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 22);
        test_playback(&pb, test_reverse_smart_pp_simple3, 32);

        // Test direct playback started inside of loop
        printf("Testing simple direct playback started inside of loop...\n");
        ps.set_delay(0);
        ps.set_start(6);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT, 2, 10);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 16);
        test_playback(&pb, test_direct_inside, 22);

        // Test reverse playback started inside of loop
        printf("Testing simple reverse playback started inside of loop...\n");
        ps.set_delay(0);
        ps.set_start(6);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE, 2, 10);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 16);
        test_playback(&pb, test_reverse_inside, 22);

        // Test direct playback started inside of loop
        printf("Testing simple direct playback started at the tail...\n");
        ps.set_delay(0);
        ps.set_start(8);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 16);
        test_playback(&pb, test_direct_tail, 4);

        // Test reverse playback started inside of loop
        printf("Testing simple reverse playback started at the tail...\n");
        ps.set_delay(0);
        ps.set_start(8);
        ps.set_loop_range(dspu::SAMPLE_LOOP_REVERSE, 4, 8);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::stop_playback(&pb, 16);
        test_playback(&pb, test_reverse_tail, 4);
    }

    void test_playback_cancel(const dspu::playback::playback_t *pb, size_t time, const float *buf_data, size_t buf_size)
    {
        for (size_t real_buf_size = buf_size / 2; real_buf_size <= buf_size * 2; real_buf_size += 2)
        {
            printf("  testing playback: real_buf_size=%d\n", int(real_buf_size));

            FloatBuffer dst(real_buf_size);
            FloatBuffer buf(1);
            FloatBuffer chk(real_buf_size);
            dst.randomize(0.0f, 0.001f); // Add some noise to check that batches are applied in additive mode
            chk.copy(dst);

            // Obtain the copy of playback and validate state
            dspu::playback::playback_t xpb = *pb;
            UTEST_ASSERT(xpb.nTimestamp == 0);
            UTEST_ASSERT(xpb.nPosition == -1);

            size_t offset               = 0;
            size_t est_processed        = lsp_min(buf_size, real_buf_size);
            dsp::add2(chk.data(), buf_data, est_processed);

            // Do the processing
            while (offset < real_buf_size)
            {
                dsp::fill_zero(buf.data(), 1);

                if (offset == time)
                    dspu::playback::cancel_playback(&xpb, 4, 0);

                size_t done     = dspu::playback::process_playback(buf.data(), &xpb, 1);
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
        } // real_buf_size
    };

    void test_playback_cancel(dspu::Sample &s)
    {
        // Init playback
        dspu::playback::playback_t pb;
        dspu::playback::clear_playback(&pb);
        dspu::PlaySettings ps;

        // Initial settings
        ps.set_volume(1.0f);
        ps.set_channel(0, 0);

        // Test the simple playback
        printf("Testing playback of full sample with cancel (version 1)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_NONE, 0, 0);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        test_playback_cancel(&pb, 9, test_playback_cancel1, 12);

        // Test the simple playback
        printf("Testing playback of full sample with cancel (version 2)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_NONE, 0, 0);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        test_playback_cancel(&pb, 5, test_playback_cancel2, 9);

        // Test the simple playback
        printf("Testing playback of full sample with cancel (version 3)...\n");
        ps.set_delay(0);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_NONE, 0, 0);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        test_playback_cancel(&pb, 3, test_playback_cancel3, 7);

        // Test the simple playback
        printf("Testing playback of direct loop with delay and cancel (version 1)...\n");
        ps.set_delay(8);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT, 2, 10);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        test_playback_cancel(&pb, 24, test_playback_cancel_direct_loop1, 28);

        // The same test but with another block sizes
        printf("Testing playback of direct loop with delay and cancel (version 2)...\n");
        ps.set_delay(8);
        ps.set_start(0);
        ps.set_loop_range(dspu::SAMPLE_LOOP_DIRECT, 2, 10);
        ps.set_loop_xfade(dspu::SAMPLE_CROSSFADE_LINEAR, 0);
        dspu::playback::start_playback(&pb, &s, &ps);
        dspu::playback::cancel_playback(&pb, 4, 24);
        test_playback(&pb, test_playback_cancel_direct_loop2, 28);
    }

    UTEST_MAIN
    {
        // Init sample
        dspu::Sample s;
        size_t sample_len = sizeof(sample_data)/sizeof(sample_data[0]);
        UTEST_ASSERT(s.init(1, sample_len, sample_len));
        dsp::copy(s.channel(0), sample_data, sample_len);

        // Test different cases without the cancel
        test_playback_without_cancel(s);

        // Test the case with cancellation
        test_playback_cancel(s);
    }
UTEST_END;



