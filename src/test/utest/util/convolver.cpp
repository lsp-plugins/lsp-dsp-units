/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 8 сент. 2018 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>
#include <lsp-plug.in/dsp-units/util/Convolver.h>
#include <lsp-plug.in/dsp/dsp.h>

#define CCONV_SIZE      0x2000
#define LCONV_SIZE      0x10000
#define CONV_SIZE       0x2000
#define SRC_SIZE        0x2000
#define SRC2_SIZE       0x20
#define PRECISION_A     2e-4f
#define PRECISION_B     2e-5f

static void convolve(float *dst, const float *src, const float *conv, size_t length, size_t count)
{
    for (size_t i=0; i<count; ++i)
    {
        float k = src[i];
        for (size_t j=0; j<length; ++j)
            dst[i+j] += k * conv[j];
    }
}

UTEST_BEGIN("dspu.util", convolver)
    void convolve(dspu::Convolver &conv, float *dst, const float *src, size_t count, size_t step)
    {
        for (size_t i=0; i<count;)
        {
            const size_t todo = lsp_min(count - i, step);
            conv.process(&dst[i], &src[i], todo);
            i += todo;
        }
    }

    void convolve_full(dspu::Convolver &conv, float *dst, const float *src, size_t count, size_t step)
    {
        for (size_t i=0; i<count;)
        {
            const size_t todo = lsp_min(count - i, step);
            conv.process(dst, src, todo);

            dst += todo;
            src += todo;
            i   += todo;
        }

        // Allocate empty data for buffer
        count = conv.data_size() - 1;
        float *buf = reinterpret_cast<float *>(::malloc(step * sizeof(float)));
        dsp::fill_zero(buf, step);

        for (size_t i=0; i<count;)
        {
            size_t todo = count - i;
            if (todo > step)
                todo = step;
            conv.process(dst, buf, todo);

            dst += todo;
            i   += todo;
        }

        ::free(buf);
    }

    void test_small()
    {
        dspu::Convolver c;

        FloatBuffer conv(0x1f);
        FloatBuffer src1(SRC_SIZE + conv.size());
        FloatBuffer src2(src1.size());
        FloatBuffer src3(src2.size());
        FloatBuffer dst1(src1.size());
        FloatBuffer dst2(dst1);
        FloatBuffer dst3(dst1);

        printf("Testing small convolution...\n");

        // Initialize data
        for (size_t i=0; i<conv.size(); ++i)
            conv[i] = i + 1;
        src1.fill_zero();
        for (size_t i=0, j=0; i<SRC_SIZE; i+=5, ++j)
            src1[i] = ((j % 3) == 0) ? 1.0f :
                     ((j % 3) == 1) ? 0.1f : 0.01f;
        src2.copy(src1);
        src3.copy(src1);

        dst1.fill_zero();
        dst2.fill_zero();
        dst3.fill_zero();

        UTEST_ASSERT(c.init(conv, conv.size(), 9, 0));
        ::convolve(dst1, src1, conv, conv.size(), SRC_SIZE);
        dsp::convolve(dst2, src2, conv, conv.size(), SRC_SIZE);
        convolve(c, dst3, src3, src3.size(), 31);

        UTEST_ASSERT_MSG(src1.valid(), "Source buffer 1 corrupted");
        UTEST_ASSERT_MSG(src2.valid(), "Source buffer 2 corrupted");
        UTEST_ASSERT_MSG(src3.valid(), "Source buffer 3 corrupted");
        UTEST_ASSERT_MSG(conv.valid(), "Convolution 1 buffer corrupted");
        UTEST_ASSERT_MSG(dst1.valid(), "Destination buffer 1 corrupted");
        UTEST_ASSERT_MSG(dst2.valid(), "Destination buffer 2 corrupted");
        UTEST_ASSERT_MSG(dst3.valid(), "Destination buffer 3 corrupted");

        if (!src2.equals_relative(src1, PRECISION_A))
        {
            src1.dump("src1");
            src2.dump("src2");

            const size_t index = src2.last_diff();
            UTEST_FAIL_MSG(
                "Malformed input data src2, started at sample=%d: %.5f, expected %.5f",
                int(index), src2[index], src1[index]);
        }
        if (!src3.equals_relative(src1, PRECISION_A))
        {
            src1.dump("src1");
            src3.dump("src3");

            const size_t index = src3.last_diff();
            UTEST_FAIL_MSG(
                "Malformed input data src3, started at sample=%d: %.5f, expected %.5f",
                int(index), src3[index], src1[index]);
        }

        if (!dst2.equals_relative(dst1, PRECISION_A))
        {
            const size_t index = dst2.last_diff();
            printf(
                "Output of convolver dst2 is invalid, started at sample=%d: %.5f, expected %.5f\n",
                int(index), dst2[index], dst1[index]);

            src1.dump("src1");
            src2.dump("src2");
            src3.dump("src3");
            conv.dump("conv");
            dst1.dump("dst1");
            dst2.dump("dst2");
            dst3.dump("dst3");
            UTEST_FAIL_MSG(
                "Output of convolver dst2 is invalid, started at sample=%d: %.5f, expected %.5f",
                int(index), dst2[index], dst1[index]);
        }
        if (!dst3.equals_relative(dst1, PRECISION_A))
        {
            const size_t index = dst3.last_diff();
            printf(
                "Output of convolver dst3 is invalid, started at sample=%d: %.5f, expected %.5f\n",
                int(index), dst3[index], dst1[index]);

            src1.dump("src1");
            src2.dump("src2");
            src3.dump("src3");
            conv.dump("conv");
            dst1.dump("dst1");
            dst2.dump("dst2");
            dst3.dump("dst3");
            UTEST_FAIL_MSG(
                "Output of convolver dst3 is invalid, started at sample=%d: %.5f, expected %.5f",
                int(index), dst3[index], dst1[index]);
        }

        c.destroy();
    }

    void test_collisions()
    {
        dspu::Convolver c;
        FloatBuffer conv(CCONV_SIZE);
        FloatBuffer src1(CCONV_SIZE);
        FloatBuffer src2(src1.size());
        FloatBuffer dst1(CCONV_SIZE * 2);
        FloatBuffer dst2(CCONV_SIZE * 2);

        conv.randomize(-1.0f, 1.0f);

        for (size_t i=1; i<CCONV_SIZE; i += 1023)
        {
            printf("Testing simple convolution i=%d...\n", i);

            UTEST_ASSERT(c.init(conv, conv.size(), 10, 0));

            src1.fill_zero();
            dst1.fill_zero();
            dst2.fill_zero();
            src1[0] = 1.0f;
            src1[i] = 1.0f;
            src2.copy(src1);

            ::convolve(dst1, src1, conv, conv.size(), src1.size());
            convolve_full(c, dst2, src2, src2.size(), 127);

            UTEST_ASSERT_MSG(src1.valid(), "Source buffer 1 corrupted");
            UTEST_ASSERT_MSG(src2.valid(), "Source buffer 2 corrupted");
            UTEST_ASSERT_MSG(conv.valid(), "Convolution 1 buffer corrupted");
            UTEST_ASSERT_MSG(dst1.valid(), "Destination buffer 1 corrupted");
            UTEST_ASSERT_MSG(dst2.valid(), "Destination buffer 2 corrupted");

            c.destroy();

            if (!src2.equals_relative(src1, PRECISION_A))
            {
                src1.dump("src1");
                src2.dump("src2");

                const size_t index = src2.last_diff();
                UTEST_FAIL_MSG(
                    "Malformed input data src2, started at sample=%d: %.5f, expected %.5f",
                    int(index), src2[index], src1[index]);
            }

            if (!dst2.equals_absolute(dst1, PRECISION_B))
            {
                const size_t index = dst2.last_diff();
                printf(
                    "Output of convolver is invalid, started at sample=%d: dst1[i]=%.8f vs dst2[i]=%.8f\n",
                    int(index), dst1[index], dst2[index]);

                src1.dump("src1");
                src2.dump("src2");
                conv.dump("conv");
                dst1.dump("dst1");
                dst2.dump("dst2");
                UTEST_FAIL_MSG(
                    "Output of convolver is invalid, started at sample=%d: dst1[i]=%.8f vs dst2[i]=%.8f",
                    int(index), dst1[index], dst2[index]);
            }

        }
    }

    void test_large()
    {
        dspu::Convolver c;

        FloatBuffer conv(CONV_SIZE);
        FloatBuffer src1(SRC2_SIZE + conv.size());
        FloatBuffer src2(src1.size());
        FloatBuffer src3(src1.size());
        FloatBuffer dst1(src1.size());
        FloatBuffer dst2(dst1);
        FloatBuffer dst3(dst1);
        dsp::fill_zero(src1.data(SRC2_SIZE), src1.size() - SRC2_SIZE);
        src2.copy(src1);
        src3.copy(src1);

        printf("Testing large convolution...\n");

        dst1.fill_zero();
        dst2.fill_zero();
        dst3.fill_zero();

        UTEST_ASSERT(c.init(conv, conv.size(), 10, 0));
        ::convolve(dst1, src1, conv, conv.size(), SRC2_SIZE);
        dsp::convolve(dst2, src2, conv, conv.size(), SRC2_SIZE);
        convolve(c, dst3, src3, src3.size(), 31);

        UTEST_ASSERT_MSG(src1.valid(), "Source buffer 1 corrupted");
        UTEST_ASSERT_MSG(src2.valid(), "Source buffer 2 corrupted");
        UTEST_ASSERT_MSG(src3.valid(), "Source buffer 3 corrupted");
        UTEST_ASSERT_MSG(conv.valid(), "Convolution 1 buffer corrupted");
        UTEST_ASSERT_MSG(dst1.valid(), "Destination buffer 1 corrupted");
        UTEST_ASSERT_MSG(dst2.valid(), "Destination buffer 2 corrupted");
        UTEST_ASSERT_MSG(dst3.valid(), "Destination buffer 3 corrupted");

        if (!src2.equals_relative(src1, PRECISION_A))
        {
            src1.dump("src1");
            src2.dump("src2");

            const size_t index = src2.last_diff();
            UTEST_FAIL_MSG(
                "Malformed input data src2, started at sample=%d: %.5f, expected %.5f",
                int(index), src2[index], src1[index]);
        }
        if (!src3.equals_relative(src1, PRECISION_A))
        {
            src1.dump("src1");
            src3.dump("src3");

            const size_t index = src3.last_diff();
            UTEST_FAIL_MSG(
                "Malformed input data src3, started at sample=%d: %.5f, expected %.5f",
                int(index), src3[index], src1[index]);
        }

        if (!dst2.equals_relative(dst1, PRECISION_A))
        {
            const size_t index = dst2.last_diff();
            printf(
                "Output of convolver dst2 is invalid, started at sample=%d: %.5f, expected %.5f\n",
                int(index), dst2[index], dst1[index]);

            src1.dump("src1");
            src2.dump("src2");
            src3.dump("src3");
            conv.dump("conv");
            dst1.dump("dst1");
            dst2.dump("dst2");
            dst3.dump("dst3");

            UTEST_FAIL_MSG(
                "Output of convolver dst2 is invalid, started at sample=%d: %.5f, expected %.5f",
                int(index), dst2[index], dst1[index]);
        }
        if (!dst3.equals_relative(dst1, PRECISION_A))
        {
            const size_t index = dst3.last_diff();
            printf(
                "Output of convolver dst3 is invalid, started at sample=%d: %.5f, expected %.5f\n",
                int(index), dst3[index], dst1[index]);

            src1.dump("src1");
            src2.dump("src2");
            src3.dump("src3");
            conv.dump("conv");
            dst1.dump("dst1");
            dst2.dump("dst2");
            dst3.dump("dst3");

            UTEST_FAIL_MSG(
                "Output of convolver dst3 is invalid, started at sample=%d: %.5f, expected %.5f",
                int(index), dst3[index], dst1[index]);
        }

        c.destroy();
    }

    UTEST_MAIN
    {
        test_small();
        test_large();
        test_collisions();
    }
UTEST_END;


