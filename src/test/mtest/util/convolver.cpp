/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 5 сент. 2020 г.
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
#include <lsp-plug.in/test-fw/FloatBuffer.h>
#include <lsp-plug.in/dsp-units/util/Convolver.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp/dsp.h>

using namespace lsp;

MTEST_BEGIN("dspu.util", convolver)

    MTEST_MAIN
    {
        FloatBuffer buf(65);
        dspu::Convolver cv;
        dspu::Sample vox, conv, out, dir;

        MTEST_ASSERT(vox.load("tmp/convolver/vox.wav") == STATUS_OK);
        MTEST_ASSERT(conv.load("tmp/convolver/mono-hall.wav") == STATUS_OK);

        MTEST_ASSERT(cv.init(conv.channel(0), conv.samples(), 13, 0.0f));

        MTEST_ASSERT(out.resize(1, vox.samples() + conv.samples()) == STATUS_OK);
        MTEST_ASSERT(dir.resize(1, vox.samples() + conv.samples()) == STATUS_OK);
        out.set_sample_rate(vox.sample_rate());
        dir.set_sample_rate(dir.sample_rate());

        // Convolve using the convolver
        buf.fill_zero();
        float *dst = out.channel(0);
        cv.process(dst, vox.channel(0), vox.samples());
        dst += vox.samples();
        for (size_t n=conv.samples(); n > 0; )
        {
            size_t to_do    = lsp_min(buf.size(), n);
            cv.process(dst, buf.data(), to_do);
            n              -= to_do;
            dst            += to_do;
        }

        // Perform direct convolution
        dsp::convolve(dir.channel(0), vox.channel(0), conv.channel(0), conv.samples(), vox.samples());

        MTEST_ASSERT(out.save("tmp/convolver/processed.wav") == STATUS_OK);
        MTEST_ASSERT(dir.save("tmp/convolver/direct.wav") == STATUS_OK);
    }

MTEST_END

