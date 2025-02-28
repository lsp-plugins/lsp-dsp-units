/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 26 июля 2016 г.
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

#include <lsp-plug.in/dsp-units/filters/Equalizer.h>
#include <lsp-plug.in/dsp-units/misc/windows.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>

#define BUFFER_SIZE         0x400U

namespace lsp
{
    namespace dspu
    {
        Equalizer::Equalizer()
        {
            construct();
        }

        Equalizer::~Equalizer()
        {
            destroy();
        }

        void Equalizer::construct()
        {
            sBank.construct();

            vFilters        = NULL;
            nFilters        = 0;
            nSampleRate     = 0;
            nFirSize        = 0;
            nFirRank        = 0;
            nLatency        = 0;
            nBufSize        = 0;
            nMode           = EQM_BYPASS;
            vInBuffer       = NULL;
            vOutBuffer      = NULL;
            vConv           = NULL;
            vNewConv        = NULL;
            vFft            = NULL;
            vTemp           = NULL;
            pData           = NULL;
            nFlags          = EF_REBUILD | EF_CLEAR;
        }

        bool Equalizer::init(size_t filters, size_t fir_rank)
        {
            destroy();

            // Initialize filter bank
            sBank.init(filters * FILTER_CHAINS_MAX);

            // Initialize filters
            nSampleRate     = 0;
            vFilters        = new Filter[filters];
            if (vFilters == NULL)
            {
                destroy();
                return false;
            }
            nFilters        = filters;

            // Allocate buffers for convolution
            if (fir_rank > 0)
            {
                nFirSize            = 1 << fir_rank;
                nFirRank            = fir_rank;
                size_t fft_size     = nFirSize << 1;
                size_t conv_size    = nFirSize << 2;
                size_t tmp_size     = lsp_max(conv_size, BUFFER_SIZE);
                size_t allocate     = fft_size*2 + conv_size*3 + tmp_size + nFirSize;

                float *ptr          = alloc_aligned<float>(pData, allocate);
                if (ptr == NULL)
                {
                    destroy();
                    return false;
                }

                dsp::fill_zero(ptr, allocate);

                // Assign pointers
                vInBuffer           = ptr;
                ptr                += fft_size;             // nFirSize * 2
                vOutBuffer          = ptr;
                ptr                += fft_size;             // nFirSize * 2
                vConv               = ptr;
                ptr                += conv_size;            // nFirSize * 4
                vNewConv            = ptr;
                ptr                += conv_size;            // nFirSize * 4
                vFft                = ptr;
                ptr                += conv_size;            // nFirSize * 4
                vTemp               = ptr;
                ptr                += tmp_size;             // nFirSize * 4
            }
            else
            {
                float *ptr          = alloc_aligned<float>(pData, BUFFER_SIZE);
                if (ptr == NULL)
                {
                    destroy();
                    return false;
                }

                dsp::fill_zero(ptr, BUFFER_SIZE);

                nFirSize            = 0;
                nFirRank            = 0;
                vInBuffer           = NULL;
                vOutBuffer          = NULL;
                vConv               = NULL;
                vNewConv            = NULL;
                vFft                = NULL;
                vTemp               = ptr;
            }

            // Initialize filters
            for (size_t i=0; i<filters; ++i)
            {
                if (!vFilters[i].init(&sBank))
                {
                    destroy();
                    return false;
                }
            }

            // Mark equalizer for rebuild
            nFlags             |= EF_REBUILD | EF_CLEAR;
            nLatency            = 0;
            nBufSize            = 0;

            return true;
        }

        void Equalizer::destroy()
        {
            if (vFilters != NULL)
            {
                for (size_t i=0; i<nFilters; ++i)
                    vFilters[i].destroy();
                delete [] vFilters;
                vFilters        = NULL;
                nFilters        = 0;
            }

            if (pData != NULL)
            {
                free_aligned(pData);
                vInBuffer       = NULL;
                vOutBuffer      = NULL;
                vConv           = NULL;
                vNewConv        = NULL;
                vFft            = NULL;
                vTemp           = NULL;
                pData           = NULL;
            }

            sBank.destroy();
        }

        void Equalizer::set_sample_rate(size_t sr)
        {
            if (nSampleRate == sr)
                return;

            filter_params_t fp;
            nSampleRate     = sr;

            for (size_t i=0; i<nFilters; ++i)
            {
                vFilters[i].get_params(&fp);
                vFilters[i].update(nSampleRate, &fp);
            }
        }

        bool Equalizer::configuration_changed() const
        {
            return (nFlags & EF_REBUILD) != 0;
        }

