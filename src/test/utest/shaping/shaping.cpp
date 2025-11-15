/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Stefano Tronci <stefano.tronci@tuta.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 28 Sept 2025.
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
#include <lsp-plug.in/dsp-units/shaping/shaping.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/test-fw/helpers.h>
#include <lsp-plug.in/dsp-units/misc/quickmath.h>

UTEST_BEGIN("dspu.shaping", shaping)

    UTEST_MAIN
    {
        dspu::shaping::shaping_t params;

        params.sinusoidal.slope = 0.5f * M_PI;
        params.sinusoidal.radius = M_PI / (2.0f * params.sinusoidal.slope);
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::sinusoidal(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::sinusoidal(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::sinusoidal(&params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::sinusoidal(&params, 2.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::sinusoidal(&params, -2.0f), -1.0f));

        params.polynomial.shape = 0.5f;
        params.polynomial.radius = 1.0f - params.polynomial.shape;
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::polynomial(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::polynomial(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::polynomial(&params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::polynomial(&params, 2.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::polynomial(&params, -2.0f), -1.0f));

        params.hyperbolic.shape = 0.5f;
        params.hyperbolic.hyperbolic_shape = dspu::quick_tanh(params.hyperbolic.shape);
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::hyperbolic(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::hyperbolic(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::hyperbolic(&params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::hyperbolic(&params, 2.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::hyperbolic(&params, -2.0f), -1.0f));

        params.exponential.shape = 2.0f;
        params.exponential.log_shape = dspu::quick_logf(params.exponential.shape);
        params.exponential.scale = params.exponential.shape / (params.exponential.shape - 1.0f);
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::exponential(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::exponential(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::exponential(&params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::exponential(&params, 2.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::exponential(&params, -2.0f), -1.0f));

        params.power.shape = 2.0f;
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::power(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::power(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::power(&params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::power(&params, 2.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::power(&params, -2.0f), -1.0f));

        params.bilinear.shape = 0.5f;
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bilinear(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bilinear(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bilinear(&params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bilinear(&params, 2.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bilinear(&params, -2.0f), -1.0f));

        params.asymmetric_clip.high_clip = 0.75f;
        params.asymmetric_clip.low_clip = 0.5f;
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_clip(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_clip(&params, 1.0f), params.asymmetric_clip.high_clip));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_clip(&params, -1.0f), -params.asymmetric_clip.low_clip));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_clip(&params, 2.0f), params.asymmetric_clip.high_clip));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_clip(&params, -2.0f), -params.asymmetric_clip.low_clip));

        params.asymmetric_softclip.high_limit = 0.75f;
        params.asymmetric_softclip.low_limit = 0.5f;
        params.asymmetric_softclip.pos_scale = 1.0f / (1.0f - params.asymmetric_softclip.high_limit);
        params.asymmetric_softclip.neg_scale = 1.0f / (1.0f - params.asymmetric_softclip.low_limit);
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_softclip(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_softclip(&params, 1.0f), params.asymmetric_clip.high_clip));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_softclip(&params, -1.0f), -params.asymmetric_clip.low_clip));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_softclip(&params, 2.0f), params.asymmetric_clip.high_clip));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::asymmetric_softclip(&params, -2.0f), -params.asymmetric_clip.low_clip));

        params.quarter_circle.radius = 1.0f;
        params.quarter_circle.radius2 = params.quarter_circle.radius * params.quarter_circle.radius;
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quarter_circle(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quarter_circle(&params, params.quarter_circle.radius), params.quarter_circle.radius));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quarter_circle(&params, -params.quarter_circle.radius), -params.quarter_circle.radius));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quarter_circle(&params, 2.0f * params.quarter_circle.radius), params.quarter_circle.radius));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quarter_circle(&params, -2.0f * params.quarter_circle.radius), -params.quarter_circle.radius));

        params.rectifier.shape = 0.0f;
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::rectifier(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::rectifier(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::rectifier(&params, -1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::rectifier(&params, 2.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::rectifier(&params, -2.0f), 1.0f));

        params.bitcrush_floor.levels = 3.0f;
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bitcrush_floor(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bitcrush_floor(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bitcrush_floor(&params, -1.0f), -1.0f));

        params.bitcrush_ceil.levels = 3.0f;
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bitcrush_ceil(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bitcrush_ceil(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bitcrush_ceil(&params, -1.0f), -1.0f));

        params.bitcrush_round.levels = 3.0f;
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bitcrush_round(&params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bitcrush_round(&params, 1.0f), 1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::bitcrush_round(&params, -1.0f), -1.0f));

        // TODO: Add TAP Tubewarmth test.
    }

UTEST_END;
