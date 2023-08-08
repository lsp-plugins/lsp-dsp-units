/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 8 авг. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_MISC_FFT_CROSSOVER_H_
#define LSP_PLUG_IN_DSP_UNITS_MISC_FFT_CROSSOVER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace dspu
    {
        namespace crossover
        {
            /**
             * This header allows to generate characteristics of linear-phase crossover filters.
             */

            /**
             * Compute the magintude of the hi-pass crossover filter
             * @param f the array of frequencies
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @return the actual filter's magnitude at the specified frequency
             */
            LSP_DSP_UNITS_PUBLIC
            float hipass(float f, float f0, float slope);

            /**
             * Compute the magintude of the lo-pass crossover filter
             * @param f the array of frequencies
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @return the actual filter's magnitude at the specified frequency
             */
            LSP_DSP_UNITS_PUBLIC
            float lopass(float f, float f0, float slope);

            /**
             * Generate characteristics of the hipass crossover filter
             * @param gain the array to store output filter magintude for each frequency at the input
             * @param f the array of frequencies
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @param count the overall number of frequencies in the input array of frequencies
             */
            LSP_DSP_UNITS_PUBLIC
            void hipass_set(float *gain, const float *f, float f0, float slope, size_t count);

            /**
             * Apply characteristics of the hipass crossover filter, usual when building bandpass filter
             * from low-pass and high-pass filters.
             * @param gain the array to store output filter magintude for each frequency at the input
             * @param f the array of frequencies
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @param count the overall number of frequencies in the input array of frequencies
             */
            LSP_DSP_UNITS_PUBLIC
            void hipass_apply(float *gain, const float *f, float f0, float slope, size_t count);

            /**
             * Generate characteristics of the lowpass crossover filter
             * @param gain the array to store output filter magintude for each frequency at the input
             * @param f the array of frequencies
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @param count the overall number of frequencies in the input array of frequencies
             */
            LSP_DSP_UNITS_PUBLIC
            void lopass_set(float *gain, const float *f, float f0, float slope, size_t count);

            /**
             * Apply characteristics of the lowpass crossover filter, usual when building bandpass filter
             * from low-pass and high-pass filters.
             * @param gain the array to store output filter magintude for each frequency at the input
             * @param f the array of frequencies
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @param count the overall number of frequencies in the input array of frequencies
             */
            LSP_DSP_UNITS_PUBLIC
            void lopass_apply(float *gain, const float *f, float f0, float slope, size_t count);

            /**
             * Generate magnitude characteristics of the hipass crossover filter for FFT processing.
             * @param gain the array to store output filter magintude for each frequency at the input
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @param count the overall number of frequencies in the input array of frequencies
             */
            LSP_DSP_UNITS_PUBLIC
            void hipass_fft_set(float *mag, float f0, float slope, float sample_rate, size_t rank);

            /**
             * Apply magnitude characteristics of the hipass crossover filter for FFT processing, usual
             * when building bandpass filter from low-pass and high-pass filters.
             * @param gain the output filter gain for each frequency at the input, should be of 2^rank length.
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @param count the overall number of frequencies in the input array of frequencies
             */
            LSP_DSP_UNITS_PUBLIC
            void hipass_fft_apply(float *mag, float f0, float slope, float sample_rate, size_t rank);

            /**
             * Generate magnitude characteristics of the lowpass crossover filter for FFT processing.
             * @param gain the output filter gain for each frequency at the input, should be of 2^rank length.
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @param count the overall number of frequencies in the input array of frequencies
             */
            LSP_DSP_UNITS_PUBLIC
            void lopass_fft_set(float *mag, float f0, float slope, float sample_rate, size_t rank);

            /**
             * Apply magnitude characteristics of the lowpass crossover filter for FFT processing, usual
             * when building bandpass filter from low-pass and high-pass filters.
             * @param gain the output filter gain for each frequency at the input, should be of 2^rank length.
             * @param f0 the cut-off frequency of the filter, at which it gives -6 dB attenuation
             * @param slope the slope of the filter, attenuation in dB/octave. Negative value. If slope
             * is greater than -3 dB, it is considered to be no attenuation after -6 dB fall-off
             * @param count the overall number of frequencies in the input array of frequencies
             */
            LSP_DSP_UNITS_PUBLIC
            void lopass_fft_apply(float *mag, float f0, float slope, float sample_rate, size_t rank);

        } /* namespace crossover */
    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_MISC_FFT_CROSSOVER_H_ */