        bool Equalizer::set_params(size_t id, const filter_params_t *params)
        {
            if (id >= nFilters)
                return false;

            vFilters[id].update(nSampleRate, params);
            nFlags     |= EF_REBUILD;
            return true;
        }

        bool Equalizer::limit_params(size_t id, filter_params_t *fp)
        {
            if (id >= nFilters)
                return false;

            vFilters[id].limit(nSampleRate, fp);
            return true;
        }

        bool Equalizer::get_params(size_t id, filter_params_t *params)
        {
            if (id >= nFilters)
                return false;
            vFilters[id].get_params(params);
            return true;
        }

        size_t Equalizer::get_latency()
        {
            reconfigure();
            return nLatency;
        }

        void Equalizer::reconfigure()
        {
            if (!(nFlags & (EF_REBUILD | EF_CLEAR)))
                return;

            if (nMode == EQM_BYPASS)
            {
                nFlags         &= ~(EF_REBUILD | EF_CLEAR | EF_XFADE);
                nLatency        = 0;
                return;
            }

            // Initialize bank
            sBank.begin();
            for (size_t i=0; i<nFilters; ++i)
                vFilters[i].rebuild();
            sBank.end(nFlags & EF_CLEAR);

            // Quit if working in IIR mode
            if (nMode == EQM_IIR)
            {
                nFlags         &= ~(EF_REBUILD | EF_CLEAR | EF_XFADE);
                nLatency        = 0;
                return;
            }

            // Clear state of equalizer?
            size_t fft_size     = nFirSize << 1;
            size_t half_size    = nFirSize >> 1;

            if (nFlags & EF_CLEAR)
            {
                dsp::fill_zero(vInBuffer,  fft_size);
                dsp::fill_zero(vOutBuffer, fft_size);
                nBufSize    = 0;
            }

            // Build filter's magnitude characteristics
            if (nMode == EQM_FIR)
            {
                windows::blackman_nuttall(vNewConv, fft_size);
                sBank.impulse_response(vTemp, nFirSize);                        // Generate impulse response of the filter
                dsp::mul2(vTemp, &vNewConv[nFirSize], nFirSize);                // Apply window function to the impulse response
                dsp::pcomplex_r2c(vFft, vTemp, nFirSize);                       // Prepare for FFT transform
                dsp::packed_direct_fft(vFft, vFft, nFirRank);                   // Perform FFT
                dsp::pcomplex_mod(vTemp, vFft, nFirSize);                       // Now we have FFT magnitude in vTemp
            }
            else if ((nMode == EQM_FFT) || (nMode == EQM_SPM))
            {
                size_t num_filters  = 0;
                size_t freq_size    = half_size + 1;
                dsp::lin_inter_set(vNewConv, 0, 0.0f, int32_t(half_size), 0.5f * nSampleRate, 0, uint32_t(freq_size)); // Compute frequencies

                // Build frequency chart for all filters
                for (size_t i=0; i<nFilters; ++i)
                {
                    // Skip inactive filters
                    if (vFilters[i].inactive())
                        continue;

                    // Get the frequency chart of the filter
                    if ((num_filters++) > 0)
                    {
                        vFilters[i].freq_chart(vFft, vNewConv, freq_size);
                        dsp::pcomplex_mod(vFft, vFft, freq_size);
                        dsp::mul2(vTemp, vFft, freq_size);
                    }
                    else
                    {
                        vFilters[i].freq_chart(vFft, vNewConv, freq_size);
                        dsp::pcomplex_mod(vTemp, vFft, freq_size);
                    }
                }

                // Finally, build the correct frequency chart for reverse FFT
                if (num_filters > 0)
                    dsp::reverse2(&vTemp[freq_size], &vTemp[1], half_size-1);
                else
                    dsp::fill_one(vTemp, nFirSize);
            }
            else
                dsp::fill_one(vTemp, nFirSize);                                 // Flat response

            if (nMode != EQM_SPM)
            {
                // Transform the magnitude into linear-phase filter
                dsp::pcomplex_r2c(vFft, vTemp, nFirSize);                           // Set phase to 0 for all frequencies
                dsp::packed_reverse_fft(vFft, vFft, nFirRank);                      // Get the synthesized impulse response
                dsp::pcomplex_c2r(&vTemp[half_size], vFft, nFirSize);               // Get real part of the impulse response
                dsp::copy(vTemp, &vTemp[nFirSize], half_size);                      // Make impulse response symmetric
                windows::blackman_nuttall(vNewConv, nFirSize);                      // Compute the window function
                dsp::mul2(vTemp, vNewConv, nFirSize);                               // Apply the window function

                // Get the final impulse response data
                if (nFlags & EF_SMOOTH)
                {
                    nFlags     |= EF_XFADE;
                    dsp::fastconv_parse(vNewConv, vTemp, nFirRank + 1);                 // Get the IR function
                }
                else
                    dsp::fastconv_parse(vConv, vTemp, nFirRank + 1);                 // Get the IR function

                nLatency    = nFirSize + half_size;
                nFlags     &= ~(EF_REBUILD | EF_CLEAR);
            }
            else // EQM_SPM
            {
                dsp::pcomplex_r2c(vConv, vTemp, nFirSize);                          // Convert magnitude to complex value
                windows::sqr_cosine(vFft, nFirSize);                                // Also provide window

                nLatency    = nFirSize;
                nFlags     &= ~(EF_REBUILD | EF_CLEAR | EF_XFADE);
            }
        }

