/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Stefano Tronci <stefano.tronci@tuta.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 15 Nov 2025.
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
#include <lsp-plug.in/fmt/lspc/File.h>
#include <lsp-plug.in/dsp-units/shaping/Shaper.h>

MTEST_BEGIN("dspu.shaping", Shaper)

    void make_ramp(float *buf, float start_value, float end_value, size_t count)
    {
        float step = (end_value - start_value) / (count - 1);

        for (size_t n = 0; n < count; ++n)
            buf[n] = start_value + n * step;
    }

    void write_buffer(const char *filePath, const char *description, const float *buf, size_t count)
    {
        printf("Writing %s to file %s\n", description, filePath);

        FILE *fp = NULL;
        fp = fopen(filePath, "w");

        if (fp == NULL)
            return;

        while (count--)
            fprintf(fp, "%.30f\n", *(buf++));

        if(fp)
            fclose(fp);
    }

MTEST_MAIN
{
    size_t sample_rate = 48000u;

    float *vInput   = new float[sample_rate];
    float *vOutput  = new float[sample_rate];

    make_ramp(vInput, -2.0f, 2.0f, sample_rate);

    io::Path path;

    MTEST_ASSERT(path.fmt("%s/shaper-input-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Input", vInput, sample_rate);

    dspu::Shaper shaper;
    shaper.set_sample_rate(sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_SINUSOIDAL);
    shaper.set_slope(1.0f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-sinusoidal-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Sinusoidal", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_POLYNOMIAL);
    shaper.set_shape(0.5f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-polynomial-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Polynomial", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_HYPERBOLIC);
    shaper.set_shape(0.5f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-hyperbolic-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Hyperbolic", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_EXPONENTIAL);
    shaper.set_shape(0.5f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-exponential-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Exponential", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_POWER);
    shaper.set_shape(0.1f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-power-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Power", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_BILINEAR);
    shaper.set_shape(0.5f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-bilinear-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Bilinear", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_RECTIFIER);
    shaper.set_shape(0.0f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-rectifier-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Rectifier", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_ASYMMETRIC_CLIP);
    shaper.set_high_level(0.75f);
    shaper.set_low_level(0.5f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-asymmetric-clip-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Asymmetric Clip", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_ASYMMETRIC_SOFTCLIP);
    shaper.set_high_level(0.75f);
    shaper.set_low_level(0.5f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-asymmetric-softclip-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Asymmetric Softclip", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_QUARTER_CIRCLE);
    shaper.set_radius(0.5f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-quarter-circle-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Quarter Circle", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_BITCRUSH_FLOOR);
    shaper.set_levels(8.0f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-bitcrush-floor-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Bitcrush Floor", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_BITCRUSH_CEIL);
    shaper.set_levels(8.0f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-bitcrush-ceil-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Bitcrush Ceil", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_BITCRUSH_ROUND);
    shaper.set_levels(8.0f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-bitcrush-round-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "Bitcrush Round", vOutput, sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_TAP_TUBEWARMTH);
    shaper.set_drive(0.0f);
    shaper.set_blend(0.0f);
    shaper.process_overwrite(vOutput, vInput, sample_rate);
    MTEST_ASSERT(path.fmt("%s/shaper-tap-tubewarmth-%s.csv", tempdir(), full_name()));
    write_buffer(path.as_native(), "TAP Tubewarmth", vOutput, sample_rate);

    shaper.destroy();

    delete [] vInput;
    delete [] vOutput;
}

MTEST_END;
