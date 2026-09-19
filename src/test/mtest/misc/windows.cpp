/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
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

namespace
{
    using namespace lsp;

    typedef struct window_t
    {
        dspu::windows::window_t wnd;
        const char *label;
    } window_t;

    static const window_t xwindows[] =
    {
        { dspu::windows::HANN, "Hann" },
        { dspu::windows::HAMMING, "Hamming" },
        { dspu::windows::BLACKMAN, "Blackman" },
        { dspu::windows::LANCZOS, "Lanczos" },
        { dspu::windows::GAUSSIAN, "Gaussian" },
        { dspu::windows::POISSON, "Poisson" },
        { dspu::windows::PARZEN, "Parzen" },
        { dspu::windows::TUKEY, "Tukey" },
        { dspu::windows::WELCH, "Welch" },
        { dspu::windows::NUTTALL, "Nuttall" },
        { dspu::windows::BLACKMAN_NUTTALL, "Blackman-Nuttall" },
        { dspu::windows::BLACKMAN_HARRIS, "Blackman-Harris" },
        { dspu::windows::HANN_POISSON, "Hann-Poisson" },
        { dspu::windows::BARTLETT_HANN, "Bartlett-Hann" },
        { dspu::windows::BARTLETT_FEJER, "Bartlett-Fejer" },
        { dspu::windows::TRIANGULAR, "Triangular" },
        { dspu::windows::RECTANGULAR, "Rectangular" },
        { dspu::windows::FLAT_TOP, "Flat top" },
        { dspu::windows::COSINE, "Cosine" },
        { dspu::windows::SQR_COSINE, "Squared Cosine" },
        { dspu::windows::CUBIC, "Cubic" },
        { dspu::windows::KAISER, "Kaiser" },
        { dspu::windows::TOTAL, NULL },
    };
} /* namespace */

MTEST_BEGIN("dspu.misc", windows)

    MTEST_MAIN
    {
        float *buf      = NULL;
        float *windows[dspu::windows::TOTAL];

        size_t points   = 1024;
        if (argc > 0)
            points          = atoi(argv[0]);
        if (points < 10)
            points          = 10;

        // Compute number of windows
        size_t num_windows  = 0;
        for (size_t i=0; xwindows[i].label != NULL; ++i)
            ++num_windows;

        // Allocate buffer to store windows
        const size_t count  = points * dspu::windows::TOTAL;
        buf             = new float[count];
        MTEST_ASSERT(buf != NULL);
        lsp_finally { delete [] buf; };

        // Generate windows
        float *ptr      = buf;
        for (size_t i=0; i < num_windows; ++i)
        {
            dspu::windows::window(ptr, points, xwindows[i].wnd);
            windows[i]      = ptr;
            ptr            += points;
        }

        // Save result
        char tmp_path[PATH_MAX];
        MTEST_ASSERT(snprintf(tmp_path, sizeof(tmp_path), "%s/%s.csv", tempdir(), full_name()) > 0);
        FILE * const out = fopen(tmp_path, "w");
        MTEST_ASSERT(out != NULL);
        lsp_finally { fclose(out); };

        // Print header
        printf("Index;");
        fprintf(out, "Index;");
        for (size_t i=0; i<num_windows; ++i)
        {
            printf("%s;", xwindows[i].label);
            fprintf(out, "%s;", xwindows[i].label);
        }
        printf("\n");
        fprintf(out, "\n");

        // Print items
        for (size_t i=0; i<points; ++i)
        {
            printf("%d;", int(i));
            fprintf(out, "%d;", int(i));
            for (size_t j=0; j<num_windows; ++j)
            {
                printf("%.8f;", windows[j][i]);
                fprintf(out, "%.8f;", windows[j][i]);
            }
            printf("\n");
            fprintf(out, "\n");
        }

        // Output final text
        printf("Results saved to %s\n", tmp_path);
    }

MTEST_END



