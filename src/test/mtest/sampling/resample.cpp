/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 29 авг. 2018 г.
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

#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>

static const char *SRC_FILE = "test_data/source.wav";
static const char *DST_FILE = "test_data/destination.wav";

MTEST_BEGIN("dspu.sampling", resample)

    MTEST_MAIN
    {
        const char *src = (argc >= 1) ? argv[0] : SRC_FILE;
        const char *dst = (argc >= 2) ? argv[1] : DST_FILE;

        dspu::Sample af;

        MTEST_ASSERT(af.load(src) == STATUS_OK); // Load audio file
        size_t sr = (af.sample_rate() == 44100) ? 48000 : 44100;
        printf("Resampling %d -> %d", int(af.sample_rate()), int(sr));

        MTEST_ASSERT(af.resample(sr) == STATUS_OK); // Resample audio file
        MTEST_ASSERT(af.save(dst) == wssize_t(af.length())); // Store file

        // Destroy data
        af.destroy();
    }

MTEST_END


