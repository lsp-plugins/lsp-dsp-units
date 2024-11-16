/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 16 нояб. 2024 г.
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
#include <lsp-plug.in/dsp-units/meters/ILUFSMeter.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/io/Path.h>

MTEST_BEGIN("dspu.meters", ilufs)

    static constexpr size_t BUFFER_SIZE = 0x400;

    MTEST_MAIN
    {
        dspu::Sample src, lufs, tpeak, psr;
        io::Path src_path, dst_path;

        // Load source file
        MTEST_ASSERT(src_path.fmt("%s/meters/loop.wav", resources()) > 0);
        printf("Loading file %s...\n", src_path.as_native());

        MTEST_ASSERT(src.load(&src_path) == STATUS_OK);
        MTEST_ASSERT(src.channels() == 2);

        // Initialize meter
        dspu::ILUFSMeter lm;
        const float integration_period = dspu::samples_to_seconds(src.sample_rate(), src.length());

        lm.init(2, integration_period, dspu::bs::LUFS_MEASURE_PERIOD_MS);
        lm.set_sample_rate(src.sample_rate());
        lm.set_integration_period(integration_period);
        lm.set_weighting(dspu::bs::WEIGHT_K);
        lm.set_active(0, true);
        lm.set_active(1, true);
        lm.set_designation(0, dspu::bs::CHANNEL_LEFT);
        lm.set_designation(1, dspu::bs::CHANNEL_RIGHT);

        dspu::Sample out;
        MTEST_ASSERT(out.init(1, src.length(), src.length()));
        out.set_sample_rate(src.sample_rate());

        for (size_t offset=0; offset < src.length(); )
        {
            size_t to_process = lsp_min(src.length() - offset, BUFFER_SIZE);

            lm.bind(0, src.channel(0, offset));
            lm.bind(1, src.channel(1, offset));
            lm.process(out.channel(0, offset), to_process);

            offset         += to_process;

            printf("Integrated LUFS: %.2f LUFS\n", dspu::gain_to_db(lm.loudness() * dspu::bs::DBFS_TO_LUFS_SHIFT_GAIN));
        }

        // Save the results
        MTEST_ASSERT(dst_path.fmt("%s/%s-lufs.wav", tempdir(), full_name()) > 0);
        printf("Saving LUFS curve to %s...\n", dst_path.as_native());
        MTEST_ASSERT(src.save(&dst_path) > 0);
    }

MTEST_END



