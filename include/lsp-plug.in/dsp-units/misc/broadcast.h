/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 19 сент. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_BROADCAST_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_BROADCAST_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

/*
 * This header contains different constants related to such broadcasting service
 * recommendations as:
 *   * ITU BS.1770-4
 *   * ITU BS.2051-3
 *   * EBU R.128
 *   * IEC 61672:2003
 */
namespace lsp
{
    namespace dspu
    {
        namespace bs
        {
            /**
             * Differenf frequency weighting functions
             */
            enum weighting_t
            {
                WEIGHT_NONE,    // Flat response
                WEIGHT_A,       // IEC 61672:2003 A-weighting
                WEIGHT_B,       // IEC 61672:2003 B-weighting
                WEIGHT_C,       // IEC 61672:2003 C-weighting
                WEIGHT_D,       // IEC 61672:2003 D-weighting
                WEIGHT_K        // ITU BS.1770-4 K-weighting
            };

            /**
             * Different ITU BS.2051-3 channel designations
             */
            enum channel_t
            {
                CHANNEL_NONE,                   // No special designation
                CHANNEL_CENTER,                 // BS.2051-3 M+000
                CHANNEL_LEFT_SCREEN,            // BS.2051-3 M+SC
                CHANNEL_RIGHT_SCREEN,           // BS.2051-3 M-SC
                CHANNEL_LEFT,                   // BS.2051-3 M+030
                CHANNEL_RIGHT,                  // BS.2051-3 M-030
                CHANNEL_FRONT_LEFT,             // BS.2051-3 M+060
                CHANNEL_FRONT_RIGHT,            // BS.2051-3 M-060
                CHANNEL_LEFT_SIDE,              // BS.2051-3 M+090
                CHANNEL_RIGHT_SIDE,             // BS.2051-3 M-090
                CHANNEL_LEFT_SURROUND,          // BS.2051-3 M+110
                CHANNEL_RIGHT_SURROUND,         // BS.2051-3 M-110
                CHANNEL_LEFT_BACK,              // BS.2051-3 M+135
                CHANNEL_RIGHT_BACK,             // BS.2051-3 M-135
                CHANNEL_BACK_CENTER,            // BS.2051-3 M+180
                CHANNEL_TOP_FRONT_CENTER,       // BS.2051-3 U+000
                CHANNEL_LEFT_TOP_FRONT,         // BS.2051-3 U+030
                CHANNEL_RIGHT_TOP_FRONT,        // BS.2051-3 U-030
                CHANNEL_LEFT_HEIGHT,            // BS.2051-3 U+045
                CHANNEL_RIGHT_HEIGHT,           // BS.2051-3 U-045
                CHANNEL_TOP_SIDE_LEFT,          // BS.2051-3 U+090
                CHANNEL_TOP_SIDE_RIGHT,         // BS.2051-3 U-090
                CHANNEL_LEFT_TOP_REAR,          // BS.2051-3 U+110
                CHANNEL_RIGHT_TOP_REAR,         // BS.2051-3 U-110
                CHANNEL_TOP_BACK_LEFT,          // BS.2051-3 U+135
                CHANNEL_TOP_BACK_RIGHT,         // BS.2051-3 U-135
                CHANNEL_TOP_BACK_CENTER,        // BS.2051-3 U+180
                CHANNEL_CENTER_HEIGHT,          // BS.2051-3 UH+180
                CHANNEL_TOP_CENTER,             // BS.2051-3 T+000
                CHANNEL_CENTER_BOTTOM_FRONT,    // BS.2051-3 B+000
                CHANNEL_BOTTOM_FRONT_LEFT,      // BS.2051-3 B+045
                CHANNEL_BOTTOM_FRONT_RIGHT,     // BS.2051-3 B-045
                CHANNEL_LFE1,                   // BS.2051-3 LFE1
                CHANNEL_LFE2                    // BS.2051-3 LFE2
            };

            constexpr float DBFS_TO_LUFS_SHIFT_DB   = -0.691f;
            constexpr float LUFS_TO_DBFS_SHIFT_DB   =  0.691f;
            constexpr float LUFS_TO_LU_SHIFT_DB     = 23.0f;
            constexpr float LO_TO_LUFS_SHIFT_DB     = -23.0f;
            constexpr float DB_TO_LU_SHIFT_DB       = 22.309f;
            constexpr float LU_TO_DB_SHIFT          = -22.309f;

            constexpr float DBFS_TO_LUFS_SHIFT_GAIN = 0.923527857225f;
            constexpr float LUFS_TO_DBFS_SHIFT_GAIN = 1.08280437041f;
            constexpr float LUFS_TO_LU_SHIFT_GAIN   = 14.1253754462f;
            constexpr float LO_TO_LUFS_SHIFT_GAIN   = 0.0707945784385f;
            constexpr float DB_TO_LU_SHIFT_GAIN     = 13.0451777184f;
            constexpr float LU_TO_DB_SHIFT_GAIN     = 0.0766566789345f;

            /**
             * According to the standard, the default measuring period is 400 ms.
             */
            constexpr float LUFS_MEASURE_PERIOD_MS  = 400.0f;

            /**
             * The period of the momentary loudness in milliseconds
             */
            constexpr float LUFS_MOMENTARY_PERIOD   = 400.0f;

            /**
             * The period of the short-term loudness in milliseconds
             */
            constexpr float LUFS_SHORT_TERM_PERIOD  = 3000.0f;

            /**
             * Absolute gating threshold for Integrated Loudness, LKFS
             */
            constexpr float LUFS_GATING_ABS_THRESH_LKFS  = -70.0f;

            /**
             * Relative gating threshold for Integrated Loudness, LKFS
             */
            constexpr float LUFS_GATING_REL_THRESH_LKFS  = -10.0f;

            /**
             * Return the channel weighting coefficient accoding to BS.1770-4 recommendation
             * @param designation channel designation
             * @return channel weighting coefficient
             */
            LSP_DSP_UNITS_PUBLIC
            float channel_weighting(channel_t designation);

        } /* namespace bs */
    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_BROADCAST_H_ */
