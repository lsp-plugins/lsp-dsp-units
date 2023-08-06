/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 6 авг. 2023 г.
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

#include <lsp-plug.in/dsp-units/util/SpectralSplitter.h>
#include <lsp-plug.in/dsp-units/misc/windows.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace dspu
    {
        SpectralSplitter::SpectralSplitter()
        {
            construct();
        }

        SpectralSplitter::~SpectralSplitter()
        {
            destroy();
        }

        void SpectralSplitter::construct()
        {
            nRank           = 0;
            nMaxRank        = 0;
            fPhase          = 0.0f;
            vWnd            = NULL;
            vInBuf          = NULL;
            vFftBuf         = NULL;
            vFftTmp         = NULL;
            nFrameOffset    = 0;
            nInOffset       = 0;
            bUpdate         = true;
            vHandlers       = NULL;
            nHandlers       = 0;
            nBindings       = 0;
            pData           = NULL;
        }

        status_t SpectralSplitter::init(size_t max_rank, size_t handlers)
        {
            if (nMaxRank < 5)
                return STATUS_INVALID_VALUE;

            nRank           = max_rank;
            nMaxRank        = max_rank;
            fPhase          = 0.0f;
            bUpdate         = true;
            vWnd            = NULL;
            vInBuf          = NULL;
            vFftBuf         = NULL;
            vFftTmp         = NULL;
            vHandlers       = NULL;
            nHandlers       = 0;
            nBindings       = 0;

            // Free data if it was previously allocated
            if (pData != NULL)
            {
                free_aligned(pData);
                pData           = NULL;
            }

            // Allocate buffer
            size_t hdl_sz   = align_size(sizeof(handler_t) * handlers, DEFAULT_ALIGN);
            size_t buf_sz   = sizeof(float) << max_rank;
            size_t to_alloc =
                hdl_sz + // vHandlers
                buf_sz + // vWindow
                buf_sz * 2 + // vInBuf
                buf_sz * 2 + // vFftBuf
                buf_sz * 2 + // vFftTmp
                handlers * buf_sz * 2; // vHandlers[i].pOutBuf

            uint8_t *ptr    = alloc_aligned<uint8_t>(pData, to_alloc, DEFAULT_ALIGN);
            if (ptr == NULL)
                return false;

            vHandlers       = reinterpret_cast<handler_t *>(ptr);
            ptr            += hdl_sz;
            vWnd            = reinterpret_cast<float *>(ptr);
            ptr            += buf_sz;
            vInBuf          = reinterpret_cast<float *>(ptr);
            ptr            += buf_sz * 2;
            vFftBuf         = reinterpret_cast<float *>(ptr);
            ptr            += buf_sz * 2;
            vFftTmp         = reinterpret_cast<float *>(ptr);
            ptr            += buf_sz * 2;

            // Initialize handlers
            for (size_t i=0; i<handlers; ++i)
            {
                handler_t *h    = &vHandlers[i];

                h->pObject      = NULL;
                h->pSubject     = NULL;
                h->pFunc        = NULL;
                h->pSink        = NULL;
                h->vOutBuf      = reinterpret_cast<float *>(ptr);
                ptr            += buf_sz * 2;
            }

            nHandlers       = handlers;

            return STATUS_OK;
        }

        void SpectralSplitter::destroy()
        {
            if (pData != NULL)
            {
                free_aligned(pData);
                pData           = NULL;
            }

            nRank           = 0;
            nMaxRank        = 0;
            fPhase          = 0.0f;
            vWnd            = NULL;
            vInBuf          = NULL;
            vFftBuf         = NULL;
            vFftTmp         = NULL;
            bUpdate         = false;
            vHandlers       = NULL;
            nHandlers       = 0;
            nBindings       = 0;
            pData           = NULL;
        }

        status_t SpectralSplitter::bind(
            size_t id,
            void *object,
            void *subject,
            spectral_splitter_func_t func,
            spectral_splitter_sink_t sink)
        {
            if (id >= nHandlers)
                return STATUS_OVERFLOW;
            if ((func == NULL) && (sink == NULL))
                return STATUS_INVALID_VALUE;

            handler_t *h = &vHandlers[id];
            if ((h->pFunc == NULL) &&
                (h->pSink == NULL))
                ++nBindings;

            h->pObject      = object;
            h->pSubject     = subject;
            h->pFunc        = func;
            h->pSink        = sink;
            h->nOutOffset   = 0;

            size_t buf_size = 1 << nRank;
            dsp::fill_zero(h->vOutBuf, buf_size);

            return STATUS_OK;
        }

        status_t SpectralSplitter::unbind(size_t id)
        {
            if (id >= nHandlers)
                return STATUS_OVERFLOW;

            handler_t *h = &vHandlers[id];
            if ((h->pFunc == NULL) &&
                (h->pSink == NULL))
                return STATUS_NOT_BOUND;

            h->pObject      = NULL;
            h->pSubject     = NULL;
            h->pFunc        = NULL;
            h->pSink        = NULL;
            --nBindings;

            return STATUS_OK;
        }

        void SpectralSplitter::update_settings()
        {
            if (!bUpdate)
                return;

            // Distribute buffers
            nRank               = lsp_min(nRank, nMaxRank);
            nChunkRank          = (nUserChunkRank > 0) ? lsp_limit(nUserChunkRank, 5, ssize_t(nRank)) : nRank;

            size_t buf_size     = 1 << nRank;
            size_t frame_size   = 1 << (nChunkRank - 1);

            // Clear buffers and reset pointers
            windows::sqr_cosine(vWnd, buf_size);
            dsp::fill_zero(vInBuf, buf_size*2);
            dsp::fill_zero(vFftBuf, buf_size*2);

            for (size_t i=0; i<nHandlers; ++i)
            {
                handler_t *h = &vHandlers[i];
                if ((h->pFunc != NULL) &&
                    (h->pSink != NULL))
                {
                    dsp::fill_zero(h->vOutBuf, buf_size);
                    h->nOutOffset   = 0;
                }
            }

            nFrameOffset        = frame_size * (fPhase * 0.5f);
            nInOffset           = 0;

            // Mark settings applied
            bUpdate             = false;
        }

        void SpectralSplitter::set_phase(float phase)
        {
            fPhase          = lsp_limit(phase, 0.0f, 1.0f);
            bUpdate         = true;
        }

        void SpectralSplitter::set_rank(size_t rank)
        {
            if ((rank == nRank) || (rank > nMaxRank))
                return;

            nRank           = rank;
            bUpdate         = true;
        }

        void SpectralSplitter::set_chunk_rank(ssize_t rank)
        {
            if (rank == nUserChunkRank)
                return;

            nUserChunkRank  = rank;
            bUpdate         = true;
        }

        size_t SpectralSplitter::latency() const
        {
            if (!bUpdate)
                return 1 << nChunkRank;

            size_t rank         = lsp_min(rank, nMaxRank);
            size_t chunk_rank   = (nUserChunkRank > 0) ? lsp_limit(nUserChunkRank, 5, ssize_t(nRank)) : nRank;

            return 1 << chunk_rank;
        }

        void SpectralSplitter::process(const float *src, size_t count)
        {
            // Check if we need to commit new settings
            update_settings();
            if (nBindings <= 0)
                return;

            size_t buf_size     = 1 << nRank;
            size_t frame_size   = 1 << (nChunkRank - 1);

            while (count > 0)
            {
                // Need to perform transformations?
                if (nFrameOffset >= frame_size)
                {
                    // Perform FFT and processing
                    dsp::pcomplex_r2c(vFftBuf, vInBuf, buf_size);       // Convert from real to packed complex
                    dsp::packed_direct_fft(vFftBuf, vFftBuf, nRank);    // Perform direct FFT

                    for (size_t i=0; i<nHandlers; ++i)
                    {
                        handler_t *h    = &vHandlers[i];
                        if (h->pFunc != NULL)
                        {
                            h->pFunc(h->pObject, h->pSubject, vFftTmp, vFftBuf, nRank);
                            dsp::packed_reverse_fft(vFftTmp, vFftTmp, nRank);   // Perform reverse FFT
                            dsp::pcomplex_c2r(vFftTmp, vFftTmp, buf_size);      // Unpack complex numbers
                        }
                        else
                            dsp::copy(vFftTmp, vInBuf, buf_size);               // Copy data to FFT buffer

                        if (h->pSink != NULL)
                        {
                            // Apply window and add to the output buffer
                            dsp::fmadd3(&h->vOutBuf[h->nOutOffset + frame_size], vFftBuf, vWnd, buf_size);  // Apply window function

                            // Call sink
                            h->pSink(h->pObject, h->pSubject, &h->vOutBuf[h->nOutOffset], frame_size);

                            // Need to shift the output buffer?
                            h->nOutOffset      += frame_size;
                            if (h->nOutOffset > (buf_size*2 - frame_size))
                            {
                                dsp::move(h->vOutBuf, &h->vOutBuf[h->nOutOffset + frame_size], frame_size); // Shift buffer
                                dsp::fill_zero(&h->vOutBuf[frame_size], buf_size*2 - frame_size);           // Clear the tail
                                h->nOutOffset       = 0;
                            }
                        }
                    }

                    // Need to shift input buffer?
                    if (nInOffset >= buf_size)
                    {
                        dsp::move(vInBuf, &vInBuf[nInOffset + frame_size], frame_size);
                        dsp::fill_zero(&vInBuf[frame_size], buf_size*2 - frame_size);
                        nInOffset       = 0;
                    }

                    // Reset read/write offset
                    nFrameOffset    = 0;
                }

                // Estimate number of samples to process
                size_t to_process   = lsp_min(frame_size - nFrameOffset, count);

                // Copy data
                dsp::copy(&vInBuf[frame_size + nInOffset], src, to_process);

                // Update pointers
                nFrameOffset   += to_process;
                nInOffset      += to_process;
                count          -= to_process;
                src            += to_process;
            }
        }

        void SpectralSplitter::dump(IStateDumper *v) const
        {
            v->write("nRank", nRank);
            v->write("nMaxRank", nMaxRank);
            v->write("nUserChunkRank", nUserChunkRank);
            v->write("nChunkRank", nChunkRank);
            v->write("fPhase", fPhase);
            v->write("vWnd", vWnd);
            v->write("vInBuf", vInBuf);
            v->write("vFftBuf", vFftBuf);
            v->write("vFftTmp", vFftTmp);
            v->write("nFrameOffset", nFrameOffset);
            v->write("nInOffset", nInOffset);

            v->begin_array("vHandlers", vHandlers, nHandlers);
            {
                for (size_t i=0; i<nHandlers; ++i)
                {
                    handler_t *h        = &vHandlers[i];

                    v->begin_object(h, sizeof(handler_t));
                    {
                        v->write("pObject", h->pObject);
                        v->write("pSubject", h->pSubject);
                        v->write("pFunc", h->pFunc);
                        v->write("pSink", h->pSink);
                        v->write("vOutBuf", h->vOutBuf);
                        v->write("nOutOffset", h->nOutOffset);
                    }
                    v->end_object();
                }
            }
            v->end_array();

            v->write("nHandlers", nHandlers);
            v->write("nBindings", nBindings);
            v->write("pData", pData);
        }
    } /* namespace dspu */
} /* namespace lsp */



