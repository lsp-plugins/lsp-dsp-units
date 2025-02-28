/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 16 нояб. 2024 г.
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

#include <lsp-plug.in/dsp-units/meters/ILUFSMeter.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/bits.h>
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr size_t BUFFER_SIZE         = 0x400;

        /**
         * For Ga = -70 LKFS, D = -0.691 LKFS:
         * because Ga = D + 10*log10(LT)
         * Then: LT = 10 ^ ((Ga - D) / 10)
         */
        static constexpr float GATING_ABS_THRESH    = 1.17246530458e-07;

        /**
         * We computed weigthed sum S = sum(Gi * sum(zij) / |Jg|);
         * We know that for D = -0.691 LKFS Gated Loudness Lkg = D + 10*log10(S)
         * We know that for R = -10 LKFS relative threshold Gr = D + 10*log10(S) + R
         *
         * Then: Gr = Lkg + R.
         * Then: 10*log10(Kr * S) = 10*log10(S) + R
         * 10*log10(Kr * S) - 10*log10(S) = R
         * 10*log10(Kr) = R
         * Kr = 10^(R/10) = 10^(-10/10) = 0.1
         *
         */
        static constexpr float GATING_REL_THRESH    = 0.1f;

        ILUFSMeter::ILUFSMeter()
        {
            construct();
        }

        ILUFSMeter::~ILUFSMeter()
        {
            destroy();
        }

        void ILUFSMeter::construct()
        {
            vChannels           = NULL;
            vBuffer             = NULL;
            vLoudness           = NULL;

            fBlockPeriod        = 0.0f;
            fIntTime            = 0.0f;
            fMaxIntTime         = 0.0f;
            fAvgCoeff           = 0.0f;
            fLoudness           = 0.0f;

            nBlockSize          = 0;
            nBlockOffset        = 0;
            nBlockPart          = 0;
            nMSSize             = 0;
            nMSHead             = 0;
            nMSInt              = 0;
            nMSCount            = -3;

            nSampleRate         = 0;
            nChannels           = 0;
            nFlags              = 0;
            enWeight            = bs::WEIGHT_K;

            pData               = NULL;
            pVarData            = NULL;
        }

        void ILUFSMeter::destroy()
        {
            if (pData != NULL)
            {
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c        = &vChannels[i];
                    c->sFilter.destroy();
                    c->sBank.destroy();
                }

                free_aligned(pData);
                pData               = NULL;
                vChannels           = NULL;
                vBuffer             = NULL;
            }

            if (pVarData != NULL)
            {
                free_aligned(pVarData);
                vLoudness           = NULL;
                pVarData            = NULL;
            }
        }

        status_t ILUFSMeter::init(size_t channels, float max_int_time, float block_period)
        {
            destroy();

            // Allocate data
            const size_t szof_channels  = align_size(channels * sizeof(channel_t), DEFAULT_ALIGN);
            const size_t szof_buffer    = align_size(sizeof(float) * BUFFER_SIZE, DEFAULT_ALIGN);
            const size_t to_alloc       =
                szof_channels +
                szof_buffer;

            uint8_t *ptr            = alloc_aligned<uint8_t>(pData, to_alloc, DEFAULT_ALIGN);
            if (ptr == NULL)
                return STATUS_NO_MEM;

            // Allocate buffers
            vChannels               = advance_ptr_bytes<channel_t>(ptr, szof_channels);
            vBuffer                 = advance_ptr_bytes<float>(ptr, szof_buffer);

            // Cleanup and init state for each channel
            dsp::fill_zero(vBuffer, BUFFER_SIZE);
            for (size_t i=0; i<channels; ++i)
            {
                channel_t *c            = &vChannels[i];
                c->sBank.construct();
                c->sFilter.construct();

                if (!c->sBank.init(4))
                    return STATUS_NO_MEM;
                if (!c->sFilter.init(&c->sBank))
                    return STATUS_NO_MEM;


                c->vIn                  = NULL;
                for (size_t i=0; i<4; ++i)
                    c->vBlock[i]            = 0.0f;

                c->fWeight              = 0.0f;
                c->enDesignation        = bs::CHANNEL_NONE;

                c->nFlags               = C_ENABLED;
            }

            // Set-up default designations
            if (channels == 1)
            {
                channel_t *c            = &vChannels[0];
                c->enDesignation        = bs::CHANNEL_CENTER;
                c->fWeight              = bs::channel_weighting(c->enDesignation);
            }
            else if (channels == 2)
            {
                channel_t *l            = &vChannels[0];
                channel_t *r            = &vChannels[1];

                l->enDesignation        = bs::CHANNEL_LEFT;
                l->fWeight              = bs::channel_weighting(l->enDesignation);
                r->enDesignation        = bs::CHANNEL_RIGHT;
                r->fWeight              = bs::channel_weighting(r->enDesignation);
            }

            // Initialize channels
            for (size_t i=0; i<channels; ++i)
            {
                channel_t *c            = &vChannels[i];

                if (!c->sFilter.init(&c->sBank))
                    return STATUS_NO_MEM;
            }

            // Initialize settings
            fBlockPeriod            = block_period;
            fIntTime                = max_int_time;
            fMaxIntTime             = max_int_time;
            fAvgCoeff               = 1.0f;
            fLoudness               = 0.0f;

            nBlockSize              = 0;
            nBlockOffset            = 0;
            nBlockPart              = 0;

            nMSSize                 = 0;
            nMSHead                 = 0;
            nMSInt                  = 0;
            nMSCount                = -3;

            nSampleRate             = 0;
            nChannels               = uint32_t(channels);
            nFlags                  = F_UPD_ALL;
            enWeight                = bs::WEIGHT_K;

            return STATUS_OK;
        }

        status_t ILUFSMeter::bind(size_t id, const float *in)
        {
            if (id >= nChannels)
                return STATUS_OVERFLOW;

            channel_t *c    = &vChannels[id];
            c->vIn          = in;

            return STATUS_OK;
        }

        status_t ILUFSMeter::set_designation(size_t id, bs::channel_t designation)
        {
            if (id >= nChannels)
                return STATUS_OVERFLOW;

            channel_t *c        = &vChannels[id];
            c->enDesignation    = designation;
            c->fWeight          = bs::channel_weighting(designation);

            return STATUS_OK;
        }

        bs::channel_t ILUFSMeter::designation(size_t id) const
        {
            return (id < nChannels) ? vChannels[id].enDesignation : bs::CHANNEL_NONE;
        }

        status_t ILUFSMeter::set_active(size_t id, bool active)
        {
            if (id >= nChannels)
                return STATUS_OVERFLOW;

            channel_t *c        = &vChannels[id];
            if (bool(c->nFlags & C_ENABLED) == active)
                return STATUS_OK;

            c->nFlags           = lsp_setflag(c->nFlags, C_ENABLED, active);

            return STATUS_OK;
        }

        bool ILUFSMeter::active(size_t id) const
        {
            return (id < nChannels) ? bool(vChannels[id].nFlags & C_ENABLED) : false;
        }

        void ILUFSMeter::set_weighting(bs::weighting_t weighting)
        {
            if (weighting == enWeight)
                return;

            enWeight        = weighting;
            nFlags         |= F_UPD_FILTERS;
        }

        void ILUFSMeter::set_integration_period(float period)
        {
            period          = lsp_limit(period, fBlockPeriod * 0.001f, fMaxIntTime);
            if (fIntTime == period)
                return;

            fIntTime        = period;
            nFlags         |= F_UPD_TIME;
        }

        status_t ILUFSMeter::set_sample_rate(size_t sample_rate)
        {
            if (nSampleRate == sample_rate)
                return STATUS_OK;

            // Reallocate ring buffers for RMS estimation and lookahead
            const size_t blk_count  = dspu::millis_to_samples(sample_rate, fBlockPeriod * 0.25f); // 75% overlapping
            size_t int_count        = (dspu::seconds_to_samples(sample_rate, fMaxIntTime) + blk_count - 1) / blk_count;

            const size_t int_szof   = align_size(int_count * sizeof(float), DEFAULT_ALIGN);
            int_count               = int_szof / sizeof(float);

            size_t to_alloc         = int_szof;

            uint8_t *buf            = realloc_aligned<uint8_t>(pVarData, to_alloc, DEFAULT_ALIGN);
            if (buf == NULL)
                return STATUS_NO_MEM;

            // Store new pointers to buffers
            vLoudness               = advance_ptr_bytes<float>(buf, int_count);

            // Update parameters
            fAvgCoeff               = 0.25f / float(blk_count);
            nSampleRate             = uint32_t(sample_rate);
            nBlockSize              = uint32_t(blk_count);
            nMSSize                 = uint32_t(int_count);
            nFlags                  = F_UPD_ALL;

            // Clear all buffers
            clear();

            return STATUS_OK;
        }

        float ILUFSMeter::compute_gated_loudness(float threshold)
        {
            float loudness      = 0.0f;
            if (nMSCount <= 0)
                return loudness;

            size_t tail         = (nMSHead + nMSSize - nMSCount) % nMSSize;
            size_t count        = 0;

            for (ssize_t j=0; j<nMSCount; ++j)
            {
                const float lj      = vLoudness[tail];
                tail                = (tail + 1) % nMSSize;
                if (lj <= GATING_ABS_THRESH)
                    continue;

                ++count;
                loudness           += lj;
            }

            return (count > 0) ? loudness / float(count) : 0.0f;
        }

        void ILUFSMeter::process(float *out, size_t count, float gain)
        {
            update_settings();

            for (size_t offset = 0; offset < count; )
            {
                // Estimate how many samples we are ready to process
                const size_t to_do      = lsp_min(count - offset, nBlockSize - nBlockOffset, BUFFER_SIZE);
                if (to_do > 0)
                {
                    // Compute square sum
                    for (size_t i=0; i<nChannels; ++i)
                    {
                        channel_t              *c = &vChannels[i];
                        if ((c->vIn != NULL) && (c->nFlags & C_ENABLED))
                        {
                            // Apply the weighting filter
                            c->sFilter.process(vBuffer, &c->vIn[offset], to_do);
                            // Compute the sum of squares
                            c->vBlock[nBlockPart]      += dsp::h_sqr_sum(vBuffer, to_do);
                        }
                    }

                    nBlockOffset           += to_do;
                }

                // Output the loudness
                if (out != NULL)
                    dsp::fill(&out[offset], fLoudness * gain, to_do);

                // Perform metering if we exceed a quarter of a gating block size
                if (nBlockOffset >= nBlockSize)
                {
                    // For each channel push new block to the history buffer
                    // Compute the loudness of the gating block and store to the buffer
                    float loudness      = 0.0f;
                    for (size_t i=0; i<nChannels; ++i)
                    {
                        channel_t *c        = &vChannels[i];
                        const float *blk    = &c->vBlock[0];
                        const float s       = (blk[0] + blk[1] + blk[2] + blk[3]) * fAvgCoeff;

                        loudness           += c->fWeight * s;
                    }
                    vLoudness[nMSHead]      = loudness;
                    nMSHead                 = (nMSHead + 1) % nMSSize;
                    nMSCount                = lsp_min(nMSCount + 1, nMSInt);    // Increment number of blocks for processing

                    // Compute integrated loudness, two-stage
                    // There is no necessity to apply the second stage if the loudness threshold
                    // is less than absolute threshold
                    loudness                = compute_gated_loudness(GATING_ABS_THRESH);
                    const float thresh      = loudness * GATING_REL_THRESH;
                    if (thresh > GATING_ABS_THRESH)
                        loudness                = compute_gated_loudness(thresh);

                    // Convert the loudness for output. Because we use amplitude decibels,
                    // we need to extract square root
                    fLoudness               = sqrtf(loudness);

                    // Reset block size and advance positions
                    nBlockOffset            = 0;
                    nBlockPart              = (nBlockPart + 1) & 0x3;
                    for (size_t i=0; i<nChannels; ++i)
                        vChannels[i].vBlock[nBlockPart]     = 0;
                }

                // Update offset
                offset                 += to_do;
            }
        }

        void ILUFSMeter::update_settings()
        {
            if (nFlags == 0)
                return;

            if (nFlags & F_UPD_TIME)
            {
                // The Integration period consists of one full block and set of overlapping blocks.
                const size_t blk_count  = dspu::millis_to_samples(nSampleRate, fBlockPeriod * 0.25f); // 75% overlapping
                nMSInt      = lsp_max((dspu::seconds_to_samples(nSampleRate, fIntTime) - blk_count*2 - 1) / blk_count, 1);
                nMSCount    = lsp_min(nMSCount, nMSInt);
            }

            if (nFlags & F_UPD_FILTERS)
            {
                dspu::filter_params_t fp;

                fp.nType        = FLT_NONE;
                fp.nSlope       = 0;
                fp.fFreq        = 0.0f;
                fp.fFreq2       = 0.0f;
                fp.fGain        = 1.0f;
                fp.fQuality     = 0.0f;

                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c            = &vChannels[i];

                    c->sBank.begin();
                    switch (enWeight)
                    {
                        case bs::WEIGHT_A:  fp.nType    = dspu::FLT_A_WEIGHTED; break;
                        case bs::WEIGHT_B:  fp.nType    = dspu::FLT_B_WEIGHTED; break;
                        case bs::WEIGHT_C:  fp.nType    = dspu::FLT_C_WEIGHTED; break;
                        case bs::WEIGHT_D:  fp.nType    = dspu::FLT_D_WEIGHTED; break;
                        case bs::WEIGHT_K:  fp.nType    = dspu::FLT_K_WEIGHTED; break;
                        case bs::WEIGHT_NONE:
                        default:
                            break;
                    }

                    c->sFilter.update(nSampleRate, &fp);
                    c->sFilter.rebuild();
                    c->sBank.end(true);
                }
            }

            // Reset flags
            nFlags      = 0;
        }

        void ILUFSMeter::clear()
        {
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                c->sFilter.clear();
                for (size_t i=0; i<4; ++i)
                    c->vBlock[i]            = 0.0f;
            }
            dsp::fill_zero(vLoudness, nMSSize);

            fLoudness               = 0.0f;

            nBlockOffset            = 0;
            nBlockPart              = 0;

            nMSHead                 = 0;
            nMSInt                  = 0;
            nMSCount                = -3;
        }

        void ILUFSMeter::dump(IStateDumper *v) const
        {
            v->begin_array("vChannels", vChannels, nChannels);
            {
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];

                    v->begin_object(c, sizeof(channel_t));
                    {
                        v->write_object("sBank", &c->sBank);
                        v->write_object("sFilter", &c->sFilter);

                        v->write("vIn", c->vIn);
                        v->writev("vBlock", c->vBlock, 4);

                        v->write("fWeight", c->fWeight);
                        v->write("enDesignation", c->enDesignation);

                        v->write("nFlags", c->nFlags);
                    }
                    v->end_object();
                }
            }
            v->end_array();

            v->write("vBuffer", vBuffer);
            v->write("vLoudness", vLoudness);

            v->write("fBlockPeriod", fBlockPeriod);
            v->write("fIntTime", fIntTime);
            v->write("fMaxIntTime", fMaxIntTime);
            v->write("fAvgCoeff", fAvgCoeff);
            v->write("fLoudness", fLoudness);

            v->write("nBlockSize", nBlockSize);
            v->write("nBlockOffset", nBlockOffset);
            v->write("nBlockPart", nBlockPart);
            v->write("nMSSize", nMSSize);
            v->write("nMSHead", nMSHead);
            v->write("nMSInt", nMSInt);
            v->write("nMSCount", nMSCount);

            v->write("nSampleRate", nSampleRate);
            v->write("nChannels", nChannels);
            v->write("nFlags", nFlags);
            v->write("enWeight", enWeight);

            v->write("pData", pData);
            v->write("pVarData", pVarData);
        }

    } /* namespace dspu */
} /* namespace lsp */


