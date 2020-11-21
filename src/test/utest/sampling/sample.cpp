/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 21 нояб. 2020 г.
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

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/stdlib/math.h>

#define TEST_SRATE      48000
#define TONE_RATE       440.0f

UTEST_BEGIN("dspu.sampling", sample)
    void init_sample(dspu::Sample *s)
    {
        UTEST_ASSERT(s->init(2, TEST_SRATE, TEST_SRATE));
        s->set_sample_rate(TEST_SRATE);

        float *c0 = s->channel(0);
        float *c1 = s->channel(1);
        float w = 2.0f * M_PI * TONE_RATE / float(TEST_SRATE);
        float k = 1.0f / (TEST_SRATE - 1);

        for (size_t i=0; i<TEST_SRATE; ++i)
        {
            c0[i] = sinf(w * i);
            c1[i] = i * k;
        }
    }

    void test_copy()
    {
        printf("Testing sample copy...\n");

        dspu::Sample s, c;

        UTEST_ASSERT(c.copy(&s) == STATUS_BAD_STATE);

        init_sample(&s);

        UTEST_ASSERT(c.copy(&s) == STATUS_OK);

        // Check the sample
        for (size_t i=0; i<s.channels(); ++i)
        {
            float *s0 = s.channel(i);
            float *s1 = c.channel(i);

            for (size_t j=0; j<s.length(); ++j)
            {
                if (!float_equals_absolute(s0[j], s1[j]))
                {
                    eprintf("Failed sample check at sample %d, channel %d: s0=%f, s1=%f\n",
                            int(j), int(i), s0[j], s1[j]);
                    UTEST_FAIL();
                }
            }
        }
    }

    void test_io()
    {
        printf("Testing save & load for Sample...\n");

        dspu::Sample s, l;
        init_sample(&s);

        io::Path path;
        UTEST_ASSERT(path.fmt("%s/%s-io-test.wav", tempdir(), full_name()) > 0);
        printf("Saving sample to '%s'\n", path.as_utf8());
        UTEST_ASSERT(s.save(&path) == TEST_SRATE);

        printf("Loading sample from '%s'\n", path.as_utf8());
        UTEST_ASSERT(l.load(&path) == STATUS_OK);
        UTEST_ASSERT(l.channels() == s.channels());
        UTEST_ASSERT(l.sample_rate() == s.sample_rate());
        UTEST_ASSERT(l.length() == s.length());

        // Check the sample
        for (size_t i=0; i<s.channels(); ++i)
        {
            float *s0 = s.channel(i);
            float *s1 = l.channel(i);

            for (size_t j=0; j<s.length(); ++j)
            {
                if (!float_equals_absolute(s0[j], s1[j]))
                {
                    eprintf("Failed sample check at sample %d, channel %d: s0=%f, s1=%f\n",
                            int(j), int(i), s0[j], s1[j]);
                    UTEST_FAIL();
                }
            }
        }
    }

    void test_resample(size_t srate)
    {
        printf("Testing resample with sample rate %d...\n", int(srate));

        dspu::Sample o, s, l;
        init_sample(&o);

        printf("Copying sample...\n");
        UTEST_ASSERT(s.copy(&o) == STATUS_OK);

        io::Path path;
        UTEST_ASSERT(path.fmt("%s/%s-resample-%d.wav", tempdir(), full_name(), int(srate)) > 0);
        printf("Resampling %d->%d to file '%s'\n", int(TEST_SRATE), srate, path.as_utf8());
        UTEST_ASSERT(s.resample(srate) == STATUS_OK);
        UTEST_ASSERT(s.save(&path) >= ssize_t(srate));

        printf("Loading sample from '%s'\n", path.as_utf8());
        UTEST_ASSERT(l.load(&path) == STATUS_OK);
        UTEST_ASSERT(l.channels() == s.channels());
        UTEST_ASSERT(l.sample_rate() == srate);
    }

    UTEST_MAIN
    {
        test_copy();
        test_io();
        test_resample(TEST_SRATE);
        test_resample(TEST_SRATE / 2);
        test_resample(TEST_SRATE * 2);
        test_resample(44100);
        test_resample(88200);
    }
UTEST_END


