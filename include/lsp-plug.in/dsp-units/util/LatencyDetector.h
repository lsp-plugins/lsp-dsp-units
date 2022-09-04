/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 5 Apr 2017
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

#ifndef LSP_PLUG_IN_DSP_UNITS_UTIL_LATENCYDETECTOR_H_
#define LSP_PLUG_IN_DSP_UNITS_UTIL_LATENCYDETECTOR_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

#define DEFAULT_ABS_THRESHOLD   0.01f
#define DEFAULT_PEAK_THRESHOLD  0.5f

namespace lsp
{
    namespace dspu
    {
        class LSP_DSP_UNITS_PUBLIC LatencyDetector
        {
            private:
                LatencyDetector & operator = (const LatencyDetector &);
                LatencyDetector(const LatencyDetector &);

            protected:

                // Input processor state enumerator
                enum ip_state_t
                {
                    IP_BYPASS,  // Bypassing the signal
                    IP_WAIT,    // Bypassing while the Output Processor fades out and emits zeros
                    IP_DETECT   // Receiving input samples and attempting latency detection
                };

                // Output processor state enumerator
                enum op_state_t
                {
                    OP_BYPASS,  // Bypassing the signal
                    OP_FADEOUT, // Fading out the signal
                    OP_PAUSE,   // Emitting zeros
                    OP_EMIT,    // Emitting the chirp samples
                    OP_FADEIN   // Fading in the signal
                };

                // Chirp System parameters
                typedef struct chirp_t
                {
                    float       fDuration;              // Chirp Duration [seconds]
                    float       fDelayRatio;            // Fraction of fChirpDuration defining 0 Hz group delay of the chirp system
                    bool        bModified;              // If any of the parameters above is modified, mark for Chirp/Antichirp recalculation

                    size_t      nDuration;              // Chirp Duration [samples] (not of the whole FIR response, only the part containing most of the chirp)

                    size_t      n2piMult;               // Integer multiplier of 2 * M_PI. Identifies the value of phase at Nyquist frequency
                    float       fAlpha;                 // Coefficient of the linear term of the phase response
                    float       fBeta;                  // Coefficient of the quadratic term of the phase response
                    size_t      nLength;                // Length of the FIR (number of samples). Equals Order + 1
                    size_t      nOrder;                 // Order of the FIR
                    size_t      nFftRank;               // Rank of the inverse FFT to obtain time domain samples

                    float       fConvScale;             // Scale factor to normalise convolution values
                } chirp_t;

                // Input Processor parameters
                typedef struct ip_t
                {
                    ip_state_t  nState;                 // State
                    size_t      ig_time;                // Global Time counter
                    size_t      ig_start;               // Fix instant at which detection starts
                    size_t      ig_stop;                // Fix instant at which detection ends

                    float       fDetect;                // Detection duration
                    size_t      nDetect;                // Detection length
                    size_t      nDetectCounter;         // Count samples in input when in IP_DETECT state
                } ip_t;

                // Output Processor parameters
                typedef struct op_t
                {
                    op_state_t  nState;                 // State
                    size_t      og_time;                // Global Time counter
                    size_t      og_start;               // Fix instant at which detection starts

                    float       fGain;                  // Fading gain
                    float       fGainDelta;             // Fading gain delta

                    float       fFade;                  // Fade time [seconds]
                    size_t      nFade;                  // Fade time [samples]

                    float       fPause;                 // Pause duration [seconds]
                    size_t      nPause;                 // Pause duration [samples]
                    size_t      nPauseCounter;          // Count samples in output when in OP_PAUSE state

                    size_t      nEmitCounter;           // Count samples in output when in OP_EMIT state
                } op_t;

                // Peak Detection parameters
                typedef struct peak_t
                {
                    float       fAbsThreshold;          // Absolute detection threshold
                    float       fPeakThreshold;         // Relative threshold between peaks (higher delta between recorded peaks will trigger early detection)
                    float       fValue;                 // Value of the detected peak (absolute)
                    size_t      nPosition;              // Position of the detected peak (referenced to sample counters)
                    size_t      nTimeOrigin;            // This should be the sample at which the convolution peak as in case 0 delay.
                    bool        bDetected;              // True if the peak was detected.
                } peak_t;

            private:
                size_t          nSampleRate;            // Sample Rate [Hz]

                chirp_t         sChirpSystem;

                ip_t            sInputProcessor;
                op_t            sOutputProcessor;

                peak_t          sPeakDetector;          // Object tracking the peak of convolution.

