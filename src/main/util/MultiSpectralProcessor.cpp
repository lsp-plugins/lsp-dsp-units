/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 13 нояб. 2025 г.
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

#include <lsp-plug.in/dsp-units/util/MultiSpectralProcessor.h>
#include <lsp-plug.in/dsp-units/misc/windows.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        MultiSpectralProcessor::MultiSpectralProcessor()
        {
            construct();
        }

        MultiSpectralProcessor::~MultiSpectralProcessor()
        {
            destroy();
        }

        void MultiSpectralProcessor::construct()
        {
            nChannels       = 0;
            nRank           = 0;
            nMaxRank        = 0;
            nOffset         = 0;
            vChannels       = NULL;
            vFftBuf         = NULL;
            pWnd            = NULL;
            fPhase          = 0.0f;
            bUpdate         = true;

            pFunc           = NULL;
            pObject         = NULL;
            pSubject        = NULL;

            pData           = NULL;
        }

        bool MultiSpectralProcessor::init(size_t channels, size_t max_rank)
        {
            if (channels <= 0)
                return false;

            const size_t szof_channels  = align_size(sizeof(channel_t) * channels, DEFAULT_ALIGN);
            const size_t szof_vfft      = align_size(sizeof(float *) * channels, DEFAULT_ALIGN);
            const size_t szof_buf       = sizeof(float) << max_rank;
            const size_t szof_wnd       = szof_buf;
            const size_t szof_fft_buf   = szof_buf * 2;
            const size_t to_alloc       = szof_channels + szof_vfft + szof_wnd + (szof_buf * 2 + szof_fft_buf) * channels;

            uint8_t *data               = NULL;
            uint8_t *ptr                = alloc_aligned<uint8_t>(data, to_alloc);
            if (ptr == NULL)
                return false;


            if (pData != NULL)
            {
                free_aligned(pData);

                pWnd            = NULL;
                vFftBuf         = NULL;
                vChannels       = NULL;
            }

            nChannels       = channels;
            nRank           = max_rank;
            nMaxRank        = max_rank;
            nOffset         = 0;

            vChannels       = advance_ptr_bytes<channel_t>(ptr, szof_channels);
            vFftBuf         = advance_ptr_bytes<float *>(ptr, szof_vfft);
            pWnd            = advance_ptr_bytes<float>(ptr, szof_wnd);

            // Initialize channels
            for (size_t i=0; i<channels; ++i)
            {
                channel_t *c    = &vChannels[i];

                c->pIn          = NULL;
                c->pOut         = NULL;

                c->pInBuf       = advance_ptr_bytes<float>(ptr, szof_buf);
                c->pOutBuf      = advance_ptr_bytes<float>(ptr, szof_buf);
                c->pFftBuf      = advance_ptr_bytes<float>(ptr, szof_fft_buf);
            }

            // Initialize FFT pointers
            for (size_t i=0; i < (szof_vfft/sizeof(float)); ++i)
                vFftBuf[i]          = NULL;

            // Initialize window
            fPhase          = 0.0f;
            bUpdate         = true;

            pFunc           = NULL;
            pObject         = NULL;
            pSubject        = NULL;

            pData           = data;

            return true;
        }

        void MultiSpectralProcessor::destroy()
        {
            if (pData != NULL)
                free_aligned(pData);

            nChannels       = 0;
            nRank           = 0;
            nMaxRank        = 0;
            nOffset         = 0;
            fPhase          = 0.0f;
            vChannels       = NULL;
            vFftBuf         = NULL;
            pWnd            = NULL;
            bUpdate         = true;

            pFunc           = NULL;
            pObject         = NULL;
            pSubject        = NULL;
        }

        void MultiSpectralProcessor::bind_handler(multi_spectral_processor_func_t func, void *object, void *subject)
        {
            pFunc           = func;
            pObject         = object;
            pSubject        = subject;
        }

        void MultiSpectralProcessor::unbind_handler()
        {
            pFunc           = NULL;
            pObject         = NULL;
            pSubject        = NULL;
        }

        status_t MultiSpectralProcessor::bind(size_t index, float *out, const float *in)
        {
            if (pData == NULL)
                return STATUS_BAD_STATE;
            if (index >= nChannels)
                return STATUS_INVALID_VALUE;

            channel_t *c    = &vChannels[index];
            c->pIn          = in;
            c->pOut         = out;

            return STATUS_OK;
        }

        status_t MultiSpectralProcessor::bind_in(size_t index, const float *in)
        {
            if (pData == NULL)
                return STATUS_BAD_STATE;
            if (index >= nChannels)
                return STATUS_INVALID_VALUE;

            channel_t *c    = &vChannels[index];
            c->pIn          = in;

            return STATUS_OK;
        }

        status_t MultiSpectralProcessor::bind_out(size_t index, float *out)
        {
            if (pData == NULL)
                return STATUS_BAD_STATE;
            if (index >= nChannels)
                return STATUS_INVALID_VALUE;

            channel_t *c    = &vChannels[index];
            c->pOut         = out;

            return STATUS_OK;
        }

        status_t MultiSpectralProcessor::unbind(size_t index)
        {
            return bind(index, NULL, NULL);
        }

        status_t MultiSpectralProcessor::unbind_in(size_t index)
        {
            return bind_in(index, NULL);
        }

        status_t MultiSpectralProcessor::unbind_out(size_t index)
        {
            return bind_out(index, NULL);
        }

        void MultiSpectralProcessor::unbind_all()
        {
            if (pData == NULL)
                return;

            // Update buffer pointers
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];
                c->pIn          = NULL;
                c->pOut         = NULL;
            }
        }

        void MultiSpectralProcessor::clear_buffers()
        {
            const size_t szof_buf       = 1 << nRank;
            const size_t szof_fft_buf   = szof_buf * 2;

            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];

                // Clear buffers and reset pointers
                dsp::fill_zero(c->pInBuf, szof_buf);
                dsp::fill_zero(c->pOutBuf, szof_buf);
                dsp::fill_zero(c->pFftBuf, szof_fft_buf);
            }
        }

        void MultiSpectralProcessor::update_settings()
        {
            // Distribute buffers
            const size_t szof_buf       = 1 << nRank;
            const size_t szof_wnd       = szof_buf;
            const size_t szof_fft_buf   = szof_buf * 2;

            float *ptr                  = &pWnd[szof_wnd];

            // Update buffer pointers
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];

                c->pInBuf       = advance_ptr_bytes<float>(ptr, szof_buf);
                c->pOutBuf      = advance_ptr_bytes<float>(ptr, szof_buf);
                c->pFftBuf      = advance_ptr_bytes<float>(ptr, szof_fft_buf);
            }

            // Clear buffers and initialize cosine window
            clear_buffers();
            windows::cosine(pWnd, szof_buf);

            // Mark settings applied
            nOffset         = szof_buf * (fPhase * 0.5f);
            bUpdate         = false;
        }

        void MultiSpectralProcessor::set_phase(float phase)
        {
            fPhase          = lsp_limit(phase, 0.0f, 1.0f);
            bUpdate         = true;
        }

        void MultiSpectralProcessor::set_rank(size_t rank)
        {
            if ((rank == nRank) || (rank > nMaxRank))
                return;

            nRank           = rank;
            bUpdate         = true;
        }

        void MultiSpectralProcessor::process(size_t count)
        {
            // Check if we need to commit new settings
            if (bUpdate)
                update_settings();

            const size_t buf_size       = 1 << nRank;
            const size_t frame_size     = 1 << (nRank - 1);

            for (size_t offset = 0; offset < count; )
            {
                // Need to perform transformations?
                if (nOffset >= frame_size)
                {
                    if (pFunc != NULL)
                    {
                        // Perform direct FFT
                        for (size_t i=0; i<nChannels; ++i)
                        {
                            channel_t *c = &vChannels[i];

                            if (c->pIn != NULL)
                            {
                                // Perform FFT and processing
                                dsp::mul3(&c->pFftBuf[buf_size], c->pInBuf, pWnd, buf_size);        // Apply cosine window before transform
                                dsp::pcomplex_r2c(c->pFftBuf, &c->pFftBuf[buf_size], buf_size);     // Convert from real to packed complex
                                dsp::packed_direct_fft(c->pFftBuf, c->pFftBuf, nRank);              // Perform direct FFT
                                vFftBuf[i]      = c->pFftBuf;
                            }
                            else
                            {
                                dsp::mul3(c->pFftBuf, c->pInBuf, pWnd, buf_size);                   // Copy data to FFT buffer
                                vFftBuf[i]      = NULL;
                            }
                        }

                        // Call the function
                        pFunc(pObject, pSubject, vFftBuf, nRank);                           // Call the function

                        // Perform reverse FFT
                        for (size_t i=0; i<nChannels; ++i)
                        {
                            channel_t *c = &vChannels[i];

                            if ((c->pIn != NULL) && (c->pOut != NULL))
                            {
                                dsp::packed_reverse_fft(c->pFftBuf, c->pFftBuf, nRank);             // Perform reverse FFT
                                dsp::pcomplex_c2r(c->pFftBuf, c->pFftBuf, buf_size);                // Unpack complex numbers
                            }
                        }
                    }
                    else
                    {
                        // Copy data of input buffer to FFT buffer
                        for (size_t i=0; i<nChannels; ++i)
                        {
                            channel_t *c = &vChannels[i];
                            dsp::mul3(c->pFftBuf, c->pInBuf, pWnd, buf_size);
                        }
                    }

                    // Apply signal to buffers
                    for (size_t i=0; i<nChannels; ++i)
                    {
                        channel_t *c = &vChannels[i];
                        dsp::move(c->pOutBuf, &c->pOutBuf[frame_size], frame_size);             // Shift output buffer
                        dsp::fill_zero(&c->pOutBuf[frame_size], frame_size);                    // Fill tail of input buffer with zeros
                        dsp::fmadd3(c->pOutBuf, c->pFftBuf, pWnd, buf_size);                    // Apply cosine window (-> squared cosine) and add to the output buffer
                        dsp::move(c->pInBuf, &c->pInBuf[frame_size], frame_size);               // Shift input buffer
                    }

                    // Reset read/write offset
                    nOffset     = 0;
                }

                // Estimate number of samples to process
                const size_t to_process     = lsp_min(frame_size - nOffset, count - offset);

                // Copy data between input and output buffers
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c = &vChannels[i];
                    if (c->pIn != NULL)
                    {
                        dsp::copy(&c->pInBuf[frame_size + nOffset], c->pIn, to_process);
                        c->pIn                 += to_process;
                    }
                    else
                        dsp::fill_zero(&c->pInBuf[frame_size + nOffset], to_process);

                    if (c->pOut != NULL)
                    {
                        dsp::copy(c->pOut, &c->pOutBuf[nOffset], to_process);
                        c->pOut                += to_process;
                    }
                }

                // Update pointers
                nOffset    += to_process;
                offset     += to_process;
            }
        }

        size_t MultiSpectralProcessor::remaining() const
        {
            const size_t frame_size     = 1 << (nRank - 1);
            return frame_size - nOffset;
        }

        void MultiSpectralProcessor::reset()
        {
            if (bUpdate) // update_settings() will automaticaly clear the buffer
                return;
            if (pData == NULL)
                return;

            clear_buffers();
        }

        void MultiSpectralProcessor::dump(IStateDumper *v) const
        {
            v->write("nChannels", nChannels);
            v->write("nRank", nRank);
            v->write("nMaxRank", nMaxRank);
            v->write("nOffset", nOffset);

            v->begin_array("vChannels", vChannels, nChannels);
            {
                for (size_t i=0; i<nChannels; ++i)
                {
                    const channel_t *c      = &vChannels[i];

                    v->write("pIn", c->pIn);
                    v->write("pOut", c->pOut);
                    v->write("pInBuf", c->pInBuf);
                    v->write("pOutBuf", c->pOutBuf);
                    v->write("pFftBuf", c->pFftBuf);
                }
            }
            v->end_array();

            v->writev("vFftBuf", vFftBuf, nChannels);
            v->write("pWnd", pWnd);
            v->write("fPhase", fPhase);
            v->write("bUpdate", bUpdate);

            v->write("pFunc", pFunc);
            v->write("pObject", pObject);
            v->write("pSubject", pSubject);

            v->write("pData", pData);
        }
    } /* namespace dspu */
} /* namespace lsp */



