/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 28 авг. 2018 г.
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
#include <lsp-plug.in/dsp-units/misc/windows.h>

static const char *xwindows[] =
{
    "Hann",
    "Hamming",
    "Blackman",
    "Lanczos",
    "Gaussian",
    "Poisson",
    "Parzen",
    "Tukey",
    "Welch",
    "Nuttall",
    "Blackman-Nuttall",
    "Blackman-Harris",
    "Hann-Poisson",
    "Bartlett-Hann",
    "Bartlett-Fejer",
    "Triangular",
    "Rectangular",
    "Flat top",
    "Cosine",
    "Squared Cosine",
    "Cubic",
    NULL
};

MTEST_BEGIN("dspu.misc", windows)

    MTEST_MAIN
    {
        float *buf      = NULL;
        float *windows[dspu::windows::TOTAL];

        size_t points   = 2400;
        if (argc > 0)
            points          = atoi(argv[0]);
        if (points < 10)
            points          = 10;

        size_t count    = points * dspu::windows::TOTAL;
        buf             = new float[count];
        MTEST_ASSERT(buf != NULL);

        float *ptr      = buf;
        for (size_t i=0; i<dspu::windows::TOTAL; ++i)
        {
            dspu::windows::window(ptr, points, dspu::windows::window_t(i + dspu::windows::FIRST));

            windows[i]      = ptr;
            ptr            += points;
        }

        // Print header
        printf("Index;");
        for (size_t i=0; i<dspu::windows::TOTAL; ++i)
            printf("%s;", xwindows[i]);
        printf("\n");

        // Print items
        for (size_t i=0; i<points; ++i)
        {
            printf("%d;", int(i));
            for (size_t j=0; j< dspu::windows::TOTAL; ++j)
                printf("%.5f;", windows[j][i]);
            printf("\n");
        }

        delete [] buf;
    }

MTEST_END



