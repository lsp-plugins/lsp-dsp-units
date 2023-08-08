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
    static constexpr float srate    = 48000.0f;
    static constexpr float fmin     = 10.0f;
    static constexpr float fmax     = srate * 0.5f;
    static constexpr size_t RANK    = 9;
    static constexpr size_t NRANK   = 1 << RANK;

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

    void dump_filters_multiple(float f0, float slope)
    {
        io::Path path;
        MTEST_ASSERT(path.fmt("%s/%s-filters-muliple-%.0f-hz-%.0f-db.csv", tempdir(), full_name(), f0, slope) > 0);

        printf("Writing file %s\n", path.as_native());

        float s                 = logf(fmax / fmin) / N;

        float *vf               = new float[N+1];
        MTEST_ASSERT(vf != NULL);
        lsp_finally { delete [] vf; };

        float *lp               = new float[N+1];
        MTEST_ASSERT(lp != NULL);
        lsp_finally { delete [] lp; };

        float *hp               = new float[N+1];
        MTEST_ASSERT(hp != NULL);
        lsp_finally { delete [] hp; };

        for (size_t i=0; i<=N; ++i)
            vf[i]                   = fmin * expf(i * s);

        dspu::crossover::hipass_set(hp, vf, f0, slope, N+1);
        dspu::crossover::lopass_set(lp, vf, f0, slope, N+1);

        FILE *fd = fopen(path.as_native(), "w");

        fprintf(fd, "f;hipass(f);lowpass(f);sum;\n");
        for (size_t i=0; i<=N; ++i)
        {
            float sum = hp[i] + lp[i];

            fprintf(fd, "%f;%f;%f;%f;\n", vf[i], dspu::gain_to_db(hp[i]), dspu::gain_to_db(lp[i]), dspu::gain_to_db(sum));
        }

        fclose(fd);
    }

    void dump_bandpass_multiple(float f0, float slope0, float f1, float slope1)
    {
        io::Path path;
        MTEST_ASSERT(path.fmt("%s/%s-bandpass-muliple-%.0f-%.0f-hz-%.0f-%.0f-db.csv",
            tempdir(), full_name(), f0, f1, slope0, slope1) > 0);

        printf("Writing file %s\n", path.as_native());

        float s                 = logf(fmax / fmin) / N;

        float *vf               = new float[N+1];
        MTEST_ASSERT(vf != NULL);
        lsp_finally { delete [] vf; };

        float *lp               = new float[N+1];
        MTEST_ASSERT(lp != NULL);
        lsp_finally { delete [] lp; };

        float *hp               = new float[N+1];
        MTEST_ASSERT(hp != NULL);
        lsp_finally { delete [] hp; };

        float *bp               = new float[N+1];
        MTEST_ASSERT(bp != NULL);
        lsp_finally { delete [] bp; };

        for (size_t i=0; i<=N; ++i)
        {
            vf[i]                   = fmin * expf(i * s);
            bp[i]                   = 1.0f;
        }

        dspu::crossover::hipass_set(hp, vf, f0, slope0, N+1);
        dspu::crossover::lopass_set(lp, vf, f1, slope1, N+1);

        dspu::crossover::hipass_apply(bp, vf, f0, slope0, N+1);
        dspu::crossover::lopass_apply(bp, vf, f1, slope1, N+1);

        FILE *fd = fopen(path.as_native(), "w");

        fprintf(fd, "f;hipass(f);lowpass(f);bandpass(f);\n");
        for (size_t i=0; i<=N; ++i)
            fprintf(fd, "%f;%f;%f;%f;\n", vf[i], dspu::gain_to_db(hp[i]), dspu::gain_to_db(lp[i]), dspu::gain_to_db(bp[i]));

        fclose(fd);
    }

    void dump_filters_fft(float f0, float slope)
    {
        io::Path path;
        MTEST_ASSERT(path.fmt("%s/%s-fft-%.0f-hz-%.0f-db.csv", tempdir(), full_name(), f0, slope) > 0);

        printf("Writing file %s\n", path.as_native());

        float kf                = srate / NRANK;

        float *vf               = new float[NRANK];
        MTEST_ASSERT(vf != NULL);
        lsp_finally { delete [] vf; };

        float *lp               = new float[NRANK];
        MTEST_ASSERT(lp != NULL);
        lsp_finally { delete [] lp; };

        float *hp               = new float[NRANK];
        MTEST_ASSERT(hp != NULL);
        lsp_finally { delete [] hp; };

        dspu::crossover::hipass_fft_set(hp, f0, slope, srate, RANK);
        dspu::crossover::lopass_fft_set(lp, f0, slope, srate, RANK);

        FILE *fd = fopen(path.as_native(), "w");

        fprintf(fd, "f;hipass(f);lowpass(f);sum;\n");
        for (size_t i=0; i<NRANK; ++i)
        {
            float f     = (i > NRANK/2) ? (NRANK - i)*kf : i*kf;
            float sum   = hp[i] + lp[i];

            fprintf(fd, "%f;%f;%f;%f;\n", f, dspu::gain_to_db(hp[i]), dspu::gain_to_db(lp[i]), dspu::gain_to_db(sum));
        }

        fclose(fd);
    }

    void dump_bandpass_fft(float f0, float slope0, float f1, float slope1)
    {
        io::Path path;
        MTEST_ASSERT(path.fmt("%s/%s-bandpass-fft-%.0f-%.0f-hz-%.0f-%.0f-db.csv",
            tempdir(), full_name(), f0, f1, slope0, slope1) > 0);

        printf("Writing file %s\n", path.as_native());

        float kf                = srate / NRANK;

        float *vf               = new float[NRANK];
        MTEST_ASSERT(vf != NULL);
        lsp_finally { delete [] vf; };

        float *lp               = new float[NRANK];
        MTEST_ASSERT(lp != NULL);
        lsp_finally { delete [] lp; };

        float *hp               = new float[NRANK];
        MTEST_ASSERT(hp != NULL);
        lsp_finally { delete [] hp; };

        float *bp               = new float[NRANK];
        MTEST_ASSERT(bp != NULL);
        lsp_finally { delete [] bp; };

        for (size_t i=0; i<=NRANK; ++i)
            bp[i]                   = 1.0f;

        dspu::crossover::hipass_fft_set(hp, f0, slope0, srate, RANK);
        dspu::crossover::lopass_fft_set(lp, f1, slope1, srate, RANK);

        dspu::crossover::hipass_fft_apply(bp, f0, slope0, srate, RANK);
        dspu::crossover::lopass_fft_apply(bp, f1, slope1, srate, RANK);

        FILE *fd = fopen(path.as_native(), "w");

        fprintf(fd, "f;hipass(f);lowpass(f);bandpass(f);\n");
        for (size_t i=0; i<NRANK; ++i)
        {
            float f     = (i > NRANK/2) ? (NRANK - i)*kf : i*kf;

            fprintf(fd, "%f;%f;%f;%f;\n", f, dspu::gain_to_db(hp[i]), dspu::gain_to_db(lp[i]), dspu::gain_to_db(bp[i]));
        }

        fclose(fd);
    }

    MTEST_MAIN
    {
        dump_filters_single(1000.0f, -64.0f);
        dump_filters_single(1000.0f, -32.0f);
        dump_filters_single(1000.0f, -12.0f);
        dump_filters_single(1000.0f, 0.0f);

        dump_filters_multiple(100.0f, -64.0f);
        dump_filters_multiple(100.0f, -32.0f);
        dump_filters_multiple(100.0f, -12.0f);
        dump_filters_multiple(100.0f, 0.0f);

        dump_bandpass_multiple(100.0f, -64.0f, 1000.0f, -32.0f);
        dump_bandpass_multiple(100.0f, -32.0f, 1000.0f, -12.0f);
        dump_bandpass_multiple(100.0f, -12.0f, 1000.0f, -64.0f);

        dump_filters_fft(1000.0f, -64.0f);
        dump_filters_fft(1000.0f, -32.0f);
        dump_filters_fft(1000.0f, -12.0f);
        dump_filters_fft(1000.0f, 0.0f);

        dump_bandpass_fft(1000.0f, -64.0f, 10000.0f, -32.0f);
        dump_bandpass_fft(1000.0f, -32.0f, 10000.0f, -12.0f);
        dump_bandpass_fft(1000.0f, -12.0f, 10000.0f, -64.0f);
    }

MTEST_END


