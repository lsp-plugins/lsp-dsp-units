/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 9 авг. 2023 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_FFTCROSSOVER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_FFTCROSSOVER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/util/Crossover.h>
#include <lsp-plug.in/dsp-units/util/SpectralSplitter.h>

namespace lsp
{
    /*
        This class implements linear-phase crossover which uses FFT transform for splitting the signal
        into different spectrum bands.

        For more flexible configuration, each band can define it's own lopass and hipass filter parameters,
        so the result sum of band signals will repeat the original signal only when performing the right
        tuning of each band.

        This class acts as an adaptor around the SpectralSplitter class for more ease usage.
     */

    namespace dspu
    {
        /** FFT Crossover with linear phase, splits signal into bands and calls processing handlers
         * for each band (if present)
         *
         */
        class LSP_DSP_UNITS_PUBLIC FFTCrossover
        {
            protected:
                typedef struct band_t
                {
                    float               fHpfFreq;       // The cut-off frequency for hi-pass filter
                    float               fLpfFreq;       // The cut-off frequency for lo-pass filter
                    float               fHpfSlope;      // Slope for hi-pass filter
                    float               fLpfSlope;      // Slope for low-pass filter
                    float               fGain;          // The output gain of the band
                    float               fFlatten;       // Flatten
                    bool                bLpf;           // LPF enabled
                    bool                bHpf;           // HPF enabled
                    bool                bEnabled;       // Enable flag
                    bool                bUpdate;        // Update of FFT tranfer function is pending

                    void               *pObject;        // The object to pass to crossover function
                    void               *pSubject;       // The subject to pass to crossover function
                    crossover_func_t    pFunc;          // Callback function
                    float              *vFFT;           // FFT frequency transfer function
                } split_t;

            protected:
                dspu::SpectralSplitter  sSplitter;      // Spectral splitter
                band_t                 *vBands;         // The overall list of bands
                size_t                  nSampleRate;    // Sample rate

                uint8_t                *pData;          // Unaligned data

            protected:
                static void spectral_func(
                    void *object,
                    void *subject,
                    float *out,
                    const float *in,
                    size_t rank);

                static void spectral_sink(
                    void *object,
                    void *subject,
                    const float *samples,
                    size_t first,
                    size_t count);

            protected:
                void            update_band(band_t *b);

                void            sync_binding(size_t band, band_t *b);
                void            mark_bands_for_update();

            public:
                explicit FFTCrossover();
                FFTCrossover(const Crossover &) = delete;
                FFTCrossover(Crossover &&) = delete;
                ~FFTCrossover();

                FFTCrossover & operator = (const FFTCrossover &) = delete;
                FFTCrossover & operator = (FFTCrossover &&) = delete;

                /** Construct crossover
                 *
                 */
                void            construct();

                /** Destroy crossover
                 *
                 */
                void            destroy();

                /** Initialize crossover
                 *
                 * @param max_rank maximum FFT rank
                 * @param bands number of bands
                 * @return status of operation
                 */
                status_t        init(size_t max_rank, size_t bands);

            public:
                /**
                 * Get number of bands
                 * @return number of bands
                 */
                inline size_t   bands() const                           { return sSplitter.handlers();  }

                /** Set slope of both LPF and HPF of the specific band
                 *
                 * @param band the number of band
                 * @param lpf the slope of the LPF (dB/octave, negative)
                 * @param hpf the slope of the HPF (dB/octave, negative)
                 */
                void            set_slope(size_t band, float lpf, float hpf);

                /**
                 * Set slope for LPF of the specific band
                 * @param band the number of band
                 * @param slope the slope of the LPF (dB/octave, negative)
                 */
                void            set_lpf_slope(size_t band, float slope);

                /**
                 * Set slope for HPF of the specific band
                 * @param band the number of band
                 * @param slope the slope of the LPF (dB/octave, negative)
                 */
                void            set_hpf_slope(size_t band, float slope);

                /**
                 * Get slope of the LPF filter of the specific band
                 * @param band the number of band
                 * @return the slope of the LPF filter, negative value for invalid index
                 */
                float           lpf_slope(size_t band) const;

                /**
                 * Get slope of the HPF filter of the specific band
                 * @param band the number of band
                 * @return the slope of the HPF filter, negative value for invalid index
                 */
                float           hpf_slope(size_t band) const;


                /** Set cutoff frequency of both LPF and HPF of the specific band
                 *
                 * @param band the number of band
                 * @param lpf the cutoff frequency of the LPF
                 * @param hpf the cutoff frequency of the HPF
                 */
                void            set_frequency(size_t band, float lpf, float hpf);

                /**
                 * Set cutoff frequency for LPF of the specific band
                 * @param band the number of band
                 * @param freq the cutoff frequency of the LPF
                 */
                void            set_lpf_frequency(size_t band, float freq);

                /**
                 * Set cutoff frequency for HPF of the specific band
                 * @param band the number of band
                 * @param freq the cutoff frequency of the LPF, negative value for invalid index
                 */
                void            set_hpf_frequency(size_t band, float freq);

                /**
                 * Get cutoff frequency of the LPF filter of the specific band
                 * @param band the number of band
                 * @return the cutoff frequency of the LPF filter, negative value for invalid index
                 */
                float           lpf_frequency(size_t band) const;

                /**
                 * Get cutoff frequency of the HPF filter of the specific band
                 * @param band the number of band
                 * @return the cutoff frequency of the HPF filter
                 */
                float           hpf_frequency(size_t band) const;

                /**
                 * Enable LPF and HPF of specific band
                 * @param band the number of band
                 * @param lpf flag that enables LPF filter
                 * @param hpf flag that enables HPF filter
                 */
                void            enable_filters(size_t band, bool lpf, bool hpf);

