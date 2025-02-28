/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UNITS_H_
#define LSP_PLUG_IN_DSP_UNITS_UNITS_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/const.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        constexpr float NEPER_PER_DB        = 0.1151277918f;
        constexpr float DB_PER_NEPER        = 8.6860000037f;
        constexpr float NOTE_OUT_OF_RANGE   = -1e6f;

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

        /** Convert period of specified frequency to samples [samp]
         *
         * @param sr sample rate [samp/s]
         * @param freq frequency [hz]
         * @return samples [samp]
         */
        inline float hz_to_samples(float sr, float freq)
        {
            return sr / freq;
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

        /** Convert decibels to nepers
         *
         * @param db decibels
         * @return nepers
         */
        inline float db_to_neper(float db)
        {
            return db * NEPER_PER_DB;
        }

        /** Convert decibels to LUFS
         *
         * @param db decibels
         * @return LUFS
         */
        inline float db_to_lufs(float db)
        {
            return db - 0.691;
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

        /**
         * Convert gain value to LKFS/LUFS
         * @param gain gain value to convert
         * @return LUFS value
         */
        inline float gain_to_lufs(float gain)
        {
            return (20.0f / M_LN10) * logf(gain) - 0.691f;
        }

        /**
         * Convert the LKFS/LUFS value to gain value
         * @param lufs LUFS value to convert
         * @return gain value
         */
        inline float lufs_to_gain(float lufs)
        {
            return expf((lufs + 0.691f) * M_LN10 * 0.05f);
        }

        /** Convert LKFS/LUFS to power value
         *
         * @param lufs LUFS value to convert
         * @return power
         */
        inline float lufs_to_power(float lufs)
        {
            return expf((lufs + 0.691f) * M_LN10 * 0.1f);
        }

        /** Convert LKFS/LUFS to nepers
         *
         * @param lufs LUFS value to convert
         * @return nepers
         */
        inline float lufs_to_neper(float lufs)
        {
            return (lufs + 0.691f) * NEPER_PER_DB;
        }

        /** Convert LUFS to decibels
         *
         * @param lufs LUFS value
         * @return decibel value
         */
        inline float lufs_to_db(float lufs)
        {
            return lufs + 0.691;
        }

        /**
         * Convert gain value to loudness units
         * @param gain gain value to convert
         * @return LUFS value
         */
        inline float gain_to_lu(float gain)
        {
            return (20.0f / M_LN10) * logf(gain) + 22.309f;
        }

        /**
         * Convert the loudness units value to gain value
         * @param lu LU value to convert
         * @return gain value
         */
        inline float lu_to_gain(float lu)
        {
            return expf((lu - 22.309f) * M_LN10 * 0.05f);
        }

        /** Convert nepers to gain value
         *
         * @param neper nepers
         * @return gain
         */
        inline float neper_to_gain(float neper)
        {
            return db_to_gain(neper * DB_PER_NEPER);
        }

        /** Convert nepers to power value
         *
         * @param neper nepers
         * @return power
         */
        inline float neper_to_power(float neper)
        {
            return db_to_power(neper * DB_PER_NEPER);
        }

        /** Convert nepers to decibels
         *
         * @param neper nepers
         * @return decibels
         */
        inline float neper_to_db(float neper)
        {
            return neper * DB_PER_NEPER;
        }

        /** Convert nepers to LUFS value
         *
         * @param neper nepers
         * @return LUFS value
         */
        inline float neper_to_lufs(float neper)
        {
            return neper * DB_PER_NEPER - 0.691f;
        }

        /** Convert gain value to nepers
         *
         * @param gain gain value
         * @return nepers
         */
        inline float gain_to_neper(float gain)
        {
            return gain_to_db(gain) * NEPER_PER_DB;
        }

        /** Convert power value to nepers
         *
         * @param pwr power value
         * @return nepers
         */
        inline float power_to_neper(float pwr)
        {
            return power_to_db(pwr) * NEPER_PER_DB;
        }

        /**
         * Convert relative musical shift expressed in semitones to frequency shift multiplier
         * @param pitch relative pitch expressed in semitones
         * @return frequency multiplication coefficient
         */
        inline float semitones_to_frequency_shift(float pitch)
        {
            return expf(pitch * (M_LN2 / 12.0f));
        }

        /**
         * Convert the frequency shift multiplier to relative musical shift expressed in semitones
         * @param pitch frequency multiplication coefficient
         * @return relative pitch expressed in semitones
         */
        inline float frequency_shift_to_semitones(float pitch)
        {
            return (12.0f / M_LN2) * logf(pitch);
        }

        /**
         * Compute the frequency of the note relying on the frequency of the A4 note
         * @param note the musical note MIDI code
         * @param a4 the frequency of the A4 note, typically 440 Hz
         * @return the frequency of the note
         */
        inline float midi_note_to_frequency(ssize_t note, float a4 = 440.0f)
        {
            float pitch = ssize_t(note) - 69; // The MIDI number of the A4 note is 69
            return a4 * semitones_to_frequency_shift(pitch);
        }

        /**
         * Compute the note that matches the specified frequency with the cent precision detune
         *
         * @param f frequency to perform the computation
         * @param a4 the frequency of the A4 note, typically 440 Hz
         * @return the floating-point value for note or the NOTE_OUT_OF_RANGE if the note can not be decoded.
         * @note the integer part of note matches the MIDI note range but belongs to the extended note range,
         *   so it may come out of the possible MIDI range 0-127 (negative values are possible, too).
         *   The fraction part defines the detune of the note.
         */
        inline float frequency_to_note(float f, float a4 = 440.0f)
        {
            if ((f < 10.0f) || (f > 24000.0f))
                return NOTE_OUT_OF_RANGE;

            return frequency_shift_to_semitones(f/a4) + 69.0f;
        }

        /**
         * Convert the time signature for the specific tempo to frequency
         * @param fraction the fration of the time signature
         * @param tempo tempo
         * @return frequency in Hz
         */
        inline float time_signature_to_frequency(float fraction, float tempo)
        {
            return tempo / (240.0f * fraction);
        }

        /**
         * Convert the time signature for the specific tempo to time period
         * @param fraction the fration of the time signature
         * @param tempo tempo
         * @return time period in seconds
         */
        inline float time_signature_to_period(float fraction, float tempo)
        {
            return (240.0f * fraction) / tempo;
        }

    } /* namespace dspu */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_DSP_UNITS_UNITS_H_ */
