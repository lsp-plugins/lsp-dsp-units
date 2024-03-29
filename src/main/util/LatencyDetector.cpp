/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Stefano Tronci <stefano.tronci@protonmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 5 Apr 2017
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/dsp-units/util/LatencyDetector.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/dsp/dsp.h>

#define LIM_BUF_SIZE        (1 << 15)

namespace lsp
{
    namespace dspu
    {
        LatencyDetector::LatencyDetector()
        {
            construct();
        }

        LatencyDetector::~LatencyDetector()
        {
            destroy();
        }

        void LatencyDetector::construct()
        {
            nSampleRate                             = -1;

            sChirpSystem.fDuration                  = 0.15; // 150 ms
            sChirpSystem.fDelayRatio                = 0.0f;

            sChirpSystem.bModified                  = true;

            sChirpSystem.nDuration                  = 0;

            sChirpSystem.n2piMult                   = 0;
            sChirpSystem.fAlpha                     = 0.0f;
            sChirpSystem.fBeta                      = 0.0f;
            sChirpSystem.nLength                    = 0;
            sChirpSystem.nOrder                     = 0;
            sChirpSystem.nFftRank                   = 0;

            sChirpSystem.fConvScale                 = 0.0f;

            sInputProcessor.nState                  = IP_BYPASS;
            sInputProcessor.ig_time                 = 0;
            sInputProcessor.ig_start                = 0;
            sInputProcessor.ig_stop                 = -1;

            sInputProcessor.fDetect                 = 0.5f; // 500 ms
            sInputProcessor.nDetect                 = 0;
            sInputProcessor.nDetectCounter          = 0;

            sOutputProcessor.nState                 = OP_BYPASS;
            sOutputProcessor.og_time                = 0;
            sOutputProcessor.og_start               = 0;

            sOutputProcessor.fGain                  = 1.0f;
            sOutputProcessor.fGainDelta             = 0.0f;

            sOutputProcessor.fFade                  = 0.01; // 10 ms
            sOutputProcessor.nFade                  = 0;

            sOutputProcessor.fPause                 = 0.5f; // 500 ms
            sOutputProcessor.nPause                 = 0;
            sOutputProcessor.nPauseCounter          = 0;

            sOutputProcessor.nEmitCounter           = 0;

            sPeakDetector.fAbsThreshold             = 0.0f;
            sPeakDetector.fPeakThreshold            = 0.0f;
            sPeakDetector.fValue                    = 0.0f;
            sPeakDetector.nPosition                 = 0;
            sPeakDetector.nTimeOrigin               = 0;
            sPeakDetector.bDetected                 = false;

            vChirp                                  = NULL;
            vAntiChirp                              = NULL;
            vCapture                                = NULL;
            vBuffer                                 = NULL;
            vChirpConv                              = NULL;
            vConvBuf                                = NULL;
            pData                                   = NULL;

            bCycleComplete                          = false;
            bLatencyDetected                        = false;
            nLatency                                = -1;

            bSync                                   = true;
        }

        void LatencyDetector::init()
        {
            // 1x chirp + 1x anti-chirp + 1x capture + 2x buffer + 4x conv image + 4x temporary convolution buffer
            size_t samples  = 13 * LIM_BUF_SIZE;

            uint8_t *ptr    = alloc_aligned<uint8_t>(pData, samples * sizeof(float), DEFAULT_ALIGN);

            vChirp          = reinterpret_cast<float *>(ptr);
            ptr            += LIM_BUF_SIZE * sizeof(float);
            vAntiChirp      = reinterpret_cast<float *>(ptr);
            ptr            += LIM_BUF_SIZE * sizeof(float);
            vCapture        = reinterpret_cast<float *>(ptr);
            ptr            += LIM_BUF_SIZE * sizeof(float);
            vBuffer         = reinterpret_cast<float *>(ptr);
            ptr            += 2 * LIM_BUF_SIZE * sizeof(float);
            vChirpConv      = reinterpret_cast<float *>(ptr);
            ptr            += 4 * LIM_BUF_SIZE * sizeof(float);
            vConvBuf        = reinterpret_cast<float *>(ptr);
            ptr            += 4 * LIM_BUF_SIZE * sizeof(float);

            dsp::fill_zero(vChirp, samples);
        }

        void LatencyDetector::destroy()
        {
            if (pData != NULL)
            {
                free_aligned(pData);
                pData = NULL;
            }

            vChirp      = NULL;
            vAntiChirp  = NULL;
            vCapture    = NULL;
            vBuffer     = NULL;
            vChirpConv  = NULL;
            vConvBuf    = NULL;
        }

