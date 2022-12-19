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

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>
#include <lsp-plug.in/dsp-units/sampling/SamplePlayer.h>
#include <lsp-plug.in/dsp/dsp.h>


#define SAMPLE_LENGTH       8

static const float samples[4][SAMPLE_LENGTH] =
{
    { 1, -1, 1, -1, 1, 1, -1, -1 },
    { 1, 2, 3, 5, 7, 11, 13, 17 },
    { 4, 3, 2, 1, 1, 2, 3, 4 },
    { 1, 2, 3, 2, 2, 3, 2, 1 }
};

UTEST_BEGIN("dspu.sampling", player)
    UTEST_MAIN
    {
        dspu::SamplePlayer sp;
        sp.init(4, 5);

        FloatBuffer src(0x100);
        src.fill_zero();
        FloatBuffer dst1(src);
        FloatBuffer dst2(src.size());

        // Initialize samples
        for (size_t i=0; i<4; ++i)
        {
            dspu::Sample *s = new dspu::Sample();
            s->init(1, SAMPLE_LENGTH, SAMPLE_LENGTH);
            dsp::copy(s->getBuffer(0), samples[i], SAMPLE_LENGTH);
            UTEST_ASSERT(sp.bind(i, s));
        }

        // Trigger playing
        float *dptr = dst1;
        dsp::copy(dptr, src, src.size());
        for (size_t i=0; i<4; ++i)
        {
            dsp::fmadd_k3(&dptr[(i+1) * 11], samples[i], (i+1)*1.1f, SAMPLE_LENGTH);
            UTEST_ASSERT(sp.play(i, 0, (i+1) * 1.1f, (i+1) * 11));
        }

        // Call processing
        dptr = dst2;
        float *sptr = src;
        for (size_t processed = 0; processed <= src.size(); processed += 16)
        {
            size_t n = src.size() - processed;
            if (n > 16)
                n = 16;
            sp.process(&dptr[processed], &sptr[processed], n);
        }

        // Destroy player
        sp.stop();
        sp.unbind_all();
        dspu::Sample *gc = sp.gc();
        UTEST_ASSERT(sp.gc() == NULL);
        sp.destroy(true);

        size_t num_gc = 0;
        while (gc != NULL)
        {
            dspu::Sample *next = gc->gc_next();
            UTEST_ASSERT(gc->gc_references() == 0);
            gc->destroy();
            delete gc;
            gc = next;
            ++num_gc;
        }
        UTEST_ASSERT(num_gc == 4);

        // Check state
        UTEST_ASSERT_MSG(src.valid(), "Source buffer corrupted");
        UTEST_ASSERT_MSG(dst1.valid(), "Destination buffer 1 corrupted");
        UTEST_ASSERT_MSG(dst2.valid(), "Destination buffer 2 corrupted");

        // Compare buffers
        if (!dst1.equals_absolute(dst2, 1e-5))
        {
            src.dump("src");
            dst1.dump("dst1");
            dst2.dump("dst2");
        }
    }
UTEST_END;







