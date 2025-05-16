/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_CONST_H_
#define LSP_PLUG_IN_DSP_UNITS_CONST_H_

#define LSP_DSP_UNITS_DEFAULT_SAMPLE_RATE           48000               /* Default sample rate                              */
#define LSP_DSP_UNITS_AIR_ADIABATIC_INDEX           1.4                 /* Adiabatic index for the Air                      */
#define LSP_DSP_UNITS_AIR_MOLAR_MASS                28.98               /* Molar mass of the air [g/mol]                    */
#define LSP_DSP_UNITS_GAS_CONSTANT                  8.3144598           /* Gas constant [ j/(mol * K) }                     */
#define LSP_DSP_UNITS_TEMP_ABS_ZERO                 -273.15             /* Temperature of the absolute zero [ C ]           */
#define LSP_DSP_UNITS_SPEC_FREQ_MIN                 10.0f               /* Minimum frequency range [ Hz ]                   */
#define LSP_DSP_UNITS_SPEC_FREQ_MAX                 24000.0f            /* Maximum frequency range [ Hz ]                   */
#define LSP_DSP_UNITS_SOUND_SPEED_M_S               340.29f             /* The default sound speed [ m / s ]                */

// Gain levels in decibels
#define GAIN_AMP_P_120_DB                   1e+6                /* +120 dB      */
#define GAIN_AMP_P_108_DB                   2.51189e+5          /* +108 dB      */
#define GAIN_AMP_P_96_DB                    3.98107e+4          /* +96 dB       */
#define GAIN_AMP_P_84_DB                    1.5849e+4           /* +84 dB       */
#define GAIN_AMP_P_72_DB                    3981.073            /* +72 dB       */
#define GAIN_AMP_P_60_DB                    1000.0              /* +60 dB       */
#define GAIN_AMP_P_48_DB                    251.18861           /* +48 dB       */
#define GAIN_AMP_P_36_DB                    63.09575            /* +36 dB       */
#define GAIN_AMP_P_24_DB                    15.84893            /* +24 dB       */
#define GAIN_AMP_P_18_DB                    7.943282            /* +18 dB       */
#define GAIN_AMP_P_16_DB                    6.30957             /* +16 dB       */
#define GAIN_AMP_P_12_DB                    3.98107             /* +12 dB       */
#define GAIN_AMP_P_11_DB                    3.54813             /* +11 dB       */
#define GAIN_AMP_P_9_DB                     2.81838             /* +9 dB        */
#define GAIN_AMP_P_7_DB                     2.23872             /* +7 dB        */
#define GAIN_AMP_P_6_DB                     1.99526             /* +6 dB        */
#define GAIN_AMP_P_5_DB                     1.77828             /* +5 dB        */
#define GAIN_AMP_P_3_DB                     1.41254             /* +3 dB        */
#define GAIN_AMP_P_2_DB                     1.25896             /* +2 dB        */
#define GAIN_AMP_P_1_DB                     1.12202             /* +1 dB        */
#define GAIN_AMP_0_DB                       1.0                 /* 0 dB         */
#define GAIN_AMP_M_1_DB                     0.89125             /* -1 dB        */
#define GAIN_AMP_M_2_DB                     0.79433             /* -2 dB        */
#define GAIN_AMP_M_3_DB                     0.707946            /* -3 dB        */
#define GAIN_AMP_M_4_DB                     0.63096             /* -4 dB        */
#define GAIN_AMP_M_5_DB                     0.56234             /* -5 dB        */
#define GAIN_AMP_M_6_DB                     0.50118             /* -6 dB        */
#define GAIN_AMP_M_7_DB                     0.44668             /* -7 dB        */
#define GAIN_AMP_M_8_DB                     0.39811             /* -8 dB        */
#define GAIN_AMP_M_9_DB                     0.354813            /* -9 dB        */
#define GAIN_AMP_M_10_DB                    0.31623             /* -10 dB       */
#define GAIN_AMP_M_11_DB                    0.28184             /* -11 dB       */
#define GAIN_AMP_M_12_DB                    0.25119             /* -12 dB       */
#define GAIN_AMP_M_18_DB                    0.12589             /* -18 dB       */
#define GAIN_AMP_M_24_DB                    0.06310             /* -24 dB       */
#define GAIN_AMP_M_30_DB                    0.03162             /* -30 dB       */
#define GAIN_AMP_M_36_DB                    0.01585             /* -36 dB       */
#define GAIN_AMP_M_42_DB                    7.943282e-3         /* -42 dB       */
#define GAIN_AMP_M_48_DB                    3.98107e-3          /* -48 dB       */
#define GAIN_AMP_M_60_DB                    0.001               /* -60 dB       */
#define GAIN_AMP_M_72_DB                    2.5119e-4           /* -72 dB       */
#define GAIN_AMP_M_84_DB                    6.309575e-5         /* -84 dB       */
#define GAIN_AMP_M_96_DB                    1.5849e-5           /* -96 dB       */
#define GAIN_AMP_M_108_DB                   3.98107e-6          /* -60 dB       */
#define GAIN_AMP_M_120_DB                   1e-6                /* -120 dB      */
#define GAIN_AMP_M_132_DB                   2.5119e-7           /* -132 dB      */
#define GAIN_AMP_M_144_DB                   6.309575e-8         /* -144 dB      */
#define GAIN_AMP_M_INF_DB                   0.0                 /* -inf dB      */

