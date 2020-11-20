/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 24 авг. 2018 г.
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
#include <lsp-plug.in/dsp-units/misc/envelope.h>
#include <lsp-plug.in/dsp-units/const.h>
#include <lsp-plug.in/stdlib/math.h>

static const char *xenvelopes[] =
{
    "Violet noise",
    "Blue noise",
    "White noise",
    "Pink noise",
    "Brown noise",
    "Fall-off 4.5dB/oct",
    "Raise 4.5dB/oct",
    NULL
};

MTEST_BEGIN("dspu.misc", envelope)

    MTEST_MAIN
    {
        float *buf      = NULL;
        float *envelopes[dspu::envelope::TOTAL];

        size_t points   = 1024;
        if (argc > 0)
            points          = atoi(argv[0]);
        if (points < 10)
            points          = 10;

        buf             = new float[points * dspu::envelope::TOTAL];
        MTEST_ASSERT(buf != NULL);

        float *ptr      = buf;
        for (size_t i=0; i<dspu::envelope::TOTAL; ++i)
        {
            dspu::envelope::noise(ptr, points, dspu::envelope::envelope_t(i + dspu::envelope::FIRST));

            envelopes[i]    = ptr;
            ptr            += points;
        }

        // Print header
        printf("Index;Frequency;");
        for (size_t i=0; i<dspu::envelope::TOTAL; ++i)
            printf("%s;%s(dB);", xenvelopes[i], xenvelopes[i]);
        printf("\n");

        // Print items
        float kf = float(LSP_DSP_UNITS_SPEC_FREQ_MAX) / float(points);
        for (size_t i=0; i<points; ++i)
        {
            printf("%d;%.3f;", int(i), i *  kf);
            for (size_t j=0; j< dspu::envelope::TOTAL; ++j)
                printf("%.7f;%.2f;", envelopes[j][i], 20 * log10(envelopes[j][i]));
            printf("\n");
        }

        delete [] buf;
    }

MTEST_END



