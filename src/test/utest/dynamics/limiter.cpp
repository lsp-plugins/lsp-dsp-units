/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 12 февр. 2020 г.
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
#include <lsp-plug.in/test-fw/helpers.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>
#include <lsp-plug.in/dsp-units/dynamics/Limiter.h>
#include <lsp-plug.in/dsp-units/util/Delay.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/io/OutSequence.h>
#include <lsp-plug.in/dsp/dsp.h>

#define SRATE       48000
#define BUF_SIZE    4096
#define OVERSAMPLE  4

UTEST_BEGIN("dspu.dynamics", limiter)

    void test_triangle_peak()
    {
        FloatBuffer in(BUF_SIZE);
        FloatBuffer out(BUF_SIZE);
        FloatBuffer gain(BUF_SIZE);

        // Prepare buffer
        in.fill_zero();
        {
            float s = 0.0f, step = 0.05f;
            int i=0;
            for ( ; s < 0.999f; s += step)
                in[i++] = s;
            for ( ; s > 0.001f; s -= step)
                in[i++] = s;
        }

        // Initialize limiter
        dspu::Limiter l;
        dspu::Delay d;

        UTEST_ASSERT(l.init(SRATE * OVERSAMPLE, 20.0f));
        UTEST_ASSERT(d.init(20.0f * SRATE * 4));

        l.set_sample_rate(SRATE);
        l.set_mode(dspu::LM_HERM_THIN);
        l.set_knee(1.0f);
        l.set_threshold(0.5f, true);
        l.set_attack(1.5);
        l.set_release(1.5);
        l.set_lookahead(5); // 5 ms lookahead
        UTEST_ASSERT(l.modified());
        l.update_settings();

        ssize_t latency = l.get_latency();
        UTEST_ASSERT(latency == ssize_t(5.0f * SRATE * 0.001f));
        d.set_delay(latency);

        l.process(gain, in, BUF_SIZE);
        d.process(out, in, BUF_SIZE);
        dsp::mul2(out, gain, BUF_SIZE);

        // Save output
        io::Path path;
        UTEST_ASSERT(path.fmt("%s/utest-tpc-%s-tri-peak.csv", tempdir(), this->full_name()));
        io::OutSequence fd;
        UTEST_ASSERT(fd.open(&path, io::File::FM_WRITE_NEW, "UTF-8") == STATUS_OK);

        LSPString line;
        UTEST_ASSERT(fd.write_ascii("in;gain;out\n") == STATUS_OK);
        for (size_t i=0; i<BUF_SIZE; ++i)
        {
            UTEST_ASSERT(line.fmt_utf8("%.6f;%.6f;%.6f\n", in[i], gain[i], out[i]));
            UTEST_ASSERT(fd.write(&line) == STATUS_OK);
        }

        UTEST_ASSERT(fd.close() == STATUS_OK);

        UTEST_ASSERT(dsp::max(out, BUF_SIZE) < 0.6f);
        UTEST_ASSERT(dsp::min(out, BUF_SIZE) >= 0.0f);

        UTEST_ASSERT(dsp::max(gain, BUF_SIZE) >= 1.0f);
        UTEST_ASSERT(dsp::min(gain, BUF_SIZE) >= 0.0f);
        UTEST_ASSERT(float_equals_adaptive(gain.get(0), 1.0f));
        UTEST_ASSERT(float_equals_adaptive(gain.get(BUF_SIZE-1), 1.0f));

        ssize_t i1 = dsp::max_index(in, BUF_SIZE);
        ssize_t i2 = dsp::max_index(out, BUF_SIZE);
        UTEST_ASSERT(latency == (i2-i1));

        l.destroy();
    }

    void test_trapezoid_peak()
    {
        FloatBuffer in(BUF_SIZE);
        FloatBuffer out(BUF_SIZE);
        FloatBuffer gain(BUF_SIZE);
        FloatBuffer reduced(BUF_SIZE);

        // Prepare buffer
        in.fill_zero();
        {
            float s = 0.0f, step = 0.05f;
            int i=0;
            for ( ; s < 0.999f; s += step)
                in[i++] = s;
            for (size_t j=0; j<512; ++j)
                in[i++] = s;
            for ( ; s > 0.001f; s -= step)
                in[i++] = s;
        }

        // Initialize limiter
        dspu::Limiter l;
        dspu::Delay d;

        UTEST_ASSERT(l.init(SRATE*4, 20.0f));
        UTEST_ASSERT(d.init(20.0f * SRATE * 4));

        l.set_sample_rate(SRATE);
        l.set_mode(dspu::LM_HERM_THIN);
        l.set_knee(1.0f);
        l.set_threshold(0.5f, true);
        l.set_attack(1.5);
        l.set_release(1.5);
        l.set_lookahead(5); // 5 ms lookahead
        UTEST_ASSERT(l.modified());
        l.update_settings();

        ssize_t latency = l.get_latency();
        UTEST_ASSERT(latency == ssize_t(5.0f * SRATE * 0.001f));
        d.set_delay(latency);

        l.process(gain, in, BUF_SIZE);
        d.process(out, in, BUF_SIZE);
        dsp::mul3(reduced, out, gain, BUF_SIZE);

        // Save output
        io::Path path;
        UTEST_ASSERT(path.fmt("%s/utest-tpc-%s-trapezoid_peak.csv", tempdir(), this->full_name()));
        io::OutSequence fd;
        UTEST_ASSERT(fd.open(&path, io::File::FM_WRITE_NEW, "UTF-8") == STATUS_OK);

        LSPString line;
        UTEST_ASSERT(fd.write_ascii("in;out;gain;reduced\n") == STATUS_OK);
        for (size_t i=0; i<BUF_SIZE; ++i)
        {
            UTEST_ASSERT(line.fmt_utf8("%.6f;%.6f;%.6f;%.6f\n", in[i], out[i], gain[i], reduced[i]));
            UTEST_ASSERT(fd.write(&line) == STATUS_OK);
        }

        UTEST_ASSERT(fd.close() == STATUS_OK);

        l.destroy();
    }

    UTEST_MAIN
    {
        test_triangle_peak();
        test_trapezoid_peak();
    }

UTEST_END
