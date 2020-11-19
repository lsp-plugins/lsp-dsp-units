/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 30 Jul 2017
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

#ifndef LSP_PLUG_IN_DSP_UINTS_UTIL_RESPONSETAKER_H_
#define LSP_PLUG_IN_DSP_UINTS_UTIL_RESPONSETAKER_H_

#include <lsp-plug.in/dsp-units/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/common/status.h>

namespace lsp
{
    namespace dspu
    {
        class ResponseTaker
        {
            private:
                ResponseTaker & operator = (const ResponseTaker &);

            protected:
                // Input processor state enumerator
                enum ip_state_t
                {
                    IP_BYPASS,                  // Bypassing the signal
                    IP_WAIT,                    // Bypassing while the Output Processor fades out and emits zeros
                    IP_ACQUIRE                  // Receiving input samples and recording input
                };

                // Output processor state enumerator
                enum op_state_t
                {
                    OP_BYPASS,                  // Bypassing the signal
                    OP_FADEOUT,                 // Fading out the signal
                    OP_PAUSE,                   // Emitting zeros
                    OP_TEST_SIG_EMIT,           // Emitting the chirp samples
                    OP_TAIL_EMIT,               // Emitting the chirp zeros tail (to allow both latency shift and acquisition of reverberant tail into the capture buffer)
                    OP_FADEIN                   // Fading in the signal
                };

                // Input Processor parameters
                typedef struct ip_t
                {
                    ip_state_t  nState;         // State
                    size_t      ig_time;        // Global Time counter
                    size_t      ig_start;       // Fix instant at which acquisition starts
                    size_t      ig_stop;        // Fix instant at which acquisition ends

                    float       fAcquire;       // Acquisition duration (chirp + tail)
                    size_t      nAcquire;       // Acquisition length (chirp + tail)
                    size_t      nAcquireTime;   // Count samples in input when in IP_ACQUIRE state
                } ip_t;

                // Output Processor parameters
                typedef struct op_t
                {
                    op_state_t  nState;         // State
                    size_t      og_time;        // Global Time counter
                    size_t      og_start;       // Fix instant at which emission starts

                    float       fGain;          // Fading gain
                    float       fGainDelta;     // Fading gain delta

                    float       fFade;          // Fade time [s]
                    size_t      nFade;          // Fade time [samples]

                    float       fPause;         // Pause duration [s]
                    size_t      nPause;         // Pause duration [samples]
                    size_t      nPauseTime;     // Count samples in output when in OP_PAUSE state

                    float       fTail;          // Tail duration [s].
                    size_t      nTail;          // Tail duration [samples]
                    size_t      nTailTime;      // Count samples when in OP_TAIL_EMIT state

                    float       fTestSig;       // Test signal duration [s]
                    size_t      nTestSig;       // Test signal duration [samples]
                    size_t      nTestSigTime;   // Count samples in output when in OP_TEST_SIG_EMIT state
                } op_t;

            private:
                size_t      nSampleRate;

                ip_t        sInputProcessor;
                op_t        sOutputProcessor;

                Sample     *pTestSig;
                Sample     *pCapture;

                size_t      nLatency;           // Latency of the transmission line under test [samples]. LatencyDetector will supply this.
                size_t      nTimeWarp;          // Entity of the warp between processors at OP_CHIRP_EMIT trigger
                size_t      nCaptureStart;      // Sample in capture buffer at which the recorded chirp actually starts

                bool        bCycleComplete;     // True if the machine operated a whole measurement cycle

                bool        bSync;

            public:

                explicit ResponseTaker();
                ~ResponseTaker();

                /** Construct the ResponseTaker
                 *
                 */
                void construct();

                /** Initialise ResponseTaker
                 *
                 */
                void init();

                /** Destroy ResponseTaker
                 *
                 */
                void destroy();

            public:
                status_t reconfigure(Sample *testsig);

                /** Check that ResponseTaker needs settings update
                 *
                 * @return true if ResponseTaker needs setting update
                 */
                inline bool needs_update() const
                {
                    return bSync;
                }

                /** Update ResponseTaker stateful settings
                 *
                 */
                void update_settings();

                /** Set sample rate for ResponseTaker
                 *
                 * @param sr sample rate
                 */
                inline void set_sample_rate(size_t sr)
                {
                    if (nSampleRate == sr)
                        return;

                    nSampleRate                     = sr;
                    bSync                           = true;
                }

                /** Set output processor fading in seconds
                 *
                 * @param fading fading duration in seconds
                 */
                inline void set_op_fading(float fading)
                {
                    if (sOutputProcessor.fFade == fading)
                        return;

                    sOutputProcessor.fFade          = fading;
                    bSync                           = true;
                }

                /** Set output processor pause in seconds
                 *
                 * @param pause pause duration in seconds
                 */
                inline void set_op_pause(float pause)
                {
                    if (sOutputProcessor.fPause == pause)
                        return;

                    sOutputProcessor.fPause         = pause;
                    bSync                           = true;
                }

                /** Set output processor tail in seconds
                 *
                 * @param tail tail duration in seconds
                 */
                inline void set_op_tail(float tail)
                {
                    if (sOutputProcessor.fTail == tail)
                        return;

                    sOutputProcessor.fTail          = tail;
                    bSync                           = true;
                }

                /** Set the latency of the transmission line
                 *
                 * @param latency latency in samples
                 */
                inline void set_latency_samples(ssize_t latency)
                {
                    if (nLatency == size_t(latency))
                        return;

                    nLatency                        = (latency > 0) ? size_t(latency) : 0;
                    bSync                           = true;
                }

                /** Start latency detection process
                 *
                 */
                void start_capture();

                /** Force the chirp system to reset it's state
                 *
                 */
                void reset_capture();

                /** Return true if the measurement cycle was completed
                 *
                 * @return bCycleComplete value
                 */
                inline bool cycle_complete() const
                {
                    return bCycleComplete;
                }

                /** Get the captured data
                 *
                 * @return pointer to captured Sample object
                 */
                inline Sample * get_capture()
                {
                    return pCapture;
                }

                /** Get sample at which the capture buffer contains data
                 *
                 * @return capture start sample
                 */
                inline size_t get_capture_start()
                {
                    return nCaptureStart;
                }

            public:

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

                /** Stream direct chirp while recording response
                 *
                 * @param dst samples destination
                 * @param src input source, allowed to be NULL
                 * @param count number of samples to process
                 */
                void process(float *dst, const float *src, size_t count);

                /**
                 * Dump the state
                 * @param dumper dumper
                 */
                void dump(IStateDumper *v) const;
        };
    }
}

#endif /* LSP_PLUG_IN_DSP_UINTS_UTIL_RESPONSETAKER_H_ */
