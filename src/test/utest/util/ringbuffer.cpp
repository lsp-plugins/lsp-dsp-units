/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 2 июн. 2023 г.
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

#include <lsp-plug.in/dsp-units/util/RingBuffer.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/helpers.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>

UTEST_BEGIN("dspu.util", ringbuffer)

    UTEST_MAIN
    {
        dspu::RingBuffer rb;
        FloatBuffer dst(16);

        UTEST_ASSERT(rb.init(8));
        UTEST_ASSERT(rb.size() == 8);

        // Append single samples
        rb.append(1.0f);
        rb.append(2.0f);
        rb.append(3.0f);
        rb.append(4.0f);

        UTEST_ASSERT(float_equals_adaptive(rb.get(8), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(7), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(6), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(5), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(4), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(3), 1.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(2), 2.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(1), 3.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(0), 4.0f));

        // Append small buffer
        static const float buf1[2] = { 5.0f, 6.0f };
        UTEST_ASSERT(rb.append(buf1, 2) == 2);

        UTEST_ASSERT(float_equals_adaptive(rb.get(8), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(7), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(6), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(5), 1.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(4), 2.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(3), 3.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(2), 4.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(1), 5.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(0), 6.0f));

        dst.randomize();
        UTEST_ASSERT(rb.get(dst, 9, 10) == 8);
        UTEST_ASSERT(!dst.corrupted());

        UTEST_ASSERT(float_equals_adaptive(dst[0], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[1], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[2], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[3], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[4], 1.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[5], 2.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[6], 3.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[7], 4.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[8], 5.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[9], 6.0f));

        // Append another small buffer
        static const float buf2[4] = { 7.0f, 8.0f, 9.0f, 10.0f };
        UTEST_ASSERT(rb.append(buf2, 4) == 4);

        UTEST_ASSERT(float_equals_adaptive(rb.get(8), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(7), 3.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(6), 4.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(5), 5.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(4), 6.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(3), 7.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(2), 8.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(1), 9.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(0), 10.0f));

        dst.randomize();
        UTEST_ASSERT(rb.get(dst, 7, 10) == 8);
        UTEST_ASSERT(!dst.corrupted());

        UTEST_ASSERT(float_equals_adaptive(dst[0], 3.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[1], 4.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[2], 5.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[3], 6.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[4], 7.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[5], 8.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[6], 9.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[7], 10.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[8], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[9], 0.0f));

        // Append large buffer
        static const float buf3[12] = { -1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f, -7.0f, -8.0f, -9.0f, -10.0f, -11.0f, -12.0f };
        UTEST_ASSERT(rb.append(buf3, 12) == 8);

        UTEST_ASSERT(float_equals_adaptive(rb.get(8), 0.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(7), -5.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(6), -6.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(5), -7.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(4), -8.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(3), -9.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(2), -10.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(1), -11.0f));
        UTEST_ASSERT(float_equals_adaptive(rb.get(0), -12.0f));

        // Test reads
        dst.randomize();
        UTEST_ASSERT(rb.get(dst, 16, 8) == 0);
        UTEST_ASSERT(!dst.corrupted());

        UTEST_ASSERT(float_equals_adaptive(dst[0], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[1], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[2], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[3], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[4], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[5], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[6], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[7], 0.0f));

        dst.randomize();
        UTEST_ASSERT(rb.get(dst, 7, 8) == 8);
        UTEST_ASSERT(!dst.corrupted());

        UTEST_ASSERT(float_equals_adaptive(dst[0], -5.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[1], -6.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[2], -7.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[3], -8.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[4], -9.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[5], -10.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[6], -11.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[7], -12.0f));

        dst.randomize();
        UTEST_ASSERT(rb.get(dst, 12, 16) == 8);
        UTEST_ASSERT(!dst.corrupted());

        UTEST_ASSERT(float_equals_adaptive(dst[0], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[1], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[2], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[3], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[4], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[5], -5.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[6], -6.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[7], -7.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[8], -8.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[9], -9.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[10], -10.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[11], -11.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[12], -12.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[13], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[14], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[15], 0.0f));

        dst.randomize();
        UTEST_ASSERT(rb.get(&dst[0], 8, 2) == 1);
        UTEST_ASSERT(rb.get(&dst[2], 6, 2) == 2);
        UTEST_ASSERT(rb.get(&dst[4], 4, 2) == 2);
        UTEST_ASSERT(rb.get(&dst[6], 2, 2) == 2);
        UTEST_ASSERT(rb.get(&dst[8], 0, 2) == 1);
        UTEST_ASSERT(!dst.corrupted());

        UTEST_ASSERT(float_equals_adaptive(dst[0], 0.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[1], -5.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[2], -6.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[3], -7.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[4], -8.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[5], -9.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[6], -10.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[7], -11.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[8], -12.0f));
        UTEST_ASSERT(float_equals_adaptive(dst[9], 0.0f));
    }

UTEST_END