        void LatencyDetector::update_settings()
        {
            if (!bSync)
                return;

            // First of all, calculate parameters

            if (sChirpSystem.bModified)
            {
                sChirpSystem.nDuration      = seconds_to_samples(nSampleRate, sChirpSystem.fDuration);

                /** Set up parameters. As all parameters are related to each other, this
                 *  loop is needed to make sure the end duration in samples is not
                 *  greater than LIM_BUF_SIZE
                 */
                while (true)
                {
                    sChirpSystem.n2piMult   = sChirpSystem.nDuration / (6.0f - sChirpSystem.fDelayRatio);
                    sChirpSystem.fAlpha     = sChirpSystem.n2piMult * sChirpSystem.fDelayRatio;

                    if (sChirpSystem.nDuration <= (LIM_BUF_SIZE - sChirpSystem.fAlpha))
                        break;

                    --sChirpSystem.nDuration;
                }

                sChirpSystem.fBeta  = sChirpSystem.n2piMult * (2.0f - sChirpSystem.fDelayRatio) * M_1_PI;

                /** Calculate the whole FIR number of samples as a power of 2. Thanks to
                 * the loop above it will be smaller or equal to LIM_BUF_SIZE
                 */
                sChirpSystem.nLength  = 1;
                sChirpSystem.nFftRank = 0;
                while (sChirpSystem.nLength  < (sChirpSystem.nDuration + sChirpSystem.fAlpha))
                {
                    sChirpSystem.nLength <<= 1;
                    ++sChirpSystem.nFftRank;
                }

                sChirpSystem.nOrder = sChirpSystem.nLength - 1;

                /** For the frequency response, frequency sample up at which the
                 * frequency is positive
                 */
                size_t nPosFreqLim = (sChirpSystem.nLength >> 1) + 1;

                /** Chirp system parameters are intended to be used in a frequency
                 * response as a function of standard DSP normalised frequency Omega:
                 *
                 *  Omega = 2pi * frequency / SampleRate
                 *
                 *  So the following parameter will convert from frequency sample
                 *  number to normalised frequency value:
                 */
                float fSample2Omega = M_PI / nPosFreqLim;

                /** The reference parabolic phase response is assumed positive in sign.
                 * So, the causal (direct) chirp frequency response has negative sign
                 * in the phase:
                 *
                 * H = exp(-i * Theta), Theta = alpha * Omega + beta * Omega^2
                 *
                 * This means that the group deleay is the simple derivative of Theta,
                 * no sign inverted:
                 *
                 * GD = dTheta / dOmega
                 */
                float *chirp_re = vChirpConv;
                float *chirp_im = &vChirpConv[LIM_BUF_SIZE];

                for (size_t k = 0; k < nPosFreqLim; ++k)
                {
                    float fOmega    =  k * fSample2Omega;
                    float angle     = (sChirpSystem.fAlpha + sChirpSystem.fBeta * fOmega) * fOmega;
                    chirp_re[k]     =  cosf(angle);
                    chirp_im[k]     = -sinf(angle);
                }

                /** Impose Hermitian symmetry (second half of the response is the first
                 * one, but flipped and conjugated). No repetitions of Omega = pi
                 * (Nyquist) and Omega = 2pi (Sample Rate).
                 */
                for (size_t k = nPosFreqLim; k < sChirpSystem.nLength; ++k)
                {
                    size_t idx      = ((nPosFreqLim - 1)<<1) - k;
                    chirp_re[k]     =  chirp_re[idx];
                    chirp_im[k]     = -chirp_im[idx];
                }

                // Get the time domain samples through inverse fft:
                // DSP FFT functions expect that all pointers are NON-NULL
                // We don't require vHChirpIm so we can pass it as a target buffer
                dsp::reverse_fft(vChirp, chirp_im, chirp_re, chirp_im, sChirpSystem.nFftRank);

                // Normalise to avoid wasting dynamic range - Needs to introduce convolution scaling factors
                float maxAbsChirp = dsp::abs_max(vChirp, sChirpSystem.nLength);
                sChirpSystem.fConvScale = maxAbsChirp * maxAbsChirp; // This should make the convolution stay between 0 and 1
                dsp::normalize(vChirp, vChirp, sChirpSystem.nLength);

                // Create Anti Chirp samples by just reversing the chirp ones:
                // Here we can use dsp::reverse2
                dsp::reverse2(vAntiChirp, vChirp, sChirpSystem.nLength);

                // Prepare the convolution image
                dsp::fastconv_parse(vChirpConv, vAntiChirp, sChirpSystem.nFftRank+1);

                sChirpSystem.bModified = false;
            }

            // Processors parameters:
            sOutputProcessor.nFade          = seconds_to_samples(nSampleRate, sOutputProcessor.fFade);
            sOutputProcessor.fGainDelta     = sOutputProcessor.fGain / (sOutputProcessor.nFade + 1);
            sOutputProcessor.nPause         = seconds_to_samples(nSampleRate, sOutputProcessor.fPause);
            sInputProcessor.nDetect         = sChirpSystem.nDuration + seconds_to_samples(nSampleRate, sInputProcessor.fDetect);

            // Mark synced
            bSync = false;
        }

