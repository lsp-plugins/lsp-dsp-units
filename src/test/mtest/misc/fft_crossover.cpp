/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 8 авг. 2023 г.
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

#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/dsp-units/misc/fft_crossover.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/stdlib/math.h>

MTEST_BEGIN("dspu.misc", fft_crossover)

    static constexpr size_t N       = 320;
    static constexpr float fmin     = 10.0f;
    static constexpr float fmax     = 24000.0f;

    void dump_filters_single(float f0, float slope)
    {
        io::Path path;
        MTEST_ASSERT(path.fmt("%s/%s-filters-single-%.0f-hz-%.0f-db.csv", tempdir(), full_name(), f0, slope) > 0);

        printf("Writing file %s\n", path.as_native());

        float s                 = logf(fmax / fmin) / N;

        FILE *fd = fopen(path.as_native(), "w");

        fprintf(fd, "f;hipass(f);lowpass(f);sum;\n");
        for (size_t i=0; i<=N; ++i)
        {
            float f = fmin * expf(i * s);
            float lp = dspu::crossover::hipass(f, f0, slope);
            float hp = dspu::crossover::lopass(f, f0, slope);
            float sum = hp + lp;

            fprintf(fd, "%f;%f;%f;%f;\n", f, dspu::gain_to_db(lp), dspu::gain_to_db(hp), dspu::gain_to_db(sum));
        }

        fclose(fd);
    }

    MTEST_MAIN
    {
        dump_filters_single(1000.0f, -64.0f);
        dump_filters_single(1000.0f, -32.0f);
        dump_filters_single(1000.0f, -12.0f);
        dump_filters_single(1000.0f, 0.0f);
    }

MTEST_END


