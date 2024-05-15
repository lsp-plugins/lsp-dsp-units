/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 11 мая 2024 г.
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

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/shared/AudioStream.h>
#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>

UTEST_BEGIN("dspu.shared", audiostream)
    UTEST_TIMELIMIT(30)

    void test_create_open()
    {
        LSPString id;
        dspu::AudioStream out, in;
        UTEST_ASSERT(id.fmt_utf8("%s-create.shm", full_name()));

        printf("Testing create and open audio stream id=%s ...\n", id.get_native());

        UTEST_ASSERT(out.create(&id, 2, 1024) == STATUS_OK);
        UTEST_ASSERT(out.channels() == 2);
        UTEST_ASSERT(out.length() == 1024);

        UTEST_ASSERT(in.open(&id) == STATUS_OK);
        UTEST_ASSERT(in.channels() == 2);
        UTEST_ASSERT(in.length() == 1024);
        UTEST_ASSERT(in.close() == STATUS_OK);

        UTEST_ASSERT(out.close() == STATUS_OK);
    }

    void test_allocate_open()
    {
        LSPString id;
        dspu::AudioStream out, in;

        printf("Testing allocate and open audio stream...\n");
        UTEST_ASSERT(out.allocate(&id, ".shm", 2, 1024) == STATUS_OK);
        printf("  allocated stream with unique id=%s ...\n", id.get_native());

        UTEST_ASSERT(out.channels() == 2);
        UTEST_ASSERT(out.length() == 1024);

        UTEST_ASSERT(in.open(&id) == STATUS_OK);
        UTEST_ASSERT(in.channels() == 2);
        UTEST_ASSERT(in.length() == 1024);
        UTEST_ASSERT(in.close() == STATUS_OK);

        UTEST_ASSERT(out.close() == STATUS_OK);
    }

    void test_read_write()
    {
        LSPString id;
        dspu::AudioStream out, in;

        printf("Testing synchronized read/write on audio stream...\n");
        UTEST_ASSERT(out.allocate(&id, ".shm", 2, 1024) == STATUS_OK);
        printf("  allocated stream with unique id=%s ...\n", id.get_native());

        UTEST_ASSERT(out.channels() == 2);
        UTEST_ASSERT(out.length() == 1024);

        UTEST_ASSERT(in.open(&id) == STATUS_OK);
        UTEST_ASSERT(in.channels() == 2);
        UTEST_ASSERT(in.length() == 1024);

        // Initialize buffers
        FloatBuffer bout_l(0x10);
        FloatBuffer bout_r(0x10);
        FloatBuffer bin_l(0x10);
        FloatBuffer bin_r(0x10);
        FloatBuffer zero(0x10);

        for (size_t i=0; i<0x10; ++i)
        {
            bout_l[i] = i + 1.0f;
            bout_r[i] = -(i + 1.0f);
        }
        bin_l.fill_zero();
        bin_r.fill_zero();
        zero.fill_zero();

        // Do full write and read
        UTEST_ASSERT(out.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(out.write(0, bout_l.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.write(1, bout_r.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);

        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin_l.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(1, bin_r.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout_l.corrupted());
        UTEST_ASSERT(!bout_r.corrupted());
        UTEST_ASSERT(!bin_l.corrupted());
        UTEST_ASSERT(!bin_r.corrupted());
        UTEST_ASSERT(bin_l.equals_relative(bout_l));
        UTEST_ASSERT(bin_r.equals_relative(bout_r));

        // Do partial write and read (write left channel only)
        bin_l.fill_zero();
        bin_r.fill_zero();
        zero.fill_zero();

        UTEST_ASSERT(out.begin() == STATUS_OK);
        UTEST_ASSERT(out.write(0, bout_l.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);

        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin_l.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(1, bin_r.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout_l.corrupted());
        UTEST_ASSERT(!bout_r.corrupted());
        UTEST_ASSERT(!bin_l.corrupted());
        UTEST_ASSERT(!bin_r.corrupted());
        UTEST_ASSERT(bin_l.equals_relative(bout_l));
        UTEST_ASSERT(bin_r.equals_relative(zero));

        // Do partial write and read (write right channel only)
        bin_l.fill_zero();
        bin_r.fill_zero();
        zero.fill_zero();

        UTEST_ASSERT(out.begin() == STATUS_OK);
        UTEST_ASSERT(out.write(1, bout_r.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);

        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin_l.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(1, bin_r.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout_l.corrupted());
        UTEST_ASSERT(!bout_r.corrupted());
        UTEST_ASSERT(!bin_l.corrupted());
        UTEST_ASSERT(!bin_r.corrupted());
        UTEST_ASSERT(bin_l.equals_relative(zero));
        UTEST_ASSERT(bin_r.equals_relative(bout_r));

        // Do partial write and read (both channels unsynced)
        bin_l.fill_zero();
        bin_r.fill_zero();
        zero.fill_zero();

        UTEST_ASSERT(out.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(out.write(0, bout_l.data(), 0x0c) == STATUS_OK);
        UTEST_ASSERT(out.write(1, bout_r.data(), 0x08) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);

        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin_l.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(1, bin_r.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout_l.corrupted());
        UTEST_ASSERT(!bout_r.corrupted());
        UTEST_ASSERT(!bin_l.corrupted());
        UTEST_ASSERT(!bin_r.corrupted());

        dsp::fill_zero(bout_l.data() + 0x0c, 4);
        dsp::fill_zero(bout_r.data() + 0x08, 8);

        UTEST_ASSERT(bin_l.equals_relative(bout_l));
        UTEST_ASSERT(bin_r.equals_relative(bout_r));

        // Do full write but partial read
        for (size_t i=0; i<0x10; ++i)
        {
            bout_l[i] = i + 1.0f;
            bout_r[i] = -(i + 1.0f);
        }
        bin_l.fill_zero();
        bin_r.fill_zero();
        zero.fill_zero();

        UTEST_ASSERT(out.begin() == STATUS_OK);
        UTEST_ASSERT(out.write(0, bout_l.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.write(1, bout_r.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);

        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin_l.data(), 0x08) == STATUS_OK);
        UTEST_ASSERT(in.read(1, bin_r.data(), 0x0c) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout_l.corrupted());
        UTEST_ASSERT(!bout_r.corrupted());
        UTEST_ASSERT(!bin_l.corrupted());
        UTEST_ASSERT(!bin_r.corrupted());

        dsp::fill_zero(bout_l.data() + 0x08, 8);
        dsp::fill_zero(bout_r.data() + 0x0c, 4);

        UTEST_ASSERT(bin_l.equals_relative(bout_l));
        UTEST_ASSERT(bin_r.equals_relative(bout_r));

        // Close the stream
        UTEST_ASSERT(in.close() == STATUS_OK);
        UTEST_ASSERT(out.close() == STATUS_OK);
    }

    void test_overrun()
    {
        LSPString id;
        dspu::AudioStream out, in;

        printf("Testing overrun on audio stream...\n");
        UTEST_ASSERT(out.allocate(&id, ".shm", 1, 1024) == STATUS_OK);
        printf("  allocated stream with unique id=%s ...\n", id.get_native());
        UTEST_ASSERT(in.open(&id) == STATUS_OK);

        // Initialize buffers
        FloatBuffer bout(0x10);
        FloatBuffer bin(0x10);

        for (size_t i=0; i<0x10; ++i)
            bout[i] = i + 1.0f;

        bin.fill_zero();

        // Perform first read-write cycle as usual
        UTEST_ASSERT(out.begin() == STATUS_OK);
        UTEST_ASSERT(out.write(0, bout.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);

        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout.corrupted());
        UTEST_ASSERT(!bin.corrupted());
        UTEST_ASSERT(bin.equals_relative(bout));

        // Do many write cycles
        for (size_t j=0; j<10; ++j)
        {
            for (size_t i=0; i<0x10; ++i)
                bout[i] = i + j + 2.0f;

            UTEST_ASSERT(out.begin() == STATUS_OK);
            UTEST_ASSERT(out.write(0, bout.data(), 0x10) == STATUS_OK);
            UTEST_ASSERT(out.end() == STATUS_OK);
        }

        // Perform the read
        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout.corrupted());
        UTEST_ASSERT(!bin.corrupted());
        UTEST_ASSERT(bin.equals_relative(bout));

        // Close the stream
        UTEST_ASSERT(in.close() == STATUS_OK);
        UTEST_ASSERT(out.close() == STATUS_OK);
    }

    void test_underrun()
    {
        LSPString id;
        dspu::AudioStream out, in;

        printf("Testing underrun on audio stream...\n");
        UTEST_ASSERT(out.allocate(&id, ".shm", 1, 1024) == STATUS_OK);
        printf("  allocated stream with unique id=%s ...\n", id.get_native());
        UTEST_ASSERT(in.open(&id) == STATUS_OK);

        // Initialize buffers
        FloatBuffer bout(0x10);
        FloatBuffer bin(0x10);
        FloatBuffer zero(0x10);

        for (size_t i=0; i<0x10; ++i)
            bout[i] = i + 1.0f;

        bin.fill_zero();
        zero.fill_zero();

        // Perform first read-write cycle as usual
        UTEST_ASSERT(out.begin() == STATUS_OK);
        UTEST_ASSERT(out.write(0, bout.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);

        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout.corrupted());
        UTEST_ASSERT(!bin.corrupted());
        UTEST_ASSERT(bin.equals_relative(bout));

        // Do many read cycles
        for (size_t j=0; j<10; ++j)
        {
            UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
            UTEST_ASSERT(in.read(0, bin.data(), 0x10) == STATUS_OK);
            UTEST_ASSERT(in.end() == STATUS_OK);

            UTEST_ASSERT(!bout.corrupted());
            UTEST_ASSERT(!bin.corrupted());
            UTEST_ASSERT(bin.equals_relative(zero));
        }

        // Perform the second read-write cycle
        UTEST_ASSERT(out.begin() == STATUS_OK);
        UTEST_ASSERT(out.write(0, bout.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);

        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout.corrupted());
        UTEST_ASSERT(!bin.corrupted());
        UTEST_ASSERT(bin.equals_relative(bout));

        // Close the stream
        UTEST_ASSERT(in.close() == STATUS_OK);
        UTEST_ASSERT(out.close() == STATUS_OK);
    }

    void test_close()
    {
        LSPString id;
        dspu::AudioStream out, in;

        printf("Testing underrun on audio stream...\n");
        UTEST_ASSERT(out.allocate(&id, ".shm", 1, 1024) == STATUS_OK);
        printf("  allocated stream with unique id=%s ...\n", id.get_native());
        UTEST_ASSERT(in.open(&id) == STATUS_OK);

        // Initialize buffers
        FloatBuffer bout(0x10);
        FloatBuffer bin(0x10);
        FloatBuffer zero(0x10);

        for (size_t i=0; i<0x10; ++i)
            bout[i] = i + 1.0f;

        bin.fill_zero();
        zero.fill_zero();

        // Perform first read-write cycle as usual
        UTEST_ASSERT(out.begin() == STATUS_OK);
        UTEST_ASSERT(out.write(0, bout.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);

        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout.corrupted());
        UTEST_ASSERT(!bin.corrupted());
        UTEST_ASSERT(bin.equals_relative(bout));

        // Write and close the output
        UTEST_ASSERT(out.begin() == STATUS_OK);
        UTEST_ASSERT(out.write(0, bout.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(out.end() == STATUS_OK);
        UTEST_ASSERT(out.close() == STATUS_OK);

        // Perform the second read-write cycle
        UTEST_ASSERT(in.begin(0x10) == STATUS_OK);
        UTEST_ASSERT(in.read(0, bin.data(), 0x10) == STATUS_OK);
        UTEST_ASSERT(in.end() == STATUS_OK);

        UTEST_ASSERT(!bout.corrupted());
        UTEST_ASSERT(!bin.corrupted());
        UTEST_ASSERT(bin.equals_relative(bout));

        // Perform the third read-write cycle
        UTEST_ASSERT(in.begin(0x10) == STATUS_EOF);

        // Close the stream
        UTEST_ASSERT(in.close() == STATUS_OK);
    }

    UTEST_MAIN
    {
        test_create_open();
        test_allocate_open();
        test_read_write();
        test_overrun();
        test_underrun();
        test_close();
    }
UTEST_END