        void LatencyDetector::detect_peak(float *buf, size_t count)
        {
            // Scan trough and update the highest absolute peak recorded.
            // If the delta between the last highest peak recorded and the previous
            // one is higher then threshold, produce early detection (provided that
            // the latency makes sense).

            size_t position     = dsp::abs_max_index(buf, count);
            float value         = sChirpSystem.fConvScale * fabs(buf[position]);

            if ((value > sPeakDetector.fAbsThreshold) && (value > sPeakDetector.fValue))
            {
                float delta                 = value - sPeakDetector.fValue;
                sPeakDetector.fValue        = value;
                sPeakDetector.nPosition     = position + sInputProcessor.nDetectCounter - sChirpSystem.nLength;

                nLatency = sPeakDetector.nPosition - sPeakDetector.nTimeOrigin;

                // Early detection
                if ((nLatency >= 0) && (delta > sPeakDetector.fPeakThreshold))
                {
                    bLatencyDetected            = true;
                    sInputProcessor.nState      = IP_BYPASS;
                    sOutputProcessor.nState     = OP_FADEIN;
                    sInputProcessor.ig_stop     = sInputProcessor.ig_time;
                    bCycleComplete              = true;
                }
            }
        }

        void LatencyDetector::process_in(float *dst, const float *src, size_t count)
        {
            if (bSync)
                update_settings();

            while (count > 0)
            {
                switch (sInputProcessor.nState)
                {
                    case IP_DETECT:
                    {
                        // Fill-in capture buffer
                        size_t captureIdx   = sInputProcessor.nDetectCounter % sChirpSystem.nLength;
                        size_t to_do        = sChirpSystem.nLength - captureIdx;
                        if (to_do > count)
                            to_do       = count;

                        dsp::copy(&vCapture[captureIdx], src, to_do);

                        sInputProcessor.nDetectCounter      += to_do;
                        sInputProcessor.ig_time             += to_do;
                        dst                                 += to_do;
                        src                                 += to_do;
                        count                               -= to_do;

                        if ((sInputProcessor.nDetectCounter % sChirpSystem.nLength) == 0)
                        {
                            // Do the convolution
                            dsp::fastconv_parse_apply(vBuffer, vConvBuf, vChirpConv, vCapture, sChirpSystem.nFftRank+1);

                            detect_peak(vBuffer, sChirpSystem.nLength);

                            // Do post-actions after processing: shift convolution buffer to the chirp system length
                            dsp::move(vBuffer, &vBuffer[sChirpSystem.nLength], sChirpSystem.nLength);
                        }

                        // Force Processor transitions after the time allowed for detection have elapsed
                        if (sInputProcessor.nDetectCounter >= sInputProcessor.nDetect)
                        {
                            sInputProcessor.nState      = IP_BYPASS;
                            sOutputProcessor.nState     = OP_FADEIN;
                            sInputProcessor.ig_stop     = sInputProcessor.ig_time;
                            bCycleComplete              = true;
                        }

                        break;
                    }

                    case IP_WAIT:
                        sInputProcessor.ig_time += count;
                        dsp::copy(dst, src, count);
                        count = 0;
                        break;
                    case IP_BYPASS:
                    default:
                        dsp::copy(dst, src, count);
                        count = 0;
                        break;
                }
            }
        }

