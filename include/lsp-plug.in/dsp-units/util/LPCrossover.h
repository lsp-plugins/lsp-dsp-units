/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 29 авг. 2026 г.
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_LPCROSSOVER_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_LPCROSSOVER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/util/Crossover.h>
#include <lsp-plug.in/dsp-units/util/SpectralSplitter.h>

namespace lsp
{
    /*
         The overall schema of signal processing by the linear phase crossover for 4 bands
         (filters are following in order of the increasing frequency):

        INPUT = Input signal passed to the process() method
        LPF   = Low-pass filter
        HPF   = High-pass filter
        OUT   = Output signal for the particular band passed to the crossover_func_t callback function

       ┌─────┐     ┌─────┐                             ┌─────┐
       │INPUT│──┬─►│LPF 0│────────────────────────────►│OUT 0│
       └─────┘  │  └─────┘                             └─────┘
                │
                │
                │  ┌─────┐                             ┌─────┐
                └─►│HPF 0│──┬─────────────────────────►│OUT 1│
                   └─────┘  │                          └─────┘
                            │
                            │
                            │  ┌─────┐     ┌─────┐     ┌─────┐
                            └─►│HPF 1│──┬─►│LPF 2│────►│OUT 2│
                               └─────┘  │  └─────┘     └─────┘
                                        │
                                        │
                                        │  ┌─────┐     ┌─────┐
                                        └─►│HPF 2│────►│OUT 3│
                                           └─────┘     └─────┘
     */

    namespace dspu
    {
        /**
         * Linear Phase Crossover, splits signal into bands, calls processing handler (if present)
         * and mixes processed bands back after adjusting the post-processing amplification gain.
         * This crossover uses the same functionality as FFTCrossover but provides the interface of
         * the classic crossover. It uses precise filter transfer functions, so all enabled frequency
         * bands with 0 dB gain amplification give the flat response while FFTCrossover can result
         * in frequency boost for specific frequency ranges.
         */
        class LSP_DSP_UNITS_PUBLIC LPCrossover
        {
            protected:
                enum reconfigure_t
                {
                    R_SPLIT         = 1 << 0,           // Need to reconfigure processing plan

                    R_ALL           = R_SPLIT
                };

                typedef struct split_t
                {
                    uint32_t            nBandId;        // Number of split point
                    float               fSlope;         // Filter slope (0 = off)
                    float               fConfigFreq;    // Configured frequency
                    float               fFreq;          // Real frequency (limited by sample rate)
                } split_t;

                typedef struct band_t
                {
                    uint32_t            nId;            // Number of the band
                    float               fGain;          // Output gain of the band
                    float               fStart;         // Start frequency of the band
                    float               fEnd;           // End frequency of the band
                    bool                bEnabled;       // Enabled flag
                    split_t            *pStart;         // Pointer to starting split point
                    split_t            *pEnd;           // Pointer to ending split point

                    void               *pObject;        // Bound object
                    void               *pSubject;       // Bound subject
                    crossover_func_t    pFunc;          // Function
                    float              *vFFT;           // FFT frequency transfer function
                } band_t;

            protected:
                dspu::SpectralSplitter  sSplitter;      // Spectral splitter
                uint32_t                nSampleRate;    // Sample rate
                uint32_t                nPlanSize;      // Size of plan
                uint32_t                nReconfigure;   // Change flag
                split_t                *vSplits;        // List of splits
                band_t                 *vBands;         // The overall list of bands
                split_t               **vPlan;          // Split plan
                float                  *vBuffer;        // Temporary buffer for computation

                uint8_t                *pData;          // Unaligned data

            protected:
                void                            band_freq_chart(size_t band, float *m, const float *f, size_t count);
                void                            update_band_freq_responses();
                void                            sync_binding(band_t *b);

            protected:
                static void spectral_func(
                    void *object, void *subject,
                    float *out, const float *in,
                    size_t rank);

                static void spectral_sink(
                    void *object, void *subject,
                    const float *samples,
                    size_t first, size_t count);

            public:
                explicit LPCrossover();
                LPCrossover(const LPCrossover &) = delete;
                LPCrossover(LPCrossover &&) = delete;
                ~LPCrossover();