                float          *vChirp;                 // Samples of the chirp system impulse response
                float          *vAntiChirp;             // Samples of the anti-chirp system impulse response
                float          *vCapture;               // Hold samples captured from audio input
                float          *vBuffer;                // Temporary buffer to apply convolution
                float          *vChirpConv;             // Chirp fast convolution image
                float          *vConvBuf;               // Temporary convolution buffer
                uint8_t        *pData;

                bool            bCycleComplete;         // True if the machine operated a whole measurement cycle
                bool            bLatencyDetected;       // True if latency was detected
                ssize_t         nLatency;               // Value of latency in samples. Signed so that -1 is meaningful

                bool            bSync;

            protected:
                void detect_peak(float *buf, size_t count);

            public:
                explicit LatencyDetector();
                ~LatencyDetector();

                /**
                 * Construct object
                 */
                void construct();

                /** Initialise LatencyDetector
                 *
                 */
                void init();

                /** Destroy LatencyDetector
                 *
                 */
                void destroy();

            public:
                /** Check that LatencyDetector needs settings update
                 *
                 * @return true if LatencyDetector needs setting update
                 */
                inline bool needs_update() const
                {
                    return bSync;
                }

                /** Update LatencyDetector stateful settings
                 *
                 */
                void update_settings();

                /** Set sample rate for the LatencyDetector
                 *
                 * @param sr sample rate
                 */
                inline void set_sample_rate(size_t sr)
                {
                    if (nSampleRate == sr)
                        return;

                    nSampleRate = sr;
                    bSync       = true;
                }

                /** Set chirp duration in seconds
                 *
                 * @param duration chirp duration in seconds
                 */
                inline void set_duration(float duration)
                {
                    if (sChirpSystem.fDuration == duration)
                        return;

                    sChirpSystem.fDuration = duration;
                    sChirpSystem.bModified = true;
                    bSync     = true;
                }

                /** Set 0 Hz Group Delay for chirp
                 *
                 * @param ratio 0 Hz Group Delay as fraction of duration
                 */
                void set_delay_ratio(float ratio);

                /** Set chirp pause in seconds
                 *
                 * @param pause pause duration in seconds
                 */
                inline void set_op_pause(float pause)
                {
                    if (sOutputProcessor.fPause == pause)
                        return;

                    sOutputProcessor.fPause = pause;
                    bSync     = true;
                }

                /** Set chirp fading in seconds
                 *
                 * @param fading fading duration in seconds
                 */
                inline void set_op_fading(float fading)
                {
                    if (sOutputProcessor.fFade == fading)
                        return;

                    sOutputProcessor.fFade = fading;
                    bSync     = true;
                }

                /** Set chirp detection in seconds
                 *
                 * @param detect overall detection time in seconds
                 */
                inline void set_ip_detection(float detect)
                {
                    if (sInputProcessor.fDetect == detect)
                        return;

                    sInputProcessor.fDetect = detect;
                    bSync     = true;
                }

                /** Set peak detector absolute detection threshold
                 *
                 * @param threshold absolute threshold
                 */
                void set_abs_threshold(float threshold);

                /** Set peak detector relative threshold
                 *
                 * @param threshold relative threshold
                 */
                void set_peak_threshold(float threshold);

                /** Start latency detection process
                 *
                 */
                void start_capture();

                /** Force the chirp system to reset it's state
                 *
                 */
                void reset_capture();

                /** Get chirp duration in samples
                 *
                 * @return chirp duration in samples
                 */
                inline size_t get_duration_samples() const
                {
                    return sChirpSystem.nDuration;
                }

                /** Get chirp duration in seconds
                 *
                 * @return chirp duration in seconds
                 */
                float get_duration_seconds() const;

                /** Return true if the measurement cycle was completed
                 *
                 * @return bCycleComplete value
                 */
                inline bool cycle_complete() const
                {
                    return bCycleComplete;
                }

                /** Return true if the latency was detected
                 *
                 * @return bLatencyDetected value
                 */
                inline bool latency_detected() const
                {
                    return bLatencyDetected;
                }

                /** Get latency in samples
                 *
                 * @return latency in samples
                 */
                inline ssize_t get_latency_samples() const
                {
                    return (bCycleComplete) ? nLatency : -1;
                }

                /** Get latency in seconds
                 *
                 * @return latency in seconds
                 */
                float get_latency_seconds() const;

            public:
                /** Stream direct chirp while recording response
                 *
                 * @param dst samples destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process(float *dst, const float *src, size_t count);

                /** Collect input samples:
                 *
                 * @param dst samples destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process_in(float *dst, const float *src, size_t count);

                /** Stream output samples:
                 *
                 * @param dst samples destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process_out(float *dst, const float *src, size_t count);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
}

#endif /* LSP_PLUG_IN_DSP_UNITS_UTIL_LATENCYDETECTOR_H_ */