        void LatencyDetector::process_out(float *dst, const float *src, size_t count)
        {
            if (bSync)
                update_settings();

            while (count > 0)
            {
                switch (sOutputProcessor.nState)
                {
                    case OP_FADEOUT:
                        while (count > 0)
                        {
                            sOutputProcessor.fGain  -= sOutputProcessor.fGainDelta;

                            if (sOutputProcessor.fGain <= 0.0f)
                            {
                                sOutputProcessor.fGain          = 0.0f;
                                sOutputProcessor.nPauseCounter  = sOutputProcessor.nPause;
                                sOutputProcessor.nState         = OP_PAUSE;
                                break;
                            }

                            *(dst++) = *(src++) * sOutputProcessor.fGain;
                            count --;
                            sOutputProcessor.og_time ++;
                        }
                        break;

                    case OP_PAUSE:
                    {
                        // For pause we're using decrementing counter
                        size_t to_do    = (sOutputProcessor.nPauseCounter > count) ? count : sOutputProcessor.nPauseCounter;
                        dsp::fill_zero(dst, to_do);

                        sOutputProcessor.nPauseCounter  -= to_do;
                        sOutputProcessor.og_time        += to_do;
                        src                             += to_do;
                        dst                             += to_do;
                        count                           -= to_do;

                        if (sOutputProcessor.nPauseCounter <= 0)
                        {
                            sOutputProcessor.nEmitCounter   = 0;
                            sOutputProcessor.nState         = OP_EMIT;
                            sInputProcessor.nState          = IP_DETECT;
                            sOutputProcessor.og_start       = sOutputProcessor.og_time;
                            sInputProcessor.ig_start        = sInputProcessor.ig_time;
                            sPeakDetector.fValue            = 0.0f;
                            sPeakDetector.nPosition         = 0;
                            // Correcting the apparent latency centre (nLength - 1) with the actually recorded samples:
                            sPeakDetector.nTimeOrigin       = sChirpSystem.nLength - (sInputProcessor.ig_start - sOutputProcessor.og_start) - 1;
                            sPeakDetector.bDetected         = false;
                            bLatencyDetected                = false;
                            nLatency                        = 0;

                            dsp::fill_zero(vBuffer, 2 * LIM_BUF_SIZE);
                        }
                        break;
                    }

                    case OP_EMIT:
                    {
                        size_t to_do;

                        // For detection we're using incrementing counter
                        if (sOutputProcessor.nEmitCounter < sChirpSystem.nLength)
                        {
                            to_do = sChirpSystem.nLength - sOutputProcessor.nEmitCounter;
                            if (to_do > count)
                                to_do = count;

                            dsp::copy(dst, &vChirp[sOutputProcessor.nEmitCounter], to_do);
                        }
                        else
                        {
                            to_do = count;
                            dsp::fill_zero(dst, to_do);
                        }

                        sOutputProcessor.nEmitCounter   += to_do;
                        sOutputProcessor.og_time        += to_do;
                        dst                             += to_do;
                        src                             += to_do;
                        count                           -= to_do;
                        break;
                    }

                    case OP_FADEIN:
                        while (count > 0)
                        {
                            sOutputProcessor.fGain  += sOutputProcessor.fGainDelta;
                            if (sOutputProcessor.fGain >= 1.0f)
                            {
                                sOutputProcessor.fGain      = 1.0f;
                                sOutputProcessor.nState     = OP_BYPASS;
                                break;
                            }

                            *(dst++) = *(src++) * sOutputProcessor.fGain;
                            count --;
                            sOutputProcessor.og_time ++;
                        }
                        break;

                    case OP_BYPASS:
                    default:
                        dsp::copy(dst, src, count);
                        count = 0;
                        break;
                }
            }
        }

        void LatencyDetector::process(float *dst, const float *src, size_t count)
        {
            process_in(dst, src, count);
            process_out(dst, dst, count);
        }

        float LatencyDetector::get_duration_seconds() const
        {
            return samples_to_seconds(nSampleRate, sChirpSystem.nDuration);
        }

        float LatencyDetector::get_latency_seconds() const
        {
            if (!bLatencyDetected)
                return 0.0f;

            return samples_to_seconds(nSampleRate, nLatency);
        }

        void LatencyDetector::set_abs_threshold(float threshold)
        {
            if (sPeakDetector.fAbsThreshold == threshold)
                return;

            sPeakDetector.fAbsThreshold = ((threshold > 0.0f) && (threshold <= 1.0f)) ? threshold : DEFAULT_ABS_THRESHOLD;
        }

        void LatencyDetector::set_peak_threshold(float threshold)
        {
            if (sPeakDetector.fPeakThreshold == threshold)
                return;

            sPeakDetector.fPeakThreshold = ((threshold > 0.0f) && (threshold <= 1.0f)) ? threshold : DEFAULT_PEAK_THRESHOLD;
        }

        void LatencyDetector::set_delay_ratio(float ratio)
        {
            if ((sChirpSystem.fDelayRatio == ratio) || (ratio <= 0))
                return;

            // This condition is needed for causality of direct chirp system
            sChirpSystem.fDelayRatio = (ratio < 4.0f) ? ratio : 4.0f;
            sChirpSystem.bModified = true;
            bSync       = true;
        }