#define GAIN_AMP_MIN                        1e-6
#define GAIN_AMP_MAX                        1e+6

#define GAIN_AMP_M_20_DB                    0.1                 /* -20 dB       */
#define GAIN_AMP_M_40_DB                    0.01                /* -40 dB       */
#define GAIN_AMP_M_80_DB                    0.0001              /* -80 dB       */
#define GAIN_AMP_M_100_DB                   0.00001             /* -100 dB      */
#define GAIN_AMP_M_140_DB                   0.0000001           /* -140 dB      */
#define GAIN_AMP_M_160_DB                   0.00000001          /* -160 dB      */
#define GAIN_AMP_P_20_DB                    10.0                /* +20 dB       */
#define GAIN_AMP_P_40_DB                    100.0               /* +40 dB       */
#define GAIN_AMP_P_80_DB                    10000.0             /* +80 dB       */

#define GAIN_AMP_N_12_DB                    GAIN_AMP_M_12_DB
#define GAIN_AMP_N_24_DB                    GAIN_AMP_M_24_DB
#define GAIN_AMP_N_36_DB                    GAIN_AMP_M_36_DB
#define GAIN_AMP_N_48_DB                    GAIN_AMP_M_48_DB
#define GAIN_AMP_N_60_DB                    GAIN_AMP_M_60_DB
#define GAIN_AMP_N_72_DB                    GAIN_AMP_M_72_DB

// Gain steps
#define GAIN_AMP_S_0_1_DB                   0.01157945426
#define GAIN_AMP_S_0_5_DB                   0.05925372518
#define GAIN_AMP_S_1_DB                     0.1220184543

// Float saturation limits
#define FLOAT_SAT_P_NAN                     0.0f
#define FLOAT_SAT_N_NAN                     0.0f
#define FLOAT_SAT_P_INF                     1e+10f
#define FLOAT_SAT_N_INF                     -1e+10f
#define FLOAT_SAT_M_INF                     1e-10f
#define FLOAT_SAT_MN_INF                   -1e-10f
#define FLOAT_SAT_P_NAN_I                   0
#define FLOAT_SAT_N_NAN_I                   0
#define FLOAT_SAT_P_INF_I                   0x501502f9
#define FLOAT_SAT_N_INF_I                   0xd01502f9


#endif /* LSP_PLUG_IN_DSP_UNITS_CONST_H_ */
