/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 17 окт. 2024 г.
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
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/meters/TruePeakMeter.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/io/Path.h>


MTEST_BEGIN("dspu.meters", true_peak)

    MTEST_MAIN
    {
        dspu::Sample src, dst;
        io::Path src_path, dst_path;

        // Load source file
        MTEST_ASSERT(src_path.fmt("%s/meters/loop.wav", resources()) > 0);
        MTEST_ASSERT(src.load(&src_path) == STATUS_OK);
        printf("Loading file %s...\n", src_path.as_native());

        // Initialize meter
        dspu::TruePeakMeter tpm;
        tpm.init();
        tpm.set_sample_rate(src.sample_rate());
        tpm.update_settings();
        const size_t latency = tpm.latency();

        MTEST_ASSERT(src.append(latency * 2) == STATUS_OK);

        MTEST_ASSERT(dst.init(src.channels(), src.length(), src.length()));
        dst.set_sample_rate(src.sample_rate());

        for (size_t i=0; i<src.channels(); ++i)
        {
            tpm.clear();
            tpm.process(dst.channel(i), src.channel(i), src.length());
            float tpl = 0.0f;

            // Find the maximum true peak value and invert the true peak polarity if it exceeds 0 dBFS
            float *c = dst.channel(i);
            for (size_t i=0, n=dst.length(); i<n; ++i)
            {
                tpl = lsp_max(tpl, c[i]);
                if (c[i] > 1.0f)
                    c[i] = -c[i];
            }

            printf("channel %d true peak level = %.2f\n", int(i), dspu::gain_to_db(tpl));
        }

        // Save the sample
        MTEST_ASSERT(dst_path.fmt("%s/%s-tpm.wav", tempdir(), full_name()) > 0);
        printf("Saving file %s...\n", dst_path.as_native());
        MTEST_ASSERT(dst.save(&dst_path) > 0);
    }

MTEST_END



