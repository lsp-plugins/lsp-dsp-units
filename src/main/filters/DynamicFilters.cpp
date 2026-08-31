/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 1 февр. 2018 г.
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

#include <lsp-plug.in/dsp-units/filters/DynamicFilters.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr size_t BLD_BUF_SIZE    = 16;
        static constexpr size_t BUF_SIZE        = 0x400;       /* 1024 samples at one time */
        static constexpr size_t FBUF_SIZE       = ((BLD_BUF_SIZE * (BUF_SIZE - BLD_BUF_SIZE) * sizeof(dsp::f_cascade_t)) / sizeof(float));

        static constexpr float C_PI             = M_PI;
        static constexpr float C_PI_MUL_2       = M_PI * 2.0;
        static constexpr float C_PI_DIV_2       = M_PI_2;

        DynamicFilters::DynamicFilters()
        {
            construct();
        }

        DynamicFilters::~DynamicFilters()
        {
            destroy();
        }

        void DynamicFilters::construct()
        {
            vFilters        = NULL;
            vMemory         = NULL;
            vCascades       = NULL;
            vBiquads.ptr    = NULL;
            nFilters        = 0;
            nSampleRate     = 0;
            pData           = NULL;
            bClearMem       = false;
        }

        status_t DynamicFilters::init(size_t filters)
        {
            // Determine how many bytes to allocate
            size_t b_per_filter_t       = align_size(sizeof(filter_t) * filters, 64);
            size_t b_per_memory         = FILTER_CHAINS_MAX * 2 * filters * sizeof(float);
            size_t b_per_cascades       = align_size(BLD_BUF_SIZE * (BUF_SIZE + BLD_BUF_SIZE) * sizeof(dsp::f_cascade_t), 64);
            size_t b_per_biquad         = sizeof(dsp::biquad_x16_t) * (BUF_SIZE + BLD_BUF_SIZE);

            size_t to_alloc             = b_per_filter_t + b_per_memory + b_per_cascades + b_per_biquad;

            // Allocate memory
            uint8_t *ptr                = alloc_aligned<uint8_t>(pData, to_alloc, 64);
            if (ptr == NULL)
                return STATUS_NO_MEM;

            // Map memory
            vFilters        = advance_ptr_bytes<filter_t>(ptr, b_per_filter_t);
            vMemory         = advance_ptr_bytes<float>(ptr, b_per_memory);
            vCascades       = advance_ptr_bytes<dsp::f_cascade_t>(ptr, b_per_cascades);
            vBiquads.ptr    = ptr;
            nFilters        = filters;

            // Initialize all filters with default values
            for (size_t i=0; i<filters; ++i)
            {
                filter_t *f         = &vFilters[i];
                filter_params_t *fp = &f->sParams;

                fp->nType       = FLT_NONE;
                fp->fFreq       = 0.0f;
                fp->fFreq2      = 0.0f;
                fp->fGain       = 0.0f;
                fp->nSlope      = 0;
                fp->fQuality    = 0.0f;
                f->bActive      = false;
            }

            // Cleanup filter memory
            dsp::fill_zero(vMemory, FILTER_CHAINS_MAX * 2 * filters);

            return STATUS_OK;
        }

        void DynamicFilters::destroy()
        {
            if (pData != NULL)
                free_aligned(pData);

            construct();
        }

        void DynamicFilters::set_sample_rate(size_t sr)
        {
            nSampleRate         = sr;
        }

        bool DynamicFilters::set_params(size_t id, const filter_params_t *params)
        {
            if (id >= nFilters)
                return false;
            filter_params_t *fp = &vFilters[id].sParams;
            if (fp->nType != params->nType)
                bClearMem           = true;

            *fp     = *params;

            // Swap frequencies if f2 < f for band-filters
            switch (fp->nType)
            {
                case FLT_BT_RLC_LADDERPASS:
                case FLT_MT_RLC_LADDERPASS:
                case FLT_BT_RLC_LADDERREJ:
                case FLT_MT_RLC_LADDERREJ:
                case FLT_BT_RLC_BANDPASS:
                case FLT_MT_RLC_BANDPASS:
                case FLT_BT_BWC_LADDERPASS:
                case FLT_MT_BWC_LADDERPASS:
                case FLT_BT_BWC_LADDERREJ:
                case FLT_MT_BWC_LADDERREJ:
                case FLT_BT_BWC_BANDPASS:
                case FLT_MT_BWC_BANDPASS:
                case FLT_BT_LRX_LADDERPASS:
                case FLT_MT_LRX_LADDERPASS:
                case FLT_BT_LRX_LADDERREJ:
                case FLT_MT_LRX_LADDERREJ:
                case FLT_BT_LRX_BANDPASS:
                case FLT_MT_LRX_BANDPASS:
                    if (fp->fFreq2 < fp->fFreq)
                    {
                        float f     = fp->fFreq;
                        fp->fFreq   = fp->fFreq2;
                        fp->fFreq2  = f;
                    }
                    break;
                default:
                    break;
            }

            // Tranform second frequency
            if (fp->nType & 1) // Bilinear transform
            {
                float nf    = C_PI / float(nSampleRate);
                fp->fFreq2  = tanf(fp->fFreq * nf) / tanf(fp->fFreq2 * nf);
            }
            else // Matched transform
            {
                fp->fFreq2  = fp->fFreq / fp->fFreq2;    // Normalize frequency
            }

            return true;
        }

        bool DynamicFilters::get_params(size_t id, filter_params_t *params)
        {
            if (id >= nFilters)
                return false;
            *params = vFilters[id].sParams;
            return true;
        }

        size_t DynamicFilters::quantify(size_t c, size_t nc)
        {
            // Check if there are some cascades left undone
            const ssize_t n = nc - c;
            if (n <= 0)
                return 0;

            if (n >= 8)
                return (n >= 16) ? 16 : 8;
            if (n >= 4)
                return 4;
            return (n >= 2) ? 2 : 1;
        }

        void DynamicFilters::process(size_t id, float *out, const float *in, const float *gain, size_t samples)
        {
            // Bypass inactive or non-existing filter
            filter_t *f     = (id < nFilters) ? &vFilters[id] : NULL;
            if ((f == NULL)  || (!f->bActive) || (f->sParams.nType == FLT_NONE) || (f->sParams.nSlope == 0) || (nSampleRate == 0))
            {
                dsp::copy(out, in, samples);
                return;
            }

            // Cleanup filter memory
            if (bClearMem)
            {
                dsp::fill_zero(vMemory, FILTER_CHAINS_MAX * 2 * nFilters);
                bClearMem = false;
            }

            // Frequency coefficient for bilinear transform
            const float kf =
                (f->sParams.nType <= FLT_MT_AMPLIFIER) ? 0.95f :
                (f->sParams.nType & 1) ?
                1.0f / tanf(f->sParams.fFreq * C_PI / float(nSampleRate)) : // bilinear transform coefficient
                C_PI_MUL_2 / nSampleRate; // Matched transfomr coefficient

            // Filter memory
            while (samples > 0)
            {
                // Initialize counter
                size_t to_process       = (samples > BUF_SIZE) ? BUF_SIZE : samples;
                float *fmem             = &vMemory[id * FILTER_CHAINS_MAX * 2];
                const float *src        = in;
                size_t cj               = 0;

                // Process all cascades
                while (true)
                {
                    // Generate cascades
                    size_t nj               = build_filter_bank(vCascades, &f->sParams, cj, gain, to_process);
                    if (nj <= 0)
                        break;

                    switch (nj)
                    {
                        case 16:
                            dsp::fcascade_fill_x16(vCascades, &vCascades[to_process << 4]);

                            if (f->sParams.nType & 1)
                                dsp::bilinear_transform_x16(vBiquads.x16, vCascades, kf, to_process + 15);
                            else
                                dsp::matched_transform_x16(vBiquads.x16, vCascades, f->sParams.fFreq, kf, to_process + 15);
                            dsp::dyn_biquad_process_x16(out, src, fmem, to_process, vBiquads.x16);
                            break;

                        case 8:
                            dsp::fcascade_fill_x8(vCascades, &vCascades[to_process << 3]);

                            if (f->sParams.nType & 1)
                                dsp::bilinear_transform_x8(vBiquads.x8, vCascades, kf, to_process + 7);
                            else
                                dsp::matched_transform_x8(vBiquads.x8, vCascades, f->sParams.fFreq, kf, to_process + 7);
                            dsp::dyn_biquad_process_x8(out, src, fmem, to_process, vBiquads.x8);
                            break;

                        case 4:
                            dsp::fcascade_fill_x4(vCascades, &vCascades[to_process << 2]);

                            if (f->sParams.nType & 1)
                                dsp::bilinear_transform_x4(vBiquads.x4, vCascades, kf, to_process + 3);
                            else
                                dsp::matched_transform_x4(vBiquads.x4, vCascades, f->sParams.fFreq, kf, to_process + 3);
                            dsp::dyn_biquad_process_x4(out, src, fmem, to_process, vBiquads.x4);
                            break;

                        case 2:
                            dsp::fcascade_fill_x2(vCascades, &vCascades[to_process << 1]);
                            if (f->sParams.nType & 1)
                                dsp::bilinear_transform_x2(vBiquads.x2, vCascades, kf, to_process + 1);
                            else
                                dsp::matched_transform_x2(vBiquads.x2, vCascades, f->sParams.fFreq, kf, to_process + 1);
                            dsp::dyn_biquad_process_x2(out, src, fmem, to_process, vBiquads.x2);
                            break;

                        case 1:
                        default:
                            if (f->sParams.nType & 1)
                                dsp::bilinear_transform_x1(vBiquads.x1, vCascades, kf, to_process);
                            else
                                dsp::matched_transform_x1(vBiquads.x1, vCascades, f->sParams.fFreq, kf, to_process);
                            dsp::dyn_biquad_process_x1(out, src, fmem, to_process, vBiquads.x1);
                            break;
                    }

                    // Update counters and pointers
                    cj                     += nj;
                    fmem                   += nj*2;
                    src                     = out;
                }

                // Update samples and pointers
                samples                -= to_process;
                gain                   += to_process;
                out                    += to_process;
                in                     += to_process;
            }
        }

        size_t DynamicFilters::precalc_lrx_ladder_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, const float *sfg, size_t samples)
        {
            size_t slope            = fp->nSlope * 4;
            size_t nc               = quantify(cj, slope); // Number of cascades to generate
            if (nc <= 0)
                return nc;

            // Initialize buffer
            dsp::f_cascade_t buf[BLD_BUF_SIZE];
            dsp::fill_zero(buf[0].t, (sizeof(dsp::f_cascade_t)*BLD_BUF_SIZE)/sizeof(float));
            size_t bptr = 0;

            // Pre-calculate some values
            dsp::f_cascade_t *c     = dst;
            for (size_t i=0; i<samples; ++i)
            {
                dsp::f_cascade_t *xc = &buf[bptr];

                xc->t[0]        = sqrtf(sfg[i]); // gain
                xc->t[1]        = 1.0f / xc->t[0]; // 1/gain
                xc->t[2]        = dsp::irootf(xc->t[0], uint32_t(slope)); // fg
                xc->t[3]        = 1.0f / xc->t[2];  // 1 / fg
                xc->b[0]        = 1.0f / (1.0f + fp->fQuality * (1.0f - expf(2.0f - xc->t[0] - xc->t[1]))); // k

                // Copy values
                for (size_t j=0; j<nc; ++j)
                    *(c++)          = buf[(bptr + j) & (BLD_BUF_SIZE-1)];

                bptr        = (bptr + BLD_BUF_SIZE-1) & (BLD_BUF_SIZE-1);
            }
            for (size_t i=nc; i>0; --i) // Complete tail
            {
                // Copy values
                for (size_t j=0; j<nc; ++j)
                    *(c++)          = buf[(bptr + j) & (BLD_BUF_SIZE-1)];
                bptr        = (bptr + BLD_BUF_SIZE-1) & (BLD_BUF_SIZE-1);
            }

            return nc;
        }

        typedef struct precalc_t
        {
            float theta;
            float tcos;
            float tcos2;
            float tsin2;
            float xtcos;
            float xtcos_xf;
        } precalc_t;

        void DynamicFilters::calc_lrx_ladder_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, size_t samples, size_t ftype, size_t nc)
        {
            size_t slope            = fp->nSlope * 4;
            dsp::f_cascade_t *c     = dst;

            precalc_t pc[BLD_BUF_SIZE];
            precalc_t *ppc;

            float gain, k, kf;
            float xf            = fp->fFreq2;
            float xf2           = xf * xf;

            for (size_t j=0; j<nc; ++j)
            {
                ppc             = &pc[j];
                ppc->theta      = ((((cj+j)&(~3)) + 2)*C_PI_DIV_2) / float(slope);
                ppc->tcos       = cosf(ppc->theta);
                ppc->tcos2      = ppc->tcos * ppc->tcos;
                ppc->tsin2      = 1.0f - ppc->tcos2;
                ppc->xtcos      = 2.0f * ppc->tcos;
                ppc->xtcos_xf   = 2.0f * ppc->tcos * xf;
            }

            size_t loops = samples + nc - 1;

            if (ftype == FLT_BT_LRX_LADDERPASS)
            {
                for (size_t i=0; i<loops; ++i)
                {
                    ppc = pc;

                    for (size_t j=0; j<nc; ++j)
                    {
                        k                   = c->b[0];
                        kf                  = ppc->tsin2 + k*k * ppc->tcos2;

                        if ((cj + j) & 1)
                        {
                            gain                = c->t[1];

                            // Transfer function
                            c->b[0]             = kf * c->t[3];
                            c->b[1]             = k * ppc->xtcos_xf;
                            c->b[2]             = c->t[2] * xf2;

                            c->t[0]             = c->t[2];
                            c->t[1]             = c->b[1];
                            c->t[2]             = c->b[0] * xf2;
                        }
                        else
                        {
                            gain                = c->t[0];

                            // Transfer function
                            c->t[0]             = kf * c->t[3];
                            c->t[1]             = k * ppc->xtcos;

                            c->b[0]             = c->t[2];
                            c->b[1]             = c->t[1];
                            c->b[2]             = c->t[0];
                        }

                        if (!((cj+j)&(~1)))
                        {
                            c->t[0]            *= gain;
                            c->t[1]            *= gain;
                            c->t[2]            *= gain;
                        }

                        // Move to next cascade
                        c       ++;
                        ppc     ++;
                    } // for j
                } // for i
            }
            else
            {
                for (size_t i=0; i<loops; ++i)
                {
                    ppc = pc;

                    for (size_t j=0; j<nc; ++j)
                    {
                        k                   = c->b[0];
                        kf                  = ppc->tsin2 + k*k * ppc->tcos2;

                        if ((cj + j) & 1)
                        {
                            gain                = c->t[0];

                            // Transfer function
                            c->b[0]             = kf * c->t[2];
                            c->b[1]             = k * ppc->xtcos_xf;
                            c->b[2]             = c->t[3] * xf2;

                            c->t[0]             = c->t[3]; // fg;
                            c->t[1]             = c->b[1];
                            c->t[2]             = c->b[0] * xf2;
                        }
                        else
                        {
                            gain                = c->t[0];

                            // Transfer function
                            c->b[0]             = kf * c->t[3];
                            c->b[1]             = k * ppc->xtcos;
                            c->b[2]             = c->t[2];

                            c->t[0]             = c->b[2];
                            c->t[1]             = c->b[1];
                            c->t[2]             = c->b[0];
                        }

                        if (!((cj+j)&(~1)))
                        {
                            c->t[0]            *= gain;
                            c->t[1]            *= gain;
                            c->t[2]            *= gain;
                        }

                        // Move to next cascade
                        c       ++;
                        ppc     ++;
                    } // for j
                } // for i
            }
        }

        size_t DynamicFilters::build_lrx_ladder_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, const float *sfg, size_t samples, size_t ftype)
        {
            size_t nc = precalc_lrx_ladder_filter_bank(dst, fp, cj, sfg, samples);
            if (nc <= 0)
                return nc;

            calc_lrx_ladder_filter_bank(dst, fp, cj, samples, ftype, nc);
            return nc;
        }

        size_t DynamicFilters::build_lrx_shelf_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, const float *sfg, size_t samples, size_t ftype)
        {
            size_t nc               = quantify(cj, fp->nSlope*2); // Number of cascades to generate (doubled for LRX filters)
            if (nc <= 0)
                return nc;

            // Initialize buffer
            dsp::f_cascade_t buf[BLD_BUF_SIZE];
            dsp::fill_zero(buf[0].t, (sizeof(dsp::f_cascade_t)*BLD_BUF_SIZE)/sizeof(float));
            size_t bptr = 0;

            // Pre-calculate some values
            dsp::f_cascade_t *c     = dst;
            for (size_t i=0; i<samples; ++i)
            {
                dsp::f_cascade_t *xc = &buf[bptr];

                xc->b[3]        = sqrtf(sfg[i]);
                xc->t[0]        = sqrtf(xc->b[3]); // gain
                xc->t[1]        = 1.0f / xc->t[0]; // 1/gain
                xc->t[2]        = dsp::irootf(sqrtf(xc->t[0]), fp->nSlope); // fg
                xc->t[3]        = 1.0f / xc->t[2];  // 1 / fg
                xc->b[0]        = 1.0f / (1.0f + fp->fQuality * (1.0f - expf(2.0f - xc->t[0] - xc->t[1]))); // k

                // Copy values
                for (size_t j=0; j<nc; ++j)
                    *(c++)          = buf[(bptr + j) & (BLD_BUF_SIZE-1)];

                bptr        = (bptr + BLD_BUF_SIZE-1) & (BLD_BUF_SIZE-1);
            }
            for (size_t i=nc; i>0; --i) // Complete tail
            {
                // Copy values
                for (size_t j=0; j<nc; ++j)
                    *(c++)          = buf[(bptr + j) & (BLD_BUF_SIZE-1)];
                bptr        = (bptr + BLD_BUF_SIZE-1) & (BLD_BUF_SIZE-1);
            }

            // Create cascades
            for (size_t j=0; j<nc; ++j)
            {
                // Intialize pointers
                dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                // Pre-calculate filter parameters
                float theta         = (((cj & (~1)) + 1)*C_PI_DIV_2)/(2*fp->nSlope); // The number of cascades is just doubled
                float tcos          = cosf(theta);
                float tcos2         = tcos * tcos;      // cos^2
                float tsin2         = 1.0f - tcos2;     // sin^2

                float xtcos         = 2.0f * tcos;

                float k, kf;

                if (ftype == FLT_BT_LRX_HISHELF)
                {
                    // Generate cascades
                    for (size_t i=0; i<samples; ++i)
                    {
                        k                   = c->b[0];
                        kf                  = tsin2 + k*k * tcos2;

                        // Transfer function
                        c->t[0]             = kf * c->t[3];
                        c->t[1]             = k * xtcos;

                        c->b[0]             = c->t[2];
                        c->b[1]             = c->t[1];
                        c->b[2]             = c->t[0];

                        // Move to next cascade
                        c                  += nc;
                    }
                }
                else
                {
                    // Generate cascades
                    for (size_t i=0; i<samples; ++i)
                    {
                        k                   = c->b[0];
                        kf                  = tsin2 + k*k * tcos2;

                        // Transfer function
                        c->b[0]             = kf * c->t[3];
                        c->b[1]             = k * xtcos;
                        c->b[2]             = c->t[2];

                        c->t[0]             = c->b[2];
                        c->t[1]             = c->b[1];
                        c->t[2]             = c->b[0];

                        // Move to next cascade
                        c                  += nc;
                    }
                }

                // Patch volume
                if (cj == 0)
                {
                    dsp::f_cascade_t *c     = dst;       // target cascade pointer
                    for (size_t i=0; i<samples; ++i)
                    {
                        c->t[0]            *= c->b[3];
                        c->t[1]            *= c->b[3];
                        c->t[2]            *= c->b[3];
                        c                  += nc;
                    }
                }

                // Increment cascade number
                cj++;
            }

            return nc;
        }

        size_t DynamicFilters::build_filter_bank(dsp::f_cascade_t *dst, const filter_params_t *fp, size_t cj, const float *sfg, size_t samples)
        {
            size_t nc;
            size_t ftype = (fp->nType & 1) ? fp->nType : fp->nType-1 ; // xxx_MT -> xxx_BT

            switch (ftype)
            {
                //----------------------------------- MISC SPECIAL FILTERS ------------------------------------
                case FLT_BT_AMPLIFIER:
                {
                    if (cj >= 1)
                        return 0;
                    nc          = 1;

                    // Generate cascades
                    dsp::f_cascade_t *c = dst;       // target cascade pointer

                    for (size_t i=0; i<samples; ++i)
                    {
                        c->t[0]     = sfg[i];
                        c->t[1]     = 0.0f;
                        c->t[2]     = 0.0f;

                        c->b[0]     = 1.0f;
                        c->b[1]     = 0.0f;
                        c->b[2]     = 0.0f;

                        // Move to next cascade
                        c           ++;
                    }

                    break;
                }

                //----------------------------------- RLC FILTERS ------------------------------------
                case FLT_BT_RLC_LOPASS:
                case FLT_BT_RLC_HIPASS:
                {
                    nc                      = quantify(cj, (fp->nSlope >> 1) + (fp->nSlope & 1)); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;
                    size_t j                = 0;

                    // Add cascade with one pole
                    if ((cj == 0) && (fp->nSlope & 1))
                    {
                        dsp::f_cascade_t *c     = dst;       // target cascade pointer

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            c->b[0]     = 1.0f;
                            c->b[1]     = 1.0f;
                            c->b[2]     = 0.0f;

                            c->t[0]     = (ftype == FLT_BT_RLC_LOPASS) ? sfg[i] : 0.0f;
                            c->t[1]     = (ftype != FLT_BT_RLC_LOPASS) ? sfg[i] : 0.0f;
                            c->t[2]     = 0.0f;

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Increment cascade number
                        cj++;
                        j++;
                    }

                    // Add additional cascades
                    for (; j<nc; j++)
                    {
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            // Transfer function
                            c->b[0]             = 1.0f;
                            c->b[1]             = 2.0f / (1.0f + fp->fQuality);
                            c->b[2]             = 1.0f;

                            c->t[0]             = (ftype == FLT_BT_RLC_LOPASS) ? 1.0f : 0.0f;
                            c->t[1]             = 0.0f;
                            c->t[2]             = (ftype != FLT_BT_RLC_LOPASS) ? 1.0f : 0.0f;

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Patch volume
                        if (cj == 0)
                        {
                            dsp::f_cascade_t *c = dst;       // target cascade pointer
                            for (size_t i=0; i<samples; ++i)
                            {
                                float gain         = sfg[i];
                                c->t[0]            *= gain;
                                c->t[1]            *= gain;
                                c->t[2]            *= gain;
                                c                  += nc;
                            }
                        }

                        cj++;
                    }

                    break;
                }

                case FLT_BT_RLC_LOSHELF:
                case FLT_BT_RLC_HISHELF:
                {
                    const size_t slope      = fp->nSlope * 2;
                    nc                      = quantify(cj, fp->nSlope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    const float rslope      = 1.0f / float(slope);
                    const float rquality    = 2.0f / (1.0f + fp->fQuality);

                    for (size_t j=0; j<nc; j++)
                    {
                        // Intialize pointers
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            float gain          = sqrtf(sfg[i]);
                            float fg            = expf(logf(gain) * rslope);

                            float *t            = (ftype == FLT_BT_RLC_LOSHELF) ? c->t : c->b;
                            float *b            = (ftype == FLT_BT_RLC_LOSHELF) ? c->b : c->t;

                            // Transfer function
                            t[0]                = fg;
                            t[1]                = rquality;
                            t[2]                = 1.0f / fg;

                            b[0]                = t[2]; // 1.0 / fg
                            b[1]                = t[1]; // rquality
                            b[2]                = t[0]; // fg

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Patch volume
                        if (cj == 0)
                        {
                            dsp::f_cascade_t *c = dst;       // target cascade pointer
                            for (size_t i=0; i<samples; ++i)
                            {
                                float gain         = sqrtf(sfg[i]);
                                c->t[0]            *= gain;
                                c->t[1]            *= gain;
                                c->t[2]            *= gain;
                                c                  += nc;
                            }
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_RLC_LADDERPASS:
                case FLT_BT_RLC_LADDERREJ:
                {
                    size_t slope            = fp->nSlope * 2;
                    nc                      = quantify(cj, slope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    const float kf          = fp->fFreq2;
                    const float kf2         = kf*kf;
                    const float rquality    = 2.0f / (1.0f + fp->fQuality);
                    const float rslope      = 1.0f / slope;

                    for (size_t j=0; j < nc; j++)
                    {
                        // Intialize pointers
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Generate cascades
                        if (cj & 1)
                        {
                            for (size_t i=0; i<samples; ++i)
                            {
                                float gain          = (ftype == FLT_BT_RLC_LADDERREJ) ? sqrtf(sfg[i]) : sqrtf(1.0/sfg[i]);
                                float fg            = expf(logf(gain) * rslope);

                                // Second shelf cascade, hi-shelf always
                                float *t            = c->b;
                                float *b            = c->t;

                                // Create transfer function
                                t[0]                = fg;
                                t[1]                = kf * rquality;
                                t[2]                = kf2 / fg;

                                b[0]                = 1.0f / fg;
                                b[1]                = kf * rquality;
                                b[2]                = fg * kf2;

                                if ((cj>>1) == 0)
                                {
                                    c->t[0]            *= gain;
                                    c->t[1]            *= gain;
                                    c->t[2]            *= gain;
                                }

                                // Move to next cascade
                                c                  += nc;
                            }
                        }
                        else
                        {
                            // Even shelf cascade, lo-shelf for LADDERREJ, hi-shelf for LADDERPASS
                            for (size_t i=0; i<samples; ++i)
                            {
                                float gain1         = (ftype == FLT_BT_RLC_LADDERREJ) ? sqrtf(1.0/sfg[i]) : sqrtf(sfg[i]);
                                float gain2         = (ftype == FLT_BT_RLC_LADDERREJ) ? sqrtf(sfg[i]) : sqrtf(1.0/sfg[i]);
                                float fg            = (ftype == FLT_BT_RLC_LADDERREJ) ? expf(logf(gain2) * rslope) : expf(logf(gain1) * rslope);
                                float gain          = (ftype == FLT_BT_RLC_LADDERREJ) ? gain2 : gain1;

                                float *t            = (ftype == FLT_BT_RLC_LADDERREJ) ? c->t : c->b;
                                float *b            = (ftype == FLT_BT_RLC_LADDERREJ) ? c->b : c->t;

                                // Create transfer function
                                t[0]                = fg;
                                t[1]                = rquality;
                                t[2]                = 1.0f / fg;

                                b[0]                = t[2]; // 1.0 / fg
                                b[1]                = t[1]; // rquality
                                b[2]                = t[0]; // fg

                                if ((cj>>1) == 0)
                                {
                                    c->t[0]            *= gain;
                                    c->t[1]            *= gain;
                                    c->t[2]            *= gain;
                                }

                                // Move to next cascade
                                c                  += nc;
                            }
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_RLC_BANDPASS:
                {
                    nc                      = quantify(cj, fp->nSlope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    float f2                = 1.0f / fp->fFreq2;
                    float k                 = (1.0f + f2)/(1.0f + fp->fQuality);

                    // Intialize pointers
                    for (size_t j=0; j < nc; j++)
                    {
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        for (size_t i=0; i<samples; ++i)
                        {
                            c->t[0]             = 0.0f;
                            c->t[1]             = (cj == 0) ? expf(fp->nSlope * logf(k)) * sfg[i] : 1.0f;
                            c->t[2]             = 0.0f;

                            c->b[0]             = f2;
                            c->b[1]             = k;
                            c->b[2]             = 1.0f;

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_RLC_BELL:
                {
                    nc                      = quantify(cj, fp->nSlope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    for (size_t j=0; j < nc; j++)
                    {
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            float fg            = expf(logf(sfg[i])/fp->nSlope);
                            float angle         = atanf(fg);
                            float tsin          = sinf(angle);
                            float tcos          = sqrtf(1.0f - tsin*tsin);
                            float k             = 2.0f * (1.0f/fg + fg) / (1.0f + (2.0f * fp->fQuality) / fp->nSlope);

                            // Create transfer function
                            c->t[0]             = 1.0f;
                            c->t[1]             = k * tsin;
                            c->t[2]             = 1.0f;

                            c->b[0]             = 1.0f;
                            c->b[1]             = k * tcos;
                            c->b[2]             = 1.0f;

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                // Resonance filter
                case FLT_BT_RLC_RESONANCE:
                {
                    nc                      = quantify(cj, fp->nSlope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    for (size_t j=0; j < nc; j++)
                    {
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Pre-calculate filter parameters
                        float k             = 2.0f / (1.0f + fp->fQuality);

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            float angle         = atanf(expf(logf(sfg[i]) / fp->nSlope));
                            float tsin          = sinf(angle);
                            float tcos          = sqrtf(1.0f - tsin*tsin);

                            // Create transfer function
                            c->t[0]             = 1.0f;
                            c->t[1]             = k * tsin;
                            c->t[2]             = 1.0f;

                            c->b[0]             = 1.0f;
                            c->b[1]             = k * tcos;
                            c->b[2]             = 1.0f;

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_RLC_NOTCH:
                {
                    // Need ONLY ONE cascade for notch
                    if (cj > 0)
                        return 0;

                    nc                  = 1;
                    dsp::f_cascade_t *c = dst;       // target cascade pointer
                    float k             = 2.0f / (1.0f + fp->fQuality);

                    // Generate cascades
                    for (size_t i=0; i<samples; ++i)
                    {
                        // Create transfer function
                        c->t[0]             = sfg[i];
                        c->t[1]             = 0.0f;
                        c->t[2]             = sfg[i];

                        c->b[0]             = 1.0f;
                        c->b[1]             = k;
                        c->b[2]             = 1.0f;

                        // Move to next cascade
                        c++;
                    }

                    break;
                }

                case FLT_BT_RLC_ENVELOPE:
                {
                    size_t slope            = fp->nSlope;
                    size_t max_nc           = (slope & 1) * 3 + (slope >> 1);
                    nc                      = quantify(cj, max_nc); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    // TODO: test this
                    size_t j=0;
                    while ((slope & 1) && (cj < 3) && (j < nc))
                    {
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer
                        float k             = 1.0f / (1 << (cj * 4)); // 1, 1/16, 1/256, 1/4096

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            c->t[0]         = 1.0f;
                            c->t[1]         = (1.0f + 0.25f)*k;
                            c->t[2]         = 0.25f * k*k;

                            c->b[0]         = 1.0f;
                            c->b[1]         = (0.5f + 0.125f)*k;
                            c->b[2]         = 0.5f * 0.125f * k*k;

                            if (!cj)
                            {
                                c->t[0]        *= fp->fGain;
                                c->t[1]        *= fp->fGain;
                                c->t[2]        *= fp->fGain;
                            }

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Increment cascade number
                        cj++;
                        j++;
                    }

                    while (j < nc)
                    {
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            // Create transfer function
                            c->t[0]             = (j == 0) ? fp->fGain : 1.0f;
                            c->t[1]             = (j == 0) ? fp->fGain : 1.0f;

                            c->b[0]             = 1.0f;
                            c->b[1]             = 0.00005f;

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Increment cascade number
                        cj++;
                        j++;
                    }
                    break;
                }

                //----------------------------------- BWC FILTERS ------------------------------------
                case FLT_BT_BWC_LOPASS:
                case FLT_BT_BWC_HIPASS:
                {
                    nc                      = quantify(cj, (fp->nSlope >> 1) + (fp->nSlope & 1)); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;
                    size_t j                = 0;

                    // Generate first cascade
                    if ((cj == 0) && (fp->nSlope & 1))
                    {
                        dsp::f_cascade_t *c = dst;       // target cascade pointer

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            c->b[0]     = 1.0f;
                            c->b[1]     = 1.0f;
                            c->b[2]     = 0.0f;

                            c->t[0]     = (ftype == FLT_BT_BWC_LOPASS) ? sfg[i] : 0.0f;
                            c->t[1]     = (ftype == FLT_BT_BWC_LOPASS) ? 0.0f : sfg[i];
                            c->t[2]     = 0.0f;

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Increment cascade number
                        cj++;
                        j++;
                    }

                    float k     = 1.0f / (1.0f + fp->fQuality);

                    // Add additional cascades
                    for (; j<nc; j++)
                    {
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Pre-calculate filter parameters
                        float theta         = ((2*(cj - (fp->nSlope & 1)) + 1)*C_PI_DIV_2)/fp->nSlope;
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0f - tsin*tsin);
                        float kf1           = 1.0f / (tsin*tsin + k*k * tcos*tcos);
                        const float alpha   = 2.0f * k * tcos * kf1;

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            // Tranfer function
                            if (ftype == FLT_BT_BWC_HIPASS)
                            {
                                c->t[0]         = 0.0f;
                                c->t[1]         = 0.0f;
                                c->t[2]         = (cj == 0) ? sfg[i] : 1.0f;

                                c->b[0]         = kf1;
                                c->b[1]         = alpha;
                                c->b[2]         = 1.0f;
                            }
                            else
                            {
                                c->t[0]         = (cj == 0) ? sfg[i] : 1.0f;
                                c->t[1]         = 0.0f;
                                c->t[2]         = 0.0f;

                                c->b[0]         = 1.0f;
                                c->b[1]         = alpha;
                                c->b[2]         = kf1;
                            }

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_BWC_HISHELF:
                case FLT_BT_BWC_LOSHELF:
                {
                    nc                      = quantify(cj, fp->nSlope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    // Create cascades
                    for (size_t j=0; j<nc; ++j)
                    {
                        // Intialize pointers
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Pre-calculate filter parameters
                        float theta         = ((2*cj + 1)*C_PI_DIV_2)/(2*fp->nSlope);
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0f - tsin*tsin);

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            float gain          = sqrtf(sfg[i]);
                            float fg            = expf(logf(gain)/(2.0f * fp->nSlope));
                            float k             = 1.0f / (1.0f + fp->fQuality * (1.0f - expf(2.0f - gain - 1.0f/gain)));
                            float kf            = tsin*tsin + k*k * tcos*tcos;

                            float *t            = (ftype == FLT_BT_BWC_HISHELF) ? c->t : c->b;
                            float *b            = (ftype == FLT_BT_BWC_HISHELF) ? c->b : c->t;

                            // Transfer function
                            t[0]                = kf / fg;
                            t[1]                = 2.0f * k * tcos;
                            t[2]                = fg;

                            b[0]                = t[2]; // fg
                            b[1]                = t[1]; // 2.0 * k * tcos;
                            b[2]                = t[0]; // kf / fg

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Patch volume
                        if (cj == 0)
                        {
                            dsp::f_cascade_t *c = dst;       // target cascade pointer
                            for (size_t i=0; i<samples; ++i)
                            {
                                float gain          = sqrtf(sfg[i]);
                                c->t[0]            *= gain;
                                c->t[1]            *= gain;
                                c->t[2]            *= gain;
                                c                  += nc;
                            }
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_BWC_LADDERPASS:
                case FLT_BT_BWC_LADDERREJ:
                {
                    // NOTE: this code was not tested because originally LRX filters are used
                    size_t slope            = fp->nSlope * 2;
                    nc                      = quantify(cj, slope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    // Create cascades
                    for (size_t j=0; j<nc; ++j)
                    {
                        // Intialize pointers
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Pre-calculate filter parameters
                        float rc_slope      = 1.0f / slope;
                        float theta         = (((cj&(~1)) + 1)*C_PI_DIV_2) * rc_slope;
                        float tcos          = cosf(theta);
                        float tcos2         = tcos * tcos;      // cos^2
                        float tsin2         = 1.0f - tcos2;     // sin^2

                        float gain, k, kf, fg;
                        float *t, *b;

                        if (cj & 1) // Second shelf cascade, always hi-shelf
                        {
                            float xf            = fp->fFreq2;
                            float xf2           = xf * xf;
                            float xtcos         = 2.0f * tcos * xf;

                            // Generate cascades
                            for (size_t i=0; i<samples; ++i)
                            {
                                gain                = (ftype == FLT_BT_BWC_LADDERPASS) ? sqrtf(sfg[i]) : sqrtf(1.0f/sfg[i]);
                                fg                  = expf(logf(gain) * rc_slope);
                                k                   = 1.0f / (1.0f + fp->fQuality * (1.0f - expf(2.0f - gain - 1.0f/gain)));
                                kf                  = tsin2 + k*k * tcos2;

                                t                   = c->b;
                                b                   = c->t;

                                // Transfer function
                                t[0]                = kf / fg;
                                t[1]                = k * xtcos;
                                t[2]                = fg * xf2;

                                b[0]                = fg;
                                b[1]                = t[1];
                                b[2]                = t[0] * xf2;

                                if (!(cj&(~1)))
                                {
                                    float gain2         = 1.0f / gain;
                                    c->t[0]            *= gain2;
                                    c->t[1]            *= gain2;
                                    c->t[2]            *= gain2;
                                }

                                // Move to next cascade
                                c                  += nc;
                            }
                        }
                        else // First shelf cascade, lo-shelf for LADDERREJ, hi-shelf for LADDERPASS
                        {
                            float xtcos         = 2.0f * tcos;

                            // Generate cascades
                            for (size_t i=0; i<samples; ++i)
                            {
                                gain            = sqrtf(sfg[i]);
                                k               = 1.0f / (1.0f + fp->fQuality * (1.0f - expf(2.0f - gain - 1.0f/gain)));
                                fg              = expf(logf(gain) * rc_slope);
                                kf              = tsin2 + k*k * tcos2;

                                if (ftype == FLT_BT_BWC_LADDERPASS)
                                {
                                    t               = c->t;
                                    b               = c->b;
                                }
                                else
                                {
                                    t               = c->b;
                                    b               = c->t;
                                }

                                // Transfer function
                                t[0]                = kf / fg;
                                t[1]                = k * xtcos;
                                t[2]                = fg;

                                b[0]                = t[2]; // fg
                                b[1]                = t[1]; // 2.0 * k * tcos
                                b[2]                = t[0]; // kf / fg;

                                if (!(cj&(~1)))
                                {
                                    c->t[0]            *= gain;
                                    c->t[1]            *= gain;
                                    c->t[2]            *= gain;
                                }

                                // Move to next cascade
                                c                  += nc;
                            }
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_BWC_BELL:
                {
                    size_t slope            = fp->nSlope * 2;
                    nc                      = quantify(cj, slope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    // Create cascades
                    for (size_t j=0; j<nc; ++j)
                    {
                        // Intialize pointers
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Pre-calculate filter parameters
                        float theta         = (((cj&(~1)) + 1)*C_PI_DIV_2)/(slope);
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0f - tsin*tsin);
                        float k             = 1.0f / (1.0f + fp->fQuality);
                        float kf            = tsin*tsin + k*k * tcos*tcos;

                        if (cj & 1) // Second cascade
                        {
                            // Generate cascades
                            for (size_t i=0; i<samples; ++i)
                            {
                                float fg            = expf(logf(sfg[i])/slope);

                                // Transfer function
                                if (sfg[i] >= 1.0f)
                                {
                                    c->t[0]             = 1.0f;
                                    c->t[1]             = 2.0f * k * tcos / fg;
                                    c->t[2]             = kf / (fg * fg);

                                    c->b[0]             = 1.0f;
                                    c->b[1]             = 2.0f * k * tcos;
                                    c->b[2]             = kf;
                                }
                                else
                                {
                                    c->t[0]             = 1.0f;
                                    c->t[1]             = 2.0f * k * tcos;
                                    c->t[2]             = kf;

                                    c->b[0]             = 1.0f;
                                    c->b[1]             = 2.0f * k * tcos * fg;
                                    c->b[2]             = kf * fg * fg;
                                }

                                // Move to next cascade
                                c                  += nc;
                            }
                        }
                        else // First cascade
                        {
                            // Generate cascades
                            for (size_t i=0; i<samples; ++i)
                            {
                                float fg            = expf(logf(sfg[i])/slope);

                                // Transfer function
                                if (sfg[i] >= 1.0f)
                                {
                                    c->t[0]             = 1.0f;
                                    c->t[1]             = 2.0f * k * tcos * fg / kf;
                                    c->t[2]             = 1.0f * fg * fg / kf;

                                    c->b[0]             = 1.0f;
                                    c->b[1]             = 2.0f * k * tcos / kf;
                                    c->b[2]             = 1.0f / kf;
                                }
                                else
                                {
                                    c->t[0]             = 1.0f;
                                    c->t[1]             = 2.0f * k * tcos / kf;
                                    c->t[2]             = 1.0f / kf;

                                    c->b[0]             = 1.0f;
                                    c->b[1]             = 2.0f * k * tcos / (fg * kf);
                                    c->b[2]             = 1.0f / (fg * fg * kf);

                                }

                                // Move to next cascade
                                c                  += nc;
                            }
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_BWC_BANDPASS:
                {
                    size_t slope            = fp->nSlope * 2;
                    nc                      = quantify(cj, slope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    float f2                = fp->fFreq2;
                    float k                 = 1.0f / (1.0f + fp->fQuality);

                    // Create cascades
                    for (size_t j=0; j<nc; ++j)
                    {
                        // Intialize pointers
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Pre-calculate filter parameters
                        float theta         = (((cj&(~1)) + 1)*C_PI_DIV_2)/slope;
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0f - tsin*tsin);
                        float kf1           = 1.0f/(tsin*tsin + k*k * tcos*tcos);

                        if (cj & 1) // Hi-pass cascade
                        {
                            for (size_t i=0; i<samples; ++i)
                            {
                                // Transfer function
                                c->t[0]             = 1.0f;
                                c->t[1]             = 0.0f;
                                c->t[2]             = 0.0f;

                                c->b[0]             = 1.0f;
                                c->b[1]             = 2.0f * k * tcos * f2 * kf1;
                                c->b[2]             = f2 * f2 * kf1;

                                // Move to next cascade
                                c                  += nc;
                            }
                        }
                        else // Lo-pass cascade
                        {
                            for (size_t i=0; i<samples; ++i)
                            {
                                // Transfer function
                                c->t[0]             = 0.0f;
                                c->t[1]             = 0.0f;
                                c->t[2]             = (cj == 0) ? sfg[i] : 1.0f;

                                c->b[0]             = kf1;
                                c->b[1]             = 2.0f * k * tcos * kf1;
                                c->b[2]             = 1.0f;

                                // Move to next cascade
                                c                  += nc;
                            }
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                //----------------------------------- LRX FILTERS ------------------------------------
                case FLT_BT_LRX_LOPASS:
                case FLT_BT_LRX_HIPASS:
                {
                    nc                      = quantify(cj, fp->nSlope*2); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    float k     = 1.0f / (1.0f + fp->fQuality);

                    // Add additional cascades
                    for (size_t j=0; j<nc; j++)
                    {
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Pre-calculate filter parameters
                        float theta         = (((cj & (~1)) + 1)*C_PI_DIV_2)/(fp->nSlope*2); // The number of cascades is just doubled
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0f - tsin*tsin);
                        float kf1           = 1.0f / (tsin*tsin + k*k * tcos*tcos);
                        const float alpha   = 2.0f * k * tcos * kf1;

                        // Generate cascades
                        for (size_t i=0; i<samples; ++i)
                        {
                            // Tranfer function
                            if (ftype == FLT_BT_LRX_HIPASS)
                            {
                                c->t[0]         = 0.0f;
                                c->t[1]         = 0.0f;
                                c->t[2]         = (cj == 0) ? sfg[i] : 1.0f;

                                c->b[0]         = kf1;
                                c->b[1]         = alpha;
                                c->b[2]         = 1.0f;
                            }
                            else
                            {
                                c->t[0]         = (cj == 0) ? sfg[i] : 1.0f;
                                c->t[1]         = 0.0f;
                                c->t[2]         = 0.0f;

                                c->b[0]         = 1.0f;
                                c->b[1]         = alpha;
                                c->b[2]         = kf1;
                            }

                            // Move to next cascade
                            c                  += nc;
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_LRX_HISHELF:
                case FLT_BT_LRX_LOSHELF:
                    return build_lrx_shelf_filter_bank(dst, fp, cj, sfg, samples, ftype);

                case FLT_BT_LRX_LADDERPASS:
                case FLT_BT_LRX_LADDERREJ:
                    return build_lrx_ladder_filter_bank(dst, fp, cj, sfg, samples, ftype);

                case FLT_BT_LRX_BELL:
                {
                    size_t slope            = fp->nSlope * 4;
                    nc                      = quantify(cj, slope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    // Create cascades
                    for (size_t j=0; j<nc; ++j)
                    {
                        // Intialize pointers
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Pre-calculate filter parameters
                        float theta         = (((cj&(~3)) + 2)*C_PI_DIV_2)/(slope);
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0f - tsin*tsin);
                        float k             = 1.0f / (1.0f + fp->fQuality);
                        float kf            = tsin*tsin + k*k * tcos*tcos;

                        if (cj & 1) // Second cascade
                        {
                            // Generate cascades
                            for (size_t i=0; i<samples; ++i)
                            {
                                float fg            = expf(logf(sfg[i])/slope);

                                // Transfer function
                                if (sfg[i] >= 1.0)
                                {
                                    c->t[0]             = 1.0f;
                                    c->t[1]             = 2.0f * k * tcos / fg;
                                    c->t[2]             = kf / (fg * fg);

                                    c->b[0]             = 1.0f;
                                    c->b[1]             = 2.0f * k * tcos;
                                    c->b[2]             = kf;
                                }
                                else
                                {
                                    c->t[0]             = 1.0f;
                                    c->t[1]             = 2.0f * k * tcos;
                                    c->t[2]             = kf;

                                    c->b[0]             = 1.0f;
                                    c->b[1]             = 2.0f * k * tcos * fg;
                                    c->b[2]             = kf * fg * fg;
                                }

                                // Move to next cascade
                                c                  += nc;
                            }
                        }
                        else // First cascade
                        {
                            // Generate cascades
                            for (size_t i=0; i<samples; ++i)
                            {
                                float fg            = expf(logf(sfg[i])/slope);

                                // Transfer function
                                if (sfg[i] >= 1.0)
                                {
                                    c->t[0]             = 1.0f;
                                    c->t[1]             = 2.0f * k * tcos * fg / kf;
                                    c->t[2]             = 1.0f * fg * fg / kf;

                                    c->b[0]             = 1.0f;
                                    c->b[1]             = 2.0f * k * tcos / kf;
                                    c->b[2]             = 1.0f / kf;
                                }
                                else
                                {
                                    c->t[0]             = 1.0f;
                                    c->t[1]             = 2.0f * k * tcos / kf;
                                    c->t[2]             = 1.0f / kf;

                                    c->b[0]             = 1.0f;
                                    c->b[1]             = 2.0f * k * tcos / (fg * kf);
                                    c->b[2]             = 1.0f / (fg * fg * kf);

                                }

                                // Move to next cascade
                                c                  += nc;
                            }
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                case FLT_BT_LRX_BANDPASS:
                {
                    size_t slope            = fp->nSlope * 4;
                    nc                      = quantify(cj, slope); // Number of cascades to generate
                    if (nc <= 0)
                        return nc;

                    float f2                = fp->fFreq2;
                    float k                 = 1.0f / (1.0f + fp->fQuality);

                    // Create cascades
                    for (size_t j=0; j<nc; ++j)
                    {
                        // Intialize pointers
                        dsp::f_cascade_t *c = &dst[(nc+1)*j];       // target cascade pointer

                        // Pre-calculate filter parameters
                        float theta         = (((cj&(~3)) + 2)*C_PI_DIV_2)/slope;
                        float tsin          = sinf(theta);
                        float tcos          = sqrtf(1.0f - tsin*tsin);
                        float kf1           = 1.0f/(tsin*tsin + k*k * tcos*tcos);

                        if (cj & 1) // Hi-pass cascade
                        {
                            for (size_t i=0; i<samples; ++i)
                            {
                                // Transfer function
                                c->t[0]             = 1.0f;
                                c->t[1]             = 0.0f;
                                c->t[2]             = 0.0f;

                                c->b[0]             = 1.0f;
                                c->b[1]             = 2.0f * k * tcos * f2 * kf1;
                                c->b[2]             = f2 * f2 * kf1;

                                // Move to next cascade
                                c                  += nc;
                            }
                        }
                        else // Lo-pass cascade
                        {
                            for (size_t i=0; i<samples; ++i)
                            {
                                // Transfer function
                                c->t[0]             = 0.0f;
                                c->t[1]             = 0.0f;
                                c->t[2]             = (cj == 0) ? sfg[i] : 1.0f;

                                c->b[0]             = kf1;
                                c->b[1]             = 2.0f * k * tcos * kf1;
                                c->b[2]             = 1.0f;

                                // Move to next cascade
                                c                  += nc;
                            }
                        }

                        // Increment cascade number
                        cj++;
                    }

                    break;
                }

                default:
                    nc = 0;
                    break;
            }

            return nc;
        }

        void DynamicFilters::vcomplex_transfer_calc(float *re, float *im, const dsp::f_cascade_t *c, const float *freq, size_t cj, size_t nc, size_t nf)
        {
            size_t i=0;
            if (!cj)
            {
                dsp::filter_transfer_calc_ri(re, im, c, freq, nf);
                c              += (nc + 1);
                ++i;
            }

            for (; i<nc; ++i)
            {
                dsp::filter_transfer_apply_ri(re, im, c, freq, nf);
                c              += (nc + 1);
            }
        }

        void DynamicFilters::vcomplex_transfer_calc(float *dst, const dsp::f_cascade_t *c, const float *freq, size_t cj, size_t nc, size_t nf)
        {
            size_t i=0;
            if (!cj)
            {
                dsp::filter_transfer_calc_pc(dst, c, freq, nf);
                c              += (nc + 1);
                ++i;
            }

            for (; i<nc; ++i)
            {
                dsp::filter_transfer_apply_pc(dst, c, freq, nf);
                c              += (nc + 1);
            }
        }

        bool DynamicFilters::freq_chart(size_t id, float *re, float *im, const float *f, float gain, size_t count)
        {
            if (id >= nFilters)
                return false;

            filter_params_t *fp = &vFilters[id].sParams;

            // Initialize values
            switch (fp->nType)
            {
                case FLT_NONE:
                    dsp::fill_one(re, count);
                    dsp::fill_zero(im, count);
                    return true;

                case FLT_BT_AMPLIFIER:
                case FLT_MT_AMPLIFIER:
                    dsp::fill(re, gain, count);
                    dsp::fill_zero(im, count);
                    return true;

                default:
                    break;
            }

            float *tf   = vCascades[BLD_BUF_SIZE * BLD_BUF_SIZE * 2].t;  // Use cascades as a temporary frequency buffer

            // Process the filter
            if (fp->nType & 1) // Bilinear
            {
                const float nf  = C_PI / float(nSampleRate);
                const float kf  = 1.0f/tanf(fp->fFreq * nf);
                const float lf  = nSampleRate * 0.499f;

                // Process frequency chart
                while (count > 0)
                {
                    const size_t fcount = lsp_min(count, FBUF_SIZE);
                    size_t cj       = 0;

                    // Generate set of frequencies
                    for (size_t i=0; i<fcount; ++i)
                        tf[i]       = tanf(lsp_min(f[i], lf) * nf) * kf;

                    // Estimate transfer function
                    while (true)
                    {
                        // Generate cascades
                        const size_t nj         = build_filter_bank(vCascades, fp, cj, &gain, 1);
                        if (nj <= 0)
                            break;

                        vcomplex_transfer_calc(re, im, vCascades, tf, cj, nj, fcount);
                        cj                     += nj;
                    }

                    // Update counter and pointers
                    count          -= fcount;
                    f              += fcount;
                    re             += fcount;
                    im             += fcount;
                }
            }
            else
            {
                const float kf  = 1.0f / fp->fFreq;

                while (count > 0)
                {
                    const size_t fcount = lsp_min(count, FBUF_SIZE);
                    size_t cj       = 0;

                    // Generate set of frequencies
                    dsp::mul_k3(tf, f, kf, fcount);

                    // Estimate transfer function
                    while (true)
                    {
                        // Generate cascades
                        const size_t nj         = build_filter_bank(vCascades, fp, cj, &gain, 1);
                        if (nj <= 0)
                            break;

                        vcomplex_transfer_calc(re, im, vCascades, tf, cj, nj, fcount);
                        cj                     += nj;
                    }

                    // Update counter and pointers
                    count          -= fcount;
                    f              += fcount;
                    re             += fcount;
                    im             += fcount;
                }
            }

            return true;
        }

        bool DynamicFilters::freq_chart(size_t id, float *dst, const float *f, float gain, size_t count)
        {
            if (id >= nFilters)
                return false;

            filter_params_t *fp = &vFilters[id].sParams;

            // Initialize values
            switch (fp->nType)
            {
                case FLT_NONE:
                    dsp::pcomplex_fill_ri(dst, 1.0f, 0.0f, count);
                    return true;

                case FLT_BT_AMPLIFIER:
                case FLT_MT_AMPLIFIER:
                    dsp::pcomplex_fill_ri(dst, gain, 0.0f, count);
                    return true;

                default:
                    break;
            }

            float *tf   = vCascades[BLD_BUF_SIZE * BLD_BUF_SIZE * 2].t;  // Use cascades as a temporary frequency buffer

            // Process the filter
            if (fp->nType & 1) // Bilinear
            {
                const float nf  = C_PI / float(nSampleRate);
                const float kf  = 1.0/tanf(fp->fFreq * nf);
                const float lf  = nSampleRate * 0.499f;

                // Process frequency chart
                while (count > 0)
                {
                    const size_t fcount = lsp_min(count, FBUF_SIZE);
                    size_t cj       = 0;

                    // Generate set of frequencies
                    for (size_t i=0; i<fcount; ++i)
                        tf[i]       = tanf(lsp_min(f[i], lf) * nf) * kf;

                    // Estimate transfer function
                    while (true)
                    {
                        // Generate cascades
                        const size_t nj         = build_filter_bank(vCascades, fp, cj, &gain, 1);
                        if (nj <= 0)
                            break;

                        vcomplex_transfer_calc(dst, vCascades, tf, cj, nj, fcount);
                        cj                     += nj;
                    }

                    // Update counter and pointers
                    count          -= fcount;
                    f              += fcount;
                    dst            += (fcount << 1);
                }
            }
            else
            {
                const float kf      = 1.0f / fp->fFreq;

                // Process frequency chart
                while (count > 0)
                {
                    const size_t fcount = lsp_min(count, FBUF_SIZE);
                    size_t cj       = 0;

                    // Generate set of frequencies
                    dsp::mul_k3(tf, f, kf, fcount);

                    // Estimate transfer function
                    while (true)
                    {
                        // Generate cascades
                        const size_t nj         = build_filter_bank(vCascades, fp, cj, &gain, 1);
                        if (nj <= 0)
                            break;

                        vcomplex_transfer_calc(dst, vCascades, tf, cj, nj, fcount);
                        cj                     += nj;
                    }

                    // Update counter and pointers
                    count          -= fcount;
                    f              += fcount;
                    dst            += (fcount << 1);
                }
            }

            return true;
        }

        void DynamicFilters::reset()
        {
            bClearMem   = true;
        }

        void DynamicFilters::dump(dspu::IStateDumper *v) const
        {
            v->begin_array("vFilters", vFilters, nFilters);
            for (size_t i=0; i<nFilters; ++i)
            {
                const filter_t *f = &vFilters[i];
                v->begin_object(f, sizeof(filter_t));
                {
                    v->write("nType", f->sParams.nType);
                    v->write("fFreq", f->sParams.fFreq);
                    v->write("fFreq2", f->sParams.fFreq2);
                    v->write("fGain", f->sParams.fGain);
                    v->write("nSlope", f->sParams.nSlope);
                    v->write("fQuality", f->sParams.fQuality);
                    v->write("bActive", f->bActive);
                }
                v->end_object();
            }
            v->end_array();
            v->write("vCascades", vCascades);
            v->write("vBiquads", vBiquads.ptr);
            v->write("nFilters", nFilters);
            v->write("nSampleRate", nSampleRate);
            v->write("pData", pData);
            v->write("bClearMem", bClearMem);
        }
    } /* namespace dspu */
} /* namespace lsp */
