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

    void test_a_law(dspu::shaping::shaping_t *params)
    {
        // Compression --------------------------------------------------------------------------------------------------------------------
        params->continuous_a_law_compression.compression = 87.6f;
        params->continuous_a_law_compression.compression_reciprocal = 1.0f / params->continuous_a_law_compression.compression;
        params->continuous_a_law_compression.scale = 1.0f / (1.0f + dspu::quick_logf(params->continuous_a_law_compression.compression));

        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_a_law_compression(params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_a_law_compression(params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_a_law_compression(params, 1.0f), 1.0f));

        params->quantized_a_law_companding.pcm_n_bits = 13;
        params->quantized_a_law_companding.pcm_clip = (1 << (params->quantized_a_law_companding.pcm_n_bits - 1)) - 1;

        // Checking the results are the same as in Table 1a, columns 3 and 6, https://www.itu.int/rec/T-REC-G.711-198811-I/en.
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(0)      == 0b10000000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(1)      == 0b10000000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(64)     == 0b10100000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(128)    == 0b10110000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(256)    == 0b11000000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(512)    == 0b11010000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(1024)   == 0b11100000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(2048)   == 0b11110000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(3968)   == 0b11111111);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(4095)   == 0b11111111); // max PCM13 value.

        // Checking the results are the same as in Table 1b, columns 3 and 6, https://www.itu.int/rec/T-REC-G.711-198811-I/en.
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(-1)     == 0b00000000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(-64)    == 0b00100000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(-128)   == 0b00110000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(-256)   == 0b01000000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(-512)   == 0b01010000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(-1024)  == 0b01100000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(-2048)  == 0b01110000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(-3968)  == 0b01111111);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_compression(-4095)  == 0b01111111); // min PCM13 value.

        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_a_law_compression_shaper(params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_a_law_compression_shaper(params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_a_law_compression_shaper(params, 1.0f), 1.0f));

        // Expansion ---------------------------------------------------------------------------------------------------------------------
        params->continuous_a_law_expansion.expansion = 87.7f;
        params->continuous_a_law_expansion.expansion_reciprocal = 1.0f / params->continuous_a_law_expansion.expansion;
        params->continuous_a_law_expansion.radius = 1.0f / (1.0f + dspu::quick_logf(params->continuous_a_law_expansion.expansion));
        params->continuous_a_law_expansion.radius_reciprocal = 1.0f + dspu::quick_logf(params->continuous_a_law_expansion.expansion);

        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_a_law_expansion(params, -1.0f), -1.0f, 1e-3));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_a_law_expansion(params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_a_law_expansion(params, 1.0f), 1.0f, 1e-3));

        params->quantized_a_law_companding.pcm_n_bits = 13;
        params->quantized_a_law_companding.pcm_clip = (1 << (params->quantized_a_law_companding.pcm_n_bits - 1)) - 1;

        // Checking that the patterns in https://en.wikipedia.org/wiki/G.711#A-law are correct.
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b10001010) == +0b0000000010101);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b10011010) == +0b0000000110101);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b10101010) == +0b0000001101010);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b10111010) == +0b0000011010100);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b11001010) == +0b0000110101000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b11011010) == +0b0001101010000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b11101010) == +0b0011010100000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b11111010) == +0b0110101000000);

        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b00001010) == -0b0000000010101);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b00011010) == -0b0000000110101);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b00101010) == -0b0000001101010);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b00111010) == -0b0000011010100);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b01001010) == -0b0000110101000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b01011010) == -0b0001101010000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b01101010) == -0b0011010100000);
        UTEST_ASSERT(dspu::shaping::quantized_a_law_expansion(0b01111010) == -0b0110101000000);

        // The maximum expanded value is 0b0111111000000 = 4032, so the peak of the quantized expander is 4032 / pcm_clip
        float expanded_peak = 4032.0f / static_cast<float>(params->quantized_a_law_companding.pcm_clip);
        // The minimum expanded value is 0b0000000000001 = 1, so the minimum of the quantized expander is 1 / pcm_clip
        float expanded_min = 1.0f / static_cast<float>(params->quantized_a_law_companding.pcm_clip);
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_a_law_expansion_shaper(params, -1.0f), -expanded_peak));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_a_law_expansion_shaper(params, 0.0f), expanded_min));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_a_law_expansion_shaper(params, 1.0f), expanded_peak));
    }

    void test_mu_law(dspu::shaping::shaping_t *params)
    {
        // Compression --------------------------------------------------------------------------------------------------------------------
        params->continuous_mu_law_compression.compression = 255.0f;
        params->continuous_mu_law_compression.scale = 1.0f / dspu::quick_logf(1.0f + params->continuous_mu_law_compression.compression);

        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_mu_law_compression(params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_mu_law_compression(params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_mu_law_compression(params, 1.0f), 1.0f));

        params->quantized_mu_law_companding.pcm_n_bits = 14;
        params->quantized_mu_law_companding.pcm_clip = (1 << (params->quantized_mu_law_companding.pcm_n_bits - 1)) - 1;
        params->quantized_mu_law_companding.pcm_bias = 33;
        params->quantized_mu_law_companding.pcm_max_magnitude = params->quantized_mu_law_companding.pcm_clip - params->quantized_mu_law_companding.pcm_bias;

        // Checking the results are the same as in Table 2a, columns 3 and 6, https://www.itu.int/rec/T-REC-G.711-198811-I/en.
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 0)     == 0b11111111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 1)     == 0b11111110);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 31)    == 0b11101111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 95)    == 0b11011111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 223)   == 0b11001111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 479)   == 0b10111111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 991)   == 0b10101111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 2015)  == 0b10011111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 4063)  == 0b10001111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 7903)  == 0b10000000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, 8158)  == 0b10000000); // max PCM14 value, minus the 33 bias.

        // Checking the results are the same as in Table 2b, columns 3 and 6, https://www.itu.int/rec/T-REC-G.711-198811-I/en.
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -1)    == 0b01111110);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -31)   == 0b01101111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -95)   == 0b01011111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -223)  == 0b01001111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -479)  == 0b00111111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -991)  == 0b00101111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -2015) == 0b00011111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -4063) == 0b00001111);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -7647) == 0b00000001);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -7903) == 0b00000000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_compression(params, -8158) == 0b00000000); // min PCM14 value, minus the 33 bias.

        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_mu_law_compression_shaper(params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_mu_law_compression_shaper(params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_mu_law_compression_shaper(params, 1.0f), 1.0f));

        // Expansion ---------------------------------------------------------------------------------------------------------------------
        params->continuous_mu_law_expansion.expansion = 255.0f;
        params->continuous_mu_law_expansion.expansion_reciprocal = 1.0f / params->continuous_mu_law_expansion.expansion;

        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_mu_law_expansion(params, -1.0f), -1.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_mu_law_expansion(params, 0.0f), 0.0f));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::continuous_mu_law_expansion(params, 1.0f), 1.0f));

        params->quantized_mu_law_companding.pcm_n_bits = 14;
        params->quantized_mu_law_companding.pcm_clip = (1 << (params->quantized_mu_law_companding.pcm_n_bits - 1)) - 1;
        params->quantized_mu_law_companding.pcm_bias = 33;
        params->quantized_mu_law_companding.pcm_max_magnitude = params->quantized_mu_law_companding.pcm_clip - params->quantized_mu_law_companding.pcm_bias;

        // Checking that the patterns in https://en.wikipedia.org/wiki/G.711#%CE%BC-law are correct.
        // Things get a bit weird because the first 7 bits are flipped in μ-law.
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b11110101) == +0b00000000110101);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b11100101) == +0b00000001101010);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b11010101) == +0b00000011010100);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b11000101) == +0b00000110101000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b10110101) == +0b00001101010000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b10100101) == +0b00011010100000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b10010101) == +0b00110101000000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b10000101) == +0b01101010000000);

        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b01110101) == -0b00000000110101);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b01100101) == -0b00000001101010);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b01010101) == -0b00000011010100);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b01000101) == -0b00000110101000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b00110101) == -0b00001101010000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b00100101) == -0b00011010100000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b00010101) == -0b00110101000000);
        UTEST_ASSERT(dspu::shaping::quantized_mu_law_expansion(0b00000101) == -0b01101010000000);

        // The maximum expanded value is 0b01111110000000 = 8064, so the peak of the quantized expander is 8064 / pcm_clip
        float expanded_peak = 8064.0f / static_cast<float>(params->quantized_mu_law_companding.pcm_clip);
        // The minimum expanded value is 0b00000000100001 = 33, so the minimum of the quantized expander is 33 / pcm_clip
        float expanded_min = 33.0f / static_cast<float>(params->quantized_mu_law_companding.pcm_clip);
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_mu_law_expansion_shaper(params, -1.0f), -expanded_peak));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_mu_law_expansion_shaper(params, 0.0f), expanded_min));
        UTEST_ASSERT(float_equals_absolute(dspu::shaping::quatized_mu_law_expansion_shaper(params, 1.0f), expanded_peak));
    }

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

        test_a_law(&params);
        test_mu_law(&params);

        // TODO: Add TAP Tubewarmth test.
    }

UTEST_END;
