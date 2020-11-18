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

#ifndef LSP_PLUG_IN_DSP_UNITS_FUNC_H_
#define LSP_PLUG_IN_DSP_UNITS_FUNC_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/const.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        /** Convert temperature from Celsium degrees to sound speed [m/s]
         *
         * @param temp temperature [ Celsium degrees ]
         * @return sound speed [m/s]
         */
        inline float sound_speed(float temp)
        {
            return sqrtf(
                    LSP_DSP_UNITS_AIR_ADIABATIC_INDEX *
                    LSP_DSP_UNITS_GAS_CONSTANT *
                    (temp - LSP_DSP_UNITS_TEMP_ABS_ZERO) * 1000.0f /* g/kg */ /
                    LSP_DSP_UNITS_AIR_MOLAR_MASS
            );
        }

        /** Convert samples [samp] to time [s]
         *
         * @param sr sample rate [samp/s]
         * @param samples number of samples [samp]
         * @return time [s]
         */
        inline float samples_to_seconds(float sr, float samples)
        {
            return samples / sr;
        }

        /** Convert time [s] to samples [samp]
         *
         * @param sr sample rate [samp/s]
         * @param time [s]
         * @return samples [samp]
         */
        inline float seconds_to_samples(float sr, float time)
        {
            return time * sr;
        }

        /** Convert samples [samp] to milliseconds [ms]
         *
         * @param sr sample rate
         * @param samples number of samples
         * @return milliseconds
         */
        inline float samples_to_millis(float sr, float samples)
        {
            return (samples / sr) * 1000.0f;
        }

        /** Convert samples [samp] to distance [m]
         *
         * @param sr sample rate [samp/s]
         * @param speed sound speed [m/s]
         * @param samples number of samples [samp]
         * @return distance [m]
         */
        inline float samples_to_meters(float sr, float speed, float samples)
        {
            return (samples * speed) / sr;
        }

        /** Convert samples [samp] to distance [cm]
         *
         * @param sr sample rate [samp/s]
         * @param speed sound speed [m/s]
         * @param samples number of samples [samp]
         * @return distance [cm]
         */
        inline float samples_to_centimeters(float sr, float speed, float samples)
        {
            return ((samples * speed) / sr) * 100.0f;
        }

        /** Convert time [ms] to samples [samp]
         *
         * @param sr sample rate [samp/s]
         * @param time time [ms]
         * @return samples [samp]
         */
        inline float millis_to_samples(float sr, float time)
        {
            return (time * 0.001f) * sr;
        }

        /** Convert decibels to gain value
         *
         * @param db decibels
         * @return gain
         */
        inline float db_to_gain(float db)
        {
            return expf(db * M_LN10 * 0.05f);
        }

        /** Convert decibels to power value
         *
         * @param db decibels
         * @return power
         */
        inline float db_to_power(float db)
        {
            return expf(db * M_LN10 * 0.1f);
        }

        /** Convert gain value to decibels
         *
         * @param gain gain value
         * @return decibels
         */
        inline float gain_to_db(float gain)
        {
            return (20.0f / M_LN10) * logf(gain);
        }

        /** Convert powerr value to decibels
         *
         * @param pwr power value
         * @return decibels
         */
        inline float power_to_db(float pwr)
        {
            return (10.0f / M_LN10) * logf(pwr);
        }
    }
}

#endif /* LSP_PLUG_IN_DSP_UNITS_FUNC_H_ */