        void Equalizer::set_mode(equalizer_mode_t mode)
        {
            if (mode == nMode)
                return;
            nMode       = mode;
            nFlags     |= EF_REBUILD | EF_CLEAR;
        }

        bool Equalizer::freq_chart(size_t id, float *re, float *im, const float *f, size_t count)
        {
            if (id >= nFilters)
                return false;
            reconfigure();

            vFilters[id].freq_chart(re, im, f, count);
            return true;
        }

        bool Equalizer::freq_chart(size_t id, float *c, const float *f, size_t count)
        {
            if (id >= nFilters)
                return false;
            reconfigure();

            vFilters[id].freq_chart(c, f, count);
            return true;
        }

        void Equalizer::freq_chart(float *re, float *im, const float *f, size_t count)
        {
            reconfigure();

            float *xre      = vTemp;
            float *xim      = &xre[BUFFER_SIZE/2];

            // Fill initial values
            dsp::fill_one(re, count);
            dsp::fill_zero(im, count);

            while (count > 0)
            {
                // Estimate number of frequencies to process
                size_t to_do    = lsp_min(count, size_t(BUFFER_SIZE/2));

                for (size_t i=0; i<nFilters; ++i)
                {
                    Filter *xf      = &vFilters[i];
                    if (!xf->active())
                        continue;

                    xf->freq_chart(xre, xim, f, to_do);
                    dsp::complex_mul2(re, im, xre, xim, to_do);
                }

                // Update pointers
                re             += to_do;
                im             += to_do;
                f              += to_do;
                count          -= to_do;
            }
        }

        void Equalizer::freq_chart(float *c, const float *f, size_t count)
        {
            reconfigure();

            // Fill initial values
            dsp::pcomplex_fill_ri(c, 1.0f, 0.0f, count);

            while (count > 0)
            {
                // Estimate number of frequencies to process
                size_t to_do    = lsp_min(count, size_t(BUFFER_SIZE/2));

                for (size_t i=0; i<nFilters; ++i)
                {
                    Filter *xf      = &vFilters[i];
                    if (!xf->active())
                        continue;

                    xf->freq_chart(vTemp, f, to_do);
                    dsp::pcomplex_mul2(c, vTemp, to_do);
                }

                // Update pointers
                c              += to_do * 2;
                f              += to_do;
                count          -= to_do;
            }
        }

