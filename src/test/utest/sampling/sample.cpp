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
    UTEST_TIMELIMIT(30)

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
            c0[i] = 0.5f * sinf(w * i);
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

    void test_stretch()
    {
        printf("Testing sample stretch...\n");

        static const dspu::sample_crossfade_t crossfades[] =
        {
            dspu::SAMPLE_CROSSFADE_LINEAR,
            dspu::SAMPLE_CROSSFADE_CONST_POWER
        };

        static const char *names[] =
        {
            "linear",
            "const-power"
        };

        io::Path path;
        dspu::Sample s, ss;
        init_sample(&s);

        for (size_t i=0, n=sizeof(crossfades)/sizeof(dspu::sample_crossfade_t); i<n; ++i)
        {
            // Test invalid cases
            printf("Testing invalid cases for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            // Region with negative length
            UTEST_ASSERT(ss.stretch(256, 1024, crossfades[i], 0.5f, TEST_SRATE/2, TEST_SRATE/2 - 1024) == STATUS_BAD_ARGUMENTS);
            // Region with invalid start position
            UTEST_ASSERT(ss.stretch(256, 1024, crossfades[i], 0.5f, s.length() + 1, s.length() + 1024) == STATUS_BAD_ARGUMENTS);
            // Region with invalid end position
            UTEST_ASSERT(ss.stretch(256, 1024, crossfades[i], 0.5f, 0, s.length() + 1024) == STATUS_BAD_ARGUMENTS);
            // Chunk size of zero length
            UTEST_ASSERT(ss.stretch(256, 0, crossfades[i], 0.5f, TEST_SRATE/2, TEST_SRATE/2 + 1024) == STATUS_BAD_ARGUMENTS);

            // Testing simple stretch
            printf("Testing simple stretch of 0 sample region for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            UTEST_ASSERT(ss.stretch(256, 1024, crossfades[i], 0.5f, TEST_SRATE/2, TEST_SRATE/2) == STATUS_OK);
            UTEST_ASSERT(ss.length() == s.length() + 256);
            UTEST_ASSERT(path.fmt("%s/%s-stretch-simple-0-%s.wav", tempdir(), full_name(), names[i]) > 0);
            printf("Saving sample to '%s'\n", path.as_utf8());
            UTEST_ASSERT(ss.save(&path) == ssize_t(ss.length()));

            // Testing simple stretch
            printf("Testing simple stretch of 1 sample region for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            UTEST_ASSERT(ss.stretch(256, 1024, crossfades[i], 0.5f, TEST_SRATE/2 + 72, TEST_SRATE/2 + 73) == STATUS_OK);
            UTEST_ASSERT(ss.length() == s.length() + 256 - 1);
            UTEST_ASSERT(path.fmt("%s/%s-stretch-simple-1-%s.wav", tempdir(), full_name(), names[i]) > 0);
            printf("Saving sample to '%s'\n", path.as_utf8());
            UTEST_ASSERT(ss.save(&path) == ssize_t(ss.length()));

            // Testing single cross-fade stretch
            printf("Testing single cross-fade stretch for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            UTEST_ASSERT(ss.stretch(3072, 2048, crossfades[i], 0.25f, TEST_SRATE/2, TEST_SRATE/2 + 8192) == STATUS_OK);
            UTEST_ASSERT(ss.length() == s.length() + 3072 - 8192);
            UTEST_ASSERT(path.fmt("%s/%s-stretch-single-cross-fade-%s.wav", tempdir(), full_name(), names[i]) > 0);
            printf("Saving sample to '%s'\n", path.as_utf8());
            UTEST_ASSERT(ss.save(&path) == ssize_t(ss.length()));

            // Testing short-region stretch
            printf("Testing simple short-region stretch for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            UTEST_ASSERT(ss.stretch(1630, 2048, crossfades[i], 0.25f, TEST_SRATE/2, TEST_SRATE/2 + 64) == STATUS_OK);
            UTEST_ASSERT(ss.length() == s.length() + 1630 - 64);
            UTEST_ASSERT(path.fmt("%s/%s-stretch-short-region-%s.wav", tempdir(), full_name(), names[i]) > 0);
            printf("Saving sample to '%s'\n", path.as_utf8());
            UTEST_ASSERT(ss.save(&path) == ssize_t(ss.length()));

            printf("Testing simple short-region stretch 2 for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            UTEST_ASSERT(ss.stretch(1630, 2048, crossfades[i], 1.0f, TEST_SRATE/2, TEST_SRATE/2 + 64) == STATUS_OK);
            UTEST_ASSERT(ss.length() == s.length() + 1630 - 64);
            UTEST_ASSERT(path.fmt("%s/%s-stretch-short-region-2-%s.wav", tempdir(), full_name(), names[i]) > 0);
            printf("Saving sample to '%s'\n", path.as_utf8());
            UTEST_ASSERT(ss.save(&path) == ssize_t(ss.length()));

            // Testing common widening stretch
            printf("Testing common widening stretch for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            UTEST_ASSERT(ss.stretch(16200, 1024, crossfades[i], 0.25f, TEST_SRATE/2, TEST_SRATE/2 + 4000) == STATUS_OK);
            UTEST_ASSERT(ss.length() == s.length() + 16200 - 4000);
            UTEST_ASSERT(path.fmt("%s/%s-stretch-widening-%s.wav", tempdir(), full_name(), names[i]) > 0);
            printf("Saving sample to '%s'\n", path.as_utf8());
            UTEST_ASSERT(ss.save(&path) == ssize_t(ss.length()));

            // Testing common shortening stretch
            printf("Testing common shortening stretch for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            UTEST_ASSERT(ss.stretch(4200, 1024, crossfades[i], 0.25f, TEST_SRATE/2, TEST_SRATE/2 + 16000) == STATUS_OK);
            UTEST_ASSERT(ss.length() == s.length() + 4200 - 16000);
            UTEST_ASSERT(path.fmt("%s/%s-stretch-shortening-%s.wav", tempdir(), full_name(), names[i]) > 0);
            printf("Saving sample to '%s'\n", path.as_utf8());
            UTEST_ASSERT(ss.save(&path) == ssize_t(ss.length()));

            // Testing extreme case: cut-off part of sample
            printf("Testing sample cutoff for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            UTEST_ASSERT(ss.stretch(0, 1024, crossfades[i], 0.25f, TEST_SRATE/2, TEST_SRATE/2 + 16000) == STATUS_OK);
            UTEST_ASSERT(ss.length() == s.length() + 0 - 16000);
            UTEST_ASSERT(path.fmt("%s/%s-stretch-cutoff-%s.wav", tempdir(), full_name(), names[i]) > 0);
            printf("Saving sample to '%s'\n", path.as_utf8());
            UTEST_ASSERT(ss.save(&path) == ssize_t(ss.length()));

            // Testing extreme case: very short region to stretch
            printf("Testing short region for %s fade...\n", names[i]);
            UTEST_ASSERT(ss.copy(s) == STATUS_OK);
            UTEST_ASSERT(ss.stretch(6200, 1024, crossfades[i], 1.0f, TEST_SRATE/2, TEST_SRATE/2 + 2) == STATUS_OK);
            UTEST_ASSERT(ss.length() == s.length() + 6200 - 2);
            UTEST_ASSERT(path.fmt("%s/%s-stretch-short-region-%s.wav", tempdir(), full_name(), names[i]) > 0);
            printf("Saving sample to '%s'\n", path.as_utf8());
            UTEST_ASSERT(ss.save(&path) == ssize_t(ss.length()));
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
        ssize_t saved = s.save(&path);
        printf("Saved frames: %d\n", int(saved));
        UTEST_ASSERT(saved >= ssize_t(srate));

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
        test_stretch();
    }
UTEST_END


