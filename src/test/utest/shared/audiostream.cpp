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

#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/shared/AudioStream.h>
#include <lsp-plug.in/test-fw/utest.h>

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

    UTEST_MAIN
    {
//        test_create_open();
        test_allocate_open();
    }
UTEST_END