        void Equalizer::process(float *out, const float *in, size_t samples)
        {
            reconfigure();

            switch (nMode)
            {
                case EQM_IIR:
                {
                    sBank.process(out, in, samples);
                    break;
                }

                case EQM_FIR:
                case EQM_FFT:
                {
                    size_t conv_rank    = nFirRank + 1;

                    while (samples > 0)
                    {
                        if (nBufSize >= nFirSize)
                        {
                            // Apply FIR processing
                            dsp::move(vOutBuffer, &vOutBuffer[nFirSize], nFirSize);     // Shift output buffer
                            dsp::fill_zero(&vOutBuffer[nFirSize], nFirSize);            // Empty tail of output buffer
                            dsp::fastconv_parse_apply(vOutBuffer, vTemp, vConv, vInBuffer, conv_rank); // Apply convolution

                            if (nFlags & EF_XFADE)
                            {
                                size_t half = nFirSize >> 1;

                                // Replace old convolution with new one and apply new convolution
                                dsp::fill_zero(vFft, nFirSize*2);
                                dsp::copy(vConv, vNewConv, nFirSize * 4);
                                dsp::fastconv_parse_apply(vFft, vTemp, vConv, vInBuffer, conv_rank); // Apply convolution

                                // Mix the result of previous convolution with new one
                                dsp::lramp1(&vOutBuffer[half], 1.0f, 0.0f, nFirSize);
                                dsp::lramp_add2(&vOutBuffer[half], &vFft[half], 0.0f, 1.0f, nFirSize);
                                dsp::copy(&vOutBuffer[nFirSize + half], &vFft[nFirSize + half], half);

                                nFlags     &= ~EF_XFADE;
                            }

                            nBufSize    = 0; // Reset buffer size
                        }

                        // Determine number of samples to process
                        size_t to_process = lsp_min(samples, nFirSize - nBufSize);

                        // Push new data for processing and emit processed data
                        dsp::copy(&vInBuffer[nBufSize], in, to_process);
                        dsp::copy(out, &vOutBuffer[nBufSize], to_process);

                        // Update pointers and counters
                        nBufSize       += to_process;
                        out            += to_process;
                        in             += to_process;
                        samples        -= to_process;
                    }

                    break;
                }

                case EQM_SPM:
                {
                    size_t half_len     = nFirSize >> 1;

                    while (samples > 0)
                    {
                        if (nBufSize >= half_len)
                        {
                            // Apply FIR processing
                            dsp::move(vOutBuffer, &vOutBuffer[half_len], half_len);     // Shift output buffer
                            dsp::fill_zero(&vOutBuffer[half_len], half_len);            // Empty tail of destination buffer

                            dsp::pcomplex_r2c(vTemp, vInBuffer, nFirSize);              // Convert source buffer to complex numbers
                            dsp::packed_direct_fft(vTemp, vTemp, nFirRank);             // Perform FFT
                            dsp::pcomplex_mul2(vTemp, vConv, nFirSize);                 // Apply magnitude
                            dsp::packed_reverse_fft(vTemp, vTemp, nFirRank);            // Transform back
                            dsp::pcomplex_c2r(vTemp, vTemp, nFirSize);                  // Add result of convolution to output
                            dsp::fmadd3(vOutBuffer, vTemp, vFft, nFirSize);             // Apply window to the signal and add to buffer

                            dsp::move(vInBuffer, &vInBuffer[half_len], half_len);       // Shift input buffer

                            nBufSize    = 0; // Reset buffer size
                        }

                        // Determine number of samples to process
                        size_t to_process = lsp_min(samples, half_len - nBufSize);

                        // Push new data for processing and emit processed data
                        dsp::copy(&vInBuffer[half_len + nBufSize], in, to_process);
                        dsp::copy(out, &vOutBuffer[nBufSize], to_process);

                        // Update pointers and counters
                        nBufSize       += to_process;
                        out            += to_process;
                        in             += to_process;
                        samples        -= to_process;
                    }

                    break;
                }

                case EQM_BYPASS:
                default:
                {
                    dsp::copy(out, in, samples);
                    break;
                }
            }
        }

        void Equalizer::reset()
        {
            nFlags &= ~EF_CLEAR;

            switch (nMode)
            {
                case EQM_BYPASS:
                    return;

                case EQM_IIR:
                    sBank.reset();
                    break;

                case EQM_FIR:
                case EQM_FFT:
                case EQM_SPM:
                    dsp::fill_zero(vInBuffer,  nFirSize << 1);
                    dsp::fill_zero(vOutBuffer, nFirSize << 1);
                    nBufSize    = 0;
                    break;

                default:
                    break;
            }
        }

        size_t Equalizer::ir_size() const
        {
            switch (nMode)
            {
                case EQM_BYPASS:
                case EQM_IIR:
                    return 0;
                    break;

                case EQM_FIR:
                case EQM_FFT:
                case EQM_SPM:
                    return nFirSize << 1;

                default:
                    return 0;
            }
        }

        bool Equalizer::smooth() const
        {
            return nFlags & EF_SMOOTH;
        }

        void Equalizer::set_smooth(bool smooth)
        {
            nFlags = lsp_setflag(nFlags, EF_SMOOTH, smooth);
        }

        void Equalizer::dump(IStateDumper *v) const
        {
            v->write_object("sBank", &sBank);

            v->begin_array("vFilters", vFilters, nFilters);
            for (size_t i=0; i<nFilters; ++i)
                v->write_object(&vFilters[i]);
            v->end_array();

            v->write("nFilters", nFilters);
            v->write("nSampleRate", nSampleRate);
            v->write("nFirSize", nFirSize);
            v->write("nFirRank", nFirRank);
            v->write("nLatency", nLatency);
            v->write("nBufSize", nBufSize);
            v->write("nMode", nMode);
            v->write("vInBuffer", vInBuffer);
            v->write("vOutBuffer", vOutBuffer);
            v->write("vConv", vConv);
            v->write("vNewConv", vNewConv);
            v->write("vFft", vFft);
            v->write("vTemp", vTemp);
            v->write("nFlags", nFlags);
            v->write("pData", pData);
        }

    } /* namespace dspu */
} /* namespace lsp */