                /**
                 * Enable LPF of specific band
                 * @param band the number of band
                 * @param enable flag that enables LPF
                 */
                void            enable_hpf(size_t band, bool enable = true);

                /**
                 * Enable HPF of specific band
                 * @param band the number of band
                 * @param enable flag that enables LPF
                 */
                void            enable_lpf(size_t band, bool enable = true);

                /**
                 * Check that the LPF is enabled for the specific band
                 * @param band the number of band
                 * @return true if LPF is enabled
                 */
                bool            lpf_enabled(size_t band) const;

                /**
                 * Check that the HPF is enabled for the specific band
                 * @param band the number of band
                 * @return true if HPF is enabled
                 */
                bool            hpf_enabled(size_t band) const;

                /**
                 * Disable LPF and HPF of specific band
                 * @param band the number of band
                 */
                inline void     disable_filters(size_t band) { enable_filters(band, false, false); }

                /**
                 * Disable LPF of specific band
                 * @param band the number of band
                 */
                inline void     disable_lpf(size_t band) { enable_lpf(band, false); }

                /**
                 * Disable HPF of specific band
                 * @param band the number of band
                 */
                inline void     disable_hpf(size_t band) { enable_hpf(band, false); }

                /**
                 * Set all parameters of the LPF for the specific band
                 * @param band the number of band
                 * @param freq the cutoff frequency of the filter
                 * @param slope the slope of the filter (dB/octave, negative)
                 * @param enabled enable flag
                 */
                void            set_lpf(size_t band, float freq, float slope, bool enabled = true);

                /**
                 * Set all parameters of the HPF for the specific band
                 * @param band the number of band
                 * @param freq the cutoff frequency of the filter
                 * @param slope the slope of the filter (dB/octave, negative)
                 * @param enabled enable flag
                 */
                void            set_hpf(size_t band, float freq, float slope, bool enabled = true);

                /**
                 * Set gain of the specific band
                 * @param band band number
                 * @param gain gain of the band
                 */
                void            set_gain(size_t band, float gain);

                /**
                 * Get gain of the specific band
                 * @param band band number
                 * @return gain of the band, negative value on invalid index
                 */
                float           gain(size_t band) const;

                /**
                 * Set flatten threshold of the band
                 * @param band band number
                 * @param amount the flatten amount in raw gain units
                 */
                void            set_flatten(size_t band, float amount);

                /**
                 * Get flatten threshold of the specific band
                 * @param band band number
                 * @return flatten of the band, in decibels
                 */
                float           flatten(size_t band) const;

                /**
                 * Enable the specific band
                 * @param band band to enable
                 * @param enable enable flag
                 */
                void            enable_band(size_t band, bool enable=true);

                /**
                 * Disable the specific band
                 * @param band band to disable
                 */
                inline void     disable_band(size_t band) { enable_band(band, false); }

                /**
                 * Check that the band is enabled
                 * @param band band number
                 * @return true if band is enabled
                 */
                bool            band_enabled(size_t band) const;

                /**
                 * Set band signal handler
                 * @param band band number
                 * @param func handler function
                 * @param object object to pass to function
                 * @param subject subject to pass to function
                 * @return false if invalid band number has been specified
                 */
                bool            set_handler(size_t band, crossover_func_t func, void *object, void *subject);

                /**
                 * Unset band signal handler
                 * @param band band number
                 * @return false if invalid band number has been specified
                 */
                bool            unset_handler(size_t band);

                /** Set sample rate, needs reconfiguration
                 *
                 * @param sr sample rate to set
                 */
                void            set_sample_rate(size_t sr);

                /**
                 * Get sample rate of the crossover
                 * @return sample rate
                 */
                inline size_t   sample_rate() const                 { return nSampleRate;           }

                /**
                 * Set the FFT rank
                 * @param rank FFT rank
                 */
                void            set_rank(size_t rank);

                /**
                 * Set processing phase
                 * @param phase processing phase
                 */
                void            set_phase(float phase);

                /**
                 * Get processing phase
                 * @return processing phase
                 */
                inline float    phase() const                       { return sSplitter.phase();     }

                /**
                 * Get the FFT rank
                 * @return FFT rank
                 */
                inline size_t   rank() const                        { return sSplitter.rank();      }

                /**
                 * Get the latency of the processor
                 * @return the latency
                 */
                inline size_t   latency() const                     { return sSplitter.latency();   }

                /** Get frequency chart of the crossover band. Because the crossover is linear-phase,
                 * it returns only magnitude of the spectrum.
                 *
                 * @param band number of the band
                 * @param m transfer function (magnitude values)
                 * @param f frequencies to calculate transfer functions
                 * @param count number of points for the chart
                 * @return false if invalid band index is specified
                 */
                bool            freq_chart(size_t band, float *m, const float *f, size_t count);

                /**
                 * Check that crossover needs to call reconfigure() before processing
                 * @return true if crossover needs to call reconfigure() before processing
                 */
                bool            needs_update() const;

                /** Reconfigure crossover after parameter update
                 *
                 */
                void            update_settings();

                /** Process data and issue callbacks, automatically calls reconfigure()
                 * if the reconfiguration is required
                 *
                 * @param in input buffer to process data
                 * @param samples number of samples to process
                 */
                void            process(const float *in, size_t samples);

                /**
                 * Clear internal memory
                 */
                inline void     clear()                             { sSplitter.clear();            }

                /**
                 * Dump the state
                 * @param v state dumper
                 */
                void            dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_FFTCROSSOVER_H_ */
