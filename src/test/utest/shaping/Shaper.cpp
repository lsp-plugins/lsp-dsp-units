/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Stefano Tronci <stefano.tronci@tuta.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 09 Nov 2025.
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

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/helpers.h>
#include <lsp-plug.in/dsp-units/shaping/Shaper.h>
#include <lsp-plug.in/dsp/dsp.h>

UTEST_BEGIN("dspu.shaping", Shaper)

    void make_ramp(float *buf, float start_value, float end_value, size_t count)
    {
        float step = (end_value - start_value) / (count - 1);

        for (size_t n = 0; n < count; ++n)
            buf[n] = start_value + n * step;
    }

    void basic_range_test(float *buf, size_t count, float expected_min, float expected_max, float tol = 1e-6)
    {
        float min;
        float max;
        dsp::minmax(buf, count, &min, &max);

        UTEST_ASSERT(float_equals_absolute(expected_min, min, tol));
        UTEST_ASSERT(float_equals_absolute(expected_max, max, tol));
    }

UTEST_MAIN
{
    // Test parameters
    size_t sample_rate = 48000u;
    float duration = 1.0;

    float ramp_start = -2.0f;
    float ramp_end = 2.0f;

    float radius = 0.5f;

    float low_level = 0.5f;
    float high_level = 0.75f;
    //

    size_t n_samples = duration * sample_rate;

    float *vInput   = new float[n_samples];
    float *vOutput  = new float[n_samples];

    make_ramp(vInput, ramp_start, ramp_end, n_samples);

    float saturated_ramp_min = lsp_max(ramp_start, -1.0f);
    float saturated_ramp_max = lsp_min(ramp_end, 1.0f);

    float circle_min = -dspu::Shaper::fQuarter_circle_max_radius * radius;
    float circle_max = dspu::Shaper::fQuarter_circle_max_radius * radius;

    dspu::Shaper shaper;
    shaper.set_sample_rate(sample_rate);

    shaper.set_function(dspu::sh_function_t::SH_FCN_SINUSOIDAL);
    shaper.set_slope(1.0f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, saturated_ramp_min, saturated_ramp_max);

    shaper.set_function(dspu::sh_function_t::SH_FCN_POLYNOMIAL);
    shaper.set_shape(0.5f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, saturated_ramp_min, saturated_ramp_max);

    shaper.set_function(dspu::sh_function_t::SH_FCN_HYPERBOLIC);
    shaper.set_shape(0.5f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, saturated_ramp_min, saturated_ramp_max);

    shaper.set_function(dspu::sh_function_t::SH_FCN_EXPONENTIAL);
    shaper.set_shape(0.5f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, saturated_ramp_min, saturated_ramp_max);

    shaper.set_function(dspu::sh_function_t::SH_FCN_POWER);
    shaper.set_shape(0.1f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, saturated_ramp_min, saturated_ramp_max);

    shaper.set_function(dspu::sh_function_t::SH_FCN_BILINEAR);
    shaper.set_shape(0.5f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, saturated_ramp_min, saturated_ramp_max);

    shaper.set_function(dspu::sh_function_t::SH_FCN_RECTIFIER);
    shaper.set_shape(0.0f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, 0.0f, saturated_ramp_max, 1e-4);

    shaper.set_function(dspu::sh_function_t::SH_FCN_ASYMMETRIC_CLIP);
    shaper.set_high_level(high_level);
    shaper.set_low_level(low_level);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, -low_level, high_level);

    shaper.set_function(dspu::sh_function_t::SH_FCN_ASYMMETRIC_SOFTCLIP);
    shaper.set_high_level(high_level);
    shaper.set_low_level(low_level);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, -low_level, high_level, 1e-4);

    shaper.set_function(dspu::sh_function_t::SH_FCN_QUARTER_CIRCLE);
    shaper.set_radius(radius);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, circle_min, circle_max);

    shaper.set_function(dspu::sh_function_t::SH_FCN_BITCRUSH_FLOOR);
    shaper.set_levels(8.0f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, ramp_start, ramp_end);

    shaper.set_function(dspu::sh_function_t::SH_FCN_BITCRUSH_CEIL);
    shaper.set_levels(8.0f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, ramp_start, ramp_end);

    shaper.set_function(dspu::sh_function_t::SH_FCN_BITCRUSH_ROUND);
    shaper.set_levels(8.0f);
    shaper.process_overwrite(vOutput, vInput, n_samples);
    basic_range_test(vOutput, n_samples, ramp_start, ramp_end);

    // TODO: Add TAP Tubewarmth test.
}

UTEST_END;