        void LatencyDetector::start_capture()
        {
            sInputProcessor.nState              = IP_WAIT;
            sInputProcessor.ig_time             = 0;
            sInputProcessor.ig_start            = 0;
            sInputProcessor.ig_stop             = -1;
            sInputProcessor.nDetectCounter      = 0;

            sOutputProcessor.nState             = OP_FADEOUT;
            sOutputProcessor.og_time            = 0;
            sOutputProcessor.og_start           = 0;
            sOutputProcessor.nPauseCounter      = 0;
            sOutputProcessor.nEmitCounter       = 0;

            sPeakDetector.fValue                = 0.0f;
            sPeakDetector.nPosition             = 0;
            sPeakDetector.nTimeOrigin           = 0;
            sPeakDetector.bDetected             = false;

            bCycleComplete                      = false;
            bLatencyDetected                    = false;
            nLatency                            = 0;
        }

        void LatencyDetector::reset_capture()
        {
            sInputProcessor.nState              = IP_BYPASS;
            sInputProcessor.ig_time             = 0;
            sInputProcessor.ig_start            = 0;
            sInputProcessor.ig_stop             = -1;
            sInputProcessor.nDetectCounter      = 0;

            sOutputProcessor.nState             = OP_BYPASS;
            sOutputProcessor.og_time            = 0;
            sOutputProcessor.og_start           = 0;
            sOutputProcessor.nPauseCounter      = 0;
            sOutputProcessor.nEmitCounter       = 0;

            sPeakDetector.fValue                = 0.0f;
            sPeakDetector.nPosition             = 0;
            sPeakDetector.nTimeOrigin           = 0;
            sPeakDetector.bDetected             = false;

            bCycleComplete                      = false;
            bLatencyDetected                    = false;
            nLatency                            = 0;
        }

        void LatencyDetector::dump(IStateDumper *v) const
        {
            v->write("nSampleRate", nSampleRate);

            v->begin_object("sChirpSystem", &sChirpSystem, sizeof(chirp_t));
            {
                const chirp_t *c = &sChirpSystem;

                v->write("fDuration", c->fDuration);
                v->write("fDelayRatio", c->fDelayRatio);
                v->write("bModified", c->bModified);
                v->write("nDuration", c->nDuration);
                v->write("n2piMult", c->n2piMult);
                v->write("fAlpha", c->fAlpha);
                v->write("fBeta", c->fBeta);
                v->write("nLength", c->nLength);
                v->write("nOrder", c->nOrder);
                v->write("nFftRank", c->nFftRank);
                v->write("fConvScale", c->fConvScale);
            }
            v->end_object();

            v->begin_object("sInputProcessor", &sInputProcessor, sizeof(ip_t));
            {
                const ip_t *p = &sInputProcessor;

                v->write("nState", p->nState);
                v->write("ig_time", p->ig_time);
                v->write("ig_start", p->ig_start);
                v->write("ig_stop", p->ig_stop);
                v->write("fDetect", p->fDetect);
                v->write("nDetect", p->nDetect);
                v->write("nDetectCounter", p->nDetectCounter);
            }
            v->end_object();

            v->begin_object("sOutputProcessor", &sOutputProcessor, sizeof(op_t));
            {
                const op_t *p = &sOutputProcessor;

                v->write("nState", p->nState);
                v->write("og_time", p->og_time);
                v->write("og_start", p->og_start);
                v->write("fGain", p->fGain);
                v->write("fGainDelta", p->fGainDelta);
                v->write("fFade", p->fFade);
                v->write("nFade", p->nFade);
                v->write("fPause", p->fPause);
                v->write("nPause", p->nPause);
                v->write("nPauseCounter", p->nPauseCounter);
                v->write("nEmitCounter", p->nEmitCounter);
            }
            v->end_object();

            v->begin_object("sPeakDetector", &sPeakDetector, sizeof(peak_t));
            {
                const peak_t *p = &sPeakDetector;

                v->write("fAbsThreshold", p->fAbsThreshold);
                v->write("fPeakThreshold", p->fPeakThreshold);
                v->write("fValue", p->fValue);
                v->write("nPosition", p->nPosition);
                v->write("nTimeOrigin", p->nTimeOrigin);
                v->write("bDetected", p->bDetected);
            }
            v->end_object();

            v->write("vChirp", vChirp);
            v->write("vAntiChirp", vAntiChirp);
            v->write("vCapture", vCapture);
            v->write("vBuffer", vBuffer);
            v->write("vChirpConv", vChirpConv);
            v->write("vConvBuf", vConvBuf);
            v->write("pData", pData);
            v->write("bCycleComplete", bCycleComplete);
            v->write("bLatencyDetected", bLatencyDetected);
            v->write("nLatency", nLatency);
            v->write("bSync", bSync);
        }
    }
}