                LPCrossover & operator = (const LPCrossover &) = delete;
                LPCrossover & operator = (LPCrossover &&) = delete;

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
                inline size_t   num_bands() const                       { return sSplitter.handlers();      }

                /**
                 * Get number of split points
                 * @return number of split points
                 */
                inline size_t   num_splits() const                      { return sSplitter.handlers() - 1;  }

                /**
                 * Set the FFT rank
                 * @param rank FFT rank
                 */
                void            set_rank(size_t rank);

                /**
                 * Get the FFT rank
                 * @return FFT rank
                 */
                inline size_t   rank() const                            { return sSplitter.rank();      }

                /**
                 * Set processing phase
                 * @param phase processing phase
                 */
                void            set_phase(float phase);

                /**
                 * Get processing phase
                 * @return processing phase
                 */
                inline float    phase() const                           { return sSplitter.phase();     }

                /**
                 * Get the latency of the processor
                 * @return the latency
                 */
                inline size_t   latency() const                         { return sSplitter.latency();   }

                /** Set slope of the split point
                 *
                 * @param sp split point number
                 * @param slope slope of crossover filters, negative value in decibels per octave.
                 * @return false if split point index is invalid
                 */
                bool            set_slope(size_t sp, float slope);

                /**
                 * Get slope of the split point
                 * @param sp split point number
                 * @return slope of the split point (negative value in decibels per octave),
                 * zero value means split point is disabled, positive value means invalid
                 * split point index.
                 */
                float           slope(size_t sp) const;

                /** Set frequency of split point
                 *
                 * @param sp split point number
                 * @param freq split frequency of the split point
                 * @return false if split point index is invalid
                 */
                bool            set_frequency(size_t sp, float freq);

                /**
                 * Get split frequency of the split point
                 * @param sp split point number
                 * @return split frequency of the split point, negative value
                 *         means invalid index
                 */
                float           frequency(size_t sp) const;

                /**
                 * Set gain of the specific output band
                 * @param band band number
                 * @param gain gain of the band
                 * @return false if band index is invalid
                 */
                bool            set_gain(size_t band, float gain);

                /**
                 * Get gain of the specific output band
                 * @param band band number
                 * @return gain of the band, negative value on invalid index
                 */
                float           gain(size_t band) const;

                /**
                 * Get start frequency of the band, may call reconfigure()
                 * @param band band number
                 * @return start frequency of the band or negative value on invalid index
                 */
                float           band_start(size_t band);

                /**
                 * Get end frequency of the band, may call reconfigure()
                 * @param band band number
                 * @return end frequency of the band or negative value on invalid index
                 */
                float           band_end(size_t band);

                /**
                 * Check that the band is active (always true for band 0), may call reconfigure()
                 * @param band band number
                 * @return true if band is active
                 */
                bool            band_active(size_t band);

                /** Set sample rate, needs reconfiguration
                 *
                 * @param sr sample rate to set
                 */
                void            set_sample_rate(size_t sr);

                /**
                 * Get sample rate of the crossover
                 * @return sample rate
                 */
                inline size_t   sample_rate() const                     { return nSampleRate;           }

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

                /**
                 * Check that we need to call reconfigure()
                 * @return true if we need to call reconfigure()
                 */
                inline bool     needs_update() const            { return nReconfigure != 0;     }

                /** Reconfigure crossover after parameter update
                 *
                 */
                void            update_settings();

                /**
                 * Clear internal memory
                 */
                inline void     clear()                             { sSplitter.clear();            }

                /**
                 * Clear internal memory
                 */
                inline void     reset()                             { sSplitter.clear();            }

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

                /** Process data and issue callbacks, automatically calls reconfigure()
                 * if the reconfiguration is required
                 *
                 * @param in input buffer to process data
                 * @param samples number of samples to process
                 */
                void            process(const float *in, size_t samples);

                /**
                 * Dump the state
                 * @param v state dumper dumper
                 */
                void            dump(IStateDumper *v) const;
        };
    } /* namespace dspu */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_LPCROSSOVER_H_ */
